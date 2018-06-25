#include <sys/types.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <stdlib.h>
#include "gl3TOF.h"
#include <rtsLog.h>
#include "gl3Node.h"
#include "gl3Bischel.h"
#include "FtfBaseHit.h"
#include <string.h>

#include <LIBJML/ThreadManager.h>

gl3TOF::gl3TOF(char* tofAlignGeom, char* zCali_4DB, char* parametersDirectory){
	tofhits = new TofEvent;
        tofhits->nHits = 0;
        
	pvpdhits = new TofEvent;
        pvpdhits->nHits = 0;
        
        bischel = new gl3Bischel(parametersDirectory);
	Init(tofAlignGeom, zCali_4DB);
}

gl3TOF::~gl3TOF(){
	if(tofhits) delete [] tofhits;
	if(pvpdhits) delete [] pvpdhits;
}

int gl3TOF::Init(char* tofAlignGeom, char* zCali_4DB){

	readzCali(zCali_4DB);
	evt_counter = 0;

	//debug = 0;// debug mode
	//mAuAuEventFlag = 1;
	local.y = -99.;
	local.z = -99.;
	tdiffCut = 0.8;//ns
	mPadWidth = 3.45;
	mTraycoverphi = PI*6./180.;// one Tray cover phi
	mModuleWidth = 9.44;// Module width (cm)
	vzOffset = 2446+11.4;//12.29;//12.4-0.11; //11.86+0.54; //cm
	c_light = 29.9792458;// light speed :cm/ns 
	mTofRmin = 210.968;//214.568-3.6;// TOF inner radius (cm)
 
	for(int i=0;i<nTray;i++){
		for(int j=0;j<nModule;j++){
			nCellhits[i][j] = 0;
			for(int k=0;k<maxModuleHits;k++) Cellhit[i][j][k] = 0;
		}
	}
	
	std::ifstream inGeom(tofAlignGeom);
	if(!inGeom){
		LOG(ERR," tofAlignGeom.dat doesn't exist! ");
	}

	while(inGeom){
		char pname[100];
		inGeom>>pname;
		if(strcmp(pname,"mTrayX0")==0){
			for(int i=0;i<120;i++) inGeom >> mTrayX0[i];
		}
		if(strcmp(pname,"mTrayY0")==0){
			for(int i=0;i<120;i++) inGeom >> mTrayY0[i];
		}
		if(strcmp(pname,"mTrayZ0")==0){
			for(int i=0;i<120;i++){
				inGeom >> mTrayZ0[i];
				if(i>59) mTrayZ0[i] = -mTrayZ0[i];
			}
		}
		if(strcmp(pname,"mModuleAngle")==0){
			for(int i=0;i<32;i++){
				inGeom >> mModuleAngle[i];
				mModuleAngle[i] *= PI/180.;
			}
		}
		if(strcmp(pname,"mModuleLocalZ")==0){
			for(int i=0;i<32;i++) inGeom >> mModuleLocalZ[i];
		}
		if(strcmp(pname,"mModuleLocalX")==0){
			for(int i=0;i<32;i++) inGeom >> mModuleLocalX[i];
		}
	}
	inGeom.close();
	// east ring, start from 108 deg (id=61) , clock-wise from east facing west
	// west ring, start from 72 deg (id=1) , clock-wise from west facing east
	for(int i=0;i<60;i++){
		int wdeg = 72-i*6;
		int edeg = 108+i*6;
		if(edeg >= 360) edeg -=360;
		if(edeg < 0)   edeg +=360;
		if(wdeg >= 360) wdeg -=360;
		if(wdeg < 0)   wdeg +=360;

		mTrayPhi[i] = PI*wdeg/180.;
		mTrayPhi[60+i] = PI*edeg/180.;
		float wsinphi = ((mTofRmin+3.6)*sin(mTrayPhi[i])+mTrayY0[i])/(mTofRmin+3.6);
		float wcosphi = ((mTofRmin+3.6)*cos(mTrayPhi[i])+mTrayX0[i])/(mTofRmin+3.6);
		mTrayPhi[i] = atan2(wsinphi,wcosphi)>0?atan2(wsinphi,wcosphi):atan2(wsinphi,wcosphi)+2*PI;
		float esinphi = ((mTofRmin+3.6)*sin(mTrayPhi[i+60])+mTrayY0[i+60])/(mTofRmin+3.6);
		float ecosphi = ((mTofRmin+3.6)*cos(mTrayPhi[i+60])+mTrayX0[i+60])/(mTofRmin+3.6);
		mTrayPhi[i+60] = atan2(esinphi,ecosphi)>0?atan2(esinphi,ecosphi):atan2(esinphi,ecosphi)+2*PI;
	}

	return 0;
}

