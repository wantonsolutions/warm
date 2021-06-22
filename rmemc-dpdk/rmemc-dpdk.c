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
#include "packets.h"
#include "clover_structs.h"
#include <arpa/inet.h>

#include <rte_jhash.h>
#include <rte_hash.h>
#include <rte_table.h>

#include <endian.h>

//#include <linux/crc32.h>
#include <zlib.h>


#define RC_SEND 0x04
#define RC_WRITE_ONLY 0x0A
#define RC_READ_REQUEST 0x0C
#define RC_READ_RESPONSE 0x10
#define RC_ACK 0x11
#define RC_ATOMIC_ACK 0x12
#define RC_CNS 0x13

#define RDMA_COUNTER_SIZE 256
#define RDMA_STRING_NAME_LEN 256
#define PACKET_SIZES 256

#define KEYSPACE 1024
//#define KEYSPACE 500
#define CACHE_KEYSPACE 1024

#define RDMA_CALL_SIZE 8192

#define SEQUENCE_NUMBER_SHIFT 256

#define TOTAL_PACKET_LATENCIES 10000

#define TOTAL_CLIENTS 4

//#define DATA_PATH_PRINT
//#define MAP_PRINT
#define COLLECT_GARBAGE

uint8_t test_ack_pkt[] = {
0xEC,0x0D,0x9A,0x68,0x21,0xCC,0xEC,0x0D,0x9A,0x68,0x21,0xD0,0x08,0x00,0x45,0x02,
0x00,0x30,0x2A,0x2B,0x40,0x00,0x40,0x11,0x8D,0x26,0xC0,0xA8,0x01,0x0C,0xC0,0xA8,
0x01,0x0D,0xCF,0x15,0x12,0xB7,0x00,0x1C,0x00,0x00,0x11,0x40,0xFF,0xFF,0x00,0x00,
0x6C,0xA9,0x00,0x00,0x0C,0x71,0x0D,0x00,0x00,0x01,0xDC,0x97,0x84,0x42,};



#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 

#define MITSUME_PTR_MASK_LH 0x0ffffffff0000000
//#define MITSUME_PTR_MASK_OFFSET                 0x0000fffff8000000
#define MITSUME_PTR_MASK_NEXT_VERSION 0x0000000007f80000
#define MITSUME_PTR_MASK_ENTRY_VERSION 0x000000000007f800
#define MITSUME_PTR_MASK_XACT_AREA 0x00000000000007fe
#define MITSUME_PTR_MASK_OPTION 0x0000000000000001

#define MITSUME_GET_PTR_LH(A) (A & MITSUME_PTR_MASK_LH) >> 28

char ib_print[RDMA_COUNTER_SIZE][RDMA_STRING_NAME_LEN];

int MAP_QP = 1;



static int rdma_counter = 0;
static int has_mapped_qp = 0;
//rdma calls counts the number of calls for each RDMA op code
uint64_t packet_latencies[TOTAL_PACKET_LATENCIES];
uint64_t packet_latency_count = 0;


void append_packet_latency(uint64_t clock_cycles) {
	if (packet_latency_count < TOTAL_PACKET_LATENCIES) {
		packet_latencies[packet_latency_count] = clock_cycles;
		packet_latency_count++;
	}
}

void write_packet_latencies_to_known_file() {
	char* filename="/tmp/latency-latest.dat";
	printf("test\n");
	printf("Writing a total of %"PRIu64" packet latencies to %s\n",packet_latency_count,filename);
	FILE *fp = fopen(filename, "w");
	if (fp == NULL) {
		printf("Unable to write file out, fopen has failed\n");
		perror("Failed: ");
		return;
	}

	for (int i=0;i<packet_latency_count;i++) {
		fprintf(fp,"%"PRIu64"\n",packet_latencies[i]);
	}
	fclose(fp);


}

uint32_t rdma_call_count[RDMA_COUNTER_SIZE];


static int packet_counter = 0;
uint64_t packet_size_index[RDMA_COUNTER_SIZE][PACKET_SIZES];
uint32_t packet_size_calls[RDMA_COUNTER_SIZE][PACKET_SIZES];

uint64_t read_req_addr_index[KEYSPACE];
uint32_t read_req_addr_count[KEYSPACE];

uint64_t read_resp_addr_index[KEYSPACE];
uint32_t read_resp_addr_count[KEYSPACE];

#define MAX_CORES 24
uint32_t core_pkt_counters[MAX_CORES];

uint64_t vaddr_swaps = 0;

uint32_t debug_start_printing_every_packet = 0;


static struct rte_hash_parameters qp2id_params = {
	.name = "qp2id",
    .entries = TOTAL_ENTRY,
    .key_len = sizeof(uint32_t),
    .hash_func = rte_jhash,
    .hash_func_init_val = 0,
    .socket_id = 0,
};
struct rte_hash* qp2id_table;
static uint32_t qp_id_counter=0;
uint32_t qp_values[TOTAL_ENTRY];
uint32_t id_qp[TOTAL_ENTRY];

struct Connection_State Connection_States[TOTAL_ENTRY];

/*
export COLOR_NC='\e[0m' # No Color
export COLOR_BLACK='\e[0;30m'
export COLOR_GRAY='\e[1;30m'
export COLOR_RED='\e[0;31m'
export COLOR_LIGHT_RED='\e[1;31m'
export COLOR_GREEN='\e[0;32m'
export COLOR_LIGHT_GREEN='\e[1;32m'
export COLOR_BROWN='\e[0;33m'
export COLOR_YELLOW='\e[1;33m'
export COLOR_BLUE='\e[0;34m'
export COLOR_LIGHT_BLUE='\e[1;34m'
export COLOR_PURPLE='\e[0;35m'
export COLOR_LIGHT_PURPLE='\e[1;35m'
export COLOR_CYAN='\e[0;36m'
export COLOR_LIGHT_CYAN='\e[1;36m'
export COLOR_LIGHT_GRAY='\e[0;37m'
export COLOR_WHITE='\e[1;37m'
*/

void red () {
  printf("\033[1;31m");
}

void yellow () {
  printf("\033[1;33m");
}

void blue () {
  printf("\033[1;34m");
}

void green () {
  printf("\033[1;32m");
}

void reset () {
  printf("\033[0m");
}

void id_colorize(uint32_t id) {
	switch(id){
		case 0:
			red();
			break;
		case 1:
			yellow();
			break;
		case 2:
			blue();
			break;
		case 3:
			green();
			break;
		default:
			reset();
			break;
	}
}

#define HASH_RETURN_IF_ERROR(handle, cond, str, ...) do {                \
    if (cond) {                         \
        printf("ERROR line %d: " str "\n", __LINE__, ##__VA_ARGS__); \
        if (handle) rte_hash_free(handle);          \
        return -1;                      \
    }                               \
} while(0)


uint32_t key_to_qp(uint64_t key) {

	//Keys start at 1, so I'm subtracting 1 to make the first key equal to index
	//zero.  qp_id_counter is the total number of qp that can be written to. So
	//here we are just taking all of the keys and wrapping them around so the
	//first key goes to the first qp, and the qp_id_counter + 1  key goes to the
	//first qp
	//uint32_t index = (key-1)%qp_id_counter;
	uint32_t index = (key)%qp_id_counter;
	//printf("qpindex %d key %"PRIu64" counter %d\n",index,key,qp_id_counter);
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
	//Turn this on and off for debugging
	#ifdef MAP_PRINT
	id_colorize(id);
	#endif

	return id;
}


uint32_t readable_seq(uint32_t seq) {
	return ntohl(seq) / 256;
}


void print_request_map(struct Request_Map *rm) {
	if (rm->open == 1) 
		printf("open\n");
	else {
		printf("closed");
	}

	printf("ID: %d\n");
	printf("Original Seq %d, mapped seq %d\n",readable_seq(rm->original_sequence), readable_seq(rm->mapped_sequence));
	printf("stcqp qp %d, mapped stcqp %d\n", rm->server_to_client_qp, rm->mapped_destination_server_to_client_qp);
	printf("stcqp port %d\n", rm->server_to_client_udp_port);
}

void print_connection_state(struct Connection_State* cs) {
	printf("ID: %d port-cts %d port-stc %d\n",cs->id, cs->udp_src_port_client, cs->udp_src_port_server);
	printf("cts qp: %d stc qp: %d \n",cs->ctsqp, cs->stcqp);
	printf("seqt %d seq %d mseqt %d mseq\n",readable_seq(cs->seq_current),cs->seq_current,readable_seq(cs->mseq_current),cs->mseq_current);
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

void init_connection_state(uint16_t udp_src_port, uint32_t cts_dest_qp, uint32_t seq, uint32_t rkey) {

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
	cs.rkey = rkey;
	cs.sender_init=1;
	//These fields do not need to be set. They are for the second half of the algorithm
	//cs.stcqp = 0; //At init time the opposite side of the qp is going to be unknown
	//cs.udp_src_port_server = 0;
	//cs.mseq_current = 0;
	Connection_States[cs.id] = cs;
}

void set_rkey_rdma_packet(struct roce_v2_header *roce_hdr, uint32_t rkey) {
	struct clover_hdr * clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));
	struct write_request *wr = (struct write_request*) clover_header;
	struct read_request * read_req = (struct read_request *)clover_header;
	struct cs_request * cs_req = (struct cs_request *)clover_header;

	switch(roce_hdr->opcode) {
		case RC_WRITE_ONLY:
			wr->rdma_extended_header.rkey = rkey;
			return;
		case RC_READ_REQUEST:
			read_req->rdma_extended_header.rkey = rkey;
			return;
		case RC_CNS:
			cs_req->atomic_req.rkey = rkey;
			return;
		default:
    		printf("op code %02X %s\n",roce_hdr->opcode, ib_print[roce_hdr->opcode]);
			printf("rh-opcode unknown while setting rkey. Exiting\n");
			exit(0);
	}
	return;
}

