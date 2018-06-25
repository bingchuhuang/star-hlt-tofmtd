#ifndef _online_TPC_CA_tracker_h_
#define _online_TPC_CA_tracker_h_

#include "TPCCATracker/code/CATracker/AliHLTTPCCAGBTracker.h"
#include "TPCCATracker/code/CATracker/AliHLTTPCCAGBTrack.h"
#include "TPCCATracker/code/CATracker/AliHLTTPCCAGBHit.h"
#include "TPCCATracker/code/CATracker/AliHLTTPCCATrackParam.h"
#include "TPCCATracker/code/KFParticle/KFParticleTopoReconstructor.h"

#include "online_tracking_TpcHitMap.h"
#include "l3CoordinateTransformer.h"
#include "FtfHit.h"
#include "FtfDedx.h"
#include "FtfTrack.h"
#include "FtfPara.h" 
#include "gl3Track.h"
#include "L4EventDataTypes.h"

#ifdef WITHSCIF
#include "scif_KFP_client.h"
#endif // WITHSCIF

#include <string>

class online_TPC_CA_tracker
{
public:
    online_TPC_CA_tracker(const double magField, const double dv,
                          const char* parDir, const unsigned int rn,
                          const int id);
    ~online_TPC_CA_tracker();

    void reset(void);
    void printInfo(void);
    void printInfo2(void);

    void setDriftVelocity(const double dv);
    void setMagField(const double magField) ;
    void setHLTParDir(const char* hltParDir);
    void setHLTParameters(void);
    void setHLTParameters(const char* hltParameters);
    void setTPCMap(void);
    void setTPCMap(const char* mapDir);
    void setHitsWeight();
    void setGainParamagers(void); // dEdx
    void setGainParamagers(const char* gainParameters); // dEdx
    void setGainParamagers(const double ig, const double og);
    void setBeamline(const double x, const double y);
    void setBeamLineFile(const char* beamline); // beam line self-calibration
    void setSpaceChargeFromScalerCounts(int* data);
#ifdef WITHSCIF
    void setKFPClient();
    void closeKFPConnection();
#endif
    
    void readHLTparameters(char* HLTparameters);
    int  readSectorFromESB(int sector, char *mem, int len);

    void makeCASettings(void);  // once per run?
    void makeCAHits(void); // read from daq file, apply tpc map, convert to CA hits
    // returns ntracks, output, CA GB tracks?

    int  process(HLTTPCData_t* out_buf,int &out_buf_used);
    int  process2(char* out_buf,int &out_buf_used);
    int  runCATracker(void);
    void makeDedx(void);
    void makePrimaryVertex(void);
    void makePrimaryTracks(void);
    int  fillTracks(HLTTPCData_t* out_buf,int &out_buf_used);
    int  fillTracks(char *out_buf,int &out_buf_used);
    int  fillTracks2(char *out_buf,int &out_buf_used);
    
    int getNHits() { return nHits; }
    int getNTracks() { return nCAGBTracks; }
    int getNPrimaryTracks() { return nPrimaryTracks; } // # of tracks send to gl3 code
    
private:
    void ReadHLTParameters();
    AliHLTTPCCAGBHit FtfHit2TPCCAGBHit(const FtfHit& fHit);
    void Convert(const KFParticle *KFTrack, gl3Track *trk, int id);
    void Convert(const AliHLTTPCCAGBTrack *CATrack, gl3Track *locTrack, int id);
    void Convert2(const AliHLTTPCCAGBTrack *CATrack, FtfTrack *locTrack, int id);
    int x2Row(double x);
    inline int id2index(int id) { return id; };
#ifdef WITHSCIF
    void sendDataToPhi();
#endif
    
    static const int    nTpcSectors = 24;
    static const int    maxHits     = 200005;

    short  debugLevel;
    double xyError;
    double zError;
    int    minTimeBin;
    int    maxTimeBin;
    int    minClusterCharge;
    int    maxClusterCharge;
    int    embedded;
    char*  outBuffer;
    int    outBufferSize;       // in byte
    
    double      bField;
    double      driftVelocity;
    std::string HLTParDir;      // directory which contents all paramaters and TPC maps
    std::string TPCMapDir;      // directory which contents all TPC maps
    std::string HLTParameters;
    std::string GainParameters;
    int         runnumber;
    int         myId;           // HLT instance id on a machine

    online_tracking_TpcHitMap* tpcHitMap[nTpcSectors];
    
    int nHits;
    FtfHit* fHits;
    std::vector<AliHLTTPCCAGBHit>* caHits;
    std::vector<AliHLTTPCCAParam> caParam;// settings for all sectors to give CATracker

    AliHLTTPCCAGBTracker* CAGBTracker;
    int nCAGBTracks;

    l3CoordinateTransformer* coordinateTransformer;
    FtfDedx*                 fDedx;
    FtfPara*                 para;

    KFParticleTopoReconstructor* topoReconstructor;
    float primaryVertex[3];

#ifdef WITHSCIF
    scif_KFP_client* kfpClient;
    bool isSCIFConnected;
#endif
    
    int nGlobalTracks;
    int nPrimaryTracks;
};

#endif  // _online_TPC_CA_tracker_h_
