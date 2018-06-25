#include <rtsLog.h>
#include <ctime>
#include <cmath>
#include <string.h>
#include <bitset>
#include <iostream>
#include <algorithm>

#include "mtd_hlt_reader.h"
using namespace std;

//================================================
mtd_hlt_reader::mtd_hlt_reader(uint run)
{
  run_number   = run;
  run_year     = run/1000000 - 1;
  time_counter = 0;
  mtd_send     = 0x0;
  mtdTF201     = 0;
  mtdTF201_2   = 0;
  memset(mtdStrip,       -1, sizeof(mtdStrip));
  memset(tray2TdigMap,   -1, sizeof(tray2TdigMap));
  memset(trigWinCut_low, -1, sizeof(trigWinCut_low));
  memset(trigWinCut_high,-1, sizeof(trigWinCut_high));
  memset(mModuleToQT,    -1, sizeof(mModuleToQT));
  memset(mModuleToQTPos, -1, sizeof(mModuleToQTPos));
  memset(mQTtoModule,    -1, sizeof(mQTtoModule));
  memset(mQTSlewBinEdge,  0,  sizeof(mQTSlewBinEdge)); 
  memset(mQTSlewCorr,     0,  sizeof(mQTSlewCorr));
  memset(mTrigQTpos,      0,  sizeof(mTrigQTpos));
  memset(mtdQTtac,        0,  sizeof(mtdQTtac)); 
  memset(mtdQTadc,        0,  sizeof(mtdQTadc));
  mFiredSectors.clear();
  mTpcSectorsForTracking = 0;
  init();
}


//================================================
mtd_hlt_reader::~mtd_hlt_reader()
{
  return;
}


//================================================
int mtd_hlt_reader::new_event()
{
  clear();
  evt_counter++;
  return evt_counter;
}

//================================================
int mtd_hlt_reader::read_rdo_event(int rdo, char *start, int words_len)
{
  LOG(NOTE,"MTD - Year %d, Event %d, RDO %d, read %d words",run_year,evt_counter,rdo,words_len);
  if(rdo!=1 && rdo!=2)
    {
      LOG(ERR,"MTD: wrong rdo = %d",rdo);
      return -1;
    }
  
  uint *ui = (uint*)start;
  std::clock_t starttime = std::clock();

  int halfbackleg = -99;
  int backleg = -99;

  for(int iword=0; iword<words_len; iword++)
    {
      uint dataword = ui[iword];
      LOG(DBG,"dataword %d: 0x%x",iword,dataword);
      
      if( (dataword&0xF0000000)>>28 == 0xA) continue;  /// header trigger data flag
      if( (dataword&0xF0000000)>>28 == 0xD) continue;  /// header tag word
      if( (dataword&0xF0000000)>>28 == 0xE) continue;  /// TDIG separator word

      // trigger time read from THUB      
      if( (dataword&0xF0000000)>>28 == 0x2)
	{
	  trigger_time[rdo-1] = dataword;
	  continue;
	}

      // geographical data
      if( (dataword&0xF0000000)>>28 == 0xC)
	{
	  halfbackleg = dataword&0x01;
	  backleg     = (dataword&0x0FE)>>1;
	  continue;
	}
      if(halfbackleg<0) continue;
      if(backleg<1 || backleg>30)
	{
	  LOG(ERR,"Unexpected backleg id: %d",backleg);
	  continue;
	}

      if( (dataword&0xF0000000)>>28 == 0x6) continue;

      // Look for edge type (4=leading, 5=trailing)
      int edge = int( (dataword&0xF0000000)>>28 );
      if( (edge!=4) && (edge!=5) ) continue;
      
      // From hereon, assume TDC data and decode accordingly
      int    tdcid = (dataword&0x0F000000)>>24;
      int    tdigid = ((tdcid&0xC)>>2) + halfbackleg*4;
      int    tdcchan = (dataword&0x00E00000)>>21;
      uint   timebin = ((dataword&0x7ffff)<<2)+((dataword>>19)&0x03);

      // Get tray number
      int tray = 0;
      for(int i=1; i<=5; i++)
	{
	  if(tray2TdigMap[backleg-1][i-1]==tdigid) 
	    {
	      tray = i;
	      break;
	    }
	}

      // Get global strip number
      int globalStrip = -1;
      int globalTdcChan = (tdcid%4 + 1) * 10 + tdcchan;
      for(int i=0; i<24; i++)
	{
	  if(mtdStrip[i]==globalTdcChan)
	    {
	      globalStrip = i+1; 
	      break;
	    }
	}
      if(tdigid>3) globalStrip = (globalStrip>12)? globalStrip-12:globalStrip+12; // Flip strip number for modules 4&5
      globalStrip += (tray-1)*24;

      // Construct raw hits
      MtdRawHit raw_hit;
      memset(&raw_hit,0,sizeof(raw_hit));
      raw_hit.tdc = timebin;
      raw_hit.fiberid = (char)(rdo-1);
      raw_hit.backleg = (char) backleg;
      raw_hit.globaltdcchan = (char) globalStrip;
      if(edge==4) mtdLeadingHits.push_back(raw_hit);
      else if (edge==5) mtdTrailingHits.push_back(raw_hit);
      LOG(DBG,"MTD - raw hit: backleg = %d, tray = %d, channel = %d, tdc = %d",(int)backleg,tray,globalStrip,timebin);
      
    }

  LOG(NOTE,"MTD: %d leading hits and %d trailing hits",mtdLeadingHits.size(),mtdTrailingHits.size());
  LOG(NOTE,"MTD: trigger time = %d of THUB %d",trigger_time,rdo-1);

  float elapsedTime = (std::clock()-starttime)/(float)CLOCKS_PER_SEC;
  LOG(NOTE,"MTD - read raw data: %f sec",elapsedTime);
  time_counter += elapsedTime;

  return 0;
}

