#ifndef _LOG_UTIL_H_
#define _LOG_UTIL_H_
#if defined(__cplusplus)
extern "C"
{
#endif
#include <stdio.h>
#include <stdarg.h>
#include "mutexutil.h"

#define CTLOG_MAX_PATH     256
#define CTLOG_MAX_BUFFER   512
#define CTLOG_MAX_FILE_SIZE 0xFFFFFFFF

#define CTLOG_EC_OK			0
#define CTLOG_EC_FAILURE	-1

typedef enum
{
	CTLOG_LEVEL_DEBUG = 0,
	CTLOG_LEVEL_INFO,
	CTLOG_LEVEL_ERROR,
	CTLOG_NUM_LOG_LEVELS,
} CTLogLevel;

typedef enum
{
	CTLOG_MODE_LOG2CONSOLE,
	CTLOG_MODE_LOG2FILE,
} CTLogMode;

typedef struct CTLogContext {
    CTLogLevel      level;
    CTLogMode       mode;
    FILE            *fpLog;
    char            pcFileName[CTLOG_MAX_PATH];
    unsigned int    logFileSize;
    unsigned int    curFileSize;
    int             numFilesToReserve;
    char            **pFileNames;
    int             curFileIdx;
    int             bInit;
    CTMutex         *pMutex;
    char            pBuffer[CTLOG_MAX_BUFFER];
} CTLogContext;


// return CTLOG_EC_OK if succeed, or CTLOG_EC_FAILURE if failed
extern int CTLogInit(CTLogContext *pLogContext, CTLogLevel level, CTLogMode mode, const char *pcFileName);
extern int CTLogDeInit(CTLogContext *pLogContext);
extern int CTLogSetLogFileSize(CTLogContext *pLogContext, unsigned int size);
extern int CTLogSetLogFile(CTLogContext *pLogContext, const char *pcFilePath);
extern int CTLogSetNumLogFilesToReserve(CTLogContext *pLogContext, int iNumFiles);
extern void CTLogDebugEx(CTLogContext *pLogContext, const char *pcModuleName, const char *pcFormat, ...);
extern void CTLogDebug(CTLogContext *pLogContext, const char *pcFormat, ...);
extern void CTLogInfoEx(CTLogContext *pLogContext, const char *pcModuleName, const char *pcFormat, ...);
extern void CTLogInfo(CTLogContext *pLogContext, const char *pcFormat, ...);
extern void CTLogErrorEx(CTLogContext *pLogContext, const char *pcModuleName, const char *pcFormat, ...);
extern void CTLogError(CTLogContext *pLogContext, const char *pcFormat, ...);

// CTLogSXXX will use the singleton log context
// return CTLOG_EC_OK if succeed, or CTLOG_EC_FAILURE if failed
extern int CTLogSInit(CTLogLevel level, CTLogMode mode, char *pcFileName);
extern int CTLogSDeInit();
extern void CTLogSDebugEx(const char *pcModuleName, const char *pcFormat, ...);
extern void CTLogSDebug(const char *pcFormat, ...);
extern void CTLogSInfoEx(const char *pcModuleName, const char *pcFormat, ...);
extern void CTLogSInfo(const char *pcFormat, ...);
extern void CTLogSErrorEx(const char *pcModuleName, const char *pcFormat, ...);
extern void CTLogSError(const char *pcFormat, ...);

// use case 1
//#define mylogfunc(fmt, ...)	CTLogDebugEx("main", fmt, __VA_ARGS__)

// user case 2
//#define CTUserLogFunc CTLogFunc;	
//CTLOGSETMODULE("usermodule");
// #define CTLOGSETMODULE(modulename) static char *pcCTLogMuduleName = modulename
// #define CTLogLevelFunc(level, fmt, ...) CTLogPrint(level, pcCTLogMuduleName, fmt, ##__VA_ARGS__);
// #define CTLogFunc(fmt, ...)				CTLogPrint(CTLOG_LEVEL_DEBUG, pcCTLogMuduleName, fmt, ##__VA_ARGS__);

#if defined(__cplusplus)
}
#endif
#endif
