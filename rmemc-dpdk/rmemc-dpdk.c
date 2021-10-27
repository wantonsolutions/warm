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
#include "rmemc-dpdk.h"
#include "roce_v2.h"
#include "clover_structs.h"
#include "print_helpers.h"
#include "measurement.h"
#include "packet_templates.h"
#include <arpa/inet.h>

#include <rte_jhash.h>
#include <rte_hash.h>
#include <rte_table.h>
#include <rte_atomic.h>

#include <execinfo.h>

#include <endian.h>

//#define PRINT_PACKET_BUFFERING

#define KEYSPACE 1024
#define CACHE_KEYSPACE 1024
#define SEQUENCE_NUMBER_SHIFT 256
int MOD_SLOT = 1;

//#define DATA_PATH_PRINT
//#define MAP_PRINT
#define CATCH_ECN

uint32_t debug_start_printing_every_packet = 0;

static int has_mapped_qp = 0;
static int packet_counter = 0;
static int hash_collisons=0;

//#define SINGLE_CORE
#define WRITE_STEER
#define READ_STEER
#define MAP_QP
#define CNS_TO_WRITE

#define WRITE_VADDR_CACHE_SIZE 16


uint64_t cached_write_vaddrs[KEYSPACE][WRITE_VADDR_CACHE_SIZE];
uint32_t writes_per_key[KEYSPACE];

#define HASHSPACE (1 << 23)
uint64_t cached_write_vaddr_mod[HASHSPACE];
uint64_t cached_write_vaddr_mod_lookup[HASHSPACE];
uint64_t cached_write_vaddr_mod_latest[KEYSPACE];

rte_rwlock_t next_lock;
rte_rwlock_t qp_lock;
rte_rwlock_t qp_init_lock;
rte_rwlock_t mem_qp_lock;

struct rte_mempool *mbuf_pool_ack;
#define IPV4_OFFSET 14
#define UDP_OFFSET 34
#define ROCE_OFFSET 42
#define CLOVER_OFFSET 54

inline struct rte_ether_hdr *get_eth_hdr(struct rte_mbuf *pkt)
{
	return rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
}

inline struct rte_ipv4_hdr *get_ipv4_hdr(struct rte_mbuf *pkt)
{
	return (uint8_t*)get_eth_hdr(pkt) + IPV4_OFFSET;
}

inline struct rte_udp_hdr *get_udp_hdr(struct rte_mbuf *pkt)
{
	return (uint8_t*)get_eth_hdr(pkt) + UDP_OFFSET;
}

inline struct roce_v2_header *get_roce_hdr(struct rte_mbuf *pkt)
{
	return (uint8_t*)get_eth_hdr(pkt) + ROCE_OFFSET;
}

inline struct clover_hdr *get_clover_hdr(struct rte_mbuf *pkt)
{
	return (uint8_t*)get_eth_hdr(pkt) + CLOVER_OFFSET;
}

void lock_qp(void)
{
	//rte_rwlock_write_lock(&qp_lock);
	//rte_smp_mb();
}

void unlock_qp(void)
{
	//rte_rwlock_write_unlock(&qp_lock);
	//rte_smp_mb();
}

void lock_mem_qp(void)
{
	//rte_rwlock_write_lock(&mem_qp_lock);
	//rte_smp_mb();
}

void unlock_mem_qp(void)
{
	//rte_rwlock_write_unlock(&mem_qp_lock);
	//rte_smp_mb();
}

void lock_next(void)
{
	//rte_rwlock_write_lock(&next_lock);
	//rte_smp_mb();
}

void unlock_next(void)
{
	//rte_rwlock_write_unlock(&next_lock);
	//rte_smp_mb();
}

uint64_t pkt_timestamp_monotonic=0;
uint64_t pkt_timestamp_not_thread_safe(void)
{
	return pkt_timestamp_monotonic++;
}

static struct rte_hash_parameters qp2id_params = {
	.name = "qp2id",
	.entries = TOTAL_ENTRY,
	.key_len = sizeof(uint32_t),
	.hash_func = rte_jhash,
	.hash_func_init_val = 0,
	.socket_id = 0,
};

void lock_connection_state(struct Connection_State *cs) {
	//printf("lock %d\n",cs->id);
	//just to remove compiler errors
	cs->id = cs->id;
	//rte_rwlock_write_lock(&(cs->cs_lock));
	//rte_smp_mb();
}

void unlock_connection_state(struct Connection_State *cs) {
	//printf("unlock %d\n",cs->id);
	cs->id = cs->id;
	//rte_rwlock_write_unlock(&(cs->cs_lock));
	//rte_smp_mb();
}


struct Connection_State Connection_States[TOTAL_ENTRY];
struct rte_hash *qp2id_table;
static uint32_t qp_id_counter = 0;
uint32_t qp_values[TOTAL_ENTRY];
uint32_t id_qp[TOTAL_ENTRY];

#define HASH_RETURN_IF_ERROR(handle, cond, str, ...)                     \
	do                                                                   \
	{                                                                    \
		if (cond)                                                        \
		{                                                                \
			printf("ERROR line %d: " str "\n", __LINE__, ##__VA_ARGS__); \
			if (handle)                                                  \
				rte_hash_free(handle);                                   \
			return -1;                                                   \
		}                                                                \
	} while (0)

//Keys start at 1, so I'm subtracting 1 to make the first key equal to index
//zero.  qp_id_counter is the total number of qp that can be written to. So here
//we are just taking all of the keys and wrapping them around so the first key
//goes to the first qp, and the qp_id_counter + 1  key goes to the first qp.
uint32_t key_to_qp(uint64_t key)
{
	//int qp = 8;
	int qp = 1;
	//uint32_t index = (key) % TOTAL_CLIENTS;
	uint32_t index = (key) % qp;
	return id_qp[index];
}

uint32_t id_to_qp(uint32_t id) {
	int qp = 2;
	uint32_t index = (id) % qp;
	return id_qp[index];
}

int init_hash(void)
{
	qp2id_table = rte_hash_create(&qp2id_params);
	HASH_RETURN_IF_ERROR(qp2id_table, qp2id_table == NULL, "qp2id_table creation failed");
	return 0;
}

int set_id(uint32_t qp, uint32_t id)
{
#ifdef DATA_PATH_PRINT
	log_printf(DEBUG, "adding (%d,%d) to hash table\n", qp, id);
	printf("adding (%d,%d) to hash table\n", qp, id);
#endif
	qp_values[id] = id;
	id_qp[id] = qp;
	int ret = rte_hash_add_key_data(qp2id_table, &qp, &qp_values[id]);
	HASH_RETURN_IF_ERROR(qp2id_table, ret < 0, "unable to add new qp id (%d,%d)\n", qp, id);
	set_fast_id(qp,id);
	return ret;
}

//Warning this is a very unsafe function. Only call it when you know that a
//packet corresponds to an ID that has an established QP. If the ID is not set,
//this will set it. Otherwise the ID is returned.
uint32_t get_id(uint32_t qp)
{
	uint32_t *return_value;
	int32_t id;

	//first try to go fast
	id = fast_find_id_qp(qp);
	if (id != -1) {
		return id;
	}

	//if the id was equal to -1 then it's not initalized and we need to do the hash lookup
	//TODO remove the hash table all together its too slow
	int ret = rte_hash_lookup_data(qp2id_table, &qp, (void **)&return_value);
	if (ret < 0)
	{
		lock_qp();
		id = qp_id_counter;
		printf("no such id exists yet adding qp id pq: %d id: %d\n", qp, id);
		set_id(qp, id);
		qp_id_counter++;
		unlock_qp();
	}
	else
	{
		id = *return_value;
	}
	id_colorize(id);
	return id;
}

int fully_qp_init(void)
{
	for (int i = 0; i < TOTAL_CLIENTS; i++)
	{
		struct Connection_State * cs = &Connection_States[i];
		//print("testing id %d\n",cs->id)
		lock_connection_state(cs);
		if (!cs->sender_init || !cs->receiver_init)
		{
			unlock_connection_state(cs);
			return 0;
		}
		unlock_connection_state(cs);
	}
	return 1;
}

struct rte_mbuf *mem_qp_buf[TOTAL_ENTRY][PKT_REORDER_BUF];
struct rte_mbuf *client_qp_buf[TOTAL_ENTRY][PKT_REORDER_BUF];
struct rte_mbuf *ect_qp_buf[TOTAL_ENTRY][PKT_REORDER_BUF];

uint64_t mem_qp_timestamp[TOTAL_ENTRY][PKT_REORDER_BUF];
uint64_t client_qp_timestamp[TOTAL_ENTRY][PKT_REORDER_BUF];
uint64_t ect_qp_timestamp[TOTAL_ENTRY][PKT_REORDER_BUF];

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

		//client
		bs = &client_buffer_states[i];
		bs->id = i;
		bs->dequeable = &(client_qp_dequeuable[i]);
		bs->head = &client_qp_buf_head[i];
		bs->tail = &client_qp_buf_tail[i];
		bs->buf = &client_qp_buf[i];
		bs->timestamps = &client_qp_timestamp[i];

		//memory
		bs = &mem_buffer_states[i];
		bs->id = Connection_States[i].id;
		bs->dequeable = &(mem_qp_dequeuable[i]);
		bs->head = &(mem_qp_buf_head[i]);
		bs->tail = &(mem_qp_buf_tail[i]);
		bs->buf = &(mem_qp_buf[i]);
		bs->timestamps = &(mem_qp_timestamp[i]);
	}
}

