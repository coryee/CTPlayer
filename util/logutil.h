#ifndef _LOG_UTIL_H_
#define _LOG_UTIL_H_
#if defined(__cplusplus)
extern "C"
{
#endif
#include <stdio.h>
#include <stdarg.h>


#define CTLOG_EC_OK			0
#define CTLOG_EC_FAILURE	-1

	typedef enum 
	{
		CTLOG_LEVEL_DEBUG = 0,
		CTLOG_LEVEL_INFO,
	} CTLogLevel;

	typedef enum 
	{
		CTLOG_MODE_LOG2CONSOLE,
		CTLOG_MODE_LOG2FILE,
	} CTLogMode; 

	typedef struct CTLogContext {
		CTLogLevel	level;
		CTLogMode	mode;
		FILE *fpLog;
		int bInit;
	} CTLogContext;


	int CTLogInit(CTLogLevel level, CTLogMode mode, char *pcFileName);
// 0 success; -1 failed
	int CTLogPrint(CTLogLevel level, char *pcModuleName, char *pcFormat, ...);

// use case 1
//#define mylog(fmt, ...)	CTLogPrint(CTLOG_LEVEL_DEBUG, "main", fmt, __VA_ARGS__)

// user case 2
//#define CTUserLogFunc CTLogFunc;	
//CTLOGSETMODULE("usermodule");
#define CTLOGSETMODULE(modulename) static char *pcCTLogMuduleName = modulename
#define CTLogLevelFunc(level, fmt, ...) CTLogPrint(level, pcCTLogMuduleName, fmt, ##__VA_ARGS__);
#define CTLogFunc(fmt, ...)				CTLogPrint(CTLOG_LEVEL_DEBUG, pcCTLogMuduleName, fmt, ##__VA_ARGS__);

#if defined(__cplusplus)
}
#endif
#endif