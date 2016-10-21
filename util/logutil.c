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
#include "fileutil.h"


#ifdef _WIN32
#define snprintf sprintf_s
#else
#define _vsnprintf vsnprintf
#endif

#include "logutil.h"

#define CTLOG_FILE_NAME_DEFAULE	"default.log"


static CTLogContext gLogContext;
static char CTLogLevelString[][16] = {
	"DEBUG", 
	"INFO",
	"ERROR"
};

static void CTLogGetCurrentTime(char *pcCurTime, int iSize);
static void CTLogGetDate(char *pcCurDate, int iSize);
static int CTLogCreateLogFile(CTLogContext *pLogContext);
static int CTLogAppendTimestamp2FileName(char *pcFileName, char *pcOutFileName);
static int CTLogAddLogFile(CTLogContext *pLogContext, char *pcFileName);
static int CTLogPrint(CTLogContext *pLogContext, CTLogLevel level, const char *pcModuleName, char *pcMessage);
static void CTLogMessage(CTLogContext *pLogContext, CTLogLevel level, const char *pcModuleName, const char *pcFormat, va_list ap);

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

int CTLogInit(CTLogContext *pLogContext, CTLogLevel level, CTLogMode mode, const char *pcFileName)
{
    int ret = CTLOG_EC_FAILURE;
    CTLogContext *pLogCtx = pLogContext;
    const char *pcLogFielName = CTLOG_FILE_NAME_DEFAULE;

    if (pLogCtx == NULL)
        return CTLOG_EC_FAILURE;

    memset(pLogCtx, 0, sizeof(*pLogCtx));

    if (pLogCtx->bInit)
        return CTLOG_EC_OK;

    pLogCtx->level = level;
    pLogCtx->mode = mode;
    pLogCtx->logFileSize = CTLOG_MAX_FILE_SIZE;
    pLogCtx->curFileSize = 0;
    pLogCtx->numFilesToReserve = -1;

    do {
        pLogCtx->pMutex = CTCreateMutex();
        if (pLogCtx->pMutex == NULL)
            break;
        if (pLogCtx->mode == CTLOG_MODE_LOG2FILE)
        {
            // close file handle if LogContext has been initialized
            if (pLogCtx->fpLog != NULL)
                fclose(pLogCtx->fpLog);

            if (pcFileName != NULL && pcFileName[0] != 0)
                pcLogFielName = pcFileName;

            strncpy(pLogCtx->pcFileName, pcLogFielName, sizeof(pLogCtx->pcFileName));
            if (CTLogCreateLogFile(pLogCtx) != CTLOG_EC_OK)
                break;
        }
        else
        {
#ifdef _WIN32
            CTLogOpenConsole();
#endif // _WIN32
            pLogCtx->fpLog = stdout;
        }

        pLogCtx->bInit = 1;
        ret = CTLOG_EC_OK;
    } while(0);

    if (ret == CTLOG_EC_FAILURE)
        CTLogDeInit(pLogCtx);

    return ret;
}
int CTLogDeInit(CTLogContext *pLogContext)
{
    int i = 0;
    CTLogContext *pLogCtx = pLogContext;

    if (pLogContext == NULL)
        return CTLOG_EC_FAILURE;

    CTDestroyMutex(pLogCtx->pMutex);
    if (pLogCtx->mode == CTLOG_MODE_LOG2FILE)
    {
        // close file handle if LogContext has been initialized
        if (pLogCtx->fpLog != NULL)
            fclose(pLogCtx->fpLog);
    }

    if (pLogContext->pFileNames)
    {
        for (i = 0; i < pLogContext->numFilesToReserve; i++)
        {
            if (pLogContext->pFileNames[i])
                free(pLogContext->pFileNames[i]);
        }
        free(pLogContext->pFileNames);
    }

    return CTLOG_EC_OK;
}

int CTLogSetLogFileSize(CTLogContext *pLogContext, unsigned int size)
{
    if (pLogContext == NULL)
        return CTLOG_EC_FAILURE;
    pLogContext->logFileSize = size;

    return CTLOG_EC_OK;
}

int CTLogSetLogFile(CTLogContext *pLogContext, const char *pcFilePath)
{
    if (pLogContext == NULL || pcFilePath == NULL)
        return CTLOG_EC_FAILURE;
    strncpy(pLogContext->pcFileName, pcFilePath, sizeof(pLogContext->pcFileName));

    return CTLOG_EC_OK;
}

int CTLogSetNumLogFilesToReserve(CTLogContext *pLogContext, int iNumFiles)
{
    int i = 0;

    if (pLogContext->pFileNames)
    {
        for (i = 0; i < pLogContext->numFilesToReserve; i++)
        {
            if (pLogContext->pFileNames[i])
                free(pLogContext->pFileNames[i]);
        }
        free(pLogContext->pFileNames);
    }

    pLogContext->numFilesToReserve = iNumFiles;
    pLogContext->pFileNames = (char **)malloc(pLogContext->numFilesToReserve *sizeof(*(pLogContext->pFileNames)));
    if (!pLogContext->pFileNames)
        return CTLOG_EC_FAILURE;
    for (i = 0; i < pLogContext->numFilesToReserve; i++)
    {
        pLogContext->pFileNames[i] = (char *)malloc(CTLOG_MAX_PATH);
        if (!pLogContext->pFileNames[i])
            return CTLOG_EC_FAILURE;

        memset(pLogContext->pFileNames[i], 0, CTLOG_MAX_PATH);
    }

    return CTLOG_EC_OK;
}

