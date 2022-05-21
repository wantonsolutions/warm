#!/bin/bash

iface=""
ipaddr=""
numasocket=""
function set_server_params {
    hname=`hostname`
    echo "Configuring NIC on $hname"
    case $hname in
        "yak-00.sysnet.ucsd.edu")
            iface="enp4s0" #100gbps NICS Mellanox
            ipaddr="192.168.1.12"
            numasocket="0"
        ;;
        "yak-01.sysnet.ucsd.edu")
            #iface="enp129s0"
            iface="enp3s0"
            ipaddr="192.168.1.13"
            numasocket="1"
        ;;
        "yak-02.sysnet.ucsd.edu")
            iface="enp4s0"
            ipaddr="192.168.1.14"
            numasocket="0"
        ;;

        "yeti-03.sysnet.ucsd.edu")
            iface="enp101s0"
            ipaddr="192.168.1.15"
            numasocket="0"
        ;;
        "yeti-04.sysnet.ucsd.edu")
            iface="enp59s0"
            ipaddr="192.168.1.16"
            numasocket="0"
        ;;

        "yeti-05.sysnet.ucsd.edu")
            iface="ens4np0"
            ipaddr="192.168.1.17"
            numasocket="0"
        ;;

        #control server
        "b09-27.sysnet.ucsd.edu")
            iface="N/A"
            ipaddr="N/A"
            numasocket="N/A"
        ;;

        *)
        echo "$machine does not have a configuration set in this script. Exiting and doing nothing"
        exit 1
        ;;
    esac
}

function setup_nic {

    #set the link up
    sudo ip link set $iface up
    #turn the interface on and configure ip
    sudo ifconfig $iface $ipaddr netmask 255.255.0.0 
    echo "Nic Configuration complete Configuration complete"
}

function setup_hugepages {
    echo "Setting up hugepages"
    sudo hugeadm --pool-pages-min 2MB:8192
    sudo mkdir -p /mnt/hugetlbfs ; sudo mount -t hugetlbfs none /mnt/hugetlbfs

    #TODO set this up so that it maps correctly on yak0, 1 and 2
    sudo numactl --physcpubind="$numasocket" --localalloc LD_PRELOAD=libhugetlbfs.so HUGETLB_MORECORE=yes ls
    echo "HUGEPAGE INFO"
    cat /proc/meminfo | grep Huge
    echo "Hugepage setup complete"
}



