/*:>-------------------------------------------------------------------
**: FILE:     gl3TriggerDecider.cxx
**: HISTORY:
**:<------------------------------------------------------------------*/

#include <iostream>
#include <string.h>
#include <fstream>
#include "TLorentzVector.h"
#include "TF1.h"
#include "gl3TriggerDecider.h"
#include <stdio.h>  // for FILE ops
#include <sys/types.h>
#include <unistd.h>
#include "online_tracking_collector.h"
#include "gl3Bischel.h"
#include "gl3TOF.h"
#include "gl3MTD.h"
#include "gl3Node.h"
#include <XMLCFG/SimpleXmlDoc.h>
#include "EventInfo.h"

const int maxNElectronTracks = 1000;
const int maxNTowers = 4800;
const int maxNMuonTracks = 1000;

//***********************************************************************
// constructor
//***********************************************************************
gl3TriggerDecider::gl3TriggerDecider(online_tracking_collector* _event, char* HLTparameters)
{
    event = _event;
    setTriggers(HLTparameters);
    f1 = 0;

}

void gl3TriggerDecider::flushQA()
{
    if(f1) fflush(f1) ;
}

void gl3TriggerDecider::closeQA()
{
    if(f1) {
        fclose(f1) ;
        f1 = 0 ;
    }
}

void gl3TriggerDecider::setTriggers(char* HLTparameters)
{
    //set which triggers to enable here
    triggerOnHighPt             = 1;
    triggerOnDiElectron         = 1;
    triggerOnHeavyFragment      = 1;
    triggerOnAllEvents          = 1;
    triggerOnRandomEvents       = 1;
    triggerOnBesGoodEvents      = 1;
    triggerOnMatchedHT2         = 1;
    triggerOnLowMult            = 1;
    triggerOnUPCpair            = 1;
    triggerOnUPCdiElectron      = 1;
    triggerOnMTDdiMuon          = 1;
    triggerOnMTDQuarkonium      = 1;
    triggerOnFixedTarget        = 1;
    triggerOnFixedTargetMonitor = 1;
    triggerOnBesMonitor         = 1;
    triggerOnHLTGood2           = 1;
    triggerOnDiElectron2Twr     = 1;
    
    triggerBitHighPt             = 0x00010000;
    triggerBitDiElectron         = 0x00020000;
    triggerBitHeavyFragment      = 0x00040000;
    triggerBitAllEvents          = 0x00080000;
    triggerBitRandomEvents       = 0x00100000;
    triggerBitBesGoodEvents      = 0x00200000;
    triggerBitMatchedHT2         = 0x00800000;
    triggerBitLowMult            = 0x01000000;
    triggerBitUPCdiElectron      = 0x02000000;
    triggerBitUPC                = 0x04000000;
    triggerBitMTDdiMuon          = 0x08000000;
    triggerBitMTDQuarkonium      = 0x00400000;
    triggerBitFixedTarget        = 0x10000000;
    triggerBitFixedTargetMonitor = 0x20000000;
    triggerBitBesMonitor         = 0x40000000;
    triggerBitHLTGood2           = 0x80000000;
    triggerBitDiElectron2Twr     = 0x00000001; // start to up lower 16 bit.
    
    memset(NumOfCalled, 0, sizeof(NumOfCalled));
    memset(NumOfSucceed, 0, sizeof(NumOfSucceed));

    //high pt trigger parametes
    nHitsCut_highPt         = 20;
    ptCut_highPt            = 5.;

    // MTD diMuon trigger parameters
    ptCutLow_MTDdiMuon       = 1.0;
    nHitsCut_MTDdiMuon       = 10;
    nDedxCut_MTDdiMuon       = 0;
    nSigmaPionLow_MTDdiMuon  = -1;
    nSigmaPionHigh_MTDdiMuon = 3;
    dcaCut_MTDdiMuon         = 10;
    ptCutHigh_MTDdiMuon      = 1.5;
    dzTrkHit_MTDdiMuon       = 50;
    // daqId_mtd2hits           = 0x10;

    // MTD Quarkonium trigger parameters
    nHitsCut_MTDQuarkonium      = 15;
    dzTrkHit_MTDQuarkonium      = 40;
    dcaCut_MTDQuarkonium        = 10;
    ptLead_MTDQuarkonium        = 1.5;
    ptSublead_MTDQuarkonium     = 1.0;

    //diElectron trigger parameters
    nHitsCut_diElectron                      = 15;
    nDedxCut_diElectron                      = 5;
    towerEnergyCut_diElectron                = 0.5;
    pCutLowForTof_diElectron                 = 0;
    pCutHighForTof_diElectron                = 0;
    pCutLowForEmc_diElectron                 = 0.5;
    pCutHighForEmc_diElectron                = 1.e10;
    pCutLowForTofAndEmc_diElectron           = 0;
    pCutHighForTofAndEmc_diElectron          = 1.e10;
    nSigmaDedxCutLowForTof_diElectron        = 0.;
    nSigmaDedxCutHighForTof_diElectron       = 3.;
    nSigmaDedxCutLowForEmc_diElectron        = -1.;
    nSigmaDedxCutHighForEmc_diElectron       = 3.;
    nSigmaDedxCutLowForTofAndEmc_diElectron  = -3.;
    nSigmaDedxCutHighForTofAndEmc_diElectron = 3.;
    p1Cut_diElectron                         = 2.;
    p2Cut_diElectron                         = 0.5;
    pOverECutLow_diElectron                  = 0.3;
    pOverECutHigh_diElectron                 = 1.5;
    invariantMassCutLow_diElectron           = 0.;
    invariantMassCutHigh_diElectron          = 1.e6;
    vertexZDiffCut_diElectron                = 20.;
    oneOverBetaCut_diElectron                = 0.03;
    onlyOppPairs_diElectron                  = 0;

    //heavyFragment trigger parameters
    nHitsCut_heavyFragment                 = 0;
    nDedxCut_heavyFragment                 = 0;
    useTofMatchedGlobalTrack_heavyfragMent = 0;
    nSigmaDedxHe3Cut_heavyFragment         = 0.;
    nSigmaDedxTritonCut_heavyFragment      = 0.;
    nSigmaMassTritonCut_heavyFragment      = 0.;
    dcaCut_heavyFragment                   = 0.;

    sampleScale_randomEvents = 997;

    //BesGoodEvents trigger parameters
    nTracksCut_besGoodEvents = 5;
    vertexRCut_besGoodEvents = 2.;
    vertexZCut_besGoodEvents = 30.;

    //HLTGood2 trigger parameters
    nTracksCut_HLTGood2 = 5;
    vertexRCut_HLTGood2 = 1.5;
    vertexZCut_HLTGood2 = 5;

    //HT trigger parameters
    nHitsCut_matchedHT2 = 10;   //20;
    dPhiCut_matchedHT2  = 0.12;
    dZCut_matchedHT2    = 10;
    PCut_matchedHT2     = 1.0;

    //UPC trigger parameters
    nTracksCut_UPC = 5;
    vertexRCut_UPC = 2.;
    vertexZCut_UPC = 100.;
    daqId_UPC      = 0x1 | 0x4;

    //UPCdiElectron trigger parameters
    nTracksCut_UPCdiElectron                    = 30;
    nHitsCut_UPCdiElectron                      = 15;
    nDedxCut_UPCdiElectron                      = 5;
    towerEnergyCut_UPCdiElectron                = 0.5;
    pCutLowForTof_UPCdiElectron                 = 0;
    pCutHighForTof_UPCdiElectron                = 0;
    pCutLowForEmc_UPCdiElectron                 = 0.5;
    pCutHighForEmc_UPCdiElectron                = 1.e10;
    pCutLowForTofAndEmc_UPCdiElectron           = 0;
    pCutHighForTofAndEmc_UPCdiElectron          = 1.e10;
    nSigmaDedxCutLowForTof_UPCdiElectron        = 0.;
    nSigmaDedxCutHighForTof_UPCdiElectron       = 3.;
    nSigmaDedxCutLowForEmc_UPCdiElectron        = -1.;
    nSigmaDedxCutHighForEmc_UPCdiElectron       = 3.;
    nSigmaDedxCutLowForTofAndEmc_UPCdiElectron  = -3.;
    nSigmaDedxCutHighForTofAndEmc_UPCdiElectron = 3.;
    p1Cut_UPCdiElectron                         = 2.;
    p2Cut_UPCdiElectron                         = 0.5;
    pOverECutLow_UPCdiElectron                  = 0.3;
    pOverECutHigh_UPCdiElectron                 = 1.5;
    invariantMassCutLow_UPCdiElectron           = 0.;
    invariantMassCutHigh_UPCdiElectron          = 1.e6;
    vertexZDiffCut_UPCdiElectron                = 20.;
    oneOverBetaCut_UPCdiElectron                = 0.03;
    onlyOppPairs_UPCdiElectron                  = 0;

    //Low Multi parameters
    nTracksLow_LowMult  = 0;
    nTracksHigh_LowMult = 10000;

    vertexYCutLow_FixedTarget  = -1.5;
    vertexYCutHigh_FixedTarget = -4;
    vertexZCutLow_FixedTarget  = 195;
    vertexZCutHigh_FixedTarget = 225;
    vertexRCutLow_FixedTarget  = 0;
    vertexRCutHigh_FixedTarget = 4;

    vertexZCutLow_FixedTargetMonitor  = vertexZCutLow_FixedTarget;
    vertexZCutHigh_FixedTargetMonitor = vertexZCutHigh_FixedTarget;

    nTracksCut_besMonitor = 5;
    vertexZCut_besMonitor = 30.;

    // DiElectron2Twr
    nHitsCut_diElectron2Twr                = 15;
    nDedxCut_diElectron2Twr                = 10;
    towerEnergyCut_diElectron2Twr          = 0.5;
    pCutLowForTof_diElectron2Twr           = 0.5;
    pCutHighForTof_diElectron2Twr          = 100;
    pCutLowForEmc_diElectron2Twr           = 1.5;
    pCutHighForEmc_diElectron2Twr          = 100;
    nSigmaDedxCutLowForTof_diElectron2Twr  = -1.5;
    nSigmaDedxCutHighForTof_diElectron2Twr = 3.;
    nSigmaDedxCutLowForEmc_diElectron2Twr  = -2;
    nSigmaDedxCutHighForEmc_diElectron2Twr = 3.;
    p1Cut_diElectron2Twr                   = 1.5;
    p2Cut_diElectron2Twr                   = 0.5;
    invariantMassCutLow_diElectron2Twr     = 2.0;
    invariantMassCutHigh_diElectron2Twr    = 1000;
    pOverECutLow_diElectron2Twr            = 0.1;
    pOverECutHigh_diElectron2Twr           = 2.5;
    oneOverBetaCut_diElectron2Twr          = 0.03;
    onlyOppPairs_diElectron2Twr            = 0;

    //debug parameters
    Debug_allTracks          = 0;
    Debug_tracks             = 0;
    Debug_towers             = 0;
    Debug_matchs             = 0;
    Debug_vertex             = 0;

    nHitsCut_debug        = 10;
    nDedxCut_debug        = 0;
    dedxCutLow_debug      = 0.e-6;
    dedxCutHigh_debug     = 1.;
    pCut_debug            = 0.;
    towerEnergyCut_debug  = 0.5;
    matchPhiDiffCut_debug = 0.2;
    matchZEdgeCut_debug   = 10;

    //others
    //   innerSectorGain = 0.;
    //   outerSectorGain = 0.;
    spaceChargeP0 = 0.;
    spaceChargeP1 = 0.;
    paraXVertex   = 0.;
    paraYVertex   = 0.;
    bField        = 0.;
    scalerCount   = 0.;
    sigmaDedx1    = 0.;
    dPOverP2      = 0.;
    sigmaTof      = 0.;

    readHLTparameters(HLTparameters);

}

void gl3TriggerDecider::setTriggersFromRunControl(SimpleXmlDoc *cfg, int index)
{
    int id = cfg->getParamI("trg_setup.triggers[%d].l3.id",index);
#ifndef OLDCFG
    id = id - 100;
    if(id < 1) return;
#endif
    // see l3_support.C
    switch (id) {
    case 0:
        // not used
        break;
    case 1:
        // allEvents_no_HLT
        break;
    case 2:
        // highPt
        nHitsCut_highPt = cfg->getParamI("trg_setup.triggers[%d].l3.userInt[%d]", index, 0);
        ptCut_highPt    = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 0);
        break;
    case 3:
        // diElectron
        nHitsCut_diElectron                     = cfg->getParamI("trg_setup.triggers[%d].l3.userInt[%d]", index, 0);
        nDedxCut_diElectron                     = cfg->getParamI("trg_setup.triggers[%d].l3.userInt[%d]", index, 1);
        nSigmaDedxCutLowForTof_diElectron       = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 0);
        nSigmaDedxCutLowForEmc_diElectron       = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 1);
        nSigmaDedxCutLowForTofAndEmc_diElectron = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 2);
        p2Cut_diElectron                        = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 3);
        oneOverBetaCut_diElectron               = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 4);
        break;
    case 4:
        // heavyFragment
        nHitsCut_heavyFragment            = cfg->getParamI("trg_setup.triggers[%d].l3.userInt[%d]", index, 0);
        nDedxCut_heavyFragment            = cfg->getParamI("trg_setup.triggers[%d].l3.userInt[%d]", index, 1);
        nSigmaDedxHe3Cut_heavyFragment    = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 0);
        nSigmaDedxTritonCut_heavyFragment = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 1);
        nSigmaMassTritonCut_heavyFragment = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 2);
        dcaCut_heavyFragment              = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 3);
        break;
    case 5:
        // allEvents_with_HLT
        break;
    case 6:
        // randomEvents
        sampleScale_randomEvents = cfg->getParamI("trg_setup.triggers[%d].l3.userInt[%d]", index, 0);
        break;
    case 7:
        // besGoodEvents
        nTracksCut_besGoodEvents = cfg->getParamI("trg_setup.triggers[%d].l3.userInt[%d]", index, 0);
        vertexRCut_besGoodEvents = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 0);
        vertexZCut_besGoodEvents = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 1);
        break;
    case 8:
        // diV0
        break;
    case 9:
        // matchedHT2
        nHitsCut_matchedHT2 = cfg->getParamI("trg_setup.triggers[%d].l3.userInt[%d]", index, 0);
        PCut_matchedHT2     = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 0);
        dPhiCut_matchedHT2  = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 1);
        dZCut_matchedHT2    = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 2);
        break;
    case 10:
        // LowMult
        nTracksLow_LowMult  = cfg->getParamI("trg_setup.triggers[%d].l3.userInt[%d]", index, 0);
        nTracksHigh_LowMult = cfg->getParamI("trg_setup.triggers[%d].l3.userInt[%d]", index, 1);
        break;
    case 11:
        // UPCdielectron
        nHitsCut_UPCdiElectron                     = cfg->getParamI("trg_setup.triggers[%d].l3.userInt[%d]", index, 0); 
        nDedxCut_UPCdiElectron                     = cfg->getParamI("trg_setup.triggers[%d].l3.userInt[%d]", index, 1); 
        nSigmaDedxCutLowForTof_UPCdiElectron       = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 0);
        nSigmaDedxCutLowForEmc_UPCdiElectron       = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 1);
        nSigmaDedxCutLowForTofAndEmc_UPCdiElectron = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 2);
        p2Cut_UPCdiElectron                        = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 3);
        oneOverBetaCut_UPCdiElectron               = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 4);
        break;
    case 12:
        // UPCpair
        break;
    case 13:
        // MTDdiMuon
        ptCutHigh_MTDdiMuon = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 0);
        dzTrkHit_MTDdiMuon  = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 1);
        break;
    case 14:
        // FixedTarget
        vertexZCutLow_FixedTarget  = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 0);
        vertexZCutHigh_FixedTarget = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 1);
        break;
    case 15:
        // FixedTargetMonitor
        vertexZCutLow_FixedTargetMonitor  = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 0);
        vertexZCutHigh_FixedTargetMonitor = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 1);
        break;
    case 16:
        // besMonitor
        nTracksCut_besMonitor = cfg->getParamI("trg_setup.triggers[%d].l3.userInt[%d]", index, 0);
        vertexZCut_besMonitor = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 0);
        break;
    case 17:
        // HLTGood2
        nTracksCut_HLTGood2 = cfg->getParamI("trg_setup.triggers[%d].l3.userInt[%d]", index, 0);
        vertexRCut_HLTGood2 = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 0);
        vertexZCut_HLTGood2 = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]",index, 1);
        break;
    case 18:
        // MTDQuarkonium
        nHitsCut_MTDQuarkonium  = cfg->getParamI("trg_setup.triggers[%d].l3.userInt[%d]", index, 0);
        dzTrkHit_MTDQuarkonium  = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]", index, 0);
        dcaCut_MTDQuarkonium    = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]", index, 1);
        ptLead_MTDQuarkonium    = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]", index, 2);
        ptSublead_MTDQuarkonium = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]", index, 3);
        break;
    case 19:
        invariantMassCutLow_diElectron2Twr = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]", index, 0);
        pOverECutLow_diElectron2Twr        = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]", index, 1);
        pOverECutHigh_diElectron2Twr       = cfg->getParamF("trg_setup.triggers[%d].l3.userFloat[%d]", index, 2);
        break;
    default:
        LOG(ERR, "Unknow gl3 trigger id");
        break;
    }

}