int gl3TOF::matchGlobalTracks(gl3Node *node)
{
	CellVec  cellv;
	gl3Track *track = node->globalTrack;
	imat = -1;//matched hit index
	//gFlag = true;
	imat = projTrayModuleCell(track,cellv,local);
	if(imat<0) return 0;
	node->globaltofCell = &tofhits->cell[imat];
	return 0;

}

int gl3TOF::matchPrimaryTracks(gl3Node *node, l3xyzCoordinate vertex)
{
	CellVec  cellv;
	gl3Track *track = node->primaryTrack;
	if(!track) return 0;
	float xVertex,yVertex,zVertex;
	xVertex = vertex.Getx();
	yVertex = vertex.Gety();
	zVertex = vertex.Getz();
	track->updateToClosestApproach(xVertex,yVertex);
	//if(track->pt<0.2) return -1;
	//if(track->nHits<20) return -1;
	//if(track->r0>1.) return -1;
	//if(vzVpd<-999.) return -1;
	Ftf3DHit closest = track->closestApproach(xVertex,yVertex);
	float vztrk = closest.z;
	//if(fabs(vzVpd-zVertex)>3.) return -1;	
	//if(fabs(vztrk-zVertex)>1.) return -1;
	//if(!checkvz(track)) return -1;

	double pathlen;
	imat = -1;//matched hit index
	//gFlag = false;
	imat = projTrayModuleCell(track,cellv,local);
	if(imat<0) return 0;

	// calculate pathlength
	double Rxyz[3],Nxyz[3],HitPosition[3];
	GetModuleRN(cellv, Rxyz, Nxyz);
	pathlen = track->pathLength(Rxyz[0],Rxyz[1],Rxyz[2],Nxyz[0],Nxyz[1],Nxyz[2],HitPosition);
    
	Global2Local(cellv,HitPosition,local);
	double localz = local.z;
	double localy = local.y;
	double corrZ = zCali(cellv, localz);
	
	double tof = tofhits->cell[imat].tof-corrZ-T0;
	double beta = pathlen/tof/c_light;
	node->primarytofCell = &tofhits->cell[imat];

        ///< LOG(INFO,"corrZ: %f, T0: %f, PathLength %f, tof: %f, beta: %f",corrZ,T0,pathlen,tof,beta);

	node->tof = tof;
	node->beta = beta;
	node->tofLocalY = local.y;
	node->tofLocalZ = local.z;
	node->tofProjectChannel = projchannel;
	node->pathlength = pathlen;
	return 1;
} 

int gl3TOF::checkvz(gl3Track* track){

	//const float vzOffset = 2446+11.4;//12.29;//12.4-0.11; //11.86+0.54; //cm
	int nHits = pvpdhits->nHits;
	float vztrk;
	Ftf3DHit closest = track->closestApproach(0.,0.);
	vztrk = closest.z;
	if(nHits<1) return -1;
	if(fabs(vztrk-vzVpd)>6.) return 0;
	else return 1;
}