void init_reorder_buf(void)
{
	printf("initalizing reorder buffs");
	bzero(mem_qp_buf_head, TOTAL_ENTRY * sizeof(uint64_t));
	bzero(mem_qp_buf_tail, TOTAL_ENTRY * sizeof(uint64_t));
	bzero(client_qp_buf_head, TOTAL_ENTRY * sizeof(uint64_t));
	bzero(client_qp_buf_tail, TOTAL_ENTRY * sizeof(uint64_t));
	bzero(ect_qp_buf_head, TOTAL_ENTRY * sizeof(uint64_t));
	bzero(ect_qp_buf_tail, TOTAL_ENTRY * sizeof(uint64_t));

	bzero(mem_qp_dequeuable, TOTAL_ENTRY * sizeof(uint8_t));
	bzero(client_qp_dequeuable, TOTAL_ENTRY * sizeof(uint8_t));
	bzero(ect_qp_dequeuable, TOTAL_ENTRY * sizeof(uint8_t));
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


struct Buffer_State get_buffer_state(struct rte_mbuf *pkt) {
	struct Buffer_State bs;

	int id = fast_find_id(pkt);
	if (unlikely(has_mapped_qp == 0) || id == -1) {
		bs.id = 0;
		bs.dequeable = &(ect_qp_dequeuable[bs.id]);
		bs.head = &(ect_qp_buf_head[bs.id]);
		bs.tail = &(ect_qp_buf_tail[bs.id]);
		bs.buf = &(ect_qp_buf[bs.id]);
		bs.timestamps = &(ect_qp_timestamp[bs.id]);
		return bs;
	}

	//Find the ID of the packet We are using generic head and tail pointers here
	//for both directions of queueing.  The memory direction has a queue and so
	//does the client.  If the requests are arriving out of order in either
	//direction they will be queued.

	struct roce_v2_header *roce_hdr = get_roce_hdr(pkt);
	//Assign to the etc buffer by default
	if (Connection_States[id].ctsqp == roce_hdr->dest_qp)
	{
		bs.id = Connection_States[id].id;
		bs.dequeable = &(mem_qp_dequeuable[id]);
		bs.head = &(mem_qp_buf_head[id]);
		bs.tail = &(mem_qp_buf_tail[id]);
		bs.buf = &(mem_qp_buf[id]);
		bs.timestamps = &(mem_qp_timestamp[id]);
		return bs;
	}
	if (Connection_States[id].receiver_init == 1 && Connection_States[id].stcqp == roce_hdr->dest_qp)
	{
		bs.id = Connection_States[id].id;
		bs.dequeable = &(client_qp_dequeuable[id]);
		bs.head = &client_qp_buf_head[id];
		bs.tail = &client_qp_buf_tail[id];
		bs.buf = &client_qp_buf[id];
		bs.timestamps = &client_qp_timestamp[id];
		return bs;
	}

	printf("something is weird here unable to get the buffer state\n");
	exit(0);

	return bs;
}

void enqueue_finish_mem_pkt_bulk(struct rte_mbuf **pkts, uint32_t size) {

	struct Buffer_State buffer_states[BURST_SIZE*BURST_SIZE];
	uint32_t sequence_numbers[BURST_SIZE*BURST_SIZE];
	lock_mem_qp();

	lock_qp();
	for (uint32_t i=0;i<size;i++) {
		buffer_states[i] = get_buffer_state(pkts[i]);
		struct roce_v2_header *roce_hdr = get_roce_hdr(pkts[i]);
		sequence_numbers[i] = readable_seq(roce_hdr->packet_sequence_number);
	}
	unlock_qp();
	//Here the buffer states have been collected
	uint32_t seq;
	struct Buffer_State bs;
	struct rte_mbuf *pkt;
	for (uint32_t i=0;i<size;i++) {
		//set loop locals
		seq = sequence_numbers[i];
		bs = buffer_states[i];
		pkt = pkts[i];
		//If it's going in the etc buffer then just throw it on fifo
		if (bs.buf == &ect_qp_buf[bs.id])
		{
			seq = *(bs.tail) + 1;
		}

		uint32_t entry = seq % PKT_REORDER_BUF;
		(*bs.buf)[entry] = pkt;

		int64_t ts = pkt_timestamp_not_thread_safe();
		(*bs.timestamps)[entry] = ts;

		//On the first call the sequence numbers are going to start somewhere
		//random. In this case just move the head of the buffer to the current
		//sequence number
		if (unlikely(*bs.head == 0))
		{
			//printf("setting head id: %d seq: %d\n",bs.id,seq);
			*bs.head = (uint64_t)seq;
		}

		if (unlikely(*bs.head > seq)) {
			printf(
				"This is a really bad situation\n"
				"we got the initial conditions wrong so enqueue on the first packet\n"
				"ie *bs.head == 0 did not work, in this case we need to move the head back a bit\n"
				"the downside here is that if this is some sort of error we will not have contiguous\n"
				"sequence numbers which could cause knock on proble3ms later\n"
				"(( MOVING HEAD BACKWARDS!!))\n"
				"Stewart Grant Oct 11 2021\n"
				);
				printf("ID %d SEQ = %d (ptr %p)\n", bs.id, seq,bs.head);
				*bs.head=seq;
				if(!contiguous_buffered_packets_2(bs)) {
					printf("(ERROR) non congiguous after head step back\n");
				}
		}


		//If the tail is the new latest sequence number than slide it forward
		if (*bs.tail < seq)
		{
			//printf("setting tail id: %d seq: %d\n",bs.id,seq);
			*bs.tail = seq;
		}

		if (likely(contiguous_buffered_packets_2(bs))) {
			*bs.dequeable = 1;
		} else {
			*bs.dequeable = 0;
		}
	}
	unlock_mem_qp();
}

int contiguous_buffered_packets_2(struct Buffer_State bs) {
	for (uint32_t i = *bs.head; i <= *bs.tail; i++)
	{
		if((*bs.buf)[i % PKT_REORDER_BUF] == NULL) {
			return 0;
		}
	}
	return 1;
}

void print_mpr(struct map_packet_response* mpr) {
	printf("MPR size: %d\n",mpr->size);
	for (uint32_t i=0;i<mpr->size;i++) {
		printf("[TS: %"PRIu64"]",mpr->timestamps[i]);
		print_packet_lite(mpr->pkts[i]);
	}
}

void merge_mpr(struct map_packet_response *dest, struct map_packet_response *source) {
	for (uint32_t i=0;i<source->size;i++) {
		dest->pkts[dest->size] = source->pkts[i];
		dest->timestamps[dest->size] = source->timestamps[i];
		dest->size++;
	}
}

inline void copy_over_mpr_index(struct map_packet_response *dest, struct map_packet_response *source, uint32_t *source_index) {
	dest->pkts[dest->size] = source->pkts[*source_index];
	dest->timestamps[dest->size] = source->timestamps[*source_index];
	dest->size++;
	*source_index = *source_index + 1;
}

void copy_from_index(struct map_packet_response *dest, struct map_packet_response * source, uint32_t *source_index){
	for (uint32_t i=*source_index;i<source->size;i++){
		copy_over_mpr_index(dest,source,source_index);
	}
}

void merge_mpr_ts(struct map_packet_response *dest, struct map_packet_response *source) {
	struct map_packet_response merged;
	uint32_t dst_index, src_index;
	dst_index=0;
	src_index=0;
	merged.size=0;

	while (dst_index < dest->size && src_index < source->size) {
		if (dest->timestamps[dst_index] < source->timestamps[src_index]) {
			copy_over_mpr_index(&merged,dest,&dst_index);
		} else {
			copy_over_mpr_index(&merged,source,&src_index);
		}
	}
	copy_from_index(&merged,dest,&dst_index);
	copy_from_index(&merged,source,&src_index);
	dest->size=0;
	merge_mpr(dest,&merged);
}

void merge_mpr_ts_2(struct map_packet_response * output, struct map_packet_response *left, struct map_packet_response *right) {
	uint32_t left_index, right_index;
	left_index=0;
	right_index=0;

	while (left_index < left->size && right_index < right->size) {
		if (left->timestamps[left_index] < right->timestamps[right_index]) {
			copy_over_mpr_index(output,left,&left_index);
		} else {
			copy_over_mpr_index(output,right,&right_index);
		}
	}
	copy_from_index(output,left,&left_index);
	copy_from_index(output,right,&right_index);
}


struct map_packet_response dequeue_finish_mem_pkt_bulk_merge2(uint8_t *dequeue_list, struct rte_mbuf *id_buf[TOTAL_ENTRY][PKT_REORDER_BUF], uint64_t id_timestamps[TOTAL_ENTRY][PKT_REORDER_BUF], uint64_t *head_list, uint64_t *tail_list) {
	struct map_packet_response mpr;
	mpr.pkts[0] = NULL;
	mpr.size = 0;
	uint32_t loop_max;


	if (id_buf == ect_qp_buf) {
		loop_max = 1;
	} else {
		loop_max = TOTAL_CLIENTS;
	}

	//Prep dequeue list
	uint8_t dequeue_ids[TOTAL_CLIENTS];
	uint8_t total_dequeue = 0;
	for (uint32_t id=0;id<loop_max;id++) {
		if(dequeue_list[id]) {
			dequeue_ids[total_dequeue]=id;
			total_dequeue++;
		}
	}

	struct Buffer_State bs;
	struct map_packet_response mpr_sub;
	for (int i=0;i<total_dequeue;i++) {
		uint32_t id = dequeue_ids[i];
		if(dequeue_list[id]) {
			//Small list for the individual dequeue
			mpr_sub.size = 0;

			bs.head = &head_list[id];
			bs.tail = &tail_list[id];
			bs.buf = &id_buf[id];
			bs.timestamps = &id_timestamps[id];

			//printf("dequeue loop\n");
			while (*bs.head <= *bs.tail)
			{
				//printf("loop\n");
				struct rte_mbuf *s_pkt = (*bs.buf)[*(bs.head) % PKT_REORDER_BUF];
				uint64_t ts = (*bs.timestamps)[*(bs.head) % PKT_REORDER_BUF];
				mpr_sub.pkts[mpr_sub.size]=s_pkt;
				mpr_sub.timestamps[mpr_sub.size]=ts;
				mpr_sub.size++;
				(*bs.buf)[*bs.head % PKT_REORDER_BUF] = NULL;
				*bs.head += 1;
			}
			merge_mpr_ts(&mpr,&mpr_sub);
			dequeue_list[id]=0;
		}
	}
	return mpr;
}

void dequeue_finish_mem_pkt_bulk_full(uint16_t port, uint32_t queue) {
	lock_mem_qp();
	struct map_packet_response mpr1;
	struct map_packet_response mpr2;
	struct map_packet_response mpr3;

	struct map_packet_response output_1;
	struct map_packet_response output_2;

	//mpr1 = dequeue_finish_mem_pkt_bulk_merge(port,queue,ect_qp_buf,ect_qp_timestamp,ect_qp_buf_head,ect_qp_buf_tail);
	//mpr2 = dequeue_finish_mem_pkt_bulk_merge(port,queue,mem_qp_buf,mem_qp_timestamp,mem_qp_buf_head,mem_qp_buf_tail);
	//mpr3 = dequeue_finish_mem_pkt_bulk_merge(port,queue,client_qp_buf,client_qp_timestamp,client_qp_buf_head,client_qp_buf_tail);
	mpr1 = dequeue_finish_mem_pkt_bulk_merge2(ect_qp_dequeuable,ect_qp_buf,ect_qp_timestamp,ect_qp_buf_head,ect_qp_buf_tail);
	mpr2 = dequeue_finish_mem_pkt_bulk_merge2(mem_qp_dequeuable,mem_qp_buf,mem_qp_timestamp,mem_qp_buf_head,mem_qp_buf_tail);
	mpr3 = dequeue_finish_mem_pkt_bulk_merge2(client_qp_dequeuable,client_qp_buf,client_qp_timestamp,client_qp_buf_head,client_qp_buf_tail);
	//merge_mpr_ts(&mpr1,&mpr2);
	//merge_mpr_ts(&mpr1,&mpr3);
	output_1.size=0;
	output_2.size=0;
	merge_mpr_ts_2(&output_1,&mpr1,&mpr2);
	merge_mpr_ts_2(&output_2,&output_1,&mpr3);

	//printf("sending %d\n",mpr1.size);
	#ifdef PRINT_PACKET_BUFFERING
	print_mpr(&mpr1);
	printf("\n\n\n");
	#endif
	//rte_eth_tx_burst(port, queue, &mpr1.pkts, mpr1.size);
	if (output_2.size > 0) {
		rte_eth_tx_burst(port, queue, (struct rte_mbuf **)&output_2.pkts, output_2.size);
	}
	unlock_mem_qp();
	return;
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
	uint16_t udp_src_port = udp_hdr->src_port;
	uint32_t cts_dest_qp = roce_hdr->dest_qp;
	uint32_t seq = roce_hdr->packet_sequence_number;
	uint32_t rkey = get_rkey_rdma_packet(roce_hdr);
	uint32_t server_ip = ipv4_hdr->dst_addr;
	uint32_t client_ip = ipv4_hdr->src_addr;

	//Find the connection state if it exists
	struct Connection_State cs;
	int id = get_id(cts_dest_qp);
	cs = Connection_States[id];

	//Connection State allready initalized for this qp
	if (cs.sender_init != 0)
	{
		return;
	}

	cs.id = id;
	cs.seq_current = seq;
	cs.udp_src_port_client = udp_src_port;
	cs.ctsqp = cts_dest_qp;
	cs.cts_rkey = rkey;
	cs.ip_addr_client = client_ip;
	cs.ip_addr_server = server_ip;
	copy_eth_addr((uint8_t *)eth_hdr->s_addr.addr_bytes, (uint8_t *)cs.cts_eth_addr);
	copy_eth_addr((uint8_t *)eth_hdr->d_addr.addr_bytes, (uint8_t *)cs.stc_eth_addr);
	cs.sender_init = 1;
	Connection_States[cs.id] = cs;
	rte_smp_mb();
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
	unlock_connection_state(cs);
	return cs->mseq_current;
}

//Update the server to clinet connection state based on roce and udp header.
//Search for the connection state first. If it's not found, just return.
//Otherwise update the MSN.
uint32_t find_and_update_stc(struct roce_v2_header *roce_hdr)
{
	struct Connection_State *cs;
	uint32_t found = 0;

	//check to see if this value has allready been set
	//Find the coonection
	for (int i = 0; i < TOTAL_ENTRY; i++)
	{
		cs = &Connection_States[i];
		//if (cs.seq_current == roce_hdr->packet_sequence_number) {
		lock_connection_state(cs);
		if (cs->stcqp == roce_hdr->dest_qp && cs->receiver_init == 1)
		{
			unlock_connection_state(cs);
			id_colorize(cs->id);
			found = 1;
			break;
		}
		unlock_connection_state(cs);
	}
	if (found == 0)
	{
		return 0;
	}

	//at this point we have the correct cs
	uint32_t msn = produce_and_update_msn(roce_hdr, cs);
	return msn;
}

void find_and_set_stc(struct roce_v2_header *roce_hdr, struct rte_udp_hdr *udp_hdr)
{
	struct Connection_State *cs;
	int32_t matching_id = -1;
	uint32_t total_matches = 0;

	//Everything has been initlaized, return
	if (likely(has_mapped_qp != 0))
	{
		return;
	}

	//Try to perform a basic update
	if (find_and_update_stc(roce_hdr) > 0)
	{
		return;
	}

	//If we are here then the connection should not be initlaized yet	/ /
	//Find the coonection
	for (int i = 0; i < TOTAL_ENTRY; i++)
	{
		cs = &Connection_States[i];
		lock_connection_state(cs);
		if (cs->sender_init != 0 && cs->seq_current == roce_hdr->packet_sequence_number)
		{
			matching_id = i;
			total_matches++;
		}
		unlock_connection_state(cs);
	}

	//Safty to prevent colisions when the sequence numbers align
	if (total_matches > 1)
	{
		//printf("find and set STC collision, wait another round\n");
		return;
	}
	//Return if nothing is found
	if (total_matches != 1)
	{
		//printf("not able to find a running connection to update\n");
		return;
	}

	//initalize the first time
	cs = &Connection_States[matching_id];
	lock_connection_state(cs);
	if (cs->receiver_init == 0)
	{
		id_colorize(cs->id);
		cs->stcqp = roce_hdr->dest_qp;
		cs->udp_src_port_server = udp_hdr->src_port;
		cs->mseq_current = get_msn(roce_hdr);
		cs->mseq_offset = htonl(ntohl(cs->seq_current) - ntohl(cs->mseq_current)); //still shifted by 256 but not in network order
		cs->receiver_init = 1;
	}
	unlock_connection_state(cs);
	return;
}

void find_and_set_stc_wrapper(struct roce_v2_header *roce_hdr, struct rte_udp_hdr *udp_hdr)
{
	if (roce_hdr->opcode != RC_ACK && roce_hdr->opcode != RC_ATOMIC_ACK && roce_hdr->opcode != RC_READ_RESPONSE)
	{
		printf("Only find and set stc on ACKS, and responses");
		return;
	}
	find_and_set_stc(roce_hdr, udp_hdr);
}

void update_cs_seq(uint32_t stc_dest_qp, uint32_t seq)
{
	uint32_t id = get_id(stc_dest_qp);
	struct Connection_State *cs = &Connection_States[id];
	if (cs->sender_init == 0)
	{
		printf("Attempting to set sequence number for non existant connection (exiting)");
		exit(0);
	}
	cs->seq_current = seq;
#ifdef DATA_PATH_PRINT
	printf("Updated connection state based on sequence number\n");
	print_connection_state(cs);
#endif
}

void update_cs_seq_wrapper(struct roce_v2_header *roce_hdr)
{
	if (roce_hdr->opcode != RC_WRITE_ONLY && roce_hdr->opcode != RC_CNS && roce_hdr->opcode != RC_READ_REQUEST)
	{
		printf("only update connection states for writers either WRITE_ONLY or CNS (and only for data path) exiting for safty");
		exit(0);
	}
	update_cs_seq(roce_hdr->dest_qp, roce_hdr->packet_sequence_number);
}

void cts_track_connection_state(struct rte_mbuf *pkt)
{
	struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	struct rte_ipv4_hdr *ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	struct rte_udp_hdr *udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	struct roce_v2_header *roce_hdr = (struct roce_v2_header *)((uint8_t *)udp_hdr + sizeof(struct rte_udp_hdr));
	if (has_mapped_qp == 1)
	{
		return;
	}
	init_cs_wrapper(pkt);
	update_cs_seq_wrapper(roce_hdr);
}

static uint64_t first_write[KEYSPACE];
static uint64_t second_write[TOTAL_ENTRY][KEYSPACE];
static uint64_t first_cns[KEYSPACE];
static uint64_t predict_address[KEYSPACE];
static uint64_t latest_cns_key[KEYSPACE];

static uint64_t outstanding_write_predicts[TOTAL_ENTRY][KEYSPACE]; //outstanding write index, contains precited addresses
static uint64_t outstanding_write_vaddrs[TOTAL_ENTRY][KEYSPACE];   //outstanding vaddr values, used for replacing addrs
static uint64_t next_vaddr[KEYSPACE];
static uint64_t latest_key[TOTAL_ENTRY];

static uint64_t overwitten_writes = 0;

uint64_t get_latest_key(uint32_t id)
{
	return latest_key[id];
}

void set_latest_key(uint32_t id, uint64_t key)
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
uint64_t murmur3(uint64_t k) {
  k ^= k >> 33;
  k *= 0xff51afd7ed558ccdull;
  k ^= k >> 33;
  k *= 0xc4ceb9fe1a85ec53ull;
  k ^= k >> 33;
  return k;
}

uint32_t mod_hash(uint64_t vaddr)
{
	//uint32_t index = ((uint32_t)crc32(0xFFFFFFFF, &vaddr, 8)) % HASHSPACE;
	uint32_t index = (uint32_t)murmur3(vaddr) % HASHSPACE;
	//uint32_t index = ((vaddr >> 36) % HASHSPACE);
	//uint32_t index = (ntohl(vaddr >> 33) % HASHSPACE);
	//uint64_t value = (ntohl(vaddr >> 42));
	//uint64_t index = be64toh(vaddr) % HASHSPACE;
	//printf("%"PRIx64"\n",value);
	//uint32_t index = (value % HASHSPACE);
	//printf("vaddr: %"PRIx64" index: %u\n",vaddr, index);
	return index;
}

inline void update_read_tail(uint64_t key, uint64_t vaddr) {
	cached_write_vaddr_mod_latest[key] = vaddr;
}

void update_write_vaddr_cache_mod(uint64_t key, uint64_t vaddr)
{
	uint32_t index = mod_hash(vaddr);
	if (cached_write_vaddr_mod[index] != 0) {
		hash_collisons++;
		if(unlikely(hash_collisons % 1000 == 0)) {
			printf("collisions %d old:%"PRIx64" new:%"PRIx64",\n",hash_collisons, cached_write_vaddr_mod[index], vaddr);
		}

	}
	//printf("%"PRIx64" \t\tWRITE\n",be64toh(vaddr));
	cached_write_vaddr_mod[index] = vaddr;
	cached_write_vaddr_mod_lookup[index] = key;

	if(unlikely(key == 0)) {
		printf("Writing Key 0 to cache, it's unlikely you want to do this as key 0 is special crashing so you are aware (Stewart Grant Sept 29 2021)\n");
	}
}


int does_read_have_cached_write_mod(uint64_t vaddr)
{
	//printf("\t%"PRIx64" \tREAD",be64toh(vaddr));
	uint32_t index = mod_hash(vaddr);
	if (cached_write_vaddr_mod[index] == vaddr)
	{
		//printf("(hit) key = %d\n",cached_write_vaddr_mod_lookup[index]);
		return cached_write_vaddr_mod_lookup[index];
	} else if (cached_write_vaddr_mod[index] == 0) {
		//printf("raw miss on read cache\n");
	} else {
		printf("collision miss (search) %"PRIx64" (existing) %"PRIx64"\n",vaddr, cached_write_vaddr_mod[index]);
		printf("key in that location %"PRIu64"\n",cached_write_vaddr_mod_lookup[index]);
	}
	//printf("\n");
	return 0;
}

void print_cache_population(void) {
	int populated = 0;
	for(int i=0;i<HASHSPACE;i++) {
		if (cached_write_vaddr_mod[i] != 0) {
			populated++;
		}
	}
	printf("CACHE SATURATION %d/%d\n",populated,HASHSPACE);
}


uint64_t get_latest_vaddr_mod(uint32_t key)
{
	return cached_write_vaddr_mod_latest[key];
}

int (*does_read_have_cached_write)(uint64_t) = does_read_have_cached_write_mod;
void (*update_write_vaddr_cache)(uint64_t, uint64_t) = update_write_vaddr_cache_mod;
uint64_t (*get_latest_vaddr)(uint32_t) = get_latest_vaddr_mod;

void steer_read(struct rte_mbuf *pkt, uint32_t key)
{
	struct clover_hdr *clover_header = get_clover_hdr(pkt);
	struct read_request *rr = (struct read_request *)clover_header;

	uint64_t vaddr = get_latest_vaddr(key);
	if (vaddr == 0)
	{
		return;
	}
	//Set the current address to the cached address of the latest write if it is old
	if (rr->rdma_extended_header.vaddr != vaddr)
	{
		rr->rdma_extended_header.vaddr = vaddr;
		recalculate_rdma_checksum(pkt);
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

uint32_t qp_is_mapped(uint32_t qp)
{
	if(fast_find_id_qp(qp) == -1) {
		return 0;
	}
	return 1;
}

struct Request_Map *get_empty_slot_mod(struct Connection_State *cs)
{
	uint32_t slot_num = mod_slot(cs->seq_current);
	struct Request_Map *slot = &(cs->Outstanding_Requests[slot_num]);
	if (unlikely(!slot_is_open(slot)))
	{
		printf("CLOSED SLOT, this is really bad [overwitten op %s] (#%"PRIu64")!!\n",ib_print_op(slot->rdma_op),overwitten_writes++);
		open_slot(slot);
	}
	return slot;
}

void map_cns_to_write(struct rte_mbuf *pkt, struct Request_Map *slot) {
	struct rte_ipv4_hdr *ipv4_hdr = get_ipv4_hdr(pkt);
	struct rte_udp_hdr *udp_hdr = get_udp_hdr(pkt);
	struct roce_v2_header *roce_hdr = get_roce_hdr(pkt);
	struct clover_hdr *clover_header = get_clover_hdr(pkt);
	if (roce_hdr->opcode == RC_CNS) {
		//print_packet(pkt);
		struct cs_request *cs = (struct cs_request *)clover_header;
		struct write_request *wr = (struct write_request *)clover_header;

		slot->was_write_swapped=1;

		//set the opcode to write
		uint64_t write_value;
		write_value = cs->atomic_req.swap_or_add;

		roce_hdr->opcode = RC_WRITE_ONLY;
		wr->rdma_extended_header.dma_length = 8;
		wr->ptr = write_value;


		//Header change now we need to adjust the length (should be -4)
		//uint32_t size_diff = (sizeof(cs_request) - sizeof(write_request));
		uint32_t size_diff = 4; // This is a magic number because the line before this (cs - write) gives 3
		rte_pktmbuf_trim(pkt,size_diff);
		udp_hdr->dgram_len = htons(ntohs(udp_hdr->dgram_len) - size_diff);
		ipv4_hdr->total_length = htons(ntohs(ipv4_hdr->total_length) - size_diff);
	}
}

void map_write_ack_to_atomic_ack(struct rte_mbuf *pkt, struct Request_Map *slot) {
	struct rte_ipv4_hdr *ipv4_hdr = get_ipv4_hdr(pkt);
	struct rte_udp_hdr *udp_hdr = get_udp_hdr(pkt);
	struct roce_v2_header *roce_hdr = get_roce_hdr(pkt);
	struct clover_hdr *clover_header = get_clover_hdr(pkt);

	if (slot->rdma_op == RC_CNS && slot->was_write_swapped==1) {
		//printf("converting missing ack to atomic-ack\n");
		//printf("converting write-ack to cns-ack\n");
		struct cs_response *cs = (struct cs_response *)clover_header;
		if(roce_hdr->opcode != RC_ACK) {
			printf("This was a safty check, we should not reach here with CNS_TO_WRITE turned on");
		}
		//Set the headers
		//First extend the size of the packt buf
		//print_packet(pkt);
		uint32_t size_diff = 8; // This is a magic number because the line before this (cs - write) gives 3
		rte_pktmbuf_append(pkt,size_diff);
		roce_hdr->opcode = RC_ATOMIC_ACK;
		//cs->atomc_ack_extended.original_remote_data = be64toh(0);
		cs->atomc_ack_extended.original_remote_data = htobe64(2048);
		//cs->atomc_ack_extended.original_remote_data = htobe64(1024);

		//printf("size diff %d\n",size_diff);

		uint16_t dgram_len = htons(ntohs(udp_hdr->dgram_len) + size_diff);
		//printf("dgram len new %d old %d\n",ntohs(dgram_len),ntohs(udp_hdr->dgram_len));
		udp_hdr->dgram_len = dgram_len;


		ipv4_hdr->total_length = htons(ntohs(ipv4_hdr->total_length) + size_diff);
		//printf("CNS - forward\n");
		//TODO remove the this part
		//ipv4_hdr->hdr_checksum = 0;
		//ipv4_hdr->hdr_checksum = rte_ipv4_cksum(ipv4_hdr);
		//recalculate_rdma_checksum(pkt);
		//print_packet(pkt);
	} 
	
}

void map_qp_forward(struct rte_mbuf *pkt, uint64_t key)
{
	struct rte_ether_hdr *eth_hdr = get_eth_hdr(pkt);
	struct rte_ipv4_hdr *ipv4_hdr = get_ipv4_hdr(pkt);
	struct rte_udp_hdr *udp_hdr = get_udp_hdr(pkt);
	struct roce_v2_header *roce_hdr = get_roce_hdr(pkt);

	//uint32_t r_qp = roce_hdr->dest_qp;
	//uint32_t id = fast_find_id(pkt);
	uint32_t id = fast_find_id(pkt);
	uint32_t n_qp = 0;

	//Keys are set to 0 when we are not going to map them. If the key is not
	//equal to zero apply the mapping policy.

	//Key to qp policy
	
	if (likely(key != 0))
	{
		n_qp = key_to_qp(key);
	}
	else
	{
		n_qp = roce_hdr->dest_qp;
	}
	/*
	n_qp = id_to_qp(id);
	*/

	//Find the connection state of the mapped destination connection.
	struct Connection_State *destination_connection;
	destination_connection = &Connection_States[fast_find_id_qp(n_qp)];
	if (unlikely(destination_connection == NULL))
	{
		printf("I did not want to end up here\n");
		return;
	}

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
	struct Request_Map *slot;
	slot = get_empty_slot_mod(destination_connection);

	//The next step is to save the data from the current packet
	//to a list of outstanding requests. As clover is blocking
	//we can use the id to track which sequence number belongs
	//to which request. These values will be used to map
	//responses back.
	close_slot(slot);
	slot->id = id;
	//Save a unique id of sequence number and the qp that this response will arrive back on
	slot->mapped_sequence = destination_connection->seq_current;
	slot->mapped_destination_server_to_client_qp = destination_connection->stcqp;
	slot->original_sequence = roce_hdr->packet_sequence_number;
	//Store the server to client to qp that this we will need to make the swap
	slot->server_to_client_qp = Connection_States[id].stcqp;
	slot->server_to_client_udp_port = Connection_States[id].udp_src_port_server;
	slot->original_src_ip = ipv4_hdr->src_addr;
	slot->server_to_client_rkey = Connection_States[id].stc_rkey;
	slot->rdma_op=roce_hdr->opcode;
	slot->was_write_swapped=0;

	copy_eth_addr(eth_hdr->s_addr.addr_bytes, slot->original_eth_addr);

	//Set the packet with the mapped information to the new qp
	roce_hdr->dest_qp = destination_connection->ctsqp;
	roce_hdr->packet_sequence_number = destination_connection->seq_current;
	udp_hdr->src_port = destination_connection->udp_src_port_client;
	ipv4_hdr->src_addr = destination_connection->ip_addr_client;
	unlock_connection_state(destination_connection);


	//Transform CNS to WRITES
	#ifdef CNS_TO_WRITE
	map_cns_to_write(pkt,slot);
	#endif

	//checksumming
	ipv4_hdr->hdr_checksum = 0;
	ipv4_hdr->hdr_checksum = rte_ipv4_cksum(ipv4_hdr);
	recalculate_rdma_checksum(pkt);
}

struct Connection_State *find_connection(struct rte_mbuf *pkt)
{
	//fast find connection
	int32_t id = fast_find_id(pkt);
	if (id == -1) {
		return NULL;
	}
	return &Connection_States[id];
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

struct rte_mbuf * generate_missing_ack(struct Request_Map *missing_write, struct Connection_State *cs) {
	//printf("Generating Ack\n");

	//struct Request_Map *
	if (missing_write)
	{
		struct rte_mbuf *pkt = rte_pktmbuf_alloc(mbuf_pool_ack);
		rte_pktmbuf_append(pkt,ACK_PKT_LEN);
		//do I need to expand the packet here?
		struct rte_ether_hdr *eth_hdr = get_eth_hdr(pkt);
		rte_memcpy(eth_hdr,template_ack,ACK_PKT_LEN);

		struct rte_ipv4_hdr *ipv4_hdr = get_ipv4_hdr(pkt);
		struct rte_udp_hdr *udp_hdr = get_udp_hdr(pkt);
		struct roce_v2_header *roce_hdr = get_roce_hdr(pkt);

		//copy over the connection state info
		copy_eth_addr(cs->stc_eth_addr,eth_hdr->s_addr.addr_bytes);
		ipv4_hdr->src_addr = cs->ip_addr_server;

		//Set the packety headers to that of the mapped request
		roce_hdr->dest_qp = missing_write->server_to_client_qp;
		roce_hdr->packet_sequence_number = missing_write->original_sequence;
		udp_hdr->src_port = missing_write->server_to_client_udp_port;
		ipv4_hdr->dst_addr = missing_write->original_src_ip;
		//!todo perhaps the issue here is that the packet is being delivered across eth?
		copy_eth_addr(missing_write->original_eth_addr, eth_hdr->d_addr.addr_bytes);
		//make a local copy of the id before unlocking
		uint32_t id = missing_write->id;
		//open up the mapped request
		open_slot(missing_write);

		//Update the tracked msn this requires adding to it, and then storing
		//back to the connection states To do this we need to take a look at
		//what the original connection was so that we can update it accordingly.

		//This is must be done after the unlock
		struct Connection_State *destination_cs = &Connection_States[id];
		uint32_t msn = produce_and_update_msn(roce_hdr, destination_cs);
		set_msn(roce_hdr, msn);

		#ifdef CNS_TO_WRITE
		map_write_ack_to_atomic_ack(pkt,missing_write);
		#endif

		//Ip mapping recalculate the ip checksum
		ipv4_hdr->hdr_checksum = 0;
		ipv4_hdr->hdr_checksum = rte_ipv4_cksum(ipv4_hdr);
		recalculate_rdma_checksum(pkt);

		//print_packet(pkt);

		//THis is a good place to inject the transition from a write to a CNS.
		return pkt;
	}
	return NULL;
}

struct Request_Map *find_missing_write(struct Connection_State * source_connection, uint32_t search_sequence_number){
	uint32_t slot_num_0 = mod_slot(search_sequence_number);
	uint32_t slot_num_1 = mod_slot_minus_one(search_sequence_number);
	struct Request_Map *mapped_request_0 = &(source_connection->Outstanding_Requests[slot_num_0]);
	struct Request_Map *mapped_request_1 = &(source_connection->Outstanding_Requests[slot_num_1]);

	if (!(mapped_request_0->rdma_op==RC_WRITE_ONLY || mapped_request_0->rdma_op==RC_CNS || mapped_request_0->rdma_op==RC_READ_REQUEST))
	{
		return NULL;
	}

	if ((!slot_is_open(mapped_request_1)) && mapped_request_1->rdma_op==RC_WRITE_ONLY)
	{
		return mapped_request_1;
	}
	if ((!slot_is_open(mapped_request_1)) && mapped_request_1->rdma_op==RC_CNS)
	{
		return mapped_request_1;
	}
	return NULL;
}


void sanity_check_mapping(struct rte_mbuf *pkt, struct Request_Map *mapped_request) {
	struct roce_v2_header *roce_hdr = get_roce_hdr(pkt);
	int invalid = 0;
	switch(roce_hdr->opcode){
		case RC_ACK:
			if ( (mapped_request->rdma_op != RC_WRITE_ONLY) &&
				 (mapped_request->rdma_op != RC_CNS)
			) {
				invalid = 1;
			}
			break;
		case RC_ATOMIC_ACK:
			if (mapped_request->rdma_op != RC_CNS) {
				invalid = 1;
			}
			break;
		case RC_READ_RESPONSE:
			if (mapped_request->rdma_op != RC_READ_REQUEST) {
				invalid = 1;
			}
			break;
		default:
			break;
	}
	if (invalid) {
		printf("INVALID MAPPING RESPONSE! OP is %s Original was %s\n",ib_print_op(roce_hdr->opcode),ib_print_op(mapped_request->rdma_op));
		print_packet_lite(pkt);
		print_packet(pkt);

		printf("BECN %d FECN %d\n",roce_hdr->bcen,roce_hdr->fecn);
	}
	return;
}

//Mappping qp backwards is the demultiplexing operation.  The first step is to
//identify the kind of packet and figure out if it has been placed on the
struct map_packet_response map_qp_backwards(struct rte_mbuf *pkt)
{
	struct rte_ether_hdr *eth_hdr = get_eth_hdr(pkt);
	struct rte_ipv4_hdr *ipv4_hdr = get_ipv4_hdr(pkt);
	struct rte_udp_hdr *udp_hdr = get_udp_hdr(pkt);
	struct roce_v2_header *roce_hdr = get_roce_hdr(pkt);

	struct map_packet_response mpr;
	mpr.pkts[0] = pkt;
	mpr.size = 1;

	if (unlikely(has_mapped_qp == 0))
	{
		printf("We have not started multiplexing yet. Returning with no packet modifictions.\n");
		return mpr;
	}

	struct Connection_State *source_connection = find_connection(pkt);
	//This packet might not be part of any active connection
	if (unlikely(source_connection == NULL))
	{
		return mpr;
	}

	lock_connection_state(source_connection);

	struct Request_Map *mapped_request = find_slot_mod(source_connection, pkt);
	//struct Request_Map *
	if (likely(mapped_request != NULL))
	{

		//Itterativly search for coalesed packets
		//TODO move this to it's own function
		uint32_t search_sequence_number = get_psn(pkt);
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
				missing_write = find_missing_write(source_connection, search_sequence_number);
			} else {
				break;
			}
		}


		//sanity_check_mapping(pkt,mapped_request);

		//Set the packety headers to that of the mapped request
		roce_hdr->dest_qp = mapped_request->server_to_client_qp;
		roce_hdr->packet_sequence_number = mapped_request->original_sequence;
		udp_hdr->src_port = mapped_request->server_to_client_udp_port;
		ipv4_hdr->dst_addr = mapped_request->original_src_ip;
		copy_eth_addr(mapped_request->original_eth_addr, eth_hdr->d_addr.addr_bytes);
		//make a local copy of the id before unlocking
		uint32_t id = mapped_request->id;
		//open up the mapped request

		//Update the tracked msn this requires adding to it, and then storing
		//back to the connection states To do this we need to take a look at
		//what the original connection was so that we can update it accordingly.

		//This is must be done after the unlock
		struct Connection_State *destination_cs = &Connection_States[id];
		uint32_t msn = produce_and_update_msn(roce_hdr, destination_cs);
		set_msn(roce_hdr, msn);

#ifdef MAP_PRINT
		uint32_t packet_msn = get_msn(roce_hdr);
		id_colorize(mapped_request->id);
		printf("        MAP BACK :: (core %d) seq(%d <- %d) mseq(%d <- %d) (op %s) (s-qp %d)\n", rte_lcore_id(), readable_seq(mapped_request->original_sequence), readable_seq(mapped_request->mapped_sequence), readable_seq(msn), readable_seq(packet_msn), ib_print_op(roce_hdr->opcode), roce_hdr->dest_qp);
#endif


		#ifdef CNS_TO_WRITE
		map_write_ack_to_atomic_ack(pkt,mapped_request);
		#endif

		//Ip mapping recalculate the ip checksum
		ipv4_hdr->hdr_checksum = 0;
		ipv4_hdr->hdr_checksum = rte_ipv4_cksum(ipv4_hdr);
		recalculate_rdma_checksum(pkt);
		open_slot(mapped_request);
		unlock_connection_state(source_connection);

		return mpr;
	}

	unlock_connection_state(source_connection);
	printf("[debug] find and update stc tracebasck on untracked packet\n");
	uint32_t msn = find_and_update_stc(roce_hdr);
	if (msn > 0)
	{
		int packet_msn = get_msn(roce_hdr);
		if (packet_msn == -1)
		{
			printf("How did we get here?\n");
		}

		//struct Buffer_State	bs = get_buffer_state(pkt);
		printf("@@@@ NO ENTRY TRANSITION @@@@ :: (seq %d) mseq(%d <- %d) (op %s) (s-qp %d) id (missing i took it out)\n", readable_seq(roce_hdr->packet_sequence_number), readable_seq(msn), readable_seq(packet_msn), ib_print_op(roce_hdr->opcode), roce_hdr->dest_qp);
		set_msn(roce_hdr, msn);
		recalculate_rdma_checksum(pkt);
	}
	return mpr;
}

struct map_packet_response map_qp(struct rte_mbuf *pkt)
{
	struct map_packet_response mpr;
	mpr.pkts[0] = pkt;
	mpr.size = 1;

	track_qp(pkt);
	//Not mapping yet
	if (has_mapped_qp == 0)
	{
		return mpr;
	}

	struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	struct rte_ipv4_hdr *ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	struct rte_udp_hdr *udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	struct roce_v2_header *roce_hdr = (struct roce_v2_header *)((uint8_t *)udp_hdr + sizeof(struct rte_udp_hdr));
	struct clover_hdr *clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));
	uint32_t size = ntohs(ipv4_hdr->total_length);
	uint8_t opcode = roce_hdr->opcode;
	uint32_t r_qp = roce_hdr->dest_qp;

	//backward path requires little checking
	if (opcode == RC_ACK || opcode == RC_ATOMIC_ACK || opcode == RC_READ_RESPONSE)
	{
		mpr = map_qp_backwards(pkt);
		return mpr;
	}

	if (qp_is_mapped(r_qp) == 0)
	{
		//This is not a packet we should map forward
		return mpr;
	}

	if (opcode == RC_READ_REQUEST)
	{
		int key = 0;
#ifdef READ_STEER
		struct read_request *rr = (struct read_request *)clover_header;
		uint32_t size = ntohl(rr->rdma_extended_header.dma_length);
		if (size == 1024) {
			key = (*does_read_have_cached_write)(rr->rdma_extended_header.vaddr);
			#ifdef TAKE_MEASUREMENTS
			increment_read_counter();
			#endif
			if (likely(key != 0)) {
				steer_read(pkt, key);
			} else {
				//printf("READ HIT  (%d) %"PRIx64" Key: %"PRIu64"\n",id, rr->rdma_extended_header.vaddr,key);
				uint32_t id = fast_find_id_qp(roce_hdr->dest_qp);
				printf("READ MISS (%d) %"PRIx64"\n",id, rr->rdma_extended_header.vaddr);
				//print_packet_lite(pkt);
				//print_packet(pkt);
				//exit(0);
				read_not_cached();
			}
		} 
#endif
		map_qp_forward(pkt, key);
	}
	else if (opcode == RC_WRITE_ONLY)
	{
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
		if (size == 68)
		{
			uint32_t id = get_id(roce_hdr->dest_qp);
			*key = get_latest_key(id);
			if (*key < 1 || *key > KEYSPACE)
			{
				printf("danger zone\n");
				*key = 0;
			}
		}
		map_qp_forward(pkt, *key);
	}
	else if (opcode == RC_CNS)
	{
		uint32_t id = get_id(roce_hdr->dest_qp);
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
		//printf("GENERAL RC ATOMIC_ACK\n");
		//print_packet(pkt);
		//struct cs_response * cs = (struct cs_response *) get_clover_hdr(pkt);
		//print_cs_request(&cs);
		//printf("%"PRIu64" ,_original data\n",cs->atomc_ack_extended.original_remote_data);
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
	if (!should_track(pkt))
	{
		return;
	}
	//Return if not mapping QP !!!THIS FEATURE SHOULD TURN ON AND OFF EASILY!!!
	if (likely(has_mapped_qp != 0))
	{
		return;
	}


	struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	struct rte_ipv4_hdr *ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	struct rte_udp_hdr *udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	struct roce_v2_header *roce_hdr = (struct roce_v2_header *)((uint8_t *)udp_hdr + sizeof(struct rte_udp_hdr));

	/*
	if (unlikely(roce_hdr->opcode == RC_WRITE_ONLY && fully_qp_init()))
	{
		//flip the switch
		print_first_mapping();
		lock_qp();
		has_mapped_qp = 1;
		unlock_qp();
		return;
	}
	*/

	switch (roce_hdr->opcode)
	{
	case RC_ACK:
	case RC_READ_RESPONSE:
		break;
	case RC_ATOMIC_ACK:
		find_and_set_stc_wrapper(roce_hdr, udp_hdr);
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
		struct Buffer_State bs = get_buffer_state(pkt);
		for (int i = 0; i < 20; i++)
		{
			printf("ECN packet # %d id %d\n", packet_counter,bs.id);
			print_packet_lite(pkt);
		}
		print_packet(pkt);
		//debug_start_printing_every_packet = 1;
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
	//void true_classify(struct rte_ipv4_hdr *ip, struct roce_v2_header *roce, struct clover_hdr * clover) {
	struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	struct rte_ipv4_hdr *ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	struct rte_udp_hdr *udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	struct roce_v2_header *roce_hdr = (struct roce_v2_header *)((uint8_t *)udp_hdr + sizeof(struct rte_udp_hdr));
	struct clover_hdr *clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));

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
	if (opcode == RC_READ_REQUEST){
		struct read_request *rr = (struct read_request *)clover_header;
		uint32_t size = ntohl(rr->rdma_extended_header.dma_length);
		if (size == 1024) {
			uint32_t key = (*does_read_have_cached_write)(rr->rdma_extended_header.vaddr);
			if (likely(key != 0)) {
				steer_read(pkt, key);
			} else {
				//printf("READ HIT  (%d) %"PRIx64" Key: %"PRIu64"\n",id, rr->rdma_extended_header.vaddr,key);
				printf("READ MISS %"PRIx64"\n", rr->rdma_extended_header.vaddr);
				//print_packet_lite(pkt);
				//print_packet(pkt);
				//exit(0);
				//read_not_cached();
			}
		}
	} 
#endif
#endif

	if (opcode == RC_WRITE_ONLY)
	{
		if (size == 252 || size == 68)
		{
			return;
		}
		lock_next();

		struct write_request *wr = (struct write_request *)clover_header;
		uint64_t *key = (uint64_t *)&(wr->data);
		uint32_t id = get_id(r_qp);
		set_latest_key(id, *key);

		uint32_t rdma_size = ntohl(wr->rdma_extended_header.dma_length);
		check_and_cache_predicted_shift(rdma_size);


		//Experimentatiopn for working with restricted cache keyspaces. Return
		//from here if the key is out of the cache range.
		if (*key > CACHE_KEYSPACE)
		{
			unlock_next();
			return;
		}

		#ifdef READ_STEER
		update_write_vaddr_cache(*key, wr->rdma_extended_header.vaddr);
		#endif

		//okay so this happens twice becasuse the order is
		//Write 0;
		//Write 1;
		//Cn wNS 1 (write 1 - > write 2)
		if (first_write[*key] != 0 && first_cns[*key] != 0)
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

		unlock_next();
	}

	if (size == 72 && opcode == RC_CNS)
	{
		lock_next();


		//Find value of the clover pointer. This is the value we are going to potentially swap out.
		struct cs_request *cs = (struct cs_request *)clover_header;
		uint64_t swap = htobe64(MITSUME_GET_PTR_LH(be64toh(cs->atomic_req.swap_or_add)));
		uint32_t id = get_id(r_qp);
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
			unlock_next();
			return;
		}

		//This is the first time we are seeking this key. We can't make a
		//prediction for it so we are just going to store the address so that we
		//can use is as an offset for performing redirections later.
		if (*first_cns_p == 0)
		{
			//log_printf(INFO,"setting swap for key %"PRIu64" id: %d -- Swap %"PRIu64"\n", latest_key[id],id, swap);
			*first_cns_p = swap;
			*first_write_p = *outstanding_write_vaddr_p;
			*next_vaddr_p = *outstanding_write_vaddr_p;

			// TODO TOTAL ENTRY is overkill here 
			for (uint i = 0; i < TOTAL_ENTRY; i++)
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

			unlock_next();
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
			recalculate_rdma_checksum(pkt);
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
			printf("predicted: %" PRIu64 "\n", be64toh(predict));
			printf("actual:    %" PRIu64 "\n", be64toh(swap));
			print_address(&predict);
			print_address(&swap);
			print_binary_address(&predict);
			print_binary_address(&swap);
			uint64_t diff = be64toh(swap) - be64toh(predict);
			printf("difference=%" PRIu64 "\n", diff);
			diff = htobe64(diff);
			print_binary_address(&diff);
			uint64_t xor = be64toh(swap) ^ be64toh(predict);
			printf("xor=%" PRIu64 "\n", xor);
			xor = htobe64(xor);
			print_binary_address(&xor);

			printf("base\n");
			print_binary_address(&first_cns[key]);
			uint64_t fcns = first_cns[key];
			print_binary_address(&fcns);

			printf("unable to find the next oustanding write, how can this be? SWAP: %" PRIu64 " latest_key[id = %d]=%" PRIu64 ", first cns[key = %" PRIu64 "]=%" PRIu64 "\n", swap, id, latest_key[id], latest_key[id], first_cns[latest_key[id]]);
			printf("we should stop here and fail, but for now lets keep going\n");

#ifdef TAKE_MEASUREMENTS
			write_run_data();
#endif

			print_packet(pkt);
			exit(0);
		}
		unlock_next();
	}
	return;
}

