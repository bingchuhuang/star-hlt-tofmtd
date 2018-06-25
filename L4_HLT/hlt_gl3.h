#ifndef _HLT_GL3_H_
#define _HLT_GL3_H_

#include <sys/types.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>

#include <rtsLog.h>
#include <DAQ_READER/daqReader.h>
#include <DAQ_READER/daq_dta.h>
#include <DAQ_TPX/daq_tpx.h>
#include <DAQ_TRG/daq_trg.h>
#include <DAQ_TOF/daq_tof.h>
#include <DAQ_MTD/daq_mtd.h>
#include <DAQ_SC/daq_sc.h>

#include <LIBJML/ThreadManager.h>

#include "hlt_entity.h"
#include <L3_SUPPORT/l3_support.h>

#include "online_tracking_collector.h"
#include "L4EventDataTypes.h"
#include "HLTFormats.h"

//#define TIMING_HLT
#ifdef TIMING_HLT
#include <time.h>
#endif

class hlt_gl3 : public hlt_entity {
public:

#ifdef TIMING_HLT
    double timeNow, timeOld;
    double timeResetEvent, timeReadEmc, timeReadTof, timeReadMtd, timeReadTpc, timeFinalize, timeDecide, timeFill;
    int nTracks, nEvents;
#endif

    hlt_gl3() {
        name = "gl3";
        rts_id = L3_ID;
        gl3 = NULL;
    }

    ~hlt_gl3() {
        delete gl3;
        gl3 = NULL;
    } 

    online_tracking_collector *gl3 ;

    char parametersDirectory[64] ;
    char beamlineFile[128] ;
    char GainParameters[128] ;

    // u_int algorithms_requested ;

    int run_config() {
        if(gl3) delete(gl3) ;
        LOG(NOTE, "  directory %s, field %f", hlt_config.config_directory, hlt_config.l3_sc.mag_field);

        sprintf(parametersDirectory, "%s/", hlt_config.config_directory);
        sprintf(beamlineFile, "%sbeamline", parametersDirectory);
        sprintf(GainParameters, "%sGainParameters", parametersDirectory);

        gl3 = new online_tracking_collector(hlt_config.l3_sc.mag_field,
                                            szGL3_default_mHits,
                                            szGL3_default_mTracks,
                                            parametersDirectory,
                                            beamlineFile) ;
        gl3->trgsetupname = hlt_config.trgsetupname;
        
        // gl3->readGainPara(GainParameters);
        // gl3->setGainPara(hlt_config.hltInitCaliData.innerGain,
        //                  hlt_config.hltInitCaliData.outerGain);
        
        // gl3->setBeamline(hlt_config.hltInitCaliData.x,
        //                  hlt_config.hltInitCaliData.y);


        // needed hlt algorithms/triggers is given by Jeff. See l4HLTTask
        // // do we need this?
        // // check triggers...
        // for(int i = 0; i < 32; i++) {
        //     l3_algos_t *alg = &(hlt_config.hlt_algo[i]) ;

        //     if(alg->rc.lx.id) ;
        //     else continue ; // nixed

        //     // only the following and _no_ other triggers will be necessary
        //     // in this run...

        //     LOG(INFO, "  hlt %4d: \"%s\" [RC %s]: HLT mask 0x%08X [RC 0x%016llX]: params: %d; %d, %d, %d, %d, %d; %f, %f, %f, %f, %f",
        //         alg->id, alg->name, alg->rc.name,
        //         alg->hlt_mask, alg->rc.daq_TrgId,
        //         alg->rc.lx.specialProcessing,
        //         alg->rc.lx.userInt[0],
        //         alg->rc.lx.userInt[1],
        //         alg->rc.lx.userInt[2],
        //         alg->rc.lx.userInt[3],
        //         alg->rc.lx.userInt[4],
        //         alg->rc.lx.userFloat[0],
        //         alg->rc.lx.userFloat[1],
        //         alg->rc.lx.userFloat[2],
        //         alg->rc.lx.userFloat[3],
        //         alg->rc.lx.userFloat[4]
        //         ) ;

        //     gl3->triggerDecider->setTriggersFromRunControl(alg);
        // }

        return 0 ;
    }

