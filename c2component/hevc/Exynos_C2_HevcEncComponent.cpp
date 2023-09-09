/*
 *
 * Copyright 2019 Samsung Electronics S.LSI Co. LTD
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
#define __C2_GENERATE_GLOBAL_VARS__

#include <cmath>

#include "Exynos_C2_HevcEncComponent.h"
#include "Exynos_HevcEnc_Filter.h"
#include "Exynos_HevcWFDEnc_Filter.h"
#include "Exynos_CSC_Filter.h"
#ifdef USE_HDR10PLUS_STAT_ENC_FILTER
#include "Exynos_HDR10PlusStatEnc_Filter.h"
#endif
#include "ExynosBufferAllocator.h"

#include "VendorVideoAPI.h"

#ifdef USE_GDC_FILTER
#include "Exynos_GDC_Filter.h"
#endif

#define LOG_ON
#include "ExynosLog.h"
#undef LOG_TAG
#define LOG_TAG "ExynosC2HevcEncComponent"

constexpr char COMPONENT_NAME[] = "c2.exynos.hevc.encoder";
constexpr char SECURE_COMPONENT_NAME[] = "c2.exynos.hevc.encoder.secure";
constexpr char MEDIA_MIMETYPE_VIDEO_SECURE_HEVC[] = "video/hevc-wfd";

#define DEFAULT_B_FRAMES 0
#define DEFAULT_RC_LOOKAHEAD 0

#define MAX_B_FRAMES 1
#define MAX_RC_LOOKAHEAD 1

#define MIN_QP 0  /* actual min : -12 */
#define MAX_QP 51

ExynosC2FilterManager::FilterModuleInfoList listFilterInfo = {
#ifdef USE_CSC_FILTER
    {   "csc",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosCSCFilter>,
        C2VendorAllocatorStore::GRALLOC_INTERNAL,
        C2MemoryUsage(DEFAULT_CSC_MEMORY_USAGE),
    },
#endif
#ifdef USE_GDC_FILTER
    {   "gdc",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosGDCFilter>,
        C2VendorAllocatorStore::GRALLOC_INTERNAL,
        C2MemoryUsage(DEFAULT_CSC_MEMORY_USAGE),
    },
#endif
    {   "hevcenc",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosExtentionFilter<ExynosHevcEncFilter, ExynosHevcWFDEncFilter>>,
#ifdef USE_BLOB_ALLOCATOR
        C2PlatformAllocatorStore::BLOB,
#else
        C2PlatformAllocatorStore::ION,
#endif
        C2MemoryUsage(DEFAULT_ENC_MEMORY_USAGE),
    },
#ifdef USE_HDR10PLUS_STAT_ENC_FILTER
    {   "hdr10plusstatenc",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosHDR10PlusStatEncFilter>,
#ifdef USE_BLOB_ALLOCATOR
        C2PlatformAllocatorStore::BLOB,
#else
        C2PlatformAllocatorStore::ION,
#endif
        C2MemoryUsage(DEFAULT_ENC_MEMORY_USAGE),
    },
#endif
};

ExynosC2FilterManager::FilterModuleInfoList listSecureFilterInfo = {
#ifdef USE_CSC_FILTER
    {   "csc.secure",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosCSCFilter>,
        C2VendorAllocatorStore::GRALLOC_INTERNAL,
        C2MemoryUsage(DEFAULT_SECURE_ENC_MEMORY_USAGE),
    },
#endif
    {   "hevcenc.secure",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosExtentionFilter<ExynosHevcEncFilter, ExynosHevcWFDEncFilter>>,
#ifdef USE_BLOB_ALLOCATOR
        C2PlatformAllocatorStore::BLOB,
#else
        C2PlatformAllocatorStore::ION,
#endif
        C2MemoryUsage(DEFAULT_SECURE_ENC_MEMORY_USAGE),
    },
};

/* setter functions */
static C2R InputDelaySetter(
        bool mayBlock,
        C2InterfaceHelper::C2P<C2PortActualDelayTuning::input> &me,
        const C2InterfaceHelper::C2P<C2StreamGopTuning::output> &gop,
        const C2InterfaceHelper::C2P<C2StreamTemporalLayeringTuning::output> &layering) {
    (void)mayBlock;
    uint32_t maxBframes = 0;
    ParseGop(gop.v, nullptr, nullptr, &maxBframes);
    me.set().value = maxBframes + DEFAULT_RC_LOOKAHEAD;

    if (layering.v.m.bLayerCount > 0) {
        unsigned int reqDelay = (unsigned int)std::pow(2, layering.v.m.layerCount - 1);
        me.set().value = c2_max(me.v.value, reqDelay);
    }

    StaticExynosLog(Level::Debug, "HevcEncParamIntf", "[%s] input delay(%d)", __FUNCTION__, me.v.value);
    return C2R::Ok();
}

