#ifndef EMC_CALIBRATION_H
#define EMC_CALIBRATION_H

#include <iostream>
//#include <fstream>

using namespace std;

class l3EmcTowerInfo
{
public:
    l3EmcTowerInfo();

    inline float getPhi() { return phi; };
    inline float getEta() { return eta; };
    inline float getEtaMin() { return etaMin; };
    inline float getEtaMax() { return etaMax; };
    inline float getZ() { return z; };
    inline float getZmin() { return zMin; };
    inline float getZmax() { return zMax; };
    inline float getPedestal() { return pedestal; };
    inline float getGain() { return gain; };
    inline int getSoftID() { return softId; };
    inline int getDaqID() { return daqId; };
    inline int getCrate() { return crate; };
    inline int getCrateSeq() { return crateSeq; };
    inline int getPhiEta() { return phiEta; };

    inline void setPhi(float _phi) { phi = _phi; };
    inline void setEta(float _eta) { eta = _eta; };
    inline void setEtaMin(float _etaMin) { etaMin = _etaMin; };
    inline void setEtaMax(float _etaMax) { etaMax = _etaMax; };
    inline void setZ(float _z) { z = _z; };
    inline void setZMin(float _zMin) { zMin = _zMin; };
    inline void setZMax(float _zMax) { zMax = _zMax; };
    inline void setPedestal(float _pedestal) {  pedestal= _pedestal; };
    inline void setGain(float _gain) { gain = _gain; };
    inline void setID(int _softId) { softId = _softId; } ;
    inline void setDaqID(int _daqId) { daqId = _daqId; };
    inline void setCrate(int _crate) { crate = _crate; };
    inline void setCrateSeq(int _crateSeq) { crateSeq = _crateSeq; };
    inline void setPhiEta(int _phiEta) { phiEta = _phiEta; };

    inline void set(float _phi, float _eta, float _etaMin, float _etaMax, float _z, float _zMin, float _zMax,
		    float _pedestal, float _gain, int _id, int _daqID, int _crate, int _crateSeq, int _phiEta) {
	setPhi(_phi);
	setEta(_eta);
	setEtaMin(_etaMin);
	setEtaMax(_etaMax);
	setZ(_z);
	setZMin(_zMin);
	setZMax(_zMax);
	setPedestal(_pedestal);
	setGain(_gain);
	setID(_id);
	setDaqID(_daqID);
	setCrate(_crate);
	setCrateSeq(_crateSeq);
	setPhiEta(_phiEta);
    }

private:
    float phi; 
    float eta;
    float etaMin;
    float etaMax;
    float z;
    float zMin;
    float zMax;
    float pedestal;
    float gain;
    int softId;
    int daqId;
    int crate;
    int crateSeq;
    int phiEta;
};


// The class l3EmcCalibration translates from DAQ tower IDs to other 
// tower information 

class l3EmcCalibration
{
public:
    l3EmcCalibration(int nTow);
    ~l3EmcCalibration();

    int loadMap(const char *filename);
    int loadTextMap(const char *filename);
    int saveTextMap(const char *filename);
    int readL2Pedestal(const char* filename);
    
    inline l3EmcTowerInfo *getTowerInfo(int softId) {
	return &(tower[softId-1]);
    }

    inline l3EmcTowerInfo *getTowerInfo(int crate, int crateSeq) {
	return &(tower[daqToId(crateToDaqId[crate-1][crateSeq])-1]);
    }

    inline l3EmcTowerInfo *getTowerInfoFromPhiEta(int phiEta) {
	return &(tower[phiEtaToSoftId[phiEta]-1]);
    }

    inline int daqToId(int daq) { if(daq > nTowers) return 0; else return daq2id[daq]; }

    inline int getNTowers() { return nTowers; }

private: 
    
    int nTowers;
    l3EmcTowerInfo *tower;

    int *daq2id;
    int crateToDaqId[30][160];
    int phiEtaToSoftId[4800];

    struct colDef_t {

	void set(int _nCols, int _daqId, int _softId, int _crate, int _crateSeq, int _phi, 
	    int _eta, int _etaMin, int _etaMax, int _z, int _zMin, int _zMax, int _pedestal, int _gain, int _phiEta) {

	    nCols  = _nCols;
	    daqId  = _daqId;
	    softId = _softId;
	    crate  = _crate;
	    crateSeq = _crateSeq;
	    phi    = _phi;
	    eta    = _eta;
	    etaMin = _etaMin;
	    etaMax = _etaMax;
	    z      = _z;
	    zMin   = _zMin;
	    zMax   = _zMax;
	    pedestal = _pedestal;
	    gain   = _gain;
	    phiEta = _phiEta;
	}


      int nCols;
      
      int daqId;
      int softId;
      int crate;
      int crateSeq;
      int phi;
      int eta;
      int etaMin;
      int etaMax;
      int z;
      int zMin;
      int zMax;
      
      int pedestal;
      int gain;
      int phiEta;
    };

    int readCalib(ifstream *from, colDef_t colDef);
    
};



#endif
