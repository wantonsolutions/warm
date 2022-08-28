import matplotlib.pyplot as plt
import numpy as np


def plot_optimal(ax1, optimal, x_space, bars, width):

    line_style=":"
    line_color="g"
    for i in range(bars):
        half_diameter=(width/2)*bars
        left = x_space[i]-half_diameter
        right = x_space[i]+half_diameter
        ax1.plot([left,right],[optimal[i],optimal[i]],linestyle=line_style,color=line_color,linewidth=2)

    ax1.plot([],[],linestyle=line_style,color=line_color,label="Optimal")
    

cns_color='#00702eff'                     #indigo
qp_mapping_color='#9470b9ff'              #ruby
read_write_steering_color='#cf243cff'     #alice
write_steering_color='#ed7d31ff'          #kelly
default_clover_color='#8caff6ff'          #Coral 

# labels = ['50', '5', '100'] 
# optimal = [1314, 1254, 1380]
# read_write = [1300, 1253, 1343]
# write = [3305, 1499, 1341]
# clover = [2457, 1491, 3091]

labels = [ '5','50', '100'] 
optimal = [ 1254,1314, 1380]
# read_write = [ 1253,1300, 1343]
# write = [ 1499,3305, 1341]
# clover = [ 1491,2457, 3091]

clover=[(934822,  1725440748),(12619792,3588778832),(929800,  1755707042),(901140,  1676371302)]
write=[(40399508,4282772272),(13405036,3588778832),(1763680, 3344320676),(1723808, 3352479068)]
read_write=[(39750630,4221172704),(35235112,3999323710),(27144320,3188139084),(17071776,2198105420)]

clover=bytes_per_op(clover)
write=bytes_per_op(write)

x = np.arange(len(labels))  # the label locations
width = 0.2  # the width of the bars

div=1
plt.rcParams.update({'font.size': 16})
fig, ax = plt.subplots(figsize=(8,4), dpi=80)
plot_optimal(ax,optimal,x,3,width)
rects2 = ax.bar(x - width, clover, width, label='Clover',color=default_clover_color,edgecolor='k')
rects1 = ax.bar(x, write, width, label='Write',color=write_steering_color,edgecolor='k')
rects0 = ax.bar(x + width, read_write, width, label='Write+Read',color=read_write_steering_color,edgecolor='k')


# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('Bytes Per Operation')
ax.set_xticks(x)
ax.set_xticklabels(labels)
ax.set_xlabel("Write Ratio")

handles, labels = ax.get_legend_handles_labels()
order = [1,2,3,0]
ax.legend([handles[idx] for idx in order],[labels[idx] for idx in order])

#ax.legend()


#ax.bar_label(rects1, padding=3)
#//ax.bar_label(rects2, padding=3)
figure_name="bandwidth_reduction"
plt.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')

