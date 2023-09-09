LOCAL_PATH := $(call my-dir)

##########################
####  libExynosC2OSAL  ###
##########################
include $(CLEAR_VARS)

LOCAL_CFLAGS :=
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        ExynosBufferAllocator.cpp \
        ExynosETC.cpp \
        ExynosLog.cpp \
        ExynosIONUtils.cpp \
        ExynosBufferManager.cpp

ifeq ($(BOARD_USE_PERFORMANCE), true)
LOCAL_SRC_FILES += \
        ExynosPerformance.cpp
endif

LOCAL_C_INCLUDES :=

ifeq ($(BOARD_USE_EXYNOS_GRAPHIC_BUFFER), true)
LOCAL_SRC_FILES += wrapped/ExynosGraphicBuffer.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/wrapped
LOCAL_EXPORT_C_INCLUDE_DIRS += $(LOCAL_PATH)/wrapped
endif

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosC2OSAL
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE

LOCAL_PROPRIETARY_MODULE := true

LOCAL_HEADER_LIBRARIES := libexynosc2_base_headers libexynosc2_osal_headers
LOCAL_HEADER_LIBRARIES += $(EXYNOS_VENDOR_HEADER_LIBS)

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libutils \
        libcutils \
        libcodec2 \
        libcodec2_vndk \
        libhidlbase \
        libstagefright_foundation_defaults \
        libion

LOCAL_SHARED_LIBRARIES += $(EXYNOS_VENDOR_SHARED_LIBS)

LOCAL_CFLAGS +=	-fPIC \
                -O3 \
                -Werror \
                -Wall \
                -Wno-unused-variable \
                -Wno-deprecated-enum-enum-conversion \
                -std=gnu++1z \
                -std=c++2a
LOCAL_CFLAGS += $(EXYNOS_GLOBAL_CFLAGS)

include $(BUILD_STATIC_LIBRARY)
