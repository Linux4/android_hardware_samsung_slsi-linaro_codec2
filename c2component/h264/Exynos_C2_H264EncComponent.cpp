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
#define __C2_GENERATE_GLOBAL_VARS__

#include <cmath>

#include "Exynos_C2_H264EncComponent.h"
#include "Exynos_H264Enc_Filter.h"
#include "Exynos_H264WFDEnc_Filter.h"
#include "Exynos_CSC_Filter.h"
#include "ExynosBufferAllocator.h"
#include "Exynos_C2_Skype.h"

#ifdef USE_GDC_FILTER
#include "Exynos_GDC_Filter.h"
#endif

#define LOG_ON
#include "ExynosLog.h"
#undef LOG_TAG
#define LOG_TAG "ExynosC2H264EncComponent"

constexpr char COMPONENT_NAME[] = "c2.exynos.h264.encoder";
constexpr char SECURE_COMPONENT_NAME[] = "c2.exynos.h264.encoder.secure";
constexpr char MEDIA_MIMETYPE_VIDEO_SECURE_AVC[] = "video/avc-wfd";

#define DEFAULT_B_FRAMES 0
#define MAX_B_FRAMES 2
#define MIN_QP 0
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
    {   "h264enc",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosExtentionFilter<ExynosH264EncFilter, ExynosH264WFDEncFilter>>,
#ifdef USE_BLOB_ALLOCATOR
        C2PlatformAllocatorStore::BLOB,
#else
        C2PlatformAllocatorStore::ION,
#endif
        C2MemoryUsage(DEFAULT_ENC_MEMORY_USAGE),
    },
};

ExynosC2FilterManager::FilterModuleInfoList listSecureFilterInfo = {
#ifdef USE_CSC_FILTER
    {   "csc.secure",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosCSCFilter>,
        C2VendorAllocatorStore::GRALLOC_INTERNAL,
        C2MemoryUsage(DEFAULT_SECURE_ENC_MEMORY_USAGE),
    },
#endif
    {   "h264enc.secure",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosExtentionFilter<ExynosH264EncFilter, ExynosH264WFDEncFilter>>,
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
    me.set().value = maxBframes;

    if (layering.v.m.bLayerCount > 0) {
        unsigned int reqDelay = (unsigned int)std::pow(2, layering.v.m.layerCount - 1);
        me.set().value = c2_max(me.v.value, reqDelay);
    }

    StaticExynosLog(Level::Debug, "H264EncParamIntf", "[%s] input delay(%d)", __FUNCTION__, me.v.value);
    return C2R::Ok();
}

static C2R BitrateModeSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosStreamBitrateModeTuning::output> &me,
                                const C2InterfaceHelper::C2P<C2ExynosPortVTCallSetting::output> &vtcall,
                                const C2InterfaceHelper::C2P<C2ExynosPortSkypeLowLatencySetting::output> &skypeLowLatency,
                                const C2InterfaceHelper::C2P<C2ExynosPortSkypeBitrateModeSetting::output> &skypeBitrateMode) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    if (vtcall.v.value == On) {
        me.set().value = (C2Config::bitrate_mode_t)VendorC2Config::BITRATE_VENDOR_CONST_VT_CALL;
    }

    if (skypeBitrateMode.v.value != VendorC2Config::SKYPE_BITRATE_MODE_NONE) {
        switch (skypeBitrateMode.v.value) {
        case VendorC2Config::SKYPE_BITRATE_MODE_IGNORE:
            me.set().value = C2Config::BITRATE_IGNORE;
            break;
        case VendorC2Config::SKYPE_BITRATE_MODE_VBR:
            me.set().value = C2Config::BITRATE_VARIABLE;
            break;
        case VendorC2Config::SKYPE_BITRATE_MODE_CBR:
            me.set().value = C2Config::BITRATE_CONST;
            break;
        case VendorC2Config::SKYPE_BITRATE_MODE_VBR_SKIP_FRAME:
            me.set().value = C2Config::BITRATE_VARIABLE_SKIP_ALLOWED;
            break;
        case VendorC2Config::SKYPE_BITRATE_MODE_CBR_SKIP_FRAME:
            me.set().value = C2Config::BITRATE_CONST_SKIP_ALLOWED;
            break;
        default:
            break;
        }
    }

    if (skypeLowLatency.v.enable == On) {
        me.set().value = (C2Config::bitrate_mode_t)BITRATE_IGNORE;
    }

    StaticExynosLog(Level::Debug, "H264EncParamIntf", "[%s] bitrate mode(0x%x)", __FUNCTION__, me.v.value);
    return res;
}

