#include "l4Emulator.h"

//  Include the interface class you want to use! 
//#include "l4RandomEmulate.h"
//#include "l4TaskEmulate.h"
#include "L4HLTTask_cpu.h"
#include "L4HLTTask_simu.h"
#include "L4HLTTask_decider.h"

#include <rtsLog.h>

int main(int argc, char *argv[]) {
    // Doesn't currently simulate multiple thread issues...
    // Simply creates one of each stage...

    L4HLTTask_cpu l4_cpu(0);
    L4HLTTask_simu l4_simu(0);
    L4HLTTask_decider l4_decider;
    
    //L4TaskEmulate l4;
    Emulator emulator(&l4_cpu, &l4_simu, &l4_decider);
    if(emulator.configureServer(argc, argv) < 0) return -1;

    LOG(DBG, "HERE");
    emulator.run();
    LOG(DBG, "HERE");
}
