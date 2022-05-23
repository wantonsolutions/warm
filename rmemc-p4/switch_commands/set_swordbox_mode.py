from common import *

swordbox_off_mode=0
write_steering_mode=1
read_steering_mode=2

mode_dict = {
    swordbox_off_mode : "swordbox_off_mode",
    write_steering_mode : "write_steering_mode",
    read_steering_mode : "read_steering_mode"
}

def mode_string(raw_value):
    if not raw_value in mode_dict:
        print("ERROR NOT MODE SPECIFIED TURNING STEERING OFF")
        return "swordbox_off_mode"
    return mode_dict[raw_value]

def set_swordbox_mode(mode_value, gc, info):

    target=gc.Target(device_id=0, pipe_id=0xffff)
    name="SwitchIngress.swordbox_mode"
    swordbox_mode = info.table_get(name)

    mode_table = swordbox_mode.entry_get(target)

    mode_entry = 0
    for i in mode_table:
        mode_entry = i
        break

    key=[mode_entry[1]]


    int_mode_val = int.from_bytes(mode_entry[0][name+str(".f1")].val, "big")
    old_value=mode_string(int_mode_val)
    new_value=mode_string(mode_value)

    print("Switching mode from OLD VALUE: " + old_value + " To NEW VALUE: " + new_value)

    mode_entry[0][name+str(".f1")].val=bytearray([mode_value])
    value=[mode_entry[0]]
    swordbox_mode.entry_mod(target,key,value)