import matplotlib.pyplot as plt
import numpy as np

def reads(arr):
    ret = []
    for val in arr:
        if val[0] != 0.0:
            ret.append(val[0])
    return ret

def writes(arr):
    ret = []
    for val in arr:
        if val[1] != 0.0:
            ret.append(val[1])
    return ret


def div_thousand_rw (list):
    ret = []
    for val in list:
        a = val[0]/1000.0
        b = val[1]/1000.0
        ret.append((a,b))
    return ret

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
fig, (ax1, ax2) = plt.subplots(1 ,2, figsize=(15,5), gridspec_kw={'width_ratios': [2,2]})

##READS
##READS
##READS
##READS

labels = [ '0', '5', '50'] 
#measure (Read,Write) 99th
clover=[(7288,0),(70343,373384),(2558888,8439914),(0,10522757)]
write=[(7107,0),(159274,21083),(5346061,37016),(0,40251)]
read=[(7083,0),(8887,21361),(8499,44432),(0, 41735)]


clover=div_thousand_rw(clover)
write=div_thousand_rw(write)
read=div_thousand_rw(read)

print("Reads")
print(reads(clover))
print(reads(write))
print(reads(read))
print()

print(writes(clover))
print(writes(write))
print(writes(read))



x = np.arange(len(labels))  # the label locations
width = master_width/3  # the width of the bars

div=1

rects0 = ax1.bar(x - width, reads(clover), width, label='Clover',color=default_clover_color,edgecolor='k')
rects1 = ax1.bar(x, reads(write), width, label='Write',color=write_steering_color,edgecolor='k')
rects2 = ax1.bar(x + width , reads(read), width, label='Write+Read',color=read_write_steering_color,edgecolor='k')


# Add some text for labels, title and custom x-axis tick labels, etc.
ax1.set_ylabel('99th percentile latency (us)')
#ax1.set_title('Read Latencies')
ax1.set_xticks(x)
ax1.set_yscale('log')
ax1.set_ylim(5,20000)
ax1.set_xticklabels(labels)
ax1.set_xlabel("Write Ratio")
ax1.legend()



labels = ['5', '50', '100'] 



x = np.arange(len(labels))  # the label locations
width = master_width/3  # the width of the bars
div=1

rects0 = ax2.bar(x - width, writes(clover), width, label='Clover',color=default_clover_color,edgecolor='k')
rects1 = ax2.bar(x, writes(write), width, label='Write',color=write_steering_color,edgecolor='k')
rects2 = ax2.bar(x + width , writes(read), width, label='Write+Read',color=read_write_steering_color,edgecolor='k')

#ax2.set_ylabel('99th percentile latency (us)')
#ax2.set_title('Write Latencies')
ax2.set_xticks(x)
ax2.set_xticklabels(labels)
ax2.set_yscale('log')
ax2.set_ylim(5,20000)
ax2.set_xlabel("Write Ratio")

#//ax.bar_label(rects2, padding=3)
figure_name="99th_latency"
plt.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')