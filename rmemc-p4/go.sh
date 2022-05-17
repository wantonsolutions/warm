#!/bin/bash


#Per host arguments
HOST=`hostname`
YAK2="yak-02.sysnet.ucsd.edu"
PSWITCH="localhost"


#Global variables
program="bs"
program_src="${program}.p4"

if [ $HOST == $YAK2 ]; then
    build_tool="/usr/local/ssgrant/p4/tools/p4_build.sh"
    RMEM_P4="/home/ssgrant/warm/rmemc-p4"
elif [ $HOST == $SWITCH ]; then
    build_tool="/root/tools/p4_build.sh"
    RMEM_P4="/root/src/warm/rmemc-p4"
else
    echo "HOST=$HOST we don't have this set up, exiting"
    exit 1
fi

P4_SOURCE_DIR="$RMEM_P4/p4src"


#build_dir="${SDE}/build/build/${program}/tofino"
INSTALL_DIR="$SDE/install/share/tofinopd/$program"
BIN_DIR="$INSTALL_DIR/pipe"
CONTEXT_DIR="$INSTALL_DIR/pipe"
BIN_FILE="${BIN_DIR}/tofino.bin"
CONTEXT_FILE="${BIN_DIR}/context.json"

XTERM_1_GEOM="500x50+3180+500"
XTERM_2_GEOM="100x50+3780+500"
XTERM_3_GEOM="100x50+4380+500"

#parse command line arguments
if [[ $1 == "-b" ]]; then
    echo "-b set going to build"
    BUILD="TRUE"
fi

#build the program using a predefiend compiler
function build_p4 () {
    pushd $P4_SOURCE_DIR
    $build_tool $program_src --with-thrift --with-p4c bf-p4c --p4v 16 
    if [ ! -f $BIN_FILE ]; then
        echo "$BIN_FILE not present"
        exit 1
    fi
    mv $BIN_FILE $INSTALL_DIR

    if [ ! -f $CONTEXT_FILE ]; then
        echo "$CONTEXT_FILE not present"
        exit 1
    fi
    mv $CONTEXT_FILE $INSTALL_DIR

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
    xterm -geometry $XTERM_1_GEOM -e $TOFINIO_SCRIPT -p $program  &
    sleep 1
    echo "launch complete"
}

function run_driver () {
    DRIVER_SCRIPT="$SDE/run_switchd.sh"
    echo "Running driver"
    #$DRIVER_SCRIPT -p $program
    xterm -geometry $XTERM_2_GEOM -e $DRIVER_SCRIPT -p $program &
}

function run_bfshell() {
    BF_SHELL_SCRIPT="$SDE/run_bfshell.sh"
    SHELL_SCRIPT="$RMEM_P4/bfshell/forward.cmd"
    xterm -geometry $XTERM_3_GEOM -e $BF_SHELL_SCRIPT -f $SHELL_SCRIPT &
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
    exit 1
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







