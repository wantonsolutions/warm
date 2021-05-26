import numpy as np
import matplotlib.pyplot as plt
import matplotlib
import csv

def cdf(data):
    high = max(data)
    low = min(data)
    norm = matplotlib.colors.Normalize(low,high)

    print(data)
    count, bins_count = np.histogram(data, bins = 1000 )
    pdf = count / sum(count)
    
    y = np.cumsum(pdf)
    return bins_count[1:], y

data = np.loadtxt('/tmp/latency-latest.dat',delimiter=',', unpack=True)
# add another variable if you have more CSV vars
#x, y = np.loadtxt('/tmp/latency-latest.dat')

x, y = cdf(data)

plt.plot(x,y, label="latency cycles")
plt.savefig("latency.pdf")


