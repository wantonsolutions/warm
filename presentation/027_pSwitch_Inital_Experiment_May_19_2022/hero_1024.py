import matplotlib
import matplotlib.pyplot as plt
from statistics import mean
import numpy as np

fig, (ax1, ax2, ax3, ax4) = plt.subplots(1,4, figsize=(20,4))
#labels =  ['2', '4', '8', '16', '32', '48', '64']
#labels =  [16,24,32,40]
#labels=[1,2,4,8,16,24,32,40,]
#labels=[1,2,4,8,16,24,32,40,48,56,64,72,80,]
labels=[1,2,4,8,16,32]
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

    if len(read_write_steering) > 0:
        perfect = [(read_write_steering[0]) * i  for i in labels]
        #ax.plot(labels,perfect, color=cns_color, label="perfect")


    # core_marker_x=[40,40]
    # ymax=clover_with_buffering[int(len(clover_with_buffering)/2)]/2
    # core_marker_y=[-1,ymax]
    # ax.plot(core_marker_x,core_marker_y,color='k',linestyle='--')
    # ax.text(core_marker_x[1]+2,core_marker_y[1],"cores per NUMA")

    ax.set_ylim(bottom=0, top=5)
    ax.set_xlim(left=0, right=labels[len(labels)-1]+1)
    ax.legend(loc='upper left', ncol=1)
    ax.set_xlabel('Threads')


figure_name='hero_1024'

#[186907,362862,678406,1135524,1753723,2058595,2138879]





####################### YCSB C
clover_with_buffering_C=[284982,532726,975968,1618353,3259938,4149006,]
write_steering_C       =[282508,531553,1003209,1649196,3241936,4149509,]
read_write_steering_C  =[282225,529317,975038,1631322,3228707,4148398,]

static_plot_attributes(ax1,read_write_steering_C,write_steering_C,clover_with_buffering_C)

ax1.set_title('0% Writes')
ax1.set_ylabel('MOPS')






####################### YCSB B
clover_with_buffering_B= [267211,505542,952764,1701429,3150543,4019492,]
write_steering_B=        [269074,508839,953730,1702045,3156658,4009522,]
read_write_steering_B=   [269657,506915,960076,1711132,3220064,4254933,]
static_plot_attributes(ax2,read_write_steering_B,write_steering_B,clover_with_buffering_B)
ax2.set_title('5% Writes')
#ax1.set_ylim(top=350)


####################### YCSB A

clover_with_buffering_A= [182922,352918,662821,1127265,1728929,2019689,]
write_steering_A=        [181177,358626,687120,1248678,2092278,2364334,]
read_write_steering_A=   [182500,362168,694458,1320697,2328253,3699575,]
#read_write_steering_A=[2573164,3246444,3265343,2542510,]
#write_steering_A=[2358382,2954014,3205494,2476648,]
#clover_with_buffering_A=[1953973,2266736,2398964,2335885,]
static_plot_attributes(ax3,read_write_steering_A,write_steering_A,clover_with_buffering_A)
ax3.set_title('50% Writes')

####################### Write Only
clover_with_buffering_W=[138791,255945,451467,746670,999091,1060887,]
write_steering_W=[]        
read_write_steering_W= []  
static_plot_attributes(ax4,read_write_steering_W,write_steering_W,clover_with_buffering_W)
ax4.set_title('100% Writes')

plt.tight_layout()
#ax1.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')
#plt.show()
















