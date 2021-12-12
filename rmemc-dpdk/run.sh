#!/bin/bash

if [[ $1 == -p ]]; then
    echo "iwicbV15" | sudo -S -E ./build/app/rmemc-dpdk -l 0 -n 1
    exit
fi

#clear the terminal
clear

if [[ $1 == -b ]]; then
    echo "about to clean"
    make clean

    #echo "setting RTE_SDK to vtune"
    #export RTE_SDK=/usr/local/ssgrant/dpdk-stable-19.11.5/vtune_install/share/dpdk
    export RTE_SDK=/usr/local/ssgrant/dpdk-stable-19.11.5/submission_install/usr/local/ssgrant/dpdk-stable-19.11.5/share/dpdk
    #export RTE_SDK=/usr/local/ssgrant/dpdk-stable-19.11.5/myinstall/share/dpdk
    #export RTE_SDK=/usr/local/ssgrant/dpdk-stable-19.11.5/fastinstall/share/dpdk
    echo "RTE_SDK=$RTE_SDK"

    make -j 30

fi

pushd build


#sudo -E ./basicfwd -l 0,2,4,6,8,10,12,14,16,18,20,22,24 -n 18
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0,2,4,6 -n 4
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0,2,4,6,8,10,12,14 -n 8
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0,2 -n 4
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0 -n 1
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0,2,4,6 -n 4
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0,2,4,6,8,10,12,14,16,18,20,22 -n 12
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0,2,4,6,8,10,12,14,16,18,20,22 -n 12
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0,2 -n 2
#gdb --args rmemc-dpdk -l 0 -n 1
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0,2,4 -n 3
#echo "iwicbV15" | sudo -S -E catchsegv ./rmemc-dpdk -l 0,2,4 -n 3
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0,2,4 -n 3
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0,2,4,6 -n 4

#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0,2,4,6,8,10,12,14,16,18,20,22,24 -n 14
echo "iwicbV15" | sudo -S -E ./rmemc-dpdk \
    --socket-mem=2048 \
    -l 0,2,4,6,8,10,12,14,16,18,20,22,24 -n 14 -- 
    #-l 0,2,4,6,8,10,12,14 -n 8 -- 
    #-l 0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38 -n 20 -- 


    #-w 0000:04:00.0,rx_vec_en=1,mprq_en=1,txq_inline_max=128\



   # rxq_cqe_comp_en=1,mprq_en=1' \
#,30,32,34,36,38,40,42,44,46 -n 24
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0 -n 1
popd

echo "completed running the switch"
pushd plot
python3 latency.py
popd
#echo "iwicbV15" | sudo -S -E ./rmemc-dpdk -l 0,2,4,6,8,10,12 -n 7
