/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2015 Intel Corporation
 */

#include <stdint.h>
#include <stdarg.h>
#include <inttypes.h>
#include <rte_cycles.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <rte_hash_crc.h>
#include <rte_launch.h>
#include <rte_rwlock.h>
#include <rte_ring.h>
#include "rmemc-dpdk.h"
#include "roce_v2.h"
#include "clover_structs.h"
#include "print_helpers.h"
#include "measurement.h"
#include "packet_templates.h"
#include <arpa/inet.h>
#include <rte_malloc.h>

#include <rte_jhash.h>
#include <rte_hash.h>
#include <rte_table.h>
#include <rte_atomic.h>

#include <locale.h>
#include <execinfo.h>
#include <endian.h>


#define KEYSPACE 1024
#define CACHE_KEYSPACE 1024
#define SEQUENCE_NUMBER_SHIFT 256



static int has_mapped_qp = 0;
static int packet_counter = 0;

#define CATCH_ECN

#define WRITE_STEER
//#define READ_STEER
//#define MAP_QP
//#define CNS_TO_WRITE

#define RX_CORES 4

#define HASHSPACE (1 << 24) // THIS ONE WORKS DONT FUCK WITH IT TOO MUCH
uint64_t cached_write_vaddr_mod[HASHSPACE];
uint64_t cached_write_vaddr_mod_lookup[HASHSPACE];
uint64_t cached_write_vaddr_mod_latest[KEYSPACE];


#define IPV4_OFFSET 14
#define UDP_OFFSET 34
#define ROCE_OFFSET 42
#define CLOVER_OFFSET 54

#define INPUT_OUTPUT_EXAMPLES

#define MEMPOOLS 14
const char mpool_names[MEMPOOLS][10] = { "MEMPOOL0", "MEMPOOL1", "MEMPOOL2", "MEMPOOL3", "MEMPOOL4", "MEMPOOL5", "MEMPOOL6", "MEMPOOL7", "MEMPOOL8", "MEMPOOL9", "MEMPOOL10", "MEMPOOL11" };
const char txq_names[MEMPOOLS][10]={"TXQ1","TXQ2","TXQ3","TXQ4","TXQ5","TXQ16","TXQ7","TXQ8","TXQ9","TXQ10", "TXQ11", "TXQ12", "TXQ13", "TXQ14"};
struct rte_mempool *mbuf_pool;
struct rte_ring *tx_queues[MEMPOOLS];

inline struct rte_ether_hdr *get_eth_hdr(struct rte_mbuf *pkt)
{
	return rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
}

inline struct rte_ipv4_hdr *get_ipv4_hdr(struct rte_mbuf *pkt)
{
	return (struct rte_ipv4_hdr *) ((uint8_t*)get_eth_hdr(pkt) + IPV4_OFFSET);
}

inline struct rte_udp_hdr *get_udp_hdr(struct rte_mbuf *pkt)
{
	return (struct rte_udp_hdr *)((uint8_t*)get_eth_hdr(pkt) + UDP_OFFSET);
}

inline struct roce_v2_header *get_roce_hdr(struct rte_mbuf *pkt)
{
	return (struct roce_v2_header *)((uint8_t*)get_eth_hdr(pkt) + ROCE_OFFSET);
}

inline struct clover_hdr *get_clover_hdr(struct rte_mbuf *pkt)
{
	return (struct clover_hdr *)((uint8_t*)get_eth_hdr(pkt) + CLOVER_OFFSET);
}

#define ID_SPACE 1<<24 //THIS IS MUCH SAFER YOU FOOL
//#define ID_SPACE 1<<16
int32_t fast_id_lookup[ID_SPACE];
int32_t fast_qp_lookup[ID_SPACE];

inline uint32_t qp_id_hash(uint32_t qp) {
	return (uint32_t) murmur3(qp) % (ID_SPACE);
}

void set_fast_id(uint32_t qp, uint32_t id) {
	if(fast_id_lookup[qp_id_hash(qp)] != -1){
		printf("curses I've hit a collision in the ID space");
		exit(0);
	}
	fast_id_lookup[qp_id_hash(qp)] = id;
}

inline int fast_find_id_qp(uint32_t qp) {
	return fast_id_lookup[qp_id_hash(qp)];
}

inline int fast_find_id(struct rte_mbuf * buf) {
	struct roce_v2_header * rh = get_roce_hdr(buf);
	return fast_find_id_qp(rh->dest_qp);
}

void init_fast_find_id(void) {
	for (int i=0;i<(ID_SPACE);i++){
		fast_id_lookup[i]=-1;
	}
}

void check_id_mapping(void) {
	for (int i=0;i<ID_SPACE;i++) {
		if(fast_id_lookup[i] != -1) {
			printf("index[%d]=%d qp %d\n",i,fast_id_lookup[i],fast_qp_lookup[i]);
		}
	}
}

rte_rwlock_t next_lock;
rte_rwlock_t qp_lock;
rte_rwlock_t qp_mapping_lock;

static inline void lock_qp(void) {
	rte_rwlock_write_lock(&qp_lock);
}

static inline void unlock_qp(void) {
	rte_rwlock_write_unlock(&qp_lock);
}


static inline void lock_write_steering(void) {
	rte_rwlock_write_lock(&next_lock);
}

static inline void unlock_write_steering(void) {
	rte_rwlock_write_unlock(&next_lock);
}

struct Connection_State Connection_States[TOTAL_ENTRY];

void print_connection_state_status(void) {
	struct Connection_State *cs;
	for (int i=0;i<TOTAL_CLIENTS;i++) {
		cs=&Connection_States[i];
		printf("id %3d SI=%d RI=%d seq %5d mseq %5d s-qpid %4d (%7d) r-qpid %4d (%7d)\n",cs->id,cs->sender_init,cs->receiver_init,readable_seq(cs->seq_current),readable_seq(cs->mseq_current),fast_find_id_qp(cs->ctsqp),cs->ctsqp, fast_find_id_qp(cs->stcqp), cs->stcqp);
	}
} 

inline void lock_connection_state(struct Connection_State *cs) {
	//just to remove compiler errors
	cs->id = cs->id;
	//printf("[core %d] try lock cs %d\n",rte_lcore_id(),cs->id);
	rte_rwlock_write_lock(&(cs->cs_lock));
	//printf("[core %d] locked cs %d\n",rte_lcore_id(),cs->id);
	//rte_smp_mb();
}

inline void unlock_connection_state(struct Connection_State *cs) {
	//printf("unlock %d\n",cs->id);
	cs->id = cs->id;
	//printf("[core %d] try unlock cs %d\n",rte_lcore_id(),cs->id);
	rte_rwlock_write_unlock(&(cs->cs_lock));
	//printf("[core %d] unlock cs %d\n",rte_lcore_id(),cs->id);
	//rte_smp_mb();
}

rte_atomic16_t atomic_qp_id_counter;
uint32_t id_qp[TOTAL_ENTRY];

//Keys start at 1, so I'm subtracting 1 to make the first key equal to index
//zero.  qp_id_counter is the total number of qp that can be written to. So here
//we are just taking all of the keys and wrapping them around so the first key
//goes to the first qp, and the qp_id_counter + 1  key goes to the first qp.
inline uint32_t key_to_qp(uint64_t key)
{
	int qp = TOTAL_CLIENTS;
	uint32_t index = (key) % qp;
	return id_qp[index];
}

inline uint32_t id_to_qp(uint32_t id) {
	int qp = 2;
	uint32_t index = (id) % qp;
	return id_qp[index];
}


//Warning this is a very unsafe function. Only call it when you know that a
//packet corresponds to an ID that has an established QP. If the ID is not set,
//this will set it. Otherwise the ID is returned.
uint32_t get_or_create_id(uint32_t qp)
{
	//first try to go fast
	uint32_t id = fast_find_id_qp(qp);
	if ((int32_t)id != -1) {
		if(unlikely(fast_qp_lookup[qp_id_hash(qp)] != (int32_t)qp)) {
			printf("QP COLLISION -- Delete this code eventually\n");
			exit(0);
		}
		return id;
	}

	lock_qp();
	int16_t new_id = rte_atomic16_add_return(&atomic_qp_id_counter,1);
	id = (int32_t)new_id;

	set_fast_id(qp,id);
	fast_qp_lookup[qp_id_hash(qp)]=qp;

	//Set globals
	id_qp[id] = qp;
	unlock_qp();

	return id;
}

int fully_qp_init(void)
{
	for (int i = 0; i < TOTAL_CLIENTS; i++)
	{
		struct Connection_State * cs = &Connection_States[i];
		if (!(cs->sender_init && cs->receiver_init))
		{
			return 0;
		}
	}
	return 1;
}

struct rte_mbuf ***mem_qp_buf;
struct rte_mbuf ***client_qp_buf;
struct rte_mbuf ***ect_qp_buf;

uint64_t **mem_qp_timestamp;
uint64_t **client_qp_timestamp;
uint64_t **ect_qp_timestamp;

uint64_t mem_qp_buf_head[TOTAL_ENTRY];
uint64_t mem_qp_buf_tail[TOTAL_ENTRY];

uint64_t client_qp_buf_head[TOTAL_ENTRY];
uint64_t client_qp_buf_tail[TOTAL_ENTRY];

uint64_t ect_qp_buf_head[TOTAL_ENTRY];
uint64_t ect_qp_buf_tail[TOTAL_ENTRY];

uint8_t mem_qp_dequeuable[TOTAL_ENTRY];
uint8_t client_qp_dequeuable[TOTAL_ENTRY];
uint8_t ect_qp_dequeuable[TOTAL_ENTRY];

struct Buffer_State mem_buffer_states[TOTAL_ENTRY];
struct Buffer_State client_buffer_states[TOTAL_ENTRY];
struct Buffer_State ect_buffer_states[TOTAL_ENTRY];

struct rte_mbuf *** dynamicly_allocate_buffer_state_pkt_buf(void) {
	struct rte_mbuf*** buf = (struct rte_mbuf ***)rte_malloc(NULL, sizeof(struct rte_mbuf **) * TOTAL_ENTRY,0);
	for (int i=0;i<TOTAL_ENTRY;i++) {
		buf[i] = (struct rte_mbuf **)rte_malloc(NULL, sizeof(struct rte_mbuf*) *PKT_REORDER_BUF,0);
	}
	return buf;
}