void gl3TriggerDecider::setQA(char* outputFileName)
{
    f1 = fopen(outputFileName, "w");

    fprintf(f1, "Settings and cuts:\n");
    fprintf(f1, "triggerOnHighPt = %i\n", triggerOnHighPt);
    fprintf(f1, "triggerOnDiElectron = %i\n", triggerOnDiElectron);
    fprintf(f1, "triggerOnHeavyFragment = %i\n", triggerOnHeavyFragment);
    fprintf(f1, "triggerOnAllEvents = %i\n", triggerOnAllEvents);
    fprintf(f1, "triggerOnRandomEvents = %i\n", triggerOnRandomEvents);
    fprintf(f1, "triggerOnBesGoodEvents = %i\n", triggerOnBesGoodEvents);
    fprintf(f1, "triggerOnMatchedHT2 = %i\n", triggerOnMatchedHT2);
    fprintf(f1, "triggerOnLowMult = %i\n", triggerOnLowMult);
    fprintf(f1, "triggerOnUPCpair = %i\n", triggerOnUPCpair);
    fprintf(f1, "triggerOnUPCdiElectron= %i\n", triggerOnUPCdiElectron);
    fprintf(f1, "triggerOnMTDdiMuon= %i\n", triggerOnMTDdiMuon);
    fprintf(f1, "triggerOnHLTGood2 = %i\n", triggerOnHLTGood2);
    fprintf(f1, "\n");
    fprintf(f1, "triggerBitHighPt = 0x%x\n", triggerBitHighPt);
    fprintf(f1, "triggerBitDiElectron = 0x%x\n", triggerBitDiElectron);
    fprintf(f1, "triggerBitHeavyFragment = 0x%x\n", triggerBitHeavyFragment);
    fprintf(f1, "triggerBitAllEvents = 0x%x\n", triggerBitAllEvents);
    fprintf(f1, "triggerBitRandomEvents = 0x%x\n", triggerBitRandomEvents);
    fprintf(f1, "triggerBitBesGoodEvents = 0x%x\n", triggerBitBesGoodEvents);
    fprintf(f1, "triggerBitMatchedHT2 = 0x%x\n", triggerBitMatchedHT2);
    fprintf(f1, "triggerBitLowMult = 0x%x\n", triggerBitLowMult);
    fprintf(f1, "triggerBitUPC = 0x%x\n", triggerBitUPC);
    fprintf(f1, "triggerBitUPCdiElectron = 0x%x\n", triggerBitUPCdiElectron);
    fprintf(f1, "triggerBitMTDdiMuon = 0x%x\n", triggerBitMTDdiMuon);
    fprintf(f1, "triggerBitHLTGood2 = 0x%x\n", triggerBitHLTGood2);
    fprintf(f1, "daqId_UPC = 0x%x\n", daqId_UPC);
    fprintf(f1, "daqId_mtd2hits = 0x%x\n", daqId_mtd2hits);
    fprintf(f1, "\n");
    fprintf(f1, "nHitsCut_highPt = %i\n", nHitsCut_highPt);
    fprintf(f1, "ptCut_highPt = %e\n", ptCut_highPt);
    fprintf(f1, "\n");
    fprintf(f1, "nHitsCut_heavyFragment = %i\n", nHitsCut_heavyFragment);
    fprintf(f1, "nDedxCut_heavyFragment = %i\n", nDedxCut_heavyFragment);
    fprintf(f1, "useTofMatchedGlobalTrack_heavyfragMent = %i\n", useTofMatchedGlobalTrack_heavyfragMent);
    fprintf(f1, "nSigmaDedxHe3Cut_heavyFragment = %e\n", nSigmaDedxHe3Cut_heavyFragment);
    fprintf(f1, "nSigmaDedxTritonCut_heavyFragment = %e\n", nSigmaDedxTritonCut_heavyFragment);
    fprintf(f1, "nSigmaMassTritonCut_heavyFragment = %e\n", nSigmaMassTritonCut_heavyFragment);
    fprintf(f1, "dcaCut_heavyFragment = %e\n", dcaCut_heavyFragment);
    fprintf(f1, "\n");
    fprintf(f1, "ptCutLow_MTDdiMuon = %e\n", ptCutLow_MTDdiMuon);
    fprintf(f1, "nHitsCut_MTDdiMuon = %i\n", nHitsCut_MTDdiMuon);
    fprintf(f1, "nDedxCut_MTDdiMuon = %i\n", nDedxCut_MTDdiMuon);
    fprintf(f1, "nSigmaPionLow_MTDdiMuon = %e\n", nSigmaPionLow_MTDdiMuon);
    fprintf(f1, "nSigmaPionHigh_MTDdiMuon = %e\n", nSigmaPionHigh_MTDdiMuon);
    fprintf(f1, "dcaCut_MTDdiMuon = %e\n", dcaCut_MTDdiMuon);
    fprintf(f1, "ptCutHigh_MTDdiMuon = %e\n", ptCutHigh_MTDdiMuon);
    fprintf(f1, "dzTrkHit_MTDdiMuon = %e\n", dzTrkHit_MTDdiMuon);
    fprintf(f1, "\n");
    fprintf(f1, "nHitsCut_MTDQuarkonium = %i\n", nHitsCut_MTDQuarkonium);
    fprintf(f1, "dzTrkHit_MTDQuarkonium = %e\n", dzTrkHit_MTDQuarkonium);
    fprintf(f1, "dcaCut_MTDQuarkonium = %e\n", dcaCut_MTDQuarkonium);
    fprintf(f1, "ptLead_MTDQuarkonium = %e\n", ptLead_MTDQuarkonium);
    fprintf(f1, "ptSublead_MTDQuarkonium = %e\n", ptSublead_MTDQuarkonium);
    fprintf(f1, "\n");
    fprintf(f1, "nHitsCut_diElectron = %i\n", nHitsCut_diElectron);
    fprintf(f1, "nDedxCut_diElectron = %i\n", nDedxCut_diElectron);
    fprintf(f1, "towerEnergyCut_diElectron = %e\n", towerEnergyCut_diElectron);
    fprintf(f1, "pCutLowForTof_diElectron = %e\n", pCutLowForTof_diElectron);
    fprintf(f1, "pCutHighForTof_diElectron = %e\n", pCutHighForTof_diElectron);
    fprintf(f1, "pCutLowForEmc_diElectron = %e\n", pCutLowForEmc_diElectron);
    fprintf(f1, "pCutHighForEmc_diElectron = %e\n", pCutHighForEmc_diElectron);
    fprintf(f1, "pCutLowForTofAndEmc_diElectron = %e\n", pCutLowForTofAndEmc_diElectron);
    fprintf(f1, "pCutHighForTofAndEmc_diElectron = %e\n", pCutHighForTofAndEmc_diElectron);
    fprintf(f1, "nSigmaDedxCutLowForTof_diElectron = %e\n", nSigmaDedxCutLowForTof_diElectron);
    fprintf(f1, "nSigmaDedxCutHighForTof_diElectron = %e\n", nSigmaDedxCutHighForTof_diElectron);
    fprintf(f1, "nSigmaDedxCutLowForEmc_diElectron = %e\n", nSigmaDedxCutLowForEmc_diElectron);
    fprintf(f1, "nSigmaDedxCutHighForEmc_diElectron = %e\n", nSigmaDedxCutHighForEmc_diElectron);
    fprintf(f1, "nSigmaDedxCutLowForTofAndEmc_diElectron = %e\n", nSigmaDedxCutLowForTofAndEmc_diElectron);
    fprintf(f1, "nSigmaDedxCutHighForTofAndEmc_diElectron = %e\n", nSigmaDedxCutHighForTofAndEmc_diElectron);
    fprintf(f1, "p1Cut_diElectron = %e\n", p1Cut_diElectron);
    fprintf(f1, "p2Cut_diElectron = %e\n", p2Cut_diElectron);
    fprintf(f1, "pOverECutLow_diElectron = %e\n", pOverECutLow_diElectron);
    fprintf(f1, "pOverECutHigh_diElectron = %e\n", pOverECutHigh_diElectron);
    fprintf(f1, "invariantMassCutLow_diElectron = %e\n", invariantMassCutLow_diElectron);
    fprintf(f1, "invariantMassCutHigh_diElectron = %e\n", invariantMassCutHigh_diElectron);
    fprintf(f1, "vertexZDiffCut_diElectron = %e\n", vertexZDiffCut_diElectron);
    fprintf(f1, "oneOverBetaCut_diElectron = %e\n", oneOverBetaCut_diElectron);
    fprintf(f1, "onlyOppPairs_diElectron = %i\n", onlyOppPairs_diElectron);
    fprintf(f1, "\n");
    fprintf(f1, "nTracksCut_UPC = %i\n", nTracksCut_UPC);
    fprintf(f1, "vertexRCut_UPC = %f\n", vertexRCut_UPC);
    fprintf(f1, "vertexZCut_UPC = %f\n", vertexZCut_UPC);
    fprintf(f1, "\n");
    fprintf(f1, "nTracksCut_UPCdiElectron = %i\n", nTracksCut_UPCdiElectron);
    fprintf(f1, "nHitsCut_UPCdiElectron = %i\n", nHitsCut_UPCdiElectron);
    fprintf(f1, "nDedxCut_UPCdiElectron = %i\n", nDedxCut_UPCdiElectron);
    fprintf(f1, "towerEnergyCut_UPCdiElectron = %e\n", towerEnergyCut_UPCdiElectron);
    fprintf(f1, "pCutLowForTof_UPCdiElectron = %e\n", pCutLowForTof_UPCdiElectron);
    fprintf(f1, "pCutHighForTof_UPCdiElectron = %e\n", pCutHighForTof_UPCdiElectron);
    fprintf(f1, "pCutLowForEmc_UPCdiElectron = %e\n", pCutLowForEmc_UPCdiElectron);
    fprintf(f1, "pCutHighForEmc_UPCdiElectron = %e\n", pCutHighForEmc_UPCdiElectron);
    fprintf(f1, "pCutLowForTofAndEmc_UPCdiElectron = %e\n", pCutLowForTofAndEmc_UPCdiElectron);
    fprintf(f1, "pCutHighForTofAndEmc_UPCdiElectron = %e\n", pCutHighForTofAndEmc_UPCdiElectron);
    fprintf(f1, "nSigmaDedxCutLowForTof_UPCdiElectron = %e\n", nSigmaDedxCutLowForTof_UPCdiElectron);
    fprintf(f1, "nSigmaDedxCutHighForTof_UPCdiElectron = %e\n", nSigmaDedxCutHighForTof_UPCdiElectron);
    fprintf(f1, "nSigmaDedxCutLowForEmc_UPCdiElectron = %e\n", nSigmaDedxCutLowForEmc_UPCdiElectron);
    fprintf(f1, "nSigmaDedxCutHighForEmc_UPCdiElectron = %e\n", nSigmaDedxCutHighForEmc_UPCdiElectron);
    fprintf(f1, "nSigmaDedxCutLowForTofAndEmc_UPCdiElectron = %e\n", nSigmaDedxCutLowForTofAndEmc_UPCdiElectron);
    fprintf(f1, "nSigmaDedxCutHighForTofAndEmc_UPCdiElectron = %e\n", nSigmaDedxCutHighForTofAndEmc_UPCdiElectron);
    fprintf(f1, "p1Cut_UPCdiElectron = %e\n", p1Cut_UPCdiElectron);
    fprintf(f1, "p2Cut_UPCdiElectron = %e\n", p2Cut_UPCdiElectron);
    fprintf(f1, "pOverECutLow_UPCdiElectron = %e\n", pOverECutLow_UPCdiElectron);
    fprintf(f1, "pOverECutHigh_UPCdiElectron = %e\n", pOverECutHigh_UPCdiElectron);
    fprintf(f1, "invariantMassCutLow_UPCdiElectron = %e\n", invariantMassCutLow_UPCdiElectron);
    fprintf(f1, "invariantMassCutHigh_UPCdiElectron = %e\n", invariantMassCutHigh_UPCdiElectron);
    fprintf(f1, "vertexZDiffCut_UPCdiElectron = %e\n", vertexZDiffCut_UPCdiElectron);
    fprintf(f1, "oneOverBetaCut_UPCdiElectron = %e\n", oneOverBetaCut_UPCdiElectron);
    fprintf(f1, "onlyOppPairs_UPCdiElectron = %i\n", onlyOppPairs_UPCdiElectron);
    fprintf(f1, "\n");
    fprintf(f1, "nHitsCut_matchedHT2 = %i\n", nHitsCut_matchedHT2);
    fprintf(f1, "dPhiCut_matchedHT2 = %f\n", dPhiCut_matchedHT2);
    fprintf(f1, "dZCut_matchedHT2 = %f\n", dZCut_matchedHT2);
    fprintf(f1, "PCut_matchedHT2 = %f\n", PCut_matchedHT2);
    fprintf(f1, "nTracksLow_LowMult = %i\n", nTracksLow_LowMult);
    fprintf(f1, "nTracksHigh_LowMult = %i\n", nTracksHigh_LowMult);
    fprintf(f1, "\n");
    fprintf(f1, "sampleScale_randomEvents = %i\n", sampleScale_randomEvents);
    fprintf(f1, "\n");
    fprintf(f1, "nTracksCut_besGoodEvents = %i\n", nTracksCut_besGoodEvents);
    fprintf(f1, "vertexRCut_besGoodEvents = %e\n", vertexRCut_besGoodEvents);
    fprintf(f1, "vertexZCut_besGoodEvents = %e\n", vertexZCut_besGoodEvents);
    fprintf(f1, "nTracksCut_HLTGood2 = %i\n", nTracksCut_HLTGood2);
    fprintf(f1, "vertexRCut_HLTGood2 = %e\n", vertexRCut_HLTGood2);
    fprintf(f1, "vertexZCut_HLTGood2 = %e\n", vertexZCut_HLTGood2);
    fprintf(f1, "\n");
//  fprintf(f1, "dedx innerSectorGain = %e\n", innerSectorGain);
//  fprintf(f1, "dedx outerSectorGain = %e\n", outerSectorGain);
    fprintf(f1, "\n");
    fprintf(f1, "spaceChargeP0 = %e\n", spaceChargeP0);
    fprintf(f1, "spaceChargeP1 = %e\n", spaceChargeP1);
    fprintf(f1, "\n");
    fprintf(f1, "xVertex = %e\n", paraXVertex);
    fprintf(f1, "yVertex = %e\n", paraYVertex);
    fprintf(f1, "\n");
    fprintf(f1, "bField = %e\n", bField);
    fprintf(f1, "scalerCount = %e\n", scalerCount);
    fprintf(f1, "\n");
    fprintf(f1, "sigmaDedx1 = %e\n", sigmaDedx1);
    fprintf(f1, "dPOverP2 = %e\n", dPOverP2);
    fprintf(f1, "sigmaTof = %e\n", sigmaTof);
    fprintf(f1, "\n");

    fclose(f1);
}

void gl3TriggerDecider::readHLTparameters(char* HLTparameters)
{
    string parameterName;
    ifstream ifs(HLTparameters);
    if(!ifs.fail())
        while(!ifs.eof()) {
            ifs >> parameterName;
            if(parameterName == "triggerOnHighPt") ifs >> triggerOnHighPt;
            if(parameterName == "triggerOnDiElectron") ifs >> triggerOnDiElectron;
            if(parameterName == "triggerOnHeavyFragment") ifs >> triggerOnHeavyFragment;
            if(parameterName == "triggerOnAllEvents") ifs >> triggerOnAllEvents;
            if(parameterName == "triggerOnRandomEvents") ifs >> triggerOnRandomEvents;
            if(parameterName == "triggerOnBesGoodEvents") ifs >> triggerOnBesGoodEvents;
            if(parameterName == "triggerOnMatchedHT2") ifs >> triggerOnMatchedHT2;
            if(parameterName == "triggerOnLowMult") ifs >> triggerOnLowMult;
            if(parameterName == "triggerOnUPCpair") ifs >> triggerOnUPCpair;
            if(parameterName == "triggerOnUPCdiElectron") ifs >> triggerOnUPCdiElectron;
            if(parameterName == "triggerOnMTDdiMuon") ifs >> triggerOnMTDdiMuon;
            if(parameterName == "triggerOnFixedTarget") ifs >> triggerOnFixedTarget;
            if(parameterName == "triggerOnFixedTargetMonitor") ifs >> triggerOnFixedTargetMonitor;
            if(parameterName == "triggerOnBesMonitor") ifs >> triggerOnBesMonitor;
            if(parameterName == "triggerOnHLTGood2") ifs >> triggerOnHLTGood2;
            
            if(parameterName == "triggerBitHighPt") ifs >> hex >> triggerBitHighPt;
            if(parameterName == "triggerBitDiElectron") ifs >> hex >> triggerBitDiElectron;
            if(parameterName == "triggerBitHeavyFragment") ifs >> hex >> triggerBitHeavyFragment;
            if(parameterName == "triggerBitAllEvents") ifs >> hex >> triggerBitAllEvents;
            if(parameterName == "triggerBitRandomEvents") ifs >> hex >> triggerBitRandomEvents;
            if(parameterName == "triggerBitBesGoodEvents") ifs >> hex >> triggerBitBesGoodEvents;
            if(parameterName == "triggerBitMatchedHT2") ifs >> hex >> triggerBitMatchedHT2;
            if(parameterName == "triggerBitLowMult") ifs >> hex >> triggerBitLowMult;
            if(parameterName == "triggerBitUPC") ifs >> hex >> triggerBitUPC;
            if(parameterName == "triggerBitUPCdiElectron") ifs >> hex >> triggerBitUPCdiElectron;
            if(parameterName == "triggerBitMTDdiMuon") ifs >> hex >> triggerBitMTDdiMuon;
            if(parameterName == "triggerBitFixedTarget") ifs >> hex >> triggerBitFixedTarget;
            if(parameterName == "triggerBitFixedTargetMonitor") ifs >> hex >> triggerBitFixedTargetMonitor;
            if(parameterName == "triggerBitBesMonitor") ifs >> hex >> triggerBitBesMonitor;
            if(parameterName == "triggerBitHLTGood2") ifs >> hex >> triggerBitHLTGood2;
            if(parameterName == "daqId_UPC") ifs >> hex >> daqId_UPC;
            if(parameterName == "daqId_mtd2hits") ifs >> hex >> daqId_mtd2hits;

            if(parameterName == "nHitsCut_highPt") ifs >> dec >> nHitsCut_highPt;
            if(parameterName == "ptCut_highPt") ifs >> ptCut_highPt;
            if(parameterName == "nHitsCut_heavyFragment") ifs >> nHitsCut_heavyFragment;
            if(parameterName == "nDedxCut_heavyFragment") ifs >> nDedxCut_heavyFragment;
            if(parameterName == "useTofMatchedGlobalTrack_heavyfragMent") ifs >> useTofMatchedGlobalTrack_heavyfragMent;
            if(parameterName == "nSigmaDedxHe3Cut_heavyFragment") ifs >> nSigmaDedxHe3Cut_heavyFragment;
            if(parameterName == "nSigmaDedxTritonCut_heavyFragment") ifs >> nSigmaDedxTritonCut_heavyFragment;
            if(parameterName == "nSigmaMassTritonCut_heavyFragment") ifs >> nSigmaMassTritonCut_heavyFragment;
            if(parameterName == "dcaCut_heavyFragment") ifs >> dcaCut_heavyFragment;

            if(parameterName == "nHitsCut_MTDdiMuon") ifs >> nHitsCut_MTDdiMuon;
            if(parameterName == "nDedxCut_MTDdiMuon") ifs >> nDedxCut_MTDdiMuon;
            if(parameterName == "nSigmaPionLow_MTDdiMuon") ifs >> nSigmaPionLow_MTDdiMuon;
            if(parameterName == "nSigmaPionHigh_MTDdiMuon") ifs >> nSigmaPionHigh_MTDdiMuon;
            if(parameterName == "dcaCut_MTDdiMuon") ifs >> dcaCut_MTDdiMuon;
            if(parameterName == "ptCutLow_MTDdiMuon") ifs >> ptCutLow_MTDdiMuon;
            if(parameterName == "ptCutHigh_MTDdiMuon") ifs >> ptCutHigh_MTDdiMuon;
            if(parameterName == "dzTrkHit_MTDdiMuon") ifs >> dzTrkHit_MTDdiMuon;

            if(parameterName == "nHitsCut_MTDQuarkonium") ifs >> nHitsCut_MTDQuarkonium;
            if(parameterName == "dzTrkHit_MTDQuarkonium") ifs >> dzTrkHit_MTDQuarkonium;
            if(parameterName == "dcaCut_MTDQuarkonium") ifs >> dcaCut_MTDQuarkonium;
            if(parameterName == "ptLead_MTDQuarkonium") ifs >> ptLead_MTDQuarkonium;
            if(parameterName == "ptSublead_MTDQuarkonium") ifs >> ptSublead_MTDQuarkonium;

            if(parameterName == "nHitsCut_diElectron") ifs >> nHitsCut_diElectron;
            if(parameterName == "nDedxCut_diElectron") ifs >> nDedxCut_diElectron;
            if(parameterName == "towerEnergyCut_diElectron") ifs >> towerEnergyCut_diElectron;
            if(parameterName == "pCutLowForTof_diElectron") ifs >> pCutLowForTof_diElectron;
            if(parameterName == "pCutHighForTof_diElectron") ifs >> pCutHighForTof_diElectron;
            if(parameterName == "pCutLowForEmc_diElectron") ifs >> pCutLowForEmc_diElectron;
            if(parameterName == "pCutHighForEmc_diElectron") ifs >> pCutHighForEmc_diElectron;
            if(parameterName == "pCutLowForTofAndEmc_diElectron") ifs >> pCutLowForTofAndEmc_diElectron;
            if(parameterName == "pCutHighForTofAndEmc_diElectron") ifs >> pCutHighForTofAndEmc_diElectron;
            if(parameterName == "nSigmaDedxCutLowForTof_diElectron") ifs >> nSigmaDedxCutLowForTof_diElectron;
            if(parameterName == "nSigmaDedxCutHighForTof_diElectron") ifs >> nSigmaDedxCutHighForTof_diElectron;
            if(parameterName == "nSigmaDedxCutLowForEmc_diElectron") ifs >> nSigmaDedxCutLowForEmc_diElectron;
            if(parameterName == "nSigmaDedxCutHighForEmc_diElectron") ifs >> nSigmaDedxCutHighForEmc_diElectron;
            if(parameterName == "nSigmaDedxCutLowForTofAndEmc_diElectron") ifs >> nSigmaDedxCutLowForTofAndEmc_diElectron;
            if(parameterName == "nSigmaDedxCutHighForTofAndEmc_diElectron") ifs >> nSigmaDedxCutHighForTofAndEmc_diElectron;
            if(parameterName == "p1Cut_diElectron") ifs >> p1Cut_diElectron;
            if(parameterName == "p2Cut_diElectron") ifs >> p2Cut_diElectron;
            if(parameterName == "pOverECutLow_diElectron") ifs >> pOverECutLow_diElectron;
            if(parameterName == "pOverECutHigh_diElectron") ifs >> pOverECutHigh_diElectron;
            if(parameterName == "invariantMassCutLow_diElectron") ifs >> invariantMassCutLow_diElectron;
            if(parameterName == "invariantMassCutHigh_diElectron") ifs >> invariantMassCutHigh_diElectron;
            if(parameterName == "vertexZDiffCut_diElectron") ifs >> vertexZDiffCut_diElectron;
            if(parameterName == "oneOverBetaCut_diElectron") ifs >> oneOverBetaCut_diElectron;
            if(parameterName == "onlyOppPairs_diElectron") ifs >> onlyOppPairs_diElectron;
            if(parameterName == "nHitsCut_matchedHT2") ifs >> nHitsCut_matchedHT2;
            if(parameterName == "dZCut_matchedHT2") ifs >> dZCut_matchedHT2;
            if(parameterName == "dPhiCut_matchedHT2") ifs >> dPhiCut_matchedHT2;
            if(parameterName == "PCut_matchedHT2") ifs >> PCut_matchedHT2;

            if(parameterName == "sampleScale_randomEvents") ifs >> sampleScale_randomEvents;

            if(parameterName == "nTracksCut_besGoodEvents") ifs >> nTracksCut_besGoodEvents;
            if(parameterName == "vertexRCut_besGoodEvents") ifs >> vertexRCut_besGoodEvents;
            if(parameterName == "vertexZCut_besGoodEvents") ifs >> vertexZCut_besGoodEvents;

            //UPC pair(rho trigger)
            if(parameterName == "nTracksCut_UPC") ifs >> nTracksCut_UPC;
            if(parameterName == "vertexRCut_UPC") ifs >> vertexRCut_UPC;
            if(parameterName == "vertexZCut_UPC") ifs >> vertexZCut_UPC;

            if(parameterName == "nTracksCut_besMonitor") ifs >> nTracksCut_besMonitor;
            if(parameterName == "vertexZCut_besMonitor") ifs >> vertexZCut_besMonitor;

            //UPC diElectron
            if(parameterName == "nTracksCut_UPCdiElectron") ifs >> nTracksCut_UPCdiElectron;
            if(parameterName == "nHitsCut_UPCdiElectron") ifs >> nHitsCut_UPCdiElectron;
            if(parameterName == "nDedxCut_UPCdiElectron") ifs >> nDedxCut_UPCdiElectron;
            if(parameterName == "towerEnergyCut_UPCdiElectron") ifs >> towerEnergyCut_UPCdiElectron;
            if(parameterName == "pCutLowForTof_UPCdiElectron") ifs >> pCutLowForTof_UPCdiElectron;
            if(parameterName == "pCutHighForTof_UPCdiElectron") ifs >> pCutHighForTof_UPCdiElectron;
            if(parameterName == "pCutLowForEmc_UPCdiElectron") ifs >> pCutLowForEmc_UPCdiElectron;
            if(parameterName == "pCutHighForEmc_UPCdiElectron") ifs >> pCutHighForEmc_UPCdiElectron;
            if(parameterName == "pCutLowForTofAndEmc_UPCdiElectron") ifs >> pCutLowForTofAndEmc_UPCdiElectron;
            if(parameterName == "pCutHighForTofAndEmc_UPCdiElectron") ifs >> pCutHighForTofAndEmc_UPCdiElectron;
            if(parameterName == "nSigmaDedxCutLowForTof_UPCdiElectron") ifs >> nSigmaDedxCutLowForTof_UPCdiElectron;
            if(parameterName == "nSigmaDedxCutHighForTof_UPCdiElectron") ifs >> nSigmaDedxCutHighForTof_UPCdiElectron;
            if(parameterName == "nSigmaDedxCutLowForEmc_UPCdiElectron") ifs >> nSigmaDedxCutLowForEmc_UPCdiElectron;
            if(parameterName == "nSigmaDedxCutHighForEmc_UPCdiElectron") ifs >> nSigmaDedxCutHighForEmc_UPCdiElectron;
            if(parameterName == "nSigmaDedxCutLowForTofAndEmc_UPCdiElectron") ifs >> nSigmaDedxCutLowForTofAndEmc_UPCdiElectron;
            if(parameterName == "nSigmaDedxCutHighForTofAndEmc_UPCdiElectron") ifs >> nSigmaDedxCutHighForTofAndEmc_UPCdiElectron;
            if(parameterName == "p1Cut_UPCdiElectron") ifs >> p1Cut_UPCdiElectron;
            if(parameterName == "p2Cut_UPCdiElectron") ifs >> p2Cut_UPCdiElectron;
            if(parameterName == "pOverECutLow_UPCdiElectron") ifs >> pOverECutLow_UPCdiElectron;
            if(parameterName == "pOverECutHigh_UPCdiElectron") ifs >> pOverECutHigh_UPCdiElectron;
            if(parameterName == "invariantMassCutLow_UPCdiElectron") ifs >> invariantMassCutLow_UPCdiElectron;
            if(parameterName == "invariantMassCutHigh_UPCdiElectron") ifs >> invariantMassCutHigh_UPCdiElectron;
            if(parameterName == "vertexZDiffCut_UPCdiElectron") ifs >> vertexZDiffCut_UPCdiElectron;
            if(parameterName == "oneOverBetaCut_UPCdiElectron") ifs >> oneOverBetaCut_UPCdiElectron;
            if(parameterName == "onlyOppPairs_UPCdiElectron") ifs >> onlyOppPairs_UPCdiElectron;

            if(parameterName == "nTracksLow_LowMult") ifs >> nTracksLow_LowMult;
            if(parameterName == "nTracksHigh_LowMult") ifs >> nTracksHigh_LowMult;

            if(parameterName == "vertexYCutLow_FixedTarget") ifs >> vertexYCutLow_FixedTarget;
            if(parameterName == "vertexYCutHigh_FixedTarget") ifs >> vertexYCutHigh_FixedTarget;
            if(parameterName == "vertexZCutLow_FixedTarget") ifs >> vertexZCutLow_FixedTarget;
            if(parameterName == "vertexZCutHigh_FixedTarget") ifs >> vertexZCutHigh_FixedTarget;
            if(parameterName == "vertexRCutLow_FixedTarget") ifs >> vertexRCutLow_FixedTarget;
            if(parameterName == "vertexRCutHigh_FixedTarget") ifs >> vertexRCutHigh_FixedTarget;

            if(parameterName == "vertexZCutLow_FixedTargetMonitor") ifs >> vertexZCutLow_FixedTargetMonitor;
            if(parameterName == "vertexZCutHigh_FixedTargetMonitor") ifs >> vertexZCutHigh_FixedTargetMonitor;

            if(parameterName == "nTracksCut_HLTGood2") ifs >> nTracksCut_HLTGood2;
            if(parameterName == "vertexRCut_HLTGood2") ifs >> vertexRCut_HLTGood2;
            if(parameterName == "vertexZCut_HLTGood2") ifs >> vertexZCut_HLTGood2;

            if(parameterName == "Debug_allTracks") ifs >> Debug_allTracks;
            if(parameterName == "Debug_tracks") ifs >> Debug_tracks;
            if(parameterName == "Debug_towers") ifs >> Debug_towers;
            if(parameterName == "Debug_matchs") ifs >> Debug_matchs;
            if(parameterName == "Debug_vertex") ifs >> Debug_vertex;
            if(parameterName == "nHitsCut_debug") ifs >> nHitsCut_debug;
            if(parameterName == "nDedxCut_debug") ifs >> nDedxCut_debug;
            if(parameterName == "dedxCutLow_debug") ifs >> dedxCutLow_debug;
            if(parameterName == "dedxCutHigh_debug") ifs >> dedxCutHigh_debug;
            if(parameterName == "pCut_debug") ifs >> pCut_debug;
            if(parameterName == "towerEnergyCut_debug") ifs >> towerEnergyCut_debug;
            if(parameterName == "matchPhiDiffCut_debug") ifs >> matchPhiDiffCut_debug;
            if(parameterName == "matchZEdgeCut_debug") ifs >> matchZEdgeCut_debug;
            //  if(parameterName == "innerSectorGain") ifs>>innerSectorGain;
            //  if(parameterName == "outerSectorGain") ifs>>outerSectorGain;
            if(parameterName == "spaceChargeP0") ifs >> spaceChargeP0;
            if(parameterName == "spaceChargeP1") ifs >> spaceChargeP1;
            if(parameterName == "xVertex") ifs >> paraXVertex;
            if(parameterName == "yVertex") ifs >> paraYVertex;
            if(parameterName == "sigmaDedx1") ifs >> sigmaDedx1;
            if(parameterName == "dPOverP2") ifs >> dPOverP2;
            if(parameterName == "sigmaTof") ifs >> sigmaTof;
        }
}