static C2R BitrateSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamBitrateInfo::output> &me,
                            const C2InterfaceHelper::C2P<C2ExynosPortSkypeBitrateModeSetting::output> &skypeBitrateMode) {
    (void)mayBlock;
    C2R res = C2R::Ok();
    if (me.v.value <= 4096) {
        me.set().value = 4096;
    }

    if ((skypeBitrateMode.v.value != VendorC2Config::SKYPE_BITRATE_MODE_NONE) &&
        (skypeBitrateMode.v.bitrate > 0)) {
        me.set().value = skypeBitrateMode.v.bitrate;
    }

    StaticExynosLog(Level::Debug, "H264EncParamIntf", "[%s] bitrate(%d)", __FUNCTION__, me.v.value);
    return res;
}

static C2R ProfileLevelSetter(
        bool mayBlock,
        C2InterfaceHelper::C2P<C2StreamProfileLevelInfo::output> &me,
        const C2InterfaceHelper::C2P<C2StreamPictureSizeInfo::input> &size,
        const C2InterfaceHelper::C2P<C2StreamFrameRateInfo::output> &frameRate,
        const C2InterfaceHelper::C2P<C2StreamBitrateInfo::output> &bitrate,
        const C2InterfaceHelper::C2P<C2ExynosPortCustomProfileLevelSetting::output> &skype) {
    (void)mayBlock;
    if (!me.F(me.v.profile).supportsAtAll(me.v.profile)) {
        me.set().profile = PROFILE_AVC_CONSTRAINED_BASELINE;
    }

    struct LevelLimits {
        C2Config::level_t level;
        float mbsPerSec;
        uint64_t mbs;
        uint32_t bitrate;
    };
    constexpr LevelLimits kLimits[] = {
        { LEVEL_AVC_1,       1485,     99,     64000 },
        { LEVEL_AVC_1B,      1485,     99,    128000 },
        { LEVEL_AVC_1_1,     3000,    396,    192000 },
        { LEVEL_AVC_1_2,     6000,    396,    384000 },
        { LEVEL_AVC_1_3,    11880,    396,    768000 },
        { LEVEL_AVC_2,      11880,    396,   2000000 },
        { LEVEL_AVC_2_1,    19800,    792,   4000000 },
        { LEVEL_AVC_2_2,    20250,   1620,   4000000 },
        { LEVEL_AVC_3,      40500,   1620,  10000000 },
        { LEVEL_AVC_3_1,   108000,   3600,  14000000 },
        { LEVEL_AVC_3_2,   216000,   5120,  20000000 },
        { LEVEL_AVC_4,     245760,   8192,  20000000 },
        { LEVEL_AVC_4_1,   245760,   8192,  50000000 },
        { LEVEL_AVC_4_2,   522240,   8704,  50000000 },
        { LEVEL_AVC_5,     589824,  22080, 135000000 },
        { LEVEL_AVC_5_1,   983040,  36864, 240000000 },
        { LEVEL_AVC_5_2,  2073600,  36864, 240000000 },
        { LEVEL_AVC_6,    4177920, 139264, 240000000 },
        { LEVEL_AVC_6_1,  8355840, 139264, 480000000 },
#if 0  /* not supported yet */
        { LEVEL_AVC_6_2, 16711680, 139264, 800000000 },
#endif
    };

    uint64_t mbs = uint64_t((size.v.width + 15) / 16) * ((size.v.height + 15) / 16);
    float mbsPerSec = float(mbs) * frameRate.v.value;

    // Check if the supplied level meets the MB / bitrate requirements. If
    // not, update the level with the lowest level meeting the requirements.

    bool found = false;
    bool needsUpdate = true;
    for (const LevelLimits &limit : kLimits) {
        StaticExynosLog(Level::Debug, "H264EncParamIntf", "[%s] mbs(%lld), bps(%f), bitrate(%d)",
                            __FUNCTION__, mbs, mbsPerSec, bitrate.v.value);
        if (mbs <= limit.mbs && mbsPerSec <= limit.mbsPerSec &&
                bitrate.v.value <= limit.bitrate) {
            // This is the lowest level that meets the requirements, and if
            // we haven't seen the supplied level yet, that means we don't
            // need the update.
            if (needsUpdate) {
                // ALOGD("Given level %x does not cover current configuration: "
                      // "adjusting to %x", me.v.level, limit.level);
                StaticExynosLog(Level::Essential, "H264EncParamIntf", "[%s] update a level(0x%x to 0x%x)",
                                    __FUNCTION__, me.v.level, limit.level);
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
        me.set().level = LEVEL_AVC_5_2;
    }

    SkypeParamIntf::SkypeProfileLevelSetter(mayBlock, me, skype);

    StaticExynosLog(Level::Debug, "H264EncParamIntf", "[%s] profile(0x%x), level(0x%x)", __FUNCTION__, me.v.profile, me.v.level);
    return C2R::Ok();
}

static C2R PictureQuantizationSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamPictureQuantizationTuning::output> &me) {
    return EncCommonSetter::PictureQuantizationSetter(mayBlock, me, MIN_QP, MAX_QP);
}

static C2R LLWFDModeSetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosPortLLWFDModeSetting::output>& me) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    StaticExynosLog(Level::Debug, "H264EncParamIntf", "[%s] LLWFD Mode is %s", __FUNCTION__,
                        (me.v.value == On)? "enabled":"disabled");
    return res;
}

