
import matplotlib
import matplotlib.pyplot as plt
import numpy as np
matplotlib.rcParams['pdf.fonttype'] = 42

from common_library import *

labels =  [ '1', '8', '64', '512', '4K', '32K', '256K' ]
memory_per_key = [16, 128, 1024, 8192, 65536, 524288, 4194304]

x = np.arange(len(labels))  # the label locations
width = .85  # the width of the bars

fig, ax = plt.subplots()

ax.axhline(y=64*1024*1024, color='tab:blue', linestyle='--',
label='Barefoot Tofino 2 (64MB)')

rects = ax.bar(x, memory_per_key, width, label='Key Caching Memory',
        color='tab:red')


# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('Bytes')
#ax.set_title('Memory Overhead per Key')
ax.set_xticks(x)
ax.set_xticklabels(labels)
ax.set_xlabel("Key Space");
ax.set_yscale('log')
ax.legend()

fig.tight_layout()
save(plt)
plt.show()
