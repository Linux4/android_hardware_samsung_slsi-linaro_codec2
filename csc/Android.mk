LOCAL_PATH := $(call my-dir)

#############################
####     libExynosCSC    ####
#############################
include $(CLEAR_VARS)

LOCAL_CFLAGS :=
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        ExynosCSC.cpp

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosCSC
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE

LOCAL_PROPRIETARY_MODULE := true

LOCAL_HEADER_LIBRARIES := libexynosc2_csc_headers
LOCAL_HEADER_LIBRARIES += $(EXYNOS_VENDOR_HEADER_LIBS)

LOCAL_STATIC_LIBRARIES := libExynosC2OSAL

LOCAL_SHARED_LIBRARIES := libcutils libutils libhardware libacryl
LOCAL_SHARED_LIBRARIES += $(EXYNOS_VENDOR_SHARED_LIBS)

LOCAL_CFLAGS +=	-fPIC \
                -O3 \
                -Werror \
                -Wall \
				-std=gnu++1z \
                -std=c++2a
LOCAL_CFLAGS += $(EXYNOS_GLOBAL_CFLAGS)

include $(BUILD_STATIC_LIBRARY)
