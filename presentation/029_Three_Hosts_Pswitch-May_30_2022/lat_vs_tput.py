import matplotlib.pyplot as plt
import numpy as np

from common import *

master_width=1

plt.rcParams.update({'font.size': 16})
#fig, ax1 = plt.subplots(1 ,1, figsize=(15,5), gridspec_kw={'width_ratios': [3,3]})
fig, (ax1, ax2) = plt.subplots(2 ,1, figsize=(12,10)) #, gridspec_kw={'width_ratios': [3,3]})

a=[1,3]
b=[2,3]


def plot_lat_vs_tput(ax, clover, write, rw, mode):
    lat_label=mode+"_lat"
    err_label=mode+"_lat_err"

    ax.plot(rw["ops"], rw[lat_label], label=read_write_steering_label, marker=read_write_steering_marker, color=read_write_steering_color, zorder=-32)
    ax.plot(write["ops"], write[lat_label], label=write_steering_label,marker=write_steering_marker,color=write_steering_color, zorder=-32)
    ax.plot(clover["ops"], clover[lat_label],label=default_clover_label,marker=default_clover_marker, color=default_clover_color, zorder=-32)

    ax.errorbar(rw["ops"], rw[lat_label], yerr=rw[err_label], xerr=rw["err"],ecolor='k',ls="none")
    ax.errorbar(write["ops"], write[lat_label], yerr=write[err_label],xerr=write["err"],ecolor='k', ls="none")
    ax.errorbar(clover["ops"], clover[lat_label], yerr=clover[err_label],xerr=clover["err"],ecolor='k', ls="none")

    ax.set_ylabel('Latency (us) 99th')
    ax.set_title(mode)
    #ax.set_yscale("log")
    ax.set_xlabel("MOPS")
    ax.legend()



measurements=["clover", "write", "rw"]
feilds=["ops", "err", "read_lat", "read_lat_err", "write_lat", "write_lat_err", "ratio"]
md=dict() #measurement dict

db = read_from_txt("lat_vs_tput.txt")
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

plot_lat_vs_tput(ax1,clover,write,rw, "read")
plot_lat_vs_tput(ax2,clover,write,rw, "write")

plt.tight_layout()
save_fig(plt)