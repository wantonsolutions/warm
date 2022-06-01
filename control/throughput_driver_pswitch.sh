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
else
    echo "THE SWITCH IS NOT RUNNING"
    echo "Go start the switch manually"
    echo "ssh -vX pswitch  -x 'cd /root/src/warm/rmemc-p4; ./go.sh'"
    exit 1
fi


rm latest.dat
rm latest_bandwidth.dat
rm latest_latency.dat

date=`date`
echo ">>>>>>>>> ($date)" >> results.dat


#some precooked measurements
#packet_size vs throughput
#packet_size=("100" "200" "500" "1000")
#packet_size=("500")
#packet_size=("1000")
#packet_size=("1000" "900" "800" "700" "600" " 500")
packet_size=("1000")
keyspaces=("1000")

#threads=("1" "2" "3" "4" "5" "6" "7" "8" "9" "10" "11" "12" "13" "14" "15" "16" "17" "18" "19" "20" "21" "22" "23" "24" "25" "26" "27" "28" "29" "30" "31" "32" "33" "34" "35" "36" "37" "38" "39" "40")
#threads=("40" "48" "56" "64" "72" "80")
#threads=("1" "2" "4" "8" "16" "24" "32" "40" "48" "56")
#threads=("1" "2" "4" "8" "16" "24" "32" "40" "48" "56" "64" "72" "80")
threads=("1" "2" "4" "8" "16" "24" "32" "40")
#threads=("16" "32" "40")
#threads=("32")
#threads=("40")
#threads=("40")
#threads=("40")
#threads=("48")
#threads=("24" "32" "40")
#threads=("40" "40" "40")
#threads=("16" "24" "32" "40")
#threads=("1" "2" "4" "8" "16" "32")
#threads=("1")
#threads=("32" "32" "32" "32" "32")
#zipfs=("0.00" "0.60" "0.80" "0.90" "1.00" "1.10" "1.20" "1.30" "1.40" "1.50")
zipfs=("1.00")

#opmodes=("MITSUME_YCSB_MODE_C" "MITSUME_YCSB_MODE_B" "MITSUME_YCSB_MODE_A" "MITSUME_YCSB_MODE_W")
#opmodes=("MITSUME_YCSB_MODE_C" "MITSUME_YCSB_MODE_B" "MITSUME_YCSB_MODE_A" "MITSUME_YCSB_MODE_W")
#opmodes=("MITSUME_YCSB_MODE_B")
#opmodes=("MITSUME_YCSB_MODE_B" "MITSUME_YCSB_MODE_A" "MITSUME_YCSB_MODE_W")
#opmodes=("MITSUME_YCSB_MODE_B" "MITSUME_YCSB_MODE_W")
#opmodes=("MITSUME_YCSB_MODE_W")
opmodes=("MITSUME_YCSB_MODE_A" "MITSUME_YCSB_MODE_W")
#opmodes=("MITSUME_YCSB_MODE_C" "MITSUME_YCSB_MODE_B" "MITSUME_YCSB_MODE_W")
switch_modes=("SWORDBOX_OFF" "WRITE_STEER" "READ_STEER")
#switch_modes=("SWORDBOX_OFF")
#switch_modes=("SWORDBOX_OFF")
#switch_modes=("WRITE_STEER")
#switch_modes=("READ_STEER")

trials="5"

test_name="nill"
if [ ! -z $1 ]; then
    test_name=$1
fi

echo $test_name

if [ $test_name == "--packet-size" ]; then
    packet_size=("100" "200" "500" "1000")
    keyspaces=("1000")
    threads=("40")
    zipfs=("0.80")
    opmodes=("MITSUME_YCSB_MODE_A")
    switch_modes=("SWORDBOX_OFF" "WRITE_STEER" "READ_STEER")
fi

#conntention
if [ $test_name == "--contention" ]; then
    packet_size=("100")
    keyspaces=("1000")
    threads=("40")
    zipfs=("0.00" "0.60" "0.80" "0.90" "1.00" "1.10" "1.20" "1.30" "1.40" "1.50")
    opmodes=("MITSUME_YCSB_MODE_A")
    switch_modes=("SWORDBOX_OFF" "WRITE_STEER" "READ_STEER")
fi



for workload in ${opmodes[@]}; do
for switch_mode in ${switch_modes[@]}; do
for thread_count in ${threads[@]}; do
for payload_size in ${packet_size[@]}; do
for keys in ${keyspaces[@]}; do
for zipf in ${zipfs[@]}; do

        echo "switch mode" "$switch_mode"
        echo "op modes: $workload"
        echo "Threads: $thread_count"
        echo "Keys: $keys"
        echo "Packet Size: $payload_size"
        echo "zipf: $zipf"
        ./throughput_bench_pswitch.sh $thread_count $keys $workload $payload_size $switch_mode $zipf $trials
done #zipf
done #keys
done #payload size
done #thread count
        echo "Mode: " $switch_mode " Workload: " $workload >> results.dat
        ./clean_results.sh 1
        rm latest.dat
done #switch mode
done #workload
echo "<<<<<<<<< ($date)" >> results.dat