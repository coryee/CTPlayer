#ifndef _CTMUTEX_h
#define _CTMUTEX_h

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  Synchronization functions which can time out return this value
 *  if they time out.
 */
#define CTMUTEX_TIMEDOUT		1

/**
 *  This is the timeout value which corresponds to never time out.
 */
#define CTMUTEX_MAXWAIT   (~(Uint32)0)


struct CTMutex;
typedef struct CTMutex CTMutex;

/**
 *  Create a mutex, initialized unlocked.
 */
extern CTMutex *CTMutexCreate(void);

/**
 *  Lock the mutex.
 *
 *  \return 0, or -1 on error.
 */
extern int CTMutexLock(CTMutex * mutex);

/**
 *  Try to lock the mutex
 *
 *  \return 0, CTMUTEX_EC_TIMEDOUT, or -1 on error
 */
extern int CTMutexTryLock(CTMutex * mutex);

/**
 *  Unlock the mutex.
 *
 *  \return 0, or -1 on error.
 *
 *  \warning It is an error to unlock a mutex that has not been locked by
 *           the current thread, and doing so results in undefined behavior.
 */
extern int CTMutexUnlock(CTMutex * mutex);

/**
 *  Destroy a mutex.
 */
extern void CTMutexDestroy(CTMutex * mutex);

#ifdef __cplusplus
}
#endif

#endif 
