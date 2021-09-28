import matplotlib
import matplotlib.pyplot as plt
import numpy as np

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


def div_thousand (list):
    return [val /1000.0 for val in list]

def static_plot_attributes(ax,cns_replacement,qp_mapping,read_write_steering,write_steering, clover_with_buffering):
    #convert to KOPS/s 
    cns_replacement=div_thousand(cns_replacement)
    qp_mapping=div_thousand(qp_mapping)
    read_write_steering=div_thousand(read_write_steering)
    write_steering=div_thousand(write_steering)
    clover_with_buffering=div_thousand(clover_with_buffering)

    #plot
    ax.plot(labels,cns_replacement,label=cns_label,marker=cns_marker,color=cns_color)
    ax.plot(labels,qp_mapping,label=qp_mapping_label,marker=qp_mapping_marker,color=qp_mapping_color)
    ax.plot(labels,read_write_steering,label=read_write_steering_label, marker=read_write_steering_marker, color=read_write_steering_color)
    ax.plot(labels,write_steering,label=write_steering_label,marker=write_steering_marker,color=write_steering_color)
    ax.plot(labels,clover_with_buffering,label=default_clover_label,marker=default_clover_marker, color=default_clover_color)
    ax.set_ylabel('KOP/s')
    ax.set_xlabel('Threads')


figure_name='experiment_1'
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
cns_replacement_B=[x * 1000 for x in range(1, 8)]
qp_mapping_B=[x * 2000 for x in range(1, 8)]
read_write_steering_B=[x * 3000 for x in range(1, 8)]
write_steering_B=[x * 4000 for x in range(1, 8)]
clover_with_buffering_B=[x * 5000 for x in range(1, 8)]

static_plot_attributes(ax2,cns_replacement_B,qp_mapping_B,read_write_steering_B,write_steering_B,clover_with_buffering_B)
ax2.set_title('YCSB B (95% Read 5% write)')

####################### Write Only
cns_replacement_W=[x * 1000 for x in range(1, 8)]
qp_mapping_W=[x * 2000 for x in range(1, 8)]
read_write_steering_W=[x * 3000 for x in range(1, 8)]
write_steering_W=[x * 4000 for x in range(1, 8)]
clover_with_buffering_W=[x * 5000 for x in range(1, 8)]

static_plot_attributes(ax3,cns_replacement_W,qp_mapping_W,read_write_steering_W,write_steering_W,clover_with_buffering_W)
ax3.set_title('Write Only')

#ax1.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')
plt.show()