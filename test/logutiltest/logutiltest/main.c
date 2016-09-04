#include <stdio.h>
#include "logutil.h"

#define CTMyLogInfoEx(fmt, ...) CTLogInfoEx("main", fmt, __VA_ARGS__)
#define CTMyLogDebug CTLogDebug
//CTLOGSETMODULE("main");
//#define mylog(fmt, ...)	CTLogPrint(CTLOG_LEVEL_DEBUG, "main", fmt, __VA_ARGS__)
#define mylog(fmt, ...)	printf(fmt, __VA_ARGS__)

int main()
{
	CTLogInit(CTLOG_LEVEL_DEBUG, CTLOG_MODE_LOG2CONSOLE, "coryee.log");
	CTMyLogDebug("%s\n", "hello");
	CTMyLogInfoEx("%s\n", "hello");

	mylog("%s", "coryee");
	return 0;
}