static const struct rte_eth_conf port_conf_default = {
	.rxmode = {
		.max_rx_pkt_len = RTE_ETHER_MAX_LEN,
	},
};

#define RSS_HASH_KEY_LENGTH 40				// for mlx5
uint64_t rss_hf = ETH_RSS_NONFRAG_IPV4_UDP; //ETH_RSS_UDP | ETH_RSS_TCP | ETH_RSS_IP;// | ETH_RSS_VLAN; /* RSS IP by default. */

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
port_init(uint16_t port, struct rte_mempool *mbuf_pool, uint32_t core_count)
{
	struct rte_eth_conf port_conf = port_conf_default;
	const uint16_t rx_rings = core_count, tx_rings = core_count;
	uint64_t nb_rxq = core_count;
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

	//STW RSS
	if (nb_rxq > 1)
	{
		//STW: use sym_hash_key for RSS
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
		port_conf.rxmode.mq_mode = (enum rte_eth_rx_mq_mode)ETH_MQ_RX_RSS;
	}
	else
	{
		port_conf.rxmode.mq_mode = ETH_MQ_RX_NONE;
	}
	//\STW RSS

	if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
		port_conf.txmode.offloads |=
			DEV_TX_OFFLOAD_MBUF_FAST_FREE;

	/* Configure the Ethernet device. */
	retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
	if (retval != 0)
		return retval;

	retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
	if (retval != 0)
		return retval;

	/* Allocate and set up 1 RX queue per Ethernet port. */
	for (q = 0; q < rx_rings; q++)
	{
		retval = rte_eth_rx_queue_setup(port, q, nb_rxd,
										rte_eth_dev_socket_id(port), NULL, mbuf_pool);
		if (retval < 0)
			return retval;
	}

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

int find_id_qp(uint32_t qp) {
	int id = -1;
	//printf("n - qp %d qp %d qpx %x n-qpx %x\n",ntohl(qp),qp,qp, ntohl(qp)>>8);
	for (int i = 0; i < TOTAL_ENTRY; i++)
	{
		if (Connection_States[i].ctsqp == qp ||
			Connection_States[i].stcqp == qp)
		{
			id = Connection_States[i].id;
			break;
		}
	}
	return id;
}

int find_id(struct rte_mbuf *buf) {
	struct roce_v2_header * rh = get_roce_hdr(buf);
	return find_id_qp(rh->dest_qp);
}

//fast id finder
#define ID_SPACE 1<<24
int32_t fast_id_lookup[ID_SPACE];


inline uint32_t qp_id_hash(uint32_t qp) {
	return ntohl(qp)>>8;
}

void set_fast_id(uint32_t qp, uint32_t id) {
	fast_id_lookup[qp_id_hash(qp)] = id;
}

int fast_find_id_qp(uint32_t qp) {
	return fast_id_lookup[qp_id_hash(qp)];
}

int fast_find_id(struct rte_mbuf * buf) {
	struct roce_v2_header * rh = get_roce_hdr(buf);
	return fast_find_id_qp(rh->dest_qp);
}

void init_fast_find_id(void) {
	for (int i=0;i<ID_SPACE;i++){
		fast_id_lookup[i]=-1;
	}
}

void populate_fast_find_id(void) {
	printf("start populate\n");
	for (int i = 0; i < TOTAL_CLIENTS; i++)
	{
		set_fast_id(Connection_States[i].ctsqp,i);
		set_fast_id(Connection_States[i].stcqp,i);
	}
	printf("end populate\n");
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
	printf("[core %d][id %3d][seq %5d][op:%19s][key %4d][msn %5d][size: %4d][dst: %d](pkt %d)\n", rte_lcore_id(), id, seq, op,key, msn, size, dest_qp, packet_counter);
}

struct rte_ipv4_hdr *ipv4_hdr_process(struct rte_ether_hdr *eth_hdr)
{
	struct rte_ipv4_hdr *ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	int hdr_len = (ipv4_hdr->version_ihl & RTE_IPV4_HDR_IHL_MASK) * RTE_IPV4_IHL_MULTIPLIER;
	if (hdr_len == sizeof(struct rte_ipv4_hdr))
	{
#ifdef TURN_PACKET_AROUND
		//Swap ipv4 addr
		uint32_t temp_ipv4_addr = ipv4_hdr->src_addr;
		ipv4_hdr->src_addr = ipv4_hdr->dst_addr;
		ipv4_hdr->dst_addr = temp_ipv4_addr;
#endif
		return ipv4_hdr;
	}
	return NULL;
}

struct rte_udp_hdr *udp_hdr_process(struct rte_ipv4_hdr *ipv4_hdr)
{
	struct rte_udp_hdr *udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	if (ipv4_hdr->next_proto_id == IPPROTO_UDP)
	{
		return udp_hdr;
	}
	return NULL;
}

struct roce_v2_header *roce_hdr_process(struct rte_udp_hdr *udp_hdr)
{
	//Dont start parsing if the udp port is not roce
	struct roce_v2_header *roce_hdr = NULL;
	if (likely(rte_be_to_cpu_16(udp_hdr->dst_port) == ROCE_PORT))
	{
		roce_hdr = (struct roce_v2_header *)((uint8_t *)udp_hdr + sizeof(struct rte_udp_hdr));
		return roce_hdr;
	}
	return NULL;
}

struct clover_hdr *mitsume_msg_process(struct roce_v2_header *roce_hdr)
{
	struct clover_hdr *clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));
	return clover_header;
}

