import scapy.all as scapy
import time
p=scapy.Ether(dst="00:55:55:55:55:55")/scapy.IP(src="192.168.1.12", dst="192.168.1.13")/scapy.UDP(dport=5)
p2=scapy.Ether(dst="00:55:55:55:55:00")/scapy.IP(src="192.168.1.12", dst="192.168.1.13")/scapy.UDP(dport=5)

print(p)


for i in range(10):
    print(p2.show())
    scapy.sendp(p2, iface="veth0") 
    time.sleep(1)
    