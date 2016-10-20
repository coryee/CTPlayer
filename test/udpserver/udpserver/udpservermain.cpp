#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <time.h>
#include "udpserver.h"

#pragma comment(lib, "ws2_32.lib") 

#define RECV_BUFFER_SIZE (1000 * 1000)
static char recvBuffer[RECV_BUFFER_SIZE];

int udpserver(int argc, char* argv[])
{
	FILE *fpOutput;

	fpOutput = fopen("output.h264", "wb+");
	
	WSADATA wsaData;
	WORD sockVersion = MAKEWORD(2, 2);
	if (WSAStartup(sockVersion, &wsaData) != 0)
	{
		return 0;
	}

	SOCKET serSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (serSocket == INVALID_SOCKET)
	{
		printf("socket error !");
		return 0;
	}

	struct sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(10001);
	serAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	if (bind(serSocket, (struct sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
	{
		printf("bind error !");
		closesocket(serSocket);
		return 0;
	}

	struct sockaddr_in remoteAddr;
	int nAddrLen = sizeof(remoteAddr);
	time_t startTime, curTime;
	time(&startTime);
	while (true)
	{
		time(&curTime);
		if (curTime - startTime > 30)
		{
			break;
		}
		
		int ret = recvfrom(serSocket, recvBuffer, RECV_BUFFER_SIZE, 0, (sockaddr *)&remoteAddr, &nAddrLen);
		if (ret > 0)
		{
			
			printf("got packet, len = %d\n", ret);
			// recvBuffer[ret] = 0;
			if (ret != fwrite(recvBuffer, 1, ret, fpOutput))
			{
				printf("fwrite failed\n");
				break;
			}
		}
		else
		{
			WSAEINTR;
			printf("recvfrom failed, errno = %d\n", WSAGetLastError());
			break;
		}
	}
	closesocket(serSocket);
	WSACleanup();
	return 0;
}


int main()
{
// 	unsigned char pcBuffer[4096];
// 	FILE *faudio = fopen("E:\\testfile\\videoplayback_audio.aac", "rb");
// 	FILE *foutput = fopen("out.aac", "wb+");
// 	fseek(faudio, 1024 * 100, SEEK_SET);
// 	while (!feof(faudio))
// 	{
// 		int ret = fread(pcBuffer, 1, 4096, faudio);
// 		if (ret > 0)
// 		{
// 			if (ret != fwrite(pcBuffer, 1, ret, foutput))
// 				break;
// 		}
// 		else
// 		{
// 			break;
// 		}
// 	}
// 
// 	fclose(foutput);
// 	fclose(faudio);


	printf("helloworld\n");

	// dump to file
	UDPServer udps("10.0.70.189", 20000, UDPS_WORK_MODE_DUMP2FILE);
	udps.SetFilePath("udpoutput.h264");

	// dump to buffer
// 	MBUFFERSYSBuffer sysbuffer;
// 	MBUFFERSYSBufferInit(0, 0, 0, 100 * 1024 * 1024, &sysbuffer);
// 	UDPServer udps("10.0.70.189", 10001, UDPS_WORK_MODE_DUMP2BUFFER);
// 	udps.setSYSBuffer(&sysbuffer);

//	UDPServer udps("10.0.70.189", 20000, UDPS_WORK_MODE_DEFAULT);
	udps.Start();

	Sleep(4 * 60 * 1000);
	udps.Stop();

	return 0;
}