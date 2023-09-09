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
#include "Exynos_C2_HevcDecComponent.h"
#include "Exynos_HevcDec_Filter.h"
#include "Exynos_CSC_Filter.h"
#include "Exynos_HDR2SDR_Filter.h"
#include "Exynos_PostControl_Filter.h"
#include "ExynosBufferAllocator.h"

#include "VendorVideoAPI.h"

#define LOG_ON
#include "ExynosLog.h"
#undef LOG_TAG
#define LOG_TAG "ExynosC2HevcDecComponent"

constexpr char COMPONENT_NAME[] = "c2.exynos.hevc.decoder";
constexpr char SECURE_COMPONENT_NAME[] = "c2.exynos.hevc.decoder.secure";


#ifndef USE_BUFFERQUEUE
#define DEFAULT_ALLOCATORSTORE_TYPE C2PlatformAllocatorStore::GRALLOC
#else
#define DEFAULT_ALLOCATORSTORE_TYPE C2PlatformAllocatorStore::BUFFERQUEUE
#endif

ExynosC2FilterManager::FilterModuleInfoList listFilterInfo = {
    {   "hevcdec",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosHevcDecFilter>,
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
    {   "hdr2sdr",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosHDR2SDRFilter>,
        DEFAULT_ALLOCATORSTORE_TYPE,
        C2MemoryUsage(DEFAULT_DEC_MEMORY_USAGE),
    },
#endif
};

ExynosC2FilterManager::FilterModuleInfoList listSecureFilterInfo = {
    {   "hevcdec.secure",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosHevcDecFilter>,
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


class ExynosC2HevcDecComponent::HevcDecParamIntf : public ExynosC2DecComponent::VdecCommonParamIntf {
public:
    HevcDecParamIntf(const std::shared_ptr<C2ReflectorHelper> &helper, bool isSecure = false)
        : VdecCommonParamIntf(
                helper,
                ((isSecure == true)? SECURE_COMPONENT_NAME:COMPONENT_NAME),
                C2Component::KIND_DECODER,
                C2Component::DOMAIN_VIDEO,
                MEDIA_MIMETYPE_VIDEO_HEVC,
                ((isSecure != true)? listFilterInfo:listSecureFilterInfo)) {
#ifndef UNSUPPORT_10BIT
        addDecodeDefaultParam(64, 64, 8192, 8192, (320 * 240 * 3 / 2), MaxPictureSizeSetter, true);
#else
        addDecodeDefaultParam(64, 64, 8192, 8192, (320 * 240 * 3 / 2), MaxPictureSizeSetter);
#endif
        /* extension feature : decode order decoding */
        addDecodeOrder(false);
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mDecodeOrder)), DecCommonCnv::cnvDecodeOrder,
                                { mReorderTimestamp });

        addReorderDepth(0);  /* if decode order is supported, reorder depth should be added */

        addOutputDelay(0u, DecCommonSetter::ActualOutputDelaySetterWithExtra, mMaxDPBNumInfo, mExtraBufferNumInfo);

        addReorderKey(C2Config::ordinal_key_t::ORDINAL, DecCommonSetter::ReorderKeySetterWithDecodeOrder, mReorderTimestamp, mDecodeOrder);

        /* supported profile and level about bitstream */
        addParameter(
                DefineParam(mProfileLevel, C2_PARAMKEY_PROFILE_LEVEL)
                .withDefault(new C2StreamProfileLevelInfo::input(0u,
                        C2Config::PROFILE_HEVC_MAIN, C2Config::LEVEL_HEVC_MAIN_5_1))
                .withFields({
                    C2F(mProfileLevel, profile).oneOf({
                            C2Config::PROFILE_HEVC_MAIN,
#ifndef UNSUPPORT_10BIT
                            C2Config::PROFILE_HEVC_MAIN_10,
                            C2Config::PROFILE_HEVC_MAIN_10_INTRA,
#endif
                    }),
                    C2F(mProfileLevel, level).oneOf({
                            /* main */
                            C2Config::LEVEL_HEVC_MAIN_1,
                            C2Config::LEVEL_HEVC_MAIN_2, C2Config::LEVEL_HEVC_MAIN_2_1,
                            C2Config::LEVEL_HEVC_MAIN_3, C2Config::LEVEL_HEVC_MAIN_3_1,
                            C2Config::LEVEL_HEVC_MAIN_4, C2Config::LEVEL_HEVC_MAIN_4_1,
                            C2Config::LEVEL_HEVC_MAIN_5, C2Config::LEVEL_HEVC_MAIN_5_1, C2Config::LEVEL_HEVC_MAIN_5_2,
                            C2Config::LEVEL_HEVC_MAIN_6, C2Config::LEVEL_HEVC_MAIN_6_1,
                            /* high */
                            C2Config::LEVEL_HEVC_HIGH_4, C2Config::LEVEL_HEVC_HIGH_4_1,
                            C2Config::LEVEL_HEVC_HIGH_5, C2Config::LEVEL_HEVC_HIGH_5_1, C2Config::LEVEL_HEVC_HIGH_5_2,
                            C2Config::LEVEL_HEVC_HIGH_6, C2Config::LEVEL_HEVC_HIGH_6_1,
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

        /* extension feature : image convert */
        addImageConvertModeParam(mImageConvert, mImageConvertMode, mImageConvertPixelFormat, mImageConvertControlFrame,
                                 cnvImageConvert, cnvImageConvertMode, cnvImageConvertPixelFormat, cnvImageConvertControlFrame);
    }

    virtual ~HevcDecParamIntf() = default;

    /* getter functions */
    std::shared_ptr<C2StreamHdrDynamicMetadataInfo::output> getHdrDynamicInfo() {
        return mHdrDynamicInfo;
    }

    int32_t getDisplayDelay() const { return mDisplayDelay->value; }

    bool getImageConvert() const { return (mImageConvert->value == On)? true:false; }

    uint32_t getImageConvertMode() const { return mImageConvertMode->value; }

    uint32_t getImageConvertPixelFormat() const { return mImageConvertPixelFormat->value; }

    uint32_t getImageConvertControlFrame() const { return mImageConvertControlFrame->value; }

    /* generate filter param */
    static FilterParamSetterRet cnvDisplayDelay(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<HevcDecParamIntf>(intf);

        return DecCommonCnv::cnvDisplayDelay(intf, intfImpl->getDisplayDelay());
    }

    static FilterParamSetterRet cnvImageConvert(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<HevcDecParamIntf>(intf);

        if (intfImpl->getImageConvert()) {
            auto id = intfImpl->findFilterID("hdr2sdr");
            if (id != 0) {  /* target filter is valid */
                auto filterParam = std::make_shared<ExynosFilterParam<ParamHDR2SDR>>();
                auto param       = std::static_pointer_cast<ExynosParam<ParamHDR2SDR>>(filterParam->getBaseParam());

                param->m.enable = 1;

                StaticExynosLog(Level::Essential, "HevcDecParamIntf", "[%s] image convert is enabled", __FUNCTION__);

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

        auto intfImpl = std::static_pointer_cast<HevcDecParamIntf>(intf);

        auto id = intfImpl->findFilterID("hdr2sdr");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamHDR2SDRMode>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamHDR2SDRMode>>(filterParam->getBaseParam());

            param->m.type = intfImpl->getImageConvertMode();

            StaticExynosLog(Level::Essential, "HevcDecParamIntf", "[%s] image convert mode(%d)", __FUNCTION__, param->m.type);

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

        auto intfImpl = std::static_pointer_cast<HevcDecParamIntf>(intf);

        auto id = intfImpl->findFilterID("hdr2sdr");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamHDR2SDRPixelFormat>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamHDR2SDRPixelFormat>>(filterParam->getBaseParam());

            param->m.format = intfImpl->getImageConvertPixelFormat();

            StaticExynosLog(Level::Essential, "HevcDecParamIntf", "[%s] image convert format(0x%x)", __FUNCTION__, param->m.format);

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

        auto intfImpl = std::static_pointer_cast<HevcDecParamIntf>(intf);

        auto count = intfImpl->getImageConvertControlFrame();

        return DecCommonCnv::cnvImageConvertControlFrameCommon(intf, count);
    }

private:
    std::shared_ptr<C2StreamHdrDynamicMetadataInfo::output> mHdrDynamicInfo; /* from bitstream */

    /* extension feature : display delay */
    std::shared_ptr<C2ExynosPortDisplayDelaySetting::output> mDisplayDelay;

    /* extension feature : image convert */
    std::shared_ptr<C2ExynosPortImageConvertSetting::output>             mImageConvert;  /* for hdr2sdr */
    std::shared_ptr<C2ExynosPortImageConvertModeSetting::output>          mImageConvertMode;
    std::shared_ptr<C2ExynosPortImageConvertPixelFormatSetting::output>   mImageConvertPixelFormat;
    std::shared_ptr<C2ExynosPortImageConvertControlFrameSetting::output>  mImageConvertControlFrame;

    friend class ExynosC2HevcDecComponent;
};

ExynosC2HevcDecComponent::ExynosC2HevcDecComponent(c2_node_id_t id, const std::shared_ptr<HevcDecParamIntf> &intfImpl)
    : ExynosC2DecComponent(std::make_shared<ExynosC2ComponentInterface>(intfImpl->mName->m.value, id, intfImpl)) {

    mObjName    = "ExynosC2HevcDecComponent";
    mbLogOff    = false;
    mParamIntf  = std::static_pointer_cast<CommonParamIntf>(intfImpl);

    mThreadPool->setObjName(mObjName);
    return;
}

bool ExynosC2HevcDecComponent::init(std::weak_ptr<C2Component> wkComponent) {
    ExynosLogFunctionTrace();

    /* TODO : secure */
    mFilterManager = std::make_shared<ExynosC2FilterManager>(mObjName, wkComponent);

    {
        HevcDecParamIntf::Lock lock = mParamIntf->lock();

        mFilter = mFilterManager->loadFilterModules((mParamIntf->mIsSecure == true)? listSecureFilterInfo:listFilterInfo);
    }

    return setCallback(mFilterManager, mFilterListener);
}

void ExynosC2HevcDecComponent::onUpdateC2Config(
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

    auto hevcDecIntf = std::static_pointer_cast<HevcDecParamIntf>(mParamIntf);

    if ((buffer->mParams.get() != nullptr) &&
        (!buffer->mParams->empty())) {
        // Todo
    }

    ExynosC2DecComponent::updateC2Buffer_HDRInfo(c2buffer, buffer);

    return;
}

c2_status_t ExynosC2HevcDecComponent::onSetup() {
    ExynosLogFunctionTrace();

    c2_status_t ret = C2_OK;

    ret = ExynosC2DecComponent::onSetup();
    if (ret != C2_OK) {
        return ret;
    }

    auto hevcDecIntf = std::static_pointer_cast<HevcDecParamIntf>(mParamIntf);

    /* crop info about bitstream */
    hevcDecIntf->addStreamCrop(8192, 8192);

    std::vector<C2Param::Index> indices;

    indices.push_back(C2ExynosPortDecodeOrderSetting::output::PARAM_TYPE);     /* decode order decoding */
    indices.push_back(C2StreamColorAspectsInfo::output::PARAM_TYPE);           /* color aspects */
    indices.push_back(C2ExynosPortImageConvertSetting::output::PARAM_TYPE);    /* image convert */
    indices.push_back(C2ExynosPortCompressedColorSetting::output::PARAM_TYPE); /* compressed color */

    return makeFilterParam(indices);
}

c2_status_t ExynosC2HevcDecComponent::onFlush() {
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

class ExynosC2HevcDecFactory : public C2ComponentFactory {
public:
    ExynosC2HevcDecFactory(bool isSecure = false)
        : mIsSecure(isSecure),
          mHelper(std::static_pointer_cast<C2ReflectorHelper>(android::GetCodec2ExynosComponentStore()->getParamReflector())) {
        SetStaticExynosLogLevel();
    }

    c2_status_t createComponent(
        c2_node_id_t id,
        std::shared_ptr<C2Component> * const component,
        std::function<void(C2Component*)> deleter) override {
        using __component_type = ExynosC2ComponentWrapper<ExynosC2HevcDecComponent,
                                                          ExynosC2HevcDecComponent::HevcDecParamIntf>;

        *component = std::shared_ptr<C2Component>(
                        new __component_type(id, std::make_shared<ExynosC2HevcDecComponent::HevcDecParamIntf>(mHelper, mIsSecure)),
                                                     deleter);

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
                                                       id, std::make_shared<ExynosC2HevcDecComponent::HevcDecParamIntf>(mHelper, mIsSecure)), deleter);

        return C2_OK;
    }

    virtual ~ExynosC2HevcDecFactory() override = default;

private:
    bool mIsSecure;
    std::shared_ptr<C2ReflectorHelper> mHelper;
};

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((cfi_canonical_jump_table))
EXYNOS_EXPORT_REF extern "C" ::C2ComponentFactory* CreateCodec2Factory() {
    return new ExynosC2HevcDecFactory();
}

__attribute__((cfi_canonical_jump_table))
EXYNOS_EXPORT_REF extern "C" ::C2ComponentFactory* CreateSecureCodec2Factory() {
    return new ExynosC2HevcDecFactory(true);
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

