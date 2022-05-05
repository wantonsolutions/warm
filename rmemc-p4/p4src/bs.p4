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
#if __TARGET_TOFINO__ == 2
#include <t2na.p4>
#else
#include <tna.p4>
#endif

#include "includes/headers.p4"
#include "includes/parser.p4"

//#include <tofino/intrinsic_metadata.p4>
//#include <tofino/constants.p4>


control SwitchIngress(inout headers hdr,
    inout metadata meta,

    in ingress_intrinsic_metadata_t ig_intr_md,
    in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
    inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
    inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {

    action set_egr(egressSpec_t port) {
        //standard_metadata.egress_spec= port;
        ig_tm_md.ucast_egress_port = port;
        //modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
    }

    action drop() {
        //mark_to_drop(standard_metadata);
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

    //T == Value
    //I == Index type

    /*
    register<bit<32>>(4096) rocev2_dst_qp_reg;
    */

    // registerAction<bit<32>, bit<24>, bit<32>>(rocev2_dst_qp_reg) rocev2_dst_qp_reg_read = {
    //             void apply(inout bit<32> value, out bit<32> read_value) {
    //                     read_value = value;
    //             }
    //     };

    // registerAction<bit<32>, bit<24>, bit<32>>(rocev2_dst_qp_reg) rocev2_dst_qp_reg_write = {
    //             void apply(inout bit<32> value) {
    //                     value = (bit<32>) 0xFEDCBAFE;
    //                     //value = value +1;
    //             }
    // };

    //    
    action write_req() {
        //rocev2_dst_qp_reg_write.execute(hdr.roce.dest_qp);
        //hdr.ipv4.src_addr=rocev2_dst_qp_reg_read.execute(hdr.roce.dest_qp);
        //register_write(hdr.ipv4.src_addr,rocev2_dst_qp_reg, hdr.roce.dest_qp)
        //bit<32>wide_qp = hdr.roce.dest_qp;

        /* 
        bit<32> to_hash = (bit<32>)hdr.roce.dest_qp;
        bit<32> value = 0xFFFFFFFF;
        rocev2_dst_qp_reg.write(to_hash, value);

        hdr.roce.opcode = RC_READ_REQUEST;
        */

        // bit<32> result;
        // rocev2_dst_qp_reg.read(to_hash, result);
        // hdr.ipv4.src_addr = result;



        //rocev2_dst_qp_reg.read(hdr.ipv4.src_addr, wide_qp);
        //rocev2_dst_qp_reg.read(hdr.ipv4.src_addr, wide);
        //register_read(hdr.ipv4.src_addr,rocev2_dst_qp_reg, hdr.roce.dest_qp);

    }

    action read_req() {

        /*
        bit<32> result;
        bit<32> to_hash = (bit<32>)hdr.roce.dest_qp;
        rocev2_dst_qp_reg.read(to_hash, result);
        */
        // hdr.ipv4.src_addr = result;
        // hdr.roce.opcode = RC_WRITE_ONLY;

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

        //size = 2048;

     }

    apply {
        forward.apply();
        update.apply();

        //call the multiplex rdma twice
        multiplex_rdma.apply();
        //multiplex_rdma.apply();
    }

}

// control MyVerifyChecksum(inout headers  hdr, inout metadata meta){
//     apply{}
// }

// control MyComputeChecksum(inout headers  hdr, inout metadata meta){
//     apply{}
// }

// control MyEgress(inout headers hdr, inout metadata meta,
//         in egress_intrinsic_metadata_t ig_intr_md,
//         in egress_intrinsic_metadata_from_parser_t ig_prsr_md,
//         inout egress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
//         ) {

//         action nop() {
//         }

//         action drop() {
//             mark_to_drop(standard_metadata);
//         }

//         table acl {
//             key = {
//                 hdr.ethernet.dstAddr : ternary;
//                 hdr.ethernet.srcAddr : ternary;
//             }
//             actions = {
//                 nop;
//                 drop;
//             }
//         }

//         apply {
//             acl.apply();
//         }
// }

// control SwitchIngressDeparser(packet_out packet, in headers hdr) {
//     apply {
//         packet.emit(hdr);
//     }
// }
control SwitchIngressDeparser(packet_out packet,
                              inout headers hdr,
                              in metadata metadata,
                              in ingress_intrinsic_metadata_for_deparser_t 
                                ig_intr_dprsr_md
                              ) {
    apply {
        packet.emit(hdr);
    }
}
                                

// V1Switch(
//     MyParser(),
//     MyVerifyChecksum(),
//     MyIngress(),
//     //MyEgress(),
//     //MyComputeChecksum(),
//     MyDeparser()
// ) main;
// Empty egress parser/control blocks
parser EmptyEgressParser(
        packet_in pkt,
        out egress_intrinsic_metadata_t eg_intr_md) {
    state start {
        transition accept;
    }
}

// control EmptyEgressDeparser(
//         packet_out pkt,
//         in egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md) {
//     apply {}
// }

control EmptyEgressDeparser(
        packet_out packet,
        inout headers hdr,
        in metadata metadata,
        in egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md) {
    apply {}
}

control EmptyEgress(
        in egress_intrinsic_metadata_t eg_intr_md,
        in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
        inout egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md,
        inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md) {
    apply {}
}


Pipeline(SwitchIngressParser(),
       SwitchIngress(),
       SwitchIngressDeparser(),
       EmptyEgressParser(),
       EmptyEgress(),
       EmptyEgressDeparser()) pipe;

Switch(pipe) main;