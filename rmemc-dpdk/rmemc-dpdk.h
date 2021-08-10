#ifndef RMEMC_DPDK_H
#define RMEMC_DPDK_H

#include "clover_structs.h"

//macro magic
#define DO_EXPAND(VAL)  VAL ## 1
#define EXPAND(VAL)     DO_EXPAND(VAL)

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

#define MITSUME_BENCHMARK_THREAD_DEFAULT 32
#if !defined(MITSUME_BENCHMARK_THREAD_NUM) || (EXPAND(MITSUME_BENCHMARK_THREAD_NUM) == 1)
    //Only here if MYVARIABLE is not defined
    //OR MYVARIABLE is the empty string
    #undef MITSUME_BENCHMARK_THREAD_NUM
    #define MITSUME_BENCHMARK_THREAD_NUM MITSUME_BENCHMARK_THREAD_DEFAULT
#endif

//checksum

struct Request_Map {
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
} Request_Map;

struct Connection_State {
  uint32_t id;
  uint16_t udp_src_port_client;
  uint16_t udp_src_port_server;
  uint32_t ip_addr_client;
  uint32_t ip_addr_server;
  uint32_t cts_rkey;
  uint32_t stc_rkey;
  uint32_t ctsqp;
  uint32_t stcqp;
  uint8_t cts_eth_addr[6];
  uint8_t stc_eth_addr[6];
  uint32_t seq_current; // Packet sequence number
  uint32_t mseq_current; // message sequence number (for acks/cns acks/reads in ATEH header)
  uint32_t sender_init;
  uint32_t receiver_init;
  uint32_t last_seq;
  //init test variables
  int32_t mseq_offset;

  //experimental
  struct rte_mbuf * read_holder;

  struct Request_Map Outstanding_Requests[CS_SLOTS];
} Connection_State;

struct map_packet_response {
  uint32_t size;
  struct rte_mbuf *pkts[BURST_SIZE];
} map_packet_response;


uint32_t check_sums(const char* method, void* known, void* test, int try);
uint32_t check_sums_wrap(const char* method, void* know, void* test);
uint32_t csum_pkt_fast(struct rte_mbuf* pkt);
uint32_t csum_pkt(struct rte_mbuf* pkt);

int init_hash(void);
int set_id(uint32_t qp, uint32_t id);
uint32_t get_id(uint32_t qp);
uint32_t readable_seq(uint32_t seq);

void count_values(uint64_t *index, uint32_t *count, uint32_t size, uint64_t value);
void count_read_req_addr(struct read_request * rr);
void classify_packet_size(struct rte_ipv4_hdr *ip, struct roce_v2_header *roce);
void true_classify(struct rte_mbuf * pkt);
void rdma_print_pattern(roce_v2_header * rdma);
void init_ib_words(void);
void init_connection_state(struct rte_mbuf *pkt);


//qp tracking
uint32_t key_to_qp(uint64_t key);
void update_cs_seq(uint32_t stc_dest_qp, uint32_t seq);
void cts_track_connection_state(struct rte_mbuf * pkt);
void find_and_set_stc_qp(uint32_t stc_dest_qp, uint32_t seq);
void find_and_set_stc_qp_wrapper(struct roce_v2_header *roce_hdr);
void update_cs_seq_wrapper(struct roce_v2_header *roce_hdr);
int coretest(void);
void fork_lcores(void);
void init_cs_wrapper(struct rte_mbuf* pkt);



struct rte_ether_hdr *eth_hdr_process(struct rte_mbuf* buf);
struct rte_ipv4_hdr* ipv4_hdr_process(struct rte_ether_hdr *eth_hdr);
struct rte_udp_hdr * udp_hdr_process(struct rte_ipv4_hdr *ipv4_hdr);
struct roce_v2_header * roce_hdr_process(struct rte_udp_hdr * udp_hdr);
struct clover_hdr * mitsume_msg_process(struct roce_v2_header * roce_hdr);
//struct mitsume_msg * mitsume_msg_process(struct roce_v2_header * roce_hdr);

void print_packet_lite(struct rte_mbuf * buf);
void debug_icrc(struct rte_mempool *mbuf_pool);



#endif
