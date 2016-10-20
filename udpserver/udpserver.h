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

#define UDPS_RECVBUF_SIZE_DEFAULT	(1024 * 1024)

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
	UDPServer(const char *pcIPAddr, unsigned int uiPort, int iMode = UDPS_WORK_MODE_DEFAULT);
	~UDPServer();
	int SetLocalIPPort(char *pcIPAddr, unsigned int uiPort);
	int SetMode(int iMode);
	int SetSYSBuffer(MBUFFERSYSBuffer *pSYSBuffer);
	int SetFilePath(const char *pcFilePath);
	int SetSockRecvBufSize(int iSize);

	int Start();
	int Stop();
	int Execute();

	int Send(unsigned char * pucBuffer, unsigned int iBufferSize);
	int Recv(unsigned char * pucBuffer, unsigned int iBufferSize);
	int GetPeerAddrInfo(struct sockaddr * peerSockAddr, int *piAddrLen);

private:
	char			m_pcIP[UPDS_MAX_IP_LENGTH];
	unsigned int	m_uiPort;
	SOCKET			m_socket;
	struct sockaddr_in m_peerSockAddr;
	int				m_iSockRecvBufSize;

	int				m_iMode; // see CT_WORK_MODE_XXX

	MBUFFERSYSBuffer	*m_pSYSBuffer;
	char				m_pcFilePath[UPDS_MAX_PATH_LENGTH];
	
	unsigned char		*m_pucBuffer;
	CTThreadHandle		m_thread;
	int					m_iStop;
	int					m_iRunning;
};