uint32_t get_rkey_rdma_packet(struct roce_v2_header *roce_hdr) {
	struct clover_hdr * clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));
	struct write_request *wr = (struct write_request*) clover_header;
	struct read_request * read_req = (struct read_request *)clover_header;
	struct cs_request * cs_req = (struct cs_request *)clover_header;

	switch(roce_hdr->opcode) {
		case RC_WRITE_ONLY:
			return wr->rdma_extended_header.rkey;
		case RC_READ_REQUEST:
			return read_req->rdma_extended_header.rkey;
		case RC_CNS:
			return cs_req->atomic_req.rkey;
		default:
			printf("rh-opcode unknown while getting rkey. Exiting\n");
			exit(0);
	}
	return -1;
}

void init_cs_wrapper(struct rte_udp_hdr *udp_hdr, struct roce_v2_header *roce_hdr) {

	if (roce_hdr->opcode != RC_WRITE_ONLY && roce_hdr->opcode != RC_READ_REQUEST && roce_hdr->opcode != RC_CNS ) {
		printf("only init connection states for writers either WRITE_ONLY or CNS (and only for data path) exiting for safty");
		exit(0);
	}

	uint32_t rkey = get_rkey_rdma_packet(roce_hdr);

	init_connection_state(udp_hdr->src_port, roce_hdr->dest_qp, roce_hdr->packet_sequence_number,rkey);
	
}

void set_msn(struct roce_v2_header *roce_hdr, uint32_t new_msn) {
	struct clover_hdr * clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));
	struct read_response * read_resp = (struct read_response*) clover_header;
	struct rdma_ack *ack = (struct rdma_ack*) clover_header;
	struct cs_response * cs_resp = (struct cs_response *)clover_header;
	uint32_t* msn;
	switch(roce_hdr->opcode) {
		case RC_READ_RESPONSE:;
			read_resp->ack_extended.sequence_number = new_msn;
			return;
		case RC_ACK:;
			ack->ack_extended.sequence_number = new_msn;
			return;
		case RC_ATOMIC_ACK:;
			cs_resp->ack_extended.sequence_number = new_msn;
			return;
		default:
			printf("WRONG HEADER MSN NOT FOUND\n");
			exit(0);
			return;
	}
	return;
}

uint32_t get_msn(struct roce_v2_header *roce_hdr) {
	struct clover_hdr * clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));
	struct read_response * read_resp = (struct read_response*) clover_header;
	struct rdma_ack *ack = (struct rdma_ack*) clover_header;
	struct cs_response * cs_resp = (struct cs_response *)clover_header;
	uint32_t* msn;
	switch(roce_hdr->opcode) {
		case RC_READ_RESPONSE:;
			msn = read_resp->ack_extended.sequence_number;
			break;
		case RC_ACK:;
			msn = ack->ack_extended.sequence_number;
			break;
		case RC_ATOMIC_ACK:;
			msn=cs_resp->ack_extended.sequence_number;
			break;
		default:
			printf("WRONG HEADER MSN NOT FOUND\n");
			exit(0);
			break;
	}
	return msn;
}

void increment_msn(struct Connection_State *cs) {
	uint32_t msn_update = cs->mseq_current;
	msn_update = htonl(ntohl(msn_update) + SEQUENCE_NUMBER_SHIFT);
	cs->mseq_current = msn_update;
}

uint32_t produce_and_update_msn(struct roce_v2_header* roce_hdr, struct Connection_State *cs) {
	//new way of doing it
	uint32_t msn = htonl(ntohl(roce_hdr->packet_sequence_number) - ntohl(cs->mseq_offset));
	if (ntohl(msn) > ntohl(cs->mseq_current)) {
		cs->mseq_current = msn;
	}
	return msn;
}

uint32_t find_and_update_stc(struct roce_v2_header *roce_hdr, struct rte_udp_hdr *udp_hdr) {
	struct Connection_State *cs;

	uint32_t found = 0;

	//check to see if this value has allready been set
	//Find the coonection
	for (int i=0;i<TOTAL_ENTRY;i++){
		cs = &Connection_States[i];
		//if (cs.seq_current == roce_hdr->packet_sequence_number) {
		if (cs->stcqp == roce_hdr->dest_qp && cs->receiver_init == 1) {
			found = 1;
			break;
		}
	}

	if (found == 0) {
		return 0;
	}

	//at this point we have the correct cs
	uint32_t msn = produce_and_update_msn(roce_hdr,cs);
	//this should be it

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
		printf("msn update successful\n");
		return;
	}

	//If we are here then the connection should not be initlaized yet	
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
	if (cs.receiver_init==0) {
		printf("MSN init on receiver side");
		cs = Connection_States[matching_id];
		cs.stcqp = roce_hdr->dest_qp;
		cs.udp_src_port_server = udp_hdr->src_port;
		cs.mseq_current = get_msn(roce_hdr);
		cs.mseq_offset = htonl(ntohl(cs.seq_current) - ntohl(cs.mseq_current)); //still shifted by 256 but not in network order
		printf("cs.mseq_offset = %d\n",readable_seq(cs.mseq_offset));
		cs.receiver_init = 1;
		Connection_States[matching_id] = cs;
		return;
	}

	printf("ERRROR we should not be reaching here, either update the msn or init it.");
	exit(0);
	return;
}

void find_and_set_stc_wrapper(struct roce_v2_header *roce_hdr, struct rte_udp_hdr *udp_hdr) {
	if (roce_hdr->opcode != RC_ACK && roce_hdr->opcode != RC_ATOMIC_ACK && roce_hdr->opcode != RC_READ_RESPONSE) {
		printf("Only find and set stc on ACKS, and responses");
		return;
	}
	//find_and_set_stc(roce_hdr->dest_qp,htonl(roce_hdr->packet_sequence_number));
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
	//update_cs_seq(roce_hdr->dest_qp,htonl(roce_hdr->packet_sequence_number));
	update_cs_seq(roce_hdr->dest_qp,roce_hdr->packet_sequence_number);
}

void cts_track_connection_state(struct rte_udp_hdr *udp_hdr , struct roce_v2_header * roce_hdr) {
	if (has_mapped_qp == 1) {
		return;
	}
	init_cs_wrapper(udp_hdr,roce_hdr);
	update_cs_seq_wrapper(roce_hdr);
}

void count_values(uint64_t *index, uint32_t *count, uint32_t size, uint64_t value) {
	//search
	for (uint32_t i=0;i<size;i++) {
		if(index[i] == value) {
			count[i]++;
			return;
		}
	}
	//add new index
	for (uint32_t i=0;i<size;i++) {
		if(index[i] == 0) {
			index[i]=value;
			count[i]=1;
			return;
		}
	}
}

void print_count(uint64_t *index, uint32_t *count, uint32_t size) {
	for (uint32_t i=0;i<size;i++) {
		if (index[i] != 0) {
			printf("[%08d] Index: ",i);
			print_bytes((uint8_t *)&index[i],sizeof(uint64_t));
			printf(" Count: %d\n",count[i]);
		}
	}

}

void count_read_req_addr(struct read_request * rr) {
	count_values(read_req_addr_index,read_req_addr_count,KEYSPACE,rr->rdma_extended_header.vaddr);
}

void print_read_req_addr(void) {
	print_count(read_req_addr_index,read_req_addr_count,KEYSPACE);
}

void classify_packet_size(struct rte_ipv4_hdr *ip, struct roce_v2_header *roce) {
	uint32_t size = ntohs(ip->total_length);
	uint8_t opcode = roce->opcode;
	if (packet_counter == 0) {
		bzero(packet_size_index,RDMA_COUNTER_SIZE*PACKET_SIZES*sizeof(uint32_t));
		bzero(packet_size_calls,RDMA_COUNTER_SIZE*PACKET_SIZES*sizeof(uint32_t));
	}
	count_values(packet_size_index[opcode],packet_size_calls[opcode], PACKET_SIZES, size);
}


void print_bytes(const uint8_t * buf, uint32_t len) {
	for (uint32_t i=0;i<len;i++)  {
		printf("%02X ", buf[i]);
	}
}

void print_binary_bytes(const uint8_t * buf, uint32_t len) {
	for (uint32_t i=0;i<len;i++)  {
		printf(BYTE_TO_BINARY_PATTERN" ", BYTE_TO_BINARY(buf[i]));
	} 
}

void print_address(uint64_t *address) {
	printf("address: ");
	print_bytes((uint8_t *) address,sizeof(uint64_t));
	printf("\n");
}


void print_binary_address(uint64_t *address) {
	printf("bin address: ");
	print_binary_bytes((uint8_t *)address,sizeof(uint64_t));
	printf("\n");
}

void print_ack_extended_header(struct AETH *aeth) {
	printf("Reserved        %u\n", ntohs(aeth->reserved));
	printf("Opcode          %u\n", ntohs(aeth->opcode));
	printf("Credit Count    %u\n", ntohs(aeth->credit_count));
	printf("Sequence Number %u\n", ntohl(aeth->sequence_number));
}

void print_rdma_extended_header(struct RTEH *rteh) {
	printf("virtual address: ");
	print_bytes((uint8_t *)&(rteh->vaddr),sizeof(uint64_t));
	printf("\n");

	printf("rkey: %u \traw:   ", ntohl(rteh->rkey));
	print_bytes((uint8_t *)&(rteh->rkey),sizeof(uint32_t));
	printf("\n");

	printf("dma len %u \traw: ", ntohl(rteh->dma_length));
	print_bytes((uint8_t *)&(rteh->dma_length),sizeof(uint32_t));
} 
 
void print_read_request(struct read_request* rr) {
	printf("(START) Read Request: \n");
	printf("(raw) ");
	print_bytes((void*) rr, 16);
	printf("\n");
	print_rdma_extended_header(&rr->rdma_extended_header);
	printf("\n");
	//printf("(STOP) Read Request\n");
	return;
}

void print_read_response(struct read_response *rr) {
	printf("(START) Read Response \t");
	//Not sure why this is ten
	uint32_t default_read_header_size=10;
	print_bytes((uint8_t*) rr, default_read_header_size);
	printf("\n");
	//printf("(STOP) Read Response\n");
	return;
}

void print_write_request(struct write_request* wr) {
	printf("(START) Write Request\n");
	print_rdma_extended_header(&wr->rdma_extended_header);
	printf("(STOP) Write Request\n");
	return;
}

