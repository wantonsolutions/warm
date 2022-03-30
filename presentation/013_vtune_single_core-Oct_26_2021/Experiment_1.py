import matplotlib
import matplotlib.pyplot as plt
import numpy as np

fig, (ax1, ax2) = plt.subplots(1,2, figsize=(15,5))
#labels =  ['2', '4', '8', '16', '32', '48', '64']
labels_1 =  [2,4,8,16,32,48,64]

cns_label='Swap CNS to write'
qp_mapping_label='qp mapping (key to qp)'
read_write_steering_label='write + read steering'
write_steering_label='write steering'
default_clover_label='clover'
default_clover_label_buffer='clover with I/O buffering'

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

def static_plot_attributes(ax,xlabels,cns_replacement,qp_mapping,read_write_steering,write_steering, clover_with_buffering):
    #convert to KOPS/s 
    cns_replacement=div_thousand(cns_replacement)
    qp_mapping=div_thousand(qp_mapping)
    read_write_steering=div_thousand(read_write_steering)
    write_steering=div_thousand(write_steering)
    clover_with_buffering=div_thousand(clover_with_buffering)

    #plot
    ax.plot(xlabels[:len(cns_replacement)],cns_replacement,label=cns_label,marker=cns_marker,color=cns_color)
    ax.plot(xlabels[:len(qp_mapping)],qp_mapping,label=qp_mapping_label,marker=qp_mapping_marker,color=qp_mapping_color)
    ax.plot(xlabels[:len(read_write_steering)],read_write_steering,label=read_write_steering_label, marker=read_write_steering_marker, color=read_write_steering_color)
    ax.plot(xlabels[:len(write_steering)],write_steering,label=write_steering_label,marker=write_steering_marker,color=write_steering_color)
    ax.plot(xlabels[:len(clover_with_buffering)],clover_with_buffering,label=default_clover_label,marker=default_clover_marker, color=default_clover_color)
    ax.set_ylabel('KOP/s')
    ax.set_xlabel('Threads')


figure_name='experiment_1'
####################### YCSB A
cns_replacement_A=[28681,38360,49696,52220,57308,58549,59176]
qp_mapping_A=latency=[28931,39344,47385,53101,57817,59103,58483,]
read_write_steering_A=[43706,76840,124477,181247,217367,220592,191877]
write_steering_A=[47579,78666,127780,185813,241292,237768,196279,]
clover_with_buffering_A=[36737,65418,106296,153510,162171,145687,114447]

static_plot_attributes(ax1,labels_1,cns_replacement_A,qp_mapping_A,read_write_steering_A,write_steering_A,clover_with_buffering_A)
ax1.set_title('YCSB A NSDI (Sept 28)')
ax1.legend(loc='upper left')
ax1.set_ylim(top=1600)

####################### YCSB A (again)
labels_2 =  [2,4,8,16,24,32,40,48,56,64]
cns_replacement_B=[128196,214811,335940,478037,552502,583284,593402,591627,593849,596849]
qp_mapping_B=           [128243,211365,329255,472082,537350,570521,589971,601295,590479,559780,]
read_write_steering_B=  [137594,230379,364749,568918,700956,821977,1013141,1209651,1301119,1500390,]
write_steering_B=       [126539,208345,326054,494129,588124,645448,679955,761329,812305,826338,]
clover_with_buffering_B=[103541,167074,250447,369556,407230,426347,414296,417591,427977,424682,]

static_plot_attributes(ax2,labels_2,cns_replacement_B,qp_mapping_B,read_write_steering_B,write_steering_B,clover_with_buffering_B)
ax2.set_title('YCSB A (Nov 1)')
ax2.set_ylim(top=1600)
ax2.legend(loc='upper left')


plt.tight_layout()
#ax1.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')
plt.show()