void gl3TriggerDecider::resetEvent() {
    highPtTriggered         = 0;
    diElectronTriggered     = 0;
    heavyFragmentTriggered  = 0;
    allEventsTriggered      = 0;
    randomEventsTriggered   = 0;
    besGoodEventsTriggered  = 0;
    matchedHT2Triggered     = 0;
    LowMultTriggered        = 0;
    UPCTriggered            = 0;
    UPCdiElectronTriggered  = 0;
    MTDdiMuonTriggered      = 0;
    HLTGood2Triggered       = 0;
    diElectron2TwrTriggered = 0;
    
    hltDecision = 0;

    hlt_hiPt.nHighPt          = 0;
    hlt_diEP.nEPairs          = 0;
    hlt_hF.nHeavyFragments    = 0;
    hlt_hT2.nPairs            = 0;
    hlt_hT2.triggered         = false;
    hlt_lm.nPrimaryTracks     = -999;
    hlt_UPCrho.nRhos          = 0;
    hlt_UPCdiEP.nEPairs       = 0;
    hlt_MTDQkn.nMTDQuarkonium = 0;
    hlt_diEP2Twr.nEPairs      = 0;
}

int gl3TriggerDecider::decide(int eventId)
{
    if(f1)  fprintf(f1, "event:  %i\n", eventId);

    // highPtTriggered        = 0;
    // diElectronTriggered    = 0;
    // heavyFragmentTriggered = 0;
    // allEventsTriggered     = 0;
    // randomEventsTriggered  = 0;
    // besGoodEventsTriggered = 0;
    // matchedHT2Triggered    = 0;
    // LowMultTriggered       = 0;
    // UPCTriggered           = 0;
    // UPCdiElectronTriggered = 0;
    // MTDdiMuonTriggered     = 0;

    // hlt_hiPt.nHighPt       = 0;
    // hlt_diEP.nEPairs       = 0;
    // hlt_hF.nHeavyFragments = 0;
    // hlt_hT2.nPairs         = 0;
    // hlt_hT2.triggered      = false;
    // hlt_lm.nPrimaryTracks  = -999;
    // hlt_UPCrho.nRhos       = 0;
    // hlt_UPCdiEP.nEPairs    = 0;

    int nTracks = event->getNGlobalTracks();
    int nPTracks = event->getNPrimaryTracks();

    //  LOG(TERR,"GL3: %d tracks",nTracks) ;

    if(triggerOnHighPt) {
        for(int i = 0; i < nTracks; i++) {
            gl3Node* node = event->getNode(i);
            gl3Track* pTrack = event->getNode(i)->primaryTrack;
            if(!pTrack) continue;

            //  LOG(TERR,"hight Pt: %d/%d : %d vs %d, %f vs %f",i,nTracks,
            //      pTrack->nHits,nHitsCut_highPt,
            //      pTrack->pt, ptCut_highPt) ;

            if(pTrack->nHits < nHitsCut_highPt) continue;
            if(pTrack->pt < ptCut_highPt) continue;

            highPtTriggered = triggerBitHighPt;

            if(!node->primarytofCell && node->primaryTrack)
                event->tof->matchPrimaryTracks(node, event->getLmVertex());
            double matchTowerEnergy = 0.;
            for(int j = 0; j < event->emc->getNBarrelTowers(); j++) {
                gl3EmcTower* tower = event->emc->getBarrelTower(j);
                double phiDiff, zEdge;
                if(!tower->matchTrack(pTrack, phiDiff, zEdge)) continue;
                if(tower->getEnergy() < matchTowerEnergy) continue;
                matchTowerEnergy = tower->getEnergy();
                node->emcTower = tower;
                node->emcMatchPhiDiff = phiDiff;
                node->emcMatchZEdge = zEdge;
            }

            hlt_hiPt.highPtNodeSN[hlt_hiPt.nHighPt] = i;
            hlt_hiPt.nHighPt ++;
        }
    }

    if(triggerOnHeavyFragment) {
        for(int i = 0; i < nTracks; i++) {
            int triggered = 0;
            gl3Node* node = event->getNode(i);
            gl3Track* gTrack = event->getGlobalTrack(i);
            gl3Track* pTrack = node->primaryTrack;
            if(gTrack->nHits < nHitsCut_heavyFragment) continue;
            if(gTrack->nDedx < nDedxCut_heavyFragment) continue;

            if(useTofMatchedGlobalTrack_heavyfragMent && !node->globaltofCell) continue;   ///< require tof match for global tracks

            double x0 = gTrack->r0 * cos(gTrack->phi0);
            double y0 = gTrack->r0 * sin(gTrack->phi0);
            double dca2 = sqrt(pow(x0 - event->beamlineX, 2) + pow(y0 - event->beamlineY, 2));

            double dcaToUse = 0;
            if(event->useBeamlineToMakePrimarys) dcaToUse = dca2 ; ///< using 2D dca in pp 500GeV
            else dcaToUse = gTrack->dca ;  ///< using 3D dca in AuAu 200 GeV

            if(dcaToUse > dcaCut_heavyFragment || dcaToUse < 0) continue;

            if(gTrack->nSigmaDedx(event->bischel, "He3", sigmaDedx1) > nSigmaDedxHe3Cut_heavyFragment) {
                triggered = 1;
                if(!node->primarytofCell && node->primaryTrack)
                    event->tof->matchPrimaryTracks(node, event->getLmVertex());
                double matchTowerEnergy = 0.;
                for(int i = 0; i < event->emc->getNBarrelTowers(); i++) {
                    gl3EmcTower* tower = event->emc->getBarrelTower(i);
                    double phiDiff, zEdge;
                    if(!tower->matchTrack(gTrack, phiDiff, zEdge)) continue;
                    if(tower->getEnergy() < matchTowerEnergy) continue;
                    matchTowerEnergy = tower->getEnergy();
                    node->emcTower = tower;
                    node->emcMatchPhiDiff = phiDiff;
                    node->emcMatchZEdge = zEdge;
                }
            }

            if(fabs(gTrack->nSigmaDedx(event->bischel, "Triton", sigmaDedx1)) < nSigmaDedxTritonCut_heavyFragment) {
                if(!node->primarytofCell && node->primaryTrack)
                    event->tof->matchPrimaryTracks(node, event->getLmVertex());
                if(node->primarytofCell) {
                    //      double c = 29.979; //cm/ns
                    double p = pTrack->pt * sqrt(1. + pow(pTrack->tanl, 2));
                    double beta = node->beta;
                    double tof = node->tof;
                    double l = beta * tof;
                    double tritonMass = 2.80925;
                    double sigmaMass1 = pow(p, 2) * 2. / l * sqrt(1. + pow(tritonMass / p, 2)) * sigmaTof;
                    double sigmaMass2 = pow(tritonMass, 2) * 2.*p * dPOverP2;
                    double sigmaMass = sqrt(pow(sigmaMass1, 2) + pow(sigmaMass2, 2));
                    //LOG(INFO,"sigmaMass1:%f, sigmaMass2:%f, sigmaMass:%f,p:%f,tof:%f",sigmaMass1,sigmaMass2,sigmaMass,p,tof);
                    double m2 = pow(p, 2) * (pow(1. / beta, 2) - 1);
                    if(fabs(m2 - pow(tritonMass, 2)) < nSigmaMassTritonCut_heavyFragment * sigmaMass) {
                        triggered = 1;
                        double matchTowerEnergy = 0.;
                        for(int i = 0; i < event->emc->getNBarrelTowers(); i++) {
                            gl3EmcTower* tower = event->emc->getBarrelTower(i);
                            double phiDiff, zEdge;
                            if(!tower->matchTrack(gTrack, phiDiff, zEdge)) continue;
                            if(tower->getEnergy() < matchTowerEnergy) continue;
                            matchTowerEnergy = tower->getEnergy();
                            node->emcTower = tower;
                            node->emcMatchPhiDiff = phiDiff;
                            node->emcMatchZEdge = zEdge;
                        }
                    }

                    //      if(mass>tritonMass-nSigmaMassTritonCut_heavyFragment*sigmaMass)
                    //        cout<<"triton trigger: "<<gTrack->nHits<<"  "<<gTrack->nDedx<<"  "<<gTrack->pt*sqrt(1.+pow(gTrack->tanl,2))<<"  "<<gTrack->dedx<<"  "<<gTrack->nSigmaDedx(event->bischel, "Triton", sigmaDedx1)<<"  "<<mass<<"+-"<<sigmaMass<<endl;
                }
            }
            if(triggered) {
                heavyFragmentTriggered = triggerBitHeavyFragment;
                hlt_hF.heavyFragmentSN[hlt_hF.nHeavyFragments] = i;
                hlt_hF.nHeavyFragments ++;

            }
        }
    }

    // mtd2hits monitor
    if(triggerOnMTDdiMuon && (event->trigger & daqId_mtd2hits)) {
        // //cout<<"trigger for MTD 0x"<<hex<<event->trigger<<endl;
        // int nMuon = 0;
        // const double muonMass = 0.106; // GeV/c^2
        // const double twopi = 6.2831854;
        // int t1 = -999, t2 = -999;
        // double _pt1 = 0, _pt2 = 0;
        // gl3Node* muonNode[maxNMuonTracks];
        // for(int i = 0; i < nTracks; i++) {
        //     gl3Node* node = event->getNode(i);
        //     gl3Track* track = node->primaryTrack;
        //     if(!track) continue;
        //     if(track->nHits < nHitsCut_MTDdiMuon) continue;
        //     double pt = track->pt;
        //     if(pt < ptCutLow_MTDdiMuon) continue;
        //     double phi = track->psi > 0. ? track->psi : track->psi + twopi;
        //     if(phi<0.4 && phi>1.2) continue;
        //     if(fabs(track->eta) > 0.5) continue;
        //     muonNode[nMuon] = node;
        //     if(pt > _pt1) {
        //         _pt2 = _pt1;
        //         t2 = t1;
        //         _pt1 = pt ;
        //         t1 = i ;
        //     } else if(pt > _pt2) {
        //         _pt2 = pt;
        //         t2 = i;
        //     }
        //     nMuon++;
        // }
        // if(nMuon >= 2) {
        //     //cout<<"t1="<<t1<<"t2="<<t2<<endl;
        //     MTDdiMuonTriggered = triggerBitMTDdiMuon;
        //     gl3Node* node1 = event->getNode(t1);
        //     gl3Node* node2 = event->getNode(t2);
        //     gl3Track* track1 = node1->primaryTrack;
        //     gl3Track* track2 = node2->primaryTrack;
        //     double p1 = track1->pt * sqrt(1. + pow(track1->tanl, 2));
        //     double pt1 = track1->pt;
        //     double px1 = pt1 * cos(track1->psi);
        //     double py1 = pt1 * sin(track1->psi);
        //     double pz1 = pt1 * track1->tanl;
        //     double E1 = sqrt(muonMass * muonMass + p1 * p1);
        //     double p2 = track2->pt * sqrt(1. + pow(track2->tanl, 2));
        //     double pt2 = track2->pt;
        //     double px2 = pt2 * cos(track2->psi);
        //     double py2 = pt2 * sin(track2->psi);
        //     double pz2 = pt2 * track2->tanl;
        //     double E2 = sqrt(muonMass * muonMass + p2 * p2);
        //     double px = px1 + px2;
        //     double py = py1 + py2;
        //     double pz = pz1 + pz2;
        //     double psi = atan2(py, px);
        //     double pt = sqrt(pow(px, 2) + pow(py, 2));
        //     double tanl = atan2(pz, pt);
        //     double invariantMass = sqrt(pow(E1 + E2, 2) - pow(pt, 2) - pow(pz, 2));

        //     hlt_diElectronPair diMuonPair;
        //     diMuonPair.dau1NodeSN = (int)(node1 - event->getNode(0));
        //     diMuonPair.dau2NodeSN = (int)(node2 - event->getNode(0));
        //     diMuonPair.invariantMass = invariantMass;
        //     diMuonPair.pt = pt;
        //     diMuonPair.psi = psi;
        //     diMuonPair.tanl = tanl;
        //     diMuonPair.dau1SelectionBit = 1;
        //     diMuonPair.dau2SelectionBit = 1;
        // }
        // //if(MTDdiMuonTriggered) cout<<"MuonPair"<<endl;
    }

    if(triggerOnDiElectron) {
        const double electronMass = 0.511e-3;
        int nElectronNodes = 0, nTowers = 0;
        gl3Node* electronNode[maxNElectronTracks];
        gl3EmcTower* emcTower[maxNTowers];
        int electronSelectionBits[maxNElectronTracks];
        int selectionBitTof = 0x1;
        int selectionBitEmc = 0x2;
        int selectionBitTofAndEmc = 0x4;

        // cout << "Test" << endl;

        for(int i = 0; i < event->emc->getNBarrelTowers(); i++) {
            gl3EmcTower* tower = event->emc->getBarrelTower(i);
            double towerEnergy = tower->getEnergy();
            if(towerEnergy < 0.5) continue;
            if(nTowers >= maxNTowers) {
                cout << "gl3TriggerDecider::decide()  WARN! towerArray Full!" << endl;
                break;
            }
            emcTower[nTowers] = tower;
            nTowers ++;
        }

        for(int i = 0; i < nTracks; i++) {
            int isElectron = 0;
            int passTofCut = 0;
            int passEmcCut = 0;
            gl3Node* node = event->getNode(i);
            gl3Track* track = node->primaryTrack;
            if(!track) continue;
            if(track->nHits < nHitsCut_diElectron) continue;
            if(track->nDedx < nDedxCut_diElectron) continue;
            double p = track->pt * sqrt(1. + pow(track->tanl, 2));
            if(p < p2Cut_diElectron) continue;
            double nSigmaDedxE = track->nSigmaDedx(event->bischel, "Electron", sigmaDedx1);
            if(nSigmaDedxE > min(nSigmaDedxCutLowForTof_diElectron, nSigmaDedxCutLowForTofAndEmc_diElectron)
               && nSigmaDedxE < max(nSigmaDedxCutHighForTof_diElectron, nSigmaDedxCutHighForTofAndEmc_diElectron)
                                && p > min(pCutLowForTof_diElectron, pCutLowForTofAndEmc_diElectron)
                && p < max(pCutHighForTof_diElectron, pCutHighForTofAndEmc_diElectron)) {
                if(!node->primarytofCell && node->primaryTrack)
                    event->tof->matchPrimaryTracks(node, event->getLmVertex());
                if(fabs(1. / node->beta - 1.) < oneOverBetaCut_diElectron)
                    passTofCut = 1;
            }

            if(nSigmaDedxE > min(nSigmaDedxCutLowForEmc_diElectron, nSigmaDedxCutLowForTofAndEmc_diElectron)
               && nSigmaDedxE < max(nSigmaDedxCutHighForEmc_diElectron, nSigmaDedxCutHighForTofAndEmc_diElectron)
                                && p > min(pCutLowForEmc_diElectron, pCutLowForTofAndEmc_diElectron)
            && p < max(pCutHighForEmc_diElectron, pCutHighForTofAndEmc_diElectron)) {
                double matchTowerEnergy = 0.;
                for(int j = 0; j < nTowers; j++) {
                    gl3EmcTower* tower = emcTower[j];
                    double phiDiff, zEdge;
                    if(!tower->matchTrack(track, phiDiff, zEdge)) continue;
                    if(tower->getEnergy() < matchTowerEnergy) continue;
                    matchTowerEnergy = tower->getEnergy();
                    node->emcTower = tower;
                    node->emcMatchPhiDiff = phiDiff;
                    node->emcMatchZEdge = zEdge;
                }
                if(matchTowerEnergy > towerEnergyCut_diElectron &&
                   p / matchTowerEnergy > pOverECutLow_diElectron &&
                   p / matchTowerEnergy < pOverECutHigh_diElectron)
                    passEmcCut = 1;
            }
            if(nSigmaDedxE > nSigmaDedxCutLowForTof_diElectron
               && nSigmaDedxE < nSigmaDedxCutHighForTof_diElectron
                                && p > pCutLowForTof_diElectron
               && p < pCutHighForTof_diElectron && passTofCut)
                isElectron |= selectionBitTof;
            if(nSigmaDedxE > nSigmaDedxCutLowForEmc_diElectron
               && nSigmaDedxE < nSigmaDedxCutHighForEmc_diElectron
                                && p > pCutLowForEmc_diElectron
               && p < pCutHighForEmc_diElectron && passEmcCut)
                isElectron |= selectionBitEmc;
            if(nSigmaDedxE > nSigmaDedxCutLowForTofAndEmc_diElectron
               && nSigmaDedxE < nSigmaDedxCutHighForTofAndEmc_diElectron
                                && p > pCutLowForTofAndEmc_diElectron
               && p < pCutHighForTofAndEmc_diElectron
               && passTofCut && passEmcCut)
                isElectron |= selectionBitTofAndEmc;

            if(isElectron) {
                if(nElectronNodes >= maxNElectronTracks) {
                    cout << "gl3TriggerDecider::decide()  WARN! electronTrackArray Full!" << endl;
                    break;
                }
                electronNode[nElectronNodes] = node;
                electronSelectionBits[nElectronNodes] = isElectron;
                nElectronNodes ++;
            }
}

    for(int i = 0; i < nElectronNodes; i++) {
        for(int j = i + 1; j < nElectronNodes; j++) {
            int i1 = i;
            int i2 = j;

            gl3Node* node1 = electronNode[i];
            gl3Track* track1 = node1->primaryTrack;
            double p1 = track1->pt * sqrt(1. + pow(track1->tanl, 2));
            gl3Node* node2 = electronNode[j];
            gl3Track* track2 = node2->primaryTrack;
            double p2 = track2->pt * sqrt(1. + pow(track2->tanl, 2));
            if(p1 < p2) {
                swap(p1, p2);
                swap(i1, i2);
                swap(node1, node2);
                swap(track1, track2);
            }
            if(p1 < p1Cut_diElectron) continue;
            if(track1->q * track2->q > 0 && onlyOppPairs_diElectron) continue;
            if(fabs(track1->z0 - track2->z0) > vertexZDiffCut_diElectron) continue;

            double pt1 = track1->pt;
            double px1 = pt1 * cos(track1->psi);
            double py1 = pt1 * sin(track1->psi);
            double pz1 = pt1 * track1->tanl;
            double E1 = sqrt(electronMass * electronMass + p1 * p1);
            double pt2 = track2->pt;
            double px2 = pt2 * cos(track2->psi);
            double py2 = pt2 * sin(track2->psi);
            double pz2 = pt2 * track2->tanl;
            double E2 = sqrt(electronMass * electronMass + p2 * p2);
            double px = px1 + px2;
            double py = py1 + py2;
            double pz = pz1 + pz2;
            double psi = atan2(py, px);
            double pt = sqrt(pow(px, 2) + pow(py, 2));
            double tanl = atan2(pz, pt);
            double invariantMass = sqrt(pow(E1 + E2, 2) - pow(pt, 2) - pow(pz, 2));
            //          cout<<"px1:"<<px1<<"  py1:"<<py1<<"  pz1:"<<pz1<<"  E1:"<<E1<<"  px2:"<<px2<<"  py2:"<<py2<<"  pz2:"<<pz2<<"  E2:"<<E2<<"  invarMass:"<<invariantMass<<endl;
            //          cout<<"p1:"<<p1<<"  E1:"<<node1->emcTower<<" beta1:"<<node1->beta<<"  p2:"<<p2<<"  E2:"<<node2->emcTower<<" beta2:"<<node2->beta<<"  invarMass:"<<invariantMass<<endl;
            if(invariantMass < invariantMassCutLow_diElectron) continue;
            if(invariantMass > invariantMassCutHigh_diElectron) continue;

            diElectronTriggered = triggerBitDiElectron;

            hlt_diElectronPair diElePair;
            diElePair.dau1NodeSN = (int)(node1 - event->getNode(0));
            diElePair.dau2NodeSN = (int)(node2 - event->getNode(0));
            diElePair.invariantMass = invariantMass;
            diElePair.pt = pt;
            diElePair.psi = psi;
            diElePair.tanl = tanl;
            diElePair.dau1SelectionBit = electronSelectionBits[i1];
            diElePair.dau2SelectionBit = electronSelectionBits[i2];
            hlt_diEP.ePair[hlt_diEP.nEPairs] = diElePair;
            hlt_diEP.nEPairs ++;

            /*
              l3EmcTowerInfo* towerInfo1 = 0;
              if(matchTower[i1]) towerInfo1 = matchTower[i1]->getTowerInfo();
              l3EmcTowerInfo* towerInfo2 = 0;
              if(matchTower[i2]) towerInfo2 = matchTower[i2]->getTowerInfo();
              if (f1){ //write out QA messages
              fprintf(f1, "pair:  \n");
              fprintf(f1, "%e  ", invariantMass);
              if(matchTower[i1])
              fprintf(f1, "%i  %i  %i  %i  %i  %i  %i  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %i  %i  %i  %i  %e  %e  %e  %e  %e  %e  %e  %e  %e  %i  %e  %e  %e  ", track1->id, track1->flag, track1->innerMostRow, track1->outerMostRow, track1->nHits, track1->nDedx, track1->q, track1->chi2[0], track1->chi2[1], track1->dedx, track1->pt, track1->phi0, track1->psi, track1->r0, track1->tanl, track1->z0, track1->length, track1->dpt, track1->dpsi, track1->dz0, track1->eta, track1->dtanl, p1, px1, py1, pz1, towerInfo1->getDaqID(), towerInfo1->getSoftID(), towerInfo1->getCrate(), towerInfo1->getCrateSeq(), towerInfo1->getPhi(), towerInfo1->getEta(), towerInfo1->getEtaMin(), towerInfo1->getEtaMax(), towerInfo1->getZ(), towerInfo1->getZmin(), towerInfo1->getZmax(), towerInfo1->getGain(), towerInfo1->getPedestal(), matchTower[i1]->getADC(), matchTower[i1]->getEnergy(), matchPhiDiff[i1], matchZEdge[i1]);
              else
              fprintf(f1, "%i  %i  %i  %i  %i  %i  %i  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %i  %i  %i  %i  %e  %e  %e  %e  %e  %e  %e  %e  %e  %i  %e  %e  %e  ", track1->id, track1->flag, track1->innerMostRow, track1->outerMostRow, track1->nHits, track1->nDedx, track1->q, track1->chi2[0], track1->chi2[1], track1->dedx, track1->pt, track1->phi0, track1->psi, track1->r0, track1->tanl, track1->z0, track1->length, track1->dpt, track1->dpsi, track1->dz0, track1->eta, track1->dtanl, p1, px1, py1, pz1, 0, 0, 0, 0, 0., 0., 0., 0., 0., 0., 0., 0., 0., 0, 0., 0., 0.);
              if(matchTower[i2])
              fprintf(f1, "%i  %i  %i  %i  %i  %i  %i  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %i  %i  %i  %i  %e  %e  %e  %e  %e  %e  %e  %e  %e  %i  %e  %e  %e\n", track2->id, track2->flag, track2->innerMostRow, track2->outerMostRow, track2->nHits, track2->nDedx, track2->q, track2->chi2[0], track2->chi2[1], track2->dedx, track2->pt, track2->phi0, track2->psi, track2->r0, track2->tanl, track2->z0, track2->length, track2->dpt, track2->dpsi, track2->dz0, track2->eta, track2->dtanl, p2, px2, py2, pz2, towerInfo2->getDaqID(), towerInfo2->getSoftID(), towerInfo2->getCrate(), towerInfo2->getCrateSeq(), towerInfo2->getPhi(), towerInfo2->getEta(), towerInfo2->getEtaMin(), towerInfo2->getEtaMax(), towerInfo2->getZ(), towerInfo2->getZmin(), towerInfo2->getZmax(), towerInfo2->getGain(), towerInfo2->getPedestal(), matchTower[i2]->getADC(), matchTower[i2]->getEnergy(), matchPhiDiff[i2], matchZEdge[i2]);
              else
              fprintf(f1, "%i  %i  %i  %i  %i  %i  %i  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %i  %i  %i  %i  %e  %e  %e  %e  %e  %e  %e  %e  %e  %i  %e  %e  %e\n", track2->id, track2->flag, track2->innerMostRow, track2->outerMostRow, track2->nHits, track2->nDedx, track2->q, track2->chi2[0], track2->chi2[1], track2->dedx, track2->pt, track2->phi0, track2->psi, track2->r0, track2->tanl, track2->z0, track2->length, track2->dpt, track2->dpsi, track2->dz0, track2->eta, track2->dtanl, p2, px2, py2, pz2, 0, 0, 0, 0, 0., 0., 0., 0., 0., 0., 0., 0., 0., 0, 0., 0., 0.);
              }
            */
        }
    }
}


    if(triggerOnUPCpair && (event->trigger & daqId_UPC)) {
        const double pionMass = 0.139;
        gl3Node* UPCpionNode[maxNElectronTracks];
        int numberPionFirstPass = 0;
        float phiFirstLoop = 0.;


        if(fabs(event->getVertex().Getz()) < vertexZCut_UPC && sqrt(pow(event->getVertex().Getx(), 2) + pow(event->getVertex().Gety(), 2)) < vertexRCut_UPC && event->getNPrimaryTracks() < nTracksCut_UPC) {
            //cout<<"  RD we have a UPC trigger "<<" vertex z "<<event->getVertex().Getz()<<" trigger word "<<event->trigger<<endl;
            for(int i = 0; i < nTracks; i++) { // these tracks are global  Maybe we should only loop over primary tracks
                gl3Node* node = event->getNode(i);
                //      gl3Track* gTrack = node->globalTrack;
                //      if(!gTrack) continue;
                //      if(gTrack->nHits < 14) continue;
                //     LOG(TERR,"UPC global tracks: %d/%d : nHits  %d, pt %f  z0 %f",i,nTracks,
                //         gTrack->nHits, gTrack->pt, gTrack->z0) ;

                gl3Track* pTrack = node->primaryTrack;
                if(!pTrack) continue;
                //if(event->trigger==32768){
                //LOG(TERR,"UPC topo tracks: %d/%d : nHits  %d, pt %f z0 %f phi %f prim Trk %d",i,nTracks,
                //pTrack->nHits, pTrack->pt, pTrack->z0, pTrack->psi, prim) ;
                // cout<<" prim tof cell "<< node->primarytofCell<<" global tof cell "<<node->globaltofCell<<endl;
                //}

                if(pTrack->nHits < 14) continue;
                //  if(!node->primarytofCell) continue;  //rejects tracks without TOF (too strong throws all tracks)



                if(!node->primarytofCell && node->primaryTrack)
                    event->tof->matchPrimaryTracks(node, event->getLmVertex());
                if(node->primarytofCell) {
                    //this track should be saved in an array and then we loop over all good tracks to select a back-to-back track that may make a good invariant mass calculation
                    // check pid with dedx
                    //
                    double nSigmaDedxPion = pTrack->nSigmaDedx(event->bischel, "Pion", sigmaDedx1);

                    if(nSigmaDedxPion > 3.0 || nSigmaDedxPion < -3.0) continue;

                    //LOG(TERR,"UPC PION : %d/%d : nHits  %d, pt %f z0 %f phi %f sector %d",i,nTracks,
                    //       pTrack->nHits, pTrack->pt, pTrack->z0, pTrack->psi, pTrack->sector) ;
                    UPCpionNode[numberPionFirstPass] = node;
                    numberPionFirstPass++;
                }
            }
            //
            //if we have at least one track from first pass, we loop over all tracks
            //
            //cout<<" num select pion "<<numberPionFirstPass<<endl;
            if(numberPionFirstPass > 0) {
                for(int i = 0; i < numberPionFirstPass; i++) {
                    gl3Node* node1 = UPCpionNode[i];
                    gl3Track* pTrack1 = node1->primaryTrack;
                    phiFirstLoop = pTrack1->psi;
                    double pt0 = pTrack1->pt;
                    double px0 = pt0 * cos(pTrack1->psi);
                    double py0 = pt0 * sin(pTrack1->psi);
                    double pz0 = pt0 * pTrack1->tanl;
                    double E0 = sqrt(pionMass * pionMass + px0 * px0 + py0 * py0 + pz0 * pz0);

                    for(int j = 0; j < nTracks; j++) { //  loop over primary tracks
                        gl3Node* node2 = event->getNode(j);
                        gl3Track* pTrack2 = node2->primaryTrack;
                        if(!pTrack2) continue;
                        if(pTrack2->nHits < 14) continue;
                        //  if(!node->primarytofCell) continue;  //rejects tracks without TOF (too strong throws all tracks)
                        //
                        // lets look at delta phi with some decently wide window
                        //
                        double deltaPhi_UPC = phiFirstLoop - pTrack2->psi;

                        if(fabs(deltaPhi_UPC - 3.14) < 0.785) {


                            double pt2 = pTrack2->pt;
                            double px2 = pt2 * cos(pTrack2->psi);
                            double py2 = pt2 * sin(pTrack2->psi);
                            double pz2 = pt2 * pTrack2->tanl;
                            double E2 = sqrt(pionMass * pionMass + px2 * px2 + py2 * py2 + pz2 * pz2);
                            double px = px0 + px2;
                            double py = py0 + py2;
                            double pz = pz0 + pz2;
                            double psi = atan2(py, px);
                            double pt = sqrt(pow(px, 2) + pow(py, 2));
                            double tanl = atan2(pz, pt);
                            double invariantMass_UPC = sqrt(pow(E0 + E2, 2) - px * px - py * py - pz * pz);

                            if(invariantMass_UPC > 0.5 && invariantMass_UPC < 1.) {
                                //LOG(TERR,"select UPC : %d/%d : index1  %d, invMass %f phi 1 %f phi %f sector %d",j,nTracks,
                                //       i, invariantMass_UPC, phiFirstLoop, pTrack2->psi, pTrack2->sector) ;
                                UPCTriggered = triggerBitUPC;
                                //SavedeltaPhi_UPC = deltaPhi_UPC;
                                hlt_diPionPair diPionPair;
                                diPionPair.dau1NodeSN = (int)(node1 - event->getNode(0));
                                diPionPair.dau2NodeSN = (int)(node2 - event->getNode(0));
                                diPionPair.invariantMass = invariantMass_UPC;
                                diPionPair.pt = pt;
                                diPionPair.psi = psi;
                                diPionPair.tanl = tanl;
                                diPionPair.deltphi = deltaPhi_UPC;
                                hlt_UPCrho.PionPair[hlt_UPCrho.nRhos] = diPionPair;
                                hlt_UPCrho.nRhos++;
                                break;
                            }
                        }

                    } //inner loop over tracks
                } // outer loop over selected pions
            }   // if number pion >0
        }   // if vertex and # tracks conditions are satisfied
    }   // if(triggerOnUPCPair)


    //-----------------------------------------------------       UPCdiElectron
    //
    if(triggerOnUPCdiElectron &&
       (event->trigger & daqId_UPC) &&
       event->getNPrimaryTracks() <= nTracksCut_UPCdiElectron) {
        const double electronMass = 0.511e-3;
        int nUPCElectronNodes = 0, nTowers = 0;
        gl3Node* UPCelectronNode[maxNElectronTracks];
        gl3EmcTower* UPCemcTower[maxNTowers];
        int UPCelectronSelectionBits[maxNElectronTracks];
        int selectionBitTof = 0x1;
        int selectionBitEmc = 0x2;
        int selectionBitTofAndEmc = 0x4;


        // return... orz
        //if( event->getNPrimaryTracks()>nTracksCut_UPC) return 0;

        for(int i = 0; i < event->emc->getNBarrelTowers(); i++) {
            gl3EmcTower* tower = event->emc->getBarrelTower(i);
            double towerEnergy = tower->getEnergy();
            if(towerEnergy < 0.5) continue;
            //    cout<<"RD tower energy "<<towerEnergy<<endl;
            if(nTowers >= maxNTowers) {
                cout << "gl3TriggerDecider::decide()  WARN! towerArray Full!" << endl;
                break;
            }
            UPCemcTower[nTowers] = tower;
            nTowers ++;
        }

        for(int i = 0; i < nTracks; i++) {
            int isElectron = 0;
            int passTofCut = 0;
            int passEmcCut = 0;
            gl3Node* node = event->getNode(i);
            gl3Track* track = node->primaryTrack;

            if(!track) continue;
            if(track->nHits < nHitsCut_UPCdiElectron) continue;
            if(track->nDedx < nDedxCut_UPCdiElectron) continue;
            double p = track->pt * sqrt(1. + pow(track->tanl, 2));
            if(p < p2Cut_UPCdiElectron) continue;

            double nSigmaDedxE = track->nSigmaDedx(event->bischel, "Electron", sigmaDedx1);
            // cout<<"  track passed  --------- nsigma  "<<nSigmaDedxE<<endl;
#if 0
            if(nSigmaDedxE > min(nSigmaDedxCutLowForTof_UPCdiElectron, nSigmaDedxCutLowForTofAndEmc_UPCdiElectron)
               && nSigmaDedxE < max(nSigmaDedxCutHighForTof_UPCdiElectron, nSigmaDedxCutHighForTofAndEmc_UPCdiElectron)
                                && p > min(pCutLowForTof_UPCdiElectron, pCutLowForTofAndEmc_UPCdiElectron)
                && p < max(pCutHighForTof_UPCdiElectron, pCutHighForTofAndEmc_UPCdiElectron)) {

                if(!node->primarytofCell && node->primaryTrack)
                    event->tof->matchPrimaryTracks(node, event->getLmVertex());
                if(fabs(1. / node->beta - 1.) < oneOverBetaCut_UPCdiElectron)
                    passTofCut = 1;
            }
#endif
            //
            //RD we may not have TOF all the time set it always to TRUE
            //
            passTofCut = 1;

            if(nSigmaDedxE > min(nSigmaDedxCutLowForEmc_UPCdiElectron, nSigmaDedxCutLowForTofAndEmc_UPCdiElectron)
               && nSigmaDedxE < max(nSigmaDedxCutHighForEmc_UPCdiElectron, nSigmaDedxCutHighForTofAndEmc_UPCdiElectron)
                                && p > min(pCutLowForEmc_UPCdiElectron, pCutLowForTofAndEmc_UPCdiElectron)
            && p < max(pCutHighForEmc_UPCdiElectron, pCutHighForTofAndEmc_UPCdiElectron)) {
                double matchTowerEnergy = 0.;
                //        cout<<"got electron track ---------"<<endl;
                for(int j = 0; j < nTowers; j++) {
                    gl3EmcTower* tower = UPCemcTower[j];
                    double phiDiff, zEdge;
                    if(!tower->matchTrack(track, phiDiff, zEdge)) continue;
                    if(tower->getEnergy() < matchTowerEnergy) continue;
                    matchTowerEnergy = tower->getEnergy();
                    node->emcTower = tower;
                    node->emcMatchPhiDiff = phiDiff;
                    node->emcMatchZEdge = zEdge;
                }
                if(matchTowerEnergy > towerEnergyCut_diElectron &&
                   p / matchTowerEnergy > pOverECutLow_diElectron &&
                   p / matchTowerEnergy < pOverECutHigh_diElectron)
                    passEmcCut = 1;
            }
            if(nSigmaDedxE > nSigmaDedxCutLowForTof_UPCdiElectron
               && nSigmaDedxE < nSigmaDedxCutHighForTof_UPCdiElectron
                                && p > pCutLowForTof_UPCdiElectron
               && p < pCutHighForTof_UPCdiElectron && passTofCut)
                isElectron |= selectionBitTof;
            if(nSigmaDedxE > nSigmaDedxCutLowForEmc_UPCdiElectron
               && nSigmaDedxE < nSigmaDedxCutHighForEmc_UPCdiElectron
                                && p > pCutLowForEmc_UPCdiElectron
               && p < pCutHighForEmc_UPCdiElectron && passEmcCut)
                isElectron |= selectionBitEmc;
            if(nSigmaDedxE > nSigmaDedxCutLowForTofAndEmc_UPCdiElectron
               && nSigmaDedxE < nSigmaDedxCutHighForTofAndEmc_UPCdiElectron
                                && p > pCutLowForTofAndEmc_UPCdiElectron
               && p < pCutHighForTofAndEmc_UPCdiElectron
               && passTofCut && passEmcCut)
                isElectron |= selectionBitTofAndEmc;

            if(isElectron) {
                //       cout<<" Fully veted electron !!!!!!!!!!!!!!!! "<<endl;
                if(nUPCElectronNodes >= maxNElectronTracks) {
                    cout << "gl3TriggerDecider::decide()  WARN! electronTrackArray Full!" << endl;
                    break;
                }
                UPCelectronNode[nUPCElectronNodes] = node;
                UPCelectronSelectionBits[nUPCElectronNodes] = isElectron;
                nUPCElectronNodes ++;
            }
}
        //cout<<" UPC electron nodes in event +++++++++++++++++++ "<<nUPCElectronNodes<<endl;
    for(int i = 0; i < nUPCElectronNodes; i++) {
        for(int j = i + 1; j < nUPCElectronNodes; j++) {
            int i1 = i;
            int i2 = j;

            gl3Node* node1 = UPCelectronNode[i];
            gl3Track* track1 = node1->primaryTrack;
            double p1 = track1->pt * sqrt(1. + pow(track1->tanl, 2));
            gl3Node* node2 = UPCelectronNode[j];
            gl3Track* track2 = node2->primaryTrack;
            double p2 = track2->pt * sqrt(1. + pow(track2->tanl, 2));
            if(p1 < p2) {
                swap(p1, p2);
                swap(i1, i2);
                swap(node1, node2);
                swap(track1, track2);
            }
            if(p1 < p1Cut_diElectron) continue;
            if(track1->q * track2->q > 0 && onlyOppPairs_UPCdiElectron) continue;
            if(fabs(track1->z0 - track2->z0) > vertexZDiffCut_UPCdiElectron) continue;

            double pt1 = track1->pt;
            double px1 = pt1 * cos(track1->psi);
            double py1 = pt1 * sin(track1->psi);
            double pz1 = pt1 * track1->tanl;
            double E1 = sqrt(electronMass * electronMass + p1 * p1);
            double pt2 = track2->pt;
            double px2 = pt2 * cos(track2->psi);
            double py2 = pt2 * sin(track2->psi);
            double pz2 = pt2 * track2->tanl;
            double E2 = sqrt(electronMass * electronMass + p2 * p2);
            double px = px1 + px2;
            double py = py1 + py2;
            double pz = pz1 + pz2;
            double psi = atan2(py, px);
            double pt = sqrt(pow(px, 2) + pow(py, 2));
            double tanl = atan2(pz, pt);
            double invariantMass = sqrt(pow(E1 + E2, 2) - pow(pt, 2) - pow(pz, 2));
            //cout<<"px1:"<<px1<<"  py1:"<<py1<<"  pz1:"<<pz1<<"  E1:"<<E1<<"  px2:"<<px2<<"  py2:"<<py2<<"  pz2:"<<pz2<<"  E2:"<<E2<<"  invarMass:"<<invariantMass<<endl;
            //cout<<"pt:"<<pt<<" pz:"<<pz<<"  E1+E2: "<<E1+E2<<"   Esquared "<<pow(E1+E2,2)<<"  pt2+pz2 "<<pow(pt,2)+pow(pz,2)<<" E2-p2: "<<pow(E1+E2,2)-pow(pt,2)-pow(pz,2)<<"  invarMass:"<<invariantMass<<endl;
            if(invariantMass < invariantMassCutLow_UPCdiElectron) continue;
            if(invariantMass > invariantMassCutHigh_UPCdiElectron) continue;

            UPCdiElectronTriggered = triggerBitUPCdiElectron;

            hlt_diElectronPair UPCdiElePair;
            UPCdiElePair.dau1NodeSN = (int)(node1 - event->getNode(0));
            UPCdiElePair.dau2NodeSN = (int)(node2 - event->getNode(0));
            UPCdiElePair.invariantMass = invariantMass;
            UPCdiElePair.pt = pt;
            UPCdiElePair.psi = psi;
            UPCdiElePair.tanl = tanl;
            UPCdiElePair.dau1SelectionBit = UPCelectronSelectionBits[i1];
            UPCdiElePair.dau2SelectionBit = UPCelectronSelectionBits[i2];
            hlt_UPCdiEP.ePair[hlt_diEP.nEPairs] = UPCdiElePair;
            hlt_UPCdiEP.nEPairs ++;

        }
    }
}  //if UPCdiElectron

    if(triggerOnMatchedHT2) {

        //cout << "foo" << endl;

        const double electronMass = 0.511e-3;
        int nTowers = 0;
        gl3EmcTower* emcTower[maxNTowers];
        double MaxADC = 0;
        double towerADCs[4800];

        //ofstream towerout("towerADC.txt",ios_base::app);
        //ofstream Pout("P.txt",ios_base::app);
        //ofstream PFinalout("PFinal.txt",ios_base::app);
        //ofstream EoPout("EoP.txt",ios_base::app);
        //ofstream nHitsout("nHits.txt",ios_base::app);
        //ofstream nHitsFinalout("nHitsFinal.txt",ios_base::app);
        //ofstream MaxADCout("maxADC.txt",ios_base::app);
        //ofstream dPhiout("dPhi.txt",ios_base::app);
        //ofstream dPhiFinalout("dPhiFinal.txt",ios_base::app);
        //ofstream dZout("dZ.txt",ios_base::app);
        //ofstream dZFinalout("dZFinal.txt",ios_base::app);

        for(int i = 0; i < event->emc->getNBarrelTowers(); i++) {
            gl3EmcTower* tower = event->emc->getBarrelTower(i);
            int towerADC = tower->getADC();
            towerADCs[i] = towerADC;
            if(towerADC < 303) continue; //double check this. Should be HT threshold
            //(304 in ADC is 19, so less than 303 bitshifts to 18 or less)
            MaxADC = (towerADC > MaxADC) ? towerADC : MaxADC;
            if(nTowers >= maxNTowers) {
                cout << "gl3TriggerDecider::decide() WARN! towerArray Full!" << endl;
                break;
            }
            emcTower[nTowers] = tower;
            nTowers++;
        }

        //if(nTowers) cout << "found " << nTowers << " towers" << endl;

        if(nTowers) {

            //MaxADCout << MaxADC<<endl;

            //for(int i=0;i<event->emc->getNBarrelTowers(); i++){
            //towerout << towerADCs[i] << endl;
            //}

            int triggerIndex = 0;
            double highPTracks[1000];
            int numHighP = 0;
            gl3Node* node;
            gl3Track* track;
            bool EmcMatch = false;

            for(int i = 0; i < nTracks; i++) {
                node = event->getNode(i);
                track = node->primaryTrack;
                if(!track) continue;
                //nHitsout << track->nHits << endl;
                if(track->nHits < nHitsCut_matchedHT2) continue;
                double p = track->pt * sqrt(1. + pow(track->tanl, 2));
                //Pout << p << endl;
                if(p < PCut_matchedHT2) continue; //ignore obvious mis-matches
                highPTracks[numHighP] = i;
                numHighP++;

                if(!EmcMatch) {
                    for(int j = 0; j < nTowers; j++) {
                        gl3EmcTower* tower = emcTower[j];
                        double dPhi, dZ;
                        int matched = tower->matchTrack(track, dPhi, dZ, dPhiCut_matchedHT2, dZCut_matchedHT2);
                        //dPhiout << dPhi << endl;
                        //dZout << dZ << endl;
                        //open up phi and z cuts to 1 tower in both directions
                        if(!matched) continue;
                        //EoPout << tower->getEnergy()/p << endl;
                        if(tower->getEnergy() / p > 3.0) continue; //very wide E/p cut

                        //nHitsFinalout << track->nHits << endl;
                        //PFinalout << p << endl;
                        //dPhiFinalout << dPhi << endl;
                        //dZFinalout << dZ << endl;


                        EmcMatch = true;
                        numHighP--;
                        triggerIndex = i;
                        hlt_hT2.p0 = p;
                        hlt_hT2.towerADC = tower->getADC();
                        hlt_hT2.EoP0 = tower->getEnergy() / p;
                        hlt_hT2.towerSoftID = tower->getTowerInfo()->getSoftID();
                        hlt_hT2.maxADC = MaxADC;
                        hlt_hT2.r = track->r0;
                        hlt_hT2.phi = track->phi0;
                        hlt_hT2.z = track->z0;
                        hlt_hT2.psi = track->psi;
                        hlt_hT2.eta = track->eta;
                        hlt_hT2.pt = track->pt;
                        hlt_hT2.triggered = true;
                        //cout<<"Found one!" << endl;

                        matchedHT2Triggered = triggerBitMatchedHT2;
                        break;
                    }
                }
            }

            if(EmcMatch) {
                track = event->getNode(triggerIndex)->primaryTrack;

                double pt0 = track->pt;
                double px0 = pt0 * cos(track->psi);
                double py0 = pt0 * sin(track->psi);
                double pz0 = pt0 * track->tanl;
                double E0 = sqrt(electronMass * electronMass + px0 * px0 + py0 * py0 + pz0 * pz0);

                for(int i = 0; i < numHighP; i++) {
                    //cout << "comparing track " << i << endl;
                    track = event->getNode(highPTracks[i])->primaryTrack;

                    double nSigmaDedxE = track->nSigmaDedx(event->bischel, "Electron", sigmaDedx1);

                    if(nSigmaDedxE > 1.0 || nSigmaDedxE < -1.0) continue; //use pid cut for QA

                    double pt2 = track->pt;
                    double px2 = pt2 * cos(track->psi);
                    double py2 = pt2 * sin(track->psi);
                    double pz2 = pt2 * track->tanl;
                    double E2 = sqrt(electronMass * electronMass + px2 * px2 + py2 * py2 + pz2 * pz2);
                    double px = px0 + px2;
                    double py = py0 + py2;
                    double pz = pz0 + pz2;
                    double invariantMass = sqrt(pow(E0 + E2, 2) - px * px - py * py - pz * pz);
                    //cout << "inv mass: " << invariantMass << endl;

                    hlt_MatchedHT2 MHT2;
                    MHT2.p2 = sqrt(px2 * px2 + py2 * py2 + pz2 * pz2);
                    MHT2.invMass = invariantMass;

                    hlt_hT2.trackPair[hlt_hT2.nPairs] = MHT2;
                    hlt_hT2.nPairs++;
                }
            }
        }

        //Pout.close();
        //PFinalout.close();
        //nHitsout.close();
        //nHitsFinalout.close();
        //EoPout.close();
        //towerout.close();
        //MaxADCout.close();
        //dPhiout.close();
        //dPhiFinalout.close();
        //dZout.close();
        //dZFinalout.close();

    }

    //Low Mult trigger
    if(triggerOnLowMult) {
        if(nPTracks > nTracksLow_LowMult && nPTracks < nTracksHigh_LowMult) {
            LowMultTriggered = triggerBitLowMult;
            hlt_lm.nPrimaryTracks = nPTracks;
        }
    }
    //printf("test: trgger 0x%x nPT %i\n",LowMultTriggered,hlt_lm.nPrimaryTracks);

    if(triggerOnAllEvents) allEventsTriggered = triggerBitAllEvents;

    if(triggerOnRandomEvents && eventId % sampleScale_randomEvents == 0) randomEventsTriggered = triggerBitRandomEvents;

    if(triggerOnBesGoodEvents) {
        if(fabs(event->getVertex().Getz()) < vertexZCut_besGoodEvents &&
           sqrt(pow(event->getVertex().Getx(), 2) + pow(event->getVertex().Gety(), 2)) < vertexRCut_besGoodEvents &&
           event->getNPrimaryTracks() > nTracksCut_besGoodEvents) {
            besGoodEventsTriggered = triggerBitBesGoodEvents;
        }
    }

    if (triggerOnFixedTarget) {
        float Vz = event->getVertex().Getz();
        if (Vz > vertexZCutLow_FixedTarget && Vz < vertexZCutHigh_FixedTarget) {
            FixedTargetTriggered = triggerBitFixedTarget;
        }
    }

    if (triggerOnFixedTargetMonitor) {
        float Vz = event->getVertex().Getz();
        if (Vz > vertexZCutLow_FixedTargetMonitor &&
            Vz < vertexZCutHigh_FixedTargetMonitor ) {
            FixedTargetMonitorTriggered = triggerBitFixedTargetMonitor;
        }
    }

    if (triggerOnBesMonitor) {
        float Vz = event->getVertex().Getz();
        if (fabs(Vz) < vertexZCut_besMonitor &&
            event->getNPrimaryTracks() > nTracksCut_besMonitor) {
            besMonitorTriggered = triggerBitBesMonitor;
        }
    }

    // test by yiguo
    //if(UPCTriggered) printf("UPCpairtriggered\n");
    //if(UPCdiElectronTriggered) printf("UPCdiElectronTriggered\n");
    // end
    if(f1) {
        fprintf(f1, "trigger: 0x%x\n",
                event->trigger | highPtTriggered | diElectronTriggered |
                heavyFragmentTriggered | allEventsTriggered |
                randomEventsTriggered | besGoodEventsTriggered |
                matchedHT2Triggered | LowMultTriggered  | UPCTriggered |
                UPCdiElectronTriggered | MTDdiMuonTriggered | FixedTargetTriggered);
    }
    
    // don't write asc files if only minbias triggred
    if(highPtTriggered || diElectronTriggered || heavyFragmentTriggered ||
       matchedHT2Triggered || LowMultTriggered || UPCTriggered ||
       UPCdiElectronTriggered || MTDdiMuonTriggered || FixedTargetTriggered) {
        writeQA();
    }

    return highPtTriggered | diElectronTriggered | heavyFragmentTriggered |
        allEventsTriggered | randomEventsTriggered | besGoodEventsTriggered |
        matchedHT2Triggered | LowMultTriggered | UPCTriggered |
        UPCdiElectronTriggered | MTDdiMuonTriggered | FixedTargetTriggered;
}

