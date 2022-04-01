import matplotlib.pyplot as plt
import numpy as np


def div_thousand (list):
    return [val /1000.0 for val in list]




cns_color='#00702eff'                     #indigo
qp_mapping_color='#9470b9ff'              #ruby
read_write_steering_color='#cf243cff'     #alice
write_steering_color='#ed7d31ff'          #kelly
default_clover_color='#8caff6ff'          #Coral 

read_write_steering_label='In-Network'
default_clover_label='End-to-End'

master_width=0.45

plt.rcParams.update({'font.size': 16})
fig, (ax1, ax2, ax3) = plt.subplots(1 ,3, figsize=(15,5), gridspec_kw={'width_ratios': [1,1,1]})

# Add some text for labels, title and custom x-axis tick labels, etc.
labels = [''] 
clover = [4179368.6]
read_write = [114996.0]

clover=div_thousand(clover)
read_write=div_thousand(read_write)

x = np.arange(len(labels))  # the label locations
width = master_width/2  # the width of the bars
div=1

rects0 = ax1.bar(x-(width/2), clover, width, label=default_clover_label,color=default_clover_color,edgecolor='k')
rects1 = ax1.bar(x+(width/2), read_write, width, label=read_write_steering_label,color=read_write_steering_color,edgecolor='k')

ax1.set_ylabel('99th percentile latency (us)')
#ax2.set_title('Write Latencies')
ax1.set_xticks(x)
ax1.set_xticklabels(labels)
ax1.set_yscale('log')
ax1.set_ylim(20,20000)
ax1.set_title("Write Tail Latency")
#ax1.set_xlabel("Write Tail Latency")
ax1.legend()

labels = [''] 
read_write = [1300]
clover = [2457]


x = np.arange(len(labels))  # the label locations
width = master_width/2  # the width of the bars
div=1

rects0 = ax2.bar(x-(width/2), clover, width, label=default_clover_label,color=default_clover_color,edgecolor='k')
rects1 = ax2.bar(x+(width/2), read_write, width, label=read_write_steering_label,color=read_write_steering_color,edgecolor='k')

ax2.set_ylabel('Bytes')
#ax2.set_title('Write Latencies')
ax2.set_xticks(x)
ax2.set_xticklabels(labels)
ax2.set_title("Bandwidth Per OP (avg)")

labels = [''] 

read_write = [1543334]
clover = [545840]
clover=div_thousand(clover)
read_write=div_thousand(read_write)


x = np.arange(len(labels))  # the label locations
width = master_width/2  # the width of the bars
div=1

rects0 = ax3.bar(x-(width/2), clover, width, label=default_clover_label,color=default_clover_color,edgecolor='k')
rects1 = ax3.bar(x+(width/2), read_write, width, label=read_write_steering_label,color=read_write_steering_color,edgecolor='k')

ax3.set_ylabel('KOPS')
#ax2.set_title('Write Latencies')
ax3.set_xticks(x)
ax3.set_xticklabels(labels)
ax3.set_title("Throughput")

#//ax.bar_label(rects2, padding=3)
figure_name="master_plot"
plt.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')