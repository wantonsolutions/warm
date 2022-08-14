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
avg_ops=[1829774,3577880,7008356,13684405,25656091,34720826,39142324,38713784,38770952,40600024,]
threads=[7,14,28,56,112,168,224,280,336,392,]
std=[0,0,0,0,0,0,0,0,0,0,]
clover_with_buffering_C= {"ops": avg_ops,"threads": threads, "err": std}
avg_ops=[1836632,3572209,7003510,13673092,25654763,34857976,39180515,38957292,38117704,40295512,]
threads=[7,14,28,56,112,168,224,280,336,392,]
std=[0,0,0,0,0,0,0,0,0,0,]
write_steering_C=        {"ops": avg_ops,"threads": threads, "err": std}
avg_ops=[1832205,3567680,7009575,13703432,25692824,34795712,39152679,39690996,40607492,39555924,]
threads=[7,14,28,56,112,168,224,280,336,392,]
std=[0,0,0,0,0,0,0,0,0,0,]
read_write_steering_C=   {"ops": avg_ops,"threads": threads, "err": std}
tput_err(ax1,read_write_steering_C,write_steering_C,clover_with_buffering_C)

ax1.set_title('0% Writes')
ax1.set_ylabel('MOPS')

####################### YCSB B
avg_ops=[1487724,2655252,4818364,7666020,12365564,13979169,13716786,13427863,12837368,12382960,]
threads=[7,14,28,56,112,168,224,280,336,392,]
std=[0,0,0,0,0,0,0,0,0,0,]
clover_with_buffering_B= {"ops": avg_ops,"threads": threads, "err": std}
# 1500840,7,250,1.00,5,100,375210,0,0,0,0,0,182427796,0,1683868,0,1
# 2680607,14,250,1.00,5,100,670912,0,0,0,0,0,345258228,0,3152802,0,1
# 4929508,28,250,1.00,5,100,1232377,0,0,0,0,0,667922752,0,6033988,0,1
# 7935485,56,250,1.00,5,100,1987274,0,0,0,0,0,1264196434,0,11110037,0,1
# 12774234,112,251,1.00,5,100,3195360,0,0,0,0,0,2338405882,0,20100933,0,1
# 14548308,168,250,1.00,5,100,3641200,0,0,0,0,0,3106151424,0,26145490,0,1
# 14439633,224,250,1.00,5,100,3612079,0,0,0,0,0,3469483412,0,28782648,0,1
# 12124372,280,250,1.00,5,100,3031093,0,0,0,0,0,3539320848,0,28755976,0,1
# 13402903,336,250,1.00,5,100,3352952,0,0,0,0,0,3731373590,0,30465537,0,1
avg_ops=[1500840,2680607,4929508,7935485,12774234,14548308,14439633,14124372,13402903,13402903,]
threads=[7,14,28,56,112,168,224,280,336,392,]
std=[0,0,0,0,0,0,0,0,0,0,]
write_steering_B=        {"ops": avg_ops,"threads": threads, "err": std}

# 1748703,7,250,1.00,5,100,437176,0,0,0,0,0,188300832,0,1783362,0,1
# 3377649,14,250,1.00,5,100,844900,0,0,0,0,0,364164222,0,3450665,0,1
# 6438944,28,250,1.00,5,100,1610656,0,0,0,0,0,692295860,0,6555284,0,1
# 12586564,56,250,1.00,5,100,3146641,0,0,0,0,0,1353824848,0,12823264,0,1
# 21354096,112,250,1.00,5,100,5338524,0,0,0,0,0,2288911800,0,21648318,0,1
# 28715113,168,250,1.00,5,100,7186904,0,0,0,0,0,3085846228,0,29143364,0,1
# 34991652,224,250,1.00,5,100,8752841,0,0,0,0,0,3850601424,0,36200454,0,1

avg_ops=[1748703,3377649,6438944,12586564,21354096,28715113,34991652,34230195,35529061,35252996,]
threads=[7,14,28,56,112,168,224,280,336,392,]
std=[0,0,0,0,0,0,0,0,0,0,]
read_write_steering_B=   {"ops": avg_ops,"threads": threads, "err": std}

tput_err(ax2,read_write_steering_B,write_steering_B,clover_with_buffering_B)
ax2.set_title('5% Writes')
#ax1.set_ylim(top=350)


####################### YCSB A
avg_ops=[554788,877068,1120964,1312144,1509604,1450528,1226730,870612,1180566,1257288,]
threads=[7,14,28,56,112,168,224,280,336,392,]
std=[0,0,0,0,0,0,0,0,0,0,]
clover_with_buffering_A={"ops": avg_ops,"threads": threads, "err": std}
avg_ops=[815460,1331156,1858488,2343504,2639795,1886740,2098574,1471908,1830812,1602349,]
threads=[7,14,28,56,112,168,224,280,336,392,]
std=[0,0,0,0,0,0,0,0,0,0,]
write_steering_A={"ops": avg_ops,"threads": threads, "err": std}
avg_ops=[1194607,2264604,4147258,8024861,13868472,19604656,24528677,28124268,28348060,28201180,]
threads=[7,14,28,56,112,168,224,280,336,392,]
std=[0,0,0,0,0,0,0,0,0,0,]
read_write_steering_A={"ops": avg_ops,"threads": threads, "err": std}
tput_err(ax3,read_write_steering_A,write_steering_A,clover_with_buffering_A)



ax3.set_title('50% Writes')


####################### Write Only
avg_ops=[357708,516197,617516,686380,735140,677488,538924,367619,548668,553904,]
threads=[7,14,28,56,112,168,224,280,336,392,]
std=[0,0,0,0,0,0,0,0,0,0,]
clover_with_buffering_W={"ops": avg_ops,"threads": threads, "err": std}
avg_ops=[866916,1639656,3114720,5418308,9386624,12991886,14967071,17022698,17165892,17432716,]                                                                                                                     
threads=[7,14,28,56,112,168,224,280,336,392,]                                                                                                                                                                      
std=[0,0,0,0,0,0,0,0,0,0,] 
write_steering_W= {"ops": avg_ops,"threads": threads, "err": std}       
avg_ops=[864548,1637644,3114852,2247666,9386053,13012869,15285964,16579671,16845081,17550776,]
threads=[7,14,28,56,112,168,224,280,336,392,]
std=[0,0,0,0,0,0,0,0,0,0,]
read_write_steering_W= {"ops": avg_ops,"threads": threads, "err": std}  

tput_err(ax4,read_write_steering_W,write_steering_W,clover_with_buffering_W)
ax4.set_title('100% Writes')

plt.tight_layout()
save_fig(plt)