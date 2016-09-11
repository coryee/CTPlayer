#include "semutil.h"

//note: the codes below are copied from SDL source code;

#ifdef _WIN32

/* Semaphore functions using the Win32 API */
#include <windows.h>

struct CTSemaphore
{
    HANDLE id;
    LONG count;
};


/* Create a semaphore */
CTSemaphore *CTCreateSemaphore(Uint32 initial_value)
{
    CTSemaphore *sem;

    /* Allocate sem memory */
    sem = (CTSemaphore *) malloc(sizeof(*sem));
    if (sem) {
        /* Create the semaphore, with max value 32K */
        sem->id = CreateSemaphore(NULL, initial_value, 32 * 1024, NULL);
        sem->count = initial_value;
        if (!sem->id) {
            free(sem);
            sem = NULL;
        }
    }
    return (sem);
}

/* Free the semaphore */
void CTDestroySemaphore(CTSemaphore * sem)
{
    if (sem) {
        if (sem->id) {
            CloseHandle(sem->id);
            sem->id = 0;
        }
        free(sem);
    }
}

int CTSemWaitTimeout(CTSemaphore * sem, Uint32 timeout)
{
    int retval;
    DWORD dwMilliseconds;

    if (!sem) {
        return -1;
    }

    if (timeout == CTSEM_MAXWAIT) {
        dwMilliseconds = INFINITE;
    } else {
        dwMilliseconds = (DWORD) timeout;
    }
    switch (WaitForSingleObject(sem->id, dwMilliseconds)) {
    case WAIT_OBJECT_0:
        InterlockedDecrement(&sem->count);
        retval = 0;
        break;
    case WAIT_TIMEOUT:
		retval = CTSEM_TIMEDOUT;
        break;
    default:
        retval = -1;
        break;
    }
    return retval;
}

int CTSemTryWait(CTSemaphore * sem)
{
    return CTSemWaitTimeout(sem, 0);
}

int CTSemWait(CTSemaphore * sem)
{
    return CTSemWaitTimeout(sem, CTSEM_MAXWAIT);
}

/* Returns the current count of the semaphore */
Uint32 CTSemValue(CTSemaphore * sem)
{
    if (!sem) {
        return 0;
    }
    return (Uint32)sem->count;
}

int CTSemPost(CTSemaphore * sem)
{
    if (!sem) {
        return -1;
    }
    /* Increase the counter in the first place, because
     * after a successful release the semaphore may
     * immediately get destroyed by another thread which
     * is waiting for this semaphore.
     */
    InterlockedIncrement(&sem->count);
    if (ReleaseSemaphore(sem->id, 1, NULL) == FALSE) {
        InterlockedDecrement(&sem->count);      /* restore */
        return -1;
    }
    return 0;
}

#endif

/* vi: set ts=4 sw=4 expandtab: */
