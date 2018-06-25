#include "L4HLTTask_cpu.h"
#include "hlt_trg.h"
#include "hlt_btow.h"
#include "hlt_tpc_ca.h"
#include "hlt_gl3.h"
#include "HLT_TOF/hlt_tof.h"
#include "HLT_MTD/hlt_mtd.h"
#include "L4EventDataTypes.h"
#include "hltCaliClient.h"

#include <XMLCFG/SimpleXmlDoc.h>
#include <LIBJML/ThreadManager.h>
#include <rtsLog.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "Rtypes.h"

// -----------------------------------------------------------------------------
L4HLTTask_cpu::L4HLTTask_cpu(int myidx) :
    L4Interface_cpu(myidx),
    HLTParameterDir("/a/hltconfig"), eventCounter(0),
    trg(NULL), btow(NULL), tof(NULL), mtd(NULL), tpc_ca(NULL), gl3(NULL),
    use_gl3(true), use_trg(true), use_btow(true), use_tof(true), use_mtd(true), 
    use_tpc_ca(true) {

    memset(&hlt_cfg, 0, sizeof(hlt_config_t));
    memset(l3AlgoNeededMask, 0, sizeof(l3AlgoNeededMask));
}

// -----------------------------------------------------------------------------
L4HLTTask_cpu::~L4HLTTask_cpu() {
    delete trg;
    delete btow;
    delete tof;
    delete mtd;
    delete tpc_ca;
    delete gl3;
}

// -----------------------------------------------------------------------------
void L4HLTTask_cpu::init() {
    LOG(INFO, "Init the run: code compiled at %s %s", __DATE__, __TIME__);
}

// -----------------------------------------------------------------------------
int L4HLTTask_cpu::sendConfig(SimpleXmlDoc* cfg) {
    TMCP;
    // char tmpfilename[128];
    // sprintf(tmpfilename, "/tmp/kehw_tmpfile_%d", getIdx());
    // int fd = open(tmpfilename, O_CREAT | O_WRONLY);
    // if(fd >= 0) { LOG("INFO", "tmpfile fd = %d", fd); close(fd); }
    // else { LOG("INFO", "failed to open tempfile: %s",strerror(errno));}

    if (!cfg) {
        LOG(ERR, "Get Invalide SimpleXmlDoc");
        // need to think about what to do in case of bad RC config
    }

    memset(&hlt_cfg, 0, sizeof(hlt_config_t));

    hlt_cfg.version = 0;      // FY11 is 0 for now..., see GL3/gl3_standalone.C
    hlt_cfg.seeding_mask = 0; // comes from switches, currently disabled
    hlt_cfg.config_directory = HLTParameterDir;
    hlt_cfg.myId = getIdx();
    hlt_cfg.trgsetupname = cfg->getParam("glb_run.GLB_SETUP_name");
    hlt_cfg.runnumber = cfg->getParamI("glb_run.run_number");
    hlt_cfg.l3_sc.mag_field = cfg->getParamF("glb_run.bField");
    hlt_cfg.l3_sc.drift_velocity_east = cfg->getParamF("glb_run.drift_velocity_east");
    hlt_cfg.l3_sc.drift_velocity_west = cfg->getParamF("glb_run.drift_velocity_west");

    LOG("THLT", "PID%i: bField = %f, drift_velocity = %f", getIdx(), hlt_cfg.l3_sc.mag_field,
        hlt_cfg.l3_sc.drift_velocity_east);

    if (fabs(hlt_cfg.l3_sc.mag_field) < 1.e-8) {
        hlt_cfg.l3_sc.mag_field = -0.5;
        LOG(CRIT, "PID%i: bField is zero !!!!! Using %f", getIdx(), hlt_cfg.l3_sc.mag_field);
    }

    if (fabs(hlt_cfg.l3_sc.drift_velocity_east) < 1.e-8 ||
        fabs(hlt_cfg.l3_sc.drift_velocity_west) < 1.e-8) {
        LOG(CAUTION, "PID%i: drift velocity is zero !!!!! using 5.555", getIdx());
        hlt_cfg.l3_sc.drift_velocity_east = 5.555;
        hlt_cfg.l3_sc.drift_velocity_west = 5.555;
    }

    if (use_trg) {
        delete trg;
        trg = new hlt_trg;
        trg->Run_config(1, &hlt_cfg);
    }

    if (use_btow) {
        delete btow;
        btow = new hlt_btow;
        btow->Run_config(1, &hlt_cfg);
    }

    if (use_tof) {
        delete tof;
        tof = new hlt_tof;
        tof->Run_config(1, &hlt_cfg);
    }

    if (use_mtd) {
        delete mtd;
        mtd = new hlt_mtd;
        mtd->Run_config(1, &hlt_cfg);
    }

    if (use_tpc_ca) {
        delete tpc_ca;
        tpc_ca = new hlt_tpc_ca;
        tpc_ca->Run_config(0, &hlt_cfg);
    }

    if (use_gl3) {
        delete gl3;
        gl3 = new hlt_gl3;
        gl3->Run_config(0, &hlt_cfg);
    }

    memset(l3AlgoNeededMask, 0, sizeof(l3AlgoNeededMask));

    for (int i = 0; i < 64; i++) {
        int used = cfg->getParamI("trg_setup.triggers[%d].used", i);
        LOG(NOTE, "trg: %i, used %i", i, used);
        if (!used) continue;

        char tname[256];
        strcpy(tname, cfg->getParam("trg_setup.triggers[%d].name", i));
        int id = cfg->getParamI("trg_setup.triggers[%d].l3.id", i);

#ifndef OLDCFG
        id = id - 100;
        if (id < 1) {
            id = 1;
            l3AlgoNeededMask[id] |= (1LL << i);
            continue;
        }
#endif

        LOG(NOTE, "PID%i: trigger[%d]:  %s uses hlt trg %d", getIdx(), i, tname, id);

        // The bits in this mask refer to the DAQ trigger id
        // The array index of this mask refer to the l3 trigger id
        //
        // so in the end, l3AlgoNeededMask[id], tells me which triggers need
        // algorithm "id"
        l3AlgoNeededMask[id] |= (1LL << i);

        gl3->gl3->triggerDecider->setTriggersFromRunControl(cfg, i);
    }

    // hlt algo id begin at 1
    for (int i = 1; i <= gl3TriggerDecider::MaxNHLTTriggers; i++) {
        LOG(NOTE, "PID%i: l3AlgoNeeded[%d] = 0x%0llx", getIdx(), i, l3AlgoNeededMask[i]);
    }

    return 0;
}

