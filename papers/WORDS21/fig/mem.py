
import matplotlib
import matplotlib.pyplot as plt
import numpy as np


#labels = ['1', '2', '4', '8', '16', '24','32']


labels =  [ '1', '10', '100', '1K', '10K', '100K', '1M' ]

def div_thousand (list):
    return [val /1000.0 for val in list]


#write_control_means = [74722.000000,133416.500000,224747.625000,355556.937500,510859.406250,316737.500000 ]
#default_means = [ 75163.101562 ,132205.703125,213194.703125,310103.218750,375318.468750,268242.000000 ]

#write_control_means = [77031.304688,137126.500000,230549.000000,362185.093750,521816.218750,590724.312500,623618.875000 ]
#default_means =       [77381.898438,136436.796875,220190.312500,315904.593750,389877.093750,373037.000000,366878.687500 ]


memory_per_key = [16, 160, 1600, 16000, 160000, 1600000, 16000000]




x = np.arange(len(labels))  # the label locations
width = .85  # the width of the bars


fig, ax = plt.subplots()

ax.axhline(y=22*1024*1024, color='tab:blue', linestyle='--',
label='barefoot tophino (22MB)')

rects = ax.bar(x, memory_per_key, width, label='Key Caching Memory',
        color='tab:red')


# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('Bytes')
ax.set_title('Memory Overhead per Key')
ax.set_xticks(x)
ax.set_xticklabels(labels)
ax.set_xlabel("Key Space");
ax.set_yscale('log')
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


#autolabel(rects1)
#autolabel(rects2)

fig.tight_layout()
plt.savefig("memory.pdf")
plt.show()
