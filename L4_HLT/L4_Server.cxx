#include "L4_Server.h"
#include "rtsLog.h"
#include <cstring>
#include <errno.h>
#include <string.h>

void L4_Server::setup() {
    // Default initializer using Default parameters 
    // initialize
    listenSocket = 0;
    nTracksCutUpdateBeamline = 50;
    nTracksCutUpdatedEdxGainPara = 10000;
    nEventsWriteBeamline = 50000;
    listenPort = 50001;

    machineCounter = 0;
    memset(clientPort,0,MAX_MACHINE);
    memset(clientIP,-1,MAX_MACHINE);

    ReceivedInfo = new (struct hltCaliData);
    SendInfo = new (struct hltCaliResult);
    
    nReceivedEvents = 0;
    runnumber = 0;
    Low = -13.3;
    Hig = -12.3;
    normInnerGain = 2.89098808e-08 ;
    normOuterGain = 3.94915375e-08 ;
    innerGain = 0;
    outerGain = 0;

    char name[10];
    sprintf(name, "dEdx");
    dEdx = new gl3Histo(name, name, 500, -13.3, -12.3);
}

L4_Server::L4_Server()
{
    setup();
    LOG(INFO," using default directory ./");
    sprintf(parametersDirectory, "./");
    sprintf(beamlineFile, "%s/beamline", parametersDirectory);
    sprintf(GainParameters, "%s/GainParameters", parametersDirectory);
    sprintf(hltParameters, "%s/HLTparameters", parametersDirectory);
    run_config();
    readBeamline();
    readGainParameters();
}

L4_Server::L4_Server(char* parameterDir)
{
    setup();
    sprintf(parametersDirectory, "%s", parameterDir);
    sprintf(beamlineFile, "%s/beamline", parametersDirectory);
    sprintf(GainParameters, "%s/GainParameters", parametersDirectory);
    sprintf(hltParameters, "%s/HLTparameters", parametersDirectory);
    run_config();
    readBeamline();
    readGainParameters();
}

L4_Server::L4_Server(struct hlt_config_t *data)
{
    setup();
    if(data) Get_config(data);
    else {
        LOG(INFO,"using default directory ./");
        sprintf(parametersDirectory, "./");
        sprintf(beamlineFile, "%s/beamline", parametersDirectory);
        sprintf(GainParameters, "%s/GainParameters", parametersDirectory);
        sprintf(hltParameters, "%s/HLTparameters", parametersDirectory);
    }
    run_config();
    readBeamline();
    readGainParameters();
}

L4_Server::~L4_Server() {
    delete ReceivedInfo;
    delete SendInfo;
    delete dEdx;
}

void L4_Server::printInfo(){
	cout<< "==============L4_Server Debug Info================="<<endl;
	cout<< "1.ParameterDir : "<<parametersDirectory<<endl;
	cout<< "  beamlineFile : "<<beamlineFile<<endl;
	cout<< "  GainParameters : "<<GainParameters<<endl;
	cout<< "2.Beamline.X : "  <<beamlineX <<endl;
	cout<< "  Beamline.Y : "  <<beamlineY <<endl;
	cout<< "3.innerGain : "   <<innerGain <<endl;
	cout<< "  outerGain : "   <<outerGain <<endl;
	cout<< "4.Parameters "<<endl;
	cout<< "  nTracksCutUpdateBeamline : "<<nTracksCutUpdateBeamline<<endl;
	cout<< "  nTracksCutUpdatedEdxGainPara : " << nTracksCutUpdatedEdxGainPara <<endl;
	cout<< "  nEventsWriteBeamline : " << nEventsWriteBeamline <<endl;
}

