#include "online_TPC_CA_tracker.h"

#include <DAQ_READER/daq_dta.h>
#include <DAQ_TPX/daq_tpx.h>
#include <DAQ_TPX/tpxCore.h>
#include <DAQ_TPX/tpxFCF.h>
#include <rtsLog.h>

#include <string>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <vector>
#include <tuple>
#include <utility>

#include "TVector3.h"

// --------------------------------------------------------------------------------------

// -- constructor -----------------------------------------------------------------------
online_TPC_CA_tracker::online_TPC_CA_tracker(const double magField, const double dv,
                                             const char* parDir, const unsigned int rn,
                                             const int id)
    : bField(magField), driftVelocity(dv), HLTParDir(parDir), runnumber(rn), myId(id)
{
    debugLevel       = 0;
    embedded         = 0;
    xyError          = 0.3;
    zError           = 1.0;
    minTimeBin       = 0;
    maxTimeBin       = 380;
    minClusterCharge = 80;
    maxClusterCharge = 100000;
    outBuffer        = NULL;
    outBufferSize    = 0;
    
    nHits  = 0;
    fHits  = new FtfHit[maxHits];
    caHits = new std::vector<AliHLTTPCCAGBHit>;
    caHits->reserve(maxHits);
    
    for (int iSector = 0; iSector < nTpcSectors; ++iSector) {
        tpcHitMap[iSector] = NULL;
    }

    TPCMapDir      = HLTParDir;
    HLTParameters  = HLTParDir + "/HLTparameters";
    GainParameters = HLTParDir + "/GainParameters";

    CAGBTracker = new AliHLTTPCCAGBTracker;
    nCAGBTracks = 0;

    coordinateTransformer = new l3CoordinateTransformer();
    coordinateTransformer->Set_parameters_by_hand(0.581, 200.668, 201.138 );
    fDedx = new FtfDedx((l3CoordinateTransformer*)coordinateTransformer, 0, 0.7, 0, HLTParameters.c_str());

    topoReconstructor = NULL;
    
#ifdef WITHSCIF
    kfpClient = NULL;
#endif // WITHSCIF
    
    primaryVertex[0] = -999;
    primaryVertex[1] = -999;
    primaryVertex[2] = -999;

    nGlobalTracks  = 0;
    nPrimaryTracks = 0;

    para = new FtfPara();
    para->setDefaults();
    para->bField = fabs(magField);
    para->bFieldPolarity = fabs(magField)<1.e-9? 0 : magField/fabs(magField);
}

// -- destructor -------------------------------------------------------------------------
online_TPC_CA_tracker::~online_TPC_CA_tracker()
{
    delete[] fHits;
    delete   caHits;
    delete   CAGBTracker;
    delete   coordinateTransformer;
    delete   fDedx;
    delete   topoReconstructor;
#ifdef WITHSCIF
    delete   kfpClient;
#endif // WITHSCIF    
    delete   para;

    for (int i = 0; i < nTpcSectors; ++i) {
        delete tpcHitMap[i];
    }
}

// ---------------------------------------------------------------------------------------
void online_TPC_CA_tracker::reset(void)
{
    nHits = 0;
    // memset taks a lot of time, access the fHits array by checking nHits
    // memset(fHits, 0, sizeof(FtfHit)*maxHits);
    caHits->clear();

    nCAGBTracks = 0;
    nGlobalTracks = 0;
    nPrimaryTracks = 0;

    primaryVertex[0] = -999;
    primaryVertex[1] = -999;
    primaryVertex[2] = -999;
}

int online_TPC_CA_tracker::process(HLTTPCData_t* out_buf,int &out_buf_used)
{
    LOG(NOTE, "Runing process..");

    outBuffer     = (char*)out_buf;
    outBufferSize = out_buf_used;

    runCATracker();
    makeDedx();
    // makePrimaryVertex();
    // makePrimaryTracks();
    // fillTracks(out_buf, out_buf_used);
    // fillTracks2(out_buf, out_buf_used);
    fillTracks(out_buf, out_buf_used);
    
    // printInfo2();

    return nCAGBTracks;
}


//------------------------------------------------------------------------
// run without KFParticle 
// vertex will be reconstructed by the old func online_Tracking_collector::makeVertex()
//   added by Yi Guo
//------------------------------------------------------------------------
int online_TPC_CA_tracker::process2(char *out_buf,int &out_buf_used)
{
    outBuffer     = out_buf;
    outBufferSize = out_buf_used;

    runCATracker();
    makeDedx();
    fillTracks2(out_buf, out_buf_used);

    //printInfo2();
    LOG(NOTE,"nCAGBTracks %i",nCAGBTracks);
    
    return nCAGBTracks;
}


void online_TPC_CA_tracker::printInfo2(void)
{

	std::ofstream ftfHitXYZ("ftfHitXYZ.dat");
	for (int i = 0; i < nHits; ++i) {
        FtfHit* fhit = fHits+i;
        ftfHitXYZ << fhit->sector << "\t"
                  << fhit->row << "\t"
                  << fhit->x << "\t"
                  << fhit->y << "\t"
                  << fhit->z << std::endl;
    }

    std::ofstream CAHitXYZ("CAHitXYZ.dat");
    for (int i = 0; i < nHits; ++i) {
        CAHitXYZ << (caHits->at(i)).ISlice() << "\t"
                 << (caHits->at(i)).IRow() << "\t"
                 << (caHits->at(i)).X() << "\t"
                 << (caHits->at(i)).Y() << "\t"
                 << (caHits->at(i)).Z() << std::endl;
    }
	// accumulate info from all events
	static std::ios_base::openmode iomode = std::ios_base::out;

	// -- print CA track info
	std::ofstream CAGBTracksInfo("CAGBTracksInfo.dat", iomode);

	int nTrks = CAGBTracker->NTracks();
	int nUsedHits = 0;
	for (int i = 0; i < nTrks; ++i) {
		const AliHLTTPCCAGBTrack& trk = CAGBTracker->Track(i);
		const AliHLTTPCCATrackParam& trkParm = trk.Param();

		double pt = std::abs(1.0 / trkParm.QPt());

		const double cosL = trkParm.DzDs();
		double localPx = pt * trkParm.GetCosPhi();
		double localPy = pt * trkParm.SinPhi();
		double pz = pt * cosL;

		// local coordinates to global coordinates
		// double alpha    = trk.Alpha();
		// double sinAlpha = std::sin(alpha);
		// double cosAlpha = std::cos(alpha);
		// double px  = cosAlpha*localPx - sinAlpha*localPy;
		// double py  = sinAlpha*localPx + cosAlpha*localPy;
		// double phi = px == 0 && py == 0 ? 0.0 : std::atan2(py, px);

		TVector3 mom(localPx, localPy, pz);
		mom.RotateZ(trk.Alpha());

		CAGBTracksInfo << std::fixed;

		CAGBTracksInfo << std::setw( 4) << trk.NHits()          << "\t"
			<< std::setw(10) << pt                   << "\t"
			<< std::setw(10) << mom.Phi()            << "\t"
			<< std::setw(10) << mom.PseudoRapidity() << "\t"
			<< std::setw(10) << mom.Mag()            << "\t"
			<< std::setw(10) << trk.DeDx()*1e6       << "\t"
			<< std::setw( 4) << trk.NDeDx()          << std::endl;

		nUsedHits += trk.NHits();
	}

	std::cout << __FILE__ << ": Line " << __LINE__ << " "
		<< "nUsedHits = " << nUsedHits << std::endl;

	// -- print primary vertex info
	std::ofstream primVertexInfo("PrimVertexInfo.dat", iomode);
	primVertexInfo << primaryVertex[0] << "\t"
		<< primaryVertex[1] << "\t"
		<< primaryVertex[2] << std::endl;


	// -- print global track infor
	std::ofstream gTrackInfo("gTrackInfo.dat", iomode);
	gTrackInfo << std::setprecision(6);
	gTrackInfo << std::fixed;

	std::ofstream pTrackInfo("pTrackInfo.dat", iomode);
	pTrackInfo << std::setprecision(6);
	pTrackInfo << std::fixed;

	gl3Track* trakcs = (gl3Track*)(outBuffer + 3*sizeof(float) + sizeof(int));
	for (int i = 0; i < nGlobalTracks; ++i) {
		gl3Track* gTrk = trakcs + 2*i;
		gl3Track* pTrk = trakcs + 2*i + 1;

		TVector3 gMom(gTrk->pt * std::cos(gTrk->psi),
                              gTrk->pt * std::sin(gTrk->psi),
                              gTrk->pt * gTrk->tanl);
		

		gTrackInfo 
			<< std::setw( 5) << gTrk->id << " "
			<< std::setw( 5) << gTrk->nHits << " "
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

		//if(gTrk->nHits > 10) {
		//	std::cout << ">>---------------------------------------"<<endl;
		//	std::cout << "para: " << para->bFieldPolarity << " bField: "<< bField <<endl;
		//	std::cout << "g>>" 
		//		<< std::setw( 5) << gTrk->nHits << " "
		//		<< std::setw( 5) << gTrk->innerMostRow*1 << " "
		//		<< std::setw( 5) << gTrk->outerMostRow*1 << " "
		//		<< std::setw( 5) << gTrk->q     << " "
		//		//<< std::setw(16) << gTrk->r0    << " "
		//		//<< std::setw(16) << gTrk->phi0  << " "
		//		//<< std::setw(16) << gTrk->z0    << " "
		//		//<< std::setw(16) << gTrk->psi   << " "
		//		//<< std::setw(16) << gTrk->tanl  << " "
		//		<< std::setw(16) << gMom.Perp() << " "
		//		<< std::setw(16) << gMom.Phi()  << " "
		//		<< std::setw(16) << gMom.Eta()  << " "
		//		<< std::setw(16) << gMom.Mag()  << " "
		//		<< std::setw(16) << gTrk->dedx * 1e6 << std::endl;
		//}
		if (pTrk->id < 0) continue;
		TVector3 pMom(pTrk->pt * std::cos(pTrk->psi),
				pTrk->pt * std::sin(pTrk->psi),
				pTrk->pt * pTrk->tanl);

		pTrackInfo << std::setw( 5) << pTrk->nHits << " "
			<< std::setw( 5) << pTrk->q     << " "
			<< std::setw(16) << pTrk->r0    << " "
			<< std::setw(16) << pTrk->phi0  << " "
			<< std::setw(16) << pTrk->z0    << " "
			<< std::setw(16) << pTrk->psi   << " "
			<< std::setw(16) << pTrk->tanl  << " "
			<< std::setw(16) << pMom.Perp() << " "
			<< std::setw(16) << pMom.Phi()  << " "
			<< std::setw(16) << pMom.Eta()  << " "
			<< std::setw(16) << pMom.Mag()  << " "
			<< std::setw(16) << pTrk->dedx * 1e6 << std::endl;

		//if(pTrk->nHits > 10){
		//	std::cout << "p>>"
		//		<< std::setw( 5) << pTrk->nHits << " "
		//		<< std::setw( 5) << pTrk->innerMostRow*1 << " "
		//		<< std::setw( 5) << pTrk->outerMostRow*1 << " "
		//		<< std::setw( 5) << pTrk->q     << " "
		//		//<< std::setw(16) << pTrk->r0    << " "
		//		//<< std::setw(16) << pTrk->phi0  << " "
		//		//<< std::setw(16) << pTrk->z0    << " "
		//		//<< std::setw(16) << pTrk->psi   << " "
		//		//<< std::setw(16) << pTrk->tanl  << " "
		//		<< std::setw(16) << pMom.Perp() << " "
		//		<< std::setw(16) << pMom.Phi()  << " "
		//		<< std::setw(16) << pMom.Eta()  << " "
		//		<< std::setw(16) << pMom.Mag()  << " "
		//		<< std::setw(16) << pTrk->dedx * 1e6 << std::endl;
		//}
	}
	// after the first event, change the io mode to append
	// in order accumulate informaton across events.
	iomode = std::ios_base::app;
}

