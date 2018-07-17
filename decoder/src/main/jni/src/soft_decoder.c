/**
 * Create By zhurongkun
 * @author zhurongkun
 * @time 2018/4/18 15:49
 * @project HappyTime ${PACKAGE_NAME} 
 * @description 
 * @version 2018/4/18 15:49 1.0
 * @updateVersion 1.0
 * @updateTime 2018/4/18 15:49
 *
 */

//
// Created by zhurongkun on 2018/4/18.
//
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <frame.h>
#include "soft_decoder.h"
#include "decoder.h"
#include "logutil.h"

static JavaVM *sJvm;

void callError(JNIEnv *env, jobject cb, jmethodID method, int ret, char error[128]);

void callMethod(JNIEnv *env, jobject cb, jmethodID method);

void callOnFrame(JNIEnv *env, jobject cb, jmethodID method, AVFrame *pFrame);

void callOnSourceFrame(JNIEnv *env, jobject cb, jmethodID method, AVPacket *pPacket, int w, int h);

void checkRecordState(JNIEnv *env, Decoder *pDecoder, AVPacket *pPacket);

void startRecord(JNIEnv *env, Decoder *pDecoder, AVPacket *pPacket);

void recordFrame(JNIEnv *env, Decoder *pDecoder, AVPacket *pPacket);

void stopRecord(JNIEnv *env, Decoder *pDecoder, AVPacket *pPacket);

void callOnRecordError(JNIEnv *env, jobject listener, int code, char *error);

void *decodeFunc(void *param) {
    JNIEnv *env = NULL;
    (*sJvm)->AttachCurrentThread(sJvm, &env, NULL);
    Decoder *pDecoder = param;

    if (!pDecoder) {
        LOGE("decoder not exists");
        return NULL;
    }

    if (!pDecoder->decodeListener) {
        LOGE("no decodeListener");
        return NULL;
    }

    jobject cb = pDecoder->decodeListener;

    jclass clazz = (*env)->GetObjectClass(env, cb);
    if (!clazz) {
        LOGE("decodeListener class not found ");
        return NULL;
    }

    jmethodID onSourceFrame = (*env)->GetMethodID(env, clazz, "onSourceFrame", "([BZII)V");
    jmethodID onFrame = (*env)->GetMethodID(env, clazz, "onFrame", "([BII)V");
    jmethodID onError = (*env)->GetMethodID(env, clazz, "onError", "(ILjava/lang/String;)V");
    jmethodID onStart = (*env)->GetMethodID(env, clazz, "onStart", "()V");
    jmethodID onEnd = (*env)->GetMethodID(env, clazz, "onEnd", "()V");

    assert(onSourceFrame != NULL);
    assert(onFrame != NULL);
    assert(onError != NULL);
    assert(onStart != NULL);
    assert(onEnd != NULL);

    callMethod(env, cb, onStart);

    char error[128];
    int ret = prepareDecoder(pDecoder, error);
    int decodeSuccess = 0;

    if (ret < 0) {
        callError(env, cb, onError, ret, error);
        callMethod(env, cb, onEnd);
    } else {


        AVFrame *avFrame = av_frame_alloc();
        AVPacket *avPacket = av_malloc(sizeof(AVPacket));

        while (pDecoder->runnable) {
            if ((ret = av_read_frame(pDecoder->avFormatContext, avPacket)) < 0) {
                LOGE("read frame end or error");
                break;
            }



            if (avPacket->stream_index == pDecoder->videoStreamId) {
                //video frame
                if (pDecoder->needSourceFrameNotify) {
                    callOnSourceFrame(env, cb, onSourceFrame, avPacket,
                                      pDecoder->codecContext->width,
                                      pDecoder->codecContext->height);
                }
                if (avcodec_decode_video2(pDecoder->codecContext, avFrame, &decodeSuccess,
                                          avPacket) > 0) {
                    if (decodeSuccess) {
                        callOnFrame(env, cb, onFrame, avFrame);
                    }
                }

                checkRecordState(env, pDecoder, avPacket);
            }

        }
        LOGI("exit loop");
        callMethod(env, cb, onEnd);
        av_frame_free(&avFrame);
        av_packet_unref(avPacket);
        av_free(avPacket);
    }

    BAIL:
    LOGI("close decoder");
    (*env)->DeleteLocalRef(env, clazz);
    (*env)->DeleteGlobalRef(env, cb);
    closeDecoder(pDecoder);
    ReleaseDecoder(&pDecoder);
    (*sJvm)->DetachCurrentThread(sJvm);
    return NULL;
}

