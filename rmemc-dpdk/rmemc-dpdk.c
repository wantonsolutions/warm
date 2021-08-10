/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2015 Intel Corporation
 */

#include <stdint.h>
#include <stdarg.h>
#include <signal.h>
#include <inttypes.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
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
#include <arpa/inet.h>

#include <rte_jhash.h>
#include <rte_hash.h>
#include <rte_table.h>

#include <endian.h>
#include <zlib.h>


#define KEYSPACE 1024
#define CACHE_KEYSPACE 1024
#define SEQUENCE_NUMBER_SHIFT 256
#define TOTAL_CLIENTS MITSUME_BENCHMARK_THREAD_NUM
int MAP_QP = 1;
int MOD_SLOT = 1;

//#define DATA_PATH_PRINT
//#define MAP_PRINT
#define COLLECT_GARBAGE
#define CATCH_ECN

uint32_t debug_start_printing_every_packet = 0;

static int has_mapped_qp = 0;
static int packet_counter = 0;

#define READ_STEER
#define WRITE_VADDR_CACHE_SIZE 16

uint64_t cached_write_vaddrs[KEYSPACE][WRITE_VADDR_CACHE_SIZE];
uint32_t writes_per_key[KEYSPACE];

#define HASHSPACE (1 << 22)
uint64_t cached_write_vaddr_mod[HASHSPACE];
uint64_t cached_write_vaddr_mod_lookup[HASHSPACE];
uint64_t cached_write_vaddr_mod_latest[KEYSPACE];

rte_rwlock_t next_lock;
rte_rwlock_t qp_lock;
rte_rwlock_t qp_init_lock;
rte_rwlock_t mem_qp_lock;

void lock_qp() {
	rte_rwlock_write_lock(&qp_lock);
	rte_smp_mb();
}

void unlock_qp() {
	rte_rwlock_write_unlock(&qp_lock);
	rte_smp_mb();
}

void lock_mem_qp() {
	rte_rwlock_write_lock(&mem_qp_lock);
	rte_smp_mb();
}

void unlock_mem_qp() {
	rte_rwlock_write_unlock(&mem_qp_lock);
	rte_smp_mb();
}

void lock_next() {
	rte_rwlock_write_lock(&next_lock);
	rte_smp_mb();
}

void unlock_next() {
	rte_rwlock_write_unlock(&next_lock);
	rte_smp_mb();
}

static struct rte_hash_parameters qp2id_params = {
	.name = "qp2id",
    .entries = TOTAL_ENTRY,
    .key_len = sizeof(uint32_t),
    .hash_func = rte_jhash,
    .hash_func_init_val = 0,
    .socket_id = 0,
};

struct Connection_State Connection_States[TOTAL_ENTRY];
struct rte_hash* qp2id_table;
static uint32_t qp_id_counter=0;
uint32_t qp_values[TOTAL_ENTRY];
uint32_t id_qp[TOTAL_ENTRY];

#define HASH_RETURN_IF_ERROR(handle, cond, str, ...) do {                \
    if (cond) {                         \
        printf("ERROR line %d: " str "\n", __LINE__, ##__VA_ARGS__); \
        if (handle) rte_hash_free(handle);          \
        return -1;                      \
    }                               \
} while(0)

//Keys start at 1, so I'm subtracting 1 to make the first key equal to index
//zero.  qp_id_counter is the total number of qp that can be written to. So here
//we are just taking all of the keys and wrapping them around so the first key
//goes to the first qp, and the qp_id_counter + 1  key goes to the first qp.
uint32_t key_to_qp(uint64_t key) {
	uint32_t index = (key)%qp_id_counter;
	return id_qp[index];
}

int init_hash(void) {
	qp2id_table = rte_hash_create(&qp2id_params);
	HASH_RETURN_IF_ERROR(qp2id_table, qp2id_table == NULL, "qp2id_table creation failed");
	return 0;
}

int set_id(uint32_t qp, uint32_t id) {
	#ifdef DATA_PATH_PRINT
	log_printf(DEBUG,"adding (%d,%d) to hash table\n",qp,id);
	printf("adding (%d,%d) to hash table\n",qp,id);
	#endif
	qp_values[id]=id;
	id_qp[id]=qp;
	int ret = rte_hash_add_key_data(qp2id_table,&qp,&qp_values[id]);
	HASH_RETURN_IF_ERROR(qp2id_table, ret < 0, "unable to add new qp id (%d,%d)\n",qp,id);
	return  ret;
}

//Warning this is a very unsafe function. Only call it when you know that a
//packet corresponds to an ID that has an established QP. If the ID is not set,
//this will set it. Otherwise the ID is returned.
uint32_t get_id(uint32_t qp) {
	uint32_t* return_value;
	uint32_t id;
	int ret = rte_hash_lookup_data(qp2id_table,&qp,(void **)&return_value);
	if (ret < 0) {
		id = qp_id_counter;
		log_printf(DEBUG,"no such id exists yet adding qp id pq: %d id: %d\n",qp, id);
		printf("no such id exists yet adding qp id pq: %d id: %d\n",qp, id);
		set_id(qp,id);
		qp_id_counter++;
	} else {
		id = *return_value;
	}
	id_colorize(id);

	return id;
}

int fully_qp_init() {
	for (int i=0;i<TOTAL_CLIENTS;i++) {
		struct Connection_State cs = Connection_States[i];
		if (!cs.sender_init || !cs.receiver_init) {
			return 0;
		}
	}
	return 1;
}

#define PKT_REORDER_BUF 64
struct rte_mbuf * mem_qp_buf[TOTAL_ENTRY][PKT_REORDER_BUF];
struct rte_mbuf * client_qp_buf[TOTAL_ENTRY][PKT_REORDER_BUF];
uint64_t mem_qp_buf_head[TOTAL_ENTRY];
uint64_t mem_qp_buf_tail[TOTAL_ENTRY];
uint64_t client_qp_buf_head[TOTAL_ENTRY];
uint64_t client_qp_buf_tail[TOTAL_ENTRY];

void init_reorder_buf(void) {
	printf("initalizing reorder buffs");
	bzero(mem_qp_buf_head,TOTAL_ENTRY*sizeof(uint64_t));
	bzero(mem_qp_buf_head,TOTAL_ENTRY*sizeof(uint64_t));
	bzero(client_qp_buf_tail,TOTAL_ENTRY*sizeof(uint64_t));
	bzero(client_qp_buf_tail,TOTAL_ENTRY*sizeof(uint64_t));
	for (int i=0;i<TOTAL_ENTRY;i++) {
		for (int j=0;j<PKT_REORDER_BUF;j++) {
			mem_qp_buf[i][j] = NULL;
			client_qp_buf[i][j] = NULL;
		}
	}
}

