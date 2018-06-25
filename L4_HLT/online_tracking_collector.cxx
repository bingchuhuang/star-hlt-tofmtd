//:>------------------------------------------------------------------
//: FILE:       online_tracking_collector.cc
//: HISTORY:
//:             3 dec 1999 version 1.00
//:             8 apr 2000 include modifications form Clemens
//:             6 jul 2000 add St_l3_Coordinate_Transformer
//:            12 jul 2000 merge tracks using parameters at closest
//:                        approach (from sl3) and them extrapolate
//:                        them to inner most hit for consumers down the line
//:            13 aug 2000 replace trackMerging with maxSectorNForTrackMerging
//:Jens Berger 03 jul 2001 makeVertex added: calcs vertex of the event
//:<------------------------------------------------------------------
#define START_FUNC LOG(NOTE, "Start %s", __PRETTY_FUNCTION__)

#include "online_tracking_collector.h"
#include "HLT_MTD/mtd_defs.h"
#include "HLTTPCOutputBlob.h"
#include "gl3Histo.h"
#include "gl3Node.h"
#include "gl3Bischel.h"
#include "gl3TOF.h"
#include "gl3MTD.h"
#include "EventInfo.h"
#include "HLTFormats.h"

#include <iostream>
#include <string.h>
#include <fstream>
#include <iomanip>

#include <rtsLog.h>
#include <LIBJML/ThreadManager.h>

#include "TVector3.h"

#ifdef OLD_DAQ_READER
#include <evpReader.hh>

#else /* OLD_DAQ_READER */
#include <DAQ_READER/daqReader.h>

#endif /* OLD_DAQ_READER */
#include "FtfSl3.h"

#ifndef OLD_DAQ_READER
#include <DAQ_READER/daq_dta.h>
#include <rtsLog.h>	// for my LOG() call
// this needs to be always included
// only the detectors we will use need to be included
// for their structure definitions...
#include <DAQ_BSMD/daq_bsmd.h>
#include <DAQ_BTOW/daq_btow.h>
#include <DAQ_EMC/daq_emc.h>
//#include <DAQ_ESMD/daq_esmd.h>
//#include <DAQ_ETOW/daq_etow.h>
//#include <DAQ_FPD/daq_fpd.h>
//#include <DAQ_FTP/daq_ftp.h>
#include <DAQ_L3/daq_l3.h>
//#include <DAQ_PP2PP/daq_pp2pp.h>
//#include <DAQ_RIC/daq_ric.h>
#include <DAQ_SC/daq_sc.h>
//#include <DAQ_SSD/daq_ssd.h>
//#include <DAQ_SVT/daq_svt.h>
#include <DAQ_TOF/daq_tof.h>
#include <DAQ_MTD/daq_mtd.h>
#include <DAQ_TPC/daq_tpc.h>
#include <DAQ_TPX/daq_tpx.h>
#include <DAQ_TRG/daq_trg.h>

#endif /* OLD_DAQ_READER */

//
// evp should already contain the event that we want to read
//
//

// #ifdef OLD_DAQ_READER
// int online_tracking_collector::readFromEvpReader(evpReader *evp, 

// #else /* OLD_DAQ_READER */

// int online_tracking_collector::readFromEvpReader(daqReader *rdr, 

// #endif /* OLD_DAQ_READER */

//#ifdef OLD_DAQ_READER
//# define READER evpReader *evp,                            
//#else
//# define READER daqReader *rdr,
//#endif
//int online_tracking_collector::readFromEvpReader(READER
//		char *mem, 
//		float defaultbField,
//		float bField,
//		int what)
//{
//
//
//#ifndef OLD_DAQ_READER
//	daq_dta *dd;
//
//#endif /* OLD_DAQ_READER */
//	//   // Read the file...
//	//   char *mem;
//	//   for(;;) {
//	//     mem = evp->get(which,type);
//	//     if(!mem) {
//	//       return evp->status;
//	//     }
//	//     if(evp->seq == 235) break;
//	//   }
//	LOG(DBG, "Reader from EVP Reader: evt=%d token=%d",rdr->seq,rdr->token);
//	// Clear this event...
//	resetEvent();
//	nHits = 0;
//	LOG(DBG, "Check magnetic field");
//	if(bField == 1000) 
//
//	{  
//
//		// try to read from file
//		bField = defaultbField;       
//		// use default
//
//#ifdef OLD_DAQ_READER
//		int ret = scReader(mem);
//		if(ret >= 0) 
//
//		{                
//
//			// but try file first!
//			if(sc.valid) bField = sc.mag_field;
//
//#else /* OLD_DAQ_READER */
//			dd = rdr->det("sc")->get("legacy");
//			if(dd) 
//
//			{
//
//				dd->iterate();
//				sc_t *sc = (sc_t *)dd->Void;
//				if(sc->valid) bField = sc->mag_field;
//
//#endif /* OLD_DAQ_READER */
//
//			}
//
//
//		}       
//
//		if(fabs(bField) < .1) bField = .1;
//		LOG(NOTE, "bField set to %f",bField,0,0,0,0);
//		setBField(bField);
//		// need a tracker...
//		coordinateTransformer->Set_parameters_by_hand(0.581, 200.668, 201.138 );
//		//    coordinateTransformer->LoadTPCLookupTable("/RTS/conf/L3/map.bin");
//		FtfSl3 *tracker = new FtfSl3(coordinateTransformer);
//		tracker->setup();
//		tracker->para.bField = fabs(bField); 
//		// +1 , -1 or 0 if zero field...
//		tracker->para.bFieldPolarity = (bField>0) ? 1 : -1;
//		////// check parms  ///////////////
//		tracker->setXyError(.12) ; 
//		tracker->setZError(.24) ;
//		tracker->para.ptMinHelixFit = 0.;
//		tracker->para.maxChi2Primary = 0.;  
//		tracker->para.trackChi2Cut = 10 ; 
//		tracker->para.hitChi2Cut   = 50 ;
//		tracker->para.goodHitChi2  = 20 ; 
//		///////////////////////////////////
//		tracker->reset();
//		// need temporary track memory...
//		L3_SECTP *sectp = NULL;
//		if(what & GL3_READ_TPC_TRACKS) 
//
//		{
//
//			sectp = (L3_SECTP *)malloc(szSECP_max);
//
//		}
//
//		int i;
//		// int ret; // unused variable, kehw
//		for(i=0;i<24;i++) 
//
//		{
//
//			if(what & GL3_READ_TPC_TRACKS) 
//
//			{
//
//				if((i%2) == 1) 
//
//				{
//
//					tracker->nHits = 0;
//					tracker->setTrackingAngles(i+1);    
//					// only set angles by hypersector...
//
//				}
//
//
//			}
//
//			LOG(DBG, "READ TPC data for sector %d  (0x%x)",i+1,rdr);
//			// Read the data....
//
//#ifdef OLD_DAQ_READER
//			ret = tpcReader(mem, i);
//			if(ret < 0) 
//
//			{ 
//
//				LOG(WARN, "No data for sector %d",i+1,0,0,0,0);
//
//#else /* OLD_DAQ_READER */
//				dd = rdr->det("tpx")->get("legacy",i+1);
//				if(dd) 
//
//				{
//
//					LOG(NOTE, "There is tpx data...");
//					dd->iterate();
//					sx_pTPC = (tpc_t *)dd->Void;
//
//				}
//
//				else 
//
//				{
//
//					LOG(NOTE, "No tpx data for sector %d check for TPC",i);
//					dd = rdr->det("tpc")->get("legacy",i+1);
//					if(dd) 
//
//					{
//
//						dd->iterate();
//						sx_pTPC = (tpc_t *)dd->Void;
//						//LOG("JEFF", "EEE: i=%d 0x%x",i,sx_pTPC);
//						// afterburner...
//						// daq_tpc *tpc_class = (daq_tpc *)rdr->det("tpc");
//						///LOG("JEFF", "EFEF 0x%x",tpc_class);
//						//int cl_found = 0;
//						//cl_found = tpc_class->fcfReader(i+1,NULL,NULL,sx_pTPC);
//						int cl_found = 0;
//						for(int pr=0;pr<45;pr++) 
//
//						{
//
//							cl_found += sx_pTPC->cl_counts[pr];
//
//						}
//
//						LOG(NOTE, "Found tpc data for sector %d... %d clusters found",i,cl_found);
//
//					}
//
//					else 
//
//					{
//
//						sx_pTPC = NULL;
//
//					}
//
//
//				}
//
//				if(!sx_pTPC) 
//
//				{
//
//					LOG(WARN, "No data for TPC sector %d",i+1,0,0,0,0);
//
//#endif /* OLD_DAQ_READER */
//					continue;
//
//				}
//
//				//LOG(NOTE, "Tpc reader done");
//
//#ifdef OLD_DAQ_READER
//				if(!tpc.has_clusters) 
//
//				{
//
//					// Construct the clusters...
//					// no calibration info, so pretty junky...
//					int ncl_recount = fcfReader(i);
//					if (ncl_recount) 
//
//					{
//
//						//LOG("JEFF", "fcf[%d] has %d clusters", i, ncl_recount);
//
//					}
//
//
//#else /* OLD_DAQ_READER */
//					if(!sx_pTPC->has_clusters) 
//
//					{
//
//						LOG(WARN, "TPC sector %d has no clusters",i);
//						continue;
//
//#endif /* OLD_DAQ_READER */
//
//					}
//
//					// read in clusters...
//					if(what & GL3_READ_TPC_CLUSTERS) 
//
//					{
//
//						LOG(DBG, "Reading clusters");
//						readClustersFromEvpReader(i+1);
//						int nnn=0;
//						for(int i=0;i<45;i++) 
//
//						{
//
//
//#ifdef OLD_DAQ_READER
//							nnn += tpc.cl_counts[i];
//
//#else /* OLD_DAQ_READER */
//							nnn += sx_pTPC->cl_counts[i];
//
//#endif /* OLD_DAQ_READER */
//
//						}
//
//						LOG(DBG, "clusters done %d",nnn);
//
//					}
//
//					// Do tracking...
//					if(what & GL3_READ_TPC_TRACKS) 
//
//					{
//
//						LOG(DBG, "Tracking...");
//						tracker->readSectorFromEvpReader(i+1);
//						// only do tracking on full hypersectors...
//						if((i%1) == 0) 
//
//						{
//
//							tracker->processSector();
//							tracker->fillTracks(szSECP_max, (char *)sectp, 0);
//							LOG(DBG, "SECP size = %d",sectp->bh.length*4 + sectp->banks[0].len*4);
//							int n = this->Tracking_readSectorTracks((char *)sectp);
//							//                printf("Got %d tracks\n",n);
//							if(n < 0) 
//
//							{
//
//								LOG(WARN, "Error reading tracker: sector %d\n",i,0,0,0,0);
//								continue;
//
//							}
//
//
//						}
//
//
//					}
//
//
//				}
//
//				// LOG(NOTE, "FINAL");
//				if(what & GL3_READ_TPC_TRACKS) 
//
//				{
//
//					//LOG(INFO, "Finalizing...\n");y
//					finalizeReconstruction();
//					free(sectp);
//
//				}
//
//				// LOG(NOTE, "FINAL2");
//				// If calibrations not loaded nothing should happen here...
//
//#ifdef OLD_DAQ_READER
//				emc->readFromEvpReader(evp, mem);
//
//#else /* OLD_DAQ_READER */
//				emc->readFromEvpReader(rdr, mem);
//				this->btow_reader(rdr,"btow");
//
//#endif /* OLD_DAQ_READER */
//				delete tracker;
//
//#ifdef TIMING_GL3
//				timeMakeNodes = 0.;
//				timeMatchEMC = 0.;
//				timeMatchTOFg = 0.;
//				timeMakeVertex = 0.;
//				timeMakePrimaries = 0.;
//				timeMatchTOFp = 0.;
//				timeMeanDedx = 0.;
//#endif
//
//				return 0;
//
//			}
//
//			// Assume global tpc structure already filled....
//			// s = sector from 1
//			//
//			void online_tracking_collector::readClustersFromEvpReader(int sector)
//
//			{
//
//
//#ifdef OLD_DAQ_READER
//				if(!tpc.has_clusters) return;
//
//#else /* OLD_DAQ_READER */
//				LOG(DBG, "have clusters?  %d", sx_pTPC->has_clusters);
//				if(!sx_pTPC->has_clusters) return;
//
//#endif /* OLD_DAQ_READER */
//				for(int r = 0; r < 45; r++)
//
//				{
//
//
//#ifdef OLD_DAQ_READER
//					for(int j = 0; j < tpc.cl_counts[r]; j++)
//
//					{
//
//						tpc_cl *c = &tpc.cl[r][j];
//
//#else /* OLD_DAQ_READER */
//						for(int j = 0; j < sx_pTPC->cl_counts[r]; j++)
//
//						{
//
//							tpc_cl *c = &sx_pTPC->cl[r][j];
//
//#endif /* OLD_DAQ_READER */
//							gl3Hit *gl3c = &hit[nHits];
//							nHits++;
//							l3_cluster sl3c;
//							sl3c.pad = (int)((c->p - 0.5) * 64);
//							sl3c.time = (int)((c->t - 0.5) * 64);
//							sl3c.charge = c->charge;
//							sl3c.flags = c->flags;
//							sl3c.padrow = r;
//							sl3c.RB_MZ = 0;
//							// need to fake rb=0 so "set" doesn't change my sector
//							//c->p1;
//							//c->p2;
//							//c->t1;
//							//c->t2;
//							gl3c->set(coordinateTransformer, sector, &sl3c);
//							//printf("i=%d nHits=%d (%f %f %f) %f %f %f\n",i,nHits,
//							//       sl3c.pad / 64.0,
//							//       sl3c.time / 64.0,
//							//       sl3c.charge,
//							//       gl3c->getX(),gl3c->getY(),gl3c->getZ());
//
//						}
//
//
//					}
//
//
//				}

