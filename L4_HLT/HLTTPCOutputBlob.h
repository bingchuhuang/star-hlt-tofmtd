#ifndef HLTTPCOUTPUTBLOB_H
#define HLTTPCOUTPUTBLOB_H

// define the data structure exported to gl3 from tpc tracker
// this sturcture supporse to be constructed from hlt_tpc_ca::output_store

#include "FtfTrack.h"

struct FtfTrackPair {
    FtfTrack globalTrack;
    FtfTrack primaryTrack;
};

class HLTTPCOutputBlob {
public:
    HLTTPCOutputBlob() : nTracks(0) {
        primaryVertex[0] = -999;
        primaryVertex[1] = -999;
        primaryVertex[2] = -999;
    }

    static const int NTRACKSMAX = 10000;
    float            primaryVertex[3];
    int              nTracks;
    FtfTrackPair     trackPairs[NTRACKSMAX];
};
#endif /* HLTTPCOUTPUTBLOb_H */