void finish_mem_pkt(struct rte_mbuf *pkt, uint16_t port, uint32_t queue) {
	struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	struct rte_ipv4_hdr* ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	struct rte_udp_hdr * udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	struct roce_v2_header * roce_hdr = (struct roce_v2_header *)((uint8_t*)udp_hdr + sizeof(struct rte_udp_hdr));
	struct clover_hdr * clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));

	#define FAKE_ID -1
	int id = FAKE_ID;
	uint32_t *head = NULL;
	uint32_t *tail = NULL;
	struct rte_mbuf * (*buf_ptr)[TOTAL_ENTRY][PKT_REORDER_BUF];
	lock_mem_qp();

	//Find the ID of the packet We are using generic head and tail pointers here
	//for both directions of queueing.  The memory direction has a queue and so
	//does the client.  If the requests are arriving out of order in either
	//direction they will be queued.
	for (int i=0;i<qp_id_counter;i++) {
		if (Connection_States[i].ctsqp == roce_hdr->dest_qp) {
				id = Connection_States[i].id;
				head = &mem_qp_buf_head[id];
				tail = &mem_qp_buf_tail[id];
				buf_ptr = &mem_qp_buf;
				break;
		}
		if (Connection_States[i].receiver_init == 1 && Connection_States[i].stcqp == roce_hdr->dest_qp) {
				id = Connection_States[i].id;
				head = &client_qp_buf_head[id];
				tail = &client_qp_buf_tail[id];
				buf_ptr = &client_qp_buf;
				break;
		}
	}

	//If the id of the packet was not found, then just send the packet out.
	if (id == FAKE_ID) {
		rte_eth_tx_burst(port, queue,&pkt, 1);
		unlock_mem_qp();
		return;
	}

	//Find the location in the circular buffer that hold the current packet
	uint32_t seq = readable_seq(roce_hdr->packet_sequence_number);
	uint32_t entry = seq%PKT_REORDER_BUF;

	//Write the packet to the buffer (direction independent)
	(*buf_ptr)[id][entry] = pkt;

	//On the first call the sequence numbers are going to start somewhere
	//random. In this case just move the head of the buffer to the current
	//sequence number
	if (unlikely(*head == 0)) {
		*head = seq;
	}

	//If the tail is the new latest sequence number than slide it forward
	if (*tail < seq) {
		*tail = seq;
	}

	//If the head is currently higher than the tail, this means that it's not
	//time to send anyhting. We are eitheir going to enqueue (the usual case) or
	//there is nothing to do.
	if (*head > *tail) {
		unlock_mem_qp();
		return;
	}

	//I'm not sure why this would happen
	if (seq < *head) {
		printf("we have a problem, perhaps the sequence numbers rolled over?\n");
		exit(0);
	}

	//make sure that all of the entries from the head to the tail are not equal
	//to null. If any are then we have non-contigous sequence numbers i.e a gap,
	//and need to move forward without sending anything.
	for (int i=*head;i<=*tail;i++){
		if ((*buf_ptr)[id][i%PKT_REORDER_BUF] == NULL) {
			unlock_mem_qp();
			//printf("[core %d] Returning due to hole in head(%d) -> tail(%d)\n",rte_lcore_id(),*head,*tail);
			//print_packet_lite(pkt);
			return;
		}
	}

	//If we made it here it's time to send
	uint32_t diff = (*tail+1)-*head;
	for (int i=*head;i<=*tail;i++){
		struct rte_mbuf * s_pkt = (*buf_ptr)[id][i%PKT_REORDER_BUF];
		#ifdef TAKE_MEASUREMENTS
		if (buf_ptr == &mem_qp_buf) {
			append_sequence_number(id,get_psn(s_pkt));
		}
		#endif

		rte_eth_tx_burst(port, queue, &s_pkt, 1);
		(*buf_ptr)[id][i%PKT_REORDER_BUF] = NULL;
	}
	*head = *tail + 1;
	//rte_eth_tx_burst(port, queue, &(*buf_ptr)[id][*head%PKT_REORDER_BUF], diff);
	unlock_mem_qp();
}

void copy_eth_addr(uint8_t *src, uint8_t *dst) {
	for (int i=0;i<6;i++) {
		dst[i] = src[i];
	}
}

void init_connection_state(struct rte_mbuf *pkt) {
	struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	struct rte_ipv4_hdr* ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	struct rte_udp_hdr * udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	struct roce_v2_header * roce_hdr = (struct roce_v2_header *)((uint8_t*)udp_hdr + sizeof(struct rte_udp_hdr));
	uint16_t udp_src_port= udp_hdr->src_port;
	uint32_t cts_dest_qp=roce_hdr->dest_qp;
	uint32_t seq=roce_hdr->packet_sequence_number;
	uint32_t rkey=get_rkey_rdma_packet(roce_hdr);
	uint32_t server_ip=ipv4_hdr->dst_addr;
	uint32_t client_ip=ipv4_hdr->src_addr;

	//Find the connection state if it exists
	struct Connection_State cs;
	int id = get_id(cts_dest_qp);
	cs = Connection_States[id];

	if (cs.sender_init != 0) {
		log_printf(DEBUG,"Connection State allready initialized DEST QP(%d)\n",cts_dest_qp);
		return;
	}

	cs.id = id;
	cs.seq_current = seq;
	cs.udp_src_port_client = udp_src_port;
	cs.ctsqp = cts_dest_qp;
	cs.cts_rkey = rkey;
	cs.ip_addr_client=client_ip;
	cs.ip_addr_server=server_ip;

	copy_eth_addr((uint8_t*)eth_hdr->s_addr.addr_bytes,(uint8_t*)cs.cts_eth_addr);
	copy_eth_addr((uint8_t*)eth_hdr->d_addr.addr_bytes,(uint8_t*)cs.stc_eth_addr);
	cs.sender_init=1;

	//These fields do not need to be set. They are for the second half of the algorithm
	//cs.stcqp = 0; //At init time the opposite side of the qp is going to be unknown
	//cs.udp_src_port_server = 0;
	//cs.mseq_current = 0;
	Connection_States[cs.id] = cs;
}

void init_cs_wrapper(struct rte_mbuf* pkt) {
	struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	struct rte_ipv4_hdr* ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	struct rte_udp_hdr * udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	struct roce_v2_header * roce_hdr = (struct roce_v2_header *)((uint8_t*)udp_hdr + sizeof(struct rte_udp_hdr));

	if (roce_hdr->opcode != RC_WRITE_ONLY && roce_hdr->opcode != RC_READ_REQUEST && roce_hdr->opcode != RC_CNS ) {
		printf("only init connection states for writers either WRITE_ONLY or CNS (and only for data path) exiting for safty");
		exit(0);
	}
	uint32_t rkey = get_rkey_rdma_packet(roce_hdr);
	init_connection_state(pkt);
}

//Calculate the MSN of the packet based on the offset. If the msn of the
//connection state is behind, update that as well.
uint32_t produce_and_update_msn(struct roce_v2_header* roce_hdr, struct Connection_State *cs) {
	uint32_t msn = htonl(ntohl(roce_hdr->packet_sequence_number) - ntohl(cs->mseq_offset));
	if (ntohl(msn) > ntohl(cs->mseq_current)) {
		cs->mseq_current = msn;
	}
	return cs->mseq_current;
}

//Update the server to clinet connection state based on roce and udp header.
//Search for the connection state first. If it's not found, just return.
//Otherwise update the MSN.
uint32_t find_and_update_stc(struct roce_v2_header *roce_hdr, struct rte_udp_hdr *udp_hdr) {
	struct Connection_State *cs;
	uint32_t found = 0;

	//check to see if this value has allready been set
	//Find the coonection
	for (int i=0;i<TOTAL_ENTRY;i++){
		cs = &Connection_States[i];
		//if (cs.seq_current == roce_hdr->packet_sequence_number) {
		if (cs->stcqp == roce_hdr->dest_qp && cs->receiver_init == 1) {
			id_colorize(cs->id);
			found = 1;
			break;
		}
	}
	if (found == 0) {
		return 0;
	}

	//at this point we have the correct cs
	uint32_t msn = produce_and_update_msn(roce_hdr,cs);
	return msn;
}

