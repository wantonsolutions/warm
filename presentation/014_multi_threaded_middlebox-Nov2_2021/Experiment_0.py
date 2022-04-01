import matplotlib
import matplotlib.pyplot as plt
import numpy as np

#fig, (ax1, ax2, ax3) = plt.subplots(1,3, figsize=(20,5))
fig, ax1 = plt.subplots(1,1, figsize=(6,5))
#labels =  ['2', '4', '8', '16', '32', '48', '64']
labels=[0,16,32,48,64,80,96,112,128,]

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
    ax.plot(labels[:len(cns_replacement)],cns_replacement,label=cns_label,marker=cns_marker,color=cns_color)
    ax.plot(labels[:len(qp_mapping)],qp_mapping,label=qp_mapping_label,marker=qp_mapping_marker,color=qp_mapping_color)
    ax.plot(labels[:len(read_write_steering)],read_write_steering,label=read_write_steering_label, marker=read_write_steering_marker, color=read_write_steering_color)
    ax.plot(labels[:len(write_steering)],write_steering,label=write_steering_label,marker=write_steering_marker,color=write_steering_color)
    ax.plot(labels[:len(clover_with_buffering)],clover_with_buffering,label=default_clover_label,marker=default_clover_marker, color=default_clover_color)
    ax.set_ylabel('KOP/s')
    ax.set_xlabel('Threads')


figure_name='experiment_0'
####################### YCSB A

read_write_steering_1=  [0,567967,825611,1182964,1471151,1598630,1659460,1657795,1677467,]
read_write_steering_2=  [0,594913,896054,1127376,1294297,1550470,1756694,1929098,2041826,]
read_write_steering_3=  [0,635318,966227,1198899,1335537,1482912,1636524,1770967,1853767,]
read_write_steering_4=  [0,644458,970065,1229660,1414592,1560351,1650448,1787662,1807445,]
read_write_steering_8=  [0,639140,1021502,1231970,1436671,1572453,1688444,1758211,1815025,]
read_write_steering_2_rx_tinker_1=[0,511475,818652,1258440,1599803,1828006,1878165,2158064,2346464,]

ax1.plot(labels,div_thousand(read_write_steering_1),label="1")
ax1.plot(labels,div_thousand(read_write_steering_2),label="2")
ax1.plot(labels,div_thousand(read_write_steering_3),label="3")
ax1.plot(labels,div_thousand(read_write_steering_4),label="4")
ax1.plot(labels,div_thousand(read_write_steering_8),label="8")
ax1.plot(labels,div_thousand(read_write_steering_2_rx_tinker_1),label="1 - rx tinker")

#static_plot_attributes(ax1,)
ax1.set_ylabel('KOP/s')
ax1.set_xlabel('Client Threads')
ax1.set_title('Middle Box Core Counts - YCSB A read write steering')
ax1.set_ylim(0,2500)
ax1.legend(loc='upper left')

plt.tight_layout()
#ax1.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')
#plt.show()