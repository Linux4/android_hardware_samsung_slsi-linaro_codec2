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
#include "Exynos_C2_Av1DecComponent.h"
#include "Exynos_Av1Dec_Filter.h"
#include "Exynos_CSC_Filter.h"
#include "Exynos_FilmGrain_Filter.h"
#include "Exynos_HDR2SDR_Filter.h"
#include "Exynos_PostControl_Filter.h"
#include "ExynosBufferAllocator.h"

#include "VendorVideoAPI.h"

#define LOG_ON
#include "ExynosLog.h"
#undef LOG_TAG
#define LOG_TAG "ExynosC2Av1DecComponent"

constexpr char COMPONENT_NAME[] = "c2.exynos.av1.decoder";
constexpr char SECURE_COMPONENT_NAME[] = "c2.exynos.av1.decoder.secure";


#ifndef USE_BUFFERQUEUE
#define DEFAULT_ALLOCATORSTORE_TYPE C2PlatformAllocatorStore::GRALLOC
#else
#define DEFAULT_ALLOCATORSTORE_TYPE C2PlatformAllocatorStore::BUFFERQUEUE
#endif

ExynosC2FilterManager::FilterModuleInfoList listFilterInfo = {
    {   "av1dec",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosAv1DecFilter>,
        DEFAULT_ALLOCATORSTORE_TYPE,
        C2MemoryUsage(DEFAULT_DEC_MEMORY_USAGE)
    },
#ifdef USE_CSC_FILTER
    {   "control",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosPostCtrlFilter>,
        DEFAULT_ALLOCATORSTORE_TYPE,
        C2MemoryUsage(DEFAULT_DEC_MEMORY_USAGE),
    },
    {   "csc",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosCSCFilter>,
        DEFAULT_ALLOCATORSTORE_TYPE,
        C2MemoryUsage(DEFAULT_DEC_MEMORY_USAGE),
    },
#ifdef USE_FILMGRAIN_FILTER
    {   "filmgrain",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosFilmGrainFilter>,
        DEFAULT_ALLOCATORSTORE_TYPE,
        C2MemoryUsage(DEFAULT_DEC_MEMORY_USAGE),
    },
#endif
    {   "hdr2sdr",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosHDR2SDRFilter>,
        DEFAULT_ALLOCATORSTORE_TYPE,
        C2MemoryUsage(DEFAULT_DEC_MEMORY_USAGE),
    },
#endif
};

