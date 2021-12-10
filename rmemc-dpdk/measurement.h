#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include <inttypes.h>
#include "rmemc-dpdk.h"

#define TOTAL_PACKET_LATENCIES 10000
#define TOTAL_PACKET_SEQUENCES 10000
#define TAKE_MEASUREMENTS

#define MEASURE_BANDWIDTH
//#define MEASURE_READ_REDIRECTIONS
//#define MEASURE_IN_FLIGHT

int64_t timestamp(void);
void append_packet_latency(uint64_t clock_cycles);
void append_sequence_number(uint32_t id, uint32_t seq);
void write_packet_latencies_to_known_file(void);
void write_sequence_order_to_known_file(void);
void write_general_stats_to_known_file(void);
void write_run_data(void);
void sum_processed_data(struct rte_mbuf * pkt);
void increment_read_counter(void);
void read_redirected(void);
void read_not_cached(void);
void kill_signal_handler(int sig);
void register_handler(void);
void init_measurements(void);

void stk_trc_handler(int sig);
void write_in_flight_to_known_file(void);
void calculate_in_flight(struct Connection_State (*states)[TOTAL_ENTRY]);
uint32_t inverse_rdma_msg_type_index_map(uint8_t opcode);
uint32_t rdma_msg_type_index_map(uint8_t opcode);


void failed_read(void);
void read_redirected(void);
void read_not_cached(void);
void increment_write_counter(void);
void failed_write(void);
#endif