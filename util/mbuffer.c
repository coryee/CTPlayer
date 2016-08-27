#include <stdlib.h>
#ifdef _WIN32
#include <malloc.h>
#endif
#include <string.h>
#include <ctype.h>
#include "mbuffer.h"

int MBUFFERSYSBufferInit(unsigned char *pucBuffer, int iHead, int iTail, int iSize, MBUFFERSYSBuffer *pSYSBuffer)
{
    if (pucBuffer == NULL)
    {
        pSYSBuffer->pucBuffer = (unsigned char *)malloc(iSize);
        pSYSBuffer->iHead = pSYSBuffer->iTail = 0;
    }
    else
    {
        pSYSBuffer->pucBuffer = pucBuffer;
        pSYSBuffer->iHead = iHead;
        pSYSBuffer->iTail = iTail;
    }
    pSYSBuffer->iSize = iSize;
    pSYSBuffer->iNumBytesInput = 0;
    pSYSBuffer->iNumBytesOutput = 0;
    return(MBUFFER_EC_OK);
}

int MBUFFERSYSBufferDeInit(int bIfFreeBuffer, MBUFFERSYSBuffer *pSYSBuffer)
{
    if (bIfFreeBuffer)
    {
        free(pSYSBuffer->pucBuffer);
        pSYSBuffer->pucBuffer = NULL;
        pSYSBuffer->iSize = 0;
    }
    pSYSBuffer->iHead = pSYSBuffer->iTail = 0;
    pSYSBuffer->iNumBytesInput = 0;
    pSYSBuffer->iNumBytesOutput = 0;
    return(MBUFFER_EC_OK);
}

int MBUFFERSYSBufferSpaceAvailable(MBUFFERSYSBuffer *pSYSBuffer)
{
    int iRet;
    int iHead, iTail;

    // we limit available space to the_true_available_space - 1 so the head will never be the same as tail
    // to differentiate between empty and full
    // Don't optimize: we do this on purpose as producer and consumer may run in different thread!
    iHead = pSYSBuffer->iHead;
    iTail = pSYSBuffer->iTail;
    iRet = iHead - iTail - 1;
    if (iHead <= iTail)
    {
        iRet += pSYSBuffer->iSize;
    }
    return(iRet);
}

int MBUFFERSYSBufferSpaceAvailableToEnd(MBUFFERSYSBuffer *pSYSBuffer)
{
    int iRet;
    int iHead, iTail;
    // We have these cases:
    // * iHead <= iTail:
    // * iTail < iSize and iHead > 0: iSize - iTail
    // * iTail < iSize and iHead = 0: iSize - iTail - 1
    // * iHead > iTail: iHead - iTail - 1
    // we limit available space to the_true_available_space - 1 so the head will never be the same as tail
    // to differentiate between empty and full
    // the buffer is empty if iHead = iTail; full if (iTail + 1) % iSize = iHead
    // Don't optimize: we do this on purpose as producer and consumer may run in different thread!
    iHead = pSYSBuffer->iHead;
    iTail = pSYSBuffer->iTail;
    if (iHead <= iTail)
    {
        iRet = pSYSBuffer->iSize - iTail;
        if (iHead == 0)
            iRet--;
    }
    else
        iRet = iHead - iTail - 1;

    return iRet;
}

int MBUFFERSYSBufferLength(MBUFFERSYSBuffer *pSYSBuffer)
{
    int iRet;

    iRet = pSYSBuffer->iTail - pSYSBuffer->iHead;
    if (iRet < 0)
    {
        iRet += pSYSBuffer->iSize;
    }
    return(iRet);
}

int MBUFFERSYSBufferLengthToEnd(MBUFFERSYSBuffer *pSYSBuffer)
{
    int iHead, iTail;
    iHead = pSYSBuffer->iHead;
    iTail = pSYSBuffer->iTail;

    if (iTail < iHead)
        return(pSYSBuffer->iSize - iHead);
    else
        return(iTail - iHead);
}

unsigned char MBUFFERSYSValueAtPos(int iPos, MBUFFERSYSBuffer *pSYSBuffer)
{
    return(*(pSYSBuffer->pucBuffer + (pSYSBuffer->iHead + iPos) % pSYSBuffer->iSize));
}

unsigned char *MBUFFERSYSAppendDataPos(MBUFFERSYSBuffer *pSYSBuffer)
{
    return(pSYSBuffer->pucBuffer + pSYSBuffer->iTail);
}

// AppendData assumes that the length of data to append is less or equal than the space available in the sysbuffer.
// So it's whole or nothing
int MBUFFERSYSAppendData(unsigned char *pucData, int iLength, MBUFFERSYSBuffer *pSYSBuffer)
{
    int iDataCanCopy;

    if (MBUFFERSYSBufferSpaceAvailable(pSYSBuffer) < iLength)
        return(MBUFFER_EC_FAIL);
    iDataCanCopy = MBUFFERSYSBufferSpaceAvailableToEnd(pSYSBuffer);
    if (iDataCanCopy < iLength)
    {
        memcpy(pSYSBuffer->pucBuffer + pSYSBuffer->iTail, pucData, iDataCanCopy);
        memcpy(pSYSBuffer->pucBuffer, pucData + iDataCanCopy, iLength - iDataCanCopy);
    }
    else
        memcpy(pSYSBuffer->pucBuffer + pSYSBuffer->iTail, pucData, iLength);
    pSYSBuffer->iTail += iLength;
    pSYSBuffer->iTail %= pSYSBuffer->iSize;
    pSYSBuffer->iNumBytesInput += iLength;
    return(MBUFFER_EC_OK);
}

// CopyByteArray2SYSBuffer will copy as much data as possible and returns the total amount o data copied
int MBUFFERSYSCopyByteArray2SYSBuffer(unsigned char *pucData, int iLength, MBUFFERSYSBuffer *pSYSBuffer)
{
    int iBytesCanCopy;

    iBytesCanCopy = MBUFFERSYSBufferSpaceAvailable(pSYSBuffer);
    if (iBytesCanCopy > iLength)
        iBytesCanCopy = iLength;
    MBUFFERSYSAppendData(pucData, iBytesCanCopy, pSYSBuffer);
    return(iBytesCanCopy);
}

