/*
 *
 * Copyright 2021 Samsung Electronics S.LSI Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <algorithm>
#include <string>

#include <Codec2Mapper.h>

#include "Exynos_C2_Component.h"
#include "Exynos_C2_DecComponent.h"
#include "Exynos_C2_EncComponent.h"
#include "Exynos_C2_IntfSetter.h"


C2R CommonSetter::SubscribedParamIndicesSetter(bool mayBlock, C2InterfaceHelper::C2P<C2SubscribedParamIndicesTuning> &me) {
    (void)mayBlock;

    StaticExynosLog(Level::Debug, "CommonParamIntf", "[%s] subscribed indices : %d", __FUNCTION__, me.v.flexCount());

    return C2R::Ok();
}

C2R CommonSetter::OperateRateSetter(bool mayBlock, C2InterfaceHelper::C2P<C2OperatingRateTuning> &me) {
    (void)mayBlock;

    if (me.v.value < 0) {
        me.set().value = 0;  /* negative value is not allowed */
    }

    StaticExynosLog(Level::Debug, "CommonParamIntf", "[%s] operating rate: %f", __FUNCTION__, me.v.value);

    return C2R::Ok();
}

C2R CommonSetter::RealTimePrioritySetter(bool mayBlock, C2InterfaceHelper::C2P<C2RealTimePriorityTuning> &me) {
    (void)mayBlock;

    if (me.v.value > 0) {
        me.set().value = 0;  /* Positive value is not allowed */
    }

    StaticExynosLog(Level::Debug, "CommonParamIntf", "[%s] RealTimePriority: %d", __FUNCTION__, me.v.value);

    return C2R::Ok();
}

C2R DecCommonSetter::UsageSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamUsageTuning::output> &me) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] usage:0x%llx", __FUNCTION__, me.v.value);

    return res;
}

C2R DecCommonSetter::StreamUsageSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosStreamOutputStreamSetting::output> &me,
                             const C2InterfaceHelper::C2P<C2StreamPixelFormatInfo::output> &pixelFormat,
                             const C2InterfaceHelper::C2P<C2StreamUsageTuning::output> &usage) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    if (pixelFormat.v.value != HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED) {
        me.set().value = me.v.value | C2MemoryUsage::CPU_READ;
    }

    me.set().value = me.v.value | usage.v.value;

    StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] stream usage:0x%llx, pixelFormat:0x%x, usage:0x%llx",
                            __FUNCTION__, me.v.value, pixelFormat.v.value, usage.v.value);

    return res;
}

C2R DecCommonSetter::SizeSetter(bool mayBlock, const C2InterfaceHelper::C2P<C2StreamPictureSizeInfo::output> &oldMe,
                      C2InterfaceHelper::C2P<C2StreamPictureSizeInfo::output> &me) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    if (!me.F(me.v.width).supportsAtAll(me.v.width)) {
        res = res.plus(C2SettingResultBuilder::BadValue(me.F(me.v.width)));

        StaticExynosLog(Level::Error, "VdecCommonParamIntf", "[%s] width(%d) is invalid", __FUNCTION__, me.v.width);
        me.set().width = oldMe.v.width;
    }
    if (!me.F(me.v.height).supportsAtAll(me.v.height)) {
        res = res.plus(C2SettingResultBuilder::BadValue(me.F(me.v.height)));

        StaticExynosLog(Level::Error, "VdecCommonParamIntf", "[%s] height(%d) is invalid", __FUNCTION__, me.v.height);
        me.set().height = oldMe.v.height;
    }

    StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] width(%d), height(%d)", __FUNCTION__, me.v.width, me.v.height);

    return res;
}

C2R DecCommonSetter::MaxInputSizeSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamMaxBufferSizeInfo::input> &me,
                              const C2InterfaceHelper::C2P<C2StreamMaxPictureSizeTuning::output> &maxSize) {
    (void)mayBlock;

    uint32_t size = 0;

    if ((maxSize.v.width > 0) && (maxSize.v.height > 0)) {
        size = GET_INPUT_MAX_SIZE(maxSize.v.width, maxSize.v.height);
    }

    me.set().value = c2_max(size, me.v.value);

    StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] max input size = %d", __FUNCTION__, me.v.value);

    return C2R::Ok();
}

C2R DecCommonSetter::MaxInputSizeSetterForSecure(bool mayBlock,
                              C2InterfaceHelper::C2P<C2StreamMaxBufferSizeInfo::input> &me,
                              const C2InterfaceHelper::C2P<C2StreamMaxPictureSizeTuning::output> &maxSize) {
    (void)mayBlock;

    uint32_t size = 0;

    if ((maxSize.v.width > 0) && (maxSize.v.height > 0)) {
        size = GET_INPUT_MAX_SIZE(maxSize.v.width, maxSize.v.height);
    }

    me.set().value = c2_max(size, me.v.value);

    StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] max input size = %d", __FUNCTION__, me.v.value);

