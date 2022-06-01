import matplotlib
import matplotlib.pyplot as plt
from statistics import mean
import numpy as np

fig, (ax1, ax2, ax3, ax4) = plt.subplots(1,4, figsize=(20,4))
#labels =  ['2', '4', '8', '16', '32', '48', '64']
#labels =  [16,24,32,40]
#labels=[1,2,4,8,16,24,32,40,]
#labels=[1,2,4,8,16,24,32,40,48,56,64,72,80,]
#labels=[2,4,8,16,32,48,64,80,]
labels=[3,6,12,24,48,72,96,120,]                                                                                                                                                                                  
#labels=[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,]

cns_label='CAS -> Write'
qp_mapping_label='QP mapping'
read_write_steering_label='Write + Read'
write_steering_label='Write'
default_clover_label='Clover'

cns_marker='x'
qp_mapping_marker='s'
read_write_steering_marker='+'
write_steering_marker="*"
default_clover_marker='.'

cns_color='#00702eff'                     #indigo
qp_mapping_color='#9470b9ff'              #ruby
read_write_steering_color='#cf243cff'     #alice
write_steering_color='#ed7d31ff'          #kelly
default_clover_color='#8caff6ff'          #Coral 


def div_mil (list):
    return [val /1000000.0 for val in list]

def scale_mil(measurement):
    measurement["ops"]=div_mil(measurement["ops"])
    measurement["err"]=div_mil(measurement["err"])

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

def plot_max_improvement_2(ax, m1, m2):
    m1_ops=[]
    m2_ops=[]
    threads=[]
    
    for thread in m1["threads"]:
        if thread in m2["threads"]:
            threads.append(thread)
            m1_ops.append(m1["ops"][m1["threads"].index(thread)])
            m2_ops.append(m2["ops"][m2["threads"].index(thread)])


    improvement=[ a/b for a ,b in zip(m1_ops, m2_ops)]
    max_improvement=max(improvement)
    max_index=improvement.index(max_improvement)

    max_improvement=round(max_improvement,2)
    ax.text(threads[len(threads) -2], m1_ops[len(m1_ops)-1]/2,str(max_improvement)+"x")
    x_1=threads[max_index]
    x=[x_1,x_1]
    y=[m1_ops[max_index],m2_ops[max_index]]

    ax.plot(x,y,color='k',linestyle='--')






def tput_err(ax,rws,ws, clover):
    scale_mil(rws)
    scale_mil(ws)
    scale_mil(clover)
    ax.errorbar(rws["threads"],rws["ops"],rws["err"],label=read_write_steering_label, marker=read_write_steering_marker, color=read_write_steering_color)
    ax.errorbar(ws["threads"],ws["ops"],ws["err"],label=write_steering_label,marker=write_steering_marker,color=write_steering_color)
    ax.errorbar(clover["threads"],clover["ops"],clover["err"],label=default_clover_label,marker=default_clover_marker, color=default_clover_color)

    plot_max_improvement_2(ax,rws,clover)

    #ax.set_yscale("log")
    ax.legend(loc='upper left', ncol=1)
    ax.set_xlabel('Threads')

    

def static_plot_attributes(ax,read_write_steering,write_steering, clover_with_buffering):
    #convert to KOPS/s 
    read_write_steering=div_mil(read_write_steering)
    write_steering=div_mil(write_steering)
    clover_with_buffering=div_mil(clover_with_buffering)

    #plot
    ax.plot(labels[:len(clover_with_buffering)],clover_with_buffering,label=default_clover_label,marker=default_clover_marker, color=default_clover_color)
    ax.plot(labels[:len(write_steering)],write_steering,label=write_steering_label,marker=write_steering_marker,color=write_steering_color)
    ax.plot(labels[:len(read_write_steering)],read_write_steering,label=read_write_steering_label, marker=read_write_steering_marker, color=read_write_steering_color)

    #add some decarations
    # if len(read_write_steering) > 0:
    #     perfect = [(read_write_steering[0]) * i  for i in labels]
    #     #ax.plot(labels,perfect, color=cns_color, label="perfect")

    plot_max_improvement(ax,read_write_steering,clover_with_buffering)


        
    # core_marker_x=[40,40]
    # ymax=clover_with_buffering[7]/2
    # core_marker_y=[-1,ymax]
    # ax.plot(core_marker_x,core_marker_y,color='k',linestyle='--')
    # ax.text(core_marker_x[1]+2,core_marker_y[1],"cores per NUMA")

    # ax.set_ylim(bottom=0, top=12)
    # ax.set_xlim(left=0, right=labels[len(labels)-1]+1)
    ax.legend(loc='upper left', ncol=1)
    ax.set_xlabel('Threads')


