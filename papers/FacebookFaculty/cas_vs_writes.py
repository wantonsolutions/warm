import matplotlib
import matplotlib.pyplot as plt
import numpy as np


cas_single_qp_memory=('cas_single_qp_memory', 2863146)
cas_multi_qp_memory=('cas_multi_qp_memory', 2928908)
cas_single_qp_device=('cas_single_qp_device', 0) #I don't think I have this value
cas_multi_qp_device=('cas_multi_qp_device', 8974950) #I don't think I have this value

write_single_qp_memory=('write_single_qp_memory', 18261484)
write_multi_qp_memory=('write_multi_qp_memory', 117935091)
write_single_qp_dev=('write_single_qp_dev', 22137480)
write_multi_qp_dev=('write_multi_qp_dev',22137480)


labels = ["CAS", "Write"]
main_memory=[cas_multi_qp_memory[1],write_single_qp_memory[1]]
device_memory=[cas_multi_qp_device[1],write_single_qp_dev[1]]

# labels = [cas_multi_qp_memory[0], cas_multi_qp_device[0], write_single_qp_memory[0], write_single_qp_dev[0]]
# mops = [cas_multi_qp_memory[1], cas_multi_qp_device[1], write_single_qp_memory[1], write_single_qp_dev[1]]

def div_mil(a):
    return[x/1000000.0 for x in a]

x = np.arange(len(labels))
print(x)

main_memory=div_mil(main_memory)
device_memory=div_mil(device_memory)

width=0.75
plt.rcParams.update({'font.size': 16})
#fig, ax = plt.subplots()
fig, ax = plt.subplots(1,1, figsize=(8,4))

#ax.bar(x + width/2, device_memory, width, label="Device Memory", edgecolor='k')
ax.bar(x, main_memory, width, label="Main Memory", color='orange', edgecolor='k')
ax.set_xticks(x,labels)

xalign=0.92
ax.text(xalign,10,str(round(main_memory[1]/main_memory[0],1))+"x")
#ax.text(xalign+(width/2),10,str(round(device_memory[1]/device_memory[0],1))+"x")




# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('MOPS')
#ax.set_xticks(x)
#ax.set_xticklabels(labels)
#ax.legend()

fig.tight_layout()
figname="cas_vs_writes"
plt.savefig(figname+".pdf")
plt.savefig(figname+".png")