void print_atomic_eth(struct AtomicETH* ae){
	printf("Vaddr: %"PRIu64"\n",ae->vaddr);
	printf("rkey: %d\n",ae->rkey);
	printf("swap || add: %"PRIu64"\n",ae->swap_or_add);
	printf("cmp: %"PRIu64"\n",ae->compare);
}

void print_cs_request(struct cs_request *csr) {
	printf("(START) compare and swap request\n");
	print_atomic_eth(&csr->atomic_req);
	printf("(STOP) compare and swap request\n");
	return;
}

void print_cs_response(struct cs_response *csr) {
	printf("(START) compare and swap response\n");
	printf("(STOP) compare and swap response\n");
	return;
}



uint32_t check_sums(const char* method, void* known, void* test, int try) {
	if (memcmp(known,test, 4) == 0) {
		printf("(%s) found the matching crc on try %d\n",method, try);
		print_bytes(test,4);
		printf("\n");
		return 1;
	}
	return 0;
}

uint32_t check_sums_wrap(const char* method, void* know, void* test) {
	uint32_t variant;

	uint32_t found = 0;
	variant = *(uint32_t *)test;
	found |= check_sums(method,know, &variant, 1);

	variant = ~variant;
	found |= check_sums(method,know, &variant, 2);

	variant = *(uint32_t *)test;
	variant = ntohl(variant);
	found |= check_sums(method,know, &variant, 3);

	variant = ~variant;
	found |= check_sums(method,know, &variant, 4);
	return found;
}


uint32_t csum_pkt_fast(struct rte_mbuf* pkt) {
	struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	struct rte_ipv4_hdr* ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	struct rte_udp_hdr * udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	struct roce_v2_header * roce_hdr = (struct roce_v2_header *)((uint8_t*)udp_hdr + sizeof(struct rte_udp_hdr));

	uint8_t ttl = ipv4_hdr->time_to_live;
	ipv4_hdr->time_to_live=0xFF;

	uint16_t ipv4_csum = ipv4_hdr->hdr_checksum;
	ipv4_hdr->hdr_checksum=0xFFFF;

	uint8_t ipv4_tos = ipv4_hdr->type_of_service;
	ipv4_hdr->type_of_service=0xFF;

	uint16_t udp_csum = udp_hdr->dgram_cksum;
	udp_hdr->dgram_cksum=0xFFFF;

	uint8_t roce_res = roce_hdr->reserverd;
	roce_hdr->reserverd=0x3F;

	uint8_t fecn = roce_hdr->fecn;
	roce_hdr->fecn=1;

	uint8_t bcen = roce_hdr->bcen;
	roce_hdr->bcen=1;


	uint8_t * start = (uint8_t*)(ipv4_hdr);
	uint32_t len = ntohs(ipv4_hdr->total_length) - 4;

	uint32_t crc_check;
	uint8_t buf[1500];


	void * current = (uint8_t *)(ipv4_hdr) + ntohs(ipv4_hdr->total_length) - 4;
	uint8_t current_val[4];
	memcpy(current_val,current,4);
	uint8_t test_buf[] = {0xff, 0xff, 0xff, 0xff};

	//TODO debug to prevent needing this bzero
	bzero(buf,1500);
	memcpy(buf,start,len);
	uLong crc = crc32(0xFFFFFFFF, test_buf, 4);
	
	//Now lets test with the dummy bytes
	crc = crc32(crc,buf,len) & 0xFFFFFFFF;
	crc_check = crc;
	//check_sums_wrap("zlib_crc",current_val, &crc_check);

	//Restore header values post masking
	ipv4_hdr->time_to_live=ttl;
	ipv4_hdr->hdr_checksum = ipv4_csum;
	ipv4_hdr->type_of_service = ipv4_tos;
	udp_hdr->dgram_cksum = udp_csum;
	roce_hdr->reserverd=roce_res;
	roce_hdr->fecn = fecn;
	roce_hdr->bcen = bcen;

	return crc_check;
}

#ifdef PACKET_DEBUG_PRINTOUT
#define KEY_VERSION_RING_SIZE 256
static uint64_t key_address[KEYSPACE];
static uint64_t key_versions[KEYSPACE][KEY_VERSION_RING_SIZE];
static uint32_t key_count[KEYSPACE];
#endif


static uint64_t first_write[KEYSPACE];
static uint64_t second_write[TOTAL_ENTRY][KEYSPACE];
static uint64_t first_cns[KEYSPACE];
static uint64_t predict_address[KEYSPACE];
static uint64_t latest_cns_key[KEYSPACE];

static uint64_t outstanding_write_predicts[TOTAL_ENTRY][KEYSPACE]; //outstanding write index, contains precited addresses
static uint64_t outstanding_write_vaddrs[TOTAL_ENTRY][KEYSPACE]; //outstanding vaddr values, used for replacing addrs
static uint64_t next_vaddr[KEYSPACE];
static uint64_t latest_key[TOTAL_ENTRY];

static int init =0;

static uint32_t write_value_packet_size = 0;
static uint32_t predict_shift_value=0;
static uint32_t nacked_cns =0;

rte_rwlock_t next_lock;


void print_first_mapping(void){
	for (int i=0;i<15;i++) {
		printf("FIRST MAPPING OF QP GET READY BOIIS\n\n");
	}
}


