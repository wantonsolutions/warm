import matplotlib
import matplotlib.pyplot as plt
import numpy as np

from common_library import *
labels = ['64','32','16'] #,'8', '4', '2', '1']

memory_qp=[47854,48935,46696]

memory_qp=div_thousand(memory_qp)
treatment_label= 'memory queue pairs'

x = np.arange(len(labels))  # the label locations

print(x)
width = 0.35  # the width of the bars

fig, ax = plt.subplots()
rects1 = ax.bar(x, memory_qp) #, width, label=treatment_label, color='tab:blue', hatch='\\')

# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('KOP/s')
ax.set_xticks(x)
ax.set_xticklabels(labels)
ax.set_xlabel("Memory Queue Pairs");

fig.tight_layout()
save(plt)
plt.show()