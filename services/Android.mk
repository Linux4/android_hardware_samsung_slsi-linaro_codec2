LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_ARCH),$(filter $(TARGET_ARCH), arm arm64))
# vendor service base seccomp policy
include $(CLEAR_VARS)
LOCAL_MODULE := codec2.vendor.base.policy
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/etc/seccomp_policy
LOCAL_REQUIRED_MODULES := crash_dump.policy
LOCAL_SRC_FILES := seccomp_policy/codec2.vendor.base-arm.policy
include $(BUILD_PREBUILT)

# vendor service ext seccomp policy
include $(CLEAR_VARS)
LOCAL_MODULE := codec2.vendor.ext.policy
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/etc/seccomp_policy
LOCAL_SRC_FILES := seccomp_policy/codec2.vendor.ext-arm.policy
include $(BUILD_PREBUILT)
endif

include $(CLEAR_VARS)

HIDL_VERSION := 1.0
ifeq ($(BOARD_USE_CODEC2_HIDL_1_2), true)
HIDL_VERSION := 1.2
LOCAL_CFLAGS += -DUSE_CODEC2_HIDL_1_2
endif

LOCAL_SRC_FILES:= \
        vendor.cpp \

LOCAL_C_INCLUDES += \
        $(TOP)/external/gtest/include \
        $(TOP)/frameworks/av/media/libstagefright/include \
        $(TOP)/frameworks/native/include \
        $(TOP)/frameworks/av/mediacodec2/include \
        $(TOP)/frameworks/av/media/codec2/hidl/$(HIDL_VERSION)/utils/include \
        $(TOP)/frameworks/av/media/codec2/vndk/include \

LOCAL_MODULE := samsung.hardware.media.c2@$(HIDL_VERSION)-service
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE

LOCAL_MODULE_TAGS := optional

LOCAL_PROPRIETARY_MODULE := true

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_INIT_RC := samsung.hardware.media.c2@$(HIDL_VERSION)-service.rc

LOCAL_HEADER_LIBRARIES := libexynosc2_base_headers libexynosc2_osal_headers media_plugin_headers
LOCAL_HEADER_LIBRARIES += $(EXYNOS_VENDOR_HEADER_LIBS)

LOCAL_SHARED_LIBRARIES := \
						android.hardware.media.c2@$(HIDL_VERSION) \
						android.hardware.media.omx@1.0 \
						libavservices_minijail \
						libbinder \
						libcodec2_hidl@$(HIDL_VERSION) \
						libcodec2_vndk \
						libhidlbase \
						libhidltransport \
						libhwbinder \
						liblog \
						libstagefright_omx \
						libstagefright_xmlparser \
						libutils \
						libExynosC2ComponentStore \

# -Wno-unused-parameter is needed for libchrome/base codes
LOCAL_CFLAGS += -Werror -Wall -Wno-unused-parameter
#LOCAL_LDFLAGS += -Wl,-rpath=/vendor/lib
LOCAL_CLANG := true

include $(BUILD_EXECUTABLE)

include $(call all-makefiles-under, $(LOCAL_PATH))
