#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <DAQ_READER/daqReader.h>
#include <DAQ_READER/daq_dta.h>
#include <DAQ_TPX/daq_tpx.h>
#include <DAQ_TRG/daq_trg.h>
#include <DAQ_TOF/daq_tof.h>
#include <DAQ_MTD/daq_mtd.h>
#include <DAQ_BTOW/daq_btow.h>
#include <DAQ_SC/daq_sc.h>
#include <XMLCFG/SimpleXmlDoc.h>
#include <rtsLog.h>

#include "l4HLTTask.h"
#include "profiler.hh"
#include "hlt_trg.h"
#include "hlt_btow.h" 
#include "HLT_TOF/hlt_tof.h"
#include "HLT_MTD/hlt_mtd.h"
#include "hlt_tpc_ca.h"
#include "hlt_gl3.h"
#include "udp_client.h"
#include "EventInfo.h"

PROFILER_DECLARE;

// -----------------------------------------------------------------------------
L4HLTTask::L4HLTTask()
{
	//HLTParameterDir = "/RTS/hlt_home/HLT/L4test/20110602164432/";
	HLTParameterDir = "/a/hltconfig";

	memset(&hlt_cfg, 0, sizeof(hlt_config_t));
	memset(&trg_blob, 0, sizeof(hlt_blob));
	memset(&btow_blob, 0, sizeof(hlt_blob));
	memset(&tof_blob, 0, sizeof(hlt_blob));
	memset(&mtd_blob, 0, sizeof(hlt_blob));
	memset(&tpc_ca_blob, 0, sizeof(hlt_blob));

	trg        = NULL;
	btow       = NULL;
	tof        = NULL;
	mtd        = NULL;
	tpc_ca     = NULL;
	gl3        = NULL;

	use_gl3    = true;
	use_trg    = true;
	use_btow   = true;
	use_tof    = true;
	use_mtd    = true;
	use_tpc_ca = true;

	eventCounter = 0;
}

// -----------------------------------------------------------------------------
L4HLTTask:: ~L4HLTTask()
{
	delete trg;
	delete btow;
	delete tof;
	delete mtd;
	delete tpc_ca;
	delete gl3;
}

// -----------------------------------------------------------------------------
int L4HLTTask::configureRun(SimpleXmlDoc *cfg) {
	LOG(NOTE, "Configuring run...");

        if (!cfg) {
            LOG(ERR, "Get Invalide SimpleXmlDoc");
            // need to think about what to do in case of bad RC config
        }

        hlt_cfg.version                   = 0 ; // FY11 is 0 for now..., see GL3/gl3_standalone.C
	hlt_cfg.seeding_mask              = 0 ; // comes from switches, currently disabled
	hlt_cfg.config_directory          = HLTParameterDir;
        hlt_cfg.myId                      = getMyId();
        hlt_cfg.runnumber                 = cfg->getParamI("glb_run.run_number");
        hlt_cfg.l3_sc.mag_field           = cfg->getParamF("glb_run.bField");
	hlt_cfg.l3_sc.drift_velocity_east = cfg->getParamF("glb_run.drift_velocity_east");
	hlt_cfg.l3_sc.drift_velocity_west = cfg->getParamF("glb_run.drift_velocity_west");

        LOG("THLT", "PID%i: bField = %f, drift_velocity = %f",
            getMyId(), hlt_cfg.l3_sc.mag_field, hlt_cfg.l3_sc.drift_velocity_east);

	if(fabs(hlt_cfg.l3_sc.mag_field)<1.e-8){
		LOG(CRIT,"PID%i: bField is zero !!!!!",getMyId());
	}

	if(fabs(hlt_cfg.l3_sc.drift_velocity_east)<1.e-8 || fabs(hlt_cfg.l3_sc.drift_velocity_west)<1.e-8){
	    LOG(CAUTION,"PID%i: drift velocity is zero !!!!! using default value",getMyId());
	    hlt_cfg.l3_sc.drift_velocity_east = 5.555;
	    hlt_cfg.l3_sc.drift_velocity_west = 5.555;
	}

	if(use_trg) {
		trg = new hlt_trg ;
		trg->Run_config(1,&hlt_cfg) ;
	}

	if(use_btow) {
		btow = new hlt_btow ;
		btow->Run_config(1,&hlt_cfg) ;
	}

	if(use_tof) {
		tof = new hlt_tof ;
		tof->Run_config(1,&hlt_cfg) ;
	}

	if(use_mtd) {
	        mtd = new hlt_mtd ;
	        mtd->Run_config(1,&hlt_cfg) ;
	}

	if (use_tpc_ca) {
		tpc_ca = new hlt_tpc_ca;
		tpc_ca->Run_config(0, &hlt_cfg);
	}

	if(use_gl3) {
		gl3 = new hlt_gl3 ;
		gl3->Run_config(0,&hlt_cfg) ;
	}

	memset(l3AlgoNeededMask, 0, sizeof(l3AlgoNeededMask));

	for(int i=0;i<64;i++) {
		int used = cfg->getParamI("trg_setup.triggers[%d].used",i);
		if(!used) continue;

		char tname[256];
		strcpy(tname, cfg->getParam("trg_setup.triggers[%d].name",i));
		int id = cfg->getParamI("trg_setup.triggers[%d].l3.id",i);

#ifndef OLDCFG
		id = id - 100;
		if(id < 1) {
                    id = 1;
		    l3AlgoNeededMask[id] |= (1LL << i);
                    continue;
                }
#endif

		LOG(NOTE, "PID%i: trigger[%d]:  %s uses hlt trg %d",getMyId(), i, tname, id);

		// The bits in this mask refer to the DAQ trigger id
		// The array index of this mask refer to the l3 trigger id
		//
		// so in the end, l3AlgoNeededMask[id], tells me which triggers need
		// algorithm "id"
		l3AlgoNeededMask[id] |= (1LL << i);

		gl3->gl3->triggerDecider->setTriggersFromRunControl(cfg, i);
	}

	// hlt algo id begin at 1
	for(int i = 1; i <= gl3TriggerDecider::MaxNHLTTriggers; i++) {
		LOG("THLT", "PID%i: l3AlgoNeeded[%d] = 0x%0llx",getMyId(), i, l3AlgoNeededMask[i]);
	}

	return 0;
}

