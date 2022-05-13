/*
Copyright 2013-present Barefoot Networks, Inc. 

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

// Template headers.p4 file for basic_switching
// Edit this file as needed for your P4 program

// Here's an ethernet header to get started.
#ifndef BS_HEADER
#define BS_HEADER

#define ID_SIZE 8
#define KEY_SIZE 8
#define VADDR_SIZE 64
#define MAX_KEYS 4096


typedef bit<9>  egressSpec_t;

typedef struct vaddr_t {
    bit<32> upper;
    bit<32> lower;
} vaddr_t ;

struct metadata {
    bit<1> existing_id;
    bit<ID_SIZE> id;
    bit<KEY_SIZE> key;
    vaddr_t vaddr;
    vaddr_t next_vaddr;
    vaddr_t outstanding_write_vaddr;
    /* empty */
}

header ethernet_t {
    bit<48>dstAddr;
    bit<48>srcAddr;
    bit<16>etherType;
}

header ipv4_t {
    bit<4>version;
    bit<4>IHL;
    bit<8>TOS;
    bit<16>totalLength;
    bit<16>id;
    bit<4>flags;
    bit<12>fragment_offset;
    bit<8>ttl;
    bit<8>protocol;
    bit<16>checksum;
    bit<32>src_addr;
    bit<32>dest_addr;
}

header udp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<16> length_;
    bit<16> checksum;
}

header rocev2_t {
    bit<8> opcode;
    bit<1> sol_event;
    bit<1> mig_req;
    bit<2> pad_count;
    bit<4> thv;
    bit<16> part_key;
    bit<6> reserved;
    bit<1> becn;
    bit<1> fecn;
    bit<24> dest_qp;
    bit<1> ack;
    bit<7> reserved_2;
    bit<24> seq_num;
}

header read_request_t {
    vaddr_t virt_addr;
    bit<32> rkey;
    bit<32> dma_length;
}

header read_response_t {
    bit<1> reserved;
    bit<2> opcode;
    bit<5> credit_count;
    bit<24> m_seq_num;
    bit<64> ptr;
    bit<KEY_SIZE> data;
    //data
    //icrc
}

header write_request_t {
    vaddr_t virt_addr;
    bit<32> rkey;
    bit<32> dma_length;
    vaddr_t ptr;
    bit<KEY_SIZE> data;
    //data
    //icrc
}

header ack_t {
    bit<1> reserved;
    bit<2> opcode;
    bit<5> credit_count;
    bit<24> m_seq_num;
}

header atomic_request_t {
    vaddr_t virt_addr;
    bit<32> rkey;
    vaddr_t swap_or_add;
    vaddr_t compare;
    //icrc
}

header atomic_response_t {
    bit<1> reserved;
    bit<2> opcode;
    bit<5> credit_count;
    bit<24> m_seq_num;
    vaddr_t original;
    //icrc
}


header vlan_tag_t {
    bit<3> pcp;
    bit<1>cfi;
    bit<12>vid;
    bit<16>etherType;
}

struct headers {
    ethernet_t ethernet;
    ipv4_t ipv4;
    udp_t udp;
    rocev2_t roce; 
    //Now we are going to do clover bth+headers
    read_request_t read_req;
    read_response_t read_resp;
    write_request_t write_req;
    ack_t ack;
    atomic_request_t atomic_req;
    atomic_response_t atomic_resp;
}

#endif