import matplotlib
import matplotlib.pyplot as plt
from statistics import mean
import numpy as np

from common import *

plt.rcParams.update({'font.size': 12})
fig, ax1 = plt.subplots(1,1, figsize=(5,3))
default_line_width=3
default_marker_size=10

def contention_err(ax,rws,ws, clover):
    scale_mil(rws)
    scale_mil(ws)
    scale_mil(clover)
    ax.errorbar(clover["zipf"],clover["ops"],clover["err"],label=default_clover_label,marker=default_clover_marker, color=default_clover_color,linewidth=default_line_width, markersize=default_marker_size )
    ax.errorbar(ws["zipf"],ws["ops"],ws["err"],label=write_steering_label,marker=write_steering_marker,color=write_steering_color,linewidth=default_line_width, markersize=default_marker_size )
    ax.errorbar(rws["zipf"],rws["ops"],rws["err"],label=read_write_steering_label, marker=read_write_steering_marker, color=read_write_steering_color,linewidth=default_line_width, markersize=default_marker_size )

    #plot_max_improvement(ax,rws,clover,"zipf")
    ax.legend( ncol=1, prop={'size': 8})
    ax.set_xlabel('zipf coefficient')
    ax.set_ylim(0,31)

def gen_tuples(ops,zipf,err):
    return {'zipf': zipf, 'ops': ops, 'err': err}


####################### 128 YCSB A


std=[0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
zipf=[0.00, 0.60, 0.80, 0.90, 1.00, 1.10, 1.20, 1.30, 1.40, 1.50]
clover=[3640555,3065432,1682906,1196656,999124,731036,630120,542596,457079,407156,]
write=[8066550,4508152,2532284,1995339,1731571,1464580,1214028,1033144,966524,841748,]
rw=[28380260,28101048,27700444,27491468,27237704,26762201,25558344,24292628,22126031,19967420,]

clover_with_buffering_A_128 = gen_tuples(clover,zipf,std)
write_steering_A_128       = gen_tuples(write,zipf,std)
read_write_steering_A_128  = gen_tuples(rw,zipf,std)


contention_err(ax1,read_write_steering_A_128,write_steering_A_128,clover_with_buffering_A_128)

#ax1.set_title('Zipf Coeff 50% Writes 128 bytes')
ax1.set_ylabel('MOPS')
#ax1.set_ylim(top=350)



plt.tight_layout()
save_fig(plt)