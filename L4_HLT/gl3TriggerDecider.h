//:>----------------------------------------------------------------------
//: FILE:      gl3TriggerDecider.h
//: HISTORY:
//:>----------------------------------------------------------------------
#ifndef GL3TRIGGERDECIDER
#define GL3TRIGGERDECIDER

#include <stdio.h>	// for FILE ops
#include "HLTFormats.h"


class online_tracking_collector;
class SimpleXmlDoc;

class gl3TriggerDecider {

public:
    gl3TriggerDecider(online_tracking_collector* _event, char* HLTparameters = "HLTparameters");
    ~gl3TriggerDecider () {}
    void setTriggers(char* HLTparameters = "HLTparameters");
    void setQA(char* outputFileName);
    void readHLTparameters(char* HLTparameters = "HLTparameters");
    void setTriggersFromRunControl(SimpleXmlDoc *cfg, int id);
    void writeQA();
    void writeScalers();
    void flushQA() ;	// tonko added
    void closeQA() ;	// tonko added
    void resetEvent();
    void printReport();
    int  getDecision() { return hltDecision; }
    
    int  decide(int eventId = 0); // run all decide_XXX()
    bool decide_HighPt();
    bool decide_DiElectron();
    bool decide_HeavyFragment();
    bool decide_AllEvents();
    bool decide_RandomEvents();
    bool decide_BesGoodEvents();
    bool decide_diV0();
    bool decide_MatchedHT2();
    bool decide_LowMult();
    bool decide_UPCDiElectron();
    bool decide_UPCPair();
    bool decide_MTDDiMuon();
    bool decide_MTDQuarkonium();
    bool decide_FixedTarget();
    bool decide_FixedTargetMonitor();
    bool decide_BesMonitor();
    bool decide_HLTGood2();
    bool decide_DiElectron2Twr();
    
    enum HLTTriggerNames {
        AllEventsNoHLT     = 1,
        HighPt             = 2,
        DiElectron         = 3,
        HeavyFragment      = 4, 
        AllEvents          = 5,
        RandomEvents       = 6,
        BesGoodEvents      = 7,
        DiV0               = 8,
        MatchedHT2         = 9,
        LowMult            = 10,
        UPCDiElectron      = 11,
        UPCPair            = 12,
        MTDDiMuon          = 13,
        FixedTarget        = 14,
        FixedTargetMonitor = 15,
        BesMonitor         = 16,
        HLTGood2           = 17,
	MTDQuarkonium      = 18,
        DiElectron2Twr     = 19,
        
        MaxNHLTTriggers    = 19
    };

    int NumOfCalled[MaxNHLTTriggers + 1];
    int NumOfSucceed[MaxNHLTTriggers + 1];
    
    double bField, scalerCount;

    int triggerOnHighPt;
    int triggerOnDiElectron;
    int triggerOnHeavyFragment;
    int triggerOnAllEvents;
    int triggerOnRandomEvents;
    int triggerOnBesGoodEvents;
    int triggerOnDiV0;
    int triggerOnMatchedHT2;
    int triggerOnLowMult;
    int triggerOnUPCpair;
    int triggerOnUPCdiElectron;
    int triggerOnMTDdiMuon;
    int triggerOnFixedTarget;
    int triggerOnFixedTargetMonitor;
    int triggerOnBesMonitor;
    int triggerOnHLTGood2;
    int triggerOnMTDQuarkonium;
    int triggerOnDiElectron2Twr;

    // trigger specified data banks
    struct HLT_DIEP          hlt_diEP;
    struct HLT_DIEP          hlt_UPCdiEP;
    struct HLT_HIPT          hlt_hiPt;
    struct HLT_HF            hlt_hF;
    struct HLT_HT2           hlt_hT2;
    struct HLT_LM            hlt_lm;
    struct HLT_RHO           hlt_UPCrho;
    struct HLT_MTDQuarkonium hlt_MTDQkn;
    struct HLT_DIEP          hlt_diEP2Twr;
    
private:
    FILE *f1 ;	// tonko: moved here from static in cxx
    online_tracking_collector* event;