    int run_start() {
        // if(hlt_config.standalone) gl3->triggerDecider->setQA("qa") ;

        // do this in constructor
        // gl3->createNetworkThread();

#ifdef TIMING_HLT
        timeResetEvent = 0.;
        timeReadEmc = 0.;
        timeReadTof = 0.;
	timeReadMtd = 0.;
        timeReadTpc = 0.;
        timeFinalize = 0.;
        timeDecide = 0.;
        timeFill = 0.;
        nTracks = 0;
        nEvents = 0;
#endif

        return 0 ;
    }

    int run_stop() {
        // gl3->killNetworkThread();
        
        // gl3->triggerDecider->flushQA() ;
        // gl3->triggerDecider->closeQA() ;
        gl3->triggerDecider->printReport();
        // gl3->writeBeamline(beamlineFile, run_number);
        // gl3->CalidEdx(GainParameters, run_number);

#ifdef TIMING_HLT
        printf("nEvents:     %i \n", nEvents);
        printf("nTracks:     %i \n", nTracks);
        printf("\t\t time \t\t per evt (ms) \t per track (micro s) \n");
        printf("timeResetEvent:\t %f\t %f\t %f\n", timeResetEvent, timeResetEvent / nEvents * 1000., timeResetEvent / nTracks * 1000000.);
        printf("timeReadEmc:\t %f\t %f\t %f\n", timeReadEmc, timeReadEmc / nEvents * 1000., timeReadEmc / nTracks * 1000000.);
        printf("timeReadTof:\t %f\t %f\t %f\n", timeReadTof, timeReadTof / nEvents * 1000., timeReadTof / nTracks * 1000000.);
        printf("timeReadMtd:\t %f\t %f\t %f\n", timeReadMtd, timeReadMtd / nEvents * 1000., timeReadMtd / nTracks * 1000000.);
        printf("timeReadTpc:\t %f\t %f\t %f\n", timeReadTpc, timeReadTpc / nEvents * 1000., timeReadTpc / nTracks * 1000000.);
        printf("timeFinalize:\t %f\t %f\t %f\n", timeFinalize, timeFinalize / nEvents * 1000., timeFinalize / nTracks * 1000000.);
        printf("timeDecide:\t %f\t %f\t %f\n", timeDecide, timeDecide / nEvents * 1000., timeDecide / nTracks * 1000000.);
        printf("timeFill:\t %f\t %f\t %f\n", timeFill, timeFill / nEvents * 1000., timeFill / nTracks * 1000000.);
        double totalTime = timeResetEvent + timeReadEmc + timeReadTof + timeReadMtd + timeReadTpc + timeFinalize + timeDecide + timeFill;
        printf("totalTime:\t %f\t %f\t %f\n", totalTime, totalTime / nEvents * 1000., totalTime / nTracks * 1000000.);
#ifdef TIMING_GL3
        printf("---------------------\n");
        printf("timeMakeNodes:\t %f\t %f\t %f\n", gl3->timeMakeNodes, gl3->timeMakeNodes / nEvents * 1000., gl3->timeMakeNodes / nTracks * 1000000.);
        printf("timeMatchEMC:\t %f\t %f\t %f\n", gl3->timeMatchEMC, gl3->timeMatchEMC / nEvents * 1000., gl3->timeMatchEMC / nTracks * 1000000.);
        printf("timeMatchTOFg:\t %f\t %f\t %f\n", gl3->timeMatchTOFg, gl3->timeMatchTOFg / nEvents * 1000., gl3->timeMatchTOFg / nTracks * 1000000.);
        printf("timeMakeVertex:\t %f\t %f\t %f\n", gl3->timeMakeVertex, gl3->timeMakeVertex / nEvents * 1000., gl3->timeMakeVertex / nTracks * 1000000.);
        printf("timeMakePrimaries:\t %f\t %f\t %f\n", gl3->timeMakePrimaries, gl3->timeMakePrimaries / nEvents * 1000., gl3->timeMakePrimaries / nTracks * 1000000.);
        printf("timeMatchTOFp:\t %f\t %f\t %f\n", gl3->timeMatchTOFp, gl3->timeMatchTOFp / nEvents * 1000., gl3->timeMatchTOFp / nTracks * 1000000.);
        printf("timeMeanDedx:\t %f\t %f\t %f\n", gl3->timeMeanDedx, gl3->timeMeanDedx / nEvents * 1000., gl3->timeMeanDedx / nTracks * 1000000.);
#endif  // TIMING_GL3
#endif  // TIMING_HLT

        return 0 ;
    }