void gl3TOF::GetModuleRN(CellVec &cell, double *Rxyz, double *Nxyz){

	Rxyz[0] = mTrayX0[cell.tray]+(mTofRmin+mModuleLocalX[cell.module])*cos(mTrayPhi[cell.tray]);
	Rxyz[1] = mTrayY0[cell.tray]+(mTofRmin+mModuleLocalX[cell.module])*sin(mTrayPhi[cell.tray]);
	if(cell.tray<60) Rxyz[2] = mTrayZ0[cell.tray]+mModuleLocalZ[cell.module];
	else Rxyz[2] = mTrayZ0[cell.tray]-mModuleLocalZ[cell.module];
	Nxyz[0] = cos(mModuleAngle[cell.module])*cos(mTrayPhi[cell.tray]);
	Nxyz[1] = cos(mModuleAngle[cell.module])*sin(mTrayPhi[cell.tray]);
	if(cell.tray<60) Nxyz[2] = sin(mModuleAngle[cell.module]);
	else Nxyz[2] = -sin(mModuleAngle[cell.module]);
	//R(x,y,z) position of plane, N(x,y,z) normal

	return;	
}

void gl3TOF::Global2Local(CellVec &cell, double *HitPosition, PointVec &local){
	float cellz;
	if(cell.tray<60) cellz = mTrayZ0[cell.tray]+mModuleLocalZ[cell.module];
	else cellz = mTrayZ0[cell.tray]-mModuleLocalZ[cell.module];
	float hitr = sqrt(HitPosition[0]*HitPosition[0]+HitPosition[1]*HitPosition[1]);
	float hitphi = atan2(HitPosition[1]/hitr,HitPosition[0]/hitr);
	if(hitphi<0) hitphi+=2*PI;
	if(cell.tray<60){
	   local.y = hitr*sin(hitphi-mTrayPhi[cell.tray])-(2.5-cell.cell)*mPadWidth;
	   local.z = (cellz-HitPosition[2])/cos(mModuleAngle[cell.module]);
	}else{
	  local.y = hitr*sin(hitphi-mTrayPhi[cell.tray])-(cell.cell-2.5)*mPadWidth;
	  local.z = (HitPosition[2]-cellz)/cos(mModuleAngle[cell.module]);
	}
}


