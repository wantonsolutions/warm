#!/bin/bash
#rm results.dat
date=`date`
echo ">>>>>>>>> ($date)" >> results.dat
threads=("1" "2" "4" "8")
<<<<<<< HEAD
#opmodes=("MITSUME_YCSB_MODE_A" "MITSUME_YCSB_MODE_B" "MITSUME_YCSB_MODE_C")
opmodes=("MITSUME_YCSB_MODE_A")
=======
opmodes=("MITSUME_YCSB_MODE_A" "MITSUME_YCSB_MODE_B" "MITSUME_YCSB_MODE_C")
#opmodes=("MITSUME_YCSB_MODE_C")
>>>>>>> e4ad39dc3fb9b27d822ad58843f6aabe6c7d7338
#keyspaces=("100" "1000" "10000" "100000")
keyspaces=("100000")

for k in ${keyspaces[@]}; do
    for i in ${opmodes[@]}; do
        for j in ${threads[@]}; do
            echo "op modes: $i"
            echo "Threads: $j"
            echo "Keys: $k"
            ./throughput_bench.sh $j $k $i
            #exit 1
        done
    done
done
echo "<<<<<<<<< ($date)" >> results.dat