uint64_t ** dynamicly_allocate_pkt_timestamps(void) {
	uint64_t ** buf = (uint64_t **) rte_malloc(NULL, sizeof(uint64_t *) * TOTAL_ENTRY,0);
	for (int i=0;i<TOTAL_ENTRY;i++) {
		buf[i] = (uint64_t *)rte_malloc(NULL, sizeof(uint64_t) *PKT_REORDER_BUF,0);
	}
	return buf;
}

void init_buffer_states(void) {
	for(int i=0;i<TOTAL_ENTRY;i++) {
		//ect
		struct Buffer_State * bs = &ect_buffer_states[i];
		bs->id = i;
		bs->dequeable = &(ect_qp_dequeuable[i]);
		bs->head = &(ect_qp_buf_head[i]);
		bs->tail = &(ect_qp_buf_tail[i]);
		bs->buf = &(ect_qp_buf[i]);
		bs->timestamps = &(ect_qp_timestamp[i]);
		rte_rwlock_init(&(bs->bs_lock));

		//client
		bs = &client_buffer_states[i];
		bs->id = i;
		bs->dequeable = &(client_qp_dequeuable[i]);
		bs->head = &client_qp_buf_head[i];
		bs->tail = &client_qp_buf_tail[i];
		bs->buf = &client_qp_buf[i];
		bs->timestamps = &client_qp_timestamp[i];
		rte_rwlock_init(&(bs->bs_lock));

		//memory
		bs = &mem_buffer_states[i];
		bs->id = i;
		bs->dequeable = &(mem_qp_dequeuable[i]);
		bs->head = &(mem_qp_buf_head[i]);
		bs->tail = &(mem_qp_buf_tail[i]);
		bs->buf = &(mem_qp_buf[i]);
		bs->timestamps = &(mem_qp_timestamp[i]);
		rte_rwlock_init(&(bs->bs_lock));

	}

	//TODO I'm doing a hacking init of the general buf, this is so that I don't have to do any checks during enqueue (Stewart Grant Nov 18 2021)
	//TODO there is probably a nice way to do this
	struct Buffer_State * bs = &ect_buffer_states[0];
	*(bs->head) = 1;
}

void init_reorder_buf(void)
{
	mem_qp_buf=dynamicly_allocate_buffer_state_pkt_buf();
	client_qp_buf=dynamicly_allocate_buffer_state_pkt_buf();
	ect_qp_buf=dynamicly_allocate_buffer_state_pkt_buf();

	ect_qp_timestamp=dynamicly_allocate_pkt_timestamps();
	client_qp_timestamp=dynamicly_allocate_pkt_timestamps();
	mem_qp_timestamp=dynamicly_allocate_pkt_timestamps();



	bzero(mem_qp_buf_head, TOTAL_ENTRY * sizeof(uint64_t));
	bzero(mem_qp_buf_tail, TOTAL_ENTRY * sizeof(uint64_t));
	bzero(client_qp_buf_head, TOTAL_ENTRY * sizeof(uint64_t));
	bzero(client_qp_buf_tail, TOTAL_ENTRY * sizeof(uint64_t));
	bzero(ect_qp_buf_head, TOTAL_ENTRY * sizeof(uint64_t));
	bzero(ect_qp_buf_tail, TOTAL_ENTRY * sizeof(uint64_t));

	bzero(mem_qp_dequeuable, TOTAL_ENTRY * sizeof(uint8_t));
	bzero(client_qp_dequeuable, TOTAL_ENTRY * sizeof(uint8_t));
	bzero(ect_qp_dequeuable, TOTAL_ENTRY * sizeof(uint8_t));

	bzero(client_buffer_states, TOTAL_ENTRY * sizeof(struct Buffer_State));
	bzero(ect_buffer_states, TOTAL_ENTRY * sizeof(struct Buffer_State));
	bzero(mem_buffer_states, TOTAL_ENTRY * sizeof(struct Buffer_State));

	for (int i = 0; i < TOTAL_ENTRY; i++)
	{
		for (int j = 0; j < PKT_REORDER_BUF; j++)
		{
			mem_qp_buf[i][j] = NULL;
			client_qp_buf[i][j] = NULL;
			ect_qp_buf[i][j] = NULL;


			mem_qp_timestamp[i][j] = 0;
			client_qp_timestamp[i][j] = 0;
			ect_qp_timestamp[i][j] = 0;
		}
	}
	init_buffer_states();
}

struct Buffer_State * get_buffer_state(struct rte_mbuf *pkt) {
	int id = fast_find_id(pkt);
	if (unlikely(has_mapped_qp == 0) || id == -1) {
		//printf("etc\n");
		return &ect_buffer_states[0];
	}

	struct roce_v2_header *roce_hdr = get_roce_hdr(pkt);
	if (Connection_States[id].ctsqp == roce_hdr->dest_qp)
	{
		//printf("mem\n");
		return &mem_buffer_states[id];
	}
	if (Connection_States[id].stcqp == roce_hdr->dest_qp)
	{
		//printf("clt\n");
		return &client_buffer_states[id];
	}
	printf("something is weird here unable to get the buffer state\n");
	exit(0);
}

int32_t count_held_packets(void) {

	printf(" (NOT) COUNTING HELD PACKETS!!\n");

	/*
	for(int i=0;i<TOTAL_CLIENTS;i++){
		int32_t total=0;
		struct Buffer_State* bs;
		bs=&ect_buffer_states[i];
		//total = *(bs->tail) - *(bs->head);
		total = rte_atomic64_read(bs->tail) - rte_atomic64_read(bs->head);
		if (total > 0) {
			printf("ect ID %d HELD %d head %d tail %d\n",i, rte_atomic64_read(bs->tail) - rte_atomic64_read(bs->head), rte_atomic64_read(bs->head), rte_atomic64_read(bs->tail));
		}
		bs=&client_buffer_states[i];
		total = rte_atomic64_read(bs->tail) - rte_atomic64_read(bs->head);
		if (total > 0) {
			printf("client ID %d HELD %d head %d tail %d\n",i, rte_atomic64_read(bs->tail) - rte_atomic64_read(bs->head), rte_atomic64_read(bs->head), rte_atomic64_read(bs->tail));
		}
		bs=&mem_buffer_states[i];
		total = rte_atomic64_read(bs->tail) - rte_atomic64_read(bs->head);
		if (total > 0) {
			printf("mem ID %d HELD %d head %d tail %d\n",i, rte_atomic64_read(bs->tail) - rte_atomic64_read(bs->head), rte_atomic64_read(bs->head), rte_atomic64_read(bs->tail));
		}
	}
	*/
	printf("\\COUNTING HELD PACKETS!!\n");
	return 0;
}

void print_mpr(struct map_packet_response* mpr) {
	printf("MPR size: %d\n",mpr->size);
	for (uint32_t i=0;i<mpr->size;i++) {
		print_packet_lite(mpr->pkts[i]);
	}
}

#define DEQUEUE_BURST 16
//#define DEQUEUE_BURST 32
void general_tx_enqueue(struct rte_mbuf * pkt) {
	uint32_t queue_index = RX_CORES;
	if (has_mapped_qp) {
		uint32_t id = fast_find_id(pkt);
		queue_index = (id % (rte_lcore_count()-RX_CORES)) + RX_CORES;
	}
	//printf("sending to queue %d\n",queue_index);
	rte_ring_enqueue(tx_queues[queue_index],pkt);
}

void general_tx_eternal(uint16_t port, uint32_t queue, struct rte_ring * in_queue) {
	for (;;) {
		general_tx(port,queue,in_queue);
	}
}

void general_tx(uint16_t port, uint32_t queue, struct rte_ring * in_queue) {
	struct rte_mbuf * tx_pkts[DEQUEUE_BURST];
	uint32_t left;
	uint16_t dequeued = rte_ring_dequeue_burst(in_queue,(void **)tx_pkts,DEQUEUE_BURST,&left);



	//Here the buffer states have been collected
	uint64_t seq;
	struct Buffer_State *bs;
	struct rte_mbuf *pkt;


	for (uint32_t i=0;i<dequeued;i++) {
		pkt = tx_pkts[i];
		//printf("txing on core %d\n",rte_lcore_id());
		//print_packet_lite(pkt);
		bs = get_buffer_state(pkt);
		seq = readable_seq(get_psn(pkt));

		if (bs->buf == &ect_qp_buf[bs->id])
		{
			seq = *(bs->tail) + 1;
		}
		struct map_packet_response mpr;
		mpr.size=0;

		uint32_t entry = seq % PKT_REORDER_BUF;
		(*bs->buf)[entry] = pkt;
		(*bs->tail)++;

		while (((*bs->head) <= (*bs->tail)) && ((*bs->buf)[(*bs->head) % PKT_REORDER_BUF] != NULL))
		{
			mpr.pkts[mpr.size]=(*bs->buf)[(*bs->head) % PKT_REORDER_BUF];
			mpr.size++;
			(*bs->buf)[*(bs->head) % PKT_REORDER_BUF] = NULL;
			*(bs->head) = (*bs->head) + 1;
		}

		for (uint j=0;j<mpr.size;j++) {
			if (likely(j < mpr.size - 1))
			{
				rte_prefetch0(rte_pktmbuf_mtod(mpr.pkts[j + 1], void *));
			}
			if(packet_is_marked(mpr.pkts[j])) {
				recalculate_rdma_checksum(mpr.pkts[j]);
			} 
		}
		rte_eth_tx_burst(port, queue, (struct rte_mbuf **)mpr.pkts, mpr.size);
	}
}

void flush_buffers(uint16_t port) {
	//printf("&& FLUSHING BUFFERS\n");
	for(int i=0;i<MEMPOOLS;i++) {
		general_tx(port,0,tx_queues[i]);
	}

	//printf("&& FLUSHING BUFFERS COMPLETE\n");

	for (int i=0;i<TOTAL_CLIENTS;i++) {
		struct Connection_State *cs = &Connection_States[i];
		struct Buffer_State *bs = &mem_buffer_states[i];
		(*bs->head) = readable_seq(cs->seq_current) + 1;
		(*bs->tail) = readable_seq(cs->seq_current);
		
		bs = &client_buffer_states[i];
		int rec_seq = readable_seq(cs->mseq_current) + readable_seq(cs->mseq_offset);
		(*bs->head) = rec_seq + 1;
		(*bs->tail) = rec_seq;
	}
}

