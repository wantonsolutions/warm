import matplotlib.pyplot as plt
import numpy as np


def div_thousand (list):
    return [val /1000.0 for val in list]

cns_color='#00702eff'                     #indigo
qp_mapping_color='#9470b9ff'              #ruby
read_write_steering_color='#cf243cff'     #alice
write_steering_color='#ed7d31ff'          #kelly
default_clover_color='#8caff6ff'          #Coral 

cns_label='cns -> write'
qp_mapping_label='qp mapping'
read_write_steering_label='write + read steering'
write_steering_label='write steering'
default_clover_label='clover'


labels = ['50', '5', '100'] 
clover = [4179368.6,269359.0399999998,4015668.9600000074,]
write = [56466.0,70309.41999999998,88447.0,]
read_write = [114996.0,71872.80000000005,147989.52000000002,]
qp = [114409.4,103558.20000000001,177089.51]
cns = [112445.8,98694.56,159229.02000000072,]

clover=div_thousand(clover)
write=div_thousand(write)
read_write=div_thousand(read_write)
qp=div_thousand(qp)
cns=div_thousand(cns)


x = np.arange(len(labels))  # the label locations
width = 0.15  # the width of the bars

div=1

fig, ax = plt.subplots()
rects0 = ax.bar(x - (2*width), clover, width, label='Clover',color=default_clover_color)
rects1 = ax.bar(x - width, write, width, label='Write',color=write_steering_color)
rects2 = ax.bar(x, read_write, width, label='Read+Write',color=read_write_steering_color)
rects3 = ax.bar(x + width, qp, width, label='QP mapping',color=qp_mapping_color)
rects4 = ax.bar(x + (2*width), cns, width, label='CNS->Write',color=cns_color)


# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('99th percentile latency (us)')
ax.set_title('Write Latencies across workloads')
ax.set_xticks(x)
ax.set_xticklabels(labels)
ax.set_xlabel("Write Ratio")
ax.set_yscale('log')
ax.legend()


#ax.bar_label(rects1, padding=3)
#//ax.bar_label(rects2, padding=3)
figure_name="summary_write"
plt.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')