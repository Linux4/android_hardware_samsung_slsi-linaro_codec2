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
#ifndef EXYNOS_C2_INTFSETTER_H
#define EXYNOS_C2_INTFSETTER_H

#include <C2Config.h>
#include <util/C2InterfaceHelper.h>
#include <system/graphics.h>

#include "exynos_format.h"
#include "Exynos_C2_Utils.h"


/* setter functions */
/* Common setter */
namespace CommonSetter {
    C2R SubscribedParamIndicesSetter(bool mayBlock, C2InterfaceHelper::C2P<C2SubscribedParamIndicesTuning> &me);
    C2R OperateRateSetter(bool mayBlock, C2InterfaceHelper::C2P<C2OperatingRateTuning> &me);
    C2R RealTimePrioritySetter(bool mayBlock, C2InterfaceHelper::C2P<C2RealTimePriorityTuning> &me);
}; // namespace CommonSetter

/* Decode common setter */
namespace DecCommonSetter {
    C2R UsageSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamUsageTuning::output> &me);
    C2R StreamUsageSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosStreamOutputStreamSetting::output> &me,
                                 const C2InterfaceHelper::C2P<C2StreamPixelFormatInfo::output> &pixelFormat,
                                 const C2InterfaceHelper::C2P<C2StreamUsageTuning::output> &usage);
    C2R SizeSetter(bool mayBlock, const C2InterfaceHelper::C2P<C2StreamPictureSizeInfo::output> &oldMe,
                          C2InterfaceHelper::C2P<C2StreamPictureSizeInfo::output> &me);
    C2R MaxInputSizeSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamMaxBufferSizeInfo::input> &me,
                                  const C2InterfaceHelper::C2P<C2StreamMaxPictureSizeTuning::output> &maxSize);
    C2R MaxInputSizeSetterForSecure(bool mayBlock, C2InterfaceHelper::C2P<C2StreamMaxBufferSizeInfo::input> &me,
                                        const C2InterfaceHelper::C2P<C2StreamMaxPictureSizeTuning::output> &maxSize);
    C2R ActualOutputDelaySetter(bool mayBlock, C2InterfaceHelper::C2P<C2PortActualDelayTuning::output> &me,
                                       const C2InterfaceHelper::C2P<C2ExynosStreamMaxDPBNumInfo::output> &maxDPBNumInfo);
    C2R ActualOutputDelaySetterWithExtra(bool mayBlock, C2InterfaceHelper::C2P<C2PortActualDelayTuning::output> &me,
                                                const C2InterfaceHelper::C2P<C2ExynosStreamMaxDPBNumInfo::output> &maxDPBNumInfo,
                                                const C2InterfaceHelper::C2P<C2ExynosPortExtraBufferNumInfo::output> &extBufferNumInfo);
    C2R PixelFormatSetter(bool mayBlock, const C2InterfaceHelper::C2P<C2StreamPixelFormatInfo::output> &old, C2InterfaceHelper::C2P<C2StreamPixelFormatInfo::output> &me);
    C2R ActualFormatSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosCSCOutputPixelFormatInfo> &me,
                                                const C2InterfaceHelper::C2P<C2StreamPixelFormatInfo::output> &pixelFormat);
    C2R DefaultColorAspectsSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamColorAspectsTuning::output> &me);
    C2R CodedColorAspectsSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamColorAspectsInfo::input> &me);
    C2R ColorAspectsSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamColorAspectsInfo::output> &me,
                                  const C2InterfaceHelper::C2P<C2StreamColorAspectsTuning::output> &def,
                                  const C2InterfaceHelper::C2P<C2StreamColorAspectsInfo::input> &coded);
    C2R ReorderTimestampSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosPortReorderTimestampSetting::output> &me);
    C2R DecodeOrderSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosPortDecodeOrderSetting::output> &me,
                                 const C2InterfaceHelper::C2P<C2ExynosPortReorderTimestampSetting::output> &reorderTimestamp);
    C2R DecodeOrderSetterWithSize(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosPortDecodeOrderSetting::output> &me,
                                         const C2InterfaceHelper::C2P<C2ExynosPortReorderTimestampSetting::output> &reorderTimestamp,
                                         const C2InterfaceHelper::C2P<C2StreamPictureSizeInfo::output> &streamSize);
    C2R ReorderDepthSetter(bool mayBlock, C2InterfaceHelper::C2P<C2PortReorderBufferDepthTuning::output> &me,
                                  const C2InterfaceHelper::C2P<C2StreamPictureSizeInfo::output> &streamSize,
                                  const C2InterfaceHelper::C2P<C2ExynosPortDecodeOrderSetting::output> &decodeOrder,
                                  const C2InterfaceHelper::C2P<C2ExynosStreamMaxDPBNumInfo::output> &maxDPBNumInfo);
    C2R ReorderKeySetter(bool mayBlock, C2InterfaceHelper::C2P<C2PortReorderKeySetting::output> &me,
                                const C2InterfaceHelper::C2P<C2ExynosPortReorderTimestampSetting::output> &reorderTimestamp);
    C2R ReorderKeySetterWithDecodeOrder(bool mayBlock, C2InterfaceHelper::C2P<C2PortReorderKeySetting::output> &me,
                                               const C2InterfaceHelper::C2P<C2ExynosPortReorderTimestampSetting::output> &reorderTimestamp,
                                               const C2InterfaceHelper::C2P<C2ExynosPortDecodeOrderSetting::output> &decodeOrder);
    C2R BufferCopySetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosPortCSCBufferCopySetting::output> &me);
    C2R BlackBarSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosPortBlackBarSetting::output> &me);
    C2R CompressedColorSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosPortCompressedColorSetting::output> &me,
                                    const C2InterfaceHelper::C2P<C2ExynosStreamOutputStreamSetting::output> &streamUsage);
    C2R MaxPictureSizeSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamMaxPictureSizeTuning::output> &me,
                                    const C2InterfaceHelper::C2P<C2StreamPictureSizeInfo::output> &size,
                                    uint32_t maxWidth, uint32_t maxHeight);
    C2R CodedHdrDynamicInfoSetter(bool mayBlock, const C2InterfaceHelper::C2P<C2StreamHdrDynamicMetadataInfo::output> &oldMe,
                                        C2InterfaceHelper::C2P<C2StreamHdrDynamicMetadataInfo::output> &me);
    C2R CodedHdrStaticValidationInfoSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosStreamHdrStaticValidationInfo::output> &me,
                                            const C2InterfaceHelper::C2P<C2StreamHdrStaticInfo::output> &hdrStatic);
    C2R CodedHdrStaticInfoSetter(bool mayBlock, const C2InterfaceHelper::C2P<C2StreamHdrStaticInfo::output> &oldMe,
                                    C2InterfaceHelper::C2P<C2StreamHdrStaticInfo::output> &me);

    template <class T>
    C2R BaseColorAspectsSetter(bool mayBlock, C2InterfaceHelper::C2P<T> &me) {
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

        StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] r(%d), p(0x%x), t(0x%x), m(0x%x)", __FUNCTION__,
                            me.v.range, me.v.primaries, me.v.transfer, me.v.matrix);
        return C2R::Ok();
    }

    template <class T>
    C2R CropRectSetter(bool mayBlock, C2InterfaceHelper::C2P<T> &me) {
        (void)mayBlock;
        // TODO: maybe apply block limit?
        C2R res = C2R::Ok();

        if (!me.F(me.v.width).supportsAtAll(me.v.width)) {
            res = res.plus(C2SettingResultBuilder::BadValue(me.F(me.v.width)));
            StaticExynosLog(Level::Error, "VdecCommonParamIntf", "[%s] width(%d) is invalid", __FUNCTION__, me.v.width);
        }

        if (!me.F(me.v.height).supportsAtAll(me.v.height)) {
            res = res.plus(C2SettingResultBuilder::BadValue(me.F(me.v.height)));
            StaticExynosLog(Level::Error, "VdecCommonParamIntf", "[%s] height(%d) is invalid", __FUNCTION__, me.v.height);
        }

        if (!me.F(me.v.left).supportsAtAll(me.v.left)) {
            res = res.plus(C2SettingResultBuilder::BadValue(me.F(me.v.left)));
            StaticExynosLog(Level::Error, "VdecCommonParamIntf", "[%s] left(%d) is invalid", __FUNCTION__, me.v.left);
        }

        if (!me.F(me.v.top).supportsAtAll(me.v.top)) {
            res = res.plus(C2SettingResultBuilder::BadValue(me.F(me.v.top)));
            StaticExynosLog(Level::Error, "VdecCommonParamIntf", "[%s] top(%d) is invalid", __FUNCTION__, me.v.top);
        }

        return res;
    }

    template <class T, class U>
    C2R CropRectSetter(bool mayBlock, C2InterfaceHelper::C2P<T> &me, const C2InterfaceHelper::C2P<U> &size) {
        (void)mayBlock;
        // TODO: maybe apply block limit?
        C2R res = C2R::Ok();

        if ((!me.F(me.v.width).supportsAtAll(me.v.width)) ||
            (me.v.width > size.v.width)) {
            res = res.plus(C2SettingResultBuilder::BadValue(me.F(me.v.width)));
            StaticExynosLog(Level::Error, "VdecCommonParamIntf", "[%s] width(%d) is invalid", __FUNCTION__, me.v.width);
        }

        if ((!me.F(me.v.height).supportsAtAll(me.v.height)) ||
            (me.v.height > size.v.height)) {
            res = res.plus(C2SettingResultBuilder::BadValue(me.F(me.v.height)));
            StaticExynosLog(Level::Error, "VdecCommonParamIntf", "[%s] height(%d) is invalid", __FUNCTION__, me.v.height);
        }

        if ((!me.F(me.v.left).supportsAtAll(me.v.left)) ||
            (me.v.left > size.v.width)) {
            res = res.plus(C2SettingResultBuilder::BadValue(me.F(me.v.left)));
            StaticExynosLog(Level::Error, "VdecCommonParamIntf", "[%s] left(%d) is invalid", __FUNCTION__, me.v.left);
        }

        if ((!me.F(me.v.top).supportsAtAll(me.v.top)) ||
            (me.v.top > size.v.height)) {
            res = res.plus(C2SettingResultBuilder::BadValue(me.F(me.v.top)));
            StaticExynosLog(Level::Error, "VdecCommonParamIntf", "[%s] top(%d) is invalid", __FUNCTION__, me.v.top);
        }

        return res;
    }

    template <class T>
    C2R ScaleSizeSetter(bool mayBlock, C2InterfaceHelper::C2P<T> &me) {
        (void)mayBlock;

        C2R res = C2R::Ok();

        if (!me.F(me.v.width).supportsAtAll(me.v.width)) {
            res = res.plus(C2SettingResultBuilder::BadValue(me.F(me.v.width)));
            StaticExynosLog(Level::Error, "VdecCommonParamIntf", "[%s] width(%d) is invalid", __FUNCTION__, me.v.width);
        }

        if (!me.F(me.v.height).supportsAtAll(me.v.height)) {
            res = res.plus(C2SettingResultBuilder::BadValue(me.F(me.v.height)));
            StaticExynosLog(Level::Error, "VdecCommonParamIntf", "[%s] height(%d) is invalid", __FUNCTION__, me.v.height);
        }

        StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] scale size(%d x %d)", __FUNCTION__, me.v.width, me.v.height);

        return res;
    }
}; // namespace DecCommonSetter