static C2R BitrateModeSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosStreamBitrateModeTuning::output> &me,
                                const C2InterfaceHelper::C2P<C2ExynosPortVTCallSetting::output> &vtcall) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    if (vtcall.v.value == On) {
        me.set().value = (C2Config::bitrate_mode_t)VendorC2Config::BITRATE_VENDOR_CONST_VT_CALL;
    }

    StaticExynosLog(Level::Debug, "HevcEncParamIntf", "[%s] bitrate mode(0x%x)", __FUNCTION__, me.v.value);
    return res;
}

static C2R BitrateSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamBitrateInfo::output> &me) {
    (void)mayBlock;
    C2R res = C2R::Ok();
    if (me.v.value <= 4096) {
        me.set().value = 4096;
    }
    StaticExynosLog(Level::Debug, "HevcEncParamIntf", "[%s] bitrate(%d)", __FUNCTION__, me.v.value);
    return res;
}

static C2R ProfileLevelSetter(
    bool mayBlock,
    C2InterfaceHelper::C2P<C2StreamProfileLevelInfo::output> &me,
    const C2InterfaceHelper::C2P<C2StreamPictureSizeInfo::input> &size,
    const C2InterfaceHelper::C2P<C2StreamFrameRateInfo::output> &frameRate,
    const C2InterfaceHelper::C2P<C2StreamBitrateInfo::output> &bitrate,
    const C2InterfaceHelper::C2P<C2ExynosCSCOutputPixelFormatInfo> &actualFormat,
    const C2InterfaceHelper::C2P<C2StreamHdrFormatInfo::output> &hdrFormat) {
    (void)mayBlock;
    if (!me.F(me.v.profile).supportsAtAll(me.v.profile)) {
        me.set().profile = PROFILE_HEVC_MAIN;
    }

    if ((hdrFormat.v.value != C2Config::hdr_format_t::UNKNOWN) &&
        (hdrFormat.v.value != C2Config::hdr_format_t::SDR)) {
        me.set().profile = PROFILE_HEVC_MAIN_10;
    }

#ifndef UNSUPPORT_10BIT
    if (ExynosUtils::Check10BitFormat(actualFormat.v.value)) {
        me.set().profile = PROFILE_HEVC_MAIN_10;
    }
#endif

    struct LevelLimits {
        C2Config::level_t level;
        uint64_t samplesPerSec;
        uint64_t samples;
        uint32_t bitrate;
    };

    constexpr LevelLimits kLimits[] = {
        { LEVEL_HEVC_MAIN_1,       552960,    36864,    128000 },
        { LEVEL_HEVC_MAIN_2,      3686400,   122880,   1500000 },
        { LEVEL_HEVC_MAIN_2_1,    7372800,   245760,   3000000 },
        { LEVEL_HEVC_MAIN_3,     16588800,   552960,   6000000 },
        { LEVEL_HEVC_MAIN_3_1,   33177600,   983040,  10000000 },
        { LEVEL_HEVC_MAIN_4,     66846720,  2228224,  12000000 },
        { LEVEL_HEVC_MAIN_4_1,  133693440,  2228224,  20000000 },
        { LEVEL_HEVC_MAIN_5,    267386880,  8912896,  25000000 },
        { LEVEL_HEVC_MAIN_5_1,  534773760,  8912896,  40000000 },
        { LEVEL_HEVC_MAIN_5_2, 1069547520,  8912896,  60000000 },
        { LEVEL_HEVC_MAIN_6,   1069547520, 35651584,  60000000 },
        { LEVEL_HEVC_MAIN_6_1, 2139095040, 35651584, 120000000 },
        { LEVEL_HEVC_MAIN_6_2, 4278190080, 35651584, 240000000 },
    };

    uint64_t samples = (uint64_t)size.v.width * size.v.height;
    uint64_t samplesPerSec = samples * frameRate.v.value;

    // Check if the supplied level meets the MB / bitrate requirements. If
    // not, update the level with the lowest level meeting the requirements.

    bool found = false;
    // By default needsUpdate = false in case the supplied level does meet
    // the requirements.
    bool needsUpdate = false;
    for (const LevelLimits &limit : kLimits) {
        if (samples <= limit.samples && samplesPerSec <= limit.samplesPerSec &&
                bitrate.v.value <= limit.bitrate) {
            // This is the lowest level that meets the requirements, and if
            // we haven't seen the supplied level yet, that means we don't
            // need the update.
            if (needsUpdate) {
                StaticExynosLog(Level::Debug, "HevcEncParamIntf", "Given level %x does not cover current configuration: "
                                    "adjusting to %x", me.v.level, limit.level);
                me.set().level = limit.level;
            }
            found = true;
            break;
        }
        if (me.v.level == limit.level) {
            // We break out of the loop when the lowest feasible level is
            // found. The fact that we're here means that our level doesn't
            // meet the requirement and needs to be updated.
            needsUpdate = true;
        }
    }
    if (!found) {
        // We set to the highest supported level.
        me.set().level = LEVEL_HEVC_MAIN_5_2;
    }

    StaticExynosLog(Level::Debug, "HevcEncParamIntf", "[%s] profile(0x%x), level(0x%x), format(0x%x)",
                                        __FUNCTION__, me.v.profile, me.v.level, actualFormat.v.value);
    return C2R::Ok();
}

