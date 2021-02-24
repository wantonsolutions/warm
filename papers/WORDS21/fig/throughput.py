
import matplotlib
import matplotlib.pyplot as plt
import numpy as np


#labels = ['1', '2', '4', '8', '16', '24','32']


labels =  [ '2', '4', '8', '16', '24','32', '40', '48', '56', '64']

def div_thousand (list):
    return [val /1000.0 for val in list]


#write_control_means = [74722.000000,133416.500000,224747.625000,355556.937500,510859.406250,316737.500000 ]
#default_means = [ 75163.101562 ,132205.703125,213194.703125,310103.218750,375318.468750,268242.000000 ]

#write_control_means = [77031.304688,137126.500000,230549.000000,362185.093750,521816.218750,590724.312500,623618.875000 ]
#default_means =       [77381.898438,136436.796875,220190.312500,315904.593750,389877.093750,373037.000000,366878.687500 ]


write_control_means = [127467,214218,334756,505495,580000,667506,926461,1131579,1266245,1356873]
default_means = [119493,181330,282863,443276,565054,618635,689105,819009,930301,949238]

write_control_means=div_thousand(write_control_means)
default_means = div_thousand(default_means)



x = np.arange(len(labels))  # the label locations
width = 0.35  # the width of the bars

fig, ax = plt.subplots()
rects2 = ax.bar(x - width/2, default_means, width, label='Clover',
        color='tab:red')
rects1 = ax.bar(x + width/2, write_control_means, width, label='Write Redirections', color='tab:blue', hatch='\\')


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


#autolabel(rects1)
#autolabel(rects2)

fig.tight_layout()
plt.savefig("throughput.pdf")
plt.show()