void checkRecordState(JNIEnv *env, Decoder *pDecoder, AVPacket *pPacket) {
    if (pDecoder->recordState == RECORD_NONE) {
        LOGI("not record");
        return;
    }
    if (pDecoder->recordState == RECORD_BEGIN) {
        LOGI("begin record");
        pDecoder->recordState = RECORDING;
        startRecord(env, pDecoder, pPacket);
    }
    if (pDecoder->recordState == RECORDING) {
        recordFrame(env, pDecoder, pPacket);
    }
    if (pDecoder->recordState == RECORD_END) {
        LOGI("record end");
        stopRecord(env, pDecoder, pPacket);
    }
}

void stopRecord(JNIEnv *env, Decoder *pDecoder, AVPacket *pPacket) {
    if (pDecoder->avRecordFormatContext) {
        av_write_trailer(pDecoder->avRecordFormatContext);

        /* close output */
        if (!(pDecoder->avRecordFormatContext->flags & AVFMT_NOFILE)){
            int ret = avio_close(pDecoder->avRecordFormatContext->pb);
            LOGI("avio_close %s",av_err2str(ret));
        }

        avformat_free_context(pDecoder->avRecordFormatContext);

        jclass clz = (*env)->GetObjectClass(env, pDecoder->recordListener);
        jmethodID onRecordEnd = (*env)->GetMethodID(env, clz, "onRecordEnd",
                                                    "()V");
        assert(onRecordEnd != NULL);
        (*env)->CallVoidMethod(env, pDecoder->recordListener, onRecordEnd);

        (*env)->DeleteGlobalRef(env, pDecoder->recordListener);
        pDecoder->recordListener = NULL;
    }
    pDecoder->recordState = RECORD_NONE;
}

