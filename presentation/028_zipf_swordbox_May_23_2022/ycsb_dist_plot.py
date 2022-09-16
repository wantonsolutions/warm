import matplotlib.pyplot as plt; plt.rcdefaults()
import numpy as np
import matplotlib.pyplot as plt

import collections

#coeffs=["1.00", "0.75", "0.10"]
coeffs=["0.00", "0.60", "0.80", "0.90", "1.00", "1.10", "1.20", "1.30", "1.40", "1.50"]


def plot_coeff(coeff):
    keys = np.loadtxt("zipf/" + str(coeff) +"/workloada_0", delimiter=' ')
    key_dist = dict()
    for key in keys:
        key = int(key[1])
        #print(key)
        if not key in key_dist:
            key_dist[key]=1
        else:
            key_dist[key] = key_dist[key]+1


    od = collections.OrderedDict(sorted(key_dist.items()))
    x=[]
    y=[]

    #print(od[1])
    for pair in od:
        x.append(pair)
        y.append(100.0 * (float(od[pair])/float(len(keys))))

    plt.plot(x,y, label=coeff)

for coeff in coeffs:
    plot_coeff(coeff)

plt.legend()
plt.yscale('log')
plt.ylabel("Frequency (%)")
plt.xlabel("Keys")
plt.title("Zipf Distributions by Coefficient")
plt.savefig("zipf.pdf")