void copy_eth_addr(uint8_t *src, uint8_t *dst)
{
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
		dst[3] = src[3];
		dst[4] = src[4];
		dst[5] = src[5];
}

void init_connection_state(struct rte_mbuf *pkt)
{
	struct rte_ether_hdr *eth_hdr = get_eth_hdr(pkt);
	struct rte_ipv4_hdr *ipv4_hdr = get_ipv4_hdr(pkt);
	struct rte_udp_hdr *udp_hdr = get_udp_hdr(pkt);
	struct roce_v2_header *roce_hdr = get_roce_hdr(pkt);

	//Find the connection state if it exists
	struct Connection_State *cs;
	int id = get_or_create_id(roce_hdr->dest_qp);
	cs = &Connection_States[id];

	lock_connection_state(cs);

	//Connection State allready initalized for this qp
	if (cs->sender_init != 0)
	{
		unlock_connection_state(cs);
		return;
	}

	cs->id = id;
	cs->seq_current = roce_hdr->packet_sequence_number;
	cs->udp_src_port_client = udp_hdr->src_port;
	cs->ctsqp = roce_hdr->dest_qp;
	cs->cts_rkey = get_rkey_rdma_packet(roce_hdr);
	cs->ip_addr_client = ipv4_hdr->src_addr;
	cs->ip_addr_server = ipv4_hdr->dst_addr;

	copy_eth_addr((uint8_t *)eth_hdr->s_addr.addr_bytes, (uint8_t *)cs->cts_eth_addr);
	copy_eth_addr((uint8_t *)eth_hdr->d_addr.addr_bytes, (uint8_t *)cs->stc_eth_addr);
	cs->sender_init = 1;
	//printf("init sender Connection State %d\n",id);

	unlock_connection_state(cs);
}

void init_cs_wrapper(struct rte_mbuf *pkt)
{
	struct roce_v2_header *roce_hdr = get_roce_hdr(pkt);
	switch (roce_hdr->opcode)
	{
	case RC_WRITE_ONLY:
	case RC_READ_REQUEST:
	case RC_CNS:
		init_connection_state(pkt);
		break;
	default:
		printf("only init connection states for writers either WRITE_ONLY or CNS (and only for data path) exiting for safty");
		exit(0);
	}
}

//Calculate the MSN of the packet based on the offset. If the msn of the
//connection state is behind, update that as well.
uint32_t produce_and_update_msn(struct roce_v2_header *roce_hdr, struct Connection_State *cs)
{
	lock_connection_state(cs);
	uint32_t msn = htonl(ntohl(roce_hdr->packet_sequence_number) - ntohl(cs->mseq_offset));
	if (ntohl(msn) > ntohl(cs->mseq_current))
	{
		cs->mseq_current = msn;
	}
	msn = cs->mseq_current;
	unlock_connection_state(cs);
	return msn;
}

//Calculate the MSN of the packet based on the offset. If the msn of the
//connection state is behind, update that as well.
uint32_t produce_and_update_msn_lockless(struct roce_v2_header *roce_hdr, struct Connection_State *cs)
{
	uint32_t msn = htonl(ntohl(roce_hdr->packet_sequence_number) - ntohl(cs->mseq_offset));
	if (ntohl(msn) > ntohl(cs->mseq_current))
	{
		cs->mseq_current = msn;
	}
	msn = cs->mseq_current;
	return msn;
}

//Update the server to clinet connection state based on roce and udp header.
//Search for the connection state first. If it's not found, just return.
//Otherwise update the MSN.
uint32_t find_and_update_stc(struct roce_v2_header *roce_hdr)
{
	struct Connection_State *cs;

	int32_t id = fast_find_id_qp(roce_hdr->dest_qp);
	if (id == -1) {
		return 0;
	}
	cs = &Connection_States[id];

	//at this point we have the correct cs
	return produce_and_update_msn(roce_hdr, cs);
}

void init_stc(struct rte_mbuf * pkt)
{
	struct Connection_State *cs;
	int32_t matching_id = -1;
	uint32_t total_matches = 0;

	struct roce_v2_header *roce_hdr = get_roce_hdr(pkt);
	struct rte_udp_hdr *udp_hdr = get_udp_hdr(pkt);

	//Everything has been initlaized, return
	if (likely(has_mapped_qp != 0))
	{
		printf("mapped\n");
		return;
	}

    //Try to perform a basic update
    if (find_and_update_stc(roce_hdr) > 0)
    {
		cs =&Connection_States[fast_find_id_qp(roce_hdr->dest_qp)];
		if (cs->receiver_init == 0) {
			printf("I think that this is a sign there has been a qp collisiion ID_SPACE could be increased in size ID_SPACE=%d\n",ID_SPACE);
			exit(0);
		}
        return;
    }

	if (roce_hdr->opcode != RC_ATOMIC_ACK)
	{
		printf("Only find and set stc on atomic ACKS");
		return;
	}

	for (int i = 0; i < TOTAL_CLIENTS; i++)
	{
		cs = &Connection_States[i];
		lock_connection_state(cs);
		if (cs->sender_init != 0 && cs->last_atomic_seq == roce_hdr->packet_sequence_number)
		{
			matching_id = i;
			total_matches++;
		}
		unlock_connection_state(cs);
	}

	//Return if nothing is found
	if (total_matches != 1)
	{
		return;
	}

	//initalize the first time
	cs = &Connection_States[matching_id];

	if (cs->receiver_init == 1 && cs->sender_init == 1) {
		//If the qp allready has an entry	
		if(fast_find_id_qp(cs->stcqp) != -1) {
			printf("failed both receiver init and not in the fast find id qp\n");
			exit(0);
			return;
		}
	}

	lock_connection_state(cs);
	if (cs->receiver_init == 0 && cs->sender_init !=0)
	{
		cs->stcqp = roce_hdr->dest_qp;
		cs->udp_src_port_server = udp_hdr->src_port;
		set_fast_id(roce_hdr->dest_qp,cs->id);
		fast_qp_lookup[qp_id_hash(roce_hdr->dest_qp)]=roce_hdr->dest_qp;
		cs->mseq_current = get_msn(roce_hdr);
		cs->mseq_offset = htonl(3184 << 8);
		cs->receiver_init = 1;
		//printf("**Client Thread %3d Fully Initalized**\n",cs->id);
	}
	unlock_connection_state(cs);
	return;
}

void update_cs_seq(struct rte_mbuf * pkt) {
	struct roce_v2_header *roce_hdr = get_roce_hdr(pkt);
	if (roce_hdr->opcode != RC_WRITE_ONLY && roce_hdr->opcode != RC_CNS && roce_hdr->opcode != RC_READ_REQUEST)
	{
		printf("only update connection states for writers either WRITE_ONLY or CNS (and only for data path) exiting for safty");
		exit(0);
	}

	uint32_t id = get_or_create_id(roce_hdr->dest_qp);
	struct Connection_State *cs = &Connection_States[id];

	lock_connection_state(cs);
	cs->seq_current = roce_hdr->packet_sequence_number;

	if (roce_hdr->opcode == RC_CNS) {
		cs->last_atomic_seq = roce_hdr->packet_sequence_number;
	}
	unlock_connection_state(cs);

}

void cts_track_connection_state(struct rte_mbuf *pkt)
{
	if (has_mapped_qp != 0)
	{
		return;
	}
	init_cs_wrapper(pkt);
	update_cs_seq(pkt);
}

static uint64_t first_write[KEYSPACE];
static uint64_t first_cns[KEYSPACE];
static uint64_t predict_address[KEYSPACE];
static uint64_t latest_cns_key[KEYSPACE];

static uint64_t outstanding_write_predicts[TOTAL_ENTRY][KEYSPACE]; //outstanding write index, contains precited addresses
static uint64_t outstanding_write_vaddrs[TOTAL_ENTRY][KEYSPACE];   //outstanding vaddr values, used for replacing addrs
static uint64_t next_vaddr[KEYSPACE];
static uint64_t latest_key[TOTAL_ENTRY];


inline uint64_t get_latest_key(uint32_t id)
{
	return latest_key[id];
}

inline void set_latest_key(uint32_t id, uint64_t key)
{
	latest_key[id] = key;
}

static int init = 0;
static uint32_t write_value_packet_size = 0;
static uint32_t predict_shift_value = 0;
static uint32_t nacked_cns = 0;

void init_connection_states(void)
{
	bzero(Connection_States, TOTAL_ENTRY * sizeof(Connection_State));
	for (int i = 0; i < TOTAL_ENTRY; i++)
	{
		struct Connection_State *source_connection;
		source_connection = &Connection_States[i];
		rte_rwlock_init(&source_connection->cs_lock);
		for (int j = 0; j < CS_SLOTS; j++)
		{
			struct Request_Map *mapped_request;
			mapped_request = &(source_connection->Outstanding_Requests[j]);
			mapped_request->open = 1;
		}
	}
}

//I'm using a bit mixer instead of a hash
//hashes are slow as AF
//http://jonkagstrom.com/bit-mixer-construction/
inline uint64_t murmur3(uint64_t k) {
  k ^= k >> 33;
  k *= 0xff51afd7ed558ccdull;
  k ^= k >> 33;
  k *= 0xc4ceb9fe1a85ec53ull;
  k ^= k >> 33;
  return k;
}

inline uint32_t mod_hash(uint64_t vaddr)
{
	return (uint32_t)murmur3(vaddr) % HASHSPACE;
}

inline void update_read_tail(uint64_t key, uint64_t vaddr) {
	cached_write_vaddr_mod_latest[key] = vaddr;
}

inline void update_write_vaddr_cache(uint64_t key, uint64_t vaddr)
{
	uint32_t index = mod_hash(vaddr);
	cached_write_vaddr_mod[index] = vaddr;
	cached_write_vaddr_mod_lookup[index] = key;
}


inline int does_read_have_cached_write(uint64_t vaddr)
{
	uint32_t index = mod_hash(vaddr);
	if (likely(cached_write_vaddr_mod[index] == vaddr))
	{
		return cached_write_vaddr_mod_lookup[index];
	} 
	return 0;
}

inline void print_cache_population(void) {
	int populated = 0;
	for(int i=0;i<HASHSPACE;i++) {
		if (cached_write_vaddr_mod[i] != 0) {
			populated++;
		}
	}
	printf("CACHE SATURATION %d/%d\n",populated,HASHSPACE);
}


