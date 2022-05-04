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

// This is P4 sample source for basic_switching
#include <core.p4>
#include <v1model.p4>

#include "includes/headers.p4"
#include "includes/parser.p4"

//#include <tofino/intrinsic_metadata.p4>
//#include <tofino/constants.p4>


control MyIngress(inout headers hdr,
    inout metadata meta,
    inout standard_metadata_t standard_metadata){

    action set_egr(egressSpec_t port) {
        standard_metadata.egress_spec= port;
        //modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
    }

    action drop() {
        mark_to_drop(standard_metadata);
    }

    action nop() {
    }

    action dec_ttl() {
        hdr.ipv4.ttl = hdr.ipv4.ttl - 1;
    }


    table forward {
        key = {
            hdr.ethernet.dstAddr : exact;
        }
        actions = {
            set_egr; 
            nop;
        }
    }

    table update {
        key = {
            hdr.ethernet.dstAddr : exact;
        }
        actions = {
            dec_ttl;
            nop;
        }
    }

    action write_req() {

    }

    action read_req() {

    }

    action read_resp() {

    }

    action ack() {

    }

    action cns() {

    }

    action atomic_ack() {

    }

    table multiplex_rdma {
        key = {
            hdr.roce.opcode: exact;
        }

        actions = {
            write_req;
            ack;
            read_req;
            read_resp;
            cns;
            atomic_ack;
        }

        const entries = {
            RC_WRITE_ONLY : write_req();
            RC_READ_REQUEST : read_req();
            RC_READ_RESPONSE : read_resp();
            RC_ACK : ack();
            RC_ATOMIC_ACK : atomic_ack();
            RC_CNS : cns();
        }

    }

    apply {
        forward.apply();
        update.apply();
        multiplex_rdma.apply();
    }

}

control MyVerifyChecksum(inout headers  hdr, inout metadata meta){
    apply{}
}

control MyComputeChecksum(inout headers  hdr, inout metadata meta){
    apply{}
}

control MyEgress(inout headers hdr, inout metadata meta,
        inout standard_metadata_t standard_metadata) {

        action nop() {
        }

        action drop() {
            mark_to_drop(standard_metadata);
        }

        table acl {
            key = {
                hdr.ethernet.dstAddr : ternary;
                hdr.ethernet.srcAddr : ternary;
            }
            actions = {
                nop;
                drop;
            }
        }

        apply {
            acl.apply();
        }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr);
        // packet.emit(hdr.ethernet);
        // packet.emit(hdr.ipv4);
        // packet.emit(hdr.udp);
        // packet.emit(hdr.roce);

        // if(hdr.write_req.isValid()) {
        //     packet.emit(hdr.write_req);
        // }
    }
}

V1Switch(
    MyParser(),
    MyVerifyChecksum(),
    MyIngress(),
    MyEgress(),
    MyComputeChecksum(),
    MyDeparser()
) main;