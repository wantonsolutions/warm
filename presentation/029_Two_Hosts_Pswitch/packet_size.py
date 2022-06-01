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
    ax.set_ylim(bottom=0,top=14.5)
    ax.legend(loc='upper left', ncol=3)
    ax.set_xlabel('RDMA Payload Size')
    return


####################### YCSB C
avg_ops=[1375391,1379040,1353326,1152410,]
rdma_size=[128,256,512,1024,]
threads=[120,120,120,120,]
std=[2263,3054,720,720,]
clover={"ops": avg_ops,"threads": threads, "err": std, "size": rdma_size}

avg_ops=[1891907,1279140,815183,495038,]
rdma_size=[128,256,512,1024,]
threads=[120,120,120,120,]
std=[28771,5330,2770,2770,]
write={"ops": avg_ops,"threads": threads, "err": std, "size": rdma_size}       

avg_ops=[12609386,12638176,10341434,6706972,]
rdma_size=[128,256,512,1024,]
threads=[120,120,120,120,]
std=[14731,4199,978,978,]
rw={"ops": avg_ops,"threads": threads, "err": std, "size": rdma_size}  

#static_plot_attributes(ax1,read_write_steering_C,write_steering_C,clover_with_buffering_C)
packet_size_plot(ax1,rw,write,clover)

ax1.set_title('RDMA Payload Size vs Throughput 120 Cores (50% write)')
ax1.set_ylabel('MOPS')
#ax1.set_ylim(top=350)


plt.tight_layout()
save_fig(plt)