class ExynosC2H264EncComponent::H264EncParamIntf : public SkypeParamIntf {
public:
    H264EncParamIntf(const std::shared_ptr<C2ReflectorHelper> &helper, bool isSecure = false)
        : SkypeParamIntf(helper,
                ((isSecure == true)? SECURE_COMPONENT_NAME:COMPONENT_NAME),
                C2Component::KIND_ENCODER,
                C2Component::DOMAIN_VIDEO,
                ((isSecure != true)? MEDIA_MIMETYPE_VIDEO_AVC:MEDIA_MIMETYPE_VIDEO_SECURE_AVC),
                ((isSecure != true)? listFilterInfo:listSecureFilterInfo)) {
        /* TODO */
        addEncodeDefaultParam({ 176, 144 }, { 32, 32 }, { 8192, 8192 });

        addActualFormatParam();

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
                            BitrateModeSetter, mVTCall, mSkypeLowLatency, mSkypeBitrateMode);

        addBitrateParamExt(mBitrate, 64000, 4096, 240000000, BitrateSetter, EncCommonCnv::cnvBitrate, mSkypeBitrateMode);

        addTemporalLayeringParam(mLayering, mBitrate,
                                 MAX_TEMPORAL_LAYERS, MAX_TEMPORAL_B_LAYERS,
                                 EncCommonSetter::LayeringSetter, cnvLayering);

        C2R (*gopSetter)(bool, C2P<C2StreamGopTuning::output> &) =
            [](bool mayBlock, C2P<C2StreamGopTuning::output> &me)->C2R {
                        return EncCommonSetter::GopSetter(mayBlock, me, MAX_B_FRAMES);
                    };

        addGopActualInputDelayParam(mGop, mActualInputDelay, gopSetter, cnvGop,
                                    DEFAULT_B_FRAMES, MAX_B_FRAMES, InputDelaySetter, mLayering);