void online_TPC_CA_tracker::printInfo(void)
{

	// print only one event, the last event will overwrite the previous ones
	std::ofstream ftfHitXYZ("ftfHitXYZ.dat");
	for (int i = 0; i < nHits; ++i) {
		FtfHit* fhit = fHits+i;
		ftfHitXYZ << fhit->sector << "\t"
			<< fhit->row << "\t"
			<< fhit->x << "\t"
			<< fhit->y << "\t"
			<< fhit->z << std::endl;
	}

	std::ofstream CAHitXYZ("CAHitXYZ.dat");
	for (int i = 0; i < nHits; ++i) {
		CAHitXYZ << (caHits->at(i)).ISlice() << "\t"
			<< (caHits->at(i)).IRow() << "\t"
			<< (caHits->at(i)).X() << "\t"
			<< (caHits->at(i)).Y() << "\t"
			<< (caHits->at(i)).Z() << std::endl;
	}

	LOG(NOTE, "%d %d %d", nCAGBTracks, nGlobalTracks, nPrimaryTracks);

	// accumulate info from all events
	static std::ios_base::openmode iomode = std::ios_base::out;

	// -- print CA track info
	std::ofstream CAGBTracksInfo("CAGBTracksInfo.dat", iomode);

	int nTrks = CAGBTracker->NTracks();
	int nUsedHits = 0;
	for (int i = 0; i < nTrks; ++i) {
		const AliHLTTPCCAGBTrack& trk = CAGBTracker->Track(i);
		const AliHLTTPCCATrackParam& trkParm = trk.Param();

		double pt = std::abs(1.0 / trkParm.QPt());

		const double cosL = trkParm.DzDs();
		double localPx = pt * trkParm.GetCosPhi();
        double localPy = pt * trkParm.SinPhi();
        double pz = pt * cosL;

        // local coordinates to global coordinates
        // double alpha    = trk.Alpha();
        // double sinAlpha = std::sin(alpha);
        // double cosAlpha = std::cos(alpha);
        // double px  = cosAlpha*localPx - sinAlpha*localPy;
        // double py  = sinAlpha*localPx + cosAlpha*localPy;
        // double phi = px == 0 && py == 0 ? 0.0 : std::atan2(py, px);

        TVector3 mom(localPx, localPy, pz);
        mom.RotateZ(trk.Alpha());

        CAGBTracksInfo << std::fixed;
        
        CAGBTracksInfo << std::setw( 4) << trk.NHits()          << "\t"
                       << std::setw(10) << pt                   << "\t"
                       << std::setw(10) << mom.Phi()            << "\t"
                       << std::setw(10) << mom.PseudoRapidity() << "\t"
                       << std::setw(10) << mom.Mag()            << "\t"
                       << std::setw(10) << trk.DeDx()*1e6       << "\t"
                       << std::setw( 4) << trk.NDeDx()          << std::endl;

        nUsedHits += trk.NHits();
    }

    std::cout << __FILE__ << ": Line " << __LINE__ << " "
              << "nUsedHits = " << nUsedHits << std::endl;

    // -- print primary vertex info
    std::ofstream primVertexInfo("PrimVertexInfo.dat", iomode);
    primVertexInfo << primaryVertex[0] << "\t"
                   << primaryVertex[1] << "\t"
                   << primaryVertex[2] << std::endl;
                   

    // -- print global track infor
    ofstream gTrackInfo("gTrackInfo.dat", iomode);
    gTrackInfo << std::setprecision(6);
    gTrackInfo << std::fixed;

    ofstream pTrackInfo("pTrackInfo.dat", iomode);
    pTrackInfo << std::setprecision(6);
    pTrackInfo << std::fixed;
    
    gl3Track* trakcs = (gl3Track*)(outBuffer + 3*sizeof(float) + sizeof(int));
    for (int i = 0; i < nGlobalTracks; ++i) {
        gl3Track* gTrk = trakcs + 2*i;
        gl3Track* pTrk = trakcs + 2*i + 1;

        TVector3 gMom(gTrk->pt * std::cos(gTrk->psi),
                      gTrk->pt * std::sin(gTrk->psi),
                      gTrk->pt * gTrk->tanl);
        
        gTrackInfo << std::setw( 5) << gTrk->nHits << " "
                   << std::setw(16) << gMom.Perp() << " "
                   << std::setw(16) << gMom.Phi()  << " "
                   << std::setw(16) << gMom.Eta()  << " "
                   << std::setw(16) << gMom.Mag()  << " "
                   << std::setw(16) << gTrk->dedx * 1e6 << std::endl;

        if (pTrk->id < 0) continue;
        TVector3 pMom(pTrk->pt * std::cos(pTrk->psi),
                      pTrk->pt * std::sin(pTrk->psi),
                      pTrk->pt * pTrk->tanl);
        
        pTrackInfo << std::setw( 5) << pTrk->nHits << " "
                   << std::setw(16) << pMom.Perp() << " "
                   << std::setw(16) << pMom.Phi()  << " "
                   << std::setw(16) << pMom.Eta()  << " "
                   << std::setw(16) << pMom.Mag()  << " "
                   << std::setw(16) << pTrk->dedx * 1e6 << std::endl;
    }
    

    // after the first event, change the io mode to append
    // in order accumulate informaton across events.
    iomode = std::ios_base::app;
}

// ---------------------------------------------------------------------------------------
void online_TPC_CA_tracker::setDriftVelocity(const double dv)
{
    driftVelocity = dv;
}

// ---------------------------------------------------------------------------------------
void online_TPC_CA_tracker::setMagField(const double magField)
{
    bField = magField;
}

// ---------------------------------------------------------------------------------------
void online_TPC_CA_tracker::setHLTParDir(const char* parDir)
{
    HLTParDir = parDir;
}

// ---------------------------------------------------------------------------------------
void online_TPC_CA_tracker::setHLTParameters(void)
{
    
}    

// ---------------------------------------------------------------------------------------
void online_TPC_CA_tracker::setHLTParameters(const char* parameters)
{
    HLTParameters = parameters;
}    

// ---------------------------------------------------------------------------------------
void online_TPC_CA_tracker::setTPCMap(void)
{
    //    delete[] tpcHitMap;
    for (int iSector = 0; iSector < nTpcSectors; ++iSector) {
        tpcHitMap[iSector] = new online_tracking_TpcHitMap(HLTParameters.c_str(), iSector);
        std::ostringstream mapName;
        mapName << TPCMapDir << "/tpcHitMap_sector" << iSector+1 << ".bin";
        tpcHitMap[iSector]->loadMap(mapName.str().c_str());
        tpcHitMap[iSector]->setDriftVelocity(driftVelocity);
    }

}

// ---------------------------------------------------------------------------------------
void online_TPC_CA_tracker::setTPCMap(const char* mapDir)
{
    TPCMapDir = mapDir;
    setTPCMap();
}

// ---------------------------------------------------------------------------------------
void online_TPC_CA_tracker::setGainParamagers(void)
{
    // need to be check later.
    // for dEdx
    LOG(NOTE, "Reading %s", GainParameters.c_str());
    fDedx->ReadGainparameters(GainParameters.c_str());
}

// ---------------------------------------------------------------------------------------
void online_TPC_CA_tracker::setBeamline(const double x, const double y){
	para->xVertex = x;
	para->yVertex = y;
	if( x != 0 || y != 0){
		para->rVertex = sqrt(x*x+y*y);
		para->phiVertex = atan2(y,x);
	} else {
		para->rVertex = 0.F;
		para->phiVertex = 0.F;
	}
	para->dxVertex = 0.05F;
	para->dyVertex = 0.05F;
	if(para->dxVertex != 0 || para->dyVertex != 0){
		para->xyWeightVertex = 1.F/sqrt(pow(para->dxVertex,2)+pow(para->dyVertex,2));
	} else {
		para->xyWeightVertex = 1.F;
	}
	LOG(NOTE,"beamline : x %f y %f dx %f dy %f",
            para->xVertex,para->yVertex,para->dxVertex,para->dyVertex);
}

// ---------------------------------------------------------------------------------------
void online_TPC_CA_tracker::setGainParamagers(const char* gainParameters)
{
    // need to be check later.
    // for dEdx
    GainParameters = gainParameters;
    setGainParamagers();
}

// ---------------------------------------------------------------------------------------
void online_TPC_CA_tracker::setGainParamagers(const double ig, const double og)
{
    fDedx->SetGainparameters(ig, og);
}

// ---------------------------------------------------------------------------------------
void online_TPC_CA_tracker::setSpaceChargeFromScalerCounts(int* data)
{
    for (int iSector = 0 ; iSector < nTpcSectors; ++iSector) {
        tpcHitMap[iSector]->setSpaceChargeFromScalerCounts(data);
    }
}

// ---------------------------------------------------------------------------------------
void online_TPC_CA_tracker::setBeamLineFile(const char* beamline)
{
    // do not need a beam line before CA tracging
    // beamline position get from calibration server
}

// ---------------------------------------------------------------------------------------
void online_TPC_CA_tracker::readHLTparameters(char* HLTparameters)
{
    for (int iSector = 0 ; iSector < nTpcSectors; ++iSector) {
        tpcHitMap[iSector]->readHLTparameters(HLTparameters);
    }

    fDedx->ReadHLTparameters(HLTparameters);
}


