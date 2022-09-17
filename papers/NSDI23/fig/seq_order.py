import numpy as np
import matplotlib.pyplot as plt
import matplotlib
import csv
from matplotlib.pyplot import figure

qp_mapping_color='#9470b9ff' 
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


def plot_charts(source_filename, sequence_output_filename):
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

    gap=[ a * -1 if a < 0 else a for a in gap ] 

    plt.rcParams.update({'font.size': 16})
    fig, axs = plt.subplots(1,1, figsize=(8,4))
    v = axs.hist(gap, color=qp_mapping_color, density=True,bins=15, edgecolor='black')
    print(v) #ordered req

    axs.set_ylabel("probability")
    axs.set_yscale('log')
    axs.set_xlabel("sequence number delta")
    axs.set_xlim(0,15)
    tick_val = range(1,14,2)
    ticks_loc = [x + 0.5 for x in tick_val]
    tick_val = [str(x) for x in tick_val]
    axs.set_xticks(ticks_loc)#,ticks_loc)
    axs.set_xticklabels(tick_val)
    #plt.plot(x,y, label="sequence_steps")
    plt.tight_layout()
    plt.savefig(sequence_output_filename)

plot_charts('sequence_order_with_map.dat','qp_reordering.pdf')
#plot_charts('sequence_order_with_map.dat','sequence_with_map.png','timeline_with_map.png')


