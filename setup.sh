#!/bin/bash

function setup_nic {
    hname=`hostname`
    echo "Configuring NIC on $hname"
    case $hname in
        "yak-00.sysnet.ucsd.edu")
            iface="enp4s0"
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
    sudo ifconfig $iface $ipaddr netmask 255.0.0.0 
    echo "Nic Configuration complete Configuration complete"
}

function setup_hugepages {
    echo "Setting up hugepages"
    sudo hugeadm --pool-pages-min 2MB:8192
    sudo mkdir -p /mnt/hugetlbfs ; mount -t hugetlbfs none /mnt/hugetlbfs
    sudo numactl --physcpubind=0 --localalloc LD_PRELOAD=libhugetlbfs.so HUGETLB_MORECORE=yes
    echo "HUGEPAGE INFO"
    cat /proc/meminfo | grep Huge
    echo "Hugepage setup complete"
}

setup_nic
setup_hugepages
exit 0
