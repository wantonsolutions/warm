import matplotlib.pyplot as plt
import numpy as np
#from statistics import stdev 

def average(arrs):
    length = len(arrs)
    first = True
    sum = []
    for arr in arrs:
        if first:
            sum = arr
            first= False

        else:
            i=0
            for val in arr:
                sum[i]+=arr[i]
                i=i+1
    i=0
    for val in sum:
        sum[i] = sum[i]/length
        i=i+1
    print sum
    return sum

def std(arrs):
    invert = []
    first = True
    for arr in arrs:
        if first:
            i=0
            for val in arr:
                invert.append([])
                invert[i].append(val)
                i=i+1
            first = False
        else:
         i = 0
         for val in arr:
             invert[i].append(i)
             i=i+1
    print invert


packet_sizes = ["128","256","512","1K","2K","4K"]
read = np.array([3.459,3.492,3.545,3.599,3.752,4.048])
write = np.array([6.616,6.690,6.694,6.784,7.190,7.458])


readmulti = [
        [3.459,3.492,3.545,3.599,3.752,4.048],
        [3.456,3.503,3.524,3.638,3.734,4.011],
        [3.476,3.508,3.539,3.661,3.736,4.005],
    ]
    
writemulti = [
        [6.616,6.690,6.694,6.784,7.190,7.458],
        [6.745,6.872,6.941,7.031,7.238,7.444],
        [6.720,6.901,6.913,7.006,7.238,7.600]
    ]

  
  
readavg = average(readmulti)
readstd = np.std(readmulti,axis=0)

writeavg = average(writemulti)
writestd = np.std(writemulti,axis=0)
print readstd

#for a, b in zip packet_sizes,readavg:
#    print("[",a",",b,"]")

print writestd


fig, ax = plt.subplots(figsize=(6, 5))

#ax.plot(packet_sizes,read,marker='x',label="Read Latency")
#ax.plot(packet_sizes,write,marker='x', label="Write Latency")

ax.errorbar(packet_sizes,readavg,readstd,marker="x",label="Read Latency")
#ax.plot(packet_sizes,write,marker='x', label="Write Latency")
ax.errorbar(packet_sizes,writeavg,writestd,marker="x",label="Write Latency")

ax.set_ylim(bottom=0,top=20)
#ax.set_yscale('log')
#ax.ticklabel_format(axis="both",style="plain")
#plt.xlim(left=0,right=1100)
#plt.yscale('log')
#plt.ylim(bottom=3,top=8)
#plt.ticklabel_format(axis="both",style="plain")
#plt.get
#plt.axes.ticklabel_format(axis="both",style="plain")
#plt.ticklabel_format(axis="both", style="plain", scilimits=(0,0))
#plt.ticklabel_format(style='sci',axis='y',scilimits=(0,0))

ax.set_ylabel('microseconds')
ax.set_xlabel('packet size (bytes)')
ax.set_title('Clover Operation Latency vs Packet Size')
plt.legend()
plt.savefig("clover_lat.pdf")