void init_connection_states(void) {
	bzero(Connection_States,TOTAL_ENTRY * sizeof(Connection_State));
	for (int i=0;i<TOTAL_ENTRY;i++) {
		struct Connection_State *source_connection;
		source_connection=&Connection_States[i];
		for (int j=0;j<TOTAL_ENTRY;j++) {
			struct Request_Map * mapped_request;
			mapped_request = &(source_connection->Outstanding_Requests[j]);
			mapped_request->open = 1;
		}
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

uint32_t garbage_collect_slots(struct Connection_State* cs) {
	//Find the highest sequence number used in this connection
	uint32_t max_sequence_number=0;
	for (int j=0;j<TOTAL_ENTRY;j++) {
		struct Request_Map* slot = &cs->Outstanding_Requests[j];
		if (!slot_is_open(slot) && readable_seq(slot->mapped_sequence) > max_sequence_number) {
			max_sequence_number = readable_seq(cs->Outstanding_Requests[j].mapped_sequence);
			printf("new max sequence %d\n",max_sequence_number);
		}
	}

	//The idea behind the stale water mark, is that entries which have sequence
	//numbers lower than it have not been accounted for or have disapeared
	//somewhere in the messaging. This is likely due to bugs, but it's hard to
	//tell. The point is that we can keep running by just removing these entries
	//probably. I think it's very important to learn why this is happening but
	//for now garbage collection might be a path forward.  The value here should
	//be TOTAL_ENYTRY, but I'm starting with TOTAL_ENTRY * constant_multiper so
	//that I'm "extra" safe.
	//!TODO figure out what's actually going on here.
	uint32_t constant_multiplier = 1;
	int32_t stale_water_mark = max_sequence_number - (TOTAL_ENTRY * constant_multiplier);
	printf("Max Sequence Number %d Stale Water Mark %d\n",max_sequence_number,stale_water_mark);
	uint32_t garbage_collected = 0;

	if (stale_water_mark < 0) {
		return garbage_collected;
	}

	//Perform garbage collection
	for (int j=0;j<TOTAL_ENTRY;j++) {
		struct Request_Map* slot = &cs->Outstanding_Requests[j];
		if (!slot_is_open(slot) && readable_seq(slot->mapped_sequence) < stale_water_mark) {
			open_slot(slot);
			garbage_collected++;
		}
	}

	return garbage_collected;
}

struct Request_Map * find_empty_slot(struct Connection_State* cs) {
	for (int j=0;j<TOTAL_ENTRY;j++) {
		if (slot_is_open(&cs->Outstanding_Requests[j])) {
			return &cs->Outstanding_Requests[j];
		}
	}
	return NULL;
}


struct Request_Map * get_empty_slot(struct Connection_State* cs) {
	
	
	//Search
	struct Request_Map * slot;
	#ifdef COLLECT_GARBAGE
	slot = find_empty_slot(cs);
	if (likely(slot)) {
		return slot;
	}

	printf(" Unable to find empty slot GARBAGE COLLECTING\n");
	uint32_t collected = garbage_collect_slots(cs);
	printf("Collected %d garbage slots\n",collected);
	#endif

	//Second Try
	slot = find_empty_slot(cs);
	if (likely(slot)) {
		return slot;
	} else {
		printf("ERROR: unable to find empty slot for forwarding. Look at TOTAL_ENTRY Exiting for safty!\n");
		for (int j=0;j<TOTAL_ENTRY;j++) {
			printf("INDEX %d\n",j);
			print_request_map(&cs->Outstanding_Requests[j]);
		}
		//Something has gone very wrong
		exit(0);
	}

	//error we did not find anything good
	exit(0);
	return NULL;
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
	//id is the senders id
	uint32_t id = get_id(r_qp);

	if (unlikely(has_mapped_qp == 0)) {
		#ifdef DATA_PATH_PRINT
		print_packet(pkt);
		print_first_mapping();
		#endif 
		has_mapped_qp = 1;
	}

	//Initalize the packet here.
	//cts_track_connection_state(udp_hdr, roce_hdr);


	//printf("old qp dst %d seq %d \n",ntohl(roce_hdr->dest_qp), ntohl(roce_hdr->packet_sequence_number));
	//printf("KEY REMAP: %"PRIu64" old qp dst %d seq %d seq_bigen %d\n",latest_key[id],roce_hdr->dest_qp, ntohl(roce_hdr->packet_sequence_number), roce_hdr->packet_sequence_number);
	//log_printf(DEBUG,"MAP FORWARD: (ID: %d) (Key: %"PRIu64") (QP dest: %d) (seq raw %d) (seq %d)\n",id, latest_key[id],roce_hdr->dest_qp, roce_hdr->packet_sequence_number, readable_seq(roce_hdr->packet_sequence_number));
	uint32_t n_qp = 0;
	if (roce_hdr->opcode==RC_READ_REQUEST) {
		//printf("No QP redirect on reads\n");
		n_qp = roce_hdr->dest_qp;
	} else {
		n_qp = key_to_qp(key);
	}

	//printf("KEY %"PRIu64" QP %d\n",key,n_qp);


	//uint32_t dest_id = get_id(n_qp);
	struct Connection_State *destination_connection;
	for (int i=0;i<TOTAL_ENTRY;i++) {
		destination_connection=&Connection_States[i];
		if (destination_connection->ctsqp == n_qp) {
			//printf("Incomming packet for id %d key %d qp %d routing to another qp %d\n",id,key,roce_hdr->dest_qp,n_qp);
			//printf("request is being mapped to this connection\n");
			//printf("printf qpid counter (should be == max (2)) == %d\n",qp_id_counter);
			//print_connection_state(destination_connection);


			//Multiple reads occur at once. We don't want them to overwrite
			//eachother. Initally I used Oustanding_Requests[id] to hold
			//requests. But this only ever used the one index. On parallel reads
			//the sequence number over write the old one because they collied.
			//To save time, I'm using the old extra space TOTAL_ENTRIES in the
			//Outstanding Requests to hold the concurrent reads. These should really be hased in the future.
			
			//Search for an open slot
			struct Request_Map* slot = get_empty_slot(destination_connection);

			//printf("dest connection\n");
			//print_connection_state(destination_connection);
			//printf("src connection\n");
			//print_connection_state(&Connection_States[id]);
			uint32_t msn = Connection_States[id].mseq_current;

			//Our middle box needs to keep track of the sequence number
			//that should be tracked for the destination. This should
			//always be an increment of 1 from it's previous number.
			//Here we increment the sequence number
			destination_connection->seq_current = htonl(ntohl(destination_connection->seq_current) + SEQUENCE_NUMBER_SHIFT); //There is bit shifting here.

			#ifdef MAP_PRINT
			printf("MAP FRWD(key %d) (id %d) (op: %s) :: (%d -> %d) (%d) (qpo %d -> qpn %d) \n",key, id, ib_print[roce_hdr->opcode], readable_seq(roce_hdr->packet_sequence_number), readable_seq(destination_connection->seq_current), readable_seq(msn), roce_hdr->dest_qp, destination_connection->ctsqp);
			#endif


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
			
			//Set the packet with the mapped information to the new qp
			roce_hdr->dest_qp = destination_connection->ctsqp;
			roce_hdr->packet_sequence_number = destination_connection->seq_current;
			udp_hdr->src_port = destination_connection->udp_src_port_client;

			//csum the modified packet
			uint32_t crc_check =csum_pkt_fast(pkt); //This need to be added before we can validate packets
			void * current_checksum = (void *)((uint8_t *)(ipv4_hdr) + ntohs(ipv4_hdr->total_length) - 4);
			memcpy(current_checksum,&crc_check,4);
			return;
		}
	}
}



//Mappping qp backwards is the demultiplexing operation.  The first step is to
//identify the kind of packet and figure out if it has been placed on the
//multiplexing list
void map_qp_backwards(struct rte_mbuf *pkt) {
	struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	struct rte_ipv4_hdr* ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	struct rte_udp_hdr * udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	struct roce_v2_header * roce_hdr = (struct roce_v2_header *)((uint8_t*)udp_hdr + sizeof(struct rte_udp_hdr));
	struct clover_hdr * clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));

	if (unlikely(has_mapped_qp ==  0 )) {
		printf("We have not started multiplexing yet. Returning with no packet modifictions.\n");
		return;
	}

	//We have very little information on the demultiplex side. We need to search
	//through the entire list in order to determine if this sequence number was
	//cached. We don't know the key, or the original sender so we can't do the
	//reverse lookup. In this case we have to pass over the whole list of
	//connections, then we have to look through each entry in each connection to
	//determine if there is an outstanding request.

	uint32_t search_sequence_number = roce_hdr->packet_sequence_number;
	//log_printf(DEBUG,"MAPPING BACKWARDS searching for sequence raw (%d) readable (%d) \n",search_sequence_number, readable_seq(search_sequence_number));
	struct Connection_State *source_connection;
	struct Request_Map * mapped_request;

	//TODO this loop can be removed by being able to do a get id on a server size qp
	for (int i=0;i<TOTAL_ENTRY;i++) {
		//This optimization is safe because we should not ever have more than qp enteries in the Connection State list
		if (unlikely(i > qp_id_counter)) {
			return;
		}
		source_connection=&Connection_States[i];
		if (source_connection->stcqp == roce_hdr->dest_qp) {
			break;
		}
		source_connection=NULL;
	}

	if (source_connection == NULL) {
		printf("Unable to find matching connection\n");
		return;
	}
	

	for (int j=0;j<TOTAL_ENTRY;j++) {

		mapped_request = &(source_connection->Outstanding_Requests[j]);
		//printf("looking for the outstanding request %d::: (maped,search) (%d,%d)\n",j,mapped_request->mapped_sequence,search_sequence_number);
		//First search to find if the sequence numbers match
		if (!slot_is_open(mapped_request) && mapped_request->mapped_sequence == search_sequence_number) {

			//I think that j has to be the id number here
			//log_printf(INFO,"Found! Mapping back raw (%d -> %d) readable (%d -> %d ) \n",mapped_request->mapped_sequence, mapped_request->original_sequence, readable_seq(mapped_request->mapped_sequence),readable_seq(mapped_request->original_sequence));
			//printf("QP mapping ( %d <-- %d )\n", mapped_request->server_to_client_qp, roce_hdr->dest_qp);

			//Now we need to map back
			roce_hdr->dest_qp = mapped_request->server_to_client_qp;
			roce_hdr->packet_sequence_number = mapped_request->original_sequence;
			udp_hdr->src_port = mapped_request->server_to_client_udp_port;

			//Update the tracked msn this requires adding to it, and then storing back to the connection states
			//TODO put this in it's own function
			struct Connection_State * destination_cs = &Connection_States[mapped_request->id];

			//destination_cs.mseq_current = 
			//increment_msn(destination_cs);



			//new way of doing it
			uint32_t msn = produce_and_update_msn(roce_hdr,destination_cs);
			#ifdef MAP_PRINT
			uint32_t packet_msn = get_msn(roce_hdr);
			id_colorize(mapped_request->id);
			printf("        MAP BACK :: seq(%d <- %d) mseq(%d <- %d) (op %s) (s-qp %d)\n",readable_seq(mapped_request->original_sequence),readable_seq(mapped_request->mapped_sequence), readable_seq(msn), readable_seq(packet_msn),ib_print[roce_hdr->opcode], roce_hdr->dest_qp);
			#endif
			
			set_msn(roce_hdr,msn);


			//old
			//set_msn(roce_hdr,destination_cs->mseq_current);
			//printf("Returned MSN %d\n", readable_seq(msn));


			//re ecalculate the checksum
			uint32_t crc_check =csum_pkt_fast(pkt); //This need to be added before we can validate packets
			void * current_checksum = (void *)((uint8_t *)(ipv4_hdr) + ntohs(ipv4_hdr->total_length) - 4);
			memcpy(current_checksum,&crc_check,4);

			//Remove the entry
			//TODO put this in its own function
			open_slot(mapped_request);
			return;
		} 
	}

	printf("\n\n\nThis is an interesting point\n\n\n\n\n\n");
	printf("I think this point means that mapping is turned on but we are seeing old packets TAG_TAG\n");
	if (find_and_update_stc(roce_hdr,udp_hdr) > 0) {
		printf("found and updated msn (hopefully this is the right thing to do)\n");
		uint32_t msn = find_and_update_stc(roce_hdr,udp_hdr);
		set_msn(roce_hdr,msn);
		print_packet(pkt);
		uint32_t crc_check =csum_pkt_fast(pkt); //This need to be added before we can validate packets
		void * current_checksum = (void *)((uint8_t *)(ipv4_hdr) + ntohs(ipv4_hdr->total_length) - 4);
		memcpy(current_checksum,&crc_check,4);

	}
}


void map_qp(struct rte_mbuf * pkt) {
	//Return if not mapping QP !!!THIS FEATURE SHOULD TURN ON AND OFF EASILY!!!
	if (MAP_QP == 0) {
		return;
	}

	//Not mapping yet
	if (has_mapped_qp == 0) {
		return;
	}

	struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	struct rte_ipv4_hdr* ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	struct rte_udp_hdr * udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	struct roce_v2_header * roce_hdr = (struct roce_v2_header *)((uint8_t*)udp_hdr + sizeof(struct rte_udp_hdr));
	struct clover_hdr * clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));
	uint32_t size = ntohs(ipv4_hdr->total_length);
	uint8_t opcode = roce_hdr->opcode;
	uint32_t r_qp= roce_hdr->dest_qp;

	//backward path requires little checking
	if (opcode == RC_ACK || opcode == RC_ATOMIC_ACK || opcode == RC_READ_RESPONSE) {
		map_qp_backwards(pkt);
	}


	if (qp_is_mapped(r_qp) == 0) {
		//This is not a packet we should map forward
		return;
	}


	if (opcode == RC_READ_REQUEST) {
		uint64_t stub_zero_key = 0;
		uint64_t *key = &stub_zero_key;
		map_qp_forward(pkt,*key);
	}  else if (opcode == RC_WRITE_ONLY) {
		struct write_request * wr = (struct write_request*) clover_header;
		uint64_t *key = (uint64_t*)&(wr->data);
		uint32_t id = get_id(roce_hdr->dest_qp);
		//!TODO TODO figure out what is actually going on here
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
			//printf("TODO REMOVE THIS LINE OF KEY MANIP CODE\n");
			//map_qp_forward(pkt,latest_key[id]);
			*key = latest_key[id];
		}
		map_qp_forward(pkt,*key);
	} else if (opcode == RC_CNS) {
		uint32_t id = get_id(roce_hdr->dest_qp);
		map_qp_forward(pkt,latest_key[id]);
	}
}

