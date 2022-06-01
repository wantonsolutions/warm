import matplotlib
import matplotlib.pyplot as plt
from statistics import mean
import numpy as np

fig, (ax1) = plt.subplots(1,1, figsize=(5,4))
labels=["128","256","512","1024"]                                                                                                                                                                        

read_write_steering_label='Write + Read'
write_steering_label='Write'
default_clover_label='Clover'

read_write_steering_marker='+'
write_steering_marker="*"
default_clover_marker='.'

read_write_steering_color='#cf243cff'     #alice
write_steering_color='#ed7d31ff'          #kelly
default_clover_color='#8caff6ff'          #Coral 


def div_mil (list):
    return [val /1000000.0 for val in list]

def plot_max_improvement(ax,read_write_steering,clover_with_buffering):
    if len(read_write_steering) > 1 and (len(read_write_steering) == len(clover_with_buffering)):
        improvement=[]
        for rw ,clov in zip(read_write_steering, clover_with_buffering):
            improvement.append(rw/clov)
        i=0
        max_index=0
        max_improvement=0

        for value in improvement:
            if value > max_improvement:
                max_improvement = value
                max_index=i
            i=i+1
        
        max_improvement=round(max_improvement,2)
        ax.text(labels[len(labels) -2], read_write_steering[len(read_write_steering)-1]/2,str(max_improvement)+"x")
        x_1=labels[max_index]
        x=[x_1,x_1]
        y=[read_write_steering[max_index],clover_with_buffering[max_index]]

        ax.plot(x,y,color='k',linestyle='--')

def invert_array(a):
    inv=[]
    for i in a:

        for j in i:
            inv.append([])
        break
    
    for i in a:
        c=0
        for j in i:
            inv[c].append(j)
            c=c+1
    return inv

def get_stats(arr):
    means=[]
    std=[]

    for i in arr:
        i = div_mil(i)
        means.append(np.mean(i))
        std.append(np.std(i))
    return (means, std)

def prepare_stat(c):
    c = invert_array(c)
    print(c)
    c_s = get_stats(c)
    return c_s



def stat_plot(ax,rw,w,c):
    c_s = prepare_stat(c)
    w_s = prepare_stat(w)
    rw_s = prepare_stat(rw)
    ax.errorbar(labels[:len(c_s[0])],c_s[0],c_s[1],label=default_clover_label,marker=default_clover_marker, color=default_clover_color)
    ax.errorbar(labels[:len(w_s[0])],w_s[0],w_s[1],label=write_steering_label,marker=write_steering_marker,color=write_steering_color)
    ax.errorbar(labels[:len(rw_s[0])],rw_s[0],rw_s[1],label=read_write_steering_label, marker=read_write_steering_marker, color=read_write_steering_color)

    plot_max_improvement(ax,rw_s[0],c_s[0])
    ax.plot()

    #ax.set_yscale('log')        
    ax.set_ylim(bottom=0,top=15.5)
    ax.legend(loc='upper left', ncol=3)
    ax.set_xlabel('RDMA Payload Size')
    return



figure_name='packet_size'

####################### YCSB C
clover_with_buffering_C=[
[2081040,2152188,2148320,1604459,],
[2326894,2280572,2041730,1687866,],
[2324180,2279528,2158332,1685049,],
]


write_steering_C       =[
[3077964,2224741,1430214,825934,],
[3082058,2225782,1423730,796210,],
[3077964,2224741,1430214,825934,],
]
read_write_steering_C  = [
    [13694586,13167842,10033896,6665202,],
    [13694586,13167842,10033896,6665202,],
    [13634080,13139716,10336806,6664112,]
]
#static_plot_attributes(ax1,read_write_steering_C,write_steering_C,clover_with_buffering_C)
stat_plot(ax1,read_write_steering_C,write_steering_C,clover_with_buffering_C)

ax1.set_title('RDMA Payload Size vs Throughput 120 Cores (50% write)')
ax1.set_ylabel('MOPS')
#ax1.set_ylim(top=350)


plt.tight_layout()
#ax1.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')
#plt.show()