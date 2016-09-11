#ifndef _CTSEM_h
#define _CTSEM_h

//note: the codes below are copied from SDL source code;

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif
#define CTSEM_TIMEDOUT		1
#define CTSEM_MAXWAIT			(~(Uint32)0)

typedef unsigned int Uint32;

/**
 *  \name Semaphore functions
 */
/* @{ */

/* The semaphore structure, defined in sem_linux/sem_win.c */
struct CTSemaphore;
typedef struct CTSemaphore CTSemaphore;

/**
 *  Create a semaphore, initialized with value, returns NULL on failure.
 */
extern CTSemaphore *CTCreateSemaphore(Uint32 initial_value);

/**
 *  Destroy a semaphore.
 */
extern void CTDestroySemaphore(CTSemaphore * sem);

/**
 *  This function suspends the calling thread until the semaphore pointed
 *  to by \c sem has a positive count. It then atomically decreases the
 *  semaphore count.
 */
extern int CTSemWait(CTSemaphore * sem);

/**
 *  Non-blocking variant of CTSemWait().
 *
 *  \return 0 if the wait succeeds, ::SDLSEM_TIMEDOUT if the wait would
 *          block, and -1 on error.
 */
extern int CTSemTryWait(CTSemaphore * sem);

/**
 *  Variant of CTSemWait() with a timeout in milliseconds.
 *
 *  \return 0 if the wait succeeds, ::CTSEM_TIMEDOUT if the wait does not
 *          succeed in the allotted time, and -1 on error.
 *
 *  \warning On some platforms this function is implemented by looping with a
 *           delay of 1 ms, and so should be avoided if possible.
 */
extern int CTSemWaitTimeout(CTSemaphore * sem, Uint32 ms);

/**
 *  Atomically increases the semaphore's count (not blocking).
 *
 *  \return 0, or -1 on error.
 */
extern int CTSemPost(CTSemaphore * sem);

/**
 *  Returns the current count of the semaphore.
 */
extern Uint32 CTSemValue(CTSemaphore * sem);

/* @} *//* Semaphore functions */


#if 0

/**
 *  \name Condition variable functions
 */
/* @{ */

/* The SDL condition variable structure, defined in SDL_syscond.c */
struct SDL_cond;
typedef struct SDL_cond SDL_cond;

/**
 *  Create a condition variable.
 *
 *  Typical use of condition variables:
 *
 *  Thread A:
 *    SDL_LockMutex(lock);
 *    while ( ! condition ) {
 *        SDL_CondWait(cond, lock);
 *    }
 *    SDL_UnlockMutex(lock);
 *
 *  Thread B:
 *    SDL_LockMutex(lock);
 *    ...
 *    condition = true;
 *    ...
 *    SDL_CondSignal(cond);
 *    SDL_UnlockMutex(lock);
 *
 *  There is some discussion whether to signal the condition variable
 *  with the mutex locked or not.  There is some potential performance
 *  benefit to unlocking first on some platforms, but there are some
 *  potential race conditions depending on how your code is structured.
 *
 *  In general it's safer to signal the condition variable while the
 *  mutex is locked.
 */
extern DECLSPEC SDL_cond *SDLCALL SDL_CreateCond(void);

/**
 *  Destroy a condition variable.
 */
extern DECLSPEC void SDLCALL SDL_DestroyCond(SDL_cond * cond);

/**
 *  Restart one of the threads that are waiting on the condition variable.
 *
 *  \return 0 or -1 on error.
 */
extern DECLSPEC int SDLCALL SDL_CondSignal(SDL_cond * cond);

/**
 *  Restart all threads that are waiting on the condition variable.
 *
 *  \return 0 or -1 on error.
 */
extern DECLSPEC int SDLCALL SDL_CondBroadcast(SDL_cond * cond);

/**
 *  Wait on the condition variable, unlocking the provided mutex.
 *
 *  \warning The mutex must be locked before entering this function!
 *
 *  The mutex is re-locked once the condition variable is signaled.
 *
 *  \return 0 when it is signaled, or -1 on error.
 */
extern DECLSPEC int SDLCALL SDL_CondWait(SDL_cond * cond, SDL_mutex * mutex);

/**
 *  Waits for at most \c ms milliseconds, and returns 0 if the condition
 *  variable is signaled, ::SDL_MUTEX_TIMEDOUT if the condition is not
 *  signaled in the allotted time, and -1 on error.
 *
 *  \warning On some platforms this function is implemented by looping with a
 *           delay of 1 ms, and so should be avoided if possible.
 */
extern DECLSPEC int SDLCALL SDL_CondWaitTimeout(SDL_cond * cond,
                                                SDL_mutex * mutex, Uint32 ms);

/* @} *//* Condition variable functions */

#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _SDL_mutex_h */

/* vi: set ts=4 sw=4 expandtab: */
