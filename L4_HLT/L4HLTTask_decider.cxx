#include "L4HLTTask_decider.h"
#include "L4EventDataTypes.h"
#include "hltCaliClient.h"
#include <rtsLog.h>
#include <pthread.h>

// -----------------------------------------------------------------------------
void* threadFuncCaliReceive(void* p)
{
    ThreadDesc *threadDesc = new ThreadDesc("hltCaliClient");

    while (1) {
        TMCP;
        hltCaliClient::GetInstance()->RecvCaliResult();
    }

    return NULL;
}

// -----------------------------------------------------------------------------
L4HLTTask_decider::L4HLTTask_decider() : L4Interface_decider() {}

// -----------------------------------------------------------------------------
L4HLTTask_decider::~L4HLTTask_decider() { }

// -----------------------------------------------------------------------------
void L4HLTTask_decider::init()
{
    createCaliThread();
}

// -----------------------------------------------------------------------------
int L4HLTTask_decider::sendConfig(SimpleXmlDoc* cfg)
{
    TMCP;
    // ask calibration server for initial calibration valutes
    hltCaliClient::GetInstance()->SendInitRequest();

    return 0;
}

// -----------------------------------------------------------------------------
int L4HLTTask_decider::startRun()
{
    TMCP;
    return 0;
}

// -----------------------------------------------------------------------------
int L4HLTTask_decider::stopRun()
{
    TMCP;
    return 0;
}    

// -----------------------------------------------------------------------------
UINT64 L4HLTTask_decider::decide(L4EventData *evt)
{
    TMCP;
    L4FinishData_t& finishData = *(reinterpret_cast<L4FinishData_t*>(evt->finish_result));

    TMCP;
    memset(evt->decide_result, 0, sizeof(evt->decide_result));

    TMCP;
    L4DeciderData_t& dcdData = *(new(evt->decide_result)L4DeciderData_t);
    // L4DeciderData_t& dcdData = *((L4DeciderData_t*)evt->decide_result);
    dcdData.l3TrgMask = finishData.l3TrgMask;

    TMCP;
    hltCaliClient::GetInstance()->SendCaliData(&finishData.caliData);
    
    TMCP;
    LOG(NOTE, "L4 decider: l3TrgMask = 0x%llX, finishData.l3TrgMask = 0x%llX 0x%llX",
        dcdData.l3TrgMask, finishData.l3TrgMask, ((L4FinishData_t*)evt->finish_result)->l3TrgMask);
    return dcdData.l3TrgMask;
}

// -----------------------------------------------------------------------------
int L4HLTTask_decider::createCaliThread()
{
    int ret = pthread_create(&caliThread, NULL, threadFuncCaliReceive, NULL);
    if (ret) {
        LOG(ERR, "L4HLTTask_decider::createCaliThread() failed");
    }

    return ret;
}

// -----------------------------------------------------------------------------
int L4HLTTask_decider::killCaliThread()
{
    int ret = pthread_cancel(caliThread);
    if (ret) {
        LOG(ERR, "L4HLTTask_decider::killCaliThread() failed");
    }

    return ret;
}
