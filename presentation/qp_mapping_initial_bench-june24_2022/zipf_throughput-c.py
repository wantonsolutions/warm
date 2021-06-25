
import matplotlib
import matplotlib.pyplot as plt
import numpy as np


#labels = ['1', '2', '4', '8', '16', '24','32']


labels =  ['1', '2', '4', '8', '16']

def div_thousand (list):
    return [val /1000.0 for val in list]


with_mapping = [109769,175704,261545,386758,525386]
without_mappings = [110658,162778,262220,385972,519575]



write_control_means=div_thousand(with_mapping)
default_means = div_thousand(without_mappings)


x = np.arange(len(labels))  # the label locations
width = 0.35  # the width of the bars

fig, ax = plt.subplots()
rects2 = ax.bar(x - width/2, default_means, width, label='Mapping Off',
        color='tab:red')
rects1 = ax.bar(x + width/2, write_control_means, width, label='Mapping On', color='tab:blue', hatch='\\')


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
plt.savefig("zipf_mapping-c.pdf")
plt.show()