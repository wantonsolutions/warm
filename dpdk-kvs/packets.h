#ifndef PACKETS_H
#define PACKETS_H

#define ETH_ALEN 6
#define ROCE_PORT 4791

#include <stdint.h>
#include <rte_mbuf.h>

typedef struct __attribute__ ((__packed__)) ether_header
{
  uint8_t  ether_dhost[ETH_ALEN];        /* destination eth addr        */
  uint8_t  ether_shost[ETH_ALEN];        /* source ether addr        */
  union eth_type{
    uint16_t ether_type_int;                        /* packet type ID field        */
    uint8_t ether_type_bytes[2];
  } eth_type;

} ether_header;


typedef struct __attribute__ ((__packed__)) ip_header
  {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned int ihl:4;
    unsigned int version:4;
#elif __BYTE_ORDER == __BIG_ENDIAN
    unsigned int version:4;
    unsigned int ihl:4;
#else
# error        "Please fix <bits/endian.h>"
#endif
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
    /*The options start here. */
  } ip_header;

typedef struct __attribute__ ((__packed__)) udp_header {
     uint16_t sport;
     uint16_t dport;
     uint16_t len;
     uint16_t check;
    } udp_header;

typedef struct __attribute__ ((__packed__)) roce_v2_header {
    uint8_t opcode;
    unsigned int solicited_event:1;
    unsigned int migration_request:1;
    unsigned int pad_count:2;
    unsigned int transport_header_version:4;
    uint16_t partition_key;
    //unsigned int fecn:1;
    //unsigned int bcen:1;
    //unsigned int reserverd:6;
    unsigned int dest_qp:24;
    unsigned int ack:1;
    unsigned int reserved:7;
    unsigned int packet_sequence_number:24;
    unsigned int padding:16;
    unsigned int ICRC:4;
} roce_v2_header;



typedef struct clover_header {
    int not_implemented;
} clover_header;

typedef struct __attribute__ ((__packed__)) agg_header {
    ether_header eth;
    ip_header ip;
    udp_header udp;
    roce_v2_header roce;
} agg_header;


void print_whole_packet(agg_header * header);
void print_eth_header(ether_header *eth);
void print_ip_addr(char *name, uint32_t ip);
void print_ip_header(ip_header *ip);
void print_udp_header(udp_header *udp);

#endif