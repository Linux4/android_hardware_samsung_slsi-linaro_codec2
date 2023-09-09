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
#include "Exynos_C2_H264DecComponent.h"
#include "Exynos_H264Dec_Filter.h"
#include "Exynos_CSC_Filter.h"
#include "ExynosBufferAllocator.h"

#define LOG_ON
#include "ExynosLog.h"
#undef LOG_TAG
#define LOG_TAG "ExynosC2H264DecComponent"

constexpr char COMPONENT_NAME[] = "c2.exynos.h264.decoder";
constexpr char SECURE_COMPONENT_NAME[] = "c2.exynos.h264.decoder.secure";

constexpr int32_t SKYPE_DRIVER_VERSION = 200429;  /* TODO */

#ifndef USE_BUFFERQUEUE
#define DEFAULT_ALLOCATORSTORE_TYPE C2PlatformAllocatorStore::GRALLOC
#else
#define DEFAULT_ALLOCATORSTORE_TYPE C2PlatformAllocatorStore::BUFFERQUEUE
#endif

ExynosC2FilterManager::FilterModuleInfoList listFilterInfo = {
    {   "h264dec",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosH264DecFilter>,
        DEFAULT_ALLOCATORSTORE_TYPE,
        C2MemoryUsage(DEFAULT_DEC_MEMORY_USAGE)
    },
#ifdef USE_CSC_FILTER
    {   "csc",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosCSCFilter>,
        DEFAULT_ALLOCATORSTORE_TYPE,
        C2MemoryUsage(DEFAULT_DEC_MEMORY_USAGE),
    },
#endif
};

ExynosC2FilterManager::FilterModuleInfoList listSecureFilterInfo = {
    {   "h264dec.secure",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosH264DecFilter>,
        C2PlatformAllocatorStore::BUFFERQUEUE,
        C2MemoryUsage(DEFAULT_SECURE_DEC_MEMORY_USAGE),
    },
#ifdef USE_CSC_FILTER
    {   "csc.secure",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosCSCFilter>,
        C2PlatformAllocatorStore::BUFFERQUEUE,
        C2MemoryUsage(DEFAULT_SECURE_DEC_MEMORY_USAGE),
    },
#endif
};


/* setter functions */
static C2R MaxPictureSizeSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamMaxPictureSizeTuning::output> &me,
                                const C2InterfaceHelper::C2P<C2StreamPictureSizeInfo::output> &size) {
    return DecCommonSetter::MaxPictureSizeSetter(mayBlock, me, size, 8192u, 8192u);
}

static C2R ProfileLevelSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamProfileLevelInfo::input> &me,
                              const C2InterfaceHelper::C2P<C2StreamPictureSizeInfo::output> &size) {
    (void)mayBlock;
    (void)size;
    (void)me;  // TODO: validate
    return C2R::Ok();
}

static C2R SkypeLowLatencySetter(bool mayBlock, C2InterfaceHelper::C2P<C2ExynosPortSkypeLowLatencySetting::output> &me) {
    (void)mayBlock;
    C2R res = C2R::Ok();

    StaticExynosLog(Level::Debug, "H264DecParamIntf", "[%s] skype :: low latency : %s",
                        __FUNCTION__, (me.v.enable == On)? "enabled":"disabled");
    return res;
}


