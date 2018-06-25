#include "udp_client.h"
#include "RC_Config.h"

// in singleton mode, DefautServerAddress::DefautServerPort
// is what we can only use

#ifdef L4STANDALONE
const char*              udp_client::DefautServerAddress = "127.0.0.1";
#else
const char*              udp_client::DefautServerAddress = "172.17.27.1";
#endif // L4STANDALONE

const unsigned short int udp_client::DefautServerPort    = 50001;

udp_client*              udp_client::udp_client_instance = NULL;

udp_client::udp_client(const char* host, const unsigned short int portNum)
{
    if (udp_client_instance) {
	LOG(WARN,"Warning: udp_client_instance is alread existed.");
    } else {
	hostInfo = gethostbyname(host);
	if (hostInfo == NULL) {
	    LOG(ERR, "problem interpreting host: %s", host);
	    //exit(1);
	}

	serverPort = portNum;

	// Create a socket.  "AF_INET" means it will use the IPv4 protocol.
	// "SOCK_STREAM" means it will be a reliable connection (i.e., TCP,
	// for UDP use SOCK_DGRAM), and I'm not sure what the 0 for the last
	// parameter means, but it seems to work.
	socketDescriptor = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketDescriptor < 0) {
	    LOG(ERR, "cannot create socket");
	    //exit(1);
	}
	// bind the client to a port start from 50003 and retry 32 times
	struct sockaddr_in clientAddress;
	listenPort = 50003;
	int ret = -1;
	for(int i=0; i<60; i++) {
	    clientAddress.sin_family = AF_INET;
	    clientAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	    clientAddress.sin_port = htons(listenPort);
	    ret = bind( socketDescriptor,
		    (struct sockaddr *) &clientAddress,
		    sizeof(clientAddress) );
	    if( ret >= 0 ) break;
	    listenPort++;
	}

	if( ret < 0 ){
	    LOG(CRIT,"Can't bind socket to any port 50003~50034");
	} else {
	    LOG(NOTE,"bind client to port %i",listenPort);
	}

	// Set some fields in the serverAddress structure.  
	serverAddress.sin_family = hostInfo->h_addrtype;
	memcpy((char *) &serverAddress.sin_addr.s_addr,
		hostInfo->h_addr_list[0], hostInfo->h_length);
	serverAddress.sin_port = htons(serverPort);

	udp_client_instance = this;
	LOG(NOTE,"Connect to %s : %i",host, portNum);
    }
}

udp_client::~udp_client()
{
    delete udp_client_instance;
    udp_client_instance = NULL;
}

udp_client* udp_client::GetInstance()
{
    if (NULL == udp_client_instance) {
	udp_client_instance = new udp_client(DefautServerAddress, DefautServerPort);
    }

    return udp_client_instance;
}

int udp_client::SendMessage(const void* msg, size_t len)
{
    // ok if return >= 0
    // error else
    int ret =  sendto(socketDescriptor, msg, len, 0,
	    (struct sockaddr *) &serverAddress,
	    sizeof(serverAddress));
    if (ret == -1) {
	LOG(ERR, "Send Message failed.");
    }

    return ret;
}

int udp_client::Receive(void* r_msg, size_t& r_len) {
    int max_r_len = r_len;
    memset(r_msg, 0xFF, max_r_len);  // Zero out the buffer.
    int ret = recv(socketDescriptor, r_msg, max_r_len, 0);
    return ret;
}

int udp_client::SendReceive(const void* s_msg, size_t s_len, void* r_msg, size_t& r_len)
{
    // Send message pointed by s_msg with length s_len in bytes.
    // Receive message from server at the same time
    // Received message will be witten in a bugger pointed by r_msg
    // with max length of r_len in bytes.
    // After received message r_len will be the actual bytes occupated.

    int max_r_len = r_len;

    int ret = sendto(socketDescriptor, s_msg, s_len, 0,
	    (struct sockaddr *) &serverAddress,
	    sizeof(serverAddress));

    if (ret == -1) {
	LOG(ERR, "Send Message failed.");
	return ret;
    }

    // wait until answer comes back, for up to 5 second
    FD_ZERO(&readSet);
    FD_SET(socketDescriptor, &readSet);
    timeVal.tv_sec = 1;
    timeVal.tv_usec = 0;

    if (select(socketDescriptor+1, &readSet, NULL, NULL, &timeVal)) {
        // Read the modified line back from the server.
        memset(r_msg, 0xFF, max_r_len);  // Zero out the buffer.
        ret = recv(socketDescriptor, r_msg, max_r_len, 0);
        if (ret == -1 ) {
            r_len = 0;
            LOG(ERR, "didn't get response from server?");
            close(socketDescriptor);
        } else {
            LOG(WARN, "%i bytes initial data received", ret);
            r_len = ret;
        }
    } else {
        LOG(WARN, "** Server did not respond in 5 second.");
	ret = -1;
    }

    return ret;
}
