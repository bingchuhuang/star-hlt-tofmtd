#ifndef _L4HLTTASK_H_
#define _L4HLTTASK_H_

#include "L4_SUPPORT/l4Interface.h"

#include "hlt_entity.h"
#include "profiler.hh"
#include "gl3TriggerDecider.h"

class SimpleXmlDoc;

//#define TIMER_USE
//#ifdef TIMER_USE
//
//#define TIMER_DEFINE			class profiler l4Timer 
//#define TIMER_DECLARE		  extern class profiler l4Timer 
//
//#define TIMER_START() 		l4Timer.start_evt(__LINE__, __FILE__) 
//
//#define TIMER_DUMP()			l4Timer.dump()
//
//#define TIMER(X)  		    l4Timer.mark(__LINE__, __FILE__,(X))
//
//#define TIMER_MHZ(X)		  l4Timer.mhz((X)) ;
//
//#endif


class hlt_trg;
class hlt_btow; 
class hlt_tof;
class hlt_mtd;
class hlt_tpc_ca;
class hlt_gl3;

//TIMER_DEFINE;

// *****L4HLTTask*******************************
// * L4HLTTask is the top level hlt code
// * use the interface defined by L4Interface
// ***********************************************
class L4HLTTask : public L4Interface {
public:
    // From interface...
    int    configureRun(SimpleXmlDoc *cfg);
    UINT64 handleEvent(daqReader *rdr, hlt_blob *blobs_out);
    int    startRun();
    int    stopRun();

    L4HLTTask();
    ~L4HLTTask();
    int starEvent();
    int stopEvent();

    void setParameterDir(char* parameterDir = NULL) { HLTParameterDir = parameterDir; }
    void setUseTrigger(bool use) { use_trg    = use; }
    void setUseBTow(bool use)    { use_btow   = use; }
    void setUseTof(bool use)     { use_tof    = use; }
    void setUseTpcCa(bool use)   { use_tpc_ca = use; }
    void setUseGl3(bool use)     { use_gl3    = use; }


private:
    UINT64 l3AlgoNeededMask[gl3TriggerDecider::MaxNHLTTriggers+1]; // hlt trigger id begin at 1 -- yiguo

    char* HLTParameterDir;
    
    hlt_config_t hlt_cfg;
    int runnumber;
    int eventCounter;
		//int nTriggered;
    
    hlt_trg*    trg;
    hlt_btow*   btow;
    hlt_tof*    tof;
    hlt_mtd*     mtd;
    hlt_tpc_ca* tpc_ca;
    hlt_gl3*    gl3;

    bool use_gl3;
    bool use_trg;
    bool use_btow;
    bool use_tof;
    bool use_mtd;
    bool use_tpc_ca;
    
    // pointers to output memary
    // gl3 blobs is defined outside, coz its final output
    hlt_blob trg_blob;
    hlt_blob btow_blob;
    hlt_blob tof_blob;
    hlt_blob mtd_blob;
    hlt_blob tpc_ca_blob;
};

#endif
