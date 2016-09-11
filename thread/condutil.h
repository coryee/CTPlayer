#ifndef _CTCOND_h
#define _CTCOND_h

//note: the codes below are copied from SDL source code;

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif
#include "mutexutil.h"

#define CTCOND_TIMEDOUT		1
#define CTCOND_MAXWAIT		(~(Uint32)0)

#ifndef NULL
#define NULL 0
#endif

typedef unsigned int Uint32;

/**
 *  \name Condition variable functions
 */
/* @{ */

/* The condition variable structure, defined in cond_linux.c/cond_win.c */
struct CTCond;
typedef struct CTCond CTCond;

/**
 *  Create a condition variable.
 *
 *  Typical use of condition variables:
 *
 *  Thread A:
 *    CTLockMutex(lock);
 *    while ( ! condition ) {
 *        CTCondWait(cond, lock);
 *    }
 *    CTUnlockMutex(lock);
 *
 *  Thread B:
 *    CTLockMutex(lock);
 *    ...
 *    condition = true;
 *    ...
 *    CTCondSignal(cond);
 *    CTUnlockMutex(lock);
 *
 *  There is some discussion whether to signal the condition variable
 *  with the mutex locked or not.  There is some potential performance
 *  benefit to unlocking first on some platforms, but there are some
 *  potential race conditions depending on how your code is structured.
 *
 *  In general it's safer to signal the condition variable while the
 *  mutex is locked.
 */
extern CTCond *CTCreateCond(void);

/**
 *  Destroy a condition variable.
 */
extern void CTDestroyCond(CTCond * cond);

/**
 *  Restart one of the threads that are waiting on the condition variable.
 *
 *  \return 0 or -1 on error.
 */
extern int CTCondSignal(CTCond * cond);

/**
 *  Restart all threads that are waiting on the condition variable.
 *
 *  \return 0 or -1 on error.
 */
extern int CTCondBroadcast(CTCond * cond);

/**
 *  Wait on the condition variable, unlocking the provided mutex.
 *
 *  \warning The mutex must be locked before entering this function!
 *
 *  The mutex is re-locked once the condition variable is signaled.
 *
 *  \return 0 when it is signaled, or -1 on error.
 */
extern int CTCondWait(CTCond * cond, CTMutex * mutex);

/**
 *  Waits for at most \c ms milliseconds, and returns 0 if the condition
 *  variable is signaled, ::CTCOND_TIMEDOUT if the condition is not
 *  signaled in the allotted time, and -1 on error.
 *
 *  \warning On some platforms this function is implemented by looping with a
 *           delay of 1 ms, and so should be avoided if possible.
 */
extern int CTCondWaitTimeout(CTCond * cond, CTMutex * mutex, Uint32 ms);

/* @} *//* Condition variable functions */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif 

/* vi: set ts=4 sw=4 expandtab: */
