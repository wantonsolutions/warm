#!/bin/bash

function clean_pswitch() {
    echo "waiting on pswitch"
    ssh pswitch -x "source ~/.bash_profile; cd /root/src/warm/rmemc-p4; ./run_pd_rpc.py ./switch_commands/setup_switch.py"
}

function kill_all_clients() {
    ssh b09-27 'sudo killall memcached'
    ssh yak0 'echo iwicbV15 | sudo -S killall init'
    ssh yak1 'echo iwicbV15 | sudo -S killall init'
    ssh yeti5 'echo iwicbV15 | sudo -S killall init'
}

export LCLOVER_THREADS=$1
export LCLOVER_KEY_RANGE=$2
export LCLOVER_YCSB_OP_MODE=$3
export LCLOVER_PAYLOAD_SIZE=$4

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
    ssh yeti5 $fullBuild &

    wait
    echo "BUILD COMPLETE"
    sleep 1
    echo "BUILD COMPLETE double check"
}


function start_machines() {
    echo "Starting up the machines for this experiment"
    #start up the program
    memcachedSource=`cat memcached_server.sh`
    ssh b09-27 $memcachedSource &
    sleep 1

    #yak1 memory
    memorySource=`cat mem_server.sh`
    echo "sshing to the memory server"
    echo "$memorySource"
    ssh yak1 $memorySource &
    sleep 1

    #yak0 meta
    metaSource=`cat meta_server.sh`
    ssh yak0 $metaSource &
    sleep 1

    #yeti 5 client
    clientSource1=`cat client_server_1.sh`
    ssh yeti5 "$clientSource1" &
    sleep 1

    echo "FINSHED ALL THE LAUNCHING SCRIPTS WAITING For experiment to complete"
    wait
    echo "DONE RUNNING"

}

function clean_up() {
    scp yeti5:/home/ssgrant/pDPM/clover/clean_1.dat clean_1.dat
    tail -1 clean_1.dat >> results.dat
    tail -1 clean_1.dat >> latest.dat
}


echo "Bencmark Script $0"
clean_pswitch
kill_all_clients
build_clover
start_machines
clean_up