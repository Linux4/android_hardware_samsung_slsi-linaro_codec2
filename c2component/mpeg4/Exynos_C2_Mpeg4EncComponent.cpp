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

#include "Exynos_C2_Mpeg4EncComponent.h"
#include "Exynos_Mpeg4Enc_Filter.h"
#include "Exynos_CSC_Filter.h"
#include "ExynosBufferAllocator.h"

#ifdef USE_GDC_FILTER
#include "Exynos_GDC_Filter.h"
#endif

#define LOG_ON
#include "ExynosLog.h"
#undef LOG_TAG
#define LOG_TAG "ExynosC2Mpeg4EncComponent"

constexpr char COMPONENT_NAME[] = "c2.exynos.mpeg4.encoder";

#define DEFAULT_B_FRAMES 0
#define MAX_B_FRAMES 2
#define MIN_QP 1
#define MAX_QP 31

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
    {   "mpeg4enc",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosMpeg4EncFilter>,
#ifdef USE_BLOB_ALLOCATOR
        C2PlatformAllocatorStore::BLOB,
#else
        C2PlatformAllocatorStore::ION,
#endif
        C2MemoryUsage(DEFAULT_ENC_MEMORY_USAGE),
    },
};


/* setter functions */
static C2R InputDelaySetter(
        bool mayBlock,
        C2InterfaceHelper::C2P<C2PortActualDelayTuning::input> &me,
        const C2InterfaceHelper::C2P<C2StreamGopTuning::output> &gop) {
    (void)mayBlock;
    uint32_t maxBframes = 0;
    ParseGop(gop.v, nullptr, nullptr, &maxBframes);
    me.set().value = maxBframes;

    StaticExynosLog(Level::Debug, "Mpeg4EncParamIntf", "[%s] input delay(%d)", __FUNCTION__, me.v.value);
    return C2R::Ok();
}

static C2R BitrateSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamBitrateInfo::output> &me) {
    (void)mayBlock;
    C2R res = C2R::Ok();
    if (me.v.value <= 4096) {
        me.set().value = 4096;
    }
    StaticExynosLog(Level::Debug, "Mpeg4EncParamIntf", "[%s] bitrate(%d)", __FUNCTION__, me.v.value);
    return res;
}

static C2R ProfileLevelSetter(
        bool mayBlock,
        C2InterfaceHelper::C2P<C2StreamProfileLevelInfo::output> &me) {
    (void)mayBlock;
    if (!me.F(me.v.profile).supportsAtAll(me.v.profile)) {
        me.set().profile = C2Config::PROFILE_MP4V_SIMPLE;
    }

    if (!me.F(me.v.level).supportsAtAll(me.v.level)) {
        me.set().level = C2Config::LEVEL_MP4V_2;
    }

    StaticExynosLog(Level::Debug, "Mpeg4EncParamIntf", "[%s] profile(0x%x), level(0x%x)", __FUNCTION__, me.v.profile, me.v.level);

    return C2R::Ok();
}

static C2R PictureQuantizationSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamPictureQuantizationTuning::output> &me) {
    return EncCommonSetter::PictureQuantizationSetter(mayBlock, me, MIN_QP, MAX_QP);
}