static C2R PictureQuantizationSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamPictureQuantizationTuning::output> &me) {
    return EncCommonSetter::PictureQuantizationSetter(mayBlock, me, MIN_QP, MAX_QP);
}

static C2R RefPframesSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosPortRefPframeSetting::output>& me,
                                    const C2InterfaceHelper::C2P<C2StreamTemporalLayeringTuning::output>& layering) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    if (layering.v.m.bLayerCount > 0) {
        me.set().value = 1;
    }

    StaticExynosLog(Level::Debug, "HevcEncParamIntf", "[%s] Ref. P frame(%d)", __FUNCTION__, me.v.value);
    return res;
}

static C2R GeneralPBSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosPortGeneralPBSetting::output>& me) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    StaticExynosLog(Level::Debug, "HevcEncParamIntf", "[%s] General PB is %s", __FUNCTION__,
                        (me.v.value == On)? "enabled":"disabled");
    return res;
}

#ifdef USE_HDR10PLUS_STAT_ENC_FILTER
static C2R Hdr10PlusStatEncSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosPortHDR10PlusStatEncSetting::output>& me,
                                        const C2InterfaceHelper::C2P<C2StreamHdrFormatInfo::output>& hdrFormat) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    if (hdrFormat.v.value == C2Config::hdr_format_t::HDR10_PLUS) {
        me.set().value = On;
    }

    StaticExynosLog(Level::Debug, "HevcEncParamIntf", "[%s] HDR10Plus Statisitc Encoding is %s", __FUNCTION__,
                        (me.v.value == On)? "enabled":"disabled");
    return res;
}
#endif

static C2R LLWFDModeSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosPortLLWFDModeSetting::output>& me) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    StaticExynosLog(Level::Debug, "HevcEncParamIntf", "[%s] LLWFD Mode is %s", __FUNCTION__,
                        (me.v.value == On)? "enabled":"disabled");
    return res;
}

class ExynosC2HevcEncComponent::HevcEncParamIntf : public ExynosC2EncComponent::VencCommonParamIntf {
public:
    HevcEncParamIntf(const std::shared_ptr<C2ReflectorHelper> &helper, bool isSecure = false)
        : VencCommonParamIntf(
                helper,
                ((isSecure == true)? SECURE_COMPONENT_NAME:COMPONENT_NAME),
                C2Component::KIND_ENCODER,
                C2Component::DOMAIN_VIDEO,
                ((isSecure != true)? MEDIA_MIMETYPE_VIDEO_HEVC:MEDIA_MIMETYPE_VIDEO_SECURE_HEVC),
                ((isSecure != true)? listFilterInfo:listSecureFilterInfo)) {
        /* TODO */
        addEncodeDefaultParam({ 196, 160 }, { 64, 64 }, { 8192, 8192 });

#ifndef UNSUPPORT_10BIT
        addActualFormatParam(true);
#else
        addActualFormatParam();
#endif

        addPrependHeaderModeParam(mPrependHeaderMode, cnvPrependHeaderMode);

        /* extension feature : vt call */
        {
            addParameter(
                    DefineParam(mVTCall, C2EXYNOS_PARAMKEY_VT_CALL)
                    .withDefault(new C2ExynosPortVTCallSetting::output(Off))
                    .withFields({ C2F(mVTCall, value).inRange(Off, On) })
                    .withSetter(Setter<decltype(*mVTCall)>::StrictValueWithNoDeps)
                    .build());
            addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mVTCall)), nullptr);
        }

        addBitrateModeParam({ C2Config::BITRATE_CONST_SKIP_ALLOWED,
                              VendorC2Config::BITRATE_VENDOR_CONST_VT_CALL,
                              VendorC2Config::BITRATE_VENDOR_CONST_WFD,
                              VendorC2Config::BITRATE_VENDOR_VARIABLE_BIT_SAVE },
                            BitrateModeSetter, mVTCall);

        addBitrateParam(mBitrate, 64000, 4096, 240000000, BitrateSetter, EncCommonCnv::cnvBitrate);

        addTemporalLayeringParam(mLayering, mBitrate,
                                 MAX_TEMPORAL_LAYERS, MAX_TEMPORAL_B_LAYERS,
                                 EncCommonSetter::LayeringSetter, cnvLayering);

        C2R (*gopSetter)(bool, C2P<C2StreamGopTuning::output> &) =
            [](bool mayBlock, C2P<C2StreamGopTuning::output> &me)->C2R {
                        return EncCommonSetter::GopSetter(mayBlock, me, MAX_B_FRAMES);
                    };

        addGopActualInputDelayParam(mGop, mActualInputDelay, gopSetter, cnvGop,
                                    (DEFAULT_B_FRAMES + DEFAULT_RC_LOOKAHEAD), (MAX_B_FRAMES + MAX_RC_LOOKAHEAD),
                                    InputDelaySetter, mLayering);

