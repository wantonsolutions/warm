import numpy as np
import matplotlib.pyplot as plt
import matplotlib
import csv

def cdf(data):
    high = max(data)
    low = min(data)
    norm = matplotlib.colors.Normalize(low,high)

    #print(data)
    count, bins_count = np.histogram(data, bins = 1000000)
    pdf = count / sum(count)
    
    y = np.cumsum(pdf)
    return bins_count[1:], y


def plot_charts(source_filename, cdf_filename, timeline_output_filename):
    timestamps, id, opcode, count = np.loadtxt(source_filename,delimiter=',', unpack=True, dtype={'names':('timestamp','id','msg_type','count'),'formats': ('i8','i','S32','i')})

    in_flight = dict()
    for ts, c in zip(timestamps,count):
        if str(ts) in in_flight:
            in_flight[str(ts)] = in_flight[str(ts)] + c
        else:
            in_flight[str(ts)] = c


    x = []
    y = []
    for v in in_flight:
        x.append(int(v))
        y.append(int(in_flight[v]))



    minimum=min(x)
    x_shrunk=[(float(v)-float(minimum))/1000000000.0 for v in x]

    #points of interest
    i=0
    x_interest=[]
    y_interest=[]
    interest_label=[]
    for x_1, x_2, y_1 in zip(x_shrunk[:len(x)-1],x_shrunk[1:len(x)],y[:len(y)-1]):
        if x_2 > x_1 + 0.1:
            x_interest.append(x_1)
            y_interest.append(y_1)
            interest_label.append(str(x[i]))
        i=i+1

    print(x_interest)
    print(y_interest)
    print(interest_label)

    for x_i, label in zip(x_interest,interest_label):
        plt.axvline(x=x_i,label=label,color="red")

    plt.legend()
     
    plt.xlabel("Seconds")
    plt.ylabel("Outstanding Messages")
    plt.plot(x_shrunk,y)
    plt.savefig(timeline_output_filename)

    plt.clf()


    in_flight = dict()
    in_flight["RC_SEND"] = dict()
    in_flight["RC_WRITE_ONLY"] = dict()
    in_flight["RC_READ_REQUEST"] = dict()
    in_flight["RC_COMPARE_AND_SWAP"] = dict()


    print(opcode)

    for ts, c, op in zip(timestamps,count,opcode):
        op = op.decode("utf-8")
        if str(ts) in in_flight[op]:
            in_flight[op][str(ts)] = in_flight[op][str(ts)] + c
        else:
            in_flight[op][str(ts)] = c


    for op in in_flight:
        x=[]
        y=[]
        print(op)
        print(in_flight[op])
        for v in in_flight[op]:
            x.append(int(v))
            y.append(int(in_flight[op][v]))

        minimum=min(x)
        x_shrunk=[(float(v)-float(minimum))/1000000000.0 for v in x]
        plt.plot(x_shrunk,y,label=op)

    plt.legend()
    plt.savefig("ops-"+timeline_output_filename)










    

    







plot_charts('/tmp/in_flight.dat','in_flight_cdf.pdf','in_flight_timeline.pdf')