// -----------------------------------------------------------------------------
int L4HLTTask_cpu::startRun() {
    TMCP;
    eventCounter = 0;

    // // set typical values when there is no calibration server
    // hlt_cfg.hltInitCaliData.innerGain = 3.13867e-08;
    // hlt_cfg.hltInitCaliData.outerGain = 4.28745e-08;

    // hltCaliClient::GetInstance()->CopyCaliResult(hlt_cfg.hltInitCaliData);
    
    // LOG("THLT", "PID%i: beamline.x = %12.6f", getIdx(), hlt_cfg.hltInitCaliData.x);
    // LOG("THLT", "PID%i: beamline.y = %12.6f", getIdx(), hlt_cfg.hltInitCaliData.y);
    // LOG("THLT", "PID%i:  innerGain = %E", getIdx(), hlt_cfg.hltInitCaliData.innerGain);
    // LOG("THLT", "PID%i:  outerGain = %E", getIdx(), hlt_cfg.hltInitCaliData.outerGain);

    // tpc_ca->tpc_ca->setBeamline(hlt_cfg.hltInitCaliData.x,
    //                             hlt_cfg.hltInitCaliData.y);

    // tpc_ca->tpc_ca->setGainParamagers(hlt_cfg.hltInitCaliData.innerGain,
    //                                   hlt_cfg.hltInitCaliData.outerGain);

    // gl3->gl3->setGainPara(hlt_cfg.hltInitCaliData.innerGain,
    //                       hlt_cfg.hltInitCaliData.outerGain);

    // gl3->gl3->setBeamline(hlt_cfg.hltInitCaliData.x,
    //                       hlt_cfg.hltInitCaliData.y);

    if (trg)
        trg->Run_start(hlt_cfg.runnumber);
    if (btow)
        btow->Run_start(hlt_cfg.runnumber);
    if (tof)
        tof->Run_start(hlt_cfg.runnumber);
    if (mtd)
        mtd->Run_start(hlt_cfg.runnumber);
    if (tpc_ca)
        tpc_ca->Run_start(hlt_cfg.runnumber);
    if (gl3)
        gl3->Run_start(hlt_cfg.runnumber);

    return 0;
}

