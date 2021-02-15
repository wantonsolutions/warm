#!/bin/bash

#Begin by logging into each of the machines and killing any of the processes which might be running

#B09-27


ssh b09-27 'sudo killall memcached'
ssh yak0 'echo iwicbV15 | sudo -S killall init'
ssh yak1 'echo iwicbV15 | sudo -S killall init'

ssh yak2 'echo iwicbV15 | sudo -S killall basicfwd'

#build the project
export LCLOVER_THREADS=$1
export LCLOVER_KEY_RANGE=$2
export LCLOVER_YCSB_OP_MODE=$3
export LCLOVER_PAYLOAD_SIZE=$4

rm lenv.sh
echo "export CLOVER_THREADS=$LCLOVER_THREADS;" > lenv.sh
echo "export CLOVER_KEY_RANGE=$LCLOVER_KEY_RANGE;" >> lenv.sh
echo "export CLOVER_YCSB_OP_MODE=$LCLOVER_YCSB_OP_MODE;" >> lenv.sh
echo "export CLOVER_PAYLOAD_SIZE=$LCLOVER_PAYLOAD_SIZE;" >> lenv.sh
lenv=`cat lenv.sh`

buildSource=`cat build.sh`
fullBuild=$lenv$buildSource
echo $fullBuild
ssh yak1 $fullBuild



#start the middlebox
echo "sshing yak2"
switchSource=`cat pswitch_server.sh`
ssh yak2 $switchSource &
sleep 2

#start up the program
memcachedSource=`cat memcached_server.sh`
ssh b09-27 $memcachedSource &
sleep 1

metaSource=`cat meta_server.sh`
ssh yak1 $metaSource &
sleep 1

clientSource=`cat client_server.sh`
ssh yak0 $clientSource &
sleep 1

memorySource=`cat mem_server.sh`
ssh yak1 $memorySource &
sleep 1

# this is the big sleep
wait
echo "DONE RUNNING"

scp yak1:/home/ssgrant/pDPM/clover/clean.dat clean.dat
tail -1 clean.dat >> results.dat

sleep 5


#'
#source ~\.bashrc;
#clover;
#echo iwicbV15 | sudo -S run_client.sh 0'