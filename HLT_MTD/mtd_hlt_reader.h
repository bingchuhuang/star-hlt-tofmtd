#ifndef _MTD_HLT_READER_H_
#define _MTD_HLT_READER_H_

/* #include <Stiostream.h> */
#include <vector>
#include "mtd_defs.h"
#include "../L4_HLT/McEvent.h"
#include <trgDataDefs.h>

/* #ifndef ST_NO_NAMESPACES */
/* using std::vector; */
/* #endif */

/* typedef vector<int>  intVec; */

class mtd_hlt_reader
{
 public:
  mtd_hlt_reader(uint run);
  ~mtd_hlt_reader();
    
  void init();
  int  new_event();
  int  read_rdo_event(int rdo, char *start, int words_len);
  int  do_event(Mtdsend *dst, int bytes_allocted, uint &tpcMask);
  void fillMtdSingleHits();
  void fillMtdHits();
  bool isHitInTrigWin(int thub, int backleg, int tray, double hit_time);

  int  readFromMcEvent(McEvent* mcevt, char *dst, int& bytes_allocted);

  bool isMtdHitFiredTrigger(const int backleg, const int module);
  int  read_mtd_trigger(void *start);
  void decodeQT(unsigned int ndata, unsigned int* data, unsigned short adc[16][32], unsigned char tdc[16][32]);
  uint getTpcSectorMask();
  void determineTpcTrackingMask();
  void findTpcSectorsForTracking(const double hit_phi, const int hit_module);
  vector<int> findWestTpcSectors(const double hit_phi);
  vector<int> findEastTpcSectors(const double hit_phi);
  double getMtdHitGlobalPhi(const int backleg, const int module, const int cell);
  double rotatePhi(const double phi);

  void clear();

 protected:
  // Byte swapping functions
  void swapI(unsigned int *);
  void swapIn(unsigned int *, unsigned int);
  void swapSS(unsigned int *);
  void swapSSn(unsigned int *, unsigned int);
  void swapL1_DSM(L1_DSM_Data* L1_DSM);
  void swapRawDetOfflen(TrgOfflen* offlen);
  void swapOfflen(TrgOfflen* offlen);

 private:
  uint       run_number;
  uint       run_year;
  float      time_counter;
  uint       evt_counter;
  int        mtdStrip[24];
  int        tray2TdigMap[MTD_NBACKLEG][MTD_NTRAY];  
  double     trigWinCut_low[MTD_NBACKLEG][MTD_NTRAY];
  double     trigWinCut_high[MTD_NBACKLEG][MTD_NTRAY];
  uint       trigger_time[2];

  Mtdsend    *mtd_send;

  int        mtdTF201;
  int        mtdTF201_2;
  int        mModuleToQT[MTD_NBACKLEG][MTD_NTRAY];     // Map from module to QT board index
  int        mModuleToQTPos[MTD_NBACKLEG][MTD_NTRAY];  // Map from module to the position on QT board
  int        mQTtoModule[MTD_NQTBOARD][8];             // Map from QT board to module index
  int        mQTSlewBinEdge[MTD_NQTBOARD][16][8];      // Bin edges for online slewing correction for QT
  int        mQTSlewCorr[MTD_NQTBOARD][16][8];         // Slewing correction values for QT
  int        mTrigQTpos[MTD_NQTBOARD][2];              // Channel fires trigger in each QT
  uint       mtdQTtac[MTD_NQTBOARD][16];
  uint       mtdQTadc[MTD_NQTBOARD][16];
  intVec     mFiredSectors;
  uint       mTpcSectorsForTracking;                   // 24-bit mask for partial tracking (sector 1 is least significant bit)

  vector<MtdRawHit> mtdLeadingHits;
  vector<MtdRawHit> mtdTrailingHits;
  vector<MtdSingleHit> mtdSingleHits[MTD_ALLTRAY];
};

inline void mtd_hlt_reader::swapI(unsigned int *var){
  *var = 
    (*var & 0xff000000) >> 24 |
    (*var & 0x00ff0000) >> 8  |
    (*var & 0x0000ff00) << 8  |
    (*var & 0x000000ff) << 24 ;
}

inline void mtd_hlt_reader::swapSS(unsigned int *var){
  *var = 
    (*var & 0xff000000) >> 8 |
    (*var & 0x00ff0000) << 8 |
    (*var & 0x0000ff00) >> 8 |
    (*var & 0x000000ff) << 8;
}

inline void mtd_hlt_reader::swapIn(unsigned int *var, unsigned int n)  {for(unsigned int i=0; i<n; i++)   {swapI(var++);} }
inline void mtd_hlt_reader::swapSSn(unsigned int *var, unsigned int n) {for(unsigned int i=0; i<n/2; i++) {swapSS(var++);} }
inline void mtd_hlt_reader::swapOfflen(TrgOfflen* offlen)
{
  swapI((unsigned int*)&offlen->offset);
  swapI((unsigned int*)&offlen->length);
}

inline void mtd_hlt_reader::swapL1_DSM(L1_DSM_Data* L1_DSM)
{
    swapI((unsigned int*)&L1_DSM->length);
    swapSSn((unsigned int*)L1_DSM->TOF,16+8*7);
}

inline void mtd_hlt_reader::swapRawDetOfflen(TrgOfflen* offlen)
{
  int i;
  for (i=0; i<MAX_OFFLEN; i++) { 
    swapOfflen(&offlen[i]); 
  }
}


#endif