//================================================
int mtd_hlt_reader::readFromMcEvent(McEvent* mcevt, char *dst, int& bytes_allocted)
{
    mtd_send = (Mtdsend*)dst;
    int bytes_used = sizeof(struct Mtdsend)-(MTD_MAX_HITS-mcevt->mNMtdHits)*sizeof(struct MtdHit);
    if (bytes_used > bytes_allocted) {
        LOG(WARN, "Too many MTD hits");
        mtd_send->nMtdHits = 0;
        bytes_allocted = 0;
        return 0;
    }

    bytes_allocted = bytes_used;
    mtd_send->nMtdHits = mcevt->mNMtdHits;
    
    for (int i = 0; i < mtd_send->nMtdHits; ++i) {
        MtdHit& iHit = mtd_send->mtdhits[i];
        iHit.fiberId = mcevt->mMtdFiberId[i];
        iHit.backleg = mcevt->mMtdBackleg[i];
        iHit.tray = mcevt->mMtdTray[i];
        iHit.channel = mcevt->mMtdChannel[i];
        iHit.leadingEdgeTime.first = mcevt->mMtdLeadingEdgeTime[i][0];
        iHit.leadingEdgeTime.second = mcevt->mMtdLeadingEdgeTime[i][1];
        iHit.trailingEdgeTime.first = mcevt->mMtdTrailingEdgeTime[i][0];
        iHit.trailingEdgeTime.second = mcevt->mMtdTrailingEdgeTime[i][1];
	iHit.hlt_trackId = -1;
	iHit.delta_z = -999.;
	iHit.delta_y = -999.;
    }

    return mtd_send->nMtdHits;
}

