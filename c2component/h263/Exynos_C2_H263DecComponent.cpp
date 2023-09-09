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
#include "Exynos_C2_H263DecComponent.h"
#include "Exynos_H263Dec_Filter.h"
#include "Exynos_CSC_Filter.h"
#include "ExynosBufferAllocator.h"

#define LOG_ON
#include "ExynosLog.h"
#undef LOG_TAG
#define LOG_TAG "ExynosC2H263DecComponent"

constexpr char COMPONENT_NAME[] = "c2.exynos.h263.decoder";


#ifndef USE_BUFFERQUEUE
#define DEFAULT_ALLOCATORSTORE_TYPE C2PlatformAllocatorStore::GRALLOC
#else
#define DEFAULT_ALLOCATORSTORE_TYPE C2PlatformAllocatorStore::BUFFERQUEUE
#endif

ExynosC2FilterManager::FilterModuleInfoList listFilterInfo = {
    {   "h263dec",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosH263DecFilter>,
        DEFAULT_ALLOCATORSTORE_TYPE,
        C2MemoryUsage(DEFAULT_DEC_MEMORY_USAGE)
    },
#ifdef USE_CSC_FILTER
    {   "csc",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosCSCFilter>,
        DEFAULT_ALLOCATORSTORE_TYPE,
        C2MemoryUsage(DEFAULT_DEC_MEMORY_USAGE)
    },
#endif
};


/* setter functions */
static C2R MaxPictureSizeSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamMaxPictureSizeTuning::output> &me,
                                const C2InterfaceHelper::C2P<C2StreamPictureSizeInfo::output> &size) {
    return DecCommonSetter::MaxPictureSizeSetter(mayBlock, me, size, 2048u, 1152u);
}

static C2R ProfileLevelSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamProfileLevelInfo::input> &me,
                              const C2InterfaceHelper::C2P<C2StreamPictureSizeInfo::output> &size) {
    (void)mayBlock;
    (void)size;
    (void)me;  // TODO: validate
    return C2R::Ok();
}


class ExynosC2H263DecComponent::H263DecParamIntf : public ExynosC2DecComponent::VdecCommonParamIntf {
public:
    H263DecParamIntf(const std::shared_ptr<C2ReflectorHelper> &helper)
        : VdecCommonParamIntf(
                helper,
                COMPONENT_NAME,
                C2Component::KIND_DECODER,
                C2Component::DOMAIN_VIDEO,
                MEDIA_MIMETYPE_VIDEO_H263,
                listFilterInfo) {
        addDecodeDefaultParam(32, 32, 2048, 1152, (320 * 240 * 3 / 2), MaxPictureSizeSetter);

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
                        C2Config::PROFILE_H263_BASELINE, C2Config::LEVEL_H263_30))
                .withFields({
                    C2F(mProfileLevel, profile).oneOf({
                            C2Config::PROFILE_H263_BASELINE,
                            C2Config::PROFILE_H263_H320,
                            C2Config::PROFILE_H263_V1BC,
                            C2Config::PROFILE_H263_ISWV2}),
                    C2F(mProfileLevel, level).oneOf({
                            C2Config::LEVEL_H263_10,
                            C2Config::LEVEL_H263_20,
                            C2Config::LEVEL_H263_30,
                            C2Config::LEVEL_H263_40,
                            C2Config::LEVEL_H263_45,
                            C2Config::LEVEL_H263_50,
                            C2Config::LEVEL_H263_60,
                            C2Config::LEVEL_H263_70})
                })
                .withSetter(ProfileLevelSetter, mStreamSize)
                .build());

        /* extension feature : cropping */
        addCroppingParam(2048, 1152, 2048, 1152, 2);

        /* extension feature : compressed color */
        addCompressedColor();
    }

    virtual ~H263DecParamIntf() = default;

    /* getter functions */

private:
    friend class ExynosC2H263DecComponent;
};

ExynosC2H263DecComponent::ExynosC2H263DecComponent(c2_node_id_t id, const std::shared_ptr<H263DecParamIntf> &intfImpl)
    : ExynosC2DecComponent(std::make_shared<ExynosC2ComponentInterface>(COMPONENT_NAME, id, intfImpl)) {

    mObjName    = "ExynosC2H263DecComponent";
    mbLogOff    = false;
    mParamIntf  = std::static_pointer_cast<CommonParamIntf>(intfImpl);

    mThreadPool->setObjName(mObjName);
    return;
}

bool ExynosC2H263DecComponent::init(std::weak_ptr<C2Component> wkComponent) {
    ExynosLogFunctionTrace();

    /* TODO : secure */
    mFilterManager = std::make_shared<ExynosC2FilterManager>(mObjName, wkComponent);

    mFilter = mFilterManager->loadFilterModules(listFilterInfo);

    return setCallback(mFilterManager, mFilterListener);
}

void ExynosC2H263DecComponent::onUpdateC2Config(
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

    auto h263DecIntf = std::static_pointer_cast<H263DecParamIntf>(mParamIntf);

    if ((buffer->mParams.get() != nullptr) &&
        (!buffer->mParams->empty())) {
        // Todo
    }

    return;
}

c2_status_t ExynosC2H263DecComponent::onSetup() {
    ExynosLogFunctionTrace();

    c2_status_t ret = C2_OK;

    ret = ExynosC2DecComponent::onSetup();
    if (ret != C2_OK) {
        return ret;
    }

    auto h263DecIntf = std::static_pointer_cast<H263DecParamIntf>(mParamIntf);

    /* crop info about bitstream */
    h263DecIntf->addStreamCrop(2048, 1152);

    std::vector<C2Param::Index> indices;

    indices.push_back(C2ExynosPortDecodeOrderSetting::output::PARAM_TYPE);     /* decode order decoding */
    indices.push_back(C2ExynosPortCompressedColorSetting::output::PARAM_TYPE); /* compressed color */

    return makeFilterParam(indices);
}

class ExynosC2H263DecFactory : public C2ComponentFactory {
public:
    ExynosC2H263DecFactory()
        : mHelper(std::static_pointer_cast<C2ReflectorHelper>(android::GetCodec2ExynosComponentStore()->getParamReflector())) {
        SetStaticExynosLogLevel();
    }

    c2_status_t createComponent(
        c2_node_id_t id,
        std::shared_ptr<C2Component> * const component,
        std::function<void(C2Component*)> deleter) override {
        using __component_type = ExynosC2ComponentWrapper<ExynosC2H263DecComponent,
                                                          ExynosC2H263DecComponent::H263DecParamIntf>;

        *component = std::shared_ptr<C2Component>(
                        new __component_type(id, std::make_shared<ExynosC2H263DecComponent::H263DecParamIntf>(mHelper)), deleter);

        auto h263Component = std::static_pointer_cast<__component_type>(*component);

        h263Component->init((*component));

        return C2_OK;
    }

    c2_status_t createInterface(
        c2_node_id_t id,
        std::shared_ptr<C2ComponentInterface> * const interface,
        std::function<void(C2ComponentInterface*)> deleter) override {
        *interface = std::shared_ptr<C2ComponentInterface>(
                        new ExynosC2ComponentInterface(COMPONENT_NAME, id, std::make_shared<ExynosC2H263DecComponent::H263DecParamIntf>(mHelper)), deleter);

        return C2_OK;
    }

    virtual ~ExynosC2H263DecFactory() override = default;

private:
    std::shared_ptr<C2ReflectorHelper> mHelper;
};

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((cfi_canonical_jump_table))
EXYNOS_EXPORT_REF extern "C" ::C2ComponentFactory* CreateCodec2Factory() {
    return new ExynosC2H263DecFactory();
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