void find_and_set_stc(struct roce_v2_header *roce_hdr, struct rte_udp_hdr *udp_hdr) {
	struct Connection_State cs;
	int32_t matching_id = -1;
	uint32_t total_matches = 0;

	//Everything has been initlaized, return
	if (likely(has_mapped_qp != 0)) {
		return;
	}

	//Try to perform a basic update
	if (find_and_update_stc(roce_hdr,udp_hdr) > 0) {
		return;
	}

	//If we are here then the connection should not be initlaized yet	/ /
	//Find the coonection
	for (int i=0;i<TOTAL_ENTRY;i++){
		cs = Connection_States[i];
		//sender_init, should not be set if the connection state has not been completely inited
		if (cs.sender_init == 0) {
			continue;
		}

		//if (cs.seq_current == roce_hdr->packet_sequence_number) {
		if (cs.seq_current == roce_hdr->packet_sequence_number) {
			matching_id = i;
			total_matches++;
		}
	}

	//Safty to prevent colisions when the sequence numbers align
	if (total_matches > 1) {
		//printf("find and set STC collision, wait another round\n");
		return;
	}
	//Return if nothing is found
	if (total_matches != 1) {
		//printf("not able to find a running connection to update\n");
		return;
	}

	//initalize the first time
	cs = Connection_States[matching_id];
	if (cs.receiver_init==0) {
		id_colorize(cs.id);
		cs.stcqp = roce_hdr->dest_qp;
		cs.udp_src_port_server = udp_hdr->src_port;
		cs.mseq_current = get_msn(roce_hdr);
		cs.mseq_offset = htonl(ntohl(cs.seq_current) - ntohl(cs.mseq_current)); //still shifted by 256 but not in network order
		cs.receiver_init = 1;
		Connection_States[matching_id] = cs;
		return;
	} 
	return;
}

void find_and_set_stc_wrapper(struct roce_v2_header *roce_hdr, struct rte_udp_hdr *udp_hdr) {
	if (roce_hdr->opcode != RC_ACK && roce_hdr->opcode != RC_ATOMIC_ACK && roce_hdr->opcode != RC_READ_RESPONSE) {
		printf("Only find and set stc on ACKS, and responses");
		return;
	}
	find_and_set_stc(roce_hdr, udp_hdr);
}

void update_cs_seq(uint32_t stc_dest_qp, uint32_t seq) {
	uint32_t id = get_id(stc_dest_qp);
	struct Connection_State * cs =&Connection_States[id];
	if (cs->sender_init == 0) {
		printf("Attempting to set sequence number for non existant connection (exiting)");
		exit(0);
	}
	cs->seq_current = seq;
	#ifdef DATA_PATH_PRINT
	printf("Updated connection state based on sequence number\n");
	print_connection_state(cs);
	#endif
}

void update_cs_seq_wrapper(struct roce_v2_header *roce_hdr){
	if (roce_hdr->opcode != RC_WRITE_ONLY && roce_hdr->opcode != RC_CNS && roce_hdr->opcode != RC_READ_REQUEST) {
		printf("only update connection states for writers either WRITE_ONLY or CNS (and only for data path) exiting for safty");
		exit(0);
	}
	update_cs_seq(roce_hdr->dest_qp,roce_hdr->packet_sequence_number);
}


void cts_track_connection_state(struct rte_mbuf * pkt) {
	struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	struct rte_ipv4_hdr* ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	struct rte_udp_hdr * udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	struct roce_v2_header * roce_hdr = (struct roce_v2_header *)((uint8_t*)udp_hdr + sizeof(struct rte_udp_hdr));
	if (has_mapped_qp == 1) {
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
static uint64_t outstanding_write_vaddrs[TOTAL_ENTRY][KEYSPACE]; //outstanding vaddr values, used for replacing addrs
static uint64_t next_vaddr[KEYSPACE];
static uint64_t latest_key[TOTAL_ENTRY];

uint64_t get_latest_key(uint32_t id) {
	return latest_key[id];
}

void set_latest_key(uint32_t id, uint64_t key) {
	latest_key[id] = key;
}

static int init =0;
static uint32_t write_value_packet_size = 0;
static uint32_t predict_shift_value=0;
static uint32_t nacked_cns =0;

void init_connection_states(void) {
	bzero(Connection_States,TOTAL_ENTRY * sizeof(Connection_State));
	for (int i=0;i<TOTAL_ENTRY;i++) {
		struct Connection_State *source_connection;
		source_connection=&Connection_States[i];
		for (int j=0;j<CS_SLOTS;j++) {
			struct Request_Map * mapped_request;
			mapped_request = &(source_connection->Outstanding_Requests[j]);
			mapped_request->open = 1;
		}
	}
}

uint32_t mod_hash(uint64_t vaddr) {
	//uint32_t index = crc32(0xFFFFFFFF, &vaddr, 8) % HASHSPACE;
	//uint32_t index = ((vaddr >> 36) % HASHSPACE);
	uint32_t index = (ntohl(vaddr >> 32) % HASHSPACE);
	//printf("%"PRIx64" %d\n",vaddr, index);
	return index;
}

void update_write_vaddr_cache_mod(uint64_t key, uint64_t vaddr) {
	uint32_t index = mod_hash(vaddr);
	cached_write_vaddr_mod[index] = vaddr;
	cached_write_vaddr_mod_lookup[index] = key;
	cached_write_vaddr_mod_latest[key] = vaddr;
}

int does_read_have_cached_write_mod(uint64_t vaddr) {
	uint32_t index = mod_hash(vaddr);
	if (cached_write_vaddr_mod[index] == vaddr) {
		return cached_write_vaddr_mod_lookup[index];
	}
	return 0;
}

uint64_t get_latest_vaddr_mod(uint32_t key) {
	return cached_write_vaddr_mod_latest[key];
}

void update_write_vaddr_cache_ring(uint64_t key, uint64_t vaddr) {
	uint32_t cache_index = writes_per_key[key]%WRITE_VADDR_CACHE_SIZE;
	//printf("updating write for key %"PRIu64", vaddr %"PRIu64" slot %d\n",key,vaddr,cache_index);
	cached_write_vaddrs[key][cache_index] = vaddr;
	writes_per_key[key]++;
}

int does_read_have_cached_write_ring(uint64_t vaddr) {
	//for (int key=0;key<KEYSPACE;key++) {
	//TODO this is an error the last key will not be indexed here
	for (int key=1;key<CACHE_KEYSPACE;key++) {

		if(writes_per_key[key]==0) {
			continue;
		}
		//!TODO start at the last written index
		int index = writes_per_key[key]%WRITE_VADDR_CACHE_SIZE;
		for (uint32_t counter=0;counter<WRITE_VADDR_CACHE_SIZE;counter++) {
			//printf("INDEX %d counter %d\n",index, counter);
			if (cached_write_vaddrs[key][index] == vaddr) {
				//printf("found key %d add stored %"PRIu64" incomming %"PRIu64"\n",key,cached_write_vaddrs[key][index],vaddr);
				return key;
			}
			//TODO decrement to optimize
			index = (index-1);
			if (index < 0) {
				index = WRITE_VADDR_CACHE_SIZE -1;
			}
		}
	}
	return 0;
}

uint64_t get_latest_vaddr_ring(uint32_t key) {
	if (unlikely(writes_per_key[key] == 0)) {
		printf("Should not be rewriting something that has not been touched\n");
		return 0;
	}
	uint32_t cache_index = (writes_per_key[key]-1)%WRITE_VADDR_CACHE_SIZE;
	return cached_write_vaddrs[key][cache_index];
}

int (*does_read_have_cached_write)(uint64_t) = does_read_have_cached_write_mod;
void (*update_write_vaddr_cache)(uint64_t, uint64_t) = update_write_vaddr_cache_mod;
uint64_t (*get_latest_vaddr)(uint32_t) = get_latest_vaddr_mod;

void steer_read(struct rte_mbuf *pkt, uint32_t key) {
	struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	struct rte_ipv4_hdr* ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	struct rte_udp_hdr * udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	struct roce_v2_header * roce_hdr = (struct roce_v2_header *)((uint8_t*)udp_hdr + sizeof(struct rte_udp_hdr));
	struct clover_hdr * clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));
	struct read_request * rr = (struct read_request*) clover_header;

	uint64_t vaddr = get_latest_vaddr(key);

	if (vaddr == 0) {
		return;
	}

	//Set the current address to the cached address of the latest write if it is old
	if (rr->rdma_extended_header.vaddr != vaddr) {
		rr->rdma_extended_header.vaddr = vaddr;
		//printf("Re routing key %d for cached index %d\n",key,cache_index);
		//With the vaddr updated redo the checksum
		recalculate_rdma_checksum(pkt);
		#ifdef TAKE_MESUREMENTS
		read_redirected();
		#endif
	}
}

