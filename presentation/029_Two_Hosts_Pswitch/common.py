import sys
import os
import numpy as np

read_write_steering_label='Write + Read'
write_steering_label='Write'
default_clover_label='Clover'

read_write_steering_marker='+'
write_steering_marker="*"
default_clover_marker='.'

read_write_steering_color='#cf243cff'     #alice
write_steering_color='#ed7d31ff'          #kelly
default_clover_color='#8caff6ff'          #Coral 

def div_mil (list):
    return [val /1000000.0 for val in list]

def scale_mil(measurement):
    measurement["ops"]=div_mil(measurement["ops"])
    measurement["err"]=div_mil(measurement["err"])


def plot_max_improvement(ax, m1, m2, xaxis):
    m1_ops=[]
    m2_ops=[]
    threads=[]
    
    for thread in m1[xaxis]:
        if thread in m2[xaxis]:
            threads.append(thread)
            m1_ops.append(m1["ops"][m1[xaxis].index(thread)])
            m2_ops.append(m2["ops"][m2[xaxis].index(thread)])


    improvement=[ a/b for a ,b in zip(m1_ops, m2_ops)]
    max_improvement=max(improvement)
    max_index=improvement.index(max_improvement)

    max_improvement=round(max_improvement,2)
    ax.text(threads[len(threads) -2], m1_ops[len(m1_ops)-1]/2,str(max_improvement)+"x")
    x_1=threads[max_index]
    x=[x_1,x_1]
    y=[m1_ops[max_index],m2_ops[max_index]]

    ax.plot(x,y,color='k',linestyle='--')

def get_plot_name_from_filename():
    return os.path.splitext(sys.argv[0])[0]

def save_fig(plt):
    name=get_plot_name_from_filename()
    plt.savefig(name+'.pdf')
    plt.savefig(name+'.png')

def read_from_txt(filename):
    names  =['ops', 'threads', 'keys', 'zipf', 'ratio', 'size', 'total_ops', 'err', 'trials']
    formats=['i',   "i",       'i',    'f',    'i',     'i',    'i',         'i',   'i']
    db = np.loadtxt(filename, delimiter=',', dtype={'names':names, 'formats':formats})
    return db

def divide_db(db, chunks):
    parts=[]
    individual_size=int(len(db)/chunks)
    bottom=0
    top=individual_size
    for i in range(chunks):
        parts.append(db[bottom:top])
        bottom=top
        top=top+individual_size

    print(parts)
    return parts

def select_feilds(db, feilds):
    selected=dict()
    for f in feilds:
        selected[f]=db[f].tolist()
        print(selected[f])
    return selected