import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

cas_label='CAS'
write_label='Writes'
read_label="Reads"

cas_marker='x'
default_marker="o"


cas_color='#00702eff'                     #indigo
default_color='#8caff6ff'          #Coral 


write_color='#ed7d31ff'          #kelly

def plot_data(ax,filename,line_label,line_color,lmarker):
    df = pd.read_csv(filename)
    x_axis=df.get("concur").values
    cas=df.get("cas_ops").values

    #normalize cores
    x_axis=[ a/16 for a in x_axis]
    cas=div_million(cas)

    ax.plot(x_axis,cas,label=line_label,color=line_color,marker=lmarker)
    ax.legend(loc='upper left')

    ax.set_xlabel("QP")
    ax.set_ylabel("MOPS")



def div_million (list):
    return [val /1e6 for val in list]


plt.rcParams.update({'font.size': 14})
fig, axs = plt.subplots(figsize=(8,4), dpi=80)
#fig, axs = plt.subplots(1,1, figsize=(5,4))

plot_data(axs,'cas_contention_control.dat','CAS',default_color,default_marker)
plot_data(axs,'cas_to_write.dat','CAS->Write',cas_color,cas_marker)
#axs.set_title("csn raw vs write to CNS")
axs.set_ylim(1,3)

figure_name="cas_vs_swap"

plt.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')