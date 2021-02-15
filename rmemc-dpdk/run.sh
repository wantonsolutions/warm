#!/bin/bash

echo "about to clean"
make clean
make -j 30

pushd build
#sudo -E ./basicfwd -l 0,2,4,6,8,10,12,14,16,18,20,22,24 -n 18
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0,2,4 -n 2
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0,2 -n 2
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0,2,4,6 -n 4
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0,2,4,6,8,10,12,14,16,18,20,22,24 -n 18
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0 -n 2
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0,2 -n 2
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0 -n 2
echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0,2,4,6,8,10,12 -n 8