bool gl3TriggerDecider::decide_HighPt()
{
    highPtTriggered = 0;
    hlt_hiPt.nHighPt = 0;
    
    if(!triggerOnHighPt) return false;

    NumOfCalled[HighPt]++;
    
    int nTracks = event->getNGlobalTracks();

    for(int i = 0; i < nTracks; i++) {
        gl3Node* node = event->getNode(i);
        gl3Track* pTrack = event->getNode(i)->primaryTrack;
        if(!pTrack) continue;

        //  LOG(TERR,"hight Pt: %d/%d : %d vs %d, %f vs %f",i,nTracks,
        //      pTrack->nHits,nHitsCut_highPt,
        //      pTrack->pt, ptCut_highPt) ;

        if(pTrack->nHits < nHitsCut_highPt) continue;
        if(pTrack->pt < ptCut_highPt) continue;

        highPtTriggered = triggerBitHighPt;

        if(!node->primarytofCell && node->primaryTrack) {
            event->tof->matchPrimaryTracks(node, event->getLmVertex());
        }
        double matchTowerEnergy = 0.;
        for(int j = 0; j < event->emc->getNBarrelTowers(); j++) {
            gl3EmcTower* tower = event->emc->getBarrelTower(j);
            double phiDiff, zEdge;
            if(!tower->matchTrack(pTrack, phiDiff, zEdge)) continue;
            if(tower->getEnergy() < matchTowerEnergy) continue;
            matchTowerEnergy = tower->getEnergy();
            node->emcTower = tower;
            node->emcMatchPhiDiff = phiDiff;
            node->emcMatchZEdge = zEdge;
        }

        hlt_hiPt.highPtNodeSN[hlt_hiPt.nHighPt] = i;
        hlt_hiPt.nHighPt ++;
    }

    hltDecision |= highPtTriggered;

    if (highPtTriggered) {
        NumOfSucceed[HighPt]++;
    }
    
    return highPtTriggered;
}
    
