#include <stdio.h>
#include "fileutil.h"

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
	FILE *fp;
	char ch;
	char pcTempFile[FU_MAX_PATH + 1];

	if (pcFile == NULL || strlen(pcFile) > FU_MAX_PATH)
	{
		return -1;
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
		return -1;
	*pcFileName = ch;

	return fopen(pcTempFile, pcMode);
}