//================================================
int mtd_hlt_reader::read_mtd_trigger(void *start)
{
  int ret = -1;
  void*           dd             = start;
  TriggerDataBlk *mdata          = (TriggerDataBlk *)dd;
  swapI((unsigned int*)&mdata->totalTriggerLength);
  unsigned int    size           = mdata->totalTriggerLength;
  TriggerDataBlk *data           = new TriggerDataBlk; 
  memcpy(data,mdata,size); 

  // MTD TF201
  swapI((unsigned int*)&data->L1_DSM_ofl.offset);
  L1_DSM_Data    *L1_DSM         = (L1_DSM_Data *)(((char *)data) + data->L1_DSM_ofl.offset);
  swapL1_DSM(L1_DSM);
  mtdTF201   = L1_DSM->TOF[3];
  if(run_year == 16)
    mtdTF201_2 = L1_DSM->TOF[5];
  else
    mtdTF201_2 = 0;

  // MTD QT
  unsigned short mxq[16][32];
  unsigned char tmxq[16][32];
  memset(mxq,0,sizeof(mxq)); memset(tmxq,0,sizeof(tmxq));

  const int MXQ_CONF_NUM = 3;
  TrgOfflen* offlen = data->MainX;
  swapRawDetOfflen(offlen);
  int length=offlen[MXQ_CONF_NUM].length; 
  if (length>0)
    {
      QTBlock* mMXQ = (QTBlock*)((char*)data + offlen[MXQ_CONF_NUM].offset);
      if (mMXQ)
	{
	  swapI((unsigned int*)&mMXQ->length);
	  swapI((unsigned int*)&mMXQ->dataLoss);
	  swapIn(mMXQ->data, mMXQ->length/4);
	  decodeQT(mMXQ->length/4, mMXQ->data, mxq, tmxq); 
		
	  for(int i=0; i<32; i++)
	    {
	      int type = (i/4)%2;
	      if(type==1)
		{
		  if(run_year<=15)
		    {
		      mtdQTtac[0][i-i/4*2-2] = mxq[0][i];
		      mtdQTtac[1][i-i/4*2-2] = mxq[10][i];
		      mtdQTtac[2][i-i/4*2-2] = mxq[12][i];
		      mtdQTtac[3][i-i/4*2-2] = mxq[14][i];
		    }
		  else if(run_year==16)
		    {
		      mtdQTtac[0][i-i/4*2-2] = mxq[0][i];
		      mtdQTtac[1][i-i/4*2-2] = mxq[9][i];
		      mtdQTtac[2][i-i/4*2-2] = mxq[10][i];
		      mtdQTtac[3][i-i/4*2-2] = mxq[11][i];
		      mtdQTtac[4][i-i/4*2-2] = mxq[12][i];
		      mtdQTtac[5][i-i/4*2-2] = mxq[13][i];
		      mtdQTtac[6][i-i/4*2-2] = mxq[14][i];
		      mtdQTtac[7][i-i/4*2-2] = mxq[15][i];
		    }
		  else
		    {
		      mtdQTtac[0][i-i/4*2-2] = mxq[0][i];
		      mtdQTtac[1][i-i/4*2-2] = mxq[9][i];
		      mtdQTtac[2][i-i/4*2-2] = mxq[11][i];
		      mtdQTtac[3][i-i/4*2-2] = mxq[13][i];
		    }
		}
	      else
		{
		  if(run_year<=15)
		    {
		      mtdQTadc[0][i-i/4*2] = mxq[0][i];
		      mtdQTadc[1][i-i/4*2] = mxq[10][i];
		      mtdQTadc[2][i-i/4*2] = mxq[12][i];
		      mtdQTadc[3][i-i/4*2] = mxq[14][i];
		    }
		  else if(run_year==16)
		    {
		      mtdQTadc[0][i-i/4*2] = mxq[0][i];
		      mtdQTadc[1][i-i/4*2] = mxq[9][i];
		      mtdQTadc[2][i-i/4*2] = mxq[10][i];
		      mtdQTadc[3][i-i/4*2] = mxq[11][i];
		      mtdQTadc[4][i-i/4*2] = mxq[12][i];
		      mtdQTadc[5][i-i/4*2] = mxq[13][i];
		      mtdQTadc[6][i-i/4*2] = mxq[14][i];
		      mtdQTadc[7][i-i/4*2] = mxq[15][i];
		    }
		  else
		    {
		      mtdQTadc[0][i-i/4*2] = mxq[0][i];
		      mtdQTadc[1][i-i/4*2] = mxq[9][i];
		      mtdQTadc[2][i-i/4*2] = mxq[11][i];
		      mtdQTadc[3][i-i/4*2] = mxq[13][i];
		    }
		}
	    }
	  ret = 0;
	}
    }

  // find the QT channel that fired the trigger
  int mxq_tacsum[MTD_NQTBOARD][2];
  int mxq_tacsum_pos[MTD_NQTBOARD][2];
  memset(mxq_tacsum,         0,  sizeof(mxq_tacsum)); 
  memset(mxq_tacsum_pos,    -1,  sizeof(mxq_tacsum_pos));
  int j[2], a[2];
  for(int im=0; im<MTD_NQTBOARD; im++)
    {
      for(int i=0; i<8; i++)
	{
	  if(run_year!=16 && im>=4) continue; // 4 QT boards except for run 2016
	  if(run_year==16 && i%2==0) continue;
	  LOG(DBG,"QT = %2d, pos = %2d, j2 = %4d, j3 = %4d, a2 = %4d, a3 = %4d\n",im+1, i+1,mtdQTtac[im][i*2],mtdQTtac[im][i*2+1],mtdQTadc[im][i*2],mtdQTadc[im][i*2+1]);

	  // slewing correction
	  for(int k=0; k<2; k++)
	    {
	      j[k] = mtdQTtac[im][i*2+k];
	      a[k] = mtdQTadc[im][i*2+k];
	      int slew_bin = -1;
	      if(a[k]>=0 && a[k]<=mQTSlewBinEdge[im][i*2+k][0]) slew_bin = 0;
	      else
		{
		  for(int l=1; l<8; l++)
		    {
		      if(a[k]>mQTSlewBinEdge[im][i*2+k][l-1] && a[k]<=mQTSlewBinEdge[im][i*2+k][l])
			{
			  slew_bin = l;
			  break;
			}
		    }
		}
	      if(slew_bin>=0)
		j[k] += mQTSlewCorr[im][i*2+k][slew_bin];
	    }

	  if(j[0]<80 || j[1]<80) continue;
	  if(abs(j[0]-j[1])>600) continue;
	  
	  // position correction
	  int module = mQTtoModule[im][i];
	  int sumTac = int( j[0] + j[1] + abs(module-3)*1./8 * (j[0]-j[1]) );

	  if(mxq_tacsum[im][0] < sumTac)
	    {
	      mxq_tacsum[im][1] = mxq_tacsum[im][0];
	      mxq_tacsum[im][0] = sumTac;

	      mxq_tacsum_pos[im][1] = mxq_tacsum_pos[im][0];
	      mxq_tacsum_pos[im][0] = i+1;
	    }
	  else if (mxq_tacsum[im][1] < sumTac)
	    {
	      mxq_tacsum[im][1]  = sumTac;
	      mxq_tacsum_pos[im][1] = i+1;
	    }
	}
    }

  LOG(NOTE, "MTD TF201 =  %s, %s",
      bitset<12>(mtdTF201).to_string().c_str(),
      bitset<12>(mtdTF201_2).to_string().c_str());

  memset(mTrigQTpos,-1, sizeof(mTrigQTpos));

  for(int i = 0; i < 4; i++)
    {
      for(int j=0; j<2; j++)
	{
	  if(run_year == 16)
	    {
	      if((mtdTF201>>(i*2+j+4))&0x1)
		{
		  int qt = i*2;
		  mTrigQTpos[qt][j] = mxq_tacsum_pos[qt][j];
		  LOG(DBG,"qt = %d, pos = %d, sum = %d\n",qt+1, mxq_tacsum_pos[qt][j], mxq_tacsum[qt][j]);
		}
	      if((mtdTF201_2>>(i*2+j+4))&0x1)
		{
		  int qt = i*2+1;
		  mTrigQTpos[qt][j] = mxq_tacsum_pos[qt][j];
		  LOG(DBG,"qt = %d, pos = %d, sum = %d\n",qt+1, mxq_tacsum_pos[qt][j], mxq_tacsum[qt][j]);
		}
	    }
	  else
	    {
	      if((mtdTF201>>(i*2+j+4))&0x1)
		{
		  int qt = i;
		  mTrigQTpos[qt][j] = mxq_tacsum_pos[qt][j];
		  LOG(DBG,"qt = %d, pos = %d, sum = %d\n",qt+1, mxq_tacsum_pos[qt][j], mxq_tacsum[qt][j]);
		}
	    }
	}
    }

  delete data;
  return ret;
}

