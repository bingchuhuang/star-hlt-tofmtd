#include "scif_KFP_client.h"
#include "TPCCATracker/code/KFParticle/KFParticleTopoReconstructor.h"
#include "profiler.hh"

#include <stdio.h>
#include <stdlib.h>

PROFILER_DEFINE;

int main(int argc, char *argv[])
{
    rtsLogOutput(RTS_LOG_STDERR);
    rtsLogLevel(NOTE);

    system("ssh mic1 \"~/run_KFPServer.sh 0\"");
    sleep(3);
    
    KFParticleTopoReconstructor* topoReconstructor = new KFParticleTopoReconstructor();

    scif_KFP_client c(0, 2000, 1, 3000, topoReconstructor, 0.5, 17001001);
    if ( c.connect() > 0 ) {
        c.sendBz();
        c.sendRunnumber();
        c.setupDMA();
    }

    sleep(10);
    
    delete topoReconstructor;
    return 0;
}
