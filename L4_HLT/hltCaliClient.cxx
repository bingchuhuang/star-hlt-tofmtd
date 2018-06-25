#include "hltCaliClient.h"
#include <stdlib.h>
#include <string.h>
#include <LIBJML/ThreadManager.h>

// -----------------------------------------------------------------------------
hltCaliClient* hltCaliClient::instance = NULL;

#ifdef L4STANDALONE
const char* hltCaliClient::udp_server_ip = "127.0.0.1";
#else
const char* hltCaliClient::udp_server_ip = "172.17.27.1";
#endif // L4STANDALONE
    
const int hltCaliClient::udp_client_port = 50000;
const int hltCaliClient::udp_server_port = 50001;

// -----------------------------------------------------------------------------
hltCaliClient::hltCaliClient()
{
    LOG("THLT", "Creating hltCaliClient");
    pthread_mutex_init(&caliResultMux, NULL);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == sock) {
        LOG(ERR, "%s", strerror(errno));
    }

    sock_opt = 1;
    if ( setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&sock_opt, sizeof(sock_opt)) == -1 ) {
        LOG(ERR, "setsockopt", strerror(errno));
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(udp_client_port);

    if ( (bind(sock, (sockaddr*)&addr, sizeof(addr))) < 0 ) {
        LOG(ERR, "%s", strerror(errno));
    }

    LOG("THLT", "SERVER = %s:%i", udp_server_ip, udp_server_port);
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(udp_server_ip);
    server_addr.sin_port = htons(udp_server_port);
}

// -----------------------------------------------------------------------------
hltCaliClient::~hltCaliClient()
{
    pthread_mutex_destroy(&caliResultMux);
    delete instance;
}

// -----------------------------------------------------------------------------
hltCaliClient* hltCaliClient::GetInstance()
{
    // hltCaliClient* ret = instance;

    // if (NULL == ret) {
    //     TakeMux();

    //     ret = instance;
    //     if (NULL == ret) {
    //         instance = new hltCaliClient();
    //         ret = instance;
    //     }
        
    //     GiveMux();
    // }

    // return ret;

    if (NULL == instance) {
        instance = new hltCaliClient();
    }
    return instance;
}

// -----------------------------------------------------------------------------
void hltCaliClient::SendInitRequest()
{
    int ret = sendto(sock, "INIT", 4, 0, (sockaddr*)&server_addr, sizeof(server_addr));
    if (-1 == ret) {
        LOG(ERR, "%s failed: %s", __FUNCTION__, strerror(errno));
    } else {
        LOG(NOTE, "%i out of %i send", ret, 4);
    }
}

// -----------------------------------------------------------------------------
void hltCaliClient::SendCaliData(hltCaliData* caliData)
{
    int ret = sendto(sock, caliData, sizeof(hltCaliData), 0, (sockaddr*)&server_addr, sizeof(server_addr));
    if (-1 == ret) {
        LOG(ERR, "%s failed: %s", __FUNCTION__, strerror(errno));
    } else {
        LOG(NOTE, "%i out of %i bytes send", ret, sizeof(hltCaliData));
    }
}

// -----------------------------------------------------------------------------
void hltCaliClient::RecvCaliResult()
{
    ThreadDesc* threadDesc = threadManager.attach();
    TMCP;

    sockaddr src_addr;
    socklen_t src_addr_len = sizeof(src_addr);
    hltCaliResult recvbuf;

    LOG(NOTE, "Waiting for data");
    
    TMCP;
    int ret = recvfrom(sock, &recvbuf, sizeof(hltCaliResult), 0,
                       (sockaddr*)&src_addr, &src_addr_len);
    if (-1 == ret) {
        LOG(ERR, "%s failed: %s", __FUNCTION__, strerror(errno));
    } else if (0 ==  ret) {
        LOG(WARN, "peer shutdown");
    } else {                    // recvfrom sucessed
        LOG(NOTE, "%i out of %i recevied, %f, %f",
            ret, sizeof(hltCaliResult), recvbuf.x, recvbuf.y);
        TMCP;
        TakeMux();
        TMCP;
        caliResult = recvbuf;
        TMCP;
        GiveMux();
    }
}

// -----------------------------------------------------------------------------
void hltCaliClient::CopyCaliResult(hltCaliResult& a) {
    TakeMux();
    a = caliResult;
    GiveMux();
}
