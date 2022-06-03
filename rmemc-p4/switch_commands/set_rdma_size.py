from common import *

RDMA_128=128
RDMA_256=256
RDMA_512=512
RDMA_1024=1024

size_dict = {
    RDMA_128 : "RDMA_128",
    RDMA_256 : "RDMA_256",
    RDMA_512 : "RDMA_512",
    RDMA_1024 : "RDMA_1024"
}


def size_string(raw_value):
    if not raw_value in size_dict:
        print("ERROR NOT SIZE NOT SPECIFIED RETURNING TO 128")
        return size_dict[RDMA_128]
    return size_dict[raw_value]

def set_rdma_size(size_value, gc, info):

    target=gc.Target(device_id=0, pipe_id=0xffff)
    name="SwitchIngress.rdma_size"
    rdma_size = info.table_get(name)

    size_table = rdma_size.entry_get(target)

    size_entry = 0
    for i in size_table:
        size_entry = i
        break

    key=[size_entry[1]]


    int_size_val = int.from_bytes(size_entry[0][name+str(".f1")].val, "big")
    old_value=size_string(int_size_val)
    new_value=size_string(size_value)

    print("Switching rdma payload size from OLD VALUE: " + old_value + " To NEW VALUE: " + new_value)
    
    if size_value == RDMA_128:
        val=b'\x00\x00\x00\x80'
    elif size_value == RDMA_256:
        val='\x00\x00\x01\x00'
    elif size_value == RDMA_512:
        val='\x00\x00\x02\x00'
    elif size_value == RDMA_1024:
        val='\x00\x00\x04\x00'
    else:
        print("Size of " + str(size_value) + " Not available for setting")
    #print(size_value)
    #print(val)

    

    size_entry[0][name+str(".f1")].val=val
    
    #bytearray([size_value])
    value=[size_entry[0]]
    rdma_size.entry_mod(target,key,value)