#ifdef USE_SMALL_SECURE_MEMORY
    if (me.v.value > LIMITED_SECURE_INPUT_SIZE) {
        me.set().value = LIMITED_SECURE_INPUT_SIZE;

        StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] use limited max input size = %d", __FUNCTION__, me.v.value);
    }
#endif

    return C2R::Ok();
}


C2R DecCommonSetter::ActualOutputDelaySetter(bool mayBlock, C2InterfaceHelper::C2P<C2PortActualDelayTuning::output> &me,
                                   const C2InterfaceHelper::C2P<C2ExynosStreamMaxDPBNumInfo::output> &maxDPBNumInfo) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    me.set().value = (maxDPBNumInfo.v.value == DEFAULT_DPB)? DEFAULT_OUTPUT_DELAY:c2_max(maxDPBNumInfo.v.value, MIN_OUTPUT_DELAY);

    StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] output delay : %d", __FUNCTION__, me.v.value);

    return res;
}

C2R DecCommonSetter::ActualOutputDelaySetterWithExtra(bool mayBlock, C2InterfaceHelper::C2P<C2PortActualDelayTuning::output> &me,
                                            const C2InterfaceHelper::C2P<C2ExynosStreamMaxDPBNumInfo::output> &maxDPBNumInfo,
                                            const C2InterfaceHelper::C2P<C2ExynosPortExtraBufferNumInfo::output> &extBufferNumInfo) {
    (void)mayBlock;
    C2R res = C2R::Ok();


    uint32_t count = (maxDPBNumInfo.v.value == DEFAULT_DPB)? DEFAULT_OUTPUT_DELAY:maxDPBNumInfo.v.value;

    me.set().value = count + extBufferNumInfo.v.value;

    StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] output delay : %d / incl. extra(%d)", __FUNCTION__,
                            me.v.value, extBufferNumInfo.v.value);

    return res;
}

C2R DecCommonSetter::PixelFormatSetter(bool mayBlock,
                                       const C2InterfaceHelper::C2P<C2StreamPixelFormatInfo::output> &old,
                                       C2InterfaceHelper::C2P<C2StreamPixelFormatInfo::output> &me) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    if (!me.F(me.v.value).supportsNow(me.v.value)) {
        StaticExynosLog(Level::Error, "VdecCommonParamIntf", "[%s] output format:0x%x is not supported", __FUNCTION__, me.v.value);
        me.set().value = old.v.value;
        res = C2SettingResultBuilder::BadValue(me.F(me.v.value));
    }

    StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] output format:0x%x", __FUNCTION__, me.v.value);

    return res;
}

C2R DecCommonSetter::ActualFormatSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosCSCOutputPixelFormatInfo> &me,
                                            const C2InterfaceHelper::C2P<C2StreamPixelFormatInfo::output> &pixelFormat) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    me.set().value = pixelFormat.v.value;

    StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] CSC's output format:0x%x", __FUNCTION__, me.v.value);

    return res;
}

C2R DecCommonSetter::DefaultColorAspectsSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamColorAspectsTuning::output> &me) {
    return BaseColorAspectsSetter(mayBlock, me);
}

C2R DecCommonSetter::CodedColorAspectsSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamColorAspectsInfo::input> &me) {
    return BaseColorAspectsSetter(mayBlock, me);
}

C2R DecCommonSetter::ColorAspectsSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamColorAspectsInfo::output> &me,
                              const C2InterfaceHelper::C2P<C2StreamColorAspectsTuning::output> &def,
                              const C2InterfaceHelper::C2P<C2StreamColorAspectsInfo::input> &coded) {
    (void)mayBlock;

    // take default values for all unspecified fields, and coded values for specified ones
    me.set().range = (coded.v.range == C2Color::RANGE_UNSPECIFIED)? def.v.range:coded.v.range;
    me.set().primaries = (coded.v.primaries == C2Color::PRIMARIES_UNSPECIFIED)? def.v.primaries:coded.v.primaries;
    me.set().transfer = (coded.v.transfer == C2Color::TRANSFER_UNSPECIFIED)? def.v.transfer:coded.v.transfer;
    me.set().matrix = (coded.v.matrix == C2Color::MATRIX_UNSPECIFIED)? def.v.matrix:coded.v.matrix;

    StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] final: r(%d), p(0x%x), t(0x%x), m(0x%x)", __FUNCTION__,
                        me.v.range, me.v.primaries, me.v.transfer, me.v.matrix);
    return C2R::Ok();
}

C2R DecCommonSetter::ReorderTimestampSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosPortReorderTimestampSetting::output> &me) {
    (void)mayBlock;

    StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] reorder timestamp is %s", __FUNCTION__,
                        (me.v.value == On)? "enabled":"disabled");

    return C2R::Ok();
}

