/*
 *
 * Copyright 2020 Samsung Electronics S.LSI Co. LTD
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
#include "Exynos_C2_Vc1DecComponent.h"
#include "Exynos_Vc1Dec_Filter.h"
#include "Exynos_CSC_Filter.h"
#include "ExynosBufferAllocator.h"

#define LOG_ON
#include "ExynosLog.h"
#undef LOG_TAG

#ifndef BUILD_WMV
#define LOG_TAG "ExynosC2Vc1DecComponent"
constexpr char COMPONENT_NAME[] = "c2.exynos.vc1.decoder";
const char *MEDIA_MIMETYPE_VIDEO_VC1 = "video/wvc1";

#define MEDIA_MIMETYPE_NAME MEDIA_MIMETYPE_VIDEO_VC1
#define FILTER_NAME "vc1dec"
#else
#define LOG_TAG "ExynosC2WmvDecComponent"
constexpr char COMPONENT_NAME[] = "c2.exynos.wmv.decoder";
const char *MEDIA_MIMETYPE_VIDEO_WMV = "video/x-ms-wmv";

#define MEDIA_MIMETYPE_NAME MEDIA_MIMETYPE_VIDEO_WMV
#define FILTER_NAME "wmvdec"
#endif

#ifndef USE_BUFFERQUEUE
#define DEFAULT_ALLOCATORSTORE_TYPE C2PlatformAllocatorStore::GRALLOC
#else
#define DEFAULT_ALLOCATORSTORE_TYPE C2PlatformAllocatorStore::BUFFERQUEUE
#endif

ExynosC2FilterManager::FilterModuleInfoList listFilterInfo = {
    {   FILTER_NAME,
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosVc1DecFilter>,
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
    return DecCommonSetter::MaxPictureSizeSetter(mayBlock, me, size, 2032u, 2032u);
}

static C2R ProfileLevelSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamProfileLevelInfo::input> &me,
                              const C2InterfaceHelper::C2P<C2StreamPictureSizeInfo::output> &size) {
    (void)mayBlock;
    (void)size;

    switch ((VendorC2Config::vendor_profile_t)me.v.profile) {
    case VendorC2Config::PROFILE_VC1_SIMPLE:
        if (me.v.level > (C2Config::level_t)VendorC2Config::LEVEL_VC1_MEDIUM) {
            me.set().level = (C2Config::level_t)VendorC2Config::LEVEL_VC1_MEDIUM;
        }
        break;
    case VendorC2Config::PROFILE_VC1_MAIN:
        if (me.v.level > (C2Config::level_t)VendorC2Config::LEVEL_VC1_HIGH) {
            me.set().level = (C2Config::level_t)VendorC2Config::LEVEL_VC1_HIGH;
        }
        break;
    default:
        break;
    }

    return C2R::Ok();
}


class ExynosC2Vc1DecComponent::Vc1DecParamIntf : public ExynosC2DecComponent::VdecCommonParamIntf {
public:
    Vc1DecParamIntf(const std::shared_ptr<C2ReflectorHelper> &helper)
        : VdecCommonParamIntf(
                helper,
                COMPONENT_NAME,
                C2Component::KIND_DECODER,
                C2Component::DOMAIN_VIDEO,
                MEDIA_MIMETYPE_NAME,
                listFilterInfo) {
        addDecodeDefaultParam(32, 32, 2032, 2032, (320 * 240 * 3 / 2), MaxPictureSizeSetter);

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
                        (C2Config::profile_t)VendorC2Config::PROFILE_VC1_MAIN, (C2Config::level_t)VendorC2Config::LEVEL_VC1_HIGH))
                .withFields({
                    C2F(mProfileLevel, profile).oneOf({
                            VendorC2Config::PROFILE_VC1_SIMPLE,
                            VendorC2Config::PROFILE_VC1_MAIN,
                            VendorC2Config::PROFILE_VC1_ADVANCED}),
                    C2F(mProfileLevel, level).oneOf({
                            VendorC2Config::LEVEL_VC1_LOW,
                            VendorC2Config::LEVEL_VC1_MEDIUM,
                            VendorC2Config::LEVEL_VC1_HIGH,
                            VendorC2Config::LEVEL_VC1_1,
                            VendorC2Config::LEVEL_VC1_2,
                            VendorC2Config::LEVEL_VC1_3,
                            VendorC2Config::LEVEL_VC1_4})
                })
                .withSetter(ProfileLevelSetter, mStreamSize)
                .build());

        /* extension feature : cropping */
        addCroppingParam(2032, 2032, 2032, 2032, 2);

        /* extension feature : compressed color */
        addCompressedColor();
    }

    virtual ~Vc1DecParamIntf() = default;

    /* getter functions */