int MBUFFERSYSAdvance(int iLength, MBUFFERSYSBuffer *pSYSBuffer)
{
    if (MBUFFERSYSBufferLength(pSYSBuffer) < iLength)
        return(MBUFFER_EC_FAIL);
    pSYSBuffer->iHead += iLength;
    pSYSBuffer->iHead %= pSYSBuffer->iSize;
    pSYSBuffer->iNumBytesOutput += iLength;
    return(MBUFFER_EC_OK);
}

int MBUFFERSYSExtend(int iLength, MBUFFERSYSBuffer *pSYSBuffer)
{
    pSYSBuffer->iTail += iLength;
    pSYSBuffer->iTail %= pSYSBuffer->iSize;
    pSYSBuffer->iNumBytesInput += iLength;
    return(MBUFFER_EC_OK);
}

unsigned char *MBUFFERSYSStartPos(MBUFFERSYSBuffer *pSYSBuffer)
{
    if (pSYSBuffer->iHead == pSYSBuffer->iTail)
        return(NULL);
    else
        return(pSYSBuffer->pucBuffer + pSYSBuffer->iHead);
}

unsigned char *MBUFFERSYSPosFromStart(int iOffset, MBUFFERSYSBuffer *pSYSBuffer)
{
    iOffset += pSYSBuffer->iHead;
    iOffset %= pSYSBuffer->iSize;
    return(pSYSBuffer->pucBuffer + iOffset);
}

int MBUFFERSYSBufferLengthToEndByOffset(int iOffset, MBUFFERSYSBuffer *pSYSBuffer)
{
    int iHead, iTail;

    iHead = pSYSBuffer->iHead;
    iTail = pSYSBuffer->iTail;

    iOffset += iHead;
    iOffset %= pSYSBuffer->iSize;
    if (iHead < iTail)
    {
        if (iOffset >= iHead && iOffset < iTail)
            return(iTail - iOffset);
    }
    else
    {
        if (iOffset >= iHead)
            return(pSYSBuffer->iSize - iOffset);
        else if (iOffset < iTail)
            return(iTail - iOffset);
    }
    return(0);
}

int MBUFFERSYSCopyToCache(int iLength, MBUFFERSYSBuffer *pSYSBuffer)
{
    int iDataCanCopy;
    unsigned char *pucStartPos = MBUFFERSYSStartPos(pSYSBuffer);

    iDataCanCopy = MBUFFERSYSBufferLengthToEnd(pSYSBuffer);
    if (iDataCanCopy < iLength)
    {
        memcpy(pSYSBuffer->pucBufferCache, pucStartPos, iDataCanCopy);
        memcpy(pSYSBuffer->pucBufferCache + iDataCanCopy, pSYSBuffer->pucBuffer, iLength - iDataCanCopy);
    }
    else
        memcpy(pSYSBuffer->pucBufferCache, pucStartPos, iLength);
     pSYSBuffer->pucCurStart = pSYSBuffer->pucBufferCache;
    return(MBUFFER_EC_OK);
}

int MBUFFERSYSCopyToCacheByOffset(int iOffset, int iLength, MBUFFERSYSBuffer *pSYSBuffer)
{
    int iDataCanCopy;
    unsigned char *pucStartPos = MBUFFERSYSPosFromStart(iOffset, pSYSBuffer);

    iDataCanCopy = MBUFFERSYSBufferLengthToEndByOffset(iOffset, pSYSBuffer);
    if (iDataCanCopy < iLength)
    {
        memcpy(pSYSBuffer->pucBufferCache, pucStartPos, iDataCanCopy);
        memcpy(pSYSBuffer->pucBufferCache + iDataCanCopy, pSYSBuffer->pucBuffer, iLength - iDataCanCopy);
    }
    else
        memcpy(pSYSBuffer->pucBufferCache, pucStartPos, iLength);
     pSYSBuffer->pucCurStart = pSYSBuffer->pucBufferCache;
    return(MBUFFER_EC_OK);
}

unsigned char *MBUFFERSYSGetContinousBuffer(int iOffset, int iLength, MBUFFERSYSBuffer *pSYSBuffer)
{
    if (MBUFFERSYSBufferLengthToEndByOffset(iOffset, pSYSBuffer) <= iLength)
    {
        // copy data to cache and start from there
        MBUFFERSYSCopyToCacheByOffset(iOffset, iLength, pSYSBuffer);
        return(pSYSBuffer->pucBufferCache);
    }
    else
        return(MBUFFERSYSPosFromStart(iOffset, pSYSBuffer));
}

// for performance, this function doesn't do buffer length check.
// It's the caller's responsibility to guarantee that we have enough data
int MBUFFERSYSPeek2ByteArray(unsigned char *pucDest, int iLength, MBUFFERSYSBuffer *pSYSBuffer)
{
    unsigned char *pucStartPos = MBUFFERSYSStartPos(pSYSBuffer);
    int iBytes2Copy;
    int iBytesLeft;
    int iHead, iTail;

    // Don't optimize: we do this on purpose as producer and consumer may run in different thread!
    iHead = pSYSBuffer->iHead;
    iTail = pSYSBuffer->iTail;
    iBytesLeft = iLength;
    // copy from iHead to iTail/iSize - 1
    if (iTail > iHead)
        iBytes2Copy = iTail - iHead;
    else
        iBytes2Copy = pSYSBuffer->iSize - iHead;
    if (iBytes2Copy > iBytesLeft)
        iBytes2Copy = iBytesLeft;
    memcpy(pucDest, pucStartPos, iBytes2Copy);
    iBytesLeft -= iBytes2Copy;
    pucDest += iBytes2Copy;

    // copy from pucBuffer
    if (iBytesLeft > 0)
        memcpy(pucDest, pSYSBuffer->pucBuffer, iBytesLeft);
    return(MBUFFER_EC_OK);
}

