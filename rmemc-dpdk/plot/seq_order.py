import numpy as np
import matplotlib.pyplot as plt
import matplotlib
import csv

def cdf(data):
    high = max(data)
    low = min(data)
    norm = matplotlib.colors.Normalize(low,high)

    #print(data)
    count, bins_count = np.histogram(data, bins = 10000 )
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

plot_charts('/tmp/sequence_order.dat','sequence.pdf','timeline.pdf')