ExynosC2FilterManager::FilterModuleInfoList listSecureFilterInfo = {
    {   "av1dec.secure",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosAv1DecFilter>,
        C2PlatformAllocatorStore::BUFFERQUEUE,
        C2MemoryUsage(DEFAULT_SECURE_DEC_MEMORY_USAGE),
    },
#ifdef USE_CSC_FILTER
    {   "control.secure",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosPostCtrlFilter>,
        C2PlatformAllocatorStore::BUFFERQUEUE,
        C2MemoryUsage(DEFAULT_SECURE_DEC_MEMORY_USAGE),
    },
    {   "csc.secure",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosCSCFilter>,
        C2PlatformAllocatorStore::BUFFERQUEUE,
        C2MemoryUsage(DEFAULT_SECURE_DEC_MEMORY_USAGE),
    },
    {   "filmgrain.secure",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosFilmGrainFilter>,
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

class ExynosC2Av1DecComponent::Av1DecParamIntf : public ExynosC2DecComponent::VdecCommonParamIntf {
public:
    Av1DecParamIntf(const std::shared_ptr<C2ReflectorHelper> &helper, bool isSecure = false)
        : VdecCommonParamIntf(
                helper,
                ((isSecure == true)? SECURE_COMPONENT_NAME:COMPONENT_NAME),
                C2Component::KIND_DECODER,
                C2Component::DOMAIN_VIDEO,
                MEDIA_MIMETYPE_VIDEO_AV1,
                ((isSecure != true)? listFilterInfo:listSecureFilterInfo)) {
#ifndef UNSUPPORT_10BIT
        addDecodeDefaultParam(64, 64, 8192, 8192, (320 * 240 * 3 / 2), MaxPictureSizeSetter, true);
#else
        addDecodeDefaultParam(64, 64, 8192, 8192, (320 * 240 * 3 / 2), MaxPictureSizeSetter);
#endif
        addOutputDelay(0u, DecCommonSetter::ActualOutputDelaySetterWithExtra, mMaxDPBNumInfo, mExtraBufferNumInfo);

        addReorderKey(C2Config::ordinal_key_t::ORDINAL, DecCommonSetter::ReorderKeySetter, mReorderTimestamp);

        /* supported profile and level about bitstream */
        addParameter(
                DefineParam(mProfileLevel, C2_PARAMKEY_PROFILE_LEVEL)
                .withDefault(new C2StreamProfileLevelInfo::input(0u,
                        C2Config::PROFILE_AV1_0, C2Config::LEVEL_AV1_5))
                .withFields({
                    C2F(mProfileLevel, profile).oneOf({
                            C2Config::PROFILE_AV1_0
                    }),
                    C2F(mProfileLevel, level).oneOf({
                            C2Config::LEVEL_AV1_2,
                            C2Config::LEVEL_AV1_2_1,
                            C2Config::LEVEL_AV1_2_2,
                            C2Config::LEVEL_AV1_2_3,
                            C2Config::LEVEL_AV1_3,
                            C2Config::LEVEL_AV1_3_1,
                            C2Config::LEVEL_AV1_3_2,
                            C2Config::LEVEL_AV1_3_3,
                            C2Config::LEVEL_AV1_4,
                            C2Config::LEVEL_AV1_4_1,
                            C2Config::LEVEL_AV1_4_2,
                            C2Config::LEVEL_AV1_4_3,
                            C2Config::LEVEL_AV1_5,
                            C2Config::LEVEL_AV1_5_1,
                            C2Config::LEVEL_AV1_5_2,
                    })
                })
                .withSetter(ProfileLevelSetter, mStreamSize)
                .build());

        addHdrStaticInfo();

        /* HDR :: Dynamic Info */
        {
            mHdrDynamicInfo = C2StreamHdrDynamicMetadataInfo::output::AllocShared(0, 0u,
                                        C2Config::HDR_DYNAMIC_METADATA_TYPE_SMPTE_2094_40);
            addParameter(
                    DefineParam(mHdrDynamicInfo, C2_PARAMKEY_OUTPUT_HDR_DYNAMIC_INFO)
                    .withDefault(mHdrDynamicInfo)
                    .withFields({
                        C2F(mHdrDynamicInfo, m.type_).oneOf({ HDR_DYNAMIC_METADATA_TYPE_SMPTE_2094_40 }),
                        C2F(mHdrDynamicInfo, m.data).any(),
                    })
                    .withSetter(DecCommonSetter::CodedHdrDynamicInfoSetter)
                    .build());
        }

        /* extension feature : cropping */
        addCroppingParam(8192, 8192, 8192, 8192, 2);

        /* extension feature : image convert */
        addImageConvertModeParam(mImageConvert, mImageConvertMode, mImageConvertPixelFormat, mImageConvertControlFrame,
                                 cnvImageConvert, cnvImageConvertMode, cnvImageConvertPixelFormat, cnvImageConvertControlFrame);

        /* extension feature : compressed color */
        addCompressedColor();
    }

    virtual ~Av1DecParamIntf() = default;

    /* getter functions */
    std::shared_ptr<C2StreamHdrDynamicMetadataInfo::output> getHdrDynamicInfo() {
        return mHdrDynamicInfo;
    }

    bool getImageConvert() const { return (mImageConvert->value == On)? true:false; }

    uint32_t getImageConvertMode() const { return mImageConvertMode->value; }

    uint32_t getImageConvertPixelFormat() const { return mImageConvertPixelFormat->value; }

    uint32_t getImageConvertControlFrame() const { return mImageConvertControlFrame->value; }

    /* generate filter param */
    static FilterParamSetterRet cnvImageConvert(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<Av1DecParamIntf>(intf);

        if (intfImpl->getImageConvert()) {
            auto id = intfImpl->findFilterID("hdr2sdr");
            if (id != 0) {  /* target filter is valid */
                auto filterParam = std::make_shared<ExynosFilterParam<ParamHDR2SDR>>();
                auto param       = std::static_pointer_cast<ExynosParam<ParamHDR2SDR>>(filterParam->getBaseParam());

                param->m.enable = 1;

                StaticExynosLog(Level::Essential, "Av1DecParamIntf", "[%s] image convert is enabled", __FUNCTION__);

                filterParam->registTargetFilter(id);

                ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
            }
        }

        return ret;
    }

    static FilterParamSetterRet cnvImageConvertMode(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<Av1DecParamIntf>(intf);

        auto id = intfImpl->findFilterID("hdr2sdr");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamHDR2SDRMode>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamHDR2SDRMode>>(filterParam->getBaseParam());

            param->m.type = intfImpl->getImageConvertMode();

            StaticExynosLog(Level::Essential, "Av1DecParamIntf", "[%s] image convert mode(%d)", __FUNCTION__, param->m.type);

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
        }

        return ret;
    }

    static FilterParamSetterRet cnvImageConvertPixelFormat(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<Av1DecParamIntf>(intf);

        auto id = intfImpl->findFilterID("hdr2sdr");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamHDR2SDRPixelFormat>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamHDR2SDRPixelFormat>>(filterParam->getBaseParam());

            param->m.format = intfImpl->getImageConvertPixelFormat();

            StaticExynosLog(Level::Essential, "Av1DecParamIntf", "[%s] image convert format(0x%x)", __FUNCTION__, param->m.format);

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
        }

        return ret;
    }

    static FilterParamSetterRet cnvImageConvertControlFrame(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<Av1DecParamIntf>(intf);

        auto count = intfImpl->getImageConvertControlFrame();

        return DecCommonCnv::cnvImageConvertControlFrameCommon(intf, count);
    }

private:
    std::shared_ptr<C2StreamHdrDynamicMetadataInfo::output> mHdrDynamicInfo; /* from bitstream */

    /* extension feature : image convert */
    std::shared_ptr<C2ExynosPortImageConvertSetting::output>              mImageConvert;  /* for hdr2sdr */
    std::shared_ptr<C2ExynosPortImageConvertModeSetting::output>          mImageConvertMode;
    std::shared_ptr<C2ExynosPortImageConvertPixelFormatSetting::output>   mImageConvertPixelFormat;
    std::shared_ptr<C2ExynosPortImageConvertControlFrameSetting::output>  mImageConvertControlFrame;

    friend class ExynosC2Av1DecComponent;
};

