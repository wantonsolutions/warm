from netaddr import IPAddress
from netaddr import EUI
from common import *


target=gc.Target(device_id=0, pipe_id=0xffff)

def get_counter(name, t):
    total_byte_counter = bfrt_info.table_get(name)
    total_bytes=0
    x = total_byte_counter.entry_get(target)
    for i in x:
        print(i[0])
        total_bytes=int.from_bytes(i[0]["$COUNTER_SPEC_"+t].val, byteorder='big')
        break
    return total_bytes

names=["total_byte_counter", "total_packets_counter"]
types=["BYTES", "PKTS"]
header=""
values=""

for name, t in zip(names,types):
    header = header + name + ","
    values = values + str(get_counter(name,t)) + ","


stat_file = open("Switch_Statistics.dat", "w")

stat_file.write(header + "\n")
stat_file.write(values + "\n")




