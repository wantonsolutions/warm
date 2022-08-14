import matplotlib
import matplotlib.pyplot as plt
from statistics import mean
import numpy as np

from common import *

fig, (ax1) = plt.subplots(1,1, figsize=(5,4))

def rdma_int_to_str(m):
    arr = m["size"]
    i=0
    for a in arr:
        arr[i]=str(a)
        i=i+1
    print(arr)
    m["size"] = arr

def packet_size_plot(ax,rw,w,c):
    scale_mil(rw)
    scale_mil(w)
    scale_mil(c)

    rdma_int_to_str(rw)
    rdma_int_to_str(w)
    rdma_int_to_str(c)




    ax.errorbar(c["size"],c["ops"],c["err"],label=default_clover_label,marker=default_clover_marker, color=default_clover_color)
    ax.errorbar(w["size"],w["ops"],w["err"],label=write_steering_label,marker=write_steering_marker,color=write_steering_color)
    ax.errorbar(rw["size"],rw["ops"],rw["err"],label=read_write_steering_label, marker=read_write_steering_marker, color=read_write_steering_color)
    plot_max_improvement(ax,rw,c, "size" )
    ax.plot()

    #ax.set_yscale('log')        
    ax.set_ylim(bottom=0, top=25)
    ax.legend(loc='upper left', ncol=3)
    ax.set_xlabel('RDMA Payload Size')
    return


####################### YCSB C
rdma_size=[128,256,512,1024,]
avg_ops=[1325264,1185251,1047740,828956,]
threads=[200,200,200,200,]
std=[0,0,0,0,]
clover={"ops": avg_ops,"threads": threads, "err": std, "size": rdma_size}

rdma_size=[128,256,512,1024,]
avg_ops=[2385936,1923504,1197124,680792,]
threads=[200,200,200,200,]
std=[0,0,0,0,]

write={"ops": avg_ops,"threads": threads, "err": std, "size": rdma_size}       

rdma_size=[128,256,512,1024,]
avg_ops=[20582012,20059844,13918228,11808436,]
threads=[200,200,200,200,]
std=[0,0,0,0,]
rw={"ops": avg_ops,"threads": threads, "err": std, "size": rdma_size}  

#static_plot_attributes(ax1,read_write_steering_C,write_steering_C,clover_with_buffering_C)
packet_size_plot(ax1,rw,write,clover)

ax1.set_title('RDMA Payload Size vs Throughput 120 Cores (50% write)')
ax1.set_ylabel('MOPS')
#ax1.set_ylim(top=350)


plt.tight_layout()
save_fig(plt)