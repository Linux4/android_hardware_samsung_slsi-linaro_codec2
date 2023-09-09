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

#include "Exynos_C2_Vp9EncComponent.h"
#include "Exynos_Vp9Enc_Filter.h"
#include "Exynos_CSC_Filter.h"
#include "ExynosBufferAllocator.h"

#ifdef USE_GDC_FILTER
#include "Exynos_GDC_Filter.h"
#endif

#define LOG_ON
#include "ExynosLog.h"
#undef LOG_TAG
#define LOG_TAG "ExynosC2Vp9EncComponent"

constexpr char COMPONENT_NAME[] = "c2.exynos.vp9.encoder";

#define MIN_QP 0   /* index */
#define MAX_QP 63  /* index */

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
    {   "vp9enc",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosVp9EncFilter>,
#ifdef USE_BLOB_ALLOCATOR
        C2PlatformAllocatorStore::BLOB,
#else
        C2PlatformAllocatorStore::ION,
#endif
        C2MemoryUsage(DEFAULT_ENC_MEMORY_USAGE),
    },
};


/* setter functions */
static C2R BitrateSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamBitrateInfo::output> &me) {
    (void)mayBlock;
    C2R res = C2R::Ok();
    if (me.v.value <= 4096) {
        me.set().value = 4096;
    }
    StaticExynosLog(Level::Debug, "Vp9EncParamIntf", "[%s] bitrate(%d)", __FUNCTION__, me.v.value);
    return res;
}

static C2R ProfileLevelSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamProfileLevelInfo::output> &me,
                                        const C2InterfaceHelper::C2P<C2ExynosCSCOutputPixelFormatInfo> &actualFormat,
                                        const C2InterfaceHelper::C2P<C2StreamHdrFormatInfo::output> &hdrFormat) {
    (void)mayBlock;
    if (!me.F(me.v.profile).supportsAtAll(me.v.profile)) {
        me.set().profile = PROFILE_VP9_0;
    }
    if (!me.F(me.v.level).supportsAtAll(me.v.level)) {
        me.set().level = LEVEL_VP9_4_1;
    }

    if ((hdrFormat.v.value != C2Config::hdr_format_t::UNKNOWN) &&
        (hdrFormat.v.value != C2Config::hdr_format_t::SDR)) {
        me.set().profile = PROFILE_VP9_2;
    }

#ifndef UNSUPPORT_10BIT
    if (ExynosUtils::Check10BitFormat(actualFormat.v.value)) {
        me.set().profile = PROFILE_VP9_2;
    }
#endif

    StaticExynosLog(Level::Debug, "Vp9EncParamIntf", "[%s] profile(0x%x), level(0x%x), format(0x%x)",
                                        __FUNCTION__, me.v.profile, me.v.level, actualFormat.v.value);
    return C2R::Ok();
}

static C2R PictureQuantizationSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamPictureQuantizationTuning::output> &me) {
    return EncCommonSetter::PictureQuantizationSetter(mayBlock, me, MIN_QP, MAX_QP);
}