void track_qp(struct rte_mbuf * pkt) {
	//Return if not mapping QP !!!THIS FEATURE SHOULD TURN ON AND OFF EASILY!!!
	if (MAP_QP == 0) {
		return;
	}

	struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	struct rte_ipv4_hdr* ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	struct rte_udp_hdr * udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	struct roce_v2_header * roce_hdr = (struct roce_v2_header *)((uint8_t*)udp_hdr + sizeof(struct rte_udp_hdr));
	struct clover_hdr * clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));
	uint32_t size = ntohs(ipv4_hdr->total_length);
	uint8_t opcode = roce_hdr->opcode;



	if (opcode == RC_ACK) {
		struct rdma_ack * ack = (struct rdma_ack*) clover_header;
		if (has_mapped_qp == 0) {
			find_and_set_stc_wrapper(roce_hdr,udp_hdr);
		}

	} else if (opcode == RC_READ_REQUEST) {
		uint64_t stub_zero_key = 0;
		uint64_t *key = &stub_zero_key;
		if (has_mapped_qp == 0) {
			cts_track_connection_state(udp_hdr,roce_hdr);
		}
	} else if (opcode == RC_READ_RESPONSE) {
		if (has_mapped_qp == 0) {
			find_and_set_stc_wrapper(roce_hdr,udp_hdr);
		}

	} else if (opcode == RC_WRITE_ONLY) {
		struct write_request * wr = (struct write_request*) clover_header;
		uint64_t *key = (uint64_t*)&(wr->data);
		//flip the switch
		//!TODO pull this out and make it it's own thing at the beginning.
		if (fully_qp_init() && has_mapped_qp == 0) {
			uint32_t id = get_id(roce_hdr->dest_qp);
			map_qp_forward(pkt,*key);
		}
		if (has_mapped_qp == 0) {
			cts_track_connection_state(udp_hdr,roce_hdr);
		}


	} else if (opcode == RC_ATOMIC_ACK) {
		if (has_mapped_qp == 0) {
			find_and_set_stc_wrapper(roce_hdr,udp_hdr);
		}
	} else if (opcode == RC_CNS) {
		if (has_mapped_qp == 0) {
			cts_track_connection_state(udp_hdr,roce_hdr);
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


	map_qp(pkt);

	if (opcode == RC_ACK) {
		track_qp(pkt);

		//find_and_set_stc_qp_wrapper(roce_hdr);
		//map_qp_backwards(pkt);
		//This is purely here for testing CRC
		//uint32_t crc_check =csum_pkt_fast(pkt); //This need to be added before we can validate packets
		//crc_check =csum_pkt(pkt); //This need to be added before we can validate packets
		//printf("Finished Checksumming as single ack, time to exit (TEST)\n");
		//exit(0);
	} 

	if (size == 60 && opcode == RC_READ_REQUEST) {
		track_qp(pkt);
		//struct read_request * rr = (struct read_request *)clover_header;
		//print_read_request(rr);
		//count_read_req_addr(rr);
	}

	if ((size == 56 || size == 1072) && opcode == RC_READ_RESPONSE) {
		track_qp(pkt);
		//struct read_response * rr = (struct read_response*) clover_header;
		//print_packet(pkt);
		//print_read_response(rr, size);
		//count_read_resp_addr(rr);
	}


	//Write Requestdest_qp
			//Dangerous section
			//print_packet(pkt);

	if (opcode == RC_WRITE_ONLY) {
		if (size == 252) {
			//TODO determine what these writes are doing
			//log_printf(DEBUG,"type1 size %d\n",size);
		} else if (size == 68) {
			log_printf(DEBUG,"write (2) size %d\n",size);
			//print_packet(pkt);
			//uint32_t id = get_id(r_qp);
			//map_qp(pkt);
		} else {
			//Obtain the wite lock
			rte_rwlock_write_lock(&next_lock);
			rte_smp_mb();
			//
			struct write_request * wr = (struct write_request*) clover_header;
			//print_packet(pkt);
			uint64_t *key = (uint64_t*)&(wr->data);
			uint32_t id = get_id(r_qp);
			#ifdef DATA_PATH_PRINT
			log_printf(INFO,"ID: %d KEY: %"PRIu64"\n",id,*key);
			log_printf(INFO,"(write) Accessing remote keyspace %d size %d\n",r_qp, size);
			log_printf(INFO,"KEY: %"PRIu64"\n", *key);
			#endif
			//print_packet(pkt);
			uint32_t rdma_size = ntohl(wr->rdma_extended_header.dma_length);
			//we should only reach this block if these are write packets	
			//init write packet size
			//When performing qp muxing track the connection state
			//Keep the state updated untill it's time to actually do the forwarding
			//TODO this needs to be done on a per key basis

			if (unlikely(write_value_packet_size == 0)) {
				printf("Write Packet size %d\n",size);
				printf("RDMA DMA size %d\n",rdma_size);
				write_value_packet_size = rdma_size;
				if (write_value_packet_size == 1024) {
					predict_shift_value = 10;
				} else if ( write_value_packet_size == 512 ) {
					predict_shift_value = 9;
				} else if ( write_value_packet_size == 256 ) {
					predict_shift_value = 8;
				} else if ( write_value_packet_size == 128 ) {
					predict_shift_value = 7;
				} else if ( write_value_packet_size == 64 ) {
					predict_shift_value = 6;
				} else {
					printf("Unknown packet size (%d) exiting\n",rdma_size);
					exit(0);
				}
			}
			//Sanity check, we should only reach here if we are dealing with statically sized write packets
			if (unlikely(write_value_packet_size != rdma_size)) {
				printf("ERROR in write packet block, but packet size not correct Established runtime size %d, Found size %d\n",write_value_packet_size,size);
				exit(0);
			}

			//FOR ADJUSTING THE CACHED KEYS ONLY, todo remove post experiments Feb 20 2021
			if (*key > CACHE_KEYSPACE) {
				//This key is out of the range that we are caching
				//it still counts as a write but we have to let if through
				latest_key[id] = *key;
				log_printf(DEBUG,"not tracking key %d\n",*key);
				rte_smp_mb();
				rte_rwlock_write_unlock(&next_lock);
				return;
			}

			//printf("(write) KEY %"PRIu64" qp_id %d \n",*key,r_qp);
			if(first_write[*key] != 0 && first_cns[*key] != 0) {
				log_printf(DEBUG,"COMMON_CASE_WRITE -- predict from not addr for key %"PRIu64", for remote key space %d\n",*key,roce_hdr->partition_key);
				//predict_address[*key] = ((be64toh(wr->rdma_extended_header.vaddr) - be64toh(first_write[*key])) >> 10);
				predict_address[*key] = ((be64toh(wr->rdma_extended_header.vaddr) - be64toh(first_write[*key])) >> predict_shift_value);
				outstanding_write_predicts[id][*key] = predict_address[*key];
				outstanding_write_vaddrs[id][*key] = wr->rdma_extended_header.vaddr;
			} else {
				//okay so this happens twice becasuse the order is 
				//Write 1;
				//Write 2;
				//Cn wNS 1 (write 1 - > write 2)
				if (first_write[*key] == 0) {
					//These are dummy values for testing because I think I got the algorithm wrong
					first_write[*key] = 1;
					//next_vaddr[*kdest_qpey] = 1;
					log_printf(DEBUG,"first write is being set, this is indeed the first write for key %"PRIu64" id: %d\n",*key,id);
				} else if (first_write[*key] == 1) {
					//Lets leave this for now, but it can likely change once the cns is solved
					first_write[*key] = wr->rdma_extended_header.vaddr; //first write subject to change
					next_vaddr[*key] = wr->rdma_extended_header.vaddr;  //next_vaddr subject to change
					log_printf(DEBUG,"second write is equal to %"PRIu64" for key %"PRIu64" id: %d\n",first_write[*key],*key,id);
					outstanding_write_predicts[id][*key] = 1;
					outstanding_write_vaddrs[id][*key] = wr->rdma_extended_header.vaddr;
				} else {
					log_printf(INFO,"THIS IS WHERE THE BUGS HAPPEN CONCURRENT INIT (might crash)!!!! ID: %d KEY: %d\n",id,*key);
					log_printf(INFO,"Trying to save the ship by hoping the prior write makes it through first\n");
					outstanding_write_predicts[id][*key] = 1;
					outstanding_write_vaddrs[id][*key] = wr->rdma_extended_header.vaddr;
					log_printf(INFO,"crash write full write is equal to %"PRIu64" for key %"PRIu64" id: %d\n",first_write[*key],*key,id);
				}
			}
			latest_key[id] = *key;
			track_qp(pkt);
			rte_smp_mb();
			rte_rwlock_write_unlock(&next_lock);


			/*
			#ifdef PACKET_DEBUG_PRINTOUT
			//Count the big writes, this is mostly for testing
			if (size >= 1084) {
				//printf("key %02X %02X %02X %02X \n",key[0], key[1], key[2], key[3]);
				//Update current write kv location
				key_address[*key] = wr->rdma_extended_header.vaddr;
				//Update the most recent version of the kv store
				key_versions[*key][key_count[*key]%KEY_VERSION_RING_SIZE]=wr->rdma_extended_header.vaddr;
				//update the keys write count
				key_count[*key]++;
			} else {
				printf("size too small to print extra data\n");
			}

			//Periodically print the sate of a particular key.
			if (*key == 1 && key_count[*key]==KEY_VERSION_RING_SIZE) {
				for (int i=0;i<KEY_VERSION_RING_SIZE;i++){
					printf("key: %"PRIu64" address:%"PRIu64" index: %d\n",*key,key_versions[*key][i], i+(key_count[*key]-KEY_VERSION_RING_SIZE));
				}
			}
			#endif
			*/
		} 
	}

    if (opcode == RC_ATOMIC_ACK) {
		//printf("atomic ack\n");
		struct cs_response * csr = (struct cs_response*) clover_header;
		//printf("original contents A%"PRIu64"\n",be64toh(csr->atomc_ack_extended.original_remote_data));

		uint32_t original = ntohl(csr->atomc_ack_extended.original_remote_data);
		if (original != 0) {
			nacked_cns++;
			printf("nacked cns %d\n",nacked_cns);
			printf("SHOULD BE EXITING (if you see this check cache!!!\n");
			print_packet(pkt);
			exit(0);
		} 
		#ifdef DATA_PATH_PRINT
		log_printf(DEBUG,"CNS ACK seq: %d dest qp: %d\n",readable_seq(roce_hdr->packet_sequence_number),roce_hdr->dest_qp);
		#endif
		track_qp(pkt);
	}

	if (size == 72 && opcode == RC_CNS) {

		rte_rwlock_write_lock(&next_lock);
		rte_smp_mb();
		//cts_track_connection_state(udp_hdr,roce_hdr);

		struct cs_request * cs = (struct cs_request*) clover_header;
		uint64_t swap = MITSUME_GET_PTR_LH(be64toh(cs->atomic_req.swap_or_add));
		swap = htobe64(swap);


		uint32_t id = get_id(r_qp);
		//printf("Latest id KEY: id: %d, key %"PRIu64"\n",id, latest_key[id]);


		//This is the first instance of the cns for this key, it is a misunderstood case
		//For now return after setting the first instance of the key to the swap value

		if(latest_key[id] > CACHE_KEYSPACE) {
			//this key is not being tracked, return
			log_printf(INFO,"(cns) Returning key not tracked no need to adjust %"PRIu64"\n",latest_key[id]);

			track_qp(pkt);
			rte_smp_mb();
			rte_rwlock_write_unlock(&next_lock);
			return;
		}


		if (first_cns[latest_key[id]] == 0) {
			//log_printf(INFO,"setting swap for key %"PRIu64" id: %d -- Swap %"PRIu64"\n", latest_key[id],id, swap);
			first_cns[latest_key[id]] = swap;
			first_write[latest_key[id]] = outstanding_write_vaddrs[id][latest_key[id]];
			next_vaddr[latest_key[id]] = outstanding_write_vaddrs[id][latest_key[id]];

			for (uint i=0;i<qp_id_counter;i++) {
				if (outstanding_write_predicts[i][latest_key[id]] == 1) {
					//log_printf(INFO,"(init conflict dected) recalculating outstanding writes for key %"PRIu64" id\n",latest_key[id],id);
					uint64_t predict = ((be64toh(outstanding_write_vaddrs[i][latest_key[id]]) - be64toh(first_write[latest_key[id]])) >> 10);
					outstanding_write_predicts[i][latest_key[id]] = predict;
				}
			}
			//Return and forward the packet if this is the first cns

			track_qp(pkt);
			rte_smp_mb();
			rte_rwlock_write_unlock(&next_lock);
			return;
		}

		//Based on the key predict where the next CNS address should go. This requires the first CNS to be set
		uint32_t key = latest_key[id];
		uint64_t predict = outstanding_write_predicts[id][key];
		//printf("Raw predict %d\n",predict);
		predict = predict + be64toh(first_cns[key]);
		predict = htobe64( 0x00000000FFFFFF & predict); // THIS IS THE CORRECT MASK
		//predict = htobe64( 0x00000000FFFFFFFF & predict);

		#ifdef DATA_PATH_PRINT
		log_printf(DEBUG,"(cns) KEY %d qp_id %d seq %d \n",key,r_qp,readable_seq(roce_hdr->packet_sequence_number));
		#endif


		//Here we have had a first cns (assuming bunk, and we eant to point to the latest in the list)
		if (next_vaddr[latest_key[id]] == cs->atomic_req.vaddr) {
			#ifdef DATA_PATH_PRINT
			log_printf(DEBUG,"this is good, it seems we made the correct prediction, this is the common case Key %d ID %d\n",latest_key[id],id);
			#endif
			
		} else {
			/*
			printf("\n\n\n\n\n SWAPPPING OUT THE VADDR!!!!!!!"
			"key %"PRIu64" id %d" 
			"\n\n\n\n\n",latest_key[id],id);
			printf("Now we need to know how different these addresses are\n");
			printf("next addr[key = %"PRIu64"] ID: %d vaddr %"PRIu64"\n",latest_key[id],id,next_vaddr[latest_key[id]]);
			printf("cs addr %"PRIu64"\n",cs->atomic_req.vaddr);
			//Modify the next cns to poinnt to the last scene write
			*/

			//vaddr_swaps++;
			//if (vaddr_swaps % 10000 == 0) {
			//	printf("virtual memory swaps %"PRIu64" packets %"PRIu64"\n",vaddr_swaps,packet_counter);
			//}

			//THIS IS TO Measure conflicts
			//#ifdef DONT_SWAP_VADDR
			//rte_rwlock_write_unlock(&next_lock);
			//return;
			//#endif
			cs->atomic_req.vaddr = next_vaddr[latest_key[id]]; //We can add this once we can predict with confidence
			//modify the ICRC checksum
			uint32_t crc_check =csum_pkt_fast(pkt); //This need to be added before we can validate packets
			void * current_checksum = (void *)((uint8_t *)(ipv4_hdr) + ntohs(ipv4_hdr->total_length) - 4);
			memcpy(current_checksum,&crc_check,4);
			

		}

		//given that a cns has been determined move the next address for this 
		//key, to the outstanding write of the cns that was just made
		if (likely(predict == swap)) {
			next_vaddr[latest_key[id]] = outstanding_write_vaddrs[id][key];
			//erase the old entries
			outstanding_write_predicts[id][latest_key[id]] = 0;
			outstanding_write_vaddrs[id][latest_key[id]] = 0;
			//printf("the next tail of the list for key %"PRIu64" has been found to be id: %d vaddr: %"PRIu64"\n",latest_key[id],id,next_vaddr[latest_key[id]]);
		} else {
			//This is the crash condtion
			//Fatal, unable to find the next key
			printf("Crasing on (CNS PREDICT) ID: %d psn %d\n",id,readable_seq(roce_hdr->packet_sequence_number));
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
			exit(0);
		}
		track_qp(pkt);
		rte_smp_mb();
		rte_rwlock_write_unlock(&next_lock);

	}

	return;
}


void print_classify_packet_size(void) {
	for (int i=0;i<RDMA_COUNTER_SIZE;i++) {
		for (int j=0;j<PACKET_SIZES;j++) {
			if(packet_size_index[i][j] != 0)
				printf("Call: %s Size: %"PRIu64", calls: %d\n",ib_print[i],packet_size_index[i][j],packet_size_calls[i][j]);
		}
	}
	printf("----------------------------\n");
}


void rdma_count_calls(roce_v2_header *rdma) {
	rdma_call_count[rdma->opcode]++;
	return;
}


void print_rdma_call_count(void) {
	if (rdma_counter % 100 == 0) {
		for (int i=0;i<RDMA_COUNTER_SIZE;i++) {
			if (rdma_call_count[i] > 0) {
				printf("Call: %s Count: %d Raw: %02X\n",ib_print[i],rdma_call_count[i],i);
			}
		}

	}
	return;
}


//ib_print[RC_ACK] = "RC_ACK\0";
void init_ib_words(void) {
	strcpy(ib_print[RC_SEND],"RC_SEND");
	strcpy(ib_print[RC_WRITE_ONLY],"RC_WRITE_ONLY");
	strcpy(ib_print[RC_READ_REQUEST],"RC_READ_REQUEST");
	strcpy(ib_print[RC_READ_RESPONSE],"RC_READ_RESPONSE");
	strcpy(ib_print[RC_ACK],"RC_ACK");
	strcpy(ib_print[RC_ATOMIC_ACK],"RC_ATOMIC_ACK");
	strcpy(ib_print[RC_CNS],"RC_COMPARE_AND_SWAP");
}


int log_printf(int level, const char *format, ...) {
	va_list args;
    va_start(args, format);
	int ret = 0;
	if (unlikely(LOG_LEVEL >= level)) {
		ret = vprintf(format,args);
	}
	va_end(args);
	return ret;
}

static const struct rte_eth_conf port_conf_default = {
	.rxmode = {
		.max_rx_pkt_len = RTE_ETHER_MAX_LEN,
	},
};

#define RSS_HASH_KEY_LENGTH 40 // for mlx5
uint64_t rss_hf = ETH_RSS_NONFRAG_IPV4_UDP; //ETH_RSS_UDP | ETH_RSS_TCP | ETH_RSS_IP;// | ETH_RSS_VLAN; /* RSS IP by default. */

//uint64_t rss_hf = ETH_RSS_NONFRAG_IPV4_UDP;
//uint64_t rss_hf = 0;
//rss_hf = 0;
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

void print_raw(struct rte_mbuf* pkt){
	printf("\n\n\n\n----(start-raw) (new packet)\n\n");
	int room = rte_pktmbuf_headroom(pkt);
	for (int i=rte_pktmbuf_headroom(pkt);(uint16_t)i<(pkt->data_len + rte_pktmbuf_headroom(pkt));i++){
		printf("%02X ",(uint8_t)((char *)(pkt->buf_addr))[i]);
		if (i - room == sizeof(struct rte_ether_hdr) - 1) { // eth
			printf("|\n");
		}
		if (i - room == sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) - 1) { // eth
			printf("|\n");
		}
		if (i  - room == sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_udp_hdr) - 1) { // eth
			printf("|\n");
		}
		if (i  - room == sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_udp_hdr) + sizeof(struct roce_v2_header) - 1) { // eth
			printf("|\n");
		}
		if (i  - room == sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_udp_hdr) + sizeof(struct roce_v2_header) + sizeof(struct mitsume_msg_header) -1 ) {
			printf("|\n");
		}
		//printf("%c-",((char *)pkt->userdata)[itter]);
	}

	printf("fullraw:\n");
	for (int i=rte_pktmbuf_headroom(pkt);(uint16_t)i<(pkt->data_len + rte_pktmbuf_headroom(pkt));i++){
	//for (int i=rte_pktmbuf_headroom(pkt) + sizeof(struct rte_ether_hdr);(uint16_t)i<(pkt->data_len + rte_pktmbuf_headroom(pkt));i++){
		printf("%02X",(uint8_t)((char *)(pkt->buf_addr))[i]);
		}
		//printf("%c-",((char *)pkt->userdata)[itter]);
	printf("\n");
	printf("fullraw ascii:\n");
	//for (int i=rte_pktmbuf_headroom(pkt);(uint16_t)i<(pkt->data_len + rte_pktmbuf_headroom(pkt));i++){
	for (int i=rte_pktmbuf_headroom(pkt) + sizeof(struct rte_ether_hdr);(uint16_t)i<(pkt->data_len + rte_pktmbuf_headroom(pkt));i++){
		printf("%c",(uint8_t)((char *)(pkt->buf_addr))[i]);
		}
		//printf("%c-",((char *)pkt->userdata)[itter]);
	printf("\n");
	printf("\n----(end-raw)----\n");
}

