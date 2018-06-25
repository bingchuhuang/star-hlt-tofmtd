#ifndef _TOF_HLT_SECTOR_H_
#define _TOF_HLT_SECTOR_H_

#include <vector>
#ifndef ST_NO_NAMESPACES
using std::vector;
#endif

#define MAX_TOF_HITS 10000
#define nTray  120 // number of trays installed
#define nModule  32 // number of MRPC modules per tray 
#define nCell  6 // number of cells per module 
#define nBoard  8 //number of TDIG board per module
#define maxNBin  50 // most binning number in spline fit
#define maxpvpdNBin  50 // most binning number in spline fit
#define nPVPDChannel 38 // 19 (east) + 19 (west)
#define VHRBIN2PS 24.4140625 // Very High resolution mode, pico-second per bin
                              // 1000*25/1024 (ps/chn)

struct TofCell{
	short trayid;
	short channel;// = nModule*6+nCell
	float triggertime;
	float tdc;
	float tot;
	float tof;
};
struct TofSend{
	int     nHits;
	TofCell cell[MAX_TOF_HITS];
};

class tof_hlt_sector
{
	public:
		tof_hlt_sector(char* pName="../HLT_TOF/");
		~tof_hlt_sector() ;

		int sector ;	// must be asigned! starts at 1
		unsigned int evt_counter ;
		int nEvent;
		float tofTotEdge[nTray][nBoard][maxNBin+1];
		float tofTotCorr[nTray][nBoard][maxNBin+1];
		float tofT0Corr[nTray][nModule][nCell];
		float pvpdTotEdge[nPVPDChannel][maxpvpdNBin+1];
		float pvpdTotCorr[nPVPDChannel][maxpvpdNBin+1];

		struct TofRawHit {
			short fiberid;           /// 0 1 2,3
			short trayid;            /// 1,2,......,120,for tray, 121, 122 for upvpd
			short globaltdcchan;     /// 0,1,......,191   
			float tdc;               /// tdc time (in bin) per hit.
			float triggertime;  /// trigger time 
		};
		vector<TofRawHit> TofLeadingHits;
		vector<TofRawHit> TofTrailingHits;
		void readparameters(char *pName="../HLT_TOF/");
		int new_event() ;
		int read_rdo_event(int rdo_1, char *start, int words_len) ;
		int do_event(TofSend *dst, int bytes_alloced) ;
		static bool optdc(const TofRawHit& lhs, const TofRawHit& rhs){return (lhs.tdc<rhs.tdc);}
	private:
		float timecount;
		inline float linearInter(float x1, float x2, float y1, float y2, float x);
		int upvpdLEchan[38];
		int upvpdTEchan[38];
		double timedelay[38];
		double delayminusfirstchannel[38];
		double delay[38];
		double mVpdDelay[38];
		double cutlow[122];
		double cuthi[122];
		int    mMRPC2TDIGChan[192];
		int    mTDIG2MRPCChan[192];
		static const int mNTray = 120;
		static const int mNTDIGOnTray = 8;
		static const int mNGLOBALCHANMAX = 192;

		static const int mNTDIGMAX = 1200;
		static const int mNChanOnTDIG = 24;
		static const int mNChanMAX = 1024;
		static const int mNBoardIdMAX = 4800;
		static const int mNValidBoards = 1023;

		static const int mEastVpdTrayId = 122;
		static const int mWestVpdTrayId = 121;

		int mTdigOnTray[mNTray][mNTDIGOnTray];
		int mTdigOnEastVpd[mNTDIGOnTray];
		int mTdigOnWestVpd[mNTDIGOnTray];   

		int mBoardId[mNTDIGMAX];
		int mBoardId2Index[mNBoardIdMAX];   // index in mNTDIGMAX for board #Id
		short mINLCorr[mNTDIGMAX][mNChanOnTDIG][mNChanMAX];


}; 
#endif
