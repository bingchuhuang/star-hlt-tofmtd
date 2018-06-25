#include "gl3EMC.h"

#include "gl3Event.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <math.h>

#ifndef OLD_DAQ_READER
#include <DAQ_READER/daq_dta.h>
#endif /* OLD_DAQ_READER */

#include "daqFormats.h"
#include "l3BankUtils.h"
#include "l3Swap.h"
#include <rtsLog.h>

#ifndef OLD_DAQ_READER
#include <DAQ_EMC/daq_emc.h>
#include <DAQ_BTOW/daq_btow.h>
#endif /* OLD_DAQ_READER */

#include "../L3_SUPPORT/l3_support.h"

int gl3EmcTower::matchTrack(gl3Track* track)
{
  double temp1, temp2;
  return matchTrack(track, temp1, temp2);
}

int gl3EmcTower::matchTrack(gl3Track* track, double& phiDiff, double& zEdge, double phiDiffCut, double zEdgeCut)
{
  double extrapolatePhi, extrapolateZ, temp1, temp2, temp3;
  if(track->extraRCyl(gl3EMC::emcR, extrapolatePhi, extrapolateZ, temp1, temp2, temp3))
    {
      phiDiff = -9999.;
      zEdge = -9999.;      
      return 0;
    }
  l3EmcTowerInfo* towerInfo = getTowerInfo();
  zEdge = max(towerInfo->getZmin()-extrapolateZ, extrapolateZ-towerInfo->getZmax());
  zEdge = max(0., zEdge);
  if(zEdge > zEdgeCut)
    {
      phiDiff = -9999.;
      return 0;
    }  
  phiDiff = fabs(towerInfo->getPhi() - extrapolatePhi);
  while(phiDiff > pi) phiDiff -= 2*pi;
  phiDiff = fabs(phiDiff);
  if(phiDiff > phiDiffCut) return 0;
  return 1;
}

gl3EMC::gl3EMC(l3EmcCalibration *BarrelCalib, l3EmcCalibration *EndcapCalib)
{
    barrelCalib = BarrelCalib;
    endcapCalib = EndcapCalib;

    if (barrelCalib)
	nBarrelTowers = barrelCalib->getNTowers();
    else 
	nBarrelTowers = 0;

    if (endcapCalib)
	nEndcapTowers = endcapCalib->getNTowers();
    else 
	nEndcapTowers = 0;

    nTotalTowers = nBarrelTowers + nEndcapTowers;

    tower = new gl3EmcTower[nTotalTowers];
    barrelTower = tower;
    endcapTower = tower + nBarrelTowers;

    for (int i=0; i<nBarrelTowers; i++) {
	barrelTower[i].setTowerInfo(barrelCalib->getTowerInfo(i));
    }

    for (int i=0; i<nEndcapTowers; i++) {
	endcapTower[i].setTowerInfo(endcapCalib->getTowerInfo(i));
    }

    reset();
}

gl3EMC::~gl3EMC() {
    delete[] tower;
}