int MBUFFERSYSCopy2ByteArray(unsigned char *pucDest, int iLength, MBUFFERSYSBuffer *pSYSBuffer)
{
    MBUFFERSYSPeek2ByteArray(pucDest, iLength, pSYSBuffer);
    MBUFFERSYSAdvance(iLength, pSYSBuffer);
    return(MBUFFER_EC_OK);
}

int MBUFFERSYSNumBytesInput(MBUFFERSYSBuffer *pSYSBuffer)
{
    return(pSYSBuffer->iNumBytesInput);
}

int MBUFFERSYSNumBytesOutput(MBUFFERSYSBuffer *pSYSBuffer)
{
    return(pSYSBuffer->iNumBytesOutput);
}

int MBUFFERSYSReadFromFile(FILE *fp_input, int iLength, MBUFFERSYSBuffer *pSYSBuffer)
{
    // return num of bytes read, buffer availability checking is done outside of this function
    int iBytes2Read, iBytesLeft = iLength;

    if (feof(fp_input))
        return(0);

    if (MBUFFERSYSBufferSpaceAvailable(pSYSBuffer) < iLength)
        return(0);

    iBytes2Read = MBUFFERSYSBufferSpaceAvailableToEnd(pSYSBuffer);
    if (iBytes2Read > iLength)
        iBytes2Read = iLength;
    fread(MBUFFERSYSAppendDataPos(pSYSBuffer), iBytes2Read, 1, fp_input);
    MBUFFERSYSExtend(iBytes2Read, pSYSBuffer);
    iBytesLeft -= iBytes2Read;
    if (iBytesLeft > 0)
    {
        fread(MBUFFERSYSAppendDataPos(pSYSBuffer), iBytesLeft, 1, fp_input);
        MBUFFERSYSExtend(iBytesLeft, pSYSBuffer);
    }
    return(iLength);
}

int MBUFFERSYSWriteToFile(FILE *fp_output, int iLength, MBUFFERSYSBuffer *pSYSBuffer)
{
    // return num of bytes written, buffer availability checking is done outside of this function
    int iBytesWrite, iBytesLeft = iLength;

    if (MBUFFERSYSBufferLengthToEnd(pSYSBuffer) < iLength)
    {
        iBytesWrite = MBUFFERSYSBufferLengthToEnd(pSYSBuffer);
        fwrite(MBUFFERSYSStartPos(pSYSBuffer), iBytesWrite, 1, fp_output);
        MBUFFERSYSAdvance(iBytesWrite, pSYSBuffer);
        iBytesLeft -= iBytesWrite;
    }
    fwrite(MBUFFERSYSStartPos(pSYSBuffer), iBytesLeft, 1, fp_output);
    MBUFFERSYSAdvance(iBytesLeft, pSYSBuffer);
    return(iLength);
}

int MBUFFERSYSWriteToFileNoAdvance(FILE *fp_output, int iLength, MBUFFERSYSBuffer *pSYSBuffer)
{
    // return num of bytes written, buffer availability checking is done outside of this function
    int iBytesWrite = 0, iBytesLeft = iLength;

    if (MBUFFERSYSBufferLengthToEnd(pSYSBuffer) < iLength)
    {
        iBytesWrite = MBUFFERSYSBufferLengthToEnd(pSYSBuffer);
        fwrite(MBUFFERSYSStartPos(pSYSBuffer), iBytesWrite, 1, fp_output);
        iBytesLeft -= iBytesWrite;
    }
    fwrite(MBUFFERSYSPosFromStart(iBytesWrite, pSYSBuffer), iBytesLeft, 1, fp_output);
    return(iLength);
}


// ESBuffer related
int MBUFFERESBufferInit(unsigned char *pucBuffer, int iSize, MBUFFERESBuffer *pESBuffer)
{
#ifndef USE_DETACHED_ESBUFFER
    if (pucBuffer == NULL)
    {
        pESBuffer->pucBuffer = (unsigned char *)malloc(iSize);
    }
    else
    {
        pESBuffer->pucBuffer = pucBuffer;
    }
#else
    pESBuffer->iIndexHead = 0;
    pESBuffer->iIndexTail = 0;
#endif
    pESBuffer->iIndexHead = pESBuffer->iIndexTail = 0;
    pESBuffer->iSize = iSize;
    pESBuffer->iTimeScale = 90000;  // default to MPEG
    pESBuffer->lLastTS = -1;
    pESBuffer->lLastAdjustedTS = -1;
    pESBuffer->lPrevLastTS = -1;
    pESBuffer->lPrevLastAdjustedTS = -1;

    return(MBUFFER_EC_OK);
}

int MBUFFERESBufferDeInit(int bIfFreeBuffer, MBUFFERESBuffer *pESBuffer)
{
    if (bIfFreeBuffer)
    {
#ifndef USE_DETACHED_ESBUFFER
        free(pESBuffer->pucBuffer);
        pESBuffer->pucBuffer = NULL;
#endif
        pESBuffer->iSize = 0;
    }
    pESBuffer->iIndexHead = 0;
    pESBuffer->iIndexTail = 0;
    pESBuffer->lLastTS = -1;
    pESBuffer->lLastAdjustedTS = -1;
    pESBuffer->lPrevLastTS = -1;
    pESBuffer->lPrevLastAdjustedTS = -1;

    return(MBUFFER_EC_OK);
}

int MBUFFERESBufferSetCodec(int iCodec, MBUFFERESBuffer *pESBuffer)
{
    pESBuffer->iCodec = iCodec;

    return(MBUFFER_EC_OK);
}

int MBUFFERESBufferCheck4KeyFrameTillPos(int iFrame, int iSize, MBUFFERESBuffer *pESBuffer)
{
    int i;
    int nal_unit_type;
#ifndef USE_DETACHED_ESBUFFER
    unsigned char *pucData = pESBuffer->pucBuffer + pESBuffer->piHead[iFrame];
#else
    unsigned char *pucData =  pESBuffer->pucBuffer[iFrame];
#endif

    if (pESBuffer->iCodec == MBUFFER_ES_CODEC_AVC)
    {
        i = 0;
        while (i < iSize - 4)
        {
            if (pucData[i] == 0 && pucData[i + 1] == 0 && pucData[i + 2] == 0 && pucData[i + 3] == 1)
            {
                nal_unit_type = (pucData[i + 4] & 0x1f);
                if (((pucData[i + 4] >> 5) & 3)/*nal_ref_idc*/ &&  nal_unit_type == 5)
                    return 1;
                else if ((nal_unit_type >= 1) && (nal_unit_type <= 4))
                    return 0;
            }

            i++;
        }
    }
    return 0;
}