// ---------------------------------------------------------------------------------------
// Read sector from Sector Broker's memory directly
// #include <DAQ_READER/daq_dta.h>
// #include <DAQ_TPX/daq_tpx.h>
// #include <DAQ_TPX/tpxCore.h>
// #include <DAQ_TPX/tpxFCF.h>

// #define SORT_HITS
#ifndef SORT_HITS
int  online_TPC_CA_tracker::readSectorFromESB(int sector, char *mem, int len)
{
    int tb_lo, tb_hi, c_lo, c_hi ;
    l3xyzCoordinate XYZ(0,0,0);
    l3ptrsCoordinate PTRS(0,0,0,0);

    // CAGBTracker does not need sector number
    // sectorNr = sector;

    tb_lo = tb_hi = c_lo = c_hi = 0 ;

    // online_tracking_TpcHitMap *hitMap = getTpcHitMap() ;
    online_tracking_TpcHitMap *hitMap = tpcHitMap[sector-1]; // sector 1 - 24
    int sectorNHits = 0;
    
    int row_cou = 0 ;

    // static tpxFCF *fcf_algo ;
    // if(fcf_algo == 0) fcf_algo = new tpxFCF ;

    u_int *buff     = (u_int *) mem ;
    u_int *end_buff = buff + len ; // len is in words already!
    u_int *p_buff   = buff ;

    while(p_buff < end_buff) {
        daq_cld c ;

        u_int row = *p_buff++ ;
        u_int cou = *p_buff++ ;

        u_int version = row >> 16 ;	// important!
        row &= 0xFFFF ;

        if((row==0) || (row>45) || (version!=1)) {
            LOG(ERR,"Corrupt data: row %d (%d), version 0x%X, cou %d, len %d, nHits %d!",row,row_cou,version,cou,len,nHits) ;
            return 0 ;
        }

        if((nHits+cou) >= (maxHits-5)) {
            LOG(ERR,"Too many hits: row %d, version 0x%X, cou %d, hits %d>=%d!",row,version,cou,nHits,maxHits) ;
            return 0 ;
        }

        if(cou > 1000) {
            if(cou > 20000) {
                LOG(ERR,"lotsa hits: row %d, version 0x%X, cou %d!",row,version,cou) ;
                return 0 ;
            } else {
                LOG(WARN,"lotsa hits: row %d, version 0x%X, cou %d!",row,version,cou) ;
            }
        }

        row_cou++ ;	

        for(u_int i=0;i<cou;i++) {
            // this loop is executed many times per sector; OPTIMIZE
            // Tonko: will replace fcf_decode here with fully
            // optimized version...soon...
            // p_buff += fcf_algo->fcf_decode(p_buff, &c, version) ;
            p_buff += tpxFCF::fcf_decode(p_buff, &c, version);
            
            // Some cuts...
            if(c.tb < minTimeBin) {
                tb_lo++ ;
                continue;
            }

            if(c.tb > maxTimeBin) {
                tb_hi++ ;
                continue;
            }

            if(c.charge < minClusterCharge) {
                c_lo++ ;
                continue;
            }

            if(c.charge > maxClusterCharge) {
                c_hi++ ;
                continue;
            }


            // if(c.flags != 0) continue ;	// Tonko: anything in the flags means that
            // this cluster needed to be deconvoluted.
            // I propose we ignore them...

            // The subtractions are to handle the pad offsets
            // My choice at the momement is to make L3 tracking identical to 
            // old versions.   The difference is because Tonko includes the pad centroid
            // shifts in the calculation of c.t & c.p.   It may be that in the old
            // L3 code this shift was included, but in the transform code instead.
            // Untill I know, I leave it as it was before...
            //
            //      PTRS.Setptrs(c.p - .5 , c.t - .5, r+1, sector);
            //      getCoordinateTransformer()->raw_to_global(PTRS, XYZ);

            double xyz[3];

            // Tonko: OPTIMIZE
            //getTpcHitMap()->mapping(xyz, row, c.pad, c.tb);
            hitMap->mapping(xyz, row, c.pad, c.tb);

            FtfHit *hitP = &fHits[nHits];

            hitP->id     = nHits; // Tonko: this is also not necessary
            hitP->sector = sector;              // 1 - 24
            hitP->row    = row;                 // 1 - 45
            hitP->x      = xyz[0];
            hitP->y      = xyz[1];
            hitP->z      = xyz[2];
            hitP->q      = c.charge;			

            // Tonko: is this fluff below really used???
            // can we get rid of it and make the hit thing
            // smaller?

            // kehw, embedded is already harded as 0
            // embedded = 0;
            // if(embedded) 
            //     hitP->flags = (c.flags | (1<<7));
            // else 
            //     hitP->flags = c.flags;
            
            hitP->flags = c.flags;

            hitP->dx = xyError;
            hitP->dy = xyError;
            hitP->dz = zError;
            hitP->buffer1 = 0;//(int)((c.p - 0.5) * 64);
            hitP->buffer2 = 0;//(int)((c.t - 0.5) * 64);

            hitP->wxy = 1. / sqrt( square(para->xyErrorScale) * ( square(hitP->dx) + square(hitP->dy) ) );
            hitP->wz  = 1. / (para->szErrorScale * hitP->dz);

            hitP->hardwareId = 0;
            hitP->Mc_key=0;
            hitP->Mc_track_key=0;

            // reset hit pointers
            // by yiguo
            hitP->track = NULL; 
            hitP->nextTrackHit = NULL;

            nHits++;
            sectorNHits++;
        }
    }

    //    LOG(TERR,"Loaded %d/%d hits",nHits,maxHits) ;
    //    LOG(TERR,"%d %d %d %d",minTimeBin,maxTimeBin,minClusterCharge,maxClusterCharge) ;
    //    LOG(TERR,"tb lo %d, hi %d; c lo %d, hi %d",tb_lo,tb_hi,c_lo,c_hi) ;

    return sectorNHits;
}

#else  // sort hits

int  online_TPC_CA_tracker::readSectorFromESB(int sector, char *mem, int len)
{
    int tb_lo, tb_hi, c_lo, c_hi ;
    embedded = 0;
    l3xyzCoordinate XYZ(0,0,0);
    l3ptrsCoordinate PTRS(0,0,0,0);

    // CAGBTracker does not need sector number
    // sectorNr = sector;

    tb_lo = tb_hi = c_lo = c_hi = 0 ;

    // online_tracking_TpcHitMap *hitMap = getTpcHitMap() ;
    online_tracking_TpcHitMap *hitMap = tpcHitMap[sector-1]; // sector 1 - 24
    int sectorNHits = 0;

    std::vector<std::pair<int, daq_cld> > row_cluster_pairs;
    row_cluster_pairs.reserve(100000);

    int row_cou = 0 ;

    static tpxFCF *fcf_algo ;
    if(fcf_algo == 0) fcf_algo = new tpxFCF ;

    u_int *buff     = (u_int *) mem ;
    u_int *end_buff = buff + len ; // len is in words already!
    u_int *p_buff   = buff ;

    while(p_buff < end_buff) {
        u_int row = *p_buff++ ;
        u_int cou = *p_buff++ ;

        u_int version = row >> 16 ;	// important!
        row &= 0xFFFF ;

        if((row==0) || (row>45) || (version!=1)) {
            LOG(ERR,"Corrupt data: row %d (%d), version 0x%X, cou %d, len %d, nHits %d!",row,row_cou,version,cou,len,nHits) ;
            return 0 ;
        }

        if((nHits+cou) >= (maxHits-5)) {
            LOG(ERR,"Too many hits: row %d, version 0x%X, cou %d, hits %d>=%d!",row,version,cou,nHits,maxHits) ;
            return 0 ;
        }

        if(cou > 600) {
            if(cou > 20000) {
                LOG(ERR,"lotsa hits: row %d, version 0x%X, cou %d!",row,version,cou) ;
                return 0 ;
            } else {
                LOG(WARN,"lotsa hits: row %d, version 0x%X, cou %d!",row,version,cou) ;
            }
        }

        row_cou++ ;	

        for(u_int i=0;i<cou;i++) {
            // this loop is executed many times per sector; OPTIMIZE
            // Tonko: will replace fcf_decode here with fully
            // optimized version...soon...
            daq_cld c ;
            p_buff += fcf_algo->fcf_decode(p_buff, &c, version) ;
	
            // Some cuts...
            if(c.tb < minTimeBin) {
                tb_lo++ ;
                continue;
            }

            if(c.tb > maxTimeBin) {
                tb_hi++ ;
                continue;
            }

            if(c.charge < minClusterCharge) {
                c_lo++ ;
                continue;
            }

            if(c.charge > maxClusterCharge) {
                c_hi++ ;
                continue;
            }


            // if(c.flags != 0) continue ;	// Tonko: anything in the flags means that
            // this cluster needed to be deconvoluted.
            // I propose we ignore them...

            // The subtractions are to handle the pad offsets
            // My choice at the momement is to make L3 tracking identical to 
            // old versions.   The difference is because Tonko includes the pad centroid
            // shifts in the calculation of c.t & c.p.   It may be that in the old
            // L3 code this shift was included, but in the transform code instead.
            // Untill I know, I leave it as it was before...
            //
            //      PTRS.Setptrs(c.p - .5 , c.t - .5, r+1, sector);
            //      getCoordinateTransformer()->raw_to_global(PTRS, XYZ);

            // printf("CLUSTER: %15.6f %15.6f\n", c.pad, c.tb);
            row_cluster_pairs.push_back(std::make_pair(row, c));
        }

    } // row loop end

    std::sort(row_cluster_pairs.begin(), row_cluster_pairs.end(),
              [](std::pair<int, daq_cld> & p1, std::pair<int, daq_cld> & p2)->bool {
                  return std::make_tuple(p1.first, p1.second.pad, p1.second.tb) <
                      std::make_tuple(p2.first, p2.second.pad, p2.second.tb);
              });

    for (auto& p : row_cluster_pairs) {
        int& row = p.first;
        daq_cld& c = p.second;

        // LOG("THLT", ">>> %6d %15.6f %15.6f", row, c.pad, c.tb);

        double xyz[3];
        hitMap->mapping(xyz, row, c.pad, c.tb);

        FtfHit *hitP = &fHits[nHits];

        hitP->id     = nHits; // Tonko: this is also not necessary
        hitP->sector = sector;              // 1 - 24
        hitP->row    = row;                 // 1 - 45
        hitP->x      = xyz[0];
        hitP->y      = xyz[1];
        hitP->z      = xyz[2];
        hitP->q      = c.charge;			

        // Tonko: is this fluff below really used???
        // can we get rid of it and make the hit thing
        // smaller?

        if(embedded) 
            hitP->flags = (c.flags | (1<<7));
        else 
            hitP->flags = c.flags;

        hitP->dx = xyError;
        hitP->dy = xyError;
        hitP->dz = zError;
        hitP->buffer1 = 0;//(int)((c.p - 0.5) * 64);
        hitP->buffer2 = 0;//(int)((c.t - 0.5) * 64);

        hitP->wxy = 1. / sqrt( square(para->xyErrorScale) * ( square(hitP->dx) + square(hitP->dy) ) );
        hitP->wz  = 1. / (para->szErrorScale * hitP->dz);

        hitP->hardwareId = 0;
        hitP->Mc_key=0;
        hitP->Mc_track_key=0;

        // reset hit pointers
        // by yiguo
        hitP->track = NULL; 
        hitP->nextTrackHit = NULL;

        nHits++;
        sectorNHits++;
    } // cluster loop end

    
    //    LOG(TERR,"Loaded %d/%d hits",nHits,maxHits) ;
    //    LOG(TERR,"%d %d %d %d",minTimeBin,maxTimeBin,minClusterCharge,maxClusterCharge) ;
    //    LOG(TERR,"tb lo %d, hi %d; c lo %d, hi %d",tb_lo,tb_hi,c_lo,c_hi) ;

    return sectorNHits;
}
#endif  // sort hits

