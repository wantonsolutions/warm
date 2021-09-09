import numpy as np
import matplotlib.pyplot as plt
import matplotlib
import csv

def cdf(data):
    high = max(data)
    low = min(data)
    norm = matplotlib.colors.Normalize(low,high)

    print(data)
    count, bins_count = np.histogram(data, bins = 100000 )
    pdf = count / sum(count)
    
    y = np.cumsum(pdf)
    return bins_count[1:], y

outliers=10

data = np.loadtxt('/tmp/latency-latest.dat',delimiter=',', unpack=True)
data.sort()
data = data[:len(data)-outliers]

# add another variable if you have more CSV vars
#x, y = np.loadtxt('/tmp/latency-latest.dat')

x, y = cdf(data)

plt.xlim(0, 25000)
plt.ylim(0, 1)
plt.plot(x,y, label="latency cycles")
plt.savefig("latency.pdf")