    unsigned int triggerBitHighPt;
    unsigned int triggerBitDiElectron;
    unsigned int triggerBitHeavyFragment;
    unsigned int triggerBitAllEvents;
    unsigned int triggerBitRandomEvents;
    unsigned int triggerBitBesGoodEvents;
    unsigned int triggerBitMatchedHT2;
    unsigned int triggerBitLowMult;
    unsigned int triggerBitUPC;
    unsigned int triggerBitUPCdiElectron;
    unsigned int triggerBitMTDdiMuon;
    unsigned int triggerBitMTDQuarkonium;
    unsigned int triggerBitFixedTarget;
    unsigned int triggerBitFixedTargetMonitor;
    unsigned int triggerBitBesMonitor;
    unsigned int triggerBitHLTGood2;
    unsigned int triggerBitDiElectron2Twr;
    
    double innerSectorGain, outerSectorGain, spaceChargeP0, spaceChargeP1;
    double paraXVertex, paraYVertex;
    double sigmaDedx1, dPOverP2, sigmaTof;

    unsigned int highPtTriggered;
    unsigned int diElectronTriggered;
    unsigned int heavyFragmentTriggered;
    unsigned int allEventsTriggered;
    unsigned int randomEventsTriggered;
    unsigned int besGoodEventsTriggered;
    unsigned int matchedHT2Triggered;
    unsigned int LowMultTriggered;
    unsigned int UPCTriggered;
    unsigned int UPCdiElectronTriggered;
    unsigned int MTDdiMuonTriggered;
    unsigned int MTDQuarkoniumTriggered;
    unsigned int FixedTargetTriggered;
    unsigned int FixedTargetMonitorTriggered;
    unsigned int besMonitorTriggered;
    unsigned int HLTGood2Triggered;
    unsigned int diElectron2TwrTriggered;
    
    unsigned int hltDecision;

    //high pt trigger parameters
    int nHitsCut_highPt;
    double ptCut_highPt;
  
    //MTD diMuon trigger parameters
    int    nHitsCut_MTDdiMuon;
    int    nDedxCut_MTDdiMuon;
    double nSigmaPionLow_MTDdiMuon;
    double nSigmaPionHigh_MTDdiMuon;
    double dcaCut_MTDdiMuon;
    double ptCutLow_MTDdiMuon;
    double ptCutHigh_MTDdiMuon;
    double dzTrkHit_MTDdiMuon;
    int    daqId_mtd2hits;
  
    //MTD quarkonium trigger parameters
    int    nHitsCut_MTDQuarkonium;
    double dzTrkHit_MTDQuarkonium;
    double dcaCut_MTDQuarkonium;
    double ptLead_MTDQuarkonium;
    double ptSublead_MTDQuarkonium;

    //diElectron trigger parameters
    int nHitsCut_diElectron;
    int nDedxCut_diElectron;
    double towerEnergyCut_diElectron;
    double pCutLowForTof_diElectron;
    double pCutHighForTof_diElectron;
    double pCutLowForEmc_diElectron;
    double pCutHighForEmc_diElectron;
    double pCutLowForTofAndEmc_diElectron;
    double pCutHighForTofAndEmc_diElectron;
    double nSigmaDedxCutLowForTof_diElectron;
    double nSigmaDedxCutHighForTof_diElectron;
    double nSigmaDedxCutLowForEmc_diElectron;
    double nSigmaDedxCutHighForEmc_diElectron;
    double nSigmaDedxCutLowForTofAndEmc_diElectron;
    double nSigmaDedxCutHighForTofAndEmc_diElectron;
    double p1Cut_diElectron;
    double p2Cut_diElectron;
    double pOverECutLow_diElectron;
    double pOverECutHigh_diElectron;
    double invariantMassCutLow_diElectron;
    double invariantMassCutHigh_diElectron;
    double vertexZDiffCut_diElectron;
    double oneOverBetaCut_diElectron;
    int onlyOppPairs_diElectron;

    //heavyFragment trigger parameters
    int nHitsCut_heavyFragment;
    int nDedxCut_heavyFragment;
    int useTofMatchedGlobalTrack_heavyfragMent;
    double nSigmaDedxHe3Cut_heavyFragment;
    double nSigmaDedxTritonCut_heavyFragment;
    double nSigmaMassTritonCut_heavyFragment;
    double dcaCut_heavyFragment;

    //MatchedHT2 trigger parameters
    int nHitsCut_matchedHT2;
    float PCut_matchedHT2;
    float dPhiCut_matchedHT2;
    float dZCut_matchedHT2;

    //Low Multi cut 
    int nTracksLow_LowMult;
    int nTracksHigh_LowMult;

