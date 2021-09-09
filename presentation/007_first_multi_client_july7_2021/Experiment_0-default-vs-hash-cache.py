
import matplotlib
import matplotlib.pyplot as plt
import numpy as np


#labels = ['1', '2', '4', '8', '16', '24','32']


#labels =  ['1', '2', '4', '8', '16', '32']
labels =  ['2', '4', '8', '16', '32', '64']

def div_thousand (list):
    return [val /1000.0 for val in list]


with_middlebox = [109313,166815,222687,287957,451698,451930]


without_middlebox = [0,0,0,0,0,0]


treatment=div_thousand(with_middlebox)
control = div_thousand(without_middlebox)

treatment_label='multi-client-single-threaded-server'
control_label='write steering'
figure_name='experiment_0'

x = np.arange(len(labels))  # the label locations
width = 0.35  # the width of the bars

fig, ax = plt.subplots()
#rects2 = ax.bar(x - width/2, control, width, label=control_label,
#        color='tab:red')
rects1 = ax.bar(x , treatment, width, label=treatment_label, color='tab:blue', hatch='\\')

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