void CTLogDebugEx(CTLogContext *pLogContext, const char *pcModuleName, const char *pcFormat, ...)
{
    va_list ap;
    va_start(ap, pcFormat);
    CTLogMessage(pLogContext, CTLOG_LEVEL_DEBUG, pcModuleName, pcFormat, ap);
    va_end(ap);
}

void CTLogDebug(CTLogContext *pLogContext, const char *pcFormat, ...)
{
    va_list ap;
    va_start(ap, pcFormat);
    CTLogMessage(pLogContext, CTLOG_LEVEL_DEBUG, NULL, pcFormat, ap);
    va_end(ap);
}

void CTLogInfoEx(CTLogContext *pLogContext, const char *pcModuleName, const char *pcFormat, ...)
{
    va_list ap;
    va_start(ap, pcFormat);
    CTLogMessage(pLogContext, CTLOG_LEVEL_INFO, pcModuleName, pcFormat, ap);
    va_end(ap);
}

void CTLogInfo(CTLogContext *pLogContext, const char *pcFormat, ...)
{
    va_list ap;
    va_start(ap, pcFormat);
    CTLogMessage(pLogContext, CTLOG_LEVEL_INFO, NULL, pcFormat, ap);
    va_end(ap);
}

void CTLogErrorEx(CTLogContext *pLogContext, const char *pcModuleName, const char *pcFormat, ...)
{
    va_list ap;
    va_start(ap, pcFormat);
    CTLogMessage(pLogContext, CTLOG_LEVEL_ERROR, pcModuleName, pcFormat, ap);
    va_end(ap);
}

void CTLogError(CTLogContext *pLogContext, const char *pcFormat, ...)
{
    va_list ap;
    va_start(ap, pcFormat);
    CTLogMessage(pLogContext, CTLOG_LEVEL_ERROR, NULL, pcFormat, ap);
    va_end(ap);
}

int CTLogSInit(CTLogLevel level, CTLogMode mode, char *pcFileName)
{
	return CTLogInit(&gLogContext, level, mode, pcFileName);
}

int CTLogSDeInit()
{
    return CTLogDeInit(&gLogContext);
}

void CTLogSDebugEx(const char *pcModuleName, const char *pcFormat, ...)
{
	va_list ap;
	va_start(ap, pcFormat);
	CTLogMessage(&gLogContext, CTLOG_LEVEL_DEBUG, pcModuleName, pcFormat, ap);
	va_end(ap);
}

void CTLogSDebug(const char *pcFormat, ...)
{
	va_list ap;
	va_start(ap, pcFormat);
	CTLogMessage(&gLogContext, CTLOG_LEVEL_DEBUG, NULL, pcFormat, ap);
	va_end(ap);
}

void CTLogSInfoEx(const char *pcModuleName, const char *pcFormat, ...)
{
	va_list ap;
	va_start(ap, pcFormat);
	CTLogMessage(&gLogContext, CTLOG_LEVEL_INFO, pcModuleName, pcFormat, ap);
	va_end(ap);
}

void CTLogSInfo(const char *pcFormat, ...)
{
	va_list ap;
	va_start(ap, pcFormat);
	CTLogMessage(&gLogContext, CTLOG_LEVEL_INFO, NULL, pcFormat, ap);
	va_end(ap);
}

void CTLogSErrorEx(const char *pcModuleName, const char *pcFormat, ...)
{
	va_list ap;
	va_start(ap, pcFormat);
	CTLogMessage(&gLogContext, CTLOG_LEVEL_ERROR, pcModuleName, pcFormat, ap);
	va_end(ap);
}

void CTLogSError(const char *pcFormat, ...)
{
	va_list ap;
	va_start(ap, pcFormat);
	CTLogMessage(&gLogContext, CTLOG_LEVEL_ERROR, NULL, pcFormat, ap);
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
		(int)tv.tv_usec);
#endif
}

static void CTLogGetDate(char *pcCurDate, int iSize)
{
#ifdef _WIN32
    SYSTEMTIME sys;

    pcCurTime[0] = 0;
    GetLocalTime(&sys);
    snprintf(pcCurTime, iSize, "%4d%02d%02d%02d%02d%02d", sys.wYear, sys.wMonth, sys.wDay, sys.wHour, sys.wMinute, sys.wSecond);
#else
    struct timeval      tv;
    struct timezone     tz;
    struct tm           *p;

    pcCurDate[0] = 0;
    gettimeofday(&tv, &tz);
    p = localtime(&tv.tv_sec);
    snprintf(pcCurDate, iSize, "%4d%02d%02d%02d%02d%02d",
        1900 + p->tm_year,
        1 + p->tm_mon,
        p->tm_mday,
        p->tm_hour,
        p->tm_min,
        p->tm_sec);
#endif
}

