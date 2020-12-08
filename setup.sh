#!/bin/bash

function setup_nic {
    hname=`hostname`
    echo "Configuring NIC on $hname"
    case $hname in
        "yak-00.sysnet.ucsd.edu")
            iface="enp4s0" #100gbps NICS Mellanox
            ipaddr="192.168.1.12"
        ;;
        "yak-01.sysnet.ucsd.edu")
            iface="enp129s0"
            ipaddr="192.168.1.13"
        ;;
        "yak-02.sysnet.ucsd.edu")
            iface="enp4s0"
            ipaddr="192.168.1.14"
        ;;

        *)
        echo "$machine does not have a configuration set in this script. Exiting and doing nothing"
        exit 1
        ;;
    esac

    #set the link up
    sudo ip link set $iface up
    #turn the interface on and configure ip
    sudo ifconfig $iface $ipaddr netmask 255.255.0.0 
    echo "Nic Configuration complete Configuration complete"
}

function setup_hugepages {
    echo "Setting up hugepages"
    sudo hugeadm --pool-pages-min 2MB:8192
    sudo mkdir -p /mnt/hugetlbfs ; mount -t hugetlbfs none /mnt/hugetlbfs

    #TODO set this up so that it maps correctly on yak0, 1 and 2
    sudo numactl --physcpubind=0 --localalloc LD_PRELOAD=libhugetlbfs.so HUGETLB_MORECORE=yes
    echo "HUGEPAGE INFO"
    cat /proc/meminfo | grep Huge
    echo "Hugepage setup complete"
}

function setup_switch {
    echo -e "The switch requires openflow rules in order to forward packets to the yak-02 middlebox
    the commands to run are as follows
    
    ssh sw100
    [pw KaicyeVapUv9]

    enable
    configure terminal

    #Port 30 is Yak01
    #Port 29 is Yak00

    openflow add-flows 1 ,table=0,in_port=Eth1/30,nw_src=192.168.1.12/32,nw_dst=192.168.1.11/32,actions=output=Eth1/31
    openflow add-flows 2 ,table=0,in_port=Eth1/29,nw_src=192.168.1.11/32,nw_dst=192.168.1.12/32,actions=output=Eth1/31

    openflow del-flows 1
    openflow del-flows 2
    "
}

setup_nic
setup_hugepages
exit 0