        /* supported profile and level about bitstream */
        addParameter(
                DefineParam(mProfileLevel, C2_PARAMKEY_PROFILE_LEVEL)
                .withDefault(new C2StreamProfileLevelInfo::output(0u,
                        C2Config::PROFILE_AVC_HIGH, C2Config::LEVEL_AVC_5_2))
                .withFields({
                    C2F(mProfileLevel, profile).oneOf({
                            C2Config::PROFILE_AVC_CONSTRAINED_BASELINE,
                            C2Config::PROFILE_AVC_BASELINE,
                            C2Config::PROFILE_AVC_MAIN,
                            C2Config::PROFILE_AVC_CONSTRAINED_HIGH,
                            C2Config::PROFILE_AVC_HIGH}),
                    C2F(mProfileLevel, level).oneOf({
                            C2Config::LEVEL_AVC_1, C2Config::LEVEL_AVC_1B, C2Config::LEVEL_AVC_1_1,
                            C2Config::LEVEL_AVC_1_2, C2Config::LEVEL_AVC_1_3,
                            C2Config::LEVEL_AVC_2, C2Config::LEVEL_AVC_2_1, C2Config::LEVEL_AVC_2_2,
                            C2Config::LEVEL_AVC_3, C2Config::LEVEL_AVC_3_1, C2Config::LEVEL_AVC_3_2,
                            C2Config::LEVEL_AVC_4, C2Config::LEVEL_AVC_4_1, C2Config::LEVEL_AVC_4_2,
                            C2Config::LEVEL_AVC_5, C2Config::LEVEL_AVC_5_1, C2Config::LEVEL_AVC_5_2,
                            C2Config::LEVEL_AVC_6, C2Config::LEVEL_AVC_6_1,
                    })
                })
                .withSetter(ProfileLevelSetter, mStreamSize, mFrameRate, mBitrate, mSkypeCustomProfileLevel)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mProfileLevel)), cnvProfileLevel,
                                { mStreamSize, mFrameRate, mBitrate, mSkypeCustomProfileLevel });

        addCroppingScalingParam(8192, 8192, 8192, 8192, 2);

        /* extension features */
        {
            /* qp range */
            addQpRangeParam(5, 50, 5, 50, 5, 50, PictureQuantizationSetter);

            /* ref p frames */
            addRefPframesParam(mRefPFrames, EncCommonSetter::RefPframesSetter, cnvRefPframes);

            /* slice size */
            addParameter(
                    DefineParam(mSliceSize, C2EXYNOS_PARAMKEY_SLICE_SIZE)
                    .withDefault(new C2ExynosStreamSliceSizeSetting::output(0u, 0))
                    .withFields({ C2F(mSliceSize, value).any() })
                    .withSetter(Setter<decltype(*mSliceSize)>::StrictValueWithNoDeps)
                    .build());
            addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mSliceSize)), cnvSliceSize);

            /* i-frame ratio */
            addIFrameRatioParam(mIFrameRatio, cnvIFrameRatio);

            /* chroma qp offset */
            addChromaQpOffsetParam(mChromaQpOffset, EncCommonSetter::ChromaQpOffsetSetter, cnvChromaQpOffset);

            /* entropy mode */
            addParameter(
                    DefineParam(mEntropyMode, C2EXYNOS_PARAMKEY_ENTROPY_MODE)
                    .withDefault(new C2ExynosStreamEntropyModeSetting::output(0u, VendorC2Config::ENTROPY_MODE_CAVLC))
                    .withFields({
                        C2F(mEntropyMode, value).oneOf({
                            VendorC2Config::ENTROPY_MODE_CAVLC,
                            VendorC2Config::ENTROPY_MODE_CABAC,
                        })
                    })
                    .withSetter(Setter<decltype(*mEntropyMode)>::StrictValueWithNoDeps)
                    .build());
            addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mEntropyMode)), cnvEntropyMode);

            /* PMV */
            addPMV(true);

            addParameter(
                    DefineParam(mLLWFDMode, C2EXYNOS_PARAMKEY_LLWFD_MODE)
                    .withDefault(new C2ExynosPortLLWFDModeSetting::output(Off))
                    .withFields({C2F(mLLWFDMode, value).inRange(Off, On)})
                    .withSetter(LLWFDModeSetter)
                    .build());

            addMaxIFrameSize(mMaxIFrameSize, cnvMaxIFrameSize);  /* if component supports BITRATE_VENDOR_CONST_WFD */
        }
    }

    virtual ~H264EncParamIntf() = default;

    /* getter functions */
    std::shared_ptr<C2StreamGopTuning::output> getGop() { return mGop; }

    std::shared_ptr<C2PrependHeaderModeSetting> getPrependHeaderMode() { return mPrependHeaderMode; }

    std::shared_ptr<C2StreamTemporalLayeringTuning::output> getLayering() { return mLayering; }

    uint32_t getRefPframes() { return mRefPFrames->value; }

    uint32_t getSliceSize() { return mSliceSize->value; }

    uint32_t getIFrameRatio() { return mIFrameRatio->value; }

    std::shared_ptr<C2ExynosStreamChromaQpOffsetSetting::output> getChromaQpOffset() { return mChromaQpOffset; }

    uint32_t getEntropyMode() { return mEntropyMode->value; }

    bool getLLWFDMode() { return (mLLWFDMode->value == On)? true:false; }

    uint32_t getMaxIFrameSize() { return mMaxIFrameSize->value; }

    /* generate filter param */
    static FilterParamSetterRet cnvPrependHeaderMode(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<H264EncParamIntf>(intf);

        return EncCommonCnv::cnvPrependHeaderMode(intf, intfImpl->getPrependHeaderMode());
    }

    static FilterParamSetterRet cnvLayering(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<H264EncParamIntf>(intf);

        auto layering = intfImpl->getLayering();

        return EncCommonCnv::cnvLayeringCommon(intf, layering);
    }

    static FilterParamSetterRet cnvProfileLevel(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        struct H264Profile {
            C2Config::profile_t profile;
            uint32_t value;
        };

        constexpr H264Profile ProfileMap[] = {
            { C2Config::PROFILE_AVC_BASELINE, 0},
            { C2Config::PROFILE_AVC_CONSTRAINED_BASELINE, 1},
            { C2Config::PROFILE_AVC_MAIN, 2},
            { C2Config::PROFILE_AVC_HIGH, 4},
            { C2Config::PROFILE_AVC_CONSTRAINED_HIGH, 17},
        };

        struct H264Level {
            C2Config::level_t level;
            uint32_t value;
        };

        constexpr H264Level LevelMap[] = {
            { C2Config::LEVEL_AVC_1, 0},
            { C2Config::LEVEL_AVC_1B, 1},
            { C2Config::LEVEL_AVC_1_1, 2},
            { C2Config::LEVEL_AVC_1_2, 3},
            { C2Config::LEVEL_AVC_1_3, 4},
            { C2Config::LEVEL_AVC_2, 5},
            { C2Config::LEVEL_AVC_2_1, 6},
            { C2Config::LEVEL_AVC_2_2, 7},
            { C2Config::LEVEL_AVC_3, 8},
            { C2Config::LEVEL_AVC_3_1, 9},
            { C2Config::LEVEL_AVC_3_2, 10},
            { C2Config::LEVEL_AVC_4, 11},
            { C2Config::LEVEL_AVC_4_1, 12},
            { C2Config::LEVEL_AVC_4_2, 13},
            { C2Config::LEVEL_AVC_5, 14},
            { C2Config::LEVEL_AVC_5_1, 15},
            { C2Config::LEVEL_AVC_5_2, 16},
            { C2Config::LEVEL_AVC_6, 17},
            { C2Config::LEVEL_AVC_6_1, 18},
        };

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<H264EncParamIntf>(intf);

        auto id = intfImpl->findFilterID("enc");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamProfileLevel>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamProfileLevel>>(filterParam->getBaseParam());

            /* default */
            param->m.profile    = 1;  /* constrained baseline */
            param->m.level      = 15; /* 5.2 */

            auto profileLevel = intfImpl->getProfileLevelInfo();

            for (const H264Profile &element : ProfileMap) {
                if (profileLevel->profile == element.profile) {
                    param->m.profile = element.value;
                }
            }

            for (const H264Level &element : LevelMap) {
                if (profileLevel->level == element.level) {
                    param->m.level = element.value;
                }
            }

            StaticExynosLog(Level::Essential, "H264EncParamIntf", "[%s] profile(c2:0x%x / val:%d), level(c2:0x%x / val:%d)", __FUNCTION__,
                                profileLevel->profile, param->m.profile, profileLevel->level, param->m.level);

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
        }

        return ret;
    }

    static FilterParamSetterRet cnvRefPframes(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<H264EncParamIntf>(intf);

        return EncCommonCnv::cnvRefPframes(intf, intfImpl->getRefPframes());
    }

    static FilterParamSetterRet cnvSliceSize(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<H264EncParamIntf>(intf);

        auto size = intfImpl->getSliceSize();
        if (size > 0) {
            auto id = intfImpl->findFilterID("enc");
            if (id != 0) {  /* target filter is valid */
                auto filterParam = std::make_shared<ExynosFilterParam<ParamSliceSize>>();
                auto param       = std::static_pointer_cast<ExynosParam<ParamSliceSize>>(filterParam->getBaseParam());

                param->m.mode = SLICE_MODE_BYTES;
                param->m.size = size;

                StaticExynosLog(Level::Essential, "H264EncParamIntf", "[%s] slice mode(0x%x), size(%d)",
                                        __FUNCTION__, param->m.mode, param->m.size);

                filterParam->registTargetFilter(id);

                ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
            }
        }

        return ret;
    }

    static FilterParamSetterRet cnvIFrameRatio(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<H264EncParamIntf>(intf);

        return EncCommonCnv::cnvIFrameRatio(intf, intfImpl->getIFrameRatio());
    }

    static FilterParamSetterRet cnvChromaQpOffset(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<H264EncParamIntf>(intf);

        return EncCommonCnv::cnvChromaQpOffset(intf, intfImpl->getChromaQpOffset());
    }

    static FilterParamSetterRet cnvEntropyMode(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<H264EncParamIntf>(intf);

        auto id = intfImpl->findFilterID("enc");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamEntropyMode>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamEntropyMode>>(filterParam->getBaseParam());

            param->m.value = intfImpl->getEntropyMode();

            StaticExynosLog(Level::Essential, "H264EncParamIntf", "[%s] entropy mode : 0x%x",
                                __FUNCTION__, param->m.value);

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
        }

        return ret;
    }

    static FilterParamSetterRet cnvGop(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<H264EncParamIntf>(intf);

        return EncCommonCnv::cnvGop(intf, intfImpl->getGop());
    }

    static FilterParamSetterRet cnvMaxIFrameSize(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<H264EncParamIntf>(intf);

        return EncCommonCnv::cnvMaxIFrameSize(intf, (int)intfImpl->getMaxIFrameSize());
    }

