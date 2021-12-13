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

master_width=0.45

#fig, (ax1, ax2) = plt.subplots(1,2, figsize=(15,5))
plt.rcParams.update({'font.size': 16})
fig, (ax1, ax2) = plt.subplots(1 ,2, figsize=(15,5), gridspec_kw={'width_ratios': [2,3]})

##READS
##READS
##READS
##READS

labels = [ '5', '50',] 
clover = [91306.62999999989,115577.12]
write = [ 112736.24000000022,1541449.1100000017,]
read_write = [38589.0,57773.0,]
qp = [54734.0,62902.0199999999,]
cns = [52214.0,63986.14000000013,]

clover=div_thousand(clover)
write=div_thousand(write)
read_write=div_thousand(read_write)
qp=div_thousand(qp)
cns=div_thousand(cns)


x = np.arange(len(labels))  # the label locations
width = master_width/4  # the width of the bars

div=1

rects0 = ax1.bar(x - (2*width), clover, width, label='Clover',color=default_clover_color)
rects1 = ax1.bar(x - width, write, width, label='Write',color=write_steering_color)
rects2 = ax1.bar(x, read_write, width, label='Read+Write',color=read_write_steering_color)
rects3 = ax1.bar(x + width, qp, width, label='QP mapping',color=qp_mapping_color)
rects4 = ax1.bar(x + (2*width), cns, width, label='CAS->Write',color=cns_color)


# Add some text for labels, title and custom x-axis tick labels, etc.
ax1.set_ylabel('99th percentile latency (us)')
#ax1.set_title('Read Latencies')
ax1.set_xticks(x)
ax1.set_xticklabels(labels)
ax1.set_xlabel("Write Ratio")
ax1.legend()



labels = ['5', '50', '100'] 
clover = [269359.0399999998,4179368.6,4015668.9600000074,]
write = [70309.41999999998,56466.0,88447.0,]
read_write = [71872.80000000005,114996.0,147989.52000000002,]
qp = [103558.20000000001,114409.4,177089.51]
cns = [98694.56,112445.8,159229.02000000072,]

clover=div_thousand(clover)
write=div_thousand(write)
read_write=div_thousand(read_write)
qp=div_thousand(qp)
cns=div_thousand(cns)


x = np.arange(len(labels))  # the label locations
width = master_width/3  # the width of the bars
div=1

rects0 = ax2.bar(x - (2*width), clover, width, label='Clover',color=default_clover_color)
rects1 = ax2.bar(x - width, write, width, label='Write',color=write_steering_color)
rects2 = ax2.bar(x, read_write, width, label='Read+Write',color=read_write_steering_color)
rects3 = ax2.bar(x + width, qp, width, label='QP mapping',color=qp_mapping_color)
rects4 = ax2.bar(x + (2*width), cns, width, label='CAS->Write',color=cns_color)

#ax2.set_ylabel('99th percentile latency (us)')
#ax2.set_title('Write Latencies')
ax2.set_xticks(x)
ax2.set_xticklabels(labels)
ax2.set_xlabel("Write Ratio")

#//ax.bar_label(rects2, padding=3)
figure_name="99th_latency"
plt.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')