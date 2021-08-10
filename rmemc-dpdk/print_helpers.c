#include "clover_structs.h"
#include "rmemc-dpdk.h"
#include "print_helpers.h"
#include "rte_ether.h"
#include "rte_ip.h"
#include <rte_udp.h>

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

const char * ib_print_op(uint8_t opcode) {
	switch (opcode){
		case RC_ACK:
			return "RC_ACK";
		case RC_SEND:
			return "RC_SEND";
		case RC_WRITE_ONLY:
			return "RC_WRITE_ONLY";
		case RC_READ_REQUEST:
			return "RC_READ_REQUEST";
		case RC_READ_RESPONSE:
			return "RC_READ_RESPONSE";
		case RC_ATOMIC_ACK:
			return "RC_ATOMIC_ACK";
		case RC_CNS: 
			return "RC_COMPARE_AND_SWAP";
		default:
			return "__UNKNOWN__OP__";
	}
}

void print_request_map(struct Request_Map *rm) {
	if (rm->open == 1) 
		printf("open\n");
	else {
		printf("closed");
	}
	printf("ID: %d\n",rm->id);
	printf("Original Seq %d, mapped seq %d\n",readable_seq(rm->original_sequence), readable_seq(rm->mapped_sequence));
	printf("stcqp qp %d, mapped stcqp %d\n", rm->server_to_client_qp, rm->mapped_destination_server_to_client_qp);
	printf("stcqp port %d\n", rm->server_to_client_udp_port);
}

void print_connection_state(struct Connection_State* cs) {
	printf("ID: %d port-cts %d port-stc %d\n",cs->id, cs->udp_src_port_client, cs->udp_src_port_server);
	printf("cts qp: %d stc qp: %d \n",cs->ctsqp, cs->stcqp);
	printf("seqt %d seq %d mseqt %d mseq %d\n",readable_seq(cs->seq_current),cs->seq_current,readable_seq(cs->mseq_current),cs->mseq_current);
}

void red (void) {printf("\033[1;31m");}
void yellow (void) {printf("\033[1;33m");}
void blue (void) {printf("\033[1;34m");}
void green (void) {printf("\033[1;32m");}
void black(void) {printf("\033[1;30m");}
void magenta(void) {printf("\033[1;35m");}
void cyan(void) {printf("\033[1;36m");}
void white(void) {printf("\033[1;37m");}
void reset (void) {printf("\033[0m");}

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
		case 4:
			black();
			break;
		case 5:
			magenta();
			break;
		case 6:
			cyan();
			break;
		case 7:
			white();
			break;
		default:
			reset();
			break;
	}
}

void print_bytes(const uint8_t * buf, uint32_t len) {
	for (uint32_t i=0;i<len;i++)  {
		printf("%02X ", buf[i]);
	}
}

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

void print_rdma_extended_header(struct RTEH * rteh) {
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
	print_ack_extended_header(&csr->ack_extended);
	//TODO this function should be written
	//print_atomic_ack_eth(&csr->atomc_ack_extended);
	printf("(STOP) compare and swap response\n");
	return;
}

void print_first_mapping(void){
	for (int i=0;i<15;i++) {
		printf("FIRST MAPPING OF QP GET READY BOIIS\n\n");
	}
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

void print_roce_v2_hdr(struct roce_v2_header * rh) {
    printf("op code             %02X %s\n",rh->opcode, ib_print_op(rh->opcode));
    printf("solicited event     %01X\n",rh->solicited_event);
    printf("migration request   %01X\n",rh->migration_request);
    printf("pad count           %01X\n",rh->pad_count);
    printf("transport version   %01X\n",rh->transport_header_version);
    printf("partition key       %02X\n",rh->partition_key);
    printf("fecn                %01X\n",rh->fecn);
    printf("becn                %01X\n",rh->bcen);
    printf("reserved            %01X\n",rh->reserverd);
    printf("dest qp        (hex)%02X\t  (dec)%d\n",rh->dest_qp, rh->dest_qp);
    printf("ack                 %01X\n",rh->ack);
    printf("reserved            %01X\n",rh->reserved);
    //printf("packet sequence #   %02X HEX %d DEC\n",rh->packet_sequence_number, rh->packet_sequence_number);
    printf("packet sequence #   %02X HEX %d DEC\n",readable_seq(rh->packet_sequence_number), readable_seq(rh->packet_sequence_number));

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


void print_packet(struct rte_mbuf * pkt) {
	struct rte_ether_hdr * eth_hdr = get_eth_hdr(pkt);
	struct rte_ipv4_hdr* ipv4_hdr = get_ipv4_hdr(pkt);
	struct rte_udp_hdr * udp_hdr = get_udp_hdr(pkt);
	struct roce_v2_header * roce_hdr = get_roce_hdr(pkt);
	print_raw(pkt);
	print_ether_hdr(eth_hdr);
	print_ip_hdr(ipv4_hdr);
	print_udp_hdr(udp_hdr);
	print_roce_v2_hdr(roce_hdr);
}