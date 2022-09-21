#!/bin/bash


export LCLOVER_THREADS=$1
export LCLOVER_KEY_RANGE=$2
export LCLOVER_YCSB_OP_MODE=$3
export LCLOVER_PAYLOAD_SIZE=$4
export SWITCH_MODE=$5
export ZIPF_DIST=$6
export TRIALS=$7
export LCLOVER_TRACKED_KEYS=$8

#export HOSTS=("yeti5")
#export HOSTS=("yeti5" "yeti0")
#export HOSTS=("yeti5" "yeti0" "yeti8")
#export HOSTS=("yeti5" "yeti0" "yeti8" "yeti1")
#export HOSTS=("yeti5" "yeti0" "yeti1" "yeti2" "yeti3")
export HOSTS=("yeti5" "yeti0" "yeti1" "yeti2" "yeti3" "yeti8")
#export HOSTS=("yeti5" "yeti0" "yeti1" "yeti2" "yeti3" "yeti8" "yeti4")
#export HOSTS=("yeti5" "yeti8")
export sleep_time="20"



function clean_pswitch() {
    echo "waiting on pswitch"
    ssh pswitch -x "source ~/.bash_profile; cd /root/src/warm/rmemc-p4; ./run_pd_rpc.py ./switch_commands/setup_switch.py"
}

function get_switch_statistics() {
    echo "getting stats"
    ssh pswitch -x "source ~/.bash_profile; cd /root/src/warm/rmemc-p4; ./run_pd_rpc.py ./switch_commands/get_switch_statistics.py"
    scp "pswitch:/root/src/warm/rmemc-p4/Switch_Statistics.dat" ./

}

function set_pswitch_mode() {
    mode=$SWITCH_MODE
    mode_script="set_off_mode.py"
    if [ $mode == "SWORDBOX_OFF" ];then
        mode_script="set_off_mode.py"
    elif [ $mode == "WRITE_STEER" ];then
        mode_script="set_write_steering_mode.py"
    elif [ $mode == "READ_STEER" ]; then
        mode_script="set_read_steering_mode.py"
    else
        echo "BAD MODE SCRIPT $mode setting pswitch to default $mode_script"
    fi
    echo "setting switch to $mode with $mode_script"
    ssh pswitch -x "source ~/.bash_profile; cd /root/src/warm/rmemc-p4; ./run_pd_rpc.py ./switch_commands/$mode_script"
}

function set_rdma_size() {
    size=$LCLOVER_PAYLOAD_SIZE
    size_script="set_rdma_size_128.py"
    if [ $size == "100" ]; then
        size_script="set_rdma_size_128.py"
    elif [ $size == "200" ]; then
        size_script="set_rdma_size_256.py"
    elif [ $size == "500" ];then
        size_script="set_rdma_size_512.py"
    elif [ $size == "1000" ];then
        size_script="set_rdma_size_1024.py"
    else
        echo "BAD RDMA SIZE $size setting to default $size_script"
    fi
    echo "setting switch to RDMA SIZE $size with script $size_script"
    ssh pswitch -x "source ~/.bash_profile; cd /root/src/warm/rmemc-p4; ./run_pd_rpc.py ./switch_commands/$size_script"
}

function set_tracked_keys() {
    size=$LCLOVER_TRACKED_KEYS
    echo "setting which keys to track"
    ssh pswitch -x "source ~/.bash_profile; cd /root/src/warm/rmemc-p4; export TRACK_KEYS=$size; ./run_pd_rpc.py ./switch_commands/set_key_track.py"
}

function kill_all_clients() {
    echo "Killing Memcached Server"
    memcached_server="b09-27"
    ssh $memcached_server 'sudo killall memcached' &

    echo "Killing Meta Server"
    meta_server="yak0"
    ssh $meta_server 'echo iwicbV15 | sudo -S killall init' &

    echo "Killing Memory Server"
    memory_server="yak1"
    ssh $memory_server 'echo iwicbV15 | sudo -S killall init' &

    for host in ${HOSTS[@]}; do
        echo "Killing Client Machine --> $host"
        ssh $host 'echo iwicbV15 | sudo -S killall init' &
    done
    wait

    echo "Done Killing"
}

function build_clover() {
    #build the project

    #Currently hardcoded, should be a parameter
    CLIENTS=1

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

    #here alone we treat yeti5 as special because it has a different package
    ssh yeti5 $fullBuild &

    wait
    echo "BUILD COMPLETE"
    sleep 1
    echo "BUILD COMPLETE double check"
}

