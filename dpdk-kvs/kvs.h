#ifndef KVS_H
#define KVS_H

#ifndef KEY_SIZE
#define KEY_SIZE 4
#endif

#ifndef VALUE_SIZE 
#define VALUE_SIZE 4
#endif


#define KEY_SPACE_SIZE 512

#define MAC_DEST_BYTES 6
#define MAC_SRC_BYTES  6
#define MAC_TYPE_BYTES 2
#define MAC_CRC_BYTES  4
#define MAC_HEADER_SIZE MAC_DEST_BYTES + MAC_SRC_BYTES + MAC_TYPE_BYTES

#define IP_VERSION_IHL_BYTES 1
#define IP_TOS_BYTES 1
#define IP_LENGTH_BYTES 2
#define IP_FRAGMENTATION_BYTES 2
#define IP_FRAGMENTATION_OFFSET_BYTES 2
#define IP_TTL_BYTES 1
#define IP_PROTOCOL_BYES 1
#define IP_CHECKSUM_BYTES 2
#define IP_SRC_BYTES 4
#define IP_DEST_BYTES 4
#define IP_HEADER_SIZE IP_VERSION_IHL_BYTES + IP_TOS_BYTES + IP_LENGTH_BYTES + IP_FRAGMENTATION_BYTES + IP_FRAGMENTATION_OFFSET_BYTES + IP_TTL_BYTES + IP_PROTOCOL_BYES + IP_CHECKSUM_BYTES + IP_SRC_BYTES + IP_DEST_BYTES 

#define UDP_SRC_PORT_BYTES 2
#define UDP_DEST_PORT_BYTES 2
#define UDP_LEN_BYTES 2
#define UDP_CHECKSUM_BYTES 2
#define UDP_HEADER_SIZE UDP_SRC_PORT_BYTES + UDP_DEST_PORT_BYTES + UDP_LEN_BYTES + UDP_CHECKSUM_BYTES

#define TOTAL_HEADER_SIZE MAC_HEADER_SIZE + IP_HEADER_SIZE + UDP_HEADER_SIZE

#define PACKET_GEN 1

#define CLOCK_RATE 3000000000

#include <stdint.h>

#include <rte_mbuf.h>

#include "cuckoohash.h"

enum pkt_type{PUT, GET, PUT_ACK, GET_ACK, FAILED};

typedef struct kv_put_header {
    int type;
    uint64_t timestamp;
    char key[KEY_SIZE];
    char value[VALUE_SIZE];
} kv_put_header;

typedef struct kv_get_header {
    int type;
    uint64_t timestamp;
    char key[KEY_SIZE];
} kv_get_header;


typedef struct stats_tracker {
    int packets_processed;
    int64_t data_processed;
    int second_per_sample;
    int64_t next_sample;
} stats_tracker;

//Generate keys and values for future requets, assumes no value updates for keys
//
/*
void pktgen_kv_request(struct rte_mbuf **pkts, size_t cnt);
void populate_request_banks(void);
void get_put_packet(int index, kv_put_header * kvph);
void print_put_packet(kv_put_header * kvph);
void get_get_packet(int index, kv_get_header * kvgh);
void print_get_packet(kv_get_header *kvgh);
uint32_t get_request(uint8_t *buf, uint32_t buf_size);
void init(void);

void write_get_latency(kv_get_header * kvgh);
uint64_t get_get_latency(kv_get_header * kvgh);
void write_put_latency(kv_put_header * kvph);
uint64_t get_put_latency(kv_put_header * kvph);
void write_get_wrapper(kv_get_header *kvgh);
uint64_t get_get_latency_wrapper(kv_get_header * kvgh);
void write_put_wrapper(kv_put_header *kvph);
uint64_t get_put_latency_wrapper(kv_put_header * kvph);
*/



void print_packet_feild(int len,int * start,const char* feild_name,struct rte_mbuf *pkt);
void print_packet_info(struct rte_mbuf *pkt);
void turn_packet_around(struct rte_mbuf *pkt);
void clear_tracker(struct stats_tracker * tracker);
void update_stats (struct rte_mbuf *pkt, struct stats_tracker *tracker);
void kv_request(struct rte_mbuf *pkt, cuckoo_hashtable_t * table);

kv_get_header * put_request(kv_put_header * kvph, cuckoo_hashtable_t *table);
kv_put_header * get_request(kv_get_header * kvgh, cuckoo_hashtable_t *table);

#endif //KVS_H
