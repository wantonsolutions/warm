import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from common import *

def mul4(a):
    return[x*4 for x in a]

def rate_per_host(a,hosts):
    return[x/hosts for x in a]

def wins(arr, keys):
    clover=arr[0]
    for a,b in zip(arr,keys):
        print(str(b) + ": " + str(a/clover) + "x")


#manual collection
#steps of 8
keys = [x for x in range(0,129,8)]
x = [x for x in range(0,129,8)]
#six hosts zipf 1.5 3 trials
#8,16,24,32
#40,48,56,64
#72,80,88,96,
#104

keys_label = [str(x) for x in range(0,129,16)]
x_lab = [x for x in range(0,129,16)]
#x_lab = np.arange(len(keys_label))  # the label locations

tput=[126264, 1200764,1614114,1981764,2376874,2849514,3117038,3330861,3431735,3506670,3630810,3722979,3915607,3914276,3958349,3989503,3970129,]

terr=[26694,11221,69906,17897,104278,71718,23941,120434, 116300,2005, 22281,102302, 108484,79231,98445,165432, 146235,]
terr=mul4(terr)
tput=mul4(tput)
tput=rate_per_host(tput,6.0)
terr=rate_per_host(terr,6.0)
tput=div_mil(tput)
terr=div_mil(terr)

print(tput)
wins(tput,keys)

print(len(keys))
print(len(tput))

read_write_steering_color='#cf243cff'     #alice
treatment_label= 'queue pairs'


x = np.arange(len(keys))  # the label locations

plt.rcParams.update({'font.size': 18})
#fig, ax = plt.subplots()
fig, ax = plt.subplots(1,1, figsize=(8,4))
rects1 = ax.errorbar(keys, tput, yerr=terr, color=read_write_steering_color, linewidth=3, label="Write+Read") #, width, label=treatment_label, color='tab:blue', hatch='\\')

# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('MOPS')
ax.set_ylim(-0.1, 3)
ax.set_xticks(x_lab)
ax.set_xticklabels(keys_label)
ax.set_xlabel("Keys")
ax.legend(loc="lower right")
fig.tight_layout()
save_fig(plt)