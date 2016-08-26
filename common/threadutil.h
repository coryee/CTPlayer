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


#ifdef _WIN32
typedef HANDLE ThreadHandle ;
typedef DWORD (*ThreadFunc)(void *);

#else
typedef pthread_t ThreadHandle;
typedef int* (*ThreadFunc)(void *);
#endif


// 0 success; -1 failed
int CTCreateThread(ThreadHandle *handle, ThreadFunc func, void *arg);
void CTCloseThreadHandle(ThreadHandle *handle);

#if defined(__cplusplus)
}
#endif
#endif