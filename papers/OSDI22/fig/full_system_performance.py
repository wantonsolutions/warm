import matplotlib
import matplotlib.pyplot as plt
import numpy as np

from common_library import *

fig, (ax1, ax2, ax3) = plt.subplots(1,3, figsize=(20,5))
#labels =  ['2', '4', '8', '16', '32', '48', '64']
labels =  [2,4,8,16,32,48,64]

cns_label='Swap CNS to write'
qp_mapping_label='qp mapping (key to qp)'
read_write_steering_label='write + read steering'
write_steering_label='write steering'
default_clover_label='clover'

cns_marker='x'
qp_mapping_marker='s'
read_write_steering_marker='+'
write_steering_marker="*"
default_clover_marker='.'

#https://s3.amazonaws.com/viget.com/legacy/uploads/image/blog/cc-link-1.png
cns_color='#0D3D56'                     #indigo
qp_mapping_color='#C02F1D'              #ruby
read_write_steering_color='#1496BB'     #alice
write_steering_color='#A3B86C'          #kelly
default_clover_color='#F26D21'          #Coral 

cns_color='#00702eff'                     #indigo
qp_mapping_color='#9470b9ff'              #ruby
read_write_steering_color='#cf243cff'     #alice
write_steering_color='#ed7d31ff'          #kelly
default_clover_color='#8caff6ff'          #Coral 

def static_plot_attributes(ax,cns_replacement,qp_mapping,read_write_steering,write_steering, clover_with_buffering):
    #convert to KOPS/s 
    cns_replacement=div_thousand(cns_replacement)
    qp_mapping=div_thousand(qp_mapping)
    read_write_steering=div_thousand(read_write_steering)
    write_steering=div_thousand(write_steering)
    clover_with_buffering=div_thousand(clover_with_buffering)

    #plot
    ax.plot(labels[:len(cns_replacement)],cns_replacement,label=cns_label,marker=cns_marker,color=cns_color)
    ax.plot(labels[:len(qp_mapping)],qp_mapping,label=qp_mapping_label,marker=qp_mapping_marker,color=qp_mapping_color)
    ax.plot(labels[:len(read_write_steering)],read_write_steering,label=read_write_steering_label, marker=read_write_steering_marker, color=read_write_steering_color)
    ax.plot(labels[:len(write_steering)],write_steering,label=write_steering_label,marker=write_steering_marker,color=write_steering_color)
    ax.plot(labels[:len(clover_with_buffering)],clover_with_buffering,label=default_clover_label,marker=default_clover_marker, color=default_clover_color)
    ax.set_ylabel('KOP/s')
    ax.set_xlabel('Threads')

####################### YCSB A
cns_replacement_A=[28681,38360,49696,52220,57308,58549,59176]
qp_mapping_A=latency=[28931,39344,47385,53101,57817,59103,58483,]
read_write_steering_A=[43706,76840,124477,181247,217367,220592,191877]
write_steering_A=[47579,78666,127780,185813,241292,237768,196279,]
clover_with_buffering_A=[36737,65418,106296,153510,162171,145687,114447]

static_plot_attributes(ax1,cns_replacement_A,qp_mapping_A,read_write_steering_A,write_steering_A,clover_with_buffering_A)
ax1.set_title('YCSB A (50% Read 50% write)')
ax1.legend(loc='upper left')
ax1.set_ylim(top=350)

####################### YCSB B
cns_replacement_B=[40201,54344,65534,69220]
qp_mapping_B=[0]
read_write_steering_B=[70161,135937,232888,357780,457676,481389,466011]
write_steering_B=[76981,131557,237639,362646,478791,486266,477505]
clover_with_buffering_B=[76231,140369,225298,374574,482870,483032,469943]


static_plot_attributes(ax2,cns_replacement_B,qp_mapping_B,read_write_steering_B,write_steering_B,clover_with_buffering_B)
ax2.set_title('YCSB B (95% Read 5% write)')

####################### Write Only
cns_replacement_W=[21466,29982,43185,63399,71201]
qp_mapping_W=[21321,30143,43783,64288,71658,53677,55691]
read_write_steering_W=[27362,60562,102853,150550,178756,216403,233423,]
write_steering_W=[35134,61289,100051,158963,207892,242373,238822]
clover_with_buffering_W=[22415,41149,68630,93214,91383,78164,64616]

static_plot_attributes(ax3,cns_replacement_W,qp_mapping_W,read_write_steering_W,write_steering_W,clover_with_buffering_W)
ax3.set_title('Write Only')

plt.tight_layout()
#ax1.tight_layout()
save(plt)
plt.show()