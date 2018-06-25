#include "l3EmcCalibration.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <string>
#include <math.h>
#include <rtsLog.h>
//#include "l3Log.h"

using namespace std;


l3EmcTowerInfo::l3EmcTowerInfo() {
    set(0., 0., 0., 0., 0., 0., 0., 0., 1.0, -1, -1, -1, -1, -1);
}


l3EmcCalibration::l3EmcCalibration(int nTow) {
    nTowers = nTow;

    tower = new l3EmcTowerInfo[nTowers];
    daq2id = new int[nTowers];
}

l3EmcCalibration::~l3EmcCalibration() {
    delete[] tower;
    delete[] daq2id;

}

int l3EmcCalibration::loadMap(const char* filename) 
{

    LOG(ERR, "Binary maps are no longer supported. Where did you find one?",0,0,0,0,0);

    return -1;

}


//#define OLDLOAD
#ifdef OLDLOAD
int l3EmcCalibration::loadTextMap(const char* filename) {
    
    ifstream txtmap(filename);

    string s;

    enum colDesc { col_phi, col_eta, 
		   col_pedestal, col_threshold, col_gain, 
		   col_id, col_daqId, col_ignore };

    

    txtmap >> s;
    if (s != "EmcCalib") {
      LOG(ERR, "No EMC calibration map in text format in %s\n", filename,0,0,0,0);
	return -1;
    }

    txtmap >> s;
    if (s != "columns:") {
      LOG(ERR,"No EMC calibration map in text format in %s\n", filename,0,0,0,0);
	return -1;
    } 
    
    int nCols;
    txtmap >> nCols;
    //l3Log("EMC calibration map with %d cols\n", nCols);

    colDesc *colTarget = new colDesc[nCols];

    // This should be determined by parsing the header
    colTarget[0] = col_id;
    colTarget[1] = col_daqId;
    colTarget[2] = col_phi;
    colTarget[3] = col_eta;
    colTarget[4] = col_pedestal;
    colTarget[5] = col_gain;
    colTarget[6] = col_threshold;


    for (int i=0; i<nTowers; i++) {

	float phi=0.;
	float eta=0.;
	float pedestal=0.;
	float gain=1.0;
	int id=-1;
	int daqId=-1;
	float threshold=0;

	float ignore;

	for (int col=0; col<nCols; col++) {

	    switch (colTarget[col]) {
		
	    case col_phi:   
		txtmap >>  phi ;
		break;

	    case col_eta:   
		txtmap >> eta ;
		break;

	    case col_pedestal:   
		txtmap >> pedestal ;
		break;

	    case col_gain:  
		txtmap >> gain ;
		break;

	    case col_threshold: 
		txtmap >> threshold ;
		break;

	    case col_id:    
		txtmap >> id ;
		id--;
		break;

	    case col_daqId: 
		txtmap >> daqId ;
		break;

	    case col_ignore: 
		txtmap >> ignore ;
		break;

	    }
	}
	
//  	cout << "  " << phi << "  " << eta << "  " 
//  	     << pedestal << "  " << threshold << "  " << gain << "  " 
//  	     << id << "  " << daqId << endl;

	if ( (id < 0) || (id >= 4800) ) {
	  LOG(ERR, "%s contains info for tower %i!!!\n",filename,id,0,0,0);
	    return -1;
	}


	tower[id].set(phi, eta, pedestal, gain, id, daqId);
	daq2id[daqId] = id;
    }

    return 0;
}
#else 

int l3EmcCalibration::loadTextMap(const char* filename) 
{
    //l3Log("Reading %s", filename);

  //  LOG(NOTE, "Loading calibration map %s",filename,0,0,0,0);

  //#ifdef JEFFDUMB

    string s;
    
    ifstream txtmap(filename);

    string type;
    txtmap >> type;

    txtmap >> s;
    if (s != "columns:") {
      LOG(ERR, "No EMC calibration found in %s", filename,0,0,0,0);
	return -1;
    } 
    
    colDef_t colDef; 
    colDef.set(0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1);

    //int nCols;
    txtmap >> colDef.nCols;
    txtmap >> s;   
    if (s != "format:") return -1;
    
    for (int i=0; i<colDef.nCols; i++) {
      txtmap >> s;
      
      if (s=="daqId")   colDef.daqId = i;
      if (s=="softId")  colDef.softId = i;
      if (s=="crate")   colDef.crate = i;
      if (s=="crateSeq")colDef.crateSeq = i;
      if (s=="phi")     colDef.phi = i;
      if (s=="eta")     colDef.eta = i;
      if (s=="etaMin")  colDef.etaMin = i;
      if (s=="etaMax")  colDef.etaMax = i;
      if (s=="z")       colDef.z = i;
      if (s=="zMin")    colDef.zMin = i;
      if (s=="zMax")    colDef.zMax = i;
      if (s=="pedestal")colDef.pedestal = i;
      if (s=="gain")    colDef.gain = i;
      if (s=="phiEta")  colDef.phiEta = i;
    }
    
	

    int nRead = readCalib(&txtmap, colDef);


    if(nRead == nTowers) 
	return 0;
    else 
	return -1;
    //#endif
    //return 0;
}

