#!/bin/bash

#Begin by logging into each of the machines and killing any of the processes which might be running

#B09-27


ssh b09-27 'sudo killall memcached'
ssh yak0 'echo iwicbV15 | sudo -S killall init'
ssh yak1 'echo iwicbV15 | sudo -S killall init'

ssh yak2 'echo iwicbV15 | sudo -S killall basicfwd'


#example usage
#./throughput_bench.sh 16 1024 MITSUME_YCSB_MODE_A 1000


#build the project
export LCLOVER_THREADS=$1
export LCLOVER_KEY_RANGE=$2
export LCLOVER_YCSB_OP_MODE=$3
export LCLOVER_PAYLOAD_SIZE=$4

#Currently hardcoded, should be a parameter
CLIENTS=2

#setup envornment variables for the remote host
rm lenv.sh
echo "export CLOVER_THREADS=$LCLOVER_THREADS;" > lenv.sh
echo "export CLOVER_KEY_RANGE=$LCLOVER_KEY_RANGE;" >> lenv.sh
echo "export CLOVER_YCSB_OP_MODE=$LCLOVER_YCSB_OP_MODE;" >> lenv.sh
echo "export CLOVER_PAYLOAD_SIZE=$LCLOVER_PAYLOAD_SIZE;" >> lenv.sh
lenv=`cat lenv.sh`

#build clover
echo "Start builing clover"
buildSource=`cat build_clover.sh`
fullBuild=$lenv$buildSource
echo $fullBuild
ssh yak1 $fullBuild
echo "Building clover complete"

let "LCLOVER_THREADS=LCLOVER_THREADS * CLIENTS" 

rm lenv.sh
echo "export CLOVER_THREADS=$LCLOVER_THREADS;" > lenv.sh
echo "export CLOVER_KEY_RANGE=$LCLOVER_KEY_RANGE;" >> lenv.sh
echo "export CLOVER_YCSB_OP_MODE=$LCLOVER_YCSB_OP_MODE;" >> lenv.sh
echo "export CLOVER_PAYLOAD_SIZE=$LCLOVER_PAYLOAD_SIZE;" >> lenv.sh
echo "export CLOVER_CLIENTS=$LCLOVER_CLIENTS;" >> lenv.sh
lenv=`cat lenv.sh`

#build the middlebox switch
echo "Start builing switch"
buildSource=`cat build_switch.sh`
fullBuild=$lenv$buildSource
echo $fullBuild
ssh yak2 $fullBuild
echo "Building switch complete"

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

memorySource=`cat mem_server.sh`
echo "sshing to the memory server"
echo "$memorySource"
ssh yak1 $memorySource &

clientSource1=`cat client_server_1.sh`
ssh yak0 "$clientSource1" &
sleep 1

clientSource2=`cat client_server_2.sh`
echo "$clientSource2"
ssh yeti5 "$clientSource2" &



echo "first sleep"
sleep 1
echo "second sleep sleep"
sleep 1
echo "third sleep sleep"
sleep 1
echo "fourth sleep sleep"
sleep 1

echo "FINSHED ALL THE LAUNCHING SCRIPTS WAITING"

# this is the big sleep
wait
echo "DONE RUNNING"

scp yak0:/home/ssgrant/pDPM/clover/clean_1.dat clean_1.dat
scp yeti5:/home/ssgrant/pDPM/clover/clean_2.dat clean_2.dat

scp yak2:/tmp/switch_statistics.dat switch_statistics.dat

tail -1 clean_1.dat >> results.dat
tail -1 clean_2.dat >> results.dat
tail -1 clean_1.dat >> latest.dat
tail -1 clean_2.dat >> latest.dat

cat switch_statistics.dat >> agg_switch_statistics.dat

sleep 5


#'
#source ~\.bashrc;
#clover;
#echo iwicbV15 | sudo -S run_client.sh 0'