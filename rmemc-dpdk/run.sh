#!/bin/bash

if [[ $1 == -b ]]; then
    echo "about to clean"
    make clean
    make -j 30
fi

pushd build
#sudo -E ./basicfwd -l 0,2,4,6,8,10,12,14,16,18,20,22,24 -n 18
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0,2,4 -n 2
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0,2 -n 2
echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0 -n 1
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0,2,4,6,8,10,12,14,16,18,20,22,24 -n 18
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0 -n 1
#gdb --args rmemc-dpdk -l 0 -n 1
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0,2 -n 2
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0,2,4 -n 3
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0,2,4,6,8 -n 5
popd

echo "completed running the switch"
pushd plot
python3 latency.py
popd
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0,2,4,6,8,10,12 -n 7
