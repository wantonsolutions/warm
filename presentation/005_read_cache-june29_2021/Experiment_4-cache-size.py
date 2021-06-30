
import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from statistics import stdev


#labels = ['1', '2', '4', '8', '16', '24','32']


labels =  ['0', '1', '2', '4', '8', '16', '32', '64']

def div_thousand (list):
    return [val /1000.0 for val in list]

throughput = [285210,385989,437217,457021,471494,464344,427846,368955]
throughput = div_thousand(throughput)

figure_name='experiment_4'
treatment_label="throughput"

x = np.arange(len(labels))  # the label locations
width = 0.85  # the width of the bars.0

fig, ax = plt.subplots()
rects1 = ax.bar(x,throughput, width, color='tab:red')

# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('KOP/S')
#ax.set_title('Clover YCSB-A (50% write)')
ax.set_xticks(x)
ax.set_xticklabels(labels)
ax.set_xlabel("Cached Keys")


fig.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')
plt.show()