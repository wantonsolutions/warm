import sys
import numpy as np

if len(sys.argv) != 3:
    print("Exiting with error, wrong number of args")
    exit(1)

trial_file=sys.argv[1]
trials=sys.argv[2]

#print("Calculating Stats on file " + trial_file + " for " + trials + " trials")

ops, threads, keys, zipf, rw_ratio, pkt_size, total_ops, read_lat, write_lat  = np.loadtxt(trial_file, delimiter=',', unpack=True)

avg_ops = np.mean(ops)
avg_total = np.mean(total_ops)
error = np.std(ops)

avg_read_lat=np.mean(read_lat)
read_lat_err=np.std(read_lat)

avg_write_lat=np.mean(write_lat)
write_lat_err=np.std(write_lat)


if int(trials) > 1:
    avg_ops_val=avg_ops
    thread_val=threads[0]
    key_val=keys[0]
    zipf_val="{:.2f}".format(zipf[0])
    rw_val=rw_ratio[0]
    pkt_size_val=pkt_size[0]
    avg_total_val=avg_total

    size=len(ops)
else:
    avg_ops_val=avg_ops
    thread_val=threads
    key_val=keys
    zipf_val="{:.2f}".format(zipf)
    rw_val=rw_ratio
    pkt_size_val=pkt_size
    avg_total_val=avg_total

    size=1

print(
str(int(avg_ops_val)) + "," +
str(int(thread_val)) + "," +
str(int(key_val)) + "," +
str(zipf_val) + "," +
str(int(rw_val)) + "," +
str(int(pkt_size_val)) + "," +
str(int(avg_total_val)) + "," + 
str(int(error)) + "," +

str(int(avg_read_lat)) + "," +
str(int(read_lat_err)) + "," +
str(int(avg_write_lat)) + "," +
str(int(write_lat_err)) + "," +

str(size))