int accept_packet(struct rte_mbuf *pkt)
{
	struct rte_ether_hdr *eth_hdr;
	struct rte_ipv4_hdr *ipv4_hdr;
	struct rte_udp_hdr *udp_hdr;
	struct roce_v2_header *roce_hdr;

	eth_hdr = eth_hdr_process(pkt);
	if (unlikely(eth_hdr == NULL))
	{
		log_printf(DEBUG, "ether header not the correct format dropping packet\n");
		rte_pktmbuf_free(pkt);
		return 0;
	}

	ipv4_hdr = ipv4_hdr_process(eth_hdr);
	if (unlikely(ipv4_hdr == NULL))
	{
		log_printf(DEBUG, "ipv4 header not the correct format dropping packet\n");
		rte_pktmbuf_free(pkt);
		return 0;
	}

	udp_hdr = udp_hdr_process(ipv4_hdr);
	if (unlikely(udp_hdr == NULL))
	{
		log_printf(DEBUG, "udp header not the correct format dropping packet\n");
		rte_pktmbuf_free(pkt);
		return 0;
	}

	roce_hdr = roce_hdr_process(udp_hdr);
	if (unlikely(roce_hdr == NULL))
	{
		log_printf(DEBUG, "roceV2 header not correct dropping packet\n");
		rte_pktmbuf_free(pkt);
		return 0;
	}
	return 1;
}

