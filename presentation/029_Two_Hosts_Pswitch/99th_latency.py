import matplotlib.pyplot as plt
import numpy as np

from common import *

master_width=1

plt.rcParams.update({'font.size': 16})
fig, (ax1, ax2) = plt.subplots(1 ,2, figsize=(15,5), gridspec_kw={'width_ratios': [3,3]})

def get_labels(measure):
    labels = measure["ratio"]
    for i in range(len(labels)):
        if labels[i] == 1:
            labels[i]=0
    labels = [str(l) for l in labels]
    return labels

def plot_latency(ax, clover, write, rw, mode):
    labels=get_labels(clover)
    x = np.arange(len(labels))  # the label locations
    width = master_width/4  # the width of the bars


    #print(clover["write_lat_err"])

    lat_label=mode+"_lat"
    err_label=mode+"_lat_err"


    ax.errorbar(x - width, clover[lat_label], yerr=clover[err_label], color='k', fmt='o')
    ax.errorbar(x, write[lat_label], yerr=write[err_label], color='k', fmt='o')
    ax.errorbar(x + width, rw[lat_label], yerr=rw[err_label], color='k' ,fmt='o')

    rects0 = ax.bar(x - width, clover[lat_label], width, label='Clover',color=default_clover_color,edgecolor='k')
    rects1 = ax.bar(x, write[lat_label], width, label='Write',color=write_steering_color,edgecolor='k')
    rects2 = ax.bar(x + width, rw[lat_label], width, label='Write+Read',color=read_write_steering_color,edgecolor='k')

    win=[]
    for a,b in zip(clover[lat_label],rw[lat_label]):
        if b == 0:
            win.append(0)
        else:
            win.append(int(round(a/b, 0)))


    for i in range(len(win)):
        if win[i] != 0:
            ax.text(x[i] + width/1.5, rw[lat_label][i] + 10, str(win[i])+"x")
    
    ax.set_xticks(x)
    ax.set_xticklabels(labels)




measurements=["clover", "write", "rw"]
feilds=["read_lat", "read_lat_err", "write_lat", "write_lat_err", "ratio"]
md=dict() #measurement dict

db = read_from_txt("99th_latency.txt")
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

plot_latency(ax1,clover,write,rw, "read")
# # Add some text for labels, title and custom x-axis tick labels, etc.
ax1.set_ylabel('99th percentile latency (us)')
ax1.set_title('Read Latencies')
ax1.set_yscale('log')
ax1.set_ylim(2,3000)
ax1.set_xlabel("Write Ratio")
ax1.legend()

plot_latency(ax2,md["clover"], md["write"], md["rw"], "write")
ax2.set_ylabel('99th percentile latency (us)')
ax2.set_title('Write Latencies')
ax2.set_yscale('log')
#ax2.set_ylim(20,10000)
ax2.set_ylim(2,3000)
ax2.set_xlabel("Write Ratio")

#//ax.bar_label(rects2, padding=3)
plt.tight_layout()
save_fig(plt)

