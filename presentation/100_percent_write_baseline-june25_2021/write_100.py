
import matplotlib
import matplotlib.pyplot as plt
import numpy as np


#labels = ['1', '2', '4', '8', '16', '24','32']


labels =  ['1', '2', '4', '8', '16', '32']

def div_thousand (list):
    return [val /1000.0 for val in list]


with_write_control = [54290,93629,149023,224125,344979,485859]
without_write_control = [54933,92152,136305,169282,155859,162371]
figure_name='write_only'





write_control_means=div_thousand(with_write_control)
default_means = div_thousand(without_write_control)


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

plt.title("100% writes")

fig.tight_layout()
plt.savefig(figure_name+".pdf")
plt.savefig(figure_name+".png")
plt.show()