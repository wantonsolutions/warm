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


figure_name='hero_1024'

avg_ops=[1]
threads=[1]
std=[1]


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
#ax1.set_ylim(top=350)



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
#ax1.set_ylim(top=350)


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
#ax1.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')
#plt.show()