// -----------------------------------------------------------------------------
int L4HLTTask_cpu::stopRun() {
    TMCP;
    LOG(NOTE, "PID%i: Stopping run...", getIdx());

    if (trg)
        trg->Run_stop();
    if (btow)
        btow->Run_stop();
    if (tof)
        tof->Run_stop();
    if (mtd)
        mtd->Run_stop();
    if (tpc_ca)
        tpc_ca->Run_stop();
    if (gl3)
        gl3->Run_stop();

    eventCounter = 0;
    
    return 0;
}

// -----------------------------------------------------------------------------
void L4HLTTask_cpu::process(daqReader* rdr, L4EventData* evt) {
    TMCP_CORE(evt->raw_input_event, L4_MAX_DATA_SZ);

    // get and apply the latest calibration values
    hltCaliClient::GetInstance()->CopyCaliResult(hlt_cfg.hltInitCaliData);

    TMCP;

    tpc_ca->tpc_ca->setBeamline(hlt_cfg.hltInitCaliData.x,
                                hlt_cfg.hltInitCaliData.y);

    tpc_ca->tpc_ca->setGainParamagers(hlt_cfg.hltInitCaliData.innerGain,
                                      hlt_cfg.hltInitCaliData.outerGain);

    gl3->gl3->setGainPara(hlt_cfg.hltInitCaliData.innerGain,
                          hlt_cfg.hltInitCaliData.outerGain);

    gl3->gl3->setBeamline(hlt_cfg.hltInitCaliData.x,
                          hlt_cfg.hltInitCaliData.y);

    LOG(NOTE, "size of L4InitData_t = %d", sizeof(L4InitData_t));
    LOG(NOTE, "size of L4SimuData_t = %d", sizeof(L4SimuData_t));
    LOG(NOTE, "size of L4DeciderData_t = %d", sizeof(L4DeciderData_t));
    LOG(NOTE, "size of L4EventData::init_result = %d, size of L4InitData_t = %d",
        sizeof(L4EventData::init_result), sizeof(L4InitData_t));

    static_assert(sizeof(evt->init_result) > sizeof(L4InitData_t), "L4 initData is too small");

    TMCP;
    memset(evt->init_result, 0, sizeof(evt->init_result));
    // L4InitData_t& initData = *(new(evt->init_result)L4InitData_t);

    TMCP;
    L4InitData_t& initData = *((L4InitData_t*)evt->init_result);
        
    set_hlt_blob(initData.detBlobs[TRG_ID],  (char*)(&initData.trgData),  sizeof(HLTTrgData_t ), "trg_sl3");
    set_hlt_blob(initData.detBlobs[BTOW_ID], (char*)(&initData.btowData), sizeof(HLTTPCData_t ), "btow_sl3");
    set_hlt_blob(initData.detBlobs[TOF_ID],  (char*)(&initData.tofData),  sizeof(HLTMTDData_t ), "tof_sl3");
    set_hlt_blob(initData.detBlobs[MTD_ID],  (char*)(&initData.mtdData),  sizeof(HLTTOFData_t ), "mtd_sl3");
    set_hlt_blob(initData.detBlobs[TPX_ID],  (char*)(&initData.tpcData),  sizeof(HLTBTOWData_t), "tpx_sl3");

    // get some handy ref
    hlt_blob& trg_blob    = initData.detBlobs[TRG_ID];
    hlt_blob& btow_blob   = initData.detBlobs[BTOW_ID];
    hlt_blob& tof_blob    = initData.detBlobs[TOF_ID];
    hlt_blob& mtd_blob    = initData.detBlobs[MTD_ID];
    hlt_blob& tpc_ca_blob = initData.detBlobs[TPX_ID];
    UInt_t& gl3Detectors = initData.evtData.gl3Detectors;
    
    LOG(NOTE, "Handling event... %i", eventCounter);
    eventCounter++;

    initData.evtData.l2TrgMask = rdr->daqbits64_l2;
    initData.evtData.l3TrgMask_fromfile = rdr->daqbits64;

    // read in data
    int got_some = 0;
    int blobs = 0;
    daq_dta* dd = NULL;
    
    gl3Detectors = 0;

    TMCP;
    
    // fill event data here
    if (trg) {
        TMCP;
        trg->Event_start(0);
        TMCP;
        dd = rdr->det("trg")->get("raw");
        if (dd && dd->iterate()) {
            TMCP;
            trg->Event_data(TRG_ID, 1, dd->Void, dd->ncontent, &trg_blob);
            TMCP;
            trg->Event_do(&trg_blob, &blobs);
            gl3Detectors |= 1 << TRG_ID;
        }
    }

    if (btow) {
        TMCP;
        btow->Event_start(0);
        TMCP;
        dd = rdr->det("btow")->get("raw");
        if (dd && dd->iterate()) {
            TMCP;
            btow->Event_data(BTOW_ID, 1, dd->Void, dd->ncontent, &btow_blob);
            TMCP;
            btow->Event_do(&btow_blob, &blobs);
            TMCP;
            gl3Detectors |= 1 << BTOW_ID;
        }
    }

    if (tof) {
        got_some = 0;
        TMCP;
        tof->Event_start(0);
        TMCP;
        dd = rdr->det("tof")->get("raw");
        while (dd && dd->iterate()) {
            got_some = 1;
            TMCP;
            tof->Event_data(TOF_ID, dd->rdo, dd->Void, dd->ncontent);
            TMCP;
        }

        if (got_some) {
            TMCP;
            tof->Event_do(&tof_blob, &blobs);
            TMCP;
            tof_blob.name = "sl3";
            gl3Detectors |= 1 << TOF_ID;
        }
    }

    if (mtd) {
        got_some = 0;
        TMCP;
        mtd->Event_start(0);
        TMCP;
        dd = rdr->det("mtd")->get("raw");
        while (dd && dd->iterate()) {
            got_some = 1;
            TMCP;
            mtd->Event_data(MTD_ID, dd->rdo, dd->Void, dd->ncontent);
            TMCP;
        }

        dd = rdr->det("trg")->get("raw");
        while (dd && dd->iterate()) {
            TMCP;
            mtd->trigger_data(dd->Void);
            TMCP;
        }

        if (got_some) {
            TMCP;
            mtd->Event_do(&mtd_blob, &blobs);
            TMCP;
            mtd_blob.name = "sl3";
            gl3Detectors |= 1 << MTD_ID;
        }
    }

    if (use_tpc_ca) {
        sc_t* sc_p = 0;

        daq_dta* dd_sc = rdr->det("sc")->get();
        if (dd_sc && dd_sc->iterate()) {
            sc_p = (sc_t*)dd_sc->Void;
        }

        if (sc_p) {
            TMCP;
            tpc_ca->Event_start((void*)sc_p->rich_scalers);
            TMCP;
        } else {
            TMCP;
            tpc_ca->Event_start((void*)0);
            TMCP;
        }
        // read in TPX and do what the SL3 does i.e. tracking
        got_some = 0;
        int nSectorHits = 0;
        initData.tpcData.tpcOutput.sectorMask = 0xFFFFFF;

        // if the current event needs other HLT algo, will no to partial tracking
        // if (initData.mtdData.mtdOutput.tpcMask &&
        //     !((initData.evtData.l2TrgMask & l3AlgoNeededMask[gl3TriggerDecider::DiElectron]) ||
        //       (initData.evtData.l2TrgMask & l3AlgoNeededMask[gl3TriggerDecider::DiElectron2Twr]) ||
        //       (initData.evtData.l2TrgMask & l3AlgoNeededMask[gl3TriggerDecider::HeavyFragment]) ||
        //       (initData.evtData.l2TrgMask & l3AlgoNeededMask[gl3TriggerDecider::HLTGood2]))) {
        //     // set it to mtdTpcMask only if the current event triggered by di-muon
        //     initData.tpcData.tpcOutput.sectorMask = initData.mtdData.mtdOutput.tpcMask;
        // }

        LOG(NOTE, "l2Trgmask = %16llX, mtdTpcSectorMask = %llX, tpcSectorMask = %llX",
            initData.evtData.l2TrgMask, initData.mtdData.mtdOutput.tpcMask,
            initData.tpcData.tpcOutput.sectorMask);

        for (int s = 1; s <= 24; s++) {
            if (!(initData.tpcData.tpcOutput.sectorMask & (1 << (s - 1)))) {
                continue;
            }
            dd = rdr->det("tpx")->get("cld_raw", s);
            while (dd && dd->iterate()) {
                got_some = 1;
                // feed sector data to SL3
                TMCP;
                nSectorHits = tpc_ca->Event_data(TPX_ID, dd->sec, dd->Void, dd->ncontent);
                TMCP;
                LOG(NOTE, "Got TPX sector %d, sector hits %d", s, nSectorHits);
            }
        } // sector loop end

        if (got_some) {
            int nTpcTracks = 0;
            TMCP;
            nTpcTracks = tpc_ca->Event_do(&tpc_ca_blob, &blobs);
            TMCP;
            LOG(NOTE, "tpc_ca: %d tracks", nTpcTracks);
            gl3Detectors |= 1 << TPX_ID;
        }
    } // use_tpc_ca end
    LOG(NOTE, "+++ %9d %12d %9d", rdr->event_size,
        tpc_ca->tpc_ca->getNHits(), tpc_ca->tpc_ca->getNTracks());
    LOG(NOTE, "CPU process done");
    TMCP_CORE(NULL, 0);
}