#if 0  /* decides a quality preset */
        addParameter(
                DefineParam(mComplexity, C2_PARAMKEY_COMPLEXITY)
                .withDefault(new C2StreamComplexityTuning::output(0u, 0))
                .withFields({C2F(mComplexity, value).inRange(0, 10)})
                .withSetter(Setter<decltype(*mComplexity)>::NonStrictValueWithNoDeps)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mComplexity)), cnvComplexity);
#endif
        /* quality for CQ(Constant Quality) bitrate mode(BITRATE_IGNORE) */
        addParameter(
                DefineParam(mQuality, C2_PARAMKEY_QUALITY)
                .withDefault(new C2StreamQualityTuning::output(0u, 57))
                .withFields({C2F(mQuality, value).inRange(0, 100)})
                .withSetter(Setter<decltype(*mQuality)>::NonStrictValueWithNoDeps)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mQuality)), cnvQuality);

        /* supported profile and level about bitstream */
        addParameter(
                DefineParam(mProfileLevel, C2_PARAMKEY_PROFILE_LEVEL)
                .withDefault(new C2StreamProfileLevelInfo::output(0u,
                        C2Config::PROFILE_HEVC_MAIN, C2Config::LEVEL_HEVC_MAIN_5_1))
                .withFields({
                    C2F(mProfileLevel, profile).oneOf({
                            C2Config::PROFILE_HEVC_MAIN,
#ifndef UNSUPPORT_10BIT
                            C2Config::PROFILE_HEVC_MAIN_10,
#endif
                    }),
                    C2F(mProfileLevel, level).oneOf({
                            /* main */
                            C2Config::LEVEL_HEVC_MAIN_1,
                            C2Config::LEVEL_HEVC_MAIN_2, C2Config::LEVEL_HEVC_MAIN_2_1,
                            C2Config::LEVEL_HEVC_MAIN_3, C2Config::LEVEL_HEVC_MAIN_3_1,
                            C2Config::LEVEL_HEVC_MAIN_4, C2Config::LEVEL_HEVC_MAIN_4_1,
                            C2Config::LEVEL_HEVC_MAIN_5, C2Config::LEVEL_HEVC_MAIN_5_1, C2Config::LEVEL_HEVC_MAIN_5_2,
                            C2Config::LEVEL_HEVC_MAIN_6,
                            /* high */
                            C2Config::LEVEL_HEVC_HIGH_4, C2Config::LEVEL_HEVC_HIGH_4_1,
                            C2Config::LEVEL_HEVC_HIGH_5, C2Config::LEVEL_HEVC_HIGH_5_1, C2Config::LEVEL_HEVC_HIGH_5_2,
                            C2Config::LEVEL_HEVC_HIGH_6,
                    })
                })
                .withSetter(ProfileLevelSetter, mStreamSize, mFrameRate, mBitrate, mActualFormat, mHdrFormat)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mProfileLevel)), cnvProfileLevel,
                                { mStreamSize, mFrameRate, mBitrate, mActualFormat, mHdrFormat });

        addHdrStaticInfo(mHdrStaticInfo, cnvHdrStaticInfo);
        addHdrDynamicInfo(mHdrDynamicInfoInput, mHdrDynamicInfoOutput, cnvHdrDynamicInfo);

        addCroppingScalingParam(8192, 8192, 8192, 8192, 2);

        /* extension features */
        {
            /* qp range */
            addQpRangeParam(5, 50, 5, 50, 5, 50, PictureQuantizationSetter);

            /* ref p frames */
            addRefPframesParam(mRefPFrames, RefPframesSetter, cnvRefPframes, mLayering);

            /* general PB */
            addParameter(
                    DefineParam(mGeneralPB, C2EXYNOS_PARAMKEY_GENERAL_PB)
                    .withDefault(new C2ExynosPortGeneralPBSetting::output(Off))
                    .withFields({ C2F(mGeneralPB, value).inRange(Off, On) })
                    .withSetter(GeneralPBSetter)
                    .build());
            addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mGeneralPB)), cnvGeneralPB);

            /* i-frame ratio */
            addIFrameRatioParam(mIFrameRatio, cnvIFrameRatio);

            /* chroma qp offset */
            addChromaQpOffsetParam(mChromaQpOffset, EncCommonSetter::ChromaQpOffsetSetter, cnvChromaQpOffset);

            /* PMV */
            addPMV(true);

#ifdef USE_HDR10PLUS_STAT_ENC_FILTER
            if (mIsSecure == false) {
                /* hdr10plus statistic encoding*/
                addParameter(
                        DefineParam(mHdr10PlusStatEnc, C2EXYNOS_PARAMKEY_HDR10PLUS_STAT_ENC)
                        .withDefault(new C2ExynosPortHDR10PlusStatEncSetting::output(Off))
                        .withFields({C2F(mHdr10PlusStatEnc, value).inRange(Off, On)})
                        .withSetter(Hdr10PlusStatEncSetter, mHdrFormat)
                        .build());
                addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mHdr10PlusStatEnc)), cnvHDR10PlusStatEnc,
                                        { mHdrFormat });
            }