private:
    std::shared_ptr<C2StreamGopTuning::output> mGop;
    std::shared_ptr<C2PrependHeaderModeSetting> mPrependHeaderMode;
    std::shared_ptr<C2StreamTemporalLayeringTuning::output> mLayering;

    /* extension features */
    std::shared_ptr<C2ExynosPortRefPframeSetting::output>        mRefPFrames;
    std::shared_ptr<C2ExynosStreamSliceSizeSetting::output>      mSliceSize;
    std::shared_ptr<C2ExynosPortVTCallSetting::output>           mVTCall;
    std::shared_ptr<C2ExynosStreamIFrameRatioTunning::output>    mIFrameRatio;
    std::shared_ptr<C2ExynosStreamChromaQpOffsetSetting::output> mChromaQpOffset;
    std::shared_ptr<C2ExynosStreamEntropyModeSetting::output>    mEntropyMode;
    std::shared_ptr<C2ExynosPortLLWFDModeSetting::output>        mLLWFDMode;
    std::shared_ptr<C2ExynosStreamMaxIFrameSizeTuning::output>   mMaxIFrameSize;
};

ExynosC2H264EncComponent::ExynosC2H264EncComponent(c2_node_id_t id, const std::shared_ptr<H264EncParamIntf> &intfImpl)
    : ExynosC2EncComponent(std::make_shared<ExynosC2ComponentInterface>(intfImpl->mName->m.value, id, intfImpl)) {
    mObjName    = "ExynosC2H264EncComponent";
    mbLogOff    = false;
    mParamIntf  = std::static_pointer_cast<CommonParamIntf>(intfImpl);

    mThreadPool->setObjName(mObjName);
    return;
}

