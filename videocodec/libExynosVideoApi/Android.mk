LOCAL_PATH := $(call my-dir)

#############################
####  libExynosVideoApi2  ###
#############################
include $(CLEAR_VARS)

LOCAL_CFLAGS :=
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        ExynosVideoInterface.c \
        common/ExynosVideoCommonApi.c \
        osal/ExynosVideo_OSAL.c \
        dec/ExynosVideoDecoder.c \
        enc/ExynosVideoEncoder.c

ifeq ($(TARGET_SOC_NAME), google)
LOCAL_C_INCLUDES := $(TARGET_BOARD_KERNEL_HEADERS)/linux
endif

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosVideoApi2
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE

LOCAL_PROPRIETARY_MODULE := true

LOCAL_C_INCLUDES += $(LOCAL_PATH)/mfc_headers

# since 4.19 kernel
ifeq ($(filter 4.4 4.9, $(TARGET_LINUX_KERNEL_VERSION)),)
LOCAL_CFLAGS += -DMAINLINE_FEATURE_IN_SINCE_4_19
endif

ifeq ($(BOARD_USE_HEVC_HWIP), true)
LOCAL_CFLAGS += -DUSE_HEVC_HWIP
endif

ifeq ($(BOARD_USE_SINGLE_PLANE_IN_DRM), true)
LOCAL_CFLAGS += -DUSE_SINGLE_PALNE_SUPPORT
endif

ifneq ($(BOARD_USE_FRAMERATE_THRESH_HOLD),)
LOCAL_CFLAGS += -DFRAMERATE_THRESH_HOLD=$(BOARD_USE_FRAMERATE_THRESH_HOLD)
endif

ifeq ($(BOARD_SUPPORT_GPU_SBWC), true)
LOCAL_CFLAGS += -DUSE_SUPPORT_GPU_SBWC
endif

LOCAL_HEADER_LIBRARIES := libexynos_videoapi_headers
LOCAL_HEADER_LIBRARIES += $(EXYNOS_VENDOR_HEADER_LIBS)

LOCAL_SHARED_LIBRARIES := liblog libcutils libexynosv4l2
LOCAL_SHARED_LIBRARIES += $(EXYNOS_VENDOR_SHARED_LIBS)

LOCAL_CFLAGS +=	-fPIC \
                -O3 \
                -Werror \
                -Wall \
                -Wno-unused-variable \
                -Wno-unused-label \
                -Wno-unused-function \
                -DUSE_ANDROID \
                -DUSE_USER_POLL_EVENT \
                -DUSE_EPOLL

include $(BUILD_STATIC_LIBRARY)
