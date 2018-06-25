#ifndef GL3TOF_H
#define GL3TOF_H

//#include <vector>

#include "gl3Track.h"
#include "l3Coordinates.h"

#define MAX_TOF_HITS 10000
#define nTray  120 // number of trays installed
#define nModule  32 // number of MRPC modules per tray 
#define nCell  6 // number of cells per module 
#define nBoard  8 //number of TDIG board per module
#define maxNBin  50 // most binning number in spline fit
#define maxNZBin  39 // most binning number in spline fit
#define nPVPDChannel 38 // 19 (east) + 19 (west)

#define M_PION 0.13957

#ifndef PI
#define PI 3.14159
#endif

//class l3xyzCoordinate;
class gl3Node;
class gl3Bischel;

class gl3TOF{
	public:
		gl3TOF(char* tofAlignGeom = "tofAlignGeom.dat", char* zCali_4DB = "zCali_4DB.dat", char* dir = "parametersDirectory");
		~gl3TOF();
		int Init(char* tofAlignGeom = "tofAlignGeom.dat", char* zCali_4DB = "zCali_4DB.dat");
		void reset();
		typedef struct pointvec{
			float y;
			float z;
		}PointVec;
		typedef struct mcell{
			int tray;
			int module;
			int cell;
		}CellVec;
		typedef struct cylvec{
			float r;
			float phi;
			float z;
		}CylVec;
		typedef struct tofcell{
			short trayid;
			short channel;// = nModule*6+nCell
			float triggertime;
			float tdc;
			float tot;
			float tof;
		}TofCell;
		typedef struct TofSend{
			int     nHits;
			TofCell cell[MAX_TOF_HITS];
		}TofEvent;
		TofEvent * tofhits;
		TofEvent * pvpdhits;

		int    readFromTofMachine(char *trgSend);
		int    matchPrimaryTracks(gl3Node *node,l3xyzCoordinate vertex);
		int    matchGlobalTracks(gl3Node *node);
		bool   projTray(gl3Track* track, int& trayId, double &extrapolateZ);
		int    projModuleCell(gl3Track* track, int& trayId, int& moduleId, int &cellId, double &extrapolateZ, CylVec &cross, PointVec &local);
		int    projTrayModuleCell(gl3Track* track, CellVec& cellv, PointVec &local);
		int    HelixCross(gl3Track* track, CellVec & cellv, CylVec& cross, PointVec& local);
		int    readzCali(char* zCali_4DB = "zCali_4DB.dat");
		double zCali(CellVec& cellId, double z);
		float  linearInter(float x1, float x2, float y1, float y2, float x);
		int    readFromTrg(char* mem, int& bytes);
		void   GetModuleRN(CellVec &cellv, double *Rxyz, double *Nxyz);
		int    checkvz(gl3Track* track);
		int    CalcvzVpd();
		int    Tstart(float& vzTpc);
		int    Tstart_NoVpd(int nglobal, gl3Node *nodes, l3xyzCoordinate vertex, gl3Bischel* bischel, double nsigma1);
		void   Global2Local(CellVec &cell, double *HitPosition, PointVec &local);

                gl3Bischel* bischel;

		int   evt_counter;

		float tdiffCut;//maximum vpdtime
		float vzVpd;//vpd vertex z
		float T0;//vpd start time
	private:
		static const int maxModuleHits = 20;
		int   nCellhits[nTray][nModule];//list for valid cell hits
		int   Cellhit[nTray][nModule][maxModuleHits];
		float tofZEdge[nTray][nBoard][maxNZBin+1];
		float tofZCorr[nTray][nBoard][maxNZBin+1];
		float mTrayX0[nTray];// alignment of tray
		float mTrayY0[nTray];
		float mTrayZ0[nTray];
		float mTrayPhi[nTray];// each tray phi center value
		float mModuleAngle[nModule];// module lean angle 
		float mModuleLocalZ[nModule];// along z direction
		float mModuleLocalX[nModule];// along r direction
		float mModuleEtaMin[nTray][nModule];
		float mModuleEtaMax[nTray][nModule];
		float mTofRmin;// TOF inner radius (cm)
		float mTraycoverphi;// one Tray cover phi
		float mModuleWidth;// Module width (cm)
		float mPadWidth;// pad width
		float vzOffset;//12.29;//12.4-0.11; //11.86+0.54; //cm
		float c_light;// light speed :cm/ns 
		int   imat;//matched hit index
		int   projchannel;//project channel
		PointVec local;
		int itof;
		int ipvpd;

};
#endif