class ExynosC2H264DecComponent::H264DecParamIntf : public ExynosC2DecComponent::VdecCommonParamIntf {
public:
    H264DecParamIntf(const std::shared_ptr<C2ReflectorHelper> &helper, bool isSecure = false)
        : VdecCommonParamIntf(
                helper,
                ((isSecure == true)? SECURE_COMPONENT_NAME:COMPONENT_NAME),
                C2Component::KIND_DECODER,
                C2Component::DOMAIN_VIDEO,
                MEDIA_MIMETYPE_VIDEO_AVC,
                ((isSecure != true)? listFilterInfo:listSecureFilterInfo)) {
        addDecodeDefaultParam(32, 32, 8192, 8192, (320 * 240 * 3 / 2), MaxPictureSizeSetter);

        /* extension feature : decode order decoding */
        addDecodeOrder(false);
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mDecodeOrder)), DecCommonCnv::cnvDecodeOrder,
                                { mReorderTimestamp });

        addReorderDepth(0);  /* if decode order is supported, reorder depth should be added */

        addOutputDelay(0u);

        addReorderKey(C2Config::ordinal_key_t::ORDINAL, DecCommonSetter::ReorderKeySetterWithDecodeOrder, mReorderTimestamp, mDecodeOrder);

        /* supported profile and level about bitstream */
        addParameter(
                DefineParam(mProfileLevel, C2_PARAMKEY_PROFILE_LEVEL)
                .withDefault(new C2StreamProfileLevelInfo::input(0u,
                        C2Config::PROFILE_AVC_CONSTRAINED_BASELINE, C2Config::LEVEL_AVC_5_2))
                .withFields({
                    C2F(mProfileLevel, profile).oneOf({
                            C2Config::PROFILE_AVC_CONSTRAINED_BASELINE,
                            C2Config::PROFILE_AVC_BASELINE,
                            C2Config::PROFILE_AVC_MAIN,
                            C2Config::PROFILE_AVC_CONSTRAINED_HIGH,
                            C2Config::PROFILE_AVC_PROGRESSIVE_HIGH,
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
                .withSetter(ProfileLevelSetter, mStreamSize)
                .build());

        /* extension feature : cropping */
        addCroppingParam(8192, 8192, 8192, 8192, 2);

        /* extension feature : display delay */
        {
            addParameter(
                    DefineParam(mDisplayDelay, C2EXYNOS_PARAMKEY_DISPLAY_DELAY)
                    .withDefault(new C2ExynosPortDisplayDelaySetting::output(-1))
                    .withFields({C2F(mDisplayDelay, value).inRange(-1, 8)})
                    .withSetter(Setter<decltype(*mDisplayDelay)>::StrictValueWithNoDeps)
                    .build());
            addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mDisplayDelay)), cnvDisplayDelay);
        }

        /* extension feature : compressed color */
        addCompressedColor();

        /* skype */
        {
            addParameter(
                    DefineParam(mSkypeDriverVersionInfo, C2EXYNOS_PARAMKEY_SKYPE_DEC_DRIVER_VERSION)
                    .withConstValue(new C2ExynosPortDriverVersionInfo::output(SKYPE_DRIVER_VERSION))
                    .build());

            addParameter(
                    DefineParam(mSkypeLowLatency, C2EXYNOS_PARAMKEY_SKYPE_DEC_LOW_LATENCY)
                    .withDefault(new C2ExynosPortSkypeLowLatencySetting::output(Off))
                    .withFields({C2F(mSkypeLowLatency, enable).inRange(Off, On)})
                    .withSetter(SkypeLowLatencySetter)
                    .build());
            addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mSkypeLowLatency)), cnvSkypeLowLatency);
        }
    }

    virtual ~H264DecParamIntf() = default;

    /* getter functions */
    int32_t getDisplayDelay() const { return mDisplayDelay->value; }

    int32_t getSkypeLowLatency() const { return mSkypeLowLatency->enable; }

    /* generate filter param */
    static FilterParamSetterRet cnvDisplayDelay(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<H264DecParamIntf>(intf);

        return DecCommonCnv::cnvDisplayDelay(intf, intfImpl->getDisplayDelay());
    }

    static FilterParamSetterRet cnvSkypeLowLatency(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<H264DecParamIntf>(intf);

        auto id = intfImpl->findFilterID("dec");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamSkypeLowLatency>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamSkypeLowLatency>>(filterParam->getBaseParam());

            param->m.enable = intfImpl->getSkypeLowLatency();

            StaticExynosLog(Level::Essential, "H264DecParamIntf", "[%s] skype :: low latency : %s",
                            __FUNCTION__, (param->m.enable == On)? "enabled":"disabled");

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
        }

        return ret;
    }

private:
    /* extension feature : display delay */
    std::shared_ptr<C2ExynosPortDisplayDelaySetting::output> mDisplayDelay;

    /* skype */
    std::shared_ptr<C2ExynosPortDriverVersionInfo::output>      mSkypeDriverVersionInfo;
    std::shared_ptr<C2ExynosPortSkypeLowLatencySetting::output> mSkypeLowLatency;

    friend class ExynosC2H264DecComponent;
};

