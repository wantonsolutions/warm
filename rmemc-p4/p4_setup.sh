#!/bin/bash

#check that the important enviornment variables are set
function check_env () {
    echo "Checking P4 enviornment"
    if [ -z $SDE ]; then
        echo '$SDE not defined'
        exit 1
    fi
    echo "SDE=$SDE"

    if [ -z $SDE_INSTALL ]; then
        echo '$SDE_INSTALL not defined'
        exit 1
    fi
    echo "SDE_INSTALL=$SDE_INSTALL"
}

function check_dir() {
    reason=$1
    dir_name=$2
    if [ ! -d $dir_name ]; then
        echo -e "$reason directory $dir_name not created run \n mkdir $dir_name"
        exit 1
    fi
}

function check_directories () {

    echo -e "\nChecking Directory Structure"
    src="p4src"
    shell="bfshell"
    rpc="run_pd_rpc"
    pkt="pkt"
    tests="ptf-tests"

    check_dir "p4 source code" $src
    check_dir "barefoot shell" $shell
    check_dir "python scripts for running rpc server" $rpc
    check_dir "packet generation" $pkt
    check_dir "p4 tests" $tests
    echo "Directory Structure... Valid"
}

check_env
check_directories