uint32_t slot_is_open(struct Request_Map *rm) {
	if (rm->open == 1) {
		return 1;
	} else {
		return 0;
	}
}

void close_slot(struct Request_Map* rm) {
	rm->open=0;
}

void open_slot(struct Request_Map* rm) {
	rm->open=1;
}

uint32_t mod_slot(uint32_t seq) {
	return readable_seq(seq) % CS_SLOTS; 
}

struct Request_Map * get_empty_slot_mod(struct Connection_State *cs) {
	uint32_t slot_num = mod_slot(cs->seq_current);
	//printf("(%d,%d)\n",cs->id,slot_num);
	struct Request_Map * slot = &(cs->Outstanding_Requests[slot_num]);
	if (!slot_is_open(slot)) {
		printf("CLOSED SLOT, this is really bad!!\n");
		//print_request_map(slot);
		//exit(0);
	}
	//open anyways
	open_slot(slot);
	return slot;
}

uint32_t qp_is_mapped(uint32_t qp) {
	uint32_t is_mapped = 0;
	for(int i=0;i<qp_id_counter;i++){
		if (id_qp[i]==qp) {
			return 1;
		}
	}
	return 0;
}

void map_qp_forward(struct rte_mbuf * pkt, uint64_t key) {
	struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	struct rte_ipv4_hdr* ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	struct rte_udp_hdr * udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	struct roce_v2_header * roce_hdr = (struct roce_v2_header *)((uint8_t*)udp_hdr + sizeof(struct rte_udp_hdr));
	struct clover_hdr * clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));
	
	
	uint32_t r_qp= roce_hdr->dest_qp;
	uint32_t id = get_id(r_qp);
	uint32_t n_qp = 0;

	//Keys are set to 0 when we are not going to map them. If the key is not
	//equal to zero apply the mapping policy.
	if (key != 0) {
		n_qp = key_to_qp(key);
	} else {
		n_qp = roce_hdr->dest_qp;
	} 

	//Find the connection state of the mapped destination connection.
	struct Connection_State *destination_connection;
	destination_connection=&Connection_States[get_id(n_qp)];
	if (destination_connection == NULL) {
		printf("I did not want to end up here\n");
		return;
	}


	//Our middle box needs to keep track of the sequence number
	//that should be tracked for the destination. This should
	//always be an increment of 1 from it's previous number.
	//Here we increment the sequence number
	destination_connection->seq_current = htonl(ntohl(destination_connection->seq_current) + SEQUENCE_NUMBER_SHIFT); //There is bit shifting here.

	#ifdef MAP_PRINT
	uint32_t msn = Connection_States[id].mseq_current;
	printf("MAP FRWD(key %"PRIu64") (id %d) (core %d) (op: %s) :: (%d -> %d) (%d) (qpo %d -> qpn %d) \n",key, id,rte_lcore_id(), ib_print_op(roce_hdr->opcode), readable_seq(roce_hdr->packet_sequence_number), readable_seq(destination_connection->seq_current), readable_seq(msn), roce_hdr->dest_qp, destination_connection->ctsqp);
	#endif

	//Multiple reads occur at once. We don't want them to overwrite
	//eachother. Initally I used Oustanding_Requests[id] to hold
	//requests. But this only ever used the one index. On parallel reads
	//the sequence number over write the old one because they collied.
	//To save time, I'm using the old extra space TOTAL_ENTRIES in the
	//Outstanding Requests to hold the concurrent reads. These should really be hased in the future.
	
	//Search for an open slot
	struct Request_Map* slot;
	slot = get_empty_slot_mod(destination_connection);

	//The next step is to save the data from the current packet
	//to a list of outstanding requests. As clover is blocking
	//we can use the id to track which sequence number belongs
	//to which request. These values will be used to map
	//responses back.
	close_slot(slot);
	slot->id=id;
	//Save a unique id of sequence number and the qp that this response will arrive back on
	slot->mapped_sequence=destination_connection->seq_current;
	slot->mapped_destination_server_to_client_qp=destination_connection->stcqp;
	slot->original_sequence=roce_hdr->packet_sequence_number;
	//Store the server to client to qp that this we will need to make the swap
	slot->server_to_client_qp=Connection_States[id].stcqp;
	slot->server_to_client_udp_port=Connection_States[id].udp_src_port_server;
	slot->original_src_ip=ipv4_hdr->src_addr;
	slot->server_to_client_rkey=Connection_States[id].stc_rkey;

	copy_eth_addr(eth_hdr->s_addr.addr_bytes, slot->original_eth_addr);
	
	//Set the packet with the mapped information to the new qp
	roce_hdr->dest_qp = destination_connection->ctsqp;
	roce_hdr->packet_sequence_number = destination_connection->seq_current;
	udp_hdr->src_port = destination_connection->udp_src_port_client;

	//print_ip_header(ipv4_hdr);
	ipv4_hdr->src_addr = destination_connection->ip_addr_client;
	ipv4_hdr->hdr_checksum = 0;
	ipv4_hdr->hdr_checksum = rte_ipv4_cksum(ipv4_hdr);

	recalculate_rdma_checksum(pkt);
}


struct Connection_State * find_connection(struct rte_mbuf* pkt) {
	struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	struct rte_ipv4_hdr* ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	struct rte_udp_hdr * udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	struct roce_v2_header * roce_hdr = (struct roce_v2_header *)((uint8_t*)udp_hdr + sizeof(struct rte_udp_hdr));
	struct clover_hdr * clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));

	//We have very little information on the demultiplex side. We need to search
	//through the entire list in order to determine if this sequence number was
	//cached. We don't know the key, or the original sender so we can't do the
	//reverse lookup. In this case we have to pass over the whole list of
	//connections, then we have to look through each entry in each connection to
	//determine if there is an outstanding request.

	struct Connection_State *source_connection = NULL;
	for (int i=0;i<TOTAL_ENTRY;i++) {
		//This optimization is safe because we should not ever have more than qp
		//enteries in the Connection State list. However it uses knowledge not local to this funtion.
		if (unlikely(i > qp_id_counter)) {
			source_connection=NULL;
			break;
		}
		source_connection=&Connection_States[i];
		if (source_connection->stcqp == roce_hdr->dest_qp && source_connection->ip_addr_client == ipv4_hdr->dst_addr) {
			break;
		}
		source_connection=NULL;
	}
	return source_connection;
}

struct Request_Map * find_slot(struct Connection_State * source_connection, struct rte_mbuf *pkt) {

	struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	struct rte_ipv4_hdr* ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	struct rte_udp_hdr * udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	struct roce_v2_header * roce_hdr = (struct roce_v2_header *)((uint8_t*)udp_hdr + sizeof(struct rte_udp_hdr));
	struct clover_hdr * clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));

	uint32_t search_sequence_number = roce_hdr->packet_sequence_number;
	struct Request_Map * mapped_request;

	for (int j=0;j<CS_SLOTS;j++) {
		mapped_request = &(source_connection->Outstanding_Requests[j]);
		//printf("looking for the outstanding request %d::: (maped,search) (%d,%d)\n",j,mapped_request->mapped_sequence,search_sequence_number);
		//First search to find if the sequence numbers match
		if ((!slot_is_open(mapped_request)) && mapped_request->mapped_sequence == search_sequence_number) {
			return mapped_request;
		}
	}
	return NULL;
}

