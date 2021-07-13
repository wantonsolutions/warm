#!/usr/bin/python3
import csv
import sys 

usage='./aggregate_client_results filename clients_count (ex ./aggregate_client_resuts results.dat 3)'

if len(sys.argv) != 3:
    print("")

filename=sys.argv[1]
clients=sys.argv[2]
print("filename: "+filename+" clients "+ clients)

clients=int(clients)

output_arr = []
  
with open(filename,'r') as csvfile:
    data = csv.reader(csvfile, delimiter = ',')
    tmp_row = [0,0,0,"",0,0,""] 
    i=0
    for row in data:
        tmp_row[0] = tmp_row[0] + int(float(row[0]))    #throughput
        tmp_row[1] = tmp_row[1] + int(row[1])    #threads
        tmp_row[2] = row[2]                 #keyspace
        tmp_row[3] = row[3]                 #distribution
        tmp_row[4] = row[4]                 #r/w ratio
        tmp_row[5] = row[5]                 #payload
        tmp_row[6] = row[6]                 #tag
        i=i+1
        if i == clients:
            print(tmp_row)
            output_arr.append(tmp_row)
            tmp_row = [0,0,0,"",0,0,""] 
            i=0

for row in output_arr:
    print(str(row[0])+ "," + str(row[1])+ "," + str(row[2])+ "," + str(row[3])+ "," + str(row[4])+ "," + str(row[5])+ "," + str(row[6]))


        