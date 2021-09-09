#!/bin/bash
python3 sum.py latest.dat > latest_clean.dat
lat=`awk -F "," '{ printf $1 "," }' latest_clean.dat`
echo "latency=[$lat]" > latest_pythonready.dat
threads=`awk -F "," '{ printf $2 "," }' latest_clean.dat`
echo "threads=[$threads]" >> latest_pythonready.dat
cat latest_pythonready.dat