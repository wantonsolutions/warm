#! /bin/bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright(c) 2010-2014 Intel Corporation


#
# Run with "source /path/to/dpdk-setup.sh"
#

#
# Change to DPDK directory ( <this-script's-dir>/.. ), and export it as RTE_SDK
#
#cd ./dpdk_deps/dpdk-19.11.3
#export RTE_SDK=/home/shw328/multi-tor-evalution/dpdk_deps/dpdk-19.11.3
if [[ -z "$RTE_SDK" ]]; then
    echo "RTE_SDK not set consut DPDK installation guide"
fi

cd $RTE_SDK
echo "------------------------------------------------------------------------------"
echo " RTE_SDK exported as $RTE_SDK"
echo "------------------------------------------------------------------------------"

HUGEPGSZ=`cat /proc/meminfo  | grep Hugepagesize | cut -d : -f 2 | tr -d ' '`

#
# Creates hugepage filesystem.
#
create_mnt_huge()
{
	echo "Creating /mnt/huge and mounting as hugetlbfs"
	sudo mkdir -p /mnt/huge

	grep -s '/mnt/huge' /proc/mounts > /dev/null
	if [ $? -ne 0 ] ; then
		sudo mount -t hugetlbfs nodev /mnt/huge
	fi
}

#
# Removes hugepage filesystem.
#
remove_mnt_huge()
{
	echo "Unmounting /mnt/huge and removing directory"
	grep -s '/mnt/huge' /proc/mounts > /dev/null
	if [ $? -eq 0 ] ; then
		sudo umount /mnt/huge
	fi

	if [ -d /mnt/huge ] ; then
		sudo rm -R /mnt/huge
	fi
}

#
# Removes all reserved hugepages.
#
clear_huge_pages()
{
	echo > .echo_tmp
	for d in /sys/devices/system/node/node? ; do
		echo "echo 0 > $d/hugepages/hugepages-${HUGEPGSZ}/nr_hugepages" >> .echo_tmp
	done
	echo "Removing currently reserved hugepages"
	sudo sh .echo_tmp
	rm -f .echo_tmp

	remove_mnt_huge
}

#
# Creates hugepages.
#
set_numa_pages()
{
	clear_huge_pages

	for d in /sys/devices/system/node/node? ; do
		node=$(basename $d)
		echo "echo 1024 > $d/hugepages/hugepages-${HUGEPGSZ}/nr_hugepages" >> .echo_tmp
	done
	echo "Reserving hugepages"
	sudo sh .echo_tmp
	rm -f .echo_tmp

	create_mnt_huge
}

#
# Sets RTE_TARGET and does a "make install".
#
setup_target()
{

	export RTE_TARGET=x86_64-native-linuxapp-gcc
	#export RTE_TARGET=x86_64-native-linux-gcc
	#CONFIG_RTE_LIBRTE_MLX5_PMD=y
	#CONFIG_RTE_LIBRTE_MLX5_DEBUG=n
	echo "CONFIG_RTE_LIBRTE_MLX5_PMD=y" | tee -a $RTE_SDK/config/defconfig_$RTE_TARGET
	echo "CONFIG_RTE_LIBRTE_MLX5_DEBUG=y" | tee -a $RTE_SDK/config/defconfig_$RTE_TARGET	
	echo "CONFIG_RTE_LIBRTE_ETHDEV_DEBUG=y" | tee -a $RTE_SDK/config/defconfig_$RTE_TARGET

    make config install -j8 T=${RTE_TARGET} DESTDIR=$RTE_SDK MAKE_PAUSE=n

	echo "------------------------------------------------------------------------------"
	echo " RTE_TARGET exported as $RTE_TARGET"
	echo "------------------------------------------------------------------------------"
}

setup_target
set_numa_pages
# There's no need to load kernel modules
# There's no need to use the dpdk-devbind.py script to bind dpdk to mellanox device
