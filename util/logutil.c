#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h> 
#include <io.h>  
#include <fcntl.h> 
#else 
#include <time.h>  
#include <sys/time.h> 
#endif

#include "mutexutil.h"

#ifdef _WIN32
#define snprintf sprintf_s
#endif

#include "logutil.h"

#define CTLOG_FILE_NAME_DEFAULE	"default.log"
#define CTLOG_MAX_BUFFER_SIZE	512

struct CTLogContext {
	CTLogLevel	level;
	CTLogMode	mode;
	FILE		*fpLog;
	int			bInit;
	CTMutex		*pMutex;
	char		pBuffer[CTLOG_MAX_BUFFER_SIZE];
};

static CTLogContext gLogContext;
static char CTLogLevelString[][16] = {
	"DEBUG", 
	"INFO",
	"ERROR"
};

static void CTLogPrint(CTLogLevel level, char *pcModuleName, char *pcMessage);
static void CTLogMessage(CTLogLevel level, char *pcModuleName, char *pcFormat, va_list ap);

#ifdef _WIN32
void CTLogOpenConsole()
{
	AllocConsole();
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	int hCrt = _open_osfhandle((long)handle, _O_TEXT);
	FILE *hf = _fdopen(hCrt, "w");
	*stdout = *hf;
}
#endif

int CTLogInit(CTLogLevel level, CTLogMode mode, char *pcFileName)
{
	CTLogContext *pLogCtx = &gLogContext;
	const char *pcLogFielName = CTLOG_FILE_NAME_DEFAULE;
	pLogCtx->bInit = 1;
	pLogCtx->level = level;
	pLogCtx->mode = mode;

	pLogCtx->pMutex = CTCreateMutex();
	if (pLogCtx->pMutex == NULL)
		return CTLOG_EC_FAILURE;
	if (pLogCtx->mode == CTLOG_MODE_LOG2FILE)
	{
		// close file handle if LogContext has been initialized
		if (pLogCtx->fpLog != NULL)
			fclose(pLogCtx->fpLog);
		
		if (pcFileName != NULL && pcFileName[0] != 0)
			pcLogFielName = pcFileName;
		pLogCtx->fpLog = fopen(pcLogFielName, "wb");
		if (pLogCtx->fpLog == NULL)
			return CTLOG_EC_FAILURE;
	}
	else
	{
#ifdef _WIN32
		CTLogOpenConsole();
#endif // _WIN32
		pLogCtx->fpLog = stdout;
	}

	return CTLOG_EC_OK;
}

int CTLogDeInit()
{
	CTLogContext *pLogCtx = &gLogContext;

	CTDestroyMutex(pLogCtx->pMutex);
	if (pLogCtx->mode == CTLOG_MODE_LOG2FILE)
	{
		// close file handle if LogContext has been initialized
		if (pLogCtx->fpLog != NULL)
			fclose(pLogCtx->fpLog);
	}

	return CTLOG_EC_OK;
}

void CTLogDebugEx(char *pcModuleName, char *pcFormat, ...)
{
	va_list ap;
	va_start(ap, pcFormat);
	CTLogMessage(CTLOG_LEVEL_DEBUG, pcModuleName, pcFormat, ap);
	va_end(ap);
}

void CTLogDebug(char *pcFormat, ...)
{
	va_list ap;
	va_start(ap, pcFormat);
	CTLogMessage(CTLOG_LEVEL_DEBUG, NULL, pcFormat, ap);
	va_end(ap);
}

void CTLogInfoEx(char *pcModuleName, char *pcFormat, ...)
{
	va_list ap;
	va_start(ap, pcFormat);
	CTLogMessage(CTLOG_LEVEL_INFO, pcModuleName, pcFormat, ap);
	va_end(ap);
}

void CTLogInfo(char *pcFormat, ...)
{
	va_list ap;
	va_start(ap, pcFormat);
	CTLogMessage(CTLOG_LEVEL_INFO, NULL, pcFormat, ap);
	va_end(ap);
}

void CTLogErrorEx(char *pcModuleName, char *pcFormat, ...)
{
	va_list ap;
	va_start(ap, pcFormat);
	CTLogMessage(CTLOG_LEVEL_ERROR, pcModuleName, pcFormat, ap);
	va_end(ap);
}

void CTLogError(char *pcFormat, ...)
{
	va_list ap;
	va_start(ap, pcFormat);
	CTLogMessage(CTLOG_LEVEL_ERROR, NULL, pcFormat, ap);
	va_end(ap);
}


// private methods
static void CTLogGetCurrentTime(char *pcCurTime, int iSize)
{
#ifdef _WIN32
	SYSTEMTIME sys;
	
	pcCurTime[0] = 0;
	GetLocalTime(&sys);
	snprintf(pcCurTime, iSize, "%4d-%02d-%02d %02d:%02d:%02d.%03d", sys.wYear, sys.wMonth, sys.wDay, sys.wHour, sys.wMinute, sys.wSecond, sys.wMilliseconds);
#else 
	struct timeval		tv;
	struct timezone		tz;
	struct tm			*p;

	pcCurTime[0] = 0;
	gettimeofday(&tv, &tz);
	p = localtime(&tv.tv_sec);
	snprintf(pcCurTime, iSize, "%4d-%02d-%02d %02d:%02d:%02d.%03d", 
		1900 + p->tm_year,
		1 + p->tm_mon, 
		p->tm_mday, 
		p->tm_hour, 
		p->tm_min, 
		p->tm_sec, 
		tv.tv_usec);
#endif
}

static void CTLogPrint(CTLogLevel level, char *pcModuleName, char *pcMessage)
{
	CTLogContext *pLogCtx = &gLogContext;
	char pcCurTime[32];

	CTLogGetCurrentTime(pcCurTime, 32);
	//TODO: need to use mutex to protect
	if (pcModuleName == NULL || pcModuleName[0] == 0)
		fprintf(pLogCtx->fpLog, "%s [%s] %s\r\n", pcCurTime, CTLogLevelString[level], pcMessage);
	else
		fprintf(pLogCtx->fpLog, "%s [%s] <%s> %s\r\n", pcCurTime, CTLogLevelString[level], pcModuleName, pcMessage);
	fflush(pLogCtx->fpLog);
}

static void CTLogMessage(CTLogLevel level, char *pcModuleName, char *pcFormat, va_list ap)
{
	CTLogContext *pLogCtx = &gLogContext;
	char *pBuffer = pLogCtx->pBuffer;

	if (level < 0 || level >= CTLOG_NUM_LOG_LEVELS)
		return;

	if (level < pLogCtx->level)
		return;

	CTMutexLock(pLogCtx->pMutex);
	_vsnprintf(pBuffer, sizeof(pBuffer), pcFormat, ap);
	if (pBuffer[strlen(pBuffer) - 1] == '\n')
		pBuffer[strlen(pBuffer) - 1] = 0;

	CTLogPrint(level, pcModuleName, pBuffer);
	CTMutexUnlock(pLogCtx->pMutex);
}