inline uint64_t get_latest_vaddr(uint32_t key)
{
	return cached_write_vaddr_mod_latest[key];
}

void steer_read(struct rte_mbuf *pkt, uint32_t key)
{
	struct clover_hdr *clover_header = get_clover_hdr(pkt);
	struct read_request *rr = (struct read_request *)clover_header;

	uint64_t vaddr = get_latest_vaddr(key);
	if (unlikely(vaddr == 0))
	{
		return;
	}
	//Set the current address to the cached address of the latest write if it is old
	if (rr->rdma_extended_header.vaddr != vaddr)
	{
		rr->rdma_extended_header.vaddr = vaddr;
		#ifdef MAP_QP
		mark_pkt_rdma_checksum(pkt);
		#else
		recalculate_rdma_checksum(pkt);
		#endif
		//recalculate_rdma_checksum(pkt);
#ifdef TAKE_MEASUREMENTS
		read_redirected();
#endif
	}
}

int get_key(struct rte_mbuf *pkt) {
	struct roce_v2_header *roce_hdr = get_roce_hdr(pkt);
	struct rte_ipv4_hdr *ipv4_hdr = get_ipv4_hdr(pkt);
	struct clover_hdr *clover_header = get_clover_hdr(pkt);
	uint32_t size = ntohs(ipv4_hdr->total_length);
	int32_t id = fast_find_id(pkt);

	if(id == -1) {
		return -1;
	}

	if (roce_hdr->opcode == RC_WRITE_ONLY && size == 1084) {

		struct write_request *wr = (struct write_request *)clover_header;
		uint64_t *key = (uint64_t *)&(wr->data);
		return (int)*key;
	} else if (roce_hdr->opcode == RC_ATOMIC_ACK || roce_hdr->opcode == RC_CNS) {
		//!TODO this can cause mapped pacekts to print out the wrong id. Ie we
		//!TODO don't know the key from the packet after mapping
		return (int)get_latest_key(id);
	} else if (roce_hdr->opcode == RC_READ_REQUEST) {
		struct read_request *rr = (struct read_request *)clover_header;
		int key = (*does_read_have_cached_write)(rr->rdma_extended_header.vaddr);
		if (key != 0)
		{
			return key;
		} 
	} else if (roce_hdr->opcode == RC_READ_RESPONSE && size == 1072) {
		struct read_response *resp = (struct read_response *)clover_header;
		uint64_t *key = (uint64_t *)&(resp->data);
		return (int)*key;
	} else {
		return -1;
	}
	return -1;
}

inline uint32_t slot_is_open(struct Request_Map *rm)
{
	return rm->open;
}

inline void close_slot(struct Request_Map *rm)
{
	rm->open = 0;
}

inline void open_slot(struct Request_Map *rm)
{
	rm->open = 1;
}

inline uint32_t mod_slot_minus_one(uint32_t seq)
{
	return (readable_seq(seq) - 1) % CS_SLOTS;
}

inline uint32_t mod_slot(uint32_t seq)
{
	return readable_seq(seq) % CS_SLOTS;
}

inline uint32_t qp_is_mapped(uint32_t qp)
{
	if(fast_find_id_qp(qp) == -1) {
		return 0;
	}
	return 1;
}

inline struct Request_Map *get_empty_slot_mod(struct Connection_State *cs)
{
	return &(cs->Outstanding_Requests[mod_slot(cs->seq_current)]);
}


#define CNS_TO_WRITE_SIZE_DIFF 4
#define CNS_TO_WRITE_DMA_LENGTH 8
#define CNS_TO_WRITE_DGRAM_LEN 0x3000
#define CNS_TO_WRITE_IPV4_LEN 0x4400

void map_cns_to_write(struct rte_mbuf *pkt, struct Request_Map *slot) {
	struct roce_v2_header *roce_hdr = get_roce_hdr(pkt);
	if (roce_hdr->opcode == RC_CNS) {
		struct rte_ipv4_hdr *ipv4_hdr = get_ipv4_hdr(pkt);
		struct rte_udp_hdr *udp_hdr = get_udp_hdr(pkt);

		struct clover_hdr *clover_header = get_clover_hdr(pkt);
		struct cs_request *cs = (struct cs_request *)clover_header;
		struct write_request *wr = (struct write_request *)clover_header;

		slot->was_write_swapped=1;

		//set the opcode to write
		roce_hdr->opcode = RC_WRITE_ONLY;
		wr->rdma_extended_header.dma_length = CNS_TO_WRITE_DMA_LENGTH;
		wr->ptr = cs->atomic_req.swap_or_add;

		//Header change now we need to adjust the length (should be -4)
		rte_pktmbuf_trim(pkt,CNS_TO_WRITE_SIZE_DIFF);
		udp_hdr->dgram_len = CNS_TO_WRITE_DGRAM_LEN;
		ipv4_hdr->total_length = CNS_TO_WRITE_IPV4_LEN;
	}
}

#define ACK_TO_ATOMIC_SIZE_DIFF 8 // This is a magic number because the line before this (cs - write) gives 3
#define ORIGINAL_DATA_ACCEPT 0x8000000000000
#define ATOMIC_ACK_DGRAM_LEN 0x2400
#define ATOMIC_ACK_IPV4_LEN 0x3800

void map_write_ack_to_atomic_ack(struct rte_mbuf *pkt, struct Request_Map *slot) {

	struct roce_v2_header *roce_hdr = get_roce_hdr(pkt);
	if (slot->rdma_op == RC_CNS && slot->was_write_swapped==1) {
		struct rte_ipv4_hdr *ipv4_hdr = get_ipv4_hdr(pkt);
		struct rte_udp_hdr *udp_hdr = get_udp_hdr(pkt);
		struct cs_response *cs = (struct cs_response *)get_clover_hdr(pkt);

		//Set the headers
		//First extend the size of the packt buf
		rte_pktmbuf_append(pkt,ACK_TO_ATOMIC_SIZE_DIFF);
		roce_hdr->opcode = RC_ATOMIC_ACK;
		cs->atomc_ack_extended.original_remote_data = ORIGINAL_DATA_ACCEPT;
		udp_hdr->dgram_len = ATOMIC_ACK_DGRAM_LEN;
		ipv4_hdr->total_length = ATOMIC_ACK_IPV4_LEN;
	} 
	
}

inline uint32_t qp_mapping_default(uint32_t key, struct roce_v2_header * roce_hdr) {
	//Key to qp policy
	//Keys are set to 0 when we are not going to map them. If the key is not
	//equal to zero apply the mapping policy.
	if (likely(key != 0)) {
		return key_to_qp(key);
	} else {
		return roce_hdr->dest_qp;
	}
}

void map_qp_forward(struct rte_mbuf *pkt, uint64_t key)
{

	struct roce_v2_header *roce_hdr = get_roce_hdr(pkt);
	uint32_t id = fast_find_id(pkt);
	uint32_t n_qp = qp_mapping_default(key,roce_hdr);

	//Find the connection state of the mapped destination connection.
	struct Connection_State *destination_connection = &Connection_States[fast_find_id_qp(n_qp)];

	//Our middle box needs to keep track of the sequence number
	//that should be tracked for the destination. This should
	//always be an increment of 1 from it's previous number.
	//Here we increment the sequence number
	lock_connection_state(destination_connection);
	destination_connection->seq_current = htonl(ntohl(destination_connection->seq_current) + SEQUENCE_NUMBER_SHIFT); //There is bit shifting here.

#ifdef MAP_PRINT
	uint32_t msn = Connection_States[id].mseq_current;
	printf("MAP FRWD(key %" PRIu64 ") (id %d) (core %d) (op: %s) :: (%d -> %d) (%d) (qpo %d -> qpn %d) \n", key, id, rte_lcore_id(), ib_print_op(roce_hdr->opcode), readable_seq(roce_hdr->packet_sequence_number), readable_seq(destination_connection->seq_current), readable_seq(msn), roce_hdr->dest_qp, destination_connection->ctsqp);
#endif

	//Multiple reads occur at once. We don't want them to overwrite eachother.
	//Initally I used Oustanding_Requests[id] to hold requests. But this only
	//ever used the one index. On parallel reads the sequence number over write
	//the old one because they collied.  To save time, I'm using the old extra
	//space TOTAL_ENTRIES in the Outstanding Requests to hold the concurrent
	//reads. These should really be hased in the future.

	//Search for an open slot
	struct Request_Map *slot = get_empty_slot_mod(destination_connection);

	//The next step is to save the data from the current packet
	//to a list of outstanding requests. As clover is blocking
	//we can use the id to track which sequence number belongs
	//to which request. These values will be used to map
	//responses back.

	//Save a unique id of sequence number and the qp that this response will arrive back on
	close_slot(slot);
	slot->id = id;
	slot->mapped_sequence = destination_connection->seq_current;
	slot->original_sequence = roce_hdr->packet_sequence_number;
	slot->rdma_op=roce_hdr->opcode;
	slot->was_write_swapped=0;

	//Set the packet with the mapped information to the new qp
	struct rte_ipv4_hdr *ipv4_hdr = get_ipv4_hdr(pkt);
	struct rte_udp_hdr *udp_hdr = get_udp_hdr(pkt);
	roce_hdr->dest_qp = destination_connection->ctsqp;
	roce_hdr->packet_sequence_number = destination_connection->seq_current;
	udp_hdr->src_port = destination_connection->udp_src_port_client;
	ipv4_hdr->src_addr = destination_connection->ip_addr_client;

	//Transform CNS to WRITES
	#ifdef CNS_TO_WRITE
	map_cns_to_write(pkt,slot);
	#endif

	mark_pkt_rdma_checksum(pkt);

	unlock_connection_state(destination_connection);
}


struct Request_Map *find_slot_mod(struct Connection_State *source_connection, struct rte_mbuf *pkt)
{
	struct roce_v2_header *roce_hdr = get_roce_hdr(pkt);
	uint32_t search_sequence_number = roce_hdr->packet_sequence_number;
	uint32_t slot_num = mod_slot(search_sequence_number);
	struct Request_Map *mapped_request = &(source_connection->Outstanding_Requests[slot_num]);

	//First search to find if the sequence numbers match
	if ((!slot_is_open(mapped_request)) && mapped_request->mapped_sequence == search_sequence_number)
	{
		return mapped_request;
	}
	return NULL;
}