int MBUFFERESBufferCheck4KeyFrame(int iFrame, MBUFFERESBuffer *pESBuffer)
{
#ifndef USE_DETACHED_ESBUFFER
    return(MBUFFERESBufferCheck4KeyFrameTillPos(iFrame, pESBuffer->piTail[iFrame] - pESBuffer->piHead[iFrame], pESBuffer));
#else
    return(MBUFFERESBufferCheck4KeyFrameTillPos(iFrame, pESBuffer->piBufferLength[iFrame], pESBuffer));
#endif

}

int MBUFFERESBufferCheckLastFrame4KeyFrame(MBUFFERESBuffer *pESBuffer)
{
    return(MBUFFERESBufferCheck4KeyFrame(MBUFFERESLastFrameNo(pESBuffer), pESBuffer));
}

int MBUFFERESBufferIsIndexFull(MBUFFERESBuffer *pESBuffer)
{
    if (pESBuffer->iIndexHead ==
        (pESBuffer->iIndexTail + 1) % MBUFFER_ES_MAX_FRAMES_IN_BUFFER)
        return(1);
    else
        return(0);
}

#ifndef USE_DETACHED_ESBUFFER
int MBUFFERESBufferSpaceAvailable(int iIfStartFrame, MBUFFERESBuffer *pESBuffer)
{
    // this function is used wen we want to append data to ES buffer
    // in ES buffer a frame is continuous. So if wraparound happens after we append data we
    // may want to move the last frame to the front of the buffer.
    // so this function returns the max(front - length_of_last_frame, size - end_of_last_frame)
    // In the case of there's only one frame in the ES buffer:
    //  * the frame can be empty, so piHead[xxx] == piTail[xxx]
    //  * if we allow the whole space to be filled, if the frame occupies the whole space, still we can get piHead[xxx] == piTail[xxx].
    // We cannot differenciate between full and empty.
    // To avoid this situation, we deliberately report one byte less than the real space.
    int iRet, iRet0;
    int iHeadPos, iTailPos;

    if (pESBuffer->iIndexHead == pESBuffer->iIndexTail)
        return(pESBuffer->iSize - 1);
    iHeadPos = pESBuffer->piHead[pESBuffer->iIndexHead];
    iTailPos = pESBuffer->piTail[MBUFFERESLastFrameNo(pESBuffer)];
    if (iTailPos >= iHeadPos)
    {
        iRet = pESBuffer->iSize - iTailPos;
        if (iIfStartFrame)
        {
            iRet0 = iHeadPos;
        }
        else
        {
            iRet0 = iHeadPos - MBUFFERESLastFrameLength(pESBuffer);
        }
        if (iRet0 > iRet)
            iRet = iRet0;
    }
    else
        iRet = iHeadPos - iTailPos;

    return(iRet - 1);
}
#endif

#ifdef USE_DETACHED_ESBUFFER
int MBUFFERESAppendData(unsigned char *pucData, int iLength, int iIfStartFrame, MBUFFERESBuffer *pESBuffer)
{
    pESBuffer->pucBuffer[pESBuffer->iIndexTail] = pucData;
    pESBuffer->piBufferLength[pESBuffer->iIndexTail] = iLength;
    pESBuffer->iIndexTail = (pESBuffer->iIndexTail + 1) % pESBuffer->iSize;
    return(MBUFFER_EC_OK);
}
#else
int MBUFFERESAppendData(unsigned char *pucData, int iLength, int iIfStartFrame, MBUFFERESBuffer *pESBuffer)
{
    int iAppendPos = 0;
    int iHeadPos, iTailPos;
    int iLastHead;
    int iIndexHead, iIndexTail;
    int iLastIndex, iCurAppendIndex;

    if (MBUFFERESBufferSpaceAvailable(iIfStartFrame, pESBuffer) < iLength)
        return(MBUFFER_EC_FAIL);
    if (iIfStartFrame && (pESBuffer->iIndexTail + 1) % MBUFFER_ES_MAX_FRAMES_IN_BUFFER == pESBuffer->iIndexHead)
        return(MBUFFER_EC_FAIL);

    iIndexHead = pESBuffer->iIndexHead;
    iIndexTail = pESBuffer->iIndexTail;
    iLastIndex = MBUFFERESLastFrameNo(pESBuffer);
    if (iIfStartFrame == 0)
        iCurAppendIndex = iLastIndex;
    else
        iCurAppendIndex = iIndexTail;

    if (iIndexHead != iIndexTail)
    {
        iHeadPos = pESBuffer->piHead[iIndexHead];
        iTailPos = pESBuffer->piTail[iLastIndex];
        if ((iTailPos < iHeadPos) || (pESBuffer->iSize - iTailPos >= iLength))
            iAppendPos = iTailPos;
        else if (iIfStartFrame == 0)
        {
            // move the last frame to the front
            iLastHead = pESBuffer->piHead[iLastIndex];
            memcpy(pESBuffer->pucBuffer, pESBuffer->pucBuffer + iLastHead, iTailPos - iLastHead);
            pESBuffer->piHead[iCurAppendIndex] = 0;
            iTailPos = iTailPos - iLastHead;
            iAppendPos = iTailPos;
        }
    }
    memcpy(pESBuffer->pucBuffer + iAppendPos, pucData, iLength);
    pESBuffer->piTail[iCurAppendIndex] = iAppendPos + iLength;
    if (iIfStartFrame)
    {
        pESBuffer->piHead[iCurAppendIndex] = iAppendPos;
        pESBuffer->iIndexTail = (pESBuffer->iIndexTail + 1) % MBUFFER_ES_MAX_FRAMES_IN_BUFFER;
    }

    return(MBUFFER_EC_OK);
}
#endif

