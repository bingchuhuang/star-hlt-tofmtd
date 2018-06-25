#include "scif_KFP_client.h"
#include "TPCCATracker/code/KFParticle/KFParticleTopoReconstructor.h"

// -----------------------------------------------------------------------------
scif_KFP_client::scif_KFP_client(int id, uint16_t lport, uint16_t rnode, uint16_t rport,
                                 KFParticleTopoReconstructor*& topo, float Bz_, int rn)
    : scif_client(id, lport, rnode, rport), topoReconstructor(topo),
      Bz(Bz_), runnumber(rn), buffer(NULL) {

    maxEventSize = 0;
    controlSignal = 0;
}

// -----------------------------------------------------------------------------
scif_KFP_client::~scif_KFP_client() {
    int ret = scif_unregister(epd, offsetSender, maxEventSize);
    if (ret) {
        LOG(ERR, "scif_unregister failed, %s", strerror(errno));
    }
    
    if (buffer) {
        free(buffer);
    }
}

// -----------------------------------------------------------------------------
int scif_KFP_client::sendBz() {
    int ret = scif_send(epd, &Bz, sizeof(Bz), SCIF_SEND_BLOCK); // synchronization
    if ( -1 == ret ) {
        LOG(ERR, "sendBz failed. Error: %s", strerror(errno));
    }

    return ret;
}

// -----------------------------------------------------------------------------
int scif_KFP_client::sendRunnumber() {
    int ret = scif_send(epd, &runnumber, sizeof(runnumber), SCIF_SEND_BLOCK); // synchronization
    if ( -1 == ret ) {
        LOG(ERR, "%s failed. Error: %s", __FUNCTION__, strerror(errno));
    }

    return ret;
}

// -----------------------------------------------------------------------------
int scif_KFP_client::setupDMA() {
    int ret = 0;
    
    ret = scif_recv(epd, &offsetServer, sizeof(offsetServer), SCIF_RECV_BLOCK);
    if ( -1 == ret ) {
        LOG(ERR, "receive offsetServer failed. Error: %s", strerror(errno));
        return ret;
    }

    ret = scif_recv(epd, &maxEventSize, sizeof(maxEventSize), SCIF_RECV_BLOCK);
    if ( -1 == ret ) {
        LOG(ERR, "receive maxEventSize failed. Error: %s", strerror(errno));
    }
    LOG("THLT", "maxEventSize = %d", maxEventSize);
    
    ret = posix_memalign(&buffer, 0x1000, maxEventSize);
    if (ret) {
        switch (ret) {
        case EINVAL:
            LOG(ERR, "posix_memalign failed, required size was not a power of two");
            return ret;
        case ENOMEM:
            LOG(ERR, "posix_memalign failed, there was insufficient memory");
            return ret;
        }
    }
    
    offsetSender = scif_register(epd, buffer, maxEventSize, 0,
                                 SCIF_PROT_READ | SCIF_PROT_WRITE,0);

    if (SCIF_REGISTER_FAILED == offsetSender) {
        LOG(ERR, "scif_register failed. Error: %s", strerror(errno));
        return SCIF_REGISTER_FAILED;
    }
    
    controlSignal = local_port;
    ret = scif_send(epd, &controlSignal, sizeof(int), SCIF_SEND_BLOCK);
    if (-1 == ret) {
        LOG(ERR, "scif_send failed. Error: %s", strerror(errno));
        return ret;
    }
    LOG("THLT", "controlSignal = %d", controlSignal);
    
    return 0;
}

// -------------------------------------------------------------------------------
#ifdef WITHSCIF
int scif_KFP_client::sendDataToPhi()
{
    topoReconstructor->SendDataToXeonPhi(myId, epd, buffer, offsetServer, offsetSender, Bz);

    return 0;
}
#endif // WITHSCIF

