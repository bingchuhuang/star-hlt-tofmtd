#ifndef MCEVENT_H
#define MCEVENT_H

#include "TObject.h"

class TrackHitInput : public TObject
{
 public:
    TrackHitInput();
    ~TrackHitInput();

    //Event level
    Int_t   mIEvt;
    Float_t mMagField;  
    Float_t mVertexX;
    Float_t mVertexY;
    Float_t mVertexZ;
    
    //mc track quantities
    Int_t   mNMcTrk;            // number of MC tracks in this event, must less or equal to 50000
    Int_t   mMcId[50000];       // StMcTrack::key, ??
    Int_t   mGeantId[50000];    // particle ID defined in Geant
    Int_t   mParentGeantId[50000]; // parent Geant ID of tracks, -999 for those tracks without parent
    Int_t   mParentMcId[50000]; // StMcTrack::parent::key
    Float_t mMcPt[50000];       // Pt of tracks
    Float_t mMcPz[50000];       // Pz of tracks
    Float_t mMcPhi[50000];      // Phi of tracks
    Int_t   mMcTrkNhits[50000]; // number of hits of each track
    /* index indicate the position of information on each hit
     * example: mMcTrkNhits[i][j] = k indicates that
     *          the information of the number j hit of the number i track 
     *          stores in mMcHitId[ k ]
     *                    mMcHitX[ k ]
     *                    mMcHitY[ k ]
     *                    mMcHitZ[ k ]*/
    Int_t   mMcTrkHitIndex[50000][200];

    Int_t   mMcNhits;           // number of TPC hits
    Short_t mMcHitSector[500000]; 
    Short_t mMcHitPadrow[500000];
    Float_t mMcHitPad[500000];  // pad + time bucket
    Float_t mMcHitTb[500000];   //  pad + time bucket
    Int_t   mMcHitId[500000];   // ??
    float   mMcHitX[500000];    // positions of hits
    float   mMcHitY[500000];
    float   mMcHitZ[500000];
    Int_t   mMcHitKey[500000];  // key of MC hit, i.e. id[x] of the MC track it belongs to, index of MC tracks arraies

    Int_t   mRcNhits;           // number of reconstructed hits (after cluster)
    Short_t mRcHitSector[500000];
    Short_t mRcHitPadrow[500000];
    Float_t mRcHitPad[500000];  // pad + time bucket
    Float_t mRcHitTb[500000];   // pad + time bucket
    Int_t   mRcHitId[500000];   // ??
    Float_t mRcHitX[500000];    // positions of hits
    Float_t mRcHitY[500000];
    Float_t mRcHitZ[500000];
    Int_t   mRcHitIdTruth[500000]; // ??


    Int_t mNRcTrk;
    Int_t mRcId[50000]; // key of matched track
    float mRcPt[50000];
    float mRcPz[50000];
    float mRcPhi[50000];
    Int_t mRcTrkNhits[50000];
    Int_t mRcTrkHitIndex[50000][50];

    // MTD hits
    Int_t    mNMtdHits;
    Char_t   mMtdBackleg[1000]; // 1-30
    Char_t   mMtdFiberId[1000]; // 0-1
    Char_t   mMtdTray[1000];    // 1-5
    Char_t   mMtdChannel[1000]; // 0-11
    Double_t mMtdLeadingEdgeTime[1000][2];
    Double_t mMtdTrailingEdgeTime[1000][2]; 
    Int_t    mMtdHitIdTruth[1000];

    ClassDef(TrackHitInput, 1);
};

typedef TrackHitInput McEvent;

#endif /* MCEVENT_H */