//####################################################################
// Constructor and Destructor
//####################################################################
online_tracking_collector::online_tracking_collector(float bField, int mxHits, int mxTracks, char* parametersDirectory, char* beamlineFile)
{

	hit                       = NULL;
	globalTrack               = NULL;
	calibDataSentFlag         = false;
	busy                      = 0;
	trackContainer            = 0;
	globalTrackIndex          = 0;
	hitProcessing             = 0;
	maxSectorNForTrackMerging = 1000000;
	coordinateTransformer     = 0;
	beamlineX                 = 0.;
	beamlineY                 = 0.;
	innerSectorGain           = 0;
	outerSectorGain           = 0;
	normInnerGain             = 2.89098808e-08 ;
	normOuterGain             = 3.94915375e-08 ;

	setup(mxHits, mxTracks);
	char tempFileName[256], tempFileName2[256];
	sprintf(tempFileName, "%semcmap.txt", parametersDirectory);
	l3EmcCalibration *bemcCalib = new l3EmcCalibration(4800);
	bemcCalib->loadTextMap(tempFileName);

	sprintf(tempFileName, "%spedestal.current", parametersDirectory);
	bemcCalib->readL2Pedestal(tempFileName);

	l3EmcCalibration *eemcCalib = new l3EmcCalibration(720);
	emc = new gl3EMC(bemcCalib, eemcCalib);
	sprintf(tempFileName, "%stofAlignGeom.dat", parametersDirectory);
	sprintf(tempFileName2, "%szCali_4DB.dat", parametersDirectory);
	tof = new gl3TOF(tempFileName, tempFileName2, parametersDirectory);

	mtd = new gl3MTD();

	bischel = new gl3Bischel(parametersDirectory);

	sprintf(tempFileName, "%sHLTparameters", parametersDirectory);
	triggerDecider = new gl3TriggerDecider(this, tempFileName);

	readHLTparameters(tempFileName);
	// readBeamline(beamlineFile);

	para.bField = fabs(bField);
	// +1 , -1 or 0 if zero field...
	para.bFieldPolarity = (bField > 0) ? 1 : -1;

	resetEvent();

	// move this after receive intial calib data
	//createNetworkThread();

#ifdef DEBUG_PV_FINDER
        pvFile = fopen("PrimaryVertex.txt", "w");
#endif  // DEBUG_PV_FINDER
}


online_tracking_collector::~online_tracking_collector()
{
    delete[] hit;
    delete[] globalTrack;
    delete[] primaryTrack;
    delete[] nodes;
    delete[] primaryTrackNodeIndex;
    delete[] trackContainer;
    delete[] globalTrackIndex;
    delete   hVz;
    delete   hVx;
    delete   hVy;                      
    delete   dEdx;

    // delete bemcCalib;
    // delete eemcCalib;
    delete emc;
    delete tof;
    delete mtd;
    delete bischel;
    delete triggerDecider;

// #ifndef L4STANDALONE
//     killNetworkThread();
// #endif // L4STANDALONE

#ifdef DEBUG_PV_FINDER
    fclose(pvFile);
#endif  // DEBUG_PV_FINDER
}

void online_tracking_collector::readHLTparameters(char* HLTparameters)
{
	string parameterName;
	ifstream ifs(HLTparameters);
	if(ifs.fail()) LOG(ERR, "HLTparameters not found: %s", HLTparameters);
	if(!ifs.fail())
		while(!ifs.eof()) {
			ifs >> parameterName;
			// if(parameterName == "xVertex") ifs >> beamlineX;
			// if(parameterName == "yVertex") ifs >> beamlineY;
			if(parameterName == "matchAllEMC") ifs >> matchAllEMC;
			if(parameterName == "matchAllTOF") ifs >> matchAllTOF;
			if(parameterName == "matchAllMTD") ifs >> matchAllMTD;
			if(parameterName == "useBeamlineToMakePrimarys") ifs >> useBeamlineToMakePrimarys;
			if(parameterName == "useVpdtoMakeStartTime") ifs >> useVpdtoMakeStartTime;
			if(parameterName == "updateBeamline") ifs >> nTracksCutUpdateBeamline;
			if(parameterName == "updatedEdxGain") ifs >> nTracksCutUpdatedEdxGainPara;
			if(parameterName == "sigmaDedx1") ifs >> sigmaDedx1;
		}
	triggerDecider->readHLTparameters(HLTparameters);
}

void online_tracking_collector::readBeamline(char* beamlineFile)
{
	int run_num[50];
	double beamX[50] , beamY[50];
	int end_num = 0;
	int nlines = 0;
	string str;

	memset(run_num, 0, sizeof(run_num)) ;
	memset(beamX, 0, sizeof(beamX)) ;
	memset(beamY, 0, sizeof(beamY)) ;

	ifstream ifs(beamlineFile);

	if(!ifs.fail()) {
		while(!ifs.eof()) {
			end_num = end_num % 50;
			int run_bit = 999;
			double tem_beamx = 999.;
			double tem_beamy = 999.;

			ifs >> str;
			run_bit = atoi(str.data());
			if(run_bit < 10000000 || run_bit > 500000000) continue;

			ifs >> str;
			tem_beamx = atof(str.data());
			if(fabs(tem_beamx - 0.40) > 10.) continue ;

			ifs >> str;
			tem_beamy = atof(str.data());
			if(fabs(tem_beamy - 0.10) > 10.) continue;

			run_num[end_num] = run_bit ;
			beamX[end_num] = tem_beamx;
			beamY[end_num] = tem_beamy;
			end_num++;
			if(nlines < 50) nlines++ ;
		}
		ifs.close();
	} else {
		LOG(ERR, "beamlineFile not found: %s", beamlineFile);
	}

	int pro_num = 0;
	int run_con = run_num[(end_num - 1 + 50) % 50];
	double beamXSum = 0;
	double beamYSum = 0;
	for(int i = 1; i <= nlines; i++) {
		int index = (end_num - i + 50) % 50;
		if(run_con == run_num[index]) {
			beamXSum = beamXSum + beamX[index] ;
			beamYSum = beamYSum + beamY[index] ;
			pro_num++;
		} else break ;
	}

	if(pro_num > 0) {
		beamlineX = beamXSum / pro_num ;
		beamlineY = beamYSum / pro_num ;
	}

	LOG(INFO, "run_number: %d , beamlineX: %f , beamlineY: %f", run_con, beamlineX, beamlineY);

}

void online_tracking_collector::setBeamline(const double bx, const double by)
{
	beamlineX = bx;
	beamlineY = by;
}

void online_tracking_collector::writeBeamline(char* beamlineFile, int run_number)
{
	FILE* f1 = fopen(beamlineFile, "a");
	if(!f1) {
		LOG(ERR, "can't write to beamline file: %s \n", beamlineFile);
		return;
	}
	fprintf(f1, "%d    %f    %f \n", run_number, beamlineX, beamlineY);
	fclose(f1);
}

int online_tracking_collector::readScalers(char* scalerFile)
{
	LOG(WARN, "Is this necessary in GL3?") ;

	double bField = 1000;
	u_int    scalerCounts[16];
	string parameterName;
	ifstream ifs(scalerFile);
	if(ifs.fail()) LOG(ERR, "sc.asc not found: %s", scalerFile);
	if(!ifs.fail())
		while(!ifs.eof()) {
			ifs >> parameterName;

			if(parameterName == "Bfield") ifs >> bField;
			if(parameterName == "RICH00") {
				for(int i = 0; i < 16; i++) {
					ifs >> scalerCounts[i] >> parameterName;
				}
			}
		}
	if(bField != 1000) {
		para.bField = fabs(bField);
		// +1 , -1 or 0 if zero field...
		para.bFieldPolarity = (bField > 0) ? 1 : -1;
	}


	return (int)scalerCounts[7] ;
}

//####################################################################
// Setters and getters
//####################################################################
gl3Track* online_tracking_collector::getGlobalTrack(int n)

{

	if(n < 0 || n > nGlobalTracks)

	{

		fprintf(stderr, " %d track index out of range \n", n);
		return NULL;

	}

	return &(globalTrack[n]);

}

gl3Track* online_tracking_collector::getPrimaryTrack(int n)

{

	if(n < 0 || n > nGlobalTracks)

	{

		fprintf(stderr, " %d track index out of range \n", n);
		return NULL;

	}

	return &(primaryTrack[n]);

}

gl3Node* online_tracking_collector::getNode(int n)

{

	if(n < 0 || n > nGlobalTracks)

	{

		fprintf(stderr, " %d track index out of range \n", n);
		return NULL;

	}

	return &(nodes[n]);

}

gl3Hit* online_tracking_collector::getHit(int n)

{

	if(n < 0 || n > nHits)

	{

		fprintf(stderr, " %d hit index out of range \n", n);
		return NULL;

	}

	return &(hit[n]);

}

gl3Sector* online_tracking_collector::getSector(int n)

{

	if(n < 0 || n > nSectors)

	{

		fprintf(stderr, " %d sector index out of range \n", n);
		return NULL;

	}

	return &(sectorInfo[n]);

}

int online_tracking_collector::getTrgCmd()

{

	//return trgData.trgCmd;
	return -1;

}

int online_tracking_collector::getTrgWord()

{

	return trgData.triggerWord;

}

int online_tracking_collector::getZDC(int n)

{

	return trgData.ZDC[n];

}

int online_tracking_collector::getCTB(int n)

{

	return trgData.CTB[n];

}

double online_tracking_collector::getZDCVertex()

{

	return ((double)(trgData.ZDC[9] - trgData.ZDC[8]) + 21.3) * 3.3;

}

// Getters for the 64bit bunch crossing number are available as
// 32 and 64 bit verions
//         PLEASE CHECK!!!
unsigned int online_tracking_collector::getBXingLo()

{

	return trgData.bunchXing_lo;

}

unsigned int online_tracking_collector::getBXingHi()

{

	return trgData.bunchXing_hi;

}

unsigned long long online_tracking_collector::getBXing()

{

	unsigned long long bx_hi_long = trgData.bunchXing_hi;
	unsigned long long bx_lo_long = trgData.bunchXing_lo;
	return (bx_hi_long << 32) | bx_lo_long;

}

//####################################################################
// addTracks: take a pointer to the local_tracks of a sector and
//            (possibly) merge them with the cuurently known tracks
//####################################################################
void online_tracking_collector::addTracks(short sector, int nTrk, local_track* localTrack)

