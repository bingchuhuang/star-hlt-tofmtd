#ifndef _L4SERVERTASK_H_
#define _L4SERVERTASK_H_ 

// #include "L4_SUPPORT/l4Interface.h"
#include "L4_Server.h"
#include "SUNRT/runControlClass.h"

#include <rtsLog.h>

#include <pthread.h>
#include <stdio.h>
#include <string.h>


class SimpleXmlDoc;

class L4ServerTask: public RunControlClass {
public:
    // from interface
    //int configureRun(SimpleXmlDoc *cfg);
    // to keep the framework?
    int  configureRun(SimpleXmlDoc *cfg);
    //UINT64 handleEvent(daqReader *rdr, hlt_blob *blobs_out) { return 0; }
    int startRun();
    int stopRun();
    void restart();

    void setParameterDir(char* parameterDir = NULL);
    int  createThread();
    int  killThread();

    L4ServerTask();
    ~L4ServerTask();

    L4_Server *l4Server;

private:
    static const size_t maxParaLength = 256;
    char HLTParameterDir[maxParaLength];
    pthread_t id;
};

#endif
