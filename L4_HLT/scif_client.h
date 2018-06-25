#ifndef _SCIF_CLIENT_H_
#define _SCIF_CLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <scif.h>

#include <rtsLog.h>

class scif_client
{
public:
    static const int      MAX_RETRY;
    static const uint16_t nXeonPhi;
    static const uint16_t localPortStart;
    static const uint16_t remotePortStart;
    
    scif_client(int id, uint16_t lpt, uint16_t rnode, uint16_t uport);
    ~scif_client();
    void setPeer(uint16_t rnode, uint16_t rport);
    int connect();
    int closeConnection();
    
protected:
    void print();

    int myId;
    scif_epd_t epd;
    uint16_t local_port;
    struct scif_portID portID;

};


#endif /* _SCIF_CLIENT_H_ */

