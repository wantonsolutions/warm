import matplotlib
import matplotlib.pyplot as plt
import numpy as np

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

def plot_latency(ax,x_axis,read_latency,write_latency,lat_label,lat_marker,lat_color):
    read_latency=div_thousand(read_latency)
    write_latency=div_thousand(write_latency)
    ax.plot(x_axis,read_latency,label=lat_label,marker=lat_marker,color=lat_color)
    ax.plot(x_axis,write_latency,marker=lat_marker,color=lat_color,linestyle=":")


figure_name='experiment_0'
####################### YCSB A

clover_read_latency_99=[27464.600000000006,33701.0,48648.65000000002,71549.0,96962.6000000001,108803.29000000021,115577.12,]
clover_write_latency_99=[50177.27999999997,66517.73000000001,114174.15999999997,256705.4299999994,1323364.3900000043,3191729.9199999995,4179368.660000005,]


w_read_latency_99=[28011.0,39351.649999999965,60693.669999999984,117409.37999999989,343008.55000000005,869271.6700000011,1541449.1100000017,]
w_write_latency_99=[25058.0,30303.139999999985,34556.630000000005,39422.0,52627.0,58604.0,56466.0,]

rw_read_latency_99=[13681.0,16191.0,19178.97999999998,25069.0,36330.0,45771.0,57773.0,]
rw_write_latency_99=[28400.320000000007,30732.019999999902,35608.0,46955.21999999997,69287.64000000001,89968.0,114996.03000000003,]

qp_read_latency_99=[15477.079999999987,17881.349999999977,20083.96000000002,25516.0,36513.0,49899.189999999944,62902.0199999999,]
qp_write_latency_99=[30579.0,33935.34,38498.07999999996,47976.0,70012.35999999999,94608.69999999995,114409.45999999996,]

cns_read_latency_99=[15049.139999999985,17793.0,19979.01000000001,27786.0,37616.0,53392.0,63986.14000000013,]
cns_write_latency_99=[30386.01999999999,33981.600000000006,37924.439999999944,51311.0,69892.56999999995,96880.0,112445.84000000008,]


plot_latency(ax1,labels,clover_read_latency_99,clover_write_latency_99,default_clover_label,default_clover_marker,default_clover_color)
plot_latency(ax1,labels,w_read_latency_99,w_write_latency_99,write_steering_label,write_steering_marker,write_steering_color)
plot_latency(ax1,labels,rw_read_latency_99,rw_write_latency_99,read_write_steering_label,read_write_steering_marker,read_write_steering_color)
plot_latency(ax1,labels,qp_read_latency_99,qp_write_latency_99,qp_mapping_label,qp_mapping_marker,qp_mapping_color)
plot_latency(ax1,labels,cns_read_latency_99,cns_write_latency_99,cns_label,cns_marker,cns_color)

ax1.plot([],[],label="(READS)",linestyle="-",color='k')
ax1.plot([],[],label="(WRITES)",linestyle=":",color='k')

ax1.set_ylabel("us latency (99th percentile)")
ax1.set_xlabel("threads")
#ax1.set_ylim(0,5000)
ax1.set_ylim(10,5000)
ax1.set_yscale('log')



ax1.set_title('YCSB A (50% Read 50% write)')
ax1.legend(ncol=2)
#ax1.legend()

plt.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')
#plt.show()


## Write only throughputs CNS to write
#latency=[88039,152566,246034,387889,578149,786835,982579,]                                                                                                                                                         
#threads=[2,4,8,16,32,64,112,]