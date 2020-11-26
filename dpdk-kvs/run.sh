#!/bin/bash
export KEY_SIZE=4
export VALUE_SIZE=1024
make -j 8
cp build/kvs-shared ./
sudo -E ./kvs -l 0-5 -n 5 -- --rx="(0,0,0),(0,1,1),(0,2,2),(0,3,3)" --tx="(0,4)" --w="5" --rsz "4,4,4,4" --bsz="(1,1), (1,1), (1,1)" --lpm="192.0.0.0/8=>0;" --pos-lb 29 
#sudo -E ./kvs -l 0-5 -n 5 -- --rx="(0,0,0),(0,1,1),(0,2,2),(0,3,3)" --tx="(0,4)" --w="5" --lpm="105.0.0.0/8=>0;" --pos-lb 29

#–bsz “(A, B), (C, D), (E, F)”:
