#ifndef _MBUFFER_H_
#define _MBUFFER_H_
#if defined(__cplusplus)
extern "C"
{
#endif
#include <stdio.h>

#define MBUFFER_SYS_MAX_PACKET_SIZE     376 // for 2 whole TS packets
#define MBUFFER_ES_MAX_FRAMES_IN_BUFFER 128

#define MBUFFER_EC_OK                0
#define MBUFFER_EC_FAIL              -1
#define MBUFFER_EC_NO_ENOUGH_DATA    -2

#define MBUFFER_EC_EXTRADATA0    1
#define MBUFFER_EC_EXTRADATA1    2
#define MBUFFER_EC_SKIP          3
#define MBUFFER_EC_SEI           4

#define MBUFFER_ES_CODEC_AVC    0
#define MBUFFER_ES_CODEC_AAC    1

#define MBUFFER_SEVERAL_SECONDS      16  // used for TS wrap around calculation
#define MBUFFER_TS_WRAPAROUND_BIT    27  // lowest possible bit for TS wrap around

#define MBUFFER_ES_FRAME_TYPE_NORMALFRAME      0
#define MBUFFER_ES_FRAME_TYPE_EXTRAINFOFRAME   1

// Extracts iFlags of SideInfo to get whether is a key frame or not
#define MBUFFERESISKEYFRAME(iFlags) ((iFlags & 0x10000000) >> 28)
// Sets bIfKey to iFlags of SideInfo
#define MBUFFERESSETKEYFRAME(bIfKey, iFlags) (iFlags = ((iFlags) & 0xefffffff) | (bIfKey << 28))

// Extracts iFlags of Side to get frame type
#define MBUFFERESGETFRAMETYPE(iFlags) ((iFlags & 0xe0000000) >> 29)
// Sets iType to iFlags of SideInfo
#define MBUFFERESSETFRAMETYPE(iType, iFlags) (iFlags = ((iFlags) & 0x1fffffff) | (iType << 29))

// Three buffers are used:
//  * buffer for data received from file or network
//  * buffer for video data
//  * buffer for audio data

// system buffer is the buffer to receive data from network/file;
// It's a circular buffer
typedef struct {
    unsigned char *pucBuffer;
    unsigned char pucBufferCache[MBUFFER_SYS_MAX_PACKET_SIZE];
        // in case the data wrapped around we copy a whole TS packet to this small cache buffer
        // at process the packet from it.
    unsigned char *pucCurStart;
    int iSize;
    int iHead;
    int iTail;

    // statistics: total num of bytes input/output
    int iNumBytesInput;
    int iNumBytesOutput;
} MBUFFERSYSBuffer;

// SideInfo stores information about a frame data in ES buffer
// the foramt of iFlags(in big endian):
// bits         meaning         range
// 3 bits       frame type      0:normal frame; 1:extra info frame
// 1 bit        is key frame    0:not key frame; 1: key frame
// 28 bits      reserved        0
typedef struct {
    int iSize;
    int iFlags;
    long long PTS;
    long long DTS;
} MBUFFERESSideInfo;

// ES buffer is the buffer for elementary bitstream, audio or video
// It's a circular buffer but a decodable packet (a whold picture, etc.) is continuous in the buffer
typedef struct {
#ifdef USE_DETACHED_ESBUFFER
    unsigned char *pucBuffer[MBUFFER_ES_MAX_FRAMES_IN_BUFFER];
    int piBufferLength[MBUFFER_ES_MAX_FRAMES_IN_BUFFER];
    int iCurHead;
#else
    unsigned char *pucBuffer;
    int piHead[MBUFFER_ES_MAX_FRAMES_IN_BUFFER];
    int piTail[MBUFFER_ES_MAX_FRAMES_IN_BUFFER];
#endif
    int iSize;
    MBUFFERESSideInfo pSideInfo[MBUFFER_ES_MAX_FRAMES_IN_BUFFER];
    int iTimeScale;             // example: 90000 means 1 is 1/90000s
    long long lLastTS;          //
    long long lLastAdjustedTS;  //
    long long lPrevLastTS;          //
    long long lPrevLastAdjustedTS;  //
    int iIndexHead; // point to the start position in piHead/piTail,which points to the first valid frame in buffer
    int iIndexTail; // point to the end position in piHead/piTail,which points to the first valid frame in buffer
    int iCodec;
} MBUFFERESBuffer;

typedef struct {
    unsigned char *pucBuffer;
    int iCurPos;
    int iEnd;
    int iSize;
} MBUFFERByteArray;

// struct for audio extra info
typedef struct {
    int iSampleRate;
    int iNumChannels;
    int iNumBits;           // 16 or 8
} MBUFFERESAUDIOEXTRAINFO;


// public APIs
// macro definitions
#define MBUFFERESFIRSTFRAMESIDEINFO(pESBuffer) (&((pESBuffer)->pSideInfo[(pESBuffer)->iIndexHead]))

/**
*  @method: MBUFFERSYSBufferInit
*  @abstract: init MBUFFERSYSBuffer object pointed by pSYSBuffer.
*
*  @param (in) pucBuffer    specifies the buffer where SYSBuffer will store data.
*                           if NULL, a new buffer with size equal to iSize will be allocated dynamically.
*  @param (in) iHead    specifies the initial value for iHead variable in SYSBuffer.
*  @param (in) iTail    specifies the initial value for iTail variable in SYSBuffer.
*  @param (in) iSize    specifies the size of buffer.
*  @param (in-out) pSYSBuffer specifies the SYSBuffer that will be initialized.
*
*  @return MBUFFER_EC_OK.
*/
extern int MBUFFERSYSBufferInit(unsigned char *pucBuffer, int iHead, int iTail, int iSize, MBUFFERSYSBuffer *pSYSBuffer);


/**
*  @method: MBUFFERSYSBufferDeInit
*  @abstract: deinit MBUFFERSYSBuffer object. set iSize/iHead/iTail to 0, and free the buffer if bIfFreeBuffer is 1;
*
*  @param (in) bIfFreeBuffer    indicates whether need to free the buffer pointed by pucBuffer.
*  @param (in-out) pSYSBuffer   specifies the SYSBuffer that will be deinitialized.
*
*  @return MBUFFER_EC_OK
*/
extern int MBUFFERSYSBufferDeInit(int bIfFreeBuffer, MBUFFERSYSBuffer *pSYSBuffer);


/**
*  @method: MBUFFERSYSBufferSpaceAvailable
*  @abstract: get the size of available space in Buffer pointed by pucBuffer.
*             it should be called before writing data to SYSBuffer.
*
*  @param (in) pSYSBuffer
*
*  @return the size of available space
*/
extern int MBUFFERSYSBufferSpaceAvailable(MBUFFERSYSBuffer *pSYSBuffer);


/**
*  @method: MBUFFERSYSBufferSpaceAvailableToEnd
*  @abstract: get the size of available space from current tail index to the end of Buffer.
*             it should be called before writing data to SYSBuffer.
*
*  @param (in) pSYSBuffer
*
*  @return the size of available space
*/
extern int MBUFFERSYSBufferSpaceAvailableToEnd(MBUFFERSYSBuffer *pSYSBuffer);


/**
*  @method: MBUFFERSYSBufferLength
*  @abstract: get the size of data in Buffer pointed by pucBuffer
*             it should be called before reading data from SYSBuffer.
*
*  @param (in) pSysBuffer
*
*  @return the size of data
*/
extern int MBUFFERSYSBufferLength(MBUFFERSYSBuffer *pSysBuffer);


/**
*  @method: MBUFFERSYSBufferLengthToEnd
*  @abstract: get the size of data from current iHead index to the end of Buffer
*             it should be called before reading data from SYSBuffer.
*
*  @param (in) pSYSBuffer
*
*  @return the size of data
*/
extern int MBUFFERSYSBufferLengthToEnd(MBUFFERSYSBuffer *pSYSBuffer);


/**
*  @method: MBUFFERSYSValueAtPos
*  @abstract: get value of the byte that at the specified position in the Buffer.
*
*  @param (in) iPos     specifies the position.
*  @param (in) pSYSBuffer
*
*  @return the value of the byte specified by iPos
*/
extern unsigned char MBUFFERSYSValueAtPos(int iPos, MBUFFERSYSBuffer *pSYSBuffer);


/**
*  @method: MBUFFERSYSAppendDataPos
*  @abstract: get a pointer to the first available byte starting from iTail.
*             it should be called before writing data to Buffer
*
*  @param (in) pSYSBuffer
*
*  @return pointer to the first available byte
*/
extern unsigned char *MBUFFERSYSAppendDataPos(MBUFFERSYSBuffer *pSYSBuffer);


/**
*  @method: MBUFFERSYSAppendData
*  @abstract: append data to Buffer.
*             it should be called before writing data to Buffer
*
*  @param (in) pucData  a pointer to the data that will be written to Buffer.
*  @param (in) iLength  specifies the size of data.
*  @param (in) pSYSBuffer
*
*  @return MBUFFER_EC_OK if succeeds;
*          MBUFFER_EC_FAIL if fails, and the reason may be that the length of available space is less than iLength
*/
extern int MBUFFERSYSAppendData(unsigned char *pucData, int iLength, MBUFFERSYSBuffer *pSYSBuffer);

/**
*  @method: MBUFFERSYSCopyByteArray2SYSBuffer
*  @abstract: 将pucData指定的数据拷贝到SYSBuffer中。
*             拷贝时，如果SYSBuffer中可用空间大于等于iLength，则将iLength个字节的数据拷贝到SYSBuffer中；
*             如果可用空间小于iLength,则拷贝的字节数等于可用空间大小
*
*  @param (in) pucData 指向带拷贝的数据
*  @param (in) iLength 指定带拷贝数据的长度
*  @param (in) pSYSBuffer 指向SYSBuffer
*
*  @return 返回实际拷贝的字节数
*/
extern int MBUFFERSYSCopyByteArray2SYSBuffer(unsigned char *pucData, int iLength, MBUFFERSYSBuffer *pSYSBuffer);

/**
*  @method: MBUFFERSYSAdvance
*  @abstract: increase iHead of SYSBuffer by iLength.
*             after reading data from Buffer, the function should be called to update the index of SYSBuffer
*
*  @param (in) iLength  specifies the iLength that should be added to iHead
*  @param (in-out) pSYSBuffer
*
*  @return MBUFFER_EC_OK if succeeds;
*          MBUFFER_EC_FAIL if fails, and the reason may be that the length of data is less than iLength
*/
extern int MBUFFERSYSAdvance(int iLength, MBUFFERSYSBuffer *pSYSBuffer);


/**
*  @method: MBUFFERSYSExtend
*  @abstract: increase iTail of SYSBuffer by iLength.
*             after writing data to Buffer, the function should be called to update the index of SYSBuffer
*
*  @param (in) iLength  specifies the iLength that should be added to iTail
*  @param (in-out) pSYSBuffer
*
*  @return MBUFFER_EC_OK
*/
extern int MBUFFERSYSExtend(int iLength, MBUFFERSYSBuffer *pSYSBuffer);


/**
*  @method: MBUFFERSYSStartPos
*  @abstract: get a pointer to the first byte of data.
*
*  @param (in) pSYSBuffer
*
*  @return 1. the pointer to the first byte of data if Buffer has data;
*          2. NULL if Buffer is empty.
*/
extern unsigned char *MBUFFERSYSStartPos(MBUFFERSYSBuffer *pSYSBuffer);


/**
*  @method: MBUFFERSYSPosFromStart
*  @abstract: get a pointer to the byte that has an offset of @iOffset to iStart
*
*  @param (in) iOffset  specifies the offset
*  @param (in) pSYSBuffer
*
*  @return a pointer
*/
extern unsigned char *MBUFFERSYSPosFromStart(int iOffset, MBUFFERSYSBuffer *pSYSBuffer);

/**
*  @method: MBUFFERSYSBufferLengthToEndByOffset
*  @abstract: 从距离数据块首字节偏移量为iOffset的位置处，计算到Buffer结束位置处的有效数据大小
*             该函数通常在读取Buffer中的数据之前调用。
*  @param (in) iOffset 指定偏移大小
*  @param (in) pSYSBuffer
*
*  @return 数据大小
*/
extern int MBUFFERSYSBufferLengthToEndByOffset(int iOffset, MBUFFERSYSBuffer *pSYSBuffer);


/**
*  @method: MBUFFERSYSCopyToCache
*  @abstract: 将Buffer中指定长度的数据拷贝到Cache Buffer中
*
*  @param (in) iLength 指定要拷贝的数据长度
*  @param (in) pSYSBuffer
*
*  @return MBUFFER_EC_OK
*/
extern int MBUFFERSYSCopyToCache(int iLength, MBUFFERSYSBuffer *pSYSBuffer);

/**
*  @method: MBUFFERSYSCopyToCacheByOffset
*  @abstract: 从距离数据块首字节偏移量为iOffset处的字节处，开始拷贝指定长度的数据到Cache Buffer中
*
*  @param (in) iOffset 指定偏移量
*  @param (in) iLength 指定要拷贝的数据长度
*  @param (in) pSYSBuffer
*
*  @return MBUFFER_EC_OK
*/
extern int MBUFFERSYSCopyToCacheByOffset(int iOffset, int iLength, MBUFFERSYSBuffer *pSYSBuffer);

/**
*  @method: MBUFFERSYSGetContinousBuffer
*  @abstract: 获取一块连续内存的首地址，该内存保存了Buffer中离数据块首地址偏移量为iOffset的一段长度为iLength的数据。
*
*  @param (in) iOffset 指定偏移量
*  @param (in) iLength 指定数据长度
*  @param (in) pSYSBuffer
*
*  @return 返回数据块的首地址。
*/
extern unsigned char *MBUFFERSYSGetContinousBuffer(int iOffset, int iLength, MBUFFERSYSBuffer *pSYSBuffer);

/**
*  @method: MBUFFERSYSPeek2ByteArray
*  @abstract: 从Buffer中拷贝iLength个字节的数据到pucDest指向的内存中。该函数执行完成之后，被拷贝的数据还保存在Buffer中，可重复使用。
*
*  @param (in) pucDest 指向保存数据的内存地址
*  @param (in) iLength 指定要拷贝的数据长度
*  @param (in) pSYSBuffer
*
*  @return MBUFFER_EC_OK
*/
extern int MBUFFERSYSPeek2ByteArray(unsigned char *pucDest, int iLength, MBUFFERSYSBuffer *pSYSBuffer);

/**
*  @method: MBUFFERSYSCopy2ByteArray
*  @abstract: 从Buffer中拷贝iLength个字节的数据到pucDest指向的内存中。该函数执行完成之后，会将拷贝的数据从Buffer中扔掉，不可重复使用。
*
*  @param (in) pucDest 指向保存数据的内存地址
*  @param (in) iLength 指定要拷贝的数据长度
*  @param (in) pSYSBuffer
*
*  @return MBUFFER_EC_OK
*/
extern int MBUFFERSYSCopy2ByteArray(unsigned char *pucDest, int iLength, MBUFFERSYSBuffer *pSYSBuffer);

/**
*  @method: MBUFFERSYSNumBytesInput
*  @abstract: 从MBUFFERSYSBufferInit以后，至今一共获得过的数据字节数
*
*  @param (in) pSYSBuffer
*
*  @return 返回字节数
*/
extern int MBUFFERSYSNumBytesInput(MBUFFERSYSBuffer *pSYSBuffer);

/**
*  @method: MBUFFERSYSNumBytesOutput
*  @abstract: 从MBUFFERSYSBufferInit以后，至今一共输出过的数据字节数
*
*  @param (in) pSYSBuffer
*
*  @return 返回字节数
*/
extern int MBUFFERSYSNumBytesOutput(MBUFFERSYSBuffer *pSYSBuffer);

/**
*  @method: MBUFFERSYSReadFromFile
*  @abstract: 从文件中读取指定长度的数据到Buffer中
*
*  @param (in) fp_input 指向打开的文件
*  @param (in) iLength 指定要读取的字节数
*  @param (in) pSYSBuffer
*
*  @return 返回成功读取的字节数
*/
extern int MBUFFERSYSReadFromFile(FILE *fp_input, int iLength, MBUFFERSYSBuffer *pSYSBuffer);

/**
*  @method: MBUFFERSYSWriteToFile
*  @abstract: 将Buffer中iLength个字节的数据写入到文件中， 并更新SYSBuffer中的iStart变量
*
*  @param (in) fp_output 指向将要写入的文件
*  @param (in) iLength 指定要写入的字节数
*  @param (in) pSYSBuffer
*
*  @return 返回成功写入的数据长度
*/
extern int MBUFFERSYSWriteToFile(FILE *fp_output, int iLength, MBUFFERSYSBuffer *pSYSBuffer);

/**
*  @method: MBUFFERSYSWriteToFileNoAdvance
*  @abstract: 将Buffer中iLength个字节的数据写入到文件中， 但不更新SYSBuffer中的iStart变量
*
*  @param (in) fp_output 指向将要写入的文件
*  @param (in) iLength 指定要写入的字节数
*  @param (in) pSYSBuffer
*
*  @return 返回成功写入的数据长度
*/
extern int MBUFFERSYSWriteToFileNoAdvance(FILE *fp_output, int iLength, MBUFFERSYSBuffer *pSYSBuffer);

/**
*  @method: MBUFFERESBufferInit
*  @abstract: 初始化MBUFFERESBuffer对象。
*
*  @param (in) pucBuffer 如果不为NULL，则ESBuffer使用@pucBuffer指定的内存来保存数据；如果为NULL，则在初始化时由函数动态创建内存空间
*  @param (in) iSize 如果@pucBuffer不为NULL，则表示@pucBuffer指定内存的大小；如果为NULL，则指定需要创建的内存大小
*  @param (in-out) pESBuffer 指向需要初始化的ESBuffer对象
*
*  @return MBUFFER_EC_OK
*/
extern int MBUFFERESBufferInit(unsigned char *pucBuffer, int iSize, MBUFFERESBuffer *pESBuffer);

/**
*  @method: MBUFFERESBufferDeInit
*  @abstract: 重置或释放ESBuffer资源。
*
*  @param (in) bIfFreeBuffer 如果为1，则释放ESBuffer的资源；如果为0；则重置ESBuffer。
*  @param (in-out) pESBuffer 指向ESBuffer对象
*
*  @return MBUFFER_EC_OK
*/
extern int MBUFFERESBufferDeInit(int bIfFreeBuffer, MBUFFERESBuffer *pESBuffer);

/**
*  @method: MBUFFERESBufferSetCodec
*  @abstract: 设置视频的编码格式。
*
*  @param (in) iCodec 指定编码格式
*  @param (in-out) pESBuffer 指向ESBuffer对象
*
*  @return MBUFFER_EC_OK
*/
extern int MBUFFERESBufferSetCodec(int iCodec, MBUFFERESBuffer *pESBuffer);

/**
*  @method: MBUFFERESBufferCheck4KeyFrameTillPos
*  @abstract: 检查指定帧是否是关键帧。
*
*  @param (in) iFrame 指定待检查的帧的编号
*  @param (in) iSize 表示只对帧的前iSize个字节进行检查
*  @param (in-out) pESBuffer 指向ESBuffer对象
*
*  @return 1：表示是关键帧；0：表示不是关键帧
*/
extern int MBUFFERESBufferCheck4KeyFrameTillPos(int iFrame, int iSize, MBUFFERESBuffer *pESBuffer);

/**
*  @method: MBUFFERESBufferCheck4KeyFrame
*  @abstract: 检查指定帧是否是关键帧。
*
*  @param (in) iFrame 指定待检查的帧的编号
*  @param (in) pESBuffer 指向ESBuffer对象
*
*  @return 1：表示是关键帧；0：表示不是关键帧
*/
extern int MBUFFERESBufferCheck4KeyFrame(int iFrame, MBUFFERESBuffer *pESBuffer);

/**
*  @method: MBUFFERESBufferCheckLastFrame4KeyFrame
*  @abstract: 检查ESBuffer中最后一帧是否是关键帧。
*
*  @param (in) pESBuffer 指向ESBuffer对象
*
*  @return 1：表示是关键帧；0：表示不是关键帧
*/
extern int MBUFFERESBufferCheckLastFrame4KeyFrame(MBUFFERESBuffer *pESBuffer);

/**
*  @method: MBUFFERESBufferIsIndexFull
*  @abstract: 检查ESBuffer缓存是否已满。
*
*  @param (in) pESBuffer 指向ESBuffer对象
*
*  @return 1：表示已满；0：表示未满
*/
extern int MBUFFERESBufferIsIndexFull(MBUFFERESBuffer *pESBuffer);

/**
*  @method: MBUFFERESBufferSpaceAvailable
*  @abstract: 获取ESBuffer当前可使用的内存空间大小。
*
*  @param (in) iIfStartFrame 表示是否是一帧的开始
*  @param (in) pESBuffer 指向ESBuffer对象
*
*  @return 可用空间大小
*/
extern int MBUFFERESBufferSpaceAvailable(int iIfStartFrame, MBUFFERESBuffer *pESBuffer);

/**
*  @method: MBUFFERESAppendData
*  @abstract: 向ESBuffer中追加数据。
*
*  @param (in) pucData 指向待追加的数据
*  @param (in) iLength 表示数据的大小
*  @param (in) iIfStartFrame 表示当前添加的数据是否是帧的最开始部分
*  @param (in) pESBuffer 指向ESBuffer对象
*
*  @return MBUFFER_EC_OK
*/
extern int MBUFFERESAppendData(unsigned char *pucData, int iLength, int iIfStartFrame, MBUFFERESBuffer *pESBuffer);

/**
*  @method: MBUFFERESFirstFrameAdvance
*  @abstract: 将ESBuffer中当前第一帧的piHead向后移动iLength个字节。
*
*  @param (in) iLength 表示向前移动的字节数
*  @param (in-out) pESBuffer 指向ESBuffer对象
*
*  @return MBUFFER_EC_OK
*/
extern int MBUFFERESFirstFrameAdvance(int iLength, MBUFFERESBuffer *pESBuffer);

/**
*  @method: MBUFFERESLastFrameAdvance
*  @abstract: 将ESBuffer中当前最后一帧的piHead向后移动iLength个字节。
*
*  @param (in) iLength 表示向前移动的字节数
*  @param (in-out) pESBuffer 指向ESBuffer对象
*
*  @return MBUFFER_EC_OK
*/
extern int MBUFFERESLastFrameAdvance(int iLength, MBUFFERESBuffer *pESBuffer);

/**
*  @method: MBUFFERESAdvanceFrame
*  @abstract: 消费掉ESBuffer中的第一帧。
*
*  @param (in-out) pESBuffer 指向ESBuffer对象
*
*  @return MBUFFER_EC_OK: 表示执行成功； MBUFFER_EC_FAIL：执行失败，原因是ESBuffer无可使用的帧
*/
extern int MBUFFERESAdvanceFrame(MBUFFERESBuffer *pESBuffer);

/**
*  @method: MBUFFERESFirstFramePos
*  @abstract: 获取ESBuffer中第一帧首字节的地址。
*
*  @param (in) pESBuffer 指向ESBuffer对象
*
*  @return 成功则返回指向第一帧首字节的指针；失败返回NULL
*/
extern unsigned char *MBUFFERESFirstFramePos(MBUFFERESBuffer *pESBuffer);

/**
*  @method: MBUFFERESFirstFrameLength
*  @abstract: 获取ESBuffer中第一帧的长度。
*
*  @param (in) pESBuffer 指向ESBuffer对象
*
*  @return 如果ESBuffer中有可用帧，则返回第一帧的长度；如果ESBuffer为空，则返回0
*/
extern int MBUFFERESFirstFrameLength(MBUFFERESBuffer *pESBuffer);

/**
 *  @method         MBUFFERESFirstFrameSideInfo
 *  @abstract       Gets the SideInfo of the first frame in ESBuffer.
 *                  Just return the pointer stored in ESBuffer, will not adjust the PTS and DTS.
 *
 *  @param (in)     pESBuffer   pointer to ESBuffer instance
 *
 *  @return         NULL if there is no frame in ESBuffer, otherwise return a pointer of MBUFFERESSideInfo instance.
*/
extern MBUFFERESSideInfo *MBUFFERESFirstFrameSideInfo(MBUFFERESBuffer *pESBuffer);

/**
 *  @method         MBUFFERESLastFrameSideInfo
 *  @abstract       Gets the SideInfo of the last frame in ESBuffer.
 *                  Just return the pointer stored in ESBuffer, will not adjust the PTS and DTS.
 *
 *  @param (in)     pESBuffer   pointer to ESBuffer instance
 *
 *  @return         NULL if there is no frame in ESBuffer, otherwise return a pointer of MBUFFERESSideInfo instance.
*/
extern MBUFFERESSideInfo *MBUFFERESLastFrameSideInfo(MBUFFERESBuffer *pESBuffer);

/**
 *  @method         MBUFFERESGetLastFrameSideInfo
 *  @abstract       Gets the SideInfo of last frame in ESBuffer with adjusted PTS and DTS.
 *
 *  @param (out)    pSideInfo   pointer to the memory that stores the SideInfo
 *  @param (in)     pESBuffer   pointer to ESBuffer instance
 *
 *  @return         MBUFFER_EC_FAIL if no frame in ESBuffer
                    MBUFFER_EC_OK   if success
*/
extern int MBUFFERESGetLastFrameSideInfo(MBUFFERESSideInfo *pSideInfo, MBUFFERESBuffer *pESBuffer);

/**
 *  @method         MBUFFERESLastTwoFrameSideInfo
 *  @abstract       Gets the SideInfo of the last but one frame in ESBuffer.
 *                  Just return the pointer stored in ESBuffer, will not adjust the PTS and DTS.
 *
 *  @param (in)     pESBuffer   pointer to ESBuffer instance
 *
 *  @return         NULL if there is not enough frame in ESBuffer, otherwise return a pointer of MBUFFERESSideInfo instance.
*/
extern MBUFFERESSideInfo *MBUFFERESLastTwoFrameSideInfo(MBUFFERESBuffer *pESBuffer);

/**
 *  @method         MBUFFERESGetLastTwoFrameSideInfo
 *  @abstract       Gets the SideInfo of the last but one frame in ESBuffer with adjusted PTS and DTS.
 *
 *  @param (out)    pSideInfo   pointer to the memory that stores the SideInfo
 *  @param (in)     pESBuffer   pointer to ESBuffer instance
 *
 *  @return         MBUFFER_EC_FAIL if not enough frame in ESBuffer
                    MBUFFER_EC_OK   if success
*/
extern int MBUFFERESGetLastTwoFrameSideInfo(MBUFFERESSideInfo *pSideInfo, MBUFFERESBuffer *pESBuffer);

/**
*  @method: MBUFFERESLastFrameNo
*  @abstract: 返回ESBuffer中最后一帧的编号。
*
*  @param (in) pESBuffer 指向ESBuffer对象
*
*  @return 最后一帧的编号
*/
extern int MBUFFERESLastFrameNo(MBUFFERESBuffer *pESBuffer);

/**
*  @method: MBUFFERESLastFramePos
*  @abstract: 获取ESBuffer中最后一帧首字节的地址。
*
*  @param (in) pESBuffer 指向ESBuffer对象
*
*  @return 成功则返回指向最后一帧首字节的指针；失败返回NULL
*/
extern unsigned char *MBUFFERESLastFramePos(MBUFFERESBuffer *pESBuffer);

/**
*  @method: MBUFFERESLastFrameLength
*  @abstract: 获取ESBuffer中最后一帧的长度。
*
*  @param (in) pESBuffer 指向ESBuffer对象
*
*  @return 如果ESBuffer中有可用帧，则返回最后一帧的长度；如果ESBuffer为空，则返回0
*/
extern int MBUFFERESLastFrameLength(MBUFFERESBuffer *pESBuffer);

/**
*  @method: MBUFFERESSetSideInfo
*  @abstract: 设置ESBuffer中最后一帧的SideInfo信息。
*
*  @param (in) pSideInfo 指向SideInof对象
*  @param (in) pESBuffer 指向ESBuffer对象
*
*  @return 0
*/
extern int MBUFFERESSetSideInfo(MBUFFERESSideInfo *pSideInfo, MBUFFERESBuffer *pESBuffer);

/**
 *  @method         MBUFFERESGetSideInfo
 *  @abstract       Gets the SideInfo of first frame in ESBuffer with adjusted PTS and DTS.
 *
 *  @param (out)    pSideInfo   pointer to the memory that stores the SideInfo
 *  @param (in)     pESBuffer   pointer to ESBuffer instance
 *
 *  @return         MBUFFER_EC_FAIL if there is no frame in ESBuffer
                    MBUFFER_EC_OK   if success
*/
extern int MBUFFERESGetSideInfo(MBUFFERESSideInfo *pSideInfo, MBUFFERESBuffer *pESBuffer);

/**
*  @method: MBUFFERESSetKeyFrame
*  @abstract: 设置ESBuffer中最后一帧的关键帧标识。
*
*  @param (in) bIfKeyFrame 如果为1，则将最后一帧设置为关键帧；为0，则设置为非关键帧
*  @param (in) pESBuffer 指向ESBuffer对象
*
*  @return 0
*/
extern int MBUFFERESSetKeyFrame(int bIfKeyFrame, MBUFFERESBuffer *pESBuffer);

/**
*  @method: MBUFFERESSplitFrame
*  @abstract: 从追加进ESBuffer的数据中分割出一帧
*
*  @param (in) iPos 指定分割的位置
*  @param (in) pESBuffer 指向ESBuffer对象
*
*  @return MBUFFER_EC_OK
*/
extern int MBUFFERESSplitFrame(int iPos, MBUFFERESBuffer *pESBuffer);

/**
*  @method: MBUFFERESThrowLastFrame
*  @abstract: 扔掉ESBuffer中最后一帧的数据。
*
*  @param (in) bIfRecede 如果为0，函数仍保存最后一帧的元信息；为1则不保存
*  @param (in) pESBuffer 指向ESBuffer对象
*
*  @return MBUFFER_EC_OK：执行成功； MBUFFER_EC_FAIL：执行失败
*/
extern int MBUFFERESThrowLastFrame(int bIfRecede, MBUFFERESBuffer *pESBuffer);

/**
*  @method: MBUFFERESNumFrames
*  @abstract: 获取ESBuffer中当前帧的数量。
*
*  @param (in) pESBuffer 指向ESBUffer对象
*
*  @return 当前帧的个数
*/
extern int MBUFFERESNumFrames(MBUFFERESBuffer *pESBuffer);
extern unsigned char *MBUFFERESPosForData(int iLength, MBUFFERESBuffer *pESBuffer);

/**
*  @method: MBUFFERESGetFrame
*  @abstract: 获取ESBuffer中第一帧的数据。
*
*  @param (out) pucFramePos 保存帧数据的首地址
*  @param (out) piLength 保存数据的长度
*  @param (in) pESBuffer 指向ESBuffer对象
*
*  @return MBUFFER_EC_OK: 表示该帧是一个普通帧
*  TODO: other error code
*/
extern int MBUFFERESGetFrame(unsigned char** pucFramePos, int *piLength, MBUFFERESBuffer *pESBuffer);

/**
*  @method: MBUFFERESReadFromFile
*  @abstract: 从文件中读取指定长度的数据到ESBuffer中
*
*  @param (in) fp_input 指向打开的文件
*  @param (in) iLength 指定读取数据的长度
*  @param (in) pESBuffer 指向ESBuffer对象
*
*  @return MBUFFER_EC_OK：读取成功； MBUFFER_EC_FAIL：读取失败（ESBuffer可用空间不够或者已满）
*/
extern int MBUFFERESReadFromFile(FILE *fp_input, int iLength, MBUFFERESBuffer *pESBuffer);

/**
*  @method: MBUFFERESWriteFirstFrameToFile
*  @abstract: 将ESBuffer中第一帧数据写入到文件中
*
*  @param (in) fp_output 指向打开的文件
*  @param (in) pESBuffer 指向ESBuffer对象
*
*  @return MBUFFER_EC_OK
*/
extern int MBUFFERESWriteFirstFrameToFile(FILE *fp_output, MBUFFERESBuffer *pESBuffer);

/**
 *  @method         MBUFFERESAddExtraInfo
 *  @abstract       Generates a frame data of extra information and appends to the ESBuffer.
 *
 *  @param (in)     pucExtraInfo        pointer to extra information
 *  @param (in)     iExtraInfoLength    length of extrainformation
 *  @param (in-out) pESBuffer           pointer to the ESBuffer instance
 *
 *  @return         MBUFFER_EC_OK
*/
extern int MBUFFERESAddExtraInfo(unsigned char *pucExtraInfo, int iExtraInfoLength, MBUFFERESBuffer *pESBuffer);

extern int MBUFFERByteArrayInit(unsigned char *pucBuffer, int iHead, int iTail, int iSize, MBUFFERByteArray *pByteArray);
extern int MBUFFERByteArrayLength(MBUFFERByteArray *pByteArray);
extern int MBUFFERByteArrayCurPos(MBUFFERByteArray *pByteArray);
extern int MBUFFERByteArrayCopy(unsigned char *pucDest, int iLength, MBUFFERByteArray *pByteArray);
extern int MBUFFERByteArrayAdvance(int iLength, MBUFFERByteArray *pByteArray);
extern int MBUFFERByteArraySeek(int iOffset, MBUFFERByteArray *pByteArray);
extern int MBUFFERByteArrayMove(int iDestPos, int iSrcPos, MBUFFERByteArray *pByteArray);
extern int MBUFFERByteArrayMove2Head(MBUFFERByteArray *pByteArray);
extern int MBUFFERByteArrayReadFromFile(FILE *fp_input, int iLength, MBUFFERByteArray *pByteArray);
extern int MBUFFERByteArrayDeInit(int bIfFreeBuffer, MBUFFERByteArray *pByteArray);

/**
 *  @method         MBUFFERESSideInfoSetIfKeyFrame
 *  @abstract       Sets the bIfKeyFrame to iFlags of pSideInfo.
 *
 *  @param (in)     bIfKeyFrame
 *  @param (in-out) pSideInfo
 *
 *  @return         MBUFFER_EC_OK
*/
extern int MBUFFERESSideInfoSetIfKeyFrame(int bIfKeyFrame, MBUFFERESSideInfo *pSideInfo);

/**
 *  @method         MBUFFERESSideInfoGetIfKeyFrame
 *  @abstract       Extracts the bIfKeyFrame from iFlags of pSideInfo.
 *
 *  @param (in)     pSideInfo   the pointer of SideInfo
 *
 *  @return         1 if the pSideInfo is a key frame, 0 if the other side.
*/
extern int MBUFFERESSideInfoGetIfKeyFrame(MBUFFERESSideInfo *pSideInfo);

/**
 *  @method         MBUFFERESSideInfoSetFrameType
 *  @abstract       Sets the frame type to iFlags of pSideInfo.
 *
 *  @param (in)     iFramType   frame type, see MBUFFER_ES_FRAME_TYPE_XXXX
 *  @param (in-out) pSideInfo   the pointer of SideInfo
 *
 *  @return         MBUFFER_EC_OK
*/
extern int MBUFFERESSideInfoSetFrameType(int iFramType, MBUFFERESSideInfo *pSideInfo);

/**
 *  @method         MBUFFERESSideInfoGetFrameType
 *  @abstract       Extracts the frame type from iFlags of pSideInfo.
 *
 *  @param (in)     pSideInfo   the pointer of SideInfo
 *
 *  @return         see MBUFFER_ES_FRAME_TYPE_XXXX
*/
extern int MBUFFERESSideInfoGetFrameType(MBUFFERESSideInfo *pSideInfo);
#if defined(__cplusplus)
}
#endif
#endif