//---------------------------------------------------------
int gl3TOF::projTrayModuleCell(gl3Track* track, CellVec &cellv, PointVec &local)
{
	int matHit = -1;//matched index of hit list
	int trayId = -1,moduleId = -1,cellId = -1;
	double extrapolatePhi, extrapolateZ, temp1, temp2, temp3;
	double Rcen = mTofRmin+3.6;

	projchannel = -1;	
	int ret =track->extraRCyl(Rcen, extrapolatePhi, extrapolateZ, temp1, temp2, temp3);

        if (ret) return -1;
        
	while(extrapolatePhi<0) extrapolatePhi += 2*PI;
	while(extrapolatePhi>=2*PI) extrapolatePhi -= 2*PI;
	if(ret==0&&(extrapolatePhi!=0||extrapolateZ!=0)){
		for(int i=0;i<tofhits->nHits;i++){
			int tray = tofhits->cell[i].trayid;
			int module = tofhits->cell[i].channel/6;
			int cell =  tofhits->cell[i].channel%6;
			float phi = mTrayPhi[tray];
			float z = 0;

			float cellphi;
			if(tray>=60){
				z = mTrayZ0[tray] - mModuleLocalZ[module];
				cellphi = mTrayPhi[tray]+(cell-2.5)*mPadWidth/(mTofRmin+3.6);
			}else{
				z = mTrayZ0[tray] + mModuleLocalZ[module];
				cellphi = mTrayPhi[tray]+(2.5-cell)*mPadWidth/(mTofRmin+3.6);
			}
		}
		
		if(extrapolateZ>=0){
			float tempphi = mTrayPhi[0]+0.5*mTraycoverphi-extrapolatePhi;
			while(tempphi<0) tempphi+=2*PI;
			while(tempphi>=2*PI) tempphi-=2*PI;
			trayId = int(floor(tempphi/mTraycoverphi));
		}else{
			float tempphi = extrapolatePhi-mTrayPhi[60]+0.5*mTraycoverphi;
			while(tempphi<0) tempphi+=2*PI;
			while(tempphi>=2*PI) tempphi-=2*PI;
			trayId = int(60.+floor(tempphi/mTraycoverphi));
		}
	}

	if(trayId<0) return -1;
	//------------- project module and cell -------------------------
	double moduleZ;


	float dLrmin=99.;
	for(int j=0;j<32;j++){
		moduleZ = mModuleLocalZ[j];
		if(trayId>=60) {
			moduleZ *= -1.;
		}
		moduleZ += mTrayZ0[trayId];

		double moduleZmin,moduleZmax;
		moduleZmin = moduleZ-0.5*mModuleWidth/cos(mModuleAngle[j]);
		moduleZmax = moduleZ+0.5*mModuleWidth/cos(mModuleAngle[j]);
		if(extrapolateZ<moduleZmax	&& extrapolateZ>=moduleZmin){ // mModuleWidth towards Local Y direction

			float moduleRcen = mModuleLocalX[j]+mTofRmin;
			float zpm;//module center project to Rmin
			if(trayId<60) zpm = (moduleZ-(moduleRcen-Rcen)*tan(mModuleAngle[j]));
			else zpm = (moduleZ+(moduleRcen-Rcen)*tan(mModuleAngle[j]));

			float Ly,Lz,dLr,dLy;// Lz is dLz
			Ly = moduleRcen*(extrapolatePhi-mTrayPhi[trayId]);
			if(trayId<60) Lz = (zpm-extrapolateZ)*cos(mModuleAngle[j]);
			else Lz = (extrapolateZ-zpm)*cos(mModuleAngle[j]);	
			int icell = -1;
			for(int i=0;i<6;i++){
				float ycenter;
				if(trayId>59) ycenter = (i-2.5)*mPadWidth;
				else ycenter = (2.5-i)*mPadWidth;
				if(Ly>=ycenter-0.5*mPadWidth && Ly<ycenter+0.5*mPadWidth){
					icell = i;
					//------------------------------------------
					for(int ij=-1;ij<2;ij++){
						if(j+ij<0 || j+ij>31) continue;
						for(int k=0;k<nCellhits[trayId][j+ij];k++){
							if(Cellhit[trayId][j+ij][k]<0) continue;
							int ihit = Cellhit[trayId][j+ij][k];
							int ihitcell = tofhits->cell[ihit].channel%6;
							if(((icell==0&&(ihitcell==icell || ihitcell==icell+1))
										||(icell==5&&(ihitcell==icell || ihitcell==icell-1))
										||((icell>=1&&icell<=5)&&
											((icell==ihitcell)||
											 (icell==ihitcell-1)||
											 (icell==ihitcell+1))))){
								float yhitcenter;
								if(trayId>59) yhitcenter = (ihitcell-2.5)*mPadWidth;
								else yhitcenter = (2.5-ihitcell)*mPadWidth;
								dLy = Ly-yhitcenter;
								dLr = fabs(dLy);//sqrt(dLy*dLy+Lz*Lz);
								if(dLr<dLrmin){
									projchannel = j*6+icell;
									dLrmin = dLr;
									moduleId = j+ij;
									cellId = ihitcell;
									local.y = dLy;
									local.z = Lz;
									matHit = ihit;
								}
							}
						}
					}
					//---------------------------------------------
				}

			}
		}
	}
	if(matHit<0){
		projchannel = -1;
		return -1;
	}
	cellv.tray = trayId;
	cellv.module = moduleId;
	cellv.cell = cellId;
	return matHit;
}