    //random trigger parameter
    int sampleScale_randomEvents;

    //UPC Pair trigger parameters
    int nTracksCut_UPC;
    float vertexRCut_UPC;
    float vertexZCut_UPC;
    //float invariantMass_UPC;
    //float deltaPhi_UPC;
    //float SavedeltaPhi_UPC;
    int daqId_UPC;
    //UPCdiElectron trigger parameters
    int nTracksCut_UPCdiElectron;
    int nHitsCut_UPCdiElectron;
    int nDedxCut_UPCdiElectron;
    double towerEnergyCut_UPCdiElectron;
    double pCutLowForTof_UPCdiElectron;
    double pCutHighForTof_UPCdiElectron;
    double pCutLowForEmc_UPCdiElectron;
    double pCutHighForEmc_UPCdiElectron;
    double pCutLowForTofAndEmc_UPCdiElectron;
    double pCutHighForTofAndEmc_UPCdiElectron;
    double nSigmaDedxCutLowForTof_UPCdiElectron;
    double nSigmaDedxCutHighForTof_UPCdiElectron;
    double nSigmaDedxCutLowForEmc_UPCdiElectron;
    double nSigmaDedxCutHighForEmc_UPCdiElectron;
    double nSigmaDedxCutLowForTofAndEmc_UPCdiElectron;
    double nSigmaDedxCutHighForTofAndEmc_UPCdiElectron;
    double p1Cut_UPCdiElectron;
    double p2Cut_UPCdiElectron;
    double pOverECutLow_UPCdiElectron;
    double pOverECutHigh_UPCdiElectron;
    double invariantMassCutLow_UPCdiElectron;
    double invariantMassCutHigh_UPCdiElectron;
    double vertexZDiffCut_UPCdiElectron;
    double oneOverBetaCut_UPCdiElectron;
    int onlyOppPairs_UPCdiElectron;

    //BesGoodEvents trigger parameters
    int   nTracksCut_besGoodEvents;
    float vertexRCut_besGoodEvents;
    float vertexZCut_besGoodEvents;

    //HLTGood2 trigger parameters
    int   nTracksCut_HLTGood2;
    float vertexRCut_HLTGood2;
    float vertexZCut_HLTGood2;

    // besMonitor trigger paramaters
    int   nTracksCut_besMonitor;
    float vertexZCut_besMonitor;
    
    // fexted target trigger parameters
    float vertexYCutLow_FixedTarget;
    float vertexYCutHigh_FixedTarget;
    float vertexZCutLow_FixedTarget;
    float vertexZCutHigh_FixedTarget;
    float vertexRCutLow_FixedTarget;
    float vertexRCutHigh_FixedTarget;
    
    float vertexZCutLow_FixedTargetMonitor;
    float vertexZCutHigh_FixedTargetMonitor;

    // diElectron2Twr parameters
    float nHitsCut_diElectron2Twr;               
    float nDedxCut_diElectron2Twr;               
    float towerEnergyCut_diElectron2Twr;         
    float pCutLowForTof_diElectron2Twr;          
    float pCutHighForTof_diElectron2Twr;         
    float pCutLowForEmc_diElectron2Twr;          
    float pCutHighForEmc_diElectron2Twr;         
    float nSigmaDedxCutLowForTof_diElectron2Twr; 
    float nSigmaDedxCutHighForTof_diElectron2Twr;
    float nSigmaDedxCutLowForEmc_diElectron2Twr; 
    float nSigmaDedxCutHighForEmc_diElectron2Twr;
    float p1Cut_diElectron2Twr;                  
    float p2Cut_diElectron2Twr;                  
    float invariantMassCutLow_diElectron2Twr;    
    float invariantMassCutHigh_diElectron2Twr;   
    float pOverECutLow_diElectron2Twr;           
    float pOverECutHigh_diElectron2Twr;          
    float oneOverBetaCut_diElectron2Twr;         
    float onlyOppPairs_diElectron2Twr;           

    //debug
    int Debug_allTracks;
    int Debug_tracks;
    int Debug_towers;
    int Debug_matchs;
    int Debug_vertex;

    int nHitsCut_debug;
    int nDedxCut_debug;
    double matchZEdgeCut_debug;
    double matchPhiDiffCut_debug;
    double dedxCutLow_debug;
    double dedxCutHigh_debug;
    double pCut_debug;
    double towerEnergyCut_debug;
   
};
#endif
