#include "fileutil.h"
#include <string.h>
#ifndef _WIN32
#include <sys/types.h>
#include <dirent.h>
#endif
#include <errno.h>

int CTCreateDir(const char *pcDir)
{
	char pcTempDir[FU_MAX_PATH + 1];
	int iLen;
	int i, iRet;

	iLen = strlen(pcDir);
	if (pcDir == NULL || iLen > FU_MAX_PATH)
	{
		return -1;
	}
	strcpy(pcTempDir, pcDir);
	
	if (pcTempDir[iLen - 1] != '\\' && pcTempDir[iLen - 1] != '/')
	{
		pcTempDir[iLen] = '/';
		pcTempDir[iLen + 1] = 0;
	}
	
	for (i = 0; pcTempDir[i] != 0; i++)
	{
		if (pcTempDir[i] == '\\' || pcTempDir[i] == '/')
		{
			pcTempDir[i] = 0;
			iRet = ACCESS(pcTempDir, 0);
			if (iRet != 0)
			{
				iRet = MKDIR(pcTempDir);
				if (iRet != 0)
					return -1;
			}
			pcTempDir[i] = '/';
		}
	}
	return 0;
}

FILE *CTCreateFile(const char *pcFile, const char *pcMode)
{
	char *pcFileName;
	char ch;
	char pcTempFile[FU_MAX_PATH + 1];

	if (pcFile == NULL || strlen(pcFile) > FU_MAX_PATH)
	{
		return NULL;
	}

	strcpy(pcTempFile, pcFile);
	if ((pcFileName = strrchr(pcTempFile, '/')) == NULL &&
		(pcFileName = strrchr(pcTempFile, '\\')) == NULL)
	{
		return fopen(pcTempFile, pcMode);
	}

	ch = *pcFileName;
	*pcFileName = 0;
	if (0 != CTCreateDir(pcTempFile))
		return NULL;
	*pcFileName = ch;

	return fopen(pcTempFile, pcMode);
}

#ifndef _WIN32
// remove a file or directory
int CTRemoveDir(const char *pcPathName)
{
    int ret;
    char pcSubPath[FU_MAX_PATH];

    DIR* dirp = opendir(pcPathName);
    if(!dirp)
    {
        return -1;
    }
    struct dirent *dir;
    struct stat st;
    while((dir = readdir(dirp)) != NULL)
    {
        if(strcmp(dir->d_name,".") == 0
                || strcmp(dir->d_name,"..") == 0)
        {
            continue;
        }
        sprintf(pcSubPath, "%s/%s", pcPathName, dir->d_name);
        if(lstat(pcSubPath,&st) == -1)
        {
            printf("failed to lstat file[%s], errno = %d\n", pcSubPath, errno);
            continue;
        }
        if(S_ISDIR(st.st_mode))
        {
            if(CTRemoveDir(pcSubPath) == -1)
            {
                closedir(dirp);
                return -1;
            }
        }
        else if(S_ISREG(st.st_mode))
        {
            unlink(pcSubPath);     // 如果是普通文件，则unlink
        }
        else
        {
            printf("CTRemove unknown file[%s]\n", pcSubPath);
            continue;
        }
    }

    ret = rmdir(pcPathName);
    closedir(dirp);
    return ret;
}

int CTRemove(const char *pcPathName)
{
    char pcTempPath[FU_MAX_PATH];
    strcpy(pcTempPath, pcPathName);
    struct stat st;
    if(lstat(pcTempPath,&st) == -1)
    {
        return -1;
    }
    if(S_ISREG(st.st_mode))
    {
        if(unlink(pcTempPath) == -1)
        {
            return -1;
        }
    }
    else if(S_ISDIR(st.st_mode))
    {
        if(strcmp(pcTempPath, ".") == 0 ||
            strcmp(pcTempPath, "..") == 0)
        {
            return -1;
        }
        if(CTRemoveDir(pcTempPath) == -1)   // delete all the files in dir.
        {
            return -1;
        }
    }
    return 0;
}
#endif