int MBUFFERESFirstFrameAdvance(int iLength, MBUFFERESBuffer *pESBuffer)
{
#ifdef USE_DETACHED_ESBUFFER
    pESBuffer->iCurHead += iLength;
    if (pESBuffer->iCurHead == pESBuffer->piBufferLength[pESBuffer->iIndexHead])
        return MBUFFERESAdvanceFrame(pESBuffer);
#else
    pESBuffer->piHead[pESBuffer->iIndexHead] += iLength;
    if (pESBuffer->piHead[pESBuffer->iIndexHead] == pESBuffer->piTail[pESBuffer->iIndexHead])
        return MBUFFERESAdvanceFrame(pESBuffer);
#endif
    return(MBUFFER_EC_OK);
}

int MBUFFERESPrevFrameNo(int iFrameNo, MBUFFERESBuffer *pESBuffer)
{
    return((iFrameNo - 1 +  MBUFFER_ES_MAX_FRAMES_IN_BUFFER) % MBUFFER_ES_MAX_FRAMES_IN_BUFFER);
}


int MBUFFERESLastFrameNo(MBUFFERESBuffer *pESBuffer)
{
    return((pESBuffer->iIndexTail - 1 +  MBUFFER_ES_MAX_FRAMES_IN_BUFFER) % MBUFFER_ES_MAX_FRAMES_IN_BUFFER);
}

#ifndef USE_DETACHED_ESBUFFER
int MBUFFERESLastFrameAdvance(int iLength, MBUFFERESBuffer *pESBuffer)
{
    int iCurIndex = MBUFFERESLastFrameNo(pESBuffer);
    pESBuffer->piHead[iCurIndex] += iLength;
    return(MBUFFER_EC_OK);
}
#endif

unsigned char *MBUFFERESFirstFramePos(MBUFFERESBuffer *pESBuffer)
{
    if (pESBuffer->iIndexHead == pESBuffer->iIndexTail)
        return(NULL);
    else
#ifdef USE_DETACHED_ESBUFFER
        return(pESBuffer->pucBuffer[pESBuffer->iIndexHead] + pESBuffer->iCurHead);
#else
        return(pESBuffer->pucBuffer + pESBuffer->piHead[pESBuffer->iIndexHead]);
#endif
}

int MBUFFERESFirstFrameLength(MBUFFERESBuffer *pESBuffer)
{
    if (pESBuffer->iIndexHead == pESBuffer->iIndexTail)
        return(0);
    else
#ifdef USE_DETACHED_ESBUFFER
        return(pESBuffer->piBufferLength[pESBuffer->iIndexHead] - pESBuffer->iCurHead);
#else
        return((pESBuffer->piTail[pESBuffer->iIndexHead] - pESBuffer->piHead[pESBuffer->iIndexHead]) % (pESBuffer->iSize));
#endif
}

int MBUFFERESGetAdjustedSideInfo(MBUFFERESSideInfo *pSideInfo, MBUFFERESBuffer *pESBuffer)
{
    long long lMaxPTS;  // candidate for TS wrap around point:
    // adjust PTS/DTS for wraparound
    lMaxPTS = ((pESBuffer->lLastTS + pESBuffer->iTimeScale * MBUFFER_SEVERAL_SECONDS) >> MBUFFER_TS_WRAPAROUND_BIT) << MBUFFER_TS_WRAPAROUND_BIT;
    if ((pSideInfo->PTS < pESBuffer->lLastTS) /* if PTS < lLastTS */ &&
        pSideInfo->PTS < pESBuffer->iTimeScale * MBUFFER_SEVERAL_SECONDS /* if PTS is small enough*/ &&
        lMaxPTS /* if lLastTS is big enough */ &&
        pESBuffer->lLastTS < lMaxPTS /* if lLastTS < candidate */)
    {
        // wrap around happens
        pESBuffer->lPrevLastAdjustedTS = pESBuffer->lLastAdjustedTS;
        pESBuffer->lPrevLastTS = pESBuffer->lLastTS;
        pESBuffer->lLastAdjustedTS += pSideInfo->PTS + lMaxPTS - pESBuffer->lLastTS;
        pESBuffer->lLastTS = pSideInfo->PTS;
    }
    // check if we should apply Prev or current adjustment
    if ((pSideInfo->PTS >= pESBuffer->lLastTS /* if PTS > PTS for current adjustment */ &&
        pSideInfo->PTS - pESBuffer->lLastTS <= pESBuffer->iTimeScale * MBUFFER_SEVERAL_SECONDS /* PTS - PTS for current adjustment is small enough */) ||
        (pSideInfo->PTS < pESBuffer->lLastTS /* if PTS < PTS for current adjustment */ &&
        pESBuffer->lLastTS - pSideInfo->PTS <= pESBuffer->iTimeScale * MBUFFER_SEVERAL_SECONDS /* PTS for current adjustment - PTS is small enough */))
    {
        if (pSideInfo->PTS != pESBuffer->lLastTS)
        {
            pESBuffer->lPrevLastAdjustedTS = pESBuffer->lLastAdjustedTS;
            pESBuffer->lPrevLastTS = pESBuffer->lLastTS;
        }
        pSideInfo->PTS += pESBuffer->lLastAdjustedTS - pESBuffer->lLastTS;
        pSideInfo->DTS += pESBuffer->lLastAdjustedTS - pESBuffer->lLastTS;
    }
    else
    {
        // it's possible the we may want to apply previous adjustment (such as B picture)
        pSideInfo->PTS += pESBuffer->lPrevLastAdjustedTS - pESBuffer->lPrevLastTS;
        pSideInfo->DTS += pESBuffer->lPrevLastAdjustedTS - pESBuffer->lPrevLastTS;
    }
    return(MBUFFER_EC_OK);
}

MBUFFERESSideInfo *MBUFFERESFirstFrameSideInfo(MBUFFERESBuffer *pESBuffer)
{
    if (pESBuffer->iIndexHead == pESBuffer->iIndexTail)
        return(NULL);
    else
        return(&(pESBuffer->pSideInfo[pESBuffer->iIndexHead]));
}

