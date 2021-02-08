#include "packets.h"
#include <arpa/inet.h>

void print_eth_header(ether_header *eth) {
    printf("Destination Host: %02X:%02X:%02X:%02X:%02X:%02X\n",
    eth->ether_dhost[0],
    eth->ether_dhost[1],
    eth->ether_dhost[2],
    eth->ether_dhost[3],
    eth->ether_dhost[4],
    eth->ether_dhost[5]
    );
    printf("Source Host:      %02X:%02X:%02X:%02X:%02X:%02X\n",
    eth->ether_shost[0],
    eth->ether_shost[1],
    eth->ether_shost[2],
    eth->ether_shost[3],
    eth->ether_shost[4],
    eth->ether_shost[5]
    );
    printf("Ethernet Type:    0x%02X%02X\n",
    eth->eth_type.ether_type_bytes[0],
    eth->eth_type.ether_type_bytes[1]
    );
}

void print_ip_addr(const char *name, uint32_t ip){
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;   
    printf("%s \t%d.%d.%d.%d\n", name, bytes[0], bytes[1], bytes[2], bytes[3]);  
}

void print_ip_header(ip_header *ip) {
    printf("IHL:           %d\n",ip->ihl);
    printf("version:       %d\n",ip->version);
    printf("tos            %d\n",ip->tos);
    printf("len            %d\n",ntohs(ip->tot_len));
    printf("id             %d\n",ip->id);
    printf("frag_off       %d\n",ip->frag_off);
    printf("ttl            %d\n",ip->ttl);
    printf("protocol       %d\n",ip->protocol);
    printf("check          %d\n",ip->check);
    print_ip_addr("source",ip->saddr);
    print_ip_addr("dest",ip->daddr);
}

void print_udp_header(udp_header *udp) {
    printf("source  %d\n",ntohs(udp->sport));
    printf("dest    %d\n",ntohs(udp->dport));
    printf("len     %d\n",udp->len);
    printf("check   %d\n",udp->check);

}

/*
void print_whole_packet(agg_header * header) {
    printf("---------ETH-----------\n");
    print_eth_header(&(header->eth));
    printf("---------IP------------\n");
    print_ip_header(&(header->ip));
    printf("---------UDP-----------\n");
    print_udp_header(&(header->udp));
    printf("---------RoCE-----------\n");
    print_roce_v2_header(&(header->roce));
    printf("only printing up to UDP header\n");
}*/