C2R DecCommonSetter::DecodeOrderSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosPortDecodeOrderSetting::output> &me,
                             const C2InterfaceHelper::C2P<C2ExynosPortReorderTimestampSetting::output> &reorderTimestamp) {
    (void)mayBlock;

    if (reorderTimestamp.v.value == On) {
        me.set().value = Off;

        StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] reorder timestamp is enabled. decode order decoding can not be used", __FUNCTION__);
    }

    StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] decode order decoding is %s", __FUNCTION__,
                        (me.v.value == On)? "enabled":"disabled");

    return C2R::Ok();
}

C2R DecCommonSetter::DecodeOrderSetterWithSize(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosPortDecodeOrderSetting::output> &me,
                                     const C2InterfaceHelper::C2P<C2ExynosPortReorderTimestampSetting::output> &reorderTimestamp,
                                     const C2InterfaceHelper::C2P<C2StreamPictureSizeInfo::output> &streamSize) {
    (void)mayBlock;

    if ((reorderTimestamp.v.value == On) ||
        ((streamSize.v.width * streamSize.v.height) > UHD_IMAGE_SIZE)) {
        me.set().value = Off;

        StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] reorder timestamp is enabled. decode order decoding can not be used", __FUNCTION__);
    }

    StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] decode order decoding is %s", __FUNCTION__,
                        (me.v.value == On)? "enabled":"disabled");

    return C2R::Ok();
}

C2R DecCommonSetter::ReorderDepthSetter(bool mayBlock, C2InterfaceHelper::C2P<C2PortReorderBufferDepthTuning::output> &me,
                              const C2InterfaceHelper::C2P<C2StreamPictureSizeInfo::output> &streamSize,
                              const C2InterfaceHelper::C2P<C2ExynosPortDecodeOrderSetting::output> &decodeOrder,
                              const C2InterfaceHelper::C2P<C2ExynosStreamMaxDPBNumInfo::output> &maxDPBNumInfo) {
    (void)mayBlock;

    if (decodeOrder.v.value == Off) {
        me.set().value = 0;
    } else {
        /* in case of big resolution, reduce reorder depth for memory resource */
        if ((streamSize.v.width * streamSize.v.height) > UHD_IMAGE_SIZE) {
            me.set().value = c2_max(((int32_t)maxDPBNumInfo.v.value - EXTRA_DPB), 0);
        }
    }

    StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] reorder depth(%d)", __FUNCTION__, me.v.value);

    return C2R::Ok();
}

C2R DecCommonSetter::ReorderKeySetter(bool mayBlock, C2InterfaceHelper::C2P<C2PortReorderKeySetting::output> &me,
                            const C2InterfaceHelper::C2P<C2ExynosPortReorderTimestampSetting::output> &reorderTimestamp) {
    (void)mayBlock;

    if (reorderTimestamp.v.value == On) {
        /* reorder timestamp := display order decoding + custom key */
        me.set().value = C2Config::ordinal_key_t::CUSTOM;
    }

    StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] ordinal key(0x%x)", __FUNCTION__,
                        me.v.value);

    return C2R::Ok();
}

C2R DecCommonSetter::ReorderKeySetterWithDecodeOrder(bool mayBlock, C2InterfaceHelper::C2P<C2PortReorderKeySetting::output> &me,
                                           const C2InterfaceHelper::C2P<C2ExynosPortReorderTimestampSetting::output> &reorderTimestamp,
                                           const C2InterfaceHelper::C2P<C2ExynosPortDecodeOrderSetting::output> &decodeOrder) {
    (void)mayBlock;

    if (reorderTimestamp.v.value == On) {
        /* reorder timestamp := display order decoding + custom key */
        me.set().value = C2Config::ordinal_key_t::CUSTOM;
    } else if ((decodeOrder.v.value == On) &&
               (me.v.value == C2Config::ordinal_key_t::ORDINAL)) {
        /*
         * ORDINAL couldn't support reordering timestamp as well
         * uses CUSTOM as default.
         */
        me.set().value = C2Config::ordinal_key_t::CUSTOM;
    }

    StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] ordinal key(0x%x)", __FUNCTION__,
                        me.v.value);

    return C2R::Ok();
}

C2R DecCommonSetter::BufferCopySetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosPortCSCBufferCopySetting::output> &me) {
    (void)mayBlock;

    StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] buffer copy is %s", __FUNCTION__,
                        (me.v.value == On)? "enabled":"disabled");

    return C2R::Ok();
}

C2R DecCommonSetter::BlackBarSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosPortBlackBarSetting::output> &me) {
    (void)mayBlock;

    StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] black bar is %s", __FUNCTION__,
                        (me.v.value == On)? "enabled":"disabled");

    return C2R::Ok();
}

