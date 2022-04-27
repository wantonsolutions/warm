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
        hdr.ipv4.ttl = hdr.ipv4.ttl - 5;
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


    apply {
        forward.apply();
        update.apply();

        // if(ipv4.ttl == 0) {
        //     drop_ttl();
        // }
        // {
        //     dec_ttl();
        // }

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
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
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