ExynosC2H264DecComponent::ExynosC2H264DecComponent(c2_node_id_t id, const std::shared_ptr<H264DecParamIntf> &intfImpl)
    : ExynosC2DecComponent(std::make_shared<ExynosC2ComponentInterface>(intfImpl->mName->m.value, id, intfImpl)) {

    mObjName    = "ExynosC2H264DecComponent";
    mbLogOff    = false;
    mParamIntf  = std::static_pointer_cast<CommonParamIntf>(intfImpl);

    mThreadPool->setObjName(mObjName);
    return;
}

bool ExynosC2H264DecComponent::init(std::weak_ptr<C2Component> wkComponent) {
    ExynosLogFunctionTrace();

    /* TODO : secure */
    mFilterManager = std::make_shared<ExynosC2FilterManager>(mObjName, wkComponent);

    {
        H264DecParamIntf::Lock lock = mParamIntf->lock();

        mFilter = mFilterManager->loadFilterModules((mParamIntf->mIsSecure == true)? listSecureFilterInfo:listFilterInfo);
    }

    return setCallback(mFilterManager, mFilterListener);
}

void ExynosC2H264DecComponent::onUpdateC2Config(
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

    ExynosC2DecComponent::onUpdateC2Config(c2buffer, buffer, inConfig, outConfig);

    auto h264DecIntf = std::static_pointer_cast<H264DecParamIntf>(mParamIntf);

    if ((buffer->mParams.get() != nullptr) &&
        (!buffer->mParams->empty())) {
        // Todo
    }

    return;
}

c2_status_t ExynosC2H264DecComponent::onSetup() {
    ExynosLogFunctionTrace();

    c2_status_t ret = C2_OK;

    ret = ExynosC2DecComponent::onSetup();
    if (ret != C2_OK) {
        return ret;
    }

    auto h264DecIntf = std::static_pointer_cast<H264DecParamIntf>(mParamIntf);

    /* crop info about bitstream */
    h264DecIntf->addStreamCrop(8192, 8192);

    std::vector<C2Param::Index> indices;

    indices.push_back(C2ExynosPortDecodeOrderSetting::output::PARAM_TYPE);     /* decode order decoding */
    indices.push_back(C2ExynosPortCompressedColorSetting::output::PARAM_TYPE); /* compressed color */

    return makeFilterParam(indices);
}

class ExynosC2H264DecFactory : public C2ComponentFactory {
public:
    ExynosC2H264DecFactory(bool isSecure = false)
        : mIsSecure(isSecure),
          mHelper(std::static_pointer_cast<C2ReflectorHelper>(android::GetCodec2ExynosComponentStore()->getParamReflector())) {
        SetStaticExynosLogLevel();
    }

    c2_status_t createComponent(
        c2_node_id_t id,
        std::shared_ptr<C2Component> * const component,
        std::function<void(C2Component*)> deleter) override {
        using __component_type = ExynosC2ComponentWrapper<ExynosC2H264DecComponent,
                                                          ExynosC2H264DecComponent::H264DecParamIntf>;

        *component = std::shared_ptr<C2Component>(
                        new __component_type(id, std::make_shared<ExynosC2H264DecComponent::H264DecParamIntf>(mHelper, mIsSecure)), deleter);

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
                                                       id, std::make_shared<ExynosC2H264DecComponent::H264DecParamIntf>(mHelper, mIsSecure)), deleter);

        return C2_OK;
    }

    virtual ~ExynosC2H264DecFactory() override = default;

private:
    bool mIsSecure;
    std::shared_ptr<C2ReflectorHelper> mHelper;
};

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((cfi_canonical_jump_table))
EXYNOS_EXPORT_REF extern "C" ::C2ComponentFactory* CreateCodec2Factory() {
    return new ExynosC2H264DecFactory();
}

__attribute__((cfi_canonical_jump_table))
EXYNOS_EXPORT_REF extern "C" ::C2ComponentFactory* CreateSecureCodec2Factory() {
    return new ExynosC2H264DecFactory(true);
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