bool gl3TriggerDecider::decide_DiElectron()
{
    diElectronTriggered = 0;
    hlt_diEP.nEPairs    = 0;
    
    if(!triggerOnDiElectron) return false;

    NumOfCalled[DiElectron]++;
    
    int nTracks  = event->getNGlobalTracks();
   
    const double electronMass = 0.511e-3;
    int nElectronNodes = 0, nTowers = 0;
    gl3Node* electronNode[maxNElectronTracks];
    gl3EmcTower* emcTower[maxNTowers];
    int electronSelectionBits[maxNElectronTracks];
    int selectionBitTof = 0x1;
    int selectionBitEmc = 0x2;
    int selectionBitTofAndEmc = 0x4;

    for(int i = 0; i < event->emc->getNBarrelTowers(); i++) {
        gl3EmcTower* tower = event->emc->getBarrelTower(i);
        double towerEnergy = tower->getEnergy();
        if(towerEnergy < 0.5) continue;
        if(nTowers >= maxNTowers) {
            cout << "gl3TriggerDecider::decide()  WARN! towerArray Full!" << endl;
            break;
        }
        emcTower[nTowers] = tower;
        nTowers ++;
    }

    for(int i = 0; i < nTracks; i++) {
        int isElectron = 0;
        int passTofCut = 0;
        int passEmcCut = 0;
        gl3Node* node = event->getNode(i);
        gl3Track* track = node->primaryTrack;
        if(!track) continue;
        if(track->nHits < nHitsCut_diElectron) continue;
        if(track->nDedx < nDedxCut_diElectron) continue;
        double p = track->pt * sqrt(1. + pow(track->tanl, 2));
        if(p < p2Cut_diElectron) continue;
        double nSigmaDedxE = track->nSigmaDedx(event->bischel, "Electron", sigmaDedx1);
        if(nSigmaDedxE > min(nSigmaDedxCutLowForTof_diElectron, nSigmaDedxCutLowForTofAndEmc_diElectron)
           && nSigmaDedxE < max(nSigmaDedxCutHighForTof_diElectron, nSigmaDedxCutHighForTofAndEmc_diElectron)
           && p > min(pCutLowForTof_diElectron, pCutLowForTofAndEmc_diElectron)
           && p < max(pCutHighForTof_diElectron, pCutHighForTofAndEmc_diElectron)) {
            if(!node->primarytofCell && node->primaryTrack)
                event->tof->matchPrimaryTracks(node, event->getLmVertex());
            if(fabs(1. / node->beta - 1.) < oneOverBetaCut_diElectron)
                passTofCut = 1;
        }

        if(nSigmaDedxE > min(nSigmaDedxCutLowForEmc_diElectron, nSigmaDedxCutLowForTofAndEmc_diElectron)
           && nSigmaDedxE < max(nSigmaDedxCutHighForEmc_diElectron, nSigmaDedxCutHighForTofAndEmc_diElectron)
           && p > min(pCutLowForEmc_diElectron, pCutLowForTofAndEmc_diElectron)
           && p < max(pCutHighForEmc_diElectron, pCutHighForTofAndEmc_diElectron)) {
            double matchTowerEnergy = 0.;
            for(int j = 0; j < nTowers; j++) {
                gl3EmcTower* tower = emcTower[j];
                double phiDiff, zEdge;
                if(!tower->matchTrack(track, phiDiff, zEdge)) continue;
                if(tower->getEnergy() < matchTowerEnergy) continue;
                matchTowerEnergy = tower->getEnergy();
                node->emcTower = tower;
                node->emcMatchPhiDiff = phiDiff;
                node->emcMatchZEdge = zEdge;
            }
            if(matchTowerEnergy > towerEnergyCut_diElectron &&
               p / matchTowerEnergy > pOverECutLow_diElectron &&
               p / matchTowerEnergy < pOverECutHigh_diElectron)
                passEmcCut = 1;
        }

        if(nSigmaDedxE > nSigmaDedxCutLowForTof_diElectron
           && nSigmaDedxE < nSigmaDedxCutHighForTof_diElectron
           && p > pCutLowForTof_diElectron
           && p < pCutHighForTof_diElectron && passTofCut)
            isElectron |= selectionBitTof;

        if(nSigmaDedxE > nSigmaDedxCutLowForEmc_diElectron
           && nSigmaDedxE < nSigmaDedxCutHighForEmc_diElectron
           && p > pCutLowForEmc_diElectron
           && p < pCutHighForEmc_diElectron && passEmcCut)
            isElectron |= selectionBitEmc;

        if(nSigmaDedxE > nSigmaDedxCutLowForTofAndEmc_diElectron
           && nSigmaDedxE < nSigmaDedxCutHighForTofAndEmc_diElectron
           && p > pCutLowForTofAndEmc_diElectron
           && p < pCutHighForTofAndEmc_diElectron
           && passTofCut && passEmcCut)
            isElectron |= selectionBitTofAndEmc;

        if(isElectron) {
            if(nElectronNodes >= maxNElectronTracks) {
                cout << "gl3TriggerDecider::decide()  WARN! electronTrackArray Full!" << endl;
                break;
            }
            electronNode[nElectronNodes] = node;
            electronSelectionBits[nElectronNodes] = isElectron;
            nElectronNodes ++;
        }
    }

    for(int i = 0; i < nElectronNodes; i++) {
        for(int j = i + 1; j < nElectronNodes; j++) {
            int i1 = i;
            int i2 = j;

            gl3Node* node1 = electronNode[i];
            gl3Track* track1 = node1->primaryTrack;
            double p1 = track1->pt * sqrt(1. + pow(track1->tanl, 2));
            gl3Node* node2 = electronNode[j];
            gl3Track* track2 = node2->primaryTrack;
            double p2 = track2->pt * sqrt(1. + pow(track2->tanl, 2));
            if(p1 < p2) {
                swap(p1, p2);
                swap(i1, i2);
                swap(node1, node2);
                swap(track1, track2);
            }
            if(p1 < p1Cut_diElectron) continue;
            if(track1->q * track2->q > 0 && onlyOppPairs_diElectron) continue;
            if(fabs(track1->z0 - track2->z0) > vertexZDiffCut_diElectron) continue;

            double pt1 = track1->pt;
            double px1 = pt1 * cos(track1->psi);
            double py1 = pt1 * sin(track1->psi);
            double pz1 = pt1 * track1->tanl;
            double E1 = sqrt(electronMass * electronMass + p1 * p1);
            double pt2 = track2->pt;
            double px2 = pt2 * cos(track2->psi);
            double py2 = pt2 * sin(track2->psi);
            double pz2 = pt2 * track2->tanl;
            double E2 = sqrt(electronMass * electronMass + p2 * p2);
            double px = px1 + px2;
            double py = py1 + py2;
            double pz = pz1 + pz2;
            double psi = atan2(py, px);
            double pt = sqrt(pow(px, 2) + pow(py, 2));
            double tanl = atan2(pz, pt);
            double invariantMass = sqrt(pow(E1 + E2, 2) - pow(pt, 2) - pow(pz, 2));
            //          cout<<"px1:"<<px1<<"  py1:"<<py1<<"  pz1:"<<pz1<<"  E1:"<<E1<<"  px2:"<<px2<<"  py2:"<<py2<<"  pz2:"<<pz2<<"  E2:"<<E2<<"  invarMass:"<<invariantMass<<endl;
            //          cout<<"p1:"<<p1<<"  E1:"<<node1->emcTower<<" beta1:"<<node1->beta<<"  p2:"<<p2<<"  E2:"<<node2->emcTower<<" beta2:"<<node2->beta<<"  invarMass:"<<invariantMass<<endl;
            if(invariantMass < invariantMassCutLow_diElectron) continue;
            if(invariantMass > invariantMassCutHigh_diElectron) continue;

            diElectronTriggered = triggerBitDiElectron;

            hlt_diElectronPair diElePair;
            diElePair.dau1NodeSN = (int)(node1 - event->getNode(0));
            diElePair.dau2NodeSN = (int)(node2 - event->getNode(0));
            diElePair.invariantMass = invariantMass;
            diElePair.pt = pt;
            diElePair.psi = psi;
            diElePair.tanl = tanl;
            diElePair.dau1SelectionBit = electronSelectionBits[i1];
            diElePair.dau2SelectionBit = electronSelectionBits[i2];
            hlt_diEP.ePair[hlt_diEP.nEPairs] = diElePair;
            hlt_diEP.nEPairs ++;

            /*
              l3EmcTowerInfo* towerInfo1 = 0;
              if(matchTower[i1]) towerInfo1 = matchTower[i1]->getTowerInfo();
              l3EmcTowerInfo* towerInfo2 = 0;
              if(matchTower[i2]) towerInfo2 = matchTower[i2]->getTowerInfo();
              if (f1){ //write out QA messages
              fprintf(f1, "pair:  \n");
              fprintf(f1, "%e  ", invariantMass);
              if(matchTower[i1])
              fprintf(f1, "%i  %i  %i  %i  %i  %i  %i  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %i  %i  %i  %i  %e  %e  %e  %e  %e  %e  %e  %e  %e  %i  %e  %e  %e  ", track1->id, track1->flag, track1->innerMostRow, track1->outerMostRow, track1->nHits, track1->nDedx, track1->q, track1->chi2[0], track1->chi2[1], track1->dedx, track1->pt, track1->phi0, track1->psi, track1->r0, track1->tanl, track1->z0, track1->length, track1->dpt, track1->dpsi, track1->dz0, track1->eta, track1->dtanl, p1, px1, py1, pz1, towerInfo1->getDaqID(), towerInfo1->getSoftID(), towerInfo1->getCrate(), towerInfo1->getCrateSeq(), towerInfo1->getPhi(), towerInfo1->getEta(), towerInfo1->getEtaMin(), towerInfo1->getEtaMax(), towerInfo1->getZ(), towerInfo1->getZmin(), towerInfo1->getZmax(), towerInfo1->getGain(), towerInfo1->getPedestal(), matchTower[i1]->getADC(), matchTower[i1]->getEnergy(), matchPhiDiff[i1], matchZEdge[i1]);
              else
              fprintf(f1, "%i  %i  %i  %i  %i  %i  %i  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %i  %i  %i  %i  %e  %e  %e  %e  %e  %e  %e  %e  %e  %i  %e  %e  %e  ", track1->id, track1->flag, track1->innerMostRow, track1->outerMostRow, track1->nHits, track1->nDedx, track1->q, track1->chi2[0], track1->chi2[1], track1->dedx, track1->pt, track1->phi0, track1->psi, track1->r0, track1->tanl, track1->z0, track1->length, track1->dpt, track1->dpsi, track1->dz0, track1->eta, track1->dtanl, p1, px1, py1, pz1, 0, 0, 0, 0, 0., 0., 0., 0., 0., 0., 0., 0., 0., 0, 0., 0., 0.);
              if(matchTower[i2])
              fprintf(f1, "%i  %i  %i  %i  %i  %i  %i  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %i  %i  %i  %i  %e  %e  %e  %e  %e  %e  %e  %e  %e  %i  %e  %e  %e\n", track2->id, track2->flag, track2->innerMostRow, track2->outerMostRow, track2->nHits, track2->nDedx, track2->q, track2->chi2[0], track2->chi2[1], track2->dedx, track2->pt, track2->phi0, track2->psi, track2->r0, track2->tanl, track2->z0, track2->length, track2->dpt, track2->dpsi, track2->dz0, track2->eta, track2->dtanl, p2, px2, py2, pz2, towerInfo2->getDaqID(), towerInfo2->getSoftID(), towerInfo2->getCrate(), towerInfo2->getCrateSeq(), towerInfo2->getPhi(), towerInfo2->getEta(), towerInfo2->getEtaMin(), towerInfo2->getEtaMax(), towerInfo2->getZ(), towerInfo2->getZmin(), towerInfo2->getZmax(), towerInfo2->getGain(), towerInfo2->getPedestal(), matchTower[i2]->getADC(), matchTower[i2]->getEnergy(), matchPhiDiff[i2], matchZEdge[i2]);
              else
              fprintf(f1, "%i  %i  %i  %i  %i  %i  %i  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %i  %i  %i  %i  %e  %e  %e  %e  %e  %e  %e  %e  %e  %i  %e  %e  %e\n", track2->id, track2->flag, track2->innerMostRow, track2->outerMostRow, track2->nHits, track2->nDedx, track2->q, track2->chi2[0], track2->chi2[1], track2->dedx, track2->pt, track2->phi0, track2->psi, track2->r0, track2->tanl, track2->z0, track2->length, track2->dpt, track2->dpsi, track2->dz0, track2->eta, track2->dtanl, p2, px2, py2, pz2, 0, 0, 0, 0, 0., 0., 0., 0., 0., 0., 0., 0., 0., 0, 0., 0., 0.);
              }
            */
        }
    }

    hltDecision |= diElectronTriggered;

    if (diElectronTriggered) {
        NumOfSucceed[DiElectron]++;
    }

    return diElectronTriggered;
}

