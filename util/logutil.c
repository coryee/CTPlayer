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

#ifdef _WIN32
#define snprintf sprintf_s
#endif

#include "logutil.h"

struct CTLogContext {
	CTLogLevel	level;
	CTLogMode	mode;
	FILE		*fpLog;
	int			bInit;
};

#define CTLOG_FILE_NAME_DEFAULE	"default.log"

static CTLogContext gLogContext;
static char gLogBuffer[512];
char CTLogLevelString[][16] = {
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
	const char *pcLogFielName = CTLOG_FILE_NAME_DEFAULE;
	gLogContext.bInit = 1;
	gLogContext.level = level;
	gLogContext.mode = mode;

	if (gLogContext.mode == CTLOG_MODE_LOG2FILE)
	{
		// close file handle if LogContext has been initialized
		if (gLogContext.fpLog != NULL)
			fclose(gLogContext.fpLog);
		
		if (pcFileName != NULL && pcFileName[0] != 0)
			pcLogFielName = pcFileName;
		gLogContext.fpLog = fopen(pcLogFielName, "wb");
		if (gLogContext.fpLog == NULL)
			return CTLOG_EC_FAILURE;
	}
	else
	{
#ifdef _WIN32
		CTLogOpenConsole();
#endif // _WIN32
		gLogContext.fpLog = stdout;
	}

	return CTLOG_EC_OK;
}

int CTLogDeInit()
{
	if (gLogContext.mode == CTLOG_MODE_LOG2FILE)
	{
		// close file handle if LogContext has been initialized
		if (gLogContext.fpLog != NULL)
			fclose(gLogContext.fpLog);
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
	if (level < 0 || level >= CTLOG_NUM_LOG_LEVELS)
		return;

	if (level < gLogContext.level)
		return;

	_vsnprintf(gLogBuffer, sizeof(gLogBuffer), pcFormat, ap);
	if (gLogBuffer[strlen(gLogBuffer) - 1] == '\n')
		gLogBuffer[strlen(gLogBuffer) - 1] = 0;

	CTLogPrint(level, pcModuleName, gLogBuffer);
}