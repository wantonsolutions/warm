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


header vlan_tag_t {
    bit<3> pcp;
    bit<1>cfi;
    bit<12>vid;
    bit<16>etherType;
}

struct headers {
    ethernet_t ethernet;
    ipv4_t ipv4;
}

#endif