void map_back_packet(struct rte_mbuf * pkt, struct Request_Map *mapped_request) {
		struct rte_ether_hdr *eth_hdr = get_eth_hdr(pkt);
		struct rte_ipv4_hdr *ipv4_hdr = get_ipv4_hdr(pkt);
		struct rte_udp_hdr *udp_hdr = get_udp_hdr(pkt);
		struct roce_v2_header *roce_hdr = get_roce_hdr(pkt);
		struct Connection_State *destination_cs = &Connection_States[mapped_request->id];
		roce_hdr->packet_sequence_number = mapped_request->original_sequence;
		roce_hdr->dest_qp = destination_cs->stcqp;
		udp_hdr->src_port = destination_cs->udp_src_port_client;
		ipv4_hdr->dst_addr = destination_cs->ip_addr_client;
		ipv4_hdr->src_addr = destination_cs->ip_addr_server;
		copy_eth_addr(destination_cs->cts_eth_addr, eth_hdr->d_addr.addr_bytes);
		copy_eth_addr(destination_cs->stc_eth_addr, eth_hdr->s_addr.addr_bytes);
}


struct rte_mbuf * generate_missing_ack(struct Request_Map *missing_write, struct Connection_State *cs) {

	if (missing_write)
	{
		struct rte_mbuf *pkt = rte_pktmbuf_alloc(mbuf_pool);
		if (pkt == NULL) {
			printf("NULL PACKET ack generation\n");
		}

		rte_pktmbuf_append(pkt,ACK_PKT_LEN);
		//do I need to expand the packet here?
		struct rte_ether_hdr *eth_hdr = get_eth_hdr(pkt);
		rte_memcpy(eth_hdr,template_ack,ACK_PKT_LEN);


		map_back_packet(pkt, missing_write);

		//make a local copy of the id before unlocking
		uint32_t id = missing_write->id;
		//open up the mapped request
		open_slot(missing_write);

		//Update the tracked msn this requires adding to it, and then storing
		//back to the connection states To do this we need to take a look at
		//what the original connection was so that we can update it accordingly.

		//This is must be done after the unlock
		struct Connection_State *destination_cs = &Connection_States[id];
		struct roce_v2_header *roce_hdr = get_roce_hdr(pkt);
		uint32_t msn;
		//Avoid double locking
		if (destination_cs == cs) {
			msn = produce_and_update_msn_lockless(roce_hdr, destination_cs);
		} else {
			msn = produce_and_update_msn(roce_hdr, destination_cs);
		}

		set_msn(roce_hdr, msn);

		#ifdef CNS_TO_WRITE
		map_write_ack_to_atomic_ack(pkt,missing_write);
		#endif

		mark_pkt_rdma_checksum(pkt);

		return pkt;
	}
	return NULL;
}

inline struct Request_Map *find_missing_write(struct Connection_State * source_connection, uint32_t search_sequence_number){

	uint32_t slot_num = mod_slot_minus_one(search_sequence_number);
	struct Request_Map *mapped_request_1 = &(source_connection->Outstanding_Requests[slot_num]);

	if(likely(slot_is_open(mapped_request_1))) {
		return NULL;
	}

	if (mapped_request_1->rdma_op==RC_WRITE_ONLY || mapped_request_1->rdma_op==RC_CNS){
		return mapped_request_1;
	}

	return NULL;
}


//Mappping qp backwards is the demultiplexing operation.  The first step is to
//identify the kind of packet and figure out if it has been placed on the
struct map_packet_response map_qp_backwards(struct rte_mbuf *pkt)
{
	struct roce_v2_header *roce_hdr = get_roce_hdr(pkt);

	struct map_packet_response mpr;
	mpr.pkts[0] = pkt;
	mpr.size = 1;

	struct Connection_State *source_connection = &Connection_States[fast_find_id_qp(roce_hdr->dest_qp)];
	lock_connection_state(source_connection);

	struct Request_Map *mapped_request = find_slot_mod(source_connection, pkt);
	//struct Request_Map *
	if (likely(mapped_request != NULL))
	{
		//Itterativly search for coalesed packets
		uint32_t search_sequence_number = roce_hdr->packet_sequence_number;
		struct Request_Map *missing_write = find_missing_write(source_connection, search_sequence_number);
		while(missing_write) {
			struct rte_mbuf * coalesed_ack = generate_missing_ack(missing_write, source_connection);
			if (coalesed_ack != NULL) {
				//Shuffle packets back
				for(int i=mpr.size;i>0;i--) {
					mpr.pkts[i] = mpr.pkts[i-1];
				}
				mpr.pkts[0] = coalesed_ack;
				mpr.size++;

				//subtract the sequence number by one and revert it back to roce header format
				search_sequence_number = revert_seq(readable_seq(search_sequence_number) - 1);
				open_slot(missing_write);

				missing_write = find_missing_write(source_connection, search_sequence_number);
			} else {
				break;
			}
		}

		//Set the packety headers to that of the mapped request
		//struct Connection_State orginial = &Connection_States[mapped_request->id];
		map_back_packet(pkt,mapped_request);

		#ifdef CNS_TO_WRITE
		map_write_ack_to_atomic_ack(pkt,mapped_request);
		#endif

		open_slot(mapped_request);
		unlock_connection_state(source_connection);

		//Update the tracked msn this requires adding to it, and then storing
		//back to the connection states To do this we need to take a look at
		//what the original connection was so that we can update it accordingly.

		//This is must be done after the unlock
		struct Connection_State *destination_cs = &Connection_States[mapped_request->id];
		uint32_t msn = produce_and_update_msn(roce_hdr, destination_cs);
		set_msn(roce_hdr, msn);

#ifdef MAP_PRINT
		uint32_t packet_msn = get_msn(roce_hdr);
		id_colorize(mapped_request->id);
		printf("        MAP BACK :: (core %d) seq(%d <- %d) mseq(%d <- %d) (op %s) (s-qp %d)\n", rte_lcore_id(), readable_seq(mapped_request->original_sequence), readable_seq(mapped_request->mapped_sequence), readable_seq(msn), readable_seq(packet_msn), ib_print_op(roce_hdr->opcode), roce_hdr->dest_qp);
#endif

		mark_pkt_rdma_checksum(pkt);

		return mpr;
	}

	unlock_connection_state(source_connection);

	uint32_t msn = find_and_update_stc(roce_hdr);
	if (msn > 0)
	{
		set_msn(roce_hdr, msn);
		mark_pkt_rdma_checksum(pkt);
	}
	return mpr;
}

struct map_packet_response map_qp(struct rte_mbuf *pkt)
{
	struct map_packet_response mpr;
	mpr.pkts[0] = pkt;
	mpr.size = 1;

	//Not mapping yet
	if (unlikely(has_mapped_qp == 0))
	{
		track_qp(pkt);
		return mpr;
	}

	struct roce_v2_header *roce_hdr = get_roce_hdr(pkt);
	if (unlikely(!qp_is_mapped(roce_hdr->dest_qp)))
	{
		return mpr;
	}

	//backward path requires little checking
	uint8_t opcode = roce_hdr->opcode;
	if (opcode == RC_ACK || opcode == RC_ATOMIC_ACK || opcode == RC_READ_RESPONSE)
	{
		return map_qp_backwards(pkt);
	}

	if (opcode == RC_READ_REQUEST)
	{
		int key = 0;
#ifdef READ_STEER
		//lock_write_steering();
		struct clover_hdr *clover_header = get_clover_hdr(pkt);
		struct read_request *rr = (struct read_request *)clover_header;
		#define network_order_1024 262144
		if (rr->rdma_extended_header.dma_length == network_order_1024) {
			key = does_read_have_cached_write(rr->rdma_extended_header.vaddr);
			if (likely(key != 0)) {
				steer_read(pkt, key);
			}
		} 
#endif
		map_qp_forward(pkt, key);
	}
	else if (opcode == RC_WRITE_ONLY)
	{
		struct clover_hdr *clover_header = get_clover_hdr(pkt);
		struct write_request *wr = (struct write_request *)clover_header;
		uint64_t *key = (uint64_t *)&(wr->data);

		/*
		When I perform writes across keys but aslo mux QP on the writes
		there is an issue where writes with size of 68 get through and
		do not cause a jump in the sequnce number. The weird thing is
		that when I wrote the inital interposition this worked fine. By
		inital I mean single key many id reader and writer. I'm not sure
		how adding a new key lead to writes of size 68 showing up, and
		why they did not before hand. The bottom line is that 1) I'm not
		sure I can get the key from these packets. so it relies on the
		fact that the prior CNS or write for this key is going to the
		same qp as the 68 byte write. I'm using the last key, so that
		there is not reordering on the recipt of the packets because
		that causes faulting.

		Jun 15 2021 - Stewart Grant
		*/
		struct rte_ipv4_hdr *ipv4_hdr = get_ipv4_hdr(pkt);
		uint32_t size = ntohs(ipv4_hdr->total_length);
		if (unlikely(size == 68))
		{
			uint32_t id = fast_find_id_qp(roce_hdr->dest_qp);
			*key = get_latest_key(id);
		}
		map_qp_forward(pkt, *key);
	}
	else if (opcode == RC_CNS)
	{
		int id = fast_find_id_qp(roce_hdr->dest_qp);
		map_qp_forward(pkt, get_latest_key(id));
	}
	return mpr;
}

int should_track(struct rte_mbuf *pkt)
{
	struct rte_ipv4_hdr *ipv4_hdr = get_ipv4_hdr(pkt);
	struct roce_v2_header *roce_hdr = get_roce_hdr(pkt);
	uint32_t size = ntohs(ipv4_hdr->total_length);
	uint8_t opcode = roce_hdr->opcode;
	uint32_t r_qp = roce_hdr->dest_qp;

	if (opcode == RC_ACK)
	{
		return 1;
	}
	if (size == 60 && opcode == RC_READ_REQUEST)
	{
		return 1;
	}
	if ((size == 56 || size == 1072) && opcode == RC_READ_RESPONSE)
	{
		return 1;
	}
	if (opcode == RC_WRITE_ONLY)
	{
		if (size == 68 && (qp_is_mapped(r_qp) == 1))
		{
			return 1;
		}
		else if (size == 1084)
		{
			return 1;
		}
	}
	if (opcode == RC_ATOMIC_ACK)
	{
		return 1;
	}
	if (size == 72 && opcode == RC_CNS)
	{
		return 1;
	}
	return 0;
}