int L4_Server::run_config()
{
    string parameterName;
    ifstream f1;
    f1.open(hltParameters);
    //if(f1.fail()) cout<<"ERR: L4_Server::run_config() HLTparameters not found"<<endl;
    if(!f1.fail())
        while(!f1.eof()) {
            f1 >> parameterName;
            if(parameterName == "updateBeamline") f1 >> nTracksCutUpdateBeamline;
            if(parameterName == "updatedEdxGain") f1 >> nTracksCutUpdatedEdxGainPara;
            if(parameterName == "nEventsWriteBeamline") f1 >> nEventsWriteBeamline;
            if(parameterName == "listenPort") f1 >> listenPort;
        }
    else {
		 LOG(ERR," HLTparameters not found");
		 return 1;
	 }

    return 0;
}

int L4_Server::run_start()
{
    // UDP server
    listenSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if(listenSocket < 0) {
        LOG(CRIT, "cannot create listen socket - ");
        return 1;
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(listenPort);

    if(bind(listenSocket,
            (struct sockaddr *) &serverAddress,
            sizeof(serverAddress)) < 0) {
        LOG(CRIT, "cannot bind socket - (%s) port=%d. There may be an unkilled ptheard!!",strerror(errno),listenPort);
        return 1; 
    }

    // set socket timeout
    struct timeval timeVal;
    timeVal.tv_sec = 10;
    timeVal.tv_usec = 0;
    if (-1 == setsockopt(listenSocket, SOL_SOCKET, SO_RCVTIMEO, &timeVal, sizeof(timeVal)) ) {
        LOG(ERR, "set sock timeout failed: %s", strerror(errno));
        return 1;
    }

    // set socekt for broadcast
    int opt = 1;
    if (-1 == setsockopt(listenSocket, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(opt)) ) {
        LOG(ERR, "set sock broadcasting failed: %s", strerror(errno));
        return 1;
    }

    // // parpare initial data
    // SendInfo->x = beamlineX;
    // SendInfo->y = beamlineY;
    // SendInfo->innerGain = innerGain;
    // SendInfo->outerGain = outerGain;

    // int N = 10;
    // while (N--) {
    //     LOG(NOTE, "Broadcasting initial calibration values %i", N);
    //     if (sendto(listenSocket, SendInfo, sizeof(struct hltCaliResult), 0,
    //                (struct sockaddr*)&addrto, sizeof(addrto)) < 0) {
    //         LOG(ERR, "Broadcasting failed: %s", strerror(errno));
    //     }
    //     sleep(1);
    // }

    LOG(INFO, "Waiting for request on port %i", listenPort);
    return 0;
}

int L4_Server::run()
{
    struct sockaddr_in addrto;
    addrto.sin_family = AF_INET;
    addrto.sin_addr.s_addr = inet_addr("172.17.255.255"); // broadcasting address
    addrto.sin_port = htons(50000);

    struct sockaddr_in clientAddress;
    socklen_t clientAddressLength;
    clientAddressLength = sizeof(clientAddress);

    // initial reveiving buffer
    memset(ReceivedInfo, 0x0, sizeof(struct hltCaliData));

    int ret = recvfrom(listenSocket, ReceivedInfo, sizeof(struct hltCaliData), 0,
                       (struct sockaddr *) &clientAddress, &clientAddressLength);
    if (-1 == ret) {
        if (errno == EAGAIN) {
            LOG(ERR, "recvfrom timeout after 10s, try again");
        } else {
            LOG(ERR, "%s", strerror(errno));
        }
        return 1;
    } else {
        LOG(NOTE, "%i out of %i bytes received", ret, sizeof(struct hltCaliData));
    }
    
    if(strncmp(ReceivedInfo->header, "INIT", 4) == 0) {
        SendInfo->x = beamlineX;
        SendInfo->y = beamlineY;
        SendInfo->innerGain = innerGain;
        SendInfo->outerGain = outerGain;

        // show the client's IP address
        LOG(NOTE, "Initial request from %s:%i", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
				
        // if (sendto(listenSocket, SendInfo, sizeof(struct hltCaliResult), 0,
        //            (struct sockaddr*)&addrto, sizeof(addrto)) < 0) {
        //     LOG(ERR, "Broadcasting failed: %s", strerror(errno));
        // }

        if (sendto(listenSocket, SendInfo, sizeof(struct hltCaliResult), 0,
                   (struct sockaddr*)&clientAddress, sizeof(struct sockaddr_in)) < 0) {
            LOG(ERR, "INIT send back failed: %s", strerror(errno));
        }

    } else if (strncmp(ReceivedInfo->header, "CALB", 4) == 0) {
        runnumber  = ReceivedInfo->runnumber;
        int   npri = ReceivedInfo->npri;
        float vx   = ReceivedInfo->vx;
        float vy   = ReceivedInfo->vy;
        int   nMip = ReceivedInfo->nMipTracks;

        // fill dEdx histogram
        if(nMip < 0 || nMip > 500) {
            LOG(DBG,"number of mip track is small than 0 or exceed the buff size!!!!");
        } else {
            for(int i = 0; i < nMip; i++) {
                dEdx->Fill((double)(ReceivedInfo->logdEdx[i]));
            }
        }

        if( fabs(vx)>1.e-8 || fabs(vy)>1.e-8 ) updateBeamline(vx, vy, npri);

        SendInfo->x = beamlineX;
        SendInfo->y = beamlineY;

        nReceivedEvents++;
        if (! (nReceivedEvents % 100) ) {
            // broadcast updated calibration values once per 1000 events
            if (sendto(listenSocket, SendInfo, sizeof(struct hltCaliResult), 0,
                       (struct sockaddr*)&addrto, sizeof(addrto)) < 0) {
                LOG(ERR, "Broadcasting failed: %s", strerror(errno));
            }
            nReceivedEvents = 0;
        }
    }

    return 0;
}

int L4_Server::run_stop()
{
    CalibGainParameters();
    writeBeamline();
    close(listenSocket);
    LOG(INFO,"L4_Server run stop");
    return 0;
}

void L4_Server::Get_config(struct hlt_config_t *hlt_config)
{
    LOG(INFO,"HLT parameter directory : %s", hlt_config->config_directory);
    sprintf(parametersDirectory, "%s/", hlt_config->config_directory);
    sprintf(beamlineFile, "%sbeamline", parametersDirectory);
    sprintf(GainParameters, "%sGainParameters", parametersDirectory);
    sprintf(hltParameters, "%sHLTparameters", parametersDirectory);
}

void L4_Server::readBeamline()
{
    int run_num[50];
    double beamX[50] , beamY[50];
    int end_num = 0;
    int nlines = 0;
    string str;

    memset(run_num, 0, sizeof(run_num)) ;
    memset(beamX, 0, sizeof(beamX)) ;
    memset(beamY, 0, sizeof(beamY)) ;

    ifstream ifs(beamlineFile);

    if(!ifs.fail()) {
        while(!ifs.eof()) {
            end_num = end_num % 50;
            int run_bit = 999;
            double tem_beamx = 999.;
            double tem_beamy = 999.;

            ifs >> str;
            run_bit = atoi(str.data());
            if(run_bit < 10000000 || run_bit > 500000000) continue;

            ifs >> str;
            tem_beamx = atof(str.data());
            if(fabs(tem_beamx - 0.40) > 10.) continue ;

            ifs >> str;
            tem_beamy = atof(str.data());
            if(fabs(tem_beamy - 0.10) > 10.) continue;

            run_num[end_num] = run_bit ;
            beamX[end_num] = tem_beamx;
            beamY[end_num] = tem_beamy;
            end_num++;
            if(nlines < 50) nlines++ ;
        }
        ifs.close();
    } else {
        LOG(ERR," beamlineFile not found: %s", beamlineFile);
    }

    int pro_num = 0;
    int run_con = run_num[(end_num - 1 + 50) % 50];
    double beamXSum = 0;
    double beamYSum = 0;
    for(int i = 1; i <= nlines; i++) {
        int index = (end_num - i + 50) % 50;
        if(run_con == run_num[index]) {
            beamXSum = beamXSum + beamX[index] ;
            beamYSum = beamYSum + beamY[index] ;
            pro_num++;
        } else break ;
    }

    if(pro_num > 0) {
        beamlineX = beamXSum / pro_num ;
        beamlineY = beamYSum / pro_num ;
    }

    LOG(INFO," run_number: %d , beamlineX: %f , beamlineY: %f", run_con, beamlineX, beamlineY);
}

void L4_Server::updateBeamline(float vx, float vy, int npri)
{
    if(npri > nTracksCutUpdateBeamline) {
        double newVertexWeight = 0.0001;
        beamlineX = (1. - newVertexWeight) * beamlineX + newVertexWeight * vx;
        beamlineY = (1. - newVertexWeight) * beamlineY + newVertexWeight * vy;
    }
}

void L4_Server::writeBeamline()
{
    LOG(NOTE,"writeBeamline start");
    ofstream f1;
    f1.open(beamlineFile, ios_base::app);
    if(!f1) {
        LOG(ERR," can't write to beamline file: %s", beamlineFile);
        return;
    }
    LOG(INFO," runnumber %i \t\tbeamlineX: %f , \t\tbeamlineY: %f ",runnumber, beamlineX, beamlineY);
    f1 << runnumber << "\t\t" << beamlineX << "\t\t" << beamlineY << endl;
    f1.close();
    LOG(NOTE,"writeBeamline done");
}

void L4_Server::readGainParameters()
{
    int run_num[50];
    double inner[50] , outer[50];
    int end_num = 0;
    int nlines = 0;
    string str;

    ifstream ifs(GainParameters);

    if(!ifs.fail()) {
        while(!ifs.eof()) {
            end_num = end_num % 50;
            int run_bit = 999;
            double tem_innergain = 999.;
            double tem_outergain = 999.;

            ifs >> str;
            run_bit = atoi(str.data());
            if(run_bit < 10000000 || run_bit > 500000000) continue;
            ifs >> str;
            tem_innergain = atof(str.data());
            if(fabs(tem_innergain - normInnerGain) / normInnerGain > 0.9) continue ;

            ifs >> str;
            tem_outergain = atof(str.data());
            if(fabs(tem_outergain - normOuterGain) / normOuterGain > 0.9) continue;

            run_num[end_num] = run_bit ;
            inner[end_num] = tem_innergain;
            outer[end_num] = tem_outergain;
            end_num++;
            if(nlines < 50) nlines++ ;
        }
    } else {
        LOG(ERR," GainParameters not found: %s ", GainParameters);
    }

    int pro_num = 0;
    int run_con = run_num[(end_num - 1 + 50) % 50];
    double innerGainSum = 0;
    double outerGainSum = 0;
    for(int i = 1; i <= nlines; i++) {
        int index = (end_num - i + 50) % 50;
        if(run_con == run_num[index]) {
            innerGainSum = innerGainSum + inner[index] ;
            outerGainSum = outerGainSum + outer[index] ;
            pro_num++;
        } else break ;
    }
    innerGain = innerGainSum / pro_num ;
    outerGain = outerGainSum / pro_num ;

    if ( !(innerGain >= normInnerGain*0.9 && innerGain <= normInnerGain*1.1) ) {
        LOG(WARN, "Get unusual value from configuration file, using normInnerGain");
        innerGain = normInnerGain;
    }

    if ( !(outerGain >= normOuterGain*0.9 && outerGain <= normOuterGain*1.1) ) {
        LOG(WARN, "Get unusual value from configuration file, using normOuterGain");
        outerGain = normOuterGain;
    }

    LOG(INFO," run_number: %d , innerSectorGain: %e , outerSectorGain: %e", run_con, innerGain, outerGain);

    ifs.close();
}

int L4_Server::CalibGainParameters()
{
    int    MaxBin;
    double MaxValue;
    double scaleFactor;

    double rmx2y = 0. ;
    double rmx4 = 0. ;
    double rmx3 = 0. ;
    double rmx2 = 0. ;
    double rmxy = 0. ;

    int Entries = dEdx->header.nEntries ;

    if(Entries < nTracksCutUpdatedEdxGainPara) { 
	LOG(WARN,"not enough pion MIP tracks to calib dEdx. We need at lest %i tracks",nTracksCutUpdatedEdxGainPara);
	return 0;
    }

    MaxBin   = dEdx->GetMaximumBin();
    MaxValue = -13.3 + MaxBin / 500. + 0.001;

    int BinStart = MaxBin - 60 ;
    int BinEnd   = MaxBin + 40 ;

    double mx2y = 0.;
    double  mx2 = 0. ;
    double  my = 0. ;
    double mx4 = 0.;
    double  mx3 = 0. ;
    double  mx = 0. ;
    double  mxy = 0. ;

    for(int i = BinStart; i < BinEnd; i++) {
        double x = -13.3 + i / 500. + 0.001 ;
        double y = dEdx->GetValue(i) ;

        mx2y =  mx2y + pow(x, 2) * y;
        mx2  =  mx2 + pow(x, 2);
        my   =  my + y;

        mx4  =  mx4 + pow(x, 4);
        mx3  =  mx3 + pow(x, 3);
        mx   =  mx + x;
        mxy  =  mxy + x * y;

    }

    float nbins = BinEnd - BinStart ;

    rmx2y = mx2y / nbins - mx2 * my / nbins / nbins ;
    rmx4  = mx4 / nbins - mx2 * mx2 / nbins / nbins ;
    rmx3  = mx3 / nbins - mx * mx2 / nbins / nbins ;
    rmx2  = mx2 / nbins - mx * mx / nbins / nbins ;
    rmxy  = mxy / nbins - mx * my / nbins / nbins ;

    double p2 = (rmx3 * rmxy - rmx2 * rmx2y) / (rmx3 * rmx3 - rmx2 * rmx4) ;
    double p1 = (rmxy - p2 * rmx3) / rmx2 ;

    double a = -0.5 * p1 / p2 ;

    scaleFactor = 2.3970691 / exp(a) * 1.e-6 ;

    double TempinnerGainPara = innerGain * scaleFactor ;
    double TempouterGainPara = outerGain * scaleFactor ;

    LOG(INFO, "InnerGain && OuterGain parameters run by run: Entries :%d \t maxBin :%d \t maxValue :%f \t pol maxValue:%f \t scale Factor:%f", (int)Entries , MaxBin , MaxValue , a , scaleFactor);

    //		cout<<"INFO InnerGain && OuterGain parameters run by run: Entries : "<<(int)Entries 
    //			<<"  \t maxBin : "<<MaxBin<< "\t maxValue : "<<MaxValue<< " \t pol maxValue: "<<a
    //			<<" \t scale Factor:"<<scaleFactor<<endl;

    if(fabs(scaleFactor - 1.) < 0.1) {
        LOG(INFO, "Writing new dedx calibration");
        ofstream f2;
        f2.open(GainParameters, ios_base::app);

        if(!f2) {
            LOG(ERR," can't write to GainParameters file: %s", GainParameters);
            return 1;
        }
        LOG(INFO, "runnumber  %i\t\t   innerGainPara %e\t\t  outerGainPara %e", runnumber, TempinnerGainPara, TempouterGainPara);
        f2 << runnumber << "\t\t" <<  TempinnerGainPara<< "\t\t" << TempouterGainPara << endl;
        f2.close();
    } else {
        LOG(INFO, "GainParameters: not recored %18i %15.6e %15.6e", runnumber, TempinnerGainPara, TempouterGainPara);
        LOG(WARN," innerSectorGain and outerSectorGain change larger than 10% !!!");
    }
    return 0;
}