bool gl3TriggerDecider::decide_DiElectron2Twr()
{
    diElectron2TwrTriggered = 0;
    hlt_diEP2Twr.nEPairs    = 0;

    if(!triggerOnDiElectron2Twr) return false;

    NumOfCalled[DiElectron2Twr]++;

    int nTracks  = event->getNGlobalTracks();

    const double electronMass   = 0.511e-3;
    int          nElectronNodes = 0;
    int          nTowers        = 0;
    gl3Node*     electronNode[maxNElectronTracks];
    gl3EmcTower* emcTower[maxNTowers];
    int          electronSelectionBits[maxNElectronTracks];
    int          selectionBitTof       = 0x1;
    int          selectionBitEmc       = 0x2;
    int          selectionBitTofAndEmc = 0x4;

    for(int i = 0; i < event->emc->getNBarrelTowers(); i++) {
        gl3EmcTower* tower = event->emc->getBarrelTower(i);
        double towerEnergy = tower->getEnergy();
        if(towerEnergy < towerEnergyCut_diElectron2Twr) continue;
        if(nTowers >= maxNTowers) {
            LOG(WARN, "gl3TriggerDecider::decide()  WARN! towerArray Full!");
            break;
        }
        emcTower[nTowers] = tower;
        nTowers ++;
    }

    for(int i = 0; i < nTracks; i++) {
        int isElectron = 0;
        int passTofCut = 0;
        int passEmcCut = 0;
        gl3Node* node = event->getNode(i);
        gl3Track* track = node->primaryTrack;
        if(!track) continue;
        if(track->nHits < nHitsCut_diElectron2Twr) continue;
        if(track->nDedx < nDedxCut_diElectron2Twr) continue;
        double p = track->pt * sqrt(1. + pow(track->tanl, 2));
        if(p < p2Cut_diElectron2Twr) continue;
        double nSigmaDedxE = track->nSigmaDedx(event->bischel, "Electron", sigmaDedx1);

        if(nSigmaDedxE > nSigmaDedxCutLowForTof_diElectron2Twr
           && nSigmaDedxE < nSigmaDedxCutHighForTof_diElectron2Twr
           && p > pCutLowForTof_diElectron2Twr
           && p < pCutHighForTof_diElectron2Twr){ 

            if(!node->primarytofCell && node->primaryTrack)
                event->tof->matchPrimaryTracks(node, event->getLmVertex());
            if(fabs(1. / node->beta - 1.) < oneOverBetaCut_diElectron2Twr)
                passTofCut = 1;
        }

        if(nSigmaDedxE > nSigmaDedxCutLowForEmc_diElectron2Twr
           && nSigmaDedxE < nSigmaDedxCutHighForEmc_diElectron2Twr
           && p > pCutLowForEmc_diElectron2Twr
           && p < pCutHighForEmc_diElectron2Twr) {

            double matchTowerEnergy = 0.;
            for(int j = 0; j < nTowers; j++) {
                gl3EmcTower* tower = emcTower[j];
                double phiDiff, zEdge;
                if(!tower->matchTrack(track, phiDiff, zEdge)) continue;
                if(tower->getEnergy() < matchTowerEnergy) continue;
                matchTowerEnergy = tower->getEnergy();
                node->emcTower = tower;
                node->emcMatchPhiDiff = phiDiff;
                node->emcMatchZEdge = zEdge;
            }

            if(matchTowerEnergy > towerEnergyCut_diElectron2Twr
               && p / matchTowerEnergy > pOverECutLow_diElectron2Twr
               && p / matchTowerEnergy < pOverECutHigh_diElectron2Twr)
                passEmcCut = 1;
        }

        if(passTofCut)
            isElectron |= selectionBitTof;

        if(passEmcCut)
            isElectron |= selectionBitEmc;

        if(isElectron) {
            if(nElectronNodes >= maxNElectronTracks) {
                LOG(WARN, "gl3TriggerDecider::decide()  WARN! electronTrackArray Full!");
                break;
            }
            electronNode[nElectronNodes] = node;
            electronSelectionBits[nElectronNodes] = isElectron;
            nElectronNodes ++;
        }
    }

    for(int i = 0; i < nElectronNodes; i++) {
        for(int j = i + 1; j < nElectronNodes; j++) {
            int i1 = i;
            int i2 = j;

            gl3Node* node1 = electronNode[i];
            gl3Track* track1 = node1->primaryTrack;
            double p1 = track1->pt * sqrt(1. + pow(track1->tanl, 2));
            gl3Node* node2 = electronNode[j];
            gl3Track* track2 = node2->primaryTrack;
            double p2 = track2->pt * sqrt(1. + pow(track2->tanl, 2));
            double pHigh = 0;
            if(p1 < p2) pHigh = p2;
            else pHigh = p1;
            if(pHigh < p1Cut_diElectron2Twr) continue;
            if(track1->q * track2->q > 0 && onlyOppPairs_diElectron2Twr) continue;

            double pt1 = track1->pt;
            double px1 = pt1 * cos(track1->psi);
            double py1 = pt1 * sin(track1->psi);
            double pz1 = pt1 * track1->tanl;
            double E1 = sqrt(electronMass * electronMass + p1 * p1);
            double pt2 = track2->pt;
            double px2 = pt2 * cos(track2->psi);
            double py2 = pt2 * sin(track2->psi);
            double pz2 = pt2 * track2->tanl;
            double E2 = sqrt(electronMass * electronMass + p2 * p2);
            double px = px1 + px2;
            double py = py1 + py2;
            double pz = pz1 + pz2;
            double psi = atan2(py, px);
            double pt = sqrt(pow(px, 2) + pow(py, 2));
            double tanl = atan2(pz, pt);
            double invariantMass = sqrt(pow(E1 + E2, 2) - pow(pt, 2) - pow(pz, 2));

            if(invariantMass < invariantMassCutLow_diElectron2Twr) continue;
            if(invariantMass > invariantMassCutHigh_diElectron2Twr) continue;

            diElectron2TwrTriggered = triggerBitDiElectron2Twr;

            hlt_diElectronPair diElePair;
            diElePair.dau1NodeSN = (int)(node1 - event->getNode(0));
            diElePair.dau2NodeSN = (int)(node2 - event->getNode(0));
            diElePair.invariantMass = invariantMass;
            diElePair.pt = pt;
            diElePair.psi = psi;
            diElePair.tanl = tanl;
            diElePair.dau1SelectionBit = electronSelectionBits[i1];
            diElePair.dau2SelectionBit = electronSelectionBits[i2];
            hlt_diEP2Twr.ePair[hlt_diEP2Twr.nEPairs] = diElePair;
            hlt_diEP2Twr.nEPairs++;
        }
    }

    hltDecision |= diElectron2TwrTriggered;

    if (diElectron2TwrTriggered) {
        NumOfSucceed[DiElectron2Twr]++;
    }

    return diElectron2TwrTriggered;
}

bool gl3TriggerDecider::decide_HeavyFragment()
{
    heavyFragmentTriggered = 0;
    hlt_hF.nHeavyFragments = 0;
    
    if(!triggerOnHeavyFragment) return false;

    NumOfCalled[HeavyFragment]++;
    
    int nTracks = event->getNGlobalTracks();
 
    for(int i = 0; i < nTracks; i++) {
        int triggered = 0;
        gl3Node* node = event->getNode(i);
        gl3Track* gTrack = event->getGlobalTrack(i);
        gl3Track* pTrack = node->primaryTrack;
        if(gTrack->nHits < nHitsCut_heavyFragment) continue;
        if(gTrack->nDedx < nDedxCut_heavyFragment) continue;

        if(useTofMatchedGlobalTrack_heavyfragMent && !node->globaltofCell) continue;   ///< require tof match for global tracks

        double x0 = gTrack->r0 * cos(gTrack->phi0);
        double y0 = gTrack->r0 * sin(gTrack->phi0);
        double dca2 = sqrt(pow(x0 - event->beamlineX, 2) + pow(y0 - event->beamlineY, 2));

        double dcaToUse = 0;
        if(event->useBeamlineToMakePrimarys) dcaToUse = dca2 ; ///< using 2D dca in pp 500GeV
        else dcaToUse = gTrack->dca ;  ///< using 3D dca in AuAu 200 GeV

        if(dcaToUse > dcaCut_heavyFragment || dcaToUse < 0) continue;

        if(gTrack->nSigmaDedx(event->bischel, "He3", sigmaDedx1) > nSigmaDedxHe3Cut_heavyFragment) {
            triggered = 1;
            if(!node->primarytofCell && node->primaryTrack)
                event->tof->matchPrimaryTracks(node, event->getLmVertex());
            double matchTowerEnergy = 0.;
            for(int i = 0; i < event->emc->getNBarrelTowers(); i++) {
                gl3EmcTower* tower = event->emc->getBarrelTower(i);
                double phiDiff, zEdge;
                if(!tower->matchTrack(gTrack, phiDiff, zEdge)) continue;
                if(tower->getEnergy() < matchTowerEnergy) continue;
                matchTowerEnergy = tower->getEnergy();
                node->emcTower = tower;
                node->emcMatchPhiDiff = phiDiff;
                node->emcMatchZEdge = zEdge;
            }
        }

        if(fabs(gTrack->nSigmaDedx(event->bischel, "Triton", sigmaDedx1)) < nSigmaDedxTritonCut_heavyFragment) {
            if(!node->primarytofCell && node->primaryTrack)
                event->tof->matchPrimaryTracks(node, event->getLmVertex());
            if(node->primarytofCell) {
                //      double c = 29.979; //cm/ns
                double p = pTrack->pt * sqrt(1. + pow(pTrack->tanl, 2));
                double beta = node->beta;
                double tof = node->tof;
                double l = beta * tof;
                double tritonMass = 2.80925;
                double sigmaMass1 = pow(p, 2) * 2. / l * sqrt(1. + pow(tritonMass / p, 2)) * sigmaTof;
                double sigmaMass2 = pow(tritonMass, 2) * 2.*p * dPOverP2;
                double sigmaMass = sqrt(pow(sigmaMass1, 2) + pow(sigmaMass2, 2));
                //LOG(INFO,"sigmaMass1:%f, sigmaMass2:%f, sigmaMass:%f,p:%f,tof:%f",sigmaMass1,sigmaMass2,sigmaMass,p,tof);
                double m2 = pow(p, 2) * (pow(1. / beta, 2) - 1);
                if(fabs(m2 - pow(tritonMass, 2)) < nSigmaMassTritonCut_heavyFragment * sigmaMass) {
                    triggered = 1;
                    double matchTowerEnergy = 0.;
                    for(int i = 0; i < event->emc->getNBarrelTowers(); i++) {
                        gl3EmcTower* tower = event->emc->getBarrelTower(i);
                        double phiDiff, zEdge;
                        if(!tower->matchTrack(gTrack, phiDiff, zEdge)) continue;
                        if(tower->getEnergy() < matchTowerEnergy) continue;
                        matchTowerEnergy = tower->getEnergy();
                        node->emcTower = tower;
                        node->emcMatchPhiDiff = phiDiff;
                        node->emcMatchZEdge = zEdge;
                    }
                }

                //      if(mass>tritonMass-nSigmaMassTritonCut_heavyFragment*sigmaMass)
                //        cout<<"triton trigger: "<<gTrack->nHits<<"  "<<gTrack->nDedx<<"  "<<gTrack->pt*sqrt(1.+pow(gTrack->tanl,2))<<"  "<<gTrack->dedx<<"  "<<gTrack->nSigmaDedx(event->bischel, "Triton", sigmaDedx1)<<"  "<<mass<<"+-"<<sigmaMass<<endl;
            }
        }

        if(triggered) {
            heavyFragmentTriggered = triggerBitHeavyFragment;
            hlt_hF.heavyFragmentSN[hlt_hF.nHeavyFragments] = i;
            hlt_hF.nHeavyFragments ++;
        }
    }

    hltDecision |= heavyFragmentTriggered;

    if (heavyFragmentTriggered) {
        NumOfSucceed[HeavyFragment]++;
    }
    
    return heavyFragmentTriggered;
}