{

	//
	gl3Track*    glTrack;
	gl3Track*    plTrack;
	local_track *trk    = localTrack ;
	int indexStore = -1 ;
	//
	int idTrack ;
	for(int i = 0 ; i < nTrk ; i += 2)

	{
		//global and primary tracks are received in pairs
		glTrack = &(globalTrack[nGlobalTracks]) ;
		glTrack->set(sector, trk) ;
		glTrack->id     = sector * 1000 + abs(trk->id) ;
		glTrack->para   = &para ;
		glTrack->sector = sector ;

		plTrack = &(primaryTrack[nGlobalTracks]);
		plTrack->set(sector, trk + 1) ;
		plTrack->id     = sector * 1000 + abs((trk + 1)->id) ;
		plTrack->para   = &para ;
		plTrack->sector = sector ;

		idTrack        = trk->id ;
		//
		//  Check where to store relation orig track<->final track
		//  but only if requested
		//
		if(hitProcessing)

		{

			indexStore = -1 ;
			if(abs(idTrack) < maxTracksSector)
				indexStore = (sector - 1) * maxTracksSector + abs(idTrack) ;
			else

			{

				LOG(ERR, " online_tracking_collector::addTracks: max number of tracks per Sector reached %d reached", idTrack , 0, 0, 0, 0) ;

			}


		}

		//
		//  if id from buffer is negative track is mergable
		//
		gl3Track* fatterTrack = 0 ;
		if(maxSectorNForTrackMerging > nTrk && idTrack < 0)

		{

			fatterTrack = glTrack->merge(trackContainer) ;
			if(fatterTrack)

			{
				(fatterTrack - globalTrack + primaryTrack)->addTrack(plTrack);
				if(hitProcessing && indexStore > 0)

				{

					globalTrackIndex[indexStore] =
						((char *)fatterTrack - (char *)globalTrack) / sizeof(gl3Track) + 1;

				}

				trk += 2 ;
				nMergedTracks++ ;
				continue ;

			}

			nMergableTracks++ ;

		}

		//   Store new track in globalTrackIndex array
		if(hitProcessing && indexStore > 0)
			globalTrackIndex[indexStore] = nGlobalTracks + 1;
		//   Increment counters
		nGlobalTracks++ ;
		trk += 2 ;
		if(nGlobalTracks + 1 >= maxTracks)

		{

			LOG(ERR, " online_tracking_collector::addTracks: max number of tracks %d reached, sector: %i nrSectorTracks: %i", maxTracks, sector, nTrk  , 0, 0) ;
			nGlobalTracks-- ;
			break;

		}


	}


}

//####################################################################
// FillGlobTracks: fill the gl3Tracks into the HLT_GT
//####################################################################
int online_tracking_collector::FillGlobTracks(int maxBytes, char* buffer)
{
	//   Check enough space
	//
	int nBytesNeeded = sizeof(HLT_GT) + (nGlobalTracks - 10000) * sizeof(hlt_track) ;
	if(nBytesNeeded > maxBytes) {

		LOG(ERR, " online_tracking_collector::writeGlobTracks: %d bytes needed less than max = %d \n",
				nBytesNeeded, maxBytes , 0, 0, 0) ;
		return 0 ;
	}

	HLT_GT* head = (HLT_GT *)buffer ;

	//   Loop over tracks now
	//
	hlt_track* gTrack = (hlt_track *)head->globalTrack ;
	int counter = 0 ;
	for(int i = 0 ; i < nGlobalTracks ; i++) {

		/*        if ( fabs(globalTrack[i].z0) > 205 )
							{

							nBadGlobTracks++ ;
							continue ;

							}
		 */
		gTrack->id   = globalTrack[i].id ;
		gTrack->flag = globalTrack[i].flag ;
		gTrack->innerMostRow = globalTrack[i].innerMostRow ;
		gTrack->outerMostRow = globalTrack[i].outerMostRow ;
		gTrack->nHits        = globalTrack[i].nHits        ;
		gTrack->ndedx        = globalTrack[i].nDedx        ;
		gTrack->q            = globalTrack[i].q            ;
		gTrack->chi2[0]      = globalTrack[i].chi2[0]      ;
		gTrack->chi2[1]      = globalTrack[i].chi2[1]      ;
		gTrack->dedx         = globalTrack[i].dedx         ;
		gTrack->pt           = globalTrack[i].pt           ;
		gTrack->phi0         = globalTrack[i].phi0         ;
		gTrack->r0           = globalTrack[i].r0           ;
		gTrack->z0           = globalTrack[i].z0           ;
		gTrack->psi          = globalTrack[i].psi          ;
		gTrack->tanl         = globalTrack[i].tanl         ;
		gTrack->length       = globalTrack[i].length       ;
		gTrack->dpt          = globalTrack[i].dpt          ;
		gTrack->dpsi         = globalTrack[i].dpsi         ;
		gTrack->dz0          = globalTrack[i].dz0          ;
		gTrack->dtanl        = globalTrack[i].dtanl        ;

                gTrack++ ;
                counter++ ;


	}
	head->nGlobalTracks = counter;
	//
	//   return number of bytes
	//
	return nBytesNeeded ;

}

//####################################################################
// FillPrimTracks: fill the gl3Tracks into the HLT_PT
//####################################################################
int online_tracking_collector::FillPrimTracks(int maxBytes, char* buffer)
{
	//   Check enough space
	//
	int nBytesNeeded = sizeof(HLT_PT) + (nPrimaryTracks - 10000) * sizeof(hlt_track) ;
	if(nBytesNeeded > maxBytes) {
            LOG(ERR, " online_tracking_collector::writePrimTracks: %d bytes needed less than max = %d \n",
				nBytesNeeded, maxBytes , 0, 0, 0) ;
		return 0;
        }


	HLT_PT* head = (HLT_PT *)buffer ;
	//   Loop over tracks now
	//
	hlt_track* pTrack = (hlt_track *)head->primaryTrack ;
	int counter = 0 ;
	gl3Track* primTrack;
	for(int i = 0 ; i < nGlobalTracks ; i++) {    // hao primary tracks is not initialized
		if(nodes[i].primaryTrack == 0) continue;
		else primTrack = nodes[i].primaryTrack;

		/*       if ( fabs(primaryTrack[i].z0) > 205 )
						 {

						 nBadPrimTracks++ ;
						 continue ;

						 }
		 */
		pTrack->id   = primTrack->id ;
		pTrack->flag = primTrack->flag ;
		pTrack->innerMostRow = primTrack->innerMostRow ;
		pTrack->outerMostRow = primTrack->outerMostRow ;
		pTrack->nHits        = primTrack->nHits        ;
		pTrack->ndedx        = primTrack->nDedx        ;
		pTrack->q            = primTrack->q            ;
		pTrack->chi2[0]      = primTrack->chi2[0]      ;
		pTrack->chi2[1]      = primTrack->chi2[1]      ;
		pTrack->dedx         = primTrack->dedx         ;
		pTrack->pt           = primTrack->pt           ;
		pTrack->phi0         = primTrack->phi0         ;
		pTrack->r0           = primTrack->r0           ;
		pTrack->z0           = primTrack->z0           ;
		pTrack->psi          = primTrack->psi          ;
		pTrack->tanl         = primTrack->tanl         ;
		pTrack->length       = primTrack->length       ;
		pTrack->dpt          = primTrack->dpt          ;
		pTrack->dpsi         = primTrack->dpsi         ;
		pTrack->dz0          = primTrack->dz0          ;
		pTrack->dtanl        = primTrack->dtanl        ;
		pTrack++ ;
		counter++ ;

	}
	head->nPrimaryTracks = counter;
	//
	//   return number of bytes
	//
	return nBytesNeeded  ;
}

//####################################################################
// FillEvent: fill the gl3Events into the HLT_EVE
//####################################################################
int online_tracking_collector::FillEvent(int maxBytes, char* buffer, unsigned int decision)
{
    //   Check enough space
    //
    int nBytesNeeded = sizeof(HLT_EVE);
    if(nBytesNeeded > maxBytes) {
        LOG(ERR, " online_tracking_collector::writeEvent: %d bytes needed less than max = %d \n",
            nBytesNeeded, maxBytes , 0, 0, 0) ;
        return 0;
    }

    HLT_EVE* head = (HLT_EVE *)buffer ;

    head->version         = HLT_GL3_VERSION ;
    head->hltDecision     = triggerDecider->getDecision();
    head->vertexX         = vertex.Getx();
    head->vertexY         = vertex.Gety();
    head->vertexZ         = vertex.Getz();
    head->lmVertexX       = lmVertex.Getx();
    head->lmVertexY       = lmVertex.Gety();
    head->lmVertexZ       = lmVertex.Getz();
    head->vpdVertexZ      = tof->vzVpd;
    head->T0              = tof->T0;
    head->innerSectorGain = innerSectorGain;
    head->outerSectorGain = outerSectorGain;
    head->beamlineX       = beamlineX;
    head->beamlineY       = beamlineY;
    head->bField          = para.bField * para.bFieldPolarity;

    // LOG("THLT", "bField = %f", head->bField);
    LOG(NOTE, "%10.5f %10.5f %10.5f %8d", head->vertexX, head->vertexY, head->vertexZ, nGlobalTracks);
    //
    //   return number of bytes
    //
    return nBytesNeeded ;
}


//####################################################################
// FillEmc: fill the gl3EmcHits into the HLT_EMC
//####################################################################
int online_tracking_collector::FillEmc(int maxBytes, char* buffer)
{
	//   Check enough space
	//
	int nEmcTowers = emc->getNBarrelTowers();
        nEmcTowers = nEmcTowers >= 0 ? nEmcTowers : 0;

        //        cout<<"nEmcTowers ="<<nEmcTowers<<endl;
	int nBytesNeeded = sizeof(HLT_EMC) + (nEmcTowers - 4800) * sizeof(hlt_emcTower) ;

	if(nBytesNeeded > maxBytes) {
		LOG(ERR, " online_tracking_collector::writeEmc: %d bytes needed less than max = %d \n",
                    nBytesNeeded, maxBytes , 0, 0, 0) ;
                ((HLT_EMC*)buffer)->nEmcTowers = 0;
                return sizeof(int);
	}

	HLT_EMC* head = (HLT_EMC *)buffer;

	head->nEmcTowers = nEmcTowers;

	hlt_emcTower* emctower = (hlt_emcTower*)head->emcTower;

	for(int i = 0 ; i < nEmcTowers ; i++) {
		emctower->adc    = emc->getTower(i)->getADC();
		emctower->energy = emc->getTower(i)->getEnergy();

		emctower->phi    = emc->getTower(i)->getTowerInfo()->getPhi();
		emctower->eta    = emc->getTower(i)->getTowerInfo()->getEta();
		emctower->z      = emc->getTower(i)->getTowerInfo()->getZ();
		emctower->softId = emc->getTower(i)->getTowerInfo()->getSoftID();
		emctower->daqId  = emc->getTower(i)->getTowerInfo()->getDaqID();

		emctower++ ;
	}

	//   return number of bytes
	//
	return nBytesNeeded ;

}

//####################################################################
// FillTofHits: fill the gl3TofHits into the HLT_TOF
//####################################################################
int online_tracking_collector::FillTofHits(int maxBytes, char* buffer)
{
        ThreadDesc* threadDesc = threadManager.attach();
        TMCP;
        //   Check enough space
	//
	int nTofHits = tof->tofhits->nHits;
        nTofHits = nTofHits >= 0 ? nTofHits : 0;

	int nBytesNeeded = sizeof(HLT_TOF) + (nTofHits - 10000) * sizeof(hlt_TofHit) ;

	if(nBytesNeeded > maxBytes) {
		LOG(ERR, "online_tracking_collector::writeTofHit: %d bytes needed less than max = %d, nTofHits = %d \n",
                    nBytesNeeded, maxBytes, nTofHits);
                ((HLT_TOF*)buffer)->nTofHits = 0;
                return sizeof(int);
	}

	HLT_TOF* head = (HLT_TOF *)buffer ;

        TMCP;
	head->nTofHits = nTofHits;
        // LOG("THLT", "nTofHits = %d", nTofHits);
	//   Loop over tracks now
	//
	hlt_TofHit* tofhit = (hlt_TofHit *)head->tofHit ;

        TMCP;
	for(int i = 0 ; i < nTofHits ; i++) {
		tofhit->trayId       =  tof->tofhits->cell[i].trayid;
		tofhit->channel      =  tof->tofhits->cell[i].channel;
		tofhit->tdc          =  tof->tofhits->cell[i].tdc;
		tofhit->tot          =  tof->tofhits->cell[i].tot;
		tofhit->tof          =  tof->tofhits->cell[i].tof;
		tofhit->triggertime  =  tof->tofhits->cell[i].triggertime;

		tofhit++ ;
        }
	//   return number of bytes
	//
	return nBytesNeeded ;
}


