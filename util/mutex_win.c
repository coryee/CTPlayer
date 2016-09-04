#include "mutexutil.h"

#if _WIN32

#include <Windows.h>
/* Mutex functions using the Win32 API */

struct CTMutex
{
    CRITICAL_SECTION cs;
};

/* Create a mutex */
CTMutex *CTMutexCreate(void)
{
	CTMutex *mutex;

    /* Allocate mutex memory */
	mutex = (CTMutex *)malloc(sizeof(*mutex));
    if (mutex) {
        /* Initialize */
        /* On SMP systems, a non-zero spin count generally helps performance */
        InitializeCriticalSectionAndSpinCount(&mutex->cs, 2000);
    } 
    return (mutex);
}

/* Free the mutex */
void CTMutexDestroy(CTMutex * mutex)
{
    if (mutex) {
        DeleteCriticalSection(&mutex->cs);
        free(mutex);
    }
}

/* Lock the mutex */
int CTMutexLock(CTMutex * mutex)
{
    if (mutex == NULL) {
        return -1;
    }

    EnterCriticalSection(&mutex->cs);
    return (0);
}

/* TryLock the mutex */
int CTMutexTryLock(CTMutex * mutex)
{
	int retval = 0;
    if (mutex == NULL) {
		return -1;
    }

    if (TryEnterCriticalSection(&mutex->cs) == 0) {
        retval = CTMUTEX_TIMEDOUT;
    }
    return retval;
}

/* Unlock the mutex */
int CTMutexUnlock(CTMutex * mutex)
{
    if (mutex == NULL) {
        return -1;
    }

    LeaveCriticalSection(&mutex->cs);
	return 0;
}

#endif /* _WIN32 */
