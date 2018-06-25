#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <DAQ_READER/daqReader.h>
#include <XMLCFG/SimpleXmlDoc.h>
#include <rtsLog.h>
#include <SFS/sfs_index.h>
#include <SFS/sfs_base.h>
#include <iccp2k.h>

#include "l4Emulator.h"

#include "L4HLTTask_cpu.h"
#include "L4HLTTask_simu.h"
#include "L4HLTTask_decider.h"

void Emulator::showArgs() {
  LOG(ERR, "-D [DBG/NOTE/WARN/OPER/INFO/CRIT]");
  LOG(ERR, "-file [input file]");
  LOG(ERR, "-out [output file]");
  LOG(ERR, "-conf [config file]");
  LOG(ERR, "-para [HLT parameter dir]");
  LOG(ERR, "-max [max evts]");
}

int Emulator::configureServer(int argc, char *argv[]) {
    rtsLogOutput(RTS_LOG_STDERR);
    rtsLogLevel(WARN);
  
    for(int i=1;i<argc;i++) {
	if(strcmp(argv[i], "-D") == 0) {
	    i++;
	    rtsLogLevel(argv[i]);
	}
	else if(strcmp(argv[i], "-file") == 0) {
	    i++;
	    filename = argv[i];
	}
	else if(strcmp(argv[i], "-out") == 0) {
	    i++;
	    outfilename = argv[i];
	}
	else if(strcmp(argv[i], "-conf") == 0) {
	    i++;
	    conffile = argv[i];
	}
	else if(strcmp(argv[i], "-para") == 0) {
	    i++;
	    paraDir = argv[i];
	}
	else if(strcmp(argv[i], "-max") == 0) {
	    i++;
	    max = atoi(argv[i]);
	}
	else {
	    LOG(ERR,"Invalid parameter: %s", argv[i]);
	    showArgs();
	    return -1;
	}
    }
    
    if(filename == NULL) {
	showArgs();
	return -1;
    }

    if (paraDir) {
        static_cast<L4HLTTask_cpu*>(l4cpu)->setParameterDir(paraDir);
    }
    
    LOG(DBG, "here");
      
    return 0;
}


void Emulator::run() {

    LOG(DBG, "here");

    threadDesc = new ThreadDesc("l4Emu");
    
    L4EventData *eventdata;
    eventdata = (L4EventData *)malloc(sizeof(L4EventData));
    if(!eventdata) {
	LOG(ERR, "Bad malloc");
	exit(0);
    }


    LOG(DBG, "here");

    SimpleXmlDoc cfg(5*(1<<20)); // use archived xml file needs larger buffer, 3MB in this case
    // First configure the run!
    if(conffile) {
	cfg.open(conffile);
    }

    l4cpu->threadDesc = threadDesc;
    l4simu->threadDesc = threadDesc;
    l4decider->threadDesc = threadDesc;
    
    LOG(DBG, "here");
    l4cpu->init();
    l4simu->init();
    l4decider->init();
    l4cpu->sendConfig(&cfg);
    l4simu->sendConfig(&cfg);
    l4decider->sendConfig(&cfg);
    l4cpu->startRun();
    l4simu->startRun();
    l4decider->startRun();

    LOG(DBG, "here");
    // Setup the file, and go through events
    LOG(NOTE,"Filename: %s",filename);
    rdr = new daqReader(filename);
    rdr->setCopyOnWriteMapping();   // allow writes that do not change the original file!

    LOG(DBG, "here");
    int good=0;

    int ofd = -1;
    if(outfilename) {
	ofd = open(outfilename, O_WRONLY | O_CREAT, 0666);
	if(ofd < 0) {
	    LOG(ERR, "Error opening output filame: %s (%s)",outfilename,strerror(errno));
	    return;
	}
    }

    LOG(DBG, "here");

    int nTrgTaken[64];
    int nTrgAccepted[64];
    memset(nTrgTaken,0,sizeof(nTrgTaken));
    memset(nTrgAccepted,0,sizeof(nTrgAccepted));

    int done = 0;
    for(;;) {
	char *ret = rdr->get(0,EVP_TYPE_ANY);
	if(!ret || rdr->status) {
	    switch(rdr->status) {
	    case EVP_STAT_OK:
		continue;
	    case EVP_STAT_EOR:
		LOG(NOTE,"Run finished!  exiting...\n");
		done = 1;
		break;
	    default:
		LOG(NOTE,"Bad Event!  exiting...\n");
		return;
	    }
	}

	if(done) break;

	good++;
	
	if(good > max) break;

	LOG(NOTE,"evt %d: sequence %d: token %4d, trgcmd %d, daqcmd %d, time %u, detectors 0x%08X (status 0x%X)",
	    good,rdr->seq, rdr->token, rdr->trgcmd, rdr->daqcmd,rdr->evt_time, rdr->detectors, rdr->status);

        LOG(NOTE, "l1 = 0x%llx  l2= 0x%llx",rdr->daqbits64_l1,rdr->daqbits64_l2);
	// Handle the event
	
	LOG(DBG, "%p",rdr->memmap->mem);

	int event_sz = rdr->event_size;
        if (event_sz > 15000000) {
            LOG(INFO, "skip event, size: ", event_sz);
            continue;
        }
        // LOG(INFO, "$$$ input event size = %9d", event_sz);
	// Set up the raw data
	memset(eventdata, 0, sizeof(L4EventData));
	memcpy(eventdata->raw_input_event, rdr->memmap->mem, event_sz);

	l4cpu->process(rdr, eventdata);
	for(int i=0;i<L4_SIMU_TASKS;i++) {
	    l4simu->process(i,eventdata);
	}
	l4cpu->finish(eventdata);
	UINT64 trgs = l4decider->decide(eventdata);

	//UINT64 trgs = l4->handleEvent(rdr, blobs_out);

	if(ofd >= 0) {
	    //rtsLogLevel(DBG);
	    LOG(DBG, "write outputfile %p",rdr->memmap->mem);
	    fileUtil.writeOutputFile(ofd, rdr, eventdata->output, trgs);
	}

	for(int i=0;i<64;i++) {
    
	    if(rdr->daqbits64_l2 & (1LL << i)) {
		nTrgTaken[i]++;
	    }
	    if(trgs & (1LL << i)) {
		nTrgAccepted[i]++;
	    }
	}
    } 

    l4cpu->stopRun();
    l4simu->stopRun();
    l4decider->stopRun();

    for(int i=0;i<64;i++) {
	if(nTrgTaken[i] > 0) {
	    LOG(INFO, "trg[%2d] - accepted l2 = %-6d accepted hlt = %-6d",i, nTrgTaken[i], nTrgAccepted[i]);
	}
    }
}

