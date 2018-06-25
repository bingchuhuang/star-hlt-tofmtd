//:>------------------------------------------------------------------
//: FILE:       online_tracking_collector.h
//: HISTORY:
//:              3dec1999 version 1.00
//:              3feb2000 add sector to addTracks of type 3
//:              6jul2000 add l3CoordinateTransformer
//:             17jul2000 move glR3HistoContainer to gl3Conductor
//:             10aug2000 remove bField, to be kept in one place only (in FtfPara para)
//:             13aug2000 replace trackMerging with maxSectorNForTrackMerging
//:<------------------------------------------------------------------

#ifndef online_tracking_collector_H
#define online_tracking_collector_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <string>

#include "l3Coordinates.h"
#include "l3CoordinateTransformer.h"
#include "sizes.h"
#include "daqFormats.h"
//#include "trgStructures.h"
#include "l3TrgReader.h"
//#include "l3GeneralHeaders.h"
#include "FtfGeneral.h"
#include "gl3Track.h"
#include "gl3Hit.h"
#include "gl3Sector.h"
#include "FtfPara.h"
#include "online_tracking_vertex.h"
#include "gl3Histo.h"
#include "gl3EMC.h"
#include "gl3TOF.h"
#include "gl3MTD.h"
#include "gl3TriggerDecider.h"
#include "hltNetworkDataStruct.h"
#include "udp_client.h"
#include "L4EventDataTypes.h"

#ifdef OLD_DAQ_READER
#include <evpReader.hh>

#else /* OLD_DAQ_READER */
#include <DAQ_READER/daqReader.h>

#endif /* OLD_DAQ_READER */
#include <rtsSystems.h>
#include <daqFormats.h>

#ifndef OLD_DAQ_READER
#include <DAQ_TPC/daq_tpc.h>

#endif /* OLD_DAQ_READER */
#include "sizes.h"
#define GL3_READ_TPC_TRACKS   0x01
#define GL3_READ_TPC_CLUSTERS 0x02
#define GL3_READ_ALL          0xffff

//#define TIMING_GL3
#ifdef TIMING_GL3
#include <time.h>
#endif

// #define DEBUG_PV_FINDER

class gl3Node;
class gl3Bischel;
class HLTTPCOutputBlob;

class online_tracking_collector 

{

    public:

#ifdef TIMING_GL3
  double timeNow, timeOld;
  double timeMakeNodes, timeMatchEMC, timeMatchTOFg, timeMakeVertex, timeMakePrimaries, timeMatchTOFp, timeMeanDedx;
#endif

  	
    bool calibDataSentFlag; 
    tpc_t *sx_pTPC;
    int btow_reader(daqReader *rdr, char  *do_print) ;
    int bsmd_reader(daqReader *rdr, char  *do_print) ;
    int tof_reader(daqReader *rdr, char  *do_print) ;
    online_tracking_collector(float bField, int mxHits=szGL3_default_mHits, int mxTracks=szGL3_default_mTracks, char* parametersDirectory = "./", char* beamlineFile = "beamline");
    ~online_tracking_collector( );
    int  setup ( int mHits=szGL3_default_mHits, int mTracks = szGL3_default_mTracks);
    void readHLTparameters(char* HLTparameters = "HLTparameters");
    void setBeamline(const double bx, const double by);
    void readBeamline(char* beamlineFile = "beamline");
    void writeBeamline(char* beamlineFile = "beamline", int run_number = 0);
    int readScalers(char* scalerFile = "sc.asc");
    int  resetEvent ( );
    // bField is taken from the datafile
    // if the datafile sc bank is invalid,
    // defaultbField is used.
    //
    // if bField is set != 1000
    // then it overrides the value in the datafile
    //
    
#ifdef OLD_DAQ_READER
    int readFromEvpReader(evpReader *evp, 
			  char *mem, 
			  float defaultbField=.5,
			  float bField=1000,
			  int what=GL3_READ_ALL);
			  
#else /* OLD_DAQ_READER */
    int readFromEvpReader(daqReader *rdr, 
			  char *mem, 
			  float defaultbField=.5,
			  float bField=1000,
			  int what=GL3_READ_ALL);
#endif /* OLD_DAQ_READER */
    void readClustersFromEvpReader(int sector);
    short getBusy() { return busy; }

