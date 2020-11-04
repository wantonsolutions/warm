import matplotlib.pyplot as plt
#%matplotlib inline
ymax=2000000
plt.style.use('ggplot')
x = ['1', '2', '4', '8']
#throughput = [256407, 490319, 956293, 1867353]
throughput = [177120,338106,633750,1255057]

x_pos = [i for i, _ in enumerate(x)]

plt.bar(x_pos, throughput, color='green')
plt.ylim(0,ymax);
plt.xlabel("Threads")
plt.ylabel("Ops/s")
plt.title("YCSB-A (50% write) 100K keys Uniform Random")

plt.xticks(x_pos, x)

plt.savefig("ycsb_a.pdf")
plt.show()

plt.clf()

x = ['1', '2', '4', '8']
throughput = [256407, 490319, 956293, 1867353]

x_pos = [i for i, _ in enumerate(x)]

plt.bar(x_pos, throughput, color='green')
plt.ylim(0,ymax);
plt.xlabel("Threads")
plt.ylabel("Ops/s")
plt.title("YCSB-B (5% write) 100K keys Uniform Random")

plt.xticks(x_pos, x)

plt.savefig("ycsb_b.pdf")
plt.show()

plt.clf()


x = ['1', '2', '4', '8']
throughput = [268070,510179,1002037,1938482]

x_pos = [i for i, _ in enumerate(x)]

plt.bar(x_pos, throughput, color='green')
plt.ylim(0,ymax);
plt.xlabel("Threads")
plt.ylabel("Ops/s")
plt.title("YCSB-C (0% write) 100K keys Uniform Random")

plt.xticks(x_pos, x)

plt.savefig("ycsb_c.pdf")
plt.show()

plt.clf()
