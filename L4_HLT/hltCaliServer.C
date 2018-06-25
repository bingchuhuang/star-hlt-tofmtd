#include "rtsLog.h"
#include "L4ServerTask.h"
#include <stdio.h>
#include "profiler.hh"

//#define CLOCK_DEFINE			class profiler clockCounter 
//#define CLOCK_DECLARE		  extern class profiler clockCounter 
//
//#define CLOCK_START() 		clockCounter.start_evt(__LINE__, __FILE__) 
//
//#define CLOCK_DUMP()			clockCounter.dump()
//
//#define CLOCK(X)  		    clockCounter.mark(__LINE__, __FILE__,(X))
//
//#define CLOCK_MHZ(X)		  clockCounter.mhz((X)) ;


//void* run(void* _L4_server)
//{
//	L4_Server *server = (L4_Server *)_L4_server;
//	server->run();

//	return NULL;
//}

int main(int argc, char *argv[])
{
    //CLOCK_DEFINE;
    char* paramDir = "./";
#ifndef L4STANDALONE
    rtsLogAddDest(RTS_DAQMAN, RTS_LOG_PORT_L3);
#else
    rtsLogOutput(RTS_LOG_STDERR);
    rtsLogLevel(NOTE);
    
    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-para") == 0) {
            i++;
            paramDir = argv[i];
        }else if(strcmp(argv[i], "-D") == 0) {
            i++;
            rtsLogLevel(argv[i]);
        } else {
            LOG(ERR, "Invalid parameter: %s", argv[i]);
            return -1;
        }
    }

    printf("%s\n", paramDir);
#endif

    L4ServerTask *l4 = new L4ServerTask;

    //l4->paramDir = paramDir;

#ifndef L4STANDALONE
    l4->run(1, 0xbdf8);
#else
    l4->setParameterDir(paramDir);
    l4->configureRun(NULL);
    l4->startRun();

    //CLOCK_START();
    cout<<"count down"<<endl;
    for(int i=0;i<2;i++){
        //CLOCK(0);
    	sleep(10);
        cout<<"=============="<<20-i<<"min============"<<endl;
        //CLOCK(1);
    }

    //CLOCK_DUMP();
    l4->stopRun();

    // second run
    l4->configureRun(NULL);
    l4->startRun();

    //CLOCK_START();
    cout<<"count down"<<endl;
    for(int i=0;i<2;i++){
        //CLOCK(0);
    	sleep(50);
        cout<<"=============="<<20-i<<"min============"<<endl;
        //CLOCK(1);
    }

    //CLOCK_DUMP();
    l4->stopRun();

#endif
    return 0;
}