function setup_switch {

    
    message="The switch requires openflow rules in order to forward packets to the yak-02 middlebox
    the commands to run are as follows
    
    ssh sw100
    [pw KaicyeVapUv9]

    enable
    configure terminal

    #enable the ports
    interface ethernet 1/3 openflow mode hybrid
    interface ethernet 1/29 openflow mode hybrid
    interface ethernet 1/30 openflow mode hybrid
    interface ethernet 1/31 openflow mode hybrid

    #Port Eth1/29 is Yak00 (192.168.1.12) (meta)
    #Port Eth1/30 is Yak01 (192.168.1.13) (memory)
    #Port Eth1/31 is Yak02 (192.168.1.14) (middle)
    #Port Eth1/3 is Yeti5 (192.168.1.17)  (client)

    #add redirection flows to table 0

    #this is for a two client setup where the middlebox is sitting behind 1/31
    openflow add-flows 1 ,table=0,in_port=Eth1/30,nw_src=192.168.1.12/32,nw_dst=192.168.1.11/32,actions=output=Eth1/31
    openflow add-flows 2 ,table=0,in_port=Eth1/29,nw_src=192.168.1.11/32,nw_dst=192.168.1.12/32,actions=output=Eth1/31
    openflow add-flows 3 ,table=0,in_port=Eth1/3,nw_src=192.168.1.17/32,nw_dst=192.168.1.12/32,actions=output=Eth1/31

    #for a single client setup where each machine has it's own physical machine
    #yak1 is memory
    #ya2 is the middlebox
    #yak0 is the metadata server
    #yeti-05 is the client
    ## the metadata server traffic should be uninterupted
    #
    ##in this case we don't want the middlebox to touch packets which are on
    #their way to and from the metadata server, so we route around it by ip
    #matching The key thing is to have the ip prefix after the flow number as seen below
    #
    #client to middlebox
    openflow add-flows 1 ip,table=0,in_port=Eth1/3,nw_src=192.168.1.17,nw_dst=192.168.1.13,actions=output=Eth1/31
    #memory to middlebox
    openflow add-flows 2 ip,table=0,in_port=Eth1/30,nw_src=192.168.1.13,nw_dst=192.168.1.17,actions=output=Eth1/31


    #show flows
    show openflow flows ethernet-names

    #we use static MAC address
    #Yak01 CX5
    mac-address-table static EC:0D:9A:68:21:CC vlan 1 interface ethernet 1/30
    #Yak00 CX5
    mac-address-table static EC:0D:9A:68:21:D0 vlan 1 interface ethernet 1/29
    #Yak02 CX5
    mac-address-table static EC:0D:9A:68:21:BC vlan 1 interface ethernet 1/31
    #Yeti5 CX5
    mac-address-table static EC:0D:9A:68:21:A0 vlan 1 interface ethernet 1/3

    #we use static MAC address
    #Yak01 CX6
    mac-address-table static 94:40:c9:8a:e6:3c vlan 1 interface ethernet 1/30
    #Yak00 CX6
    mac-address-table static 94:40:c9:8a:d6:fe vlan 1 interface ethernet 1/29



    #remove redirection flows
    openflow del-flows 1
    openflow del-flows 2
    openflow del-flows 3

    #change speed
    interface etherent 1/3  #where 1/3 is the port
    speed 100G force        #the force is there because without it the command fails

    #check congestion control
    show interfaces ethernet 1/30 congestion-control
    https://community.mellanox.com/s/article/howto-configure-roce-with-ecn-end-to-end-using-connectx-4-and-spectrum--trust-l2-x#jive_content_id_Test_the_RDMA_Layer
    "
    #echo -e $message

    #Make sure the switch is not configured with custom openflow rules
    #echo "Checking for openflow rules on the switch"
    pwd=$(cat switch.password)      # Tip: Use (chmod) 600 access for this file

    rules=$(sshpass -p "$pwd" ssh sw100 cli -h '"enable" "show openflow flows ethernet-names"')
    #echo $rules
    num_rules=$(echo "$rules" | grep "in_port" | wc -l)
    #echo $num_rules


    ONE_CLIENT_CONFIG="ONE_CLIENT_CONFIG"
    CONFIGURATION=$ONE_CLIENT_CONFIG

    if [[ $num_rules -eq 0 ]]; then
        echo "no custom openflow rules on switch; we're good!"
    else
        echo "rules exist on the switch clearing them off"
        sshpass -p "$pwd" ssh sw100 cli -h '"enable" "configure terminal" "openflow del-flows"'
    fi

    if [ $CONFIGURATION == $ONE_CLIENT_CONFIG ]; then
        echo "setting config for single client"
        #client (Yeti5) to middlebox (Yak2)
        yeti5_yak2="openflow add-flows 1 ip,table=0,in_port=Eth1/3,nw_src=192.168.1.17,nw_dst=192.168.1.13,actions=output=Eth1/31"
        yak1_yak2="openflow add-flows 2 ip,table=0,in_port=Eth1/30,nw_src=192.168.1.13,nw_dst=192.168.1.17,actions=output=Eth1/31"
        #memory (Yak 1) to middlebox (Yak2)
        sshpass -p "$pwd" ssh sw100 cli -h "\"enable\" \"configure terminal\" \"$yeti5_yak2\" \"$yak1_yak2\""
    else
        echo "No configuration defined"
    fi


    MAC_ADDRESS_CONFIG="CX5"
    MAC_ADDRESS_CONFIG="HYBRID_CX5_CX6"
    MAC_ADDRESS_CONFIG="NONE"

    yak1_cx6="mac-address-table static 94:40:c9:8a:e6:3c vlan 1 interface ethernet 1/30"
    yak0_cx6="mac-address-table static 94:40:c9:8a:d6:fe vlan 1 interface ethernet 1/29"

    yak1_cx5="mac-address-table static EC:0D:9A:68:21:CC vlan 1 interface ethernet 1/30"
    yak0_cx5="mac-address-table static EC:0D:9A:68:21:D0 vlan 1 interface ethernet 1/29"
    yak2_cx5="mac-address-table static EC:0D:9A:68:21:BC vlan 1 interface ethernet 1/31"
    yeti5_cx5="mac-address-table static EC:0D:9A:68:21:A0 vlan 1 interface ethernet 1/3"
    if [ MAC_ADDRESS_CONFIG == "HYBRID_CX5_CX6" ]; then
        #CX5 CX6 hybrid setup
        echo "configuring mac address table for CX6's on yak0 and yak1"
        sshpass -p "$pwd" ssh sw100 cli -h "\"enable\" \"configure terminal\" \"$yak1_cx6\" \"$yak0_cx6\" \"$yak2_cx5\" \"$yeti5_cx5\""
    elif [ MAC_ADDRESS_CONFIG == "CX5" ]; then
        echo "configuring mac address table assuming we only have cx5s"
        sshpass -p "$pwd" ssh sw100 cli -h "\"enable\" \"configure terminal\" \"$yak1_cx5\" \"$yak0_cx5\" \"$yak2_cx5\" \"$yeti5_cx5\""
    else
        echo "Not configuring mac address table"
    fi

}

