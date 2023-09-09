/*
 *
 * Copyright 2018 Samsung Electronics S.LSI Co. LTD
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
#include <string>
#include <cmath>

#include <system/graphics.h>
#include "exynos_format.h"

#include "ExynosGraphicBuffer.h"

#include "ExynosVideoCodecEnc.h"
#include "ExynosVideoApi.h"

#include "ExynosVideoCodecEncCodecSetApi.h"

#define LOG_ON
#include "ExynosLog.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "ExynosVideoCodecEnc"

#define TODO(willUse) (void)willUse

#define TAG_CSD_DATA 0xC5D

constexpr bool INPUT_FIXED_FD_ENABLE = true;
constexpr bool OUTPUT_FIXED_FD_ENABLE = false;

static void getDefaultVideoHdrStatic(ExynosVideoHdrStatic &VideoSTInfo) {
    memset(&VideoSTInfo, 0, sizeof(VideoSTInfo));

    /* TODO */
    VideoSTInfo.max_pic_average_light = 0;
    VideoSTInfo.max_content_light     = 0;
    VideoSTInfo.max_display_luminance = 10000000;
    VideoSTInfo.min_display_luminance = 50;

    VideoSTInfo.red.x     = 34000;
    VideoSTInfo.red.y     = 16000;
    VideoSTInfo.green.x   = 13250;
    VideoSTInfo.green.y   = 34500;
    VideoSTInfo.blue.x    = 7500;
    VideoSTInfo.blue.y    = 3000;
    VideoSTInfo.white.x   = 15635;
    VideoSTInfo.white.y   = 16450;

    StaticExynosLog(Level::Debug, LOG_TAG, "[%s] uses default HDR static info", __FUNCTION__);

    return;
}

static void cnvHdrStaticInfotoVideoHdrStatic(
    ExynosVideoHdrStatic    &VideoSTInfo,
    ExynosHdrStaticInfo     &STInfo) {
    memset(&VideoSTInfo, 0, sizeof(VideoSTInfo));

    VideoSTInfo.max_pic_average_light = (int)STInfo.sType1.mMaxFrameAverageLightLevel;
    VideoSTInfo.max_content_light     = (int)STInfo.sType1.mMaxContentLightLevel;
    VideoSTInfo.max_display_luminance = (int)STInfo.sType1.mMaxDisplayLuminance;
    VideoSTInfo.min_display_luminance = (int)STInfo.sType1.mMinDisplayLuminance;

    VideoSTInfo.red.x     = (unsigned short)STInfo.sType1.mR.x;
    VideoSTInfo.red.y     = (unsigned short)STInfo.sType1.mR.y;
    VideoSTInfo.green.x   = (unsigned short)STInfo.sType1.mG.x;
    VideoSTInfo.green.y   = (unsigned short)STInfo.sType1.mG.y;
    VideoSTInfo.blue.x    = (unsigned short)STInfo.sType1.mB.x;
    VideoSTInfo.blue.y    = (unsigned short)STInfo.sType1.mB.y;
    VideoSTInfo.white.x   = (unsigned short)STInfo.sType1.mW.x;
    VideoSTInfo.white.y   = (unsigned short)STInfo.sType1.mW.y;

    return;
}

static void cnvHdrDynamicInfotoVideoHdrDynamic(
    ExynosVideoHdrDynamic    &VideoDYInfo,
    ExynosHdrDynamicInfo     &DYInfo) {
    memset(&VideoDYInfo, 0, sizeof(VideoDYInfo));

    VideoDYInfo.valid = DYInfo.valid;

    if (DYInfo.valid != 0) {
        VideoDYInfo.itu_t_t35_country_code                    = DYInfo.data.country_code;
        VideoDYInfo.itu_t_t35_terminal_provider_code          = DYInfo.data.provider_code;
        VideoDYInfo.itu_t_t35_terminal_provider_oriented_code = DYInfo.data.provider_oriented_code;
        VideoDYInfo.application_identifier                    = DYInfo.data.application_identifier;
        VideoDYInfo.application_version                       = DYInfo.data.application_version;

#ifdef USE_FULL_ST2094_40
        VideoDYInfo.targeted_system_display_maximum_luminance = DYInfo.data.targeted_system_display_maximum_luminance;
#else
        VideoDYInfo.targeted_system_display_maximum_luminance = DYInfo.data.display_maximum_luminance;
#endif

        /* save information on window-0 only */
        VideoDYInfo.num_windows = 1;
        {
            ExynosVideoHdrWindowInfo &windowInfo = VideoDYInfo.window_info[0];

#ifdef USE_FULL_ST2094_40
            /* maxscl */
            for (int i = 0; i < 3; i++) {
                windowInfo.maxscl[i] = DYInfo.data.maxscl[0][i];
            }

            /* distribution maxrgb */
            windowInfo.num_distribution_maxrgb_percentiles = DYInfo.data.num_maxrgb_percentiles[0];
            for (int i = 0; i < DYInfo.data.num_maxrgb_percentiles[0]; i++) {
                windowInfo.distribution_maxrgb_percentages[i] = DYInfo.data.maxrgb_percentages[0][i];
                windowInfo.distribution_maxrgb_percentiles[i] = DYInfo.data.maxrgb_percentiles[0][i];
            }

            /* tone mapping curve */
            windowInfo.tone_mapping_flag = DYInfo.data.tone_mapping.tone_mapping_flag[0];
            if (DYInfo.data.tone_mapping.tone_mapping_flag[0] != 0) {
                windowInfo.knee_point_x = DYInfo.data.tone_mapping.knee_point_x[0];
                windowInfo.knee_point_y = DYInfo.data.tone_mapping.knee_point_y[0];

                windowInfo.num_bezier_curve_anchors = DYInfo.data.tone_mapping.num_bezier_curve_anchors[0];
                for (int i = 0; i < DYInfo.data.tone_mapping.num_bezier_curve_anchors[0]; i++) {
                    windowInfo.bezier_curve_anchors[i] = DYInfo.data.tone_mapping.bezier_curve_anchors[0][i];
                }
            }
#else
            /* maxscl */
            for (int i = 0; i < 3; i++) {
                windowInfo.maxscl[i] = DYInfo.data.maxscl[i];
            }

            /* distribution maxrgb */
            windowInfo.num_distribution_maxrgb_percentiles = DYInfo.data.num_maxrgb_percentiles;
            for (int i = 0; i < DYInfo.data.num_maxrgb_percentiles; i++) {
                windowInfo.distribution_maxrgb_percentages[i] = DYInfo.data.maxrgb_percentages[i];
                windowInfo.distribution_maxrgb_percentiles[i] = DYInfo.data.maxrgb_percentiles[i];
            }

            /* tone mapping curve */
            windowInfo.tone_mapping_flag = DYInfo.data.tone_mapping.tone_mapping_flag;
            if (DYInfo.data.tone_mapping.tone_mapping_flag != 0) {
                windowInfo.knee_point_x = DYInfo.data.tone_mapping.knee_point_x;
                windowInfo.knee_point_y = DYInfo.data.tone_mapping.knee_point_y;

                windowInfo.num_bezier_curve_anchors = DYInfo.data.tone_mapping.num_bezier_curve_anchors;
                for (int i = 0; i < DYInfo.data.tone_mapping.num_bezier_curve_anchors; i++) {
                    windowInfo.bezier_curve_anchors[i] = DYInfo.data.tone_mapping.bezier_curve_anchors[i];
                }
            }
#endif
        }
    }

    return;
}

class ExynosVideoCodecEnc::CodecEncImpl : public CodecImpl {
public:
    CodecEncImpl(std::string name) : CodecImpl(name, true) {
        mbLogOff = false;

        memset(&mVideoInstInfo, 0, sizeof(mVideoInstInfo));
        memset(&mEncParam, 0, sizeof(mEncParam));
        memset(&mInGeometry, 0, sizeof(mInGeometry));
        mIsDropControl = true;
        mFramerate = 30;
        mIsWeightedPrediction = false;
        mIsAverageQp = false;
        mIsGDCvOTF = false;
        mIsROI = false;
        mIsDRCRequired = false;
        mIsSkype = false;
        mBitrateMode = BITRATE_MODE_DEFAULT;
        mHasHDRStaticInfo = false;
        mHdrEncodingType = HDR_ENCODING_UNKNOWN;
        mIsHdr10PlusStat = false;
    }

    ~CodecEncImpl() = default;

    ExynosVideoErrorType setDefaultConfig(ExynosBufferInfo &input, bool isDRC = false);
    void                 printConfig();

    ExynosVideoErrorType enablePrependSpsPpsToIDR();
    ExynosVideoErrorType setBitrate(uint32_t bitrate);
    ExynosVideoErrorType setFramerate(uint32_t framerate);
    ExynosVideoErrorType setIDRPeriod(uint32_t interval);
    ExynosVideoErrorType setLayering(struct ParamLayering &layering);
    ExynosVideoErrorType setColorAspects(int r, int p, int t, int m);
    ExynosVideoErrorType setQpRange(struct ParamQpRange &range);
    ExynosVideoErrorType enableWeightedPrediction();
    ExynosVideoErrorType setYSumData(uint32_t high, uint32_t low);
    ExynosVideoErrorType setHDRStaticInfo(ExynosVideoHdrStatic &info);
    ExynosVideoErrorType setHDRDynamicInfo(ExynosVideoHdrDynamic &info);
    ExynosVideoErrorType enableGDCvOTF(int type);
    ExynosVideoErrorType setGDCvOTF(ExynosVideoBoolType use);
    ExynosVideoErrorType setActualFormat(uint32_t format);
    ExynosVideoErrorType setOperatingRate(uint32_t framerate);
    ExynosVideoErrorType setRealTimePriority(uint32_t realTimePriority);
    ExynosVideoErrorType setIFrameRatio(uint32_t ratio);
    ExynosVideoErrorType setROIInfo(RoiInfoBuffer &info);
    ExynosVideoErrorType setPMV(struct ParamPMV &pmv);
    ExynosVideoErrorType setFrameType(ExynosVideoFrameType type);
    ExynosVideoErrorType setCrop(ExynosVideoRect &crop);
    ExynosVideoErrorType applySkypeConfig(ExynosParams &params);
    ExynosVideoErrorType enableHdrEncoding(ExynosHdrEncodingType type);
    ExynosVideoErrorType enableHDR10PlusStat();
    ExynosVideoErrorType setMaxIFrameSize(int size);

    ExynosErrorType      checkRealTimeResource(int32_t width, int32_t height, int32_t operatingRate);


#ifdef USE_PERFORMANCE
    void runPerformance() {
        if (mPerf.get() != nullptr) {
            mPerf->notify(mTagNum, mFramerate);
        }
    }
#endif

    ExynosVideoEncParam mEncParam;
    ExynosVideoGeometry mInGeometry;

    bool     mIsDropControl;
    uint32_t mFramerate;
    bool     mIsWeightedPrediction;
    bool     mIsAverageQp;
    bool     mIsGDCvOTF;
    bool     mIsROI;
    bool     mIsDRCRequired;
    bool     mIsSkype;

