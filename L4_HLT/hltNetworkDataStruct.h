#ifndef _HLTNETWORKDATASTRUCT_H_
#define _HLTNETWORKDATASTRUCT_H_
// data format from client

#include <stdlib.h>


struct hltCaliData {
    // TODO add header here
    // 1.INIT for initial when star run
    // 2.CALB for calib data during the run
    char   header[4];
    char   padding[4];
    // unsigned int port;
    int    runnumber;
    int    npri;
    float  vx;
    float  vy;
    // for dEdx calib
    int    nMipTracks;
    static const int MaxMipTracks = 100;
    float  logdEdx[MaxMipTracks];        // -13.3 -12.3
    static const int MaxDcaTracks = 100;
    int nDcaTracks;
    float  dcaXY[MaxDcaTracks];
};

struct hltCaliResult {
    unsigned int port;
    float x;
    float y;
    float innerGain;
    float outerGain;
    //float sum;
};

typedef struct hltCaliResult hltBeamline;
typedef struct hltCaliResult hltInitialCalib;
typedef struct hltCaliResult hlt_send;
typedef struct hltCaliData   hlt_receive;
#endif /* _HLTNETWORKDATASTRUCT_H_ */
