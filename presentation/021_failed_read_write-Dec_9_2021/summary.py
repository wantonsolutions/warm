import matplotlib.pyplot as plt
import numpy as np


cns_color='#00702eff'                     #indigo
qp_mapping_color='#9470b9ff'              #ruby
read_write_steering_color='#cf243cff'     #alice
write_steering_color='#ed7d31ff'          #kelly
default_clover_color='#8caff6ff'          #Coral 

labels = ['1', '2', '4', '8', '16', '32', '64', '128'] 

reads = [1,150603,259278,409865,648005,760637,865991,997893]
reads_failed = [0,39192,73638,129558,237592,308883,366726,531099]
writes = [1,139788,230340,369264,586237,848302,1471518,1686381]
writes_failed = [0,33569,60047,106092,200484,412579,990907,1236584]

write_percentage=[ 100 - (a / b * 100) for a,b in zip(writes_failed, writes)]
read_percentage=[ 100- (a / b * 100) for a,b in zip(reads_failed, reads)]

print(write_percentage)

x = np.arange(len(labels))  # the label locations
width = 0.4  # the width of the bars

div=1

fig, ax = plt.subplots()
rects1 = ax.bar(x - width/2, read_percentage, width, label='Read',color=read_write_steering_color,edgecolor='k')
rects2 = ax.bar(x + width/2, write_percentage, width, label='Write',color=write_steering_color,edgecolor='k')

# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('success rate')
#ax.set_title('Operation Success Rate')
ax.set_xticks(x)
ax.set_xticklabels(labels)
ax.set_xlabel("client threads")
ax.legend()


#ax.bar_label(rects1, padding=3)
#//ax.bar_label(rects2, padding=3)
figure_name="success_rate"
plt.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')