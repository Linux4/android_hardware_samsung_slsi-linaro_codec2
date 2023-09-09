LOCAL_PATH := $(call my-dir)

#############################
####     libExynosGDC    ####
#############################
include $(CLEAR_VARS)

LOCAL_CFLAGS :=
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        ExynosGDCWrapper.cpp

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosGDCWrapper

LOCAL_PROPRIETARY_MODULE := true

LOCAL_HEADER_LIBRARIES := libexynosc2_gdc_headers
LOCAL_HEADER_LIBRARIES += $(EXYNOS_VENDOR_HEADER_LIBS)

LOCAL_STATIC_LIBRARIES := libExynosC2OSAL

LOCAL_SHARED_LIBRARIES := libcutils libutils libhardware libexynosgdc

LOCAL_CFLAGS +=	-fPIC \
                -O3 \
                -Werror \
                -Wall \
				-std=gnu++1z \
				-std=c++2a
LOCAL_CFLAGS += $(EXYNOS_GLOBAL_CFLAGS)

include $(BUILD_STATIC_LIBRARY)