#endif

            addParameter(
                    DefineParam(mLLWFDMode, C2EXYNOS_PARAMKEY_LLWFD_MODE)
                    .withDefault(new C2ExynosPortLLWFDModeSetting::output(Off))
                    .withFields({C2F(mLLWFDMode, value).inRange(Off, On)})
                    .withSetter(LLWFDModeSetter)
                    .build());

            addMaxIFrameSize(mMaxIFrameSize, cnvMaxIFrameSize);  /* if component supports BITRATE_VENDOR_CONST_WFD */
        }
    }

    virtual ~HevcEncParamIntf() = default;

    /* getter functions */
    std::shared_ptr<C2StreamGopTuning::output> getGop() { return mGop; }

    std::shared_ptr<C2PrependHeaderModeSetting> getPrependHeaderMode() { return mPrependHeaderMode; }

    std::shared_ptr<C2StreamTemporalLayeringTuning::output> getLayering() { return mLayering; }

    uint32_t getQuality() { return mQuality->value; }

    std::shared_ptr<C2StreamHdrStaticInfo::input> getHdrStaticInfo() { return mHdrStaticInfo; }

    std::shared_ptr<C2StreamHdrDynamicMetadataInfo::input> getHdrDynamicInfoInput() { return mHdrDynamicInfoInput; }
    std::shared_ptr<C2StreamHdrDynamicMetadataInfo::output> getHdrDynamicInfoOutput() { return mHdrDynamicInfoOutput; }

    uint32_t getRefPframes() { return mRefPFrames->value; }

    bool getGeneralPB() { return (mGeneralPB->value == On)? true:false; }

    uint32_t getIFrameRatio() { return mIFrameRatio->value; }

    bool getHDR10PlusStatEnc() const { return (mHdr10PlusStatEnc->value == On)? true:false; }

    bool getLLWFDMode() { return (mLLWFDMode->value == On)? true:false; }

    std::shared_ptr<C2ExynosStreamChromaQpOffsetSetting::output> getChromaQpOffset() { return mChromaQpOffset; }

    /* generate filter param */
    static FilterParamSetterRet cnvPrependHeaderMode(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<HevcEncParamIntf>(intf);

        return EncCommonCnv::cnvPrependHeaderMode(intf, intfImpl->getPrependHeaderMode());
    }

    uint32_t getMaxIFrameSize() { return mMaxIFrameSize->value; }

    static FilterParamSetterRet cnvBitrate(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        ret = EncCommonCnv::cnvBitrate(intf);

        /* update porifle & level */
        {
            auto param = cnvProfileLevel(intf);

            if (!param.empty()) {
                std::copy(param.begin(), param.end(),
                          std::inserter(ret, ret.end()));
            }
        }

        return ret;
    }

    static FilterParamSetterRet cnvLayering(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<HevcEncParamIntf>(intf);

        auto layering = intfImpl->getLayering();

        return EncCommonCnv::cnvLayeringCommon(intf, layering);
    }

    static FilterParamSetterRet cnvQuality(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<HevcEncParamIntf>(intf);

        auto id = intfImpl->findFilterID("enc");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamFrameQP>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamFrameQP>>(filterParam->getBaseParam());

            int32_t quality = intfImpl->getQuality();

            auto qp = [](int32_t quality)->uint32_t {
                          int qp;

                          /* Quality: 100 -> Qp : MIN_QP
                           * Quality: 0 -> Qp : MAX_QP
                           * Qp = ((MIN_QP - MAX_QP) * quality / 100) + MAX_QP;
                           */
                          qp = ((MIN_QP - MAX_QP) * quality / 100) + MAX_QP;
                          qp = std::min(qp, MAX_QP);
                          qp = std::max(qp, MIN_QP);

                          return (uint32_t)qp;
                      }(quality);

            param->m.I = qp;
            param->m.P = qp;
            param->m.B = qp;

            StaticExynosLog(Level::Essential, "HevcEncParamIntf", "[%s] QP(%u) based on quality(%u)",
                                __FUNCTION__, qp, quality);

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
        }

        return ret;
    }

    static FilterParamSetterRet cnvProfileLevel(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        struct HevcProfile {
            C2Config::profile_t profile;
            uint32_t value;
        };

        constexpr HevcProfile ProfileMap[] = {
            { C2Config::PROFILE_HEVC_MAIN, 0},
            { C2Config::PROFILE_HEVC_MAIN_10, 3},
        };

        struct HevcLevel {
            C2Config::level_t level;
            uint32_t value;
        };

        constexpr HevcLevel LevelMap[] = {
            /* main */
            { C2Config::LEVEL_HEVC_MAIN_1, 10},
            { C2Config::LEVEL_HEVC_MAIN_2, 20},
            { C2Config::LEVEL_HEVC_MAIN_2_1, 21},
            { C2Config::LEVEL_HEVC_MAIN_3, 30},
            { C2Config::LEVEL_HEVC_MAIN_3_1, 31},
            { C2Config::LEVEL_HEVC_MAIN_4, 40},
            { C2Config::LEVEL_HEVC_MAIN_4_1, 41},
            { C2Config::LEVEL_HEVC_MAIN_5, 50},
            { C2Config::LEVEL_HEVC_MAIN_5_1, 51},
            { C2Config::LEVEL_HEVC_MAIN_5_2, 52},
            { C2Config::LEVEL_HEVC_MAIN_6, 60},
            { C2Config::LEVEL_HEVC_MAIN_6_1, 61},
            { C2Config::LEVEL_HEVC_MAIN_6_2, 61},
            /* high */
            { C2Config::LEVEL_HEVC_HIGH_4, 40 + 0xFF},
            { C2Config::LEVEL_HEVC_HIGH_4_1, 41 + 0xFF},
            { C2Config::LEVEL_HEVC_HIGH_5, 50 + 0xFF},
            { C2Config::LEVEL_HEVC_HIGH_5_1, 51 + 0xFF},
            { C2Config::LEVEL_HEVC_HIGH_5_2, 52 + 0xFF},
            { C2Config::LEVEL_HEVC_HIGH_6, 60 + 0xFF},
            { C2Config::LEVEL_HEVC_HIGH_6_1, 61 + 0xFF},
            { C2Config::LEVEL_HEVC_HIGH_6_2, 61 + 0xFF},
        };

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<HevcEncParamIntf>(intf);

        auto id = intfImpl->findFilterID("enc");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamProfileLevel>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamProfileLevel>>(filterParam->getBaseParam());

            /* default */
            param->m.profile    = 0;  /* main */
            param->m.level      = 51; /* main tier 5.1 */

            auto profileLevel = intfImpl->getProfileLevelInfo();

            for (const HevcProfile &element : ProfileMap) {
                if (profileLevel->profile == element.profile) {
                    param->m.profile = element.value;
                }
            }

            for (const HevcLevel &element : LevelMap) {
                if (profileLevel->level == element.level) {
                    param->m.level = element.value;
                }
            }

            StaticExynosLog(Level::Essential, "HevcEncParamIntf", "[%s] profile(c2:0x%x / val:%d), level(c2:0x%x / val:%d)", __FUNCTION__,
                                profileLevel->profile, param->m.profile, profileLevel->level, param->m.level);

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
        }

        return ret;
    }

    static FilterParamSetterRet cnvHdrStaticInfo(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<HevcEncParamIntf>(intf);

        return EncCommonCnv::cnvHdrStaticInfo(intf, intfImpl->getHdrStaticInfo());
    }

    static FilterParamSetterRet cnvHdrDynamicInfo(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<HevcEncParamIntf>(intf);

        return EncCommonCnv::cnvHdrDynamicInfo(intf, intfImpl->getHdrDynamicInfoInput());
    }

    static FilterParamSetterRet cnvRefPframes(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<HevcEncParamIntf>(intf);

        return EncCommonCnv::cnvRefPframes(intf, intfImpl->getRefPframes());
    }

    static FilterParamSetterRet cnvGeneralPB(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<HevcEncParamIntf>(intf);

        auto id = intfImpl->findFilterID("enc");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamGeneralPB>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamGeneralPB>>(filterParam->getBaseParam());

            auto enable = intfImpl->getGeneralPB();

            param->m.enable = (enable)? 1:0;

            StaticExynosLog(Level::Essential, "HevcEncParamIntf", "[%s] General PB is %s", __FUNCTION__,
                                (enable)? "enabled":"disabled");

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
        }

        return ret;
    }

    static FilterParamSetterRet cnvIFrameRatio(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<HevcEncParamIntf>(intf);

        return EncCommonCnv::cnvIFrameRatio(intf, intfImpl->getIFrameRatio());
    }

    static FilterParamSetterRet cnvChromaQpOffset(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<HevcEncParamIntf>(intf);

        return EncCommonCnv::cnvChromaQpOffset(intf, intfImpl->getChromaQpOffset());
    }

    static FilterParamSetterRet cnvGop(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<HevcEncParamIntf>(intf);

        return EncCommonCnv::cnvGop(intf, intfImpl->getGop());
    }

    static FilterParamSetterRet cnvHDR10PlusStatEnc(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<HevcEncParamIntf>(intf);

        auto id = intfImpl->findFilterID("enc");
        if (id != 0) {  /* target filter is valid */
            if (intfImpl->getHDR10PlusStatEnc()) {
                auto filterParam = std::make_shared<ExynosFilterParam<ParamHDR10PlusStat>>();
                auto param       = std::static_pointer_cast<ExynosParam<ParamHDR10PlusStat>>(filterParam->getBaseParam());

                param->m.enable = 1;

                filterParam->registTargetFilter(id);

                ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
            }
        }
        return ret;
    }

    static FilterParamSetterRet cnvMaxIFrameSize(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<HevcEncParamIntf>(intf);

        return EncCommonCnv::cnvMaxIFrameSize(intf, (int)intfImpl->getMaxIFrameSize());
    }