//####################################################################
// FillMtdHits: fill the gl3MtdHits into the HLT_MTDDIMU
//####################################################################
int online_tracking_collector::FillMtdHits(int maxBytes, char* buffer)
{
    //   Check enough space
    //
    
    int nMtdHits = 0;

    if (mtd->mtdEvent) {
        nMtdHits = mtd->mtdEvent->nMtdHits;
    }

    LOG(NOTE, "%d MTD hits for storage", nMtdHits);
    int nBytesNeeded = sizeof(HLT_MTD) + (nMtdHits - 10000) * sizeof(hlt_MtdHit) ;

    if(nBytesNeeded > maxBytes) {
        LOG(ERR, " online_tracking_collector::writeMtdHit: %d bytes needed less than max = %d \n",
            nBytesNeeded, maxBytes , 0, 0, 0) ;
        ((HLT_MTD*)buffer)->nMtdHits = 0;
        return sizeof(int);
    }

    HLT_MTD* head = (HLT_MTD *)buffer ;

    head->nMtdHits = nMtdHits;
    //   Loop over hits
    hlt_MtdHit* mtdhit = (hlt_MtdHit *)head->mtdHit ;

    for(int i = 0 ; i < nMtdHits ; i++) {
        mtdhit->fiberId             = mtd->mtdEvent->mtdhits[i].fiberId;
        mtdhit->backleg             = mtd->mtdEvent->mtdhits[i].backleg;
        mtdhit->tray                = mtd->mtdEvent->mtdhits[i].tray;
        mtdhit->channel             = mtd->mtdEvent->mtdhits[i].channel;
        mtdhit->leadingEdgeTime[0]  = mtd->mtdEvent->mtdhits[i].leadingEdgeTime.first;
        mtdhit->leadingEdgeTime[1]  = mtd->mtdEvent->mtdhits[i].leadingEdgeTime.second;
        mtdhit->trailingEdgeTime[0] = mtd->mtdEvent->mtdhits[i].trailingEdgeTime.first;
        mtdhit->trailingEdgeTime[1] = mtd->mtdEvent->mtdhits[i].trailingEdgeTime.second;
        mtdhit->hlt_trackId         = mtd->mtdEvent->mtdhits[i].hlt_trackId;
        mtdhit->delta_z             = mtd->mtdEvent->mtdhits[i].delta_z;
        mtdhit->delta_y             = mtd->mtdEvent->mtdhits[i].delta_y;
	mtdhit->isTrigger           = mtd->mtdEvent->mtdhits[i].isTrigger;

        mtdhit++ ;

    }
    //   return number of bytes
    //
    return nBytesNeeded ;
}

//####################################################################
// FillPvpdHits: fill the gl3PvpdHits into the HLT_PVPD
//####################################################################
int online_tracking_collector::FillPvpdHits(int maxBytes, char* buffer)
{
	int nPvpdHits = tof->pvpdhits->nHits;
        nPvpdHits = nPvpdHits >= 0 ? nPvpdHits : 0;
        
	int nBytesNeeded = sizeof(HLT_PVPD) + (nPvpdHits - 10000) * sizeof(hlt_TofHit) ;

	if(nBytesNeeded > maxBytes) {
            LOG(ERR, " online_tracking_collector::writePvpdHit: %d bytes needed less than max = %d \n",
                    nBytesNeeded, maxBytes , 0, 0, 0) ;
            ((HLT_PVPD*)buffer)->nPvpdHits = 0;
            return sizeof(int);
	}

	HLT_PVPD* head = (HLT_PVPD *)buffer ;

	head->nPvpdHits = nPvpdHits;

	//   Loop over tracks now
	//
	hlt_TofHit* pvpdhit = (hlt_TofHit *)head->pvpdHit ;

	for(int i = 0 ; i < nPvpdHits ; i++) {
		pvpdhit->trayId       =  tof->pvpdhits->cell[i].trayid;
		pvpdhit->channel      =  tof->pvpdhits->cell[i].channel;
		pvpdhit->tdc          =  tof->pvpdhits->cell[i].tdc;
		pvpdhit->tot          =  tof->pvpdhits->cell[i].tot;
		pvpdhit->tof          =  tof->pvpdhits->cell[i].tof;
		pvpdhit->triggertime  =  tof->pvpdhits->cell[i].triggertime;

		pvpdhit++ ;

	}
	//   return number of bytes
	//
	return nBytesNeeded ;
}

//####################################################################
// FillPvpdHits: fill the gl3PvpdHits into the HLT_PVPD
//####################################################################
int online_tracking_collector::FillNodes(int maxBytes, char* buffer)
{
	int nNodes = nGlobalTracks;

	//   Check enough space
	//
	int nBytesNeeded = sizeof(HLT_NODE) + (nNodes - 10000) * sizeof(hlt_node) ;

	if(nBytesNeeded > maxBytes) {
		LOG(ERR, " online_tracking_collector::writeNodes: %d bytes needed less than max = %d \n",
                    nBytesNeeded, maxBytes , 0, 0, 0) ;
		return 0 ;
	}

	HLT_NODE* head = (HLT_NODE *)buffer ;

	head->nNodes = nNodes;

	hlt_node* node = (hlt_node *)head->node ;

	int primTrackSN = 0;

	for(int i = 0 ; i < nNodes ; i++)

	{

		if(nodes[i].globalTrack == 0) node->globalTrackSN = -1 ;
		else node->globalTrackSN = (int)(nodes[i].globalTrack - &globalTrack[0]) ;

		if(nodes[i].primaryTrack == 0) node->primaryTrackSN = -1 ;
		else {
			node->primaryTrackSN = primTrackSN ;
			primTrackSN ++;
		}

		if(nodes[i].primarytofCell == 0)node->tofHitSN = -1 ;
		else  node->tofHitSN = (int)(nodes[i].primarytofCell - & (tof->tofhits->cell[0])) ;

		if(nodes[i].emcTower == 0)node->emcTowerSN = -1;
		else  node->emcTowerSN = (int)(nodes[i].emcTower - emc->getTower(0)) ;

		node->emcMatchPhiDiff = nodes[i].emcMatchPhiDiff ;
		node->emcMatchZEdge   = nodes[i].emcMatchZEdge ;
		node->projChannel =  nodes[i].tofProjectChannel;
		node->localY      =  nodes[i].tofLocalY;
		node->localZ      =  nodes[i].tofLocalZ;
		node->beta        =  nodes[i].beta;
		node->tof         =  nodes[i].tof;
		node->pathlength         =  nodes[i].pathlength;

		node++   ;
	}
	//   return number of bytes
	//
	return nBytesNeeded;

}

// //####################################################################
// // fillglobaltracks: fill the gl3Tracks into the HLT_globalTracks
// //####################################################################
// int online_tracking_collector::fillGlobalTracks ( int maxBytes, char* buffer, unsigned int token )

// {

// //
// //   Check enough space
// //
// int nBytesNeeded = sizeof(HLT_globalTracks) + (nGlobalTracks-1) * sizeof(global_track) ;
// if ( nBytesNeeded > maxBytes )

// {

// LOG(ERR, " online_tracking_collector::writeTracks: %d bytes needed less than max = %d \n",
// nBytesNeeded, maxBytes ,0,0,0) ;
// return 0 ;

// }

// HLT_globalTracks* head = (HLT_globalTracks *)buffer ;
// head->nHits   = nHits;
// head->xVert   = vertex.Getx();
// head->yVert   = vertex.Gety();
// head->zVert   = vertex.Getz();
// // bankHeader
// //
// memcpy(head->bh.bank_type,CHAR_L3_GTD,8);
// head->bh.bank_id    = 1;
// head->bh.format_ver = DAQ_RAW_FORMAT_VERSION ;
// head->bh.byte_order = DAQ_RAW_FORMAT_ORDER ;
// head->bh.format_number = 0;
// head->bh.token      = token;
// head->bh.w9         = DAQ_RAW_FORMAT_WORD9;
// head->bh.crc        = 0;
// //don't know yet....
// head->bh.length     = (sizeof(struct L3_GTD)
// + (nGlobalTracks-1) * sizeof(struct global_track))/4 ;
// //
// //   Loop over tracks now
// //
// global_track* oTrack = (global_track *)head->track ;
// int counter = 0 ;
// for ( int i = 0 ; i < nGlobalTracks ; i++ )

// {

// if ( fabs(globalTrack[i].z0) > 205 )

// {

// nBadTracks++ ;
// continue ;

// }

// Track->id   = globalTrack[i].id ;
// Track->flag = globalTrack[i].flag ;
// Track->innerMostRow = globalTrack[i].innerMostRow ;
// Track->outerMostRow = globalTrack[i].outerMostRow ;
// Track->nHits        = globalTrack[i].nHits        ;
// Track->ndedx        = globalTrack[i].nDedx        ;
// Track->q            = globalTrack[i].q            ;
// Track->chi2[0]      = globalTrack[i].chi2[0]      ;
// Track->chi2[1]      = globalTrack[i].chi2[1]      ;
// Track->dedx         = globalTrack[i].dedx         ;
// Track->pt           = globalTrack[i].pt           ;
// Track->phi0         = globalTrack[i].phi0         ;
// Track->r0           = globalTrack[i].r0           ;
// Track->z0           = globalTrack[i].z0           ;
// Track->psi          = globalTrack[i].psi          ;
// Track->tanl         = globalTrack[i].tanl         ;
// Track->length       = globalTrack[i].length       ;
// Track->dpt          = globalTrack[i].dpt          ;
// Track->dpsi         = globalTrack[i].dpsi         ;
// Track->dz0          = globalTrack[i].dz0          ;
// Track->dtanl        = globalTrack[i].dtanl        ;
// Track++ ;
// counter++ ;

// }

// head->nTracks = counter ;
// //
// //   return number of bytes
// //
// return ((char *)oTrack-buffer) ;

// }


//####################################################################
// readL3Data: read the L3 contributions for an event. Currently
//             includes TPC data, but SVT and FTPC data will also
//             be in this data block at some time.
//####################################################################
int online_tracking_collector::readL3Data(L3_P* header)

{

	char* buffer = (char *)header;
	//L3_P* header = (L3_P *)buffer ;
	int length, offset ;
	char* trackPointer ;
	char* hitPointer ;
	resetEvent();
	int i ;
	L3_SECP* sectorP ;
	for(i = 0 ; i < nSectors ; i++)

	{

		length = header->sector[i].len ;
		if(length == 0) continue ;
		offset = 4 * header->sector[i].off ;
		sectorP  = (L3_SECP *) & (buffer[offset]);
		trackPointer  = (char *)sectorP + sectorP->trackp.off * 4 ;
		int nSectorTracks = 0;
		if(sectorP->trackp.off)

		{

			nSectorTracks = Tracking_readSectorTracks(trackPointer) ;
			if(nSectorTracks < 0)

			{

				LOG(ERR, "online_tracking_collector:readEvent: error reading tracks, sector %d", i + 1, 0, 0, 0, 0);
				return -1 ;

			}


		}

		if(hitProcessing && sectorP->sl3clusterp.off)

		{

			hitPointer    = (char *)sectorP + sectorP->sl3clusterp.off * 4 ;
			readSectorHits(hitPointer, nSectorTracks) ;

		}


	}

	if(header->bh.format_number >= 5 && header->trig.len)

	{

		//l3Log("Reading TRG data\n");
		//trgData.read( (int*)header + header->trig.off );
		trgData.readL3P(header);

	}

	if(header->bh.format_number >= 7)

	{

		//l3Log("Reading EMC data\n");
		emc->readRawData(header);

	}

	else

	{

		//l3Log("Not reading EMC: format_number=%i, len=%i" ,
		//    header->bh.format_number, header->emc[0].len);

	}


#ifdef EVENTDISPLAY
	//  For best merging (as least as 7/12/00) tracks
	//  are passed from sl3 to gl3 at point of closest approach
	//  the event viewer wants them(at least for now) at
	//  inner most point so we extrapolate to inner most hit radius
	double radius ;
	for(int i = 0 ; i < nGlobalTracks ; i++)

	{

		radius = coordinateTransformer->
			GetRadialDistanceAtRow(globalTrack[i].innerMostRow - 1) ;
		globalTrack[i].updateToRadius(radius) ;
		//   If track outside of TPC move radius since row
		//   radius is equal or larger than DistanceAtRow
		if(fabs(globalTrack[i].z0) > 205) globalTrack[i].updateToRadius(radius + 5.) ;
		if(fabs(globalTrack[i].z0) > 205)

		{

			LOG(ERR, "online_tracking_collector:: problem after extrapolation id %d z0 %f",
					globalTrack[i].id, globalTrack[i].z0 , 0, 0, 0) ;

		}


	}


#endif
	//
	//   declare event as busy
	//
	busy = 1 ;
	return 0 ;

}