#ifdef OLD_DAQ_READER
int gl3EMC::readFromEvpReader(evpReader *evp, char *mem)
#else /* OLD_DAQ_READER */
int gl3EMC::readFromEvpReader(daqReader *rdr, char *mem)
#endif /* OLD_DAQ_READER */
{
  int i,j;
#ifdef OLD_DAQ_READER
  int ret = emcReader(mem);
#endif /* ! OLD_DAQ_READER */

#ifdef OLD_DAQ_READER
  if(ret <= 0) {
#else /* OLD_DAQ_READER */
  daq_dta *dd  = rdr->det("emc_pseudo")->get("legacy");
  
  if(!dd) {
#endif /* OLD_DAQ_READER */
    LOG(NOTE, "No EMC data present...",0,0,0,0,0);
    return 0;
  }

#ifndef OLD_DAQ_READER
  dd->iterate();
  emc_t *pEMC = (emc_t *)dd->Void;

#endif /* OLD_DAQ_READER */
  // First zero out whatever we have...
  for(i=0;i<nBarrelTowers;i++) {
    barrelTower[i].setADC(0);
    barrelTower[i].setNTracks(0);
  }

  for(i=0;i<nEndcapTowers;i++) {
    endcapTower[i].setADC(0);
    endcapTower[i].setNTracks(0);
  }

  // Do some checks...
  if(barrelCalib) {
    if(nBarrelTowers < BTOW_MAXFEE * BTOW_DATSIZE) {
      LOG(WARN, "nBarrelTowers = %d, smaller than %d",nBarrelTowers,BTOW_MAXFEE * BTOW_DATSIZE,0,0,0);
    }
  }

  if(endcapCalib) {
    if(nEndcapTowers < ETOW_MAXFEE * ETOW_DATSIZE) {
      LOG(WARN, "nEndcapTowers = %d, smaller than %d",nEndcapTowers,ETOW_MAXFEE * ETOW_DATSIZE,0,0,0);
    }
  }

  // Read the btow, if its there...
  if(barrelCalib) {

#ifdef OLD_DAQ_READER
    if(emc.btow_in) { 
#else /* OLD_DAQ_READER */
      if(pEMC->btow_in) { 
#endif /* OLD_DAQ_READER */
      LOG(DBG, "Reading BTOW data",0,0,0,0,0);
      for(i=0;i<BTOW_MAXFEE*BTOW_DATSIZE;i++) {
	
	int daqid = i;                          // linearized
	int id = barrelCalib->daqToId(daqid);   // emc uses different linearization
	
#ifdef OLD_DAQ_READER
	if(emc.btow[i] != 0)
	  LOG(DBG, "i=%d id=%d adc=%d",daqid,id,emc.btow[i]);
#else /* OLD_DAQ_READER */
	if(pEMC->btow[i] != 0)
	  LOG(DBG, "i=%d id=%d adc=%d",daqid,id,pEMC->btow[i]);
#endif /* OLD_DAQ_READER */
	
	if(id >= nBarrelTowers) continue;
	
#ifdef OLD_DAQ_READER
	barrelTower[id].setADC(emc.btow[i]);
#else /* OLD_DAQ_READER */
	barrelTower[id].setADC(pEMC->btow[i]);
#endif /* OLD_DAQ_READER */
	barrelTower[id].setNTracks(0);
      }
    }
  }

  // Read the etow, if its there...
  if(endcapCalib) {
#ifdef OLD_DAQ_READER
    if(emc.etow_in) {
#else /* OLD_DAQ_READER */
    if(pEMC->etow_in) {
#endif /* OLD_DAQ_READER */
      LOG(DBG, "Reading etow data",0,0,0,0,0);
      for(i=0;i<ETOW_MAXFEE;i++) {
	for(j=0;j<ETOW_DATSIZE;j++) {
	  
	  int daqid = i + j*ETOW_MAXFEE;          // linearized (note linearization seems backward to me!)
	  int id = endcapCalib->daqToId(daqid);   // emc uses different linearization

	  if(id >= nEndcapTowers) continue;

#ifdef OLD_DAQ_READER
	  if(emc.etow[i][j] != 0)
	    LOG(DBG, "etow: i=%d j=%d daqid=%d id=%d adc=%d",i,j,daqid,id, emc.etow[i][j]);
#else /* OLD_DAQ_READER */
	  if(pEMC->etow[i][j] != 0)
	    LOG(DBG, "etow: i=%d j=%d daqid=%d id=%d adc=%d",i,j,daqid,id, pEMC->etow[i][j]);
#endif /* OLD_DAQ_READER */

#ifdef OLD_DAQ_READER
	  endcapTower[id].setADC(emc.etow[i][j]);
#else /* OLD_DAQ_READER */
	  endcapTower[id].setADC(pEMC->etow[i][j]);
#endif /* OLD_DAQ_READER */
	  endcapTower[id].setNTracks(0);
	}
      }
    }
    
  }
  return 0;
}


int gl3EMC::readRawData(L3_P *l3p)
{

 int ret=0;

    if (l3p->emc[0].len > 10000) {
      LOG(ERR,"Illegal length of BTOW contribution",0,0,0,0,0);
	return 0;
    }

    if (l3p->emc[3].len > 10000) {
      LOG(ERR," ---> ERROR <--- Illegal length of ETOW contribution\n",0,0,0,0,0);
	return 0;
    }

    // Read BTOW
    if (l3p->emc[0].len) {
	if (readEMCSECP((EMCSECP*)offlen2ptr(l3p, l3p->emc[0]))) {
	    // something went wrong reading the data -> zero all towers
	    
	    for (int i=0; i < nBarrelTowers; i++) {
		barrelTower[i].setADC(0);
		barrelTower[i].setNTracks(0);
	    }
	}
    }

    if (l3p->emc[3].len) {
	ret = readEMCSECP((EMCSECP*)offlen2ptr(l3p, l3p->emc[3]));
    }


    if (ret) {
	// something went wrong reading the data, maybe there is some 
	// garbage in the data, so zero it 

      LOG(ERR,"Error reading endcap",0,0,0,0,0);

	for (int i=0; i < nEndcapTowers; i++) {
	    endcapTower[i].setADC(0);
	    endcapTower[i].setNTracks(0);
	}
    }

    Ebarrel = 0.0;
    for (int i=0; i < nBarrelTowers; i++) {
	Ebarrel += barrelTower[i].getEnergy();
    }
    
    Eendcap = 0.0;
    for (int i=0; i < nEndcapTowers; i++) {
	Eendcap += endcapTower[i].getEnergy();
    }
    
    Etotal = Ebarrel + Eendcap;

    return 0;
}


int gl3EMC::readEMCSECP(EMCSECP* secp)
{

    enum {barrel, endcap} secpType;
    
    l3EmcCalibration *calib = NULL;
    gl3EmcTower *twr = NULL;
    int nCrates;

    if (!secp) {
	return -2;
    }

    if(strncmp(secp->bh.bank_type, CHAR_EMCSECP, 8) == 0) {
	secpType = barrel;
	calib = barrelCalib;
	twr = barrelTower;
	nCrates = 30;

    } else if (strncmp(secp->bh.bank_type, CHAR_EECSECP, 8) == 0) {
	secpType = endcap;
	calib = endcapCalib;
	twr = endcapTower;
	nCrates = 6;

    } else {
      LOG(ERR,"Unknown bank type '%s'", secp->bh.bank_type,0,0,0,0);
	return -3;
    }

    if (calib == NULL) {
	return -4;
    }


    // This really seems to be a data contribution, so let's read it

    int nFibers = (swap32(secp->bh.length)-10)/2;
	
    for (int i=0; i<nFibers; i++) {
	EMCRBP* rbp = (EMCRBP*)offlen2ptr(secp, secp->fiber[i]);
	if (!rbp) continue;
	
	int nBANKS = (swap32(rbp->bh.length)-10)/2;
	
	if (nBANKS < 1) {
	  LOG(ERR, "EMC data coming in without BTOW bank?!?!?\n",0,0,0,0,0);
	    return -5;
	}
	
	bankHeader *dataBank = (bankHeader *)
	    offlen2ptr(rbp, rbp->banks[0]);
	
	if (!dataBank) continue;
	
	// Skip some headers:
	//   -  40 bytes bank header,
	//   -   4 bytes dummy
	//   - 128 bytes fiber header

	unsigned short* data = (unsigned short*)
	    (((char *)dataBank) + 40 + 4 + 128);

	// Check preamble
	for(int c=0; c<nCrates; c++) { // Loop over all crates

	    unsigned short cr_count   = data[c+30*0];
	    unsigned short cr_errflag = data[c+30*1];
	    //	    unsigned short cr_token   = data[c+30*2];
	    //	    unsigned short cr_trgcrt  = data[c+30*3];
	    
	    if (secpType == barrel && c<15) {
		
		//l3Log("%d %d", c, cr_count);
		
		if (cr_count != 164) errFlags |= 1;
		if (cr_errflag) errFlags |= 1;
		
	    }

	    
	}
	
	// Now skip the preamble:
	data += nCrates*4; // 4 shorts per crate

	for (int daqid=0; daqid < calib->getNTowers(); daqid++) {
	    int id = calib->daqToId(daqid);
	    
	    twr[id].setADC(data[daqid]);
	    twr[id].setNTracks(0);
	    	    
	}
    }
    

    return 0;
}

int gl3EMC::matchTracks(gl3Event *event)
{
  LOG(ERR, "matchTracks currently not available",0,0,0,0,0);

    return -1;

    //     l3xyzCoordinate extra;
    
    //     for (int i=0; i<event->getNTracks(); i++) {
    // 	gl3Track *track = event->getTrack(i);
    
    // 	extra = track->extraRadius(radius);
    
    // 	int id = etaPhiToId(extra.Geteta(), extra.Getphi());
    
    // 	tower[id].incrNTracks();
    //     }
    
    //     return 0;
}

void gl3EMC::reset()
{

    errFlags = 0;

    Etotal  = 0.;
    Ebarrel = 0.;
    Eendcap = 0.;

    minEnergyForTrackingSeed = 0.5;

    nBarrelTowers = 0;

    nEndcapTowers = 0;
}

int gl3EMC::readFromDaqReader(daqReader *rdr)
{
  int found = 0 ;
  daq_dta *dd ;
  
  dd = rdr->det("btow")->get("adc") ;
  if(dd) {
    while(dd->iterate()) {
      found = 1 ;
      
      btow_t *d = (btow_t *) dd->Void ;
      
      nBarrelTowers = 0;
      for(int i=0;i<BTOW_MAXFEE;i++) {
	for(int j=0;j<BTOW_DATSIZE;j++) {

	  //apply the stupid DAQ-crateNumber transfer
	  int crate;
	  if(i<=2) crate = 18-i;
	  else if(i>2 && i<=14) crate = 33-i;
	  else if(i==15) crate = 1;
	  else if(i>15) crate = 31-i;
	  
	  l3EmcTowerInfo* towerInfo = barrelCalib->getTowerInfo(crate, j);
	  int adc = d->adc[i][j];
	  barrelTower[nBarrelTowers].setTowerInfo(towerInfo);
	  barrelTower[nBarrelTowers].setADC(adc);
	  if(barrelTower[nBarrelTowers].getEnergy() > 0) nBarrelTowers++;
	}
	
      }
    }
  }

  
  return found ;
  
}

int gl3EMC::readRawBtowers(btow_t *d)
{
  nBarrelTowers = 0;
  for(int i=0;i<BTOW_MAXFEE;i++) {
    for(int j=0;j<BTOW_DATSIZE;j++) {
      
      int crate = d->preamble[i][3]%1024;
      // see l3EmcCalibration::getTowerInfo(int crate, int crateSeq)
      if (crate < 1 || crate > 30) {
          LOG(ERR, "Bad BTOW crate number: %d", crate);
          continue;
      }
      
      /*
      //apply the stupid DAQ-crateNumber transfer
      if(i<=2) crate = 18-i;
      else if(i>2 && i<=14) crate = 33-i;
      else if(i==15) crate = 1;
      else if(i>15) crate = 31-i;
      */
      
      l3EmcTowerInfo* towerInfo = barrelCalib->getTowerInfo(crate, j);
      int adc = d->adc[i][j];
      barrelTower[nBarrelTowers].setTowerInfo(towerInfo);
      barrelTower[nBarrelTowers].setADC(adc);
      //      cout<<"daqId:"<<towerInfo->getDaqID()<<" softId:"<<towerInfo->getSoftID()<<" crate:"<<crate<<" crateSeq:"<<j<<" adc:"<<adc<<" energy:"<<barrelTower[nBarrelTowers].getEnergy()<<" new crate:"<<d->preamble[i][3]%1024<<endl;
      if(barrelTower[nBarrelTowers].getEnergy() > 0.5) nBarrelTowers++;
      if (nBarrelTowers >= nTotalTowers) break;
    }
    
  }
  return 1 ;  
}

#if 0	// not used
int gl3EMC::readFromGl3Trg(gl3_trg_send_t *trgSend)
{
  if(nBarrelTowers != 0) return 0;
  for(int i=0; i<60; i++)
    {
      int adc = trgSend->btow_adc[i];
      if(adc == 0) break;
      int phiEta = trgSend->btow_soft_id[i];
      l3EmcTowerInfo* towerInfo = barrelCalib->getTowerInfoFromPhiEta(phiEta);
      barrelTower[nBarrelTowers].setTowerInfo(towerInfo);
      barrelTower[nBarrelTowers].setADC(adc, 0);
      nBarrelTowers ++;
      //      cout<<"daqId:"<<towerInfo->getDaqID()<<" softId:"<<towerInfo->getSoftID()<<" crate:"<<towerInfo->getCrate()<<" seq:"<<towerInfo->getCrateSeq()<<" adc:"<<adc<<" adc0:"<<adc+towerInfo->getPedestal()<<" energy:"<<barrelTower[nBarrelTowers-1].getEnergy()<<endl;
    }
  return 1;
}
#endif

void gl3EMC::getTrackingSeeds()
{
  const double pi = 3.1415927;
  const double zEdge = 20.;
  const double maxPhiDiff = 3.1415927/12 + 0.1;
  const double beginLayer = 45;
  const double endLayer = 26;
  const double sectorPhi[24] = {
    2.*pi/6., 1.*pi/6., 0.*pi/6., 11.*pi/6., 10.*pi/6., 9.*pi/6., 8.*pi/6., 7.*pi/
    6., 6.*pi/6., 5.*pi/6., 4.*pi/6., 3.*pi/6., 
    4.*pi/6., 5.*pi/6., 6.*pi/6., 7.*pi/6., 8.*pi/6., 9.*pi/6., 10.*pi/6., 11.*pi/
    6., 0.*pi/6., 1.*pi/6., 2.*pi/6., 3.*pi/6.};

  for(int i=0; i<24; i++)
    nTrackingSeeds[i] = 0;
  for(int i=0; i<nBarrelTowers; i++)
    {
      if(barrelTower[i].getEnergy() < minEnergyForTrackingSeed) continue;
      l3EmcTowerInfo* towerInfo = barrelTower[i].getTowerInfo();
      
      float z = towerInfo->getZ();
      float phi = towerInfo->getPhi();

      for(int j=0; j<24; j++)
	{
	  if(!((j<12 && z>-zEdge) || (j>=12 && z<zEdge))) continue;
	  float phiDiff = fabs(phi - sectorPhi[j]);
	  while(phiDiff > pi) phiDiff -= 2*pi;
	  phiDiff = fabs(phiDiff);
	  if(phiDiff > maxPhiDiff) continue;
	  if(nTrackingSeeds[j] >= 200) 
	    {
	      cout<<"gl3EMC::getTrackingSeeds(): Warn! seeds array full!   "<<j<<endl;
	      continue;
	    }
	  trackingSeeds[j][nTrackingSeeds[j]][0] = beginLayer;
	  trackingSeeds[j][nTrackingSeeds[j]][1] = endLayer;
	  trackingSeeds[j][nTrackingSeeds[j]][2] = emcR;
	  trackingSeeds[j][nTrackingSeeds[j]][3] = phi;
	  trackingSeeds[j][nTrackingSeeds[j]][4] = z;
	  nTrackingSeeds[j] ++;
	}
    }
	       
     
}