function set_distribution() {
    echo "Setting Zipf to $ZIPF_DIST"
    ssh yeti5 -x "cd /home/ssgrant/pDPM/clover/workload/; ./make_active.sh zipf/$ZIPF_DIST"
}

function start_machines() {
    echo "Starting up the machines for this experiment"
    num_clients="${#HOSTS[@]}"
    #start up the program
    memcachedSource=`cat memcached_server.sh`
    memcachedSource=$(sed "s|%%SLEEP%%|$sleep_time|g" <<< $memcachedSource)
    memcachedSource=$(sed "s|%%NR_CN%%|$num_clients|g" <<< $memcachedSource)
    echo $memcachedSource

    ssh b09-27 $memcachedSource &

    #yak1 memory
    let "mem_id=${#HOSTS[@]}+1"
    memorySource=`cat mem_server.sh`
    memorySource=$(sed "s|%%SLEEP%%|$sleep_time|g" <<< $memorySource)
    memorySource=$(sed "s|%%ID%%|$mem_id|g" <<< $memorySource)
    memorySource=$(sed "s|%%NR_CN%%|$num_clients|g" <<< $memorySource)
    echo "sshing to the memory server"
    echo "$memorySource"
    ssh yak1 $memorySource &

    #yak0 meta
    meta_id="0"
    metaSource=`cat meta_server.sh`
    metaSource=$(sed "s|%%SLEEP%%|$sleep_time|g" <<< $metaSource)
    metaSource=$(sed "s|%%ID%%|$meta_id|g" <<< $metaSource)
    metaSource=$(sed "s|%%NR_CN%%|$num_clients|g" <<< $metaSource)
    ssh yak0 $metaSource &

    #start multiple clients
    let "i=1"
    for host in ${HOSTS[@]}; do
        #yeti 5 client
        #clientSource=`cat client_server_$i.sh`
        clientSource=`cat client_server.sh`
        clientSource=$(sed "s|%%ID%%|$i|g" <<< $clientSource)
        clientSource=$(sed "s|%%SLEEP%%|$sleep_time|g" <<< $clientSource)
        clientSource=$(sed "s|%%NR_CN%%|$num_clients|g" <<< $clientSource)

        echo "SSHING to host $host"
        ssh $host "$clientSource" &
        let "i=i+1"
    done

    echo "FINSHED ALL THE LAUNCHING SCRIPTS WAITING For experiment to complete"
    wait
    echo "DONE RUNNING"

}

function get_latency_stats() {
    echo "Collecting Latency stats from servers"
    pushd latency_scratch
    ./latency.sh
    popd
    cat latency_scratch/latency_result_latest.dat >> latest_latency.dat
}


RUN_SUCCESS="FALSE"

function clean_up() {

    tmp="tmp_results.dat"
    rm $tmp
    echo "processing bandwidth information"
    let "i=1"
    for host in ${HOSTS[@]}; do
        rm clean_$i.dat
        scp $host:/home/ssgrant/pDPM/clover/clean_$i.dat clean_$i.dat
        res=`tail -1 clean_$i.dat`
        echo $res

        echo " collecting on host $host"
        if [ -z "$res" ] ; then
            RUN_SUCCESS="FALSE"
            echo "Experiment Failed $host"
            return
        else

            echo "${res}" >> $tmp
        fi
        let "i=i+1"
    done

    RUN_SUCCESS="TRUE"

    combined=`python3 sum.py ${#HOSTS[@]} $tmp`

    get_latency_stats
    latency=`cat latency_scratch/latency_result_latest.dat`

    get_switch_statistics
    sed -i "s/,$//g" Switch_Statistics.dat
    statistics=`tail -1 Switch_Statistics.dat`

    echo "Experiment Complete"
    #echo "$combined" >> results.dat
    echo "${combined}${latency},${statistics}" >> bench_results.dat
}



echo "Bencmark Script $0"
rm bench_results.dat

build_clover
set_rdma_size
set_tracked_keys
set_pswitch_mode
set_distribution
for t in $(seq 1 $TRIALS); do

    #keep retring untill success
    RUN_SUCCESS="FALSE"
    while [ $RUN_SUCCESS == "FALSE" ]; do
        clean_pswitch
        kill_all_clients
        start_machines
        clean_up
    done

done




#combine results and product stderr
#inject zipf into bench results
#todo this is super quick and dirty, consider moving somewhere else
sed -i "s/zipf/${ZIPF_DIST}/g" bench_results.dat
sed -i "s/,@TAG@//g" bench_results.dat
sed -i "s/,$//g" bench_results.dat
res=`python3 stats.py bench_results.dat $TRIALS`

echo "$res" >> results.dat
echo "$res" >> latest.dat