#!/bin/bash


#Per host arguments
HOST=`hostname`
YAK2="yak-02.sysnet.ucsd.edu"
PSWITCH="localhost"


#Global variables
program="bs"
program_src="${program}.p4"

if [ ${HOST} == ${YAK2} ]; then
    build_tool="/usr/local/ssgrant/p4/tools/p4_build.sh"
    RMEM_P4="/home/ssgrant/warm/rmemc-p4"
elif [ ${HOST} == ${PSWITCH} ]; then
    build_tool="/root/src/warm/rmemc-p4/p4_build.sh"
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

function clean_p4() {
    rm -r $SDE/build/p4-build/tofino/$program
    rm -r $SDE/install/share/tofinopd/$program
    rm -r $SDE/install/lib/python3.5/site-packages/tofinopd/$program
    rm -r $SDE/install/lib/tofinopd/$program
    rm -r $SDE/install/share/p4/targets/tofino/$program.conf
    rm -r $SDE/logs/p4-build/tofino/$program


}

#build the program using a predefiend compiler
function build_p4 () {
    pushd $P4_SOURCE_DIR

    if [ ${HOST} == ${YAK2} ]; then
        $build_tool $program_src --with-thrift --with-p4c bf-p4c --p4v 16
    elif [ ${HOST} == ${PSWITCH} ]; then
        $build_tool $program_src P4_ARCHITECTURE=tna P4_VERSION=p4-16 ##--with-thrift
    else
        echo "HOST=$HOST we don't have a correct build set up"
        exit 1
    fi

    if [ ! -f $BIN_FILE ]; then
        echo "$BIN_FILE not present"
        exit 1
    fi


    if [ ! -f $CONTEXT_FILE ]; then
        echo "$CONTEXT_FILE not present"
        exit 1
    fi

    #I've set up yak2 differently so I need to move the files to a different location
    if [ ${HOST} == ${YAK2} ]; then
        mv $BIN_FILE $INSTALL_DIR
        mv $CONTEXT_FILE $INSTALL_DIR
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
    xterm -geometry $XTERM_1_GEOM -e $TOFINIO_SCRIPT -p $program  &
    sleep 1
    echo "launch complete"
}

#launch the tofinio model in an xterm
function run_tofino_direct () {
    TOFINIO_SCRIPT="$SDE/run_tofino_model.sh"
    echo "launching new xterm window for the tofino model"
    $TOFINIO_SCRIPT -p $program  &
    sleep 1
    echo "launch complete"
}

function run_driver () {
    DRIVER_SCRIPT="$SDE/run_switchd.sh"
    echo "Running driver"
    #$DRIVER_SCRIPT -p $program
    xterm -geometry $XTERM_2_GEOM -e $DRIVER_SCRIPT -p $program &
}

function run_driver_direct () {
    DRIVER_SCRIPT="$SDE/run_switchd.sh"
    $DRIVER_SCRIPT -p $program
}

function run_bfshell() {
    BF_SHELL_SCRIPT="$SDE/run_bfshell.sh"
    SHELL_SCRIPT="$RMEM_P4/bfshell/forward.cmd"
    xterm -geometry $XTERM_3_GEOM -e $BF_SHELL_SCRIPT -f $SHELL_SCRIPT &
}

function run_bfshell() {
    BF_SHELL_SCRIPT="$SDE/run_bfshell.sh"
    SHELL_SCRIPT="$RMEM_P4/bfshell/forward.cmd"
    $BF_SHELL_SCRIPT -f $SHELL_SCRIPT &
}

function run_ptf() {
    cd $RMEM_P4
    sudo -E env "PATH=$PATH" "PYTHONPATH=$PYTHONPATH" $SDE/install/bin/ptf --test-dir ptf-tests --log-dir /usr/local/ssgrant/p4/ptf-logs
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
    clean_p4
    build_p4
    exit 1
fi


if [ ${HOST} == ${YAK2} ]; then
    setup_veth
    run_tofino
    run_driver
    run_bfshell
    sleep 20
    run_ptf
elif [ ${HOST} == ${PSWITCH} ]; then
    run_driver_direct


    #run_bfshell
else
    echo "$HOST not set up exiting .." 
fi


#main body of execution
sleep 4000

#clean up the execution
clean_up







