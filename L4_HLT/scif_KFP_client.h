#ifndef _SCIF_KFP_CLIENT_H_
#define _SCIF_KFP_CLIENT_H_

#include "scif_client.h"

class KFParticleTopoReconstructor;

class scif_KFP_client : public scif_client
{
public:
    scif_KFP_client(int id, uint16_t lpt, uint16_t rnode, uint16_t rport,
                    KFParticleTopoReconstructor*& topo, float Bz_, int rn);
    ~scif_KFP_client();
    int sendBz();
    int sendRunnumber();
    int setupDMA();
    int sendDataToPhi();
    
private:
    KFParticleTopoReconstructor*& topoReconstructor;
    float Bz;
    int   runnumber;
    void* buffer;
    off_t offsetServer;
    off_t offsetSender;
    int   maxEventSize;
    int   controlSignal;
};


#endif /* _SCIF_KFP_CLIENT_H_ */
