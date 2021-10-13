import numpy as np
import matplotlib.pyplot as plt
import matplotlib
import csv

def cdf(data):
    high = max(data)
    low = min(data)
    norm = matplotlib.colors.Normalize(low,high)

    #print(data)
    count, bins_count = np.histogram(data, bins = 1000000)
    pdf = count / sum(count)
    
    y = np.cumsum(pdf)
    return bins_count[1:], y


def plot_charts(source_filename, sequence_output_filename, timeline_output_filename):
    id, sequences, timestamps = np.loadtxt(source_filename,delimiter=',', unpack=True)
    # add another variable if you have more CSV vars
    #x, y = np.loadtxt('/tmp/latency-latest.dat')

    clocks_to_seconds = (1.2 * 1000 * 1000 * 1000)
    minimum = min(timestamps)
    timestamps = [ (a - minimum) / clocks_to_seconds  for a in timestamps]

    last_seq=0
    current_id=-1
    index=0
    gap = []
    individual_gap = []
    individual_gap_single = []
    individual_timestamps = []
    individual_timestamps_single = []

    for i in id:
        if current_id != i:
            current_id=i
            last_seq=sequences[index]
            individual_gap.append(individual_gap_single)
            individual_gap_single = []
            individual_timestamps.append(individual_timestamps_single)
            individual_timestamps_single = []

        else:
            diff=sequences[index] - last_seq
            gap.append(diff)
            individual_gap_single.append(diff)
            individual_timestamps_single.append(timestamps[index])
            last_seq = sequences[index]

        index=index+1
        
    print(len(gap))

    x, y = cdf(gap)
    #x =x.astype(int)
    print(x)
    plt.ylim(0, 1)
    plt.plot(x,y, label="sequence_steps")
    plt.savefig(sequence_output_filename)
    plt.clf()

    plt.figure(dpi=100)
    index=0
    for thread in individual_gap:
        #x = range(len(thread))
        x = individual_timestamps[index]
        #y = thread
        y = np.full(len(x),index)
        plt.plot(x,y, label=str(index), marker='x')
        index=index+1
    plt.ylabel("threads id")
    plt.xlabel("Seconds")
    plt.savefig(timeline_output_filename)
    plt.clf()


    print(individual_timestamps[0])
    index=0
    op_diffs=[]
    per_thread_diffs=[]
    for thread in individual_timestamps:
        per_thread_diffs.append([])
        operation_timestamps=individual_timestamps[index]
        if(len(operation_timestamps)==0):
            print("no operations for thread " + str(index))
            index=index+1
            continue
        
        last_op_time=operation_timestamps[0]
        operation_index=1

        for op_time in operation_timestamps[1:]:
            diff=op_time-last_op_time
            print(diff)
            op_diffs.append(diff)
            per_thread_diffs[index].append(diff)
            last_op_time=op_time

        index=index+1
    
    x, y = cdf(op_diffs)
    print(x)
    plt.ylim(0, 1)
    plt.plot(x,y, label="Op Completion Latencies")
    plt.title("Operation Interval")
    plt.xscale('log')
    plt.xlabel("seconds")
    plt.tight_layout()
    plt.savefig("op_latencies.pdf")
    plt.clf()

    thread_index=0
    for thread_diff in per_thread_diffs:
        if(len(thread_diff) == 0):
            thread_index=thread_index+1
            continue

        x, y = cdf(thread_diff)
        plt.plot(x,y, label=str(thread_index))
        thread_index=thread_index+1

    plt.ylim(0, 1)
    plt.plot(x,y, label="Per Thread Operation Interval")
    plt.title("Operation Interval Per Thread")
    plt.xscale('log')
    plt.xlabel("seconds")
    plt.tight_layout()
    plt.savefig("op_latencies_per_thread.pdf")
    plt.clf()
    







plot_charts('/tmp/sequence_order.dat','sequence.pdf','timeline.pdf')


