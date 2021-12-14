import matplotlib
import matplotlib.pyplot as plt
from statistics import mean
import numpy as np
import matplotlib.font_manager as font_manager

plt.rcParams.update({'font.size': 12})

fig, (ax1, ax2, ax3) = plt.subplots(1,3, figsize=(15,4))
#labels =  ['2', '4', '8', '16', '32', '48', '64']
labels =  [2,4,8,16,32,48,64]

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

default_line_width=3
default_marker_size=10


def div_thousand (list):
    return [val /1000.0 for val in list]

def static_plot_attributes(ax,cns_replacement,qp_mapping,read_write_steering,write_steering, clover_with_buffering):
    #convert to KOPS/s 
    cns_replacement=div_thousand(cns_replacement)
    qp_mapping=div_thousand(qp_mapping)
    read_write_steering=div_thousand(read_write_steering)
    write_steering=div_thousand(write_steering)
    clover_with_buffering=div_thousand(clover_with_buffering)

    #plot
    ax.plot(labels[:len(clover_with_buffering)],clover_with_buffering,label=default_clover_label,marker=default_clover_marker, color=default_clover_color,linewidth=default_line_width,markersize=default_marker_size)
    ax.plot(labels[:len(write_steering)],write_steering,label=write_steering_label,marker=write_steering_marker,color=write_steering_color,linewidth=default_line_width,markersize=default_marker_size)
    ax.plot(labels[:len(read_write_steering)],read_write_steering,label=read_write_steering_label, marker=read_write_steering_marker, color=read_write_steering_color,linewidth=default_line_width,markersize=default_marker_size)
    #ax.plot(labels[:len(qp_mapping)],qp_mapping,label=qp_mapping_label,marker=qp_mapping_marker,color=qp_mapping_color,linewidth=default_line_width,markersize=default_marker_size)
    #ax.plot(labels[:len(cns_replacement)],cns_replacement,label=cns_label,marker=cns_marker,color=cns_color,linewidth=default_line_width,markersize=default_marker_size)


figure_name='full_system_performance'
####################### YCSB B
cns_replacement_B=[174447,306051,526629,832772,1183346,1404188,1565973,]
qp_mapping_B=[186479,303590,532564,838632,1150512,1464160,1611634]
read_write_steering_B=[219591,360438,664283,1143081,1839693,2370522,2700739,]
write_steering_B=[191925,346629,631555,1057324,1678926,2047208,2370244,]
clover_with_buffering_B=[197762,348117,618548,1086834,1673550,2033202,2303143,]


static_plot_attributes(ax1,cns_replacement_B,qp_mapping_B,read_write_steering_B,write_steering_B,clover_with_buffering_B)
ax1.set_title('5% Writes')
ax1.set_ylabel('KOPS')
#ax1.legend(loc='upper left', ncol=2)

font = font_manager.FontProperties(
                                   style='normal', size=10)

ax1.legend(loc='upper left', ncol=2, prop=font)
#ax1.legend(loc='lower right', ncol=2, prop=font)
#ax1.set_ylim(top=350)

####################### YCSB A
cns_replacement_A=[124468,228639,387401,602844,895405,1019713,1137059]
qp_mapping_A=[123485,226459,379778,603110,878468,1004016,1151613,]
read_write_steering_A=[142858,265347,450168,744408,1150055,1348643,1543334,]
write_steering_A=[133011,242798,396038,632241,900990,969858,947018,]
clover_with_buffering_A=[109437,192177,315501,463162,567417,544734,545840,]


ax2.set_title('50% Writes')
ax2.set_xlabel('Threads')
static_plot_attributes(ax2,cns_replacement_A,qp_mapping_A,read_write_steering_A,write_steering_A,clover_with_buffering_A)

####################### Write Only
cns_replacement_W=[95169,172095,286039,460738,675995,817067,845466]
qp_mapping_W=[99037,169357,286712,456699,644856,837993,mean([859407,851275,818586,])]
read_write_steering_W=[108699,196560,344495,547073,791945,914179,1015296,]
write_steering_W=[112570,207525,353610,565764,830269,983598,1086724,]
clover_with_buffering_W=[74546,121660,200582,279802,315018,297374,278763,]

422233



static_plot_attributes(ax3,cns_replacement_W,qp_mapping_W,read_write_steering_W,write_steering_W,clover_with_buffering_W)
ax3.set_title('100% Writes')

plt.tight_layout()
#ax1.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')
#plt.show()