//================================================
int mtd_hlt_reader::do_event(Mtdsend *dst, int bytes_allocated, uint &tpcMask)
{
  LOG(NOTE,"MTD-Event %d: maximum %d bytes",evt_counter,bytes_allocated);

  std::clock_t starttime = std::clock();
  mtd_send = dst;

  fillMtdSingleHits();
  fillMtdHits();
  tpcMask = getTpcSectorMask();
  mtd_send->tpcMask = tpcMask;
  
  int nMuon = 0;
  for(int i = 0; i < 4; i++)
    {
      for(int j=0; j<2; j++)
	{
	  if((mtdTF201>>(i*2+j+4))&0x1) nMuon++;
	  if((mtdTF201_2>>(i*2+j+4))&0x1) nMuon++;
	}
    }

  LOG(NOTE, "# of muon triggers: %d", nMuon);
  if(nMuon>1) mtd_send->isDimuon = 1;
  else        mtd_send->isDimuon = 0;
  
  int bytes_used = sizeof(struct Mtdsend)-(MTD_MAX_HITS-mtd_send->nMtdHits)*sizeof(struct MtdHit);

  float elapsedTime = (std::clock()-starttime)/(float)CLOCKS_PER_SEC;
  LOG(NOTE,"MTD - make hits: %f sec",elapsedTime);
  LOG(NOTE,"Bytes used = %d",bytes_used);
  time_counter += elapsedTime;

  return bytes_used;
}

