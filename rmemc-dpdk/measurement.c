#include "measurement.h"
#include "rmemc-dpdk.h"
#include <signal.h>

#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

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
uint64_t read_misses = 0;

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
    fprintf(fp, "READS %" PRIu64 "\n", reads);
    fprintf(fp, "READ REDIRECTIONS %" PRIu64 "\n", read_redirections);
    fprintf(fp, "READ MISSES %" PRIu64 "\n", read_misses);
    fprintf(fp, "READ HITS %" PRIu64 "\n", reads - read_misses);
    fclose(fp);
}

void write_run_data(void)
{
    write_packet_latencies_to_known_file();
    write_sequence_order_to_known_file();
    write_general_stats_to_known_file();
}

void read_redirected(void)
{
    read_redirections++;
}

void read_not_cached(void)
{
    printf("cache miss\n");
    read_misses++;
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
}

void init_measurements(void)
{
    register_handler();
    bzero(packet_latencies, TOTAL_PACKET_LATENCIES * sizeof(uint64_t));
    bzero(sequence_order, TOTAL_ENTRY * TOTAL_PACKET_SEQUENCES * sizeof(uint32_t));
    bzero(sequence_order_timestamp, TOTAL_ENTRY * TOTAL_PACKET_SEQUENCES * sizeof(uint64_t));
    bzero(request_count_id, TOTAL_ENTRY * sizeof(uint32_t));
}