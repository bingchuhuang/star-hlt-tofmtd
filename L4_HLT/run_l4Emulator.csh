#!/bin/tcsh -f

################################################################################
# default values
set DEBUG = 0
set CALLGRIND = 0
set CACHEGRIND = 0
set MASSIF = 0
set LOG_LEVEL = 'WARN'
set nEvent = 1000000
set id = 0 # set different id if you run multiple HLT instances at the same time

# set daqfile = ./daq/2016/038/17038046/st_physics_17038046_raw_1000015.daq
# set cfgfile = /RTS/conf/handler/archive/conf_17038046.xml

# set daqfile = ./daq/st_hlt_17046032_raw_1000069.daq
# set cfgfile = /RTS/conf/handler/archive/conf_17046032.xml

# set daqfile = ./daq/2014/146/st_physics_15146050_raw_2000020.daq
# set cfgfile = ./daq/2014/146/conf_15146050.xml

# set daqfile = ./daq/2016/050/17050041/st_physics_17050041_raw_1000015.daq
# set cfgfile = /RTS/conf/handler/archive/conf_17050041.xml

# set daqfile = daq/2016/127/17127030/st_physics_17127030_raw_1000028.daq
# set cfgfile = /RTS/conf/handler/archive/conf_17127030.xml

set daqfile = /star/u/kehw/home_kehw/hlt/hlt_dev/online/RTS/src/L4_HLT/daq/2017/039/18039026/st_cosmic_adc_18039026_raw_0000001.daq
set cfgfile = /RTS/conf/handler/archive/conf_18039026.xml

set paradir = /net/daqman.l4.bnl.local/RTS/home_kehw/hlt/HLTConfig

################################################################################
# parse arguments with dark magic
set OPTS=(`getopt -s tcsh -o cgmp -l nevents:,id:,log: -- $argv:q`)
if ($? != 0) then
    echo "Terminating..." >/dev/stderr
    exit 1
endif

eval set argv=\($OPTS:q\)

while (1)
    switch($1:q)
    case -c:
        set CACHEGRIND = 1; shift
        breaksw;
    case -g:
        set DEBUG = 1 ; shift
        breaksw;
    case -m:
        set MASSIF = 1; shift
        breaksw;
    case -p:
        set CALLGRIND = 1; shift
        breaksw;
    case --nevents:
        set nEvent = $2:q ; shift; shift # shift twoice if there is a (optional) argument
        breaksw;
    case --id:
        set id = $2:q ; shift; shift
        breaksw;
    case --log:
        set LOG_LEVEL = $2:q ; shift; shift
        breaksw;
    case --:
	shift
	break
    default:
	echo "Internal error!" ; 
        echo "Usage Example: ./run_l4Emulator.csh --nevent 10"
        exit 1
    endsw
end

################################################################################
set sfsfile = ./`basename ${daqfile} .daq`.sfs
echo "Output file: $sfsfile"
if ( -e $sfsfile ) then
    echo "remove existing ${sfsfile}"
    rm -f $sfsfile
endif

#valgrind --leak-check=full --show-reachable=yes        \
#    --suppressions=$ROOTSYS/etc/valgrind-root.supp     \

# source /opt/intel/inspector_xe/inspxe-vars.csh        \
# inspxe-cl -collect=mi1                                

# Log levels
# [DBG/NOTE/WARN/OPER/INFO/CRIT]
set CMD = "./emulate -D ${LOG_LEVEL} -file ${daqfile} -out ${sfsfile} -conf ${cfgfile} -para ${paradir} -max ${nEvent}"

if ( $DEBUG ) then
    set CMD = "gdb --args ${CMD}"
else if ( $CALLGRIND ) then
    set CMD = "valgrind --tool=callgrind ${CMD}"
else if ( $CACHEGRIND ) then
    set CMD = "valgrind --tool=cachegrind ${CMD}"
else if ( $MASSIF ) then
    set CMD = "valgrind --tool=memcheck ${CMD}"
endif

# run the command
set echo = 1
$CMD
