#include <stdio.h>

#include "logutil.h"

#define CTMainLogLevel CTLogLevelFunc
#define CTMainLog CTLogFunc
CTLOGSETMODULE("main");
//#define mylog(fmt, ...)	CTLogPrint(CTLOG_LEVEL_DEBUG, "main", fmt, __VA_ARGS__)
#define mylog(fmt, ...)	printf(fmt, __VA_ARGS__)

int main()
{
	CTLogInit(CTLOG_LEVEL_DEBUG, CTLOG_MODE_LOG2FILE, "coryee.log");
	CTMainLog("%s\n", "hello");
	CTMainLogLevel(CTLOG_LEVEL_DEBUG, "%s\n", "hello");

	mylog("%s", "coryee");
	return 0;
}