void print_ether_hdr(struct rte_ether_hdr * eth){
	// L2 headers
	struct rte_ether_addr src_macaddr;
	struct rte_ether_addr dst_macaddr;	

	src_macaddr = eth->s_addr;
	dst_macaddr = eth->d_addr;
	printf("src_macaddr: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
		" %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
		src_macaddr.addr_bytes[0], src_macaddr.addr_bytes[1],
		src_macaddr.addr_bytes[2], src_macaddr.addr_bytes[3],
		src_macaddr.addr_bytes[4], src_macaddr.addr_bytes[5]);

	printf("dst_macaddr: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
		" %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
		dst_macaddr.addr_bytes[0], dst_macaddr.addr_bytes[1],
		dst_macaddr.addr_bytes[2], dst_macaddr.addr_bytes[3],
		dst_macaddr.addr_bytes[4], dst_macaddr.addr_bytes[5]);

	return;
}

struct rte_ether_hdr *eth_hdr_process(struct rte_mbuf* buf) {
	struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(buf, struct rte_ether_hdr *);

	if(eth_hdr->ether_type == rte_be_to_cpu_16(RTE_ETHER_TYPE_IPV4)){									

		#ifdef TURN_PACKET_AROUND
		//Swap ethernet addresses
		struct rte_ether_addr temp_eth_addr   = eth_hdr->s_addr;
		eth_hdr->s_addr = eth_hdr->d_addr;
		eth_hdr->d_addr = temp_eth_addr;
		#endif

		#ifdef PACKET_DEBUG_PRINTOUT

		print_ether_hdr(eth_hdr);
		print_raw(buf);

		#endif
		return eth_hdr;
	}
	return NULL;
}

