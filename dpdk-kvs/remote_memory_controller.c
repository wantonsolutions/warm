#include "packets.h"
#include "rte_ip.h"
#include <rte_ether.h>
#include <rte_hash_crc.h>
#include "remote_memory_controller.h"
#include <zlib.h>


const unsigned char YAK0_MAC[ETH_ALEN] = {0xec, 0x0d, 0x9a, 0x68, 0x21, 0xd0};
const unsigned char YAK1_MAC[ETH_ALEN] = {0xec, 0x0d, 0x9a, 0x68, 0x21, 0xcc};
const unsigned char YAK2_MAC[ETH_ALEN] = {0xec, 0x0d, 0x9a, 0x68, 0x21, 0xbc};

uint32_t crc32_table[256];

//generate table
void generate_crc32_table()
{
	int i, j;
	uint32_t crc;
	for (i = 0; i < 256; i++)
	{
		crc = i;
		for (j = 0; j < 8; j++)
		{
			if (crc & 1)
				crc = (crc >> 1) ^ 0xEDB88320;
			else
				crc >>= 1;
		}
		crc32_table[i] = crc;
	}
}

uint32_t calculate_crc(uint8_t *buffer, int len)
{
	int i;
	uint32_t crc;
	crc = 0xffffffff;
	for (i = 0; i < len; i++)
	{
		crc = (crc >> 8) ^ crc32_table[(crc & 0xFF) ^ buffer[i]];
	}
	crc ^= 0xffffffff;
	return crc;
}

uint32_t calculate_crc_wrap(uint8_t *buf, int len) {
    static int generated_table;

    if (generated_table == 0)  {
        generate_crc32_table();
        generated_table = 1;
    }

    return calculate_crc(buf,len);
}


static int total = 0;
void rmem_switch(struct rte_mbuf *pkt) {
    total++;
    if (total % 1000 == 0) {
        printf("pkts: %d\n",total);
    }
    return;

    printf("made it to the remote memeory switch\n");

    
    int packet_index = rte_pktmbuf_headroom(pkt);
    agg_header *full_header;
    full_header = (agg_header*)(((char*)pkt->buf_addr)+packet_index);


    if (ntohs(full_header->eth.eth_type.ether_type_int) == 0x0027 || full_header->eth.eth_type.ether_type_int == 0x0027) {
        printf("Likely STP protocol, continuing because this is not what I'm looking for\n");
        return;
    }

    if (ntohs(full_header->udp.sport) != ROCE_PORT && ntohs(full_header->udp.dport) != ROCE_PORT) {
        printf("Packet is not ROCE, and we are only printing RoCEv2 packets\n");
        //return;
    }

    //try to capture the arp packets
    print_whole_packet(full_header);

    //IP forwarding
    uint32_t src_ip=full_header->ip.saddr;
    uint32_t dst_ip;
    unsigned char* dst_mac;
    unsigned char* src_mac;

    switch (src_ip) {
        case YAK0_IP:
            dst_ip=YAK1_IP;
            src_ip=YAK0_IP;
            dst_mac=&YAK1_MAC;
            src_mac=&YAK0_MAC;
            break;
        case YAK1_IP:
            dst_ip=YAK0_IP;
            src_ip=YAK1_IP;
            dst_mac=&YAK0_MAC;
            src_mac=&YAK1_MAC;
            break;
        default:
            printf("unknown source ip no table entry%d\n",src_ip);
            return;
    }

    //#define SWITCH_SOURCE
    #ifdef SWITCH_SOURCE
        src_ip=YAK2_IP;
        src_mac=&YAK2_MAC;

        dst_ip=YAK2_IP;
        dst_mac=&YAK2_MAC;
    #endif


    full_header->ip.daddr = dst_ip;
    full_header->ip.saddr = src_ip;

    //Set MAC
    strncpy((char*)full_header->eth.ether_shost,src_mac,ETH_ALEN);
    strncpy((char*)full_header->eth.ether_dhost,dst_mac,ETH_ALEN);
    //set the new destination

    //ethcrc
    //print original
    uint8_t *crc_local;
    int crc_index=rte_pktmbuf_headroom(pkt) + pkt->data_len - 4;
    crc_local = (uint8_t *)(pkt->buf_addr + crc_index);
    printf("buff addr %p\n",pkt->buf_addr);
    printf("index = %d crc_local %p\n",crc_index,crc_local);
    printf("crc: [%02X,%02X,%02X,%02X]\n",crc_local[0],crc_local[1],crc_local[2],crc_local[3]);

    //zero out the CRC
    //crc_local[0] = 0;
    //crc_local[1] = 0;
    //crc_local[2] = 0;
    //crc_local[3] = 0;

    //calculate original crc (sanity check);

    uint8_t * pkt_buf =  (uint8_t *)pkt->buf_addr + rte_pktmbuf_headroom(pkt);
    uint32_t len = pkt->data_len - 4;

    uint32_t rte_hash = rte_hash_crc ( pkt_buf, len,1);
    crc_local = (uint8_t*)&rte_hash;
    printf("calculated crc: [%02X,%02X,%02X,%02X]\n",crc_local[0],crc_local[1],crc_local[2],crc_local[3]);
    rte_hash = calculate_crc_wrap (pkt_buf, len);
    crc_local = (uint8_t*)&rte_hash;
    printf("calculated crc 2: [%02X,%02X,%02X,%02X]\n",crc_local[0],crc_local[1],crc_local[2],crc_local[3]);







    //Do the checksums of the packet

	struct rte_ipv4_hdr *ipv4_hdr;
    ipv4_hdr = rte_pktmbuf_mtod_offset(pkt,
                        struct rte_ipv4_hdr *,
                        sizeof(struct rte_ether_hdr));

    printf("checksum = %d new = %d \n",ipv4_hdr->hdr_checksum,rte_ipv4_cksum(ipv4_hdr));
    ipv4_hdr->hdr_checksum = 0;
    printf("checksum = %d new = %d \n",ipv4_hdr->hdr_checksum,rte_ipv4_cksum(ipv4_hdr));
    ipv4_hdr->hdr_checksum = rte_ipv4_cksum(ipv4_hdr);




    print_whole_packet(full_header);
    printf("-------\n-------\n-------\n-------\n");
}
