import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

cas_label='CAS'
write_label='Writes'
read_label="Reads"

cas_marker='x'
read_marker='+'
write_marker="*"


cas_color='#00702eff'                     #indigo
read_color='#cf243cff'     #alice
write_color='#ed7d31ff'          #kelly


def div_million (list):
    return [val /1e6 for val in list]


fig, axs = plt.subplots(1,1, figsize=(5,4))

def plot_data(ax,filename,title_text):
    df = pd.read_csv(filename)
    x_axis=df.get("keys").values
    writes=df.get("write_ops").values
    reads=df.get("read_ops").values
    cas=df.get("cas_ops").values

    writes=div_million(writes)
    reads=div_million(reads)
    cas=div_million(cas)

    print(writes)
    ax.plot(x_axis,writes,label=write_label,color=write_color,marker=write_marker)
    ax.plot(x_axis,reads,label=read_label,color=read_color,marker=read_marker)
    ax.plot(x_axis,cas,label=cas_label,color=cas_color,marker=cas_marker)
    ax.set_title(title_text)
    ax.legend(loc='upper left')

    ax.set_xlabel("Unique Keys")
    ax.set_ylabel("MOPS")

dir='keys'
plot_data(axs,dir+'/keys.dat','Total Keys')
#plot_data(axs,dir+'/multithread_1024.dat','QP: 20')

figure_name="keys"

plt.tight_layout()
plt.savefig(figure_name+'.pdf')
plt.savefig(figure_name+'.png')
#plt.show()


## Write only throughputs CNS to write
#latency=[88039,152566,246034,387889,578149,786835,982579,]                                                                                                                                                         
#threads=[2,4,8,16,32,64,112,]