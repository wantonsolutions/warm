from common import *
import sys

def set_key_tracking(keys_to_track):
    name="SwitchIngress.track_key"
    target=gc.Target(device_id=0, pipe_id=0xffff)
    table = bfrt_info.table_get(name)

    print("tracking " + str(keys_to_track) + " keys")

    ks = []
    dat = []
    print("Clearing Register: "+name+ " Size: "+ str(table.info.size) + "...")

    x = table.entry_get(target)
    counter=0
    for i in x:
        ks.append(i[1])
        #byte_len=len(i[0][name+str(".f1")].val)
        if counter < keys_to_track:
            pval="1"
            val='\x01'
        else:
            pval="0"
            val='\x00'

        print("counter:" + str(counter) + " val:" + pval)


        byte_len=len(i[0][name+str(".f1")].val)
        print(byte_len)
        i[0][name+str(".f1")].val=val
        dat.append(i[0])
        counter = counter + 1
    
    table.entry_mod(target,ks,dat)

    # try
    #     #table.default_entry_reset(target)
    #     table.entry_del(target)
    # except:
    #     print("wtf - 2")
    print("Done")

track_keys_var='TRACK_KEYS'
number_track_keys = os.getenv(track_keys_var)
if (number_track_keys == None):
    print(track_keys_var + " not set env")
    exit(1)
print("TOTAL KEYS: " + number_track_keys)
set_key_tracking(int(number_track_keys))