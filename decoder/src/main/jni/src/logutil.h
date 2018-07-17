/**
 * Create By zhurongkun
 * @author zhurongkun
 * @time 2017/11/9 15:38
 * @project HappyTime ${PACKAGE_NAME} 
 * @description 
 * @version 2017/11/9 15:38 1.0
 * @updateVersion 1.0
 * @updateTime 2017/11/9 15:38
 *
 */

//
// Created by zhurongkun on 2017/11/9.
//
#ifndef HAPPYTIME_LOGUTIL_H
#define HAPPYTIME_LOGUTIL_H
#ifdef __cplusplus
extern "C" {
#endif
#include <android/log.h>
#define TAG "FFMPEG"
#define DEBUGGABLE 1

#if DEBUGGABLE
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,TAG ,__VA_ARGS__) // 定义LOGD类型
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG ,__VA_ARGS__) // 定义LOGI类型
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,TAG ,__VA_ARGS__) // 定义LOGW类型
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG ,__VA_ARGS__) // 定义LOGE类型
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL,TAG ,__VA_ARGS__) // 定义LOGF类型
#else
#define LOGD(...)
#define LOGI(...)
#define LOGW(...)
#define LOGE(...)
#define LOGF(...)
#endif


#ifdef __cplusplus
}
#endif
#endif //HAPPYTIME_LOGUTIL_H
