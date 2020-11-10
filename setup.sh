#!/bin/bash

hname=`hostname`
case $hname in
    "yak-00.sysnet.ucsd.edu")
        iface="enp4s0"
        ipaddr="192.168.1.12"
    ;;
    "yak-01.sysnet.ucsd.edu")
        iface="enp129s0"
        ipaddr="192.168.1.13"
    ;;

    *)
    echo "$machine does not have a configuration set in this script. Exiting and doing nothing"
    exit 1
    ;;
esac

echo "Configuring Dumb NIC on $hname"
#set the link up
sudo ip link set $iface up
#turn the interface on and configure ip
sudo ifconfig enp4s0 $ipaddr netmask 255.0.0.0 
echo "Configuration complete"
exit 0