ExynosC2Av1DecComponent::ExynosC2Av1DecComponent(c2_node_id_t id, const std::shared_ptr<Av1DecParamIntf> &intfImpl)
    : ExynosC2DecComponent(std::make_shared<ExynosC2ComponentInterface>(intfImpl->mName->m.value, id, intfImpl)) {

    mObjName    = "ExynosC2Av1DecComponent";
    mbLogOff    = false;
    mParamIntf  = std::static_pointer_cast<CommonParamIntf>(intfImpl);

    mThreadPool->setObjName(mObjName);
    return;
}

bool ExynosC2Av1DecComponent::init(std::weak_ptr<C2Component> wkComponent) {
    ExynosLogFunctionTrace();

    /* TODO : secure */
    mFilterManager = std::make_shared<ExynosC2FilterManager>(mObjName, wkComponent);

    {
        Av1DecParamIntf::Lock lock = mParamIntf->lock();

        mFilter = mFilterManager->loadFilterModules((mParamIntf->mIsSecure == true)? listSecureFilterInfo:listFilterInfo);
    }

    return setCallback(mFilterManager, mFilterListener);
}

void ExynosC2Av1DecComponent::onUpdateC2Config(
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

    auto av1DecIntf = std::static_pointer_cast<Av1DecParamIntf>(mParamIntf);
#if 0
    if ((buffer->mParams.get() != nullptr) &&
        (!buffer->mParams->empty())) {
        // Todo
    }
#endif

    ExynosC2DecComponent::updateC2Buffer_HDRInfo(c2buffer, buffer);

    return;
}

