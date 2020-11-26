

#include "cuckoohash.h"
#include "kvs.h"

#define FAILURE_STATS 1
#define ERROR_PRINT_RATE 1000000


int missarray[1024];
int putarray[1024];

void print_array(int * arr) {
    for(int i=0;i<1024;i++){
        if (arr[i] != 0) {
            printf("[%d]: %d\n",i,arr[i]);
        }
    }
}

void 
print_packet_feild(
        int len,
        int * start,
        const char* feild_name,
        struct rte_mbuf *pkt) {

        printf("%s",feild_name);
        for (int i=*start; i<(*start)+len;i++){
            printf("%2X",(uint8_t)((char *)(pkt->buf_addr))[i]);
        }
        printf("\n");
        *start += len;
}

void
print_packet_info(
        struct rte_mbuf *pkt) {

        printf("Got a packet of size %d\n"
                "and RTE_SIZE %d\n"
                "on port %d\n"
                "on address %p\n"
                "with # segments %d\n"
                "and offset %d\n"
                "(%s:%d)\n"
                ,pkt->pkt_len,
                rte_pktmbuf_data_len(pkt),
                pkt->port,
                pkt->buf_addr,
                pkt->nb_segs,
                *rte_pktmbuf_mtod(pkt,int*),
                

                __func__,
                __LINE__);

        /*
        printf("Raw Packet Contents \n\n[");
        for (i=rte_pktmbuf_headroom(pkt);(uint16_t)i<(pkt->data_len + rte_pktmbuf_headroom(pkt));i++){
            printf("%X",(uint8_t)((char *)(pkt->buf_addr))[i]);
            //printf("%c-",((char *)pkt->userdata)[itter]);
        }
        printf("]\n\n");
        */

        printf("Structured Packet Contents \n\n");
        int packet_index = rte_pktmbuf_headroom(pkt);

        print_packet_feild(MAC_DEST_BYTES,&packet_index,"MAC DEST:\t",pkt);
        print_packet_feild(MAC_SRC_BYTES,&packet_index, "MAC SRC:\t",pkt);
        print_packet_feild(MAC_TYPE_BYTES,&packet_index, "MAC TYPE:\t",pkt);
        printf("\n");

        print_packet_feild(IP_VERSION_IHL_BYTES,&packet_index, "IP VERSION:\t",pkt);
        print_packet_feild(IP_TOS_BYTES,&packet_index, "IP TOS:\t",pkt);
        print_packet_feild(IP_LENGTH_BYTES,&packet_index, "IP LENGTH:\t",pkt);
        print_packet_feild(IP_FRAGMENTATION_BYTES,&packet_index, "IP FRAGMENTATION:\t",pkt);
        print_packet_feild(IP_FRAGMENTATION_OFFSET_BYTES,&packet_index, "IP FRAGMENTATION OFFSET:\t",pkt);
        print_packet_feild(IP_TTL_BYTES,&packet_index, "IP TTL:\t",pkt);
        print_packet_feild(IP_PROTOCOL_BYES,&packet_index, "IP PROTOCOL:\t",pkt);
        print_packet_feild(IP_CHECKSUM_BYTES,&packet_index, "IP CHECKSUM:\t",pkt);
        print_packet_feild(IP_SRC_BYTES,&packet_index, "IP SRC:\t",pkt);
        print_packet_feild(IP_DEST_BYTES,&packet_index, "IP DEST:\t",pkt);
        printf("\n");

        print_packet_feild(UDP_SRC_PORT_BYTES,&packet_index, "UDP SRC PORT:\t",pkt);
        print_packet_feild(UDP_DEST_PORT_BYTES,&packet_index, "UDP DEST PORT:\t",pkt);
        print_packet_feild(UDP_LEN_BYTES,&packet_index, "UDP LEN:\t",pkt);
        print_packet_feild(UDP_CHECKSUM_BYTES,&packet_index, "UDP CHECKSUM:\t",pkt);
        printf("\n");

        print_packet_feild(pkt->data_len - (packet_index - rte_pktmbuf_headroom(pkt)),&packet_index, "PAYLOAD:\t",pkt);
}

void turn_packet_around(struct rte_mbuf *pkt) {
    //Flip source and destination mac address
    char tmp;
    int mac_dest_offset=rte_pktmbuf_headroom(pkt);
    int mac_source_offset = mac_dest_offset + MAC_DEST_BYTES;

    int ip_src_offset = rte_pktmbuf_headroom(pkt) + MAC_HEADER_SIZE + 12;
    int ip_dest_offset = ip_src_offset + IP_SRC_BYTES;

    int udp_src_offset = rte_pktmbuf_headroom(pkt) + MAC_HEADER_SIZE + IP_HEADER_SIZE;
    int udp_dest_offset = udp_src_offset + UDP_SRC_PORT_BYTES;

    //Swap MAC
    for (int i=0;i<MAC_DEST_BYTES;i++) {
        tmp = ((char *)(pkt->buf_addr))[mac_dest_offset + i];
        ((char *)(pkt->buf_addr))[mac_dest_offset + i] = ((char * )pkt->buf_addr)[mac_source_offset + i];
        ((char *)(pkt->buf_addr))[mac_source_offset + i] = tmp;
    }
    //Swap IP
    for (int i=0;i<IP_SRC_BYTES;i++) {
        tmp = ((char *)(pkt->buf_addr))[ip_dest_offset + i];
        ((char *)(pkt->buf_addr))[ip_dest_offset + i] = ((char * )pkt->buf_addr)[ip_src_offset + i];
        ((char *)(pkt->buf_addr))[ip_src_offset + i] = tmp;
    }

    //Swap Port
    for (int i=0;i<UDP_SRC_PORT_BYTES;i++) {
        tmp = ((char *)(pkt->buf_addr))[udp_dest_offset + i];
        ((char *)(pkt->buf_addr))[udp_dest_offset + i] = ((char * )pkt->buf_addr)[udp_src_offset + i];
        ((char *)(pkt->buf_addr))[udp_src_offset + i] = tmp;
    }
}


