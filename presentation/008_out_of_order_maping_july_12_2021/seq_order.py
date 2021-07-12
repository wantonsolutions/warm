import numpy as np
import matplotlib.pyplot as plt
import matplotlib
import csv
from matplotlib.pyplot import figure

def cdf(data):
    high = max(data)
    low = min(data)
    norm = matplotlib.colors.Normalize(low,high)

    #print(data)
    count, bins_count = np.histogram(data, bins = 100000 )
    pdf = count / sum(count)
    
    y = np.cumsum(pdf)
    x = bins_count[1:]

    print('inserting')
    y= np.insert(y,0,0)
    x= np.insert(x,0,x[0])
    print(y[0])
    return x, y


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

    figure(figsize=(10,4), dpi=160)
    plt.ylim(-0.01, 1.01)
    plt.xlim(-15,15)
    plt.xlabel("Sequence number difference between sequential packets")
    plt.plot(x,y, label="sequence_steps")
    plt.tight_layout()
    plt.savefig(sequence_output_filename)
    plt.clf()


    figure(figsize=(10,4), dpi=160)
    plt.ylim(.99, 1.001)
    plt.xlim(-15,15)
    plt.xlabel("Sequence number difference between sequential packets")
    plt.plot(x,y, label="sequence_steps")
    plt.tight_layout()
    plt.savefig("99th_"+sequence_output_filename)
    plt.clf()

    plt.figure(dpi=100)
    plt.xlim(0,5)
    plt.ylim(-15,15)
    index=0
    for thread in individual_gap:
        #x = range(len(thread))
        x = individual_timestamps[index]
        y = thread
        plt.plot(x,y, label=str(index))
        index=index+1
    plt.xlabel("Seconds")
    plt.ylabel("Sequence number difference between sequential packets")
    plt.savefig(timeline_output_filename)

plot_charts('sequence_order_no_map.dat','sequence_no_map.pdf','timeline_no_map.pdf')
plot_charts('sequence_order_no_map.dat','sequence_no_map.png','timeline_no_map.png')
plot_charts('sequence_order_with_map.dat','sequence_with_map.pdf','timeline_with_map.pdf')
plot_charts('sequence_order_with_map.dat','sequence_with_map.png','timeline_with_map.png')