// -----------------------------------------------------------------------------
void L4HLTTask_cpu::finish(L4EventData* evt) {
    TMCP_CORE(evt->raw_input_event, L4_MAX_DATA_SZ);

    TMCP;
    // L4HLTTask_cpu::process should have make the init data
    L4InitData_t& initData = *(reinterpret_cast<L4InitData_t*>(evt->init_result));

    // get some handy ref
    hlt_blob& trg_blob     = initData.detBlobs[TRG_ID];
    hlt_blob& btow_blob    = initData.detBlobs[BTOW_ID];
    hlt_blob& tof_blob     = initData.detBlobs[TOF_ID];
    hlt_blob& mtd_blob     = initData.detBlobs[MTD_ID];
    hlt_blob& tpc_ca_blob  = initData.detBlobs[TPX_ID];

    UInt_t&   gl3Detectors = initData.evtData.gl3Detectors;

    static_assert(sizeof(evt->finish_result) > sizeof(L4FinishData_t), "L4 finish data is too small");
    LOG(NOTE, "size of L4EventData::finish_result = %d, size of L4FinishData_t = %d",
        sizeof(L4EventData::finish_result), sizeof(L4FinishData_t));
    memset(evt->finish_result, 0, sizeof(evt->finish_result));
    // L4FinishData_t& finishData = *(new(evt->finish_result)L4FinishData_t);
    L4FinishData_t& finishData = *((L4FinishData_t*)evt->finish_result);
    
    LOG(NOTE, "finishData.l3TrgMask = 0x%llX", finishData.l3TrgMask);
    
    if (gl3) {
        TMCP;
        gl3->Event_start();

        // load data into gl3
        TMCP;
        if (gl3Detectors & (1 << TRG_ID))
            gl3->Event_data(TRG_ID, 1, trg_blob.buff, trg_blob.bytes);

        TMCP;
        if (gl3Detectors & (1 << BTOW_ID))
            gl3->Event_data(BTOW_ID, 1, btow_blob.buff, btow_blob.bytes);

        TMCP;
        if (gl3Detectors & (1 << TOF_ID))
            gl3->Event_data(TOF_ID, 1, tof_blob.buff, tof_blob.bytes);

        TMCP;
        if (gl3Detectors & (1 << MTD_ID))
            gl3->Event_data(MTD_ID, 1, mtd_blob.buff, mtd_blob.bytes);

        TMCP;
        if (gl3Detectors & (1 << TPX_ID))
            gl3->Event_data(TPX_ID, 1, tpc_ca_blob.buff, tpc_ca_blob.bytes);

        TMCP;
        gl3->gl3->eventNumber = eventCounter;

        TMCP;
        gl3->gl3->finalizeReconstruction();
    }

    // This goes through the hlt algorithms
    for (int algo = 1; algo <= gl3TriggerDecider::MaxNHLTTriggers; algo++) {

        LOG(NOTE, "Check HLT Trigger %2i, l2TrgMask %16llX, l3AlgoNeededMask %16llX",
            algo, initData.evtData.l2TrgMask, l3AlgoNeededMask[algo]);

        UINT64 satisfied = 0;

        if (initData.evtData.l2TrgMask & l3AlgoNeededMask[algo]) {
            // This is satisfied if at least one of the triggers satisfied at level 2
            // is required at HLT, so now I must run algo

            switch (algo) {
            case 1: // always accept!
                TMCP;
                satisfied = 1;
                break;
            case 2: // highpt
                TMCP;
                satisfied = gl3->gl3->triggerDecider->decide_HighPt();
                break;
            case 3: // dielectron
                TMCP;
                satisfied = gl3->gl3->triggerDecider->decide_DiElectron();
                break;
            case 4:
                TMCP;
                satisfied = gl3->gl3->triggerDecider->decide_HeavyFragment();
                break;
            case 5: // always accept with tracks!
                // these are always accepted so add the
                // triggers that need them
                TMCP;
                satisfied = gl3->gl3->triggerDecider->decide_AllEvents();
                break;
            case 6:
                TMCP;
                satisfied = gl3->gl3->triggerDecider->decide_RandomEvents();
                break;
            case 7:
                TMCP;
                satisfied = gl3->gl3->triggerDecider->decide_BesGoodEvents();
                break;
            case 8:
                TMCP;
                satisfied = gl3->gl3->triggerDecider->decide_diV0();
                break;
            case 9:
                TMCP;
                satisfied = gl3->gl3->triggerDecider->decide_MatchedHT2();
                break;
            case 10:
                TMCP;
                satisfied = gl3->gl3->triggerDecider->decide_LowMult();
                break;
            case 11:
                TMCP;
                satisfied = gl3->gl3->triggerDecider->decide_UPCDiElectron();
                break;
            case 12:
                TMCP;
                satisfied = gl3->gl3->triggerDecider->decide_UPCPair();
                break;
            case 13:
                TMCP;
                satisfied = gl3->gl3->triggerDecider->decide_MTDDiMuon();
                break;
            case 14:
                TMCP;
                satisfied = gl3->gl3->triggerDecider->decide_FixedTarget();
                break;
            case 15:
                TMCP;
                satisfied = gl3->gl3->triggerDecider->decide_FixedTargetMonitor();
                break;
            case 16:
                TMCP;
                satisfied = gl3->gl3->triggerDecider->decide_BesMonitor();
                break;
            case 17:
                TMCP;
                satisfied = gl3->gl3->triggerDecider->decide_HLTGood2();
                break;
            case 18:
                TMCP;
                satisfied = gl3->gl3->triggerDecider->decide_MTDQuarkonium();
                break;
            case 19:
                TMCP;
                satisfied = gl3->gl3->triggerDecider->decide_DiElectron2Twr();
                break;
            default: // and all the others...
                TMCP;
                satisfied = ((l3AlgoNeededMask[algo] & initData.evtData.l3TrgMask_fromfile) > 0LL) ? 1 : 0;
            }

            if (satisfied) {
                TMCP;
                finishData.l3TrgMask |= (initData.evtData.l2TrgMask & l3AlgoNeededMask[algo]);
            }
        }

    }

    LOG(NOTE, "l2TrgMask = 0x%0llX, l3TrgMask = 0x%0llX, hltDecision = 0x%llX",
        initData.evtData.l2TrgMask, finishData.l3TrgMask,
        gl3->gl3->triggerDecider->getDecision());

    TMCP;
    gl3->makeNetworkData(finishData.caliData);
    
    int n_gl3_blobs = 0;
    TMCP;
    gl3->fill_blobs(evt->output, &n_gl3_blobs, &finishData);

    // if (gl3->gl3->getNGlobalTracks() > 0) {
    //     TMCP;
    //     gl3->fill_blobs(evt->output, &n_gl3_blobs, &finishData);
    // } else {
    //     TMCP;
    //     // no output, l4Triggertask will stop check the following buffers
    //     LOG(INFO, "skip output, %d", gl3->gl3->getNGlobalTracks());
    //     evt->output[0].buff = NULL;
    //     evt->output[0].bytes = 0;
    //     evt->output[0].name = NULL;
    // }

    TMCP;
    int bytes = 0;
    for (int i = 0; i < n_gl3_blobs; i++) {
        if (evt->output[i].bytes <= 0) {
            LOG(ERR, "PID%i: Blob %d/%d: bytes %d", getIdx(), i, n_gl3_blobs, evt->output[i].bytes);
            continue;
        }

        LOG(NOTE, "GL3: blob %d/%d: %s: bytes %d",
            i, n_gl3_blobs, evt->output[i].name, evt->output[i].bytes);
        bytes += evt->output[i].bytes;
    }

    LOG(NOTE, "GL3: evt %d, bytes %d, blobs %d, nGTrack %d, nPTrack %d",
        eventCounter, bytes, n_gl3_blobs,
        gl3->gl3->getNGlobalTracks(), gl3->gl3->getNPrimaryTracks());

    TMCP_CORE(NULL, 0);
}