struct Request_Map * find_slot_mod(struct Connection_State * source_connection, struct rte_mbuf *pkt) {

	struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	struct rte_ipv4_hdr* ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	struct rte_udp_hdr * udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	struct roce_v2_header * roce_hdr = (struct roce_v2_header *)((uint8_t*)udp_hdr + sizeof(struct rte_udp_hdr));
	struct clover_hdr * clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));

	uint32_t search_sequence_number = roce_hdr->packet_sequence_number;
	uint32_t slot_num = mod_slot(search_sequence_number);
	struct Request_Map * mapped_request = &(source_connection->Outstanding_Requests[slot_num]);

	//First search to find if the sequence numbers match
	if ((!slot_is_open(mapped_request)) && mapped_request->mapped_sequence == search_sequence_number) {
		return mapped_request;
	}
	return NULL;
}

//Mappping qp backwards is the demultiplexing operation.  The first step is to
//identify the kind of packet and figure out if it has been placed on the
struct map_packet_response map_qp_backwards(struct rte_mbuf* pkt) {
	struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	struct rte_ipv4_hdr* ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	struct rte_udp_hdr * udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	struct roce_v2_header * roce_hdr = (struct roce_v2_header *)((uint8_t*)udp_hdr + sizeof(struct rte_udp_hdr));
	struct clover_hdr * clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));


	struct map_packet_response mpr;
	mpr.pkts[0] = pkt;
	mpr.size = 1;

	if (unlikely(has_mapped_qp ==  0 )) {
		printf("We have not started multiplexing yet. Returning with no packet modifictions.\n");
		return;
	}

	struct Connection_State *source_connection = find_connection(pkt);
	if (source_connection == NULL) {
		//This packet is not part of an activly mapped connection
		return mpr;
	}

	struct Request_Map * mapped_request;
	if (MOD_SLOT) {
		mapped_request = find_slot_mod(source_connection,pkt);
	} else {
		mapped_request = find_slot(source_connection,pkt);
	}
	//struct Request_Map * 
	if (mapped_request != NULL) {
		//Set the packety headers to that of the mapped request
		roce_hdr->dest_qp = mapped_request->server_to_client_qp;
		roce_hdr->packet_sequence_number = mapped_request->original_sequence;
		udp_hdr->src_port = mapped_request->server_to_client_udp_port;
		//set_rkey_rdma_packet(roce_hdr,mapped_request->server_to_client_rkey);

		//Update the tracked msn this requires adding to it, and then storing back to the connection states
		//To do this we need to take a look at what the original connection was so that we can update it accordingly.
		struct Connection_State * destination_cs = &Connection_States[mapped_request->id];
		uint32_t msn = produce_and_update_msn(roce_hdr,destination_cs);
		#ifdef MAP_PRINT
		uint32_t packet_msn = get_msn(roce_hdr);
		id_colorize(mapped_request->id);
		printf("        MAP BACK :: (core %d) seq(%d <- %d) mseq(%d <- %d) (op %s) (s-qp %d)\n",rte_lcore_id(),readable_seq(mapped_request->original_sequence),readable_seq(mapped_request->mapped_sequence), readable_seq(msn), readable_seq(packet_msn),ib_print_op(roce_hdr->opcode), roce_hdr->dest_qp);
		#endif
		
		set_msn(roce_hdr,msn);

		//Ip mapping recalculate the ip checksum
		ipv4_hdr->dst_addr = mapped_request->original_src_ip;
		ipv4_hdr->hdr_checksum = 0;
		ipv4_hdr->hdr_checksum = rte_ipv4_cksum(ipv4_hdr);

		//copy_eth_addr(eth_hdr->d_addr.addr_bytes,mapped_request->original_eth_addr);
		copy_eth_addr(mapped_request->original_eth_addr,eth_hdr->d_addr.addr_bytes);

		//re ecalculate the checksum
		uint32_t crc_check =csum_pkt_fast(pkt); //This need to be added before we can validate packets
		void * current_checksum = (void *)((uint8_t *)(ipv4_hdr) + ntohs(ipv4_hdr->total_length) - 4);
		memcpy(current_checksum,&crc_check,4);

		//repoen the slot
		open_slot(mapped_request);
		return mpr;
	}

	uint32_t msn = find_and_update_stc(roce_hdr,udp_hdr);
	if (msn > 0) {
		uint32_t packet_msn = get_msn(roce_hdr);
		if (packet_msn == -1 ) {
			printf("How did we get here?\n");
		}
		printf("@@@@ NO ENTRY TRANSITION @@@@ :: (seq %d) mseq(%d <- %d) (op %s) (s-qp %d)\n",readable_seq(roce_hdr->packet_sequence_number), readable_seq(msn), readable_seq(packet_msn),ib_print_op(roce_hdr->opcode), roce_hdr->dest_qp);
		set_msn(roce_hdr,msn);
		recalculate_rdma_checksum(pkt);
	}
	return mpr;
}


struct map_packet_response map_qp(struct rte_mbuf * pkt) {
	//Return if not mapping QP !!!THIS FEATURE SHOULD TURN ON AND OFF EASILY!!!

	struct map_packet_response mpr;
	mpr.pkts[0] = pkt;
	mpr.size = 1;

	if (MAP_QP == 0) {
		return mpr;
	}

	track_qp(pkt);

	//Not mapping yet
	if (has_mapped_qp == 0) {
		return mpr;
	}

	struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	struct rte_ipv4_hdr* ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	struct rte_udp_hdr * udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	struct roce_v2_header * roce_hdr = (struct roce_v2_header *)((uint8_t*)udp_hdr + sizeof(struct rte_udp_hdr));
	struct clover_hdr * clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));
	uint32_t size = ntohs(ipv4_hdr->total_length)
;
	uint8_t opcode = roce_hdr->opcode;
	uint32_t r_qp= roce_hdr->dest_qp;

	//backward path requires little checking
	if (opcode == RC_ACK || opcode == RC_ATOMIC_ACK || opcode == RC_READ_RESPONSE) {
		mpr = map_qp_backwards(pkt);
		return mpr;
	}

	if (qp_is_mapped(r_qp) == 0) {
		//This is not a packet we should map forward
		return mpr;
	}

	if (opcode == RC_READ_REQUEST) {
		uint32_t stub_key = 0;
		uint32_t *key = &stub_key;

		#ifdef READ_STEER
		struct read_request * rr = (struct read_request*) clover_header;
		//printf("DMA LEN %d\n", ntohl(dma_len));
		*key = (*does_read_have_cached_write)(rr->rdma_extended_header.vaddr);
		//uint32_t dma_len = ntohl(rr->rdma_extended_header.dma_length);
		//if (*key && dma_len == 1024) {
		if (*key) {
			steer_read(pkt,*key);
		} else {
			*key=0;
			#ifdef TAKE_MEASUREMENTS
			read_not_cached();
			#endif
		}
		#endif
		map_qp_forward(pkt, *key);

	}  else if (opcode == RC_WRITE_ONLY) {
		struct write_request * wr = (struct write_request*) clover_header;
		uint64_t *key = (uint64_t*)&(wr->data);
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
		if (size == 68) {
			uint32_t id = get_id(roce_hdr->dest_qp);
			*key = get_latest_key(id);
			if (*key < 1 || *key > KEYSPACE) {
				printf("danger zone\n");
				*key = 0;
			}
		}
		map_qp_forward(pkt,*key);
	} else if (opcode == RC_CNS) {
		uint32_t id = get_id(roce_hdr->dest_qp);
		map_qp_forward(pkt,get_latest_key(id));
	}
	return mpr;
}

