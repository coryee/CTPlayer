#include <stdlib.h>
#include <string.h>

#include "logutil.h"

#define CTLOG_FILE_NAME_DEFAULE	"default.log"
static CTLogContext gLogContext;
static char gLogBuffer[512];
char CTLogLevelString[][16] = {
	"DEBUG", 
	"INFO"
};


int CTLogInit(CTLogLevel level, CTLogMode mode, char *pcFileName)
{
	const char *pcLogFielName = CTLOG_FILE_NAME_DEFAULE;
	gLogContext.bInit = 1;
	gLogContext.level = level;
	gLogContext.mode = mode;

	if (gLogContext.mode | CTLOG_MODE_LOG2FILE)
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

	return CTLOG_EC_OK;
}

char *CTLogGetFormatString(char *pcFormat, va_list al)
{
	if (-1 == _vsnprintf(gLogBuffer, sizeof(gLogBuffer), pcFormat, al))
		return "";
	return gLogBuffer;
}

int CTLogPrintInternal(char *pcFormat, ...)
{
	va_list al;
	char *pcFmtStr;

	//TODO: need to use mutex to protect

	va_start(al, pcFormat);
	pcFmtStr = CTLogGetFormatString(pcFormat, al);
	va_end(al);
	printf(pcFmtStr);

	return CTLOG_EC_OK;
}


int CTLogPrint(CTLogLevel level, char *pcModuleName, char *pcFormat, ...)
{
	va_list al;
	char *pcFmtStr;

	if (level < gLogContext.level)
		return CTLOG_EC_OK;

	CTLogPrintInternal("[%s] [%s] ", CTLogLevelString[level], pcModuleName);

	va_start(al, pcFormat);
	pcFmtStr = CTLogGetFormatString(pcFormat, al);
	va_end(al);

	CTLogPrintInternal("%s", pcFmtStr);

	return CTLOG_EC_OK;
}
