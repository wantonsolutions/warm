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

#define SWORDBOX_OFF 0
#define WRITE_STEER 1
#define READ_STEER 2

#define RDMA_128 128
#define RDMA_256 246
#define RDMA_512 512
#define RDMA_1024 1024


#define STATISTICS

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

    Register<bit<8>, bit<1>>(1,SWORDBOX_OFF) swordbox_mode;
    RegisterAction<bit<8>, bit<1>, bit<8>>(swordbox_mode) get_swordbox_mode = {
        void apply(inout bit<8> value, out bit<8> read_value) {
            read_value = value;
        }
    };
    action get_swordbox_mode_action(bit<1> get_val){
        meta.swordbox_mode = get_swordbox_mode.execute(get_val);
    }

    Register<bit<32>, bit<1>>(1,RDMA_128) rdma_size;
    RegisterAction<bit<32>, bit<1>, bit<32>>(rdma_size) get_rdma_size = {
        void apply(inout bit<32> value, out bit<32> read_value) {
            read_value = value;
        }
    };
    action get_rdma_size_action(bit<1> get_val){
        meta.rdma_size = get_rdma_size.execute(get_val);
    }

    //#ifdef STATISTICS    
    Register<bit<32>, bit<1>>(1,0) read_miss_counter;
    RegisterAction<bit<32>, bit<1>, bit<32>>(read_miss_counter) inc_read_miss = {
        void apply(inout bit<32> value, out bit<32> read_value) {
            value=value+1;
        }
    };

    action inc_read_miss_counter_action(bit<1> get_val){
        inc_read_miss.execute(get_val);
    }
    //#endif


    //The size of the hashed value for qp's make this larger if there are
    //collisons, make it smaller if you start to run out of space
    #define QP_HASH_WIDTH 14
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
    Register<bit<ID_SIZE>,bit<1>>(1,0) id_counter;
    RegisterAction<bit<ID_SIZE>,bit<1>,bit<ID_SIZE>>(id_counter) increment_id_counter_action = {
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
            meta.id = increment_id_counter_action.execute(0);
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


    #define HALF_ADDR_WIDTH 32

    //Next Key Write - Takes the value from the metadata on writes
    Register<bit<HALF_ADDR_WIDTH>, bit<KEY_SIZE>>(MAX_KEYS, 0) next_vaddr_low;
    RegisterAction<bit<HALF_ADDR_WIDTH>, bit<KEY_SIZE>, bit<HALF_ADDR_WIDTH>>(next_vaddr_low) read_then_write_next_vaddr_low = {
        void apply(inout bit<HALF_ADDR_WIDTH> value, out bit<HALF_ADDR_WIDTH> read_value) {
            read_value = value;
            value = meta.outstanding_write_vaddr.lower;
        }
    };

    Register<bit<HALF_ADDR_WIDTH>, bit<KEY_SIZE>>(MAX_KEYS, 0) next_vaddr_high;
    RegisterAction<bit<HALF_ADDR_WIDTH>, bit<KEY_SIZE>, bit<HALF_ADDR_WIDTH>>(next_vaddr_high) read_then_write_next_vaddr_high = {
        void apply(inout bit<HALF_ADDR_WIDTH> value, out bit<HALF_ADDR_WIDTH> read_value) {
            read_value = value;
            value = meta.outstanding_write_vaddr.upper;
        }
    };

    action get_then_set_next_vaddr_low(bit <KEY_SIZE> key) {
        meta.next_vaddr.lower = read_then_write_next_vaddr_low.execute(key);
    }

    action get_then_set_next_vaddr_high(bit <KEY_SIZE> key) {
        meta.next_vaddr.upper = read_then_write_next_vaddr_high.execute(key);
    }


    //Next Key Write - Takes the value from the metadata on writes
    Register<bit<HALF_ADDR_WIDTH>, bit<ID_SIZE>>(MAX_IDS, 0) outstanding_write_vaddr_low;
    RegisterAction<bit<HALF_ADDR_WIDTH>, bit<ID_SIZE>, bit<HALF_ADDR_WIDTH>>(outstanding_write_vaddr_low) write_outstanding_write_vaddr_low = {
        void apply(inout bit<HALF_ADDR_WIDTH> value, out bit<HALF_ADDR_WIDTH> read_value) {
            value = meta.vaddr.lower;
        }
    };
    RegisterAction<bit<HALF_ADDR_WIDTH>, bit<ID_SIZE>, bit<HALF_ADDR_WIDTH>>(outstanding_write_vaddr_low) read_outstanding_write_vaddr_low = {
        void apply(inout bit<HALF_ADDR_WIDTH> value, out bit<HALF_ADDR_WIDTH> read_value) {
            read_value = value;
        }
    };

    //Next Key Write - Takes the value from the metadata on writes
    Register<bit<HALF_ADDR_WIDTH>, bit<ID_SIZE>>(MAX_IDS, 0) outstanding_write_vaddr_high;
    RegisterAction<bit<HALF_ADDR_WIDTH>, bit<ID_SIZE>, bit<HALF_ADDR_WIDTH>>(outstanding_write_vaddr_high) write_outstanding_write_vaddr_high = {
        void apply(inout bit<HALF_ADDR_WIDTH> value, out bit<HALF_ADDR_WIDTH> read_value) {
            value = meta.vaddr.upper;
        }
    };
    RegisterAction<bit<HALF_ADDR_WIDTH>, bit<ID_SIZE>, bit<HALF_ADDR_WIDTH>>(outstanding_write_vaddr_high) read_outstanding_write_vaddr_high = {
        void apply(inout bit<HALF_ADDR_WIDTH> value, out bit<HALF_ADDR_WIDTH> read_value) {
            read_value = value;
        }
    };

    action set_outstanding_write_vaddr_low(bit <ID_SIZE> id) {
        write_outstanding_write_vaddr_low.execute(id);
    }

    action set_outstanding_write_vaddr_high(bit <ID_SIZE> id) {
        write_outstanding_write_vaddr_high.execute(id);
    }

    action get_outstanding_write_vaddr_low(bit <ID_SIZE> id) {
        meta.outstanding_write_vaddr.lower = read_outstanding_write_vaddr_low.execute(id);
    }

    action get_outstanding_write_vaddr_high(bit <ID_SIZE> id) {
        meta.outstanding_write_vaddr.upper = read_outstanding_write_vaddr_high.execute(id);
    }


    //Next Key Write - Takes the value from the metadata on writes
    #define WRITE_HASH_WIDTH_MAX 17 //This is the max value I can fit without fanangaling crap
    #define WRITE_HASH_WIDTH 19
    #define WRITE_CACHE_SIZE (1 << WRITE_HASH_WIDTH)
    Hash<bit<32>>(HashAlgorithm_t.CRC32) write_cache_hash;
    Hash<bit<32>>(HashAlgorithm_t.CRC32) read_cache_hash;

    #define CHOPPED_ADDR_WIDTH 8

    Register<bit<CHOPPED_ADDR_WIDTH>, bit<WRITE_HASH_WIDTH>>(WRITE_CACHE_SIZE, 0) write_cache_low;
    RegisterAction<bit<CHOPPED_ADDR_WIDTH>, bit<WRITE_HASH_WIDTH>, bit<CHOPPED_ADDR_WIDTH>>(write_cache_low) write_write_cache_low = {
        void apply(inout bit<CHOPPED_ADDR_WIDTH> value, out bit<CHOPPED_ADDR_WIDTH> read_value) {
            value = (bit<CHOPPED_ADDR_WIDTH>)meta.vaddr.lower;
        }
    };
    RegisterAction<bit<CHOPPED_ADDR_WIDTH>, bit<WRITE_HASH_WIDTH>, bit<CHOPPED_ADDR_WIDTH>>(write_cache_low) read_write_cache_low = {
        void apply(inout bit<CHOPPED_ADDR_WIDTH> value, out bit<CHOPPED_ADDR_WIDTH> read_value) {
            read_value = value;
        }
    };

    action set_write_cache_low(bit <WRITE_HASH_WIDTH> vaddr_hash) {
        write_write_cache_low.execute(vaddr_hash);
    }
    action get_write_cache_low(bit <WRITE_HASH_WIDTH> vaddr_hash) {
        meta.write_cached_addr.lower=(bit<HALF_ADDR_WIDTH>)read_write_cache_low.execute(vaddr_hash);
    }

    Register<bit<CHOPPED_ADDR_WIDTH>, bit<WRITE_HASH_WIDTH>>(WRITE_CACHE_SIZE, 0) write_cache_high;
    RegisterAction<bit<CHOPPED_ADDR_WIDTH>, bit<WRITE_HASH_WIDTH>, bit<CHOPPED_ADDR_WIDTH>>(write_cache_high) write_write_cache_high = {
        void apply(inout bit<CHOPPED_ADDR_WIDTH> value, out bit<CHOPPED_ADDR_WIDTH> read_value) {
            value = (bit<CHOPPED_ADDR_WIDTH>)meta.vaddr.upper;
        }
    };
    RegisterAction<bit<CHOPPED_ADDR_WIDTH>, bit<WRITE_HASH_WIDTH>, bit<CHOPPED_ADDR_WIDTH>>(write_cache_high) read_write_cache_high = {
        void apply(inout bit<CHOPPED_ADDR_WIDTH> value, out bit<CHOPPED_ADDR_WIDTH> read_value) {
            read_value = value;
        }
    };

    action set_write_cache_high(bit <WRITE_HASH_WIDTH> vaddr_hash) {
        write_write_cache_high.execute(vaddr_hash);
    }
    action get_write_cache_high(bit <WRITE_HASH_WIDTH> vaddr_hash) {
        meta.write_cached_addr.upper=(bit<HALF_ADDR_WIDTH>)read_write_cache_high.execute(vaddr_hash);
    }

    Register<bit<KEY_SIZE>, bit<WRITE_HASH_WIDTH>>(WRITE_CACHE_SIZE, 0) write_cache_key;
    RegisterAction<bit<KEY_SIZE>, bit<WRITE_HASH_WIDTH>, bit<KEY_SIZE>>(write_cache_key) write_write_cache_key = {
        void apply(inout bit<KEY_SIZE> value, out bit<KEY_SIZE> read_value) {
            value = meta.key;
        }
    };
    RegisterAction<bit<KEY_SIZE>, bit<WRITE_HASH_WIDTH>, bit<KEY_SIZE>>(write_cache_key) read_write_cache_key = {
        void apply(inout bit<KEY_SIZE> value, out bit<KEY_SIZE> read_value) {
            read_value = value;
        }
    };

    action set_write_cache_key(bit <WRITE_HASH_WIDTH> vaddr_hash) {
        write_write_cache_key.execute(vaddr_hash);
    }

    action get_write_cache_key(bit <WRITE_HASH_WIDTH> vaddr_hash) {
        meta.key=read_write_cache_key.execute(vaddr_hash);
    }

    //Read Tail
    Register<bit<HALF_ADDR_WIDTH>, bit<KEY_SIZE>>(MAX_KEYS, 0) read_tail_low;
    RegisterAction<bit<HALF_ADDR_WIDTH>, bit<KEY_SIZE>, bit<HALF_ADDR_WIDTH>>(read_tail_low) read_read_tail_low = {
        void apply(inout bit<HALF_ADDR_WIDTH> value, out bit<HALF_ADDR_WIDTH> read_value) {
            read_value = value;
        }
    };
    RegisterAction<bit<HALF_ADDR_WIDTH>, bit<KEY_SIZE>, bit<HALF_ADDR_WIDTH>>(read_tail_low) write_read_tail_low = {
        void apply(inout bit<HALF_ADDR_WIDTH> value, out bit<HALF_ADDR_WIDTH> read_value) {
            //value = meta.next_vaddr.lower;
            //value = hdr.atomic_req.virt_addr.lower;
            value = meta.outstanding_write_vaddr.lower;
        }
    };

    action get_read_tail_low(bit <KEY_SIZE> key) {
        meta.read_tail.lower=read_read_tail_low.execute(key);
    }
    action set_read_tail_low(bit <KEY_SIZE> key) {
        write_read_tail_low.execute(key);
    }

    Register<bit<HALF_ADDR_WIDTH>, bit<KEY_SIZE>>(MAX_KEYS, 0) read_tail_high;
    RegisterAction<bit<HALF_ADDR_WIDTH>, bit<KEY_SIZE>, bit<HALF_ADDR_WIDTH>>(read_tail_high) read_read_tail_high = {
        void apply(inout bit<HALF_ADDR_WIDTH> value, out bit<HALF_ADDR_WIDTH> read_value) {
            read_value = value;
        }
    };
    RegisterAction<bit<HALF_ADDR_WIDTH>, bit<KEY_SIZE>, bit<HALF_ADDR_WIDTH>>(read_tail_high) write_read_tail_high = {
        void apply(inout bit<HALF_ADDR_WIDTH> value, out bit<HALF_ADDR_WIDTH> read_value) {
            //value = meta.next_vaddr.upper;
            //value = hdr.atomic_req.virt_addr.upper;
            value = meta.outstanding_write_vaddr.upper;
        }
    };

    action get_read_tail_high(bit <KEY_SIZE> key) {
        meta.read_tail.upper=read_read_tail_high.execute(key);
    }
    action set_read_tail_high(bit <KEY_SIZE> key) {
        write_read_tail_high.execute(key);
    }


    apply {

        get_swordbox_mode_action(0);
        get_rdma_size_action(0);
        if (
            (meta.swordbox_mode >= WRITE_STEER) &&
            //Write Packtes
            (hdr.roce.opcode == RC_WRITE_ONLY && 
             hdr.write_req.dma_length == meta.rdma_size) ||
            //CAS packets
            (hdr.roce.opcode == RC_CNS) ||
            //Read Packets
            (hdr.roce.opcode == RC_READ_REQUEST &&
             hdr.read_req.dma_length == meta.rdma_size)
        ) {

            bit<QP_HASH_WIDTH> qp_hash_index = (bit<QP_HASH_WIDTH>) id_hash.get(hdr.roce.dest_qp);
            check_and_set_id_exists(qp_hash_index);
            if (meta.existing_id == 0) {
                gen_new_id();
                write_new_id(qp_hash_index);
            } else {
                read_id(qp_hash_index);
            }



            //Write Path
            if (hdr.roce.opcode == RC_WRITE_ONLY) {// && (hdr.ipv4.totalLength != 252 && hdr.ipv4.totalLength != 68)) {

                //Get the key from the data packet, and place it in metadata
                meta.key = hdr.write_req.data;
                set_latest_key(meta.id);
                //Grab the virtual address from the packet
                meta.vaddr.lower=hdr.write_req.virt_addr.lower;
                meta.vaddr.upper=hdr.write_req.virt_addr.upper;
                //put the virtual address of the write into the set of outstanding writes.
                set_outstanding_write_vaddr_low(meta.id);
                set_outstanding_write_vaddr_high(meta.id);

                //#ifdef READ_STEER
                if (meta.swordbox_mode >= READ_STEER) {
                    //TODO mega dang use the entire 64 bit address for hashing
                    bit<WRITE_HASH_WIDTH> write_hash_index = (bit<WRITE_HASH_WIDTH>) write_cache_hash.get(hdr.write_req.virt_addr.lower);
                    set_write_cache_low(write_hash_index);
                    set_write_cache_high(write_hash_index);
                    set_write_cache_key(write_hash_index);
                }
                //update_write_vaddr_cache(*key, wr->rdma_extended_header.vaddr);



            } else if (hdr.roce.opcode == RC_CNS) {


                get_latest_key(meta.id);

                meta.vaddr.lower = hdr.atomic_req.virt_addr.lower;
                meta.vaddr.upper = hdr.atomic_req.virt_addr.upper;

                get_outstanding_write_vaddr_low(meta.id);
                get_outstanding_write_vaddr_high(meta.id);

            //Move to state machine 
                get_then_set_next_vaddr_low(meta.key);
                get_then_set_next_vaddr_high(meta.key);



                if (meta.next_vaddr.lower != 0) {
                    //TODO I should compare both
                    if((meta.next_vaddr.lower != hdr.atomic_req.virt_addr.lower)) { //} || (meta.next_vaddr.upper != hdr.atomic_req.virt_addr.upper)) {
                        hdr.atomic_req.virt_addr.lower = meta.next_vaddr.lower;
                        hdr.atomic_req.virt_addr.upper = meta.next_vaddr.upper;
                    }
                } 

                if (meta.swordbox_mode >= READ_STEER) {
                    set_read_tail_low(meta.key);
                    set_read_tail_high(meta.key);
                }
                


            } else if (hdr.roce.opcode == RC_READ_REQUEST) {// && hdr.read_req.dma_length == 1024) {

                if (meta.swordbox_mode >= READ_STEER) {

                    bit<WRITE_HASH_WIDTH> read_hash_index = (bit<WRITE_HASH_WIDTH>) read_cache_hash.get(hdr.read_req.virt_addr.lower);
                    get_write_cache_key(read_hash_index);

                    //Check if the hash of this vadder is a hit and matches
                    get_write_cache_low(read_hash_index);
                    get_write_cache_high(read_hash_index);
                    get_read_tail_low(meta.key);
                    get_read_tail_high(meta.key);                

                    //Check that the cache hit is legitimate, we know that this address is for a known key
                    //TODO make this simpler so that I can use both addresses
                    bit<CHOPPED_ADDR_WIDTH> chopped_lower = (bit<CHOPPED_ADDR_WIDTH>)hdr.read_req.virt_addr.lower;
                    if ((bit<CHOPPED_ADDR_WIDTH>)meta.write_cached_addr.lower == chopped_lower && (meta.read_tail.lower != 0)) { //&& (meta.key != 0) ) { // && meta.write_cached_addr.upper == hdr.write_req.virt_addr.upper) {
                        //We found that the latest value was cached so we know the key
                        //In this case we always update the packet
                        //We could skip updating the packet if the value was allready correct

                        hdr.read_req.virt_addr.lower=meta.read_tail.lower;
                        hdr.read_req.virt_addr.upper=meta.read_tail.upper;
                        //hdr.read_req.virt_addr.lower=hdr.read_req.virt_addr.lower;
                    }
                    //#ifdef STATISTICS
                    else {
                        inc_read_miss_counter_action(0);
                    }
                    //#endif
                }
            }
        }



        forward.apply();
        ig_tm_md.bypass_egress = 1w1;
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