// ---------------------------------------------------------------------------------------
// set hit weight for the old tracker FtfTrack::fitHelix() 
void online_TPC_CA_tracker::setHitsWeight() {
    for(int i=0 ; i<nHits ; i++) { 
        FtfHit *hitP = &fHits[i];
        hitP->wxy = 1. / sqrt( square(para->xyErrorScale) * ( square(hitP->dx) + square(hitP->dy) ) );
        hitP->wz  = 1. / (para->szErrorScale * hitP->dz);
    }

    // FtfHit* p = &fHits[0];
    // float wxy = 1. / sqrt( square(para->xyErrorScale) * ( square(p->dx) + square(p->dy) ) );
    // float wz  = 1. / (para->szErrorScale * p->dz);
    
    // for(int i=0 ; i<nHits ; i++) { 
    //     FtfHit *hitP = &fHits[i];
    //     hitP->wxy = wxy;
    //     hitP->wz  = wz;
    // }
}

// ---------------------------------------------------------------------------------------
//   make settings for CA tracker
void online_TPC_CA_tracker::makeCASettings()
{
    const double PI          = 3.14159265358979312;
    const double DegToRad    = 1.74532925199432955e-02;

    // come from StRoot/Sti/StiTPCCATrackerInterface.cxx
    const int NSlices = 24; //TODO initialize from StRoot
    const int NRows = 45;

    float R[NRows] = {  60.000,  64.800,  69.600,   74.400,  79.200, //  5  //TODO initialize from StRoot
                        84.000,  88.800,  93.600,   98.800, 104.000, // 10
                        109.200, 114.400, 119.600, 127.195, 129.195, // 15
                        131.195, 133.195, 135.195, 137.195, 139.195, // 20
                        141.195, 143.195, 145.195, 147.195, 149.195, // 25
                        151.195, 153.195, 155.195, 157.195, 159.195, // 30
                        161.195, 163.195, 165.195, 167.195, 169.195, // 35
                        171.195, 173.195, 175.195, 177.195, 179.195, // 40
                        181.195, 183.195, 185.195, 187.195, 189.195};// 45

    for ( int iSlice = 0; iSlice < NSlices; iSlice++ ) {
        AliHLTTPCCAParam SlicePar;
        memset(&SlicePar, 0, sizeof(AliHLTTPCCAParam));

        Int_t sector = iSlice+1;
        // Int_t sector = iSlice;
        SlicePar.SetISlice( iSlice );
        SlicePar.SetNRows ( NRows ); 
        Double_t beta = 0;
        if (sector > 12) beta = (24-sector)*2.*PI/12.;
        else             beta =     sector *2.*PI/12.;
        SlicePar.SetAlpha    ( beta );
        SlicePar.SetDAlpha   ( 30*DegToRad );                        //TODO initialize from StRoot
        SlicePar.SetCosAlpha ( std::cos(SlicePar.Alpha()) );
        SlicePar.SetSinAlpha ( std::sin(SlicePar.Alpha()) );
        SlicePar.SetAngleMin ( SlicePar.Alpha() - 0.5*SlicePar.DAlpha() );
        SlicePar.SetAngleMax ( SlicePar.Alpha() + 0.5*SlicePar.DAlpha() );
        SlicePar.SetRMin     (  51. );                                        //TODO initialize from StRoot
        SlicePar.SetRMax     ( 194. );                                        //TODO initialize from StRoot
        SlicePar.SetErrX     (   0. );                                        //TODO initialize from StRoot
        SlicePar.SetErrY     (   0.12 ); // 0.06  for Inner                   //TODO initialize from StRoot
        SlicePar.SetErrZ     (   0.16 ); // 0.12  for Inner                NodePar->fitPars()        //TODO initialize from StRoot
        //   SlicePar.SetPadPitch (   0.675 );// 0.335 -"-

        SlicePar.SetBz ( -bField * 10 ); // change sign because change z

        // if (sector <= 12) {
        //     SlicePar.SetZMin     (   0. );                                        //TODO initialize from StRoot
        //     SlicePar.SetZMax     ( 210. );                                        //TODO initialize from StRoot
        // } else {
        //     SlicePar.SetZMin     (-210. );                                        //TODO initialize from StRoot
        //     SlicePar.SetZMax     (   0. );                                        //TODO initialize from StRoot
        // }

        if (sector > 12) {
            SlicePar.SetZMin     (   0. );                                        //TODO initialize from StRoot
            SlicePar.SetZMax     ( 210. );                                        //TODO initialize from StRoot
        } else {
            SlicePar.SetZMin     (-210. );                                        //TODO initialize from StRoot
            SlicePar.SetZMax     (   0. );                                        //TODO initialize from StRoot
        }

        for( int iR = 0; iR < NRows; iR++){
            SlicePar.SetRowX(iR, R[iR]);
        }

        // Double_t *coeffInner = StiTpcInnerHitErrorCalculator::instance()->coeff();
        // for (int iCoef=0; iCoef<6; iCoef++) {
        //     SlicePar.SetParamS0Par(0, 0, iCoef, (float)coeffInner[iCoef] );
        // }  
        // TODO tipical values, need to check if they work all the time, change event by event?
        // SlicePar.SetParamS0Par(0, 0, 0, 0.000944592 );
        // SlicePar.SetParamS0Par(0, 0, 1, 0.000968047 );
        // SlicePar.SetParamS0Par(0, 0, 2, 0.030703000 );
        // SlicePar.SetParamS0Par(0, 0, 3, 0.005380770 );
        // SlicePar.SetParamS0Par(0, 0, 4, 0.002762130 );
        // SlicePar.SetParamS0Par(0, 0, 5, 0.018512600 );

        // Run11AuAu200
        SlicePar.SetParamS0Par(0, 0, 0, 0.0004    );
        SlicePar.SetParamS0Par(0, 0, 1, 0.0022262 );
        SlicePar.SetParamS0Par(0, 0, 2, 0.0361101 );
        SlicePar.SetParamS0Par(0, 0, 3, 0.0004    );
        SlicePar.SetParamS0Par(0, 0, 4, 0.0062577 );
        SlicePar.SetParamS0Par(0, 0, 5, 0.0192329 );

        SlicePar.SetParamS0Par(0, 0, 6, 0.0f );
    
        // Double_t *coeffOuter =StiTpcOuterHitErrorCalculator::instance()->coeff();
        // for(int iCoef=0; iCoef<6; iCoef++) {
        //     SlicePar.SetParamS0Par(0, 1, iCoef, (float)coeffOuter[iCoef] );
        // }
        // SlicePar.SetParamS0Par(0, 1, 0, 0.001199550 );
        // SlicePar.SetParamS0Par(0, 1, 1, 0.000499620 );
        // SlicePar.SetParamS0Par(0, 1, 2, 0.055848000 );
        // SlicePar.SetParamS0Par(0, 1, 3, 0.010038400 );
        // SlicePar.SetParamS0Par(0, 1, 4, 0.000534858 );
        // SlicePar.SetParamS0Par(0, 1, 5, 0.047930500 );

        // Run11AuAu200
        SlicePar.SetParamS0Par(0, 1, 0, 0.0007434 );
        SlicePar.SetParamS0Par(0, 1, 1, 0.0009383 );
        SlicePar.SetParamS0Par(0, 1, 2, 0.0601024 );
        SlicePar.SetParamS0Par(0, 1, 3, 0.0046255 );
        SlicePar.SetParamS0Par(0, 1, 4, 0.0006434 );
        SlicePar.SetParamS0Par(0, 1, 5, 0.0476205 );

        SlicePar.SetParamS0Par(0, 1, 6, 0.0f );
        SlicePar.SetParamS0Par(0, 2, 0, 0.0f );
        SlicePar.SetParamS0Par(0, 2, 1, 0.0f );
        SlicePar.SetParamS0Par(0, 2, 2, 0.0f );
        SlicePar.SetParamS0Par(0, 2, 3, 0.0f );
        SlicePar.SetParamS0Par(0, 2, 4, 0.0f );
        SlicePar.SetParamS0Par(0, 2, 5, 0.0f );
        SlicePar.SetParamS0Par(0, 2, 6, 0.0f );
        SlicePar.SetParamS0Par(1, 0, 0, 0.0f );
        SlicePar.SetParamS0Par(1, 0, 1, 0.0f );
        SlicePar.SetParamS0Par(1, 0, 2, 0.0f );
        SlicePar.SetParamS0Par(1, 0, 3, 0.0f );
        SlicePar.SetParamS0Par(1, 0, 4, 0.0f );
        SlicePar.SetParamS0Par(1, 0, 5, 0.0f );
        SlicePar.SetParamS0Par(1, 0, 6, 0.0f );
        SlicePar.SetParamS0Par(1, 1, 0, 0.0f );
        SlicePar.SetParamS0Par(1, 1, 1, 0.0f );
        SlicePar.SetParamS0Par(1, 1, 2, 0.0f );
        SlicePar.SetParamS0Par(1, 1, 3, 0.0f );
        SlicePar.SetParamS0Par(1, 1, 4, 0.0f );
        SlicePar.SetParamS0Par(1, 1, 5, 0.0f );
        SlicePar.SetParamS0Par(1, 1, 6, 0.0f );
        SlicePar.SetParamS0Par(1, 2, 0, 0.0f );
        SlicePar.SetParamS0Par(1, 2, 1, 0.0f );
        SlicePar.SetParamS0Par(1, 2, 2, 0.0f );
        SlicePar.SetParamS0Par(1, 2, 3, 0.0f );
        SlicePar.SetParamS0Par(1, 2, 4, 0.0f );
        SlicePar.SetParamS0Par(1, 2, 5, 0.0f );
        SlicePar.SetParamS0Par(1, 2, 6, 0.0f );
    
        caParam.push_back(SlicePar);
    } // for iSlice
}

