#include "semutil.h"
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

/* Wrapper around POSIX 1003.1b semaphores */

struct CTSemaphore
{
    sem_t sem;
};

/* Create a semaphore, initialized with value */
CTSemaphore *CTCreateSemaphore(Uint32 initial_value)
{
    CTSemaphore *sem = (CTSemaphore *) malloc(sizeof(CTSemaphore));
    if (sem) {
        if (sem_init(&sem->sem, 0, initial_value) < 0) {
            free(sem);
            sem = NULL;
        }
    }
    return sem;
}

void CTDestroySemaphore(CTSemaphore * sem)
{
    if (sem) {
        sem_destroy(&sem->sem);
        free(sem);
    }
}

int CTSemTryWait(CTSemaphore * sem)
{
    int retval;

    if (!sem) {
        return -1;
    }
    retval = CTSEM_TIMEDOUT;
    if (sem_trywait(&sem->sem) == 0) {
        retval = 0;
    }
    return retval;
}

int CTSemWait(CTSemaphore * sem)
{
    int retval;

    if (!sem) {
        return -1;
    }

    retval = sem_wait(&sem->sem);
    if (retval < 0) {
        retval = -1;
    }
    return retval;
}

int CTSemWaitTimeout(CTSemaphore * sem, Uint32 timeout)
{
    int retval;
    struct timeval now;
    struct timespec ts_timeout;

    if (!sem) {
        return -1;
    }

    /* Try the easy cases first */
    if (timeout == 0) {
        return CTSemTryWait(sem);
    }
    if (timeout == CTSEM_MAXWAIT) {
        return CTSemWait(sem);
    }

    /* Setup the timeout. sem_timedwait doesn't wait for
    * a lapse of time, but until we reach a certain time.
    * This time is now plus the timeout.
    */
    gettimeofday(&now, NULL);

    /* Add our timeout to current time */
    now.tv_usec += (timeout % 1000) * 1000;
    now.tv_sec += timeout / 1000;

    /* Wrap the second if needed */
    if ( now.tv_usec >= 1000000 ) {
        now.tv_usec -= 1000000;
        now.tv_sec ++;
    }

    /* Convert to timespec */
    ts_timeout.tv_sec = now.tv_sec;
    ts_timeout.tv_nsec = now.tv_usec * 1000;

    /* Wait. */
    do {
        retval = sem_timedwait(&sem->sem, &ts_timeout);
    } while (retval < 0 && errno == EINTR);

    if (retval < 0) {
        if (errno == ETIMEDOUT) {
            retval = CTSEM_TIMEDOUT;
        } else {
			retval = -1;
        }
    }
    return retval;
}

Uint32 CTSemValue(CTSemaphore * sem)
{
    int ret = 0;
    if (sem) {
        sem_getvalue(&sem->sem, &ret);
        if (ret < 0) {
            ret = 0;
        }
    }
    return (Uint32) ret;
}

int CTSemPost(CTSemaphore * sem)
{
    int retval;

    if (!sem) {
        return -1;
    }

    retval = sem_post(&sem->sem);
    return retval;
}

/* vi: set ts=4 sw=4 expandtab: */