bool ExynosC2H264EncComponent::init(std::weak_ptr<C2Component> wkComponent) {
    ExynosLogFunctionTrace();

    /* TODO : secure */
    mFilterManager = std::make_shared<ExynosC2FilterManager>(mObjName, wkComponent);

    {
        H264EncParamIntf::Lock lock = mParamIntf->lock();

        mFilter = mFilterManager->loadFilterModules((mParamIntf->mIsSecure == true)? listSecureFilterInfo:listFilterInfo);
    }

    return setCallback(mFilterManager, mFilterListener);
}

c2_status_t ExynosC2H264EncComponent::onSetup() {
    ExynosLogFunctionTrace();

    c2_status_t ret = C2_OK;

    ret = ExynosC2EncComponent::onSetup();
    if (ret != C2_OK) {
        return ret;
    }

    {
        H264EncParamIntf::Lock lock = mParamIntf->lock();

        auto h264EncIntf = std::static_pointer_cast<H264EncParamIntf>(mParamIntf);

        skypeSetup(mParamIntf, h264EncIntf->getSkypeInputControl(), h264EncIntf->getLayering());

        if (h264EncIntf->getLLWFDMode()) {
            auto name = std::string("h264enc");
            mFilterManager->changeToSubFilter(name);
        }
    }

    return ret;
}