void track_qp(struct rte_mbuf *pkt)
{
	//Return if not mapping QP !!!THIS FEATURE SHOULD TURN ON AND OFF EASILY!!!
	if (likely(has_mapped_qp != 0))
	{
		return;
	}

	if (!should_track(pkt))
	{
		return;
	}

	struct roce_v2_header *roce_hdr = get_roce_hdr(pkt);
	switch (roce_hdr->opcode)
	{
	case RC_ACK:
	case RC_READ_RESPONSE:
		find_and_update_stc(roce_hdr);
		break;
	case RC_ATOMIC_ACK:
		init_stc(pkt);
		break;
	case RC_READ_REQUEST:
	case RC_CNS:
	case RC_WRITE_ONLY:
		cts_track_connection_state(pkt);
		break;
	default:
		printf("Should not reach this case statement\n");
		break;
	}
}

uint32_t get_predicted_shift(uint32_t packet_size)
{
	switch (packet_size)
	{
	case 1024:
		return 10;
	case 512:
		return 9;
	case 256:
		return 8;
	case 128:
		return 7;
	case 64:
		return 6;
	default:
		printf("Unknown packet size (%d) exiting\n", packet_size);
		exit(0);
	}
}

void check_and_cache_predicted_shift(uint32_t rdma_size)
{
	//init write packet size
	if (unlikely(write_value_packet_size == 0))
	{
		write_value_packet_size = rdma_size;
		predict_shift_value = get_predicted_shift(write_value_packet_size);
	}
	//sanity check, we should only reach here if we are dealing with statically sized write packets
	if (unlikely(write_value_packet_size != rdma_size))
	{
		printf("error in write packet block, but packet size not correct established runtime size %d\n", write_value_packet_size);
		exit(0);
	}
}

void catch_ecn(struct rte_mbuf *pkt, uint8_t opcode)
{

#ifdef CATCH_ECN

	if (opcode == ECN_OPCODE)
	{
			printf("The ecn packets are usually being issued from Yak1, it's very hard to tell this\n"
			"though because there are not really any performance counters that you can read. The command to turn ECN off is\n"
			"> cd /sys/class/net/enp129s0/ecn/roce_np/enable\n"
			"> sudo su\n"
			"> echo 0 > 0\n"
			"> for i in $(seq 0 7); do echo 0>$i; cat $i; done\n"
			"I would automate this but its a pain to ssh and sudo (Stewart Grant Oc13 2021)"
			);
		struct Buffer_State * bs = get_buffer_state(pkt);
		for (int i = 0; i < 20; i++)
		{
			setlocale(LC_NUMERIC, "");
			printf("ECN packet # %'d id %d\n", packet_counter,bs->id);
			print_packet_lite(pkt);
		}
		print_packet(pkt);
		count_held_packets();
#ifdef TAKE_MEASUREMENTS
		write_run_data();
#endif
		exit(0);
	}
#endif
	if (opcode == ECN_OPCODE)
	{
		printf("ECN Found, but continuing to run as normal\n");
	}

}

//in the case of regular operation where all of the writes are
//cached, there should be no nacks. however if we set
//cache_keyspace to a value less than all of the keys, then we are
//going to start to see nacks. this only is a problem when the
//writes are allowed to pass through. reads should be a strict
//subset of the writes so it's not going to be a problem there. if
//you are running an experiment to show the effect of varying
//cache size just comment out the exit below.
void catch_nack(struct clover_hdr *clover_header, uint8_t opcode)
{
	if (opcode == RC_ATOMIC_ACK)
	{
		struct cs_response *csr = (struct cs_response *)clover_header;
		uint32_t original = ntohl(csr->atomc_ack_extended.original_remote_data);
		if (original != 0)
		{
			nacked_cns++;
			printf("danger only hit here if you have the keyspace option turned on\n");
			//exit(0);
		}
	}
}


void true_classify(struct rte_mbuf *pkt)
{
	struct rte_ipv4_hdr *ipv4_hdr = get_ipv4_hdr(pkt);
	struct roce_v2_header *roce_hdr = get_roce_hdr(pkt);
	struct clover_hdr *clover_header = get_clover_hdr(pkt);

	uint32_t size = ntohs(ipv4_hdr->total_length);
	uint8_t opcode = roce_hdr->opcode;
	uint32_t r_qp = roce_hdr->dest_qp;

	catch_ecn(pkt, opcode);
	catch_nack(clover_header, opcode);

//insert if we are doing read steering, but not qp_mapping.
//TODO this should exist here reguardless of MAP_QP, 
//TODO mapqp should depend on read steering and not the other way around	
#ifdef READ_STEER
#ifndef MAP_QP

	//lock_write_steering();
	if (opcode == RC_READ_REQUEST){
		struct read_request *rr = (struct read_request *)clover_header;
		uint32_t size = ntohl(rr->rdma_extended_header.dma_length);
		uint32_t id = fast_find_id_qp(roce_hdr->dest_qp);
		if (size == 1024) {
			uint32_t key = (*does_read_have_cached_write)(rr->rdma_extended_header.vaddr);
			if (likely(key != 0)) {
				//printf("READ HIT  (%d) %"PRIx64" Key: %"PRIu64"\n",id, rr->rdma_extended_header.vaddr,key);
				steer_read(pkt, key);
			} else {
				//printf("READ MISS (%d) %"PRIx64"\n",id, rr->rdma_extended_header.vaddr);
			}
		}
	} 
	//unlock_write_steering();
#endif
#endif

	if (opcode == RC_WRITE_ONLY)
	{
		if (size == 252 || size == 68)
		{
			return;
		}
		lock_write_steering();

		struct write_request *wr = (struct write_request *)clover_header;
		uint64_t *key = (uint64_t *)&(wr->data);
		uint32_t id = get_or_create_id(r_qp);
		set_latest_key(id, *key);

		uint32_t rdma_size = ntohl(wr->rdma_extended_header.dma_length);
		check_and_cache_predicted_shift(rdma_size);

		//Experimentatiopn for working with restricted cache keyspaces. Return
		//from here if the key is out of the cache range.
		if (unlikely(*key > CACHE_KEYSPACE))
		{
			unlock_write_steering();
			return;
		}

		#ifdef READ_STEER
		update_write_vaddr_cache(*key, wr->rdma_extended_header.vaddr);
		#endif

		//okay so this happens twice becasuse the order is
		//Write 0;
		//Write 1;
		//Cn wNS 1 (write 1 - > write 2)
		if (likely(first_write[*key] != 0 && first_cns[*key] != 0))
		{
			predict_address[*key] = ((be64toh(wr->rdma_extended_header.vaddr) - be64toh(first_write[*key])) >> predict_shift_value);
			outstanding_write_predicts[id][*key] = predict_address[*key];
			outstanding_write_vaddrs[id][*key] = wr->rdma_extended_header.vaddr;
		}
		else
		{
			if (first_write[*key] == 0)
			{
				//Write 0
				//This is the init write. On the first write, for some reason we
				//don't do anything. I just mark that it's received.
				first_write[*key] = 1;

			}
			else if (first_write[*key] == 1)
			{
				//Write 1
				//Here we actually record both the first vaddr for the write, and the
				first_write[*key] = wr->rdma_extended_header.vaddr; //first write subject to change
				next_vaddr[*key] = wr->rdma_extended_header.vaddr;	//next_vaddr subject to change
				outstanding_write_predicts[id][*key] = 1;
				outstanding_write_vaddrs[id][*key] = wr->rdma_extended_header.vaddr;
			}
			else
			{
				//We should not really reach this point, I think that it's an error if we do.
				outstanding_write_predicts[id][*key] = 1;
				outstanding_write_vaddrs[id][*key] = wr->rdma_extended_header.vaddr;
			}
		}
		unlock_write_steering();
	}

	if (size == 72 && opcode == RC_CNS)
	{
		lock_write_steering();
		//Find value of the clover pointer. This is the value we are going to potentially swap out.
		struct cs_request *cs = (struct cs_request *)clover_header;
		uint64_t swap = htobe64(MITSUME_GET_PTR_LH(be64toh(cs->atomic_req.swap_or_add)));
		uint32_t id = get_or_create_id(r_qp);
		uint64_t key = get_latest_key(id);
		uint64_t *first_cns_p = &first_cns[key];
		uint64_t *first_write_p = &first_write[key];
		uint64_t *outstanding_write_vaddr_p = &outstanding_write_vaddrs[id][key];
		uint64_t *outstanding_write_predict_p = &outstanding_write_predicts[id][key];
		uint64_t *next_vaddr_p = &next_vaddr[key];

		//This is the first instance of the cns for this key, it is a misunderstood case
		//For now return after setting the first instance of the key to the swap value
		if (key > CACHE_KEYSPACE)
		{
			//this key is not being tracked, return
			unlock_write_steering();
			return;
		}

		//This is the first time we are seeking this key. We can't make a
		//prediction for it so we are just going to store the address so that we
		//can use is as an offset for performing redirections later.
		if (unlikely(*first_cns_p == 0))
		{
			//log_printf(INFO,"setting swap for key %"PRIu64" id: %d -- Swap %"PRIu64"\n", latest_key[id],id, swap);
			*first_cns_p = swap;
			*first_write_p = *outstanding_write_vaddr_p;
			*next_vaddr_p = *outstanding_write_vaddr_p;

			for (uint i = 0; i < TOTAL_CLIENTS; i++)
			{
				if (outstanding_write_predicts[i][key] == 1)
				{
					//printf("(init conflict dected) recalculating outstanding writes for key %"PRIu64" id\n",latest_key[id],id);
					uint64_t predict = ((be64toh(outstanding_write_vaddrs[i][key]) - be64toh(*first_write_p)) >> 10);
					outstanding_write_predicts[i][key] = predict;
				}
			}
			//Return and forward the packet if this is the first cns
			#ifdef READ_STEER
			update_read_tail(key, *next_vaddr_p);
			#endif

			unlock_write_steering();
			return;
		}

		//Based on the key predict where the next CNS address should go. This
		//requires the first CNS to be set
		uint64_t predict = *outstanding_write_predict_p;
		predict = predict + be64toh(*first_cns_p);
		predict = htobe64(0x00000000FFFFFF & predict); // THIS IS THE CORRECT MASK

		//Here we have had a first cns (assuming bunk, and we eant to point to the latest in the list)
		if (*next_vaddr_p != cs->atomic_req.vaddr)
		{
			cs->atomic_req.vaddr = *next_vaddr_p; //We can add this once we can predict with confidence

			#ifdef MAP_QP
			mark_pkt_rdma_checksum(pkt);
			#else
			recalculate_rdma_checksum(pkt);
			#endif

		}

		//given that a cns has been determined move the next address for this
		//key, to the outstanding write of the cns that was just made
		if (likely(predict == swap))
		{
			//This is where the write (for all intents and purposes has been commited)
			*next_vaddr_p = *outstanding_write_vaddr_p;

#ifdef READ_STEER
			update_read_tail(key, *next_vaddr_p);
#endif
			//erase the old entries
			*outstanding_write_predict_p = 0;
			*outstanding_write_vaddr_p = 0;
		}
		else
		{
			//This is the crash condtion
			//Fatal, unable to find the next key
			printf("Crashing on (CNS PREDICT) ID: %d psn %d\n", id, readable_seq(roce_hdr->packet_sequence_number));
			check_id_mapping();
			print_connection_state_status();
			printf("predicted: %" PRIu64 "\n", be64toh(predict));
			printf("actual:    %" PRIu64 "\n", be64toh(swap));
			print_address(&predict);
			print_address(&swap);

			printf("unable to find the next oustanding write, how can this be? SWAP: %" PRIu64 " latest_key[id = %d]=%" PRIu64 ", first cns[key = %" PRIu64 "]=%" PRIu64 "\n", swap, id, latest_key[id], latest_key[id], first_cns[latest_key[id]]);
			printf("we should stop here and fail, but for now lets keep going\n");
			print_packet_lite(pkt);
			count_held_packets();

#ifdef TAKE_MEASUREMENTS
			write_run_data();
#endif

			print_packet(pkt);
			exit(0);
		}
		unlock_write_steering();
	}
	return;
}