C2R DecCommonSetter::CompressedColorSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosPortCompressedColorSetting::output> &me,
                                const C2InterfaceHelper::C2P<C2ExynosStreamOutputStreamSetting::output> &streamUsage) {
    (void)mayBlock;

    if (streamUsage.v.value & C2MemoryUsage::CPU_READ) {
        me.set().value = VendorC2Config::COMPRESSED_COLOR_NONE;
    }

    StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] compressed color : 0x%x", __FUNCTION__, me.v.value);

    return C2R::Ok();
}

C2R DecCommonSetter::CodedHdrDynamicInfoSetter(bool mayBlock,
                                        const C2InterfaceHelper::C2P<C2StreamHdrDynamicMetadataInfo::output> &oldMe,
                                        C2InterfaceHelper::C2P<C2StreamHdrDynamicMetadataInfo::output> &me) {
    (void)mayBlock;

    if ((oldMe.v.flexCount() == me.v.flexCount()) &&
        (!memcmp(oldMe.v.m.data, me.v.m.data, me.v.flexCount()))) {
        /* no changes */
        return C2R::Corrupted();
    }

    return C2R::Ok();
}

C2R DecCommonSetter::MaxPictureSizeSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamMaxPictureSizeTuning::output> &me,
                                const C2InterfaceHelper::C2P<C2StreamPictureSizeInfo::output> &size,
                                uint32_t maxWidth, uint32_t maxHeight) {
    (void)mayBlock;

    // TODO: get max width/height from the size's field helpers vs. hardcoding
    me.set().width = ALIGN(c2_min(c2_max(me.v.width, size.v.width), maxWidth), 2);
    me.set().height = ALIGN(c2_min(c2_max(me.v.height, size.v.height), maxHeight), 2);

    StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] max size(%d x %d)", __FUNCTION__, me.v.width, me.v.height);

    return C2R::Ok();
}

C2R DecCommonSetter::CodedHdrStaticValidationInfoSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosStreamHdrStaticValidationInfo::output> &me,
                                                          const C2InterfaceHelper::C2P<C2StreamHdrStaticInfo::output> &hdrStatic) {
    (void)mayBlock;
    (void)hdrStatic;

    /* when C2StreamHdrStaticInfo is changed, always set C2_TRUE
     * on getter, this will be changed to C2_FALSE.
     */
    me.set().value = C2_TRUE;

    return C2R::Ok();
}

C2R DecCommonSetter::CodedHdrStaticInfoSetter(bool mayBlock, const C2InterfaceHelper::C2P<C2StreamHdrStaticInfo::output> &oldMe,
                                              C2InterfaceHelper::C2P<C2StreamHdrStaticInfo::output> &me) {
    (void)mayBlock;

    if (!memcmp(&(oldMe.v), &(me.v), sizeof(struct C2HdrStaticMetadataStruct))) {
        StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] not changed", __FUNCTION__);
    } else {
        StaticExynosLog(Level::Info, "VdecCommonParamIntf", "[%s] HDR :: static info[framework] r(%f, %f), g(%f, %f), b(%f, %f), w(%f, %f), max(%f), min(%f), cll(%f), fall(%f)", __FUNCTION__,
            me.v.mastering.red.x, me.v.mastering.red.y,
            me.v.mastering.green.x, me.v.mastering.green.y,
            me.v.mastering.blue.x, me.v.mastering.blue.y,
            me.v.mastering.white.x, me.v.mastering.white.y,
            me.v.mastering.maxLuminance, me.v.mastering.minLuminance,
            me.v.maxCll, me.v.maxFall);
    }

    return C2R::Ok();
}

C2R EncCommonSetter::SizeSetter(bool mayBlock,
                      const C2InterfaceHelper::C2P<C2StreamPictureSizeInfo::input> &oldMe,
                      C2InterfaceHelper::C2P<C2StreamPictureSizeInfo::input> &me) {
    (void)mayBlock;

    C2R res = C2R::Ok();
    if (!me.F(me.v.width).supportsAtAll(me.v.width)) {
        res = res.plus(C2SettingResultBuilder::BadValue(me.F(me.v.width)));

        StaticExynosLog(Level::Error, "VencCommonParamIntf", "[%s] width(%d) is invalid", __FUNCTION__, me.v.width);
        me.set().width = oldMe.v.width;
    }
    if (!me.F(me.v.height).supportsAtAll(me.v.height)) {
        res = res.plus(C2SettingResultBuilder::BadValue(me.F(me.v.height)));

        StaticExynosLog(Level::Error, "VencCommonParamIntf", "[%s] height(%d) is invalid", __FUNCTION__, me.v.height);
        me.set().height = oldMe.v.height;
    }

    return res;
}

