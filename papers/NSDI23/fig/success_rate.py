

import matplotlib.pyplot as plt
import numpy as np


cns_color='#00702eff'                     #indigo
qp_mapping_color='#9470b9ff'              #ruby
read_write_steering_color='#cf243cff'     #alice
write_steering_color='#ed7d31ff'          #kelly
default_clover_color='#8caff6ff'          #Coral 

plt.rcParams.update({'font.size': 16})

labels=['1','6','12','24','48','96','144','192','240']
successes=[(4,3),(496554, 1151907),(715313, 2075927),(953092, 3962723),(1170025,7781642),(1502067,14889731),(1470896,20529916),(1350399,23514602),(1152454,23280662)]

success_percentage=[(a[0]/4)/(a[1]/3) * 100 for a in successes]
# write_percentage=[ 100 - (a / b * 100) for a,b in zip(writes_failed, writes)]
# read_percentage=[ 100- (a / b * 100) for a,b in zip(reads_failed, reads)]

print(success_percentage)
x = np.arange(len(labels))  # the label locations
width = 0.75  # the width of the bars

div=1

fig, ax = plt.subplots()
rects1 = ax.bar(x, success_percentage, width, label='Operation Success',color=default_clover_color,edgecolor='k')

# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('success rate')
#ax.set_title('Operation Success Rate')
ax.set_xticks(x)
ax.set_xticklabels(labels)
ax.set_xlabel("client threads")
#ax.legend()


#ax.bar_label(rects1, padding=3)
#//ax.bar_label(rects2, padding=3)
figure_name="success_rate"
plt.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')