void recordFrame(JNIEnv *env, Decoder *pDecoder, AVPacket *pPacket) {
    int ret = -1;
    AVStream *in_stream;
    AVStream *out_stream;
    AVPacket *pkt = pPacket;

    //record video
    in_stream = pDecoder->avFormatContext->streams[pDecoder->videoStreamId];
    out_stream = pDecoder->avRecordFormatContext->streams[pDecoder->videoStreamId];


    if (pkt->pts == AV_NOPTS_VALUE) {
        LOGE("recording frame AV_NOPTS_VALUE");
        //Write PTS
        AVRational time_base1 = in_stream->time_base;
        //Duration between 2 frames (us)
        int64_t calc_duration = (int64_t) (AV_TIME_BASE / av_q2d(in_stream->r_frame_rate));
        //Parameters
        pkt->pts = (int64_t) ((double) (pDecoder->recordFrameCount * calc_duration) /
                              (double) (av_q2d(time_base1) * AV_TIME_BASE));
        pkt->dts = pkt->pts;
        pkt->duration = (int64_t) ((double) calc_duration /
                                   (double) (av_q2d(time_base1) * AV_TIME_BASE));
    }

    pkt->pts = av_rescale_q_rnd(pkt->pts, in_stream->time_base, out_stream->time_base,
                                AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
    pkt->dts = av_rescale_q_rnd(pkt->dts, in_stream->time_base, out_stream->time_base,
                                AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
    pkt->duration = av_rescale_q(pkt->duration, in_stream->time_base, out_stream->time_base);
    pkt->pos = -1;

//    av_packet_rescale_ts(&pkt, in_stream->time_base, out_stream->time_base);
    pkt->stream_index = pDecoder->videoStreamId;


    ret = av_interleaved_write_frame(pDecoder->avRecordFormatContext, pkt);
    if (ret < 0) {
        LOGE("av_interleaved_write_frame failed %d", ret);
        callOnRecordError(env, pDecoder->recordListener, ret, "Can't write frame");
    }
    pDecoder->recordFrameCount++;

}

void startRecord(JNIEnv *env, Decoder *pDecoder, AVPacket *pPacket) {
    int ret = 0;
    ret = avformat_alloc_output_context2(&pDecoder->avRecordFormatContext, NULL, NULL,
                                         pDecoder->recPath);
    if (ret < 0) {
        LOGE("can't open out file!");
        pDecoder->recordState = RECORD_NONE;
        return;
    }

    AVStream *inStream;
    AVStream *outStream;

    inStream = pDecoder->avFormatContext->streams[pDecoder->videoStreamId];
    outStream = avformat_new_stream(pDecoder->avRecordFormatContext, inStream->codec->codec);

    if (outStream == NULL) {
        LOGE("can't open output stream!");
        pDecoder->recordState = RECORD_NONE;
        callOnRecordError(env, pDecoder->recordListener, -1, "Inner error");
        return;
    }

    ret = avcodec_copy_context(outStream->codec, inStream->codec);
    if (ret < 0) {
        LOGE("record avcodec_copy_context video failed");
        pDecoder->recordState = RECORD_NONE;
        callOnRecordError(env, pDecoder->recordListener, ret, "Inner error");
        return;
    }
    outStream->codec->codec_tag = 0;
    if (pDecoder->avRecordFormatContext->oformat->flags & AVFMT_GLOBALHEADER) {
        outStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }


    if (!(pDecoder->avRecordFormatContext->flags & AVFMT_NOFILE)) {
        ret = avio_open(&pDecoder->avRecordFormatContext->pb, pDecoder->recPath, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("record avio_open failed : %s", av_err2str(ret));
            pDecoder->recordState = RECORD_NONE;
            char err[64] = {0};
            sprintf(err, "Open file failed,%s", av_err2str(ret));
            callOnRecordError(env, pDecoder->recordListener, ret, err);
            return;
        }
    }

    ret = avformat_write_header(pDecoder->avRecordFormatContext, NULL);
    if (ret < 0) {
        LOGE("record avformat_write_header failed");
        pDecoder->recordState = RECORD_NONE;
        char err[64] = {0};
        sprintf(err, "Write file header failed,%s", av_err2str(ret));
        callOnRecordError(env, pDecoder->recordListener, ret, err);
        return;
    }
    pDecoder->recordFrameCount = 0;

    jclass clz = (*env)->GetObjectClass(env, pDecoder->recordListener);
    jmethodID onRecordBegin = (*env)->GetMethodID(env, clz, "onRecordBegin",
                                                  "()V");
    assert(onRecordBegin != NULL);
    (*env)->CallVoidMethod(env, pDecoder->recordListener, onRecordBegin);
}

void callOnRecordError(JNIEnv *env, jobject listener, int code, char *error) {
    if (listener != NULL) {
        jclass clz = (*env)->GetObjectClass(env, listener);
        jmethodID onError = (*env)->GetMethodID(env, clz, "onRecordError",
                                                "(ILjava/lang/String;)V");
        assert(onError != NULL);
        jstring errStr = (*env)->NewStringUTF(env, error);
        (*env)->CallVoidMethod(env, listener, onError, code, errStr);
    }
}

void callOnSourceFrame(JNIEnv *env, jobject cb, jmethodID method, AVPacket *pPacket, int w, int h) {
    bool isKey = (pPacket->flags == AV_PKT_FLAG_KEY);
    jbyteArray bytes = (*env)->NewByteArray(env, pPacket->size);
    (*env)->SetByteArrayRegion(env, bytes, 0, pPacket->size, (const jbyte *) pPacket->data);
    (*env)->CallVoidMethod(env, cb, method, bytes, isKey, w, h);
    (*env)->DeleteLocalRef(env, bytes);
}

void callOnFrame(JNIEnv *env, jobject cb, jmethodID method, AVFrame *pFrame) {
    int w = pFrame->width;
    int h = pFrame->height;
    int size = w * h * 3 / 2;


    jbyteArray bytes = (*env)->NewByteArray(env, size);
    (*env)->SetByteArrayRegion(env, bytes, 0, w * h, (const jbyte *) pFrame->data[0]);
    (*env)->SetByteArrayRegion(env, bytes, w * h, w * h / 4, (const jbyte *) pFrame->data[1]);
    (*env)->SetByteArrayRegion(env, bytes, w * h + w * h / 4, w * h / 4,
                               (const jbyte *) pFrame->data[2]);

    (*env)->CallVoidMethod(env, cb, method, bytes, w, h);
    (*env)->DeleteLocalRef(env, bytes);
}

void callMethod(JNIEnv *env, jobject cb, jmethodID method) {
    (*env)->CallVoidMethod(env, cb, method);
}

void callError(JNIEnv *env, jobject cb, jmethodID method, int ret, char error[128]) {
    jstring errString = (*env)->NewStringUTF(env, error);
    (*env)->CallVoidMethod(env, cb, method, ret, errString);
    (*env)->DeleteLocalRef(env, errString);
}

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    jint version = JNI_VERSION_1_6;
    JNIEnv *env = NULL;
    if ((*vm)->GetEnv(vm, (void **) &env, version) < 0) {
        LOGE("jni onload get version JNI_VERSION_1_6 failed!");
        return -1;
    }

    sJvm = vm;

    return version;
}

JNIEXPORT jlong JNICALL
Java_com_zrk_decoder_SoftDecoder_start(JNIEnv *env, jobject instance, jstring uri_,
                                       jobject listener) {
    const char *uri = (*env)->GetStringUTFChars(env, uri_, 0);
    size_t len = strlen(uri);
    if (len > 256) {
        LOGE("uri length is too long !");
        return -1;
    }

    Decoder *pDecoder = NewDecoder();
    strcpy(pDecoder->uri, uri);

    pDecoder->decodeListener = (*env)->NewGlobalRef(env, listener);
    if (0 > pthread_create(&pDecoder->pDecThread, NULL, decodeFunc, pDecoder)) {
        LOGE("create thread failed");
        return -1;
    }
    (*env)->ReleaseStringUTFChars(env, uri_, uri);
    return (jlong) pDecoder;
}


JNIEXPORT void JNICALL
Java_com_zrk_decoder_SoftDecoder_stop(JNIEnv *env, jobject instance, jlong session) {
    if (session != 0 && session != -1) {
        Decoder *pDecoder = (Decoder *) session;
        if (pDecoder) {
            pDecoder->runnable = false;
        }
    }
}

JNIEXPORT void JNICALL
Java_com_zrk_decoder_SoftDecoder_setNeedSourceFrame(JNIEnv *env, jobject instance, jlong session,
                                                    jboolean need) {
    if (session > 0) {
        Decoder *pDecoder = (Decoder *) session;
        pDecoder->needSourceFrameNotify = need;
    }
}

JNIEXPORT void JNICALL
Java_com_zrk_decoder_SoftDecoder_startRecord(JNIEnv *env, jobject instance, jlong session,
                                             jstring path_, jobject listener) {
    const char *path = (*env)->GetStringUTFChars(env, path_, 0);

    size_t len = strlen(path);
    len = len > 256 ? 256 : len;

    if (session > 0) {
        Decoder *pDecoder = (Decoder *) session;
        if (pDecoder->recordState == RECORD_NONE) {
            strncpy(pDecoder->recPath, path, len);
            pDecoder->recordListener = (*env)->NewGlobalRef(env, listener);
            pDecoder->recordState = RECORD_BEGIN;
            LOGI("startRecord");
        }
    }

    (*env)->ReleaseStringUTFChars(env, path_, path);
}

JNIEXPORT void JNICALL
Java_com_zrk_decoder_SoftDecoder_stopRecord(JNIEnv *env, jobject instance, jlong session) {
    if (session > 0) {
        Decoder *pDecoder = (Decoder *) session;
        if (pDecoder->recordState == RECORDING) {
            pDecoder->recordState = RECORD_END;
        }
    }
}