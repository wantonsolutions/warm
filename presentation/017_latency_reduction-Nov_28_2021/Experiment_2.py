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

def plot_latency(ax,x_axis,write_latency,lat_label,lat_marker,lat_color):
    write_latency=div_thousand(write_latency)
    ax.plot(x_axis,write_latency,label=lat_label,marker=lat_marker,color=lat_color,linestyle=":")


figure_name='experiment_2'
####################### YCSB A

clover_write_latency_99=[54952.359999999986,77592.0,120160.84000000008,297009.5,1135671.879999999,2410798.590000038,4015668.9600000074,]
w_write_latency_99=[26837.0,28246.28999999998,31734.0,37910.189999999944,55086.0,73172.0,88447.0,]
rw_write_latency_99=[27610.23999999999,29857.679999999993,34668.72999999998,49665.71999999997,74531.0,113821.73999999999,147989.52000000002,]
qp_write_latency_99=[31331.33999999991,32450.5,40493.59999999998,54208.94999999995,94718.20999999996,129937.19999999972,177089.51]
cns_write_latency_99=[30411.449999999953,32686.040000000037,39398.0,52540.95999999996,84851.62000000011,128038.0,159229.02000000072,]

##cns_write_latency_99=[31541.699999999975,36530.19999999998,38286.39,47994.899999999994,62713.31999999995,80740.56000000001,143237.77999999988,]


plot_latency(ax1,labels,clover_write_latency_99,default_clover_label,default_clover_marker,default_clover_color)
plot_latency(ax1,labels,w_write_latency_99,write_steering_label,write_steering_marker,write_steering_color)
plot_latency(ax1,labels,rw_write_latency_99,read_write_steering_label,read_write_steering_marker,read_write_steering_color)
plot_latency(ax1,labels,qp_write_latency_99,qp_mapping_label,qp_mapping_marker,qp_mapping_color)
plot_latency(ax1,labels,cns_write_latency_99,cns_label,cns_marker,cns_color)

ax1.plot([],[],label="(READS)",linestyle="-",color='k')
ax1.plot([],[],label="(WRITES)",linestyle=":",color='k')

ax1.set_ylabel("us latency (99th percentile)")
ax1.set_xlabel("threads")
#ax1.set_ylim(0,5000)
ax1.set_ylim(10,5000)
ax1.set_yscale('log')



ax1.set_title('100% writes')
ax1.legend(ncol=2)
#ax1.legend()

plt.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')
#plt.show()


## Write only throughputs CNS to write
#latency=[88039,152566,246034,387889,578149,786835,982579,]                                                                                                                                                         
#threads=[2,4,8,16,32,64,112,]