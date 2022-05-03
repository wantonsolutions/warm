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

typedef bit<9>  egressSpec_t;
struct metadata {
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


            //{ RC_SEND: "RC_SEND", RC_WRITE_ONLY: "RC_WRITE_ONLY", RC_READ_REQUEST: "RC_READ_REQUEST", RC_READ_RESPONSE: "RC_READ_RESPONSE", RC_ACK: "RC_ACK", RC_ATOMIC_ACK: "RC_ATOMIC_ACK", RC_CNS: "RC_CSN", ECN_OPCODE: "ECN_OPCODE"} ),
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
    rocev2_t rdma; 
}

#endif