bool gl3TriggerDecider::decide_AllEvents()
{
    // allEvents_with_HLT
    allEventsTriggered = 0;

    if (!triggerOnAllEvents) return false;

    NumOfCalled[AllEvents]++;
    
    allEventsTriggered = triggerBitAllEvents;

    hltDecision |= allEventsTriggered;

    if (allEventsTriggered) {
        NumOfSucceed[AllEvents]++;
    }
    
    return allEventsTriggered;
}

bool gl3TriggerDecider::decide_RandomEvents()
{
    randomEventsTriggered  = 0;

    if(!triggerOnRandomEvents) return false;

    NumOfCalled[RandomEvents]++;
    
    // static int RandomEventNumOffset = getpid() % sampleScale_randomEvents;
    // int eventId = event->eventNumber + RandomEventNumOffset;

    int eventId = rand();

    if ( !(eventId % sampleScale_randomEvents) ) {
        randomEventsTriggered = triggerBitRandomEvents;
    }

    LOG(NOTE, "evtId %-6d, sampleScale %d, %d",
        eventId, sampleScale_randomEvents, eventId%sampleScale_randomEvents);
    
    hltDecision |= randomEventsTriggered;

    if (randomEventsTriggered) {
        NumOfSucceed[RandomEvents]++;
    }
    
    return randomEventsTriggered;
}

bool gl3TriggerDecider::decide_BesGoodEvents()
{
    besGoodEventsTriggered = 0;

    if(!triggerOnBesGoodEvents) return false;

    NumOfCalled[BesGoodEvents]++;
    
    float beamPipeCenX = 0;
    float beamPipeCenY = 0;

    float vx = event->getVertex().Getx()-beamPipeCenX;
    float vy = event->getVertex().Gety()-beamPipeCenY;
    float vr = sqrt(vx*vx+vy*vy);

    if(fabs(event->getVertex().Getz()) < vertexZCut_besGoodEvents
       && vr < vertexRCut_besGoodEvents
       && event->getNPrimaryTracks() > nTracksCut_besGoodEvents) {
        besGoodEventsTriggered = triggerBitBesGoodEvents;
        LOG(NOTE, "accept besgoodevents %f %f %d", vertexZCut_besGoodEvents, vertexRCut_besGoodEvents, nTracksCut_besGoodEvents);
    }

    hltDecision |= besGoodEventsTriggered;

    if (besGoodEventsTriggered) {
        NumOfSucceed[BesGoodEvents]++;
    }
    
    return besGoodEventsTriggered;
}

bool gl3TriggerDecider::decide_HLTGood2()
{
    HLTGood2Triggered = 0;

    if(!triggerOnHLTGood2) return false;

    NumOfCalled[HLTGood2]++;
    
    float beamPipeCenX = 0;
    float beamPipeCenY = 0;

    float vx = event->getVertex().Getx()-beamPipeCenX;
    float vy = event->getVertex().Gety()-beamPipeCenY;
    float vr = sqrt(vx*vx+vy*vy);

    LOG(NOTE, "running HLTGood2");
    
    if(fabs(event->getVertex().Getz()) < vertexZCut_HLTGood2
       && vr < vertexRCut_HLTGood2
       && event->getNPrimaryTracks() > nTracksCut_HLTGood2) {
        HLTGood2Triggered = triggerBitHLTGood2;
        LOG(NOTE, "accept HLTGood2 %f %f %d", vertexZCut_HLTGood2, vertexRCut_HLTGood2, nTracksCut_HLTGood2);
    }
    
    hltDecision |= HLTGood2Triggered;

    if (HLTGood2Triggered) {
        NumOfSucceed[HLTGood2]++;
    }

    return HLTGood2Triggered;
}

bool gl3TriggerDecider::decide_diV0()
{
    NumOfCalled[DiV0]++;
    
    return false;
}

bool gl3TriggerDecider::decide_MatchedHT2()
{
    matchedHT2Triggered = 0;
    hlt_hT2.nPairs      = 0;
    hlt_hT2.triggered   = false;

    if(!triggerOnMatchedHT2) return false;

    NumOfCalled[MatchedHT2]++;

    const double electronMass = 0.511e-3;

    int          nTracks  = event->getNGlobalTracks();

    int          nTowers  = 0;
    gl3EmcTower* emcTower[maxNTowers];
    double       MaxADC   = 0;
    double       towerADCs[4800];

    for(int i = 0; i < event->emc->getNBarrelTowers(); i++) {
        gl3EmcTower* tower = event->emc->getBarrelTower(i);
        int towerADC = tower->getADC();
        towerADCs[i] = towerADC;
        if(towerADC < 303) continue; //double check this. Should be HT threshold
        //(304 in ADC is 19, so less than 303 bitshifts to 18 or less)
        MaxADC = (towerADC > MaxADC) ? towerADC : MaxADC;
        if(nTowers >= maxNTowers) {
            cout << "gl3TriggerDecider::decide() WARN! towerArray Full!" << endl;
            break;
        }
        emcTower[nTowers] = tower;
        nTowers++;
    }

    //if(nTowers) cout << "found " << nTowers << " towers" << endl;

    if(nTowers) {

        //MaxADCout << MaxADC<<endl;

        //for(int i=0;i<event->emc->getNBarrelTowers(); i++){
        //towerout << towerADCs[i] << endl;
        //}

        int       triggerIndex = 0;
        double    highPTracks[1000];
        int       numHighP = 0;
        gl3Node*  node;
        gl3Track* track;
        bool      EmcMatch = false;

        for(int i = 0; i < nTracks; i++) {
            node = event->getNode(i);
            track = node->primaryTrack;
            if(!track) continue;
            //nHitsout << track->nHits << endl;
            if(track->nHits < nHitsCut_matchedHT2) continue;
            double p = track->pt * sqrt(1. + pow(track->tanl, 2));
            //Pout << p << endl;
            if(p < PCut_matchedHT2) continue; //ignore obvious mis-matches
            highPTracks[numHighP] = i;
            numHighP++;

            if(!EmcMatch) {
                for(int j = 0; j < nTowers; j++) {
                    gl3EmcTower* tower = emcTower[j];
                    double dPhi, dZ;
                    int matched = tower->matchTrack(track, dPhi, dZ, dPhiCut_matchedHT2, dZCut_matchedHT2);
                    //dPhiout << dPhi << endl;
                    //dZout << dZ << endl;
                    //open up phi and z cuts to 1 tower in both directions
                    if(!matched) continue;
                    //EoPout << tower->getEnergy()/p << endl;
                    if(tower->getEnergy() / p > 3.0) continue; //very wide E/p cut

                    //nHitsFinalout << track->nHits << endl;
                    //PFinalout << p << endl;
                    //dPhiFinalout << dPhi << endl;
                    //dZFinalout << dZ << endl;

                    EmcMatch = true;
                    numHighP--;
                    triggerIndex = i;
                    hlt_hT2.p0 = p;
                    hlt_hT2.towerADC = tower->getADC();
                    hlt_hT2.EoP0 = tower->getEnergy() / p;
                    hlt_hT2.towerSoftID = tower->getTowerInfo()->getSoftID();
                    hlt_hT2.maxADC = MaxADC;
                    hlt_hT2.r = track->r0;
                    hlt_hT2.phi = track->phi0;
                    hlt_hT2.z = track->z0;
                    hlt_hT2.psi = track->psi;
                    hlt_hT2.eta = track->eta;
                    hlt_hT2.pt = track->pt;
                    hlt_hT2.triggered = true;
                    //cout<<"Found one!" << endl;

                    matchedHT2Triggered = triggerBitMatchedHT2;
                    break;
                }
            }
        }

        if(EmcMatch) {
            track = event->getNode(triggerIndex)->primaryTrack;

            double pt0 = track->pt;
            double px0 = pt0 * cos(track->psi);
            double py0 = pt0 * sin(track->psi);
            double pz0 = pt0 * track->tanl;
            double E0 = sqrt(electronMass * electronMass + px0 * px0 + py0 * py0 + pz0 * pz0);

            for(int i = 0; i < numHighP; i++) {
                //cout << "comparing track " << i << endl;
                track = event->getNode(highPTracks[i])->primaryTrack;

                double nSigmaDedxE = track->nSigmaDedx(event->bischel, "Electron", sigmaDedx1);

                if(nSigmaDedxE > 1.0 || nSigmaDedxE < -1.0) continue; //use pid cut for QA

                double pt2 = track->pt;
                double px2 = pt2 * cos(track->psi);
                double py2 = pt2 * sin(track->psi);
                double pz2 = pt2 * track->tanl;
                double E2 = sqrt(electronMass * electronMass + px2 * px2 + py2 * py2 + pz2 * pz2);
                double px = px0 + px2;
                double py = py0 + py2;
                double pz = pz0 + pz2;
                double invariantMass = sqrt(pow(E0 + E2, 2) - px * px - py * py - pz * pz);
                //cout << "inv mass: " << invariantMass << endl;

                hlt_MatchedHT2 MHT2;
                MHT2.p2 = sqrt(px2 * px2 + py2 * py2 + pz2 * pz2);
                MHT2.invMass = invariantMass;

                hlt_hT2.trackPair[hlt_hT2.nPairs] = MHT2;
                hlt_hT2.nPairs++;
            }
        }
    }

    hltDecision |= matchedHT2Triggered;

    if (matchedHT2Triggered) {
        NumOfSucceed[MatchedHT2]++;
    }
    
    return matchedHT2Triggered;
}

bool gl3TriggerDecider::decide_LowMult()
{
    LowMultTriggered      = 0;
    hlt_lm.nPrimaryTracks = -999;

    int nTracks = event->getNGlobalTracks();
    // int nPTracks = event->getNPrimaryTracks();

    if(triggerOnLowMult) return false;

    NumOfCalled[LowMult]++;
    
    if(nTracks > nTracksLow_LowMult && nTracks < nTracksHigh_LowMult) {
        LowMultTriggered = triggerBitLowMult;
        hlt_lm.nPrimaryTracks = nTracks;
    }

    hltDecision |= LowMultTriggered;

    if (LowMultTriggered) {
        NumOfSucceed[LowMult]++;
    }

    return LowMultTriggered;
}

bool gl3TriggerDecider::decide_UPCPair()
{
    UPCTriggered = 0;
    hlt_UPCrho.nRhos = 0;

    // if(triggerOnUPCpair && (event->trigger & daqId_UPC)) return false;
    if(!triggerOnUPCpair) return false;

    NumOfCalled[UPCPair]++;
    
    const double pionMass = 0.139;

    int nTracks  = event->getNGlobalTracks();

    gl3Node* UPCpionNode[maxNElectronTracks];
    int      numberPionFirstPass = 0;
    float    phiFirstLoop        = 0.;

    if(fabs(event->getVertex().Getz()) < vertexZCut_UPC && sqrt(pow(event->getVertex().Getx(), 2) + pow(event->getVertex().Gety(), 2)) < vertexRCut_UPC && event->getNPrimaryTracks() < nTracksCut_UPC) {
        //cout<<"  RD we have a UPC trigger "<<" vertex z "<<event->getVertex().Getz()<<" trigger word "<<event->trigger<<endl;
        for(int i = 0; i < nTracks; i++) { // these tracks are global  Maybe we should only loop over primary tracks
            gl3Node* node = event->getNode(i);
            //      gl3Track* gTrack = node->globalTrack;
            //      if(!gTrack) continue;
            //      if(gTrack->nHits < 14) continue;
            //     LOG(TERR,"UPC global tracks: %d/%d : nHits  %d, pt %f  z0 %f",i,nTracks,
            //         gTrack->nHits, gTrack->pt, gTrack->z0) ;

            gl3Track* pTrack = node->primaryTrack;
            if(!pTrack) continue;
            //if(event->trigger==32768){
            //LOG(TERR,"UPC topo tracks: %d/%d : nHits  %d, pt %f z0 %f phi %f prim Trk %d",i,nTracks,
            //pTrack->nHits, pTrack->pt, pTrack->z0, pTrack->psi, prim) ;
            // cout<<" prim tof cell "<< node->primarytofCell<<" global tof cell "<<node->globaltofCell<<endl;
            //}

            if(pTrack->nHits < 14) continue;
            //  if(!node->primarytofCell) continue;  //rejects tracks without TOF (too strong throws all tracks)



            if(!node->primarytofCell && node->primaryTrack)
                event->tof->matchPrimaryTracks(node, event->getLmVertex());
            if(node->primarytofCell) {
                //this track should be saved in an array and then we loop over all good tracks to select a back-to-back track that may make a good invariant mass calculation
                // check pid with dedx
                //
                double nSigmaDedxPion = pTrack->nSigmaDedx(event->bischel, "Pion", sigmaDedx1);

                if(nSigmaDedxPion > 3.0 || nSigmaDedxPion < -3.0) continue;

                //LOG(TERR,"UPC PION : %d/%d : nHits  %d, pt %f z0 %f phi %f sector %d",i,nTracks,
                //       pTrack->nHits, pTrack->pt, pTrack->z0, pTrack->psi, pTrack->sector) ;
                UPCpionNode[numberPionFirstPass] = node;
                numberPionFirstPass++;
            }
        }
        //
        //if we have at least one track from first pass, we loop over all tracks
        //
        //cout<<" num select pion "<<numberPionFirstPass<<endl;
        if(numberPionFirstPass > 0) {
            for(int i = 0; i < numberPionFirstPass; i++) {
                gl3Node* node1 = UPCpionNode[i];
                gl3Track* pTrack1 = node1->primaryTrack;
                phiFirstLoop = pTrack1->psi;
                double pt0 = pTrack1->pt;
                double px0 = pt0 * cos(pTrack1->psi);
                double py0 = pt0 * sin(pTrack1->psi);
                double pz0 = pt0 * pTrack1->tanl;
                double E0 = sqrt(pionMass * pionMass + px0 * px0 + py0 * py0 + pz0 * pz0);

                for(int j = 0; j < nTracks; j++) { //  loop over primary tracks
                    gl3Node* node2 = event->getNode(j);
                    gl3Track* pTrack2 = node2->primaryTrack;
                    if(!pTrack2) continue;
                    if(pTrack2->nHits < 14) continue;
                    //  if(!node->primarytofCell) continue;  //rejects tracks without TOF (too strong throws all tracks)
                    //
                    // lets look at delta phi with some decently wide window
                    //
                    double deltaPhi_UPC = phiFirstLoop - pTrack2->psi;

                    if(fabs(deltaPhi_UPC - 3.14) < 0.785) {


                        double pt2 = pTrack2->pt;
                        double px2 = pt2 * cos(pTrack2->psi);
                        double py2 = pt2 * sin(pTrack2->psi);
                        double pz2 = pt2 * pTrack2->tanl;
                        double E2 = sqrt(pionMass * pionMass + px2 * px2 + py2 * py2 + pz2 * pz2);
                        double px = px0 + px2;
                        double py = py0 + py2;
                        double pz = pz0 + pz2;
                        double psi = atan2(py, px);
                        double pt = sqrt(pow(px, 2) + pow(py, 2));
                        double tanl = atan2(pz, pt);
                        double invariantMass_UPC = sqrt(pow(E0 + E2, 2) - px * px - py * py - pz * pz);

                        if(invariantMass_UPC > 0.5 && invariantMass_UPC < 1.) {
                            //LOG(TERR,"select UPC : %d/%d : index1  %d, invMass %f phi 1 %f phi %f sector %d",j,nTracks,
                            //       i, invariantMass_UPC, phiFirstLoop, pTrack2->psi, pTrack2->sector) ;
                            UPCTriggered = triggerBitUPC;
                            //SavedeltaPhi_UPC = deltaPhi_UPC;
                            hlt_diPionPair diPionPair;
                            diPionPair.dau1NodeSN = (int)(node1 - event->getNode(0));
                            diPionPair.dau2NodeSN = (int)(node2 - event->getNode(0));
                            diPionPair.invariantMass = invariantMass_UPC;
                            diPionPair.pt = pt;
                            diPionPair.psi = psi;
                            diPionPair.tanl = tanl;
                            diPionPair.deltphi = deltaPhi_UPC;
                            hlt_UPCrho.PionPair[hlt_UPCrho.nRhos] = diPionPair;
                            hlt_UPCrho.nRhos++;
                            break;
                        }
                    }

                } //inner loop over tracks
            } // outer loop over selected pions
        }   // if number pion >0
    }   // if vertex and # tracks conditions are satisfied

    hltDecision |= UPCTriggered;

    if (UPCTriggered) {
        NumOfSucceed[UPCPair]++;
    }
    
    return UPCTriggered;
}