figure_name='hero_128'

avg_ops=[1]
threads=[1]
std=[1]


####################### YCSB C
clover_with_buffering_C= {"ops": avg_ops,"threads": threads, "err": std}

write_steering_C=        {"ops": avg_ops,"threads": threads, "err": std}

read_write_steering_C=   {"ops": avg_ops,"threads": threads, "err": std}

tput_err(ax1,read_write_steering_C,write_steering_C,clover_with_buffering_C)

ax1.set_title('0% Writes')
ax1.set_ylabel('MOPS')
#ax1.set_ylim(top=350)



####################### YCSB B

avg_ops=[779646,1497432,2801172,4611903,7548014,9250425,9634269,9094907,]
threads=[3,6,12,24,48,72,96,120,]
std=[2048,3863,5093,71521,7035,111950,8870,8870,]
clover_with_buffering_B= {"ops": avg_ops,"threads": threads, "err": std}

write_steering_B=        {"ops": avg_ops,"threads": threads, "err": std}

read_write_steering_B=   {"ops": avg_ops,"threads": threads, "err": std}

tput_err(ax2,read_write_steering_B,write_steering_B,clover_with_buffering_B)
ax2.set_title('5% Writes')
#ax1.set_ylim(top=350)


####################### YCSB A

avg_ops=[350643,1256167,1400842,1389080,1394428,]
threads=[3,48,72,96,120,]
std=[31567,35467,26267,1639,1639,]
clover_with_buffering_A={"ops": avg_ops,"threads": threads, "err": std}

avg_ops=[463486,811052,1343690,1962017,2337676,2547328,2337104,1833293,]
threads=[3,6,12,24,48,72,96,120,]
std=[741,24033,4429,63569,1879,3158,75781,75781,]
write_steering_A={"ops": avg_ops,"threads": threads, "err": std}

avg_ops=[579289,1100143,2114343,4004643,7255844,9522098,10962757,12766810,]
threads=[3,6,12,24,48,72,96,120,]
std=[13700,3071,69168,17292,54123,600205,92184,92184,]
read_write_steering_A={"ops": avg_ops,"threads": threads, "err": std}

tput_err(ax3,read_write_steering_A,write_steering_A,clover_with_buffering_A)
ax3.set_title('50% Writes')


####################### Write Only
avg_ops=[211678,359252,522902,632002,660740,731444,724635,714810,]
threads=[3,6,12,24,48,72,96,120,]
std=[0,0,0,0,0,0,0,0,]
clover_with_buffering_W={"ops": avg_ops,"threads": threads, "err": std}

avg_ops=[423794,806729,1545074,2894566,3988856,6067806,6916010,7439997,]
threads=[3,6,12,24,48,72,96,120,]
std=[0,0,0,0,0,0,0,0,]
write_steering_W= {"ops": avg_ops,"threads": threads, "err": std}       

avg_ops=[424198,806856,1543887,2734326,4756884,6070445,6746916,7460868,]
threads=[3,6,12,24,48,72,96,120,]
std=[0,0,0,0,0,0,0,0,]
read_write_steering_W= {"ops": avg_ops,"threads": threads, "err": std}  

tput_err(ax4,read_write_steering_W,write_steering_W,clover_with_buffering_W)
ax4.set_title('100% Writes')

plt.tight_layout()
#ax1.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')
#plt.show()