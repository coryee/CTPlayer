#include <stdio.h>
#include <stdlib.h>
#include "threadutil.h"
#include "udpserver.h"

#pragma comment(lib, "Ws2_32.lib")


#define SAFERELEASE(ptr) do \
						{								\
							if (ptr != NULL)			\
							{							\
								free(ptr);				\
								ptr = NULL;				\
							}							\
						} while (0);					\

#ifdef _WIN32
static DWORD UDPServerExecute(void *arg)
{
	UDPServer *pServer = (UDPServer *)arg;
	pServer->execute();
	return(0);
}
#endif

UDPServer::UDPServer(char *pcIPAddr, unsigned int uiPort, int iMode)
{
	if (pcIPAddr != NULL)
	{
		strcpy_s(m_pcIP, UPDS_MAX_IP_LENGTH - 1, pcIPAddr);
		m_pcIP[UPDS_MAX_IP_LENGTH - 1] = 0;
	}

	m_uiPort = uiPort;
	m_iMode = iMode;

	m_pucBuffer = (unsigned char *)malloc(UDPS_BUFFER_SIZE);
}

int UDPServer::setSYSBuffer(MBUFFERSYSBuffer *pSYSBuffer)
{
	if (pSYSBuffer == NULL)
		return UDPS_EC_FAILIED;
	m_pSYSBuffer = pSYSBuffer;
	
	return UDPS_EC_OK;
}

int UDPServer::setFilePath(const char *pcFilePath)
{
	if (pcFilePath == NULL)
		return UDPS_EC_FAILIED;
	
	strcpy(m_pcFilePath, pcFilePath);
	return UDPS_EC_OK;
}

int UDPServer::start()
{
	WSADATA wsaData;
	WORD sockVersion = MAKEWORD(2, 2);
	if (WSAStartup(sockVersion, &wsaData) != 0)
	{
		return UDPS_EC_FAILIED;
	}

	m_s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_s == INVALID_SOCKET)
	{
		printf("socket error !\n");
		return UDPS_EC_FAILIED;
	}

	struct sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(m_uiPort);
	serAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	if (bind(m_s, (struct sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
	{
		printf("bind error !\n");
		return UDPS_EC_FAILIED;
	}


	if (m_iMode == UDPS_WORK_MODE_DUMP2BUFFER || m_iMode == UDPS_WORK_MODE_DUMP2FILE)
	{
		int iRet = CTCreateThread(&m_thread, UDPServerExecute, this);
		if (iRet == -1)
		{
			printf("create thread failed\n");
			return UDPS_EC_FAILIED;
		}
	}

	return UDPS_EC_OK;
}

int UDPServer::stop()
{
	m_iStop = 1;
	while (m_iRunning)
	{
		Sleep(1);
	}
	
	CTCloseThreadHandle(&m_thread);
	return UDPS_EC_OK;
}

int UDPServer::execute()
{
	FILE *fpOutput = NULL;
	m_iRunning = 1;
	m_iStop = 0;

	if (m_iMode == UDPS_WORK_MODE_DUMP2FILE || m_iMode == UDPS_WORK_MODE_DUMP2BUFFER)
	{
		fpOutput = fopen(m_pcFilePath, "wb+");
		if (fpOutput == NULL)
		{
			printf("Couldn't open file\n");
			return UDPS_EC_FAILIED;
		}
	}

	while (!m_iStop)
	{
		int iRet = recv(m_pucBuffer, UDPS_BUFFER_SIZE);
		if (iRet > 0)
		{
			printf("got packet, len = %d\n", iRet);
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

				MBUFFERSYSExtend(iRet, m_pSYSBuffer);

				// test write to file
				if (iRet != fwrite(m_pucBuffer, 1, iRet, fpOutput))
				{
					printf("fwrite failed\n");
					break;
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

	if (m_iMode == UDPS_WORK_MODE_DUMP2FILE)
	{
		fclose(fpOutput);
	}


	return UDPS_EC_OK;
}

int UDPServer::send(unsigned char * pucBuffer, unsigned int iBufferSize)
{
	return 0;
}

// return the number of bytes received if succeed.
// or return the errcode of recvfrom
int UDPServer::recv(unsigned char * pucBuffer, unsigned int iBufferSize)
{
	struct sockaddr_in remoteAddr;
	int iAddrLen = sizeof(remoteAddr);
	return recvfrom(m_s, (char *)m_pucBuffer, UDPS_BUFFER_SIZE, 0, (sockaddr *)&remoteAddr, &iAddrLen);
}

int UDPServer::getPeerAddrInfo(struct sockaddr * peerSockAddr, int *piAddrLen)
{
	return UDPS_EC_OK;
}

UDPServer::~UDPServer()
{
	SAFERELEASE(m_pucBuffer);
	closesocket(m_s);
}