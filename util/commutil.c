#include "commutil.h"
#include <string.h>

char *CTStrncpy(char *dst, const char *src, unsigned int size)
{
	strncpy(dst, src, size);
	dst[size - 1] = 0;

	return dst;
}

void CTSleep(int msec)
{
#ifdef _WIN32
	Sleep(msec);
#else
	usleep(msec * 1000);
#endif
}

#ifdef _WIN32
// struct timeval {
//   long tv_sec, tv_usec;
// };

// Based on: http://www.google.com/codesearch/p?hl=en#dR3YEbitojA/os_win32.c&q=GetSystemTimeAsFileTime%20license:bsd
// See COPYING for copyright information.
int gettimeofday(struct timeval *tv, void* tz) 
{
#define EPOCHFILETIME (116444736000000000ULL)
  FILETIME ft;
  LARGE_INTEGER li;
  unsigned long long tt;

  GetSystemTimeAsFileTime(&ft);
  li.LowPart = ft.dwLowDateTime;
  li.HighPart = ft.dwHighDateTime;
  tt = (li.QuadPart - EPOCHFILETIME) / 10;
  tv->tv_sec = (long)(tt / 1000000);
  tv->tv_usec = (long)tt % 1000000;

  return 0;
}
#endif

unsigned long CTGetMilliSeconds()
{
	return timeGetTime();
}

long long CTGetMicroseconds() 
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (long long)(tv.tv_sec) * 1000000 + tv.tv_usec;
}

char *CTStrncpy(char *dst, char *src, int size)
{
	strncpy(dst, src, size);
	dst[size - 1] = 0;
	return dst;
}