int should_track(struct rte_mbuf * pkt) {
//void true_classify(struct rte_ipv4_hdr *ip, struct roce_v2_header *roce, struct clover_hdr * clover) {
	struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	struct rte_ipv4_hdr* ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	struct rte_udp_hdr * udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	struct roce_v2_header * roce_hdr = (struct roce_v2_header *)((uint8_t*)udp_hdr + sizeof(struct rte_udp_hdr));
	struct clover_hdr * clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));

	uint32_t size = ntohs(ipv4_hdr->total_length);
	uint8_t opcode = roce_hdr->opcode;
	uint32_t r_qp= roce_hdr->dest_qp;

	if (opcode == RC_ACK) {
		return 1;
	} 
	if (size == 60 && opcode == RC_READ_REQUEST) {
		return 1;
	}
	if ((size == 56 || size == 1072) && opcode == RC_READ_RESPONSE) {
		return 1;
	}
	if (opcode == RC_WRITE_ONLY) {
		if (size == 68 && (qp_is_mapped(r_qp) == 1)) {
			return 1;
		} else if (size == 1084) {
			return 1;
		} 
	}
    if (opcode == RC_ATOMIC_ACK) {
		return 1;
	}
	if (size == 72 && opcode == RC_CNS) {
		return 1;
	}
	return 0;
}



