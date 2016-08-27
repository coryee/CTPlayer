#pragma once

#ifdef _WIN32
#include <WinSock2.h>
#endif
#include "mbuffer.h"
#include "threadutil.h"


#ifndef _WIN32
typedef SOCKET int;
#endif


#define UPDS_MAX_IP_LENGTH			16
#define UPDS_MAX_PATH_LENGTH		256
#define UDPS_BUFFER_SIZE			(1024 * 1024)

#define UDPS_WORK_MODE_DEFAULT		1
#define UDPS_WORK_MODE_DUMP2FILE	2
#define UDPS_WORK_MODE_DUMP2BUFFER	3


// error code
#define UDPS_EC_OK				0
#define UDPS_EC_FAILURE			-1

class UDPServer
{
public:
	UDPServer();
	UDPServer(char *pcIPAddr, unsigned int uiPort, int iMode = UDPS_WORK_MODE_DEFAULT);
	int setLocalIPPort(char *pcIPAddr, unsigned int uiPort);
	int setMode(int iMode);
	int setSYSBuffer(MBUFFERSYSBuffer *pSYSBuffer);
	int setFilePath(const char *pcFilePath);

	int start();
	int stop();
	int execute();
	
	int recv2file();
	int recv2buffer();

	int send(unsigned char * pucBuffer, unsigned int iBufferSize);
	int recv(unsigned char * pucBuffer, unsigned int iBufferSize);
	int getPeerAddrInfo(struct sockaddr * peerSockAddr, int *piAddrLen);

	~UDPServer();

private:
	char m_pcIP[UPDS_MAX_IP_LENGTH];
	unsigned int m_uiPort;
	SOCKET m_s;
	struct sockaddr_in m_peerSockAddr;

	int m_iMode; // see CT_WORK_MODE_XXX

	MBUFFERSYSBuffer *m_pSYSBuffer;
	char m_pcFilePath[UPDS_MAX_PATH_LENGTH];
	
	unsigned char *m_pucBuffer;
	ThreadHandle m_thread;
	int m_iStop;
	int m_iRunning;
};
