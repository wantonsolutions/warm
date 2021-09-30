import matplotlib
import matplotlib.pyplot as plt
import numpy as np


#labels = ['1', '2', '4', '8', '16', '24','32']


#labels = ['64' '48' '32' '16' '8' '4' '2' '1']
labels = ['64','32','16'] #,'8', '4', '2', '1']

memory_qp=[47854,48935,46696]






def div_thousand (list):
    return [val /1000.0 for val in list]


memory_qp=div_thousand(memory_qp)

treatment_label= 'memory queue pairs'
figure_name='QP_restriction'

x = np.arange(len(labels))  # the label locations

print(x)
width = 0.35  # the width of the bars

fig, ax = plt.subplots()
rects1 = ax.bar(x, memory_qp) #, width, label=treatment_label, color='tab:blue', hatch='\\')

# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('KOP/s')
#ax.set_title('Clover YCSB-A (50% write)')
ax.set_xticks(x)
ax.set_xticklabels(labels)
ax.set_xlabel("Memory Queue Pairs");
ax.legend()


fig.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')
plt.show()