// ---------------------------------------------------------------------------------------
AliHLTTPCCAGBHit online_TPC_CA_tracker::FtfHit2TPCCAGBHit(const FtfHit& fHit)
{
    const double pi      = 3.14159265358979323846;
    const double piOver6 = pi / 6.0;
    const int NRows = 45;
    float R[NRows] = {  60.000,  64.800,  69.600,   74.400,  79.200, //  5  //TODO initialize from StRoot
                        84.000,  88.800,  93.600,   98.800, 104.000, // 10
                        109.200, 114.400, 119.600, 127.195, 129.195, // 15
                        131.195, 133.195, 135.195, 137.195, 139.195, // 20
                        141.195, 143.195, 145.195, 147.195, 149.195, // 25
                        151.195, 153.195, 155.195, 157.195, 159.195, // 30
                        161.195, 163.195, 165.195, 167.195, 169.195, // 35
                        171.195, 173.195, 175.195, 177.195, 179.195, // 40
                        181.195, 183.195, 185.195, 187.195, 189.195};// 45

    // STAR global coordinates to CA local
    // setcori ( 1 <= i <= 12), rotateZ i * pi/6
    // (13 <= i <= 24), rotateZ (24 - i) * pi/6
    // x <-> y
    // z -> -Z

    double x = fHit.x;
    double y = fHit.y;
    double z = fHit.z;

    int i = fHit.sector;        // Ftf Hit sector 1 - 24
    double turn_angle = 0;
    if ( i <= 12 ) {
        turn_angle = i * piOver6;
    } else {
        turn_angle = (24 - i) * piOver6;
    }

    double cos_turn_angle = std::cos(turn_angle);
    double sin_turn_angle = std::sin(turn_angle);
    
    // root TVector3::RotateZ(turn_angle)
    double xn = x*cos_turn_angle - y*sin_turn_angle ;
    double yn = x*sin_turn_angle + y*cos_turn_angle ;
    std::swap(xn, yn);
    double zn = -z;

    AliHLTTPCCAGBHit caHit;
    caHit.SetID(fHit.id);
    caHit.SetISlice(fHit.sector-1); // CA Hit sector 0 - 23
    caHit.SetIRow(fHit.row-1);
    // caHit.SetX(xn);
    caHit.SetX(R[fHit.row-1]);
    caHit.SetY(yn);
    caHit.SetZ(zn);
    caHit.SetErrY(0.12);// TODO: read parameters from somewhere 
    caHit.SetErrZ(0.16);
		
		//printf("convert>>>%f  %f  %f  %f  %f  %f  \n",x,y,z,caHit.X(),caHit.Y(),caHit.Z());

    return caHit;
}

// ---------------------------------------------------------------------------------------
void online_TPC_CA_tracker::makeCAHits(void)
{
    for (int iHit = 0; iHit < nHits; ++iHit) {
        caHits->push_back( FtfHit2TPCCAGBHit(fHits[iHit]) );
    }

}

// ---------------------------------------------------------------------------------------
int online_TPC_CA_tracker::runCATracker(void)
{
    LOG(DBG, "CA Tracking ...");
    
    // makeCASettings() only need to be run once per run
    makeCAHits();

    CAGBTracker->StartEvent();  // init. clear up everything inside
    CAGBTracker->SetSettings(caParam);
    //    CAGBTracker->ReadSettingsFromFile("/star/u/kehw/bnl/src/CATracker/data/settings.data");
    CAGBTracker->SetHits(*caHits);
    CAGBTracker->FindTracks();
    nCAGBTracks = CAGBTracker->NTracks();
    

    //static int iEvent = -1;
    //iEvent++;
    //char name[256];
    //sprintf(name, "./data/");
    //if (iEvent == 0) CAGBTracker->SaveSettingsInFile(name);
    //sprintf(name, "./data/event%d_", iEvent);
    //CAGBTracker->SaveHitsInFile(name);

    return nCAGBTracks;
}

// ---------------------------------------------------------------------------------------
//     Calculates deposited Energy
void online_TPC_CA_tracker::makeDedx(void)
{
    const int minHitsForDedx = 15; // from FtfPara.cxx
    
    AliHLTTPCCAGBTrack* CATracks = CAGBTracker->Tracks();
    for (int i = 0; i < nCAGBTracks; ++i) {
        AliHLTTPCCAGBTrack* caTrk = CATracks + i;
        if ( caTrk->NHits() < minHitsForDedx ) {
            caTrk->SetDeDx(-1e-6);
            continue;
        }
        fDedx->TruncatedMean(caTrk, CAGBTracker, fHits, nHits, bField);
    }
}

// ---------------------------------------------------------------------------------------
void online_TPC_CA_tracker::makePrimaryVertex(void)
{
    static int nKFPEvent = 0;
    if ( nKFPEvent++ % 50 ) {
        return;
    }

    // keep the topoReconstructor until the next event comes up
    // AliHLTTPCTopoReconstructor has memory leak promblem if reused
    // need to check again after update
    delete topoReconstructor;
    topoReconstructor = new KFParticleTopoReconstructor();

    std::vector<int> pid(CAGBTracker->NTracks(), -1);
    for (int i = 0; i < CAGBTracker->NTracks(); i++) {
        const AliHLTTPCCAGBTrack& t = CAGBTracker->Track(i);
        if (t.Param().GetQPt() > 0) {
            pid[i] = 211;       // pi+
        } else {
            pid[i] = -211;      // pi-
        }
    }
    
    topoReconstructor->Init(CAGBTracker, &pid);

    double param[6] = {para->xVertex, para->yVertex, 0, 0, 0, 0};
    double cov[21]  = {100,
                       0, 100,
                       0, 0, 100,
                       0, 0, 0, 100,
                       0, 0, 0, 0, 100,
                       0, 0, 0, 0, 0, 100};
    cov[0] = para->dxVertex * para->dxVertex;
    cov[2] = para->dyVertex * para->dyVertex;
    cov[5] = 200*200;           // Vz can be everywhere
    KFParticle beamline;
    beamline.Create(param, cov, 0, 0);
    topoReconstructor->SetBeamLine(beamline);

    topoReconstructor->SetChi2PrimaryCut(50);
    topoReconstructor->ReconstructPrimVertex(0);
    topoReconstructor->SortTracks();
#ifdef WITHSCIF
    sendDataToPhi();
#endif // WITH
    
    // int bestPVIndex = 0;
    // int bestNDaughters = (*topoReconstructor).GetPVTrackIndexArray(0).size();
    // for (int i = 1; i < topoReconstructor->NPrimaryVertices(); ++i) {
    //     int nDaughters = (*topoReconstructor).GetPVTrackIndexArray(i).size();
    //     if (nDaughters > bestNDaughters) {
    //         bestPVIndex = i;
    //         bestNDaughters = nDaughters;
    //     }
    // }

    // KFParticle& pVtx = topoReconstructor->GetPrimVertex(bestPVIndex);
    // primaryVertex[0] = pVtx.GetX();
    // primaryVertex[1] = pVtx.GetY();
    // primaryVertex[2] = pVtx.GetZ();
}

// ---------------------------------------------------------------------------------------
#ifdef WITHSCIF
void online_TPC_CA_tracker::sendDataToPhi()
{
    if(kfpClient && isSCIFConnected && topoReconstructor) {
        kfpClient->sendDataToPhi();
    }
}
#endif  // WITHSCIF

// ---------------------------------------------------------------------------------------
void online_TPC_CA_tracker::makePrimaryTracks(void)
{
    assert(topoReconstructor != NULL);

    // need to think hwo to make primary tracks with KFParticle
}
// --------------------------------------------------------------------------------------