rte_atomic16_t thread_barrier;
rte_atomic16_t thread_barrier2;
void all_thread_barrier(rte_atomic16_t *barrier) {
	printf("stalling on thread %d\n",rte_lcore_id());
	rte_atomic16_add(barrier,1);
	while(barrier->cnt < (int16_t)rte_lcore_count()) {}
}

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

	printf("Running lcore main\n");
	printf("Client Threads %d\n", TOTAL_CLIENTS);
	printf("Keyspace %d\n", KEYSPACE);

	/* Run until the application is quit or killed. */
	for (;;)
	{
		/*
		 * Receive packets on a port and forward them on the paired
		 * port. The mapping is 0 -> 1, 1 -> 0, 2 -> 3, 3 -> 2, etc.
		 */
		RTE_ETH_FOREACH_DEV(port)
		{
			/* Get burst of RX packets, from first and only port */

			struct rte_mbuf *rx_pkts[BURST_SIZE];
			struct rte_mbuf *tx_pkts[BURST_SIZE*PACKET_INFLATION];
			uint32_t to_tx = 0;

			uint32_t queue = rte_lcore_id() / 2;
			const uint16_t nb_rx = rte_eth_rx_burst(port, queue, rx_pkts, BURST_SIZE);
			//if (nb_rx > 0) {
			//	printf("[core %d] rx %d\n",rte_lcore_id(),nb_rx);
			//}


			if (unlikely(nb_rx == 0))
				continue;


			#define PRINT_COUNT 10000
			for (uint16_t i = 0; i < nb_rx; i++)
			{
				if (likely(i < nb_rx - 1))
				{
					rte_prefetch0(rte_pktmbuf_mtod(rx_pkts[i + 1], void *));
				}
				packet_counter++;

				#ifdef TAKE_MEASUREMENTS
				sum_processed_data(rx_pkts[i]);
				#endif

				#ifdef WRITE_STEER
				true_classify(rx_pkts[i]);
				#endif


				#ifdef MAP_QP
				struct map_packet_response mpr;
				#ifdef PRINT_PACKET_BUFFERING
				print_packet_lite(rx_pkts[i]);
				#endif
				mpr = map_qp(rx_pkts[i]);
				for (uint32_t j = 0; j < mpr.size; j++)
				{
					tx_pkts[to_tx] = mpr.pkts[j];
					to_tx++;
				}

				#else
				tx_pkts[i]=rx_pkts[i];
				to_tx++;
				#endif
			}

			#ifdef PRINT_PACKET_BUFFERING
			printf("----------------------------------------------------\n");
			//bulk sending

			for (uint16_t i = 0; i< to_tx;i++){
				print_packet_lite(tx_pkts[i]);
			}
			printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");


			#endif
			#ifndef MAP_QP
			rte_eth_tx_burst(port, queue, tx_pkts, to_tx);
			#else
			enqueue_finish_mem_pkt_bulk(tx_pkts,to_tx);
			dequeue_finish_mem_pkt_bulk_full(port,queue);
			#endif

			#ifdef TAKE_MEASUREMENTS
			//if(has_mapped_qp){
			//	calculate_in_flight(&Connection_States);
			//}
			#endif

			if (unlikely((has_mapped_qp ==0) && fully_qp_init()))
			{
				all_thread_barrier(&thread_barrier);
				lock_qp();
				printf("core %d is flipping the switch\n",rte_lcore_id());
				//flip the switch
				print_first_mapping();

				//start doing fast operations now
				populate_fast_find_id();
				has_mapped_qp=1;
				unlock_qp();
			}


		}
	}
}