C2R EncCommonSetter::PixelFormatSetter(bool mayBlock,
                                       const C2InterfaceHelper::C2P<C2StreamPixelFormatInfo::input> &old,
                                       C2InterfaceHelper::C2P<C2StreamPixelFormatInfo::input> &me) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    if (!me.F(me.v.value).supportsNow(me.v.value)) {
        StaticExynosLog(Level::Error, "VencCommonParamIntf", "[%s] input format:0x%x is not supported", __FUNCTION__, me.v.value);
        me.set().value = old.v.value;
        res = C2SettingResultBuilder::BadValue(me.F(me.v.value));
    }

    StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] input format:0x%x", __FUNCTION__, me.v.value);

    return res;
}

C2R EncCommonSetter::ActualFormatSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosCSCOutputPixelFormatInfo> &me,
                                            const C2InterfaceHelper::C2P<C2StreamPixelFormatInfo::input> &pixelFormat,
                                            const C2InterfaceHelper::C2P<C2ExynosStreamCSCDataSpaceInfo::input> &cscDataSpace,
                                            const C2InterfaceHelper::C2P<C2Exynos10BitSupportInfo> &support10Bit) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    auto format = pixelFormat.v.value;

    if ((format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED) ||
        (format == HAL_PIXEL_FORMAT_YCBCR_420_888)) {
        /* it is kind of opaque format
         * need to check a pixel format on buffer */
        format = me.v.value;
    }

    switch (format) {
    case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
        [[fallthrough]];
#ifdef SUPPORT_MFC_ENC_RGB  /* it depends on H/W */
    case HAL_PIXEL_FORMAT_RGBA_8888:
        [[fallthrough]];
#endif
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_256_SBWC:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L40:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L60:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L80:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_10B_32_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_10B_64_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SPN_10B_32_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SPN_10B_64_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_256_SBWC:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L50:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L75:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_32_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_64_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SPN_32_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SPN_64_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_SBWC:
        /* no need to color conversion */
        me.set().value = format;
        break;
    case HAL_PIXEL_FORMAT_YV12:
        me.set().value = HAL_PIXEL_FORMAT_EXYNOS_YV12_M;  /* need to changes for H/W constraints */
        break;
    case HAL_PIXEL_FORMAT_YCBCR_P010:
        if (support10Bit.v.value) {
            me.set().value = HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M;
#ifdef USE_FLEXIBLE_P010
            me.set().value = HAL_PIXEL_FORMAT_YCBCR_P010;
#endif
        } else {
            me.set().value = HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M;
        }
        break;
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
        me.set().value = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M;  /* need to changes for H/W constraints */
        break;
    case HAL_PIXEL_FORMAT_RGBA_1010102:
        me.set().value = format;  /* no need to color conversion */

#ifndef SUPPORT_MFC_ENC_10B_RGB
        me.set().value = (support10Bit.v.value)? HAL_PIXEL_FORMAT_YCBCR_P010:HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M;
#endif
        break;
    default:
        me.set().value = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M;  /* need to color conversion */
        break;
    }

    StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] CSC's output format:0x%x, dataspace:0x%x", __FUNCTION__,
                        me.v.value, cscDataSpace.v.value);

    return res;
}

C2R EncCommonSetter::BitrateModeSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamBitrateModeTuning::output>& me) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] bitrate mode : 0x%x", __FUNCTION__, me.v.value);

    return res;
}

C2R EncCommonSetter::VendorBitrateModeSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosStreamBitrateModeTuning::input>& me) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    if ((uint32_t)VendorC2Config::BITRATE_VENDOR_START <= (uint32_t)me.v.value) {
        StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] final bitrate mode : 0x%x", __FUNCTION__, me.v.value);
    }

    return res;
}

C2R EncCommonSetter::IntraRefreshSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamIntraRefreshTuning::output> &me) {
    (void)mayBlock;
    C2R res = C2R::Ok();
    if (me.v.period < 1) {
        me.set().mode = C2Config::INTRA_REFRESH_DISABLED;
        me.set().period = 0;
    } else {
        me.set().mode = C2Config::INTRA_REFRESH_ARBITRARY;
    }

    StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] intra refresh: mode(0x%x), period(%d)", __FUNCTION__,
                    me.v.mode, me.v.period);

    return res;
}

C2R EncCommonSetter::DataSpaceSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamDataSpaceInfo::input> &me) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] dataspace(0x%x)", __FUNCTION__, me.v.value);

    return res;
}

C2R EncCommonSetter::CSCDataSpaceSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosStreamCSCDataSpaceInfo::input> &me,
                                                const C2InterfaceHelper::C2P<C2StreamColorAspectsInfo::input> &ca) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    uint32_t dataspace = HAL_DATASPACE_UNKNOWN;

    android::C2Mapper::map(ca.v.range, ca.v.primaries, ca.v.matrix, ca.v.transfer, &dataspace);

    me.set().value = dataspace;

    StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] csc dataspace(0x%x)", __FUNCTION__, me.v.value);

    return res;
}

