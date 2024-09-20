import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import csv
from textwrap import wrap
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
            x=x.astype(float)
            values.append(x.tolist())
        i=i+1

#remove unwanted fields
#rm_field_list=["TCAM","8-bitActionSlots","16-bitActionSlots","32-bitActionSlots","TernaryMatchInputxbar"]
rm_field_list=[]
for val in rm_field_list:
    rm_index=headers.index(val)
    headers.pop(rm_index)
    for v in values:
        v.pop(rm_index)

#sort lists based on resource utilization
maxes=[]
for i in range(len(values[0])):
    vlist=[]
    for j in range(len(labels)-1):
        vlist.append(values[j][i])
    #print(vlist)
    maxes.append(max(vlist))




_, headers = zip(*sorted(zip(maxes,headers)))
for i in range(len(values)):
    #print(values[i])
    _ , values[i] = zip(*sorted(zip(maxes,values[i])))
    #print(values[i])



# fig, ax = plt.subplots()
fig, ax = plt.subplots(1,1, figsize=(7,3))
width = 0.65       # the width of the bars: can also be len(x) sequence

colors=[default_clover_color,connection_color, write_steering_color,read_write_steering_color]


for i in range(len(labels)-2, -1, -1):
    ax.bar(headers, values[i], width, label=labels[i],color=colors[i],edgecolor='k')

#plot the simple switch
i=len(labels)-1

print(headers)
#ax.scatter(headers, values[i], label=labels[i],color="green", markerfacecolor='None', marker="_", markersize=13, alpha=0.9)
ax.scatter(headers, values[i], label=labels[i], marker="o", linewidth=1, edgecolor='k', color="green")

# headers = [ '\n'.join(wrap(l, 12)) for l in headers ]
headers = [ l.replace("\\n","\n") for l in headers ]
ax.set_xticks(np.arange(len(headers)))
ax.set_xticklabels(headers,rotation=35, ha='right', fontsize=8)
#ax.set_xlabel("Resources")
ax.set_ylabel("% Total Resources")
ax.set_ylim(0,50)
#ax.set_title("Breakdown of switch resource utilization by swordbox component")
plt.legend(loc='upper left',fontsize=8)
plt.tight_layout()
save_fig(plt)