MBUFFERESSideInfo *MBUFFERESLastFrameSideInfo(MBUFFERESBuffer *pESBuffer)
{
    if (pESBuffer->iIndexHead == pESBuffer->iIndexTail)
        return(NULL);
    else
        return(&(pESBuffer->pSideInfo[MBUFFERESLastFrameNo(pESBuffer)]));
}

int MBUFFERESGetLastFrameSideInfo(MBUFFERESSideInfo *pSideInfo, MBUFFERESBuffer *pESBuffer)
{
    if (pESBuffer->iIndexHead == pESBuffer->iIndexTail)
    {
        return(MBUFFER_EC_FAIL);
    }
    else
    {
        *pSideInfo = pESBuffer->pSideInfo[MBUFFERESLastFrameNo(pESBuffer)];
        return MBUFFERESGetAdjustedSideInfo(pSideInfo, pESBuffer);
    }
    
}

MBUFFERESSideInfo *MBUFFERESLastTwoFrameSideInfo(MBUFFERESBuffer *pESBuffer)
{
    if (pESBuffer->iIndexHead == ((pESBuffer->iIndexTail - 1 + MBUFFER_ES_MAX_FRAMES_IN_BUFFER) % MBUFFER_ES_MAX_FRAMES_IN_BUFFER))
        return(NULL);
    else
        return(&(pESBuffer->pSideInfo[(MBUFFERESLastFrameNo(pESBuffer) - 1 + MBUFFER_ES_MAX_FRAMES_IN_BUFFER) % MBUFFER_ES_MAX_FRAMES_IN_BUFFER]));
}

int MBUFFERESGetLastTwoFrameSideInfo(MBUFFERESSideInfo *pSideInfo, MBUFFERESBuffer *pESBuffer)
{
    if (pESBuffer->iIndexHead == ((pESBuffer->iIndexTail - 1 + MBUFFER_ES_MAX_FRAMES_IN_BUFFER) % MBUFFER_ES_MAX_FRAMES_IN_BUFFER))
    {
        return(MBUFFER_EC_FAIL);
    }
    else
    {
        *pSideInfo = pESBuffer->pSideInfo[(MBUFFERESLastFrameNo(pESBuffer) - 1 + MBUFFER_ES_MAX_FRAMES_IN_BUFFER) % MBUFFER_ES_MAX_FRAMES_IN_BUFFER];
        return MBUFFERESGetAdjustedSideInfo(pSideInfo, pESBuffer);
    }

}

unsigned char *MBUFFERESLastFramePos(MBUFFERESBuffer *pESBuffer)
{
    if (pESBuffer->iIndexHead == pESBuffer->iIndexTail)
        return(NULL);
    else
#ifdef USE_DETACHED_ESBUFFER
        return(pESBuffer->pucBuffer[MBUFFERESLastFrameNo(pESBuffer)]);
#else
        return(pESBuffer->pucBuffer + pESBuffer->piHead[MBUFFERESLastFrameNo(pESBuffer)]);
#endif
}

int MBUFFERESLastFrameLength(MBUFFERESBuffer *pESBuffer)
{
    int iCurIndex = MBUFFERESLastFrameNo(pESBuffer);
    if (pESBuffer->iIndexHead == pESBuffer->iIndexTail)
        return(0);
    else
#ifdef USE_DETACHED_ESBUFFER
        return(pESBuffer->piBufferLength[iCurIndex]);
#else
        return(pESBuffer->piTail[iCurIndex] - pESBuffer->piHead[iCurIndex]);
#endif
}

int MBUFFERESSetSideInfo(MBUFFERESSideInfo *pSideInfo, MBUFFERESBuffer *pESBuffer)
{
    int iCurIndex = MBUFFERESLastFrameNo(pESBuffer);
    pESBuffer->pSideInfo[iCurIndex] = *pSideInfo;
    if (pESBuffer->lLastTS == -1)
    {
        pESBuffer->lLastTS = pESBuffer->lLastAdjustedTS = pESBuffer->lPrevLastTS = pESBuffer->lPrevLastAdjustedTS = pSideInfo->PTS;
    }
    return(0);
}

int MBUFFERESSetKeyFrame(int bIfKeyFrame, MBUFFERESBuffer *pESBuffer)
{
    int iCurIndex = MBUFFERESLastFrameNo(pESBuffer);
    MBUFFERESSideInfoSetIfKeyFrame(bIfKeyFrame, &(pESBuffer->pSideInfo[iCurIndex]));
    return(MBUFFER_EC_OK);
}

int MBUFFERESGetSideInfo(MBUFFERESSideInfo *pSideInfo, MBUFFERESBuffer *pESBuffer)
{
    if (pESBuffer->iIndexHead == pESBuffer->iIndexTail)
    {
        return(MBUFFER_EC_FAIL);
    }
    else
    {
        *pSideInfo = pESBuffer->pSideInfo[pESBuffer->iIndexHead];
        return MBUFFERESGetAdjustedSideInfo(pSideInfo, pESBuffer);
    }
}

#ifndef USE_DETACHED_ESBUFFER
int MBUFFERESSplitFrame(int iPos, MBUFFERESBuffer *pESBuffer)
{
    // iPos: offset to the start of the frame t b split
    int iCurIndex = MBUFFERESLastFrameNo(pESBuffer);
    pESBuffer->piTail[pESBuffer->iIndexTail] = pESBuffer->piTail[iCurIndex];
    pESBuffer->piTail[iCurIndex] = pESBuffer->piHead[pESBuffer->iIndexTail] = pESBuffer->piHead[iCurIndex] + iPos;
    pESBuffer->iIndexTail = (pESBuffer->iIndexTail + 1) % MBUFFER_ES_MAX_FRAMES_IN_BUFFER;
    return(MBUFFER_EC_OK);
}