C2R EncCommonSetter::ColorAspectsSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamColorAspectsInfo::input> &me,
                              const C2InterfaceHelper::C2P<C2StreamDataSpaceInfo::input> &dataspace) {
    (void)mayBlock;
    if (me.v.range > C2Color::RANGE_OTHER) {
        me.set().range = C2Color::RANGE_OTHER;
    }
    if (me.v.primaries > C2Color::PRIMARIES_OTHER) {
        me.set().primaries = C2Color::PRIMARIES_OTHER;
    }
    if (me.v.transfer > C2Color::TRANSFER_OTHER) {
        me.set().transfer = C2Color::TRANSFER_OTHER;
    }
    if (me.v.matrix > C2Color::MATRIX_OTHER) {
        me.set().matrix = C2Color::MATRIX_OTHER;
    }

    C2Color::range_t        range       = me.v.range;
    C2Color::primaries_t    primaries   = me.v.primaries;
    C2Color::transfer_t     transfer    = me.v.transfer;
    C2Color::matrix_t       matrix      = me.v.matrix;

    switch (dataspace.v.value) {
    case HAL_DATASPACE_SRGB_LINEAR:
        [[fallthrough]];
    case HAL_DATASPACE_V0_SRGB_LINEAR:
        range       = C2Color::RANGE_FULL;
        primaries   = C2Color::PRIMARIES_BT709;
        transfer    = C2Color::TRANSFER_LINEAR;
        matrix      = C2Color::MATRIX_BT709;
        break;
    case HAL_DATASPACE_SRGB:
        [[fallthrough]];
    case HAL_DATASPACE_V0_SRGB:
        range       = C2Color::RANGE_FULL;
        primaries   = C2Color::PRIMARIES_BT709;
        transfer    = C2Color::TRANSFER_SRGB;
        matrix      = C2Color::MATRIX_BT709;
        break;
    case HAL_DATASPACE_JFIF:
        [[fallthrough]];
    case HAL_DATASPACE_V0_JFIF:
        range       = C2Color::RANGE_FULL;
        primaries   = C2Color::PRIMARIES_BT601_625;
        transfer    = C2Color::TRANSFER_170M;
        matrix      = C2Color::MATRIX_BT601;
        break;
    case HAL_DATASPACE_BT601_625:
        [[fallthrough]];
    case HAL_DATASPACE_V0_BT601_625:
        range       = C2Color::RANGE_LIMITED;
        primaries   = C2Color::PRIMARIES_BT601_625;
        transfer    = C2Color::TRANSFER_170M;
        matrix      = C2Color::MATRIX_BT601;
        break;
    case HAL_DATASPACE_BT601_525:
        [[fallthrough]];
    case HAL_DATASPACE_V0_BT601_525:
        range       = C2Color::RANGE_LIMITED;
        primaries   = C2Color::PRIMARIES_BT601_525;
        transfer    = C2Color::TRANSFER_170M;
        matrix      = C2Color::MATRIX_BT601;
        break;
    case HAL_DATASPACE_BT709:
        [[fallthrough]];
    case HAL_DATASPACE_V0_BT709:
        range       = C2Color::RANGE_LIMITED;
        primaries   = C2Color::PRIMARIES_BT709;
        transfer    = C2Color::TRANSFER_170M;
        matrix      = C2Color::MATRIX_BT709;
        break;
    case (HAL_DATASPACE_STANDARD_BT709 | HAL_DATASPACE_TRANSFER_SMPTE_170M | HAL_DATASPACE_RANGE_FULL):
        range       = C2Color::RANGE_FULL;
        primaries   = C2Color::PRIMARIES_BT709;
        transfer    = C2Color::TRANSFER_170M;
        matrix      = C2Color::MATRIX_BT709;
        break;
    case HAL_DATASPACE_BT2020:
        range       = C2Color::RANGE_FULL;
        primaries   = C2Color::PRIMARIES_BT2020;
        transfer    = C2Color::TRANSFER_170M;
        matrix      = C2Color::MATRIX_BT2020;
        break;
    case HAL_DATASPACE_BT2020_PQ:
        range       = C2Color::RANGE_FULL;
        primaries   = C2Color::PRIMARIES_BT2020;
        transfer    = C2Color::TRANSFER_ST2084;
        matrix      = C2Color::MATRIX_BT2020;
        break;
    case HAL_DATASPACE_BT2020_ITU:
        range       = C2Color::RANGE_LIMITED;
        primaries   = C2Color::PRIMARIES_BT2020;
        transfer    = C2Color::TRANSFER_170M;
        matrix      = C2Color::MATRIX_BT2020;
        break;
    case HAL_DATASPACE_BT2020_ITU_PQ:
        range       = C2Color::RANGE_LIMITED;
        primaries   = C2Color::PRIMARIES_BT2020;
        transfer    = C2Color::TRANSFER_ST2084;
        matrix      = C2Color::MATRIX_BT2020;
        break;
    case HAL_DATASPACE_BT2020_ITU_HLG:
        range       = C2Color::RANGE_LIMITED;
        primaries   = C2Color::PRIMARIES_BT2020;
        transfer    = C2Color::TRANSFER_HLG;
        matrix      = C2Color::MATRIX_BT2020;
        break;
    case HAL_DATASPACE_BT2020_HLG:
        range       = C2Color::RANGE_FULL;
        primaries   = C2Color::PRIMARIES_BT2020;
        transfer    = C2Color::TRANSFER_HLG;
        matrix      = C2Color::MATRIX_BT2020;
        break;
    default:
        break;
    }

    me.set().range      = range;
    me.set().primaries  = primaries;
    me.set().transfer   = transfer;
    me.set().matrix     = matrix;

    StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] r(%d), p(0x%x), t(0x%x), m(0x%x)", __FUNCTION__,
                        me.v.range, me.v.primaries, me.v.transfer, me.v.matrix);
    return C2R::Ok();
}

