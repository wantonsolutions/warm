from netaddr import IPAddress
from netaddr import EUI


#ip_map=[()]


#client
yeti_5_mac="ec:0d:9a:68:21:a0"
yeti_5_port=0
#memory
yak_0_mac="ec:0d:9a:68:21:d0"
yak_0_port=1
#meta
yak_1_mac="ec:0d:9a:68:21:cc"
yak_1_port=2

mac_forwarding_table=[
    (yak_0_port,yak_0_mac),
    (yeti_5_port,yeti_5_mac),
    (yak_1_port,yak_1_mac),
]

def flush_table(table, target):
    ks = []
    x = table.entry_get(target)
    for i in x:
        ks.append(i[1])
    table.entry_del(target, ks)

def clear_forwarding_rules():
    target=gc.Target(device_id=0, pipe_id=0xffff)
    forward_table = bfrt_info.table_get("SwitchIngress.forward")
    flush_table(forward_table,target)


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


ip=int(IPAddress("192.169.1.1"))
mac = int(EUI('ec:f4:bb:87:2d:0c'))
print(mac)

# target=gc.Target(device_id=0, pipe_id=0xffff)
# forward_table = bfrt_info.table_get("SwitchIngress.forward")
# key_list=[forward_table.make_key([gc.KeyTuple("hdr.ethernet.dstAddr", mac )])]
# data_list=[forward_table.make_data([gc.DataTuple('port', 1)], "SwitchIngress.set_egr")]

# forward_table.entry_add(target,key_list,data_list)

add_forwarding_rules(mac_forwarding_table)
clear_forwarding_rules()



# tables=bfrt_info.table_dict.keys()
# for name in tables:
#     if name.split('.')[0] == "forward":
        



#forward

# p4 = bfrt.redis.pipe

# dst_port_mapping = {
#     '10.0.0.1'      : 2,
#     '10.0.0.2'      : 3
# }

# def clear_all(verbose=True, batching=True):
#     global p4
#     for table_types in (['MATCH_DIRECT', 'MATCH_INDIRECT_SELECTOR'],
#                         ['SELECTOR'],
#                         ['ACTION_PROFILE']):
#         for table in p4.info(return_info=True, print_info=False):
#             if table['type'] in table_types:
#                 if verbose:
#                     print("Clearing table {:<40} ... ".
#                         format(table['full_name']), end='', flush=True)
#                 table['node'].clear(batch=batching)
#                 if verbose:
#                     print('Done')

# clear_all()

# for dst in dst_port_mapping:
#     p4.Ingress.ipv4_host.add_with_send(dst_addr=IPAddress(dst), port=dst_port_mapping[dst])
# bfrt.complete_operations()

# p4.Ingress.ipv4_host.dump(table=True)