    int event_start(void *hlt_mask_p = 0) {
#ifdef TIMING_HLT
        timeOld = (double)(clock()) / CLOCKS_PER_SEC;
#endif
        gl3->resetEvent() ;

#ifdef TIMING_HLT
        timeNow = (double)(clock()) / CLOCKS_PER_SEC;
        timeResetEvent += timeNow - timeOld;
#endif

        // // do we really use this mask? kehw
        // // this is the mask of requested algorithms!
        // if(hlt_mask_p == 0) {
        //     algorithms_requested = 0xFFFFFFFF ; // ALL, knowing nothing better
        // } else {
        //     algorithms_requested = *((u_int *)hlt_mask_p) ;
        // }

        return 0 ;
    }

    int event_data(int rts_id, int sector, void *buff, int bytes, hlt_blob* blob = NULL) {
        int ret ;

        LOG(DBG, "%d: %d %d %d", evt_counter, rts_id, sector, bytes) ;
        if(!buff) return -1;

        switch(rts_id) {
        case TPX_ID :
            // read 24 sectors at a time
            ret = gl3->readTPCCA((HLTTPCData_t*)buff);
            break ;
        case TRG_ID :
            gl3->trigger = ((gl3_trg_send_t *)buff)->daq_TrgId ;
            gl3->eventNumber = ((gl3_trg_send_t *)buff)->eventNumber ;
            //Tonko: this is gone now! -- use raw data only
            //  gl3->emc->readFromGl3Trg((gl3_trg_send_t *)buff);
            break ;
        case BTOW_ID :
            gl3->emc->readRawBtowers((btow_t *)buff);
            break ;
        case TOF_ID :
            gl3->tof->readFromTofMachine((char*)buff);
            break ;
	case MTD_ID :
	    gl3->mtd->readFromMtdMachine((char*)buff);
	    break;
        default :
            LOG(ERR, "Unknown detector rts_id %d, sector %d", rts_id, sector) ;
            break ;
        }

        return 0 ;
    }

    int event_do(hlt_blob *out, int *blobs) {
        // this func is not used right now --- by yiguo
        int decision ;

        // gl3->finalizeReconstruction() ;
        // //gl3->finalizeReconstruction2() ; // use CA track

        // // USE algoritms_requested here!
        // decision = gl3->triggerDecider->decide(gl3->eventNumber) ;

        // fill_blobs(out, blobs);
        
        return 0;
    }