C2R EncCommonSetter::PictureQuantizationSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamPictureQuantizationTuning::output> &me,
                                                const int32_t minQp, const int32_t maxQp) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    for (size_t i = 0; i < me.v.flexCount(); ++i) {
        const C2PictureQuantizationStruct &layer = me.v.m.values[i];

        StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] request: min(%d), max(%d) vs support: min(%d), max(%d)",
                            __FUNCTION__, layer.min, layer.max, minQp, maxQp);

        auto max = std::clamp(layer.max, minQp, maxQp);

        me.set().m.values[i].min = std::clamp(layer.min, minQp, max);
        me.set().m.values[i].max = std::clamp(layer.max, me.v.m.values[i].min, max);

        std::string type("unknown frame");
        switch (layer.type_) {
        case C2Config::picture_type_t(I_FRAME):
            type = std::string("I frame");
            break;
        case C2Config::picture_type_t(P_FRAME):
            type = std::string("P frame");
            break;
        case C2Config::picture_type_t(B_FRAME):
            type = std::string("B frame");
            break;
        default:
            break;
        }

        StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] final: %s: min(%d), max(%d)", __FUNCTION__,
                            type.c_str(), me.v.m.values[i].min, me.v.m.values[i].max);
    }

    return res;
}

C2R EncCommonSetter::QpRangeSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosPortQpRangeTuning::output> &me,
                                const C2InterfaceHelper::C2P<C2StreamPictureQuantizationTuning::output> &quantization) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    me.set().I_maxQP = c2_max(me.v.I_minQP, me.v.I_maxQP);
    me.set().P_maxQP = c2_max(me.v.P_minQP, me.v.P_maxQP);
    me.set().B_maxQP = c2_max(me.v.B_minQP, me.v.B_maxQP);

    for (size_t i = 0; i < quantization.v.flexCount(); ++i) {
        const C2PictureQuantizationStruct &layer = quantization.v.m.values[i];

        switch (layer.type_) {
        case C2Config::picture_type_t(I_FRAME):
            me.set().I_minQP = quantization.v.m.values[i].min;
            me.set().I_maxQP = quantization.v.m.values[i].max;
            break;
        case C2Config::picture_type_t(P_FRAME):
            me.set().P_minQP = quantization.v.m.values[i].min;
            me.set().P_maxQP = quantization.v.m.values[i].max;
            break;
        case C2Config::picture_type_t(B_FRAME):
            me.set().B_minQP = quantization.v.m.values[i].min;
            me.set().B_maxQP = quantization.v.m.values[i].max;
            break;
        default:
            break;
        }
    }

    StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] QP range I(%d, %d)", __FUNCTION__,
                        me.v.I_minQP, me.v.I_maxQP);
    StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] QP range P(%d, %d)", __FUNCTION__,
                        me.v.P_minQP, me.v.P_maxQP);
    StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] QP range B(%d, %d)", __FUNCTION__,
                        me.v.B_minQP, me.v.B_maxQP);

    return res;
}

C2R EncCommonSetter::LayeringSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamTemporalLayeringTuning::output>& me) {
    (void)mayBlock;
    C2R res = C2R::Ok();
    if (me.v.m.layerCount > MAX_TEMPORAL_LAYERS) {
        me.set().m.layerCount = MAX_TEMPORAL_LAYERS;
    }

    if (me.v.m.bLayerCount > 0) {
        me.set().m.layerCount = me.v.m.bLayerCount + 1;
    }

    if (me.v.m.layerCount > 0) {
        // ensure ratios are monotonic and clamped between 0 and 1
        for (size_t ix = 0; ix < me.v.flexCount(); ++ix) {
            me.set().m.bitrateRatios[ix] = c2_clamp(
                ((ix > 0)? me.v.m.bitrateRatios[ix - 1]:0), me.v.m.bitrateRatios[ix], 1.);
        }

        StaticExynosLog(Level::Debug, "EncParamIntf", "[%s] setting temporal layering %u + %u", __FUNCTION__, me.v.m.layerCount, me.v.m.bLayerCount);
    }

    return res;
}