    ExynosBitrateMode mBitrateMode;

    bool                   mHasHDRStaticInfo;
    ExynosHdrEncodingType  mHdrEncodingType;
    bool                   mIsHdr10PlusStat;

private:
    void setH264StaticConfig(ExynosParams &params, ExynosVideoMeta *meta);
    void setHevcStaticConfig(ExynosParams &params, ExynosVideoMeta *meta);
    void setMpeg4StaticConfig(ExynosParams &params, ExynosVideoMeta *meta);
    void setH263StaticConfig(ExynosParams &params, ExynosVideoMeta *meta);
    void setVpXStaticConfig(std::string name, ExynosParams &params, ExynosVideoEncVpXParam &vpXParam);
    void setVp8StaticConfig(ExynosParams &params, ExynosVideoMeta *meta);
    void setVp9StaticConfig(ExynosParams &params, ExynosVideoMeta *meta);

    CodecEncImpl() = delete;
};

ExynosVideoErrorType ExynosVideoCodecEnc::CodecEncImpl::setDefaultConfig(
    ExynosBufferInfo &input,
    bool              isDRC) {
    ExynosLogFunctionTrace();

    ExynosVideoMeta *meta = input.obj->metadata();

    if (!isDRC) {
        memset(&mEncParam, 0, sizeof(mEncParam));
    }

    switch (mVideoInstInfo.eCodecType) {
    case VIDEO_CODING_AVC:
    {
        if (!isDRC) {
            setH264DefaultConfig(mEncParam.common, mEncParam.codec.h264);
        }
        setH264StaticConfig(input.params, meta);
    }
        break;
    case VIDEO_CODING_HEVC:
    {
        if (!isDRC) {
            setHevcDefaultConfig(mEncParam.common, mEncParam.codec.hevc);
        }
        setHevcStaticConfig(input.params, meta);
    }
        break;
    case VIDEO_CODING_MPEG4:
    {
        if (!isDRC) {
            setMpeg4DefaultConfig(mEncParam.common, mEncParam.codec.mpeg4);
        }
        setMpeg4StaticConfig(input.params, meta);
    }
        break;
    case VIDEO_CODING_H263:
    {
        if (!isDRC) {
            setH263DefaultConfig(mEncParam.common, mEncParam.codec.h263);
        }
        setH263StaticConfig(input.params, meta);
    }
        break;
    case VIDEO_CODING_VP8:
    {
        if (!isDRC) {
            setVp8DefaultConfig(mEncParam.common, mEncParam.codec.vp8);
        }
        setVp8StaticConfig(input.params, meta);
    }
        break;
    case VIDEO_CODING_VP9:
    {
        if (!isDRC) {
            setVp9DefaultConfig(mEncParam.common, mEncParam.codec.vp9);
        }
        setVp9StaticConfig(input.params, meta);
    }
        break;
    default:
        ExynosLogE("[%s] invalid type(%x)", __FUNCTION__, mVideoInstInfo.eCodecType);
        break;
    }

    ExynosVideoEncCommonParam &commonParam = mEncParam.common;

    commonParam.SourceWidth  = input.stImageInfo.nWidth;
    commonParam.SourceHeight = input.stImageInfo.nHeight;

    /* apply common configurations */
    {
        /* intra refresh */
        {
            auto baseParam = input.params.getParam(ExynosParamIndex::IntraRefreshIndex);
            if (baseParam.get() != nullptr) {
                auto param = std::static_pointer_cast<ExynosParam<ParamIntraRefresh>>(baseParam);

                if (param->m.period > 0.0) {
                    int mbsize = 16;
                    if ((mVideoInstInfo.eCodecType == VIDEO_CODING_HEVC) ||
                        (mVideoInstInfo.eCodecType == VIDEO_CODING_VP9)) {
                        mbsize = 32;
                    }

                    /* W/A : the behavior of ACodec */
                    int mbs = (int)std::ceil(
                                    std::ceil((double)commonParam.SourceWidth / mbsize) *
                                    std::ceil((double)commonParam.SourceHeight / mbsize) / param->m.period);

                    commonParam.RandomIntraMBRefresh = mbs;

                    ExynosLogD("[%s] intra refresh(%d)", __FUNCTION__, mbs);
                }
            }
        }
    }

    if (std::get<ExynosVideoEncOps>(mCommonOps).Set_EncParam(mHandle, &mEncParam) != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] Set_EncParam() is failed", __FUNCTION__);
        return VIDEO_ERROR_APIFAIL;
    }

    /* GOP size is based on interval of I-frame */
    if (std::get<ExynosVideoEncOps>(mCommonOps).Set_GopMode(mHandle, VIDEO_FRAME_I) != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] Set_GopMode() is failed", __FUNCTION__);
        return VIDEO_ERROR_APIFAIL;
    }

    /* dynamic framerate */
    {
        auto baseParam = input.params.getParam(ExynosParamIndex::DynamicFramerateIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamDynamicFramerate>>(baseParam);

            if (param->m.enable == 0) {
                std::get<ExynosVideoEncOps>(mCommonOps).Disable_DynamicFrameRate(mHandle);
            }
        }
    }

    /* min quality */
    {
        auto baseParam = input.params.getParam(ExynosParamIndex::MinQualityIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamMinQuality>>(baseParam);

            if (std::get<ExynosVideoEncOps>(mCommonOps).Set_MinQuality(mHandle, param->m.level) == VIDEO_ERROR_NONE) {
                ExynosLogD("[%s] min quality level(%d)", __FUNCTION__, param->m.level);
            }
        }
    }

    return VIDEO_ERROR_NONE;
}

