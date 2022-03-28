#include "measurement.h"
#include "rmemc-dpdk.h"
#include "print_helpers.h"
#include <signal.h>

#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>



#define TOTAL_IN_FLIGHT_TIMESTAMPS 10000
#define TOTAL_MSG_TYPES 4

#define SEND 0
#define CAS 1
#define WRITE 2
#define READ 3

uint32_t rdma_msg_type_index_map(uint8_t opcode) {
	switch (opcode)
	{
	case RC_SEND:
		return SEND;
	case RC_WRITE_ONLY:
		return WRITE;
	case RC_READ_REQUEST:
		return READ;
	case RC_CNS:
		return CAS;
	default:
        printf("ERROR (EXITING) Simplified message type not accounted for\n");
        exit(1);
		return TOTAL_MSG_TYPES + 1;
	}
}

uint32_t inverse_rdma_msg_type_index_map(uint8_t opcode) {
	switch (opcode)
	{
	case SEND:
		return RC_SEND;
	case WRITE:
		return RC_WRITE_ONLY;
	case READ:
		return RC_READ_REQUEST;
	case CAS:
		return RC_CNS;
	default:
        printf("ERROR (EXITING) Simplified message type not accounted for\n");
        exit(1);
		return 666;
	}
}

uint32_t in_flight_measurement[TOTAL_IN_FLIGHT_TIMESTAMPS][TOTAL_CLIENTS][TOTAL_MSG_TYPES];
uint64_t in_flight_timestamps[TOTAL_IN_FLIGHT_TIMESTAMPS];
uint64_t in_flight_count=0;

void calculate_in_flight(struct Connection_State (*states)[TOTAL_ENTRY]) {
    if (in_flight_count >= TOTAL_IN_FLIGHT_TIMESTAMPS) {
        return;
    }
    uint64_t ts = timestamp();
    in_flight_timestamps[in_flight_count]=ts;
    for (int i=0;i<TOTAL_CLIENTS;i++){
        struct Connection_State *cs = &(*states)[i];
        for(int j=0;j<CS_SLOTS;j++) {
            struct Request_Map * slot = &(cs->Outstanding_Requests[j]);
            if(!slot_is_open(slot)) {
                in_flight_measurement[in_flight_count][i][rdma_msg_type_index_map(slot->rdma_op)]++;
            }
        }
    }
    in_flight_count++;
}

void write_in_flight_to_known_file(void)
{
    const char *filename = "/tmp/in_flight.dat";
    printf("Writing a total of %" PRIu64 " in flight measuremnts to %s\n", in_flight_count, filename);
    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
    {
        printf("Unable to write file out, fopen has failed\n");
        perror("Failed: ");
        return;
    }
    for (uint32_t i=0; i<in_flight_count; i++){
        for(uint32_t j =0; j<TOTAL_CLIENTS;j++) {
            for(uint32_t k=0;k<TOTAL_MSG_TYPES;k++) {
                //timestamp,id,msg_type,count
                //printf("%"PRIu64",%d,%s,%d\n", in_flight_timestamps[i],j, ib_print_op(inverse_rdma_msg_type_index_map(k)), in_flight_measurement[i][j][k]);
                fprintf(fp, "%"PRIu64",%d,%s,%d\n", in_flight_timestamps[i],j, ib_print_op(inverse_rdma_msg_type_index_map(k)), in_flight_measurement[i][j][k]);
            }
        }
    }
    fclose(fp);
}



//Measurement for getting end host latencies
//rdma calls counts the number of calls for each RDMA op code
uint64_t packet_latencies[TOTAL_PACKET_LATENCIES];
uint64_t packet_latency_count = 0;

//measurement for understanding mapped packet ordering
uint32_t sequence_order[TOTAL_ENTRY][TOTAL_PACKET_SEQUENCES];
uint64_t sequence_order_timestamp[TOTAL_ENTRY][TOTAL_PACKET_SEQUENCES];
uint32_t request_count_id[TOTAL_ENTRY];
uint64_t read_redirections = 0;
uint64_t reads = 0;
uint64_t reads_failed = 0;
uint64_t read_misses = 0;
uint64_t writes =0;
uint64_t writes_failed=0;

uint64_t bytes_processed = 0;

static __inline__ int64_t rdtsc_s(void)
{
    unsigned a, d;
    asm volatile("cpuid" ::
                     : "%rax", "%rbx", "%rcx", "%rdx");
    asm volatile("rdtsc"
                 : "=a"(a), "=d"(d));
    return ((unsigned long)a) | (((unsigned long)d) << 32);
}

static __inline__ int64_t rdtsc_e(void)
{
    unsigned a, d;
    asm volatile("rdtscp"
                 : "=a"(a), "=d"(d));
    asm volatile("cpuid" ::
                     : "%rax", "%rbx", "%rcx", "%rdx");
    return ((unsigned long)a) | (((unsigned long)d) << 32);
}

void sum_processed_data(struct rte_mbuf * pkt){
	struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
	struct rte_ipv4_hdr *ipv4_hdr = (struct rte_ipv4_hdr *)((uint8_t *)eth_hdr + sizeof(struct rte_ether_hdr));
	uint64_t size = ntohs(ipv4_hdr->total_length);
    bytes_processed += size;
}