private:
    std::shared_ptr<C2StreamGopTuning::output> mGop;
    std::shared_ptr<C2PrependHeaderModeSetting> mPrependHeaderMode;
    std::shared_ptr<C2StreamTemporalLayeringTuning::output> mLayering;
    std::shared_ptr<C2StreamQualityTuning::output> mQuality;
    std::shared_ptr<C2StreamHdrStaticInfo::input>  mHdrStaticInfo;   /* from external */
    std::shared_ptr<C2StreamHdrDynamicMetadataInfo::input>  mHdrDynamicInfoInput;   /* from external */
    std::shared_ptr<C2StreamHdrDynamicMetadataInfo::output> mHdrDynamicInfoOutput;  /* to client */

    /* extension features */
    std::shared_ptr<C2ExynosPortRefPframeSetting::output>        mRefPFrames;
    std::shared_ptr<C2ExynosPortGeneralPBSetting::output>        mGeneralPB;
    std::shared_ptr<C2ExynosPortVTCallSetting::output>           mVTCall;
    std::shared_ptr<C2ExynosStreamIFrameRatioTunning::output>    mIFrameRatio;
    std::shared_ptr<C2ExynosStreamChromaQpOffsetSetting::output> mChromaQpOffset;
    std::shared_ptr<C2ExynosPortHDR10PlusStatEncSetting::output> mHdr10PlusStatEnc;
    std::shared_ptr<C2ExynosPortLLWFDModeSetting::output>        mLLWFDMode;
    std::shared_ptr<C2ExynosStreamMaxIFrameSizeTuning::output>   mMaxIFrameSize;