class ExynosC2Vp9EncComponent::Vp9EncParamIntf : public ExynosC2EncComponent::VencCommonParamIntf {
public:
    Vp9EncParamIntf(const std::shared_ptr<C2ReflectorHelper> &helper)
        : VencCommonParamIntf(
                helper,
                COMPONENT_NAME,
                C2Component::KIND_ENCODER,
                C2Component::DOMAIN_VIDEO,
                MEDIA_MIMETYPE_VIDEO_VP9,
                listFilterInfo) {
        /* TODO */
        addEncodeDefaultParam({ 320, 240 }, { 64, 64 }, { 4096, 8192 });

#ifndef UNSUPPORT_10BIT
        addActualFormatParam(true);
#else
        addActualFormatParam();
#endif

        addBitrateModeParam({ /* nothing */ }, EncCommonSetter::BitrateModeSetter);

        addBitrateParam(mBitrate, 64000, 4096, 80000000, BitrateSetter, EncCommonCnv::cnvBitrate);

        addTemporalLayeringParam(mLayering, mBitrate,
                                 MAX_VPX_TEMPORAL_LAYERS, 0,
                                 EncCommonSetter::LayeringSetterVpx, cnvLayering);

        /* supported profile and level about bitstream */
        addParameter(
                DefineParam(mProfileLevel, C2_PARAMKEY_PROFILE_LEVEL)
                .withDefault(new C2StreamProfileLevelInfo::output(0u,
                        C2Config::PROFILE_VP9_0, C2Config::LEVEL_VP9_4_1))
                .withFields({
                    C2F(mProfileLevel, profile).oneOf({
                            C2Config::PROFILE_VP9_0,
#ifndef UNSUPPORT_10BIT
                            C2Config::PROFILE_VP9_2,
#endif
                    }),
                    C2F(mProfileLevel, level).oneOf({
                            /* main */
                            C2Config::LEVEL_VP9_1, C2Config::LEVEL_VP9_1_1,
                            C2Config::LEVEL_VP9_2, C2Config::LEVEL_VP9_2_1,
                            C2Config::LEVEL_VP9_3, C2Config::LEVEL_VP9_3_1,
                            C2Config::LEVEL_VP9_4, C2Config::LEVEL_VP9_4_1,
                            C2Config::LEVEL_VP9_5, C2Config::LEVEL_VP9_5_1, C2Config::LEVEL_VP9_5_2,
                    })
                })
                .withSetter(ProfileLevelSetter, mActualFormat, mHdrFormat)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mProfileLevel)), cnvProfileLevel,
                                { mActualFormat, mHdrFormat });

        /* HDR :: Static Info : it is handled by framework */
        addHdrDynamicInfo(mHdrDynamicInfoInput, mHdrDynamicInfoOutput);

        addCroppingScalingParam(4096, 8192, 4096, 8192, 2);

        /* extension feature : qp range */
        {
            /* qp range */
            addQpRangeParam(2, 63, 2, 63, 0, 0, PictureQuantizationSetter);

            /* ref p frames */
            addRefPframesParam(mRefPFrames, EncCommonSetter::RefPframesSetter, cnvRefPframes);

            /* PMV */
            addPMV(true);
        }
    }

    virtual ~Vp9EncParamIntf() = default;

    /* getter functions */
    std::shared_ptr<C2StreamTemporalLayeringTuning::output> getLayering() { return mLayering; }

    uint32_t getRefPframes() { return mRefPFrames->value; }

    /* generate filter param */
    static FilterParamSetterRet cnvLayering(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<Vp9EncParamIntf>(intf);

        auto layering = intfImpl->getLayering();

        return EncCommonCnv::cnvLayeringCommon(intf, layering);
    }

    static FilterParamSetterRet cnvProfileLevel(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        struct Vp9Profile {
            C2Config::profile_t profile;
            uint32_t value;
        };

        constexpr Vp9Profile ProfileMap[] = {
            { C2Config::PROFILE_VP9_0, 0},
            { C2Config::PROFILE_VP9_1, 1},
            { C2Config::PROFILE_VP9_2, 2},
            { C2Config::PROFILE_VP9_3, 3},
        };

        struct Vp9Level {
            C2Config::level_t level;
            uint32_t value;
        };

        constexpr Vp9Level LevelMap[] = {
            /* main */
            { C2Config::LEVEL_VP9_1, 10},
            { C2Config::LEVEL_VP9_1_1, 11},
            { C2Config::LEVEL_VP9_2, 20},
            { C2Config::LEVEL_VP9_2_1, 21},
            { C2Config::LEVEL_VP9_3, 30},
            { C2Config::LEVEL_VP9_3_1, 31},
            { C2Config::LEVEL_VP9_4, 40},
            { C2Config::LEVEL_VP9_4_1, 41},
            { C2Config::LEVEL_VP9_5, 50},
            { C2Config::LEVEL_VP9_5_1, 52},
            { C2Config::LEVEL_VP9_5_2, 52},
            { C2Config::LEVEL_VP9_6, 60},
            { C2Config::LEVEL_VP9_6_1, 61},
            { C2Config::LEVEL_VP9_6_2, 62},
        };

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<Vp9EncParamIntf>(intf);

        auto id = intfImpl->findFilterID("enc");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamProfileLevel>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamProfileLevel>>(filterParam->getBaseParam());

            /* default */
            param->m.profile    = 0;  /* 0 */
            param->m.level      = 41; /* 4.1 */

            auto profileLevel = intfImpl->getProfileLevelInfo();

            for (const Vp9Profile &element : ProfileMap) {
                if (profileLevel->profile == element.profile) {
                    param->m.profile = element.value;
                }
            }

            for (const Vp9Level &element : LevelMap) {
                if (profileLevel->level == element.level) {
                    param->m.level = element.value;
                }
            }

            StaticExynosLog(Level::Essential, "Vp9EncParamIntf", "[%s] profile(c2:0x%x / val:%d), level(c2:0x%x / val:%d)", __FUNCTION__,
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

        auto intfImpl = std::static_pointer_cast<Vp9EncParamIntf>(intf);

        return EncCommonCnv::cnvRefPframes(intf, intfImpl->getRefPframes());
    }

    std::shared_ptr<C2StreamHdrDynamicMetadataInfo::output> getHdrDynamicInfo() {
        return mHdrDynamicInfoOutput;
    }

private:
    std::shared_ptr<C2StreamTemporalLayeringTuning::output> mLayering;

    std::shared_ptr<C2StreamHdrDynamicMetadataInfo::input>   mHdrDynamicInfoInput;   /* from external */
    std::shared_ptr<C2StreamHdrDynamicMetadataInfo::output>  mHdrDynamicInfoOutput;  /* bypass */

    /* extension feature : ref p frames */
    std::shared_ptr<C2ExynosPortRefPframeSetting::output> mRefPFrames;
};

