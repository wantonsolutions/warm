import matplotlib
import matplotlib.pyplot as plt
import numpy as np

#labels = ['1', '2', '3', '4', '5', '6', '7', '8', '9', '10' ]
#memory_qp=[18261484.86,37054262.29,56117045.81,74918129.99,93975076.02,112654762.33,117922784.98,117935091.21,117934017.01,117935898.95]

memory_qp_cx5=[18261484.86,37054262.29,56117045.81,74918129.99,93975076.02,112654762.33,117922784.98,117935091.21]
labels_cx5 = ['1', '2', '3', '4', '5', '6', '7', '8']


memory_qp_cx6=[18626072.46,35837340.25,51614012.73,67190898.64,82400061.89,96971019.09,111257818.90,125029552.70,137874655.72,146808970.38,147815835.96,148078238.97,147869647.39,152253231.73,149877496.03,148010482.45,149190713.83,154876997.28,154986716.80,154932782.70,155590827.90,154922583.74,155434721.70]

labels_cx6 = []
for i in range(len(memory_qp_cx6)):
    labels_cx6.append(str(i+1))




write_steering_color='#ed7d31ff'          #kelly

def div_mil(a):
    return[x/1000000.0 for x in a]

memory_qp_cx6=div_mil(memory_qp_cx6)
memory_qp_cx5=div_mil(memory_qp_cx5)

treatment_label= 'queue pairs'

x = np.arange(len(labels_cx6))  # the label locations
x_2 = np.arange(len(labels_cx5))  # the label locations

width = 0.35  # the width of the bars

plt.rcParams.update({'font.size': 8})
fig, ax = plt.subplots()
rects2 = ax.bar(x_2 - width/2, memory_qp_cx5, width, color='g', edgecolor='k', label="cx5 (OSDI)") #, width, label=treatment_label, color='tab:blue', hatch='\\')
rects1 = ax.bar(x + width/2, memory_qp_cx6, width, color=write_steering_color, edgecolor='k', label="cx6") #, width, label=treatment_label, color='tab:blue', hatch='\\')

# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('MOPS')
ax.set_xticks(x)
ax.set_xticklabels(labels_cx6)
ax.set_xlabel("QP")
ax.legend()

fig.tight_layout()
plt.savefig("qp_bottleneck.pdf")
plt.savefig("qp_bottleneck.png")