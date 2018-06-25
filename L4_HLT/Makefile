# Common Makefile all OS

include ../../../../StRoot/RTS/src/rtsmakefile.def
include ../../../../StRoot/RTS/src/rtsplusplus.def

STD_DAQ_OBJS += tof_hlt_sector.o 
STD_DAQ_OBJS += mtd_hlt_reader.o
STD_DAQ_OBJS += l3_support.o
STD_DAQ_OBJS += MultiSender.o EthernetServer.o

DAQREADER_OBJS += daq_tof.o tof_reader.o
DAQREADER_OBJS += daq_mtd.o
DAQREADER_OBJS += daq_btow.o daq_emc.o emc_reader.o emc_single_reader.o
DAQREADER_OBJS += daq_sc.o sc_reader.o
DAQREADER_OBJS += daq_trg.o trg_reader.o
DAQREADER_OBJS += trgReader10.o trgReader12.o
DAQREADER_OBJS += trgReader20.o trgReader21.o trgReader22.o 
DAQREADER_OBJS += trgReader30.o trgReader32.o
#DAQREADER_OBJS += thread_dbg.o
DAQREADER_OBJS += ScatterGather.o
DAQREADER_OBJS += ThreadManager.o

MY_VPATH += ../HLT_TOF
MY_VPATH += ../HLT_MTD
MY_VPATH += ../L3_SUPPORT
MY_VPATH += ../L4_SUPPORT ../MISC_LIBS ../LIBJML
MY_VPATH += $(OFFSRC)/DAQ_TOF
MY_VPATH += $(OFFSRC)/DAQ_MTD
MY_VPATH += $(OFFSRC)/DAQ_BTOW:$(OFFSRC)/DAQ_EMC
MY_VPATH += $(OFFSRC)/DAQ_SC
MY_VPATH += $(OFFSRC)/DAQ_TRG

vpath %.C $(MY_VPATH)
vpath %.c $(MY_VPATH)
vpath %.cxx $(MY_VPATH)

# set up the install directory!
FOUND := $(findstring evb, $(HOSTNAME))

STROOT = ../../../../StRoot/
# CXXFLAGS += -fomit-frame-pointer
CXXFLAGS += -funroll-loops -DIPCQLIB 
#-DTRG_VERSION=0x41
CXXFLAGS += -fPIC
CXXFLAGS += -Dlinux
CXXFLAGS += -g
# CXXFLAGS += -pg
CXXFLAGS += -std=c++11

CXXFLAGS += -Wno-unused-but-set-variable
CXXFLAGS += -Wno-unused-variable

all: hltCaliServer libl4HLT.a  emulate

# This trashes old "default" DAQ libraries from /RTS
# as well as adds system libraries...
LDLIBS := -lpthread -lrt -ldl

CXXFLAGS += -I./Vc/build/include
CXXFLAGS += -I./TPCCATracker/code
CXXFLAGS += -I./TPCCATracker/code/CATracker
CXXFLAGS += -I./TPCCATracker/code/CATrackerPerformance
CXXFLAGS += -I./TPCCATracker/code/KFParticle
CXXFLAGS += -I./TPCCATracker/code/KFParticlePerformance
CXXFLAGS += -DNDEBUG
# CXXFLAGS += -inline-forceinline

CXXFLAGS += -Wno-write-strings
CXXFLAGS += -Wno-parentheses
#CXXFLAGS += -Wl,-whole-archive

# the following therr are used when build libTPCCATracker.a
CXXFLAGS += -DDO_TPCCATRACKER_EFF_PERFORMANCE -DSTAR_STANDALONE -DNVALGRIND 
CXXFLAGS += -DKFPARTICLE -DKFPWITHTRACKER -DHomogeneousField -DKFParticleStandalone
# CXXFLAGS += -DWITHSCIF -DUSE_TIMERS
# DEFINES += -DHLTCA_STANDALONE # if define, do not use ROOT
CXXFLAGS += -DENABLE_VECTORIZATION

# surpress the Vc releated warnings, to be removed
CXXFLAGS += -Wno-unused-function -Wno-deprecated-declarations -Wno-unused-local-typedefs

CXXFLAGS += -I../L4_SUPPORT

LDLIBS += -L./TPCCATracker/build -lCATracker -lKFParticle -lCATrackerPerf -lKFParticlePerf # -lscif
LDLIBS += -L./Vc/build/lib -lVc
# LDLIBS += /opt/intel_17/lib/intel64/libirc.a

ROOTCFLAGS = $(shell root-config --cflags)
ROOTLIBS   = $(shell root-config --evelibs)
ROOTLIBS  += -lRHTTP
CXXFLAGS += $(ROOTCFLAGS)
LDLIBS   += $(ROOTLIBS)

################################################################################

CAFLAGS += -I./TPCCATracker/code
CAFLAGS += -I./TPCCATracker/code/CATracker
CAFLAGS += -I./TPCCATracker/code/CATrackerPerformance
CAFLAGS += -I./TPCCATracker/code/KFParticle
CAFLAGS += -I./TPCCATracker/code/KFParticlePerformance
CAFLAGS += -DDO_TPCCATRACKER_EFF_PERFORMANCE -DUSE_TIMERS -DSTAR_STANDALONE -DNVALGRIND 
CAFLAGS += -DKFPARTICLE -DKFPWITHTRACKER -DHomogeneousField -DKFParticleStandalone
CAFLAGS += -DENABLE_VECTORIZATION