//================================================
void mtd_hlt_reader::fillMtdSingleHits()
{
  // loop over leading edges
  for(size_t j=0; j<mtdLeadingHits.size(); j++)
    {
      char   fiberid   = mtdLeadingHits[j].fiberid;
      int    backleg   = (int)mtdLeadingHits[j].backleg;
      int    gchan     = (int)mtdLeadingHits[j].globaltdcchan; // 1-120
      uint   tdc       = mtdLeadingHits[j].tdc;
      int    itray     = (gchan-1)/24 + 1; // 1-5
      int    gtray     = (backleg-1)*MTD_NTRAY + itray; // 1-150
      int    ichan     = (gchan-1)%24;  // 0-23

      if (9 == backleg) continue; // backleg is not really in use

      if(backleg<1 || backleg>MTD_NBACKLEG ||
	 itray<1   || itray>MTD_NTRAY ||
	 ichan<0   || ichan>=MTD_NCHANNEL)
	{
	  LOG(ERR,"MTD - wrong raw hit: backleg = %d, tray = %d, channel = %d",(int)backleg,itray,ichan);
	  continue;
	}
      
      // check if raw hit already exists in array
      bool exist = false;
      int  index = -1;
      for(size_t ii=0; ii<mtdSingleHits[gtray-1].size(); ii++)
	{
	  if(backleg== (int)mtdSingleHits[gtray-1][ii].backleg &&
	     itray  == (int)mtdSingleHits[gtray-1][ii].tray &&
	     ichan  == (int)mtdSingleHits[gtray-1][ii].channel)
	    {
	      exist = true;
	      index = (int)ii;
	      break;
	    }
	}
      if(exist)
	{
	  mtdSingleHits[gtray-1][index].leadingEdgeTime.push_back(tdc);
	}
      else
	{
	  MtdSingleHit rawHit;
	  rawHit.fiberId = fiberid;
	  rawHit.backleg = (char)backleg;
	  rawHit.tray    = (char)itray;
	  rawHit.channel = (char)ichan;
	  rawHit.leadingEdgeTime.push_back(tdc);
	  mtdSingleHits[gtray-1].push_back(rawHit);
	}	
    }

  // loop over trailing edges
  for(size_t j=0; j<mtdTrailingHits.size(); j++)
    {
      char   fiberid   = mtdTrailingHits[j].fiberid;
      int    backleg   = (int)mtdTrailingHits[j].backleg;
      int    gchan     = (int)mtdTrailingHits[j].globaltdcchan; // 1-120
      uint   tdc       = mtdTrailingHits[j].tdc;
      int    itray     = (gchan-1)/24 + 1; // 1-5
      int    gtray     = (backleg-1)*MTD_NTRAY + itray; // 1-150
      int    ichan     = (gchan-1)%24;  // 0-23

      if (9 == backleg) continue; // backleg is not really in use
      
      if(backleg<1 || backleg>MTD_NBACKLEG ||
	 itray<1   || itray>MTD_NTRAY ||
	 ichan<0   || ichan>=MTD_NCHANNEL)
	{
	  LOG(ERR,"MTD - wrong raw hit: backleg = %d, tray = %d, channel = %d",(int)backleg,itray,ichan);
	  continue;
	}
      
      // check if raw hit already exists in array
      bool exist = false;
      int  index = -1;
      for(size_t ii=0; ii<mtdSingleHits[gtray-1].size(); ii++)
	{
	  if(backleg== (int)mtdSingleHits[gtray-1][ii].backleg &&
	     itray  == (int)mtdSingleHits[gtray-1][ii].tray &&
	     ichan  == (int)mtdSingleHits[gtray-1][ii].channel)
	    {
	      exist = true;
	      index = (int)ii;
	      break;
	    }
	}
      if(exist)
	{
	  mtdSingleHits[gtray-1][index].trailingEdgeTime.push_back(tdc);
	}
      else
	{
	  MtdSingleHit rawHit;
	  rawHit.fiberId = fiberid;
	  rawHit.backleg = (char)backleg;
	  rawHit.tray    = (char)itray;
	  rawHit.channel = (char)ichan;
	  rawHit.trailingEdgeTime.push_back(tdc);
	  mtdSingleHits[gtray-1].push_back(rawHit);
	}	
    }
}