void track_qp(struct rte_mbuf * pkt) {
	//Return if not mapping QP !!!THIS FEATURE SHOULD TURN ON AND OFF EASILY!!!
	if (!should_track(pkt)) {
		return;
	}
	if (likely(has_mapped_qp != 0) || MAP_QP == 0) {
		return;
	}

	struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	struct rte_ipv4_hdr* ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	struct rte_udp_hdr * udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	struct roce_v2_header * roce_hdr = (struct roce_v2_header *)((uint8_t*)udp_hdr + sizeof(struct rte_udp_hdr));

	if (unlikely(roce_hdr->opcode == RC_WRITE_ONLY && fully_qp_init())) {
		//flip the switch
		print_first_mapping();
		has_mapped_qp = 1;
		return;
	}

	switch(roce_hdr->opcode){
		case RC_ACK:
		case RC_READ_RESPONSE:
			break;
		case RC_ATOMIC_ACK:
			find_and_set_stc_wrapper(roce_hdr,udp_hdr);
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


uint32_t get_predicted_shift(packet_size) {
	switch(packet_size){
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
			printf("Unknown packet size (%d) exiting\n",packet_size);
			exit(0);
	}
}

void check_and_cache_predicted_shift(uint32_t rdma_size) {
	//init write packet size
	if (unlikely(write_value_packet_size == 0)) {
		write_value_packet_size = rdma_size;
		predict_shift_value = get_predicted_shift(write_value_packet_size);
	}
	//Sanity check, we should only reach here if we are dealing with statically sized write packets
	if (unlikely(write_value_packet_size != rdma_size)) {
		printf("ERROR in write packet block, but packet size not correct Established runtime size %d\n",write_value_packet_size);
		exit(0);
	}
}

void catch_ecn(struct rte_mbuf* pkt, uint8_t opcode) {
	#ifdef CATCH_ECN
	if (opcode == ECN_OPCODE) {
		for (int i=0;i<20;i++) {
			printf("Packet # %d\n",packet_counter);
			printf("ECN\n");
		}
		print_packet(pkt);
		//debug_start_printing_every_packet = 1;
	}
	#endif
}

//In the case of regular operation where all of the writes are
//cached, there should be no nacks. However if we set
//CACHE_KEYSPACE to a value less than all of the keys, then we are
//going to start to see NACKS. This only is a problem when the
//writes are allowed to pass through. Reads should be a strict
//subset of the writes so it's not going to be a problem there. If
//you are running an experiment to show the effect of varying
//cache size just comment out the exit below.
void catch_nack(struct clover_hdr * clover_header, uint8_t opcode) {
    if (opcode == RC_ATOMIC_ACK) {
		struct cs_response * csr = (struct cs_response*) clover_header;
		uint32_t original = ntohl(csr->atomc_ack_extended.original_remote_data);
		if (original != 0) {
			nacked_cns++;
			printf("danger only hit here if you have the keyspace option turned on\n");
			exit(0);
		} 
	}
}

void true_classify(struct rte_mbuf * pkt) {
//void true_classify(struct rte_ipv4_hdr *ip, struct roce_v2_header *roce, struct clover_hdr * clover) {
	struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	struct rte_ipv4_hdr* ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	struct rte_udp_hdr * udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	struct roce_v2_header * roce_hdr = (struct roce_v2_header *)((uint8_t*)udp_hdr + sizeof(struct rte_udp_hdr));
	struct clover_hdr * clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));

	uint32_t size = ntohs(ipv4_hdr->total_length);
	uint8_t opcode = roce_hdr->opcode;
	uint32_t r_qp= roce_hdr->dest_qp;

	catch_ecn(pkt,opcode);
	catch_nack(clover_header, opcode);

	if (opcode == RC_WRITE_ONLY) {
		if (size == 252 || size == 68) {
			return;
		}
		lock_next();

		struct write_request * wr = (struct write_request*) clover_header;
		uint64_t *key = (uint64_t*)&(wr->data);
		uint32_t id = get_id(r_qp);
		set_latest_key(id,*key);

		uint32_t rdma_size = ntohl(wr->rdma_extended_header.dma_length);
		check_and_cache_predicted_shift(rdma_size);

		//Experimentatiopn for working with restricted cache keyspaces. Return
		//from here if the key is out of the cache range.
		if (*key > CACHE_KEYSPACE) {
			unlock_next();
			return;
		}
		//okay so this happens twice becasuse the order is 
		//Write 0;
		//Write 1;
		//Cn wNS 1 (write 1 - > write 2)
		if(first_write[*key] != 0 && first_cns[*key] != 0) {
			predict_address[*key] = ((be64toh(wr->rdma_extended_header.vaddr) - be64toh(first_write[*key])) >> predict_shift_value);
			outstanding_write_predicts[id][*key] = predict_address[*key];
			outstanding_write_vaddrs[id][*key] = wr->rdma_extended_header.vaddr;
		} else {
			if (first_write[*key] == 0) {
				//Write 0
				//This is the init write. On the first write, for some reason we
				//don't do anything. I just mark that it's received.
				first_write[*key] = 1;
			} else if (first_write[*key] == 1) {
				//Write 1
				//Here we actually record both the first vaddr for the write, and the 
				first_write[*key] = wr->rdma_extended_header.vaddr; //first write subject to change
				next_vaddr[*key] = wr->rdma_extended_header.vaddr;  //next_vaddr subject to change

				outstanding_write_predicts[id][*key] = 1;
				outstanding_write_vaddrs[id][*key] = wr->rdma_extended_header.vaddr;
			} else {
				//Write n
				//We should not really reach this point, I think that it's an error if we do.
				outstanding_write_predicts[id][*key] = 1;
				outstanding_write_vaddrs[id][*key] = wr->rdma_extended_header.vaddr;
			}
		}
		unlock_next();
	}

	if (size == 72 && opcode == RC_CNS) {
		lock_next();

		//Find value of the clover pointer. This is the value we are going to potentially swap out.
		struct cs_request * cs = (struct cs_request*) clover_header;
		uint64_t swap =  htobe64(MITSUME_GET_PTR_LH(be64toh(cs->atomic_req.swap_or_add)));
		uint32_t id = get_id(r_qp);
		uint64_t key = get_latest_key(id);
		uint64_t* first_cns_p = &first_cns[key];
		uint64_t* first_write_p = &first_write[key];
		uint64_t* outstanding_write_vaddr_p = &outstanding_write_vaddrs[id][key];
		uint64_t* outstanding_write_predict_p = &outstanding_write_predicts[id][key];
		uint64_t* next_vaddr_p = &next_vaddr[key];

		//This is the first instance of the cns for this key, it is a misunderstood case
		//For now return after setting the first instance of the key to the swap value
		if(key > CACHE_KEYSPACE) {
			//this key is not being tracked, return
			unlock_next();
			return;
		}

		//This is the first time we are seeking this key. We can't make a
		//prediction for it so we are just going to store the address so that we
		//can use is as an offset for performing redirections later.
		if (*first_cns_p == 0) {
			//log_printf(INFO,"setting swap for key %"PRIu64" id: %d -- Swap %"PRIu64"\n", latest_key[id],id, swap);
			*first_cns_p = swap;
			*first_write_p = *outstanding_write_vaddr_p;
			*next_vaddr_p = *outstanding_write_vaddr_p;

			for (uint i=0;i<qp_id_counter;i++) {
				if (outstanding_write_predicts[i][key] == 1) {
					//printf("(init conflict dected) recalculating outstanding writes for key %"PRIu64" id\n",latest_key[id],id);
					uint64_t predict = ((be64toh(outstanding_write_vaddrs[i][key]) - be64toh(*first_write_p)) >> 10);
					outstanding_write_predicts[i][key] = predict;
				}
			}
			//Return and forward the packet if this is the first cns
			unlock_next();
			return;
		}

		//Based on the key predict where the next CNS address should go. This
		//requires the first CNS to be set
		uint64_t predict = *outstanding_write_predict_p;
		predict = predict + be64toh(*first_cns_p);
		predict = htobe64( 0x00000000FFFFFF & predict); // THIS IS THE CORRECT MASK

		//Here we have had a first cns (assuming bunk, and we eant to point to the latest in the list)
		if (*next_vaddr_p != cs->atomic_req.vaddr) {
			cs->atomic_req.vaddr = *next_vaddr_p; //We can add this once we can predict with confidence
			recalculate_rdma_checksum(pkt);
		}

		//given that a cns has been determined move the next address for this 
		//key, to the outstanding write of the cns that was just made
		if (likely(predict == swap)) {
			//This is where the write (for all intents and purposes has been commited)
			#ifdef READ_STEER
			update_write_vaddr_cache(key,*next_vaddr_p);
			#endif

			*next_vaddr_p = *outstanding_write_vaddr_p;
			//erase the old entries
			*outstanding_write_predict_p = 0;
			*outstanding_write_vaddr_p = 0;
		} else {
			//This is the crash condtion
			//Fatal, unable to find the next key
			printf("Crashing on (CNS PREDICT) ID: %d psn %d\n",id,readable_seq(roce_hdr->packet_sequence_number));
			printf("predicted: %"PRIu64"\n",be64toh(predict));
			printf("actual:    %"PRIu64"\n",be64toh(swap));
			print_address(&predict);
			print_address(&swap);
			print_binary_address(&predict);
			print_binary_address(&swap);
			uint64_t diff = be64toh(swap)-be64toh(predict) ;
			printf("difference=%"PRIu64"\n",diff);
			diff = htobe64(diff);
			print_binary_address(&diff);
			uint64_t xor = be64toh(swap)^be64toh(predict) ;
			printf("xor=%"PRIu64"\n",xor);
			xor = htobe64(xor);
			print_binary_address(&xor);

			printf("base\n");
			print_binary_address(&first_cns[key]);
			uint64_t fcns = first_cns[key];
			print_binary_address(&fcns);

			printf("unable to find the next oustanding write, how can this be? SWAP: %"PRIu64" latest_key[id = %d]=%"PRIu64", first cns[key = %"PRIu64"]=%"PRIu64"\n",swap,id,latest_key[id],latest_key[id],first_cns[latest_key[id]]);
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

#define RSS_HASH_KEY_LENGTH 40 // for mlx5
uint64_t rss_hf = ETH_RSS_NONFRAG_IPV4_UDP; //ETH_RSS_UDP | ETH_RSS_TCP | ETH_RSS_IP;// | ETH_RSS_VLAN; /* RSS IP by default. */

uint8_t sym_hash_key[RSS_HASH_KEY_LENGTH] = {
        0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
        0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
        0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
        0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
        0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
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
	if (retval != 0) {
		printf("Error during getting device (port %u) info: %s\n",
				port, strerror(-retval));
		return retval;
	}

//STW RSS
	if (nb_rxq > 1) {
		//STW: use sym_hash_key for RSS
		port_conf.rx_adv_conf.rss_conf.rss_key = sym_hash_key;
		port_conf.rx_adv_conf.rss_conf.rss_key_len = RSS_HASH_KEY_LENGTH;
		port_conf.rx_adv_conf.rss_conf.rss_hf =
			rss_hf & dev_info.flow_type_rss_offloads;
	} else {
		port_conf.rx_adv_conf.rss_conf.rss_key = NULL;
		port_conf.rx_adv_conf.rss_conf.rss_hf = 0;
	}        if( port_conf.rx_adv_conf.rss_conf.rss_hf != 0){
		port_conf.rxmode.mq_mode = (enum rte_eth_rx_mq_mode) ETH_MQ_RX_RSS;
	}
	else{
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
	for (q = 0; q < rx_rings; q++) {
		retval = rte_eth_rx_queue_setup(port, q, nb_rxd,
				rte_eth_dev_socket_id(port), NULL, mbuf_pool);
		if (retval < 0)
			return retval;
	}

	txconf = dev_info.default_txconf;
	txconf.offloads = port_conf.txmode.offloads;
	/* Allocate and set up 1 TX queue per Ethernet port. */
	for (q = 0; q < tx_rings; q++) {
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

struct rte_ipv4_hdr* ipv4_hdr_process(struct rte_ether_hdr *eth_hdr) {
	struct rte_ipv4_hdr* ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	int hdr_len = (ipv4_hdr->version_ihl & RTE_IPV4_HDR_IHL_MASK) * RTE_IPV4_IHL_MULTIPLIER;
	if (hdr_len == sizeof(struct rte_ipv4_hdr)) {
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

struct rte_udp_hdr * udp_hdr_process(struct rte_ipv4_hdr *ipv4_hdr) {
	struct rte_udp_hdr * udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	if (ipv4_hdr->next_proto_id == IPPROTO_UDP){
		return udp_hdr;
	}
	return NULL;
}

struct roce_v2_header * roce_hdr_process(struct rte_udp_hdr * udp_hdr) {
	//Dont start parsing if the udp port is not roce
	struct roce_v2_header * roce_hdr = NULL;
	if (likely(rte_be_to_cpu_16(udp_hdr->dst_port) == ROCE_PORT)) {
		roce_hdr = (struct roce_v2_header *)((uint8_t*)udp_hdr + sizeof(struct rte_udp_hdr));
		return roce_hdr;
	}
	return NULL;
}

struct clover_hdr * mitsume_msg_process(struct roce_v2_header * roce_hdr){
	struct clover_hdr * clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));
	return clover_header;
}

void print_packet_lite(struct rte_mbuf * buf) {
	struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(buf, struct rte_ether_hdr *);
	struct rte_ipv4_hdr* ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	struct rte_udp_hdr * udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	struct roce_v2_header * roce_hdr = (struct roce_v2_header *)((uint8_t*)udp_hdr + sizeof(struct rte_udp_hdr));
	char * op = ib_print_op(roce_hdr->opcode);
	uint32_t size = ntohs(ipv4_hdr->total_length);
	uint32_t dest_qp = roce_hdr->dest_qp;
	uint32_t seq = readable_seq(roce_hdr->packet_sequence_number);
	uint32_t msn = get_msn(roce_hdr);

	if (msn != -1) {
		msn = readable_seq(msn);
	}

	int id = -1;
	for (int i=0;i<TOTAL_ENTRY;i++) {
		if (Connection_States[i].ctsqp == roce_hdr->dest_qp ||
			Connection_States[i].stcqp == roce_hdr->dest_qp) {
				id = Connection_States[i].id;
				break;
		}
	}

	id_colorize(id);
	printf("[core %d][id %d][op:%s (%d)][size: %d][dst: %d][seq %d][msn %d]\n",rte_lcore_id(),id, op,roce_hdr->opcode,size,dest_qp,seq,msn);
}

int accept_packet(struct rte_mbuf * pkt) {
	struct rte_ether_hdr* eth_hdr;
	struct rte_ipv4_hdr *ipv4_hdr; 
	struct rte_udp_hdr* udp_hdr;
	struct roce_v2_header * roce_hdr;
	struct clover_hdr * clover_header;

	eth_hdr = eth_hdr_process(pkt);
	if (unlikely(eth_hdr == NULL)) {
		log_printf(DEBUG, "ether header not the correct format dropping packet\n");
		rte_pktmbuf_free(pkt);
		return 0;
	}

	ipv4_hdr = ipv4_hdr_process(eth_hdr);
	if (unlikely(ipv4_hdr == NULL)) {
		log_printf(DEBUG, "ipv4 header not the correct format dropping packet\n");
		rte_pktmbuf_free(pkt);
		return 0;
	}

	udp_hdr = udp_hdr_process(ipv4_hdr);
	if (unlikely(udp_hdr == NULL)) {
		log_printf(DEBUG, "udp header not the correct format dropping packet\n");
		rte_pktmbuf_free(pkt);
		return 0;
	}

	roce_hdr = roce_hdr_process(udp_hdr);
	if (unlikely(roce_hdr == NULL)) {
		log_printf(DEBUG, "roceV2 header not correct dropping packet\n");
		rte_pktmbuf_free(pkt);
		return 0;
	}
	return 1;
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
			log_printf(INFO,"WARNING, port %u is on remote NUMA node to "
					"polling thread.\n\tPerformance will "
					"not be optimal.\n", port);

	log_printf(INFO,"\nCore %u forwarding packets. [Ctrl+C to quit]\n",
			rte_lcore_id());

	printf("Running lcore main\n");
	printf("Client Threads %d\n",TOTAL_CLIENTS);
	printf("Keyspace %d\n", KEYSPACE);

	/* Run until the application is quit or killed. */
	for (;;) {
		/*
		 * Receive packets on a port and forward them on the paired
		 * port. The mapping is 0 -> 1, 1 -> 0, 2 -> 3, 3 -> 2, etc.
		 */
		RTE_ETH_FOREACH_DEV(port) {
			uint16_t ipv4_udp_rx = 0;	

			/* Get burst of RX packets, from first and only port */
			struct rte_mbuf *rx_pkts[BURST_SIZE];
			struct rte_mbuf *tx_pkts[BURST_SIZE];
			uint32_t to_tx = 0;
			//printf("%X bufs\n",&rx_pkts[0]);

			uint32_t queue = rte_lcore_id()/2;
			const uint16_t nb_rx = rte_eth_rx_burst(port, queue, rx_pkts, BURST_SIZE);
			uint16_t nb_tx = 0;
			
			if (unlikely(nb_rx == 0))
				continue;

			for (uint16_t i = 0; i < nb_rx; i++){
				if (likely(i < nb_rx - 1)) {
					rte_prefetch0(rte_pktmbuf_mtod(rx_pkts[i+1],void *));
				}
				
				struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(rx_pkts[i], struct rte_ether_hdr *);
				struct rte_ipv4_hdr* ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
				struct rte_udp_hdr * udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
				struct roce_v2_header * roce_hdr = (struct roce_v2_header *)((uint8_t*)udp_hdr + sizeof(struct rte_udp_hdr));
				struct clover_hdr * clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));
				packet_counter++;

				lock_qp();
				true_classify(rx_pkts[i]);
				struct map_packet_response mpr;

				mpr = map_qp(rx_pkts[i]);
				uint32_t to_send = 0;
				for (int j=0;j<mpr.size;j++) {
					tx_pkts[to_tx] = mpr.pkts[j];
					finish_mem_pkt(tx_pkts[to_tx],port,queue);
					to_tx++;
					to_send++;
				}
			  	unlock_qp();
			}
		}
	}
}

int coretest(void) {
	printf("I'm actually running on core %d\n",rte_lcore_id());
	lcore_main();
	return 1;
}

void fork_lcores(void) {
	printf("Running on #%d cores\n",rte_lcore_count());
	int lcore;
	RTE_LCORE_FOREACH_SLAVE(lcore) {
		printf("running core %d\n",lcore);
		rte_eal_remote_launch(coretest, NULL, lcore);
	} 	
}

volatile sig_atomic_t flag = 0;
void kill_signal_handler(int sig){
	#ifdef TAKE_MEASUREMENTS
	write_run_data();
	#endif
	exit(0);
}

/*
 * The main function, which does initialization and calls the per-lcore
 * functions.
 */
int
main(int argc, char *argv[])
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
		if (port_init(portid, mbuf_pool,rte_lcore_count()) != 0)
			rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu16 "\n",
					portid);

	if (rte_lcore_count() > 1)
		printf("\nWARNING: Too many lcores enabled. Only 1 used.\n");


	printf("master core %d\n",rte_get_master_lcore());

	if (init == 0) {
		signal(SIGINT, kill_signal_handler);
		bzero(first_write,KEYSPACE*sizeof(uint64_t));
		bzero(second_write,TOTAL_ENTRY*KEYSPACE*sizeof(uint64_t));
		bzero(first_cns,KEYSPACE*sizeof(uint64_t));
		bzero(predict_address,KEYSPACE*sizeof(uint64_t));
		bzero(latest_cns_key,KEYSPACE*sizeof(uint64_t));
		bzero(latest_key,TOTAL_ENTRY*sizeof(uint64_t));
		bzero(outstanding_write_predicts,TOTAL_ENTRY*KEYSPACE*sizeof(uint64_t));
		bzero(outstanding_write_vaddrs,TOTAL_ENTRY*KEYSPACE*sizeof(uint64_t));
		bzero(next_vaddr,KEYSPACE*sizeof(uint64_t));

		#ifdef TAKE_MEASUREMENTS
		bzero(packet_latencies,TOTAL_PACKET_LATENCIES*sizeof(uint64_t));
		bzero(sequence_order,TOTAL_ENTRY*TOTAL_PACKET_SEQUENCES*sizeof(uint32_t));
		bzero(sequence_order_timestamp,TOTAL_ENTRY*TOTAL_PACKET_SEQUENCES*sizeof(uint64_t));
		bzero(request_count_id, TOTAL_ENTRY * sizeof(uint32_t));
		#endif

		#ifdef READ_STEER
		bzero(cached_write_vaddrs,KEYSPACE * WRITE_VADDR_CACHE_SIZE * sizeof(uint64_t));
		bzero(writes_per_key, KEYSPACE * sizeof(uint32_t));
		bzero(cached_write_vaddr_mod, sizeof(uint64_t) * HASHSPACE);
		bzero(cached_write_vaddr_mod_lookup, sizeof(uint64_t) * HASHSPACE);
		bzero(cached_write_vaddr_mod_latest, sizeof(uint64_t) * KEYSPACE);
		#endif

		init_reorder_buf();
		init_connection_states();
		init_hash();
		write_value_packet_size=0;
		predict_shift_value=0;
		has_mapped_qp=0;
		init = 1;
	}

	rte_rwlock_init(&next_lock);
	rte_rwlock_init(&qp_lock);
	rte_rwlock_init(&qp_init_lock);
	rte_rwlock_init(&mem_qp_lock);
	fork_lcores();
	lcore_main();
	return 0;
}
