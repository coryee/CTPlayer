#ifndef _COMMON_UTIL_H_
#define _COMMON_UTIL_H_

#if defined(__cplusplus)
extern "C"
{
#endif

#ifdef _WIN32
#include <windows.h>
#else
#endif

void CTSleep(int msec);

#if defined(__cplusplus)
}
#endif
#endif