//================================================
void mtd_hlt_reader::fillMtdHits()
{
  int nhit = 0;
  for(int i=0; i<MTD_ALLTRAY; i++)
    {
      for(size_t j=0; j<mtdSingleHits[i].size(); j++)
	{
	  int jchan = (int)mtdSingleHits[i][j].channel;
	  if(mtdSingleHits[i][j].leadingEdgeTime.size()<=0 || mtdSingleHits[i][j].trailingEdgeTime.size()<=0) continue;
	  for(size_t k=j+1; k<mtdSingleHits[i].size(); k++)
	    {
	      int kchan = (int)mtdSingleHits[i][k].channel;
	      if(fabs(jchan-kchan)!=12) continue;
	      if(mtdSingleHits[i][k].leadingEdgeTime.size()<=0 || mtdSingleHits[i][k].trailingEdgeTime.size()<=0) continue;
	      MtdHit hit;
	      hit.fiberId     = mtdSingleHits[i][j].fiberId;
	      hit.backleg     = mtdSingleHits[i][j].backleg;
	      hit.tray        = mtdSingleHits[i][j].tray;
	      hit.hlt_trackId = -1;
	      hit.delta_z     = -999.;
	      hit.delta_y     = -999.;

	      if(jchan<12)
		{
		  hit.channel = mtdSingleHits[i][j].channel;
		  hit.leadingEdgeTime.first   = mtdSingleHits[i][j].leadingEdgeTime[0]*MTD_VHRBIN2PS/1e3;
		  hit.leadingEdgeTime.second  = mtdSingleHits[i][k].leadingEdgeTime[0]*MTD_VHRBIN2PS/1e3;
		  hit.trailingEdgeTime.first  = mtdSingleHits[i][j].trailingEdgeTime[0]*MTD_VHRBIN2PS/1e3;
		  hit.trailingEdgeTime.second = mtdSingleHits[i][k].trailingEdgeTime[0]*MTD_VHRBIN2PS/1e3;
		}
	      else
		{
		  hit.channel = mtdSingleHits[i][k].channel;
		  hit.leadingEdgeTime.first   = mtdSingleHits[i][k].leadingEdgeTime[0]*MTD_VHRBIN2PS/1e3;
		  hit.leadingEdgeTime.second  = mtdSingleHits[i][j].leadingEdgeTime[0]*MTD_VHRBIN2PS/1e3;
		  hit.trailingEdgeTime.first  = mtdSingleHits[i][k].trailingEdgeTime[0]*MTD_VHRBIN2PS/1e3;
		  hit.trailingEdgeTime.second = mtdSingleHits[i][j].trailingEdgeTime[0]*MTD_VHRBIN2PS/1e3;
		}
	      if(isMtdHitFiredTrigger(hit.backleg,hit.tray)) hit.isTrigger = 1;
	      else hit.isTrigger = 0;

	      double hitTime = 0.5 * (hit.leadingEdgeTime.first + hit.leadingEdgeTime.second);
	      if( isHitInTrigWin( (int)hit.fiberId, (int)hit.backleg, (int)hit.tray, hitTime ) )
		{
		  mtd_send->mtdhits[nhit] = hit;
		  LOG(DBG,"MTD - hit: backleg = %d, tray = %d, channel = %d, lead1 = %f, lead2 = %f, trail1 = %f, trail2 = %f",(int)mtd_send->mtdhits[nhit].backleg,(int)mtd_send->mtdhits[nhit].tray,(int)mtd_send->mtdhits[nhit].channel,mtd_send->mtdhits[nhit].leadingEdgeTime.first,mtd_send->mtdhits[nhit].leadingEdgeTime.second,mtd_send->mtdhits[nhit].trailingEdgeTime.first,mtd_send->mtdhits[nhit].trailingEdgeTime.second);
		  nhit++;
		}
	    } 
	}      
    }
  mtd_send->nMtdHits = nhit; 
  LOG(NOTE,"MTD: %d hits",nhit);
}

//================================================
bool mtd_hlt_reader::isHitInTrigWin(int thub, int backleg, int tray, double hit_time)
{
  return true;
}

//================================================
uint mtd_hlt_reader::getTpcSectorMask()
{
  mTpcSectorsForTracking = 0xFFFFFF;
  mFiredSectors.clear();

  // find the mtd hits that fire the trigger
  int nMtdHits = mtd_send->nMtdHits;
  for(int i=0; i<nMtdHits; i++)
    {
      MtdHit hit = mtd_send->mtdhits[i];
      int backleg = hit.backleg;
      int module  = hit.tray;
      int cell    = hit.channel;
      if(!hit.isTrigger) continue;
      LOG(NOTE, "Check hit with (%d,%d)",backleg,module);
      double hit_phi = getMtdHitGlobalPhi(backleg, module, cell);
      findTpcSectorsForTracking(hit_phi, module);
    }
  determineTpcTrackingMask();
  return mTpcSectorsForTracking;
}


