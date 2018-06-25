#ifndef L4HLTTASK_CPU_H
#define L4HLTTASK_CPU_H

#include "L4_SUPPORT/l4Interface.h"
#include "hlt_entity.h"
#include "gl3TriggerDecider.h"

class SimpleXmlDoc;
class hlt_trg;
class hlt_btow; 
class hlt_tof;
class hlt_mtd;
class hlt_tpc_ca;
class hlt_gl3;

class L4HLTTask_cpu : public L4Interface_cpu {
public:
    L4HLTTask_cpu(int myidx);
    ~L4HLTTask_cpu();

    void init();
    int sendConfig(SimpleXmlDoc* cfg);
    int startRun();
    int stopRun();

    void process(daqReader* rdr, L4EventData* evt);
    void finish(L4EventData* evt);

    void setParameterDir(char* parameterDir = NULL) { HLTParameterDir = parameterDir; }
    void setUseTrigger(bool use) { use_trg    = use; }
    void setUseBTow(bool use)    { use_btow   = use; }
    void setUseTof(bool use)     { use_tof    = use; }
    void setUseTpcCa(bool use)   { use_tpc_ca = use; }
    void setUseGl3(bool use)     { use_gl3    = use; }

private:
    char*        HLTParameterDir;
    int          eventCounter;
    hlt_config_t hlt_cfg;
    UINT64       l3AlgoNeededMask[gl3TriggerDecider::MaxNHLTTriggers+1]; // hlt trigger id begin at 1 -- yiguo
    
    hlt_trg*    trg;
    hlt_btow*   btow;
    hlt_tof*    tof;
    hlt_mtd*    mtd;
    hlt_tpc_ca* tpc_ca;
    hlt_gl3*    gl3;
    
    bool use_gl3;
    bool use_trg;
    bool use_btow;
    bool use_tof;
    bool use_mtd;
    bool use_tpc_ca;
    
    void set_hlt_blob(hlt_blob& blob, char* buff, int sz, char* name) {
        blob.buff = buff;
        blob.bytes = sz;
        blob.name = name;
    }
};

#endif /* L4HLTTASK_CPU_H */
