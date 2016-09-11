#include "condutil.h"

#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

struct CTCond
{
    pthread_cond_t cond;
};

/* Create a condition variable */
CTCond *CTCreateCond(void)
{
    CTCond *cond;

    cond = (CTCond *) malloc(sizeof(CTCond));
    if (cond) {
        if (pthread_cond_init(&cond->cond, NULL) < 0) {
            free(cond);
            cond = NULL;
        }
    }
    return (cond);
}

/* Destroy a condition variable */
void CTDestroyCond(CTCond * cond)
{
    if (cond) {
        pthread_cond_destroy(&cond->cond);
        free(cond);
    }
}

/* Restart one of the threads that are waiting on the condition variable */
int CTCondSignal(CTCond * cond)
{
    int retval;

    if (!cond) {
        return -1;
    }

    retval = 0;
    if (pthread_cond_signal(&cond->cond) != 0) {
        return -1;
    }
    return retval;
}

/* Restart all threads that are waiting on the condition variable */
int CTCondBroadcast(CTCond * cond)
{
    int retval;

    if (!cond) {
        return -1;
    }

    retval = 0;
    if (pthread_cond_broadcast(&cond->cond) != 0) {
        return -1;
    }
    return retval;
}

int CTCondWaitTimeout(CTCond * cond, CTMutex * mutex, Uint32 ms)
{
    int retval;
    struct timeval delta;
    struct timespec abstime;

    if (!cond) {
        return -1;
    }

    gettimeofday(&delta, NULL);

    abstime.tv_sec = delta.tv_sec + (ms / 1000);
    abstime.tv_nsec = (delta.tv_usec + (ms % 1000) * 1000) * 1000;
    if (abstime.tv_nsec > 1000000000) {
        abstime.tv_sec += 1;
        abstime.tv_nsec -= 1000000000;
    }

  tryagain:
    retval = pthread_cond_timedwait(&cond->cond, &mutex->id, &abstime);
    switch (retval) {
    case EINTR:
        goto tryagain;
        break;
    case ETIMEDOUT:
        retval = CTCOND_TIMEDOUT;
        break;
    case 0:
        break;
    default:
        retval = -1;
    }
    return retval;
}

/* Wait on the condition variable, unlocking the provided mutex.
   The mutex must be locked before entering this function!
 */
int CTCondWait(CTCond * cond, CTMutex * mutex)
{
    if (!cond) {
        return -1;
    } else if (pthread_cond_wait(&cond->cond, &mutex->id) != 0) {
        return -1;
    }
    return 0;
}