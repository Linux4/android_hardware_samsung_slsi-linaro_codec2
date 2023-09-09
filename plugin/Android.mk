LOCAL_PATH := $(call my-dir)
#############################
####  libc2filterplugin  ####
#############################
include $(CLEAR_VARS)

LOCAL_CFLAGS :=
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        Exynos_C2_FilterPlugin.cpp

LOCAL_C_INCLUDES :=

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libc2filterplugin
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE

LOCAL_PROPRIETARY_MODULE := true

LOCAL_HEADER_LIBRARIES := \
        libcodec2_hidl_plugin_headers \
        libgui_headers \
        libexynosc2_base_headers \
        libexynosc2_osal_headers \
LOCAL_HEADER_LIBRARIES += $(EXYNOS_VENDOR_HEADER_LIBS)

LOCAL_STATIC_LIBRARIES := librenderfright libExynosC2OSAL

LOCAL_SHARED_LIBRARIES := \
        libbase \
        liblog \
        libcodec2 \
        libcodec2_vndk \
        libutils \
        libcutils \
        libion \
        libEGL \
        libGLESv1_CM \
        libGLESv2 \
        libGLESv3 \
        libprocessgroup \
        libsfplugin_ccodec_utils \
        libsync \
        libui

LOCAL_SHARED_LIBRARIES += $(EXYNOS_VENDOR_SHARED_LIBS)

LOCAL_MIN_SDK_VERSION := 29

LOCAL_CFLAGS +=	-fPIC \
                -O3 \
                -Werror \
                -Wall \
                -Wno-deprecated-enum-enum-conversion \
                -std=gnu++1z \
                -std=c++2a
LOCAL_CFLAGS += $(EXYNOS_GLOBAL_CFLAGS)

include $(BUILD_SHARED_LIBRARY)

