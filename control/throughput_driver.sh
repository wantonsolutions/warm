#!/bin/bash
#rm results.dat
date=`date`
echo ">>>>>>>>> ($date)" >> results.dat
#threads=("1" "2" "4" "8")
threads=("16")
#opmodes=("MITSUME_YCSB_MODE_A" "MITSUME_YCSB_MODE_B" "MITSUME_YCSB_MODE_C")
#packet_size=("1000" "500" "250" "125" "64" "32")
packet_size=("500")
#opmodes=("MITSUME_YCSB_MODE_C")
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
