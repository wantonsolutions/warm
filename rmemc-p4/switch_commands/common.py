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
    "SwitchIngress.read_miss_counter",
]


counter_registers = [
    "SwitchIngress.id_counter",
    "SwitchIngress.read_miss_counter",
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
#client 2
yeti_0_mac="ec:0d:9a:68:21:b4"
yeti_0_port=60
#client 3
yeti_1_mac="ec:0d:9a:68:21:88"
yeti_1_port=40

mac_forwarding_table=[
    (yak_0_port,yak_0_mac),
    (yeti_5_port,yeti_5_mac),
    (yeti_0_port,yeti_0_mac),
    (yeti_1_port,yeti_1_mac),
    (yak_1_port,yak_1_mac),
]