int64_t timestamp(void) {
    return rdtsc_s();
}

void append_packet_latency(uint64_t clock_cycles)
{
    if (packet_latency_count < TOTAL_PACKET_LATENCIES)
    {
        packet_latencies[packet_latency_count] = clock_cycles;
        packet_latency_count++;
    }
}

void append_sequence_number(uint32_t id, uint32_t seq)
{
    if (request_count_id[id] < TOTAL_PACKET_SEQUENCES)
    {
        sequence_order[id][request_count_id[id]] = readable_seq(seq);
        sequence_order_timestamp[id][request_count_id[id]] = rdtsc_s();
        request_count_id[id]++;
    }
}

void write_packet_latencies_to_known_file(void)
{
    const char *filename = "/tmp/latency-latest.dat";
    printf("Writing a total of %" PRIu64 " packet latencies to %s\n", packet_latency_count, filename);
    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
    {
        printf("Unable to write file out, fopen has failed\n");
        perror("Failed: ");
        return;
    }
    for (uint32_t i = 0; i < packet_latency_count; i++)
    {
        fprintf(fp, "%" PRIu64 "\n", packet_latencies[i]);
    }
    fclose(fp);
}

void write_sequence_order_to_known_file(void)
{
    const char *filename = "/tmp/sequence_order.dat";
    printf("Writing Sequence Order to file %s\n", filename);
    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
    {
        printf("Unable to write file out, fopen has failed\n");
        perror("Failed: ");
        return;
    }
    for (int i = 0; i < TOTAL_ENTRY; i++)
    {
        for (uint32_t j = 0; j < request_count_id[i]; j++)
        {
            fprintf(fp, "%d,%d,%" PRIu64 "\n", i, sequence_order[i][j], sequence_order_timestamp[i][j]);
        }
    }
    fclose(fp);
}

void write_general_stats_to_known_file(void)
{
    const char *filename = "/tmp/switch_statistics.dat";
    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
    {
        printf("Unable to write file out, fopen has failed\n");
        perror("Failed: ");
        return;
    }
    printf("Writing to %s\n",filename);
    fprintf(fp, "READS %" PRIu64 "\n", reads);
    fprintf(fp, "READS FAILED %" PRIu64 "\n", reads_failed);
    fprintf(fp, "READ REDIRECTIONS %" PRIu64 "\n", read_redirections);
    fprintf(fp, "READ MISSES %" PRIu64 "\n", read_misses);
    fprintf(fp, "READ HITS %" PRIu64 "\n", reads - read_misses);
    fprintf(fp, "WRITES %" PRIu64 "\n",writes);
    fprintf(fp, "WRITES FAILED %" PRIu64 "\n",writes_failed);
    fprintf(fp, "Data Processed %"PRIu64"\n", bytes_processed);
    fprintf(fp, "%2.3f,\n",(float)reads_failed/(float)reads*100.0);
    fprintf(fp, "%2.3f,\n",(float)writes_failed/(float)writes*100.0);
    fclose(fp);
}

void write_run_data(void)
{
    printf("WRITING OUT DATA!!\n");
    write_packet_latencies_to_known_file();
    write_sequence_order_to_known_file();
    write_general_stats_to_known_file();
    write_in_flight_to_known_file();
}

void increment_read_counter(void) {
    reads++;
}

void failed_read(void) {
    reads_failed++;
}

void read_redirected(void)
{
    read_redirections++;
}

void read_not_cached(void)
{
    read_misses++;
}

void increment_write_counter(void) {
    writes++;
}

void failed_write(void) {
    writes_failed++;
}

void stk_trc_handler(int sig) {
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}




volatile sig_atomic_t flag = 0;
void kill_signal_handler(int sig)
{
    printf("Kill signal handlers [sig %d]\n", sig);
#ifdef TAKE_MEASUREMENTS
    write_run_data();
#endif
    exit(0);
}

void register_handler(void)
{
    signal(SIGSEGV, stk_trc_handler);   // install our handler
    signal(SIGINT, kill_signal_handler);
    signal(SIGTERM, kill_signal_handler);
}

void init_measurements(void)
{
    register_handler();
    bzero(packet_latencies, TOTAL_PACKET_LATENCIES * sizeof(uint64_t));
    bzero(sequence_order, TOTAL_ENTRY * TOTAL_PACKET_SEQUENCES * sizeof(uint32_t));
    bzero(sequence_order_timestamp, TOTAL_ENTRY * TOTAL_PACKET_SEQUENCES * sizeof(uint64_t));
    bzero(request_count_id, TOTAL_ENTRY * sizeof(uint32_t));
    bzero(in_flight_measurement, TOTAL_IN_FLIGHT_TIMESTAMPS*TOTAL_CLIENTS*TOTAL_MSG_TYPES*sizeof(uint32_t));
    bzero(in_flight_timestamps, TOTAL_IN_FLIGHT_TIMESTAMPS*sizeof(uint64_t));
}