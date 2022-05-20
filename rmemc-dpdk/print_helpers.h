#ifndef PRINT_HELPERS
#define PRINT_HELPERS

#include "clover_structs.h"
#include "rmemc-dpdk.h"

int log_printf(int level, const char *format, ...);
const char *ib_print_op(uint8_t opcode);
void print_request_map(struct Request_Map *rm);
void print_connection_state(struct Connection_State *cs);

void id_colorize(uint32_t id);
void red(void);
void yellow(void);
void blue(void);
void green(void);
void black(void);
void magenta(void);
void cyan(void);
void white(void);
void reset(void);

void print_bytes(const uint8_t *buf, uint32_t len);
void print_binary_bytes(const uint8_t *buf, uint32_t len);
void print_address(uint64_t *address);
void print_binary_address(uint64_t *address);
void print_ack_extended_header(struct AETH *aeth);
void print_rdma_extended_header(struct RTEH *rteh);
void print_read_request(struct read_request *rr);
void print_read_response(struct read_response *rr);
void print_write_request(struct write_request *wr);
void print_atomic_eth(struct AtomicETH *ae);
void print_cs_request(struct cs_request *csr);
void print_cs_response(struct cs_response *csr);

void print_first_mapping(void);

void print_raw(struct rte_mbuf *pkt);
void print_ether_hdr(struct rte_ether_hdr *eth);
struct rte_ether_hdr *eth_hdr_process(struct rte_mbuf *buf);
void print_ip_hdr(struct rte_ipv4_hdr *ipv4_hdr);
void print_udp_hdr(struct rte_udp_hdr *udp_hdr);
void print_roce_v2_hdr(struct roce_v2_header *rh);
void print_clover_hdr(struct clover_hdr *clover_header);

void print_packet(struct rte_mbuf *buf);


FILE * open_logfile(char * filename);
void close_logfile(FILE * fp);
void print_raw_file(struct rte_mbuf * pkt, FILE * fp);

#endif