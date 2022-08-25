import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import csv
from common import *


inputfilename='switch_resources.csv'
# df = pd.read_csv('switch_resources.csv')
# df.set_index('SRAM').plot()

# plt.show()
import csv

with open(inputfilename, newline='') as infh:
    reader = csv.reader(infh)
    i=0
    headers=[]
    labels=[]
    values=[]
    for row in reader:
        if i==0:
            headers=row[1:]
        else:
            labels.append(row[0])
            x=np.array(row[1:])
            x=x.astype(np.float)
            values.append(x.tolist())
        i=i+1

    print(headers)
    print(labels[0])
    print(values[0])

#remove unwanted fields
rm_field_list=["TCAM","8-bitActionSlots","16-bitActionSlots","32-bitActionSlots","TernaryMatchInputxbar"]
for val in rm_field_list:
    rm_index=headers.index(val)
    headers.pop(rm_index)
    for v in values:
        v.pop(rm_index)



#sort lists based on resource utilization

#get_max_list
maxes=[]
for i in range(len(values[0])):
    vlist=[]
    for j in range(len(labels)):
        vlist.append(values[j][i])
    print(vlist)
    maxes.append(max(vlist))

print(maxes)
print("done with maxes")



_, headers = zip(*sorted(zip(maxes,headers)))
for i in range(len(values)):
    print(values[i])
    _ , values[i] = zip(*sorted(zip(maxes,values[i])))
    print(values[i])



fig, ax = plt.subplots()
width = 0.65       # the width of the bars: can also be len(x) sequence

colors=[default_clover_color,connection_color, write_steering_color,read_write_steering_color]

for i in range(len(labels)-1, -1, -1):
    ax.bar(headers, values[i], width, label=labels[i],color=colors[i])


ax.set_xticklabels(headers,rotation=90)
#ax.set_xlabel("Resources")
ax.set_ylabel("% Total Resources")
ax.set_title("Breakdown of switch resource utilization by swordbox component")
plt.legend()
plt.tight_layout()
plt.savefig("switch_resources.pdf")
plt.savefig("switch_resources.png")