int gl3TOF::readzCali(char *zCali_4DB){
	int itray, iboard, imaxN;
	std::ifstream indataZ(zCali_4DB);
	if(!indataZ){
		LOG(ERR," zCali_4DB.dat doesn't exist! ");
	}
	for(int i=0;i<nTray;i++){
		for(int j=0;j<nBoard;j++){
			indataZ>>itray>>iboard>>imaxN;
			if(imaxN<0.) continue;
			for(int k=0;k<=imaxN;k++){
				indataZ>>tofZEdge[i][j][k];
			}
			for(int k=0;k<=imaxN;k++){
				indataZ>>tofZCorr[i][j][k];
			}
		}
	}
	indataZ.close();
	return 0;
}

double gl3TOF::zCali(CellVec& cell, double z){ //z is local z
	double corrZ;
	//local Z
	int ibin = -1;
	int tray = cell.tray;
	int board = cell.module/4;
	for(int k=0;k<maxNZBin;k++){
		if(z>=tofZEdge[tray][board][k]&&z<tofZEdge[tray][board][k+1]) {
			ibin = k;
			break;
		}
	}

	//corrZ = (tofZCorr[tray][board][ibin]+tofZCorr[tray][board][ibin+1])/2.;

	if(ibin<0) return -9999.;
	corrZ = linearInter(tofZEdge[tray][board][ibin],tofZEdge[tray][board][ibin+1],
			tofZCorr[tray][board][ibin],tofZCorr[tray][board][ibin+1], z);

	return corrZ;
}

float gl3TOF::linearInter(float x1, float x2, float y1, float y2, float x){

    double corrZ = ((x-x1)*y2+(x2-x)*y1)/(x2-x1);
	return corrZ;
}

int gl3TOF::CalcvzVpd()
{
	int nHits = pvpdhits->nHits;
	if(nHits<1 || nHits > MAX_TOF_HITS) return -1;
	int Iwest = 0, Ieast = 0;
	float sumEast=0., sumWest=0.; 
	//const float vzOffset = 2446+11.4;//12.29;//12.4-0.11; //11.86+0.54; //cm
	//---------------------------------
	int nwest[19]={0},neast[19]={0};
	float wtot[19]={0.},etot[19]={0.};
	float wtdc[19]={0.},etdc[19]={0.};
	float wtcor[19]={0.},etcor[19]={0.};//tdc-cor;
	for(int i=0;i<nHits;i++){
            //if(pvpdhits->cell[i].tof<0.) continue;
		int j = pvpdhits->cell[i].channel;
		if(pvpdhits->cell[i].trayid == 120){
			nwest[j]++;
			if(nwest[j]==1){
				wtdc[j] = pvpdhits->cell[i].tdc; 
				wtot[j] = pvpdhits->cell[i].tot;
				wtcor[j] = pvpdhits->cell[i].tof;
			}else if(nwest[j]>1 && pvpdhits->cell[i].tdc<wtdc[j]){
				wtdc[j] = pvpdhits->cell[i].tdc;
				wtot[j] = pvpdhits->cell[i].tot;
				wtcor[j] = pvpdhits->cell[i].tof;
			} 
		}
                if(pvpdhits->cell[i].trayid == 121){
			neast[j]++; 
			if(neast[j]==1){
				etdc[j] = pvpdhits->cell[i].tdc; 
				etot[j] = pvpdhits->cell[i].tot;
				etcor[j] = pvpdhits->cell[i].tof;
			}else if(neast[j]>1 && pvpdhits->cell[i].tdc<etdc[j]){
				etdc[j] = pvpdhits->cell[i].tdc;
				etot[j] = pvpdhits->cell[i].tot;
				etcor[j] = pvpdhits->cell[i].tof;
			} 
		}
	}

	for(int i=0;i<nPVPDChannel/2;i++){
            // LOG(INFO,"pvpd channel:%d,tdc[%d]:%f,tot[%d]:%f,cor[%d]:%f",i,i,wtdc[i],i,wtot[i],i,wtdc[i]-wtcor[i]);
		if(nwest[i]>0){ 
			sumWest += wtcor[i];
			Iwest++;
		}
		if(neast[i]>0){ 
			sumEast += etcor[i];
			Ieast++;
		}
	}

	for(int i=0;i<nPVPDChannel/2;i++){
            // LOG(INFO,"pvpd channel:%d,tdc[%d]:%f,tot[%d]:%f,cor[%d]:%f",i+19,i+19,etdc[i],i+19,etot[i],i+19,etdc[i]-etcor[i]);
            double vpdtime;
		if(wtcor[i]>0.&&Iwest>1){
			vpdtime = (wtcor[i]*Iwest-sumWest)/(Iwest-1);
			if(fabs(vpdtime)>tdiffCut) {
				sumWest -= wtcor[i];
				Iwest--;
			}
		}
                if(etcor[i]>0.&&Ieast>1){
			vpdtime = (etcor[i]*Ieast-sumEast)/(Ieast-1);
			if(fabs(vpdtime)>tdiffCut) {
				sumEast -= etcor[i];
				Ieast--;
			}
		}
	}

	//-----------------------------
	//sumEast += 2*vzOffset*Ieast/c_light;
	if(Ieast>0 || Iwest>0){
		float tdiff;
		if(Ieast>0&&Iwest>0){
			tdiff = sumEast/Ieast -sumWest/Iwest;
			vzVpd = tdiff/2*c_light;
		}else{
			vzVpd = -1e6;
		}
	}else{
		vzVpd = -1.e6;
	}
	return 0;

}

