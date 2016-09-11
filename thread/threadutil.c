#include "threadutil.h"

#include <stdlib.h>
#ifdef _WIN32
#include <malloc.h>
#endif

int CTCreateThread(CTThreadHandle *handle, CTThreadFunc func, void *arg)
{
    CTThreadHandle tmpHandle;
    int iRet = 0;
#ifdef _WIN32
	DWORD dwThreadId;
    tmpHandle = CreateThread(NULL,				// default security attributes
		0,										// use default stack size  
		(LPTHREAD_START_ROUTINE)func,			// thread function 
		arg,									// argument to thread function 
		0,										// use default creation flags 
		&dwThreadId);

    if (handle == NULL)
        iRet = -1;
#else
	iRet = pthread_create(handle, NULL, func, arg);
	if (iRet != 0)
        iRet = -1;

#endif // _WIN32

    if (handle != NULL)
    {
        if (iRet == 0)
            *handle = tmpHandle;
        else
            *handle = INVALID_THREAD;
    }

    return iRet;
}

int CTWaitThread(CTThreadHandle handle)
{
#ifdef _WIN32
	WaitForSingleObject(handle, INFINITE);
#endif
	return 0;
}

void CTCloseThreadHandle(CTThreadHandle handle)
{
    if (handle == INVALID_THREAD)
        return;
#ifdef _WIN32
	CloseHandle(handle);
#else
	pthread_detach(handle);
#endif
}