//================================================
void mtd_hlt_reader::decodeQT(unsigned int ndata, unsigned int* data, unsigned short adc[16][32], unsigned char tdc[16][32])
{
    int MaxQTData = 529;
    if (ndata==0) return;
    if ((int)ndata>MaxQTData)    { printf("QT data length %d is too long (max is %d)\n",ndata,MaxQTData); return;}
    if (data[ndata-1] != 0xac10) { printf("Wrong QT data last word %x (should be 0xAC10)\n",data[ndata-1]); return;}
    int header=1;
    unsigned int crate,addr,ch,nline,oldch;
    for (unsigned int i=0; i<ndata-1; i++){
        unsigned int d = data[i];
        if (header==1){
            crate =  (d & 0xff000000) >> 24;
            addr  = ((d & 0x00ff0000) >> 16) - 0x10;
            nline =  (d & 0x000000ff);
            oldch = 0;
            if(nline>0) header=0;
        }
        else {
            ch = (d & 0xf8000000) >> 27;
            adc[addr][ch] = (unsigned short)  (d & 0x00000fff);
            tdc[addr][ch] = (unsigned char)  ((d & 0x0001f000) >> 12);
            oldch=ch;
            nline--;
            if (nline==0) header=1;
        }    
    }
}

//================================================
void mtd_hlt_reader::determineTpcTrackingMask()
{
  // remove duplicated TPC sectors
  sort(mFiredSectors.begin(),mFiredSectors.end());
  mFiredSectors.erase(unique(mFiredSectors.begin(),mFiredSectors.end()),mFiredSectors.end());
  
  // 24-bit mask for TPC tracking
  if (mFiredSectors.size()) mTpcSectorsForTracking = 0;
  for(unsigned int i=0; i<mFiredSectors.size(); i++)
    {
      int bit = mFiredSectors[i] - 1;
      mTpcSectorsForTracking |= (1U << bit);
    }
  LOG(NOTE, "output TPC mask =  %s, %2d sectors",
      bitset<24>(mTpcSectorsForTracking).to_string().c_str(),
      mFiredSectors.size());
}

//================================================
void mtd_hlt_reader::findTpcSectorsForTracking(const double hit_phi, const int hit_module)
{
  ///
  /// Find the corresponding TPC sector for a given MTD hit
  /// For hits in module 1, only tracking TPC sectors on east
  /// For hits in module 5, only tracking TPC sectors on west
  /// For hits in module 2-4, tracking TPC sectors both on east and west
  ///

  intVec westTpc, eastTpc;
  westTpc.clear();
  eastTpc.clear();

  if(hit_module<5) eastTpc = findEastTpcSectors(hit_phi);
  if(hit_module>1) westTpc = findWestTpcSectors(hit_phi);

  for(unsigned int i=0; i<eastTpc.size(); i++)
    mFiredSectors.push_back(eastTpc[i]);

  for(unsigned int i=0; i<westTpc.size(); i++)
    mFiredSectors.push_back(westTpc[i]);
}

//================================================
vector<int> mtd_hlt_reader::findWestTpcSectors(const double hit_phi)
{
  ///
  /// Given the azimuthal angle of a MTD hit, find the two spatially closest TPC
  /// sectors on the west side of STAR
  ///
 
  intVec sectors;
  sectors.clear();

  double tpc_sector_width = MTD_PI/6.;

  int tpc_sector_1 = 3 - int(floor(hit_phi/tpc_sector_width));
  if(tpc_sector_1<1) tpc_sector_1 += 12;

  int tpc_sector_2 = tpc_sector_1 - 1;
  if(tpc_sector_2<1) tpc_sector_2 += 12;
  
  sectors.push_back(tpc_sector_1);
  sectors.push_back(tpc_sector_2);

  return sectors;
}

//================================================
vector<int> mtd_hlt_reader::findEastTpcSectors(const double hit_phi)
{
  ///
  /// Given the azimuthal angle of a MTD hit, find the two spatially closest TPC
  /// sectors on the east side of STAR
  ///

  intVec sectors;
  sectors.clear();

  double tpc_sector_width = MTD_PI/6.;

  int tpc_sector_1 = int(floor(hit_phi/tpc_sector_width))+21;
  if(tpc_sector_1>24) tpc_sector_1 -= 12;

  int tpc_sector_2 = tpc_sector_1 + 1;
  if(tpc_sector_2>24) tpc_sector_2 -= 12;
  
  sectors.push_back(tpc_sector_1);
  sectors.push_back(tpc_sector_2);

  return sectors;
}

