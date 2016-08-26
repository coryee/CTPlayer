#include <stdlib.h>
#ifdef _WIN32
#include <malloc.h>
#endif

#include "threadutil.h"

int CTCreateThread(ThreadHandle *handle, ThreadFunc func, void *arg)
{
#ifdef _WIN32
	DWORD dwThreadId;
	*handle = CreateThread(NULL,				// default security attributes 
		0,										// use default stack size  
		(LPTHREAD_START_ROUTINE)func,			// thread function 
		arg,									// argument to thread function 
		0,										// use default creation flags 
		&dwThreadId);

	if (*handle == NULL)
		return -1;
#else
	int iRet = pthread_create(handle, NULL, func, arg);
	if (iRet == -1)
		return -1;

#endif // _WIN32

	return 0;
}


void CTCloseThreadHandle(ThreadHandle *handle)
{
#ifdef _WIN32
	CloseHandle(*handle);
#endif
}