#!/bin/bash
number_clients=$1

python3 sum.py $number_clients latest.dat > latest_clean.dat
lat=`awk -F "," '{ printf $1 "," }' latest_clean.dat`
echo "avg_ops=[$lat]" > latest_pythonready.dat
lat=`awk -F "," '{ printf $7 "," }' latest_clean.dat`
echo "total_ops=[$lat]" >> latest_pythonready.dat
threads=`awk -F "," '{ printf $2 "," }' latest_clean.dat`
echo "threads=[$threads]" >> latest_pythonready.dat
bandwidth=`awk -F "," '{ printf $1 "," }' latest_bandwidth.dat`
echo "bandwidth=[$bandwidth]" >> latest_pythonready.dat
read_latency_99=`awk -F "," '{ printf $1 "," }' latest_latency.dat`
echo "read_latency_99=[$read_latency_99]" >> latest_pythonready.dat
write_latency_99=`awk -F "," '{ printf $2 "," }' latest_latency.dat`
echo "write_latency_99=[$write_latency_99]" >> latest_pythonready.dat

cat latest_pythonready.dat