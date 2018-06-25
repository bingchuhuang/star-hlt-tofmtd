#include "scif_client.h"

#include <error.h>
#include <string.h>

// -----------------------------------------------------------------------------
const int      scif_client::MAX_RETRY       = 20;
const uint16_t scif_client::nXeonPhi        = 2;
const uint16_t scif_client::localPortStart  = 2000;
const uint16_t scif_client::remotePortStart = 3000;

// -----------------------------------------------------------------------------
scif_client::scif_client(int id, uint16_t lpt, uint16_t rnode, uint16_t rport)
    : myId(id), local_port(lpt) {

    /* scif_open : creates an end point, when successful returns end pt descriptor */
    if ((epd = scif_open()) == SCIF_OPEN_FAILED) {
        LOG("Err", "scif_open failed with error %d\n", (int)epd);
        exit(1);
    }

    /* scif_bind : binds an end pt to a port_no, when successful returns the port_no
     * to which the end pt is bound
     */
    int conn_port;
    if ((conn_port = scif_bind(epd, local_port)) < 0) {
        LOG(ERR, "[id:%d] : scif_bind failed with error %d\n, %s",
            myId, conn_port, strerror(errno));
        exit(1);
    }
    LOG("THLT", "[id:%d] : scif_bind to port %d success, %d\n", myId, conn_port, local_port);

    portID.node = rnode;
    portID.port = rport;
    
}

// -----------------------------------------------------------------------------
scif_client::~scif_client() {
    scif_close(epd);
}

// -----------------------------------------------------------------------------
void scif_client::setPeer(uint16_t rnode, uint16_t rport) {
    portID.node = rnode;
    portID.port = rport;
}

// -----------------------------------------------------------------------------
int scif_client::connect() {
    int ret = 0;
    int trail = 0;

    while (trail++ < MAX_RETRY && (ret = scif_connect(epd, &portID)) < 0) {
        LOG("THLT", "connect to phi, try %i, error %s", trail, strerror(errno));
    }

    LOG("THLT", "ret = %i", ret);
    
    if (ret == -1) {
        LOG("THLT", "connection to node %d failed: trial %d, error %s\n",
            portID.node, trail, strerror(errno));
        return ret;
    } else {
        LOG("THLT", "connected to phi");
    }

    return ret;
}

// -----------------------------------------------------------------------------
int scif_client::closeConnection()
{
    int ret = scif_close(epd);
    if (ret) {
        LOG(ERR, "Closs SCIF connection failed");
    }

    return ret;
}
