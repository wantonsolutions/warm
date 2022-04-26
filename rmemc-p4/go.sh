#!/bin/bash

#Global variables
program="bs"
program_src="${program}.p4"
RMEM_P4="/home/ssgrant/warm/rmemc-p4"
P4_SOURCE_DIR="$RMEM_P4/p4src"
build_tool="/usr/local/ssgrant/p4/tools/p4_build.sh"

BIN_DIR="$SDE/install/share/tofinopd/$program"
BIN_FILE="${BIN_DIR}/tofino.bin"

#parse command line arguments
if [[ $1 == "-b" ]]; then
    echo "-b set going to build"
    BUILD="TRUE"
fi

#build the program using a predefiend compiler
function build_p4 () {
    #actually build
    pushd $P4_SOURCE_DIR
    $build_tool $program_src --with-p4c bf-p4c
    #sanity check that the program was actually built
    if [ ! -f $BIN_FILE ]; then
        echo "$BIN_FILE not present"
        exit 1
    fi
}

#set up the virtual interfaces
function setup_veth () {
    VETH_SCRIPT="${SDE}/install/bin/veth_setup.sh"
    sudo $VETH_SCRIPT

}

#launch the tofinio model in an xterm
function run_tofino () {
    TOFINIO_SCRIPT="$SDE/run_tofino_model.sh"
    echo "launching new xterm window for the tofino model"
    xterm -e $TOFINIO_SCRIPT -p $program &
    sleep 1
    echo "launch complete"
}

function run_driver () {
    DRIVER_SCRIPT="$SDE/run_switchd.sh"
    echo "Running driver"
    xterm -e $DRIVER_SCRIPT -p $program &
}

function run_bfshell() {
    BF_SHELL_SCRIPT="$SDE/run_bfshell.sh"
    SHELL_SCRIPT="$RMEM_P4/bfshell/forward.cmd"
    xterm -e $BF_SHELL_SCRIPT -f $SHELL_SCRIPT &
}

function run_ptf() {

    cd $RMEM_P4
    sudo -E env "PATH=$PATH" "PYTHONPATH=$PYTHONPATH" /usr/local/ssgrant/p4/bf-sde-9.3.0/install/bin/ptf --test-dir ptf-tests --log-dir /usr/local/ssgrant/p4/ptf-logs
}

#clean up the xterminal and other variables
function clean_up () {
    echo "Cleaning up"
    echo "killing xterms (tofinio model, driver)"
    killall xterm
}

trap ctrl_c INT

function ctrl_c(){
    echo "Trapped ctrl-c"
    clean_up
    exit 1
}

if [ ! -z $BUILD ]; then
    build_p4
fi


setup_veth
run_tofino
run_driver
run_bfshell

sleep 20
run_ptf

#main body of execution
sleep 4000

#clean up the execution
clean_up