#if 0 // TODO : TBD
    std::shared_ptr<C2StreamComplexityTuning::output>   mComplexity;
#endif
};

ExynosC2HevcEncComponent::ExynosC2HevcEncComponent(c2_node_id_t id, const std::shared_ptr<HevcEncParamIntf> &intfImpl)
    : ExynosC2EncComponent(std::make_shared<ExynosC2ComponentInterface>(intfImpl->mName->m.value, id, intfImpl)) {
    mObjName    = "ExynosC2HevcEncComponent";
    mbLogOff    = false;
    mParamIntf  = std::static_pointer_cast<CommonParamIntf>(intfImpl);

    mThreadPool->setObjName(mObjName);
    return;
}

bool ExynosC2HevcEncComponent::init(std::weak_ptr<C2Component> wkComponent) {
    ExynosLogFunctionTrace();

    /* TODO : secure */
    mFilterManager = std::make_shared<ExynosC2FilterManager>(mObjName, wkComponent);

    {
        HevcEncParamIntf::Lock lock = mParamIntf->lock();

        mFilter = mFilterManager->loadFilterModules((mParamIntf->mIsSecure == true)? listSecureFilterInfo:listFilterInfo);
    }

    return setCallback(mFilterManager, mFilterListener);
}

void ExynosC2HevcEncComponent::onUpdateC2Config(
    std::shared_ptr<C2Buffer>               c2buffer,
    std::shared_ptr<ExynosBuffer>           buffer,
    std::vector<std::unique_ptr<C2Param>>  &inConfig,
    std::vector<std::unique_ptr<C2Param>>  &outConfig) {
    ExynosLogFunctionTrace();

    if (buffer.get() == nullptr) {
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return;
    }

    if (mParamIntf.get() == nullptr) {
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return;
    }

    ExynosC2EncComponent::onUpdateC2Config(c2buffer, buffer, inConfig, outConfig);

    auto hevcEncIntf = std::static_pointer_cast<HevcEncParamIntf>(mParamIntf);

    auto filterParams = std::static_pointer_cast<ExynosFilterParams>(buffer->mParams);

    auto updateHDRDynamicMetadataInfo =
        [hevcEncIntf, c2buffer, &outConfig](auto param, bool gen = false) {
            if (c2buffer.get() == nullptr) {
                return;
            }

            std::vector<std::unique_ptr<C2SettingResult>> failures;

            c2_status_t err = hevcEncIntf->config({ param.get() }, C2_MAY_BLOCK, &failures);
            if (err == C2_OK) {
                HevcEncParamIntf::Lock lock = hevcEncIntf->lock();

                auto hdrDynamicInfo = hevcEncIntf->getHdrDynamicInfoOutput();

                c2buffer->setInfo(hdrDynamicInfo);

                /* for android.mediav2.cts.EncoderHDRInfoTest#testHDRMetadata
                 * At this test cases, HDR10Plus metadata should be updated on
                 * output.configUpdate of C2Work.
                 */
                outConfig.emplace_back(C2Param::Copy(*(hdrDynamicInfo.get())));

                StaticExynosLog(Level::Info, LOG_TAG, "[onUpdateC2Config] HDR :: dynamic info from %s", (gen)? "internal":"user");
            } else {
                StaticExynosLog(Level::Error, LOG_TAG, "[onUpdateC2Config] config(C2StreamHdrDynamicMetadataInfo) is failed");
            }
        };

    if ((buffer->mParams.get() != nullptr) &&
        (!buffer->mParams->empty())) {
        if (!(buffer->getFlags() & ExynosBuffer::REPLICA)) {
            /* HDR Dynamic : updated from internal */
            auto filterParam = filterParams->getParam(ParamHdrDynamicInfo::INDEX, TARGET_OWNER_COMPONENT);
            if (filterParam.get() != nullptr) {
                auto param = std::static_pointer_cast<ExynosParam<ParamHdrDynamicInfo>>(filterParam->getBaseParam());
                if (param->m.DY.valid != 0) {
                    char data[MAX_HDR10PLUS_SIZE] = { 0, };
                    int size = Exynos_dynamic_meta_to_itu_t_t35(&param->m.DY, (char *)data);
                    if (size > 0) {
                        auto hdrDynamicInfo = C2StreamHdrDynamicMetadataInfo::output::AllocShared(size, 0u,
                                                        C2Config::HDR_DYNAMIC_METADATA_TYPE_SMPTE_2094_40);

                        memcpy(hdrDynamicInfo->m.data, data, size);

                        updateHDRDynamicMetadataInfo(hdrDynamicInfo, true);
                    } else {
                        StaticExynosLog(Level::Error, LOG_TAG, "[%s] Exynos_dynamic_meta_to_itu_t_t35 is failed", __FUNCTION__);
                    }
                }
            }
        }
    }

    if (c2buffer.get() != nullptr) {
        /* update some configs from inConfig */
        {
            for (const std::unique_ptr<C2Param> &param : inConfig) {
                if (param) {
                    C2StreamHdrDynamicMetadataInfo::input *hdrDynamicInfo =
                            C2StreamHdrDynamicMetadataInfo::input::From(param.get());

                    /* HDR Dynamic : passthrough from inConfig */
                    if (hdrDynamicInfo != nullptr) {
                        auto outParam = C2Param::CopyAsStream(*param.get(), true /*output*/, param->stream());

                        updateHDRDynamicMetadataInfo(std::forward<decltype(outParam)>(outParam));
                        break;
                    }
                }
            }
        }
    }

    return;
}

