#include "commutil.h"


void CTSleep(int msec)
{
#ifdef _WIN32
	Sleep(msec);
#else
	usleep(msec * 1000);
#endif
}