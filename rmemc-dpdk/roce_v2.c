#include "roce_v2.h"
#include "clover_structs.h"

#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include "print_helpers.h"

uint32_t get_psn(struct rte_mbuf *pkt) {
	struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	struct rte_ipv4_hdr* ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	struct rte_udp_hdr * udp_hdr = (struct rte_udp_hdr *)((uint8_t *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));
	struct roce_v2_header * roce_hdr = (struct roce_v2_header *)((uint8_t*)udp_hdr + sizeof(struct rte_udp_hdr));
	struct clover_hdr * clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));
	return roce_hdr->packet_sequence_number;
}

uint32_t readable_seq(uint32_t seq) {
	return ntohl(seq) / 256;
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
	ulong crc = crc32(0xFFFFFFFF, test_buf, 4);
	
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

void recalculate_rdma_checksum(struct rte_mbuf *pkt) {
	struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	struct rte_ipv4_hdr* ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	uint32_t crc_check =csum_pkt_fast(pkt); //This need to be added before we can validate packets
	void * current_checksum = (void *)((uint8_t *)(ipv4_hdr) + ntohs(ipv4_hdr->total_length) - 4);
	memcpy(current_checksum,&crc_check,4);
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
    		printf("op code %02X %s\n",roce_hdr->opcode, ib_print_op(roce_hdr->opcode));
			printf("rh-opcode unknown while setting rkey. Exiting\n");
			exit(0);
	}
	return;
}

void set_msn(struct roce_v2_header *roce_hdr, uint32_t new_msn) {
	struct clover_hdr * clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));
	struct read_response * read_resp = (struct read_response*) clover_header;
	struct rdma_ack *ack = (struct rdma_ack*) clover_header;
	struct cs_response * cs_resp = (struct cs_response *)clover_header;
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

int get_msn(struct roce_v2_header *roce_hdr) {
	struct clover_hdr * clover_header = (struct clover_hdr *)((uint8_t *)roce_hdr + sizeof(roce_v2_header));
	struct read_response * read_resp = (struct read_response*) clover_header;
	struct rdma_ack *ack = (struct rdma_ack*) clover_header;
	struct cs_response * cs_resp = (struct cs_response *)clover_header;
	uint32_t msn;
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
			msn=-1;
			break;
	}
	return msn;
}

uint8_t test_ack_pkt[] = {
0xEC,0x0D,0x9A,0x68,0x21,0xCC,0xEC,0x0D,0x9A,0x68,0x21,0xD0,0x08,0x00,0x45,0x02,
0x00,0x30,0x2A,0x2B,0x40,0x00,0x40,0x11,0x8D,0x26,0xC0,0xA8,0x01,0x0C,0xC0,0xA8,
0x01,0x0D,0xCF,0x15,0x12,0xB7,0x00,0x1C,0x00,0x00,0x11,0x40,0xFF,0xFF,0x00,0x00,
0x6C,0xA9,0x00,0x00,0x0C,0x71,0x0D,0x00,0x00,0x01,0xDC,0x97,0x84,0x42,};

void debug_icrc(struct rte_mempool *mbuf_pool) {
	printf("debugging CRC using a cached packet\n");
	struct rte_mbuf* buf = rte_pktmbuf_alloc(mbuf_pool);
	struct rte_ether_hdr * eth_hdr = rte_pktmbuf_mtod(buf, struct rte_ether_hdr *);
	uint16_t pkt_len = 62;
	memcpy(eth_hdr,test_ack_pkt,pkt_len);
	buf->pkt_len=pkt_len;
	buf->data_len=pkt_len;
	printf("pkt copied\n");
	print_packet(buf);
	csum_pkt_fast(buf);
	exit(0);
}