CAMICFLAGS =  -I/home/mzyzak/trunk_Vc/code
CAMICFLAGS += -I/home/mzyzak/trunk_Vc/code/CATracker
CAMICFLAGS += -I/home/mzyzak/trunk_Vc/code/CATrackerPerformance
CAMICFLAGS += -I/home/mzyzak/trunk_Vc/code/KFParticle
CAMICFLAGS += -I/home/mzyzak/trunk_Vc/code/KFParticlePerformance
CAMICFLAGS += -DDO_TPCCATRACKER_EFF_PERFORMANCE -DUSE_TIMERS -DSTAR_STANDALONE -DNVALGRIND 
CAMICFLAGS += -DKFPARTICLE -DKFPWITHTRACKER -DHomogeneousField -DKFParticleStandalone -DHLTCA_STANDALONE
CAMICFLAGS += -DENABLE_VECTORIZATION

CAMICLIBS = -L./TPCCATracker/build_mic -lKFParticle -lCATracker # -lscif

# VCFLAGS   = -I./Vc/build/include
# VCMICLIBS = -L./Vc/build/lib -lVc_MIC
VCFLAGS   = -I/home/mzyzak/Vc/Vc-master/build/include
VCMICLIBS = -L/home/mzyzak/Vc/Vc-master/build/lib -lVc_MIC
# VCFLAGS   = -I/home/kehw/hlt/vc.build/install/include
# VCMICLIBS = -L/home/kehw/hlt/vc.build/install/lib -lVc_MIC

USE_COI = 0
ifeq ($(USE_COI),1)
	CXXFLAGS += -DUSE_COI
	LDLIBS += -lcoi_host -Wl,--enable-new-dtags
endif

################################################################################
# Here are some debug options and offline option
# should never be added when run online
# Tonko's profiler
# CXXFLAGS += -DPROFILER_USE

# can be over write from command line
L4STANDALONE = 1
ifeq ($(L4STANDALONE),1)
	CXXFLAGS += -DL4STANDALONE
endif

#########################################################################

#########################################################################
# There are some options for year>=13 
CXXFLAGS += -DRUN13GMT
#########################################################################


#########################################################################
# There are some options for old HLT compatibility  
# CXXFLAGS += -DOLDCFG
#########################################################################

#########################################################################
# if turn on this option, use realtime drift velocity to correct
# z corrodinate of TPC hits
# CXXFLAGS += -DUSE_REALTIME_DRIFT_VELOCITY
#########################################################################

#########################################################################
# if turn on this option, use TPC sectors fired by MTD trigger unit
CXXFLAGS += -DUSE_TPC_SECTOR_MASK_MTD
#########################################################################

#############################################################
# if turn on this option, use TPC sectors fired by MTD trigge
# CXXFLAGS += -DCOLLECT_EVENT_INFO
#############################################################

RTS_ST_ROOT = ../../../../StRoot/RTS
VPATH += $(RTS_ST_ROOT)/src/DAQ_TOF $(RTS_ST_ROOT)/src/DAQ_MTD $(RTS_ST_ROOT)/src/DAQ_TPX \
	$(RTS_ST_ROOT)/src/DAQ_TRG $(RTS_ST_ROOT)/src/DAQ_BTOW $(RTS_ST_ROOT)/src/DAQ_EMC \
	$(RTS_ST_ROOT)/src/DAQ_SC $(RTS_ST_ROOT)/src/DAQ_READER

CXXFLAGS += -I$(RTS_ST_ROOT)/include

OBJS := FtfBaseHit.o				\
	FtfBaseTrack.o				\
	FtfDedx.o				\
	FtfFinder.o				\
	FtfHit.o				\
	FtfMcTrack.o				\
	FtfPara.o				\
	FtfSl3.o				\
	FtfTrack.o				\
	FtfUtilities.o				\
	gl3Bischel.o				\
	gl3EMC.o				\
	gl3Event.o				\
	gl3Histo.o				\
	gl3HistoManager.o			\
	gl3Hit.o				\
	gl3LMVertexFinder.o			\
	gl3Node.o				\
	gl3TOF.o				\
	gl3MTD.o				\
	gl3Track.o				\
	gl3TriggerDecider.o			\
	l3BankUtils.o				\
	l3CoordinateTransformer.o		\
	l3EmcCalibration.o			\
	l3Swap.o				\
	l3TrgReader.o				\
	l3TrgReaderV12.o			\
	l3TrgReaderV20.o			\
	l3TrgReaderV21.o			\
	online_TPC_CA_tracker.o			\
	online_tracking_collector.o		\
	online_tracking_TpcHitMap.o		\
	online_tracking_vertex.o		\
	EventInfo.o				\
	EventInfoDict.o				\
	L4HLTTask_cpu.o				\
	L4HLTTask_simu.o			\
	L4HLTTask_decider.o			\
	udp_client.o				\
	hltCaliClient.o

