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



####################### YCSB C
avg_ops=[866556,1677449,3286434,6437521,11997007,16172503,16220920,16221296,]
threads=[3,6,12,24,48,72,96,120,]
std=[882,3252,3550,183620,10874,4896,6800,6800,]
clover_with_buffering_C= {"ops": avg_ops,"threads": threads, "err": std}
avg_ops=[867771,1678017,3288402,6433192,12053542,16107614,16221201,16226420,]
threads=[3,6,12,24,48,72,96,120,]
std=[1487,2961,3384,60538,119171,6796,4238,4238,]
write_steering_C=        {"ops": avg_ops,"threads": threads, "err": std}
avg_ops=[867641,1678272,3282914,6440277,12016216,16165432,16219664,16219396,]
threads=[3,6,12,24,48,72,96,120,]
std=[883,13447,4635,74019,8458,4664,5952,5952,]
read_write_steering_C=   {"ops": avg_ops,"threads": threads, "err": std}

tput_err(ax1,read_write_steering_C,write_steering_C,clover_with_buffering_C)

ax1.set_title('0% Writes')
ax1.set_ylabel('MOPS')

####################### YCSB B

avg_ops=[771261,1477929,2778626,4563829,7584997,8988955,9696498,9105597,]
threads=[3,6,12,24,48,72,96,120,]
std=[11127,22274,13403,36597,155369,7244,9522,9522,] 
clover_with_buffering_B= {"ops": avg_ops,"threads": threads, "err": std}
avg_ops=[775847,1483216,2804457,4700867,7828226,9476730,9820703,9225293,]
threads=[3,6,12,24,48,72,96,120,]
std=[2431,17835,5310,14054,64580,99589,11232,11232,] 
write_steering_B=        {"ops": avg_ops,"threads": threads, "err": std}
avg_ops=[821256,1584619,3095496,5984433,11292677,15311122,16205093,16266863,]
threads=[3,6,12,24,48,72,96,120,]
std=[12827,27646,51060,24129,31629,99816,40071,40071,]
read_write_steering_B=   {"ops": avg_ops,"threads": threads, "err": std}

tput_err(ax2,read_write_steering_B,write_steering_B,clover_with_buffering_B)
ax2.set_title('5% Writes')
#ax1.set_ylim(top=350)


####################### YCSB A

avg_ops=[350771,583653,888214,1125869,1273476,1426466,1405170,1393045,]
threads=[3,6,12,24,48,72,96,120,]
std=[1245,14676,19404,24274,1266,2664,1828,1828,]
clover_with_buffering_A={"ops": avg_ops,"threads": threads, "err": std}

avg_ops=[463353,805939,1324920,1958157,2330401,2539998,2279287,1849486,]
threads=[3,6,12,24,48,72,96,120,]
std=[7394,35731,2166,91332,5306,70302,48740,48740,]
write_steering_A={"ops": avg_ops,"threads": threads, "err": std}

avg_ops=[579542,1105637,2089796,4002665,7036191,9271950,11127037,12628846,]
threads=[3,6,12,24,48,72,96,120,]
std=[1472,40654,64325,199929,323045,370850,398494,398494,]
read_write_steering_A={"ops": avg_ops,"threads": threads, "err": std}

tput_err(ax3,read_write_steering_A,write_steering_A,clover_with_buffering_A)
ax3.set_title('50% Writes')


####################### Write Only
avg_ops=[212850,352813,529428,621600,684549,733559,711748,706982,]
threads=[3,6,12,24,48,72,96,120,]
std=[4605,12689,13944,20481,1304,16725,12736,12736,]

clover_with_buffering_W={"ops": avg_ops,"threads": threads, "err": std}

avg_ops=[423825,807568,1548658,2893532,4747502,6049855,6899267,7434914,]
threads=[3,6,12,24,48,72,96,120,]
std=[371,769,6977,13587,5498,19697,13422,13422,]
write_steering_W= {"ops": avg_ops,"threads": threads, "err": std}       

avg_ops=[423390,807382,1548106,2859218,4748105,5791846,6817763,7339224,]
threads=[3,6,12,24,48,72,96,120,]
std=[1024,1985,61037,5600,176064,108314,240301,240301,]

read_write_steering_W= {"ops": avg_ops,"threads": threads, "err": std}  

tput_err(ax4,read_write_steering_W,write_steering_W,clover_with_buffering_W)
ax4.set_title('100% Writes')

plt.tight_layout()
save_fig(plt)