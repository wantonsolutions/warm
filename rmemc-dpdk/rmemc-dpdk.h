#ifndef RMEMC_DPDK_H
#define RMEMC_DPDK_H

#include "clover_structs.h"
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>

//macro magic
#define DO_EXPAND(VAL) VAL##1
#define EXPAND(VAL) DO_EXPAND(VAL)

#define DEBUG 2
#define INFO 1
#define NONE 0

//#define LOG_LEVEL DEBUG
#define LOG_LEVEL NONE

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

//#define NUM_MBUFS 8191
#define NUM_MBUFS 8191 * 16
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

#define TOTAL_ENTRY 128
#define CS_SLOTS 1024

#define MITSUME_BENCHMARK_THREAD_DEFAULT 2
#if !defined(MITSUME_BENCHMARK_THREAD_NUM) || (EXPAND(MITSUME_BENCHMARK_THREAD_NUM) == 1)
//Only here if MYVARIABLE is not defined
//OR MYVARIABLE is the empty string
#undef MITSUME_BENCHMARK_THREAD_NUM
#define MITSUME_BENCHMARK_THREAD_NUM MITSUME_BENCHMARK_THREAD_DEFAULT
#endif

//checksum

struct Request_Map
{
  uint32_t open;
  uint32_t id;
  uint32_t original_sequence;
  uint32_t original_src_ip;
  uint32_t mapped_sequence;
  uint32_t server_to_client_qp;
  uint32_t mapped_destination_server_to_client_qp;
  uint16_t server_to_client_udp_port;
  uint32_t server_to_client_rkey;
  uint8_t original_eth_addr[6];
  uint8_t rdma_op;
  uint8_t was_write_swapped;
} Request_Map;

struct Connection_State
{
  uint32_t id;                  // ID of the destination connection
  uint16_t udp_src_port_client; // udp port of the client (dynamic)
  uint16_t udp_src_port_server; // udp port of the server (should be static RDMA port)
  uint32_t ip_addr_client;      // per machine IP addresses, clients may have many
  uint32_t ip_addr_server;      // memory server address
  uint32_t cts_rkey;            // rkey of the client to server connection
  uint32_t stc_rkey;            // rkey of the server to client connection
  uint32_t ctsqp;               // client to server qp identifier (per run specific)
  uint32_t stcqp;               // server to client qp identifier, must be captured from acking packets
  uint8_t cts_eth_addr[6];      // client to server eth addr
  uint8_t stc_eth_addr[6];      // server to client eth adder
  uint32_t sender_init;         // client side initalized
  uint32_t receiver_init;       // server side initialized
  uint32_t last_seq;            // (deprecated) last sequence observed
  int32_t mseq_offset;          // message sequence offset from the first seqeunce number
  //Past here the variables actually change while the program is running in the fast path
  rte_rwlock_t cs_lock;
  uint32_t seq_current;  // Packet sequence number
  uint32_t mseq_current; // message sequence number (for acks/cns acks/reads in ATEH header)
  //Slot for storing outstanding requests on this connection
  struct Request_Map Outstanding_Requests[CS_SLOTS];
} Connection_State;

struct map_packet_response
{
  uint32_t size;
  struct rte_mbuf *pkts[BURST_SIZE*BURST_SIZE];
  uint64_t timestamps[BURST_SIZE*BURST_SIZE];
} map_packet_response;

//Locking
void lock_qp(void);
void unlock_qp(void);
void lock_mem_qp(void);
void unlock_mem_qp(void);
void lock_next(void);
void unlock_next(void);

uint32_t check_sums(const char *method, void *known, void *test, int try);
uint32_t check_sums_wrap(const char *method, void *know, void *test);
uint32_t csum_pkt_fast(struct rte_mbuf *pkt);
uint32_t csum_pkt(struct rte_mbuf *pkt);

int init_hash(void);
int set_id(uint32_t qp, uint32_t id);
uint32_t get_id(uint32_t qp);
uint32_t readable_seq(uint32_t seq);

void count_values(uint64_t *index, uint32_t *count, uint32_t size, uint64_t value);
void count_read_req_addr(struct read_request *rr);
void classify_packet_size(struct rte_ipv4_hdr *ip, struct roce_v2_header *roce);
void true_classify(struct rte_mbuf *pkt);
void rdma_print_pattern(roce_v2_header *rdma);
void init_ib_words(void);
void init_connection_state(struct rte_mbuf *pkt);