c2_status_t ExynosC2Av1DecComponent::onSetup() {
    ExynosLogFunctionTrace();

    c2_status_t ret = C2_OK;

    ret = ExynosC2DecComponent::onSetup();
    if (ret != C2_OK) {
        return ret;
    }

    auto av1DecIntf = std::static_pointer_cast<Av1DecParamIntf>(mParamIntf);

    /* crop info about bitstream */
    av1DecIntf->addStreamCrop(8192, 8192);

    std::vector<C2Param::Index> indices;

    indices.push_back(C2StreamColorAspectsInfo::output::PARAM_TYPE);           /* color aspects */
    indices.push_back(C2ExynosPortImageConvertSetting::output::PARAM_TYPE);    /* image convert */
    indices.push_back(C2ExynosPortCompressedColorSetting::output::PARAM_TYPE); /* compressed color */

    return makeFilterParam(indices);
}

c2_status_t ExynosC2Av1DecComponent::onFlush() {
    ExynosLogFunctionTrace();

    c2_status_t ret = C2_OK;

    ret = ExynosC2DecComponent::onFlush();
    if (ret == C2_OK) {
        std::vector<C2Param::Index> indices;

        indices.push_back(C2ExynosPortImageConvertSetting::output::PARAM_TYPE);
        indices.push_back(C2ExynosPortImageConvertModeSetting::output::PARAM_TYPE);
        indices.push_back(C2ExynosPortImageConvertPixelFormatSetting::output::PARAM_TYPE);
        indices.push_back(C2ExynosPortImageConvertControlFrameSetting::output::PARAM_TYPE);

        ret = makeFilterParam(indices);
    }

    return ret;
}

class ExynosC2Av1DecFactory : public C2ComponentFactory {
public:
    ExynosC2Av1DecFactory(bool isSecure = false)
        : mIsSecure(isSecure),
          mHelper(std::static_pointer_cast<C2ReflectorHelper>(android::GetCodec2ExynosComponentStore()->getParamReflector())) {
        SetStaticExynosLogLevel();
    }

    c2_status_t createComponent(
        c2_node_id_t id,
        std::shared_ptr<C2Component> * const component,
        std::function<void(C2Component*)> deleter) override {
        using __component_type = ExynosC2ComponentWrapper<ExynosC2Av1DecComponent,
                                                          ExynosC2Av1DecComponent::Av1DecParamIntf>;

        *component = std::shared_ptr<C2Component>(
                        new __component_type(id, std::make_shared<ExynosC2Av1DecComponent::Av1DecParamIntf>(mHelper, mIsSecure)),
                                                    deleter);

        auto av1Component = std::static_pointer_cast<__component_type>(*component);

        av1Component->init((*component));

        return C2_OK;
    }

    c2_status_t createInterface(
        c2_node_id_t id,
        std::shared_ptr<C2ComponentInterface> * const interface,
        std::function<void(C2ComponentInterface*)> deleter) override {
        *interface = std::shared_ptr<C2ComponentInterface>(
                        new ExynosC2ComponentInterface(((mIsSecure == false)? COMPONENT_NAME: SECURE_COMPONENT_NAME),
                                                       id, std::make_shared<ExynosC2Av1DecComponent::Av1DecParamIntf>(mHelper, mIsSecure)), deleter);

        return C2_OK;
    }

    virtual ~ExynosC2Av1DecFactory() override = default;

private:
    bool mIsSecure;
    std::shared_ptr<C2ReflectorHelper> mHelper;
};

#ifdef __cplusplus
extern "C" {
#endif

EXYNOS_EXPORT_REF extern "C" ::C2ComponentFactory* CreateCodec2Factory() {
    return new ExynosC2Av1DecFactory();
}

EXYNOS_EXPORT_REF extern "C" ::C2ComponentFactory* CreateSecureCodec2Factory() {
    return new ExynosC2Av1DecFactory(true);
}

EXYNOS_EXPORT_REF extern "C" void DestroyCodec2Factory(::C2ComponentFactory *factory) {
    if (factory != nullptr) {
        delete factory;
    }
}

#ifdef __cplusplus
}
#endif

