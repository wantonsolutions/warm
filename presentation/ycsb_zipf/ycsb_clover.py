import matplotlib.pyplot as plt
#%matplotlib inline

def milops(ops):
    return [x/1000000.0 for x in ops]


ymax=2.5
plt.style.use('ggplot')
x = ['C (0)','B (5)','A (50)']
throughput = [2053797.375000,1977440.000000,1221867.875000]




x_pos = [i for i, _ in enumerate(x)]

print throughput
print milops(throughput)
plt.bar(x_pos, milops(throughput), color='green')
plt.ylim(0,ymax);
plt.xlabel("Workloads")
plt.ylabel("Throughput (MOPS)")
plt.title("Zipf 8 threads YCSB A/B/C")

plt.xticks(x_pos, x)
plt.tight_layout()

plt.savefig("ycsb-workloads.pdf")
plt.show()

plt.clf()
###############################################################################
ymax=2.5
plt.style.use('ggplot')
x = ['100', '1000', '10000', '100000']
#throughput = [1938482,1977440.000000,1221867.875000]
throughput = [214979.000000, 435442.406250, 828999.687500, 1221867.875000,]

x_pos = [i for i, _ in enumerate(x)]

plt.bar(x_pos, milops(throughput), color='green')
plt.ylim(0,ymax);
plt.xlabel("Keys")
plt.ylabel("Throughput (MOPS)")
plt.title("Throughput vs Keyspace YCSB-A (50)")

plt.xticks(x_pos, x)
plt.tight_layout()

plt.savefig("ycsb-keyspace.pdf")
plt.show()

plt.clf()
#############################################################################
ymax=2.5
plt.style.use('ggplot')
x = ['100-A', '1K-A', '10K-A', '100K-A', '100-B', '1K-B', '100K-B', '100K-B', '100-C', '1K-C', '10K-C', '100K-C']
#throughput = [1938482,1977440.000000,1221867.875000]
throughputA = [214979.000000, 435442.406250, 828999.687500, 1221867.875000]
throughputB = [238838.890625,515786.312500,1132115.250000,1977440.000000]
throughputC = [240861.796875,514202.812500,1154482.125000,2053797.375000]



throughput =[]
throughput.extend(throughputA)
throughput.extend(throughputB)
throughput.extend(throughputC)

x_pos = [i for i, _ in enumerate(x)]

plt.bar(x_pos, milops(throughput), color='green')
plt.ylim(0,ymax)
plt.xlabel("Keys Workload")
plt.ylabel("Throughput (MOPS)")
plt.title("Throughput vs Keyspace")

plt.xticks(x_pos, x, rotation=20)
plt.tight_layout()
plt.savefig("ycsb-keyspace-full.pdf")
#plt.show()

plt.clf()
#############################################################################
from scipy import misc 

def slope(x, y):
    m = []
    for i in range(0,len(x)-1):
        m.append((y[i+1] - y[i]) / (x[i+1] - x[i]))
    return m


throughputA = [214979.000000, 435442.406250, 828999.687500, 1221867.875000]
xA = [100,1000,10000,100000]
throughputB = [238838.890625,515786.312500,1132115.250000,1977440.000000]
xB = [100,1000,10000,100000]
throughputC = [240861.796875,514202.812500,1154482.125000,2053797.375000]
xC = [100,1000,10000,100000]


a = slope(xA,throughputA)
print a
plt.plot(xA[0:(len(xA)-1)], slope(xA,throughputA), label="YCSB-A", color='r', marker='o')
plt.plot(xB[0:(len(xB)-1)], slope(xB,throughputB), label="YCSB-B", color='g', marker='o')
plt.plot(xC[0:(len(xC)-1)], slope(xC,throughputC), label="YCSB-C", color='b',marker='o')
plt.xscale('log')
#plt.yscale('log')

plt.xlabel("Key Space")
plt.ylabel("Change in Throughput (MOPS)")
plt.title("Change in Throughput vs Keyspace (aprox 1st derivative)")
plt.legend()
plt.tight_layout()
plt.savefig("delta-tput.pdf")
plt.show()