uint32_t get_predicted_shift(uint32_t packet_size);
void check_and_cache_predicted_shift(uint32_t rdma_size);
void catch_ecn(struct rte_mbuf *pkt, uint8_t opcode);
void catch_nack(struct clover_hdr *clover_header, uint8_t opcode);

//Packet Buffering
void init_reorder_buf(void);
void finish_mem_pkt(struct rte_mbuf *pkt, uint16_t port, uint32_t queue);

//qp tracking
uint32_t key_to_qp(uint64_t key);
void update_cs_seq(uint32_t stc_dest_qp, uint32_t seq);
void cts_track_connection_state(struct rte_mbuf *pkt);
void find_and_set_stc_qp(uint32_t stc_dest_qp, uint32_t seq);
void find_and_set_stc_qp_wrapper(struct roce_v2_header *roce_hdr);
void update_cs_seq_wrapper(struct roce_v2_header *roce_hdr);
int coretest(__attribute__((unused)) void *arg);
void fork_lcores(void);
void init_cs_wrapper(struct rte_mbuf *pkt);
int fully_qp_init(void);

//Connection States
void copy_eth_addr(uint8_t *src, uint8_t *dst);
void init_connection_state(struct rte_mbuf *pkt);
void init_cs_wrapper(struct rte_mbuf *pkt);
uint32_t produce_and_update_msn(struct roce_v2_header *roce_hdr, struct Connection_State *cs);
uint32_t find_and_update_stc(struct roce_v2_header *roce_hdr);
void find_and_set_stc(struct roce_v2_header *roce_hdr, struct rte_udp_hdr *udp_hdr);
void find_and_set_stc_wrapper(struct roce_v2_header *roce_hdr, struct rte_udp_hdr *udp_hdr);
void update_cs_seq(uint32_t stc_dest_qp, uint32_t seq);
void update_cs_seq_wrapper(struct roce_v2_header *roce_hdr);
void cts_track_connection_state(struct rte_mbuf *pkt);
uint64_t get_latest_key(uint32_t id);
void set_latest_key(uint32_t id, uint64_t key);
void init_connection_states(void);

//Read Caching
uint32_t mod_hash(uint64_t vaddr);
void update_write_vaddr_cache_mod(uint64_t key, uint64_t vaddr);
int does_read_have_cached_write_mod(uint64_t vaddr);
uint64_t get_latest_vaddr_mod(uint32_t key);
void steer_read(struct rte_mbuf *pkt, uint32_t key);

//Slots
uint32_t slot_is_open(struct Request_Map *rm);
void close_slot(struct Request_Map *rm);
void open_slot(struct Request_Map *rm);
uint32_t mod_slot(uint32_t seq);
struct Request_Map *get_empty_slot_mod(struct Connection_State *cs);

//mapping and tracking
uint32_t qp_is_mapped(uint32_t qp);
void track_qp(struct rte_mbuf *pkt);
int should_track(struct rte_mbuf *pkt);
struct map_packet_response map_qp(struct rte_mbuf *pkt);
struct Connection_State *find_connection(struct rte_mbuf *pkt);
struct Request_Map *find_slot_mod(struct Connection_State *source_connection, struct rte_mbuf *pkt);
void map_qp_forward(struct rte_mbuf *pkt, uint64_t key);
struct map_packet_response map_qp_backwards(struct rte_mbuf *pkt);

//Packet processing
struct rte_ether_hdr *eth_hdr_process(struct rte_mbuf *buf);
struct rte_ipv4_hdr *ipv4_hdr_process(struct rte_ether_hdr *eth_hdr);
struct rte_udp_hdr *udp_hdr_process(struct rte_ipv4_hdr *ipv4_hdr);
struct roce_v2_header *roce_hdr_process(struct rte_udp_hdr *udp_hdr);
struct clover_hdr *mitsume_msg_process(struct roce_v2_header *roce_hdr);
int accept_packet(struct rte_mbuf *pkt);
//struct mitsume_msg * mitsume_msg_process(struct roce_v2_header * roce_hdr);

void print_packet_lite(struct rte_mbuf *buf);
void debug_icrc(struct rte_mempool *mbuf_pool);

struct rte_ether_hdr *get_eth_hdr(struct rte_mbuf *pkt);
struct rte_ipv4_hdr *get_ipv4_hdr(struct rte_mbuf *pkt);
struct rte_udp_hdr *get_udp_hdr(struct rte_mbuf *pkt);
struct roce_v2_header *get_roce_hdr(struct rte_mbuf *pkt);



#endif
