import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1 import host_subplot
from mpl_toolkits import axisartist
import numpy as np

from common import *

master_width=1

plt.rcParams.update({'font.size': 16})
#fig, ax1 = plt.subplots(1 ,1, figsize=(15,5), gridspec_kw={'width_ratios': [3,3]})
fig, ax1 = plt.subplots(1 ,1, figsize=(7,7)) #, gridspec_kw={'width_ratios': [3,3]})
fig.subplots_adjust(right=0.75)


def plot_ops_bs(ax, clover, write, rw):

    ax.errorbar( rw["threads"],rw["ops"], label=read_write_steering_label, marker=read_write_steering_marker, color=read_write_steering_color)
    ax.errorbar( write["threads"],write["ops"], label=write_steering_label,marker=write_steering_marker,color=write_steering_color)
    ax.errorbar( clover["threads"],clover["ops"],label=default_clover_label,marker=default_clover_marker, color=default_clover_color)



    ax.set_ylabel('MOPS')
    ax.set_title('ops vs bandwidth')
    #ax.set_yscale("log")
    ax.set_xlabel("Threads")
    ax.set_ylim(0)
    ax.legend()

    ax2=ax.twinx()
    ax3=ax.twinx()

    rw_bw, rw_bw_err = get_gbps_and_err(rw)
    w_bw, w_bw_err = get_gbps_and_err(write)
    c_bw, c_bw_err = get_gbps_and_err(clover)
    rw_pk, rw_pk_err = get_mpps_and_err(rw)
    w_pk, w_pk_err = get_mpps_and_err(write)
    c_pk, c_pk_err = get_mpps_and_err(clover)
    ax2.errorbar(rw["threads"], rw_bw, yerr=rw_bw_err, color=read_write_steering_color, marker='o', markersize=20, markerfacecolor="none", label="bandwidth")
    ax2.errorbar(write["threads"], w_bw, yerr=w_bw_err, color=write_steering_color, marker='o', markersize=20, markerfacecolor="none")
    ax2.errorbar(clover["threads"], c_bw, yerr=c_bw_err, color=default_clover_color, marker='o', markersize=20, markerfacecolor="none")
    ax2.set_ylabel("GBPS")
    ax2.set_ylim(0)
    ax2.legend(loc="lower left")

    offset = 60

    ax3.axis["right"] = par2.new_fixed_axis(loc="right", offset=(60, 0))
    #new_fixed_axis = ax3.get_grid_helper().new_fixed_axis
    # ax3.axis["right"] = new_fixed_axis(loc="right", axes=ax3,
    #                                     offset=(offset, 0))

    # ax3.axis["right"].toggle(all=True)

    #ax3.spines.right.set_position(("axes", 1.2))
    ax3.errorbar(rw["threads"], rw_pk, yerr=rw_pk_err, color=read_write_steering_color, marker='x', markersize=20, markerfacecolor="none", linestyle='--', label="pps")
    ax3.errorbar(write["threads"], w_pk, yerr=w_pk_err, color=write_steering_color, marker='x', markersize=20, markerfacecolor="none", linestyle='--')
    ax3.errorbar(clover["threads"], c_pk, yerr=w_pk_err, color=default_clover_color, marker='x', markersize=20, markerfacecolor="none", linestyle='--')




measurements=["clover", "write", "rw"]
feilds=["ops", "err", "threads", "duration", "total_bytes", "total_bytes_error", "total_packets", "total_packets_error"]
md=dict() #measurement dict

db = read_from_txt("single_bandwidth.txt")
chunked_db=divide_db(db,len(measurements))
for cdb, m in zip(chunked_db, measurements):
    md[m]=select_feilds(cdb, feilds)
    print(md[m])

clover=md["clover"]
write=md["write"]
rw=md["rw"]
scale(clover)
scale(write)
scale(rw)

plot_ops_bs(ax1,clover,write,rw)

plt.tight_layout()
save_fig(plt)