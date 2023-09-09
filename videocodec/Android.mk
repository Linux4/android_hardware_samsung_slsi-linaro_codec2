LOCAL_PATH := $(call my-dir)

#############################
####  libExynosVideoCodec  ###
#############################
include $(CLEAR_VARS)

LOCAL_CFLAGS :=
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        ExynosVideoCodec.cpp \
        ExynosVideoCodecBase.cpp \
        ExynosVideoCodecCommon.cpp \
        ExynosVideoCodecDec.cpp \
        ExynosVideoCodecEnc.cpp

LOCAL_C_INCLUDES :=

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosVideoCodec
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE

LOCAL_PROPRIETARY_MODULE := true

LOCAL_HEADER_LIBRARIES := libexynosc2_videocodec_headers libexynosc2_codecfilter_headers
LOCAL_HEADER_LIBRARIES += $(EXYNOS_VENDOR_HEADER_LIBS)

LOCAL_STATIC_LIBRARIES := libExynosC2OSAL libExynosVideoApi2

LOCAL_SHARED_LIBRARIES := libcutils libhardware libhidlbase libexynosv4l2
LOCAL_SHARED_LIBRARIES += $(EXYNOS_VENDOR_SHARED_LIBS)

LOCAL_CFLAGS +=	-fPIC \
                -O3 \
                -Werror \
                -Wall \
				-std=gnu++1z \
                -std=c++2a
LOCAL_CFLAGS += $(EXYNOS_GLOBAL_CFLAGS)

ifeq ($(BOARD_USE_PERFORMANCE), true)
LOCAL_CFLAGS += -DUSE_PERFORMANCE
endif

ifeq ($(BOARD_USE_DEC_SW_CSC), true)
LOCAL_CFLAGS += -DUSE_DEC_SW_CSC
endif

include $(BUILD_STATIC_LIBRARY)

include $(LOCAL_PATH)/libExynosVideoApi/Android.mk