static const struct rte_eth_conf port_conf_default = {
	.rxmode = {
		.max_rx_pkt_len = RTE_ETHER_MAX_LEN,
	},
};

#define RSS_HASH_KEY_LENGTH 40				// for mlx5
//uint64_t rss_hf = ETH_RSS_NONFRAG_IPV4_UDP; //ETH_RSS_UDP | ETH_RSS_TCP | ETH_RSS_IP;// | ETH_RSS_VLAN; /* RSS IP by default. */
uint64_t rss_hf = ETH_RSS_UDP | ETH_RSS_TCP | ETH_RSS_IP;// | ETH_RSS_VLAN; /* RSS IP by default. */

uint8_t sym_hash_key[RSS_HASH_KEY_LENGTH] = {
	0x6D,
	0x5A,
	0x6D,
	0x5A,
	0x6D,
	0x5A,
	0x6D,
	0x5A,
	0x6D,
	0x5A,
	0x6D,
	0x5A,
	0x6D,
	0x5A,
	0x6D,
	0x5A,
	0x6D,
	0x5A,
	0x6D,
	0x5A,
	0x6D,
	0x5A,
	0x6D,
	0x5A,
	0x6D,
	0x5A,
	0x6D,
	0x5A,
	0x6D,
	0x5A,
	0x6D,
	0x5A,
	0x6D,
	0x5A,
	0x6D,
	0x5A,
	0x6D,
	0x5A,
	0x6D,
	0x5A,
};

/* basicfwd.c: Basic DPDK skeleton forwarding example. */

/*
 * Initializes a given port using global settings and with the RX buffers
 * coming from the mbuf_pool passed as a parameter.
 */
static inline int
//port_init(uint16_t port, struct rte_mempool *mbuf_pool[MEMPOOLS], uint32_t core_count)
port_init(uint16_t port, struct rte_mempool *mbuf_pool, uint32_t core_count)
{
	struct rte_eth_conf port_conf = port_conf_default;
	const uint16_t rx_rings = core_count, tx_rings = core_count;
	//const uint16_t rx_rings = RX_CORES, tx_rings = core_count-RX_CORES;

	uint16_t nb_rxd = RX_RING_SIZE;
	uint16_t nb_txd = TX_RING_SIZE;
	int retval;
	uint16_t q;
	struct rte_eth_dev_info dev_info;
	struct rte_eth_txconf txconf;

	if (!rte_eth_dev_is_valid_port(port))
		return -1;

	retval = rte_eth_dev_info_get(port, &dev_info);
	if (retval != 0)
	{
		printf("Error during getting device (port %u) info: %s\n",
			   port, strerror(-retval));
		return retval;
	}



	port_conf.rxmode.split_hdr_size=0;
	port_conf.rxmode.offloads|=DEV_RX_OFFLOAD_CHECKSUM;
	port_conf.rxmode.offloads|=DEV_RX_OFFLOAD_SCATTER;

	int rxing_cores;

	#ifdef MAP_QP
		rxing_cores=RX_CORES;
	#else
		rxing_cores=rte_lcore_count();
	#endif

	//STW RSS
	if (rxing_cores > 1)
	{
		//STW: use sym_hash_key for RSS
		printf("Configuring RSS for a total of %d cores\n",core_count);
		port_conf.rx_adv_conf.rss_conf.rss_key = sym_hash_key;
		port_conf.rx_adv_conf.rss_conf.rss_key_len = RSS_HASH_KEY_LENGTH;
		port_conf.rx_adv_conf.rss_conf.rss_hf =
			rss_hf & dev_info.flow_type_rss_offloads;
	}
	else
	{
		port_conf.rx_adv_conf.rss_conf.rss_key = NULL;
		port_conf.rx_adv_conf.rss_conf.rss_hf = 0;
	}

	if (port_conf.rx_adv_conf.rss_conf.rss_hf != 0)
	{
		printf("configuring multi queue rss ETH_MQ_RX_RSS\n");
		port_conf.rxmode.mq_mode = (enum rte_eth_rx_mq_mode)ETH_MQ_RX_RSS;
	}
	else
	{
		printf("Single Queue RSS\n");
		port_conf.rxmode.mq_mode = ETH_MQ_RX_NONE;
	}
	//\STW RSS

	if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
		port_conf.txmode.offloads |=
			DEV_TX_OFFLOAD_MBUF_FAST_FREE;

	/* Configure the Ethernet device. */
	printf("configuring %d rx rings %d tx rings\n",rx_rings,tx_rings);
	retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
	if (retval != 0)
		return retval;

	retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
	if (retval != 0)
		return retval;
	/* Allocate and set up 1 RX queue per Ethernet port. */
	/*
	printf("RX desc %d, TX desc %d\n",nb_rxd,nb_txd);
	retval = rte_eth_rx_queue_setup(port, q, nb_rxd,
									rte_eth_dev_socket_id(port), NULL, mbuf_pool);
	if (retval < 0)
		return retval;
		*/
	//for (q = 0; q < rx_rings; q++)
	for (q = 0; q < rxing_cores; q++)
	{
		//printf("allocating queue %d (cores %d)\n",q,core_count);
		retval = rte_eth_rx_queue_setup(port, q, nb_rxd,
										rte_eth_dev_socket_id(port), NULL, mbuf_pool);
		if (retval < 0)
			return retval;
	}

	//printf("RX queues %d, TX queues %d\n",dev_info.nb_rx_queues,dev_info.nb_tx_queues);

	txconf = dev_info.default_txconf;
	txconf.offloads = port_conf.txmode.offloads;
	/* Allocate and set up 1 TX queue per Ethernet port. */
	for (q = 0; q < tx_rings; q++)
	{
		retval = rte_eth_tx_queue_setup(port, q, nb_txd,
										rte_eth_dev_socket_id(port), &txconf);
		if (retval < 0)
			return retval;
	}

	/* Start the Ethernet port. */
	retval = rte_eth_dev_start(port);
	if (retval < 0)
		return retval;

	/* Display the port MAC address. */
	struct rte_ether_addr addr;
	retval = rte_eth_macaddr_get(port, &addr);
	if (retval != 0)
		return retval;

	printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
		   " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
		   port,
		   addr.addr_bytes[0], addr.addr_bytes[1],
		   addr.addr_bytes[2], addr.addr_bytes[3],
		   addr.addr_bytes[4], addr.addr_bytes[5]);

	/* Enable RX in promiscuous mode for the Ethernet device. */
	retval = rte_eth_promiscuous_enable(port);
	if (retval != 0)
		return retval;

	return 0;
}


void print_packet_lite(struct rte_mbuf *buf)
{
	struct rte_ipv4_hdr *ipv4_hdr = get_ipv4_hdr(buf);
	struct roce_v2_header *roce_hdr = get_roce_hdr(buf);

	uint32_t size = ntohs(ipv4_hdr->total_length);
	const char *op = ib_print_op(roce_hdr->opcode);
	uint32_t dest_qp = roce_hdr->dest_qp;
	uint32_t seq = readable_seq(roce_hdr->packet_sequence_number);
	int msn = get_msn(roce_hdr);

	if (msn != -1)
	{
		msn = readable_seq(msn);
	}

	int id = fast_find_id(buf);
	int key = get_key(buf);

	id_colorize(id);
	//printf("[core %d][id %d][op:%s (%d)][size: %d][dst: %d][seq %d][msn %d][key %d](pkt %d)\n", rte_lcore_id(), id, op, roce_hdr->opcode, size, dest_qp, seq, msn, key,packet_counter);
	printf("[core %2d][id %3d][seq %5d][op:%19s][key %4d][msn %5d][size: %4d][dst: %d][dst-readable %d](pkt %d)\n", rte_lcore_id(), id, seq, op,key, msn, size, dest_qp, ntohl(dest_qp) >> 8, packet_counter);
}


rte_atomic16_t thread_barrier;
rte_atomic16_t thread_barrier2;
void all_thread_barrier(rte_atomic16_t *barrier) {
	//printf("stalling on thread %d\n",rte_lcore_id());
	rte_atomic16_add(barrier,1);
	while(barrier->cnt < (int16_t)rte_lcore_count()) {}
}

int print_counter=0;

/*
 * The lcore main. This is the main thread that does the work, reading from
 * an input port and writing to an output port.
 */
