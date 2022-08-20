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

def gen_tuples(ops,zipf,err):
    return {'zipf': zipf, 'ops': ops, 'err': err}


####################### 128 YCSB A


std=[0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
zipf=[0.00, 0.60, 0.80, 0.90, 1.00, 1.10, 1.20, 1.30, 1.40, 1.50]
clover=[7764629,4234248,2277200,1681447,1320832,1063940,729975,737028,505416,556716,]
write=[10801420,6625309,3886915,2976940,2388756,1965756,1660965,1409957,968532,837804,]
rw=[21694160,21540721,21353131,20935003,20589709,19823556,19043452,17940872,16672884,15133444,]
clover_with_buffering_A_128 = gen_tuples(clover,zipf,std)
write_steering_A_128       = gen_tuples(write,zipf,std)
read_write_steering_A_128  = gen_tuples(rw,zipf,std)


contention_err(ax1,read_write_steering_A_128,write_steering_A_128,clover_with_buffering_A_128)

#ax1.set_title('Zipf Coeff 50% Writes 128 bytes')
ax1.set_ylabel('MOPS')
#ax1.set_ylim(top=350)



plt.tight_layout()
save_fig(plt)