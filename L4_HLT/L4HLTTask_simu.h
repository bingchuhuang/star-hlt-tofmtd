#ifndef L4HLTTASK_SIMU_H
#define L4HLTTASK_SIMU_H

#include "L4_SUPPORT/l4Interface.h"

#ifdef USE_COI
#include <intel-coi/source/COIProcess_source.h>
#include <intel-coi/source/COIEngine_source.h>
#include <intel-coi/source/COIPipeline_source.h>
#include <intel-coi/source/COIEvent_source.h>
#endif // USE_COI

class L4HLTTask_simu : public L4Interface_simu {
public:
    L4HLTTask_simu(int myidx) : L4Interface_simu(myidx) {}

#ifdef USE_COI
    ~L4HLTTask_simu();
    void init();
    int sendConfig(SimpleXmlDoc* cfg);
    int startRun();
    int stopRun();
    void process(int task_id, L4EventData* evt);
#else
    void process(int task_id, L4EventData* evt) {}
#endif // USE_COI

private:

#ifdef USE_COI
    COIPROCESS              proc;
    COIENGINE               engine;
    COIFUNCTION             func[1];
    COIEVENT                completion_event;
    COIPIPELINE             pipeline;
    uint32_t                num_engines = 0;
    const char*             SINK_NAME = "/RTS/exe/l4_sl6/bin/LINUX/x86_64/coi_simple_sink_mic";
#endif // USE_COI

};

#endif /* L4HLTTASK_SIMU_H */
