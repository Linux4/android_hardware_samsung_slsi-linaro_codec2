ifneq ($(filter exynos google, $(TARGET_SOC_NAME)),)
LOCAL_PATH := $(call my-dir)

EXYNOS_CODEC2_TOP := $(LOCAL_PATH)

ifeq ($(TARGET_SOC_NAME), exynos)
EXYNOS_VENDOR_HEADER_LIBS := libexynos_headers
EXYNOS_VENDOR_SHARED_LIBS := \
    libion_exynos \
    libui \
    libexynosgraphicbuffer

ifeq ($(BOARD_USE_PERFORMANCE), true)
EXYNOS_VENDOR_SHARED_LIBS += libepicoperator
endif
endif

ifeq ($(TARGET_SOC_NAME), google)
EXYNOS_VENDOR_HEADER_LIBS := google_hal_headers
EXYNOS_VENDOR_SHARED_LIBS := \
    libion_google \
    libui \
    libvendorgraphicbuffer
BOARD_USE_EXYNOS_GRAPHIC_BUFFER := true
endif

CODEC2_HIDL_VERSION := 1.0
ifeq ($(BOARD_USE_CODEC2_HIDL_1_2), true)
CODEC2_HIDL_VERSION := 1.2
endif

$(warning #### [CODEC2] HIDL VERSION = $(CODEC2_HIDL_VERSION))

ifneq ($(BOARD_SUPPORT_FLEXIBLE_P010), false)
EXYNOS_GLOBAL_CFLAGS += -DUSE_FLEXIBLE_P010
$(info #### [CODEC2] SUPPORTS FLEXIBLE FORMAT)
endif

ifeq ($(BOARD_USE_CSC_FILTER), true)
EXYNOS_DEC_CFLAGS += -DUSE_CSC_FILTER
EXYNOS_ENC_CFLAGS += -DUSE_CSC_FILTER
endif

ifeq ($(BOARD_USE_ENC_SW_CSC), true)
ifeq ($(BOARD_USE_CSC_FILTER), false)
EXYNOS_ENC_CFLAGS += -DUSE_CSC_FILTER
endif
EXYNOS_ENC_CFLAGS += -DUSE_SW_CSC
endif

ifeq ($(BOARD_USE_DEC_SW_CSC), true)
ifeq ($(BOARD_USE_CSC_FILTER), false)
EXYNOS_DEC_CFLAGS += -DUSE_CSC_FILTER
endif
EXYNOS_DEC_CFLAGS += -DUSE_SW_CSC
endif

ifeq ($(BOARD_SUPPORT_MFC_ENC_RGB), true)
EXYNOS_ENC_CFLAGS += -DSUPPORT_MFC_ENC_RGB
endif

ifeq ($(BOARD_USE_BLOB_ALLOCATOR), true)
EXYNOS_ENC_CFLAGS += -DUSE_BLOB_ALLOCATOR
endif

ifeq ($(BOARD_USE_QUERY_HDR2SDR), true)
EXYNOS_DEC_CFLAGS += -DUSE_QUERY_HDR2SDR
endif

ifeq ($(BOARD_USE_FULL_ST2094_40), true)
EXYNOS_GLOBAL_CFLAGS += -DUSE_FULL_ST2094_40
endif

ifeq ($(BOARD_USE_GDC), true)
EXYNOS_ENC_CFLAGS += -DUSE_GDC_FILTER
EXYNOS_VENDOR_ENC_HEADER_LIBS += libexynosc2_gdcfilter_headers
EXYNOS_VENDOR_ENC_STATIC_LIBS += libExynosGDCWrapper
EXYNOS_VENDOR_ENC_SHARED_LIBS += libexynosgdc
EXYNOS_VENDOR_ENC_SRC_FILES += filter/gdc/Exynos_GDC_Filter.cpp
endif

ifeq ($(BOARD_UNSUPPORT_10BIT), true)
EXYNOS_ENC_CFLAGS += -DUNSUPPORT_10BIT
EXYNOS_DEC_CFLAGS += -DUNSUPPORT_10BIT
endif

ifeq ($(BOARD_GPU_TYPE), sgpu)
EXYNOS_GLOBAL_CFLAGS += -DHW_GPU_ALIGN=256
EXYNOS_GLOBAL_CFLAGS += -DYV12_ALIGN=512
endif

ifeq ($(BOARD_SUPPORT_GPU_SBWC), true)
EXYNOS_GLOBAL_CFLAGS += -DUSE_SUPPORT_GPU_SBWC
endif

ifdef BOARD_MFC_CHROMA_VALIGN
LOCAL_CFLAGS += -DHW_CHROMA_VALIGN=$(BOARD_MFC_CHROMA_VALIGN)
endif

ifneq ($(BOARD_HW_SUPPORT_FILMGRAIN), true)
EXYNOS_DEC_CFLAGS += -DUSE_FILMGRAIN_FILTER
endif

ifeq ($(BOARD_USE_SMALL_SECURE_MEMORY), true)
EXYNOS_GLOBAL_CFLAGS += -DUSE_SMALL_SECURE_MEMORY
endif

ifneq ($(BOARD_USE_MAX_SECURE_RESOURCE),)
EXYNOS_GLOBAL_CFLAGS += -DMAX_SECURE_RESOURCE=$(BOARD_USE_MAX_SECURE_RESOURCE)
endif

####################################
####  libExynosC2ComponentStore  ###
####################################
include $(CLEAR_VARS)

LOCAL_CFLAGS :=
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        Exynos_C2_ComponentStore.cpp

LOCAL_C_INCLUDES :=

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosC2ComponentStore
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE

LOCAL_PROPRIETARY_MODULE := true

LOCAL_HEADER_LIBRARIES := libexynosc2_base_headers libexynosc2_osal_headers
LOCAL_HEADER_LIBRARIES += $(EXYNOS_VENDOR_HEADER_LIBS)

LOCAL_STATIC_LIBRARIES := libExynosC2OSAL

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libcutils \
        libion \
        libhidlbase \
        libcodec2 \
        libcodec2_vndk \
        libsfplugin_ccodec_utils \
        libstagefright_xmlparser \
        libion

LOCAL_SHARED_LIBRARIES += $(EXYNOS_VENDOR_SHARED_LIBS)

LOCAL_CFLAGS +=	-fPIC \
                -O3 \
                -Werror \
                -Wall \
                -Wno-deprecated-enum-enum-conversion \
                -std=gnu++1z \
                -std=c++2a
LOCAL_CFLAGS += $(EXYNOS_GLOBAL_CFLAGS)

include $(BUILD_SHARED_LIBRARY)


#############################
####  libExynosC2H264Dec  ###
#############################
include $(CLEAR_VARS)

LOCAL_CFLAGS :=
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        c2component/Exynos_C2_Component.cpp \
        c2component/Exynos_C2_DecComponent.cpp \
        c2component/h264/Exynos_C2_H264DecComponent.cpp \
        c2component/Exynos_C2_IntfSetter.cpp \
        c2component/Exynos_C2_FilterParamCnv.cpp \
        filter/Exynos_Filter.cpp \
        filter/codec/Exynos_CodecBase_Filter.cpp \
        filter/codec/Exynos_CodecDecBase_Filter.cpp \
        filter/codec/h264/Exynos_H264Dec_Filter.cpp \
        filter/csc/Exynos_CSC_Filter.cpp \

LOCAL_C_INCLUDES :=

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosC2H264Dec
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE

LOCAL_PROPRIETARY_MODULE := true

LOCAL_HEADER_LIBRARIES := libexynosc2_h264component_headers libstagefright_foundation_headers
LOCAL_HEADER_LIBRARIES += $(EXYNOS_VENDOR_HEADER_LIBS)

LOCAL_STATIC_LIBRARIES := libExynosCSC libExynosVideoCodec libExynosC2OSAL libExynosVideoApi2 libVendorVideoApi

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libutils \
        libcutils \
        libhidlbase \
        libcodec2 \
        libcodec2_vndk \
        libsfplugin_ccodec_utils \
        libstagefright_foundation \
        libstagefright_xmlparser \
        libExynosC2ComponentStore \
        libexynosv4l2 \
        libhardware \
        libacryl
LOCAL_SHARED_LIBRARIES += $(EXYNOS_VENDOR_SHARED_LIBS)

LOCAL_CFLAGS +=	-fPIC \
                -O3 \
                -Werror \
                -Wall \
                -Wno-deprecated-enum-enum-conversion \
                -std=gnu++1z \
                -std=c++2a
LOCAL_CFLAGS += $(EXYNOS_GLOBAL_CFLAGS)
LOCAL_CFLAGS += $(EXYNOS_DEC_CFLAGS)

include $(BUILD_SHARED_LIBRARY)


#############################
####  libExynosC2H264Enc  ###
#############################
include $(CLEAR_VARS)

LOCAL_CFLAGS :=
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        c2component/Exynos_C2_Component.cpp \
        c2component/Exynos_C2_EncComponent.cpp \
        c2component/h264/Exynos_C2_H264EncComponent.cpp \
        c2component/Exynos_C2_IntfSetter.cpp \
        c2component/Exynos_C2_FilterParamCnv.cpp \
        filter/Exynos_Filter.cpp \
        filter/codec/Exynos_CodecBase_Filter.cpp \
        filter/codec/Exynos_CodecEncBase_Filter.cpp \
        filter/codec/h264/Exynos_H264Enc_Filter.cpp \
        filter/codec/h264/Exynos_H264WFDEnc_Filter.cpp \
        filter/csc/Exynos_CSC_Filter.cpp
LOCAL_SRC_FILES += $(EXYNOS_VENDOR_ENC_SRC_FILES)

LOCAL_C_INCLUDES :=

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosC2H264Enc
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE

LOCAL_PROPRIETARY_MODULE := true

LOCAL_HEADER_LIBRARIES := libexynosc2_h264component_headers libstagefright_foundation_headers
LOCAL_HEADER_LIBRARIES += $(EXYNOS_VENDOR_HEADER_LIBS) $(EXYNOS_VENDOR_ENC_HEADER_LIBS)

LOCAL_STATIC_LIBRARIES := libExynosCSC libExynosVideoCodec libExynosC2OSAL libExynosVideoApi2 libVendorVideoApi
LOCAL_STATIC_LIBRARIES += $(EXYNOS_VENDOR_ENC_STATIC_LIBS)

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libutils \
        libcutils \
        libhidlbase \
        libcodec2 \
        libcodec2_vndk \
        libsfplugin_ccodec_utils \
        libstagefright_foundation \
        libstagefright_xmlparser \
        libExynosC2ComponentStore \
        libexynosv4l2 \
        libhardware \
        libacryl
LOCAL_SHARED_LIBRARIES += $(EXYNOS_VENDOR_SHARED_LIBS) $(EXYNOS_VENDOR_ENC_SHARED_LIBS)

LOCAL_CFLAGS +=	-fPIC \
                -O3 \
                -Werror \
                -Wall \
                -Wno-deprecated-enum-enum-conversion \
                -std=gnu++1z \
                -std=c++2a
LOCAL_CFLAGS += $(EXYNOS_GLOBAL_CFLAGS)
LOCAL_CFLAGS += $(EXYNOS_ENC_CFLAGS)

include $(BUILD_SHARED_LIBRARY)

#############################
####  libExynosC2HevcDec  ###
#############################
include $(CLEAR_VARS)

LOCAL_CFLAGS :=
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        c2component/Exynos_C2_Component.cpp \
        c2component/Exynos_C2_DecComponent.cpp \
        c2component/hevc/Exynos_C2_HevcDecComponent.cpp \
        c2component/Exynos_C2_IntfSetter.cpp \
        c2component/Exynos_C2_FilterParamCnv.cpp \
        filter/Exynos_Filter.cpp \
        filter/codec/Exynos_CodecBase_Filter.cpp \
        filter/codec/Exynos_CodecDecBase_Filter.cpp \
        filter/codec/hevc/Exynos_HevcDec_Filter.cpp \
        filter/csc/Exynos_CSC_Filter.cpp \
        filter/postprocess/Exynos_External_Filter.cpp \
        filter/postprocess/Exynos_HDR2SDR_Filter.cpp \
        filter/postprocess/Exynos_PostControl_Filter.cpp

LOCAL_C_INCLUDES :=

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosC2HevcDec
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE

LOCAL_PROPRIETARY_MODULE := true

LOCAL_HEADER_LIBRARIES := \
        libexynosc2_hevccomponent_headers \
        libstagefright_foundation_headers \
        libexynosc2_postprocessfilter_headers

LOCAL_HEADER_LIBRARIES += $(EXYNOS_VENDOR_HEADER_LIBS)

LOCAL_STATIC_LIBRARIES := \
        libExynosCSC \
        libExynosVideoCodec \
        libExynosC2OSAL \
        libExynosVideoApi2 \
        libVendorVideoApi

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libutils \
        libcutils \
        libhidlbase \
        libcodec2 \
        libcodec2_vndk \
        libsfplugin_ccodec_utils \
        libstagefright_foundation \
        libstagefright_xmlparser \
        libExynosC2ComponentStore \
        libexynosv4l2 \
        libhardware \
        libacryl
LOCAL_SHARED_LIBRARIES += $(EXYNOS_VENDOR_SHARED_LIBS)

LOCAL_CFLAGS +=	-fPIC \
                -O3 \
                -Werror \
                -Wall \
                -Wno-deprecated-enum-enum-conversion \
                -std=gnu++1z \
                -std=c++2a
LOCAL_CFLAGS += $(EXYNOS_GLOBAL_CFLAGS)
LOCAL_CFLAGS += $(EXYNOS_DEC_CFLAGS)

include $(BUILD_SHARED_LIBRARY)


#############################
####  libExynosC2HevcEnc  ###
#############################
include $(CLEAR_VARS)

LOCAL_CFLAGS :=
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        c2component/Exynos_C2_Component.cpp \
        c2component/Exynos_C2_EncComponent.cpp \
        c2component/hevc/Exynos_C2_HevcEncComponent.cpp \
        c2component/Exynos_C2_IntfSetter.cpp \
        c2component/Exynos_C2_FilterParamCnv.cpp \
        filter/Exynos_Filter.cpp \
        filter/codec/Exynos_CodecBase_Filter.cpp \
        filter/codec/Exynos_CodecEncBase_Filter.cpp \
        filter/codec/hevc/Exynos_HevcEnc_Filter.cpp \
        filter/codec/hevc/Exynos_HevcWFDEnc_Filter.cpp \
        filter/csc/Exynos_CSC_Filter.cpp
LOCAL_SRC_FILES += $(EXYNOS_VENDOR_ENC_SRC_FILES)

LOCAL_C_INCLUDES :=

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosC2HevcEnc
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE

LOCAL_PROPRIETARY_MODULE := true

LOCAL_HEADER_LIBRARIES := libexynosc2_hevccomponent_headers libstagefright_foundation_headers
LOCAL_HEADER_LIBRARIES += $(EXYNOS_VENDOR_HEADER_LIBS) $(EXYNOS_VENDOR_ENC_HEADER_LIBS)

LOCAL_STATIC_LIBRARIES := libExynosCSC libExynosVideoCodec libExynosC2OSAL libExynosVideoApi2 libVendorVideoApi
LOCAL_STATIC_LIBRARIES += $(EXYNOS_VENDOR_ENC_STATIC_LIBS)

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libutils \
        libcutils \
        libhidlbase \
        libcodec2 \
        libcodec2_vndk \
        libsfplugin_ccodec_utils \
        libstagefright_foundation \
        libstagefright_xmlparser \
        libExynosC2ComponentStore \
        libexynosv4l2 \
        libhardware \
        libacryl
LOCAL_SHARED_LIBRARIES += $(EXYNOS_VENDOR_SHARED_LIBS) $(EXYNOS_VENDOR_ENC_SHARED_LIBS)

LOCAL_CFLAGS +=	-fPIC \
                -O3 \
                -Werror \
                -Wall \
                -Wno-deprecated-enum-enum-conversion \
                -std=gnu++1z \
                -std=c++2a
LOCAL_CFLAGS += $(EXYNOS_GLOBAL_CFLAGS)
LOCAL_CFLAGS += $(EXYNOS_ENC_CFLAGS)

ifeq ($(BOARD_USE_HDR10PLUS_STAT_ENC), true)
LOCAL_CFLAGS += -DUSE_HDR10PLUS_STAT_ENC_FILTER
LOCAL_HEADER_LIBRARIES += libexynosc2_postprocessfilter_headers
LOCAL_SRC_FILES += filter/postprocess/Exynos_External_Filter.cpp \
                   filter/postprocess/Exynos_HDR10PlusStatEnc_Filter.cpp
ifneq ($(HDR_DYNAMIC_META_LIB), )
LOCAL_CFLAGS += -DHDR_DYNAMIC_META_LIB="\"$(HDR_DYNAMIC_META_LIB)\""
endif
endif

include $(BUILD_SHARED_LIBRARY)

#############################
#### libExynosC2Mpeg4Dec  ###
#############################
include $(CLEAR_VARS)

LOCAL_CFLAGS :=
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        c2component/Exynos_C2_Component.cpp \
        c2component/Exynos_C2_DecComponent.cpp \
        c2component/mpeg4/Exynos_C2_Mpeg4DecComponent.cpp \
        c2component/Exynos_C2_IntfSetter.cpp \
        c2component/Exynos_C2_FilterParamCnv.cpp \
        filter/Exynos_Filter.cpp \
        filter/codec/Exynos_CodecBase_Filter.cpp \
        filter/codec/Exynos_CodecDecBase_Filter.cpp \
        filter/codec/mpeg4/Exynos_Mpeg4Dec_Filter.cpp \
        filter/csc/Exynos_CSC_Filter.cpp

LOCAL_C_INCLUDES :=

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosC2Mpeg4Dec
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE

LOCAL_PROPRIETARY_MODULE := true

LOCAL_HEADER_LIBRARIES := libexynosc2_mpeg4component_headers libstagefright_foundation_headers
LOCAL_HEADER_LIBRARIES += $(EXYNOS_VENDOR_HEADER_LIBS)

LOCAL_STATIC_LIBRARIES := libExynosCSC libExynosVideoCodec libExynosC2OSAL libExynosVideoApi2 libVendorVideoApi

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libutils \
        libcutils \
        libhidlbase \
        libcodec2 \
        libcodec2_vndk \
        libsfplugin_ccodec_utils \
        libstagefright_foundation \
        libstagefright_xmlparser \
        libExynosC2ComponentStore \
        libexynosv4l2 \
        libhardware \
        libacryl
LOCAL_SHARED_LIBRARIES += $(EXYNOS_VENDOR_SHARED_LIBS)

LOCAL_CFLAGS +=	-fPIC \
                -O3 \
                -Werror \
                -Wall \
                -Wno-deprecated-enum-enum-conversion \
                -std=gnu++1z \
                -std=c++2a
LOCAL_CFLAGS += $(EXYNOS_GLOBAL_CFLAGS)
LOCAL_CFLAGS += $(EXYNOS_DEC_CFLAGS)

include $(BUILD_SHARED_LIBRARY)

#############################
#### libExynosC2Mpeg4Enc  ###
#############################
include $(CLEAR_VARS)

LOCAL_CFLAGS :=
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        c2component/Exynos_C2_Component.cpp \
        c2component/Exynos_C2_EncComponent.cpp \
        c2component/mpeg4/Exynos_C2_Mpeg4EncComponent.cpp \
        c2component/Exynos_C2_IntfSetter.cpp \
        c2component/Exynos_C2_FilterParamCnv.cpp \
        filter/Exynos_Filter.cpp \
        filter/codec/Exynos_CodecBase_Filter.cpp \
        filter/codec/Exynos_CodecEncBase_Filter.cpp \
        filter/codec/mpeg4/Exynos_Mpeg4Enc_Filter.cpp \
        filter/csc/Exynos_CSC_Filter.cpp
LOCAL_SRC_FILES += $(EXYNOS_VENDOR_ENC_SRC_FILES)

LOCAL_C_INCLUDES :=

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosC2Mpeg4Enc
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE

LOCAL_PROPRIETARY_MODULE := true

LOCAL_HEADER_LIBRARIES := libexynosc2_mpeg4component_headers libstagefright_foundation_headers
LOCAL_HEADER_LIBRARIES += $(EXYNOS_VENDOR_HEADER_LIBS) $(EXYNOS_VENDOR_ENC_HEADER_LIBS)

LOCAL_STATIC_LIBRARIES := libExynosCSC libExynosVideoCodec libExynosC2OSAL libExynosVideoApi2  libVendorVideoApi
LOCAL_STATIC_LIBRARIES += $(EXYNOS_VENDOR_ENC_STATIC_LIBS)

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libutils \
        libcutils \
        libhidlbase \
        libcodec2 \
        libcodec2_vndk \
        libsfplugin_ccodec_utils \
        libstagefright_foundation \
        libstagefright_xmlparser \
        libExynosC2ComponentStore \
        libexynosv4l2 \
        libhardware \
        libacryl
LOCAL_SHARED_LIBRARIES += $(EXYNOS_VENDOR_SHARED_LIBS) $(EXYNOS_VENDOR_ENC_SHARED_LIBS)

LOCAL_CFLAGS +=	-fPIC \
                -O3 \
                -Werror \
                -Wall \
                -Wno-deprecated-enum-enum-conversion \
                -std=gnu++1z \
                -std=c++2a
LOCAL_CFLAGS += $(EXYNOS_GLOBAL_CFLAGS)
LOCAL_CFLAGS += $(EXYNOS_ENC_CFLAGS)

include $(BUILD_SHARED_LIBRARY)

#############################
#### libExynosC2H263Dec  ###
#############################
include $(CLEAR_VARS)

LOCAL_CFLAGS :=
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        c2component/Exynos_C2_Component.cpp \
        c2component/Exynos_C2_DecComponent.cpp \
        c2component/h263/Exynos_C2_H263DecComponent.cpp \
        c2component/Exynos_C2_IntfSetter.cpp \
        c2component/Exynos_C2_FilterParamCnv.cpp \
        filter/Exynos_Filter.cpp \
        filter/codec/Exynos_CodecBase_Filter.cpp \
        filter/codec/Exynos_CodecDecBase_Filter.cpp \
        filter/codec/h263/Exynos_H263Dec_Filter.cpp \
        filter/csc/Exynos_CSC_Filter.cpp

LOCAL_C_INCLUDES :=

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosC2H263Dec
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE

LOCAL_PROPRIETARY_MODULE := true

LOCAL_HEADER_LIBRARIES := libexynosc2_h263component_headers libstagefright_foundation_headers
LOCAL_HEADER_LIBRARIES += $(EXYNOS_VENDOR_HEADER_LIBS)

LOCAL_STATIC_LIBRARIES := libExynosCSC libExynosVideoCodec libExynosC2OSAL libExynosVideoApi2 libVendorVideoApi

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libutils \
        libcutils \
        libhidlbase \
        libcodec2 \
        libcodec2_vndk \
        libsfplugin_ccodec_utils \
        libstagefright_foundation \
        libstagefright_xmlparser \
        libExynosC2ComponentStore \
        libexynosv4l2 \
        libhardware \
        libacryl
LOCAL_SHARED_LIBRARIES += $(EXYNOS_VENDOR_SHARED_LIBS)

LOCAL_CFLAGS +=	-fPIC \
                -O3 \
                -Werror \
                -Wall \
                -Wno-deprecated-enum-enum-conversion \
                -std=gnu++1z \
                -std=c++2a
LOCAL_CFLAGS += $(EXYNOS_GLOBAL_CFLAGS)
LOCAL_CFLAGS += $(EXYNOS_DEC_CFLAGS)

include $(BUILD_SHARED_LIBRARY)

#############################
#### libExynosC2H263Enc  ###
#############################
include $(CLEAR_VARS)

LOCAL_CFLAGS :=
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        c2component/Exynos_C2_Component.cpp \
        c2component/Exynos_C2_EncComponent.cpp \
        c2component/h263/Exynos_C2_H263EncComponent.cpp \
        c2component/Exynos_C2_IntfSetter.cpp \
        c2component/Exynos_C2_FilterParamCnv.cpp \
        filter/Exynos_Filter.cpp \
        filter/codec/Exynos_CodecBase_Filter.cpp \
        filter/codec/Exynos_CodecEncBase_Filter.cpp \
        filter/codec/h263/Exynos_H263Enc_Filter.cpp \
        filter/csc/Exynos_CSC_Filter.cpp
LOCAL_SRC_FILES += $(EXYNOS_VENDOR_ENC_SRC_FILES)

LOCAL_C_INCLUDES :=

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosC2H263Enc
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE

LOCAL_PROPRIETARY_MODULE := true

LOCAL_HEADER_LIBRARIES := libexynosc2_h263component_headers libstagefright_foundation_headers
LOCAL_HEADER_LIBRARIES += $(EXYNOS_VENDOR_HEADER_LIBS) $(EXYNOS_VENDOR_ENC_HEADER_LIBS)

LOCAL_STATIC_LIBRARIES := libExynosCSC libExynosVideoCodec libExynosC2OSAL libExynosVideoApi2 libVendorVideoApi
LOCAL_STATIC_LIBRARIES += $(EXYNOS_VENDOR_ENC_STATIC_LIBS)

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libutils \
        libcutils \
        libhidlbase \
        libcodec2 \
        libcodec2_vndk \
        libsfplugin_ccodec_utils \
        libstagefright_foundation \
        libstagefright_xmlparser \
        libExynosC2ComponentStore \
        libexynosv4l2 \
        libhardware \
        libacryl
LOCAL_SHARED_LIBRARIES += $(EXYNOS_VENDOR_SHARED_LIBS) $(EXYNOS_VENDOR_ENC_SHARED_LIBS)

LOCAL_CFLAGS +=	-fPIC \
                -O3 \
                -Werror \
                -Wall \
                -Wno-deprecated-enum-enum-conversion \
                -std=gnu++1z \
                -std=c++2a
LOCAL_CFLAGS += $(EXYNOS_GLOBAL_CFLAGS)
LOCAL_CFLAGS += $(EXYNOS_ENC_CFLAGS)

include $(BUILD_SHARED_LIBRARY)

#############################
#### libExynosC2Vp8Dec  ###
#############################
include $(CLEAR_VARS)

LOCAL_CFLAGS :=
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        c2component/Exynos_C2_Component.cpp \
        c2component/Exynos_C2_DecComponent.cpp \
        c2component/vp8/Exynos_C2_Vp8DecComponent.cpp \
        c2component/Exynos_C2_IntfSetter.cpp \
        c2component/Exynos_C2_FilterParamCnv.cpp \
        filter/Exynos_Filter.cpp \
        filter/codec/Exynos_CodecBase_Filter.cpp \
        filter/codec/Exynos_CodecDecBase_Filter.cpp \
        filter/codec/vp8/Exynos_Vp8Dec_Filter.cpp \
        filter/csc/Exynos_CSC_Filter.cpp

LOCAL_C_INCLUDES :=

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosC2Vp8Dec
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE

LOCAL_PROPRIETARY_MODULE := true

LOCAL_HEADER_LIBRARIES := libexynosc2_vp8component_headers libstagefright_foundation_headers
LOCAL_HEADER_LIBRARIES += $(EXYNOS_VENDOR_HEADER_LIBS)

LOCAL_STATIC_LIBRARIES := libExynosCSC libExynosVideoCodec libExynosC2OSAL libExynosVideoApi2 libVendorVideoApi

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libutils \
        libcutils \
        libhidlbase \
        libcodec2 \
        libcodec2_vndk \
        libsfplugin_ccodec_utils \
        libstagefright_foundation \
        libstagefright_xmlparser \
        libExynosC2ComponentStore \
        libexynosv4l2 \
        libhardware \
        libacryl
LOCAL_SHARED_LIBRARIES += $(EXYNOS_VENDOR_SHARED_LIBS)

LOCAL_CFLAGS +=	-fPIC \
                -O3 \
                -Werror \
                -Wall \
                -Wno-deprecated-enum-enum-conversion \
                -std=gnu++1z \
                -std=c++2a
LOCAL_CFLAGS += $(EXYNOS_GLOBAL_CFLAGS)
LOCAL_CFLAGS += $(EXYNOS_DEC_CFLAGS)

include $(BUILD_SHARED_LIBRARY)

#############################
####  libExynosC2Vp8Enc  ###
#############################
include $(CLEAR_VARS)

LOCAL_CFLAGS :=
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        c2component/Exynos_C2_Component.cpp \
        c2component/Exynos_C2_EncComponent.cpp \
        c2component/vp8/Exynos_C2_Vp8EncComponent.cpp \
        c2component/Exynos_C2_IntfSetter.cpp \
        c2component/Exynos_C2_FilterParamCnv.cpp \
        filter/Exynos_Filter.cpp \
        filter/codec/Exynos_CodecBase_Filter.cpp \
        filter/codec/Exynos_CodecEncBase_Filter.cpp \
        filter/codec/vp8/Exynos_Vp8Enc_Filter.cpp \
        filter/csc/Exynos_CSC_Filter.cpp
LOCAL_SRC_FILES += $(EXYNOS_VENDOR_ENC_SRC_FILES)

LOCAL_C_INCLUDES :=

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosC2Vp8Enc
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE

LOCAL_PROPRIETARY_MODULE := true

LOCAL_HEADER_LIBRARIES := libexynosc2_vp8component_headers libstagefright_foundation_headers
LOCAL_HEADER_LIBRARIES += $(EXYNOS_VENDOR_HEADER_LIBS) $(EXYNOS_VENDOR_ENC_HEADER_LIBS)

LOCAL_STATIC_LIBRARIES := libExynosCSC libExynosVideoCodec libExynosC2OSAL libExynosVideoApi2 libVendorVideoApi
LOCAL_STATIC_LIBRARIES += $(EXYNOS_VENDOR_ENC_STATIC_LIBS)

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libutils \
        libcutils \
        libhidlbase \
        libcodec2 \
        libcodec2_vndk \
        libsfplugin_ccodec_utils \
        libstagefright_foundation \
        libstagefright_xmlparser \
        libExynosC2ComponentStore \
        libexynosv4l2 \
        libhardware \
        libacryl
LOCAL_SHARED_LIBRARIES += $(EXYNOS_VENDOR_SHARED_LIBS) $(EXYNOS_VENDOR_ENC_SHARED_LIBS)

LOCAL_CFLAGS +=	-fPIC \
                -O3 \
                -Werror \
                -Wall \
                -Wno-deprecated-enum-enum-conversion \
                -std=gnu++1z \
                -std=c++2a
LOCAL_CFLAGS += $(EXYNOS_GLOBAL_CFLAGS)
LOCAL_CFLAGS += $(EXYNOS_ENC_CFLAGS)

include $(BUILD_SHARED_LIBRARY)

#############################
#### libExynosC2Vp9Dec  ###
#############################
include $(CLEAR_VARS)

LOCAL_CFLAGS :=
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        c2component/Exynos_C2_Component.cpp \
        c2component/Exynos_C2_DecComponent.cpp \
        c2component/vp9/Exynos_C2_Vp9DecComponent.cpp \
        c2component/Exynos_C2_IntfSetter.cpp \
        c2component/Exynos_C2_FilterParamCnv.cpp \
        filter/Exynos_Filter.cpp \
        filter/codec/Exynos_CodecBase_Filter.cpp \
        filter/codec/Exynos_CodecDecBase_Filter.cpp \
        filter/codec/vp9/Exynos_Vp9Dec_Filter.cpp \
        filter/csc/Exynos_CSC_Filter.cpp \
        filter/postprocess/Exynos_External_Filter.cpp \
        filter/postprocess/Exynos_HDR2SDR_Filter.cpp \
        filter/postprocess/Exynos_PostControl_Filter.cpp

LOCAL_C_INCLUDES :=

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosC2Vp9Dec
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE

LOCAL_PROPRIETARY_MODULE := true

LOCAL_HEADER_LIBRARIES := \
        libexynosc2_vp9component_headers \
        libstagefright_foundation_headers \
        libexynosc2_postprocessfilter_headers

LOCAL_HEADER_LIBRARIES += $(EXYNOS_VENDOR_HEADER_LIBS)

LOCAL_STATIC_LIBRARIES := libExynosCSC libExynosVideoCodec libExynosC2OSAL libExynosVideoApi2 libVendorVideoApi

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libutils \
        libcutils \
        libhidlbase \
        libcodec2 \
        libcodec2_vndk \
        libsfplugin_ccodec_utils \
        libstagefright_foundation \
        libstagefright_xmlparser \
        libExynosC2ComponentStore \
        libexynosv4l2 \
        libhardware \
        libacryl
LOCAL_SHARED_LIBRARIES += $(EXYNOS_VENDOR_SHARED_LIBS)

LOCAL_CFLAGS +=	-fPIC \
                -O3 \
                -Werror \
                -Wall \
                -Wno-deprecated-enum-enum-conversion \
                -std=gnu++1z \
                -std=c++2a
LOCAL_CFLAGS += $(EXYNOS_GLOBAL_CFLAGS)
LOCAL_CFLAGS += $(EXYNOS_DEC_CFLAGS)

include $(BUILD_SHARED_LIBRARY)

#############################
####  libExynosC2Vp9Enc  ###
#############################
include $(CLEAR_VARS)

LOCAL_CFLAGS :=
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        c2component/Exynos_C2_Component.cpp \
        c2component/Exynos_C2_EncComponent.cpp \
        c2component/vp9/Exynos_C2_Vp9EncComponent.cpp \
        c2component/Exynos_C2_IntfSetter.cpp \
        c2component/Exynos_C2_FilterParamCnv.cpp \
        filter/Exynos_Filter.cpp \
        filter/codec/Exynos_CodecBase_Filter.cpp \
        filter/codec/Exynos_CodecEncBase_Filter.cpp \
        filter/codec/vp9/Exynos_Vp9Enc_Filter.cpp \
        filter/csc/Exynos_CSC_Filter.cpp
LOCAL_SRC_FILES += $(EXYNOS_VENDOR_ENC_SRC_FILES)

LOCAL_C_INCLUDES :=

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosC2Vp9Enc
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE

LOCAL_PROPRIETARY_MODULE := true

LOCAL_HEADER_LIBRARIES := libexynosc2_vp9component_headers libstagefright_foundation_headers
LOCAL_HEADER_LIBRARIES += $(EXYNOS_VENDOR_HEADER_LIBS) $(EXYNOS_VENDOR_ENC_HEADER_LIBS)

LOCAL_STATIC_LIBRARIES := libExynosCSC libExynosVideoCodec libExynosC2OSAL libExynosVideoApi2 libVendorVideoApi
LOCAL_STATIC_LIBRARIES += $(EXYNOS_VENDOR_ENC_STATIC_LIBS)

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libutils \
        libcutils \
        libhidlbase \
        libcodec2 \
        libcodec2_vndk \
        libsfplugin_ccodec_utils \
        libstagefright_foundation \
        libstagefright_xmlparser \
        libExynosC2ComponentStore \
        libexynosv4l2 \
        libhardware \
        libacryl
LOCAL_SHARED_LIBRARIES += $(EXYNOS_VENDOR_SHARED_LIBS) $(EXYNOS_VENDOR_ENC_SHARED_LIBS)

LOCAL_CFLAGS +=	-fPIC \
                -O3 \
                -Werror \
                -Wall \
                -Wno-deprecated-enum-enum-conversion \
                -std=gnu++1z \
                -std=c++2a
LOCAL_CFLAGS += $(EXYNOS_GLOBAL_CFLAGS)
LOCAL_CFLAGS += $(EXYNOS_ENC_CFLAGS)

include $(BUILD_SHARED_LIBRARY)

#############################
#### libExynosC2Mpeg2Dec  ###
#############################
include $(CLEAR_VARS)

LOCAL_CFLAGS :=
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        c2component/Exynos_C2_Component.cpp \
        c2component/Exynos_C2_DecComponent.cpp \
        c2component/mpeg2/Exynos_C2_Mpeg2DecComponent.cpp \
        c2component/Exynos_C2_IntfSetter.cpp \
        c2component/Exynos_C2_FilterParamCnv.cpp \
        filter/Exynos_Filter.cpp \
        filter/codec/Exynos_CodecBase_Filter.cpp \
        filter/codec/Exynos_CodecDecBase_Filter.cpp \
        filter/codec/mpeg2/Exynos_Mpeg2Dec_Filter.cpp \
        filter/csc/Exynos_CSC_Filter.cpp

LOCAL_C_INCLUDES :=

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosC2Mpeg2Dec
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE

LOCAL_PROPRIETARY_MODULE := true

LOCAL_HEADER_LIBRARIES := libexynosc2_mpeg2component_headers libstagefright_foundation_headers
LOCAL_HEADER_LIBRARIES += $(EXYNOS_VENDOR_HEADER_LIBS)

LOCAL_STATIC_LIBRARIES := libExynosCSC libExynosVideoCodec libExynosC2OSAL libExynosVideoApi2 libVendorVideoApi

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libutils \
        libcutils \
        libhidlbase \
        libcodec2 \
        libcodec2_vndk \
        libsfplugin_ccodec_utils \
        libstagefright_foundation \
        libstagefright_xmlparser \
        libExynosC2ComponentStore \
        libexynosv4l2 \
        libhardware \
        libacryl
LOCAL_SHARED_LIBRARIES += $(EXYNOS_VENDOR_SHARED_LIBS)

LOCAL_CFLAGS +=	-fPIC \
                -O3 \
                -Werror \
                -Wall \
                -Wno-deprecated-enum-enum-conversion \
                -std=gnu++1z \
                -std=c++2a
LOCAL_CFLAGS += $(EXYNOS_GLOBAL_CFLAGS)
LOCAL_CFLAGS += $(EXYNOS_DEC_CFLAGS)

include $(BUILD_SHARED_LIBRARY)

#############################
#### libExynosC2Vc1Dec    ###
#############################
include $(CLEAR_VARS)

LOCAL_CFLAGS :=
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        c2component/Exynos_C2_Component.cpp \
        c2component/Exynos_C2_DecComponent.cpp \
        c2component/vc1/Exynos_C2_Vc1DecComponent.cpp \
        c2component/Exynos_C2_IntfSetter.cpp \
        c2component/Exynos_C2_FilterParamCnv.cpp \
        filter/Exynos_Filter.cpp \
        filter/codec/Exynos_CodecBase_Filter.cpp \
        filter/codec/Exynos_CodecDecBase_Filter.cpp \
        filter/codec/vc1/Exynos_Vc1Dec_Filter.cpp \
        filter/csc/Exynos_CSC_Filter.cpp

LOCAL_C_INCLUDES :=

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosC2Vc1Dec
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE

LOCAL_PROPRIETARY_MODULE := true

LOCAL_HEADER_LIBRARIES := libexynosc2_vc1component_headers libstagefright_foundation_headers
LOCAL_HEADER_LIBRARIES += $(EXYNOS_VENDOR_HEADER_LIBS)

LOCAL_STATIC_LIBRARIES := libExynosCSC libExynosVideoCodec libExynosC2OSAL libExynosVideoApi2  libVendorVideoApi

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libutils \
        libcutils \
        libhidlbase \
        libcodec2 \
        libcodec2_vndk \
        libsfplugin_ccodec_utils \
        libstagefright_foundation \
        libstagefright_xmlparser \
        libExynosC2ComponentStore \
        libexynosv4l2 \
        libhardware \
        libacryl
LOCAL_SHARED_LIBRARIES += $(EXYNOS_VENDOR_SHARED_LIBS)

LOCAL_CFLAGS +=	-fPIC \
                -O3 \
                -Werror \
                -Wall \
                -Wno-deprecated-enum-enum-conversion \
                -std=gnu++1z \
                -std=c++2a
LOCAL_CFLAGS += $(EXYNOS_GLOBAL_CFLAGS)
LOCAL_CFLAGS += $(EXYNOS_DEC_CFLAGS)

include $(BUILD_SHARED_LIBRARY)

#############################
#### libExynosC2WmvDec    ###
#############################
include $(CLEAR_VARS)

LOCAL_CFLAGS :=
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        c2component/Exynos_C2_Component.cpp \
        c2component/Exynos_C2_DecComponent.cpp \
        c2component/vc1/Exynos_C2_Vc1DecComponent.cpp \
        c2component/Exynos_C2_IntfSetter.cpp \
        c2component/Exynos_C2_FilterParamCnv.cpp \
        filter/Exynos_Filter.cpp \
        filter/codec/Exynos_CodecBase_Filter.cpp \
        filter/codec/Exynos_CodecDecBase_Filter.cpp \
        filter/codec/vc1/Exynos_Vc1Dec_Filter.cpp \
        filter/csc/Exynos_CSC_Filter.cpp

LOCAL_C_INCLUDES :=

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosC2WmvDec
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE

LOCAL_PROPRIETARY_MODULE := true

LOCAL_HEADER_LIBRARIES := libexynosc2_vc1component_headers libstagefright_foundation_headers
LOCAL_HEADER_LIBRARIES += $(EXYNOS_VENDOR_HEADER_LIBS)

LOCAL_STATIC_LIBRARIES := libExynosCSC libExynosVideoCodec libExynosC2OSAL libExynosVideoApi2 libVendorVideoApi

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libutils \
        libcutils \
        libhidlbase \
        libcodec2 \
        libcodec2_vndk \
        libsfplugin_ccodec_utils \
        libstagefright_foundation \
        libstagefright_xmlparser \
        libExynosC2ComponentStore \
        libexynosv4l2 \
        libhardware \
        libacryl
LOCAL_SHARED_LIBRARIES += $(EXYNOS_VENDOR_SHARED_LIBS)

LOCAL_CFLAGS +=	-fPIC \
                -O3 \
                -Werror \
                -Wall \
                -Wno-deprecated-enum-enum-conversion \
                -std=gnu++1z \
                -std=c++2a
LOCAL_CFLAGS += $(EXYNOS_GLOBAL_CFLAGS)
LOCAL_CFLAGS += $(EXYNOS_DEC_CFLAGS)
LOCAL_CFLAGS += -DBUILD_WMV

include $(BUILD_SHARED_LIBRARY)

#############################
####  libExynosC2Av1Dec  ###
#############################
include $(CLEAR_VARS)

LOCAL_CFLAGS :=
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        c2component/Exynos_C2_Component.cpp \
        c2component/Exynos_C2_DecComponent.cpp \
        c2component/av1/Exynos_C2_Av1DecComponent.cpp \
        c2component/Exynos_C2_IntfSetter.cpp \
        c2component/Exynos_C2_FilterParamCnv.cpp \
        filter/Exynos_Filter.cpp \
        filter/codec/Exynos_CodecBase_Filter.cpp \
        filter/codec/Exynos_CodecDecBase_Filter.cpp \
        filter/codec/av1/Exynos_Av1Dec_Filter.cpp \
        filter/csc/Exynos_CSC_Filter.cpp \
        filter/postprocess/Exynos_External_Filter.cpp \
        filter/postprocess/Exynos_FilmGrain_Filter.cpp \
        filter/postprocess/Exynos_HDR2SDR_Filter.cpp \
        filter/postprocess/Exynos_PostControl_Filter.cpp

LOCAL_C_INCLUDES :=

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libExynosC2Av1Dec
LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_NOTICE_FILE := $(LOCAL_PATH)/NOTICE

LOCAL_PROPRIETARY_MODULE := true

LOCAL_HEADER_LIBRARIES := \
        libexynosc2_av1component_headers \
        libstagefright_foundation_headers \
        libexynosc2_postprocessfilter_headers

LOCAL_HEADER_LIBRARIES += $(EXYNOS_VENDOR_HEADER_LIBS)

LOCAL_STATIC_LIBRARIES := \
        libExynosCSC \
        libExynosVideoCodec \
        libExynosC2OSAL \
        libExynosVideoApi2 \
        libVendorVideoApi

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libutils \
        libcutils \
        libhidlbase \
        libcodec2 \
        libcodec2_vndk \
        libsfplugin_ccodec_utils \
        libstagefright_foundation \
        libExynosC2ComponentStore \
        libstagefright_xmlparser \
        libexynosv4l2 \
        libhardware \
        libacryl
LOCAL_SHARED_LIBRARIES += $(EXYNOS_VENDOR_SHARED_LIBS)

LOCAL_CFLAGS +=	-fPIC \
                -O3 \
                -Werror \
                -Wall \
                -Wno-deprecated-enum-enum-conversion \
                -std=gnu++1z \
                -std=c++2a
LOCAL_CFLAGS += $(EXYNOS_GLOBAL_CFLAGS)
LOCAL_CFLAGS += $(EXYNOS_DEC_CFLAGS)

include $(BUILD_SHARED_LIBRARY)

include $(EXYNOS_CODEC2_TOP)/osal/Android.mk
include $(EXYNOS_CODEC2_TOP)/videocodec/Android.mk
include $(EXYNOS_CODEC2_TOP)/csc/Android.mk
ifeq ($(BOARD_USE_GDC), true)
include $(EXYNOS_CODEC2_TOP)/gdc/Android.mk
endif

ifneq ($(BOARD_USE_DEFAULT_SERVICE), true)
include $(EXYNOS_CODEC2_TOP)/services/Android.mk

$(warning #### [CODEC2] SDK VERSION = $(PLATFORM_SDK_VERSION))
ifeq ($(shell expr $(PLATFORM_SDK_VERSION) \> 31), 1)
include $(EXYNOS_CODEC2_TOP)/plugin/Android.mk
endif

endif
endif
