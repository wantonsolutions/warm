import matplotlib.pyplot as plt
import numpy as np

from common import *

master_width=1

plt.rcParams.update({'font.size': 16})
#fig, ax1 = plt.subplots(1 ,1, figsize=(15,5), gridspec_kw={'width_ratios': [3,3]})
fig, ax1 = plt.subplots(1 ,1, figsize=(7,7)) #, gridspec_kw={'width_ratios': [3,3]})

def get_gbps(total_bytes, duration):
    bw=[]
    for a,b in zip(total_bytes,duration):
        bw_from_data= a/(b/1000) #a is the total bytes, duration is in ms
        gbps=(bw_from_data*8.0)/1000000000.0 #convert to gbps
        bw.append(gbps)
    return bw

def get_mpps(total_bytes, duration):
    bw=[]
    for a,b in zip(total_bytes,duration):
        bw_from_data= a/(b/1000)       #a is the total packets that would have run in a second 
        gbps=(bw_from_data)/1000000.0 #convert to mpps
        bw.append(gbps)
    return bw

def get_mpps_and_err(measure):
    mpps = get_mpps(measure["total_packets"],measure["duration"])
    mpps_err = get_mpps(measure["total_packets_error"],measure["duration"])

def get_gbps_and_err(measure):
    bw = get_gbps(measure["total_bytes"],measure["duration"])
    bw_err = get_gbps(measure["total_bytes_error"],measure["duration"])
    return(bw,bw_err)



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
    # rw_bw, rw_bw_err = get_gbps_and_err(rw)
    # w_bw, w_bw_err = get_gbps_and_err(write)
    # c_bw, c_bw_err = get_gbps_and_err(clover)
    rw_bw, rw_bw_err = get_mpps_and_err(rw)
    w_bw, w_bw_err = get_mpps_and_err(write)
    c_bw, c_bw_err = get_mpps_and_err(clover)
    ax2.errorbar(rw["threads"], rw_bw, yerr=rw_bw_err, color=read_write_steering_color, marker='o', markersize=20, markerfacecolor="none", label="bandwidth")
    ax2.errorbar(write["threads"], w_bw, yerr=w_bw_err, color=write_steering_color, marker='o', markersize=20, markerfacecolor="none")
    ax2.errorbar(clover["threads"], c_bw, yerr=c_bw_err, color=default_clover_color, marker='o', markersize=20, markerfacecolor="none")
    ax2.set_ylabel("GBPS")
    ax2.set_ylim(0)
    ax2.legend(loc="lower left")



measurements=["clover", "write", "rw"]
feilds=["ops", "err", "threads", "duration", "total_bytes", "total_bytes_error"]
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