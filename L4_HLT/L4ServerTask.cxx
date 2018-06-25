#include "L4ServerTask.h"
#include <iostream>
using namespace std;

L4ServerTask::L4ServerTask() {
    l4Server = NULL;
    // default parameterDir
    // using L4ServerTask::setParameterDir()
    // to change setting
    sprintf(HLTParameterDir, "%s/","/a/hltconfig");
}

L4ServerTask::~L4ServerTask() {
    delete l4Server;
}

int  L4ServerTask::configureRun(SimpleXmlDoc *cfg) {
    LOG(NOTE,"-------------L4ServerTask start!!!--------------");
    if(l4Server) delete l4Server;
    l4Server = new L4_Server(HLTParameterDir);
    l4Server->printInfo();
    l4Server->run_start();
    createThread();
    return 0;
}

int L4ServerTask::startRun() {
    return 0;
}

int L4ServerTask::stopRun() {
    int ret = killThread();
    l4Server->run_stop();
    LOG(NOTE,"-------------L4ServerTask stop !!!--------------");
    return ret;
}

void L4ServerTask::restart() {
    // this function will be called at a flushtoken commender
    // in case the run is not start or stop because some critical err
    killThread();
    delete l4Server;
    LOG(WARN,"hltCaliServer is killed because a flushtoken signal");
    exit(1);
}

void L4ServerTask::setParameterDir(char* parameterDir)
{
    if (strlen(parameterDir) > maxParaLength) {
        LOG(ERR, "HLT Parameter dir Path length is too long");
    }

    strcpy(HLTParameterDir, parameterDir);
}

// thread function receive a point to L4ServerTask
// this should not be a member function of L4ServerTask 
void* networkThreadFunc(void* _L4ServerTask) {
    // int oldtype;
    // set the thread to be canceled at cancel point 
    // rather than canceled immediately after receive the cancel signal 
    //pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,&oldtype);

    L4ServerTask *l4 = (L4ServerTask *) _L4ServerTask;
    while(1){
        // set the cancel point
        // only enable cancellation just before the cancel point is set
        //pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
        //pthread_testcancel();
        //pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
        l4->l4Server->run();
    }

    return NULL;
}

int L4ServerTask::createThread() {
	int ret = pthread_create(&id,NULL,networkThreadFunc,this);
	if(ret != 0){
		LOG(ERR,"Create pthread error !");
		return ret;
	}
	return 0;
}

int L4ServerTask::killThread() {
    int ret = pthread_cancel(id);
    if(ret !=0 ){
       LOG(ERR,"Can't cancel pthread !");
		 return ret;
    }
    else {
        // wait for the thread to be canceled 
        // the thread will be canceled at where the pthread_testcancel() call
        ret = pthread_join(id,NULL); 
        if( ret !=0 ) {
            LOG(ERR,"pthread is not terminated properly! ");
            return ret;
        }
    }
    return 0;
}