int MBUFFERESThrowLastFrame(int bIfRecede, MBUFFERESBuffer *pESBuffer)
{
    int iCurIndex = MBUFFERESLastFrameNo(pESBuffer);
    pESBuffer->piTail[iCurIndex] = pESBuffer->piHead[iCurIndex];
    // recede the iIndexTail when throw last frame
    if (bIfRecede)
    {
        if (pESBuffer->iIndexHead == pESBuffer->iIndexTail)
            return(MBUFFER_EC_FAIL);
        pESBuffer->iIndexTail = (pESBuffer->iIndexTail + MBUFFER_ES_MAX_FRAMES_IN_BUFFER - 1) % MBUFFER_ES_MAX_FRAMES_IN_BUFFER;
    }
    return(MBUFFER_EC_OK);
}
#endif

int MBUFFERESNumFrames(MBUFFERESBuffer *pESBuffer)
{
    int iNumFrames;

    iNumFrames = pESBuffer->iIndexTail - pESBuffer->iIndexHead;
    if (iNumFrames < 0)
        iNumFrames += MBUFFER_ES_MAX_FRAMES_IN_BUFFER;
    return(iNumFrames);
}

int MBUFFERESAdvanceFrame(MBUFFERESBuffer *pESBuffer)
{
    if (pESBuffer->iIndexHead == pESBuffer->iIndexTail)
        return(MBUFFER_EC_FAIL);
#ifdef USE_DETACHED_ESBUFFER
    pESBuffer->iIndexHead = (pESBuffer->iIndexHead + 1) % pESBuffer->iSize;
#else
    pESBuffer->iIndexHead = (pESBuffer->iIndexHead + 1) % MBUFFER_ES_MAX_FRAMES_IN_BUFFER;
#endif

    return(MBUFFER_EC_OK);
}

int MBUFFERESGetFrame(unsigned char** pucFramePos, int *piLength, MBUFFERESBuffer *pESBuffer)
{
    // return MBUFFER_EC_OK if got a regular frame. The caller should make sure the data is processed before it calls MBUFFERESAdvanceFrame() to
    //          move to the next frame. Otherwise there maybe risk that the data is trashed by new coming.
    //          MBUFFER_EC_EXTRADATA0/MBUFFER_EC_EXTRADATA1/MBUFFER_EC_SKIP if, e.g., sps/pps in case of AVC
    //          The caller should copy sps/pps based on *piLength, then calls MBUFFERESFirstFrameAdvance(*piLength).
    unsigned char *pucPos;
    int iPos, iLength, iFoundNextNALU, ret = MBUFFER_EC_OK;
    *pucFramePos = MBUFFERESFirstFramePos(pESBuffer);
    *piLength = iLength = MBUFFERESFirstFrameLength(pESBuffer);
    switch (pESBuffer->iCodec)
    {
    case MBUFFER_ES_CODEC_AVC:
        iPos = 0;
        pucPos = *pucFramePos;
        while (pucPos[iPos] == 0 && pucPos[iPos + 1] == 0 && pucPos[iPos + 2] == 0)
            iPos++;

        if (pucPos[iPos] == 0 && pucPos[iPos + 1] == 0 && pucPos[iPos + 2] == 1 && ((pucPos[iPos+3] & 0x1f) == 0x07))
        {
            // sps
            ret = MBUFFER_EC_EXTRADATA0;
        }
        else if (pucPos[iPos] == 0 && pucPos[iPos+1] == 0 && pucPos[iPos+2] == 1 && ((pucPos[iPos+3] & 0x1f) == 0x08))
        {
            // pps
            ret = MBUFFER_EC_EXTRADATA1;
        }
        else if (pucPos[iPos] == 0 && pucPos[iPos+1] == 0 && pucPos[iPos+2] == 1 && ((pucPos[iPos+3] & 0x1f)  == 0x09))
        {
            // delimiter
            ret = MBUFFER_EC_SKIP;
        }
        else if (pucPos[iPos] == 0 && pucPos[iPos+1] == 0 && pucPos[iPos+2] == 1 && ((pucPos[iPos+3] & 0x1f)  == 0x06))
        {
            // SEI
            ret = MBUFFER_EC_SEI;
        }

        if (ret != MBUFFER_EC_OK)
        {
            // find next nalu
            iFoundNextNALU = 0;
            iPos += 4;
            while (iPos < iLength - 3)
            {
                if (pucPos[iPos] == 0 && pucPos[iPos + 1] == 0 && pucPos[iPos + 2] == 1)
                {
                    iFoundNextNALU = 1;
                    break;
                }
                else
                    iPos++;
            }
            if (iFoundNextNALU)
                *piLength = iPos - 1;
        }
        break;
    default:
        break;
    }
    return(ret);
}

#if 0
unsigned char *MBUFFERESPosForData(int iLength, MBUFFERESBuffer *pESBuffer)
{
    int iAppendPos = 0;
    int iHeadPos, iTailPos;

    iHeadPos = pESBuffer->piHead[pESBuffer->iIndexHead];
    iTailPos = pESBuffer->piTail[pESBuffer->iLastIndex];
    if ((iTailPos < iHeadPos) || (pESBuffer->iSize - iTailPos >= iLength))
            iAppendPos = iTailPos;

}
#endif

int MBUFFERESReadFromFile(FILE *fp_input, int iLength, MBUFFERESBuffer *pESBuffer)
{
#ifndef USE_DETACHED_ESBUFFER
    int iAppendPos = 0;
    int iHeadPos, iTailPos;
    int iLastIndex, iCurAppendIndex;

    if (MBUFFERESBufferSpaceAvailable(1, pESBuffer) < iLength)
        return(MBUFFER_EC_FAIL);
    if ((pESBuffer->iIndexTail + 1) % MBUFFER_ES_MAX_FRAMES_IN_BUFFER == pESBuffer->iIndexHead)
        return(MBUFFER_EC_FAIL);

    iLastIndex = (pESBuffer->iIndexTail - 1 + MBUFFER_ES_MAX_FRAMES_IN_BUFFER) % MBUFFER_ES_MAX_FRAMES_IN_BUFFER;
    iCurAppendIndex = pESBuffer->iIndexTail;

    if (pESBuffer->iIndexHead != pESBuffer->iIndexTail)
    {
        iHeadPos = pESBuffer->piHead[pESBuffer->iIndexHead];
        iTailPos = pESBuffer->piTail[iLastIndex];
        if ((iTailPos < iHeadPos) || (pESBuffer->iSize - iTailPos >= iLength))
            iAppendPos = iTailPos;
    }
    fread(pESBuffer->pucBuffer + iAppendPos, iLength, 1, fp_input);
    pESBuffer->piTail[iCurAppendIndex] = iAppendPos + iLength;
    pESBuffer->piHead[iCurAppendIndex] = iAppendPos;
    pESBuffer->iIndexTail = (pESBuffer->iIndexTail + 1) % MBUFFER_ES_MAX_FRAMES_IN_BUFFER;
#else
    fread(pESBuffer->pucBuffer[pESBuffer->iIndexTail], iLength, 1, fp_input);
    pESBuffer->piBufferLength[pESBuffer->iIndexTail] = iLength;
    pESBuffer->iIndexTail = (pESBuffer->iIndexTail + 1) % pESBuffer->iSize;
#endif

    return(MBUFFER_EC_OK);
}

