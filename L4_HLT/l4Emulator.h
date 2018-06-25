#ifndef _EMULATOR_H_
#define _EMULATOR_H_

#include "rts.h"
#include "dataFileUtils.h"
#include <LIBJML/ThreadManager.h>

class L4Interface;
class hlt_blob;
class daqReader;
class SimpleXmlDoc;
class gbPayload;

class Emulator {
 public:
  L4Interface_cpu *l4cpu;
  L4Interface_simu *l4simu;
  L4Interface_decider *l4decider;

  char *filename;    // The input data file
  char *conffile;    // The configuration file
  char *paraDir;     // HLT configuration/calibration dir
  char *outfilename; // The output filename
  daqReader *rdr;
  int max;
  ThreadDesc *threadDesc;
    
  Emulator(L4Interface_cpu *cpu, L4Interface_simu *simu, L4Interface_decider *decider) {
    filename = NULL;
    outfilename = NULL;
    conffile = NULL;
    paraDir = NULL;
    max = 999999999;
    l4cpu = cpu;
    l4simu = simu;
    l4decider = decider;
    rdr = NULL;
  }
  
  int configureServer(int argc, char *argv[]);
  void run();

 private:
  dataFileUtil fileUtil;
  void showArgs();
  gbPayload *getEventSummarySfsFile(daqReader *rdr);
  int writeOutputFile(int fd, daqReader *rdr, hlt_blob *output_blobs, UINT64 trg_decision);
};

#endif