private:
    friend class ExynosC2Vc1DecComponent;
};

ExynosC2Vc1DecComponent::ExynosC2Vc1DecComponent(c2_node_id_t id, const std::shared_ptr<Vc1DecParamIntf> &intfImpl)
    : ExynosC2DecComponent(std::make_shared<ExynosC2ComponentInterface>(COMPONENT_NAME, id, intfImpl)) {

    mObjName    = "ExynosC2Vc1DecComponent";
    mbLogOff    = false;
    mParamIntf  = std::static_pointer_cast<CommonParamIntf>(intfImpl);

    mThreadPool->setObjName(mObjName);
    return;
}

bool ExynosC2Vc1DecComponent::init(std::weak_ptr<C2Component> wkComponent) {
    ExynosLogFunctionTrace();

    /* TODO : secure */
    mFilterManager = std::make_shared<ExynosC2FilterManager>(mObjName, wkComponent);

    mFilter = mFilterManager->loadFilterModules(listFilterInfo);

    return setCallback(mFilterManager, mFilterListener);
}

void ExynosC2Vc1DecComponent::onUpdateC2Config(
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

    auto vc1DecIntf = std::static_pointer_cast<Vc1DecParamIntf>(mParamIntf);

    if ((buffer->mParams.get() != nullptr) &&
        (!buffer->mParams->empty())) {
        // Todo
    }

    return;
}

c2_status_t ExynosC2Vc1DecComponent::onSetup() {
    ExynosLogFunctionTrace();

    c2_status_t ret = C2_OK;

    ret = ExynosC2DecComponent::onSetup();
    if (ret != C2_OK) {
        return ret;
    }

    auto vc1DecIntf = std::static_pointer_cast<Vc1DecParamIntf>(mParamIntf);

    /* crop info about bitstream */
    vc1DecIntf->addStreamCrop(2032, 2032);

    std::vector<C2Param::Index> indices;

    indices.push_back(C2ExynosPortDecodeOrderSetting::output::PARAM_TYPE);     /* decode order decoding */
    indices.push_back(C2ExynosPortCompressedColorSetting::output::PARAM_TYPE); /* compressed color */

    return makeFilterParam(indices);
}

class ExynosC2Vc1DecFactory : public C2ComponentFactory {
public:
    ExynosC2Vc1DecFactory()
        : mHelper(std::static_pointer_cast<C2ReflectorHelper>(android::GetCodec2ExynosComponentStore()->getParamReflector())) {
        SetStaticExynosLogLevel();
    }

    c2_status_t createComponent(
        c2_node_id_t id,
        std::shared_ptr<C2Component> * const component,
        std::function<void(C2Component*)> deleter) override {
        using __component_type = ExynosC2ComponentWrapper<ExynosC2Vc1DecComponent,
                                                          ExynosC2Vc1DecComponent::Vc1DecParamIntf>;

        *component = std::shared_ptr<C2Component>(
                        new __component_type(id, std::make_shared<ExynosC2Vc1DecComponent::Vc1DecParamIntf>(mHelper)), deleter);

        auto vc1Component = std::static_pointer_cast<__component_type>(*component);

        vc1Component->init((*component));

        return C2_OK;
    }

    c2_status_t createInterface(
        c2_node_id_t id,
        std::shared_ptr<C2ComponentInterface> * const interface,
        std::function<void(C2ComponentInterface*)> deleter) override {
        *interface = std::shared_ptr<C2ComponentInterface>(
                        new ExynosC2ComponentInterface(COMPONENT_NAME, id, std::make_shared<ExynosC2Vc1DecComponent::Vc1DecParamIntf>(mHelper)), deleter);

        return C2_OK;
    }

    virtual ~ExynosC2Vc1DecFactory() override = default;

private:
    std::shared_ptr<C2ReflectorHelper> mHelper;
};

#ifdef __cplusplus
extern "C" {
#endif

EXYNOS_EXPORT_REF extern "C" ::C2ComponentFactory* CreateCodec2Factory() {
    return new ExynosC2Vc1DecFactory();
}

EXYNOS_EXPORT_REF extern "C" void DestroyCodec2Factory(::C2ComponentFactory *factory) {
    if (factory != nullptr) {
        delete factory;
    }
}

#ifdef __cplusplus
}
#endif

