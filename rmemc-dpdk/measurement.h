#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include <inttypes.h>

#define TOTAL_PACKET_LATENCIES 1000000
#define TOTAL_PACKET_SEQUENCES 1000000
#define TAKE_MEASUREMENTS

int64_t timestamp(void);
void append_packet_latency(uint64_t clock_cycles);
void append_sequence_number(uint32_t id, uint32_t seq);
void write_packet_latencies_to_known_file(void);
void write_sequence_order_to_known_file(void);
void write_general_stats_to_known_file(void);
void write_run_data(void);
void increment_read_counter(void);
void read_redirected(void);
void read_not_cached(void);
void kill_signal_handler(int sig);
void register_handler(void);
void init_measurements(void);

#endif