int coretest(__attribute__((unused)) void *arg)
{
	printf("I'm actually running on core %d\n", rte_lcore_id());
	lcore_main();
	return 1;
}

void fork_lcores(void)
{
	printf("Running on #%d cores\n", rte_lcore_count());
	int lcore;
	RTE_LCORE_FOREACH_SLAVE(lcore)
	{
		printf("running core %d\n", lcore);
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

void create_ack_mem_pool(void) {
	//TODO create an mbuf pool per core

	#define ACK_POOL_SIZE 128
	#define ACK_CACHE_SIZE 64
	mbuf_pool_ack = rte_pktmbuf_pool_create("MBUF_POOL_ACK", ACK_POOL_SIZE,
										ACK_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

	printf("ack create error %d\n",rte_errno);
	error_switch();

	if (mbuf_pool_ack == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf ack pool\n");
}

/*
 * The main function, which does initialization and calls the per-lcore
 * functions.
 */
int main(int argc, char *argv[])
{
	struct rte_mempool *mbuf_pool;
	unsigned nb_ports;
	uint16_t portid;

	/* Initialize the Environment Abstraction Layer (EAL). */
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

	argc -= ret;
	argv += ret;

	/* Check that there is an even number of ports to send/receive on. */
	nb_ports = rte_eth_dev_count_avail();
	printf("num ports:%u\n", nb_ports);

	/* Creates a new mempool in memory to hold the mbufs. */
	//TODO create an mbuf pool per core
	mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * nb_ports,
										MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

	/* Initialize all ports. */
	RTE_ETH_FOREACH_DEV(portid)
	if (port_init(portid, mbuf_pool, rte_lcore_count()) != 0)
		rte_exit(EXIT_FAILURE, "Cannot init port %" PRIu16 "\n",
				 portid);

	if (rte_lcore_count() > 1)
		printf("\nWARNING: Too many lcores enabled. Only 1 used.\n");

	create_ack_mem_pool();

	printf("master core %d\n", rte_get_master_lcore());

	if (init == 0)
	{
		bzero(first_write, KEYSPACE * sizeof(uint64_t));
		bzero(second_write, TOTAL_ENTRY * KEYSPACE * sizeof(uint64_t));
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
		bzero(cached_write_vaddrs, KEYSPACE * WRITE_VADDR_CACHE_SIZE * sizeof(uint64_t));
		bzero(writes_per_key, KEYSPACE * sizeof(uint32_t));
		bzero(cached_write_vaddr_mod, sizeof(uint64_t) * HASHSPACE);
		bzero(cached_write_vaddr_mod_lookup, sizeof(uint64_t) * HASHSPACE);
		bzero(cached_write_vaddr_mod_latest, sizeof(uint64_t) * KEYSPACE);
#endif

		rte_rwlock_init(&next_lock);
		rte_rwlock_init(&qp_lock);
		rte_rwlock_init(&qp_init_lock);
		rte_rwlock_init(&mem_qp_lock);

		init_reorder_buf();
		init_connection_states();
		init_hash();
		init_fast_find_id();
		write_value_packet_size = 0;
		predict_shift_value = 0;
		has_mapped_qp = 0;
		init = 1;
	}

	fork_lcores();
	lcore_main();
	return 0;
}