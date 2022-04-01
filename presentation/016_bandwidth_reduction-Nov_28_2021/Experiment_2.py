import matplotlib
import matplotlib.pyplot as plt
import numpy as np

#fig, (ax1, ax2, ax3) = plt.subplots(1,3, figsize=(20,5))
fig, ax1 = plt.subplots(1,1, figsize=(6,5))
#labels =  ['2', '4', '8', '16', '32', '48', '64']




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


cns_color='#00702eff'                     #indigo
qp_mapping_color='#9470b9ff'              #ruby
read_write_steering_color='#cf243cff'     #alice
write_steering_color='#ed7d31ff'          #kelly
default_clover_color='#8caff6ff'          #Coral 


def div_thousand (list):
    return [val /1000.0 for val in list]

def ops_byte(ops,bytes):

    #print("Correcting for the bytes used to populate the kv store")
    #keys = 1024
    #write_size=62.0 + 1024.0
    #ack_size=48.0
    #population_overhead=keys*(write_size+ack_size)
    #print("population overhead = ", str(population_overhead))
    #bytes=[byte - population_overhead for byte in bytes]
    return [(byte/op) for op, byte in zip(ops,bytes)]


def plot_bandwidth(ax,threads,ops,bandwidth,this_label,this_marker,this_color):
    opb=ops_byte(ops,bandwidth)
    print("label " + str(this_label) + "ops " + str(opb))
    ax.plot(threads,opb,label=this_label,marker=this_marker,color=this_color)

def plot_optimial(ax,write_percentage,threads):
    payload_size=1024.0
    write_header_size=62.0
    read_header_size=48.0
    shortcut_size=8.0

    cns_size=72.0
    ack_size=48.0
    atomic_ack_size=56.0
    read_request_size=60.0

    write_size=payload_size+write_header_size
    shortcut_write_size=shortcut_size+write_header_size
    read_response_size=payload_size+read_header_size
    short_cut_response_size=shortcut_size+read_header_size

    read_percentage=100.0-write_percentage

    optimal_write=write_size+ack_size+cns_size+atomic_ack_size+shortcut_write_size+ack_size
    print("optimal write: " + str(optimal_write))
    optimal_read=read_response_size+read_request_size+short_cut_response_size+read_request_size
    print("optimal read: " + str(optimal_read))

    optimal=(write_percentage*optimal_write + read_percentage*optimal_read)/100.0

    print(optimal)
    #ax.hlines(y=optimal, color='r', linestyle='-',xmin=threads[0],xmax=threads[:len(threads)-1])
    ax.hlines(y=optimal, color='g', linestyle=':',linewidth=3, xmin=0,xmax=64, label="calculated optimal")





figure_name='experiment_2'
####################### YCSB A



rw_ops=[226739,376972,630662,969612,1411790,1698382,2156998,]
rw_threads=[2,4,8,16,32,48,64,]
rw_bandwidth=[312172444,516755248,860235552,1316411404,1911376212,2289550536,2898627336,]

w_ops=[232688,399525,651162,1019030,1507304,1879767,2455725,]
w_threads=[2,4,8,16,32,48,64,]
w_bandwidth=[320301284,547328960,887220144,1383348948,2035730776,2525868732,3295094216,]

clover_ops=[139501,227381,344451,486973,496529,460688,486819,]
clover_threads=[2,4,8,16,32,48,64,]
clover_bandwidth=[226345684,373087184,576995976,873959468,1083977740,1215833932,1505063620,]

 

#static_plot_attributes(ax1,cns_replacement_A,qp_mapping_A,read_write_steering_A,write_steering_A,clover_with_buffering_A)

plot_optimial(ax1,100,clover_threads)
plot_bandwidth(ax1, rw_threads,rw_ops,rw_bandwidth,read_write_steering_label,read_write_steering_marker,read_write_steering_color)
plot_bandwidth(ax1, w_threads,w_ops,w_bandwidth,write_steering_label,write_steering_marker,write_steering_color)
plot_bandwidth(ax1, clover_threads,clover_ops,clover_bandwidth,default_clover_label,default_clover_marker,default_clover_color)
ax1.set_ylabel('Bytes Per Operation')
ax1.set_xlabel('Threads')
ax1.set_ylim(0,4000)
ax1.set_xlim(0,clover_threads[len(clover_threads)-1]+2)

ax1.set_title('Write Only (100% write)')
ax1.legend(loc='upper left')

plt.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')