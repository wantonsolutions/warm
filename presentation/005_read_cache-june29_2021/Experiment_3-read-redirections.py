
import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from statistics import stdev


#labels = ['1', '2', '4', '8', '16', '24','32']


labels =  ['1', '2', '4', '8', '16', '32']

def div_thousand (list):
    return [val /1000.0 for val in list]

reads = [260248,415079,583561,745291,888848,1173081]
read_redirections= [0,240,1398,6481,25857,87606]

percent_redirections=[a/b * 100 for a,b in zip(read_redirections,reads)]


figure_name='experiment_3'
treatment_label="read_redirections"

x = np.arange(len(labels))  # the label locations
width = 0.75  # the width of the bars.0

fig, ax = plt.subplots()
rects1 = ax.bar(x,percent_redirections, width, color='tab:blue')

# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('% Read Redirections')
#ax.set_title('Clover YCSB-A (50% write)')
ax.set_xticks(x)
ax.set_xticklabels(labels)
ax.set_xlabel("Threads");
ax.legend()


fig.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')
plt.show()