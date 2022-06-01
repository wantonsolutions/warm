import numpy as np
import matplotlib.pyplot as plt
import matplotlib
import csv
from matplotlib.pyplot import figure

import sys

def cdf(data):
    high = max(data)
    low = min(data)
    norm = matplotlib.colors.Normalize(low,high)

    #print(data)
    count, bins_count = np.histogram(data, bins = 100000 )
    pdf = count / sum(count)
    
    y = np.cumsum(pdf)
    x = bins_count[1:]

    y= np.insert(y,0,0)
    x= np.insert(x,0,x[0])
    return x, y

filenames=sys.argv[1:]
output_99=[]
for file in filenames:
    latency = np.loadtxt(file, delimiter=',', unpack=True)

    ##This should only happen to the read input file for the 100% write case
    if len(latency) == 0:
        output_99.append(0)
        continue

    x,y = cdf(latency)
    plt.plot(x,y, label="latency")

    percentile_99=np.percentile(np.array(latency),99)
    output_99.append(percentile_99)
    #print(file, "99th percentile =",percentile_99)

print(str(output_99[0])+","+str(output_99[1]))
plt.xlim(0,100000)
plt.savefig("latency.pdf")
    
