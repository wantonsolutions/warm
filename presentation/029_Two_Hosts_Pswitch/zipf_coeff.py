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


    plot_max_improvement(ax,read_write_steering,clover_with_buffering)

    ax.legend(loc='lower left', ncol=1)
    ax.set_xlabel('Zipf Coeff')
    #ax.set_ylim(bottom=0, top=5)


figure_name='zipf_coeff'

#[186907,362862,678406,1135524,1753723,2058595,2138879]





####################### 1024 YCSB A
clover_with_buffering_A_1024= []
write_steering_A_1024       = []
read_write_steering_A_1024  = []
static_plot_attributes(ax1,read_write_steering_A_1024,write_steering_A_1024,clover_with_buffering_A_1024)

ax1.set_title('Zipf Coeff 50% Writes 1024 bytes')
ax1.set_ylabel('MOPS')

####################### 128 YCSB A
clover_with_buffering_A_128 =[5337892,3236486,2320588,1775090,1394630,1054035,970016,760301,656326,582746,]
write_steering_A_128       = [6992062,5287498,3067801,2389720,1887098,1542794,1219392,1129354,901714,886260,]
read_write_steering_A_128  = [14209284,13898312,13694586,13188586,12962634,11937014,11706232,10673222,9149111,8860992,]
static_plot_attributes(ax2,read_write_steering_A_128,write_steering_A_128,clover_with_buffering_A_128)

ax2.set_title('Zipf Coeff 50% Writes 128 bytes')
ax2.set_ylabel('MOPS')
#ax1.set_ylim(top=350)



plt.tight_layout()
#ax1.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')
#plt.show()