//####################################################################
// Do last reconstruction steps before analysis
//####################################################################
int online_tracking_collector::finalizeReconstruction()
{
    ThreadDesc* threadDesc = threadManager.attach();

    LOG(NOTE, "%10.5f %10.5f %10.5f %8d", vertex.Getx(), vertex.Gety(), vertex.Getz(), nGlobalTracks);

    TMCP;
    makeNodes2();

    TMCP;
    if(matchAllEMC) matchEMC();

    TMCP;
    if(matchAllTOF) matchTOF(0);

    TMCP;
    makeVertex();

    TMCP;
    makePrimaries();

#ifdef DEBUG_PV_FINDER
    fprintf(pvFile, "%10.4f %10.4f %10.4f %10d\n",
            vertex.Getx(), vertex.Gety(), vertex.Getz(), nPrimaryTracks);
#endif // DEBUG_PV_FINDER

    TMCP;
    float zVertex = getLmVertex().Getz();

    TMCP;
    tof->CalcvzVpd();

    if(useVpdtoMakeStartTime) {
        TMCP;
        tof->Tstart(zVertex);
    } else {
        TMCP;
        tof->Tstart_NoVpd(nGlobalTracks, nodes, getLmVertex(), bischel, sigmaDedx1);
    }
    // calculate vpdVz Tstart

    TMCP;
    if(matchAllTOF) matchTOF(1);

    //meandEdx();

#ifdef USE_GPU
    //  LOG(TERR,"GPU: %s",__func__) ;
    makeRawV0();
#endif

    TMCP;
    if(matchAllMTD) matchMTD();
    //printInfo();

    return 0;
}

int online_tracking_collector::finalizeReconstruction2()
{
	LOG("THLT", "%S: %d %d", __FUNCTION__, nGlobalTracks, nPrimaryTracks);
	makeNodes2();
	if(matchAllEMC) matchEMC();
	if(matchAllTOF) matchTOF(0);

	float zVertex = getLmVertex().Getz();
	tof->CalcvzVpd();
	if(useVpdtoMakeStartTime)tof->Tstart(zVertex);
	else tof->Tstart_NoVpd(nGlobalTracks, nodes, getLmVertex(), bischel, sigmaDedx1);

	if(matchAllTOF) matchTOF(1);
	moveGTrackToPrimaryVertex(); // PV should be set in event_data()
	if(matchAllMTD) matchMTD();

	return 0;
}

//####################################################################
// readSectorHits: does what you expect
//####################################################################
int online_tracking_collector::readSectorHits(char* buffer, int nSectorTracks)

{

	L3_SECCD* head = (L3_SECCD *)buffer ;
	//
	//   Check coordinate transformer is there
	//
	if(!coordinateTransformer)

	{

		LOG(ERR, "online_tracking_collector::readSectorHits: there is not Coordinate Transformer", 0, 0, 0, 0, 0);
		return 0 ;

	}

	//
	//   Check bank header type
	//
	if(strncmp(head->bh.bank_type, CHAR_L3_SECCD, 8))

	{

		LOG(ERR, "online_tracking_collector::readSectorHits: wrong bank type %s",
				head->bh.bank_type, 0, 0, 0, 0) ;
		LOG(ERR, " correct bank type would be %s", CHAR_L3_SECCD, 0, 0, 0, 0) ;
		return 0 ;

	}

	int sector = head->bh.bank_id;
	int nSectorHits = head->nrClusters_in_sector ;
	//
	//    Check number of hits
	//
	if(nHits + nSectorHits > maxHits)

	{

		LOG(ERR, "online_tracking_collector:readSectorHits: not enough space for hits in sector %d", sector, 0, 0, 0, 0) ;
		LOG(ERR, "         maxHits %d nSectorHits %d nHits %d", maxHits,
				nSectorHits, nHits , 0, 0) ;
		return 0 ;

	}

	l3_cluster* cluster = (l3_cluster *)head->cluster ;
	l3_cluster* hitP ;
	gl3Hit*     gHitP = 0  ;
	for(int i = 0 ; i < nSectorHits ; i++)

	{

		hitP = &(cluster[i]) ;
		//
		//   Only if hits are going to be used for analysis unpack them
		//
		if(hitProcessing > 1)

		{

			gHitP = &(hit[nHits + i]);
			gHitP->set(coordinateTransformer, sector, hitP);

		}

		//
		//  Now logic to reset trackId in hits
		//  This is to take care of track merging
		//
		int trkId = hitP->trackId ;
		if(trkId < 0 || trkId > nSectorTracks)

		{

			LOG(ERR, "online_tracking_collector:readSectorHits: %d wrong track id in hit of sector %d \n",
					trkId, sector , 0, 0, 0) ;
			continue ;

		}

		int indexStore = (sector - 1) * maxTracksSector + trkId ;
		if(indexStore < 0 || indexStore > nSectors * maxTracksSector)

		{

			LOG(ERR, "online_tracking_collector:readSectorHits: %d wrong indexStore\n",
					indexStore , 0, 0, 0, 0) ;
			continue ;

		}

		int index = globalTrackIndex[indexStore] - 1 ;
		if(index < 0 || index > nGlobalTracks) continue ;
		//
		//   Only if hits are gonig to be used the
		//   linked lists are set
		//
		if(hitProcessing > 1)

		{

			if(globalTrack[index].firstHit == 0)
				globalTrack[index].firstHit = (void *)gHitP ;
			else
				((gl3Hit *)(globalTrack[index].lastHit))->nextHit = (void *)gHitP ;
			globalTrack[index].lastHit = (void *)gHitP ;
			gHitP->trackId = globalTrack[index].id ;

		}

		//
		//   Modify trackId of clusters got from sl3
		//
		hitP->trackId = globalTrack[index].id ;
		//    l3Log ( "hit trackId %d \n", globalTrack[index].id  ) ;

	}

	nHits += nSectorHits ;
	return nSectorHits ;

}

//####################################################################
// readSectorTracks: guess what it does ;)
//        fills some general info and calls addTracks()
//####################################################################
int online_tracking_collector::Tracking_readSectorTracks(char* buffer)

{

	struct L3_SECTP *head = (struct L3_SECTP *)buffer ;
	if(strncmp(head->bh.bank_type, CHAR_L3_SECTP, 8))

	{

		LOG(ERR, "online_tracking_collector::readSectorTracks, wrong bank type %s\n",
				head->bh.bank_type, 0, 0, 0, 0) ;
		return -1 ;

	}

	int sector = head->bh.bank_id ;
	if(sector < 0 || sector > nSectors)

	{

		LOG(ERR, " online_tracking_collector::readSector: %d wrong sector \n", sector , 0, 0, 0, 0) ;
		return 1 ;

	}

	gl3Sector* sectorP = &(sectorInfo[sector - 1]) ;
	sectorP->filled = 1 ;
	sectorP->nHits     = head->nHits ;
	sectorP->nTracks   = head->nTracks ;
	sectorP->cpuTime   = head->cpuTime ;
	sectorP->realTime  = head->realTime ;
	sectorP->xVert     = float(head->xVert) / 1000000 ;
	sectorP->yVert     = float(head->yVert) / 1000000 ;
	sectorP->zVert     = float(head->zVert) / 1000000 ;
	sectorP->rVert     = sqrt((double)(sectorP->xVert * sectorP->xVert +
				sectorP->yVert * sectorP->yVert)) ;
	sectorP->phiVert   = atan2((double)sectorP->yVert, (double)sectorP->xVert) ;
	if(sectorP->phiVert < 0) sectorP->phiVert += 2. * M_PI ;
	//sectorP->print();
	//
	//   Set vertex parameters for track merging
	//
	para.xVertex   = sectorP->xVert ;
	para.yVertex   = sectorP->yVert ;
	para.zVertex   = sectorP->zVert ;
	para.rVertex   = sectorP->rVert ;
	para.phiVertex = sectorP->phiVert ;
	//
	char* pointer = head->banks[0].off * 4 + buffer ;
	int nSectorTracks ;
	//
	if((head->banks[0].len > 0) && (head->bh.format_number > 0))

	{

		//    l3Log ( "banks[0].len %d\n", head->banks[0].len ) ;
		nSectorTracks = (4 * head->banks[0].len - sizeof(struct bankHeader))
			/ sizeof(struct local_track);

	}

	else nSectorTracks = 0 ;
	//
	//   Add tracks
	//
	if(nSectorTracks > 0)

	{

		struct L3_LTD* headerLocal = (struct L3_LTD*)pointer ;
		local_track* localTrack = headerLocal->track ;
		addTracks(sector, nSectorTracks, localTrack) ;

	}

	//
	return sectorP->nTracks ;

}

//####################################################################
// Read global and primary tracks from CA track and/or KFParticle
// Tracks have been converted to gl3Track format
//####################################################################
int  online_tracking_collector::readTPCCA(const char* buffer)
{
	LOG(NOTE, "Running readTPCCA()");

	float*    pVertex = (float*)(buffer);
	int*      nTracks = (int*)(buffer + 3 * sizeof(float));
	gl3Track* tracks  = (gl3Track*)(buffer + 3 * sizeof(float) + sizeof(int));

        setPrimaryVertex(pVertex);
	nGlobalTracks = *nTracks < szGL3_default_mTracks ? *nTracks : szGL3_default_mTracks;

	for(int i = 0; i < nGlobalTracks; ++i) {
            // if I use memcpy, dtor cause segmentation fault, which I do not quite understand
            // memcpy(&(globalTrack[i]),  tracks + 2 * i,     sizeof(gl3Track));
            // memcpy(&(primaryTrack[i]), tracks + 2 * i + 1, sizeof(gl3Track));
            globalTrack[i] = *(tracks + 2 * i);
            primaryTrack[i] = *(tracks + 2 * i + 1);
	}

	for(int i = 0; i < nGlobalTracks; ++i) {
		gl3Track* gTrk = globalTrack + i;
		gTrk->para = &para;

		gl3Track* pTrk = primaryTrack + i;
		// bad primary track id = -1, set in online_TPC_CA_tracker
		if(pTrk->id >= 0) {
			pTrk->para = &para;
			//         nPrimaryTracks++;
		}
	}

	return 0;
}

int online_tracking_collector::readTPCCA(HLTTPCData_t* tpc_blob)
{
    // use HLTTPCOutputBlob to transfer data from tpc
    LOG(NOTE, "Running readTPCCA() / tpc_blob");

    HLTTPCData& tpcData = tpc_blob->tpcOutput;
    
    setPrimaryVertex(tpcData.primaryVertex);
    nGlobalTracks  = tpcData.nTracks < szGL3_default_mTracks ? tpcData.nTracks : szGL3_default_mTracks;

    LOG(NOTE, ">>> %10.2f%10.2f%10.2f%10d", tpcData.primaryVertex[0],
        tpcData.primaryVertex[1], tpcData.primaryVertex[2], nGlobalTracks);

    setPrimaryVertex(tpcData.primaryVertex);

    for(int i = 0; i < nGlobalTracks; ++i) {
        globalTrack[i]  = tpcData.trackPairs[i].globalTrack;
        primaryTrack[i] = tpcData.trackPairs[i].primaryTrack;
    }

    for(int i = 0; i < nGlobalTracks; ++i) {
        gl3Track* gTrk = globalTrack + i;
        gTrk->para = &para;

        gl3Track* pTrk = primaryTrack + i;
        // bad primary track id = -1, set in online_TPC_CA_tracker
        if(pTrk->id >= 0) {
            pTrk->para = &para;
            // nPrimaryTracks++;
        }
    }

    return 0;
}

