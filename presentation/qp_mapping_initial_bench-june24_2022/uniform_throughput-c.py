
import matplotlib
import matplotlib.pyplot as plt
import numpy as np


#labels = ['1', '2', '4', '8', '16', '24','32']


labels =  ['1', '2', '4', '8', '16']

def div_thousand (list):
    return [val /1000.0 for val in list]


with_mapping = [110357,172618,261810,388144,536150]
without_mappings = [109599,177276,260462,388496,529007]

#not done
#without_mappings = [71799,110024,146619,162595,182549]


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
plt.savefig("uniform_mapping-c.pdf")
plt.show()