void print_ip_hdr(struct rte_ipv4_hdr * ipv4_hdr) {
	// L3 headers: IPv4
	uint32_t dst_ipaddr;
	uint32_t src_ipaddr;

	src_ipaddr = rte_be_to_cpu_32(ipv4_hdr->src_addr);
	dst_ipaddr = rte_be_to_cpu_32(ipv4_hdr->dst_addr);
	uint8_t src_addr[4];
	src_addr[0] = (uint8_t) (src_ipaddr >> 24) & 0xff;
	src_addr[1] = (uint8_t) (src_ipaddr >> 16) & 0xff;
	src_addr[2] = (uint8_t) (src_ipaddr >> 8) & 0xff;
	src_addr[3] = (uint8_t) src_ipaddr & 0xff;
	printf("src_addr: %" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8 "\n", 
			src_addr[0], src_addr[1], src_addr[2], src_addr[3]);

	uint8_t dst_addr[4];
	dst_addr[0] = (uint8_t) (dst_ipaddr >> 24) & 0xff;
	dst_addr[1] = (uint8_t) (dst_ipaddr >> 16) & 0xff;
	dst_addr[2] = (uint8_t) (dst_ipaddr >> 8) & 0xff;
	dst_addr[3] = (uint8_t) dst_ipaddr & 0xff;
	printf("dst_addr: %" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8 "\n", 
		dst_addr[0], dst_addr[1], dst_addr[2], dst_addr[3]);
	return;

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
		
		#ifdef PACKET_DEBUG_PRINTOUT
		print_ipv4_hdr(ipv4_hdr);
		#endif		

		return ipv4_hdr;
	}
	return NULL;
}

void print_udp_hdr(struct rte_udp_hdr * udp_hdr) {
	// L4 headers: UDP 
	uint16_t dst_port = 0;
	uint16_t src_port = 0;
	dst_port = rte_be_to_cpu_16(udp_hdr->dst_port);
	src_port = rte_be_to_cpu_16(udp_hdr->src_port);
	//Because of the way we fill in these data, we don't need rte_be_to_cpu_32 or rte_be_to_cpu_16 
	printf("src_port:%" PRIu16 ", dst_port:%" PRIu16 "\n", src_port, dst_port);
	printf("-------------------\n");
	return;

}

struct rte_udp_hdr * udp_hdr_process(struct rte_ipv4_hdr *ipv4_hdr) {

	//ipv4_udp_rx++;
	//log_printf(INFO,"ipv4_udp_rx:%" PRIu16 "\n",ipv4_udp_rx);

	struct rte_udp_hdr * udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	if (ipv4_hdr->next_proto_id == IPPROTO_UDP){

		#ifdef TURN_PACKET_AROUND
		//Swap udp ports
		uint16_t temp_udp_port = udp_hdr->src_port;
		udp_hdr->src_port = udp_hdr->dst_port; 
		udp_hdr->dst_port = temp_udp_port;							
		udp_hdr->dgram_len = rte_cpu_to_be_16(sizeof(struct alt_header) + sizeof(struct rte_udp_hdr));
		//alt = (struct alt_header *)((uint8_t *)udp_hdr + sizeof(struct rte_udp_hdr));
		#endif

		#ifdef PACKET_DEBUG_PRINTOUT
		print_udp_hdr(udp_hdr);
		#endif

		//udp_hdr->dgram_cksum = 0;									
		//udp_hdr->dgram_cksum = rte_ipv4_udptcp_cksum(ipv4_hdr, (void*)udp_hdr);
		//printf("udp src port : %d\n",udp_hdr->src_port);
		return udp_hdr;
	}
	return NULL;
}

void print_roce_v2_hdr(struct roce_v2_header * rh) {
    printf("op code             %02X %s\n",rh->opcode, ib_print[rh->opcode]);
    printf("solicited event     %01X\n",rh->solicited_event);
    printf("migration request   %01X\n",rh->migration_request);
    printf("pad count           %01X\n",rh->pad_count);
    printf("transport version   %01X\n",rh->transport_header_version);
    printf("partition key       %02X\n",rh->partition_key);
    printf("fecn                %01X\n",rh->fecn);
    printf("becn                %01X\n",rh->bcen);
    printf("reserved            %01X\n",rh->reserverd);
    printf("dest qp             %02X\n",rh->dest_qp);
    printf("ack                 %01X\n",rh->ack);
    printf("reserved            %01X\n",rh->reserved);
    printf("packet sequence #   %02X HEX %d DEC\n",rh->packet_sequence_number, rh->packet_sequence_number);


	struct clover_hdr * clover_header = (struct clover_hdr *)((uint8_t *)rh + sizeof(roce_v2_header));

	switch(rh->opcode) {
		case RC_SEND:
			printf("roce send\n");
			break;
		case RC_WRITE_ONLY:
			printf("roce write\n");
			struct write_request *wr = (struct write_request*) clover_header;
			print_write_request(wr);
			break;
		case RC_READ_REQUEST:
			printf("read_request\n");
			struct read_request * read_req = (struct read_request *)clover_header;
			print_read_request(read_req);
			break;
		case RC_READ_RESPONSE:
			printf("roce read\n");
			struct read_response * read_resp = (struct read_response*) clover_header;
			print_read_response(read_resp);
			break;
		case RC_ACK:
			printf("roce ack\n");
			break;
		case RC_ATOMIC_ACK:
			printf("atomic_req\n");
			struct cs_response * cs_resp = (struct cs_response *)clover_header;
			print_cs_response(cs_resp);
			break;
		case RC_CNS:
			printf("atomic_ack\n");
			struct cs_request * cs_req = (struct cs_request *)clover_header;
			print_cs_request(cs_req);
			break;
		default:
			printf("DEFAULT RDMA NOT HANEDLED\n");
			break;
	}
}

struct roce_v2_header * roce_hdr_process(struct rte_udp_hdr * udp_hdr) {
	//Dont start parsing if the udp port is not roce
	struct roce_v2_header * roce_hdr = NULL;
	if (likely(rte_be_to_cpu_16(udp_hdr->dst_port) == ROCE_PORT)) {
		roce_hdr = (struct roce_v2_header *)((uint8_t*)udp_hdr + sizeof(struct rte_udp_hdr));

		rdma_counter++;

		#ifdef PACKET_DEBUG_PRINTOUT
		print_roce_v2_header(roce_hdr);
		#endif

		return roce_hdr;
	}
	return NULL;
}

void print_clover_hdr(struct clover_hdr * clover_header) {
		printf("-----------------------------------------\n");
		printf("size of rocev2 header = %ld\n",sizeof(struct roce_v2_header));
		printf("CLOVER MESSAGE TIME\n");

		printf("((potential first 8 byte addr ");
		print_bytes((uint8_t *)&clover_header->ptr.pointer, sizeof(uint64_t));
		printf("\n");

		struct mitsume_msg * clover_msg;
		clover_msg = &(clover_header->mitsume_hdr);
		struct mitsume_msg_header *header = &(clover_msg->msg_header);

		printf("msg-type  %d ntohl %d\n",header->type,ntohl(header->type));
		printf("source id %d ntohl %d\n",header->src_id,ntohl(header->src_id));
		printf("dest id %d ntohl %d\n",header->des_id,ntohl(header->des_id));
		printf("thread id %d ntohl %d \n",header->thread_id, ntohl(header->thread_id));

		printf("(ib_mr_attr) -- Addr");
		print_bytes((uint8_t *) &header->reply_attr.addr, sizeof(uint64_t));
		printf("\n");

		printf("(ib_mr_attr) -- rkey %d\n",ntohl(header->reply_attr.rkey));
		printf("(ib_mr_attr) -- mac id %d\n",ntohs(header->reply_attr.machine_id));

}

struct clover_hdr * mitsume_msg_process(struct roce_v2_header * roce_hdr){

	struct clover_hdr * clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));

	#ifdef PACKET_DEBUG_PRINTOUT
	print_clover_header(clover_hdr);
	#endif 

	return clover_header;
}

