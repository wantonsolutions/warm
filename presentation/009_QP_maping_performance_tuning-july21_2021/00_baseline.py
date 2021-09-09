
import matplotlib
import matplotlib.pyplot as plt
import numpy as np


#labels = ['1', '2', '4', '8', '16', '24','32']



def div_thousand (list):
    return [val /1000.0 for val in list]


labels=['2','4','8','16','32','64']
map_on=[61403,63782,70417,82370,98014,93028]
map_off=[104786,153514,196951,286878,306535,244274]

treatment=div_thousand(map_on)
control = div_thousand(map_off)

treatment_label= 'QP mapping on'
control_label='QP mapping off'
figure_name='00_baseline'

x = np.arange(len(labels))  # the label locations
width = 0.35  # the width of the bars

fig, ax = plt.subplots()
rects2 = ax.bar(x - width/2, control, width, label=control_label,
        color='tab:red')
rects1 = ax.bar(x + width/2, treatment, width, label=treatment_label, color='tab:blue', hatch='\\')

# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('KOP/s')
#ax.set_title('Clover YCSB-A (50% write)')
ax.set_xticks(x)
ax.set_xticklabels(labels)
ax.set_xlabel("Threads");
ax.legend()

fig.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')
plt.show()