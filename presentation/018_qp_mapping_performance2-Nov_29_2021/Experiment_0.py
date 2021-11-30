import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from scipy.stats import sem
from statistics import mean

#fig, (ax1, ax2, ax3) = plt.subplots(1,3, figsize=(20,5))
fig, ax1 = plt.subplots(1,1, figsize=(6,5))
#labels =  ['2', '4', '8', '16', '32', '48', '64']

labels=[2,4,8,16,32,48,64]

cns_label='cns -> write'
qp_mapping_label='qp mapping'
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

def plot_qp(ax,x_axis,ops,ops_label,ops_marker,ops_color):
    ops=div_thousand(ops)
    ax.plot(x_axis,ops,label=ops_label,marker=ops_marker,color=ops_color)

def plot_qp_error(ax,x_axis,ops,ops_label,ops_marker,ops_color):
    ops_mean=[]
    ops_error=[]
    for op in ops:
        ops_mean.append(mean(op))
        ops_error.append(sem(op))

    ops_mean=div_thousand(ops_mean)
    ops_error=div_thousand(ops_error)
    ax.errorbar(x_axis,ops_mean,ops_error,label=ops_label,marker=ops_marker,color=ops_color)        



figure_name='experiment_0'
####################### YCSB A
#qp=[1,2,4,8,16,32,40]
#qp=[1,2,4,8]
qp=[1]
cns=[]
cns_1=[525073,515478,499577,562232,481150,]
cns.append(cns_1)

plot_qp_error(ax1,qp,cns,cns_label,cns_marker,cns_color)


#cns=[554527,631871,635481,664461]
#qp_map=[555819,614756,628769,682127]

#plot_qp(ax1,qp,cns,cns_label,cns_marker,cns_color)
#plot_qp(ax1,qp,qp_map,qp_mapping_label,qp_mapping_marker,qp_mapping_color)

ax1.set_ylabel("Average KOPS")
ax1.set_xlabel("Destination Queue Pairs")
#ax1.set_ylim(0,5000)
#ax1.set_ylim(10,5000)



ax1.set_title('100% writes, 40 threads, qp bandwidth measure')
ax1.legend(ncol=2)
#ax1.legend()

plt.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')
#plt.show()


## Write only throughputs CNS to write
#latency=[88039,152566,246034,387889,578149,786835,982579,]                                                                                                                                                         
#threads=[2,4,8,16,32,64,112,]