    gl3Track*   getGlobalTrack  ( int n );
    gl3Track*   getPrimaryTrack  ( int n );
    gl3Node*    getNode  ( int n );
    gl3Hit*     getHit    ( int n );
    gl3Sector*  getSector ( int n );
    int getNGlobalTracks() { return nGlobalTracks; }
    int getNPrimaryTracks() { return nPrimaryTracks; }
    int getNMergedTracks() { return nMergedTracks; }
    int getNBadGlobTracks() { return nBadGlobTracks; }
    int getNBadPrimTracks() { return nBadPrimTracks; }
    int getNBadNodes() { return nBadNodes; }
    int getNHits() { return nHits; }
    l3TrgReader* getTrgData() { return &trgData; }
    l3xyzCoordinate getVertex() { return vertex; }
    l3xyzCoordinate getLmVertex() { return lmVertex; }
    l3CoordinateTransformer* getCoordinateTransformer() { return coordinateTransformer; }
    int getTrgCmd();
    int getTrgWord();
    int getCTB(int n);
    int getZDC(int n);
    double getZDCVertex();
    int getToken() { return trgData.token; }
    void setToken(int tk) { trgData.token = tk; }
    unsigned int getBXingLo();
    unsigned int getBXingHi();
    unsigned long long getBXing();
    void setHitProcessing(int hitPro) { hitProcessing = hitPro; }
    void setMaxSectorNForTrackMerging(int _in) { maxSectorNForTrackMerging = _in; }
    void setBField(float _bField) {
        para.bField = fabs(_bField);
        para.bFieldPolarity = int(_bField / fabs(_bField));
    }
    void setCoordinateTransformer(l3CoordinateTransformer* in) { coordinateTransformer = in; }
    // int  readEventDescriptor ( EventDescriptor *descr);
    int  readL3Data  (L3_P* buffer);
    /* #if ( FORMAT_VERSION == 0x12 ) */
    /*     int  readEventDescriptor ( EventDescriptor *descr); */
    /*     int  readTrgData (TrgSumData* trgSum, RawTrgDet* rawTrg); */
    /* #elif ( FORMAT_VERSION == 0x20 ) */
    /*     int  readEventDescriptor ( EvtDescData *descr); */
    /*     int  readTrgData (TrgSumData* trgSum, RawTrgDet* rawTrg); */
    /* #endif */
    int  finalizeReconstruction();
    int  finalizeReconstruction2();
    int  readSectorHits   ( char* buffer, int nSectorTracks );
    int  Tracking_readSectorTracks ( char* buffer );
    int  readTPCCA(const char* buffer);
    int  readTPCCA(HLTTPCData_t* tpc_blob);
    void addTracks ( short sector, int nTracks, local_track* track );
    int  makeVertex ();
    int  makeLmVertex();
    void printInfo();

    int FillGlobTracks ( int maxBytes, char* buffer);
    int FillPrimTracks ( int maxBytes, char* buffer);
    int FillEvent ( int maxBytes, char* buffer, unsigned int decision = 0);
    int FillEmc ( int maxBytes, char* buffer);
    int FillTofHits ( int maxBytes, char* buffer);
    int FillMtdHits ( int maxBytes, char* buffer);
    int FillPvpdHits ( int maxBytes, char* buffer);
    int FillNodes ( int maxBytes, char* buffer);

    void setPrimaryVertex(float pVtx[3])   { vertex.Setxyz(pVtx[0], pVtx[1], pVtx[2]); }
    void setPrimaryVertexX(float x)        { vertex.Setx(x); }
    void setPrimaryVertexY(float y)        { vertex.Sety(y); }
    void setPrimaryVertexZ(float z)        { vertex.Setz(z); }
    void setLmPrimaryVertex(float pVtx[3]) { lmVertex.Setxyz(pVtx[0], pVtx[1], pVtx[2]); }
    void setLmPrimaryVertexX(float x)      { lmVertex.Setx(x); }
    void setLmPrimaryVertexY(float y)      { lmVertex.Sety(y); }
    void setLmPrimaryVertexZ(float z)      { lmVertex.Setz(z); }
    