void ExynosVideoCodecEnc::CodecEncImpl::printConfig() {
    ExynosLogFunctionTrace();

    printCommonConfig(mEncParam.common);

    switch (mVideoInstInfo.eCodecType) {
    case VIDEO_CODING_AVC:
    {
        printH264Config(mEncParam.codec.h264);
    }
        break;
    case VIDEO_CODING_HEVC:
    {
        printHevcConfig(mEncParam.codec.hevc);
    }
        break;
    case VIDEO_CODING_MPEG4:
    {
        printMpeg4Config(mEncParam.codec.mpeg4);
    }
        break;
    case VIDEO_CODING_H263:
    {
        printH263Config(mEncParam.codec.h263);
    }
        break;
    case VIDEO_CODING_VP8:
    {
        printVp8Config(mEncParam.codec.vp8);
    }
        break;
    case VIDEO_CODING_VP9:
    {
        printVp9Config(mEncParam.codec.vp9);
    }
        break;
    default:
        ExynosLogE("[%s] invalid type(%x)", __FUNCTION__, mVideoInstInfo.eCodecType);
        break;
    }

    return;
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecEncImpl::enablePrependSpsPpsToIDR() {
    ExynosLogFunctionTrace();

    if (mHandle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    return std::get<ExynosVideoEncOps>(mCommonOps).Enable_PrependSpsPpsToIdr(mHandle);
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecEncImpl::setBitrate(uint32_t bitrate) {
    ExynosLogFunctionTrace();

    if (mHandle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    auto ret = std::get<ExynosVideoEncOps>(mCommonOps).Set_BitRate(mHandle, bitrate);

    if (ret == VIDEO_ERROR_NONE) {
        mEncParam.common.Bitrate = bitrate;
    }

    return ret;
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecEncImpl::setFramerate(uint32_t framerate) {
    ExynosLogFunctionTrace();

    auto ret = ::setFramerate(mHandle, mCommonOps, mVideoInstInfo, mEncParam, framerate);
    if (VIDEO_ERROR_NONE == ret) {
        mFramerate = framerate;
    }

    return ret;
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecEncImpl::setIDRPeriod(uint32_t interval) {
    ExynosLogFunctionTrace();

    if (mHandle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    auto ret = std::get<ExynosVideoEncOps>(mCommonOps).Set_IDRPeriod(mHandle, interval);
    if (ret == VIDEO_ERROR_NONE) {
        mEncParam.common.IDRPeriod = interval;
    }

    return ret;
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecEncImpl::setLayering(struct ParamLayering &layering) {
    ExynosLogFunctionTrace();

    return ::setLayering(mHandle, mCommonOps, mVideoInstInfo, mEncParam, layering);
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecEncImpl::setColorAspects(int r, int p, int t, int m) {
    ExynosLogFunctionTrace();

    ExynosVideoColorAspects CA;

    memset(&CA, 0, sizeof(CA));

    if (mHandle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    CA.eRangeType       = (ExynosRangeType)r;
    CA.ePrimariesType   = (ExynosPrimariesType)p;
    CA.eTransferType    = (ExynosTransferType)t;
    CA.eCoeffType       = (ExynosMatrixCoeffType)m;

    auto ret = std::get<ExynosVideoEncOps>(mCommonOps).Set_ColorAspects(mHandle, &CA);

    return ret;
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecEncImpl::setQpRange(struct ParamQpRange &range) {
    ExynosLogFunctionTrace();

    ExynosVideoErrorType ret = VIDEO_ERROR_NONE;

    ExynosVideoQPRange qpRange;

    memset(&qpRange, 0, sizeof(qpRange));

    if (mHandle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    qpRange.QpMin_I = range.minI;
    qpRange.QpMax_I = range.maxI;
    qpRange.QpMin_P = range.minP;
    qpRange.QpMax_P = range.maxP;
    qpRange.QpMin_B = range.minB;
    qpRange.QpMax_B = range.maxB;

    ret = std::get<ExynosVideoEncOps>(mCommonOps).Set_QpRange(mHandle, qpRange);
    if (ret == VIDEO_ERROR_NONE) {
        memcpy(&mEncParam.common.QpRange, &qpRange, sizeof(qpRange));
    }

    return ret;
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecEncImpl::enableWeightedPrediction() {
    ExynosLogFunctionTrace();

    if (mHandle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    return std::get<ExynosVideoEncOps>(mCommonOps).Enable_WeightedPrediction(mHandle);
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecEncImpl::setYSumData(uint32_t high, uint32_t low) {
    ExynosLogFunctionTrace();

    if (mHandle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    return std::get<ExynosVideoEncOps>(mCommonOps).Set_YSumData(mHandle, high, low);
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecEncImpl::setHDRStaticInfo(ExynosVideoHdrStatic &info) {
    ExynosLogFunctionTrace();

    if (mHandle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    auto ret = std::get<ExynosVideoEncOps>(mCommonOps).Set_HDRStaticInfo(mHandle, &info);
    if (ret == VIDEO_ERROR_NONE) {
        mHasHDRStaticInfo = true;
    }

    return ret;
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecEncImpl::setHDRDynamicInfo(ExynosVideoHdrDynamic &info) {
    ExynosLogFunctionTrace();

    if (mHandle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    return std::get<ExynosVideoEncOps>(mCommonOps).Set_HDRDynamicInfo(mHandle, &info);
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecEncImpl::enableGDCvOTF(int type) {
    ExynosLogFunctionTrace();

    if (mHandle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    return std::get<ExynosVideoEncOps>(mCommonOps).Enable_GDCvOTF(mHandle, type);
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecEncImpl::setGDCvOTF(ExynosVideoBoolType use) {
    ExynosLogFunctionTrace();

    if (mHandle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    return std::get<ExynosVideoEncOps>(mCommonOps).Set_GDCvOTF(mHandle, use);
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecEncImpl::setActualFormat(uint32_t format) {
    ExynosLogFunctionTrace();

    ExynosVideoErrorType ret = VIDEO_ERROR_NONE;

    if (mHandle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    if ((mInGeometry.eFilledDataType == DATA_8BIT_SBWC) ||
        (mInGeometry.eFilledDataType == DATA_10BIT_SBWC)) {
        ret = std::get<ExynosVideoEncOps>(mCommonOps).Set_ActualFormat(mHandle, (ExynosVideoColorFormatType)format);
    }

    return ret;
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecEncImpl::setOperatingRate(uint32_t framerate) {
    ExynosLogFunctionTrace();

    if (mHandle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    return std::get<ExynosVideoEncOps>(mCommonOps).Set_OperatingRate(mHandle, framerate);
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecEncImpl::setRealTimePriority(uint32_t realTimePriority) {
    ExynosLogFunctionTrace();

    if (mHandle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    return std::get<ExynosVideoEncOps>(mCommonOps).Set_RealTimePriority(mHandle, realTimePriority);
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecEncImpl::setIFrameRatio(uint32_t ratio) {
    ExynosLogFunctionTrace();

    if (mHandle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    return std::get<ExynosVideoEncOps>(mCommonOps).Set_IFrameRatio(mHandle, ratio);
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecEncImpl::setROIInfo(RoiInfoBuffer &info) {
    ExynosLogFunctionTrace();

    if (mHandle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    return std::get<ExynosVideoEncOps>(mCommonOps).Set_RoiInfo(mHandle, &info);
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecEncImpl::setPMV(struct ParamPMV &pmv) {
    ExynosLogFunctionTrace();

    return ::setPMV(mHandle, mCommonOps, mEncParam, pmv);
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecEncImpl::setFrameType(ExynosVideoFrameType type) {
    ExynosLogFunctionTrace();

    if (mHandle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    return std::get<ExynosVideoEncOps>(mCommonOps).Set_FrameType(mHandle, type);
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecEncImpl::setCrop(ExynosVideoRect &crop) {
    ExynosLogFunctionTrace();

    if (mHandle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    return std::get<ExynosVideoEncOps>(mCommonOps).Set_Crop(mHandle, &crop);
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecEncImpl::enableHdrEncoding(ExynosHdrEncodingType type) {
    ExynosLogFunctionTrace();

    if (mHandle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    if (type == HDR_ENCODING_UNKNOWN) {
        return VIDEO_ERROR_BADPARAM;
    }

    if ((mVideoInstInfo.eCodecType == VIDEO_CODING_HEVC) &&
        (mInGeometry.eFilledDataType & DATA_10BIT) &&
        (mVideoInstInfo.supportInfo.enc.bHDRStaticInfoSupport == VIDEO_TRUE)) {
        mHdrEncodingType = type;

        if ((type == HDR_ENCODING_HDR10_PLUS) &&
            (mVideoInstInfo.supportInfo.enc.bHDRDynamicInfoSupport != VIDEO_TRUE)) {
            mHdrEncodingType = HDR_ENCODING_HDR10;
        }
    }

    return VIDEO_ERROR_NONE;
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecEncImpl::enableHDR10PlusStat() {
    ExynosLogFunctionTrace();

    if (mHandle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    auto ret = std::get<ExynosVideoEncOps>(mCommonOps).Enable_HDRDynamicStatInfo(mHandle);

    if (ret == VIDEO_ERROR_NONE) {
        mIsHdr10PlusStat = true;
    }

    return ret;
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecEncImpl::setMaxIFrameSize(int size) {
    ExynosLogFunctionTrace();

    if (mHandle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    if (mBitrateMode != BITRATE_MODE_CONST_WFD) {
        ExynosLogE("[%s] bitrate mode(0x%x) is not \"CBR for WFD\"", __FUNCTION__, mBitrateMode);
        return VIDEO_ERROR_BADPARAM;
    }

    return std::get<ExynosVideoEncOps>(mCommonOps).Set_MaxIFrameSize(mHandle, size);
}

ExynosErrorType ExynosVideoCodecEnc::CodecEncImpl::checkRealTimeResource(
    int32_t width,
    int32_t height,
    int32_t operatingRate) {
    ExynosLogFunctionTrace();

    if (mHandle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return EXYNOS_ERROR_INVALID_PARAM;
    }

    ExynosVideoGeometry geometry;
    memset(&geometry, 0, sizeof(geometry));

    geometry.nWidth  = width;
    geometry.nHeight = height;
    geometry.eCompressionFormat = mVideoInstInfo.eCodecType;

    if (mOutBufOps.Try_Geometry(mHandle, &geometry) != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] outbuf : Try_Geometry() for real time resource is failed", __FUNCTION__);
        return EXYNOS_ERROR_UNKNOWN;
    }

    if (std::get<ExynosVideoEncOps>(mCommonOps).Set_RealTimePriority(mHandle, 0 /* real time */) != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] Set_RealTimePriority for real time resource is failed", __FUNCTION__);
        return EXYNOS_ERROR_UNKNOWN;
    }

    if (std::get<ExynosVideoEncOps>(mCommonOps).Set_OperatingRate(mHandle, operatingRate) != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] Set_OperatingRate(%d) for real time resource is failed", __FUNCTION__, operatingRate);
        return EXYNOS_ERROR_UNKNOWN;
    }

    if (std::get<ExynosVideoEncOps>(mCommonOps).Get_RealTimePriority(mHandle) != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] Get_RealTimePriority for real time resource is failed", __FUNCTION__);
        return EXYNOS_ERROR_UNKNOWN;
    }

    return EXYNOS_ERROR_NONE;
}

void ExynosVideoCodecEnc::CodecEncImpl::setH264StaticConfig(
    ExynosParams    &params,
    ExynosVideoMeta *meta) {
    ExynosLogFunctionTrace();

    ::setH264StaticConfig(mHandle, mCommonOps, params, mEncParam, mVideoInstInfo, mIsSkype, mIsROI, mBitrateMode, meta);
}

void ExynosVideoCodecEnc::CodecEncImpl::setHevcStaticConfig(
    ExynosParams    &params,
    ExynosVideoMeta *meta) {
    ExynosLogFunctionTrace();

    ::setHevcStaticConfig(mHandle, mCommonOps, params, mEncParam, mVideoInstInfo, mIsROI, mBitrateMode, meta);
}

void ExynosVideoCodecEnc::CodecEncImpl::setMpeg4StaticConfig(
    ExynosParams    &params,
    ExynosVideoMeta *meta) {
    ExynosLogFunctionTrace();

    ::setMpeg4StaticConfig(params, mEncParam, mBitrateMode, meta);
}

void ExynosVideoCodecEnc::CodecEncImpl::setH263StaticConfig(
    ExynosParams    &params,
    ExynosVideoMeta *meta) {
    ExynosLogFunctionTrace();

    ::setH263StaticConfig(params, mEncParam, mBitrateMode, meta);
}

void ExynosVideoCodecEnc::CodecEncImpl::setVp8StaticConfig(
    ExynosParams    &params,
    ExynosVideoMeta *meta) {
    ExynosLogFunctionTrace();

    ::setVp8StaticConfig(mHandle, mCommonOps, params, mEncParam, mBitrateMode, meta);
}

void ExynosVideoCodecEnc::CodecEncImpl::setVp9StaticConfig(
    ExynosParams    &params,
    ExynosVideoMeta *meta) {
    ExynosLogFunctionTrace();

    ::setVp9StaticConfig(mHandle, mCommonOps, params, mEncParam, mBitrateMode, meta);
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecEncImpl::applySkypeConfig(ExynosParams &params) {
    ExynosLogFunctionTrace();

    return ::applySkypeConfig(mHandle, mCommonOps, params);
}

void EncCodecInfoRegistry::registCodecInfo() {
    if (mCodecInfo.get() != nullptr) {
        mCodecInfo->clear();
        mCodecInfo.reset();
    }

    std::map<ExynosVideoCodecBase::Type, CodecInfo> CodecInfoMap = {
        /* H.264 */
        {ExynosVideoCodecBase::Type::H264, {"ExynosVideoCodecEnc-H264Enc", VIDEO_CODING_AVC, VIDEO_NORMAL, VIDEO_FALSE}},
        {ExynosVideoCodecBase::Type::H264_SECURE, {"ExynosVideoCodecEnc-H264Enc.secure", VIDEO_CODING_AVC, VIDEO_SECURE, VIDEO_FALSE}},
        /* HEVC */
        {ExynosVideoCodecBase::Type::HEVC, {"ExynosVideoCodecEnc-HevcEnc", VIDEO_CODING_HEVC, VIDEO_NORMAL, VIDEO_FALSE}},
        {ExynosVideoCodecBase::Type::HEVC_SECURE, {"ExynosVideoCodecEnc-HevcEnc.secure", VIDEO_CODING_HEVC, VIDEO_SECURE, VIDEO_FALSE}},
        /* Mpeg4 */
        {ExynosVideoCodecBase::Type::MPEG4, {"ExynosVideoCodecEnc-Mpeg4Enc", VIDEO_CODING_MPEG4, VIDEO_NORMAL, VIDEO_FALSE}},
        /* H.263 */
        {ExynosVideoCodecBase::Type::H263, {"ExynosVideoCodecEnc-H263Enc", VIDEO_CODING_H263, VIDEO_NORMAL, VIDEO_FALSE}},
        /* VP8 */
        {ExynosVideoCodecBase::Type::VP8, {"ExynosVideoCodecEnc-Vp8Enc", VIDEO_CODING_VP8, VIDEO_NORMAL, VIDEO_FALSE}},
        /* VP9 */
        {ExynosVideoCodecBase::Type::VP9, {"ExynosVideoCodecEnc-Vp9Enc", VIDEO_CODING_VP9, VIDEO_NORMAL, VIDEO_FALSE}},

        /* H.264 LLWFD */
        {ExynosVideoCodecBase::Type::H264WFD, {"ExynosVideoCodecEnc-H264WFDEnc", VIDEO_CODING_AVC, VIDEO_NORMAL, VIDEO_TRUE}},
        /* H.264 LLWFD SECURE*/
        {ExynosVideoCodecBase::Type::H264WFD_SECURE, {"ExynosVideoCodecEnc-H264WFDEnc.secure", VIDEO_CODING_AVC, VIDEO_SECURE, VIDEO_TRUE}},
        /* HEVC LLWFD */
        {ExynosVideoCodecBase::Type::HEVCWFD, {"ExynosVideoCodecEnc-HevcWFDEnc", VIDEO_CODING_HEVC, VIDEO_NORMAL, VIDEO_TRUE}},
        /* HEVC LLWFD SECURE*/
        {ExynosVideoCodecBase::Type::HEVCWFD_SECURE, {"ExynosVideoCodecEnc-HevcWFDEnc.secure", VIDEO_CODING_HEVC, VIDEO_SECURE, VIDEO_TRUE}},
    };

    mCodecInfo = std::make_shared<std::map<ExynosVideoCodecBase::Type, CodecInfo>>(CodecInfoMap);
}

ExynosVideoCodecEnc::ExynosVideoCodecEnc(ExynosVideoCodecBase::Type type) {
    mbLogOff = false;

    auto codecInfo = getCodecInfo(type);

    mObjName    = codecInfo.objName;
    mCodecImpl  = std::make_shared<CodecEncImpl>(mObjName);
    mCodecImpl->mVideoInstInfo.eCodecType       = codecInfo.eCodecType;
    mCodecImpl->mVideoInstInfo.eSecurityType    = codecInfo.eSecurityType;
    mCodecImpl->mVideoInstInfo.bOTFMode         = codecInfo.bOTFMode;

    setCodecImpl(mCodecImpl, true);

    mExynosPort[ExynosPort::Input].mBufManager  = std::make_shared<SharedPtrManager>(INPUT_FIXED_FD_ENABLE);
    mExynosPort[ExynosPort::Output].mBufManager = std::make_shared<SharedPtrManager>(OUTPUT_FIXED_FD_ENABLE);

    mCodecImpl->mVideoInstInfo.nMemoryType = VIDEO_MEMORY_DMABUF;

    if (Exynos_Video_GetInstInfo(&(mCodecImpl->mVideoInstInfo), VIDEO_FALSE) != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] Exynos_Video_GetInstInfo() is failed", __FUNCTION__);
    }

    mExynosPort[ExynosPort::Input].mIsConfigured = false;
    mExynosPort[ExynosPort::Output].mIsConfigured = false;

    {
        std::lock_guard<std::mutex> lock(mExynosPort[ExynosPort::Input].mStreamMutex);
        mExynosPort[ExynosPort::Input].mIsOn = false;
    }

    {
        std::lock_guard<std::mutex> lock(mExynosPort[ExynosPort::Output].mStreamMutex);
        mExynosPort[ExynosPort::Output].mIsOn = false;
    }

    init();
}

ExynosVideoCodecEnc::~ExynosVideoCodecEnc() {
    deinit();

    mCodecImpl.reset();
    mExynosPort[ExynosPort::Input].mBufManager->reset();
    mExynosPort[ExynosPort::Output].mBufManager->reset();
}

bool ExynosVideoCodecEnc::isValidCodecHandle() {
    auto handle = mCodecImpl->mHandle;

    if (handle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return false;
    }

    return true;
}

void ExynosVideoCodecEnc::updateBufferInfo(ExynosVideoBuffer &buffer, ExynosBufferInfo &buf) {
    int frameType = buffer.extraInfo.specific.enc.frameType;
    ExynosLogD("[%s] frame type : %x", __FUNCTION__, frameType);
    buf.stImageInfo.eFrameInfo = getFrameInfoType(frameType);

    buf.nID = getFrameTag(mCodecImpl);

    if (buf.nID == TAG_CSD_DATA) {
        buf.eDataInfo  = DataInfo::MultiData;  /* the header will be returned with I-frame */
        buf.stImageInfo.eFrameInfo = FrameInfo::CodecSpecificData;

        ExynosLogD("[%s] output : dequeue / fd(%d), size(%d), ptr(%p) / CSD",
                        __FUNCTION__, buf.nFD[0], buf.nDataSize[0], buf.obj.get());
    } else {
        if (mCodecImpl->mIsAverageQp) {
            updateAverageQpInfoToParam(buf.params);
        }

        if (frameType & VIDEO_FRAME_WITH_HDR_STAT_INFO) {
            updateHDR10PlusStatInfoToParam(buf.params);
        }

        ExynosLogD("[%s] output : dequeue / fd(%d), size(%d), ptr(%p), id(%d)",
                        __FUNCTION__, buf.nFD[0], buf.nDataSize[0], buf.obj.get(), buf.nID);
    }
}

void ExynosVideoCodecEnc::setDropControl() {
    ExynosVideoEncOps &ops = std::get<ExynosVideoEncOps>(mCodecImpl->mCommonOps);

    auto handle = mCodecImpl->mHandle;

    if (mCodecImpl->mVideoInstInfo.supportInfo.enc.bDropControlSupport == VIDEO_TRUE) {
        auto isDropControl = (mCodecImpl->mIsDropControl)? VIDEO_TRUE:VIDEO_FALSE;
        ops.Set_DropControl(handle, isDropControl);
    }

    return;
}

void ExynosVideoCodecEnc::runPerformance() {
#ifdef USE_PERFORMANCE
    mCodecImpl->runPerformance();
#endif

    return;
}

ExynosErrorType ExynosVideoCodecEnc::init() {
    ExynosLogFunctionTrace();

    ExynosVideoEncOps encOps;
    auto err = Exynos_Video_Register_Encoder(&encOps, &(mCodecImpl->mInBufOps), &(mCodecImpl->mOutBufOps));
    if (err != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] Exynos_Video_Register_Decoder() is failed", __FUNCTION__);
        return EXYNOS_ERROR_UNKNOWN;
    }
    mCodecImpl->mCommonOps = encOps;

    mCodecImpl->mHandle = std::get<ExynosVideoEncOps>(mCodecImpl->mCommonOps).Init(&(mCodecImpl->mVideoInstInfo));
    if (mCodecImpl->mHandle == nullptr) {
        ExynosLogE("[%s] Init() is failed", __FUNCTION__);
        return EXYNOS_ERROR_RESOURCE;
    }

    return EXYNOS_ERROR_NONE;
}

void ExynosVideoCodecEnc::deinit() {
    ExynosLogFunctionTrace();

    if (mCodecImpl->mVideoInstInfo.eCodecType == VIDEO_CODING_UNKNOWN) {
        return;
    }

    auto handle = mCodecImpl->mHandle;

    if (handle != nullptr) {
        streamOnOff(ExynosPort::Input, ExynosPort::Off);
        streamOnOff(ExynosPort::Output, ExynosPort::Off);

        if (std::get<ExynosVideoEncOps>(mCodecImpl->mCommonOps).Finalize(handle) != VIDEO_ERROR_NONE) {
            ExynosLogE("[%s] Finalize() is failed", __FUNCTION__);
        }

        mCodecImpl->mHandle = nullptr;
    }
}

ExynosErrorType ExynosVideoCodecEnc::srcSetup(ExynosBufferInfo input) {
    ExynosLogFunctionTrace();

    ExynosVideoEncOps    &ops       = std::get<ExynosVideoEncOps>(mCodecImpl->mCommonOps);
    ExynosVideoBufferOps &inBufOps  = mCodecImpl->mInBufOps;
    ExynosVideoBufferOps &outBufOps = mCodecImpl->mOutBufOps;

    auto handle = mCodecImpl->mHandle;

    if (handle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return EXYNOS_ERROR_BAD_STATE;
    }

    streamOnOff(ExynosPort::Input, ExynosPort::Off);

    if (mExynosPort[ExynosPort::Input].mIsConfigured) {
        inBufOps.Cleanup_Buffer(handle);
        mExynosPort[ExynosPort::Input].mIsConfigured = false;
    }

    /* set default configurations */
    mCodecImpl->setDefaultConfig(input);

    /* input buffer geometry */
    {
        ExynosVideoGeometry geometry;
        memset(&geometry, 0, sizeof(geometry));

        geometry.nWidth          = input.stImageInfo.nWidth;
        geometry.nHeight         = input.stImageInfo.nHeight;
        geometry.nStride         = input.stImageInfo.nStride;
        geometry.nCStride        = vendor::graphics::ExynosGraphicBufferMeta::get_cstride(input.obj->handle());
        geometry.eColorFormat    = (ExynosVideoColorFormatType)getVideoFormat(input.stImageInfo.nFormat);
        geometry.nPlaneCnt       = input.nPlane;
        geometry.eFilledDataType = (ExynosFilledDataType)getDataType(geometry.eColorFormat);

        ExynosLogD("[%s] resolution info : %d x %d (%d, %d, %d, %d) s:%d / cs:%d", __FUNCTION__,
                    geometry.nWidth, geometry.nHeight,
                    geometry.cropRect.nTop, geometry.cropRect.nLeft, geometry.cropRect.nWidth, geometry.cropRect.nHeight,
                    geometry.nStride, geometry.nCStride);

        if (inBufOps.Set_Geometry(handle, &geometry) != VIDEO_ERROR_NONE) {
            ExynosLogE("[%s] inbuf : Set_Geometry() is failed", __FUNCTION__);
            return EXYNOS_ERROR_UNKNOWN;
        }

        mCodecImpl->mEncParam.common.FrameMap = geometry.eColorFormat;

        memcpy(&(mCodecImpl->mInGeometry), &geometry, sizeof(geometry));
    }

    if (inBufOps.Setup(handle, 0/* DYNAMIC */) != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] inbuf : Setup() is failed", __FUNCTION__);
        return EXYNOS_ERROR_UNKNOWN;
    }

    if (mCodecImpl->mVideoInstInfo.eSecurityType == VIDEO_SECURE) {
        if (ops.Set_HeaderMode(handle, VIDEO_FALSE) != VIDEO_ERROR_NONE) {
            ExynosLogE("[%s] Set_HeaderMode() is failed", __FUNCTION__);
            return EXYNOS_ERROR_UNKNOWN;
        }
    }

    /* output buffer geometry */
    {
        ExynosVideoGeometry geometry;
        memset(&geometry, 0, sizeof(geometry));

        geometry.nSizeImage         = (mCodecImpl->mVideoInstInfo.eSecurityType == VIDEO_SECURE) ? ExynosUtils::GetOutputSizeForEncSecure(input.stImageInfo.nWidth, input.stImageInfo.nHeight)
                                                                                                   : ExynosUtils::GetOutputSizeForEnc(input.stImageInfo.nWidth, input.stImageInfo.nHeight);
        geometry.nPlaneCnt          = 1;  /* it must be 1 */
        geometry.eCompressionFormat = mCodecImpl->mVideoInstInfo.eCodecType;

        if (outBufOps.Set_Geometry(handle, &geometry) != VIDEO_ERROR_NONE) {
            ExynosLogE("[%s] outbuf : Set_Geometry() is failed", __FUNCTION__);
            return EXYNOS_ERROR_UNKNOWN;
        }
    }

    /* apply configurations */
    applyConfig(input.params);

    applyExtraInfo(input);

    if ((mCodecImpl->mHdrEncodingType != HDR_ENCODING_UNKNOWN) &&
        (!mCodecImpl->mHasHDRStaticInfo)) {
        ExynosVideoHdrStatic STInfo;

        getDefaultVideoHdrStatic(STInfo);

        auto err = mCodecImpl->setHDRStaticInfo(STInfo);
        if (err != VIDEO_ERROR_NONE) {
            ExynosLogW("[%s] setHDRStaticInfo() is failed", __FUNCTION__);
        }
    }

    mExynosPort[ExynosPort::Input].mIsConfigured = true;

    /* print configurations info */
    mCodecImpl->printConfig();

    return EXYNOS_ERROR_NONE;
}

ExynosErrorType ExynosVideoCodecEnc::dstSetup() {
    ExynosLogFunctionTrace();

    ExynosVideoBufferOps &outBufOps = mCodecImpl->mOutBufOps;

    auto handle = mCodecImpl->mHandle;

    if (handle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return EXYNOS_ERROR_BAD_STATE;
    }

    streamOnOff(ExynosPort::Output, ExynosPort::Off);

    if (mExynosPort[ExynosPort::Output].mIsConfigured) {
        outBufOps.Cleanup_Buffer(handle);
        mExynosPort[ExynosPort::Output].mIsConfigured = false;
    }

    if (outBufOps.Enable_Cacheable(handle) != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] outbuf : Enable_Cacheable() is failed", __FUNCTION__);
        return EXYNOS_ERROR_UNKNOWN;
    }

    if (outBufOps.Setup(handle, 0/* DYNAMIC */) != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] outbuf : Setup() is failed", __FUNCTION__);
        return EXYNOS_ERROR_UNKNOWN;
    }

    mExynosPort[ExynosPort::Output].mIsConfigured = true;

    return EXYNOS_ERROR_NONE;
}

bool ExynosVideoCodecEnc::availableToEnqueue(ExynosBufferInfo &buf) {
    ExynosLogFunctionTrace();

    if (needToChangeResolution(buf) &&
        (mExynosPort[ExynosPort::Input].mBufManager->size() > 0)) {
        /* wait for input consumption */
        return false;
    }

    return true;
}

bool ExynosVideoCodecEnc::needToChangeResolution(ExynosBufferInfo &buf) {
    ExynosLogFunctionTrace();

    if (mExynosPort[ExynosPort::Input].mIsConfigured) {
        if (mCodecImpl->mIsDRCRequired) {
            return true;
        }

        if ((mCodecImpl->mEncParam.common.SourceWidth != buf.stImageInfo.nWidth) ||
            (mCodecImpl->mEncParam.common.SourceHeight != buf.stImageInfo.nHeight) ||
            (mCodecImpl->mEncParam.common.FrameMap != (ExynosVideoColorFormatType)getVideoFormat(buf.stImageInfo.nFormat))) {
            return true;
        }

        ExynosVideoMeta *meta = buf.obj->metadata();
        if (meta != nullptr) {
            if ((meta->eType & VIDEO_INFO_TYPE_YSUM_DATA) &&
                (!mCodecImpl->mIsWeightedPrediction)) {
                return true;
            }

            if ((meta->eType & VIDEO_INFO_TYPE_GDC_OTF) &&
                (meta->data.enc.nUseGdcOTF != GDC_TYPE_NONE) &&
                (!mCodecImpl->mIsGDCvOTF)) {
                return true;
            }

            if ((meta->eType & VIDEO_INFO_TYPE_ROI_INFO) &&
                ((mCodecImpl->mVideoInstInfo.eCodecType == VIDEO_CODING_AVC) ||
                 (mCodecImpl->mVideoInstInfo.eCodecType == VIDEO_CODING_HEVC)) &&
                (!mCodecImpl->mIsROI)) {
                return true;
            }
        }
    }

    return false;
}

ExynosErrorType ExynosVideoCodecEnc::changeResolution(ExynosBufferInfo &buf) {
    ExynosLogFunctionTrace();

    ExynosVideoEncOps       &ops        = std::get<ExynosVideoEncOps>(mCodecImpl->mCommonOps);
    ExynosVideoBufferOps    &inBufOps   = mCodecImpl->mInBufOps;

    auto handle = mCodecImpl->mHandle;

    if (handle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return EXYNOS_ERROR_BAD_STATE;
    }

    streamOnOff(ExynosPort::Input, ExynosPort::Off);

    ExynosVideoGeometry geometry;
    memset(&geometry, 0, sizeof(geometry));

    /* for sync with MFC D/D */
    if (inBufOps.Get_Geometry(handle, &geometry) != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] inbuf : Get_Geometry() is failed", __FUNCTION__);
        return EXYNOS_ERROR_UNKNOWN;
    }

    if (mExynosPort[ExynosPort::Input].mIsConfigured) {
        inBufOps.Cleanup_Buffer(handle);
        mExynosPort[ExynosPort::Input].mIsConfigured = false;
    }

    /* set default configurations */
    mCodecImpl->setDefaultConfig(buf, true);

    /* input buffer geometry */
    {
        geometry.nWidth          = buf.stImageInfo.nWidth;
        geometry.nHeight         = buf.stImageInfo.nHeight;
        geometry.nStride         = buf.stImageInfo.nStride;
        geometry.nCStride        = vendor::graphics::ExynosGraphicBufferMeta::get_cstride(buf.obj->handle());
        geometry.eColorFormat    = (ExynosVideoColorFormatType)getVideoFormat(buf.stImageInfo.nFormat);
        geometry.nPlaneCnt       = buf.nPlane;
        geometry.eFilledDataType = (ExynosFilledDataType)getDataType(geometry.eColorFormat);

        ExynosLogD("[%s] resolution info : %d x %d (%d, %d, %d, %d) s:%d / cs:%d", __FUNCTION__,
                    geometry.nWidth, geometry.nHeight,
                    geometry.cropRect.nTop, geometry.cropRect.nLeft, geometry.cropRect.nWidth, geometry.cropRect.nHeight,
                    geometry.nStride, geometry.nCStride);

        if (inBufOps.Set_Geometry(handle, &geometry) != VIDEO_ERROR_NONE) {
            ExynosLogE("[%s] inbuf : Set_Geometry() is failed", __FUNCTION__);
            return EXYNOS_ERROR_UNKNOWN;
        }

        mCodecImpl->mEncParam.common.FrameMap = geometry.eColorFormat;
    }

    if (inBufOps.Setup(handle, 0/* DYNAMIC */) != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] inbuf : Setup() is failed", __FUNCTION__);
        return EXYNOS_ERROR_UNKNOWN;
    }

    /* header with IDR */
    if (ops.Set_HeaderMode(handle, VIDEO_FALSE) != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] Set_HeaderMode() is failed", __FUNCTION__);
        return EXYNOS_ERROR_UNKNOWN;
    }

    if (geometry.eColorFormat == VIDEO_COLORFORMAT_RGBA8888) {
        /* need to set color aspects */
        int range       = RANGE_UNSPECIFIED;
        int primaries   = PRIMARIES_UNSPECIFIED;
        int transfer    = TRANSFER_UNSPECIFIED;
        int matrix      = MATRIX_COEFF_UNSPECIFIED;

        if (geometry.HdrInfo.eValidType & (HDR_INFO_RANGE | HDR_INFO_COLOR_ASPECTS)) {
            range       = geometry.HdrInfo.sColorAspects.eRangeType;
            primaries   = geometry.HdrInfo.sColorAspects.ePrimariesType;
            transfer    = geometry.HdrInfo.sColorAspects.eTransferType;
            matrix      = geometry.HdrInfo.sColorAspects.eCoeffType;
        }

        getColorAspectsForRGB(range, primaries, transfer, matrix);

        auto err = mCodecImpl->setColorAspects(range, primaries, transfer, matrix);
        if (err == VIDEO_ERROR_NONE) {
            ExynosLogD("[%s] ColorAspects is changed(r:%d, p:%d, t:%d, m:%d)", __FUNCTION__,
                        range, primaries, transfer, matrix);
        } else {
            ExynosLogE("[%s] Set_ColorAspects(r:%d, p:%d, t:%d, m:%d) is failed", __FUNCTION__,
                        range, primaries, transfer, matrix);
        }
    }

    ExynosLogD("[%s] stream is changed to (w:%d, h:%d, f:0x%x)", __FUNCTION__,
                buf.stImageInfo.nWidth, buf.stImageInfo.nHeight, buf.stImageInfo.nFormat);

    applyExtraInfo(buf);

    if ((mCodecImpl->mHdrEncodingType != HDR_ENCODING_UNKNOWN) &&
        (!mCodecImpl->mHasHDRStaticInfo)) {
        ExynosVideoHdrStatic STInfo;

        getDefaultVideoHdrStatic(STInfo);

        auto err = mCodecImpl->setHDRStaticInfo(STInfo);
        if (err != VIDEO_ERROR_NONE) {
            ExynosLogW("[%s] setHDRStaticInfo() is failed", __FUNCTION__);
        }
    }

    mExynosPort[ExynosPort::Input].mIsConfigured = true;

    return EXYNOS_ERROR_NONE;
}

void ExynosVideoCodecEnc::setDRCRequired(bool bDRCRequired) {
    mCodecImpl->mIsDRCRequired = bDRCRequired;
    return;
}

bool ExynosVideoCodecEnc::getDRCRequired() {
    return mCodecImpl->mIsDRCRequired;
}

void ExynosVideoCodecEnc::applyConfig_Bitrate(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* bitrate */
    auto baseParam = params.getParam(ExynosParamIndex::BitrateIndex);
    if (baseParam.get() == nullptr) {
        return;
    }

    auto param = std::static_pointer_cast<ExynosParam<ParamBitrate>>(baseParam);

    auto err = mCodecImpl->setBitrate(param->m.bitrate);

    if (err == VIDEO_ERROR_NONE) {
        ExynosLogD("[%s] bitrate is changed to %d", __FUNCTION__, param->m.bitrate);
    } else {
        ExynosLogE("[%s] Set_BitRate(%d) is failed", __FUNCTION__, param->m.bitrate);
    }
}

void ExynosVideoCodecEnc::applyConfig_Framerate(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* framerate */
    auto baseParam = params.getParam(ExynosParamIndex::FramerateIndex);
    if (baseParam.get() == nullptr) {
        return;
    }

    auto param = std::static_pointer_cast<ExynosParam<ParamFramerate>>(baseParam);

    auto framerate = param->m.framerate;

    /* TODO : apply to all codecs? */
    if ((framerate == 0) &&
        ((mCodecImpl->mVideoInstInfo.eCodecType == VIDEO_CODING_VP8) ||
         (mCodecImpl->mVideoInstInfo.eCodecType == VIDEO_CODING_VP9))) {
         framerate = 30;
    }

    auto err = mCodecImpl->setFramerate(framerate);

    if (err == VIDEO_ERROR_NONE) {
        ExynosLogD("[%s] framerate is changed to %d", __FUNCTION__, framerate);
    } else {
        ExynosLogE("[%s] Set_FrameRate(%d) is failed", __FUNCTION__, framerate);
    }
}

void ExynosVideoCodecEnc::applyConfig_IdrPeriod(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* IDR period */
    auto baseParam = params.getParam(ExynosParamIndex::IDRPeriodIndex);
    if (baseParam.get() == nullptr) {
        return;
    }

    auto param = std::static_pointer_cast<ExynosParam<ParamIDRPeriod>>(baseParam);

    auto err = mCodecImpl->setIDRPeriod(param->m.period);

    if (err == VIDEO_ERROR_NONE) {
        ExynosLogD("[%s] IDR Period is changed to %d", __FUNCTION__, param->m.period);
    } else {
        ExynosLogE("[%s] Set_IDRPeriod(%d) is failed", __FUNCTION__, param->m.period);
    }
}

void ExynosVideoCodecEnc::applyConfig_Layering(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* layering (Temporal SVC) */
    auto baseParam = params.getParam(ExynosParamIndex::LayeringIndex);
    if (baseParam.get() == nullptr) {
        return;
    }

    auto param = std::static_pointer_cast<ExynosParam<ParamLayering>>(baseParam);

    auto err = mCodecImpl->setLayering(param->m);

    if (err == VIDEO_ERROR_NONE) {
        ExynosLogD("[%s] layering is changed to %d", __FUNCTION__, param->m.layerCount);
    } else {
        ExynosLogE("[%s] Set_LayerChange(%d) is failed", __FUNCTION__, param->m.layerCount);
    }
}

void ExynosVideoCodecEnc::applyConfig_QpRange(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* qp range */
    auto baseParam = params.getParam(ExynosParamIndex::QpRangeIndex);
    if (baseParam.get() == nullptr) {
        return;
    }

    auto param = std::static_pointer_cast<ExynosParam<ParamQpRange>>(baseParam);

    auto err = mCodecImpl->setQpRange(param->m);

    if (err == VIDEO_ERROR_NONE) {
        ExynosLogD("[%s] qp range is changed", __FUNCTION__);
    } else {
        ExynosLogE("[%s] Set_QpRange() is failed", __FUNCTION__);
    }
}

void ExynosVideoCodecEnc::applyConfig_OperatingRate(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* operating rate */
    auto baseParam = params.getParam(ExynosParamIndex::OperatingRateIndex);
    if (baseParam.get() != nullptr) {
        auto param = std::static_pointer_cast<ExynosParam<ParamOperatingRate>>(baseParam);

        auto err = mCodecImpl->setOperatingRate(param->m.value);

        if (err == VIDEO_ERROR_NONE) {
            ExynosLogD("[%s] operating rate is %d", __FUNCTION__, param->m.value);
        } else {
            ExynosLogE("[%s] Set_OperatingRate(%d) is failed", __FUNCTION__, param->m.value);
        }
    }
}

void ExynosVideoCodecEnc::applyConfig_RealTimePriority(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* realtime priority */
    auto baseParam = params.getParam(ExynosParamIndex::RealTimePriorityIndex);
    if (baseParam.get() == nullptr) {
        return;
    }

    auto param = std::static_pointer_cast<ExynosParam<ParamRealTimePriority>>(baseParam);

    auto err = mCodecImpl->setRealTimePriority(param->m.value);

    if (err == VIDEO_ERROR_NONE) {
        ExynosLogD("[%s] RealTime priority is %d", __FUNCTION__, param->m.value);
    } else {
        ExynosLogE("[%s] Set_RealTimePriority(%d) is failed", __FUNCTION__, param->m.value);
    }
}

void ExynosVideoCodecEnc::applyConfig_AverageQp(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* average qp */
    auto baseParam = params.getParam(ExynosParamIndex::AverageQpIndex);
    if (baseParam.get() != nullptr) {
        auto param = std::static_pointer_cast<ExynosParam<ParamAverageQp>>(baseParam);

        mCodecImpl->mIsAverageQp = (param->m.enable == 1)? true:false;
    }
}

void ExynosVideoCodecEnc::applyConfig_IFrameRatio(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* i-frame ratio */
    auto baseParam = params.getParam(ExynosParamIndex::IFrameRatioIndex);
    if (baseParam.get() != nullptr) {
        auto param = std::static_pointer_cast<ExynosParam<ParamIFrameRatio>>(baseParam);

        auto err = mCodecImpl->setIFrameRatio(param->m.value);

        if (err == VIDEO_ERROR_NONE) {
            ExynosLogD("[%s] I frame ratio is %d", __FUNCTION__, param->m.value);
        } else {
            ExynosLogE("[%s] Set_IFrameRatio(%d) is failed", __FUNCTION__, param->m.value);
        }
    }
}

void ExynosVideoCodecEnc::applyConfig_PMV(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* PMV */
    auto baseParam = params.getParam(ExynosParamIndex::PMVIndex);
    if (baseParam.get() != nullptr) {
        auto param = std::static_pointer_cast<ExynosParam<ParamPMV>>(baseParam);

        auto err = mCodecImpl->setPMV(param->m);

        if (err != VIDEO_ERROR_NONE) {
            ExynosLogE("[%s] Set_PMV() is failed", __FUNCTION__);
        }
    }
}

void ExynosVideoCodecEnc::applyConfig_IntraVopRefresh(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* intra VOP refresh */
    auto baseParam = params.getParam(ExynosParamIndex::IntraVOPRefreshIndex);
    if (baseParam.get() != nullptr) {
        auto param = std::static_pointer_cast<ExynosParam<ParamIntraVOPRefresh>>(baseParam);

        if (param->m.request == On) {
            auto err = mCodecImpl->setFrameType(VIDEO_FRAME_I);

            if (err == VIDEO_ERROR_NONE) {
                ExynosLogD("[%s] VOP refresh", __FUNCTION__);
            } else {
                ExynosLogE("[%s] Set_FrameType(VIDEO_FRAME_I) is failed", __FUNCTION__);
            }
        }
    }
}

void ExynosVideoCodecEnc::applyConfig_PrependHeaderMode(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* prepend header mode */
    auto baseParam = params.getParam(ExynosParamIndex::PrependHeaderModeIndex);
    if (baseParam.get() == nullptr) {
        return;
    }

    auto param = std::static_pointer_cast<ExynosParam<ParamPrependHeaderMode>>(baseParam);

    if (param->m.mode != PREPEND_HEADER_MODE_SPS_PPS_TO_IDR) {
        return;
    }

    auto err = mCodecImpl->enablePrependSpsPpsToIDR();

    if (err == VIDEO_ERROR_NONE) {
        ExynosLogD("[%s] prepend SPS/PPS to IDR", __FUNCTION__);
    } else {
        ExynosLogE("[%s] enablePrependSpsPpsToIDR() is failed", __FUNCTION__);
    }
}

void ExynosVideoCodecEnc::applyConfig_ColorAspects(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* Color Aspects */
    auto baseParam = params.getParam(ExynosParamIndex::ColorAspectsIndex);
    if (baseParam.get() == nullptr) {
        return;
    }

    auto param = std::static_pointer_cast<ExynosParam<ParamColorAspects>>(baseParam);

    /* convert value in RGBA case */
    if (mCodecImpl->mEncParam.common.FrameMap == VIDEO_COLORFORMAT_RGBA8888) {
        getColorAspectsForRGB(param->m.range, param->m.primaries, param->m.transfer, param->m.matrix);
    }

    auto err = mCodecImpl->setColorAspects(param->m.range, param->m.primaries,
                                           param->m.transfer, param->m.matrix);

    if (err == VIDEO_ERROR_NONE) {
        ExynosLogD("[%s] ColorAspects is changed(r:%d, p:%d, t:%d, m:%d)", __FUNCTION__,
                    param->m.range, param->m.primaries, param->m.transfer, param->m.matrix);
    } else {
        ExynosLogE("[%s] Set_ColorAspects(r:%d, p:%d, t:%d, m:%d) is failed", __FUNCTION__,
                    param->m.range, param->m.primaries, param->m.transfer, param->m.matrix);
    }
}

void ExynosVideoCodecEnc::applyConfig_DropControl(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* drop control */
    auto baseParam = params.getParam(ExynosParamIndex::DropControlIndex);
    if (baseParam.get() == nullptr) {
        return;
    }

    auto param = std::static_pointer_cast<ExynosParam<ParamDropControl>>(baseParam);

    mCodecImpl->mIsDropControl = (param->m.enable != 0)? true:false;
}

void ExynosVideoCodecEnc::applyConfig_Skype(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* skype features */
    if (mCodecImpl->mVideoInstInfo.eCodecType == VIDEO_CODING_AVC) {
        mCodecImpl->applySkypeConfig(params);
    }
}

void ExynosVideoCodecEnc::applyConfig_HDREncoding(ExynosParams &params) {
    ExynosLogFunctionTrace();

    auto baseParam = params.getParam(ExynosParamIndex::HDREncodingIndex);
    if (baseParam.get() == nullptr) {
        return;
    }

    auto param = std::static_pointer_cast<ExynosParam<ParamHdrEncoding>>(baseParam);

    auto err = mCodecImpl->enableHdrEncoding(param->m.type);

    if (err == VIDEO_ERROR_NONE) {
        ExynosLogD("[%s] hdr type : 0x%x", __FUNCTION__, mCodecImpl->mHdrEncodingType);
    }

    return;
}

void ExynosVideoCodecEnc::applyConfig_HDRStaticInfo(ExynosParams &params) {
    ExynosLogFunctionTrace();

    auto baseParam = params.getParam(ExynosParamIndex::HDRStaticInfoIndex);
    if (baseParam.get() == nullptr) {
        return;
    }

    auto param = std::static_pointer_cast<ExynosParam<ParamHdrStaticInfo>>(baseParam);

    if ((mCodecImpl->mVideoInstInfo.eCodecType == VIDEO_CODING_HEVC) &&
        (mCodecImpl->mInGeometry.eFilledDataType & DATA_10BIT) &&
        (mCodecImpl->mVideoInstInfo.supportInfo.enc.bHDRStaticInfoSupport == VIDEO_TRUE)) {
        auto baseParam = params.getParam(ExynosParamIndex::HDRStaticInfoIndex);
        if (baseParam.get() != nullptr) {
            ExynosLogD("[%s] has StaticInfo", __FUNCTION__);
            auto param = std::static_pointer_cast<ExynosParam<ParamHdrStaticInfo>>(baseParam);

            ExynosVideoHdrStatic STInfo;
            cnvHdrStaticInfotoVideoHdrStatic(STInfo, param->m.ST);

            auto err = mCodecImpl->setHDRStaticInfo(STInfo);
            if (err != VIDEO_ERROR_NONE) {
                ExynosLogW("[%s] setHDRStaticInfo() is failed", __FUNCTION__);
            }
        }
    }
}

void ExynosVideoCodecEnc::applyConfig_HDR10PlusInfo(ExynosParams &params) {
    ExynosLogFunctionTrace();

    auto baseParam = params.getParam(ExynosParamIndex::HDRDynamicInfoIndex);
    if (baseParam.get() == nullptr) {
        return;
    }

    auto param = std::static_pointer_cast<ExynosParam<ParamHdrDynamicInfo>>(baseParam);

    if ((mCodecImpl->mVideoInstInfo.eCodecType == VIDEO_CODING_HEVC) &&
        (mCodecImpl->mInGeometry.eFilledDataType & DATA_10BIT) &&
        (mCodecImpl->mVideoInstInfo.supportInfo.enc.bHDRDynamicInfoSupport == VIDEO_TRUE)) {
        auto baseParam = params.getParam(ExynosParamIndex::HDRDynamicInfoIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamHdrDynamicInfo>>(baseParam);

            ExynosLogD("[%s] has DynamicInfo", __FUNCTION__);

            if (mCodecImpl->mIsHdr10PlusStat) {
                ExynosLogD("[%s] postponed about handling hdr dynamic info", __FUNCTION__);
            } else {
                ExynosVideoHdrDynamic DYInfo;
                cnvHdrDynamicInfotoVideoHdrDynamic(DYInfo, param->m.DY);

                auto err = mCodecImpl->setHDRDynamicInfo(DYInfo);
                if (err != VIDEO_ERROR_NONE) {
                    ExynosLogW("[%s] setHDRDynamicInfo() is failed", __FUNCTION__);
                }
            }
        }
    }
}

void ExynosVideoCodecEnc::applyConfig_HDR10PlusStat(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* hdr10plus statistic*/
    auto baseParam = params.getParam(ExynosParamIndex::HDR10PlusStatIndex);
    if (baseParam.get() == nullptr) {
        return;
    }

    auto param = std::static_pointer_cast<ExynosParam<ParamHDR10PlusStat>>(baseParam);

    if (param->m.enable == On) {
        auto err = mCodecImpl->enableHDR10PlusStat();

        if (err == VIDEO_ERROR_NONE) {
            ExynosLogD("[%s] hdr10plus statistic is enable", __FUNCTION__);
        } else {
            ExynosLogE("[%s] Set_HDRDynamicStatInfo is failed", __FUNCTION__);
        }
    }
}

void ExynosVideoCodecEnc::applyConfig_SetMaxIFrameSize(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* hdr10plus statistic*/
    auto baseParam = params.getParam(ExynosParamIndex::MaxIFrameSizeIndex);
    if (baseParam.get() == nullptr) {
        return;
    }

    auto param = std::static_pointer_cast<ExynosParam<ParamMaxIFrameSize>>(baseParam);

    auto err = mCodecImpl->setMaxIFrameSize(param->m.size);
    if (err == VIDEO_ERROR_NONE) {
        ExynosLogD("[%s] max I frame size : %d", __FUNCTION__, param->m.size);
    } else {
        ExynosLogE("[%s] Set_MaxIFrameSize() is failed", __FUNCTION__);
    }
}

void ExynosVideoCodecEnc::applyConfig(ExynosParams &params) {
    ExynosLogFunctionTrace();

    if (params.empty()) {
        /* there is nothing to change */
        return;
    }

    auto handle = mCodecImpl->mHandle;

    if (handle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return;
    }

    /* dynamic configurations */
    /* bitrate */
    applyConfig_Bitrate(params);

    /* framerate */
    applyConfig_Framerate(params);

    /* IDR period */
    applyConfig_IdrPeriod(params);

    /* layering (Temporal SVC) */
    applyConfig_Layering(params);

    /* qp range */
    applyConfig_QpRange(params);

    /* operating rate */
    applyConfig_OperatingRate(params);

    /* realtime priority */
    applyConfig_RealTimePriority(params);

    /* average qp */
    applyConfig_AverageQp(params);

    /* i-frame ratio */
    applyConfig_IFrameRatio(params);

    /* PMV */
    applyConfig_PMV(params);

    /* intra VOP refresh */
    applyConfig_IntraVopRefresh(params);

    /* max I frame size */
    applyConfig_SetMaxIFrameSize(params);

    /* hdr10plus statistic
     * it must be decided before setting HDR static or HDR dynamic
     */
    applyConfig_HDR10PlusStat(params);

    /* it can not be applied during encoding */
    if (!isStreamOn(ExynosPort::Input)) {
        /* prepend header mode */
        applyConfig_PrependHeaderMode(params);

        /* Color Aspects */
        applyConfig_ColorAspects(params);

        applyConfig_HDREncoding(params);

        applyConfig_HDRStaticInfo(params);

        /* drop control */
        applyConfig_DropControl(params);
    }

    /* skype features */
    applyConfig_Skype(params);

    applyConfig_HDR10PlusInfo(params);
}

void ExynosVideoCodecEnc::getColorAspectsForRGB(
    int &range,
    int &primaries,
    int &transfer,
    int &matrix) {
    /* range must be set for VUI encoding */
    if (range == RANGE_UNSPECIFIED) {
        range = RANGE_LIMITED;
    }

    /* SMPTE_170M could be used at BT.601/BT.709/BT.2020
     * check primaries and matrix for deciding suitable transfer
     * because MFC D/D refers only transfer value.
     */
    if ((transfer == TRANSFER_UNSPECIFIED) ||
        (transfer == TRANSFER_SMPTE_170M)) {
        if ((primaries == PRIMARIES_BT601_6_625) ||
            (primaries == PRIMARIES_BT601_6_525)) {
            transfer  = TRANSFER_SMPTE_170M;
            matrix    = MATRIX_COEFF_SMPTE170M;
        } else if (matrix == MATRIX_COEFF_SMPTE170M) {
            primaries = PRIMARIES_BT601_6_625;
            transfer  = TRANSFER_SMPTE_170M;
            matrix    = MATRIX_COEFF_SMPTE170M;
        } else if ((primaries == PRIMARIES_BT709_5) ||
                   (matrix == MATRIX_COEFF_REC709)) {
            primaries = PRIMARIES_BT709_5;
            transfer  = TRANSFER_BT709;
            matrix    = MATRIX_COEFF_REC709;
#ifdef BOARD_SUPPORT_MFC_ENC_BT2020
        } else if ((primaries == PRIMARIES_BT2020) ||
                   (matrix == MATRIX_COEFF_BT2020) ||
                   (matrix == MATRIX_COEFF_BT2020_CONSTANT)) {
            primaries = PRIMARIES_BT2020;
            transfer  = TRANSFER_BT2020_1;
            matrix    = MATRIX_COEFF_BT2020;
#endif
        }
    }

    /* it is a restriction by GPU */
    {
        /* if color aspects is not decided,
         * set default values according to decision same as ColorUtils,
         * because GPU works like that.
         */
        if (transfer == TRANSFER_UNSPECIFIED) {
            auto dataspace = ExynosUtils::GetDefaultDataSpaceIfNeeded(mCodecImpl->mEncParam.common.SourceWidth,
                                                                      mCodecImpl->mEncParam.common.SourceHeight);
            int r = range;  /* do not override the range. the range is already set */
            ExynosUtils::GetColorAspectsFromDataspace(r, primaries, transfer, matrix, dataspace);

            if (ExynosUtils::CheckBT709(dataspace)) {
                transfer = TRANSFER_BT709;
            } else if (ExynosUtils::CheckBT2020(dataspace)) {
                transfer = TRANSFER_BT2020_1;

#ifndef BOARD_SUPPORT_MFC_ENC_BT2020
                /* uses BT.709 instead of BT.2020 */
                primaries = PRIMARIES_BT709_5;
                transfer  = TRANSFER_BT709;
                matrix    = MATRIX_COEFF_REC709;
#endif
            }
        }

        auto limit = ExynosUtils::GetSupportedDataspaceOnGPU();
        switch (limit) {
        case HAL_DATASPACE_BT709:
            if ((transfer == TRANSFER_BT2020_1) ||
                (transfer == TRANSFER_BT2020_2) ||
                (transfer == TRANSFER_ST2084) ||
                (transfer == TRANSFER_ST428) ||
                (transfer == TRANSFER_HLG)) {
                primaries = PRIMARIES_BT709_5;
                transfer  = TRANSFER_BT709;
                matrix    = MATRIX_COEFF_REC709;
            }
            break;
        case HAL_DATASPACE_BT601_625:
            primaries = PRIMARIES_BT601_6_625;
            transfer  = TRANSFER_SMPTE_170M;
            matrix    = MATRIX_COEFF_SMPTE170M;
            break;
        case HAL_DATASPACE_BT2020:
            /* nothing todo */
            [[fallthrough]];
        default:
            break;
        }
    }

    return;
}

// before setup
void ExynosVideoCodecEnc::applyExtraInfo_Config(ExynosBufferInfo &buf) {
    ExynosLogFunctionTrace();

    if ((buf.obj.get() == nullptr) ||
        (buf.obj->metadata() == nullptr)) {
        return;
    }

    ExynosVideoMeta *meta = buf.obj->metadata();

    if (meta->eType & VIDEO_INFO_TYPE_YSUM_DATA) {
        if (mCodecImpl->enableWeightedPrediction() == VIDEO_ERROR_NONE) {
            ExynosLogI("[%s] weighted prediction is enabled", __FUNCTION__);
            mCodecImpl->mIsWeightedPrediction = true;
        }
    }

    if ((meta->eType & VIDEO_INFO_TYPE_GDC_OTF) &&
        ((meta->data.enc.nUseGdcOTF == GDC_TYPE_VOTF) ||
         (meta->data.enc.nUseGdcOTF == GDC_TYPE_OTF)) &&
        (!mCodecImpl->mIsGDCvOTF)) {
        auto err = mCodecImpl->enableGDCvOTF(meta->data.enc.nUseGdcOTF);

        if ((err == VIDEO_ERROR_NONE) ||
            (err == VIDEO_ERROR_NOSUPPORT)) {
            ExynosLogD("[%s] GDC vOTF is enabled", __FUNCTION__,
                        (err == VIDEO_ERROR_NONE)? "enabled":"not supported");
            mCodecImpl->mIsGDCvOTF = true;

            auto err = mCodecImpl->setGDCvOTF((meta->data.enc.nIsGdcOTF == 1)? VIDEO_TRUE:VIDEO_FALSE);
            if (err != VIDEO_ERROR_NONE) {
                ExynosLogW("[%s] setGDCvOTF() is failed", __FUNCTION__);
            }
        } else {
            ExynosLogE("[%s] enableGDCvOTF() is failed", __FUNCTION__);
        }
    }

    if (meta->eType & VIDEO_INFO_TYPE_COLOR_ASPECTS) {
        /* it could be set by camera or csc filter */
        ExynosColorAspects *pCA = &(meta->sColorAspects);

        ExynosLogD("[%s] has ColorAspects(ISO:: r:%d, p:0x%x, t:0x%x, m:0x%x)", __FUNCTION__,
                    pCA->mRange, pCA->mPrimaries, pCA->mTransfer, pCA->mMatrixCoeffs);

        auto err = mCodecImpl->setColorAspects(pCA->mRange, pCA->mPrimaries, pCA->mTransfer, pCA->mMatrixCoeffs);
        if (err != VIDEO_ERROR_NONE) {
            ExynosLogW("[%s] setColorAspects() is failed", __FUNCTION__);
        }
    }

    if ((meta->eType & VIDEO_INFO_TYPE_HDR_STATIC) &&
        (mCodecImpl->mVideoInstInfo.eCodecType == VIDEO_CODING_HEVC) &&
        (mCodecImpl->mInGeometry.eFilledDataType & DATA_10BIT) &&
        (mCodecImpl->mVideoInstInfo.supportInfo.enc.bHDRStaticInfoSupport == VIDEO_TRUE)) {
        ExynosLogD("[%s] has StaticInfo", __FUNCTION__);

        ExynosVideoHdrStatic STInfo;
        cnvHdrStaticInfotoVideoHdrStatic(STInfo, meta->sHdrStaticInfo);

        auto err = mCodecImpl->setHDRStaticInfo(STInfo);
        if (err != VIDEO_ERROR_NONE) {
            ExynosLogW("[%s] setHDRStaticInfo() is failed", __FUNCTION__);
        }
    }

    if (meta->eType & VIDEO_INFO_TYPE_CROP_INFO) {
        ExynosVideoCrop &rect = meta->crop;

        ExynosVideoRect crop;
        crop.nTop = rect.top;
        crop.nLeft = rect.left;
        crop.nWidth = rect.width;
        crop.nHeight = rect.height;

        auto err = mCodecImpl->setCrop(crop);
        if (err == VIDEO_ERROR_NONE) {
            ExynosLogD("[%s] set crop(left:%d, top:%d, width:%d, height:%d)", __FUNCTION__,
                            crop.nLeft, crop.nTop, crop.nWidth, crop.nHeight);
        } else {
           ExynosLogW("[%s] setCrop() is failed", __FUNCTION__);
        }
    }

    return;
}

// after setup
void ExynosVideoCodecEnc::applyExtraInfo_Param(ExynosBufferInfo &buf) {
    ExynosLogFunctionTrace();

    if ((buf.obj.get() == nullptr) ||
        (buf.obj->metadata() == nullptr)) {
        return;
    }

    ExynosVideoMeta *meta = buf.obj->metadata();

    if ((meta->eType & VIDEO_INFO_TYPE_YSUM_DATA) &&
        (mCodecImpl->mIsWeightedPrediction)) {
        auto err = mCodecImpl->setYSumData(meta->data.enc.sYsumData.high, meta->data.enc.sYsumData.low);
        if (err != VIDEO_ERROR_NONE) {
            ExynosLogW("[%s] setYSumData() is failed", __FUNCTION__);
        }
    }

    if ((mCodecImpl->mIsGDCvOTF) &&
        (meta->eType & VIDEO_INFO_TYPE_GDC_OTF)) {
        auto err = mCodecImpl->setGDCvOTF((meta->data.enc.nIsGdcOTF == 1)? VIDEO_TRUE:VIDEO_FALSE);
        if (err != VIDEO_ERROR_NONE) {
            ExynosLogW("[%s] setGDCvOTF() is failed", __FUNCTION__);
        }
    }

    if ((meta->eType & VIDEO_INFO_TYPE_CHECK_PIXEL_FORMAT) &&
        (meta->nPixelFormat != 0)) {
        auto err = mCodecImpl->setActualFormat(getVideoFormat(meta->nPixelFormat));

        if (err == VIDEO_ERROR_NONE) {
            ExynosLogD("[%s] filled with non-compressed format(0x%x)", __FUNCTION__, meta->nPixelFormat);
        } else {
            ExynosLogW("[%s] setYSumData() is failed", __FUNCTION__);
        }
    }

    if ((meta->eType & VIDEO_INFO_TYPE_HDR_DYNAMIC) &&
        (mCodecImpl->mVideoInstInfo.eCodecType == VIDEO_CODING_HEVC) &&
        (mCodecImpl->mInGeometry.eFilledDataType & DATA_10BIT) &&
        (mCodecImpl->mVideoInstInfo.supportInfo.enc.bHDRDynamicInfoSupport == VIDEO_TRUE)) {
        if (meta->sHdrDynamicInfo.valid != 0) {
            ExynosLogD("[%s] has DynamicInfo", __FUNCTION__);
        }

        if (mCodecImpl->mIsHdr10PlusStat) {
            updateHDRDynamicInfoToParam(buf.params, meta->sHdrDynamicInfo);
        } else {
            ExynosVideoHdrDynamic DYInfo;
            cnvHdrDynamicInfotoVideoHdrDynamic(DYInfo, meta->sHdrDynamicInfo);

            auto err = mCodecImpl->setHDRDynamicInfo(DYInfo);
            if (err != VIDEO_ERROR_NONE) {
                ExynosLogW("[%s] setHDRDynamicInfo() is failed", __FUNCTION__);
            }
        }
    }

    if ((meta->eType & VIDEO_INFO_TYPE_ROI_INFO) &&
        (meta->data.enc.pRoiData != 0) &&
        (mCodecImpl->mVideoInstInfo.supportInfo.enc.bRoiInfoSupport == VIDEO_TRUE) &&
        ((mCodecImpl->mVideoInstInfo.eCodecType == VIDEO_CODING_AVC) ||
         (mCodecImpl->mVideoInstInfo.eCodecType == VIDEO_CODING_HEVC))) {
        ExynosVideoROIData *pROI = (ExynosVideoROIData *)meta->data.enc.pRoiData;

        RoiInfoBuffer info;
        memset(&info, 0, sizeof(info));

        info.pRoiMBInfo     = (unsigned long long)pROI->pRoiMBInfo;
        info.nRoiMBInfoSize = pROI->nRoiMBInfoSize;
        info.nLowerQpOffset = pROI->nLowerQpOffset;
        info.nUpperQpOffset = pROI->nUpperQpOffset;
        info.bUseRoiInfo    = (pROI->bUseRoiInfo == 1)? VIDEO_TRUE:VIDEO_FALSE;

        ExynosLogD("[%s] has ROI info(%s)", __FUNCTION__, (info.bUseRoiInfo == VIDEO_TRUE)? "valid":"invalid");

        auto err = mCodecImpl->setROIInfo(info);
        if (err != VIDEO_ERROR_NONE) {
            ExynosLogW("[%s] setROIInfo() is failed", __FUNCTION__);
        }

        pROI->bUseRoiInfo = 0;
    }

    return;
}

void ExynosVideoCodecEnc::applyExtraInfo(ExynosBufferInfo &buf) {
    ExynosLogFunctionTrace();

    if (!mExynosPort[ExynosPort::Input].mIsConfigured) {
        applyExtraInfo_Config(buf);
    } else {
        applyExtraInfo_Param(buf);
    }

    return;
}

void ExynosVideoCodecEnc::updateAverageQpInfoToParam(ExynosParams &params) {
    ExynosLogFunctionTrace();

    int qp = 0;

    auto handle = mCodecImpl->mHandle;
    if (!CHECK_POINTER(handle)) {
        return;
    }

    if (std::get<ExynosVideoEncOps>(mCodecImpl->mCommonOps).Get_AverageQp(handle, &qp) != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] Get_AverageQp() is failed", __FUNCTION__);
        return;
    }

    auto param = std::make_shared<ExynosParam<ParamAverageQpInfo>>();
    param->m.value = qp;

    params.addParam(param);

    ExynosLogV("[%s] average qp is %d", __FUNCTION__, param->m.value);

    return;
}

void ExynosVideoCodecEnc::updateHDR10PlusStatInfoToParam(ExynosParams &params) {
    ExynosLogFunctionTrace();

    ExynosVideoEncOps &ops = std::get<ExynosVideoEncOps>(mCodecImpl->mCommonOps);

    ExynosVideoHdrDynamicStat info;
    memset(&info, 0, sizeof(info));

    auto handle = mCodecImpl->mHandle;

    if (ops.Get_HDRDynamicStatInfo(handle, &info) != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] Get_HDRDynamicStatInfo() is failed", __FUNCTION__);
        return;
    }

    auto param = std::make_shared<ExynosParam<ParamHDR10PlusStatInfo>>();

    memcpy(&(param->m.info.sHdrDynamicStatInfo), &info.statistic_info, sizeof(ExynosHdrDynamicStatInfo));
    param->m.info.hdr10_plus_stat_sei_size = info.hdr10_plus_stat_sei_size;
    param->m.info.hdr10_plus_stat_offset = info.hdr10_plus_stat_offset;

    params.addParam(param);

    ExynosLogD("[%s] hdr10plus statistic info", __FUNCTION__);

    return;
}

void ExynosVideoCodecEnc::updateHDRDynamicInfoToParam(ExynosParams &params, ExynosHdrDynamicInfo &DY) {
    ExynosLogFunctionTrace();

    if (DY.valid != 0) {
        auto param = std::make_shared<ExynosParam<ParamHdrDynamicInfo>>();

        memcpy(&(param->m.DY), &DY, sizeof(param->m.DY));

        params.addParam(param);

        ExynosLogD("[%s] postponed about handling hdr dynamic info", __FUNCTION__);
    }

    return;
}

ExynosErrorType ExynosVideoCodecEnc::checkRealTimeResource(
    int32_t width,
    int32_t height,
    int32_t operatingRate) {
    ExynosLogFunctionTrace();

    return mCodecImpl->checkRealTimeResource(width, height, operatingRate);
}

int ExynosVideoCodecEnc::CodecSpecific_Set_FrameTag(std::shared_ptr<CodecImpl> codecImpl) {
    auto handle = codecImpl->mHandle;
    if (!CHECK_POINTER(handle)) {
        return -1;
    }

    std::get<ExynosVideoEncOps>(codecImpl->mCommonOps).Set_FrameTag(handle, codecImpl->mTagNum);
    return codecImpl->mTagNum;
}

int ExynosVideoCodecEnc::CodecSpecific_Get_FrameTag(std::shared_ptr<CodecImpl> codecImpl) {
    auto handle = codecImpl->mHandle;
    if (!CHECK_POINTER(handle)) {
        return -1;
    }

    return std::get<ExynosVideoEncOps>(codecImpl->mCommonOps).Get_FrameTag(handle);
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecSpecific_Stop_Wait_Buffer(std::shared_ptr<CodecImpl> codecImpl) {
    auto handle = codecImpl->mHandle;
    if (!CHECK_POINTER(handle)) {
        return VIDEO_ERROR_BADPARAM;
    }

    return std::get<ExynosVideoEncOps>(codecImpl->mCommonOps).Stop_Wait_Buffer(handle);
}

ExynosVideoErrorType ExynosVideoCodecEnc::CodecSpecific_Reset_Wait_Buffer(std::shared_ptr<CodecImpl> codecImpl) {
    auto handle = codecImpl->mHandle;
    if (!CHECK_POINTER(handle)) {
        return VIDEO_ERROR_BADPARAM;
    }

    return std::get<ExynosVideoEncOps>(codecImpl->mCommonOps).Reset_Wait_Buffer(handle);
}
