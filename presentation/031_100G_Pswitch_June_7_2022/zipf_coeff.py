import matplotlib
import matplotlib.pyplot as plt
from statistics import mean
import numpy as np

from common import *

fig, (ax1, ax2) = plt.subplots(2,1, figsize=(10,8))


def static_plot_attributes(ax,read_write_steering,write_steering, clover_with_buffering):
    #convert to KOPS/s 
    read_write_steering=div_mil(read_write_steering)
    write_steering=div_mil(write_steering)
    clover_with_buffering=div_mil(clover_with_buffering)

    #plot
    ax.plot(labels[:len(clover_with_buffering)],clover_with_buffering,label=default_clover_label,marker=default_clover_marker, color=default_clover_color)
    ax.plot(labels[:len(write_steering)],write_steering,label=write_steering_label,marker=write_steering_marker,color=write_steering_color)
    ax.plot(labels[:len(read_write_steering)],read_write_steering,label=read_write_steering_label, marker=read_write_steering_marker, color=read_write_steering_color)


    plot_max_improvement(ax,read_write_steering,clover_with_buffering)

    ax.legend(loc='lower left', ncol=1)
    ax.set_xlabel('Zipf Coeff')
    #ax.set_ylim(bottom=0, top=5)

def contention_err(ax,rws,ws, clover):
    scale_mil(rws)
    scale_mil(ws)
    scale_mil(clover)
    ax.errorbar(rws["zipf"],rws["ops"],rws["err"],label=read_write_steering_label, marker=read_write_steering_marker, color=read_write_steering_color)
    ax.errorbar(ws["zipf"],ws["ops"],ws["err"],label=write_steering_label,marker=write_steering_marker,color=write_steering_color)
    ax.errorbar(clover["zipf"],clover["ops"],clover["err"],label=default_clover_label,marker=default_clover_marker, color=default_clover_color)

    plot_max_improvement(ax,rws,clover,"zipf")

    #ax.set_yscale("log")
    ax.legend(loc='upper left', ncol=1)
    ax.set_xlabel('Threads')


figure_name='zipf_coeff'

#[186907,362862,678406,1135524,1753723,2058595,2138879]

measurements=["clover_128", "write_128","rw_128"]
feilds=["ops", "zipf", "err"]
md=dict() #measurement dict

db = read_from_txt("contention.txt")
chunked_db=divide_db(db,len(measurements))
for cdb, m in zip(chunked_db, measurements):
    md[m]=select_feilds(cdb, feilds)
    print(md[m])



####################### 1024 YCSB A
# clover_with_buffering_A_1024= md["clover_1024"]
# write_steering_A_1024       = md["write_1024"]
# read_write_steering_A_1024  = md["rw_1024"]
# contention_err(ax1,read_write_steering_A_1024,write_steering_A_1024,clover_with_buffering_A_1024)

# ax1.set_title('Zipf Coeff 50% Writes 1024 bytes')
# ax1.set_ylabel('MOPS')

####################### 128 YCSB A
clover_with_buffering_A_128 = md["clover_128"]
write_steering_A_128       = md["write_128"]
read_write_steering_A_128  = md["rw_128"]
contention_err(ax2,read_write_steering_A_128,write_steering_A_128,clover_with_buffering_A_128)

ax2.set_title('Zipf Coeff 50% Writes 128 bytes')
ax2.set_ylabel('MOPS')
#ax1.set_ylim(top=350)



plt.tight_layout()
#ax1.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')
#plt.show()