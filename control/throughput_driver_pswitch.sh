#!/bin/bash

#start up the programmable switch and check if anything is running on it

function kill_switch() {
    echo "SSH killing the switch"
    printf '\xE2\x98\xA0'
    printf '\xE2\x98\xA0'
    echo ""

    ssh pswitch -x 'killall run_switchd.sh'
}

response=`ssh pswitch -x 'ps -e | grep run_switchd.sh'`
echo $response
if [ ! -z "$response" ]; then

    echo "Switch Process Currently Running..."
    # echo "would you like to kill it? y/n"
    # read user_input
    # if [ $user_input == 'y' ]; then
    #     echo "killing switch"
    #     kill_switch
    # elif [ $user_input == 'n' ]; then
    #     echo "leaving the switch running"
    #     start_switch="false"
    # else
    #     echo "unknown command, killing the switch anyways for a clean restary you sillybilly"
    #     kill_switch
    # fi
else
    echo "Go start the switch manually"
    echo"ssh -vX pswitch  -x 'cd /root/src/warm/rmemc-p4; ./go.sh'"
    exit 1
fi


rm latest.dat
rm latest_bandwidth.dat
rm latest_latency.dat

date=`date`
echo ">>>>>>>>> ($date)" >> results.dat

#threads=("72")
#opmodes=("MITSUME_YCSB_MODE_A" "MITSUME_YCSB_MODE_B" "MITSUME_YCSB_MODE_C")
#packet_size=("1000" "500" "250" "125" "64" "32")
#packet_size=("500" "250" "125" "64" "32")
#packet_size=("1000")
#packet_size=("1000")
#threads=("1" "2" "3" "4" "5" "6" "7" "8" "9" "10" "11" "12" "13" "14" "15" "16" "17" "18" "19" "20")
threads=("32" "32" "32" "32" "32")
packet_size=("1000")
#packet_size=("1000" "900" "800" "700" "600" " 500")
#opmodes=("MITSUME_YCSB_MODE_A" "MITSUME_YCSB_MODE_B" "MITSUME_YCSB_MODE_C" "MITSUME_YCSB_MODE_W")
#opmodes=("MITSUME_YCSB_MODE_B" "MITSUME_YCSB_MODE_W")
#opmodes=("MITSUME_YCSB_MODE_A")
opmodes=("MITSUME_YCSB_MODE_A")
#opmodes=("MITSUME_YCSB_MODE_A")
#keyspaces=("100" "1000" "10000" "100000")
keyspaces=("1000")

for k in ${keyspaces[@]}; do
    for i in ${opmodes[@]}; do
        for j in ${threads[@]}; do
            for l in ${packet_size[@]}; do
                echo "op modes: $i"
                echo "Threads: $j"
                echo "Keys: $k"
                echo "Packet Size: $l"
                ./throughput_bench_pswitch.sh $j $k $i $l
            done
        done
    done
done
echo "<<<<<<<<< ($date)" >> results.dat

./clean_results.sh 1
