#ifndef L4HLTTASK_DECIDER_H
#define L4HLTTASK_DECIDER_H

#include "L4_SUPPORT/l4Interface.h"
#include "hltNetworkDataStruct.h"

#include <pthread.h>

class L4HLTTask_decider : public L4Interface_decider {
public:
    L4HLTTask_decider();
    ~L4HLTTask_decider();
    
    void init();
    int sendConfig(SimpleXmlDoc* cfg);
    int startRun();
    int stopRun();

    UINT64 decide(L4EventData *evt);

    int createCaliThread();
    int killCaliThread();

private:
    pthread_t caliThread;
};

#endif /* L4HLTTASK_DECIDER_H */