//================================================
double mtd_hlt_reader::getMtdHitGlobalPhi(const int backleg, const int module, const int cell)
{
  ///
  /// Approximate phi center of a MTD hit
  ///

  double backlegPhiCen = MTD_FIRSTBACKLEGPHICENTER + (backleg-1) * (MTD_BACKLEGPHIWIDTH+MTD_BACKLEGPHIGAP);
  if(backlegPhiCen>2*MTD_PI) backlegPhiCen -= 2*MTD_PI;
  double stripPhiCen = 0;
  if(module>0 && module<4)
    {
      stripPhiCen = backlegPhiCen - (MTD_NCHANNEL/4.-0.5-cell)*(MTD_CELLWIDTH+MTD_CELLGAP)/MTD_MINRADIUS;
    }
  else
    {
      stripPhiCen = backlegPhiCen + (MTD_NCHANNEL/4.-0.5-cell)*(MTD_CELLWIDTH+MTD_CELLGAP)/MTD_MINRADIUS;
    }
  return rotatePhi(stripPhiCen);
}

//================================================
double mtd_hlt_reader::rotatePhi(const double phi)
{
  double outPhi = phi;
  while(outPhi<0) outPhi += 2*MTD_PI;
  while(outPhi>2*MTD_PI) outPhi -= 2*MTD_PI;
  return outPhi;
}


//================================================
bool mtd_hlt_reader::isMtdHitFiredTrigger(const int backleg, const int module)
{
  int qt = mModuleToQT[backleg-1][module-1];
  int pos = mModuleToQTPos[backleg-1][module-1];
  return (pos==mTrigQTpos[qt-1][0] || pos==mTrigQTpos[qt-1][1]);
}


//================================================
void mtd_hlt_reader::init()
{

  clear();
  for(int i=0;i<24;i++) mtdStrip[i] = MTD_STRIP_MAP[i];
  for(int i=0; i<MTD_NBACKLEG; i++)
    {
      for (int j=0; j<MTD_NTRAY; j++)
	{
	  tray2TdigMap[i][j]    = (short)MTD_TRAY2DIG_MAP[i*MTD_NTRAY+j];
	}
    }

  if(run_year>=14)
    {
      int qt, channel;
      for(int i=0; i<MTD_NBACKLEG; i++)
	{
	  for (int j=0; j<MTD_NTRAY; j++)
	    {
	      if(run_year==16)
		{
		  qt = MODULE2QT_RUN16[i][j];
		  channel = MODULE2CHAN_RUN16[i][j];
		}
	      else
		{
		  qt = MODULE2QT_RUN17[i][j];
		  channel = MODULE2CHAN_RUN17[i][j];	  
		}
	      mModuleToQT[i][j]    = qt;
	      if(channel<0)
		{
		  mModuleToQTPos[i][j] = channel;
		}
	      else
		{
		  if(channel%8==1) mModuleToQTPos[i][j] = 1 + channel/8 * 2;
		  else             mModuleToQTPos[i][j] = 2 + channel/8 * 2;
		}
	      if(mModuleToQT[i][j]>0 && mModuleToQTPos[i][j]>0)
		mQTtoModule[mModuleToQT[i][j]-1][mModuleToQTPos[i][j]-1] = j + 1;
	      LOG(DBG,"bl = %d, mod = %d, qt = %d, chan = %d",i+1,j+1,mModuleToQT[i][j],mModuleToQTPos[i][j]);
	    }
	}

      for(int j=0; j<MTD_NQTBOARD; j++)
	{
	  for(int i=0; i<16; i++)
	    {
	      for(Int_t k=0; k<8; k++)
		{
		  Int_t index = j*16*8 + i*8 + k;
		  if(run_year==16)
		    {
		      mQTSlewBinEdge[j][i][k] = MTD_QTSLEW_BINEDGE_RUN16[index];
		      mQTSlewCorr[j][i][k] = MTD_QTSLEW_CORR_RUN16[index];
		    }
		  else
		    {
		      // do not apply slewing correction
		      mQTSlewBinEdge[j][i][k] = 4095;
		      mQTSlewCorr[j][i][k] = 0;
		    }
		}
	    }
	}
    }
}


//================================================
void mtd_hlt_reader::clear()
{
  memset(trigger_time, -1, sizeof(trigger_time));
  mtdLeadingHits.clear();
  mtdTrailingHits.clear();
  for(int i=0; i<MTD_ALLTRAY; i++)
    {
      mtdSingleHits[i].clear();
    }
}