int L4HLTTask::startRun() {
	LOG(NOTE, "PID%i: Starting run...", getMyId());

	eventCounter = 0;

#ifndef L4STANDALONE
        // in standalone mode, do not talk to calibration server
	char header[] = "INIT";
	size_t caliDataLength = sizeof(hltInitialCalib);
	LOG(NOTE, "PID%i: INIT Data length = %u bytes", getMyId(), caliDataLength);
	int ret = -1;
	int *tmp;
	tmp = new int;
        // wait a random time from 0~0.02s
	//unsigned int seed = udp_client::GetInstance()->listenPort % 50000 * 1000;
	uintptr_t seed = (uintptr_t)tmp;	
	delete tmp;
	LOG(DBG,"seed %i", seed);
	srand((unsigned int)seed);
	long waitTime = (long)(rand()*2.e7/RAND_MAX); // nano sec
	struct timespec timeVal1,timeVal2;
	timeVal1.tv_sec = 0;
	timeVal1.tv_nsec = waitTime; 

	if(nanosleep(&timeVal1, &timeVal2) < 0) {
	    LOG(ERR, "PID%i: Nano sleep system call failed", getMyId());
	}
	LOG(DBG,"wait %i ns", waitTime);

	for(int i=0; i<5 ; i++) {
	    ret = udp_client::GetInstance()->SendReceive(header, sizeof(header),
		    (void*)&(hlt_cfg.hltInitCaliData), caliDataLength);
	    if(ret>=0) break;
	    LOG(WARN,"PID%i: retry %i", getMyId(), i);
	}

	struct timespec timeVal3,timeVal4;
	timeVal3.tv_sec = 0;
	timeVal3.tv_nsec = 25000000-waitTime; // make sure sleep for 0.25s in total

	if(nanosleep(&timeVal3, &timeVal4) < 0) {
	    LOG(ERR, "PID%i: Nano sleep system call failed", getMyId());
	}

	if(ret<0) {
            LOG(CRIT,"PID%i: after 5 tries still can't receive from server",getMyId());
            // set a typical value if the client fail
            hlt_cfg.hltInitCaliData.innerGain = 3.13867e-08;
            hlt_cfg.hltInitCaliData.outerGain = 4.28745e-08;
        }
#else  /* L4STANDALONE */
        // set typical values when there is no calibration server
        hlt_cfg.hltInitCaliData.innerGain = 3.13867e-08;
        hlt_cfg.hltInitCaliData.outerGain = 4.28745e-08;
#endif /* L4STANDALONE */

        LOG("THLT", "PID%i: >>> beamline.x = %12.6f", getMyId(), hlt_cfg.hltInitCaliData.x);
	LOG("THLT", "PID%i: >>> beamline.y = %12.6f", getMyId(), hlt_cfg.hltInitCaliData.y);
	LOG("THLT", "PID%i: >>>  innerGain = %E", getMyId(), hlt_cfg.hltInitCaliData.innerGain);
	LOG("THLT", "PID%i: >>>  outerGain = %E", getMyId(), hlt_cfg.hltInitCaliData.outerGain);

        tpc_ca->tpc_ca->setBeamline(hlt_cfg.hltInitCaliData.x,
                                    hlt_cfg.hltInitCaliData.y);
        tpc_ca->tpc_ca->setGainParamagers(hlt_cfg.hltInitCaliData.innerGain,
                                          hlt_cfg.hltInitCaliData.outerGain);

        gl3->gl3->setGainPara(hlt_cfg.hltInitCaliData.innerGain,
                              hlt_cfg.hltInitCaliData.outerGain);
        
        gl3->gl3->setBeamline(hlt_cfg.hltInitCaliData.x,
                              hlt_cfg.hltInitCaliData.y);

#ifndef L4STANDALONE
	//gl3->gl3->listenPort = hlt_cfg.hltInitCaliData.port;
        gl3->gl3->createNetworkThread();
#endif  // L4STANDALONE

	if(trg)    trg->Run_start(hlt_cfg.runnumber);
	if(btow)   btow->Run_start(hlt_cfg.runnumber);
	if(tof)    tof->Run_start(hlt_cfg.runnumber);
	if(mtd)    mtd->Run_start(hlt_cfg.runnumber);
	if(tpc_ca) tpc_ca->Run_start(hlt_cfg.runnumber);
	if(gl3)    gl3->Run_start(hlt_cfg.runnumber);

	return 0;
}

