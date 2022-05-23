import matplotlib
import matplotlib.pyplot as plt
from statistics import mean
import numpy as np

fig, (ax1, ax2) = plt.subplots(2,1, figsize=(10,8))
labels=[0.00, 0.60, 0.80, 0.90, 1.00, 1.10, 1.20, 1.30, 1.40, 1.50]


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



    ax.legend(loc='lower left', ncol=1)
    ax.set_xlabel('Zipf Coeff')
    ax.set_ylim(bottom=0, top=5)


figure_name='zipf_coeff'

#[186907,362862,678406,1135524,1753723,2058595,2138879]





####################### 1024 YCSB A
clover_with_buffering_A_1024=[2583011,2332167,1835258,1552706,1280321,1081449,930896,809385,710825,631861,]
write_steering_A_1024       = [2681447,2444335,2267648,2004352,1656089,1356620,1132836,952161,814560,712372,]
read_write_steering_A_1024  = [3797010,3761832,3709804,3668844,3607512,3532414,3439236,3309403,3148357,2951064,]
static_plot_attributes(ax1,read_write_steering_A_1024,write_steering_A_1024,clover_with_buffering_A_1024)

ax1.set_title('Zipf Coeff 50% Writes 1024 bytes')
ax1.set_ylabel('MOPS')

####################### 128 YCSB A
clover_with_buffering_A_128=[2597568,2276844,1818916,1525250,1247858,1044692,910730,799066,712420,632900,]
write_steering_A_128       = [2706328,2396735,2398005,2125747,1992119,1647973,1544402,1316489,1219969,1086440,]
read_write_steering_A_128  = [4233677,4122829,4036986,3982922,3910302,3738081,3611958,3430678,3187445,2916099,]
static_plot_attributes(ax2,read_write_steering_A_128,write_steering_A_128,clover_with_buffering_A_128)

ax2.set_title('Zipf Coeff 50% Writes 128 bytes')
ax2.set_ylabel('MOPS')
#ax1.set_ylim(top=350)



plt.tight_layout()
#ax1.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')
#plt.show()