ExynosC2Vp9EncComponent::ExynosC2Vp9EncComponent(c2_node_id_t id, const std::shared_ptr<Vp9EncParamIntf> &intfImpl)
    : ExynosC2EncComponent(std::make_shared<ExynosC2ComponentInterface>(COMPONENT_NAME, id, intfImpl)) {
    mObjName    = "ExynosC2Vp9EncComponent";
    mbLogOff    = false;
    mParamIntf  = std::static_pointer_cast<CommonParamIntf>(intfImpl);

    mThreadPool->setObjName(mObjName);
    return;
}

bool ExynosC2Vp9EncComponent::init(std::weak_ptr<C2Component> wkComponent) {
    ExynosLogFunctionTrace();

    /* TODO : secure */
    mFilterManager = std::make_shared<ExynosC2FilterManager>(mObjName, wkComponent);

    mFilter = mFilterManager->loadFilterModules(listFilterInfo);

    return setCallback(mFilterManager, mFilterListener);
}

void ExynosC2Vp9EncComponent::onUpdateC2Config(
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

    auto vp9EncIntf = std::static_pointer_cast<Vp9EncParamIntf>(mParamIntf);
#if 0
    if ((buffer->mParams.get() != nullptr) &&
        (!buffer->mParams->empty())) {
        // Todo
    }
#endif
    if (c2buffer.get() != nullptr) {
        /* update some configs from inConfig */
        {
            for (const std::unique_ptr<C2Param> &param : inConfig) {
                if (param) {
                    C2StreamHdrDynamicMetadataInfo::input *hdrDynamicInfo =
                            C2StreamHdrDynamicMetadataInfo::input::From(param.get());

                    /* HDR Dynamic : passthrough from inConfig */
                    if (hdrDynamicInfo != nullptr) {
                        std::vector<std::unique_ptr<C2SettingResult>> failures;

                        std::unique_ptr<C2Param> outParam = C2Param::CopyAsStream(
                                                                *param.get(), true /*output*/, param->stream());

                        c2_status_t err = vp9EncIntf->config({ outParam.get() }, C2_MAY_BLOCK, &failures);
                        if (err == C2_OK) {
                            Vp9EncParamIntf::Lock lock = vp9EncIntf->lock();

                            auto hdrDynamicInfo = vp9EncIntf->getHdrDynamicInfo();

                            c2buffer->setInfo(hdrDynamicInfo);

                            /* for android.media.cts.EncoderTest#testVp9Hdr10PlusMetaData
                             * At this test cases, HDR10Plus metadata should be updated on
                             * output.configUpdate of C2Work.
                             */
                            outConfig.emplace_back(C2Param::Copy(*(hdrDynamicInfo.get())));

                            ExynosLogI("[%s] HDR :: dynamic info", __FUNCTION__);
                        } else {
                            ExynosLogE("[%s] config(C2StreamHdrDynamicMetadataInfo) is failed", __FUNCTION__);
                        }
                        break;
                    }
                }
            }
        }
    }

    return;
}


c2_status_t ExynosC2Vp9EncComponent::onSetup() {
    ExynosLogFunctionTrace();

    c2_status_t ret = C2_OK;

    ret = ExynosC2EncComponent::onSetup();
    if (ret != C2_OK) {
        return ret;
    }

    /* TODO */

    return ret;
}

class ExynosC2Vp9EncFactory : public C2ComponentFactory {
public:
    ExynosC2Vp9EncFactory()
        : mHelper(std::static_pointer_cast<C2ReflectorHelper>(android::GetCodec2ExynosComponentStore()->getParamReflector())) {
        SetStaticExynosLogLevel();
    }

    c2_status_t createComponent(
        c2_node_id_t id,
        std::shared_ptr<C2Component> * const component,
        std::function<void(C2Component*)> deleter) override {
        using __component_type = ExynosC2ComponentWrapper<ExynosC2Vp9EncComponent,
                                                          ExynosC2Vp9EncComponent::Vp9EncParamIntf>;

        *component = std::shared_ptr<C2Component>(
                        new __component_type(id, std::make_shared<ExynosC2Vp9EncComponent::Vp9EncParamIntf>(mHelper)), deleter);

        auto vp9Component = std::static_pointer_cast<__component_type>(*component);

        vp9Component->init((*component));

        return C2_OK;
    }

    c2_status_t createInterface(
        c2_node_id_t id,
        std::shared_ptr<C2ComponentInterface> * const interface,
        std::function<void(C2ComponentInterface*)> deleter) override {
        *interface = std::shared_ptr<C2ComponentInterface>(
                        new ExynosC2ComponentInterface(COMPONENT_NAME, id, std::make_shared<ExynosC2Vp9EncComponent::Vp9EncParamIntf>(mHelper)), deleter);

        return C2_OK;
    }

    virtual ~ExynosC2Vp9EncFactory() override = default;

private:
    std::shared_ptr<C2ReflectorHelper> mHelper;
};

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((cfi_canonical_jump_table))
EXYNOS_EXPORT_REF extern "C" ::C2ComponentFactory* CreateCodec2Factory() {
    return new ExynosC2Vp9EncFactory();
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

