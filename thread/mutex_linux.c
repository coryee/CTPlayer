/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2013 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef _WIN32
#include <pthread.h>
#include <errno.h>

#include "mutexutil.h"


struct CTMutex
{
    pthread_mutex_t id;
} CTMutex;

CTMutex *CTCreateMutex(void)
{
	CTMutex *mutex;
    pthread_mutexattr_t attr;

    /* Allocate the structure */
	mutex = (CTMutex *)malloc(sizeof(*mutex));
    if (mutex) {
        pthread_mutexattr_init(&attr);
        if (pthread_mutex_init(&mutex->id, &attr) != 0) {
            printf("pthread_mutex_init() failed\n");
            free(mutex);
            mutex = NULL;
        }
    } 
    return (mutex);
}

void CTDestroyMutex(CTMutex* mutex)
{
    if (mutex) {
        pthread_mutex_destroy(&mutex->id);
        free(mutex);
    }
}

/* Lock the mutex */
int CTMutexLock(CTMutex * mutex)
{
    if (mutex == NULL) {
        return -1;
    }

    if (pthread_mutex_lock(&mutex->id) < 0) {
		printf("pthread_mutex_lock() failed\n");
		return -1;
    }
    return 0;
}

int CTMutexTryLock(CTMutex * mutex)
{
    int retval;

    if (mutex == NULL) {
        printf("Passed a NULL mutex\n");
		return -1;
    }

    retval = 0;
    if (pthread_mutex_trylock(&mutex->id) != 0) {
        if (errno == EBUSY) {
            retval = CTMUTEX_TIMEDOUT;
        } else {
            printf("pthread_mutex_trylock() failed \n");
			retval = -1;
        }
    }
    return retval;
}

int CTMutexUnlock(CTMutex * mutex)
{
    if (mutex == NULL) {
		printf("Passed a NULL mutex\n");
		return -1;
    }

    if (pthread_mutex_unlock(&mutex->id) < 0) {
        printf("pthread_mutex_unlock() failed\n");
		return -1;
    }

	return 0;
}

#endif