static int CTLogAppendTimestamp2FileName(char *pcFileName, char *pcOutFileName)
{
    char pcTempFile[CTLOG_MAX_PATH];
    char pcCurDate[64];

    if (!pcFileName || !pcOutFileName)
        return CTLOG_EC_FAILURE;

    CTLogGetDate(pcCurDate, 64);

    strncpy(pcTempFile, pcFileName, sizeof(pcTempFile));

    char *ptr = strrchr(pcTempFile, '.');
    if (ptr)
    {
        *ptr = 0;
        sprintf(pcOutFileName, "%s-%s.%s", pcTempFile, pcCurDate, ptr + 1);
        *ptr = '.';
    }
    else
    {
        sprintf(pcOutFileName, "%s-%s", pcTempFile, pcCurDate);
    }

    return CTLOG_EC_OK;
}

static int CTLogCreateLogFile(CTLogContext *pLogContext)
{
    CTLogContext *pLogCtx = pLogContext;
    char pcTempPath[CTLOG_MAX_PATH];

    CTLogAppendTimestamp2FileName(pLogCtx->pcFileName, pcTempPath);
    pLogCtx->fpLog = CTCreateFile(pcTempPath, "wb+");
    if (pLogCtx->fpLog == NULL)
    {
        printf("failed to create log file[%s]\n", pLogCtx->pcFileName);
        return CTLOG_EC_FAILURE;
    }
    CTLogAddLogFile(pLogCtx, pcTempPath);
    return CTLOG_EC_OK;
}

static int CTLogAddLogFile(CTLogContext *pLogContext, char *pcFileName)
{
    if (!pLogContext->pFileNames || pLogContext->numFilesToReserve <= 0)
    {
        return CTLOG_EC_OK;
    }

    if (pLogContext->pFileNames[pLogContext->curFileIdx][0])
    {
        remove(pLogContext->pFileNames[pLogContext->curFileIdx]);
    }

    strcpy(pLogContext->pFileNames[pLogContext->curFileIdx], pcFileName);
    pLogContext->curFileIdx = (pLogContext->curFileIdx + 1) % pLogContext->numFilesToReserve;
    printf("curFileIdx=%d\n", pLogContext->curFileIdx);
    return CTLOG_EC_OK;
}

static int CTLogPrint(CTLogContext *pLogContext, CTLogLevel level, const char *pcModuleName, char *pcMessage)
{
	CTLogContext *pLogCtx = pLogContext;
	char pcCurTime[32];
	int ret = 0;

	if (pLogCtx->fpLog == NULL)
	    return 0;

	CTLogGetCurrentTime(pcCurTime, 32);
	//TODO: need to use mutex to protect
	if (pcModuleName == NULL || pcModuleName[0] == 0)
		ret = fprintf(pLogCtx->fpLog, "%s [%s] %s\n", pcCurTime, CTLogLevelString[level], pcMessage);
	else
		ret = fprintf(pLogCtx->fpLog, "%s [%s] <%s> %s\n", pcCurTime, CTLogLevelString[level], pcModuleName, pcMessage);
	fflush(pLogCtx->fpLog);

	return ret;
}

static void CTLogMessage(CTLogContext *pLogContext, CTLogLevel level, const char *pcModuleName, const char *pcFormat, va_list ap)
{
    CTLogContext *pLogCtx = pLogContext;
    char *pBuffer = pLogCtx->pBuffer;

    if (level < 0 || level >= CTLOG_NUM_LOG_LEVELS)
        return;

    if (level < pLogCtx->level)
        return;

    CTMutexLock(pLogCtx->pMutex);
    if (!pLogCtx->bInit)
    {
        if (pLogCtx->mode == CTLOG_MODE_LOG2CONSOLE) {
            if (CTLOG_EC_OK != CTLogInit(pLogContext, CTLOG_LEVEL_DEBUG, CTLOG_MODE_LOG2CONSOLE, NULL))
            {
                CTMutexUnlock(pLogCtx->pMutex);
                return;
            }
        }
        else
        {
            return;
        }

    }

    // make sure size of the log file is smaller than the size we set,
    // or we will create a new file to save the log.
    if (pLogCtx->mode == CTLOG_MODE_LOG2FILE)
    {
        if (pLogCtx->curFileSize > pLogCtx->logFileSize)
        {
            if (pLogCtx->fpLog)
                fclose(pLogCtx->fpLog);
            if (CTLogCreateLogFile(pLogCtx) != CTLOG_EC_OK)
                return;
            pLogCtx->curFileSize = 0;
        }
    }

    _vsnprintf(pBuffer, CTLOG_MAX_BUFFER, pcFormat, ap);
    if (pBuffer[strlen(pBuffer) - 1] == '\n')
        pBuffer[strlen(pBuffer) - 1] = 0;

    pLogCtx->curFileSize += CTLogPrint(pLogContext, level, pcModuleName, pBuffer);
    CTMutexUnlock(pLogCtx->pMutex);
}
