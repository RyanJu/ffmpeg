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
#include <malloc.h>
#include <string.h>
#include <dict.h>
#include <avutil.h>
#include <avcodec.h>
#include <frame.h>
#include <imgutils.h>
#include <swscale.h>
#include "decoder.h"
#include "logutil.h"

Decoder *NewDecoder() {
    Decoder *decoder = malloc(sizeof(Decoder));
    memset(decoder, 0, sizeof(Decoder));
    decoder->runnable = true;
    decoder->videoStreamId = -1;
    decoder->recordState = RECORD_NONE;
    return decoder;
}

void ReleaseDecoder(Decoder **pDecoder) {
    if (*pDecoder) {
        (*pDecoder)->recordState = RECORD_NONE;
        (*pDecoder)->runnable = false;
        free(*pDecoder);
    }
    (*pDecoder) = NULL;
}

int prepareDecoder(Decoder *decoder, char *msg) {
    av_register_all();
    int ret = 0;
    if ((ret = avformat_network_init()) < 0) {
        strcpy(msg, "Network disallowed!");
        avformat_network_deinit();
        return ret;
    }

    AVDictionary *dict = NULL;
    av_dict_set(&dict, "rtsp_transport", "tcp", 0);

    if ((ret = avformat_open_input(&decoder->avFormatContext, decoder->uri, NULL,
                                   &dict)) < 0) {
        avformat_close_input(&decoder->avFormatContext);
        strcpy(msg, "Can't open this stream!");
        return ret;
    }
    LOGD("avformat_open_input %s", decoder->uri);

    decoder->avFormatContext->max_analyze_duration = 5000;
    decoder->avFormatContext->probesize = 1024 * 10;

    if ((ret = avformat_find_stream_info(decoder->avFormatContext, &dict)) < 0) {
        strcpy(msg, "Can't find stream!");
        return ret;
    }

    for (int i = 0; i < decoder->avFormatContext->nb_streams; ++i) {
        if (decoder->avFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            decoder->videoStreamId = i;
            LOGD("find stream video %d", i);
        } else if (decoder->avFormatContext->streams[i]->codec->codec_type
                   == AVMEDIA_TYPE_AUDIO) {
            LOGD("find stream audio %d", i);
        } else {
            LOGD("find stream type %d , %s", i,
                 av_get_media_type_string(decoder->avFormatContext->streams[i]->codec->codec_type));
        }
    }

    if (decoder->videoStreamId < 0) {
        strcpy(msg, "No video stream!");
        return -1;
    }

    AVCodecContext *codecContext = decoder->avFormatContext->streams[decoder->videoStreamId]->codec;
    AVCodec *avCodec = avcodec_find_decoder(codecContext->codec_id);

    if (!avCodec) {
        strcpy(msg, "Inner error!");
        ret = -1;
        return ret;
    }

    LOGI("find video decodec %s", avcodec_get_name(codecContext->codec_id));

    if ((ret = avcodec_open2(codecContext, avCodec, 0) < 0)) {
        strcpy(msg, "Inner error2!");
        avcodec_close(codecContext);
        return -1;
    }

    decoder->codecContext = codecContext;
    decoder->codec = avCodec;

    return 0;

}

int closeDecoder(Decoder *decoder) {
    if (decoder) {
        avcodec_close(decoder->codecContext);
        avformat_close_input(&decoder->avFormatContext);
        avformat_network_deinit();
    }
    return 0;
}
