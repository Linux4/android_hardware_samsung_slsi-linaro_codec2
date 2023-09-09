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

#include "Exynos_C2_H263EncComponent.h"
#include "Exynos_H263Enc_Filter.h"
#include "Exynos_CSC_Filter.h"
#include "ExynosBufferAllocator.h"

#ifdef USE_GDC_FILTER
#include "Exynos_GDC_Filter.h"
#endif

#define LOG_ON
#include "ExynosLog.h"
#undef LOG_TAG
#define LOG_TAG "ExynosC2H263EncComponent"

constexpr char COMPONENT_NAME[] = "c2.exynos.h263.encoder";

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
    {   "h263enc",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosH263EncFilter>,
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
    StaticExynosLog(Level::Debug, "H263EncParamIntf", "[%s] bitrate(%d)", __FUNCTION__, me.v.value);
    return res;
}

static C2R ProfileLevelSetter(
        bool mayBlock,
        C2InterfaceHelper::C2P<C2StreamProfileLevelInfo::output> &me) {
    (void)mayBlock;

    StaticExynosLog(Level::Debug, "H263EncParamIntf", "[%s] profile(0x%x), level(0x%x)", __FUNCTION__, me.v.profile, me.v.level);

    return C2R::Ok();
}

static C2R PictureQuantizationSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamPictureQuantizationTuning::output> &me) {
    return EncCommonSetter::PictureQuantizationSetter(mayBlock, me, MIN_QP, MAX_QP);
}

class ExynosC2H263EncComponent::H263EncParamIntf : public ExynosC2EncComponent::VencCommonParamIntf {
public:
    H263EncParamIntf(const std::shared_ptr<C2ReflectorHelper> &helper)
        : VencCommonParamIntf(
                helper,
                COMPONENT_NAME,
                C2Component::KIND_ENCODER,
                C2Component::DOMAIN_VIDEO,
                MEDIA_MIMETYPE_VIDEO_H263,
                listFilterInfo) {
        /* TODO */
        addEncodeDefaultParam({ 320, 240 }, { 32, 32 }, { 2048, 1152 });

        addActualFormatParam();

        addBitrateModeParam({ /* nothing */ }, EncCommonSetter::BitrateModeSetter);

        addBitrateParam(mBitrate, 64000, 4096, 80000000, BitrateSetter, EncCommonCnv::cnvBitrate);

        /* supported profile and level about bitstream */
        addParameter(
                DefineParam(mProfileLevel, C2_PARAMKEY_PROFILE_LEVEL)
                .withDefault(new C2StreamProfileLevelInfo::output(0u,
                        C2Config::PROFILE_H263_BASELINE, C2Config::LEVEL_H263_70))
                .withFields({
                    C2F(mProfileLevel, profile).oneOf({ C2Config::PROFILE_H263_BASELINE }),
                    C2F(mProfileLevel, level).oneOf({ C2Config::LEVEL_H263_70 })
                })
                .withSetter(ProfileLevelSetter)
                .build());

        addCroppingScalingParam(2048, 1152, 2048, 1152, 4);

        /* extension feature */
        {
            /* qp range */
            addQpRangeParam(3, 30, 3, 30, 3, 30, PictureQuantizationSetter);

            /* PMV */
            addPMV();
        }
    }

    virtual ~H263EncParamIntf() = default;

    /* getter functions */

    /* generate filter param */

private:
};

ExynosC2H263EncComponent::ExynosC2H263EncComponent(c2_node_id_t id, const std::shared_ptr<H263EncParamIntf> &intfImpl)
    : ExynosC2EncComponent(std::make_shared<ExynosC2ComponentInterface>(COMPONENT_NAME, id, intfImpl)) {
    mObjName    = "ExynosC2H263EncComponent";
    mbLogOff    = false;
    mParamIntf  = std::static_pointer_cast<CommonParamIntf>(intfImpl);

    mThreadPool->setObjName(mObjName);
    return;
}

bool ExynosC2H263EncComponent::init(std::weak_ptr<C2Component> wkComponent) {
    ExynosLogFunctionTrace();

    /* TODO : secure */
    mFilterManager = std::make_shared<ExynosC2FilterManager>(mObjName, wkComponent);

    mFilter = mFilterManager->loadFilterModules(listFilterInfo);

    return setCallback(mFilterManager, mFilterListener);
}

c2_status_t ExynosC2H263EncComponent::onSetup() {
    ExynosLogFunctionTrace();

    c2_status_t ret = C2_OK;

    ret = ExynosC2EncComponent::onSetup();
    if (ret != C2_OK) {
        return ret;
    }

    /* TODO */

    return ret;
}

class ExynosC2H263EncFactory : public C2ComponentFactory {
public:
    ExynosC2H263EncFactory()
        : mHelper(std::static_pointer_cast<C2ReflectorHelper>(android::GetCodec2ExynosComponentStore()->getParamReflector())) {
        SetStaticExynosLogLevel();
    }

    c2_status_t createComponent(
        c2_node_id_t id,
        std::shared_ptr<C2Component> * const component,
        std::function<void(C2Component*)> deleter) override {
        using __component_type = ExynosC2ComponentWrapper<ExynosC2H263EncComponent,
                                                          ExynosC2H263EncComponent::H263EncParamIntf>;

        *component = std::shared_ptr<C2Component>(
                        new __component_type(id, std::make_shared<ExynosC2H263EncComponent::H263EncParamIntf>(mHelper)), deleter);

        auto h263Component = std::static_pointer_cast<__component_type>(*component);

        h263Component->init((*component));

        return C2_OK;
    }

    c2_status_t createInterface(
        c2_node_id_t id,
        std::shared_ptr<C2ComponentInterface> * const interface,
        std::function<void(C2ComponentInterface*)> deleter) override {
        *interface = std::shared_ptr<C2ComponentInterface>(
                        new ExynosC2ComponentInterface(COMPONENT_NAME, id, std::make_shared<ExynosC2H263EncComponent::H263EncParamIntf>(mHelper)), deleter);

        return C2_OK;
    }

    virtual ~ExynosC2H263EncFactory() override = default;

private:
    std::shared_ptr<C2ReflectorHelper> mHelper;
};

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((cfi_canonical_jump_table))
EXYNOS_EXPORT_REF extern "C" ::C2ComponentFactory* CreateCodec2Factory() {
    return new ExynosC2H263EncFactory();
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

