#ifndef _FILEUTIL_H_
#define _FILTUTIL_H_
#if defined(__cplusplus)
extern "C"
{
#endif

#include <stdio.h>
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#else
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>
#endif


#ifdef _WIN32
#define ACCESS		_access
#define MKDIR(dir)	_mkdir((dir))
#else
#define ACCESS		access
#define MKDIR(dir)	mkdir((dir), 0755)
#endif 

#define FU_MAX_PATH	256


// 0 on success; -1 on failure
int CTCreateDir(const char *pcDir);
FILE *CTCreateFile(const char *pcFile, const char *pcMode);

#ifndef _WIN32
// 0 on success; -1 on failure
int CTRemoveDir(const char *pcPathName);
// remove a file or directory
// 0 on success; -1 on failure
int CTRemove(const char *pcPathName);
#endif

#if defined(__cplusplus)
}
#endif
#endif
