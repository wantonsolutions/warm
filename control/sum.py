# importing csv module 
import csv 
import sys
  
# csv file name 
clients = int(sys.argv[1])
filename = sys.argv[2]
#filename = "sum.dat"
  
# initializing the titles and rows list 
fields = [] 
rows = [] 
  
# reading csv file 
with open(filename, 'r') as csvfile: 
    csvreader = csv.reader(csvfile) 
    for row in csvreader: 
        rows.append(row) 

sum_val = 0
sum_thread = 0
sum_ops = 0
thread_val=int(float(rows[0][1]))

index_counter = 0
#print("init thread val",thread_val)
#print("Starting Thread Value %s"%thread_val)
for row in rows: 
    if index_counter >= clients: 
        index_counter=0
        i=0
        for col in row: 
            if i==0:
                print(str(sum_val)+",", end = '')
            elif i==1:
                print(str(sum_thread)+",", end = '')
            elif i==6:
                print(str(sum_ops)+",", end = '')
            else:
                print("%s,"%col, end = '')
            i=i+1
        sum_val=0
        thread_val=int(row[1])
        sum_thread=0
        sum_ops=0
        print()
    index_counter = index_counter + 1
    sum_thread = int(row[1]) + sum_thread
    sum_val = int(float(row[0])) + sum_val
    sum_ops = int(float(row[6])) + sum_ops
    #print("summing")
    #print(row)

row = rows[-1]
i=0
for col in row: 
    if i==0:
        print(str(sum_val)+",", end = '')
    elif i==1:
        print(str(sum_thread)+",", end = '')
    elif i==6:
        print(str(sum_ops)+",", end = '')
    else:
        print("%s,"%col, end = '')
    i=i+1
print()
