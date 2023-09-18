import matplotlib
import matplotlib.pyplot as plt
import numpy as np

#labels = ['1', '2', '3', '4', '5', '6', '7', '8', '9', '10' ]
#memory_qp=[18261484.86,37054262.29,56117045.81,74918129.99,93975076.02,112654762.33,117922784.98,117935091.21,117934017.01,117935898.95]

labels = ['1', '2', '3', '4', '5', '6', '7', '8']
memory_qp=[18261484.86,37054262.29,56117045.81,74918129.99,93975076.02,112654762.33,117922784.98,117935091.21]
write_steering_color='#ed7d31ff'          #kelly

def div_mil(a):
    return[x/1000000.0 for x in a]

memory_qp=div_mil(memory_qp)
treatment_label= 'queue pairs'

x = np.arange(len(labels))  # the label locations

width = 0.35  # the width of the bars

plt.rcParams.update({'font.size': 18})
#fig, ax = plt.subplots()
fig, ax = plt.subplots(1,1, figsize=(8,4))
rects1 = ax.bar(x, memory_qp, color=write_steering_color, edgecolor='k') #, width, label=treatment_label, color='tab:blue', hatch='\\')

# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('MOPS')
ax.set_xticks(x)
ax.set_xticklabels(labels)
ax.set_xlabel("QP")

fig.tight_layout()
plt.savefig("qp_bottleneck.pdf")
plt.savefig("qp_bottleneck.png")