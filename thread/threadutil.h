#ifndef _THREAD_UTIL_H_
#define _THREAD_UTIL_H_
#if defined(__cplusplus)
extern "C"
{
#endif
#include <stdio.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <pthread.h>
#endif

#define INVALID_THREAD  NULL

#ifdef _WIN32
typedef HANDLE CTThreadHandle ;
typedef DWORD (*CTThreadFunc)(void *);
#else
typedef pthread_t CTThreadHandle;
typedef int* (*CTThreadFunc)(void *);
#endif


// 0 success; -1 failed
extern int CTCreateThread(CTThreadHandle *handle, CTThreadFunc func, void *arg);
extern int CTWaitThread(CTThreadHandle handle);
extern void CTCloseThreadHandle(CTThreadHandle handle);

#if defined(__cplusplus)
}
#endif
#endif
