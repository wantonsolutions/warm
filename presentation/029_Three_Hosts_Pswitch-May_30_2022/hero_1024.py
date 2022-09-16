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
avg_ops=[792400,1365694,2438155,4135054,4150832,4153698,4150676,4135850,]
threads=[3,6,12,24,48,72,96,120,]
std=[17561,159431,2483,2443,1190,3793,11519,11519,]
clover_with_buffering_C= {"ops": avg_ops,"threads": threads, "err": std}
avg_ops=[778536,1337471,2278904,4140382,4153698,4153550,4152705,4127696,]
threads=[3,6,12,24,48,72,96,120,]
std=[19108,18311,6656,219,658,2007,8575,8575,]
write_steering_C=        {"ops": avg_ops,"threads": threads, "err": std}
avg_ops=[781174,1380559,2600647,4148367,4152701,4154141,4149423,4122351,]
threads=[3,6,12,24,48,72,96,120,]
std=[15445,36912,1339,993,1163,3782,3980,3980,]
read_write_steering_C=   {"ops": avg_ops,"threads": threads, "err": std}

tput_err(ax1,read_write_steering_C,write_steering_C,clover_with_buffering_C)

ax1.set_title('0% Writes')
ax1.set_ylabel('MOPS')


####################### YCSB B
avg_ops=[697789,1296803,2287629,3277717,3036183,2833652,2731705,2494283,]                                                                                                                                          
threads=[3,6,12,24,48,72,96,120,]                                                                                                                                                                                  
std=[1924,2565,2053,3229,2218,2219,4491,4491,]
clover_with_buffering_B= {"ops": avg_ops,"threads": threads, "err": std}
avg_ops=[705463,1310468,2320500,3300750,3041726,2823903,2717726,2463746,]                                                                                                                                          
threads=[3,6,12,24,48,72,96,120,]                                                                                                                                                                                  
std=[1146,2091,792,3162,1180,2102,3365,3365,] 
write_steering_B=        {"ops": avg_ops,"threads": threads, "err": std}
avg_ops=[769438,1420323,2595152,4271659,4286393,4280241,4168778,3931812,]
threads=[3,6,12,24,48,72,96,120,]
std=[6428,15589,968,355,1372,2365,4605,4605,]
read_write_steering_B=   {"ops": avg_ops,"threads": threads, "err": std}

tput_err(ax2,read_write_steering_B,write_steering_B,clover_with_buffering_B)
ax2.set_title('5% Writes')

####################### YCSB A
avg_ops=[317350,554189,869837,1155980,1304640,1316197,1245422,1154292,]
threads=[3,6,12,24,48,72,96,120,]
std=[2362,593,2310,2692,1885,3329,2467,2467,]
clover_with_buffering_A={"ops": avg_ops,"threads": threads, "err": std}

avg_ops=[427959,761485,1240947,1698519,1153834,792204,614217,496982,]
threads=[3,6,12,24,48,72,96,120,]
std=[1225,1715,1090,1141,728,287,1038,1038,]
write_steering_A={"ops": avg_ops,"threads": threads, "err": std}

avg_ops=[538500,1042142,1857515,3596520,5760747,6478277,6650238,6706320,]                                                                                                                                          
threads=[3,6,12,24,48,72,96,120,]                                                                                                                                                                                  
std=[907,216730,5274,4589,2005,1557,2854,2854,]   
read_write_steering_A={"ops": avg_ops,"threads": threads, "err": std}

tput_err(ax3,read_write_steering_A,write_steering_A,clover_with_buffering_A)
ax3.set_title('50% Writes')


####################### Write Only
avg_ops=[204583,353182,523120,635765,710846,713778,683036,629644,]
threads=[3,6,12,24,48,72,96,120,]
std=[536,624,1254,1096,1341,1893,2541,2541,]
clover_with_buffering_W={"ops": avg_ops,"threads": threads, "err": std}

avg_ops=[402735,766394,1450877,2610197,3782986,3864792,3883800,3894565,]
threads=[3,6,12,24,48,72,96,120,]
std=[1205,1497,1897,574,1533,3699,1651,1651,]
write_steering_W= {"ops": avg_ops,"threads": threads, "err": std}       

avg_ops=[403343,765721,1447410,2606486,3782986,3866430,3884630,3894210,]
threads=[3,6,12,24,48,72,96,120,]
std=[1754,3249,4606,2414,2097,2064,1704,1704,]
read_write_steering_W= {"ops": avg_ops,"threads": threads, "err": std}  

tput_err(ax4,read_write_steering_W,write_steering_W,clover_with_buffering_W)
ax4.set_title('100% Writes')

plt.tight_layout()
save_fig(plt)