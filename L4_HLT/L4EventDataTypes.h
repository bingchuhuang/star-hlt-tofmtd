#ifndef L4EVENTDATATYPES_H
#define L4EVENTDATATYPES_H

#include "Rtypes.h"

#include "L4_SUPPORT/l4Interface.h"
#include "rtsSystems.h"

#include "DAQ_BTOW/daq_btow.h"
#include "HLT_MTD/mtd_defs.h"
#include "HLT_TOF/tof_hlt_sector.h"
#include "HLTTPCData.h"
#include "TPCCATracker/code/KFParticle/KFPInputData.h"
#include "hltNetworkDataStruct.h"
#include "HLTFormats.h"

// -----------------------------------------------------------------------------
struct HLTProfileData_t
{
    // profiling data
};

// -----------------------------------------------------------------------------
struct HLTEvtData_t
{
    UInt_t    eventId;
    UInt_t    eventTime;
    UInt_t    detectors;        // hex number
    ULong64_t daqbits64_l1;     // hex
    ULong64_t daqbits64_l2;     // hex
    ULong64_t daqbits64;

    ULong64_t l2TrgMask;
    ULong64_t l3TrgMask_fromfile;
    
    UInt_t    tpcSectorMask;    // lower 24 bits indicate fired TPC sectors
    UInt_t    gl3Detectors;
};

// -----------------------------------------------------------------------------
struct HLTTrgData_t
{
    ULong64_t daq_TrgId;
    UInt_t    token;
    Int_t     daq_cmd;
    Int_t     trg_cmd;
};

// -----------------------------------------------------------------------------
struct HLTKFTData_t
{
    // KFParticle tracks
    int kftBuffer[100*1024];
};

// -----------------------------------------------------------------------------
struct HLTTPCData_t
{
    HLTTPCData   tpcOutput;       // kehw, bad type name
    HLTKFTData_t KFTracks;
};

// -----------------------------------------------------------------------------
struct HLTTOFData_t
{
    TofSend tofOutput;
};

// -----------------------------------------------------------------------------
struct HLTMTDData_t
{
    Mtdsend mtdOutput;
};

// -----------------------------------------------------------------------------
struct HLTBTOWData_t
{
    btow_t btowOutput;
};

// -----------------------------------------------------------------------------
struct HLTKFPData_t
{
    // KFParticle particles
    int kfpBuffer[100*1024];
};

// -----------------------------------------------------------------------------
struct L4InitData_t
{
    hlt_blob      detBlobs[RTS_NUM_SYSTEMS];
    HLTEvtData_t  evtData;
    HLTTrgData_t  trgData;
    HLTTPCData_t  tpcData;
    HLTMTDData_t  mtdData;
    HLTTOFData_t  tofData;
    HLTBTOWData_t btowData;
    HLTKFTData_t  kftData;
};

// -----------------------------------------------------------------------------
struct L4SimuData_t
{
    HLTKFPData_t kfpData;
};

// -----------------------------------------------------------------------------
struct L4FinishData_t
{
    ULong64_t l3TrgMask;

    // detector data
    struct HLT_GT   store_GT; 
    struct HLT_PT   store_PT;
    struct HLT_EVE  store_EVE;
    struct HLT_TOF  store_TOF;
    struct HLT_MTD  store_MTD;
    struct HLT_PVPD store_PVPD;
    struct HLT_EMC  store_EMC;
    struct HLT_NODE store_NODE;

    // trigger data
    struct HLT_DIEP          hlt_diEP;
    struct HLT_DIEP          hlt_UPCdiEP;
    struct HLT_HIPT          hlt_hiPt;
    struct HLT_HF            hlt_hF;
    struct HLT_HT2           hlt_hT2;
    struct HLT_LM            hlt_lm;
    struct HLT_RHO           hlt_UPCrho;
    struct HLT_MTDQuarkonium hlt_MTDQkn;
    struct HLT_DIEP          hlt_diEP2Twr;

    // calibration data
    hltCaliData caliData;
};

// -----------------------------------------------------------------------------
struct L4DeciderData_t
{
    ULong64_t l3TrgMask;
}; 

#endif /* L4EVENTDATATYPES_H */
