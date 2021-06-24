#!/bin/bash
#rm results.dat
rm latest.dat
date=`date`
echo ">>>>>>>>> ($date)" >> results.dat
#threads=("1" "2" "4" "8")
threads=("1" "2" "4" "8" "16")
#threads=("8" "16")
#threads=("32")
#threads=("20" "24" "28" "32")
#threads=("16")
#threads=()
#threads=("1" "2" "4" "8" "16" "24" "32" "48")
#threads=("16" "16" "16")
#threads=("12" "12" "12")
#threads=("8" "8" "8" "8")
#threads=("16" "16" "16" "16")
#threads=("24" "24" "24")
#opmodes=("MITSUME_YCSB_MODE_A" "MITSUME_YCSB_MODE_B" "MITSUME_YCSB_MODE_C")
#packet_size=("1000" "500" "250" "125" "64" "32")
#packet_size=("500" "250" "125" "64" "32")
packet_size=("1000")
#packet_size=("1000" "900" "800" "700" "600" " 500")
#opmodes=("MITSUME_YCSB_MODE_A" "MITSUME_YCSB_MODE_B" "MITSUME_YCSB_MODE_C")
opmodes=("MITSUME_YCSB_MODE_A")
#keyspaces=("100" "1000" "10000" "100000")
keyspaces=("1000")

for k in ${keyspaces[@]}; do
    for i in ${opmodes[@]}; do
        for j in ${threads[@]}; do
            for l in ${packet_size[@]}; do
                echo "op modes: $i"
                echo "Threads: $j"
                echo "Keys: $k"
                echo "Packet Size: $l"
                ./throughput_bench.sh $j $k $i $l
                #exit 1
            done
        done
    done
done
echo "<<<<<<<<< ($date)" >> results.dat


python3 sum.py latest.dat > latest_clean.dat
