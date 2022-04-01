#!/bin/bash

#Begin by logging into each of the machines and killing any of the processes which might be running

#B09-27


ssh b09-27 'sudo killall memcached'
ssh yak0 'echo iwicbV15 | sudo -S killall init'
ssh yak1 'echo iwicbV15 | sudo -S killall init'
ssh yak2 'echo iwicbV15 | sudo -S killall basicfwd'
ssh yeti5 'echo iwicbV15 | sudo -S killall init'
ssh yeti0 'echo iwicbV15 | sudo -S killall init'


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
ssh yak1 $fullBuild &

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
ssh yak2 $fullBuild &

wait
echo "BUILD COMPLETE"

#start up the program
memcachedSource=`cat memcached_server.sh`
ssh b09-27 $memcachedSource &
sleep 1

#start the middlebox
echo "sshing yak2"
switchSource=`cat pswitch_server.sh`
ssh yak2 $switchSource &
sleep 2


memorySource=`cat mem_server.sh`
echo "$memorySource"
ssh yak1 $memorySource &
sleep 1

metaSource=`cat meta_server.sh`
ssh yak0 $metaSource &
sleep 1

clientSource1=`cat client_server_1.sh`
ssh yeti5 "$clientSource1" &
sleep 1

clientSource2=`cat client_server_2.sh`
echo "$clientSource2"
ssh yeti0 "$clientSource2" &


echo "FINSHED ALL THE LAUNCHING SCRIPTS WAITING"
wait
echo "DONE RUNNING"

echo "processing bandwidth information"
rm clean_1.dat
rm clean_2.dat

scp yeti5:/home/ssgrant/pDPM/clover/clean_1.dat clean_1.dat
scp yeti0:/home/ssgrant/pDPM/clover/clean_2.dat clean_2.dat

res1=`tail -1 clean_1.dat`
res2=`tail -1 clean_2.dat`

echo $res1
echo $res2

if [ -z "$res1" ] || [ -z "$res2" ]; then
    echo "Experiment Failed"
    exit 1
else
    echo "Experiment Complete"
    echo $res1 >> results.dat
    echo $res2 >> results.dat

    echo $res1 >> latest.dat
    echo $res2 >> latest.dat
fi

echo "Collecting stats from the middlebox"
scp yak2:/tmp/switch_statistics.dat switch_statistics.dat
scp yak2:/tmp/sequence_order.dat sequence_order.dat
cat switch_statistics.dat >> agg_switch_statistics.dat
cat switch_statistics.dat  | grep "Data Processed" | cut -d' ' -f 3 >> latest_bandwidth.dat

tail -1 switch_statistics.dat >> write_failures.dat
tail -2 switch_statistics.dat | head -1 >> read_failures.dat 

echo "Collecting Latency stats from servers"
pushd latency_scratch
./latency.sh
popd
cat latency_scratch/latency_result_latest.dat >> latest_latency.dat

#if we made it here we won
exit 0