void online_TPC_CA_tracker::Convert2(const AliHLTTPCCAGBTrack *CATrack, FtfTrack *locTrack, int id)
{
    // basic idea (╯-_-)╯┴—┴
    // star global to CA local:
    // 1. rotate to sector 12
    // 2. (x,y,z) -> (y,x,-z)
    // const int NHits = CATrack->NHits();

    const AliHLTTPCCATrackParam *firstParam;
    const AliHLTTPCCATrackParam *lastParam;

    firstParam = &(CATrack->InnerParam());
    lastParam  = &(CATrack->OuterParam());

    double length = 0;
    //int    innerRow = 1;
    //int    outerRow = 45;

    //innerRow = x2Row((double)(firstParam->GetX()));
    //outerRow = x2Row((double)(lastParam->GetX()));

    //pathLength(firstParame, lastParam, alpha, &length, &innerRow, &outerRow);
    // get parameters
    // ca local -> star tpc sector local
    double localxyz[3], xyz[3];
    localxyz[0] = firstParam->GetY(); // X <-> Y
    localxyz[1] = firstParam->GetX();
    localxyz[2] = -1.*firstParam->GetZ();

    // star tpc sector local -> tpc global
    const double alpha = CATrack->Alpha();
    const double ca = cos(alpha);
    const double sa = sin(alpha);
    xyz[0] =  ca * localxyz[0] + sa * localxyz[1];
    xyz[1] = -sa * localxyz[0] + ca * localxyz[1];
    xyz[2] =  localxyz[2];

    double r0   = sqrt(xyz[0] * xyz[0] + xyz[1] * xyz[1]);
    // double phi0 = atan2(xyz[1] / r0, xyz[0] / r0);
    double phi0 = atan2(xyz[1], xyz[0]);

    // AliHLTTPCCATrackParam globalCAParam = firstParam->GetGlobalParam(CATrack->Alpha());
    // printf(">>>%12.6f %12.6f\n", xyz[0], xyz[1]);
    // printf(">>>%12.6f %12.6f\n\n", globalCAParam.GetX(), globalCAParam.GetY());

    // double phi0 = atan2(globalCAParam.GetX(), globalCAParam.GetY());

    if (phi0 < 0) phi0 += 2 * M_PI;

    //double localxyz2[3], xyz2[3];
    double xyz2[3];
    //localxyz2[0]    = lastParam->GetY(); // X <-> Y
    //localxyz2[1]    = lastParam->GetX();

    //xyz2[0] =  ca * localxyz2[0] + sa * localxyz2[1];
    //xyz2[1] = -sa * localxyz2[0] + ca * localxyz2[1];

    double pt, localPx, localPy, px, py;
    short charge = 0;
    pt = fabs(1. / firstParam->GetQPt());
    charge = firstParam->GetQPt() == 0 ? 0 : firstParam->GetQPt() / fabs(firstParam->GetQPt());
    charge *= -1; // QPt ~ sign(-q) ?
    
    localPx = pt * firstParam->GetSinPhi(); // X <-> Y
    localPy = pt * firstParam->GetCosPhi();

    px =  ca * localPx + sa * localPy;
    py = -sa * localPx + ca * localPy;

    // double psi = atan2(py / pt, px / pt);// 0~2pi TODO make sure
    double psi = atan2(py, px);// 0~2pi TODO make sure
    if (psi < 0) psi += 2 * M_PI;

    ////pathlength
    //double dx = xyz[0] - xyz2[0];
    //double dy = xyz[1] - xyz2[1];
    //double radius = (double) fabs(pt / 2.9979e-3 * bField); // which bField?
    //double localPsi = 0.5F * sqrt(dx * dx + dy * dy) / radius;
    //if (localPsi > 1) localPsi = 1; // (╯=＿=)╯╧╧
    //length = radius * 2.0F * asin(localPsi);

    // see AliHLTTPCTopoReconstructor::Init()
    // short q = firstParam->GetQPt() > 0 ? 1:-1;

    // locTrack->chi2[0] = firstParam->Chi2();
    // locTrack->chi2[1] = firstParam->Chi2();

    // calculate dpt dpsi dtanl dz0 dy0
    // get cov matrises
    const float *caCov = firstParam->GetCov();

    locTrack->id    =  id;
    locTrack->dedx  =  CATrack->DeDx();
    locTrack->nDedx =  CATrack->NDeDx();
    locTrack->r0    =  r0;
    locTrack->q     =  charge;
    locTrack->phi0  =  phi0;
    locTrack->z0    =  xyz[2];
    locTrack->pt    =  pt ;
    locTrack->psi   =  psi ; // psi        phi+h*PI*0.5
    locTrack->tanl  =  -1. * firstParam->DzDs(); // CA z in opposite direction
    //locTrack->eta   =  locTrack->getRealEta();
    locTrack->dpt   =  sqrt(caCov[14]); //cPP
    locTrack->dtanl =  sqrt(caCov[9]); //cTT
    locTrack->dz0   =  sqrt(caCov[2]);   //cZZ
    locTrack->dpsi  =  sqrt(caCov[5] / (sin(psi)) / sin(psi)); //cEE  do we really care about this error?
    //locTrack->innerMostRow = innerRow;
    //locTrack->outerMostRow = outerRow;
    //locTrack->length  = length;

    locTrack->chi2[0] = firstParam->Chi2();
    locTrack->chi2[1] = firstParam->NDF();

    // TODO	locTrack->para
    locTrack->para = para;

    //pass track hits
    //from outer to inner
    locTrack->firstHit  = NULL;
    locTrack->lastHit   = NULL;

    const int NHits = CATrack->NHits();	
    int NHits_counter = 0;

    memset(xyz,0,sizeof(xyz));
    memset(xyz2,0,sizeof(xyz));

    for ( int iHits = NHits-1; iHits>=0; iHits--){
        //printf(">>> %i\t%i\n",NHits, iHits);
        int index_CA = CAGBTracker->TrackHit(CATrack->FirstHitRef()+iHits);
        int hid      = CAGBTracker->Hit(index_CA).ID();
        int index    = id2index(hid);

        assert(index >= 0 && index < nHits);
        FtfHit *thisHit = &(fHits[index]);

        //	printf("thisHit->id %i: hid %i  %f  %f  %f  %f  %f  %f\n",
        //			thisHit->id, hid, thisHit->x,thisHit->y,thisHit->z,
        //			CAGBTracker->Hit(index_CA).X(),
        //			CAGBTracker->Hit(index_CA).Y(),
        //			CAGBTracker->Hit(index_CA).Z());

        if(thisHit->track){ 
            //printf("ERR online_TPC_CA_tracker::Convert2()  current hit is used by another track!!!\n");
            //printf("thisHit->id %i\n",thisHit->id);
            continue; 
        }

        assert(thisHit->id == hid);
        thisHit->track = locTrack;
        thisHit->nextTrackHit = 0;

        //if(iHits == NHits-1) {
        if(!(locTrack->firstHit)) {
            // set the first hits
            locTrack->innerMostRow = locTrack->outerMostRow = thisHit->row;	
            locTrack->firstHit     = locTrack->lastHit = thisHit;
            xyz[0] = xyz2[0] = thisHit->x;
            xyz[1] = xyz2[1] = thisHit->y;
        }
        else {
            ((FtfBaseHit *)locTrack->lastHit)->nextTrackHit = thisHit;
            locTrack->lastHit = thisHit;
            locTrack->innerMostRow = thisHit->row;
            xyz[0] = thisHit->x;
            xyz[1] = thisHit->y;
        }
        NHits_counter++;
    }

    double dx = xyz[0] - xyz2[0];
    double dy = xyz[1] - xyz2[1];
    double radius = (double) fabs(pt / 2.9979e-3 * bField); // which bField?
    double localPsi = 0.5F * sqrt(dx * dx + dy * dy) / radius;
    if (localPsi > 1) localPsi = 1; // (╯=＿=)╯╧╧
    length = radius * 2.0F * asin(localPsi);
    locTrack->length = length;
    locTrack->nHits = NHits_counter;
}

// ---------------------------------------------------------------------------------------
void online_TPC_CA_tracker::Convert(const AliHLTTPCCAGBTrack *CATrack, gl3Track *locTrack, int id)
{
    // basic idea (╯-_-)╯┴—┴
    // star global to CA local:
    // 1. rotate to sector 12
    // 2. (x,y,z) -> (y,x,-z)
    // const int NHits = CATrack->NHits();

    const AliHLTTPCCATrackParam *firstParam;
    const AliHLTTPCCATrackParam *lastParam;

    firstParam = &(CATrack->InnerParam());
    lastParam  = &(CATrack->OuterParam());

    double length = 0;
    int    innerRow = 1;
    int    outerRow = 45;

    innerRow = x2Row((double)(firstParam->GetX()));
    outerRow = x2Row((double)(lastParam->GetX()));

    //pathLength(firstParame, lastParam, alpha, &length, &innerRow, &outerRow);
    // get parameters
    // ca local -> star tpc sector local
    double localxyz[3], xyz[3];
    localxyz[0]    = firstParam->GetY(); // X <-> Y
    localxyz[1]    = firstParam->GetX();
    localxyz[2]    = -1.*firstParam->GetZ();

    // star tpc sector local -> tpc global
    const double alpha = CATrack->Alpha();
    const double ca = cos(alpha);
    const double sa = sin(alpha);
    xyz[0] =  ca * localxyz[0] + sa * localxyz[1];
    xyz[1] = -sa * localxyz[0] + ca * localxyz[1];
    xyz[2] =  localxyz[2];

    double r0   = sqrt(xyz[0] * xyz[0] + xyz[1] * xyz[1]);
    // double phi0 = atan2(xyz[1] / r0, xyz[0] / r0);
    double phi0 = atan2(xyz[1], xyz[0]);

    // AliHLTTPCCATrackParam globalCAParam = firstParam->GetGlobalParam(CATrack->Alpha());
    // printf(">>>%12.6f %12.6f\n", xyz[0], xyz[1]);
    // printf(">>>%12.6f %12.6f\n\n", globalCAParam.GetX(), globalCAParam.GetY());

    // double phi0 = atan2(globalCAParam.GetX(), globalCAParam.GetY());

    if (phi0 < 0) phi0 += 2 * M_PI;

    double localxyz2[3], xyz2[3];
    localxyz2[0] = lastParam->GetY(); // X <-> Y
    localxyz2[1] = lastParam->GetX();

    xyz2[0] =  ca * localxyz2[0] + sa * localxyz2[1];
    xyz2[1] = -sa * localxyz2[0] + ca * localxyz2[1];

    double pt, localPx, localPy, px, py;
    short charge = 0;
    pt = fabs(1. / firstParam->GetQPt());
    charge = firstParam->GetQPt() == 0 ? 0 : firstParam->GetQPt() / fabs(firstParam->GetQPt());
    charge *= -1; // QPt ~ sign(-q) ?
    
    localPx = pt * firstParam->GetSinPhi(); // X <-> Y
    localPy = pt * firstParam->GetCosPhi();

    px =  ca * localPx + sa * localPy;
    py = -sa * localPx + ca * localPy;

    // double psi = atan2(py / pt, px / pt);// 0~2pi TODO make sure
    double psi = atan2(py, px);// 0~2pi TODO make sure
    if (psi < 0) psi += 2 * M_PI;

    //pathlength
    double dx = xyz[0] - xyz2[0];
    double dy = xyz[1] - xyz2[1];
    double radius = (double) fabs(pt / 2.9979e-3 * bField); // which bField?
    double localPsi = 0.5F * sqrt(dx * dx + dy * dy) / radius;
    if (localPsi > 1) localPsi = 1; // (╯=＿=)╯╧╧
    length = radius * 2.0F * asin(localPsi);

    // see AliHLTTPCTopoReconstructor::Init()
    // short q = firstParam->GetQPt() > 0 ? 1:-1;

    // locTrack->chi2[0] = firstParam->Chi2();
    // locTrack->chi2[1] = firstParam->Chi2();

    // calculate dpt dpsi dtanl dz0 dy0
    // get cov matrises
    const float *caCov = firstParam->GetCov();

    locTrack->id    =  id;
    locTrack->nHits =  CATrack->NHits();
    locTrack->dedx  =  CATrack->DeDx();
    locTrack->nDedx =  CATrack->NDeDx();
    locTrack->r0    =  r0;
    locTrack->q     =  charge;
    locTrack->phi0  =  phi0;
    locTrack->z0    =  xyz[2];
    locTrack->pt    =  pt ;
    locTrack->psi   =  psi ; // psi        phi+h*PI*0.5
    locTrack->tanl  =  -1. * firstParam->DzDs(); // CA z in opposite direction
    locTrack->eta   =  locTrack->getRealEta();
    locTrack->dpt   =  sqrt(caCov[14]); //cPP
    locTrack->dtanl =  sqrt(caCov[9]); //cTT
    locTrack->dz0   =  sqrt(caCov[2]);   //cZZ
    locTrack->dpsi  =  sqrt(caCov[5] / (sin(psi)) / sin(psi)); //cEE  do we really care about this error?
    locTrack->innerMostRow = innerRow;
    locTrack->outerMostRow = outerRow;
    locTrack->length  = length;

    // locTrack->xy_chisq = firstParam->Chi2();
    // locTrack->sz_chisq = firstParam->Chi2();
}

