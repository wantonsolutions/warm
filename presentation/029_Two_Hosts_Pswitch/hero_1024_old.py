import matplotlib
import matplotlib.pyplot as plt
from statistics import mean
import numpy as np

fig, (ax1, ax2, ax3, ax4) = plt.subplots(1,4, figsize=(20,4))
#labels =  ['2', '4', '8', '16', '32', '48', '64']
#labels =  [16,24,32,40]
#labels=[1,2,4,8,16,24,32,40,]
#labels=[1,2,4,8,16,24,32,40,48,56,64,72,80,]
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

#[186907,362862,678406,1135524,1753723,2058595,2138879]







####################### YCSB C
clover_with_buffering_C=[761209,1314880,1.00000,4126310,4160990,4160730,4159610,4118380,]
write_steering_C       =[785950,1368174,2432700,4111170,4137845,4162520,4153850,4144670,]
read_write_steering_C  =[765375,1316720,2217471,4115800,4145990,4140886,4145938,4144720,]
static_plot_attributes(ax1,read_write_steering_C,write_steering_C,clover_with_buffering_C)

ax1.set_title('0% Writes')
ax1.set_ylabel('MOPS')
#ax1.set_ylim(top=350)







####################### YCSB B
clover_with_buffering_B= [692780,1294390,2349340,3584970,3429220,3253365,3170020,3017320,]
write_steering_B=        [698240,1298398,2365770,3593630,3417067,3278780,3201010,2988573,]
read_write_steering_B=   [752290,1.0000,1.0000,4259660,4286200,4279760,4224310,4107790,]
static_plot_attributes(ax2,read_write_steering_B,write_steering_B,clover_with_buffering_B)
ax2.set_title('5% Writes')
#ax1.set_ylim(top=350)


####################### YCSB A






clover_with_buffering_A= [323540,1.0000,1.0000,1481250,1764046,1248610,1.0000,1572170,] 
write_steering_A=        [430448,809920,1372240,2030150,1780968,150740,1027950,838560,]
read_write_steering_A=   [523160,1017950,1820300,3324580,5391660,6312670,6572420,6366890,]
#[346529,675840,1295270,2437920,4081846,5412910,6139740,6560150,]
#read_write_steering_A=[2573164,3246444,3265343,2542510,]
#write_steering_A=[2358382,2954014,3205494,2476648,]
#clover_with_buffering_A=[1953973,2266736,2398964,2335885,]
static_plot_attributes(ax3,read_write_steering_A,write_steering_A,clover_with_buffering_A)
ax3.set_title('50% Writes')









####################### Write Only
clover_with_buffering_W= [210900,348990,629710,630400,691980,983740,657157,865750,]
write_steering_W=        [346138,533240,1018190,1635800,2822296,3790210,1.0000,3641550,]
read_write_steering_W=   [346720,509160,1284210,1643110,2620100,3098205,3575880,1.0000]
static_plot_attributes(ax4,read_write_steering_W,write_steering_W,clover_with_buffering_W)
ax4.set_title('100% Writes')

plt.tight_layout()
#ax1.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')
#plt.show()