class ExynosC2Mpeg4EncComponent::Mpeg4EncParamIntf : public ExynosC2EncComponent::VencCommonParamIntf {
public:
    Mpeg4EncParamIntf(const std::shared_ptr<C2ReflectorHelper> &helper)
        : VencCommonParamIntf(
                helper,
                COMPONENT_NAME,
                C2Component::KIND_ENCODER,
                C2Component::DOMAIN_VIDEO,
                MEDIA_MIMETYPE_VIDEO_MPEG4,
                listFilterInfo) {
        /* TODO */
        addEncodeDefaultParam({ 320, 240 }, { 32, 32 }, { 2048, 2048 });

        addActualFormatParam();

        C2R (*gopSetter)(bool, C2P<C2StreamGopTuning::output> &) =
            [](bool mayBlock, C2P<C2StreamGopTuning::output> &me)->C2R {
                        return EncCommonSetter::GopSetter(mayBlock, me, MAX_B_FRAMES);
                    };

        addGopActualInputDelayParam(mGop, mActualInputDelay, gopSetter, cnvGop,
                                    DEFAULT_B_FRAMES, MAX_B_FRAMES, InputDelaySetter);

        addBitrateModeParam({ /* nothing */ }, EncCommonSetter::BitrateModeSetter);

        addBitrateParam(mBitrate, 64000, 4096, 80000000, BitrateSetter, EncCommonCnv::cnvBitrate);

        /* supported profile and level about bitstream */
        addParameter(
                DefineParam(mProfileLevel, C2_PARAMKEY_PROFILE_LEVEL)
                .withDefault(new C2StreamProfileLevelInfo::output(0u,
                        C2Config::PROFILE_MP4V_SIMPLE, C2Config::LEVEL_MP4V_2))
                .withFields({
                    C2F(mProfileLevel, profile).oneOf({
                            C2Config::PROFILE_MP4V_SIMPLE,
                            C2Config::PROFILE_MP4V_ADVANCED_SIMPLE}),
                    C2F(mProfileLevel, level).oneOf({
                            C2Config::LEVEL_MP4V_0, C2Config::LEVEL_MP4V_0B,
                            C2Config::LEVEL_MP4V_1,
                            C2Config::LEVEL_MP4V_2,
                            C2Config::LEVEL_MP4V_3, C2Config::LEVEL_MP4V_3B,
                            C2Config::LEVEL_MP4V_4, C2Config::LEVEL_MP4V_4A,
                            C2Config::LEVEL_MP4V_5
                    })
                })
                .withSetter(ProfileLevelSetter)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mProfileLevel)), cnvProfileLevel);

        addCroppingScalingParam(2048, 2048, 2048, 2048, 2);

        /* extension feature */
        {
            /* qp range */
            addQpRangeParam(3, 30, 3, 30, 3, 30, PictureQuantizationSetter);

            /* PMV */
            addPMV();
        }
    }

    virtual ~Mpeg4EncParamIntf() = default;

    /* getter functions */
    std::shared_ptr<C2StreamGopTuning::output> getGop() { return mGop; }

    /* generate filter param */
    static FilterParamSetterRet cnvProfileLevel(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        struct Mpeg4Profile {
            C2Config::profile_t profile;
            uint32_t value;
        };

        constexpr Mpeg4Profile ProfileMap[] = {
            { C2Config::PROFILE_MP4V_SIMPLE, 0},
            { C2Config::PROFILE_MP4V_ADVANCED_SIMPLE, 1},
        };

        struct Mpeg4Level {
            C2Config::level_t level;
            uint32_t value;
        };

        constexpr Mpeg4Level LevelMap[] = {
            { C2Config::LEVEL_MP4V_0, 0},
            { C2Config::LEVEL_MP4V_0B, 1},
            { C2Config::LEVEL_MP4V_1, 2},
            { C2Config::LEVEL_MP4V_2, 3},
            { C2Config::LEVEL_MP4V_3, 4},
            { C2Config::LEVEL_MP4V_3B, 5},
            { C2Config::LEVEL_MP4V_4, 6},
            { C2Config::LEVEL_MP4V_4A, 6},
            { C2Config::LEVEL_MP4V_5, 7},
            { C2Config::LEVEL_MP4V_6, 8},
        };

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<Mpeg4EncParamIntf>(intf);

        auto id = intfImpl->findFilterID("enc");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamProfileLevel>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamProfileLevel>>(filterParam->getBaseParam());

            /* default */
            param->m.profile    = 0;  /* Simple */
            param->m.level      = 6; /* 4 */

            auto profileLevel = intfImpl->getProfileLevelInfo();

            for (const Mpeg4Profile &element : ProfileMap) {
                if (profileLevel->profile == element.profile) {
                    param->m.profile = element.value;
                }
            }

            for (const Mpeg4Level &element : LevelMap) {
                if (profileLevel->level == element.level) {
                    param->m.level = element.value;
                }
            }

            StaticExynosLog(Level::Essential, "Mpeg4EncParamIntf", "[%s] profile(c2:0x%x / val:%d), level(c2:0x%x / val:%d)", __FUNCTION__,
                                profileLevel->profile, param->m.profile, profileLevel->level, param->m.level);

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

        auto intfImpl = std::static_pointer_cast<Mpeg4EncParamIntf>(intf);

        return EncCommonCnv::cnvGop(intf, intfImpl->getGop());
    }

