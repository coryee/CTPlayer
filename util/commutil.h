#ifndef _COMMON_UTIL_H_
#define _COMMON_UTIL_H_

/*
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
# define OS_WINDOWS
#elif defined(linux) || defined(__linux) || defined(__linux__)
# define OS_LINUX
#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
# define OS_MACOSX
#endif
*/

#if defined(__cplusplus)
extern "C"
{
#endif

#ifdef _WIN32
#include <windows.h>
#else
#endif

extern void CTSleep(int msec);

extern long long CTGetCurrentMicroseconds();

#if defined(__cplusplus)
}
#endif
#endif