void online_tracking_collector::printInfo() {
	// accumulate info from all events
	static std::ios_base::openmode iomode = std::ios_base::out;

	ofstream gl3_vertex("gl3_Vertex.dat", iomode);
	gl3_vertex << std::setw(16) << vertex.Getx()
		<< std::setw(16) << vertex.Gety()
		<< std::setw(16) << vertex.Getz() << std::endl;

	ofstream gl3_lmVertex("gl3_lmVertex.dat", iomode);
	gl3_lmVertex 
		<< std::setw(16) << lmVertex.Getx()
		<< std::setw(16) << lmVertex.Gety()
		<< std::setw(16) << lmVertex.Getz() << std::endl;

	cout
		<< ">>>npri " << std::setw(5) << nPrimaryTracks
		<< " nTracksUsed " << std::setw(5) << nTracksUsed
		<< " vertex " 
		<< std::setw(16) << vertex.Getx()
		<< std::setw(16) << vertex.Gety()
		<< std::setw(16) << vertex.Getz() << std::endl;

	ofstream gl3_GTracks("gl3_GTracks.dat", iomode);
	gl3_GTracks << std::setprecision(6);
	gl3_GTracks << std::fixed;

	ofstream gl3_PTracks("gl3_PTracks.dat", iomode);
	gl3_PTracks << std::setprecision(6);
	gl3_PTracks << std::fixed;

	for(int i = 0; i < nGlobalTracks; ++i) {
		gl3Node*  node = getNode(i);
		gl3Track* gTrk = node->globalTrack;
		gl3Track* pTrk = node->primaryTrack;

		TVector3 gMom(gTrk->pt * std::cos(gTrk->psi),
				gTrk->pt * std::sin(gTrk->psi),
				gTrk->pt * gTrk->tanl);

		gl3_GTracks 
			<< std::setw(5) << gTrk->id   << " "
			<< std::setw(5) << gTrk->nHits   << " "
			<< std::setw( 5) << gTrk->q     << " "
			<< std::setw(16) << gTrk->r0    << " "
			<< std::setw(16) << gTrk->phi0  << " "
			<< std::setw(16) << gTrk->z0    << " "
			<< std::setw(16) << gTrk->psi   << " "
			<< std::setw(16) << gTrk->tanl  << " "
			<< std::setw(16) << gMom.Perp() << " "
			<< std::setw(16) << gMom.Phi()  << " "
			<< std::setw(16) << gMom.Eta()  << " "
			<< std::setw(16) << gMom.Mag()  << " "
			<< std::setw(16) << gTrk->dedx * 1e6 << std::endl;

		if(!pTrk) continue;
		l3xyzCoordinate pMom(pTrk->pt * cos(pTrk->psi),
				pTrk->pt * sin(pTrk->psi),
				pTrk->pt * pTrk->tanl);

		gl3_PTracks
			<< std::setw(5) << pTrk->id   << " "
			<< std::setw(5) << pTrk->nHits << " "
			<< std::setw( 5) << pTrk->q     << " "
			<< std::setw(16) << pTrk->r0    << " "
			<< std::setw(16) << pTrk->phi0  << " "
			<< std::setw(16) << pTrk->z0    << " "
			<< std::setw(16) << pTrk->psi   << " "
			<< std::setw(16) << pTrk->tanl  << " "
			<< std::setw(16) << pMom.Getr() * sin(pMom.Gettheta()) << " "
			<< std::setw(16) << pMom.Getphi() << " "
			<< std::setw(16) << pMom.Geteta() << " "
			<< std::setw(16) << pMom.Getr()   << " "
			<< std::setw(16) << pTrk->dedx * 1e6 << std::endl;
	}


	iomode = std::ios_base::app;
}

//####################################################################
// Reconstrucht the lmVertex and store it in online_tracking_collector::lmVertex
//####################################################################
int online_tracking_collector::makeLmVertex()
{
        double tofMatchWeight = 10.;
	double emcMatchWeight = 10.;
	gl3Track* gTrack;
	Ftf3DHit closestHit ;
	hVz->Reset();
	int nUsedTrack = 0;
	double trackZ[maxTracks];
	double weights[maxTracks];
	// loop over gtracks
	for(int trkcnt = 0 ; trkcnt < getNGlobalTracks(); trkcnt++) {
		gTrack = getGlobalTrack(trkcnt);
		//  printf("-----track %d\n", trkcnt);
		// acept only tracks with nHits more then minNoOfHitsOnTrack
		if(gTrack->nHits > minNoOfHitsOnTrackUsedForVertexCalc &&
				gTrack->pt > minMomUsedForVertexCalc) {
			double weight = 1.;
			if(getNode(trkcnt)->globaltofCell) weight += tofMatchWeight;
			if(getNode(trkcnt)->emcTower) weight += emcMatchWeight;
			closestHit = gTrack->closestApproach(beamlineX, beamlineY);
			if(pow(closestHit.x, 2) + pow(closestHit.y, 2) > 3 * 3) continue;
			hVz->Fill(closestHit.z, weight);
			trackZ[nUsedTrack] = closestHit.z;
			weights[nUsedTrack] = weight;
			nUsedTrack ++;
		}
	}
	int rangeBinNumber = 3;
	double peakCounts = 0;
	int peakBin = 0;
	for(int i = rangeBinNumber; i + rangeBinNumber < hVz->header.nBins; i++) {
		double integral =  hVz->Integral(i - rangeBinNumber + 1, i + rangeBinNumber);
		if(peakCounts < integral) {
			peakCounts = integral;
			peakBin = i;
		}
	}
	// int sum = hVz->header.sum;
	//    printf("nUsedTrack: %i   sum: %i   nMatchedTracks: %i   peakCounts: %f \n", nUsedTrack, sum, (sum-nUsedTrack)/10, peakCounts);
	if(peakBin == 0) return 0;
	double peakPosition = hVz->header.xStep * peakBin + hVz->header.xMin;
	double range = 3.;
	int maxIter = 20;
	for(int iter = 0; iter < maxIter; iter++) {
		double totalZ = 0.;
		double totalWeight = 0.;
		int nTracksForVertex = 0;
		for(int i = 0 ; i < nUsedTrack; i++) {
			if(fabs(trackZ[i] - peakPosition) > range) continue;
			totalZ += trackZ[i] * weights[i];
			totalWeight += weights[i];
			nTracksForVertex ++;
		}
		if(totalWeight == 0) return 0;
		double meanZ = totalZ / totalWeight;
		//  printf("meanZ: %f    totalWeight: %f     nTracksForVertex: %i     tofAndEmcMatchRatio: %f\n", meanZ, totalWeight, nTracksForVertex, (totalWeight-nTracksForVertex)/10./nTracksForVertex);
		if(fabs(meanZ - peakPosition) < 0.1) break;
		peakPosition = meanZ;
	}
	lmVertex.Setxyz(beamlineX, beamlineY, peakPosition);
	return 1;
}

//####################################################################
// Reconstrucht the vertex and store it in online_tracking_collector::vertex
//####################################################################
int online_tracking_collector::makeVertex()
{
#ifdef DEBUG_PV_FINDER
    // fprintf(pvFile, "%12d ", EventInfo::Instance().eventId);
    // fprintf(pvFile, " %10.4f %10.4f %10.4f ", vertex.Getx(), vertex.Gety(), vertex.Getz());
#endif // DEBUG_PV_FINDER

    //if(fabs(tof->vzVpd)<200) lmVertex.Setxyz(beamlineX, beamlineY, tof->vzVpd);
    //else if(lmVertex.Getz() == -999.) makeLmVertex();
    makeLmVertex();
    LOG(NOTE, "%10.4f %10.4f %10.4f ", vertex.Getx(), vertex.Gety(), vertex.Getz());
    
    gl3Track* gTrack ;
    Ftf3DHit closestHit ;
    vertex = lmVertex;
    if (trgsetupname.find("fixedTarget_2018") != std::string::npos) {
        // LOG("THLT", "fixed target!");
        vertex.Setz(201);           // for fixed target events, we know where the target is
    }
    l3xyzCoordinate oldVertex;
    int nTracks = 0;
    int maxIter = 3;
    for(int iter = 0 ; iter < maxIter; iter++) {
        oldVertex = vertex;
        double xTotal = 0;
        double yTotal = 0;
        double zTotal = 0;
        nTracks = 0;
        // loop over gtracks
        for(int trkcnt = 0 ; trkcnt < getNGlobalTracks();
            trkcnt++) {
            gTrack = getGlobalTrack(trkcnt);
            //printf("-----track %d\n", trkcnt);
            // acept only tracks with nHits more then minNoOfHitsOnTrack
            if(gTrack->nHits > minNoOfHitsOnTrackUsedForVertexCalc &&
               gTrack->pt > minMomUsedForVertexCalc) {
                closestHit = gTrack->closestApproach(getVertex().Getx(), getVertex().Gety());
                //printf("---------- (%4.2f %4.2f %4.2f)\n",closestHit.x,closestHit.y,closestHit.z);
                if(pow(closestHit.x - getVertex().Getx(), 2)
                   + pow(closestHit.y - getVertex().Gety(), 2)
                   + pow(closestHit.z - getVertex().Getz(), 2) > pow(3., 2)) continue;
                xTotal += closestHit.x;
                yTotal += closestHit.y;
                zTotal += closestHit.z;
                nTracks ++;
            }
        }

        if(nTracks < 5) {
            vertex.Setxyz(-999., -999., -999.);
            // return 0;
            break;
        }
        nTracksUsed = nTracks;

        double feedbackFactor = 1.;
        vertex.Setxyz(xTotal / nTracks * (1. + feedbackFactor) - feedbackFactor * getVertex().Getx(),
                      yTotal / nTracks * (1. + feedbackFactor) - feedbackFactor * getVertex().Gety(),
                      zTotal / nTracks * (1. + feedbackFactor) - feedbackFactor * getVertex().Getz());

        if(pow(vertex.Getx() - oldVertex.Getx(), 2) +
           pow(vertex.Gety() - oldVertex.Gety(), 2) +
           pow(vertex.Getz() - oldVertex.Getz(), 2) < pow(0.01, 2))
            break;
    }
    //if(nTracks > nTracksCutUpdateBeamline) {
    //	double newVertexWeight = 0.0001;
    //	beamlineX = (1. - newVertexWeight) * beamlineX + newVertexWeight * vertex.Getx();
    //	beamlineY = (1. - newVertexWeight) * beamlineY + newVertexWeight * vertex.Gety();
    //}

    // LOG("THLT", "%10.4f %10.4f %10.4f", vertex.Getx(), vertex.Gety(), vertex.Getz());
    
#ifdef DEBUG_PV_FINDER
    // fprintf(pvFile, "%10.4f %10.4f %10.4f\n", vertex.Getx(), vertex.Gety(), vertex.Getz());
#endif // DEBUG_PV_FINDER
        
    return 1;
}
/*  //qiuh: old one
		int online_tracking_collector::makeVertex ()

		{

// debug
//printf ( "doing gl3Vertex process!!!\n");
// init
//short sector = 0 ;
gl3Track* gTrack ;
Ftf3DHit closestHit ;
hVz->Reset();
hVx->Reset();
hVy->Reset();
// Comment to use the vertex of the last event
vertex.Setxyz(0.0,0.0,0.0);
//    for(int iter = 0 ; iter<2; iter++ )
for(int iter = 0 ; iter<10; iter++ )

{
// loop over gtracks
for(int trkcnt = 0 ; trkcnt<getNGlobalTracks();
trkcnt++ )

{
gTrack = getGlobalTrack(trkcnt);
//printf("-----track %d\n", trkcnt);
// acept only tracks with nHits more then minNoOfHitsOnTrack
if ( gTrack->nHits > minNoOfHitsOnTrackUsedForVertexCalc &&
gTrack->pt > minMomUsedForVertexCalc)

{

// bad bad bad baby! Wouldn't it be nicer to use Vx and Vz!
closestHit = gTrack->closestApproach(getVertex().Getx(),
getVertex().Gety());
//printf("---------- (%4.2f %4.2f %4.2f)\n",closestHit.x,closestHit.y,closestHit.z);
// fill histos with the coordinates of the closest point to x=y=0
hVz->Fill(closestHit.z,1.0);
hVx->Fill(closestHit.x,1.0);
hVy->Fill(closestHit.y,1.0);

}


}

// for(int trkcnt = 0 ; trkcnt<event->getNGlobalTracks(); trkcnt++ )
// get and set the weighted mean
vertex.Setxyz(hVx->getWeightedMean(6.0),
hVy->getWeightedMean(6.0),
hVz->getWeightedMean(4.0));

//  cout<<"getVertex(): "<<getVertex().Getx()<<"     "<<getVertex().Gety()<<"     "<<getVertex().Getz()<<"     "<<endl;
}

//for(int iter = 0 ; iter<2; iter++ )
return 0;

}
 */

int online_tracking_collector::makePrimaries()
{
	nPrimaryTracks = 0;
	for(int i = 0; i < getNGlobalTracks(); i++) {
		gl3Node* node = getNode(i);
		if(lmVertex.Getz() == -999.) {
			node->primaryTrack = 0;
			continue;
		}
		// gl3Track* primary = node->primaryTrack; // unused variable, kehw
		gl3Track* global = node->globalTrack;
		global->updateToClosestApproach(beamlineX, beamlineY);
		double x0 = global->r0 * cos(global->phi0);
		double y0 = global->r0 * sin(global->phi0);
		global->dca = sqrt(pow(x0 - beamlineX, 2) + pow(y0 - beamlineY, 2) + pow(global->z0 - lmVertex.Getz(), 2));
		double dca2 = sqrt(pow(x0 - beamlineX, 2) + pow(y0 - beamlineY, 2));
		double dcaToUse = 0;
		if(useBeamlineToMakePrimarys) dcaToUse = dca2;
		else dcaToUse = global->dca;
		if(dcaToUse > 3.) node->primaryTrack = 0;
		else {
			nPrimaryTracks++;
		}
	}

	return nPrimaryTracks;
}