private:
    std::shared_ptr<C2StreamGopTuning::output> mGop;
};

ExynosC2Mpeg4EncComponent::ExynosC2Mpeg4EncComponent(c2_node_id_t id, const std::shared_ptr<Mpeg4EncParamIntf> &intfImpl)
    : ExynosC2EncComponent(std::make_shared<ExynosC2ComponentInterface>(COMPONENT_NAME, id, intfImpl)) {
    mObjName    = "ExynosC2Mpeg4EncComponent";
    mbLogOff    = false;
    mParamIntf  = std::static_pointer_cast<CommonParamIntf>(intfImpl);

    mThreadPool->setObjName(mObjName);
    return;
}

bool ExynosC2Mpeg4EncComponent::init(std::weak_ptr<C2Component> wkComponent) {
    ExynosLogFunctionTrace();

    /* TODO : secure */
    mFilterManager = std::make_shared<ExynosC2FilterManager>(mObjName, wkComponent);

    mFilter = mFilterManager->loadFilterModules(listFilterInfo);

    return setCallback(mFilterManager, mFilterListener);
}

c2_status_t ExynosC2Mpeg4EncComponent::onSetup() {
    ExynosLogFunctionTrace();

    c2_status_t ret = C2_OK;

    ret = ExynosC2EncComponent::onSetup();
    if (ret != C2_OK) {
        return ret;
    }

    /* TODO */

    return ret;
}

class ExynosC2Mpeg4EncFactory : public C2ComponentFactory {
public:
    ExynosC2Mpeg4EncFactory()
        : mHelper(std::static_pointer_cast<C2ReflectorHelper>(android::GetCodec2ExynosComponentStore()->getParamReflector())) {
        SetStaticExynosLogLevel();
    }

    c2_status_t createComponent(
        c2_node_id_t id,
        std::shared_ptr<C2Component> * const component,
        std::function<void(C2Component*)> deleter) override {
        using __component_type = ExynosC2ComponentWrapper<ExynosC2Mpeg4EncComponent,
                                                          ExynosC2Mpeg4EncComponent::Mpeg4EncParamIntf>;

        *component = std::shared_ptr<C2Component>(
                        new __component_type(id, std::make_shared<ExynosC2Mpeg4EncComponent::Mpeg4EncParamIntf>(mHelper)), deleter);

        auto mpeg4Component = std::static_pointer_cast<__component_type>(*component);

        mpeg4Component->init((*component));

        return C2_OK;
    }

    c2_status_t createInterface(
        c2_node_id_t id,
        std::shared_ptr<C2ComponentInterface> * const interface,
        std::function<void(C2ComponentInterface*)> deleter) override {
        *interface = std::shared_ptr<C2ComponentInterface>(
                        new ExynosC2ComponentInterface(COMPONENT_NAME, id, std::make_shared<ExynosC2Mpeg4EncComponent::Mpeg4EncParamIntf>(mHelper)), deleter);

        return C2_OK;
    }

    virtual ~ExynosC2Mpeg4EncFactory() override = default;

private:
    std::shared_ptr<C2ReflectorHelper> mHelper;
};

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((cfi_canonical_jump_table))
EXYNOS_EXPORT_REF extern "C" ::C2ComponentFactory* CreateCodec2Factory() {
    return new ExynosC2Mpeg4EncFactory();
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

