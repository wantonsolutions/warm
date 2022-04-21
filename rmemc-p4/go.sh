#compile the program
program="bs"
program_src="${program}.p4"
RMEM_P4="/home/ssgrant/warm/rmemc-p4"
P4_SOURCE_DIR="$RMEM_P4/p4src"
build_tool="/usr/local/ssgrant/p4/tools/p4_build.sh"

BIN_DIR="$SDE/install/shyare/tofinopd/$program"


pushd $P4_SOURCE_DIR
$build_tool $program_src --with-p4c bf-p4c





