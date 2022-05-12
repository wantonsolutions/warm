/*
        SWORDBOX
*/

// use barefoot tofino arch
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
        ig_tm_md.ucast_egress_port = port;
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

    //The size of the hashed value for qp's make this larger if there are
    //collisons, make it smaller if you start to run out of space
    #define QP_HASH_WIDTH 12
    #define QP_ID_TABLE_SIZE ( 1 << QP_HASH_WIDTH)

    //The size of the ID's used in the table, and the total number of ID's.
    #define MAX_IDS 255
    //id_hash hashes qp to 16 bytes. It is the unique id for qp
    Hash<bit<16>>(HashAlgorithm_t.CRC16) id_hash;

    //This register is used to track the existance of ID's. Because we cant both
    //lookup check for the id counter in a single step this is used to check if
    //the ID exists prior either reading or writing it.
    Register<bit<1>, bit<QP_HASH_WIDTH>>(QP_ID_TABLE_SIZE, 0) id_exists_reg;
    RegisterAction<bit<1>, bit<QP_HASH_WIDTH>, bit<1>>(id_exists_reg) id_exists_reg_action = {
            void apply(inout bit<1> value, out bit<1> read_value) {
                    read_value = value;
                    value = 1;
            }
    };

    //Counter tracks how man threads are running. This is incremented each time a new qp shows up.
    DirectRegister<bit<ID_SIZE>>() id_counter;
    DirectRegisterAction<bit<ID_SIZE>,bit<ID_SIZE>>(id_counter) increment_id_counter_action = {
        void apply(inout bit<ID_SIZE> value, out bit<ID_SIZE> read_value) {
            read_value = value;
            value = value + 1;
        }
    };

    //Register for storing qp-id maps. The ID's that we will use are small ie
    //1-255 which are easy to index into tables. but the qp is large so we need
    //this to map between the two.
    Register<bit<ID_SIZE>, bit<QP_HASH_WIDTH>>(QP_ID_TABLE_SIZE, 0) qp_id_reg;

    //Read the qp-id map
    RegisterAction<bit<ID_SIZE>, bit<QP_HASH_WIDTH>, bit<ID_SIZE>>(qp_id_reg) qp_id_reg_action_read = {
            void apply(inout bit<ID_SIZE> value, out bit<ID_SIZE> read_value) {
                    read_value = value;
            }
    };

    //write the qp id map
    RegisterAction<bit<ID_SIZE>, bit<QP_HASH_WIDTH>, bit<ID_SIZE>>(qp_id_reg) qp_id_reg_action_write = {
            void apply(inout bit<ID_SIZE> value) {
                    value = meta.id;
            }
    };


    action check_and_set_id_exists(bit<QP_HASH_WIDTH> qp_hash){
            meta.existing_id = id_exists_reg_action.execute(qp_hash);
    }

    action gen_new_id(){
            meta.id = increment_id_counter_action.execute();
    }

    action write_new_id(bit<QP_HASH_WIDTH> qp_hash) {
            qp_id_reg_action_write.execute(qp_hash);
    }

    action read_id(bit<QP_HASH_WIDTH> qp_hash) {
            meta.id = qp_id_reg_action_read.execute(qp_hash);
    }


    //Latest Key read/write
    Register<bit<KEY_SIZE>, bit<ID_SIZE>>(MAX_IDS,0) latest_keys;
    //Write to the latest key
    RegisterAction<bit<KEY_SIZE>, bit<ID_SIZE>, bit<KEY_SIZE>>(latest_keys) write_latest_key = {
        void apply(inout bit<KEY_SIZE> value) {
            value = meta.key;
        }

    };
    //Read the latest key
    RegisterAction<bit<KEY_SIZE>, bit<ID_SIZE>, bit<KEY_SIZE>>(latest_keys) read_latest_key = {
        void apply(inout bit<KEY_SIZE> value, out bit<KEY_SIZE> read_value) {
            read_value=value;
        }
    };
    action get_latest_key(bit<ID_SIZE> id) {
        meta.key = read_latest_key.execute(id);
    }
    //The key must allready be in the metadata
    action set_latest_key(bit<ID_SIZE> id) {
        write_latest_key.execute(id);
    }


    #define ADDR_WIDTH 32
    //First Write Register
    Register<bit<ADDR_WIDTH>, bit<KEY_SIZE>>(MAX_KEYS, 0) first_write;

    //set the metadata to the vadder here
    RegisterAction<bit<ADDR_WIDTH>, bit<KEY_SIZE>, bit<ADDR_WIDTH>>(first_write) test_first_write_low = {
        void apply(inout bit<ADDR_WIDTH> value, out bit<ADDR_WIDTH> read_value) {


            read_value = value;

            if (value == 1) {
                value = meta.vaddr.lower;
            }

            if (value == 0) {
                value = 1;
            }

        }
    };

    action check_first_write(bit <KEY_SIZE> key) {
        //meta.first_write.lower = 
        
        bit<ADDR_WIDTH> val = test_first_write_low.execute(key);
        bit<ADDR_WIDTH> local_key = 0;
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

        //size = 2048;

     }

    apply {

        //get the ID for the rdma packet
        bit<QP_HASH_WIDTH> qp_hash_index = (bit<QP_HASH_WIDTH>) id_hash.get(hdr.roce.dest_qp);
        check_and_set_id_exists(qp_hash_index);

        if (meta.existing_id == 0) {
            gen_new_id();
            write_new_id(qp_hash_index);
        } else {
            read_id(qp_hash_index);
        }
        //From here on the metadata has the id set.

        //Write Path
        if (hdr.roce.opcode == RC_WRITE_ONLY && (hdr.ipv4.totalLength != 252 && hdr.ipv4.totalLength != 68)) {

            /* the shift values changes based on rdma packet size. 
            I never figured out other packets, but this will work for 1024

            the function can be found in check_and_cache_predicted_shift rmemc-dpdk.c 
            May10 2022 -Stew
            */

            #define SHIFT_VALUE_1024 10
            meta.key = hdr.write_req.data;
            set_latest_key(meta.id);

            check_first_write(meta.key);

            
            //This is a write we want to cache
            //hdr.ipv4.checksum=0;
            //hdr.ipv4.checksum=hdr.ipv4.checksum;

            /*
            //This is a sanity check, i gues we are working in the correct endian
            if (hdr.ipv4.totalLength == 1084){
                hdr.ipv4.checksum=1;
            }*/
        } else if (hdr.roce.opcode == RC_CNS) {
            get_latest_key(meta.id);
        }


        forward.apply();
        //update.apply();

        //call the multiplex rdma twice
        multiplex_rdma.apply();
        ig_tm_md.bypass_egress = 1w1;
        //multiplex_rdma.apply();
    }

}

control SwitchIngressDeparser(packet_out packet,
                              inout headers hdr,
                              in metadata meta,
                              in ingress_intrinsic_metadata_for_deparser_t ig_intr_dprsr_md
                              ) {
    apply {
        packet.emit(hdr);
    }
}
                                


parser EmptyEgressParser(
        packet_in packet,
        out headers hdr,
        out metadata meta,
        out egress_intrinsic_metadata_t eg_intr_md) {
    state start {
        transition accept;
    }
}

control EmptyEgressDeparser(
        packet_out packet,
        inout headers hdr,
        in metadata meta,
        in egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md) {
    apply {}
}


control EmptyEgress(
        inout headers hdr,
        inout metadata meta,
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