static __attribute__((noreturn)) void
lcore_main(void)
{
	uint16_t port;

	/*
	 * Check that the port is on the same NUMA node as the polling thread
	 * for best performance.
	 */
	RTE_ETH_FOREACH_DEV(port)
	if (rte_eth_dev_socket_id(port) > 0 &&
		rte_eth_dev_socket_id(port) !=
			(int)rte_socket_id())
		log_printf(INFO, "WARNING, port %u is on remote NUMA node to "
						 "polling thread.\n\tPerformance will "
						 "not be optimal.\n",
				   port);

	log_printf(INFO, "\nCore %u forwarding packets. [Ctrl+C to quit]\n",
			   rte_lcore_id());


	//printf("@@ Switch Core %d Initalized @@\n", rte_lcore_id());
	/* Run until the application is quit or killed. */

	#ifdef INPUT_OUTPUT_EXAMPLES
	FILE * ingress_trace_file=open_logfile("/tmp/ingress.pkttrace");
	FILE * egress_trace_file=open_logfile("/tmp/egress.pkttrace");
	#endif



	port=0;
	for (;;)
	{
		/*
		 * Receive packets on a port and forward them on the paired
		 * port. The mapping is 0 -> 1, 1 -> 0, 2 -> 3, 3 -> 2, etc.
		 */

		//Once we are initalized just hop in forever
		uint32_t queue = rte_lcore_id() / 2;

		#ifdef MAP_QP
		if (has_mapped_qp) {
			if (queue >= RX_CORES)  {
				general_tx_eternal(port,queue,tx_queues[queue]);
			}
		}
		#endif

		struct rte_mbuf *rx_pkts[BURST_SIZE];
		#ifndef MAP_QP
		struct rte_mbuf *tx_pkts[BURST_SIZE*PACKET_INFLATION];
		uint32_t to_tx = 0;
		#endif

		#ifdef MAP_QP
		if (unlikely((has_mapped_qp==0))){
			if(unlikely(fully_qp_init())) {
			
				all_thread_barrier(&thread_barrier);
				if (rte_lcore_id() == 0) {
					printf("\n$$ Queue Pair Multiplexing On $$\n");
					//print_connection_state_status();
					flush_buffers(port);
					lock_qp();
					has_mapped_qp=1;
					unlock_qp();
				}
				all_thread_barrier(&thread_barrier2);
			}
		}
		#endif

		#ifdef MAP_QP
		if (queue >= RX_CORES)  {
			general_tx(port,queue,tx_queues[queue]);
			continue;
		}
		#endif

		const uint16_t nb_rx = rte_eth_rx_burst(port, queue, rx_pkts, BURST_SIZE);
		if (unlikely(nb_rx == 0)) {
			continue;
		}

		for (uint16_t i = 0; i < nb_rx; i++)
		{

			if (likely(i < nb_rx - 1))
			{
				rte_prefetch0(rte_pktmbuf_mtod(rx_pkts[i + 1], void *));
			}


			//print_packet_lite(rx_pkts[i]);
			#ifdef TAKE_MEASUREMENTS
			sum_processed_data(rx_pkts[i]);
			#endif

			#ifdef INPUT_OUTPUT_EXAMPLES
			#define MAX_PRINT_PACKET 100
			if (print_counter < MAX_PRINT_PACKET) {
				print_raw_file(rx_pkts[i],ingress_trace_file);
			}
			#endif


			#ifdef WRITE_STEER
			true_classify(rx_pkts[i]);
			#endif


			#ifdef INPUT_OUTPUT_EXAMPLES
			if (print_counter < MAX_PRINT_PACKET) {
				print_raw_file(rx_pkts[i],egress_trace_file);
				print_counter++;
			}
			if (print_counter == MAX_PRINT_PACKET){
				close_logfile(ingress_trace_file);
				close_logfile(egress_trace_file);
				print_counter++;
			}
			#endif


			#ifdef MAP_QP
			struct map_packet_response mpr = map_qp(rx_pkts[i]);
			for (uint32_t j = 0; j < mpr.size; j++)
			{
				//printf("enqueuing\n");
				//print_packet_lite(rx_pkts[i]);
				general_tx_enqueue(mpr.pkts[j]);
			}

			#else
			tx_pkts[i]=rx_pkts[i];
			to_tx++;
			#endif
		}

		#ifndef MAP_QP
		rte_eth_tx_burst(port, queue, tx_pkts, to_tx);
		#endif

		#ifdef TAKE_MEASUREMENTS
		if(has_mapped_qp){
			calculate_in_flight(&Connection_States);
		}
		#endif
	}
}




int coretest(__attribute__((unused)) void *arg)
{
	lcore_main();
	return 1;
}

void fork_lcores(void)
{
	int lcore;
	RTE_LCORE_FOREACH_SLAVE(lcore)
	{
		rte_eal_remote_launch(coretest, NULL, lcore);
	}
}

void error_switch(void) {
	switch (rte_errno) {
		case E_RTE_NO_CONFIG:
			printf("no config\n");
			break;
		case E_RTE_SECONDARY:
			printf("called from secondary\n");
			break;
		case EINVAL:
			printf("cache size provided is too large\n");
			break;
		case ENOSPC:
			printf("The maximum number of memzones has allready been allocated\n");
			break;
		case EEXIST:
			printf("a memzone with the same name already exists\n");
			break;
		case ENOMEM:
			printf("no appropriate memory area found\n");
			break;
		default:
			printf("I've not finished the error defs yet\n");
			break;
	}
}

void mode_print(void){
	#ifdef WRITE_STEER
		printf("WRITE STEERING:\tON\n");
	#else
		printf("WRITE STEERING\tOFF\n");
	#endif

	#ifdef READ_STEER
		printf("READ STEERING:\tON\n");
	#else
		printf("READ STEERING:\tOFF\n");
	#endif

	#ifdef MAP_QP
		printf("QP MAPPING:\tON\n");
	#else
		printf("QP MAPPING:\tOFF\n");
	#endif

	#ifdef CNS_TO_WRITE
		printf("CNS TO WRITE:\tON\n");
	#else
		printf("CNS TO WRITE:\tOFF\n");
	#endif

	#ifdef PERFORM_ICRC
		printf("ICRC:\t\tON\n");
	#else
		printf("ICRC:\t\tOFF\n");
	#endif


}
/*
 * The main function, which does initialization and calls the per-lcore
 * functions.
 */
int main(int argc, char *argv[])
{
	unsigned nb_ports;
	uint16_t portid;

	/* Initialize the Environment Abstraction Layer (EAL). */
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

	argc -= ret;
	argv += ret;

	printf("\n\t---Starting DPDK Switch---\n");
	printf("\n\t---Machine Config---\n");

	/* Check that there is an even number of ports to send/receive on. */
	nb_ports = rte_eth_dev_count_avail();

	printf("num ports:%u\n", nb_ports);

	/* Creates a new mempool in memory to hold the mbufs. */

	mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * nb_ports,
										MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "cannot create mbuf pool\n");
	printf("created mbuf_pool [size %d]\n", mbuf_pool->cache_size);


	printf("initalizing ports with %d cores...\n",rte_lcore_count());
	/* Initialize all ports. */
	RTE_ETH_FOREACH_DEV(portid)
	if (port_init(portid, mbuf_pool, rte_lcore_count()) != 0)
	//if (port_init(portid, mbuf_pool, RX_CORES) != 0)
		rte_exit(EXIT_FAILURE, "Cannot init port %" PRIu16 "\n",
				 portid);

	// if (rte_lcore_count() > 1)
	// 	printf("\nWARNING: Too many lcores enabled. Only 1 used.\n");

	//dome debugging
	struct rte_eth_link link;
	RTE_ETH_FOREACH_DEV(portid)
	rte_eth_link_get(0,&link);
	printf("Link Speed %d MBPS\n",link.link_speed);
	if (link.link_duplex == ETH_LINK_FULL_DUPLEX) {
		printf("FULL DUPLEX ON\n");
	} else {
		printf("NOT FULL DUPLEX\n");
	}

	printf("\n\t---Software Config---\n");

	printf("Client Threads:\t%d\n", TOTAL_CLIENTS);
	printf("Keyspace:\t%d\n", KEYSPACE);

	mode_print();

	if (init == 0)
	{
		bzero(first_write, KEYSPACE * sizeof(uint64_t));
		bzero(first_cns, KEYSPACE * sizeof(uint64_t));
		bzero(predict_address, KEYSPACE * sizeof(uint64_t));
		bzero(latest_cns_key, KEYSPACE * sizeof(uint64_t));
		bzero(latest_key, TOTAL_ENTRY * sizeof(uint64_t));
		bzero(outstanding_write_predicts, TOTAL_ENTRY * KEYSPACE * sizeof(uint64_t));
		bzero(outstanding_write_vaddrs, TOTAL_ENTRY * KEYSPACE * sizeof(uint64_t));
		bzero(next_vaddr, KEYSPACE * sizeof(uint64_t));

#ifdef TAKE_MEASUREMENTS
		init_measurements();
#endif

#ifdef READ_STEER
		bzero(cached_write_vaddr_mod, sizeof(uint64_t) * HASHSPACE);
		bzero(cached_write_vaddr_mod_lookup, sizeof(uint64_t) * HASHSPACE);
		bzero(cached_write_vaddr_mod_latest, sizeof(uint64_t) * KEYSPACE);
#endif

		rte_rwlock_init(&next_lock);
		rte_rwlock_init(&qp_lock);

		init_reorder_buf();
		init_connection_states();
		init_fast_find_id();
		write_value_packet_size = 0;
		predict_shift_value = 0;
		has_mapped_qp = 0;
		init = 1;
		rte_atomic16_init(&atomic_qp_id_counter);
		rte_atomic16_set(&atomic_qp_id_counter,-1);
		for (int i=0;i<MEMPOOLS;i++) {
			//printf("init tx mempool %d\n",i);
			//tx_queues[i] = rte_ring_create(txq_names[i], 4096, rte_eth_dev_socket_id(0), RING_F_SP_ENQ | RING_F_SC_DEQ);
			tx_queues[i] = rte_ring_create(txq_names[i], 4096, rte_eth_dev_socket_id(0), 0);
		}

		printf("\n\t---Running DPDK Switch (%d cores)---\n",rte_lcore_count());
		// printf("[Master Core %d] Static Initalization Complete\n", rte_lcore_id());
		// printf("[Master Core %d] Forking %d Switch Cores\n",rte_lcore_id(),rte_lcore_count());
	}

	fork_lcores();
	lcore_main();
	return 0;
}