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


figure_name='experiment_1'
####################### YCSB A

clover_read_latency_99=[19389.719999999972,22001.55999999994,31270.0,42743.0,60783.0,76008.0,91306.62999999989,]
clover_write_latency_99=[42168.63999999999,48706.84999999999,60006.15,80947.20000000004,129807.62000000005,205353.46999999986,269359.0399999998,]

w_read_latency_99=[19422.0,22383.939999999944,30044.62999999989,42944.1399999999,64026.87999999989,88678.02000000002,112736.24000000022,]
w_write_latency_99=[26173.659999999993,26463.479999999996,28028.379999999932,37738.96000000002,50811.369999999995,60374.31999999999,70309.41999999998,]

rw_read_latency_99=[12683.73999999999,13181.0,15526.0,19385.820000000065,26611.0,33716.0,38589.0,]
rw_write_latency_99=[27157.949999999997,29970.24000000002,32572.399999999987,39711.729999999996,52599.640000000014,63940.96000000002,71872.80000000005,]

qp_read_latency_99=[14130.0,16227.309999999998,18794.0,25371.0,34414.0,45024.0,54734.0,]                                                                                                                                                                                 
qp_write_latency_99=[30105.84,33458.83,37434.65000000001,48074.64,65776.6,85561.45,103558.20000000001,]


cns_read_latency_99=[14437.820000000007,17952.0,19059.920000000042,24816.0,32879.0,42380.0,52214.0,]
cns_write_latency_99=[31541.699999999975,36530.19999999998,38286.39,47994.899999999994,62713.31999999995,80740.56000000001,98694.56]

#137008.52999999997 


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



ax1.set_title('YCSB B (95% Read 5% write)')
ax1.legend(ncol=2)
#ax1.legend()

plt.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')
#plt.show()


## Write only throughputs CNS to write
#latency=[88039,152566,246034,387889,578149,786835,982579,]                                                                                                                                                         
#threads=[2,4,8,16,32,64,112,]