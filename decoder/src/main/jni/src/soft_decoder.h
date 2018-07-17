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
#ifndef HAPPYTIME_SOFT_DECODER_H
#define HAPPYTIME_SOFT_DECODER_H

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jlong JNICALL
Java_com_zrk_decoder_SoftDecoder_start(JNIEnv *env, jobject instance, jstring uri_,
                                       jobject listener);

JNIEXPORT void JNICALL
Java_com_zrk_decoder_SoftDecoder_stop(JNIEnv *env, jobject instance, jlong session);


JNIEXPORT void JNICALL
Java_com_zrk_decoder_SoftDecoder_setNeedSourceFrame(JNIEnv *env, jobject instance, jlong session,
                                                    jboolean need);

JNIEXPORT void JNICALL
Java_com_zrk_decoder_SoftDecoder_startRecord(JNIEnv *env, jobject instance, jlong session,
                                             jstring path_, jobject listener);

JNIEXPORT void JNICALL
Java_com_zrk_decoder_SoftDecoder_stopRecord(JNIEnv *env, jobject instance, jlong session);

#ifdef __cplusplus
}
#endif
#endif //HAPPYTIME_SOFT_DECODER_H
