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

// Template parser.p4 file for basic_switching
// Edit this file as needed for your P4 program

// This parses an ethernet header

parser SwitchIngressParser(packet_in packet,
                out headers hdr,
                inout metadata meta,
                inout ingress_intrinsic_metadata_t ig_intr_md)
                //inout standard_metadata_t standard_metadata)
                {

    state start {
        transition parse_ethernet;
    }

    #define ETHERTYPE_IPV4 0x0800

    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            ETHERTYPE_IPV4 : parse_ipv4;
            default: accept;
        }
    }

    #define IPTYPE_UDP_PROTO 0x11

    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            IPTYPE_UDP_PROTO: parse_udp;
            default: accept;
        }
    }

    #define ROCE_PORT 0x12B7 
    
    state parse_udp {
        packet.extract(hdr.udp);
        transition select(hdr.udp.dstPort) {
            ROCE_PORT: parse_roce;
            default: accept;
        }
    }

    #define RC_SEND 0x04
    #define RC_WRITE_ONLY 0x0A
    #define RC_READ_REQUEST 0x0C
    #define RC_READ_RESPONSE 0x10
    #define RC_ACK 0x11
    #define RC_ATOMIC_ACK 0x12
    #define RC_CNS 0x13
    #define ECN_OPCODE 0x81

    state parse_roce {
        packet.extract(hdr.roce);
        transition select(hdr.roce.opcode) {
            RC_WRITE_ONLY: parse_write;
            RC_READ_REQUEST: parse_read_req;
            RC_READ_RESPONSE: parse_read_resp;
            RC_ACK: parse_ack;
            RC_ATOMIC_ACK: parse_atomic_ack;
            RC_CNS: parse_cns;
            default: accept;
        }
    }

    state parse_write{
        packet.extract(hdr.write_req);
        transition accept;
    }

    state parse_read_req{
        packet.extract(hdr.read_req);
        transition accept;
    }

    state parse_read_resp{
        packet.extract(hdr.read_resp);
        transition accept;
    }

    state parse_ack{
        packet.extract(hdr.ack);
        transition accept;
    }

    state parse_atomic_ack{
        packet.extract(hdr.atomic_resp);
        transition accept;
    }

    state parse_cns{
        packet.extract(hdr.atomic_req);
        transition accept;
    }

}

