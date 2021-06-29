
import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from statistics import stdev


#labels = ['1', '2', '4', '8', '16', '24','32']


labels =  ['1', '2', '4', '8', '16']
labels =  ['1', '2', '4', '8', '16', '32']

def div_thousand (list):
    return [val /1000.0 for val in list]


size_1 = [313211,317520,312969,313498,308685]
size_1_std = stdev(size_1)
size_1_mean = np.mean(size_1)

size_2 = [316145,327026,321128,316812]
size_2_std = stdev(size_2)
size_2_mean = np.mean(size_2)

size_4 = [318868,314985,317944,338155,333915]
size_4_std = stdev(size_4)
size_4_mean = np.mean(size_4)

size_8 = [337017,337531,316409,317409,337600]
size_8_std = stdev(size_8)
size_8_mean = np.mean(size_8)

size_16 = [313838,333808,336839,336419,320301]
size_16_std = stdev(size_16)
size_16_mean = np.mean(size_16)


size_32 = [303217,329805,303742,302252,302993]
size_32_std = stdev(size_32)
size_32_mean = np.mean(size_32)

cache_sizes = [size_1_mean, size_2_mean, size_4_mean, size_8_mean,size_16_mean,size_32_mean]
cache_size_std = [size_1_std, size_2_std, size_4_std, size_8_std,size_16_std,size_32_std]


cache_sizes=div_thousand(cache_sizes)
cache_sizes_std = div_thousand(cache_size_std)

figure_name='experiment_2'
treatment_label="cache depths"

x = np.arange(len(labels))  # the label locations
width = 0.75  # the width of the bars.0

fig, ax = plt.subplots()
rects1 = ax.bar(x,cache_sizes, width, yerr=cache_sizes_std, color='tab:green')

# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('KOP/s')
#ax.set_title('Clover YCSB-A (50% write)')
ax.set_ylim(250,350)
ax.set_xticks(x)
ax.set_xticklabels(labels)
ax.set_xlabel("Read Cache Depth");
ax.legend()


fig.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')
plt.show()