c2_status_t ExynosC2HevcEncComponent::onSetup() {
    ExynosLogFunctionTrace();

    c2_status_t ret = C2_OK;

    ret = ExynosC2EncComponent::onSetup();
    if (ret != C2_OK) {
        return ret;
    }

    {
        HevcEncParamIntf::Lock lock = mParamIntf->lock();

        auto hevcEncIntf = std::static_pointer_cast<HevcEncParamIntf>(mParamIntf);

        if (hevcEncIntf->getLLWFDMode()) {
            auto name = std::string("hevcenc");
            mFilterManager->changeToSubFilter(name);
        }
    }

    return ret;
}

class ExynosC2HevcEncFactory : public C2ComponentFactory {
public:
    ExynosC2HevcEncFactory(bool isSecure = false)
        : mIsSecure(isSecure),
          mHelper(std::static_pointer_cast<C2ReflectorHelper>(android::GetCodec2ExynosComponentStore()->getParamReflector())) {
        SetStaticExynosLogLevel();
    }

    c2_status_t createComponent(
        c2_node_id_t id,
        std::shared_ptr<C2Component> * const component,
        std::function<void(C2Component*)> deleter) override {
        using __component_type = ExynosC2ComponentWrapper<ExynosC2HevcEncComponent,
                                                          ExynosC2HevcEncComponent::HevcEncParamIntf>;

        *component = std::shared_ptr<C2Component>(
                        new __component_type(id, std::make_shared<ExynosC2HevcEncComponent::HevcEncParamIntf>(mHelper, mIsSecure)), deleter);

        auto hevcComponent = std::static_pointer_cast<__component_type>(*component);

        hevcComponent->init((*component));

        return C2_OK;
    }

    c2_status_t createInterface(
        c2_node_id_t id,
        std::shared_ptr<C2ComponentInterface> * const interface,
        std::function<void(C2ComponentInterface*)> deleter) override {
        *interface = std::shared_ptr<C2ComponentInterface>(
                        new ExynosC2ComponentInterface(((mIsSecure == true)? SECURE_COMPONENT_NAME:COMPONENT_NAME),
                                                       id, std::make_shared<ExynosC2HevcEncComponent::HevcEncParamIntf>(mHelper, mIsSecure)), deleter);

        return C2_OK;
    }

    virtual ~ExynosC2HevcEncFactory() override = default;

private:
    bool mIsSecure;
    std::shared_ptr<C2ReflectorHelper> mHelper;
};

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((cfi_canonical_jump_table))
EXYNOS_EXPORT_REF extern "C" ::C2ComponentFactory* CreateCodec2Factory() {
    return new ExynosC2HevcEncFactory();
}

__attribute__((cfi_canonical_jump_table))
EXYNOS_EXPORT_REF extern "C" ::C2ComponentFactory* CreateSecureCodec2Factory() {
    return new ExynosC2HevcEncFactory(true);
}

__attribute__((cfi_canonical_jump_table))
EXYNOS_EXPORT_REF extern "C" void DestroyCodec2Factory(::C2ComponentFactory *factory) {
    if (factory != nullptr) {
        delete factory;
    }
}

#ifdef __cplusplus
}
#endif

