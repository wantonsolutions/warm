import matplotlib
import matplotlib.pyplot as plt
from statistics import mean
import numpy as np

fig, (ax1, ax2, ax3, ax4) = plt.subplots(1,4, figsize=(20,4))
#labels =  ['2', '4', '8', '16', '32', '48', '64']
#labels =  [16,24,32,40]
#labels=[1,2,4,8,16,24,32,40,]
labels=[1,2,4,8,16,24,32,40,48,56,64,72,80,]
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


    core_marker_x=[40,40]
    ymax=clover_with_buffering[7]/2
    core_marker_y=[-1,ymax]
    ax.plot(core_marker_x,core_marker_y,color='k',linestyle='--')
    ax.text(core_marker_x[1]+2,core_marker_y[1],"cores per NUMA")

    ax.set_ylim(bottom=0, top=12)
    ax.set_xlim(left=0, right=labels[len(labels)-1]+1)
    ax.legend(loc='upper left', ncol=1)
    ax.set_xlabel('Threads')


figure_name='hero_128'

#[186907,362862,678406,1135524,1753723,2058595,2138879]





####################### YCSB C
clover_with_buffering_C=[301523,572472,1126605,2215649,4208508,5992526,7666448,9258056,10370666,10236726,8417438,7053752,6558155,]
write_steering_C       =[301949,572372,1119388,2209585,4186171,5980338,7678287,9249756,10278597,10279862,8408234,7100675,6528895,]
read_write_steering_C  =[301894,569408,1126890,2215627,4201272,5989607,7657016,9168463,10247452,10270437,8435722,7079993,6538316,]
static_plot_attributes(ax1,read_write_steering_C,write_steering_C,clover_with_buffering_C)

ax1.set_title('0% Writes')
ax1.set_ylabel('MOPS')
#ax1.set_ylim(top=350)








####################### YCSB B
clover_with_buffering_B= [287553,555438,1056215,2053621,3846589,5351497,6729319,7927539,8651719,8632893,7671043,6690115,]
write_steering_B=        [287244,556748,1055824,2053968,3858034,5351325,6754062,7978329,8767726,8726396,7688501,]
read_write_steering_B=   [285128,555408,1057671,2073712,3936808,5530256,7026695,8440366,9369218,9272922,7927868,]
static_plot_attributes(ax2,read_write_steering_B,write_steering_B,clover_with_buffering_B)
ax2.set_title('5% Writes')
#ax1.set_ylim(top=350)


####################### YCSB A
#read_write_steering_A=[185761,368570,548196,724079,902446,1064490,1226965,1385449,1551584,1708075,1853088,2006774,2152950,2286119,2439318,2590177,2690985,2818385,2963478,3090369,2585444,3233792,3245172,2894584,2783820,3276348,3044378,3338028,3430141,3442372,3277753,3290072,3438824,3300230,3276828,3330956,2916299,2480775,2546719,2536838,]








clover_with_buffering_A= [211245,375094,704901,1219091,1895241,2092248,2201368,2315214,2277974,2244648,1857490,1444406,]
write_steering_A=        [192530,380541,733192,1357879,2322786,2451783,2458734,2444690,2405967,2329508,2037702,]
read_write_steering_A=   [192530,382863,744338,1435372,2564948,3333071,4105436,4737992,4774200,4778120,4833263,]
#read_write_steering_A=[2573164,3246444,3265343,2542510,]
#write_steering_A=[2358382,2954014,3205494,2476648,]
#clover_with_buffering_A=[1953973,2266736,2398964,2335885,]
static_plot_attributes(ax3,read_write_steering_A,write_steering_A,clover_with_buffering_A)
ax3.set_title('50% Writes')







####################### Write Only
clover_with_buffering_W= [146124,269535,482655,791754,1052663,1068134,1121467,1126608,1120757,1092931,]
write_steering_W=        [145972,275713,538980,1029151,1645954,2105248,2429456,2672887,2490711,2386695,2391374,]
read_write_steering_W=   [145738,280188,537025,1025614,1637521,2127753,2399304,2653379,2446373,2380150,2402088,]
static_plot_attributes(ax4,read_write_steering_W,write_steering_W,clover_with_buffering_W)
ax4.set_title('100% Writes')

plt.tight_layout()
#ax1.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')
#plt.show()