void print_packet_lite(struct rte_mbuf * buf) {
	struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(buf, struct rte_ether_hdr *);
	struct rte_ipv4_hdr* ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	struct rte_udp_hdr * udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	struct roce_v2_header * roce_hdr = (struct roce_v2_header *)((uint8_t*)udp_hdr + sizeof(struct rte_udp_hdr));

	char * op = ib_print[roce_hdr->opcode];
	uint32_t size = ntohs(ipv4_hdr->total_length);
	uint32_t dest_qp = roce_hdr->dest_qp;
	uint32_t seq = readable_seq(roce_hdr->packet_sequence_number);

	printf("[op:%s (%d)][size: %d][dst: %d][seq %d]\n",op,roce_hdr->opcode,size,dest_qp,seq);

	if (roce_hdr->opcode == 129) {
		print_packet(buf);
		uint8_t ecn = ipv4_hdr->type_of_service;
		printf("\n\n\n");
		printf("ECN TOS %X ntoh(%X)\n",ecn,ntohs(ecn));
		printf("\n\n\n");
	}

}

void print_packet(struct rte_mbuf * buf) {
	struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(buf, struct rte_ether_hdr *);
	struct rte_ipv4_hdr* ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	struct rte_udp_hdr * udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	struct roce_v2_header * roce_hdr = (struct roce_v2_header *)((uint8_t*)udp_hdr + sizeof(struct rte_udp_hdr));
	struct clover_hdr * clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));
	print_raw(buf);
	print_ether_hdr(eth_hdr);
	print_ip_hdr(ipv4_hdr);
	print_udp_hdr(udp_hdr);
	print_roce_v2_hdr(roce_hdr);
	//print_clover_hdr(clover_header);

}

static __inline__ int64_t rdtsc_s(void)
{
  unsigned a, d; 
  asm volatile("cpuid" ::: "%rax", "%rbx", "%rcx", "%rdx");
  asm volatile("rdtsc" : "=a" (a), "=d" (d)); 
  return ((unsigned long)a) | (((unsigned long)d) << 32); 
}

static __inline__ int64_t rdtsc_e(void)
{
  unsigned a, d; 
  asm volatile("rdtscp" : "=a" (a), "=d" (d)); 
  asm volatile("cpuid" ::: "%rax", "%rbx", "%rcx", "%rdx");
  return ((unsigned long)a) | (((unsigned long)d) << 32); 
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

	/* Run until the application is quit or killed. */
	struct rte_ether_hdr* eth_hdr;
	struct rte_ipv4_hdr *ipv4_hdr; 
	struct rte_udp_hdr* udp_hdr;
	struct roce_v2_header * roce_hdr;
	struct clover_hdr * clover_header;


	for (;;) {
		/*
		 * Receive packets on a port and forward them on the paired
		 * port. The mapping is 0 -> 1, 1 -> 0, 2 -> 3, 3 -> 2, etc.
		 */

		RTE_ETH_FOREACH_DEV(port) {
			uint16_t ipv4_udp_rx = 0;	

			/* Get burst of RX packets, from first and only port */
			struct rte_mbuf *rx_pkts[BURST_SIZE];
			//printf("%X bufs\n",&rx_pkts[0]);

			uint32_t queue = rte_lcore_id()/2;
			const uint16_t nb_rx = rte_eth_rx_burst(port, queue, rx_pkts, BURST_SIZE);
			//const uint16_t nb_rx = rte_eth_rx_burst(port, 0, rx_pkts, BURST_SIZE);
			//uint32_t current_ring =  rand() %2;
			//printf("currently reading from ring %d\n",current_ring);
			//const uint16_t nb_rx = rte_eth_rx_burst(port, current_ring, rx_pkts, BURST_SIZE);
			
			if (unlikely(nb_rx == 0))
				continue;

			//log_printf(INFO,"rx:%" PRIu16 "\n",nb_rx);			

			for (uint16_t i = 0; i < nb_rx; i++){
				if (likely(i < nb_rx - 1))
					rte_prefetch0(rte_pktmbuf_mtod(rx_pkts[i+1],void *));
				

				packet_counter++;

				eth_hdr = eth_hdr_process(rx_pkts[i]);
				if (unlikely(eth_hdr == NULL)) {
					log_printf(DEBUG, "ether header not the correct format dropping packet\n");
					rte_pktmbuf_free(rx_pkts[i]);
					continue;
				}

				ipv4_hdr = ipv4_hdr_process(eth_hdr);
				if (unlikely(ipv4_hdr == NULL)) {
					log_printf(DEBUG, "ipv4 header not the correct format dropping packet\n");
					rte_pktmbuf_free(rx_pkts[i]);
					continue;
				}

				udp_hdr = udp_hdr_process(ipv4_hdr);
				if (unlikely(udp_hdr == NULL)) {
					log_printf(DEBUG, "udp header not the correct format dropping packet\n");
					rte_pktmbuf_free(rx_pkts[i]);
					continue;
				}

				roce_hdr = roce_hdr_process(udp_hdr);
				if (unlikely(roce_hdr == NULL)) {
					log_printf(DEBUG, "roceV2 header not correct dropping packet\n");
					rte_pktmbuf_free(rx_pkts[i]);
					continue;
				}

				/*
				clover_header = mitsume_msg_process(roce_hdr);
				if (unlikely(clover_header == NULL)) {
					log_printf(DEBUG, "clover msg not parsable for some reason\n");
					rte_pktmbuf_free(rx_pkts[i]);
					continue;
				}
*/
				#ifdef PACKET_DEBUG_PRINTOUT
				classify_packet_size(ipv4_hdr,roce_hdr);
				if (packet_counter % 1000000 == 0) {
					print_classify_packet_size();
				}
				#endif


				//rte_rwlock_write_lock(&next_lock);
				log_printf(DEBUG,"Packet Start\n");
				int64_t clocks_before = rdtsc_s ();
				true_classify(rx_pkts[i]);
				int64_t clocks_after = rdtsc_e ();
				int64_t clocks_per_packet = clocks_after - clocks_before;

				if (roce_hdr->opcode == RC_ACK) {
					append_packet_latency(clocks_per_packet);
				}
				
				if (debug_start_printing_every_packet != 0) {
					//print_packet(rx_pkts[i]);
					print_packet_lite(rx_pkts[i]);
				}
				//printf("cpp %"PRIu64"\n",clocks_per_packet);
				log_printf(DEBUG,"Packet End\n\n");
				//rte_rwlock_write_unlock(&next_lock);

				/*
				uint32_t core_id = rte_lcore_id() / 2;
				core_pkt_counters[core_id]++;
				if ((core_pkt_counters[core_id] % 10000) == 0) {
					for(int i=0;i<MAX_CORES;i++) {
						if (core_pkt_counters[i] != 0) {
							printf("core_id %d -- pkts %d\n",i,core_pkt_counters[i]);
						}
					}
				}*/


				//this must be recomputed if the packet is changed
				uint16_t ipcsum, old_ipcsum;
				old_ipcsum = ipv4_hdr->hdr_checksum;
				ipv4_hdr->hdr_checksum = 0;
				ipcsum = rte_ipv4_cksum(ipv4_hdr);
				if (ipcsum != old_ipcsum) {
					printf("someting in the packet changed, the csums don't aline (org/new) (%d/%d) \n",ipv4_hdr->hdr_checksum,ipcsum);
				}
				//ipv4_hdr->hdr_checksum = old_ipcsum;
				ipv4_hdr->hdr_checksum = ipcsum;

				//rte_pktmbuf_free(rx_pkts[i]);
			}							
			//log_printf(INFO,"rx:%" PRIu16 ",udp_rx:%" PRIu16 "\n",nb_rx, ipv4_udp_rx);	

			/* Send burst of TX packets, to the same port */
			//const uint16_t nb_tx = rte_eth_tx_burst(port, 0, rx_pkts, nb_rx);
			const uint16_t nb_tx = rte_eth_tx_burst(port, queue, rx_pkts, nb_rx);
			//printf("rx:%" PRIu16 ",tx:%" PRIu16 ",udp_rx:%" PRIu16 "\n",nb_rx, nb_tx, ipv4_udp_rx);
			//printf("rx:%" PRIu16 ",tx:%" PRIu16 "\n",nb_rx, nb_tx);

			/* Free any unsent packets. */
			 if (unlikely(nb_tx < nb_rx)) {
				log_printf(DEBUG, "Freeing packets that were not sent %d",nb_rx - nb_tx);
			 	uint16_t buf;
			 	for (buf = nb_tx; buf < nb_rx; buf++)
			 		rte_pktmbuf_free(rx_pkts[buf]);
			 }
		}
	}
}

void debug_icrc(struct rte_mempool *mbuf_pool) {
	printf("debugging CRC using a cached packet\n");
	struct rte_mbuf* buf = rte_pktmbuf_alloc(mbuf_pool);
	struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(buf, struct rte_ether_hdr *);
	uint16_t pkt_len = 62;
	memcpy(eth_hdr,test_ack_pkt,pkt_len);
	buf->pkt_len=pkt_len;
	buf->data_len=pkt_len;
	//printf("%d\n",test_ack_pkt[0]);
	//memcpy(eth_hdr,test_ack_pkt,1);
	printf("pkt copied\n");
	print_packet(buf);
	csum_pkt_fast(buf);
	exit(0);
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
	printf("Captured Signal (%d)\n",sig);
	write_packet_latencies_to_known_file();
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
	//if (nb_ports < 2 || (nb_ports & 1))
	//	rte_exit(EXIT_FAILURE, "Error: number of ports must be even\n");

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
		bzero(core_pkt_counters,MAX_CORES*sizeof(uint32_t));
		bzero(outstanding_write_predicts,TOTAL_ENTRY*KEYSPACE*sizeof(uint64_t));
		bzero(outstanding_write_vaddrs,TOTAL_ENTRY*KEYSPACE*sizeof(uint64_t));
		bzero(next_vaddr,KEYSPACE*sizeof(uint64_t));

		bzero(packet_latencies,TOTAL_PACKET_LATENCIES*sizeof(uint64_t));

		init_connection_states();
		init_hash();
		write_value_packet_size=0;
		predict_shift_value=0;
		has_mapped_qp=0;
		init = 1;
	}
	rte_rwlock_init(&next_lock);

	init_ib_words();
	/* Call lcore_main on the master core only. */
	//debug_icrc(mbuf_pool);

	fork_lcores();


	lcore_main();

	return 0;
}
