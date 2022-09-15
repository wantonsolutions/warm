import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Patch

def reads(arr):
    ret = []
    for val in arr:
        if val != 0.0:
            ret.append(val)
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
        ret.append(val/1000.0)
    return ret

def wins(clover,reads):
    for a,b in zip(clover,reads):
        try:
            win=str(a/b)
        except:
            win="1"

        print("win " + win + "x")

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
fig, ax1 = plt.subplots(1 ,1, figsize=(10,6) )

##READS
##READS
##READS
##READS

labels = [ '0', '5', '50', '100'] 
#measure (Read,Write) 99th

clover_r=[7288,70343,2558888,0]
write_r=[7107,159274,5346061,0,]
read_r=[7083,8887,8499,0]
clover_w=[0,373384,8439914,10522757]
write_w=[0,21083,37016,40251]
read_w=[0,21361,44432,41735]

# wins(clover_r,read_r)
# wins(clover_w,read_w)

clover_r=div_thousand_rw(clover_r)
write_r=div_thousand_rw(write_r)
read_r=div_thousand_rw(read_r)
clover_w=div_thousand_rw(clover_w)
write_w=div_thousand_rw(write_w)
read_w=div_thousand_rw(read_w)

wins(clover_r,read_r)
wins(clover_w,read_w)

def shuffle_x(arr,distance):
    for i in range(len(arr)):
        arr[i].xy=(arr[i].xy[0]+distance,arr[i].xy[1])
    return


x = np.arange(len(labels))  # the label locations
print(x)
width = master_width/3  # the width of the bars

div=1

widths=[x - (width * 2 + width/2),x - (width + width/2), x - width/2, x + width/2, x + (width + width/2), x + (width * 2 + width/2)]
i=0
rects_clover_r = ax1.bar(widths[i], clover_r, width, label='Clover',color=default_clover_color,edgecolor='k'); i=i+1
rects_write_r = ax1.bar(widths[i], write_r, width, label='Write',color=write_steering_color,edgecolor='k'); i=i+1
rects_read_r = ax1.bar(widths[i], read_r, width, label='Write+Read',color=read_write_steering_color,edgecolor='k'); i=i+1
rects_clover_w = ax1.bar(widths[i], clover_w, width, label='Clover',color=default_clover_color,edgecolor='k',hatch='/'); i=i+1
rects_write_w = ax1.bar(widths[i], write_w, width, label='Write',color=write_steering_color,edgecolor='k',hatch='/'); i=i+1
rects_read_w = ax1.bar(widths[i] , read_w, width, label='Write+Read',color=read_write_steering_color,edgecolor='k',hatch='/'); i=i+1

shuffle=[]

shuffle_x([rects_clover_r[0],rects_write_r[0],rects_read_r[0]],width + width/2)
shuffle_x([rects_clover_w[3],rects_write_w[3],rects_read_w[3]],-(width + width/2))



# Add some text for labels, title and custom x-axis tick labels, etc.
ax1.set_ylabel('99th percentile latency (us)')
#ax1.set_title('Read Latencies')
ax1.set_xticks(x)
ax1.set_yscale('log')
ax1.set_ylim(5,20000)
ax1.set_xticklabels(labels)
ax1.set_xlabel("Write Ratio")

p1= Patch(facecolor='white', edgecolor='k', hatch="", label="read_latency")
p2= Patch(facecolor='white', edgecolor='k', hatch="/", label='write_latency')
custom_lines = [rects_clover_r,rects_write_r,rects_read_r,p1,p2]

ax1.legend(custom_lines, ["Clover", "Write", "Write+Read", "read latency", "write latency"])
figure_name="99th_latency_dense"
plt.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')