EventInfoDict.cxx: EventInfo.h EventInfoLinkDef.h
	rootcint -f $@ -c $^

L4_TRACKER_OBJS := $(OBJS)

#OBJS += l4TaskEmulate.o l4RandomEmulate.o l4TaskEth.o MultiServer.o HltChooser.o l4Emulator.o dataFileUtils.o
OBJS += l4Emulator.o dataFileUtils.o ScatterGather.o

OBJS += ${DAQREADER_OBJS}

#BINS := hltCaliServer 
LIBS := libl4HLT.a

libl4HLT.a: $(OBJS)
	$(RTS_AR)

libl4HLT.so: $(OBJS)
	ld -shared -o $@ $^

hltCaliServer: hltCaliServer.o L4_Server.o L4ServerTask.o gl3Histo.o ScatterGather.o $(STD_DAQ_OBJS)

# Jeff's Server targets
L4_SERVER_OBJS := l4TaskEth.o MultiServer.o			\
		  HltChooser.o l4Emulator.o dataFileUtils.o	\
		  l4MonClass.o l4EvpSender.o ThreadManager.o	\
                  l4TriggerTask.o l4EvbSenderTask.o

#l4Emulator: l4Emulator.o libl4HLT.a $(STD_DAQ_OBJS) $(DAQREADER_OBJS)
#emulate: emulate.o libl4HLT.a $(STD_DAQ_OBJS) $(DAQREADER_OBJS)
emulate: emulate.o libl4HLT.a $(STD_DAQ_OBJS) MultiSender.o
	$(CXX) -o $@ -Wl,--whole-archive $^ -Wl,--no-whole-archive $(LDFLAGS) $(LDLIBS)

libL4Server.a: $(STD_DAQ_OBJS) $(L4_SERVER_OBJS) $(L4_TRACKER_OBJS)
	$(RTS_AR)

l4Mother: l4Mother.o $(DAQREADER_OBJS) libL4Server.a

l4CalMother: l4CalMother.o $(DAQREADER_OBJS) libL4Server.a

l4Task: l4Task.o $(DAQREADER_OBJS) libL4Server.a -lcoi_host

servers: l4Mother l4Task l4CalMother hltCaliServer

installservers: servers
	$(INSTALL) l4Mother        $(PROJDIR)/bin/LINUX/x86_64
	$(INSTALL) l4Task          $(PROJDIR)/bin/LINUX/x86_64
	$(INSTALL) coi_simple_sink_mic   $(PROJDIR)/bin/LINUX/x86_64
	$(INSTALL) l4CalMother     $(PROJDIR)/bin/LINUX/x86_64
	$(INSTALL) hltCaliServer   $(PROJDIR)/bin/LINUX/x86_64

tests: test_server test_clint
test_server: test_server.o MultiServer.o $(STD_DAQ_OBJS)
test_clint: test_clint.o MultiSender.o $(STD_DAQ_OBJS)

scifTest: test_scif_KFP.o libl4HLT.a $(STD_DAQ_OBJS)
	$(CXX) -o $@ -Wl,--whole-archive $^ -Wl,--no-whole-archive $(LDFLAGS) $(LDLIBS)

KFPCOISender: KFPCOISender.cxx
	$(CXX) $(CXXFLAGS) -o $@ -Wl,--whole-archive $^ -Wl,--no-whole-archive $(LDFLAGS) $(LDLIBS) -lcoi_host -Wl,--enable-new-dtags

KFPSource = TPCCATracker/code/KFParticle/KFParticleBase.cxx			\
            TPCCATracker/code/KFParticle/KFParticleBaseSIMD.cxx			\
	    TPCCATracker/code/KFParticle/KFParticle.cxx				\
	    TPCCATracker/code/KFParticle/KFParticleDatabase.cxx			\
	    TPCCATracker/code/KFParticle/KFParticleFinder.cxx			\
	    TPCCATracker/code/KFParticle/KFParticlePVReconstructor.cxx		\
	    TPCCATracker/code/KFParticle/KFParticleSIMD.cxx			\
	    TPCCATracker/code/KFParticle/KFParticleTopoReconstructor.cxx	\
	    TPCCATracker/code/KFParticle/KFPEmcCluster.cxx			\
	    TPCCATracker/code/KFParticle/KFPTrack.cxx				\
	    TPCCATracker/code/KFParticle/KFPTrackVector.cxx			\
	    TPCCATracker/code/KFParticle/KFPVertex.cxx				\
	    TPCCATracker/code/KFParticle/KFVertex.cxx

KFPCOIServer: KFPCOIServer.cxx
	icpc -mmic -g -lcoi_device -O0 -std=c++11 -rdynamic -Wl,--enable-new-dtags $(VCFLAGS) $(CAMICFLAGS) $^ -o $@ $(VCMICLIBS) $(CAMICLIBS)

BINS := hltCaliServer emulate l4TriggerTask l4Mother l4Task l4CalMother l4EvbSenderTask hltCaliServer
# DO NOT DELETE
