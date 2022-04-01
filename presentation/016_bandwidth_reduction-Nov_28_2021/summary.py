import matplotlib.pyplot as plt
import numpy as np


cns_color='#00702eff'                     #indigo
qp_mapping_color='#9470b9ff'              #ruby
read_write_steering_color='#cf243cff'     #alice
write_steering_color='#ed7d31ff'          #kelly
default_clover_color='#8caff6ff'          #Coral 

labels = ['50', '5', '100'] 
optimal = [1314, 1254, 1380]
read_write = [1300, 1253, 1343]
write = [3305, 1499, 1341]
clover = [2457, 1491, 3091]

x = np.arange(len(labels))  # the label locations
width = 0.2  # the width of the bars

div=1

fig, ax = plt.subplots()
rects0 = ax.bar(x - (width + width/2), optimal, width, label='Optimal',color='g')
rects1 = ax.bar(x - width/2, read_write, width, label='Read+Write',color=read_write_steering_color)
rects2 = ax.bar(x + width/2, write, width, label='Write',color=write_steering_color)
rects3 = ax.bar(x + (width+ width/2), clover, width, label='Clover',color=default_clover_color)

# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('Average Bytes Per Operation')
ax.set_title('Bytes Per Operation')
ax.set_xticks(x)
ax.set_xticklabels(labels)
ax.set_xlabel("Write Ratio")
ax.legend()


#ax.bar_label(rects1, padding=3)
#//ax.bar_label(rects2, padding=3)
figure_name="summary"
plt.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')