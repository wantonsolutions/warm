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
    try:
        #table.default_entry_reset(target)
        table.entry_del(target)
    except:
        print("wtf - 2")
    print("Done")


def clear_counter(target,name):
    counter = bfrt_info.table_get(name)
    zero_val_64='\x00\x00\x00\x00\x00\x00\x00\x00'
    ks = []
    dat = []
    x = counter.entry_get(target)
    for i in x:
        try:
            i[0]["$COUNTER_SPEC_BYTES"].val=zero_val_64
            ks.append(i[1])
            dat.append(i[0])
            continue
        except:
            print("perhaps its a packet counter")

        try:
            i[0]["$COUNTER_SPEC_PKTS"].val=zero_val_64
            ks.append(i[1])
            dat.append(i[0])
            continue
        except:
            print("perhaps its a byte conunter")
    counter.entry_mod(target,ks,dat)

def clear_counters():
    target=gc.Target(device_id=0, pipe_id=0xffff)
    names=counters
    for name in names:
        print("Clearing Counters: " + name)
        clear_counter(target, name)


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
            reg.entry_del(target)
            #table.default_entry_reset(target)
            #flush_register(reg, target, name)
        except Exception as ex:
            template = "An exception of type {0} occurred. Arguments:\n{1!r}"
            message = template.format(type(ex).__name__, ex.args)
            print(message)
        # except:
        #     print(name + " does not seem to exist")


def add_forwarding_rules(table):
    target=gc.Target(device_id=0, pipe_id=0xffff)
    forward_table = bfrt_info.table_get("SwitchIngress.forward")

    key_list=[]
    data_list=[]
    for entry in table:
        print(entry)
        port=entry[0]
        mac=int(EUI(entry[1]))
        key_list.append(forward_table.make_key([gc.KeyTuple("hdr.ethernet.dstAddr", mac )]))
        data_list.append(forward_table.make_data([gc.DataTuple('port', port)], "SwitchIngress.set_egr"))
    forward_table.entry_add(target,key_list,data_list)



clear_forwarding_rules()
clear_registers()
clear_counters()
add_forwarding_rules(mac_forwarding_table)


