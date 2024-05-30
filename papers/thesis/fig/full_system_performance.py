import matplotlib
import matplotlib.pyplot as plt
from statistics import mean
import numpy as np
from common import *

plt.rcParams.update({'font.size': 16})
default_line_width=3
default_marker_size=10

fig, (ax1, ax2, ax3, ax4) = plt.subplots(1,4, figsize=(20,4))

def tput_err(ax,rws,ws, clover, fusee=None, sherman=None):
    scale_mil(rws)
    scale_mil(ws)
    scale_mil(clover)
    ax.errorbar(clover["threads"],clover["ops"],clover["err"],label=default_clover_label,marker=default_clover_marker, color=default_clover_color,linewidth=default_line_width, markersize=default_marker_size )
    ax.errorbar(ws["threads"],ws["ops"],ws["err"],label=write_steering_label,marker=write_steering_marker,color=write_steering_color,linewidth=default_line_width, markersize=default_marker_size )
    ax.errorbar(rws["threads"],rws["ops"],rws["err"],label=read_write_steering_label, marker=read_write_steering_marker, color=read_write_steering_color, linewidth=default_line_width, markersize=default_marker_size)
    if fusee is not None:
        scale_mil(fusee)
        ax.errorbar(fusee["threads"],fusee["ops"],fusee["err"],label=fusee_label, marker=fusee_marker, color=fusee_color, linewidth=default_line_width, markersize=default_marker_size)
    if sherman is not None:
        scale_mil(sherman)
        ax.errorbar(sherman["threads"],sherman["ops"],sherman["err"],label=sherman_label, marker=sherman_marker, color=sherman_color, linewidth=default_line_width, markersize=default_marker_size)

    plot_max_improvement(ax,rws,clover,"threads")

    #ax.set_yscale("log")
    #ax.set_xlabel('Threads')



####################### YCSB C
avg_ops=[1829774,3577880,7008356,13684405,25656091,34720826,39142324,39713784,39970952,40600024,]
threads=[7,14,28,56,112,168,224,280,336,392,]
std=[0,0,0,0,0,0,0,0,0,0,]
clover_with_buffering_C= {"ops": avg_ops,"threads": threads, "err": std}
avg_ops=[1836632,3572209,7003510,13673092,25654763,34857976,39180515,39757292,39917704,40295512,]
threads=[7,14,28,56,112,168,224,280,336,392,]
std=[0,0,0,0,0,0,0,0,0,0,]
write_steering_C=        {"ops": avg_ops,"threads": threads, "err": std}
avg_ops=[1832205,3567680,7009575,13703432,25692824,34795712,39152679,39790996,39960749,40455924,]
threads=[7,14,28,56,112,168,224,280,336,392,]
std=[0,0,0,0,0,0,0,0,0,0,]
read_write_steering_C=   {"ops": avg_ops,"threads": threads, "err": std}

threads = [8, 16, 32, 64, 128, 256]
# avg_ops= [29502.2,125800.2,912565.2,6498447.2,12683884.25,10641378.6,8924234.4 ]
avg_ops=[ 0.159183,   0.2876015,  2.2447135, 12.025642,  16.322664,  16.160061 ]
avg_ops = [ s * 1000000 for s in avg_ops]

std=     [0,0,0,0,0,0]
fusee_C= {"ops": avg_ops,"threads": threads, "err": std}

avg_ops= [1596000.0, 3074000.0, 5784000.0, 10304000.0, 12781000.0, 12406000.0, 13009000.0]
threads= [5, 10, 20, 40, 80, 160, 200]
std=     [0,0,0,0,0,0,0]
sherman_C= {"ops": avg_ops,"threads": threads, "err": std}



# tput_err(ax1,read_write_steering_C,write_steering_C,clover_with_buffering_C, fusee_C, sherman_C)
tput_err(ax1,read_write_steering_C,write_steering_C,clover_with_buffering_C, fusee_C)

ax1.set_title('0% Writes')
ax1.set_ylabel('MOPS')
ax1.legend(loc='lower right', ncol=1, fontsize=12)

####################### YCSB B
avg_ops=[1487724,2655252,4818364,7666020,12365564,13979169,13716786,13427863,12837368,12382960,]
threads=[7,14,28,56,112,168,224,280,336,392,]
std=[0,0,0,0,0,0,0,0,0,0,]
clover_with_buffering_B= {"ops": avg_ops,"threads": threads, "err": std}
avg_ops=[1500840,2680607,4929508,7935485,12774234,14548308,14439633,14124372,13402903,13402903,]
threads=[7,14,28,56,112,168,224,280,336,392,]
std=[0,0,0,0,0,0,0,0,0,0,]
write_steering_B=        {"ops": avg_ops,"threads": threads, "err": std}

avg_ops=[1748703,3377649,6438944,12586564,21354096,28715113,34991652,34930195,35529061,35252996,]
threads=[7,14,28,56,112,168,224,280,336,392,]
std=[0,0,0,0,0,0,0,0,0,0,]
read_write_steering_B=   {"ops": avg_ops,"threads": threads, "err": std}





