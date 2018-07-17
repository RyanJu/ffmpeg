/**
 * Create By zhurongkun
 * @author zhurongkun
 * @time 2018/4/18 15:51
 * @project HappyTime ${PACKAGE_NAME} 
 * @description 
 * @version 2018/4/18 15:51 1.0
 * @updateVersion 1.0
 * @updateTime 2018/4/18 15:51
 *
 */

//
// Created by zhurongkun on 2018/4/18.
//
#ifndef HAPPYTIME_DECODER_H
#define HAPPYTIME_DECODER_H

#include <stdbool.h>
#include <avformat.h>
#include <avcodec.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RECORD_NONE (0)
#define RECORD_BEGIN (1)
#define RECORDING (2)
#define RECORD_END (3)


typedef struct _DECODER {
    char uri[256];
    volatile bool runnable;
    volatile bool needSourceFrameNotify;
    int videoStreamId;
    pthread_t pDecThread;
    void *decodeListener;
    AVFormatContext *avFormatContext;
    AVCodecContext *codecContext;
    AVCodec *codec;

    volatile int recordState;
    AVFormatContext *avRecordFormatContext;
    char recPath[256];
    void *recordListener;
    int recordFrameCount;

} Decoder;


Decoder *NewDecoder();

int prepareDecoder(Decoder *decoder, char *errorMsg);

int closeDecoder(Decoder *decoder);

void ReleaseDecoder(Decoder **pDecoder);

#ifdef __cplusplus
}
#endif
#endif //HAPPYTIME_DECODER_H
