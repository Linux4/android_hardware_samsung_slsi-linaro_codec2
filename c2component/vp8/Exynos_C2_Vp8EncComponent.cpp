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

#include "Exynos_C2_Vp8EncComponent.h"
#include "Exynos_Vp8Enc_Filter.h"
#include "Exynos_CSC_Filter.h"
#include "ExynosBufferAllocator.h"

#ifdef USE_GDC_FILTER
#include "Exynos_GDC_Filter.h"
#endif

#define LOG_ON
#include "ExynosLog.h"
#undef LOG_TAG
#define LOG_TAG "ExynosC2Vp8EncComponent"

constexpr char COMPONENT_NAME[] = "c2.exynos.vp8.encoder";

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
    {   "vp8enc",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosVp8EncFilter>,
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
    StaticExynosLog(Level::Debug, "Vp8EncParamIntf", "[%s] bitrate(%d)", __FUNCTION__, me.v.value);
    return res;
}

static C2R PictureQuantizationSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamPictureQuantizationTuning::output> &me) {
    return EncCommonSetter::PictureQuantizationSetter(mayBlock, me, MIN_QP, MAX_QP);
}

class ExynosC2Vp8EncComponent::Vp8EncParamIntf : public ExynosC2EncComponent::VencCommonParamIntf {
public:
    Vp8EncParamIntf(const std::shared_ptr<C2ReflectorHelper> &helper)
        : VencCommonParamIntf(
                helper,
                COMPONENT_NAME,
                C2Component::KIND_ENCODER,
                C2Component::DOMAIN_VIDEO,
                MEDIA_MIMETYPE_VIDEO_VP8,
                listFilterInfo) {
        /* TODO */
        addEncodeDefaultParam({ 320, 240 }, { 32, 32 }, { 4096, 4096 });

        addActualFormatParam();

        addBitrateModeParam({ /* nothing */ }, EncCommonSetter::BitrateModeSetter);

        addBitrateParam(mBitrate, 64000, 4096, 80000000, BitrateSetter, EncCommonCnv::cnvBitrate);

        addTemporalLayeringParam(mLayering, mBitrate,
                                 MAX_VPX_TEMPORAL_LAYERS, 0,
                                 EncCommonSetter::LayeringSetterVpx, cnvLayering);

        /* supported profile and level about bitstream */
        addParameter(
                DefineParam(mProfileLevel, C2_PARAMKEY_PROFILE_LEVEL)
                .withConstValue(new C2StreamProfileLevelInfo::output(0u,
                        C2Config::PROFILE_UNUSED, C2Config::LEVEL_UNUSED))
                .build());

        addCroppingScalingParam(4096, 4096, 4096, 4096, 2);

        /* extension feature */
        {
            /* qp range */
            addQpRangeParam(2, 63, 2, 63, 0, 0, PictureQuantizationSetter);

            /* ref p frames */
            addRefPframesParam(mRefPFrames, EncCommonSetter::RefPframesSetter, cnvRefPframes);

            /* PMV */
            addPMV(true);
        }
    }

    virtual ~Vp8EncParamIntf() = default;

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

        auto intfImpl = std::static_pointer_cast<Vp8EncParamIntf>(intf);

        auto layering = intfImpl->getLayering();

        return EncCommonCnv::cnvLayeringCommon(intf, layering);
    }

    static FilterParamSetterRet cnvRefPframes(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<Vp8EncParamIntf>(intf);

        return EncCommonCnv::cnvRefPframes(intf, intfImpl->getRefPframes());
    }

private:
    std::shared_ptr<C2StreamTemporalLayeringTuning::output> mLayering;

    /* extension feature : ref p frames */
    std::shared_ptr<C2ExynosPortRefPframeSetting::output> mRefPFrames;
};

ExynosC2Vp8EncComponent::ExynosC2Vp8EncComponent(c2_node_id_t id, const std::shared_ptr<Vp8EncParamIntf> &intfImpl)
    : ExynosC2EncComponent(std::make_shared<ExynosC2ComponentInterface>(COMPONENT_NAME, id, intfImpl)) {
    mObjName    = "ExynosC2Vp8EncComponent";
    mbLogOff    = false;
    mParamIntf  = std::static_pointer_cast<CommonParamIntf>(intfImpl);

    mThreadPool->setObjName(mObjName);
    return;
}

bool ExynosC2Vp8EncComponent::init(std::weak_ptr<C2Component> wkComponent) {
    ExynosLogFunctionTrace();

    /* TODO : secure */
    mFilterManager = std::make_shared<ExynosC2FilterManager>(mObjName, wkComponent);

    mFilter = mFilterManager->loadFilterModules(listFilterInfo);

    return setCallback(mFilterManager, mFilterListener);
}

c2_status_t ExynosC2Vp8EncComponent::onSetup() {
    ExynosLogFunctionTrace();

    c2_status_t ret = C2_OK;

    ret = ExynosC2EncComponent::onSetup();
    if (ret != C2_OK) {
        return ret;
    }

    /* TODO */

    return ret;
}

class ExynosC2Vp8EncFactory : public C2ComponentFactory {
public:
    ExynosC2Vp8EncFactory()
        : mHelper(std::static_pointer_cast<C2ReflectorHelper>(android::GetCodec2ExynosComponentStore()->getParamReflector())) {
        SetStaticExynosLogLevel();
    }

    c2_status_t createComponent(
        c2_node_id_t id,
        std::shared_ptr<C2Component> * const component,
        std::function<void(C2Component*)> deleter) override {
        using __component_type = ExynosC2ComponentWrapper<ExynosC2Vp8EncComponent,
                                                          ExynosC2Vp8EncComponent::Vp8EncParamIntf>;

        *component = std::shared_ptr<C2Component>(
                        new __component_type(id, std::make_shared<ExynosC2Vp8EncComponent::Vp8EncParamIntf>(mHelper)), deleter);

        auto vp8Component = std::static_pointer_cast<__component_type>(*component);

        vp8Component->init((*component));

        return C2_OK;
    }

    c2_status_t createInterface(
        c2_node_id_t id,
        std::shared_ptr<C2ComponentInterface> * const interface,
        std::function<void(C2ComponentInterface*)> deleter) override {
        *interface = std::shared_ptr<C2ComponentInterface>(
                        new ExynosC2ComponentInterface(COMPONENT_NAME, id, std::make_shared<ExynosC2Vp8EncComponent::Vp8EncParamIntf>(mHelper)), deleter);

        return C2_OK;
    }

    virtual ~ExynosC2Vp8EncFactory() override = default;

private:
    std::shared_ptr<C2ReflectorHelper> mHelper;
};

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((cfi_canonical_jump_table))
EXYNOS_EXPORT_REF extern "C" ::C2ComponentFactory* CreateCodec2Factory() {
    return new ExynosC2Vp8EncFactory();
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