class ExynosC2H264EncFactory : public C2ComponentFactory {
public:
    ExynosC2H264EncFactory(bool isSecure = false)
        : mIsSecure(isSecure),
          mHelper(std::static_pointer_cast<C2ReflectorHelper>(android::GetCodec2ExynosComponentStore()->getParamReflector())) {
        SetStaticExynosLogLevel();
    }

    c2_status_t createComponent(
        c2_node_id_t id,
        std::shared_ptr<C2Component> * const component,
        std::function<void(C2Component*)> deleter) override {
        using __component_type = ExynosC2ComponentWrapper<ExynosC2H264EncComponent,
                                                          ExynosC2H264EncComponent::H264EncParamIntf>;

        *component = std::shared_ptr<C2Component>(
                        new __component_type(id, std::make_shared<ExynosC2H264EncComponent::H264EncParamIntf>(mHelper, mIsSecure)), deleter);

        auto h264Component = std::static_pointer_cast<__component_type>(*component);

        h264Component->init((*component));

        return C2_OK;
    }

    c2_status_t createInterface(
        c2_node_id_t id,
        std::shared_ptr<C2ComponentInterface> * const interface,
        std::function<void(C2ComponentInterface*)> deleter) override {
        *interface = std::shared_ptr<C2ComponentInterface>(
                        new ExynosC2ComponentInterface(((mIsSecure == true)? SECURE_COMPONENT_NAME:COMPONENT_NAME),
                                                       id, std::make_shared<ExynosC2H264EncComponent::H264EncParamIntf>(mHelper, mIsSecure)), deleter);

        return C2_OK;
    }

    virtual ~ExynosC2H264EncFactory() override = default;

private:
    bool mIsSecure;
    std::shared_ptr<C2ReflectorHelper> mHelper;
};

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((cfi_canonical_jump_table))
EXYNOS_EXPORT_REF extern "C" ::C2ComponentFactory* CreateCodec2Factory() {
    return new ExynosC2H264EncFactory();
}

__attribute__((cfi_canonical_jump_table))
EXYNOS_EXPORT_REF extern "C" ::C2ComponentFactory* CreateSecureCodec2Factory() {
    return new ExynosC2H264EncFactory(true);
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

