#include <stdio.h>
#include <WS2tcpip.h>
#include "udpserver.h"
#include "threadutil.h"


#define SAFERELEASE(ptr) do \
						{								\
							if (ptr != NULL)			\
							{							\
								free(ptr);				\
								ptr = NULL;				\
							}							\
						} while (0);					\

#ifdef _WIN32
typedef unsigned long block_value_type;
#elif
#define INVALID_SOCKET  -1
#define SOCKET_ERROR    -1

#define closesocket     close
#define ioctlsocket     ioctl

typedef int block_value_type;
typedef struct timeval TIMEVAL;

#define Sleep(millisec) usleep((millisec) * 1000);
#endif


#ifdef _WIN32
static DWORD UDPServerExecute(void *arg)
#elif
static void *UDPServerExecute(void *arg)
#endif
{
	UDPServer *pServer = (UDPServer *)arg;
	pServer->Execute();
	return(0);
}

UDPServer::UDPServer()
{
	m_pcIP[0] = 0;
	m_uiPort = 0;
	m_iMode = UDPS_WORK_MODE_DEFAULT;

	m_pucBuffer = (unsigned char *)malloc(UDPS_BUFFER_SIZE);
}

int UDPServer::SetLocalIPPort(char *pcIPAddr, unsigned int uiPort)
{
	if (pcIPAddr == NULL)
		return UDPS_EC_FAILURE;
	strncpy(m_pcIP, pcIPAddr, UPDS_MAX_IP_LENGTH - 1);
	m_pcIP[UPDS_MAX_IP_LENGTH - 1] = 0;
	m_uiPort = uiPort;

	return UDPS_EC_OK;
}

int UDPServer::SetMode(int iMode)
{
	m_iMode = iMode;
	return UDPS_EC_OK;
}


UDPServer::UDPServer(const char *pcIPAddr, unsigned int uiPort, int iMode)
{
	if (pcIPAddr != NULL)
	{
		strncpy(m_pcIP, pcIPAddr, UPDS_MAX_IP_LENGTH - 1);
		m_pcIP[UPDS_MAX_IP_LENGTH - 1] = 0;
	}
	m_uiPort = uiPort;
	m_socket = INVALID_SOCKET;
	m_iSockRecvBufSize = UDPS_RECVBUF_SIZE_DEFAULT;
	m_iMode = iMode;
	m_pucBuffer = (unsigned char *)malloc(UDPS_BUFFER_SIZE);
	m_iStop = 0;
	m_iRunning = 0;
}

UDPServer::~UDPServer()
{
	SAFERELEASE(m_pucBuffer);
	closesocket(m_socket);
}

int UDPServer::SetSYSBuffer(MBUFFERSYSBuffer *pSYSBuffer)
{
	if (pSYSBuffer == NULL)
		return UDPS_EC_FAILURE;
	m_pSYSBuffer = pSYSBuffer;
	
	return UDPS_EC_OK;
}

int UDPServer::SetFilePath(const char *pcFilePath)
{
	if (pcFilePath == NULL)
		return UDPS_EC_FAILURE;
	
	strcpy(m_pcFilePath, pcFilePath);
	return UDPS_EC_OK;
}

int UDPServer::SetSockRecvBufSize(int iSize)
{
	m_iSockRecvBufSize = iSize;
	return UDPS_EC_OK;
}

int UDPServer::Start()
{
#ifdef _WIN32
	WSADATA wsaData;
	WORD sockVersion = MAKEWORD(2, 2);
	WSAStartup(sockVersion, &wsaData);
#endif
	m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_socket == INVALID_SOCKET)
	{
		printf("socket error !\n");
		return UDPS_EC_FAILURE;
	}
	setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (const char *)&m_iSockRecvBufSize, sizeof(m_iSockRecvBufSize));

	struct sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(m_uiPort);
	inet_pton(AF_INET, m_pcIP, &(serAddr.sin_addr.S_un.S_addr));
	//serAddr.sin_addr.S_un.S_addr = inet_pton(AF_INET, m_pcIP); // INADDR_ANY;
	if (bind(m_socket, (struct sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
	{
		printf("bind error !\n");
		return UDPS_EC_FAILURE;
	}


	if (m_iMode == UDPS_WORK_MODE_DUMP2BUFFER || m_iMode == UDPS_WORK_MODE_DUMP2FILE)
	{
		int iRet = CTCreateThread(&m_thread, UDPServerExecute, this);
		if (iRet == -1)
		{
			printf("create thread failed\n");
			return UDPS_EC_FAILURE;
		}
	}

	return UDPS_EC_OK;
}

int UDPServer::Stop()
{
	m_iStop = 1;
	while (m_iRunning)
	{
		Sleep(1);
	}

	return UDPS_EC_OK;
}

int UDPServer::Execute()
{
	FILE *fpOutput = NULL;
	m_iRunning = 1;
	m_iStop = 0;

	if (m_iMode == UDPS_WORK_MODE_DUMP2FILE)
	{
		fpOutput = fopen(m_pcFilePath, "wb+");
		if (fpOutput == NULL)
		{
			printf("Couldn't open file\n");
			return UDPS_EC_FAILURE;
		}
	}

	int iTotalNum = 0;
	while (!m_iStop)
	{
		int iRet = Recv(m_pucBuffer, UDPS_BUFFER_SIZE);
		if (iRet > 0)
		{
			iTotalNum += iRet;
			printf("got packet, len = %d, total_bytes = %d\n", iRet, iTotalNum);
			if (m_iMode == UDPS_WORK_MODE_DUMP2FILE)
			{
				if (iRet != fwrite(m_pucBuffer, 1, iRet, fpOutput))
				{
					printf("fwrite failed\n");
					break;
				}
			}
			else if (m_iMode == UDPS_WORK_MODE_DUMP2BUFFER)
			{
				while (MBUFFERSYSBufferSpaceAvailable(m_pSYSBuffer) < iRet)
				{
					Sleep(1);
				}

				int iLen2End = MBUFFERSYSBufferSpaceAvailableToEnd(m_pSYSBuffer);
				if (iLen2End >= iRet)
					memcpy(MBUFFERSYSAppendDataPos(m_pSYSBuffer), m_pucBuffer, iRet);
				else
				{
					memcpy(MBUFFERSYSAppendDataPos(m_pSYSBuffer), m_pucBuffer, iLen2End);
					memcpy(MBUFFERSYSAppendDataPos(m_pSYSBuffer), m_pucBuffer + iLen2End, iRet - iLen2End);
				}
			}
		}
		else
		{
			// WSAEINTR;
			printf("recvfrom failed, errno = %d\n", WSAGetLastError());
			break;
		}
	}
	m_iRunning = 0;

	printf("totalNum = %d\n", iTotalNum);

	return UDPS_EC_OK;
}

int UDPServer::Send(unsigned char * pucBuffer, unsigned int iBufferSize)
{
	return 0;
}

// return the number of bytes received if succeed.
// or return the errcode of recvfrom
int UDPServer::Recv(unsigned char * pucBuffer, unsigned int iBufferSize)
{
	struct sockaddr_in remoteAddr;
	int iAddrLen = sizeof(remoteAddr);
	return recvfrom(m_socket, (char *)m_pucBuffer, UDPS_BUFFER_SIZE, 0, (sockaddr *)&remoteAddr, &iAddrLen);
}

int UDPServer::GetPeerAddrInfo(struct sockaddr * peerSockAddr, int *piAddrLen)
{
	return UDPS_EC_OK;
}