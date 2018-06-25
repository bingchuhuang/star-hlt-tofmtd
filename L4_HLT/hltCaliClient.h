#ifndef HLTCALICLIENT_H
#define HLTCALICLIENT_H

#include "hltNetworkDataStruct.h"

#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <pthread.h>
#include <rtsLog.h>


class hltCaliClient
{
public:
    ~hltCaliClient();

    static hltCaliClient* GetInstance();
    void SendInitRequest();
    void SendCaliData(hltCaliData* source);
    void RecvCaliResult();
    void CopyCaliResult(hltCaliResult& a);
    
private:
    hltCaliClient();
    static hltCaliClient* instance;

    hltCaliResult caliResult;
    pthread_mutex_t caliResultMux;
    inline void TakeMux() { pthread_mutex_lock(&caliResultMux); }
    inline void GiveMux() { pthread_mutex_unlock(&caliResultMux); }

    int sock;
    int sock_opt;
    struct sockaddr_in addr;
    struct sockaddr_in server_addr;

    static const int   udp_client_port;
    static const char* udp_server_ip;
    static const int   udp_server_port;
};

#endif /* HLTCALICLIENT_H */