void clear_tracker(struct stats_tracker * tracker) {
    tracker->packets_processed = 0;
    tracker->data_processed = 0;
    tracker->second_per_sample =0;
    tracker->next_sample=0;
}

void update_stats (
        struct rte_mbuf *pkt,
        struct stats_tracker *tracker) {

    int64_t time;
    tracker->packets_processed++;
    tracker->data_processed += pkt->data_len;
    time = rte_rdtsc();

    if (unlikely(time > tracker->next_sample)) {
        printf(
                "Packets: %d\n"
                "Throughput: %" PRId64 "\n",
                tracker->packets_processed/tracker->second_per_sample,
                tracker->data_processed/(1024 * 1024)
                );
         //TODO cacluate the next time based on samples per second
        tracker->next_sample = time + (CLOCK_RATE * tracker->second_per_sample); //1billion cycles (1/4 a second)
        tracker->packets_processed =0;
        tracker->data_processed=0;
        //print_packet_info(pkt);
    }
}

kv_get_header * put_request(kv_put_header * kvph, cuckoo_hashtable_t *table) {
    cuckoo_status st;
    int failure =0 ;
    st = cuckoo_insert(table, (const char*) kvph->key, (const char*) kvph->value);
    if (st != ok) {
        //printf("inserting key %" PRIu64" to smalltable fails \n", (uint64_t)kvph->key);

        //There is a chance that we could not insert because the table was
        //full, but also the probability that we could not overwrite an
        //existing key. In the real world the former case could occur, and we
        //might want to do something smart like allocate a new table and
        //shuffle over all of the requests. This is however science so we are
        //just going to assume that it is a duplicate and that the key space we
        //are working with is much smaller than that of the hash table. However
        //we will spit out an error if the value is actually not in the table.
        //Which would be the we need a bigger hash table case.
       
        //Test if the value is in the table.. 
        //TODO this op can be deleted if we want to make it faster
        ValType empty_val;
        st = cuckoo_find(table, (const char*) kvph->key, (char*) empty_val);
        if (st != ok) {
            printf("Key %" PRIu64 " cannot be inserted (table likely full)\n", (uint64_t)kvph->key);
            failure=1;
        } else {

            //Key is in the table we just have to delete the old value before
            //we get a new on because of how the cuckoohash is implemented
            st = cuckoo_delete(table,(const char *) kvph->key);
            if ( st != ok ) {
                printf("ERROR something is wrong, key found but not deleteable (doing nothing)\n");
                failure = 1;
            } else {
                //Here we can insert because we know that we deleted the correct key
                st = cuckoo_insert(table, (const char*) kvph->key, (const char*) kvph->value);
                if (st != ok) {
                    printf("Critical - Key deleted, but not inserted\n");
                    failure =1;
                } 
            }
        }       
    }

    kv_get_header * kvgh;
    kvgh = (kv_get_header *)kvph;
    if (failure) {
        printf("Failure to put key %" PRIu64 "\n",(uint64_t)kvph->key);
        kvgh->type = FAILED;
    } else {
        kvgh->type = PUT_ACK;
        //printf("put\n");
    }
#ifdef FAILURE_STATS
    static uint64_t successes, failures;
    if (kvgh->type == FAILED) {
        ++failures;
    } else {
        ++successes;

        putarray[*(uint32_t*)kvph->key]++;
    }
    if (((failures + successes) % (ERROR_PRINT_RATE/10)) == 0) {
        printf("PUT (S: %"PRIu64", F:%"PRIu64")\n",successes,failures);
        //print_array(putarray);
    }
#endif
    return kvgh;
}

kv_put_header * get_request(kv_get_header * kvgh, cuckoo_hashtable_t *table) {
    cuckoo_status st;
    int failure =0 ;


    //TODO BUG this is dangerous, only do this for basic testing will cause a
    //seg fault otherwise it assumes that there is enough space in a get header
    //to store the result, where there may very well not be
    kv_put_header * kvph = (kv_put_header *)kvgh;

    st = cuckoo_find(table, (const char*) kvgh->key, (char*) kvph->value);
    if (st != ok ) {
        //printf("Unable to find the key %d in the has table\n", *(uint32_t*)kvgh->key);
        kvph->type = FAILED;
    } else {
        kvph->type = GET_ACK;
    }

#ifdef FAILURE_STATS
    static uint64_t successes, failures;
    if (kvph->type == FAILED) {
        missarray[*(uint32_t*)kvph->key]++;
        ++failures;
    } else {
        ++successes;
    }
    if (((failures + successes) % ERROR_PRINT_RATE) == 0) {
        printf("GET (S: %"PRIu64", F:%"PRIu64" rate: ~%f)\n",successes,failures,((float)successes / (float)(successes + failures)));
        //print_array(missarray);
    }
#endif
    return kvph;
}


void kv_request(struct rte_mbuf *pkt, cuckoo_hashtable_t * table) {
    int kv_header_start = rte_pktmbuf_headroom(pkt) + TOTAL_HEADER_SIZE;
    uint8_t * buf = pkt->buf_addr + kv_header_start;
    //printf("KV REQUEST!!!");
    kv_get_header* test_header = (kv_get_header *)buf;

    if (test_header->type == GET) {
        get_request(test_header, table);
    } else if (test_header->type == PUT) {
        put_request((kv_put_header *)test_header, table);
    }

    return;
}
