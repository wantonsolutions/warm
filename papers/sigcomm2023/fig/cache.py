
import matplotlib
import matplotlib.pyplot as plt
import numpy as np

from common_library import *

matplotlib.rcParams['pdf.fonttype'] = 42


labels = [ '0', '1', '2', '4', '8', '16', '32', '64', '128', '256', '512', '1028']
memory_per_key = [918515,1085906,1165264,1193130,1224045,1237129,1256852,1276991,1297062,1318849,1339986,1365151]
memory_per_key = div_thousand(memory_per_key)

x = np.arange(len(labels))  # the label locations
width = .85  # the width of the bars

fig, ax = plt.subplots()
rects = ax.bar(x, memory_per_key, width,
        color='tab:red')

ax.axhline(y=918, color='tab:blue', linestyle='--',
label='Default Throughput')


# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('KOP/s')
ax.set_xticks(x)
ax.set_xticklabels(labels)
ax.set_xlabel("Cached Keys (top-N)");
ax.legend()

fig.tight_layout()
save(plt)
plt.show()