C2R EncCommonSetter::LayeringSetterVpx(bool mayBlock, C2InterfaceHelper::C2P<C2StreamTemporalLayeringTuning::output>& me) {
    (void)mayBlock;
    C2R res = C2R::Ok();
    if (me.v.m.layerCount > MAX_VPX_TEMPORAL_LAYERS) {
        me.set().m.layerCount = MAX_VPX_TEMPORAL_LAYERS;
    }
    me.set().m.bLayerCount = 0;

    if (me.v.m.layerCount > 0) {
        // ensure ratios are monotonic and clamped between 0 and 1
        for (size_t ix = 0; ix < me.v.flexCount(); ++ix) {
            me.set().m.bitrateRatios[ix] = c2_clamp(
                ((ix > 0)? me.v.m.bitrateRatios[ix - 1]:0), me.v.m.bitrateRatios[ix], 1.);
        }

        StaticExynosLog(Level::Debug, "VpxEncParamIntf", "[%s] setting temporal layering %u + %u", __FUNCTION__, me.v.m.layerCount, me.v.m.bLayerCount);
    }

    return res;
}

C2R EncCommonSetter::MinQualitySetter(bool mayBlock, const C2InterfaceHelper::C2P<C2EncodingQualityLevel> &old,
                                        C2InterfaceHelper::C2P<C2EncodingQualityLevel> &me) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    if (!me.F(me.v.value).supportsNow(me.v.value)) {
        me.set().value = old.v.value;
    }

    auto level = ExynosUtils::GetMinQuality();
    if (level > 0) {
        me.set().value = level;
    }

    StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] quality level : %d", __FUNCTION__, me.v.value);

    return res;
}

C2R EncCommonSetter::PMVSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosStreamPMVTuning::output> &me) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] PMV mode is %d", __FUNCTION__, me.v.Mode);

    if (me.v.Mode == VendorC2Config::PMV_GLOBAL) {
        StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] Global PMV horizon(L0:%d, L1:%d)", __FUNCTION__,
                            me.v.HorizonL0, me.v.HorizonL1);
        StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] Global PMV vertical(L0:%d, L1:%d)", __FUNCTION__,
                            me.v.VerticalL0, me.v.VerticalL1);
    } else {
        me.set().HorizonL0  = 0;
        me.set().HorizonL1  = 0;
        me.set().VerticalL0 = 0;
        me.set().VerticalL1 = 0;
    }

    return res;
}

C2R EncCommonSetter::GopSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamGopTuning::output> &me, uint32_t maxBCount) {
    (void)mayBlock;
    for (size_t i = 0; i < me.v.flexCount(); ++i) {
        const C2GopLayerStruct &layer = me.v.m.values[i];
        if (layer.type_ == C2Config::picture_type_t(P_FRAME | B_FRAME)
                && layer.count > maxBCount) {
            me.set().m.values[i].count = maxBCount;
        }

        StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] GOP layer[%d]: type(0x%x), count(%d)", __FUNCTION__,
                        i, layer.type_, layer.count);
    }

    return C2R::Ok();
}

C2R EncCommonSetter::ChromaQpOffsetSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosStreamChromaQpOffsetSetting::output>& me) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] chroma qp offset(cb:%d, cr:%d)",
                        __FUNCTION__, me.v.Cb, me.v.Cr);
    return res;
}

C2R EncCommonSetter::RefPframesSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosPortRefPframeSetting::output>& me) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] Ref. P frame(%d)", __FUNCTION__, me.v.value);
    return res;
}

C2R EncCommonSetter::HdrFormatSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamHdrFormatInfo::output> &me) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] HDR format:0x%x", __FUNCTION__, me.v.value);

    return res;
}

C2R EncCommonSetter::HdrStaticInfoSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamHdrStaticInfo::input> &me) {
    (void)mayBlock;
    (void)me;  // TODO : validate

    return C2R::Ok();
}

C2R EncCommonSetter::HdrDynamicInfoInputSetter(bool mayBlock,
                                               C2InterfaceHelper::C2P<C2StreamHdrDynamicMetadataInfo::input> &me) {
    (void)mayBlock;
    (void)me;  // TODO: validate

    StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] changed", __FUNCTION__);

    return C2R::Ok();
}

C2R EncCommonSetter::HdrDynamicInfoOutputSetter(bool mayBlock,
                                                C2InterfaceHelper::C2P<C2StreamHdrDynamicMetadataInfo::output> &me) {
    (void)mayBlock;
    (void)me;  // TODO: validate

    StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] applied", __FUNCTION__);

    return C2R::Ok();
}