    int fill_blobs(hlt_blob *out, int *blobs, L4FinishData_t* fData)
    {
        ThreadDesc* threadDesc = threadManager.attach();
    
        int bytes ;

        //--------fill global track--------//
        TMCP;
        bytes = gl3->FillGlobTracks(sizeof(HLT_GT), (char*)&fData->store_GT);
        LOG(DBG, "bytes %d, globle tracks %d", bytes, gl3->getNGlobalTracks()) ;
        TMCP;
        out[0].buff = (char*)&fData->store_GT;
        out[0].name = "gl3/HLT_GT" ;
        out[0].bytes = bytes;

        //---------fill primary tracks-------//
        TMCP;
        bytes = gl3->FillPrimTracks(sizeof(HLT_PT), (char*)&fData->store_PT) ;
        LOG(DBG, "bytes %d, primary tracks %d", bytes, gl3->getNPrimaryTracks()) ;
        TMCP;
        out[1].buff = (char*)&fData->store_PT;
        out[1].name = "gl3/HLT_PT";
        out[1].bytes = bytes ;

        //--------fill event---------//
        TMCP;
        bytes = gl3->FillEvent(sizeof(HLT_EVE), (char*)&fData->store_EVE);
        LOG(DBG, "bytes %d, HLTEvent", bytes) ;
        TMCP;
        out[2].buff = (char*)&fData->store_EVE ;
        out[2].name = "gl3/HLT_EVE" ;
        out[2].bytes = bytes ;

        //----------fill Tof hits------------//
        TMCP;
        bytes = gl3->FillTofHits(sizeof(HLT_TOF), (char*)&fData->store_TOF) ;
        LOG(DBG, "bytes %d", bytes) ;
        TMCP;
        out[3].buff = (char*)&fData->store_TOF;
        out[3].name = "gl3/HLT_TOF" ;
        out[3].bytes = bytes ;

        //----------fill pvpd Hits------------//
        TMCP;
        bytes = gl3->FillPvpdHits(sizeof(HLT_PVPD), (char*)&fData->store_PVPD) ;
        LOG(DBG, "bytes %d", bytes) ;
        TMCP;
        out[4].buff = (char*)&fData->store_PVPD ;
        out[4].name = "gl3/HLT_PVPD" ;
        out[4].bytes = bytes ;

        //-----------fill emc towers -----------//
        TMCP;
        bytes = gl3->FillEmc(sizeof(HLT_EMC), (char*)&fData->store_EMC) ;
        LOG(DBG, "bytes %d", bytes) ;
        TMCP;
        out[5].buff = (char*)&fData->store_EMC ;
        out[5].name = "gl3/HLT_EMC" ;
        out[5].bytes = bytes ;

        //----------fill Nodes------------//
        TMCP;
        bytes = gl3->FillNodes(sizeof(HLT_NODE), (char*)&fData->store_NODE);
        LOG(DBG, "bytes %d", bytes) ;
        TMCP;
        out[6].buff = (char*)&fData->store_NODE ;
        out[6].name = "gl3/HLT_NODE" ;
        out[6].bytes = bytes ;

        //-----------fill di electrons---------//
        TMCP;
        bytes = sizeof(HLT_DIEP) + (gl3->triggerDecider->hlt_diEP.nEPairs - 1000) * sizeof(hlt_diElectronPair);
        TMCP;
        memcpy(&fData->hlt_diEP, &gl3->triggerDecider->hlt_diEP, bytes);
        TMCP;
        out[7].buff = (char*)&fData->hlt_diEP;
        out[7].name = "gl3/HLT_DIEP";
        out[7].bytes = bytes;

        //-----------fill high pt---------//
        TMCP;
        bytes = sizeof(HLT_HIPT) + (gl3->triggerDecider->hlt_hiPt.nHighPt - 1000) * sizeof(int);
        TMCP;
        memcpy(&fData->hlt_hiPt, &gl3->triggerDecider->hlt_hiPt, bytes);
        TMCP;
        out[8].buff = (char*)&fData->hlt_hiPt;
        out[8].name = "gl3/HLT_HIPT";
        out[8].bytes = bytes;

        //-----------fill heavy fragment---------//
        TMCP;
        bytes = sizeof(HLT_HF) + (gl3->triggerDecider->hlt_hF.nHeavyFragments - 1000) * sizeof(int);
        TMCP;
        memcpy(&fData->hlt_hF, &gl3->triggerDecider->hlt_hF, bytes);
        TMCP;
        out[9].buff = (char*)&fData->hlt_hF;
        out[9].name = "gl3/HLT_HF";
        out[9].bytes = bytes;

        //-----------fill matched ht2---------//
        TMCP;
        bytes = sizeof(HLT_HT2) + (gl3->triggerDecider->hlt_hT2.nPairs - 1000) * sizeof(hlt_MatchedHT2);
        TMCP;
        memcpy(&fData->hlt_hT2, &gl3->triggerDecider->hlt_hT2, bytes);
        TMCP;
        out[10].buff = (char*)&fData->hlt_hT2;
        out[10].name = "gl3/HLT_HT2";
        out[10].bytes = bytes;

        //---------------fill Low Mult UU------------//
        //bytes = sizeof(HLT_LM);
        //out[11].buff = (char *)&gl3->triggerDecider->hlt_lm;
        //out[11].name = "gl3/HLT_LM";
        //out[11].bytes = bytes;

        //-------fill UPCdielectron-------//
        TMCP;
        bytes = sizeof(HLT_DIEP) + (gl3->triggerDecider->hlt_UPCdiEP.nEPairs - 1000) * sizeof(hlt_diElectronPair);
        TMCP;
        memcpy(&fData->hlt_UPCdiEP, &gl3->triggerDecider->hlt_UPCdiEP, bytes);
        TMCP;
        out[11].buff = (char*)&fData->hlt_UPCdiEP;
        out[11].name = "gl3/HLT_UPCDIEP";
        out[11].bytes = bytes;

        //-------fill UPCpair-------//
        TMCP;
        bytes = sizeof(HLT_RHO) + (gl3->triggerDecider->hlt_UPCrho.nRhos - 1000) * sizeof(hlt_diPionPair);
        TMCP;
        memcpy(&fData->hlt_UPCrho, &gl3->triggerDecider->hlt_UPCrho, bytes);
        TMCP;
        out[12].buff = (char*)&fData->hlt_UPCrho;
        out[12].name = "gl3/HLT_UPCRHO";
        out[12].bytes = bytes;

        //-----fill MTD hits------//
	//Currently, only the MTD hits are stored
        TMCP;
        bytes = gl3->FillMtdHits(sizeof(HLT_MTD), (char*)&fData->store_MTD) ;
        TMCP;
        out[13].buff = (char*)&fData->store_MTD;
        out[13].name = "gl3/HLT_MTDDIMU";
        out[13].bytes = bytes;

        //-----fill MTD quarkonium ------//
        TMCP;
        bytes = sizeof(int) + (gl3->triggerDecider->hlt_MTDQkn.nMTDQuarkonium) * sizeof(HLT_MTDPair);
        TMCP;
        memcpy(&fData->hlt_MTDQkn, &gl3->triggerDecider->hlt_MTDQkn, bytes);
        TMCP;
        out[14].buff = (char*)&fData->hlt_MTDQkn;
        out[14].name = "gl3/HLT_MTDQuarkonium";
        out[14].bytes = bytes;
        
        //-----------fill DiElectron2Twr ---------//
        TMCP;
        bytes = sizeof(HLT_DIEP) + (gl3->triggerDecider->hlt_diEP2Twr.nEPairs - 1000) * sizeof(hlt_diElectronPair);
        TMCP;
        memcpy(&fData->hlt_diEP2Twr, &gl3->triggerDecider->hlt_diEP2Twr, bytes);
        TMCP;
        out[15].buff = (char*)&fData->hlt_diEP2Twr;
        out[15].name = "gl3/HLT_DIEP2Twr";
        out[15].bytes = bytes;

        *blobs = 16;

        return *blobs;
    }

    size_t makeNetworkData(hltCaliData& caliData) {
        return gl3->makeNetworkData(run_number, caliData);
    }
    
private:
    // NOTE: this determines the max amount of data in the current
    // scheme -- look at "fillTracks" above
    // char store_GT[sizeof(HLT_GT)] ; 
    // char store_PT[sizeof(HLT_PT)] ;
    // char store_EVE[sizeof(HLT_EVE)] ;
    // char store_TOF[sizeof(HLT_TOF)] ;
    // char store_MTD[sizeof(HLT_MTD)] ;
    // char store_PVPD[sizeof(HLT_PVPD)] ;
    // char store_EMC[sizeof(HLT_EMC)] ;
    // char store_NODE[sizeof(HLT_NODE)] ;
} ;

#endif
