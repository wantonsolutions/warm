all_registers = [
    "SwitchIngress.id_exists_reg",
    "SwitchIngress.id_counter",
    "SwitchIngress.qp_id_reg",
    "SwitchIngress.latest_keys",
    "SwitchIngress.next_vaddr_low",
    "SwitchIngress.next_vaddr_high",
    "SwitchIngress.outstanding_write_vaddr_low",
    "SwitchIngress.outstanding_write_vaddr_high",
    #"SwitchIngress.write_cache_low",
    #"SwitchIngress.write_cache_high",
    #"SwitchIngress.write_cache_key",
    "SwitchIngress.read_tail_low",
    "SwitchIngress.read_tail_high",
]

#memory
yak_0_mac="ec:0d:9a:68:21:d0"
yak_0_port=48
#meta
yak_1_mac="ec:0d:9a:68:21:cc"
yak_1_port=52
#client
yeti_5_mac="ec:0d:9a:68:21:a0"
yeti_5_port=56

mac_forwarding_table=[
    (yak_0_port,yak_0_mac),
    (yeti_5_port,yeti_5_mac),
    (yak_1_port,yak_1_mac),
]


def set_register(table, target, name, value):
    ks = []
    dat = []
    print("Setting Register: "+name+ " Size: "+ str(table.info.size) + "...")

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