//####################################################################
// move global tracks to the DCA point to beamline
//####################################################################
void online_tracking_collector::moveGTrackToBeamline()
{
	for(int i = 0; i < nGlobalTracks; i++) {
		gl3Track* global = globalTrack + i;
		global->updateToClosestApproach(beamlineX, beamlineY);
		double x0 = global->r0 * cos(global->phi0);
		double y0 = global->r0 * sin(global->phi0);
		double z0 = global->z0;
		global->dca = sqrt(pow(x0 - beamlineX, 2) +
				pow(y0 - beamlineY, 2) +
				pow(z0 - lmVertex.Getz(), 2));
	}
}

//####################################################################
// move global tracks to the DCA point to PV
//####################################################################
void online_tracking_collector::moveGTrackToPrimaryVertex()
{
        double vx = vertex.Getx();
	double vy = vertex.Gety();
	double vz = vertex.Getz();

	for(int i = 0; i < nGlobalTracks; i++) {
		gl3Track* global = globalTrack + i;
		global->updateToClosestApproach(vx, vy);
		double x0 = global->r0 * cos(global->phi0);
		double y0 = global->r0 * sin(global->phi0);
		double z0 = global->z0;
		global->dca = sqrt(pow(x0 - vx, 2) +
				pow(y0 - vy, 2) +
				pow(z0 - vz, 2));
	}
}


//####################################################################
//
//####################################################################
int online_tracking_collector::resetEvent()

{
	nHits           = 0;
	calibDataSentFlag = false;
	nGlobalTracks   = 0;
	nPrimaryTracks  = 0;
	nMergedTracks   = 0;
	nMergableTracks = 0;
	nBadGlobTracks  = 0;
	nBadPrimTracks  = 0;
	nBadNodes       = 0;
	busy            = 0;
	trigger         = 0;
	eventNumber     = 0;
	nTracksUsed     = 0;
	// Reset tracks
	memset(trackContainer, 0,
               para.nPhiTrackPlusOne * para.nEtaTrackPlusOne * sizeof(FtfContainer));

	// Reset hits
	if(hitProcessing)

	{

		memset(globalTrackIndex, 0, maxTracksSector * nSectors * sizeof(int)) ;
		delete[] hit;
		hit = new gl3Hit[maxHits];

	}

	// Reset trigger data
	for(int i = 0; i < 16; i++)
		trgData.ZDC[i] = 0;
	for(int i = 0; i < 240; i++)
		trgData.CTB[i] = 0;
	/*
		 for ( int i = 0 ; i < nGlobalTracks ; i++ ) {
		 globalTrack[i].firstHit = 0 ;
		 globalTrack[i].lastHit = 0 ;
		 globalTrack[i].nextTrack = 0 ;
		 }
	 */
	// Reset vertex seed
	vertex.Setxyz(-999., -999., -999.);
	lmVertex.Setxyz(beamlineX, beamlineY, -999.);
	emc->reset();
	tof->reset();
        mtd->reset();
	triggerDecider->resetEvent();
	return 0 ;

}

//####################################################################
//
//####################################################################
int online_tracking_collector::setup(int mxHits, int mxTracks)

{

	if(mxHits < 0 || mxHits > 1000000)

	{

		LOG(ERR, " online_tracking_collector::setup: maxHits %d out of range \n", maxHits, 0, 0, 0, 0) ;
		mxHits = 500000 ;

	}

	if(mxTracks < 0 || mxTracks > 1000000)

	{

		LOG(ERR, " online_tracking_collector::setup: maxTracks %d out of range \n", maxTracks, 0, 0, 0, 0);
		mxTracks = 50000 ;

	}

	maxHits               = mxHits ;
	maxTracks             = mxTracks ;
	maxTracksSector       = maxTracks * 2 / nSectors ;
	hit                   = new gl3Hit[maxHits] ;
	globalTrack           = new gl3Track[maxTracks] ;
	primaryTrack          = new gl3Track[maxTracks] ;
	nodes                 = new gl3Node[maxTracks] ;
	primaryTrackNodeIndex = new int[maxTracks];
	globalTrackIndex      = new int[maxTracksSector * nSectors];
	//
	//   Merging variables
	//
	nMergedTracks         = 0 ;
	para.nPhiTrackPlusOne = para.nPhiTrack + 1 ;
	para.nEtaTrackPlusOne = para.nEtaTrack + 1 ;
	//-----------------------------------------------------------------------
	//         If needed calculate track area dimensions
	//-----------------------------------------------------------------------
	para.phiSliceTrack    = (para.phiMaxTrack - para.phiMinTrack) / para.nPhiTrack;
	para.etaSliceTrack    = (para.etaMaxTrack - para.etaMinTrack) / para.nEtaTrack;
	int nTrackVolumes     = para.nPhiTrackPlusOne * para.nEtaTrackPlusOne ;
	trackContainer        = new FtfContainer[nTrackVolumes];
	if(trackContainer == NULL)

	{

		LOG(ERR, "Problem with memory allocation... exiting\n", 0, 0, 0, 0, 0) ;
		return 1 ;

	}

	para.primaries = 1 ;
	para.ptMinHelixFit = 1.e60 ;
	nGlobalTracks = 0 ;
	//-----------------------------------------------------------------------
	// instanziate for vertex calc
	//-----------------------------------------------------------------------
	minNoOfHitsOnTrackUsedForVertexCalc = 14; // points
	minMomUsedForVertexCalc = 0.25; // GeV
	matchAllEMC = 0;
	matchAllTOF = 0;
	matchAllMTD = 1;
	useBeamlineToMakePrimarys = 0;
	useVpdtoMakeStartTime = 0;
	char hid[50] ;
	char title[100] ;
	strcpy(hid, "Vertex_Vz") ;
	strcpy(title, "Vertex_Vz") ;
	hVz = new gl3Histo(hid, title, 400, -200., 200.) ;
	strcpy(hid, "Vertex_Vx") ;
	strcpy(title, "Vertex_Vx") ;
	hVx = new gl3Histo(hid, title, 100, -10, 10);
	strcpy(hid, "Vertex_Vy") ;
	strcpy(title, "Vertex_Vy") ;
	hVy = new gl3Histo(hid, title, 100, -10, 10);
	// -----------------------------------------------------------------------
	dEdx = new gl3Histo("dEdx", "dEdx", 500, -13.3, -12.3) ;

	return 0 ;

}

int online_tracking_collector::btow_reader(daqReader *rdr, char *do_print)

{

	int found = 0 ;
	daq_dta *dd ;
	if(strcasestr(do_print, "btow")) ;
	// leave as is...

	else do_print = 0 ;
	dd = rdr->det("btow")->get("adc") ;
	if(dd) {

		while(dd->iterate()) {

			found = 1 ;
			btow_t *d = (btow_t *) dd->Void ;
			for(int i = 0; i < BTOW_MAXFEE; i++) {

				for(int j = 0; j < BTOW_PRESIZE; j++) {

					if(do_print) printf("BTOW: fee %2d: preamble %d: 0x%04X [%d dec]\n", i, j, d->preamble[i][j], d->preamble[i][j]) ;

				}

				for(int j = 0; j < BTOW_DATSIZE; j++) {

					if(do_print) printf("BTOW: fee %2d: data %d: 0x%04X [%d dec]\n", i, j, d->adc[i][j], d->adc[i][j]) ;

				}


			}


		}


	}

	return found ;

}

int online_tracking_collector::bsmd_reader(daqReader *rdr, char *do_print)

{

	int found = 0 ;
	daq_dta *dd ;
	if(strcasestr(do_print, "bsmd")) ;
	// leave as is...

	else do_print = 0 ;
	// do I see the non-zero-suppressed bank? let's do this by fiber...
	for(int f = 1; f <= 12; f++) {

		dd = rdr->det("bsmd")->get("adc_non_zs", 0, f) ;
		// sector is ignored (=0)

		if(dd) {

			while(dd->iterate()) {

				found = 1 ;
				bsmd_t *d = (bsmd_t *) dd->Void ;
				if(do_print) printf("BSMD non-ZS: fiber %2d, capacitor %d:\n", dd->rdo, d->cap) ;
				for(int i = 0; i < BSMD_DATSIZE; i++) {

					if(do_print) printf("   %4d = %4d\n", i, d->adc[i]) ;

				}


			}


		}


	}

	// do I see the zero suppressed bank?
	for(int f = 1; f <= 12; f++) {

		dd = rdr->det("bsmd")->get("adc", 0, f) ;
		if(dd) {

			while(dd->iterate()) {

				found = 1 ;
				bsmd_t *d = (bsmd_t *) dd->Void ;
				if(do_print) printf("BSMD ZS: fiber %2d, capacitor %d:\n", dd->rdo, d->cap) ;
				for(int i = 0; i < BSMD_DATSIZE; i++) {

					// since this is zero-suppressed, I'll skip zeros...
					if(do_print) if(d->adc[i]) printf("   %4d = %4d\n", i, d->adc[i]) ;

				}


			}


		}


	}

	return found ;

}

int online_tracking_collector::tof_reader(daqReader *rdr, char *do_print)

{

	int found = 0 ;
	daq_dta *dd ;
	dd = rdr->det("tof")->get("legacy") ;
	if(dd) {

		LOG(NOTE, "TOF found") ;
		if(strcasecmp(do_print, "tof") == 0) {

			while(dd->iterate()) {

				tof_t *tof = (tof_t *)dd->Void ;
				for(int r = 0; r < 4; r++) {

					printf("RDO %d: words %d:\n", r + 1, tof->ddl_words[r]) ;
					for(u_int i = 0; i < tof->ddl_words[r]; i++) {

						printf("\t%d: 0x%08X [%u dec]\n", i, tof->ddl[r][i], tof->ddl[r][i]) ;

					}


				}


			}


		}


	} else {

		LOG(ERR,"TOF is NOT found");

	}

	return found ;

}

int online_tracking_collector::makeNodes()
{
	for(int i = 0; i < nGlobalTracks; i++) {
		gl3Node*  node          = &nodes[i];
		gl3Track* gTrack        = &globalTrack[i];
		gl3Track* pTrack        = &primaryTrack[i];
		node->globalTrack       = gTrack;
		node->primaryTrack      = pTrack;
		node->globaltofCell     = 0;
		node->primarytofCell    = 0;
		node->emcTower          = 0;
		node->emcMatchPhiDiff   = -999.;
		node->emcMatchZEdge     = -999.;
		node->tof               = -999.;
		node->beta              = -999.;
		node->tofLocalY         = -999.;
		node->tofLocalZ         = -999.;
		node->pathlength        = -999.;
		node->tofProjectChannel = -999;
	}

	return 0;
}

int online_tracking_collector::makeNodes2()
{
	// make nodes with CA tracks
	// global and primary tracks are all available now
	for(int i = 0; i < nGlobalTracks; i++) {
		gl3Node*  node          = &nodes[i];
		gl3Track* gTrack        = &globalTrack[i];
		gl3Track* pTrack        = primaryTrack[i].id >= 0 ? &(primaryTrack[i]) : NULL;
		node->globalTrack       = gTrack;
		node->primaryTrack      = pTrack;
		node->globaltofCell     = 0;
		node->primarytofCell    = 0;
		node->emcTower          = 0;
		node->emcMatchPhiDiff   = -999.;
		node->emcMatchZEdge     = -999.;
		node->tof               = -999.;
		node->beta              = -999.;
		node->tofLocalY         = -999.;
		node->tofLocalZ         = -999.;
		node->pathlength        = -999.;
		node->tofProjectChannel = -999;
	}

	return 0;
}

int online_tracking_collector::matchEMC()
{
    LOG(NOTE, "nTow = %d", emc->getNBarrelTowers());

    for (int i = 0; i < nGlobalTracks; i++) {
        gl3Node* node = &nodes[i];
        gl3Track* gTrack = &globalTrack[i];
        if (gTrack->nHits < minNoOfHitsOnTrackUsedForVertexCalc) continue;
        if (gTrack->pt < minMomUsedForVertexCalc) continue;
        for (int i = 0; i < emc->getNBarrelTowers(); i++) {
            gl3EmcTower* tower = emc->getBarrelTower(i);
            // double towerEnergy = tower->getEnergy(); // unused variables, kehw
            double phiDiff, zEdge;
            if (!tower->matchTrack(gTrack, phiDiff, zEdge)) continue;
            node->emcTower = tower;
            node->emcMatchPhiDiff = phiDiff;
            node->emcMatchZEdge = zEdge;
        }
    }

    return 0;
}

