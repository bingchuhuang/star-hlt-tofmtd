#include <sys/types.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstdlib>
#include <rtsLog.h>
#include <ctime>
#include <string.h>

#include "tof_hlt_sector.h"
//using std::endl;
using namespace std;
tof_hlt_sector::tof_hlt_sector(char* pName)
{
	//std::clock_t initTimer;
	//initTimer = std::clock();
	sector = -1 ;	// un-asigned!
	evt_counter = 0 ;
	readparameters(pName);
	//double elapsedTime = (std::clock()-initTimer)/(double)CLOCKS_PER_SEC;
	//LOG(INFO,"CPU time for TOF initialization : %f sec",  elapsedTime);
	timecount = 0.;
	return ;
}

tof_hlt_sector::~tof_hlt_sector()
{
	return ;
}

int tof_hlt_sector::new_event()
{
	evt_counter++ ;
	//LOG(NOTE,"Event %d: new",evt_counter) ;
	// clear structures, counters etc in preparation of a new event...

	return 0 ;
}
void tof_hlt_sector::readparameters(char* pName){
	// read pVPD corr parameters
	char fname[256];
	sprintf(fname,"%spvpdCali_4DB.dat",pName);
	std::ifstream indata(fname);
	int nchl, nbin;
	if(!indata){
		LOG(ERR,"file %s doesnot exist!",fname);
	}
	for(int i=0;i<=nPVPDChannel;i++){
		indata>>nchl;
		indata>>nbin;
		for(int j=0;j<=nbin;j++){
			indata>>pvpdTotEdge[i][j];
		}
		for(int j=0;j<=nbin;j++){
			indata>>pvpdTotCorr[i][j];
		}
		for(int j=nbin+1; j<=maxpvpdNBin;j++)
		  {
		    pvpdTotEdge[i][j] = 0;
		    pvpdTotCorr[i][j] = 0;
		  }
	}
	indata.close();
	//   read tot correction parameters
	int itray, imodule, iboard, icell, imaxN;
	sprintf(fname,"%stotCali_4DB.dat",pName);
	std::ifstream indataTot(fname);
	if(!indataTot){
		LOG(ERR,"file %s doesnot exist! (%s)",fname, strerror(errno));
	}
	for(int i=0;i<nTray;i++){
		for(int j=0;j<nBoard;j++){
		  int nBins;
			indataTot>>itray>>iboard>>nBins;
			for(int k=0;k<=nBins;k++){
				indataTot>>tofTotEdge[i][j][k];
			}
			for(int k=0;k<=nBins;k++){
				indataTot>>tofTotCorr[i][j][k];
			}
			for(int k=nBins+1; k<=maxNBin; k++)
			  {
			    tofTotEdge[i][j][k] = 0;
			    tofTotCorr[i][j][k] = 0;
			  }
		}
	}
	indataTot.close();

	// read t0
	sprintf(fname,"%st0_4DB.dat",pName);
	std::ifstream indataT0(fname);
	if(!indataT0){
		LOG(ERR,"file %s doesnot exist!",fname);
	}
	for(int i=0;i<nTray;i++){
		for(int j=0;j<nModule;j++){
			for(int k=0;k<nCell;k++){
				indataT0>>itray>>imodule>>icell>>tofT0Corr[i][j][k];
			}
		}
	}
	indataT0.close();
	//red T0 paramters;

	sprintf(fname,"%sfiberpams.dat",pName);
	std::ifstream inpams(fname);
	if(!inpams){
		LOG(ERR,"file %s doesnot exist!",fname);
	}

	while(inpams){
		char pname[100]=" ";
		inpams >> pname;
		if(strcmp(pname,"timedelay")==0){
			for(int i=0;i<38;i++) inpams>>timedelay[i];
		}
		if(strcmp(pname,"delayminusfirstchannel")==0){
			for(int i=0;i<38;i++) inpams>>delayminusfirstchannel[i];
		}
		if(strcmp(pname,"delay")==0){
			for(int i=0;i<38;i++){
				inpams>>delay[i];
			}
		}
		if(strcmp(pname,"mVpdDelay")==0){
			for(int i=0;i<38;i++){
				inpams>>mVpdDelay[i];
				mVpdDelay[i] += delay[i];
			}
		}
		if(strcmp(pname,"cutlow")==0){
			for(int i=0;i<122;i++) 
			{
				inpams>>cutlow[i];
			}
		}
		if(strcmp(pname,"cuthi")==0){
			for(int i=0;i<122;i++){
				inpams>>cuthi[i];
			}
		}
		if(strcmp(pname,"mMRPC2TDIGChan")==0){
			for(int i=0;i<192;i++) inpams>>mMRPC2TDIGChan[i];
		}
		if(strcmp(pname,"mTDIG2MRPCChan")==0){
			for(int i=0;i<192;i++) inpams>>mTDIG2MRPCChan[i];
		}
		if(strcmp(pname,"upvpdLEchan")==0){
			for(int i=0;i<38;i++) inpams>>upvpdLEchan[i];
		}
		if(strcmp(pname,"upvpdTEchan")==0){
			for(int i=0;i<38;i++){
				inpams>>upvpdTEchan[i];
			}
		}

	}
	inpams.close();

	sprintf(fname,"%stofTDIG.dat",pName);
	std::ifstream intdig(fname);
	if(!intdig){
		LOG(ERR,"file %s doesnot exist!",fname);
	}
	while(intdig){
		char pname[100] = " ";
		intdig >> pname;
		if(strcmp(pname,"mTdigOnTray")==0){
			for(int i=0;i<mNTray;i++){
				for(int j=0;j<mNTDIGOnTray;j++){
					intdig >> mTdigOnTray[i][j];
				}
			}
		}
		if(strcmp(pname,"mTdigOnEastVpd")==0){
			for(int j=0;j<mNTDIGOnTray;j++){
				intdig >> mTdigOnEastVpd[j];
			}
		}

		if(strcmp(pname,"mTdigOnWestVpd")==0){
			for(int j=0;j<mNTDIGOnTray;j++){
				intdig >> mTdigOnWestVpd[j];
			}
		}
		if(strcmp(pname, "mBoardId")==0){
			for(int j=0;j<mNTDIGMAX;j++){
				intdig >> mBoardId[j];
			}
		}
		if(strcmp(pname,"mBoardId2Index")==0){
			for(int j=0;j<mNBoardIdMAX;j++){
				intdig >>mBoardId2Index[j];
			}
		}
	}
	intdig.close();
	
	sprintf(fname,"%stofINLCorr.bin",pName);
	FILE* fINL = fopen(fname,"r");
	if(!fINL){
		LOG(ERR,"file %s doesnot exist!",fname);
	}
	if(fINL){
		fread(mINLCorr,sizeof(mINLCorr),1,fINL);
		fclose(fINL);
	}
	return;	
}