threads = [8, 16, 32, 64, 128, 256]
# avg_ops= [25471,96619,633937.5,4833325,9352250.4,7582265.6,7111248.8]
avg_ops=[ 0.049226,   0.190538,   1.4487765, 10.241494,  14.2780315, 14.2005985]
avg_ops = [ s * 1000000 for s in avg_ops]
std=     [0,0,0,0,0,0]
fusee_B= {"ops": avg_ops,"threads": threads, "err": std}

threads= [5, 10, 20, 40, 80, 160, 200]
avg_ops= [1467000.0, 2851000.0, 5380000.0, 9705000.0, 12653000.0, 11356000.0, 5818000.0]
std=     [0,0,0,0,0,0,0]
sherman_B= {"ops": avg_ops,"threads": threads, "err": std}

# tput_err(ax2,read_write_steering_B,write_steering_B,clover_with_buffering_B,fusee_B,sherman_B)
tput_err(ax2,read_write_steering_B,write_steering_B,clover_with_buffering_B,fusee_B)
ax2.set_title('5% Writes')
#ax1.set_ylim(top=350)


####################### YCSB A
avg_ops=[554788,877068,1120964,1312144,1509604,1450528,1226730,870612,1180566,1257288,]
threads=[7,14,28,56,112,168,224,280,336,392,]
std=[0,0,0,0,0,0,0,0,0,0,]
clover_with_buffering_A={"ops": avg_ops,"threads": threads, "err": std}
avg_ops=[815460,1331156,1858488,2343504,2639795,2386740,2098574,1471908,1830812,1602349,]
threads=[7,14,28,56,112,168,224,280,336,392,]
std=[0,0,0,0,0,0,0,0,0,0,]
write_steering_A={"ops": avg_ops,"threads": threads, "err": std}
avg_ops=[1194607,2264604,4147258,8024861,13868472,19604656,24528677,28124268,28348060,28201180,]
threads=[7,14,28,56,112,168,224,280,336,392,]
std=[0,0,0,0,0,0,0,0,0,0,]
read_write_steering_A={"ops": avg_ops,"threads": threads, "err": std}

# threads= [5, 10, 20, 40, 80, 160, 200]
threads = [8, 16, 32, 64, 128, 256]
# avg_ops= [17501.4,70734.75,501113.4,3760888.8,7634511,6026668.4,5606705.8 ]
avg_ops=[ 0.0360255, 0.145604,   1.0589075,  7.5669515, 11.9509565, 11.9186975]
avg_ops = [ s * 1000000 for s in avg_ops]
std=     [0,0,0,0,0,0]
fusee_A = {"ops": avg_ops,"threads": threads, "err": std}

avg_ops= [846000.0, 1680000.0, 3144000.0, 4767000.0, 4623000.0, 794000.0, 510000.0]
threads= [5, 10, 20, 40, 80, 160, 200]
std=     [0,0,0,0,0,0,0]
sherman_A = {"ops": avg_ops,"threads": threads, "err": std}

# tput_err(ax3,read_write_steering_A,write_steering_A,clover_with_buffering_A,fusee_A,sherman_A)
tput_err(ax3,read_write_steering_A,write_steering_A,clover_with_buffering_A,fusee_A)



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
avg_ops=[864548,1637644,3114852,5417666,9386053,13012869,15085964,16979671,16945081,17550776,]
threads=[7,14,28,56,112,168,224,280,336,392,]
std=[0,0,0,0,0,0,0,0,0,0,]
read_write_steering_W= {"ops": avg_ops,"threads": threads, "err": std}  


# threads= [5,10,20,40,80,160,200]
threads = [8, 16, 32, 64, 128, 256]
# avg_ops= [18148.4,72549.25,441385.25,2539115.8,5442831.2,6340093.2,6736537.]
avg_ops=[0.03022,   0.123381,  0.868236,  6.1967235, 9.7452085, 9.7876635]
avg_ops = [ s * 1000000 for s in avg_ops]
std=     [0,0,0,0,0,0]
fusee_W = {"ops": avg_ops,"threads": threads, "err": std}

avg_ops= [573000.0, 1148000.0, 2056000.0, 2477000.0, 2367000.0, 392000.0, 171000.0]
threads= [5, 10, 20, 40, 80, 160, 200]
std=     [0,0,0,0,0,0,0]
sherman_W = {"ops": avg_ops,"threads": threads, "err": std}

# tput_err(ax4,read_write_steering_W,write_steering_W,clover_with_buffering_W,fusee_W,sherman_W)
tput_err(ax4,read_write_steering_W,write_steering_W,clover_with_buffering_W,fusee_W)
ax4.set_title('100% Writes')

textvar = fig.text(0.515,0.00, "Threads", ha='center', size=18)
plt.tight_layout()
save_fig(plt)
textvar.remove()
axs = [ax1, ax2, ax3, ax4]
for ax in axs:
    ax.set_xlabel('Threads')
    ax.set_ylabel('MOPS')
    ax.legend(loc='lower right', ncol=1, fontsize=12)

save_figs(plt, fig, [ax1, ax2, ax3, ax4])