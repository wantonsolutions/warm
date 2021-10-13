import matplotlib
import matplotlib.pyplot as plt
import numpy as np


#labels = ['1', '2', '4', '8', '16', '24','32']


labels =  ['2', '4', '8', '16', '32', '48', '64']

def div_thousand (list):
    return [val /1000.0 for val in list]

cns_replacement=[28681,38360,49696,52220,57308,58549,59176]
read_write_steering=[43706,76840,124477,181247,217367,220592,191877,]
#clover_with_buffering=[36737,65418,106296,153510,162171,145687,114447]



treatment=div_thousand(cns_replacement)
control = div_thousand(read_write_steering)

treatment_label= 'Swap CNS to write'
control_label='No mapping, read and write'
figure_name='experiment_0'

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


def autolabel(rects):
    """Attach a text label above each bar in *rects*, displaying its height."""
    for rect in rects:
        height = rect.get_height()
        ax.annotate('{}'.format(height),
                    xy=(rect.get_x() + rect.get_width() / 2, height),
                    xytext=(0, 3),  # 3 points vertical offset
                    textcoords="offset points",
                    ha='center', va='bottom')

fig.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')
plt.show()