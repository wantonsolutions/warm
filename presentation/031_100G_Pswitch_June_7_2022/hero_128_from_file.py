import matplotlib
import matplotlib.pyplot as plt
from statistics import mean
import numpy as np
from common import *

fig, (ax1, ax2, ax3, ax4) = plt.subplots(1,4, figsize=(20,4))

def tput_err(ax,rws,ws, clover):
    scale_mil(rws)
    scale_mil(ws)
    scale_mil(clover)
    ax.errorbar(rws["threads"],rws["ops"],rws["err"],label=read_write_steering_label, marker=read_write_steering_marker, color=read_write_steering_color)
    ax.errorbar(ws["threads"],ws["ops"],ws["err"],label=write_steering_label,marker=write_steering_marker,color=write_steering_color)
    ax.errorbar(clover["threads"],clover["ops"],clover["err"],label=default_clover_label,marker=default_clover_marker, color=default_clover_color)

    plot_max_improvement(ax,rws,clover,"threads")

    #ax.set_yscale("log")
    ax.legend(loc='upper left', ncol=1)
    ax.set_xlabel('Threads')




#def add_bw(ax, rws, ws, clover, pps=False):
def add_bw(ax, rws, ws, clover, pps=True):
    rw_bw, rw_bw_err = get_gbps_and_err(rws)
    w_bw, w_bw_err = get_gbps_and_err(ws)
    c_bw, c_bw_err = get_gbps_and_err(clover)
    rw_pk, rw_pk_err = get_mpps_and_err(rws)
    w_pk, w_pk_err = get_mpps_and_err(ws)
    c_pk, c_pk_err = get_mpps_and_err(clover)

    #ax.set_ylim(0,60)

    ax2=ax.twinx()
    p1 = ax2.errorbar(rws["threads"], rw_bw, yerr=rw_bw_err, color=read_write_steering_color, marker='o', markersize=10, markerfacecolor="none", linestyle='none',label="bandwidth")
    ax2.errorbar(ws["threads"], w_bw, yerr=w_bw_err, color=write_steering_color, marker='o', markersize=10, linestyle='none', markerfacecolor="none")
    ax2.errorbar(clover["threads"], c_bw, yerr=c_bw_err, color=default_clover_color, marker='o',linestyle='none', markersize=10, markerfacecolor="none")

    ax2.spines.right.set_linestyle(':')
    ax2.set_ylabel("GBPS")
    ax2.set_ylim(0)


    #ax2.set_ylim(0,400)

    if pps:
        ax3=ax.twinx()
        ax3.spines.right.set_position(("axes", 1.2))

        p2 = ax3.errorbar(rws["threads"], rw_pk, yerr=rw_pk_err, color=read_write_steering_color, marker="D", markersize=5, markerfacecolor="none", linestyle='none', label="pps")
        ax3.errorbar(ws["threads"], w_pk, yerr=w_pk_err, color=write_steering_color, markersize=5, marker="D", markerfacecolor="none", linestyle='none')
        ax3.errorbar(clover["threads"], c_pk, yerr=w_pk_err, color=default_clover_color, markersize=5, marker="D", markerfacecolor="none", linestyle='none')
        ax3.spines.right.set_linestyle('--')
        ax3.spines.right.set_color('green')
        ax3.spines.right.set_hatch('O')
        ax3.yaxis.label.set_color('green')

        ax3.set_ylabel("MPPS")
        ax3.legend(handles=[p1, p2], loc="lower right")

measurements=["clover_C", "write_C","rw_C", "clover_B", "write_B","rw_B", "clover_A", "write_A","rw_A", "clover_W", "write_W","rw_W"]

ycsbs=["C", "B", "A", "W"]

#feilds=["ops", "threads", "err"]
feilds=["ops", "err", "threads", "duration", "total_bytes", "total_bytes_error", "total_packets", "total_packets_error"]
md=dict() #measurement dict

db = read_from_txt("128_avg.txt")
chunked_db=divide_db(db,len(measurements))
for cdb, m in zip(chunked_db, measurements):
    md[m]=select_feilds(cdb, feilds)
    print(md[m])



####################### 128 YCSB A
clover_with_buffering_C= md["clover_C"]
write_steering_C    =   md["write_C"]
read_write_steering_C  = md["rw_C"]
tput_err(ax1,read_write_steering_C,write_steering_C,clover_with_buffering_C)
add_bw(ax1,read_write_steering_C,write_steering_C,clover_with_buffering_C)


ax1.set_title('0% Writes')
ax1.set_ylabel('MOPS')

####################### YCSB B
clover_with_buffering_B=md["clover_B"]
write_steering_B=        md["write_B"]
read_write_steering_B=   md["rw_B"]

tput_err(ax2,read_write_steering_B,write_steering_B,clover_with_buffering_B)
add_bw(ax2,read_write_steering_B,write_steering_B,clover_with_buffering_B)
ax2.set_title('5% Writes')
#ax1.set_ylim(top=350)


####################### YCSB A
clover_with_buffering_A= md["clover_A"]
write_steering_A=        md["write_A"]
read_write_steering_A=   md["rw_A"]
tput_err(ax3,read_write_steering_A,write_steering_A,clover_with_buffering_A)
add_bw(ax3,read_write_steering_A,write_steering_A,clover_with_buffering_A)
ax3.set_title('50% Writes')


clover_with_buffering_W= md["clover_W"]
write_steering_W=        md["write_W"]
read_write_steering_W=   md["rw_W"]
tput_err(ax4,read_write_steering_W,write_steering_W,clover_with_buffering_W)
add_bw(ax4,read_write_steering_W,write_steering_W,clover_with_buffering_W)
ax4.set_title('100% Writes')






# plt.tight_layout()
# #ax1.tight_layout()
# plt.savefig(figure_name+'.pdf')
# plt.savefig(figure_name+'.png')
# #plt.show()

plt.tight_layout()
save_fig(plt)