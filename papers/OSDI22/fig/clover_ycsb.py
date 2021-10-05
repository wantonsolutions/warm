
import matplotlib
import matplotlib.pyplot as plt
import numpy as np

from common_library import *

labels =  [ 'C(0%)', 'B(5%)', 'A(50%)' ]
memory_per_key = [21, 14, 4]

x = np.arange(len(labels))  # the label locations
width = .85  # the width of the bars
fig, ax = plt.subplots()
rects = ax.bar(x, memory_per_key, width, label='Clover (Default)',
        color='tab:red')


# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('Throughput (MOPS)')
ax.set_title('Clover Throughput vs YCSB')
ax.set_xticks(x)
ax.set_xticklabels(labels)
ax.set_xlabel("");
ax.legend()


fig.tight_layout()
save(plt)
plt.show()
