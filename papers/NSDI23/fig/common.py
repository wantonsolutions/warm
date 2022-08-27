import sys
import os
import numpy as np

read_write_steering_label='Write + Read'
write_steering_label='Write'
default_clover_label='Clover'

read_write_steering_marker='+'
write_steering_marker="*"
default_clover_marker='.'

read_write_steering_color='#cf243cff'     #red
write_steering_color='#ed7d31ff'          #orange
default_clover_color='#8caff6ff'          #blueish

connection_color='#7D31ED'              #dark purple

def div_mil (list):
    return [val /1000000.0 for val in list]

def div_tho (list):
    return [val /1000.0 for val in list]

def scale_mil(measurement):
    names  =['ops', 'err']
    for name in names:
        if not name in measurement:
            continue
        measurement[name]=div_mil(measurement[name])

def scale_tho(measurement):
    names  =['read_lat', 'read_lat_err', 'write_lat', 'write_lat_err']
    for name in names:
        if not name in measurement:
            continue
        measurement[name]=div_tho(measurement[name])

def scale(measurement):
    scale_mil(measurement)
    scale_tho(measurement)


def get_gbps(total_bytes, duration):
    bw=[]
    for a,b in zip(total_bytes,duration):
        bw_from_data= a/(b/1000) #a is the total bytes, duration is in ms
        gbps=(bw_from_data*8.0)/1000000000.0 #convert to gbps
        bw.append(gbps)
    return bw

def get_mpps(total_bytes, duration):
    bw=[]
    for a,b in zip(total_bytes,duration):
        bw_from_data= a/(b/1000)       #a is the total packets that would have run in a second 
        gbps=(bw_from_data)/1000000.0 #convert to mpps
        bw.append(gbps)
    return bw

def get_mpps_and_err(measure):
    mpps = get_mpps(measure["total_packets"],measure["duration"])
    mpps_err = get_mpps(measure["total_packets_error"],measure["duration"])
    return(mpps,mpps_err)

def get_gbps_and_err(measure):
    bw = get_gbps(measure["total_bytes"],measure["duration"])
    bw_err = get_gbps(measure["total_bytes_error"],measure["duration"])
    return(bw,bw_err)


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

# 2003364,120,1000,1.00,50,100,1001806,1712,1628754,16548,19892,68,5
def read_from_txt(filename):
    names  =['ops', 'threads', 'duration', 'zipf', 'ratio', 'size', 'total_ops', 'err', 'read_lat', 'read_lat_err', 'write_lat', 'write_lat_err', 'total_bytes', 'total_bytes_error','total_packets', 'total_packets_error','trials',  ]
    formats=['i',   'i',       'i',        'f',    'i',     'i',    'i',         'i',   'i',        'i',            'i',         'i',             'ulonglong',   'ulonglong',        'ulonglong',     'ulonglong',          'i']
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

    return parts

def select_feilds(db, feilds):
    selected=dict()
    for f in feilds:
        selected[f]=db[f].tolist()
    return selected