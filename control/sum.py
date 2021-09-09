# importing csv module 
import csv 
import sys
  
# csv file name 
filename = sys.argv[1]
#filename = "sum.dat"
  
# initializing the titles and rows list 
fields = [] 
rows = [] 
  
# reading csv file 
with open(filename, 'r') as csvfile: 
    # creating a csv reader object 
    csvreader = csv.reader(csvfile) 
      
    # extracting field names through first row 
    #fields = next(csvreader) 
  
    # extracting each data row one by one 
    for row in csvreader: 
        rows.append(row) 
  
    # get total number of rows 
    #print("Total no. of rows: %d"%(csvreader.line_num)) 
  
# printing the field names 
# print('Field names are:' + ', '.join(field for field in fields)) 
  
#  printing first 5 rows 
#print('\nFirst 5 rows are:\n') 
sum_val = 0
sum_thread = 0
thread_val=int(float(rows[0][1]))
#print("init thread val",thread_val)
#print("Starting Thread Value %s"%thread_val)
for row in rows: 
    if int(row[1]) != int(thread_val): 
        #print("dumping")
        i=0
        for col in row: 
            if i==0:
                print(str(sum_val)+",", end = '')
            elif i==1:
                print(str(sum_thread)+",", end = '')
            else:
                print("%s,"%col, end = '')
            i=i+1
        sum_val=0
        thread_val=int(row[1])
        sum_thread=0
        print()
    sum_thread = int(row[1]) + sum_thread
    sum_val = int(float(row[0])) + sum_val
    #print("summing")
    #print(row)

row = rows[-1]
i=0
for col in row: 
    if i==0:
        print(str(sum_val)+",", end = '')
    elif i==1:
        print(str(sum_thread)+",", end = '')
    else:
        print("%s,"%col, end = '')
    i=i+1
print()
