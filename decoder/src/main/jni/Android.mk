LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := avcodec
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_SRC_FILES := lib/libavcodec-57.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := avfilter
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_SRC_FILES := lib/libavfilter-6.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := avformat
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_SRC_FILES := lib/libavformat-57.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE :=  avutil
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_SRC_FILES := lib/libavutil-55.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE :=  avswresample
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_SRC_FILES := lib/libswresample-2.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE :=  swscale
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_SRC_FILES := lib/libswscale-4.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -std=c99

LOCAL_MODULE := zdecoder


LOCAL_SHARED_LIBRARIES:= avcodec \
                         avfilter \
                         avformat \
                         avutil \
                         avswresample \
                         swscale \

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include\
                    $(LOCAL_PATH)/include/libavcodec\
                    $(LOCAL_PATH)/include/libavfilter\
                    $(LOCAL_PATH)/include/libavformat\
                    $(LOCAL_PATH)/include/libavutil\
                    $(LOCAL_PATH)/include/libswresample\
                    $(LOCAL_PATH)/include/libswscale\
                    $(LOCAL_PATH)/src\

LOCAL_LDLIBS += -llog -lz -pthread
LOCAL_SRC_FILES :=  src/decoder.c \
                    src/soft_decoder.c \

include $(BUILD_SHARED_LIBRARY)