    int makeNodes();
    int makeNodes2();
    int makePrimaries();
    void moveGTrackToBeamline();
    void moveGTrackToPrimaryVertex();
    int matchEMC();
    int matchTOF(int usePrimary = 0);
    int matchMTD();
    int meandEdx();
    int CalidEdx(char* GainParameters = "GainParameters", int run_number = 0);
    int readGainPara(char* GainParameters = "GainParameters");
    void setGainPara(const double ig, const double og);
    int  nMergableTracks;
    
    unsigned long long int trigger; // 64-bit integer
    int eventNumber;
    int listenPort;
    gl3EMC* emc;
    gl3TOF* tof;
    gl3MTD* mtd;
    gl3Bischel* bischel;
    gl3TriggerDecider* triggerDecider;
    
    int useBeamlineToMakePrimarys;
    int useVpdtoMakeStartTime;
    double beamlineX, beamlineY;
    double sigmaDedx1;
    int nTracksUsed; // # of tracks used to reconstruct vertex

    size_t makeNetworkData(int runnumber, hltCaliData& caliData);
    // int  sendNetworkData();
    // int  createNetworkThread();
    // int  killNetworkThread();

    std::string trgsetupname;
    
 private:
    // ############################################################
    // # Control parameters
    // ############################################################
    int   hitProcessing;  
    // 0=does read hits
    // 1=reassigns trackId in hits to
    //   pass that info downstream
    // 2=full hit unpacking for module use
    int minNoOfHitsOnTrackUsedForVertexCalc;
    double minMomUsedForVertexCalc;
    // ############################################################
    // # Data structures
    // ############################################################
    gl3Hit*    hit;
    gl3Track*  globalTrack;
    gl3Track*  primaryTrack;
    gl3Node*   nodes;
    int*       primaryTrackNodeIndex;
    // to keep according node index of primary tracks
    int*       globalTrackIndex;     
    // to keep track of relationship between
    // orig. tracks and final tracks
    int        busy;
    int        maxSectorNForTrackMerging;
    int        maxTracks;
    int        maxTracksSector;
    int        nGlobalTracks;
    int        nPrimaryTracks;
    int        maxHits;
    int        nHits;
    int        nMergedTracks;
    int        nBadGlobTracks;
    int        nBadPrimTracks;
    int        nBadNodes;
    // trigger
    /*     struct { */
    /* 	int            token; */
    /* 	unsigned int   trgCmd; */
    /* 	unsigned int   trgWord; */
    /* 	unsigned int   ZDC[16];  */
    /* 	unsigned int   CTB[256]; */
    /* 	unsigned int   bx_hi, bx_lo; */
    /*     } trgData; */
    l3TrgReader trgData;
    // vertex calc results
    l3xyzCoordinate vertex, lmVertex;
    int matchAllEMC, matchAllTOF, matchAllMTD;
    // ############################################################
    // # Other stuff
    // ############################################################
    double innerSectorGain, outerSectorGain;
    double normInnerGain ;
    double normOuterGain ;

    double nTracksCutUpdateBeamline;
    double nTracksCutUpdatedEdxGainPara;

    FtfPara          para;
    FtfContainer*    trackContainer;
    gl3Sector        sectorInfo[24];
    // histos used for the vertex determination
    gl3Histo* hVz ;
    gl3Histo* hVx ;
    gl3Histo* hVy ;

    // histo used for dEdx mean vealue determination & tune innerGain outerGain parameter
    gl3Histo* dEdx ;

    // Helper objects
    l3CoordinateTransformer* coordinateTransformer;
    static const int nSectors = 24;
			  
    // hltCaliData caliData;
    pthread_t networkThread;

#ifdef DEBUG_PV_FINDER
    FILE* pvFile;
#endif // DEBUG_PV_FINDER
};

#endif

