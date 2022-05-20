from netaddr import IPAddress
from netaddr import EUI

from common import *


def flush_table(table, target):
    ks = []
    x = table.entry_get(target)
    for i in x:
        ks.append(i[1])
    table.entry_del(target, ks)


def flush_register(table, target, name):
    ks = []
    dat = []
    print("Clearing Register: "+name+ " Size: "+ str(table.info.size) + "...")

    x = table.entry_get(target)
    for i in x:
        ks.append(i[1])

        byte_len=len(i[0][name+str(".f1")].val)
        val='\x00'
        if byte_len == 2:
            val='\x00\x00'
        elif byte_len == 3:
            val='\x00\x00\x00'
        elif byte_len == 4:
            val='\x00\x00\x00\x00'

        i[0][name+str(".f1")].val=val
        dat.append(i[0])
    
    table.entry_mod(target,ks,dat)
    print("Done")


def clear_forwarding_rules():
    print("Clearing forwarding rules")
    target=gc.Target(device_id=0, pipe_id=0xffff)
    forward_table = bfrt_info.table_get("SwitchIngress.forward")
    flush_table(forward_table,target)

def clear_registers():
    target=gc.Target(device_id=0, pipe_id=0xffff)
    names=all_registers
    for name in names:
        try:
            reg = bfrt_info.table_get(name)
            flush_register(reg, target, name)
        except:
            print(name + " does not seem to exist")


def add_forwarding_rules(table):
    target=gc.Target(device_id=0, pipe_id=0xffff)
    forward_table = bfrt_info.table_get("SwitchIngress.forward")

    key_list=[]
    data_list=[]
    for entry in table:
        port=entry[0]
        mac=int(EUI(entry[1]))
        key_list.append(forward_table.make_key([gc.KeyTuple("hdr.ethernet.dstAddr", mac )]))
        data_list.append(forward_table.make_data([gc.DataTuple('port', port)], "SwitchIngress.set_egr"))
    forward_table.entry_add(target,key_list,data_list)


clear_forwarding_rules()
clear_registers()
add_forwarding_rules(mac_forwarding_table)