/* Encode common setter */
namespace EncCommonSetter {
    C2R SizeSetter(bool mayBlock,
                          const C2InterfaceHelper::C2P<C2StreamPictureSizeInfo::input> &oldMe,
                          C2InterfaceHelper::C2P<C2StreamPictureSizeInfo::input> &me);
    C2R PixelFormatSetter(bool mayBlock, const C2InterfaceHelper::C2P<C2StreamPixelFormatInfo::input> &old,
                                                    C2InterfaceHelper::C2P<C2StreamPixelFormatInfo::input> &me);
    C2R ActualFormatSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosCSCOutputPixelFormatInfo> &me,
                                                const C2InterfaceHelper::C2P<C2StreamPixelFormatInfo::input> &pixelFormat,
                                                const C2InterfaceHelper::C2P<C2ExynosStreamCSCDataSpaceInfo::input> &cscDataSpace,
                                                const C2InterfaceHelper::C2P<C2Exynos10BitSupportInfo> &support10Bit);
    C2R BitrateModeSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamBitrateModeTuning::output>& me);
    C2R VendorBitrateModeSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosStreamBitrateModeTuning::input>& me);
    C2R IntraRefreshSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamIntraRefreshTuning::output> &me);
    C2R DataSpaceSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamDataSpaceInfo::input> &me);
    C2R CSCDataSpaceSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosStreamCSCDataSpaceInfo::input> &me,
                                    const C2InterfaceHelper::C2P<C2StreamColorAspectsInfo::input> &ca);
    C2R ColorAspectsSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamColorAspectsInfo::input> &me,
                                  const C2InterfaceHelper::C2P<C2StreamDataSpaceInfo::input> &dataspace);
    C2R PictureQuantizationSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamPictureQuantizationTuning::output> &me, const int32_t minQp, const int32_t maxQp);
    C2R QpRangeSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosPortQpRangeTuning::output> &me,
                                const C2InterfaceHelper::C2P<C2StreamPictureQuantizationTuning::output> &quantization);
    C2R LayeringSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamTemporalLayeringTuning::output>& me);
    C2R LayeringSetterVpx(bool mayBlock, C2InterfaceHelper::C2P<C2StreamTemporalLayeringTuning::output>& me);
    C2R MinQualitySetter(bool mayBlock, const C2InterfaceHelper::C2P<C2EncodingQualityLevel> &old,
                                        C2InterfaceHelper::C2P<C2EncodingQualityLevel> &me);
    C2R PMVSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosStreamPMVTuning::output> &me);
    C2R GopSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamGopTuning::output> &me, uint32_t maxBCount);
    C2R ChromaQpOffsetSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosStreamChromaQpOffsetSetting::output>& me);
    C2R RefPframesSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosPortRefPframeSetting::output>& me);
    C2R HdrFormatSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamHdrFormatInfo::output> &me);
    C2R HdrStaticInfoSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamHdrStaticInfo::input> &me);
    C2R HdrDynamicInfoInputSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamHdrDynamicMetadataInfo::input> &me);
    C2R HdrDynamicInfoOutputSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamHdrDynamicMetadataInfo::output> &me);

    template <class T>
    C2R CropRectSetter(bool mayBlock, C2InterfaceHelper::C2P<T> &me) {
        (void)mayBlock;

        C2R res = C2R::Ok();

        if (!me.F(me.v.width).supportsAtAll(me.v.width)) {
            res = res.plus(C2SettingResultBuilder::BadValue(me.F(me.v.width)));
            StaticExynosLog(Level::Error, "VencCommonParamIntf", "[%s] width(%d) is invalid", __FUNCTION__, me.v.width);
        }

        if (!me.F(me.v.height).supportsAtAll(me.v.height)) {
            res = res.plus(C2SettingResultBuilder::BadValue(me.F(me.v.height)));
            StaticExynosLog(Level::Error, "VencCommonParamIntf", "[%s] height(%d) is invalid", __FUNCTION__, me.v.height);
        }

        if (!me.F(me.v.left).supportsAtAll(me.v.left)) {
            res = res.plus(C2SettingResultBuilder::BadValue(me.F(me.v.left)));
            StaticExynosLog(Level::Error, "VencCommonParamIntf", "[%s] left(%d) is invalid", __FUNCTION__, me.v.left);
        }

        if (!me.F(me.v.top).supportsAtAll(me.v.top)) {
            res = res.plus(C2SettingResultBuilder::BadValue(me.F(me.v.top)));
            StaticExynosLog(Level::Error, "VencCommonParamIntf", "[%s] top(%d) is invalid", __FUNCTION__, me.v.top);
        }

        return res;
    }

    template <class T, class U>
    C2R CropRectSetter(bool mayBlock, C2InterfaceHelper::C2P<T> &me, const C2InterfaceHelper::C2P<U> &size) {
        (void)mayBlock;
        // TODO: maybe apply block limit?
        C2R res = C2R::Ok();

        if ((!me.F(me.v.width).supportsAtAll(me.v.width)) ||
            (me.v.width > size.v.width)) {
            res = res.plus(C2SettingResultBuilder::BadValue(me.F(me.v.width)));
            StaticExynosLog(Level::Error, "VencCommonParamIntf", "[%s] width(%d) is invalid", __FUNCTION__, me.v.width);
        }

        if ((!me.F(me.v.height).supportsAtAll(me.v.height)) ||
            (me.v.height > size.v.height)) {
            res = res.plus(C2SettingResultBuilder::BadValue(me.F(me.v.height)));
            StaticExynosLog(Level::Error, "VencCommonParamIntf", "[%s] height(%d) is invalid", __FUNCTION__, me.v.height);
        }

        if ((!me.F(me.v.left).supportsAtAll(me.v.left)) ||
            (me.v.left > size.v.width)) {
            res = res.plus(C2SettingResultBuilder::BadValue(me.F(me.v.left)));
            StaticExynosLog(Level::Error, "VencCommonParamIntf", "[%s] left(%d) is invalid", __FUNCTION__, me.v.left);
        }

        if ((!me.F(me.v.top).supportsAtAll(me.v.top)) ||
            (me.v.top > size.v.height)) {
            res = res.plus(C2SettingResultBuilder::BadValue(me.F(me.v.top)));
            StaticExynosLog(Level::Error, "VencCommonParamIntf", "[%s] top(%d) is invalid", __FUNCTION__, me.v.top);
        }

        return res;
    }

    template <class T>
    C2R ScaleSizeSetter(bool mayBlock, C2InterfaceHelper::C2P<T> &me) {
        (void)mayBlock;

        C2R res = C2R::Ok();

        if (!me.F(me.v.width).supportsAtAll(me.v.width)) {
            res = res.plus(C2SettingResultBuilder::BadValue(me.F(me.v.width)));
            StaticExynosLog(Level::Error, "VencCommonParamIntf", "[%s] width(%d) is invalid", __FUNCTION__, me.v.width);
        }

        if (!me.F(me.v.height).supportsAtAll(me.v.height)) {
            res = res.plus(C2SettingResultBuilder::BadValue(me.F(me.v.height)));
            StaticExynosLog(Level::Error, "VencCommonParamIntf", "[%s] height(%d) is invalid", __FUNCTION__, me.v.height);
        }

        StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] scale size(%d x %d)", __FUNCTION__, me.v.width, me.v.height);

        return res;
    }
}; // namespace EncCommonSetter

#endif // EXYNOS_C2_INTFSETTER_H

