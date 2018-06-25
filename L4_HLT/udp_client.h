#ifndef _UDP_CLIENT_H_
#define _UDP_CLIENT_H_

#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>

#include <rtsLog.h>

using std::cout;
using std::cerr;
using std::endl;

class udp_client
{
public:
    udp_client(const char* host = udp_client::DefautServerAddress,
               const unsigned short int portNum = udp_client::DefautServerPort);
    ~udp_client();
    static udp_client* GetInstance();
    int SendMessage(const void* msg, size_t len);
    int SendReceive(const void* s_msg, size_t s_len, void* r_msg, size_t& r_len);
    int Receive(void* r_msg, size_t& r_len);
    unsigned short int listenPort;
    
private:
    int socketDescriptor;
    unsigned short int serverPort;
    struct sockaddr_in serverAddress;
    struct hostent *hostInfo;
    struct timeval timeVal;
    fd_set readSet;

    static const char*              DefautServerAddress;
    static const unsigned short int DefautServerPort;
    static udp_client*              udp_client_instance;

    // static const int internalBufferLength = 1<<10;
    // char internalBuffer[internalBufferLength]; // 1K bytes
};

#endif  // _UDP_CLIENT_H_
