//:>----------------------------------------------------------------------
//: FILE:      FtfDedx.h
//: HISTORY:
//:>----------------------------------------------------------------------
#ifndef FTFDEDX
#define FTFDEDX
#include "FtfGeneral.h"
#include "FtfTrack.h"
#include "l3CoordinateTransformer.h"

#include "TPCCATracker/code/CATracker/AliHLTTPCCAGBTracker.h"
#include "TPCCATracker/code/CATracker/AliHLTTPCCAGBTrack.h"
#include "TPCCATracker/code/CATracker/AliHLTTPCCAGBHit.h"
#include "TPCCATracker/code/CATracker/AliHLTTPCCATrackParam.h"

#define padLengthInnerSector    1.15     // cm
#define padLengthOuterSector    1.95     // cm

struct christofs_2d_vector {
    double x;
    double y;
};


class FtfDedx {

public:
    FtfDedx(l3CoordinateTransformer *trafo, float cutLow = 0, float cutHigh = 0.7,
            float driftLoss = 0, const char* HLTparameters = NULL);
    ~FtfDedx () {};

    int  TruncatedMean(FtfTrack *track);
    int  TruncatedMean(AliHLTTPCCAGBTrack *CATrack,
                       const AliHLTTPCCAGBTracker *fCATracker,
                       const FtfHit* ftfHits, const int nHits,
                       const double bField);

    inline int id2index(int hid);
    
    void ReadHLTparameters(const char* HLTparameters = "HLTparameters");
    void SetGainparameters(const double ig, const double og);
    void ReadGainparameters(const char* Gainparameters = "Gainparameters");

private:

    //FtfTrack *fTrack;
    l3CoordinateTransformer *fCoordTransformer;
    double    fDedxArray[45];

    float     fCutLow ;     // Truncated Mean: lower cut in %
    float     fCutHigh ;    // Truncated Mean: higher cut in %
    short     fNTruncate ;  // Pablo: # points to truncate in dEdx
    float     fDriftLoss ;  // corr. factor for drift length dependence of charge loss in m^-1
    double    innerSectorGain;
    double    outerSectorGain;
    double    normInnerGain ;
    double    normOuterGain ;
    struct christofs_2d_vector fUnitVec[24];
};
#endif