int online_tracking_collector::matchTOF(int usePrimary)
{
	if(usePrimary) {
		if(lmVertex.Getz() == -999.) return -1;

		/// move the following codes to finalizeReconstruction function
		/*
			 float zVertex = getLmVertex().Getz();

			 tof->CalcvzVpd();
			 if(useVpdtoMakeStartTime)tof->Tstart(zVertex);
			 else tof->Tstart_NoVpd(nGlobalTracks,nodes,getLmVertex(),bischel,sigmaDedx1);
		 */

		for(int i = 0; i < nGlobalTracks; i++) {
			gl3Node* node = &nodes[i];
			if(node->globalTrack->nHits < minNoOfHitsOnTrackUsedForVertexCalc) continue;
			if(node->globalTrack->pt < minMomUsedForVertexCalc) continue;
			if(!node->primaryTrack) continue;
			tof->matchPrimaryTracks(node, getLmVertex());
		}
	} else {
		for(int i = 0; i < nGlobalTracks; i++) {
			gl3Node* node = &nodes[i];
			if(node->globalTrack->nHits < minNoOfHitsOnTrackUsedForVertexCalc) continue;
			if(node->globalTrack->pt < minMomUsedForVertexCalc) continue;
			tof->matchGlobalTracks(node);
		}
	}

	return 0;
}


int online_tracking_collector::matchMTD()
{
    if (!mtd->mtdEvent) {
        LOG(NOTE, "no mtd");
        return 0;
    }

    double dz = -999, dy = -999.;
    LOG(NOTE, "Match MTD - %d global tracks", nGlobalTracks);
    vector<int> hitIndex_multMatch;
    const int nMtdHits = mtd->mtdEvent->nMtdHits;
    vector<double> match_dy[nMtdHits];
    vector<double> match_dz[nMtdHits];
    vector<int> match_index[nMtdHits];
    for(int i = 0; i < nGlobalTracks; i++) 
      {
        gl3Track* gTrack = getGlobalTrack(i);
        double pt = gTrack->pt;
        if(!gTrack) continue;
        if(pt < 1) continue;
        if(fabs(gTrack->eta) > 0.8) continue;
        if(gTrack->nHits < 10) continue;
        if(fabs(gTrack->dca) > 10) continue;

        // track-hit matching
        int hit_index = mtd->findMatchedMtdHit(gTrack, dz, dy);
        if(hit_index < 0) continue;
	LOG(DBG,"Track %d is matched to hit %d, pt = %2.3f, eta = %2.3f, nHits = %d, nHitdDedx = %d, dca = %2.3f\n"
	      ,i,hit_index,pt,gTrack->eta,gTrack->nHits,gTrack->nDedx,gTrack->dca);

	match_index[hit_index].push_back(i);
	match_dy[hit_index].push_back(dy);
	match_dz[hit_index].push_back(dz);
      }

    for(int j = 0; j < nMtdHits; j++) 
      {
	int nMatch = match_index[j].size();
	int trackId = -1;
	double dy = -999., dz = -999., minR = 999.;

	// find track that is closest to hit
	for(int i=0; i<nMatch; i++)
	  {
	    double dr = sqrt(match_dy[j][i]*match_dy[j][i]+match_dz[j][i]*match_dz[j][i]);
	    if(dr<minR)
	      {
		trackId = match_index[j][i];
		dy = match_dy[j][i];
		dz = match_dz[j][i];
		minR = dr;
	      }
	  }
        if(trackId>-1)
          {
	    // position calibration
            int backleg = mtd->mtdEvent->mtdhits[j].backleg;
            int module  = mtd->mtdEvent->mtdhits[j].tray;
            int channel = mtd->mtdEvent->mtdhits[j].channel;
            dy -= MTD_YCORR_RUN15[ (backleg-1)*5 + module-1 ];
            dz -= MTD_ZCORR_RUN15[ (backleg-1)*60 + (module-1)*12 + channel ];
          }
	mtd->mtdEvent->mtdhits[j].hlt_trackId = trackId;
	mtd->mtdEvent->mtdhits[j].delta_z     = dz;
	mtd->mtdEvent->mtdhits[j].delta_y     = dy;
      }
    return 0;
}


int online_tracking_collector::meandEdx()
{
	gl3Track* primTrack;

	for(int i = 0 ; i < nGlobalTracks ; i++) {    // hao primary tracks is not initialized
		if(nodes[i].primaryTrack == 0) continue;
		else primTrack = nodes[i].primaryTrack;

		float nHits = primTrack->nHits ;
		float nDedx = primTrack->nDedx ;
		float dedx  = primTrack->dedx  ;
		float pt    = primTrack->pt    ;
		float tanl  = primTrack->tanl  ;
		float p     = pt * sqrt(1 + pow(tanl, 2)) ;

		if(nHits >= 20 && nDedx >= 15 && p >= 0.5 && p <= 0.6) {

			dEdx->Fill(log(dedx));

		}

	}

	return 0;
}

int online_tracking_collector::CalidEdx(char *GainParameters, int run_number)
{
	int    MaxBin;
	double MaxValue;
	double scaleFactor;

	double rmx2y = 0. ;
	double rmx4 = 0. ;
	double rmx3 = 0. ;
	double rmx2 = 0. ;
	double rmxy = 0. ;

	int Entries = dEdx->header.nEntries ;

	if(Entries < nTracksCutUpdatedEdxGainPara) return 0;

	MaxBin   = dEdx->GetMaximumBin();
	MaxValue = -13.3 + MaxBin / 500. + 0.001;

	int BinStart = MaxBin - 60 ;
	int BinEnd   = MaxBin + 40 ;

	double mx2y = 0.;
	double mx2  = 0. ;
	double my   = 0. ;
	double mx4  = 0.;
	double mx3  = 0. ;
	double mx   = 0. ;
	double mxy  = 0. ;

	for(int i = BinStart; i < BinEnd; i++) {
		double x = -13.3 + i / 500. + 0.001 ;
		double y = dEdx->GetValue(i) ;

		mx2y =  mx2y + pow(x, 2) * y;
		mx2  =  mx2 + pow(x, 2);
		my   =  my + y;

		mx4  =  mx4 + pow(x, 4);
		mx3  =  mx3 + pow(x, 3);
		mx   =  mx + x;
		mxy  =  mxy + x * y;

	}

	float nbins = BinEnd - BinStart ;

	rmx2y = mx2y / nbins - mx2 * my / nbins / nbins ;
	rmx4  = mx4 / nbins - mx2 * mx2 / nbins / nbins ;
	rmx3  = mx3 / nbins - mx * mx2 / nbins / nbins ;
	rmx2  = mx2 / nbins - mx * mx / nbins / nbins ;
	rmxy  = mxy / nbins - mx * my / nbins / nbins ;

	double p2 = (rmx3 * rmxy - rmx2 * rmx2y) / (rmx3 * rmx3 - rmx2 * rmx4) ;
	double p1 = (rmxy - p2 * rmx3) / rmx2 ;

	double a = -0.5 * p1 / p2 ;

	scaleFactor = 2.3970691 / exp(a) * 1.e-6 ;

	double TempinnerGainPara = innerSectorGain * scaleFactor ;
	double TempouterGainPara = outerSectorGain * scaleFactor ;

	LOG(INFO, "InnerGain && OuterGain parameters run by run: Entries :%d \t maxBin :%d \t maxValue :%f \t pol maxValue:%f \t scale Factor:%f \n", Entries , MaxBin , MaxValue , a , scaleFactor);

	if(fabs(scaleFactor - 1.) < 0.1) {

		FILE* f2 = fopen(GainParameters, "a");
		if(!f2) {
			LOG(ERR, "can't write to GainParameters file: %s \n", GainParameters);
			return 0;
		}
		fprintf(f2, "%d    %e    %e  \n", run_number, TempinnerGainPara, TempouterGainPara);
		fclose(f2);
	} else {
		LOG(WARN, "innerSectorGain and outerSectorGain change larger than 10% ... ");
	}

	dEdx->Reset();

	return 0;
}

int online_tracking_collector::readGainPara(char *GainParameters)
{
	int run_num[50];
	double innerGain[50] , outerGain[50];
	int end_num = 0;
	int nlines = 0;
	string str;

	ifstream ifs(GainParameters);

	if(!ifs.fail()) {
		while(!ifs.eof()) {
			end_num = end_num % 50;
			int run_bit = 999;
			double tem_innergain = 999.;
			double tem_outergain = 999.;

			ifs >> str;
			run_bit = atoi(str.data());
			if(run_bit < 10000000 || run_bit > 500000000) continue;
			ifs >> str;
			tem_innergain = atof(str.data());
			if(fabs(tem_innergain - normInnerGain) / normInnerGain > 0.9) continue ;

			ifs >> str;
			tem_outergain = atof(str.data());
			if(fabs(tem_outergain - normOuterGain) / normOuterGain > 0.9) continue;

			run_num[end_num] = run_bit ;
			innerGain[end_num] = tem_innergain;
			outerGain[end_num] = tem_outergain;
			end_num++;
			if(nlines < 50) nlines++ ;
		}
	} else {
		LOG(ERR, "GainParameters not found: %s", GainParameters);
	}

	int pro_num = 0;
	int run_con = run_num[(end_num - 1 + 50) % 50];
	double innerGainSum = 0;
	double outerGainSum = 0;
	for(int i = 1; i <= nlines; i++) {
		int index = (end_num - i + 50) % 50;
		if(run_con == run_num[index]) {
			innerGainSum = innerGainSum + innerGain[index] ;
			outerGainSum = outerGainSum + outerGain[index] ;
			pro_num++;
		} else break ;
	}
	innerSectorGain = innerGainSum / pro_num ;
	outerSectorGain = outerGainSum / pro_num ;

	LOG(INFO, "run_number: %d , innerSectorGain: %e , outerSectorGain: %e", run_con, innerSectorGain, outerSectorGain);

	if(fabs(innerSectorGain - normInnerGain) / normInnerGain > 0.9 || fabs(outerSectorGain - normOuterGain) / normOuterGain > 0.9) {
		innerSectorGain = normInnerGain ;
		outerSectorGain = normOuterGain ;

		LOG(WARN, "innerSectorGain or outerSectorGain change from Calibrate value larger than 20% ... ");
	}

	ifs.close();

	return 0;
}

void online_tracking_collector::setGainPara(const double ig, const double og)
{
	innerSectorGain = ig;
	outerSectorGain = og;
}

size_t online_tracking_collector::makeNetworkData(int runnumber, hltCaliData& caliData)
{
    memset(&caliData, 0, sizeof(caliData));

    memcpy(caliData.header, "CALB", 4); // see L4_Server::run()
    caliData.runnumber = runnumber;
    caliData.npri      = nTracksUsed;
    // TODO which vertex to be send?
    caliData.vx        = vertex.Getx();
    caliData.vy        = vertex.Gety();

    LOG(NOTE, "%f %f", caliData.vx, caliData.vy);
    
    int nMipTrk = 0;
    int nDcaTrk = 0;

    for(int i = 0 ; i < nGlobalTracks; i++) {
        gl3Track* primTrack = NULL;
        gl3Track* globalTrack = NULL;
        
        // see online_tracking_collector::meandEdx()
        if(nodes[i].primaryTrack) {
            primTrack = nodes[i].primaryTrack;
            globalTrack = nodes[i].globalTrack;
        } else {
            continue;
        }

        short nHits = primTrack->nHits ;
        float nDedx = primTrack->nDedx ;
        float dedx  = primTrack->dedx  ;
        float pt    = primTrack->pt    ;
        float tanl  = primTrack->tanl  ;
        float p     = pt * sqrt(1 + pow(tanl, 2)) ;

        if(nHits >= 20 && nDedx >= 15 && p >= 0.5 && p <= 0.6
           && nMipTrk < hltCaliData::MaxMipTracks) {  // max capacity per event
            caliData.logdEdx[nMipTrk] = log(dedx);
            nMipTrk++;
        }

        if (nodes[i].emcTower && nDcaTrk < hltCaliData::MaxDcaTracks
            && triggerDecider->getDecision()) {
            double dcaX    = globalTrack->r0 * cos(globalTrack->phi0) - lmVertex.Getx();
            double dcaY    = globalTrack->r0 * sin(globalTrack->phi0) - lmVertex.Gety();
            double cross   = dcaX * sin(globalTrack->psi) - dcaY * cos(globalTrack->psi);
            double theSign = (cross >= 0) ? 1. : -1.;
            double dcaXY   = theSign * sqrt(pow(dcaX, 2) + pow(dcaY, 2));
            caliData.dcaXY[nDcaTrk] = dcaXY;
            nDcaTrk++;
        }

    }

    caliData.nMipTracks = nMipTrk;
    caliData.nDcaTracks = nDcaTrk;

    LOG(NOTE, "vx = %12.6f, vy = %12.6f, nMipTracks = %d, nDcaTracks = %d",
        caliData.vx, caliData.vy, caliData.nMipTracks, caliData.nDcaTracks);

    return 0;
}