// ---------------------------------------------------------------------------------------
void online_TPC_CA_tracker::Convert(const KFParticle *KFTrack, gl3Track *locTrack, int id)
{
    // KFPartile CA global to STAR tpc global
    // (X Y Z) -> (Y X -Z)
    double xyz[3] = {
        KFTrack->GetY(),
        KFTrack->GetX(),
        -1.*KFTrack->GetZ(),
    };

    double p[3]   = {
        KFTrack->GetPy() , //* y-compoment of 3-momentum
        KFTrack->GetPx() , //* x-compoment of 3-momentum
        -1.*KFTrack->GetPz()  //* z-compoment of 3-momentum
    };

    double r0 = sqrt(xyz[0] * xyz[0] + xyz[1] * xyz[1]);
    double phi0 = atan2(xyz[1] / r0, xyz[0] / r0);
    if (phi0 < 0) phi0 += 2 * M_PI;

    short q = KFTrack->GetQ();
    double pt = sqrt(p[0] * p[0] + p[1] * p[1]);
    double psi = atan2(p[1] / pt, p[0] / pt);// 0~2pi TODO make sure
    if (psi < 0) psi += 2 * M_PI;

    double tanl = p[2] / pt;
    double dpz  = KFTrack->GetErrPz();
    double dpt  = KFTrack->GetErrPt();
    double dtanl = fabs(tanl) * sqrt(dpz * dpz / p[2] / p[2] + dpt * dpt / pt / pt);
    
    locTrack->id    =  id;
    locTrack->r0    =  r0;
    locTrack->phi0  =  phi0;
    locTrack->z0    =  xyz[2];
    locTrack->pt    =  pt * q ;
    locTrack->q     =  q;
    locTrack->psi   =  psi ; // psi        phi+h*PI*0.5
    locTrack->tanl  =  tanl;
    locTrack->eta   =  locTrack->getRealEta();
    // error again do we need to calculate it?
    locTrack->dpt   =  dpt;
    locTrack->dtanl =  dtanl;
    locTrack->dz0   =  KFTrack->GetErrZ();
    locTrack->dpsi  =  0 ;
    // don't know how to get these information
    // getting these from the corresponding CAGBTrack
    // somewhere else
    locTrack->nHits =  0 ;
    locTrack->dedx  =  0 ;
    locTrack->nDedx =  0 ;
    locTrack->innerMostRow = -1 ;
    locTrack->outerMostRow = -1 ;
    locTrack->length  = -1 ;

    // locTrack->xy_chisq = KFTrack->GetChi2(); // what does this chi2 mean ?
    // locTrack->sz_chisq = KFTrack->GetChi2();
}

// ----------------------------------------------------------------------------------
int online_TPC_CA_tracker::fillTracks(char *out_buf,int &out_buf_used)
{
    if (NULL == topoReconstructor) {
        LOG(WARN, "No AliHLTTPCTopoReconstructor found");
    }

    // out_buf_used is also used as input, which tell me the max size of out_buf
    int maxOut = out_buf_used;
    int bytesNeeded = 3*sizeof(float) + // primary vertex
                      sizeof(int) +     // number of global tracks
                      2*nCAGBTracks*sizeof(gl3Track); // global and primary tracks

    float*    pVertex = (float*)(out_buf);
    int*      nTracks = (int*)(out_buf + 3*sizeof(float));
    gl3Track* locTrk  = (gl3Track*)(out_buf + 3*sizeof(float) + sizeof(int));

    if ( bytesNeeded > maxOut ) {
        memset(out_buf, 0, maxOut);

        pVertex[0] = -999;
        pVertex[1] = -999;
        pVertex[2] = -999;

        nTracks = 0;
        
        LOG(ERR, "online_TPC_CA_tracker::fillTracks(): %d bytes needed, %d bytes max, output buffer is too short!\n", bytesNeeded, maxOut);
        return -1;
    }

    out_buf_used = bytesNeeded;
    nGlobalTracks = nCAGBTracks;

    // set all bit to 1
    // bad track will have id = -1
    memset(out_buf, -1, maxOut);

    memcpy(pVertex, primaryVertex, 3*sizeof(float));
    *nTracks = nCAGBTracks;

    // KFPTopoReconstructor& KFReconstructor = topoReconstructor->GetKFPTopoReconstructor();
    // int nKFParticles = KFReconstructor.GetNParticles();
    // // KFPIdMap[i] is the index of its corresponding CA track
    // vector<int>& KF2CAMap = topoReconstructor->GetKF2CAMap();
    // int KFIndex = 0;

    // const double Chi2NDFCut = 3.0;

    // AliHLTTPCCAGBTrack* CATracks = CAGBTracker->Tracks();
    // for (int iTrk = 0; iTrk < nCAGBTracks; ++iTrk) {
    //     // convert global track
    //     AliHLTTPCCAGBTrack* caTrk = CATracks + iTrk;
    //     Convert(caTrk, locTrk, iTrk);
    //     locTrk++;

    //     // convert primary track
    //     // only convert good track, skip bad track
    //     if (KFIndex < nKFParticles && KF2CAMap[KFIndex] == iTrk) {
    //         AliKFParticle& particle = KFReconstructor.GetParticle(KFIndex);
    //         if (particle.GetChi2()/particle.GetNDF() < Chi2NDFCut) {
    //             Convert(&particle, locTrk, iTrk);
    //             // NHits, NDeDx and DeDx are not available in KFParticle
    //             // grab from corresponding CA track
    //             // this is ugly
    //             locTrk->nHits = caTrk->NHits() + 1; // fit with PV
    //             locTrk->nDedx = caTrk->NDeDx();
    //             locTrk->dedx  = caTrk->DeDx();
                
    //             nPrimaryTracks++;
    //         }
    //         KFIndex++;
    //     }
    //     locTrk++;
        
    // } // CA track loop end

    // LOG(NOTE, "nGTrk = %d, nPTrk = %d", nGlobalTracks, nPrimaryTracks);
    
    return 0;
}

// ----------------------------------------------------------------------------------

// #define PRINTTRACK

int online_TPC_CA_tracker::fillTracks2(char *out_buf,int &out_buf_used)
{

#ifdef PRINTTRACK
    static std::ios_base::openmode iomode = std::ios_base::out;
    static int eventId = 0;
#endif
    // out_buf_used is also used as input, which tell me the max size of out_buf
    int maxOut = out_buf_used;
    int bytesNeeded = 3*sizeof(float) + // primary vertex
                      sizeof(int) +     // number of global tracks
                      2*nCAGBTracks*sizeof(gl3Track); // global and primary tracks

    float*    pVertex = (float*)(out_buf);
    int*      nTracks = (int*)(out_buf + 3*sizeof(float));
    gl3Track* locTrk  = (gl3Track*)(out_buf + 3*sizeof(float) + sizeof(int));

    if ( bytesNeeded > maxOut ) {
        memset(out_buf, 0, maxOut);

        pVertex[0] = -999;
        pVertex[1] = -999;
        pVertex[2] = -999;

        nTracks = 0;
        
        LOG(ERR, "online_TPC_CA_tracker::fillTracks2(): %d bytes needed, %d bytes max, output buffer is too short!\n", bytesNeeded, maxOut);
        return -1;
    }

    out_buf_used = bytesNeeded;
    nGlobalTracks = nCAGBTracks;

    // set all bit to 1
    // bad track will have id = -1
    memset(out_buf, -1, maxOut);

    // use beamline to constrain pTrk fit
    pVertex[0] = para->xVertex;
    pVertex[1] = para->yVertex;
    pVertex[2] = para->zVertex;

    // use PV reconstructed by KFParticle
    // para->xVertex = primaryVertex[0];
    // para->yVertex = primaryVertex[1];
    // para->zVertex = primaryVertex[2];    

    // pVertex[0] = primaryVertex[0];
    // pVertex[1] = primaryVertex[1];
    // pVertex[2] = primaryVertex[2];

    *nTracks = nCAGBTracks;

#ifdef PRINTTRACK
    char name[100];
    sprintf(name,"./trackdat/gTrackInfo_%i.dat",eventId);
    std::ofstream gTrackHits(name);
    gTrackHits << std::setprecision(6);
    gTrackHits << std::fixed;

    sprintf(name,"./trackdat/pTrackInfo_%i.dat",eventId);
    std::ofstream pTrackHits(name);
    pTrackHits << std::setprecision(6);
    pTrackHits << std::fixed;
		 

    sprintf(name,"./trackdat/gTrackParInfo_%i.dat",eventId);
    std::ofstream gTrackParHits(name);
    gTrackParHits << std::setprecision(6);
    gTrackParHits << std::fixed;

    sprintf(name,"./trackdat/pTrackParInfo_%i.dat",eventId);
    std::ofstream pTrackParHits(name);
    pTrackParHits << std::setprecision(6);
    pTrackParHits << std::fixed;

    eventId ++ ;
#endif

    AliHLTTPCCAGBTrack* CATracks = CAGBTracker->Tracks();
    for (int iTrk = 0; iTrk < nCAGBTracks; ++iTrk) {
        // convert global track
        AliHLTTPCCAGBTrack* caTrk = CATracks + iTrk;
        FtfTrack *ftfTrk = new FtfTrack();
        Convert2(caTrk, ftfTrk, iTrk);
        locTrk->set(ftfTrk);
        locTrk++;

#ifdef PRINTTRACK
        gTrackHits<<"#"<<iTrk<<endl;
        for ( FtfHit *ihit = (FtfHit *)ftfTrk->firstHit ; 
              ihit != 0 ;
              ihit = (FtfHit *)ihit->nextTrackHit) {
            gTrackHits<<std::setw(10)<<ihit->x 
                      <<" "<<std::setw(10)<<ihit->y 
                      <<" "<<std::setw(10)<<ihit->z
                      <<endl;
        }

        float length = ftfTrk->length*sqrt(1+ftfTrk->tanl*ftfTrk->tanl);
        gTrackParHits<<"#"<<iTrk<<" "
                     <<std::setw( 5)<<1*ftfTrk->q << " "
                     <<std::setw( 5)<<1*ftfTrk->innerMostRow<< " "
                     <<std::setw( 5)<<1*ftfTrk->outerMostRow<< " "
                     <<std::setw(10)<<ftfTrk->length<<" "
                     <<std::setw(10)<<ftfTrk->tanl<< " "
                     <<std::setw(10)<<ftfTrk->psi<< " "
                     <<std::setw(10)<<ftfTrk->r0*cos(ftfTrk->phi0)<< " "
                     <<std::setw(10)<<ftfTrk->r0*sin(ftfTrk->phi0)<< " "
                     <<std::setw(10)<<ftfTrk->z0
                     <<endl;

        for(int i = 0 ; i < 51 ; i++){

            Ftf3DHit hit3D = ftfTrk->extraRadius(200./50.*i);  

            gTrackParHits<<std::setw(10)<<hit3D.x 
                         <<" "<<std::setw(10)<<hit3D.y 
                         <<" "<<std::setw(10)<<hit3D.z
                         <<endl;
        }

        gTrackHits<<endl<<endl;
        gTrackParHits<<endl<<endl;
#endif
        if(ftfTrk->nHits > 5){
            ftfTrk->getPara()->vertexConstrainedFit = 1;
            ftfTrk->fitHelix();
            locTrk->set(ftfTrk);
#ifdef PRINTTRACK
            pTrackHits<<"#"<<iTrk<<endl;
            for ( FtfHit *ihit = (FtfHit *)ftfTrk->firstHit ; 
                  ihit != 0 ;
                  ihit = (FtfHit *)ihit->nextTrackHit) {
                pTrackHits<<std::setw(10)<<ihit->x 
                          <<" "<<std::setw(10)<<ihit->y 
                          <<" "<<std::setw(10)<<ihit->z
                          <<endl;
            }
            pTrackHits<<endl<<endl;

            length = ftfTrk->length*sqrt(1+ftfTrk->tanl*ftfTrk->tanl);
            pTrackParHits<<"#"<<iTrk<<" "
                         <<std::setw( 5)<<1*ftfTrk->q << " "
                         <<std::setw( 5)<<1*ftfTrk->innerMostRow<< " "
                         <<std::setw( 5)<<1*ftfTrk->outerMostRow<< " "
                         <<std::setw(10)<<ftfTrk->length<<" "
                         <<std::setw(10)<<ftfTrk->tanl<< " "
                         <<std::setw(10)<<ftfTrk->psi<< " "
                         <<std::setw(10)<<ftfTrk->r0*cos(ftfTrk->phi0)<< " "
                         <<std::setw(10)<<ftfTrk->r0*sin(ftfTrk->phi0)<< " "
                         <<std::setw(10)<<ftfTrk->z0
                         <<endl;
            for(int i = 0 ; i<50+1 ; i++){

                Ftf3DHit hit3D =ftfTrk->extraRadius(200./50.*i);  

                pTrackParHits<<std::setw(10)<<hit3D.x 
                             <<" "<<std::setw(10)<<hit3D.y 
                             <<" "<<std::setw(10)<<hit3D.z
                             <<endl;
            }

            pTrackParHits<<endl<<endl;
#endif
        } else {
            locTrk->id = -1;
        }

        locTrk++;
        delete ftfTrk;

    } // CA track loop end
    LOG(NOTE, "gl3Track :: nTracks %i, bytesNeeded %i ",nCAGBTracks, bytesNeeded);

    return 0;
}

