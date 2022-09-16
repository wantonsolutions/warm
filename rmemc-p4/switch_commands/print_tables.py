from common import *

def print_register(table,target,name):
    print("Printing Register: "+name+ " Size: "+ str(table.info.size) + "...")
    x = table.entry_get(target)
    total_set=0
    for i in x:
        key = i[1]
        key = i[1]['$REGISTER_INDEX'].value
        key=int.from_bytes(key, byteorder='big', signed=False)
        #print(key)
        data = i[0]
        data = data[name+str(".f1")].val
        data =int.from_bytes(data, byteorder='big', signed=False)
        if(data != 0):
            print("Key: "+ str(key) + "   Value: " + str(data) + " Hvalue: " + str(hex(data)))
            total_set=total_set+1

        #print(data[name+str(".f1")])
    print("Register: " + str(total_set) + "/" + str(table.info.size) +" full")



def print_registers(name_list):
    target=gc.Target(device_id=0, pipe_id=0xffff)
    names=name_list
    for name in names:
        try:
            reg = bfrt_info.table_get(name)
            print_register(reg, target, name)
        except:
            print(name + " does not seem to exist")


#print_registers(all_registers)
print_registers(counter_registers)