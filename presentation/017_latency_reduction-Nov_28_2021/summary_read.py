import matplotlib.pyplot as plt
import numpy as np

def div_thousand (list):
    return [val /1000.0 for val in list]


cns_color='#00702eff'                     #indigo
qp_mapping_color='#9470b9ff'              #ruby
read_write_steering_color='#cf243cff'     #alice
write_steering_color='#ed7d31ff'          #kelly
default_clover_color='#8caff6ff'          #Coral 

labels = ['50', '5'] 
clover = [115577.12,91306.62999999989,]
write = [1541449.1100000017, 112736.24000000022,]
read_write = [57773.0,38589.0]
qp = [62902.0199999999,54734.0,]
cns = [63986.14000000013,52214.0,]

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
ax.set_title('Read Latencies across workloads')
ax.set_xticks(x)
ax.set_xticklabels(labels)
ax.set_xlabel("Write Ratio")
ax.set_yscale("log")
ax.legend()


#ax.bar_label(rects1, padding=3)
#//ax.bar_label(rects2, padding=3)
figure_name="summary_read"
plt.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')