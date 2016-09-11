#include "condutil.h"
#include <stdlib.h>

#include "semutil.h"
#include "mutexutil.h"

/* An implementation of condition variables using semaphores and mutexes */
/*
   This implementation borrows heavily from the BeOS condition variable
   implementation, written by Christopher Tate and Owen Smith.  Thanks!
 */

struct CTCond
{
    CTMutex *lock;
    int waiting;
    int signals;
    CTSemaphore *wait_sem;
    CTSemaphore *wait_done;
};

/* Create a condition variable */
CTCond *CTCreateCond(void)
{
    CTCond *cond;

    cond = (CTCond *) malloc(sizeof(CTCond));
    if (cond) {
        cond->lock = CTCreateMutex();
        cond->wait_sem = CTCreateSemaphore(0);
        cond->wait_done = CTCreateSemaphore(0);
        cond->waiting = cond->signals = 0;
        if (!cond->lock || !cond->wait_sem || !cond->wait_done) {
            CTDestroyCond(cond);
            cond = NULL;
        }
    } 
    return (cond);
}

/* Destroy a condition variable */
void CTDestroyCond(CTCond * cond)
{
    if (cond) {
        if (cond->wait_sem) {
            CTDestroySemaphore(cond->wait_sem);
        }
        if (cond->wait_done) {
            CTDestroySemaphore(cond->wait_done);
        }
        if (cond->lock) {
            CTDestroyMutex(cond->lock);
        }
        free(cond);
    }
}

/* Restart one of the threads that are waiting on the condition variable */
int
CTCondSignal(CTCond * cond)
{
    if (!cond) {
        return -1;
    }

    /* If there are waiting threads not already signalled, then
       signal the condition and wait for the thread to respond.
     */
	CTMutexLock(cond->lock);
    if (cond->waiting > cond->signals) {
        ++cond->signals;
        CTSemPost(cond->wait_sem);
        CTMutexUnlock(cond->lock);
        CTSemWait(cond->wait_done);
    } else {
        CTMutexUnlock(cond->lock);
    }

    return 0;
}

/* Restart all threads that are waiting on the condition variable */
int
CTCondBroadcast(CTCond * cond)
{
    if (!cond) {
        return 1;
    }

    /* If there are waiting threads not already signalled, then
       signal the condition and wait for the thread to respond.
     */
    CTMutexLock(cond->lock);
    if (cond->waiting > cond->signals) {
        int i, num_waiting;

        num_waiting = (cond->waiting - cond->signals);
        cond->signals = cond->waiting;
        for (i = 0; i < num_waiting; ++i) {
            CTSemPost(cond->wait_sem);
        }
        /* Now all released threads are blocked here, waiting for us.
           Collect them all (and win fabulous prizes!) :-)
         */
        CTMutexUnlock(cond->lock);
        for (i = 0; i < num_waiting; ++i) {
            CTSemWait(cond->wait_done);
        }
    } else {
        CTMutexUnlock(cond->lock);
    }

    return 0;
}

/* Wait on the condition variable for at most 'ms' milliseconds.
   The mutex must be locked before entering this function!
   The mutex is unlocked during the wait, and locked again after the wait.

Typical use:

Thread A:
    CTMutexLock(lock);
    while ( ! condition ) {
        CTCondWait(cond, lock);
    }
    CTMutexUnlock(lock);

Thread B:
    CTMutexLock(lock);
    ...
    condition = true;
    ...
    CTCondSignal(cond);
    CTMutexUnlock(lock);
 */
int CTCondWaitTimeout(CTCond * cond, CTMutex * mutex, Uint32 ms)
{
    int retval;

    if (!cond) {
        return -1;
    }

    /* Obtain the protection mutex, and increment the number of waiters.
       This allows the signal mechanism to only perform a signal if there
       are waiting threads.
     */
    CTMutexLock(cond->lock);
    ++cond->waiting;
    CTMutexUnlock(cond->lock);

    /* Unlock the mutex, as is required by condition variable semantics */
    CTMutexUnlock(mutex);

    /* Wait for a signal */
    if (ms == CTCOND_MAXWAIT) {
        retval = CTSemWait(cond->wait_sem);
    } else {
        retval = CTSemWaitTimeout(cond->wait_sem, ms);
    }

    /* Let the signaler know we have completed the wait, otherwise
       the signaler can race ahead and get the condition semaphore
       if we are stopped between the mutex unlock and semaphore wait,
       giving a deadlock.  See the following URL for details:
       http://web.archive.org/web/20010914175514/http://www-classic.be.com/aboutbe/benewsletter/volume_III/Issue40.html#Workshop
     */
    CTMutexLock(cond->lock);
    if (cond->signals > 0) {
        /* If we timed out, we need to eat a condition signal */
        if (retval > 0) {
            CTSemWait(cond->wait_sem);
        }
        /* We always notify the signal thread that we are done */
        CTSemPost(cond->wait_done);

        /* Signal handshake complete */
        --cond->signals;
    }
    --cond->waiting;
    CTMutexUnlock(cond->lock);

    /* Lock the mutex, as is required by condition variable semantics */
    CTMutexLock(mutex);

    return retval;
}

/* Wait on the condition variable forever */
int CTCondWait(CTCond * cond, CTMutex * mutex)
{
    return CTCondWaitTimeout(cond, mutex, CTCOND_MAXWAIT);
}

