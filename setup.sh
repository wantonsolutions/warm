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
            iface="enp129s0"
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
            iface="enp59s0"
            ipaddr="192.168.1.17"
            numasocket="0"
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
    echo -e "The switch requires openflow rules in order to forward packets to the yak-02 middlebox
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

    #Port Eth1/29 is Yak00 (192.168.1.11)
    #Port Eth1/30 is Yak01 (192.168.1.12)
    #Port Eth1/31 is Yak02 

    #add redirection flows to table 0
    openflow add-flows 1 ,table=0,in_port=Eth1/30,nw_src=192.168.1.12/32,nw_dst=192.168.1.11/32,actions=output=Eth1/31
    openflow add-flows 2 ,table=0,in_port=Eth1/29,nw_src=192.168.1.11/32,nw_dst=192.168.1.12/32,actions=output=Eth1/31

    #remove redirection flows
    openflow del-flows 1
    openflow del-flows 2
    "
}

set_server_params
setup_nic
setup_hugepages

if [[ $hname == "yak-02.sysnet.ucsd.edu" ]]; then
    #turn off roce
    echo "turning off ROCEv2 on $hname"
    sudo su
    echo 0 >  /sys/bus/pci/devices/0000\:04\:00.0/roce_enable
    exit
fi
exit 0


