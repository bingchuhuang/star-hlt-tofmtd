#ifndef HLTTPCDATA_H
#define HLTTPCDATA_H

// define the data structure exported to gl3 from tpc tracker
// this sturcture supporse to be constructed from hlt_tpc_ca::output_store

#include "gl3Track.h"

struct gl3TrackPair {
    gl3Track globalTrack;
    gl3Track primaryTrack;
};

class HLTTPCData {
public:
    HLTTPCData() : primaryVertex{-999, -999, -999},
                   sectorMask(0xFFFFFF), nTracks(0) {
    }

    static const int NTRACKSMAX = 4000;

    float            primaryVertex[3];
    int              sectorMask;
    int              nTracks;
    gl3TrackPair     trackPairs[NTRACKSMAX];
};

#endif /* HLTTPCDATA_H */
