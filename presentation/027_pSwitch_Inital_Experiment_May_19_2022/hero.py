import matplotlib
import matplotlib.pyplot as plt
from statistics import mean
import numpy as np

fig, (ax1, ax2, ax3, ax4) = plt.subplots(1,4, figsize=(20,4))
#labels =  ['2', '4', '8', '16', '32', '48', '64']
labels =  [1,2,4,8,16,24,32]

cns_label='CAS -> Write'
qp_mapping_label='QP mapping'
read_write_steering_label='Write + Read'
write_steering_label='Write'
default_clover_label='Clover'

cns_marker='x'
qp_mapping_marker='s'
read_write_steering_marker='+'
write_steering_marker="*"
default_clover_marker='.'

cns_color='#00702eff'                     #indigo
qp_mapping_color='#9470b9ff'              #ruby
read_write_steering_color='#cf243cff'     #alice
write_steering_color='#ed7d31ff'          #kelly
default_clover_color='#8caff6ff'          #Coral 


def div_thousand (list):
    return [val /1000.0 for val in list]

def static_plot_attributes(ax,read_write_steering,write_steering, clover_with_buffering):
    #convert to KOPS/s 
    read_write_steering=div_thousand(read_write_steering)
    write_steering=div_thousand(write_steering)
    clover_with_buffering=div_thousand(clover_with_buffering)

    #plot
    ax.plot(labels[:len(clover_with_buffering)],clover_with_buffering,label=default_clover_label,marker=default_clover_marker, color=default_clover_color)
    ax.plot(labels[:len(write_steering)],write_steering,label=write_steering_label,marker=write_steering_marker,color=write_steering_color)
    ax.plot(labels[:len(read_write_steering)],read_write_steering,label=read_write_steering_label, marker=read_write_steering_marker, color=read_write_steering_color)


figure_name='hero'

#[186907,362862,678406,1135524,1753723,2058595,2138879]


####################### YCSB C
read_write_steering_C=[263773.500000,500941.000000,940054.500000,1577086.500000,3192490.000000,4147898.500000,4150532.250000]
write_steering_C=[264792,506349,942163,1559834,3228172,4147783,4150481,]
clover_with_buffering_C=[280203,529865,963919,1619481,3293370,4147754,4150900]
static_plot_attributes(ax1,read_write_steering_C,write_steering_C,clover_with_buffering_C)
ax1.set_title('0% Writes')
ax1.set_ylabel('KOPS')
ax1.legend(loc='upper left', ncol=2)
#ax1.set_ylim(top=350)


####################### YCSB B
read_write_steering_B=[249726.500000,478003.500000,920228.500000,1653355.000000,3122626.500000,4189691.500000,4255242.500000]
write_steering_B=[177638,484289,925218,1648351,3077692,4036921,4016480,]
clover_with_buffering_B=[186907,502152,956731,1696762,3157133,4050294,4021827]
static_plot_attributes(ax2,read_write_steering_B,write_steering_B,clover_with_buffering_B)
ax2.set_title('5% Writes')
ax2.set_ylabel('KOPS')
ax2.legend(loc='upper left', ncol=2)
#ax1.set_ylim(top=350)


####################### YCSB A
read_write_steering_A=[175829.000000,348741.000000,675890.000000,1280155.000000,2336774.000000,3154041.000000,3263446.500000]
write_steering_A=[178300.593750,349755.000000,677746.562500,1231525.625000,2127434.500000,2622872.000000,2858032.000000]
clover_with_buffering_A=[187475.796875,363553.093750,679143.000000,1147633.500000,1772697.625000,2052135.500000,2140299.000000]
ax3.set_title('50% Writes')
ax3.set_xlabel('Threads')
static_plot_attributes(ax3,read_write_steering_A,write_steering_A,clover_with_buffering_A)

####################### Write Only
read_write_steering_W=[137878,263420.500000,508887.500000,973879.500000,]
write_steering_W=[137878,266046,514048,981434,1783051,]
clover_with_buffering_W=[144640,268338,473576,773097,1046435,1068856,1154510,]
static_plot_attributes(ax4,read_write_steering_W,write_steering_W,clover_with_buffering_W)
ax4.set_title('100% Writes')

plt.tight_layout()
#ax1.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')
#plt.show()