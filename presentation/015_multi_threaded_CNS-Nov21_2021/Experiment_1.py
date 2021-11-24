import matplotlib
import matplotlib.pyplot as plt
import numpy as np

#fig, (ax1, ax2, ax3) = plt.subplots(1,3, figsize=(20,5))
fig, ax1 = plt.subplots(1,1, figsize=(6,5))
#labels =  ['2', '4', '8', '16', '32', '48', '64']
#labels =  [2,4,8,16,24,32,40,48,56,64]
labels=[1,2,4,8,16,24,32,40,48,56,64,]

#labels=[2,4,8,16,32,48,64,80,96,112,128,]

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
    #ax.plot(labels[:len(clover_with_buffering)],clover_with_buffering,label=default_clover_label,marker=default_clover_marker, color=default_clover_color)
    #ax.plot(labels[:len(write_steering)],write_steering,label=write_steering_label,marker=write_steering_marker,color=write_steering_color)
    ax.plot(labels[:len(read_write_steering)],read_write_steering,label=read_write_steering_label, marker=read_write_steering_marker, color=read_write_steering_color)
    ax.plot(labels[:len(qp_mapping)],qp_mapping,label=qp_mapping_label,marker=qp_mapping_marker,color=qp_mapping_color)
    ax.plot(labels[:len(cns_replacement)],cns_replacement,label=cns_label,marker=cns_marker,color=cns_color)
    ax.set_ylabel('KOP/s')
    ax.set_xlabel('Threads')


figure_name='experiment_1'
####################### YCSB A

cns_replacement_A=[66821,122363,223457,383481,596730,725380,841559,918889,988483,1063526,1097760,]
qp_mapping_A=[69553,142873,224073,382494,609152,719682,841559,927204,1019899,1067026,1097760,]
read_write_steering_A=[81242,143978,262748,465466,730418,892252,1060707,1192438,1303253,1414516,1494687,]
write_steering_A=[0,0,0,0,0,0,0,0,0,0,]
clover_with_buffering_A=[0,0,0,0,0,0,0,0,0,0,]


static_plot_attributes(ax1,cns_replacement_A,qp_mapping_A,read_write_steering_A,write_steering_A,clover_with_buffering_A)
ax1.set_title('YCSB A (50% Read 50% write)')
ax1.legend(loc='upper left')

plt.tight_layout()
#ax1.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')
#plt.show()


## Write only throughputs CNS to write
#latency=[88039,152566,246034,387889,578149,786835,982579,]                                                                                                                                                         
#threads=[2,4,8,16,32,64,112,]