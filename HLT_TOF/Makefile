RTS_ST_ROOT = ../../../../StRoot/RTS

include $(RTS_ST_ROOT)/src/rtsmakefile.def
include $(RTS_ST_ROOT)/src/rtsplus.def


INCLUDES = -I.. -I$(RTS_ST_ROOT)/include -I$(RTS_ST_ROOT)/src -I$(RTS_ST_ROOT)/trg/include


# disable the loading of obsolete daqlibs from /RTS
LDLIBS := -lpthread -lrt

DEFINES += -DRTS_LOG_COLORED

#VPATH = $(RTS_ST_ROOT)/src/LOG $(RTS_ST_ROOT)/src/SFS ../SUNRT ../RTSCFG
#VOBJS = librtslog.a libsfs.a libsunRT.a librtsCfgLib.a

#VPATH += $(RTS_ST_ROOT)/src/DAQ_TOF $(RTS_ST_ROOT)/src/DAQ_READER
	
# ...with their libraries
#VOBJS += libtof.a libdaqReader.a
# make sure we have the right target in place


OBJS = tof_hlt_sector.o

LIBS = libhlt_tof.a

BINS = 

all: $(LIBS) $(BINS)

libs: $(LIBS)

libhlt_tof.a: tof_hlt_sector.o
	$(RTS_AR)

daq_hlt_tof: daq_hlt_tof.o $(OBJS) $(VOBJS) timestamp.o
	$(LINK.o) -o $@ $(LDLIBS) -Wl,--whole-archive $^ -Wl,--no-whole-archive

timestamp.o : daq_hlt_tof.o $(OBJS) $(VOBJS)
	touch timestamp.C
	$(CXX) -c timestamp.C

CONF_FILES = pvpdCali_4DB.dat totCali_4DB.dat t0_4DB.dat fiberpams.dat tofINLCorr.bin tofTDIG.dat

save:
	@/RTS/bin/install.sh daq_hlt_tof /RTS/bin/TPX

#	@/RTS/bin/install.sh $(CONF_FILES) /RTS/conf/tpx

# DO NOT DELETE

daq_hlt_tof.o: ../../../../StRoot/RTS/include/rtsLog.h
daq_hlt_tof.o: ../../../../StRoot/RTS/include/iccp.h
daq_hlt_tof.o: ../../../../StRoot/RTS/include/iccp2k.h
daq_hlt_tof.o: ../../../../StRoot/RTS/include/daqFormats.h
daq_hlt_tof.o: ../../../../StRoot/RTS/include/SECTOR/sector.h
daq_hlt_tof.o: ../../../../StRoot/RTS/trg/include/trgDataDefs.h
daq_hlt_tof.o: ../../../../StRoot/RTS/include/L3/L3Formats.h
daq_hlt_tof.o: ../../../../StRoot/RTS/include/cmds.h
daq_hlt_tof.o: ../../../../StRoot/RTS/include/rtsCmds.h
daq_hlt_tof.o: ../../../../StRoot/RTS/include/tasks.h
daq_hlt_tof.o: ../../../../StRoot/RTS/include/status.h
daq_hlt_tof.o: ../../../../StRoot/RTS/include/iccpHeader.h
daq_hlt_tof.o: ../../../../StRoot/RTS/include/rtsSystems.h
daq_hlt_tof.o: ../../../../StRoot/RTS/include/SUNRT/ipcQLib.hh
daq_hlt_tof.o: ../../../../StRoot/RTS/include/SUNRT/clock.h
daq_hlt_tof.o: ../../../../StRoot/RTS/include/SUNRT/shmLib.h
daq_hlt_tof.o: ../../../../StRoot/RTS/src/SFS/sfs_index.h
daq_hlt_tof.o: ../../../../StRoot/RTS/src/SFS/fs_index.h
daq_hlt_tof.o: ../../../../StRoot/RTS/src/SFS/sfs_base.h
daq_hlt_tof.o: ../../../../StRoot/RTS/src/DAQ_READER/daqReader.h
daq_hlt_tof.o: ../../../../StRoot/RTS/src/DAQ_READER/daqConfig.h
daq_hlt_tof.o: ../../../../StRoot/RTS/src/DAQ_READER/daq_dta.h
daq_hlt_tof.o: ../../../../StRoot/RTS/include/rts.h
daq_hlt_tof.o: ../../../../StRoot/RTS/src/DAQ_READER/daq_dta_structs.h
daq_hlt_tof.o: ../../../../StRoot/RTS/src/DAQ_TOF/daq_tof.h
daq_hlt_tof.o: ../../../../StRoot/RTS/src/DAQ_READER/daq_det.h
daq_hlt_tof.o: tof_hlt_sector.h
tof_hlt_sector.o: ../../../../StRoot/RTS/include/rtsLog.h
tof_hlt_sector.o: tof_hlt_sector.h
hlt_tof.o: ../GL3/hlt_entity.h
hlt_tof.o: ../../../../StRoot/RTS/include/rtsSystems.h
hlt_tof.o: ../L3_SUPPORT/l3_support.h
hlt_tof.o: ../../../../StRoot/RTS/src/SFS/sfs_index.h
hlt_tof.o: ../../../../StRoot/RTS/src/SFS/fs_index.h
hlt_tof.o: ../../../../StRoot/RTS/src/SFS/sfs_base.h
hlt_tof.o: ../../../../StRoot/RTS/include/RC_Config.h
hlt_tof.o: ../../../../StRoot/RTS/include/rts.h
hlt_tof.o: ../L3_SUPPORT/gl3_chooser.h
hlt_tof.o: ../GL3/profiler.hh
hlt_tof.o: ../../../../StRoot/RTS/include/rtsLog.h
hlt_tof.o: ../../../../StRoot/RTS/include/I386/i386Lib.h
hlt_tof.o: tof_hlt_sector.h