int gl3TOF::Tstart(float& vzTpc)
{

	int nHits = pvpdhits->nHits;
	if(nHits<1) return -1;
	int Iwest = 0, Ieast = 0;
	float sumEast=0., sumWest=0.; 
	//const float vzOffset = 2446+11.4;//12.29;//12.4-0.11; //11.86+0.54; //cm
	//---------------------------------
	int nwest[19]={0},neast[19]={0};
	float wtot[19]={0.},etot[19]={0.};
	float wtdc[19]={0.},etdc[19]={0.};
	float wtcor[19]={0.},etcor[19]={0.};//tdc-cor;
	for(int i=0;i<nHits;i++){
		//if(pvpdhits->cell[i].tof<0.) continue;
		int j = pvpdhits->cell[i].channel;
		if(pvpdhits->cell[i].trayid == 120){
			nwest[j]++;
			if(nwest[j]==1){
				wtdc[j] = pvpdhits->cell[i].tdc; 
				wtot[j] = pvpdhits->cell[i].tot;
				wtcor[j] = pvpdhits->cell[i].tof;
			}else if(nwest[j]>1 && pvpdhits->cell[i].tdc<wtdc[j]){
				wtdc[j] = pvpdhits->cell[i].tdc;
				wtot[j] = pvpdhits->cell[i].tot;
				wtcor[j] = pvpdhits->cell[i].tof;
			} 
		}
		if(pvpdhits->cell[i].trayid == 121){
			neast[j]++; 
			if(neast[j]==1){
				etdc[j] = pvpdhits->cell[i].tdc; 
				etot[j] = pvpdhits->cell[i].tot;
				etcor[j] = pvpdhits->cell[i].tof;
			}else if(neast[j]>1 && pvpdhits->cell[i].tdc<etdc[j]){
				etdc[j] = pvpdhits->cell[i].tdc;
				etot[j] = pvpdhits->cell[i].tot;
				etcor[j] = pvpdhits->cell[i].tof;
			} 
		}
	}

	for(int i=0;i<nPVPDChannel/2;i++){
		//LOG(INFO,"pvpd channel:%d,tdc[%d]:%f,tot[%d]:%f,cor[%d]:%f",i,i,wtdc[i],i,wtot[i],i,wtdc[i]-wtcor[i]);
		if(nwest[i]>0){ 
			sumWest += wtcor[i];
			Iwest++;
		}
		if(neast[i]>0){ 
			sumEast += etcor[i];
			Ieast++;
		}
	}

	for(int i=0;i<nPVPDChannel/2;i++){
		//LOG(INFO,"pvpd channel:%d,tdc[%d]:%f,tot[%d]:%f,cor[%d]:%f",i+19,i+19,etdc[i],i+19,etot[i],i+19,etdc[i]-etcor[i]);
		double vpdtime;
		if(wtcor[i]>0.&&Iwest>1){
			vpdtime = (wtcor[i]*Iwest-sumWest)/(Iwest-1);
			if(fabs(vpdtime)>tdiffCut) {
				sumWest -= wtcor[i];
				Iwest--;
			}
		}
		if(etcor[i]>0.&&Ieast>1){
			vpdtime = (etcor[i]*Ieast-sumEast)/(Ieast-1);
			if(fabs(vpdtime)>tdiffCut) {
				sumEast -= etcor[i];
				Ieast--;
			}
		}
	}

	//-----------------------------
	//sumEast += 2*vzOffset*Ieast/c_light;
		if(Iwest+Ieast>0) T0 = (sumEast+sumWest+(Iwest-Ieast)*vzTpc/c_light)/(Ieast+Iwest);
	    else T0 = -1.e6;

	return 0;

}