// read one RDO's worth of data. 
// rdo_1 starts at 1
int tof_hlt_sector::read_rdo_event(int rdo_1, char *start, int words_len)
{

	//LOG(NOTE,"Event %d: RDO %d: read %d words",evt_counter, rdo_1, words_len) ;
	u_int *ui = (u_int *)start ;
	std::clock_t starttime;
	starttime = std::clock();

	TofRawHit mCellData;

	int nTof=0;
	int r = rdo_1-1;
	mCellData.fiberid = r;
	mCellData.triggertime = 0;
	int halftrayid  = -99;
	int trayid      = -99;

	for(int i=0;i<words_len;i++) {
		//LOG(NOTE,"\t %2d: 0x%08X",i,ui[i]) ;
		//if(i<10) LOG(TERR,"\t %2d: 0x%08X",i,ui[i]) ;

		// do what you want with the raw RDO data which is in start and
		// has words_len words (i.e. bytes=words_len * 4)

		unsigned int dataword = ui[i];
		if( (dataword&0xF0000000)>>28 == 0xD) continue;  /// header tag word
		if( (dataword&0xF0000000)>>28 == 0xE) continue;  /// TDIG separator word
		if( (dataword&0xF0000000)>>28 == 0xA) {  /// header trigger data flag    
			///  do nothing at this moment.
			continue;
		}
		if( (dataword&0xF0000000)>>28 == 0x2) {   /// trigger time here.
			if(mCellData.triggertime==0)
			{
				mCellData.triggertime = 25.*(dataword & 0xfff);
			}
			continue; 
		}
		if( (dataword&0xF0000000)>>28 == 0xC) {   /// geographical data
			halftrayid = dataword&0x01;
			trayid     = (dataword&0x0FE)>>1;
			continue;
		}
		if(halftrayid<0) continue;
		if(trayid<1 || (trayid >122 && trayid != 124)) {
			//        LOG_ERROR<<"StBTofHitMaker::DATA ERROR!! unexpected trayid ! "<<endm;
			continue;
		}
		int edgeid =int( (dataword & 0xf0000000)>>28 );
		if((edgeid !=4) && (edgeid!=5)) continue;   /// not leading or trailing

		int tdcid=(dataword & 0x0F000000)>>24;  /// 0-15
		int tdigid=tdcid/4;   /// 0-3 for half tray.
		int tdcchan=(dataword&0x00E00000)>>21;         /// tdcchan is 0-7 here.
		///
		//int flag = (edgeid==4) ? r+1 : -(r+1);

#ifndef RUN13GMT
		mCellData.trayid = trayid;
		mCellData.globaltdcchan = tdcchan + (tdcid%4)*8+tdigid*24+halftrayid*96;
#else
		//////////////////////////////////////////////////////
		// add selection for GMT ---- Run13
		// tdigid 0 and 7 are removes for GMT, and tdigid 1 is 
		// changed to 0 --- by Yi Guo
		//////////////////////////////////////////////////////

		/// For Run13+ the GMT trays (8,23,93,108) the  TDIGs at tdigid==1 identifies itself as tdigid==0.
		bool isGMT = (trayid==8) || (trayid==23) || (trayid==93) || (trayid==108);	

		mCellData.trayid = trayid;

		if((isGMT) && (tdigid==0)) {
		    mCellData.globaltdcchan = tdcchan + (tdcid%4)*8+1*24+halftrayid*96;
		}
		else if ((isGMT) && ((halftrayid==0 && tdigid==1) || (halftrayid==1 && tdigid==3))){
		    LOG(ERR,"Unexpected TDIG-Id %i in TOF GMT tray %i", tdigid, trayid);
		}
		else { 
		    mCellData.globaltdcchan = tdcchan + (tdcid%4)*8+tdigid*24+halftrayid*96;
		}
#endif
		unsigned int timeinbin = ((dataword&0x7ffff)<<2)+((dataword>>19)&0x03);
		unsigned bin = timeinbin&0x3ff;

		int iTdig = mCellData.globaltdcchan/mNChanOnTDIG; //0-7
		int boardId = -1;
		if(trayid==121){ // west pvpd 
			boardId = mTdigOnWestVpd[iTdig];
		}else if(trayid==122){ //east pvpd
			boardId = mTdigOnEastVpd[iTdig];
		}else{
			boardId = mTdigOnTray[trayid-1][iTdig];
		}
		int index = mBoardId2Index[boardId];
		int chan = mCellData.globaltdcchan%mNChanOnTDIG;//0-23
		double corr = mINLCorr[index][chan][bin]/100.;
		double tmptdc_f = timeinbin+corr;
		mCellData.tdc = tmptdc_f * 25./1024;

		if(edgeid == 4) {     /// leading edge data
			TofLeadingHits.push_back(mCellData);
		} else if (edgeid==5){     /// trailing edge data
			TofTrailingHits.push_back(mCellData);
		}  else {
			LOG(INFO,"Unknown TDC data type ! ");
			continue;
		}

		nTof++;

	}//end one event
	double elapsedTime = (std::clock()-starttime)/(double)CLOCKS_PER_SEC;
	//LOG(INFO,"CPU time for TOF read rdo, : %f sec",  elapsedTime);

	timecount+=elapsedTime;	
	return 0 ;
}


