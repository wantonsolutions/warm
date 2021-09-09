#ifndef ROCE_V2
#define ROCE_V2

#include "clover_structs.h"

uint32_t get_psn(struct rte_mbuf *pkt);
uint32_t readable_seq(uint32_t seq);
uint32_t check_sums(const char *method, void *known, void *test, int try);
uint32_t check_sums_wrap(const char *method, void *know, void *test);
uint32_t csum_pkt_fast(struct rte_mbuf *pkt);
void recalculate_rdma_checksum(struct rte_mbuf *pkt);

uint32_t get_rkey_rdma_packet(struct roce_v2_header *roce_hdr);
void set_rkey_rdma_packet(struct roce_v2_header *roce_hdr, uint32_t rkey);
void set_msn(struct roce_v2_header *roce_hdr, uint32_t new_msn);
int get_msn(struct roce_v2_header *roce_hdr);

#endif