int gl3TOF::Tstart_NoVpd(int nglobal, gl3Node *nodes, l3xyzCoordinate vertex, gl3Bischel* bischel, double nsigma1)
{
	///< calculate Tstart without vpd

	int nCan = 0;
	double tSum = 0;
	double t0[MAX_TOF_HITS];
	memset(t0, 0., sizeof(t0));
		
	for(int i=0; i<nglobal; i++)
	{
		CellVec  cellv;
		gl3Node *node = &nodes[i];
		//if(node->globalTrack->nHits < 10) continue;
		if(node->globalTrack->pt < 0.2) continue;
		gl3Track *track = node->primaryTrack;
		if(!track) continue;
		float xVertex,yVertex,zVertex;
		xVertex = vertex.Getx();
		yVertex = vertex.Gety();
		zVertex = vertex.Getz();
		track->updateToClosestApproach(xVertex,yVertex);
		//if(track->pt<0.2) return -1;
		//if(track->nHits<20) return -1;
		//if(track->r0>1.) return -1;
		//if(vzVpd<-999.) return -1;
		Ftf3DHit closest = track->closestApproach(xVertex,yVertex);
		float vztrk = closest.z;
		//if(fabs(vzVpd-zVertex)>3.) return -1; 
		//if(fabs(vztrk-zVertex)>1.) return -1;
		//if(!checkvz(track)) return -1;

		double pathlen;
		imat = -1;//matched hit index
		//gFlag = false;
		imat = projTrayModuleCell(track,cellv,local);
		if(imat<0) continue;

		// calculate pathlength
		double Rxyz[3],Nxyz[3],HitPosition[3];
		GetModuleRN(cellv, Rxyz, Nxyz);
		pathlen = track->pathLength(Rxyz[0],Rxyz[1],Rxyz[2],Nxyz[0],Nxyz[1],Nxyz[2],HitPosition);

		Global2Local(cellv,HitPosition,local);
		double localz = local.z;
		double localy = local.y;
		double corrZ = zCali(cellv, localz);

		int trayId = tofhits->cell[imat].trayid;

		if(trayId>0&&trayId<=nTray) {

			double px = track->pt*cos(track->psi);
			double py = track->pt*sin(track->psi);
			double pz = track->tanl*track->pt;
			double p = sqrt(px*px + py*py +pz*pz) ;

			double nSigPi = track->nSigmaDedx(bischel, "Pion", nsigma1);
			if(fabs(nSigPi)>2.) continue;

			double corrZ = zCali(cellv, localz);
			if(corrZ<-9000.) continue;
			double tofcorr = tofhits->cell[imat].tof - corrZ;

			double tofPi = pathlen*sqrt(M_PION*M_PION+p*p)/(p*c_light);

			tSum += tofcorr - tofPi;
			t0[nCan] = tofcorr - tofPi;
			nCan++;

		}
	}

	///< LOG(INFO,"nCan : %i",nCan);

	if(nCan<=0) {
		T0 = -9999.;
		return 0;
	}

	int nTzero = nCan;
	if(nCan>1) { // remove hits too far from others
		for(int i=0;i<nCan;i++) {
			double tdiff = t0[i] - (tSum-t0[i])/(nTzero-1);
			if(fabs(tdiff)>5.0) {
				tSum -= t0[i];
				nTzero--;
			}
		}
	}

	T0 = nTzero>0 ? tSum / nTzero : -9999.;

	return 1;

}