function set_ecn {
    off=0
    on=1
    state=$on

    dir="/sys/class/net/$iface/ecn/roce_np/enable"
    for i in $(seq 0 7); do
        echo "roce ecn write -> $dir/$i"
        echo "current state"
        cat $dir/$i
        echo $state > $dir/$i
    done

    dir="/sys/class/net/$iface/ecn/roce_rp/enable"
    for i in $(seq 0 7); do
        echo "roce ecn write -> $dir/$i"
        echo "current state"
        cat $dir/$i
        echo $state > $dir/$i
    done
}

#out of order 
function set_ooo {
    echo "Setting Out of Order"
    # Enable all mlx5 devices.
    export MLX5_RELAXED_PACKET_ORDERING_ON="all"
}

function disable_icrc {
    DEV=mlx5_0
    REGS="0x5361c.12:1 0x5363c.12:1 0x53614.29:1 0x53634.29:1"
    echo "WARNING: this script assumes you're using a ConnectX-5 NIC. If you're using something different, restore the register values and modify the script."

    echo ibv_devinfo -d $DEV
    ibv_devinfo -d $DEV

    echo Before:
    for i in $REGS
    do
        CMD="sudo mstmcra $DEV $i"
        printf "$CMD => "
        $CMD
    done    

    echo Modifying:
    for i in $REGS
    do
        CMD="sudo mstmcra $DEV $i 0x0"
        echo "$CMD"
        $CMD
    done    

    echo After:
    for i in $REGS
    do
        CMD="sudo mstmcra $DEV $i"
        printf "$CMD => "
        $CMD
    done  
}

function enable_icrc {
    #cx5
DEV=mlx5_0
REGS="0x5361c.12:1=1 0x5363c.12:1=1 0x53614.29:1=0 0x53634.29:1=1"
echo "WARNING: this script assumes you're using a ConnectX-5 NIC. If you're using something different, restore the register values and modify the script."

echo ibv_devinfo -d $DEV
ibv_devinfo -d $DEV

echo Before:
for i in $REGS
do
    CMD="sudo mstmcra $DEV ${i%=*}"
    printf "$CMD => "
    $CMD
done    

echo Modifying:
for i in $REGS
do
    CMD="sudo mstmcra $DEV ${i%=*} ${i#*=}"
    echo "$CMD"
    $CMD
done    

echo After:
for i in $REGS
do
    CMD="sudo mstmcra $DEV ${i%=*}"
    printf "$CMD => "
    $CMD
done 
}

#global commands
set_server_params

#commands for all but the control server
if [ $hname == "b09-27.sysnet.ucsd.edu" ]; then
    echo "setting up the switch"
    setup_switch
else
    setup_nic
    setup_hugepages
    disable_icrc
    #enable_icrc
    #set_ooo
    #set_ecn
fi

if [[ $hname == "yak-02.sysnet.ucsd.edu" ]]; then
    #turn off roce
    echo "turning off ROCEv2 on $hname"
    sudo su
    echo 0 >  /sys/bus/pci/devices/0000\:04\:00.0/roce_enable
fi

exit 0