int tof_hlt_sector::do_event(TofSend *dst, int bytes_alloced)
{
	int bytes_used = 0 ;
	//LOG(NOTE,"Event %d: max: %d bytes",evt_counter,bytes_alloced) ;

	// analyze this event and store the results in "dst"
	// returning number of bytes used but taking care not	
	// to go above bytes_alloced!

	std::clock_t start;
	start = std::clock();

	// pass raw hits
	TofSend *tof_send  = dst;
	int nLead = TofLeadingHits.size();
	int nTrail = TofTrailingHits.size();

	//int upvpdLEchan[38]={142,122,118,98,46,26,22,2,112,101,24,136,123,120,99,40,27,16,3,   //west
	//	142,122,118,98,46,26,22,2,112,101,24,136,123,120,99,40,27,16,3};  //east
	//int upvpdTEchan[38]={129,131,105,107,33,35,9,11,109,110,39,133,132,135,108,37,36,13,12,  //west
	//	129,131,105,107,33,35,9,11,109,110,39,133,132,135,108,37,36,13,12}; //east

	/*double timedelay[38]={11.852,11.928,11.764,11.899,11.915,23.927,11.735,11.925,-0.0139,0.0381,11.92,0.1426,-0.0747,24.103,24.046,0.0619,0.0657,0.0325,-0.0261,  //west
	  11.915,10.695,11.913,11.915,11.814,24.026,11.934,11.8,  0.0666,-0.4998,11.876,0,    0.022,  24.016,24.284,0.0457,0.0551,0.0191,-0.03};   //east

	  double delayminusfirstchannel[38]={
	  0,-0.564753,-4.62291,-4.84402,-4.05943,6.32389,-34.4035,-35.3113,-17.0374,-17.3734,-6.04608,-11.9614,-12.7579,8.79609,3.8467,-17.2994,-17.6424,-46.4749,-47.9736,
	  62.4177,60.2293,57.5805,55.8839,58.0891,68.7116,28.5345,27.2963,45.803,44.4412,57.1921,50.2751,50.1588,70.8001,67.9702,45.8064,45.9848,16.0924,16.2548
	  };*/

	/*float delay[38]={
	  0,-0.564753,-4.62291,-4.84402,-4.05943,6.32389,-9.4035,-10.3113,-17.0374,-17.3734,-6.04608,-11.9614,-12.7579,8.79609,3.8467,-17.2994,-17.6424,-21.4749,-22.9736,
	  0,-2.1707,  -4.8195, -6.5161, -4.3109, 6.3116, -8.8655,-10.1037,-16.5970,-17.9588,-5.2079, -12.1249,-12.2412,8.4001, 5.5702,-16.5936,-16.4152,-21.3076,-21.1452
	  };
	  double mVpdDelay[38]={0.};
	  for(int i=0;i<38;i++) {
	  mVpdDelay[i] = delay[i];
	  }
	// additional delay due to trigger time drift in some boards
	mVpdDelay[6]  -= 25.0;
	mVpdDelay[7]  -= 25.0;
	mVpdDelay[17] -= 25.0;
	mVpdDelay[18] -= 25.0;
	mVpdDelay[25] -= 25.0;
	mVpdDelay[26] -= 25.0;
	mVpdDelay[36] -= 25.0;
	mVpdDelay[37] -= 25.0;

	 */


	// index is MRPC channel ID
	//const int mMRPC2TDIGChan[192]={14,5,10,0,15,7,6,11,2,20,4,23,22,1,19,3,18,8,9,21,12,17,13,16,38,29,34,24,39,31,30,35,26,44,28,47,46,25,43,27,42,32,33,45,36,41,37,40,62,53,58,48,63,55,54,59,50,68,52,71,70,49,67,51,66,56,57,69,60,65,61,64,86,77,82,72,87,79,78,83,74,92,76,95,94,73,91,75,90,80,81,93,84,89,85,88,110,101,106,96,111,103,102,107,98,116,100,119,118,97,115,99,114,104,105,117,108,113,109,112,134,125,130,120,135,127,126,131,122,140,124,143,142,121,139,123,138,128,129,141,132,137,133,136,158,149,154,144,159,151,150,155,146,164,148,167,166,145,163,147,162,152,153,165,156,161,157,160,182,173,178,168,183,175,174,179,170,188,172,191,190,169,187,171,186,176,177,189,180,185,181,184};

	// input TDIG index, give corresponding MPRC channel.
	//const int mTDIG2MRPCChan[192]={3,13,8,15,10,1,6,5,17,18,2,7,20,22,0,4,23,21,16,14,9,19,12,11,27,37,32,39,34,25,30,29,41,42,26,31,44,46,24,28,47,45,40,38,33,43,36,35,51,61,56,63,58,49,54,53,65,66,50,55,68,70,48,52,71,69,64,62,57,67,60,59,75,85,80,87,82,73,78,77,89,90,74,79,92,94,72,76,95,93,88,86,81,91,84,83,99,109,104,111,106,97,102,101,113,114,98,103,116,118,96,100,119,117,112,110,105,115,108,107,123,133,128,135,130,121,126,125,137,138,122,127,140,142,120,124,143,141,136,134,129,139,132,131,147,157,152,159,154,145,150,149,161,162,146,151,164,166,144,148,167,165,160,158,153,163,156,155,171,181,176,183,178,169,174,173,185,186,170,175,188,190,168,172,191,189,184,182,177,187,180,179};

	int iHits = 0;// number of hits to be send
	//don't need sort
	//std::sort(TofLeadingHits.begin(),TofLeadingHits.end(),tof_hlt_sector::optdc);
	//std::sort(TofTrailingHits.begin(),TofTrailingHits.end(),tof_hlt_sector::optdc);

	vector<TofRawHit>::iterator itlead;
	vector<TofRawHit>::iterator ittrail;
	//static const double   timestamp=0.0244140625;

	int LErdo = -1;
	int TErdo = -1;
	itlead= TofLeadingHits.begin();
	ittrail= TofTrailingHits.begin();
	while(itlead<TofLeadingHits.end()&&ittrail<TofTrailingHits.end()){
		LErdo = (*itlead).fiberid;
		TErdo = (*ittrail).fiberid;
		if(LErdo==TErdo){
			int LEtray = (*itlead).trayid;
			int TEtray = (*ittrail).trayid;
			if(LEtray==TEtray){
			vector<TofRawHit> tmpTofLEHits;
			vector<TofRawHit> tmpTofTEHits;
			for(;itlead<TofLeadingHits.end();itlead++){
				int tmpLErdo = (*itlead).fiberid;
				int tmpLEtray = (*itlead).trayid;
				if(LErdo==tmpLErdo&&LEtray==tmpLEtray){
					tmpTofLEHits.push_back((*itlead));
				}else{
					break;
				}
			}
			for(;ittrail<TofTrailingHits.end();ittrail++){
				int tmpTErdo = (*ittrail).fiberid;
				int tmpTEtray = (*ittrail).trayid;
				if(TErdo==tmpTErdo&&TEtray==tmpTEtray){
					tmpTofTEHits.push_back((*ittrail));
				}else{
					break;
				}
			}
			vector<TofRawHit>::iterator itleadtmp;
			vector<TofRawHit>::iterator ittrailtmp;
			for(ittrailtmp=tmpTofTEHits.begin();ittrailtmp<tmpTofTEHits.end();ittrailtmp++){
				for(itleadtmp=tmpTofLEHits.begin();itleadtmp<tmpTofLEHits.end();itleadtmp++){

					if((*itleadtmp).fiberid!=(*ittrailtmp).fiberid) continue;
					if((*itleadtmp).trayid!=(*ittrailtmp).trayid) continue;
					float tDiff = (*itleadtmp).tdc-(*itleadtmp).triggertime;
					while(tDiff<0) tDiff += 51200;

					float tot = (*ittrailtmp).tdc-(*itleadtmp).tdc;
					int tray = (*itleadtmp).trayid-1;

					if(tray==120){ //west
						int pvpdchan = -1;
						//LOG(INFO,"tray:%d, LE chan:%d, TE chan:%d",tray,(*itleadtmp).globaltdcchan,(*ittrailtmp).globaltdcchan);
						for(int i=0;i<19;i++){
							if((*itleadtmp).globaltdcchan==upvpdLEchan[i] && (*ittrailtmp).globaltdcchan==upvpdTEchan[i]){
								pvpdchan = i; break;
							}
						}
						if(pvpdchan<0) continue;
						double tdcvpd = tDiff - mVpdDelay[pvpdchan];
						if(tdcvpd>cuthi[tray] || tdcvpd<cutlow[tray]) continue;//loose cut
						if(tot<0 || tot>100) continue;
						tof_send->cell[iHits].trayid = tray;
						tof_send->cell[iHits].channel = pvpdchan;
						tof_send->cell[iHits].triggertime = (*itleadtmp).triggertime;
						tof_send->cell[iHits].tdc = (*itleadtmp).tdc;
						tof_send->cell[iHits].tot = tot;
						tof_send->cell[iHits].tof = -9999.;
						iHits++;
					}else if(tray==121){//east
						int pvpdchan = -1;
						for(int i=0;i<19;i++){
							if((*itleadtmp).globaltdcchan==upvpdLEchan[i+19] && (*ittrailtmp).globaltdcchan==upvpdTEchan[i+19]){
								pvpdchan = i; break;
							}
						}
						if(pvpdchan<0) continue;

						double tdcvpd = tDiff - mVpdDelay[19+pvpdchan];
						if(tdcvpd>cuthi[tray] || tdcvpd<cutlow[tray]) continue;
						if(tot<0 || tot>100) continue;
						tof_send->cell[iHits].trayid = tray;
						tof_send->cell[iHits].channel = pvpdchan;
						tof_send->cell[iHits].triggertime = (*itleadtmp).triggertime;
						tof_send->cell[iHits].tdc = (*itleadtmp).tdc;
						tof_send->cell[iHits].tot = tot;
						tof_send->cell[iHits].tof = -9999.;
						iHits++;
					}else{
						if((*itleadtmp).globaltdcchan!=(*ittrailtmp).globaltdcchan) continue;	
						if((*itleadtmp).tdc>(*ittrailtmp).tdc) continue;
						if(tDiff>cuthi[tray] || tDiff<cutlow[tray]) continue;
						if(tot<0 || tot>100) continue;

						//accept hits
						int TDIGChan = (*itleadtmp).globaltdcchan;

						tof_send->cell[iHits].trayid = tray;
						tof_send->cell[iHits].channel = mTDIG2MRPCChan[TDIGChan];
						tof_send->cell[iHits].triggertime = (*itleadtmp).triggertime;
						tof_send->cell[iHits].tdc = (*itleadtmp).tdc;
						tof_send->cell[iHits].tot = tot;
						tof_send->cell[iHits].tof = -9999.;
						iHits++;	
					}
				}
			}
			tmpTofLEHits.clear();
			tmpTofTEHits.clear();
			}else if(LEtray>TEtray){
				ittrail++;
			}else{
				itlead++;
			}
		}else if(LErdo>TErdo){
			ittrail++;
		}else{
			itlead++;
		}
	}// end of Method 1

	TofLeadingHits.clear();
	TofTrailingHits.clear();

	// do tot and t0 correction
	int nHits = iHits;
	const float phaseDiff = 0;
	for(int i=0;i<nHits;i++){
		int tray = tof_send->cell[i].trayid;
		int channel = tof_send->cell[i].channel;
		int module = channel/6;
		int cell = channel%6;
		float tdc = tof_send->cell[i].tdc;
		float tot = tof_send->cell[i].tot;
		tdc-=phaseDiff;
		while(tdc>51200) tdc-=51200;
		if( tray<0 || tray>121) continue;
		if(module>=nModule || module<0) continue;
		if(cell>=nCell || cell<0) continue;

		if(tray==120 || tray==121){ // 120 west, 121 east
			int ipb = -1;
			if(tray==121) channel += 19;
			for(int k=0;k<=maxpvpdNBin;k++){
				if(tot>pvpdTotEdge[channel][k] && tot<pvpdTotEdge[channel][k+1]){
					ipb = k;
					break;
				}
			}
			if(ipb<0) continue;
			float	cor = linearInter(pvpdTotEdge[channel][ipb],pvpdTotEdge[channel][ipb+1],
					pvpdTotCorr[channel][ipb],pvpdTotCorr[channel][ipb+1], tot);
			tof_send->cell[i].tof = tof_send->cell[i].tdc - cor;
		}else{

			int board = module/(nModule/nBoard);
			float corrT0 = tofT0Corr[tray][module][cell];

			//tot
			int ibin = -1;
			for(int k=0;k<maxNBin;k++){
				if(tot>=tofTotEdge[tray][board][k]&&tot<tofTotEdge[tray][board][k+1]) {
					ibin = k;
					break;
				}
			}
			if(ibin<0) continue;
			float corrTot = linearInter(tofTotEdge[tray][board][ibin],tofTotEdge[tray][board][ibin+1],
					tofTotCorr[tray][board][ibin],tofTotCorr[tray][board][ibin+1], tot);
			if(tofTotEdge[tray][board][ibin]==tofTotEdge[tray][board][ibin+1]) continue;
			float tof = tdc- corrT0 - corrTot;// tot + t0 corr
			tof_send->cell[i].tof = tof;
		}
	}

	tof_send->nHits = nHits;
	int nBytes = sizeof(struct TofSend)-(MAX_TOF_HITS-nHits)*sizeof(struct TofCell);
	bytes_used = nBytes;

	//double elapsedTime = (std::clock()-start)/(double)CLOCKS_PER_SEC;
	//LOG(INFO,"CPU time for TOF do_event : %f sec",  elapsedTime);
	//timecount += elapsedTime;
	return bytes_used ;	// return number of bytes used!
}

float tof_hlt_sector::linearInter(float x1, float x2, float y1, float y2, float x){

	return ((x-x1)*y2+(x2-x)*y1)/(x2-x1);
}