int MBUFFERESWriteFirstFrameToFile(FILE *fp_output, MBUFFERESBuffer *pESBuffer)
{
    fwrite(MBUFFERESFirstFramePos(pESBuffer),
        MBUFFERESFirstFrameLength(pESBuffer),
        1, fp_output);
    MBUFFERESAdvanceFrame(pESBuffer);
    return(MBUFFER_EC_OK);
}

// ByteArray related
int MBUFFERByteArrayInit(unsigned char *pucBuffer, int iHead, int iTail, int iSize, MBUFFERByteArray *pByteArray)
{
    if (pucBuffer == NULL)
    {
        pByteArray->pucBuffer = (unsigned char *)malloc(iSize);
        pByteArray->iEnd = 0;
    }
    else
    {
        pByteArray->pucBuffer = pucBuffer + iHead;
        pByteArray->iEnd = iTail - iHead;
    }
    pByteArray->iSize = iSize;
    pByteArray->iCurPos = 0;

    return(MBUFFER_EC_OK);
}

int MBUFFERByteArrayLength(MBUFFERByteArray *pByteArray)
{
    return(pByteArray->iEnd - pByteArray->iCurPos);
}

int MBUFFERByteArrayCurPos(MBUFFERByteArray *pByteArray)
{
    return(pByteArray->iCurPos);
}

int MBUFFERByteArrayCopy(unsigned char *pucDest, int iLength, MBUFFERByteArray *pByteArray)
{
    memcpy(pucDest, pByteArray->pucBuffer + pByteArray->iCurPos, iLength);
    pByteArray->iCurPos += iLength;
    return(MBUFFER_EC_OK);
}

int MBUFFERByteArrayAdvance(int iLength, MBUFFERByteArray *pByteArray)
{
    pByteArray->iCurPos += iLength;
    return(MBUFFER_EC_OK);
}

int MBUFFERByteArraySeek(int iOffset, MBUFFERByteArray *pByteArray)
{
    pByteArray->iCurPos = iOffset;
    return(MBUFFER_EC_OK);
}

int MBUFFERByteArrayMove(int iDestPos, int iSrcPos, MBUFFERByteArray *pByteArray)
{
    int iLength = pByteArray->iEnd - iSrcPos;
    memcpy(pByteArray->pucBuffer + iDestPos, pByteArray->pucBuffer + iSrcPos, iLength);
    pByteArray->iCurPos -= iSrcPos - iDestPos;
    pByteArray->iEnd -= iSrcPos - iDestPos;
    return(MBUFFER_EC_OK);
}

int MBUFFERByteArrayMove2Head(MBUFFERByteArray *pByteArray)
{
    return(MBUFFERByteArrayMove(0, pByteArray->iCurPos, pByteArray));
}

int MBUFFERByteArrayReadFromFile(FILE *fp_input, int iLength, MBUFFERByteArray *pByteArray)
{
    fread(pByteArray->pucBuffer + pByteArray->iEnd, iLength, 1, fp_input);
    pByteArray->iEnd += iLength;
    return(MBUFFER_EC_OK);
}

int MBUFFERByteArrayDeInit(int bIfFreeBuffer, MBUFFERByteArray *pByteArray)
{
    if (bIfFreeBuffer)
    {
        free(pByteArray->pucBuffer);
        pByteArray->pucBuffer = NULL;
        pByteArray->iSize = 0;
    }
    pByteArray->iCurPos = 0;
    return(MBUFFER_EC_OK);
}

int MBUFFERESAddExtraInfo(unsigned char *pucExtraInfo, int iExtraInfoLength, MBUFFERESBuffer *pESBuffer)
{
    int iRet = MBUFFERESAppendData(pucExtraInfo, iExtraInfoLength, 1, pESBuffer);
    if (iRet == MBUFFER_EC_OK)
    {
        MBUFFERESSideInfo sideInfo;
        sideInfo.iSize = iExtraInfoLength;
        MBUFFERESSideInfoSetFrameType(MBUFFER_ES_FRAME_TYPE_EXTRAINFOFRAME, &sideInfo);
        sideInfo.PTS = -1;
        sideInfo.DTS = -1;
        MBUFFERESSetSideInfo(&sideInfo, pESBuffer);
    }
    return iRet;
}

int MBUFFERESSideInfoSetIfKeyFrame(int bIfKeyFrame, MBUFFERESSideInfo *pSideInfo)
{
    pSideInfo->iFlags = (pSideInfo->iFlags & 0xefffffff) | (bIfKeyFrame << 28);
    return (MBUFFER_EC_OK);
}

int MBUFFERESSideInfoGetIfKeyFrame(MBUFFERESSideInfo *pSideInfo)
{
    return ((pSideInfo->iFlags & 0x10000000) >> 28);
}

int MBUFFERESSideInfoSetFrameType(int iFramType, MBUFFERESSideInfo *pSideInfo)
{
    pSideInfo->iFlags = (pSideInfo->iFlags & 0x1fffffff) | (iFramType << 29);
    return (MBUFFER_EC_OK);
}

int MBUFFERESSideInfoGetFrameType(MBUFFERESSideInfo *pSideInfo)
{
    return ((pSideInfo->iFlags & 0xe0000000) >> 29);
}