int gl3TOF::readFromTofMachine(char *trgSend){

	TofEvent *temp = (TofEvent *) trgSend;
	for(int i=0;i<temp->nHits;i++){
		if(temp->cell[i].tof>0 && temp->cell[i].trayid>=0 && temp->cell[i].trayid<120){
			tofhits->cell[itof].trayid = temp->cell[i].trayid;
			tofhits->cell[itof].channel = temp->cell[i].channel;
			tofhits->cell[itof].triggertime = temp->cell[i].triggertime;
			tofhits->cell[itof].tdc = temp->cell[i].tdc;
			tofhits->cell[itof].tot = temp->cell[i].tot;
			tofhits->cell[itof].tof = temp->cell[i].tof;
			itof++;
                        if (itof >= MAX_TOF_HITS) break;
		}else if(temp->cell[i].tof>0 && (temp->cell[i].trayid==120 || temp->cell[i].trayid==121)){
		pvpdhits->cell[ipvpd].trayid = temp->cell[i].trayid;
		pvpdhits->cell[ipvpd].channel = temp->cell[i].channel;
		pvpdhits->cell[ipvpd].triggertime = temp->cell[i].triggertime;
		pvpdhits->cell[ipvpd].tdc = temp->cell[i].tdc;
		pvpdhits->cell[ipvpd].tot = temp->cell[i].tot;
		pvpdhits->cell[ipvpd].tof = temp->cell[i].tof;
		ipvpd++;
                if (ipvpd >= MAX_TOF_HITS) break;
		}
	}
	tofhits->nHits = itof;
	pvpdhits->nHits = ipvpd;
	//LOG(INFO," pvpd hits: %d last tdc: %f, last tot-cor: %f", pvpdhits->nHits, pvpdhits->cell[pvpdhits->nHits-1].tdc,pvpdhits->cell[pvpdhits->nHits-1].tof);
        LOG(DBG, "nTofHits = %d, pvpd hits = %d", itof, ipvpd);
	for(int i=0;i<nTray;i++){
		for(int j=0;j<nModule;j++){
			nCellhits[i][j] = 0;
			for(int k=0;k<maxModuleHits;k++) Cellhit[i][j][k] = -1;
		}
	}
	int ic[120][32]={{0}};
	evt_counter++;
	for(int i=0;i<tofhits->nHits;i++){
		int tray = tofhits->cell[i].trayid;//all id -1 
		int module = tofhits->cell[i].channel/6;
		if(ic[tray][module]>=maxModuleHits){ 
			//LOG(WARN,"module %d, hits more than :%d!",module,maxModuleHits);
			continue;
		}
		//int cell = tofhits->cell[i].channel%6;
		nCellhits[tray][module] ++;
		Cellhit[tray][module][ic[tray][module]] = i;
		ic[tray][module] ++;
	}
	return 0;
}

void gl3TOF::reset(){
        tofhits->nHits = 0;
        pvpdhits->nHits = 0;
	itof = 0;
	ipvpd = 0;
	T0 = -1.e6;
	vzVpd = -1.e6;
}