#endif

int l3EmcCalibration::readCalib(ifstream *from, colDef_t colDef)
{

    int nTwr = 0;
    //while(1) {
    for (int t=0; t<nTowers; t++)  {
        float phi=0.0, eta=0.0, etaMin=-999.0, etaMax=-999.0, z=-999.0, zMin=-999.0, zMax=-999.0;
        float pedestal=0.0, gain=1.0;
        int   softId=-1, daqId=-1, crate=-1, crateSeq=-1, phiEta=-1;
        
	string dummy;

	//l3Log("asd %d\n", colDef.nCols);

        for (int i=0; i<colDef.nCols; i++) {
 	    if (i == colDef.softId)   *from >> softId;   else
 	    if (i == colDef.daqId)    *from >> daqId;    else
 	    if (i == colDef.crate)    *from >> crate;    else
 	    if (i == colDef.crateSeq) *from >> crateSeq; else
 	    if (i == colDef.eta)      *from >> eta;      else
 	    if (i == colDef.etaMin)   *from >> etaMin;   else
 	    if (i == colDef.etaMax)   *from >> etaMax;   else
	    if (i == colDef.z)        *from >> z;        else
	    if (i == colDef.zMin)     *from >> zMin;     else
	    if (i == colDef.zMax)     *from >> zMax;     else
 	    if (i == colDef.phi)      *from >> phi;      else
 	    if (i == colDef.pedestal) *from >> pedestal; else
 	    if (i == colDef.gain)     *from >> gain;     else
 	    if (i == colDef.phiEta)   *from >> phiEta;   else
 	    *from >> dummy;
        }


	if(from->eof()) break;

	// BTOW maps do not have etamin/max, so let's calculate it
	if (nTowers == 4800) {
	    if(etaMin == -999.) 
		etaMin = floor(eta*20.)/20.;

	    if(etaMax == -999.) 
		etaMax = ceil(eta*20.)/20.;
	}

	tower[softId-1].set(phi, eta, etaMin, etaMax, z, zMin, zMax, pedestal, gain, softId, daqId, crate, crateSeq, phiEta);
	daq2id[daqId] = softId;
	crateToDaqId[crate-1][crateSeq] = daqId;
	phiEtaToSoftId[phiEta] = softId;
        nTwr++;
    }
    

    return nTwr;
} 





int l3EmcCalibration::saveTextMap(const char* filename) 
{
    ofstream txtmap(filename);

    txtmap << "EmcCalib" << endl
	   << "columns: 7" << endl;

    for (int i=0; i<nTowers; i++) {
	txtmap << tower[i].getSoftID() << " "
	       << tower[i].getDaqID() << " "
	       << tower[i].getPhi() << " "
	       << tower[i].getEta() << " "
	       << tower[i].getPedestal() << " "
	       << tower[i].getGain() << " "
	       << 0.0 << endl;
    }

    return 0;
}

int l3EmcCalibration::readL2Pedestal(const char* filename)
{
  const int BTOW=4800;
  float L2pedestal[BTOW];   // use daqID
  memset(L2pedestal,0,sizeof(L2pedestal));
 
  const int mx=1000;
  char buf[mx];
  FILE *fd=fopen(filename,"r");
  if (!fd) {
      LOG(ERR, "No EMC pedestal file found in %s", filename,0,0,0,0);
	return -1;
    } 
  for(int i=0;i<731;i++)
    fgets(buf,mx,fd);
  char cVal[4][100];
  float ped,Sigped;
  int id;
  for(int i=731;i<731+BTOW;i++){
    fgets(buf,mx,fd);
	if(buf[0]=='#') continue;
        int n=sscanf(buf,"%s %f %f %s %s %s %d",
                     cVal[0],&ped,&Sigped,cVal[1],cVal[2],cVal[3],&id);
	if(n!=7) continue;
	if(!(id>=0&&id<=BTOW-1)) {cout<<"id err!  id="<<id<<endl; continue;}
	L2pedestal[id]=ped;
  }

  for(int i=0;i<BTOW;i++)
    {
      for(int j=0; j<BTOW; j++)
	if(i == tower[j].getDaqID()) 
	  {
	    tower[j].setPedestal(L2pedestal[i]);
	    break;
	  }
    }

  fclose(fd);
  return 1;
}
