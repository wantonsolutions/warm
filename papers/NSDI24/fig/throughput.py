
import matplotlib
import matplotlib.pyplot as plt
import numpy as np

from common_library import *

matplotlib.rcParams['pdf.fonttype'] = 42

labels =  [ '2', '4', '8', '16', '24','32', '40', '48', '56', '64']

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
ax.set_xticks(x)
ax.set_xticklabels(labels)
ax.set_xlabel("Threads");
ax.legend()

fig.tight_layout()
save(plt)
plt.show()