// -----------------------------------------------------------------------------
int online_TPC_CA_tracker::fillTracks(HLTTPCData_t* out_buf,int &out_buf_used)
{
    HLTTPCData& tpcData = out_buf->tpcOutput;

    if (nCAGBTracks > HLTTPCData::NTRACKSMAX) {
        // LOG(WARN, "Too many CA global tracks, %d, will trancate to %d",
        //     nCAGBTracks, HLTTPCData::NTRACKSMAX);

        LOG(WARN, "Too many CA global tracks, %d > %d, I will not process it",
            nCAGBTracks, HLTTPCData::NTRACKSMAX);
        tpcData.primaryVertex[0] = -999;
        tpcData.primaryVertex[1] = -999;
        tpcData.primaryVertex[2] = -999;
        tpcData.nTracks          = 0;

        nGlobalTracks = 0;
        out_buf_used = sizeof(HLTTPCData) -
            (HLTTPCData::NTRACKSMAX - nGlobalTracks) * sizeof(gl3TrackPair);
    
        LOG(NOTE, ">>> %10.2f%10.2f%10.2f%10d", tpcData.primaryVertex[0],
            tpcData.primaryVertex[1], tpcData.primaryVertex[2], nGlobalTracks);
        return 0;
    }
    
    nGlobalTracks = nCAGBTracks <= HLTTPCData::NTRACKSMAX ? nCAGBTracks : HLTTPCData::NTRACKSMAX;

    // unless we construct PV in CA, otherwise no usefull information
    // gl3 also have beamline info, no need to pass to it here
    tpcData.primaryVertex[0] = para->xVertex;
    tpcData.primaryVertex[1] = para->yVertex;
    tpcData.primaryVertex[2] = para->zVertex;
    tpcData.nTracks          = 0;

    LOG(NOTE, "beamline %10.4f %10.4f %10.4f", para->xVertex, para->yVertex, para->zVertex);
    
    AliHLTTPCCAGBTrack* CATracks = CAGBTracker->Tracks();
    nGlobalTracks = 0;
    
    for (int iTrk = 0; iTrk < nCAGBTracks && iTrk < HLTTPCData::NTRACKSMAX; ++iTrk) {
        // convert global track
        AliHLTTPCCAGBTrack* caTrk = CATracks + iTrk;
        FtfTrack *ftfTrk = new FtfTrack();
        Convert2(caTrk, ftfTrk, iTrk);

        if (ftfTrk->nHits >= 5) {
            tpcData.trackPairs[nGlobalTracks].globalTrack.set(ftfTrk);
            ftfTrk->getPara()->vertexConstrainedFit = 1;
            ftfTrk->fitHelix();
            tpcData.trackPairs[nGlobalTracks].primaryTrack.set(ftfTrk);
            nGlobalTracks++;
        }

        delete ftfTrk;
    } // CA track loop end
    
    tpcData.nTracks = nGlobalTracks;
    
    out_buf_used = sizeof(HLTTPCData) -
        (HLTTPCData::NTRACKSMAX - nGlobalTracks) * sizeof(gl3TrackPair);
        
    LOG(NOTE, "gl3Track :: nTracks %i, bytes used %i ",nCAGBTracks, out_buf_used);
    LOG(NOTE, ">>> %10.2f%10.2f%10.2f%10d", tpcData.primaryVertex[0],
        tpcData.primaryVertex[1], tpcData.primaryVertex[2], nGlobalTracks);

    return 0;
}
// ----------------------------------------------------------------------------------
int online_TPC_CA_tracker::x2Row(double x)
{
    const int NRows = 45;
    // pad width in CA localX direction
    float dpadin = 1.15 / 2.;
    float dpadout = 1.95 / 2. ;
    float R[NRows] = {  60.000,  64.800,  69.600,   74.400,  79.200, //  5
                        84.000,  88.800,  93.600,   98.800, 104.000, // 10
                        109.200, 114.400, 119.600, 127.195, 129.195, // 15
                        131.195, 133.195, 135.195, 137.195, 139.195, // 20
                        141.195, 143.195, 145.195, 147.195, 149.195, // 25
                        151.195, 153.195, 155.195, 157.195, 159.195, // 30
                        161.195, 163.195, 165.195, 167.195, 169.195, // 35
                        171.195, 173.195, 175.195, 177.195, 179.195, // 40
                        181.195, 183.195, 185.195, 187.195, 189.195};// 45

    for (int i = 0; i < NRows; i++) {
        if (x < R[i]) {
            if (i > 12) return x > (R[i] - dpadout) ? i + 1 : i;
            else return x > (R[i] - dpadin) ? i + 1 : i;
        }
    }

    return NRows;
}

// ----------------------------------------------------------------------------------
#ifdef WITHSCIF
void online_TPC_CA_tracker::setKFPClient()
{
    isSCIFConnected       = false;
    uint16_t localPort    = scif_client::localPortStart + myId;
    uint16_t remoteNode   = myId % scif_client::nXeonPhi + 1; // node number starts from zero
    uint16_t remoteId     = myId;
    uint16_t remotePort   = scif_client::remotePortStart + remoteId;
    float    bFieldForKFP = -bField * 10; // same as CA settings

    char cmd[256];
    char KFPOutdir[512];
    bool isKFPOutdirOk = false;
    sprintf(KFPOutdir, "/home/kehw/KFPHistograms/%d/%d", runnumber, myId);
    sprintf(cmd, "mkdir -p %s", KFPOutdir);
    system(cmd);

    usleep(200*myId);

    // if (0 == myId || 1 == myId) {
    //     sprintf(cmd, "/usr/bin/ssh -T mic%d \"/usr/bin/killall -9 KFPServer\"", remoteNode-1);
    //     LOG("THLT", "%s", cmd);
    //     sleep(2);
    // } else {
    //     sleep(3);
    // }
    
    sprintf(cmd, "/usr/bin/ssh -T mic%d \"./run_KFPServer.sh %d %d\"", remoteNode-1, remoteId, runnumber);
    LOG("THLT", "%s", cmd);
    LOG("THLT", "%i %i %i", localPort, remoteNode, remotePort);
    int retry = 20;
    bool KFPStarted = false;
    while (retry--) {
        if ( -1 == system(cmd) ) { // error
            LOG(ERR, "launch KFPServer failed, try %i", retry);
            usleep(500);
        } else {
            KFPStarted = true;
            break;
        }
    }
    
    if (!KFPStarted) {
        kfpClient = NULL;
        return;
    }
    
    sleep(5);
    
    kfpClient = new scif_KFP_client(myId, localPort, remoteNode, remotePort,
                                    topoReconstructor, bFieldForKFP, runnumber);
    if ( kfpClient->connect() > 0 ) {
        isSCIFConnected = true;
        kfpClient->sendBz();
        kfpClient->sendRunnumber();
        kfpClient->setupDMA();
    }
}

// ----------------------------------------------------------------------------------
void online_TPC_CA_tracker::closeKFPConnection()
{
    LOG("THLT", "close SCIF connection");
    kfpClient->closeConnection();
}
#endif  // WITHSCIF