UINT64 L4HLTTask::handleEvent(daqReader *rdr, hlt_blob *blobs_out) {
	eventCounter++;
	LOG(NOTE, "Handling event... %i", eventCounter);

	UINT64 l3TrgMask = 0LL;
	UINT64 l3TrgMask_fromfile = rdr->daqbits64;
	UINT64 l2TrgMask = rdr->daqbits64_l2;

	int VAL = PROFILER(0);

	// read in data
	int      got_some = 0;
	int      blobs    = 0;
	daq_dta* dd       = NULL;

	unsigned int gl3_detectors = 0;
        EventInfo::Instance().Reset();
        
	if(trg) {
            trg->Event_start(0) ;

            dd = rdr->det("trg")->get("raw") ;
            if(dd && dd->iterate()) {
                trg->Event_data(TRG_ID, 1,dd->Void,dd->ncontent) ;
                trg->Event_do(&trg_blob, &blobs) ;
                gl3_detectors |= 1 << TRG_ID ;
            }
	}


	VAL = PROFILER(VAL);

	if(btow) {
		btow->Event_start(0) ;

		dd = rdr->det("btow")->get("raw") ;
		if(dd && dd->iterate()) {
			btow->Event_data(BTOW_ID,1,dd->Void,dd->ncontent) ;
			btow->Event_do(&btow_blob, &blobs) ;
			gl3_detectors |= 1<<BTOW_ID ;
		}
	}

	if(tof) {
		got_some = 0 ;
		tof->Event_start(0) ;
		dd = rdr->det("tof")->get("raw") ;
		while(dd && dd->iterate()) {
			got_some = 1 ;
			tof->Event_data(TOF_ID, dd->rdo,dd->Void,dd->ncontent) ;
		}

		if(got_some) {
			tof->Event_do(&tof_blob, &blobs) ;
			tof_blob.name = "sl3" ;
			gl3_detectors |= 1 << TOF_ID ;
		}
	}

	if(mtd) {
	        got_some = 0 ;
		mtd->Event_start(0);
		dd = rdr->det("mtd")->get("raw");
		while(dd && dd->iterate()) {
                    got_some = 1 ;
                    mtd->Event_data(MTD_ID, dd->rdo, dd->Void, dd->ncontent);
		}

		dd = rdr->det("trg")->get("raw") ;
		while(dd && dd->iterate()) {
                    mtd->trigger_data(dd->Void);
		}
		
		if(got_some) {
                    mtd->Event_do(&mtd_blob, &blobs) ;
                    mtd_blob.name = "sl3";
                    gl3_detectors |= 1 << MTD_ID;
		}

                // if the current event needs other HLT algo, will no to partial tracking
                if ( (l2TrgMask & l3AlgoNeededMask[gl3TriggerDecider::DiElectron])     ||
                     (l2TrgMask & l3AlgoNeededMask[gl3TriggerDecider::DiElectron2Twr]) ||
                     (l2TrgMask & l3AlgoNeededMask[gl3TriggerDecider::HeavyFragment])  ||
                     (l2TrgMask & l3AlgoNeededMask[gl3TriggerDecider::HLTGood2]) ) {

                    EventInfo::Instance().tpcSectorMask = 0xFFFFFF;
                }
	}

	
        // VAL = PROFILER(VAL);

	if (use_tpc_ca) {
		sc_t *sc_p = 0 ;		

		daq_dta *dd_sc = rdr->det("sc")->get() ;
		if(dd_sc && dd_sc->iterate()) {
			sc_p = (sc_t *) dd_sc->Void ;
		}

		if (sc_p) tpc_ca->Event_start((void *)sc_p->rich_scalers);
		else tpc_ca->Event_start((void *)0);

		// read in TPX and do what the SL3 does i.e. tracking
		int got_some = 0;
		int nSectorHits = 0;
                LOG(NOTE, ">>> %16llX %llX", l2TrgMask, EventInfo::Instance().tpcSectorMask);
		for(int s=1;s<=24;s++) {
		  if (!( EventInfo::Instance().tpcSectorMask & (1<<(s-1)) )) continue;
                    dd = rdr->det("tpx")->get("cld_raw",s);
                    while(dd && dd->iterate()) {
                        got_some = 1;
                        // feed sector data to SL3
                        nSectorHits = tpc_ca->Event_data(TPX_ID,dd->sec,dd->Void,dd->ncontent) ;
                        LOG(NOTE,"Got TPX sector %d, sector hits %d", s, nSectorHits) ;
                    }                    
		} // sector loop end

		if (got_some) {
                    int nTpcTracks = 0;
                    nTpcTracks = tpc_ca->Event_do(&tpc_ca_blob, &blobs);
                    LOG(NOTE, "tpc_ca: %d tracks", nTpcTracks);
                    gl3_detectors |= 1 << TPX_ID ;
#ifdef COLLECT_EVENT_INFO
                EventInfo::Instance().tpcNHits = tpc_ca->tpc_ca->getNHits();
                EventInfo::Instance().tpcNTracks = nTpcTracks;
#endif    // COLLECT_EVENT_INFO
		}
        } // use_tpc_ca end

	if(gl3) {
		u_int trg_mask_dummy = 0xFFFFFFFF ;
		gl3->Event_start(&trg_mask_dummy) ;
		// load data into gl3
		if(gl3_detectors & (1<<TRG_ID))  gl3->Event_data(TRG_ID, 1, trg_blob.buff, trg_blob.bytes) ;
		if(gl3_detectors & (1<<BTOW_ID)) gl3->Event_data(BTOW_ID, 1, btow_blob.buff, btow_blob.bytes) ;
		if(gl3_detectors & (1<<TOF_ID))  gl3->Event_data(TOF_ID, 1, tof_blob.buff, tof_blob.bytes) ;
		if(gl3_detectors & (1<<MTD_ID))  gl3->Event_data(MTD_ID, 1, mtd_blob.buff, mtd_blob.bytes) ;
		if(gl3_detectors & (1<<TPX_ID))  gl3->Event_data(TPX_ID, 1, tpc_ca_blob.buff, tpc_ca_blob.bytes);

		gl3->gl3->eventNumber = eventCounter;

		VAL = PROFILER(VAL);

		// pass in the max number of blobs
		// pass out the number of filled blobs
		blobs = sizeof(blobs_out)/sizeof(hlt_blob);

		// this time we can not just run gl3->event_do();
		// finalize the event reconstrction
		//gl3->gl3->finalizeReconstruction2();        
		gl3->gl3->finalizeReconstruction();        

		// This goes through the hlt algorithms
		for(int algo = 1; algo <= gl3TriggerDecider::MaxNHLTTriggers; algo++) {

			LOG(NOTE, "Check HLT Trigger %i", algo);
                        LOG(NOTE, ">>>> %16llX", l2TrgMask);
                        LOG(NOTE, ">>>> %16llX", l3AlgoNeededMask[algo]);
                        
			UINT64 satisfied=0;

                        if(l2TrgMask & l3AlgoNeededMask[algo]) {
                            // This is satisfied if at least one of the triggers satisfied at level 2
                            // is required at HLT, so now I must run algo

				switch(algo) {
					case 1:    // always accept!
						satisfied = 1;
						break;
					case 2:    // highpt
						satisfied = gl3->gl3->triggerDecider->decide_HighPt();
						break;
					case 3:    // dielectron
						satisfied = gl3->gl3->triggerDecider->decide_DiElectron();
						break;
					case 4:
						satisfied = gl3->gl3->triggerDecider->decide_HeavyFragment();
						break;
					case 5:    // always accept with tracks!
						// these are always accepted so add the 
						// triggers that need them
						satisfied = gl3->gl3->triggerDecider->decide_AllEvents();
						break;
					case 6:
						satisfied = gl3->gl3->triggerDecider->decide_RandomEvents();
						break;
					case 7:
						satisfied = gl3->gl3->triggerDecider->decide_BesGoodEvents();
						break;
					case 8:
						satisfied = gl3->gl3->triggerDecider->decide_diV0();
						break;
					case 9:
						satisfied = gl3->gl3->triggerDecider->decide_MatchedHT2();
						break;
					case 10:
						satisfied = gl3->gl3->triggerDecider->decide_LowMult();
						break;
					case 11:
						satisfied = gl3->gl3->triggerDecider->decide_UPCDiElectron();
						break;
					case 12:
						satisfied = gl3->gl3->triggerDecider->decide_UPCPair();
						break;
					case 13:
						satisfied = gl3->gl3->triggerDecider->decide_MTDDiMuon();
						break;
                                        case 14:
						satisfied = gl3->gl3->triggerDecider->decide_FixedTarget();
						break;
                                        case 15:
                                                satisfied = gl3->gl3->triggerDecider->decide_FixedTargetMonitor();
                                                break;
                                        case 16:
                                                satisfied = gl3->gl3->triggerDecider->decide_BesMonitor();
                                                break;
                                        case 17:
                                                satisfied = gl3->gl3->triggerDecider->decide_HLTGood2();
                                                break;
					case 18:
                                                satisfied = gl3->gl3->triggerDecider->decide_MTDQuarkonium();
						break;
                                        case 19:
                                                satisfied = gl3->gl3->triggerDecider->decide_DiElectron2Twr();
                                                break;
                                        default:   // and all the others...
                                                satisfied = ((l3AlgoNeededMask[algo] & l3TrgMask_fromfile) > 0LL) ? 1 : 0;
				}

				if(satisfied) {
					l3TrgMask |= (l2TrgMask & l3AlgoNeededMask[algo]);
				}
			}
		}

		LOG(NOTE, "l3TrgMask = 0x%0llX, hltDecision = 0x%0x", l3TrgMask, gl3->gl3->triggerDecider->getDecision());

		if(gl3){
			gl3->fill_blobs(blobs_out, &blobs, l3TrgMask);

			int bytes = 0 ;
			for(int i=0;i<blobs;i++) {
				if(blobs_out[i].bytes <= 0) {
					LOG(ERR,"PID%i: Blob %d/%d: bytes %d",getMyId(),i,blobs,blobs_out[i].bytes) ;
					continue ;
				}

				LOG(NOTE,"GL3: blob %d/%d: %s: bytes %d",i,blobs,blobs_out[i].name, blobs_out[i].bytes);
				bytes += blobs_out[i].bytes;
			}
			LOG(NOTE,"GL3: evt %d, bytes %d, blobs %d, nGTrack %d, nPTrack %d",rdr->seq,bytes,blobs,
					gl3->gl3->getNGlobalTracks(),gl3->gl3->getNPrimaryTracks()) ;

                        gl3->Event_Stop();
                }
	}
	VAL = PROFILER(VAL);

	return l3TrgMask;
}


int L4HLTTask::stopRun() {
	LOG(NOTE, "PID%i: Stopping run...", getMyId());

	if(trg)   trg->Run_stop();
	if(btow)  btow->Run_stop();
	if(tof)   tof->Run_stop();
	if(mtd)   mtd->Run_stop();
	if(tpc_ca)tpc_ca->Run_stop();
	if(gl3)   gl3->Run_stop();

	return 0;
}
