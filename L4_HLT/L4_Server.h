#ifndef _L4_SERVER_
#define _L4_SERVER_

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <cmath>

#include "hltNetworkDataStruct.h"
#include "gl3Histo.h"
#include "hlt_entity.h"

#define MAX_MSG 5000
#define LINE_ARRAY_SIZE (MAX_MSG+1)
#define MAX_MACHINE 100

using namespace std;

class L4_Server {
public:
    L4_Server();
    L4_Server(char* parameterDir);
    L4_Server(hlt_config_t *data);
    ~L4_Server();
    void setup();

    char parametersDirectory[256];
    char beamlineFile[256];
    char GainParameters[256];
    char hltParameters[256];
    // char line[LINE_ARRAY_SIZE];
    struct hltCaliData* ReceivedInfo;
    struct hltCaliResult* SendInfo;
    
    int run_config();
    int run_start();
    int run();
    int run_stop();

    void Get_config(hlt_config_t *data);
    void readBeamline();
    void updateBeamline(float vx, float vy, int npri);
    void writeBeamline();
    void printInfo();

    void readGainParameters();
    int CalibGainParameters();

    unsigned int clientPort[MAX_MACHINE];
    in_addr_t    clientIP[MAX_MACHINE];
    int          machineCounter;

    int          nReceivedEvents;
    int          runnumber;
    float        beamlineX;
    float        beamlineY;

    // dEdx
    gl3Histo *dEdx; // -13.3 ~ -12.3 log(dEdx (GeV/cm))
    float innerGain;
    float outerGain;
    float normInnerGain ;
    float normOuterGain ;
    float Low;
    float Hig;

private:
    int listenSocket;
    unsigned short int listenPort;
    //socklen_t clientAddressLength;
    //struct sockaddr_in clientAddress, serverAddress;
    int nTracksCutUpdateBeamline;
    int nTracksCutUpdatedEdxGainPara;
    int nEventsWriteBeamline;
};

#endif