bool gl3TriggerDecider::decide_UPCDiElectron()
{
    UPCdiElectronTriggered = 0;
    hlt_UPCdiEP.nEPairs    = 0;

    // if(!(triggerOnUPCdiElectron
    //      && (event->trigger & daqId_UPC)
    //      && event->getNPrimaryTracks() <= nTracksCut_UPCdiElectron)) return false;

    if( !(triggerOnUPCdiElectron
          && event->getNPrimaryTracks() <= nTracksCut_UPCdiElectron) ) return false;

    NumOfCalled[UPCDiElectron]++;

    int nTracks  = event->getNGlobalTracks();

    const double electronMass = 0.511e-3;
    int nUPCElectronNodes = 0, nTowers = 0;
    gl3Node* UPCelectronNode[maxNElectronTracks];
    gl3EmcTower* UPCemcTower[maxNTowers];
    int UPCelectronSelectionBits[maxNElectronTracks];
    int selectionBitTof = 0x1;
    int selectionBitEmc = 0x2;
    int selectionBitTofAndEmc = 0x4;


    // return... orz
    //if( event->getNPrimaryTracks()>nTracksCut_UPC) return 0;

    for(int i = 0; i < event->emc->getNBarrelTowers(); i++) {
        gl3EmcTower* tower = event->emc->getBarrelTower(i);
        double towerEnergy = tower->getEnergy();
        if(towerEnergy < 0.5) continue;
        //    cout<<"RD tower energy "<<towerEnergy<<endl;
        if(nTowers >= maxNTowers) {
            cout << "gl3TriggerDecider::decide()  WARN! towerArray Full!" << endl;
            break;
        }
        UPCemcTower[nTowers] = tower;
        nTowers ++;
    }

    for(int i = 0; i < nTracks; i++) {
        int isElectron = 0;
        int passTofCut = 0;
        int passEmcCut = 0;
        gl3Node* node = event->getNode(i);
        gl3Track* track = node->primaryTrack;

        if(!track) continue;
        if(track->nHits < nHitsCut_UPCdiElectron) continue;
        if(track->nDedx < nDedxCut_UPCdiElectron) continue;
        double p = track->pt * sqrt(1. + pow(track->tanl, 2));
        if(p < p2Cut_UPCdiElectron) continue;

        double nSigmaDedxE = track->nSigmaDedx(event->bischel, "Electron", sigmaDedx1);
        // cout<<"  track passed  --------- nsigma  "<<nSigmaDedxE<<endl;
#if 0
        if(nSigmaDedxE > min(nSigmaDedxCutLowForTof_UPCdiElectron, nSigmaDedxCutLowForTofAndEmc_UPCdiElectron)
           && nSigmaDedxE < max(nSigmaDedxCutHighForTof_UPCdiElectron, nSigmaDedxCutHighForTofAndEmc_UPCdiElectron)
           && p > min(pCutLowForTof_UPCdiElectron, pCutLowForTofAndEmc_UPCdiElectron)
           && p < max(pCutHighForTof_UPCdiElectron, pCutHighForTofAndEmc_UPCdiElectron)) {

            if(!node->primarytofCell && node->primaryTrack)
                event->tof->matchPrimaryTracks(node, event->getLmVertex());
            if(fabs(1. / node->beta - 1.) < oneOverBetaCut_UPCdiElectron)
                passTofCut = 1;
        }
#endif
        //
        //RD we may not have TOF all the time set it always to TRUE
        //
        passTofCut = 1;

        if(nSigmaDedxE > min(nSigmaDedxCutLowForEmc_UPCdiElectron, nSigmaDedxCutLowForTofAndEmc_UPCdiElectron)
           && nSigmaDedxE < max(nSigmaDedxCutHighForEmc_UPCdiElectron, nSigmaDedxCutHighForTofAndEmc_UPCdiElectron)
           && p > min(pCutLowForEmc_UPCdiElectron, pCutLowForTofAndEmc_UPCdiElectron)
           && p < max(pCutHighForEmc_UPCdiElectron, pCutHighForTofAndEmc_UPCdiElectron)) {
            double matchTowerEnergy = 0.;
            //        cout<<"got electron track ---------"<<endl;
            for(int j = 0; j < nTowers; j++) {
                gl3EmcTower* tower = UPCemcTower[j];
                double phiDiff, zEdge;
                if(!tower->matchTrack(track, phiDiff, zEdge)) continue;
                if(tower->getEnergy() < matchTowerEnergy) continue;
                matchTowerEnergy = tower->getEnergy();
                node->emcTower = tower;
                node->emcMatchPhiDiff = phiDiff;
                node->emcMatchZEdge = zEdge;
            }
            if(matchTowerEnergy > towerEnergyCut_diElectron &&
               p / matchTowerEnergy > pOverECutLow_diElectron &&
               p / matchTowerEnergy < pOverECutHigh_diElectron)
                passEmcCut = 1;
        }
        if(nSigmaDedxE > nSigmaDedxCutLowForTof_UPCdiElectron
           && nSigmaDedxE < nSigmaDedxCutHighForTof_UPCdiElectron
           && p > pCutLowForTof_UPCdiElectron
           && p < pCutHighForTof_UPCdiElectron && passTofCut)
            isElectron |= selectionBitTof;
        if(nSigmaDedxE > nSigmaDedxCutLowForEmc_UPCdiElectron
           && nSigmaDedxE < nSigmaDedxCutHighForEmc_UPCdiElectron
           && p > pCutLowForEmc_UPCdiElectron
           && p < pCutHighForEmc_UPCdiElectron && passEmcCut)
            isElectron |= selectionBitEmc;
        if(nSigmaDedxE > nSigmaDedxCutLowForTofAndEmc_UPCdiElectron
           && nSigmaDedxE < nSigmaDedxCutHighForTofAndEmc_UPCdiElectron
           && p > pCutLowForTofAndEmc_UPCdiElectron
           && p < pCutHighForTofAndEmc_UPCdiElectron
           && passTofCut && passEmcCut)
            isElectron |= selectionBitTofAndEmc;

        if(isElectron) {
            //       cout<<" Fully veted electron !!!!!!!!!!!!!!!! "<<endl;
            if(nUPCElectronNodes >= maxNElectronTracks) {
                cout << "gl3TriggerDecider::decide()  WARN! electronTrackArray Full!" << endl;
                break;
            }
            UPCelectronNode[nUPCElectronNodes] = node;
            UPCelectronSelectionBits[nUPCElectronNodes] = isElectron;
            nUPCElectronNodes ++;
        }
    }
    //cout<<" UPC electron nodes in event +++++++++++++++++++ "<<nUPCElectronNodes<<endl;
    for(int i = 0; i < nUPCElectronNodes; i++) {
        for(int j = i + 1; j < nUPCElectronNodes; j++) {
            int i1 = i;
            int i2 = j;

            gl3Node* node1 = UPCelectronNode[i];
            gl3Track* track1 = node1->primaryTrack;
            double p1 = track1->pt * sqrt(1. + pow(track1->tanl, 2));
            gl3Node* node2 = UPCelectronNode[j];
            gl3Track* track2 = node2->primaryTrack;
            double p2 = track2->pt * sqrt(1. + pow(track2->tanl, 2));
            if(p1 < p2) {
                swap(p1, p2);
                swap(i1, i2);
                swap(node1, node2);
                swap(track1, track2);
            }
            if(p1 < p1Cut_diElectron) continue;
            if(track1->q * track2->q > 0 && onlyOppPairs_UPCdiElectron) continue;
            if(fabs(track1->z0 - track2->z0) > vertexZDiffCut_UPCdiElectron) continue;

            double pt1 = track1->pt;
            double px1 = pt1 * cos(track1->psi);
            double py1 = pt1 * sin(track1->psi);
            double pz1 = pt1 * track1->tanl;
            double E1 = sqrt(electronMass * electronMass + p1 * p1);
            double pt2 = track2->pt;
            double px2 = pt2 * cos(track2->psi);
            double py2 = pt2 * sin(track2->psi);
            double pz2 = pt2 * track2->tanl;
            double E2 = sqrt(electronMass * electronMass + p2 * p2);
            double px = px1 + px2;
            double py = py1 + py2;
            double pz = pz1 + pz2;
            double psi = atan2(py, px);
            double pt = sqrt(pow(px, 2) + pow(py, 2));
            double tanl = atan2(pz, pt);
            double invariantMass = sqrt(pow(E1 + E2, 2) - pow(pt, 2) - pow(pz, 2));
            //cout<<"px1:"<<px1<<"  py1:"<<py1<<"  pz1:"<<pz1<<"  E1:"<<E1<<"  px2:"<<px2<<"  py2:"<<py2<<"  pz2:"<<pz2<<"  E2:"<<E2<<"  invarMass:"<<invariantMass<<endl;
            //cout<<"pt:"<<pt<<" pz:"<<pz<<"  E1+E2: "<<E1+E2<<"   Esquared "<<pow(E1+E2,2)<<"  pt2+pz2 "<<pow(pt,2)+pow(pz,2)<<" E2-p2: "<<pow(E1+E2,2)-pow(pt,2)-pow(pz,2)<<"  invarMass:"<<invariantMass<<endl;
            if(invariantMass < invariantMassCutLow_UPCdiElectron) continue;
            if(invariantMass > invariantMassCutHigh_UPCdiElectron) continue;

            UPCdiElectronTriggered = triggerBitUPCdiElectron;

            hlt_diElectronPair UPCdiElePair;
            UPCdiElePair.dau1NodeSN = (int)(node1 - event->getNode(0));
            UPCdiElePair.dau2NodeSN = (int)(node2 - event->getNode(0));
            UPCdiElePair.invariantMass = invariantMass;
            UPCdiElePair.pt = pt;
            UPCdiElePair.psi = psi;
            UPCdiElePair.tanl = tanl;
            UPCdiElePair.dau1SelectionBit = UPCelectronSelectionBits[i1];
            UPCdiElePair.dau2SelectionBit = UPCelectronSelectionBits[i2];
            hlt_UPCdiEP.ePair[hlt_diEP.nEPairs] = UPCdiElePair;
            hlt_UPCdiEP.nEPairs ++;

        }
    }

    hltDecision |= UPCdiElectronTriggered;

    if (UPCdiElectronTriggered) {
        NumOfSucceed[UPCDiElectron]++;
    }
    
    return UPCdiElectronTriggered;
}

bool gl3TriggerDecider::decide_MTDDiMuon()
{
    MTDdiMuonTriggered  = 0;

    LOG(NOTE, "Enter decide_MTDDiMuon");
    if(!triggerOnMTDdiMuon) return false;

    NumOfCalled[MTDDiMuon]++;

    if (!(event->mtd)) {
        LOG(WARN, "Trying to run %s without data", __FUNCTION__);
        return false;
    }
    
    int nMtdHits = event->mtd->mtdEvent->nMtdHits;
    int nMuon = 0;
    for(int i = 0; i < nMtdHits; i++) {
        if(!event->mtd->mtdEvent->mtdhits[i].isTrigger) continue;
        if(event->mtd->mtdEvent->mtdhits[i].hlt_trackId < 0) continue;
        nMuon ++;
    }

    if(nMuon >= 2) {
        MTDdiMuonTriggered = triggerBitMTDdiMuon;
    }

    LOG(NOTE, "Dimuon - %d muon candidates", nMuon);
    LOG(NOTE, "Is dimuon trigger: %d", event->mtd->mtdEvent->isDimuon);
    if(nMuon>=2)
      {
	LOG(NOTE,"Event with id = %d is loosely triggered\n",EventInfo::Instance().eventId);
      }

    hltDecision |= MTDdiMuonTriggered;
    if(MTDdiMuonTriggered) NumOfSucceed[MTDDiMuon]++;
    return MTDdiMuonTriggered;
}

bool gl3TriggerDecider::decide_MTDQuarkonium()
{
    LOG(NOTE, "Enter decide_MTDQuarkonium");
    MTDQuarkoniumTriggered  = 0;
    hlt_MTDQkn.nMTDQuarkonium = 0;
    if(!triggerOnMTDQuarkonium) return false;
    NumOfCalled[MTDQuarkonium]++;

    if (!(event->mtd) || !(event->mtd->mtdEvent)) {
        LOG(WARN, "Trying to run %s without data", __FUNCTION__);
        return false;
    }
    
    vector<int> trkId;
    trkId.reserve(100);
    int nMtdHits = event->mtd->mtdEvent->nMtdHits;
    for(int i = 0; i < nMtdHits; i++)
      {
        if(!event->mtd->mtdEvent->mtdhits[i].isTrigger) continue;
        if(event->mtd->mtdEvent->mtdhits[i].hlt_trackId < 0) continue;

	int hlt_index = event->mtd->mtdEvent->mtdhits[i].hlt_trackId;
	gl3Track* gTrack = event->getGlobalTrack(hlt_index);
        double pt = gTrack->pt;
	if(gTrack->nHits < nHitsCut_MTDQuarkonium) continue;
	if(pt < ptSublead_MTDQuarkonium) continue;

	double dy = event->mtd->mtdEvent->mtdhits[i].delta_y;
	double dz = event->mtd->mtdEvent->mtdhits[i].delta_z;
	if(fabs(dy)>dzTrkHit_MTDQuarkonium  || fabs(dz)>dzTrkHit_MTDQuarkonium) continue;

	trkId.push_back(hlt_index);
      }
   
    const double muMass = 0.10566;
    int NMuon = trkId.size();
    for(int i=0; i<NMuon; i++)
      {
	gl3Track* itrack = event->getGlobalTrack(trkId[i]);
	double ipt = itrack->pt;
	double ipx = ipt * cos(itrack->psi);
	double ipy = ipt * sin(itrack->psi);
	double ipz = ipt * itrack->tanl;
	TLorentzVector muon1;
	muon1.SetXYZM(ipx, ipy, ipz, muMass);
	for(int j=i+1; j<NMuon; j++)
	  {
	    gl3Track* jtrack = event->getGlobalTrack(trkId[j]);
	    double jpt = jtrack->pt;
	    double jpx = jpt * cos(jtrack->psi);
	    double jpy = jpt * sin(jtrack->psi);
	    double jpz = jpt * jtrack->tanl;
	    TLorentzVector muon2;
	    muon2.SetXYZM(jpx, jpy, jpz, muMass);

	    double leadpt = (ipt>jpt) ? ipt : jpt;
	    TLorentzVector jpsi = muon1 + muon2;

	    if(leadpt>ptLead_MTDQuarkonium && 
	       ( (leadpt<2.5 && jpsi.M()>2.5) || leadpt>2.5 ))
	      {
                  HLT_MTDPair& pair = hlt_MTDQkn.MTDQuarkonium[hlt_MTDQkn.nMTDQuarkonium];
                  pair.muonTrackId1 = (ipt>jpt) ? trkId[i] : trkId[j];
                  pair.muonTrackId2 = (ipt<=jpt) ? trkId[i] : trkId[j];
                  pair.pt  = jpsi.Pt();
                  pair.eta = jpsi.Eta();
                  pair.phi = jpsi.Phi();
                  pair.invMass = jpsi.M();
                  hlt_MTDQkn.nMTDQuarkonium++;
                  
                  MTDQuarkoniumTriggered = triggerBitMTDQuarkonium;
                  LOG(NOTE, "Dimuon events: ptlead,M = %f %f", leadpt, jpsi.M());
	      }
	  }
      }

    LOG(NOTE, "%i MTDQuarkonium", hlt_MTDQkn.nMTDQuarkonium);
    
    hltDecision |= MTDQuarkoniumTriggered;

    if(MTDQuarkoniumTriggered)
      {
	LOG(NOTE,"Event id = %d is triggered\n",EventInfo::Instance().eventId);
        NumOfSucceed[MTDQuarkonium]++;
      }

    return MTDQuarkoniumTriggered;
}

bool gl3TriggerDecider::decide_FixedTarget()
{
    FixedTargetTriggered = 0;
    if(!triggerOnFixedTarget) return false;

    NumOfCalled[FixedTarget]++;
    
    float Vx = event->getVertex().Getx();
    float Vy = event->getVertex().Gety();
    float Vz = event->getVertex().Getz();
    float Vr = sqrt(Vx*Vx + Vy*Vy);
    
    if (// Vy > vertexYCutLow_FixedTarget && Vy < vertexYCutHigh_FixedTarget &&
        Vz > vertexZCutLow_FixedTarget && Vz < vertexZCutHigh_FixedTarget &&
        Vr > vertexRCutLow_FixedTarget && Vr < vertexRCutHigh_FixedTarget ) {
        
        // float SumPz = 0.0;
        // int nTracks = event->getNGlobalTracks();
        // for(int i = 0; i < nTracks; i++) {
        //     gl3Track* pTrack = event->getNode(i)->primaryTrack;
        //     if(!pTrack) continue;
        //     float pz = pTrack->pt * pTrack->tanl;
        //     SumPz += pz;
        // }

        // if (SumPz < 0) {        // event goes towards east side
        //     FixedTargetTriggered = triggerBitFixedTarget;
        // }

        FixedTargetTriggered = triggerBitFixedTarget;
        
    }
	
    hltDecision |= FixedTargetTriggered;

    if (FixedTargetTriggered) {
        NumOfSucceed[FixedTarget]++;
    }
    
    return FixedTargetTriggered;
}

bool gl3TriggerDecider::decide_FixedTargetMonitor()
{
    FixedTargetMonitorTriggered = 0;
    if (!triggerOnFixedTargetMonitor) return false;

    NumOfCalled[FixedTargetMonitor]++;

    float Vz = event->getVertex().Getz();

    if (Vz > vertexZCutLow_FixedTargetMonitor &&
        Vz < vertexZCutHigh_FixedTargetMonitor ) {
        FixedTargetMonitorTriggered = triggerBitFixedTargetMonitor;
    }

    hltDecision |= FixedTargetMonitorTriggered;

    if (FixedTargetMonitorTriggered) {
        NumOfSucceed[FixedTargetMonitor]++;
    }
    
    return FixedTargetMonitorTriggered;
}

bool gl3TriggerDecider::decide_BesMonitor()
{
    besMonitorTriggered = 0;
    if (!triggerOnBesMonitor) return false;

    NumOfCalled[BesMonitor]++;
    
    float Vz = event->getVertex().Getz();
    if (fabs(Vz) < vertexZCut_besMonitor &&
        event->getNPrimaryTracks() > nTracksCut_besMonitor) {
        besMonitorTriggered = triggerBitBesMonitor;
    }

    hltDecision |= besMonitorTriggered;

    if (besMonitorTriggered) {
        NumOfSucceed[BesMonitor]++;
    }
    
    return besMonitorTriggered;
}

void gl3TriggerDecider::writeQA()
{
    if(!f1) return;

    int nElectronTracks = 0, nTowers = 0, nMatch = 0;
    gl3Track* electronTrack[maxNElectronTracks];
    gl3EmcTower* emcTower[maxNTowers];
    gl3Track* matchTrack[maxNElectronTracks];
    gl3EmcTower* matchTower[maxNTowers];
    double matchPhiDiff[maxNTowers];
    double matchZEdge[maxNTowers];
    int nTracks = event->getNGlobalTracks();
    if(Debug_vertex) {
        fprintf(f1, "vertex: %e %e %e\n", event->getVertex().Getx(), event->getVertex().Gety(), event->getVertex().Getz());
        fprintf(f1, "lmVertex: %e %e %e\n", event->getLmVertex().Getx(), event->getLmVertex().Gety(), event->getLmVertex().Getz());
    }
    if(Debug_tracks || Debug_matchs)
        for(int i = 0; i < nTracks; i++) {
            gl3Track* track = event->getGlobalTrack(i);
            if(track->nHits < nHitsCut_debug) continue;
            if(track->nDedx < nDedxCut_debug) continue;
            //     if(!track->flag) continue; // reject secondaries
            double p = track->pt * sqrt(1. + pow(track->tanl, 2));
            if(p < pCut_debug) continue;
            if(track->dedx < dedxCutLow_debug) continue;
            if(track->dedx > dedxCutHigh_debug) continue;
            if(nElectronTracks >= maxNElectronTracks) {
                cout << "gl3TriggerDecider::debug()  WARN! electronTrackArray Full!" << endl;
                break;
            }
            double xVertex = track->getPara()->xVertex;
            double yVertex = track->getPara()->yVertex;
            track->updateToClosestApproach(xVertex, yVertex);
            electronTrack[nElectronTracks] = track;
            nElectronTracks ++;
        }
    if(Debug_towers || Debug_matchs)
        for(int i = 0; i < event->emc->getNBarrelTowers(); i++) {
            gl3EmcTower* tower = event->emc->getBarrelTower(i);
            double towerEnergy = tower->getEnergy();
            if(towerEnergy < towerEnergyCut_debug) continue;
            if(nTowers >= maxNTowers) {
                cout << "gl3TriggerDecider::debug()  WARN! towerArray Full!" << endl;
                break;
            }
            emcTower[nTowers] = tower;
            nTowers ++;
        }
    if(Debug_matchs)
        for(int i = 0; i < nElectronTracks; i++)
            for(int j = 0; j < nTowers; j++) {
                gl3EmcTower* tower = emcTower[j];
                gl3Track* track = electronTrack[i];
                double phiDiff, zEdge;
                if(!tower->matchTrack(track, phiDiff, zEdge, matchPhiDiffCut_debug, matchZEdgeCut_debug)) continue;
                matchTrack[nMatch] = electronTrack[i];
                matchTower[nMatch] = emcTower[j];
                matchPhiDiff[nMatch] = phiDiff;
                matchZEdge[nMatch] = zEdge;
                nMatch ++;
            }

    if(Debug_tracks) {
        fprintf(f1, "tracks:  %i\n", nElectronTracks);
        for(int i = 0; i < nElectronTracks; i++) {
            gl3Track* track = electronTrack[i];
            double pt = track->pt;
            double px = pt * cos(track->psi);
            double py = pt * sin(track->psi);
            double pz = pt * track->tanl;
            double p = sqrt(pow(pt, 2) + pow(pz, 2));
            fprintf(f1, "%i  %i  %i  %i  %i  %i  %i  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e\n", track->id, track->flag, track->innerMostRow, track->outerMostRow, track->nHits, track->nDedx, track->q, track->chi2[0], track->chi2[1], track->dedx, track->pt, track->phi0, track->psi, track->r0, track->tanl, track->z0, track->length, track->dpt, track->dpsi, track->dz0, track->eta, track->dtanl, p, px, py, pz);
        }
    }
    if(Debug_towers) {
        fprintf(f1, "towers:    %i\n", nTowers);
        for(int i = 0; i < nTowers; i++) {
            gl3EmcTower* tower = emcTower[i];
            l3EmcTowerInfo* towerInfo = tower->getTowerInfo();
            fprintf(f1, "%i  %i  %i  %i  %e  %e  %e  %e  %e  %e  %e  %e  %e  %i  %e\n", towerInfo->getDaqID(), towerInfo->getSoftID(), towerInfo->getCrate(), towerInfo->getCrateSeq(), towerInfo->getPhi(), towerInfo->getEta(), towerInfo->getEtaMin(), towerInfo->getEtaMax(), towerInfo->getZ(), towerInfo->getZmin(), towerInfo->getZmax(), towerInfo->getGain(), towerInfo->getPedestal(), tower->getADC(), tower->getEnergy());
        }
    }
    if(Debug_matchs) {
        fprintf(f1, "matchs:     %i\n", nMatch);
        for(int i = 0; i < nMatch; i++) {
            gl3Track* track = matchTrack[i];
            gl3EmcTower* tower = matchTower[i];
            l3EmcTowerInfo* towerInfo = tower->getTowerInfo();
            double pt = track->pt;
            double px = pt * cos(track->psi);
            double py = pt * sin(track->psi);
            double pz = pt * track->tanl;
            double p = sqrt(pow(pt, 2) + pow(pz, 2));
            fprintf(f1, "%i  %i  %i  %i  %i  %i  %i  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %e  %i  %i  %i  %i  %e  %e  %e  %e  %e  %e  %e  %e  %e  %i  %e  %e  %e\n", track->id, track->flag, track->innerMostRow, track->outerMostRow, track->nHits, track->nDedx, track->q, track->chi2[0], track->chi2[1], track->dedx, track->pt, track->phi0, track->psi, track->r0, track->tanl, track->z0, track->length, track->dpt, track->dpsi, track->dz0, track->eta, track->dtanl, p, px, py, pz, towerInfo->getDaqID(), towerInfo->getSoftID(), towerInfo->getCrate(), towerInfo->getCrateSeq(), towerInfo->getPhi(), towerInfo->getEta(), towerInfo->getEtaMin(), towerInfo->getEtaMax(), towerInfo->getZ(), towerInfo->getZmin(), towerInfo->getZmax(), towerInfo->getGain(), towerInfo->getPedestal(), tower->getADC(), tower->getEnergy(), matchPhiDiff[i], matchZEdge[i]);
        }
    }
}

void gl3TriggerDecider::writeScalers()
{
    if(f1) {
        fprintf(f1, "bField = %e\n", bField);
        fprintf(f1, "scalerCount = %e\n", scalerCount);
    }
}

void gl3TriggerDecider::printReport()
{
    for (int i = 1; i <= MaxNHLTTriggers; ++i) {
        LOG("THLT", "HLT Trigger statistics [Succeed/Called]: Trg #%2i:%6i/%-i", i, NumOfSucceed[i], NumOfCalled[i]);
    }
}
