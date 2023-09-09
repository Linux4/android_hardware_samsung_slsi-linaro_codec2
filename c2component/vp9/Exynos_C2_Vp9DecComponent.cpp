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
#include "Exynos_C2_Vp9DecComponent.h"
#include "Exynos_Vp9Dec_Filter.h"
#include "Exynos_CSC_Filter.h"
#include "Exynos_HDR2SDR_Filter.h"
#include "Exynos_PostControl_Filter.h"
#include "ExynosBufferAllocator.h"

#define LOG_ON
#include "ExynosLog.h"
#undef LOG_TAG
#define LOG_TAG "ExynosC2Vp9DecComponent"

constexpr char COMPONENT_NAME[] = "c2.exynos.vp9.decoder";
constexpr char SECURE_COMPONENT_NAME[] = "c2.exynos.vp9.decoder.secure";


#ifndef USE_BUFFERQUEUE
#define DEFAULT_ALLOCATORSTORE_TYPE C2PlatformAllocatorStore::GRALLOC
#else
#define DEFAULT_ALLOCATORSTORE_TYPE C2PlatformAllocatorStore::BUFFERQUEUE
#endif

ExynosC2FilterManager::FilterModuleInfoList listFilterInfo = {
    {   "vp9dec",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosVp9DecFilter>,
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
    {   "vp9dec.secure",
        &ExynosC2FilterManager::FilterModule::makeFilterModule<ExynosVp9DecFilter>,
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

static C2R HdrDynamicInfoInputSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamHdrDynamicMetadataInfo::input> &me) {
    (void)mayBlock;
    (void)me;  // TODO: validate

    StaticExynosLog(Level::Debug, "Vp9DecParamIntf", "[%s] changed", __FUNCTION__);

    return C2R::Ok();
}

static C2R HdrDynamicInfoOutputSetter(bool mayBlock, C2InterfaceHelper::C2P<C2StreamHdrDynamicMetadataInfo::output> &me) {
    (void)mayBlock;
    (void)me;  // TODO: validate

    StaticExynosLog(Level::Debug, "Vp9DecParamIntf", "[%s] applied", __FUNCTION__);

    return C2R::Ok();
}


class ExynosC2Vp9DecComponent::Vp9DecParamIntf : public ExynosC2DecComponent::VdecCommonParamIntf {
public:
    Vp9DecParamIntf(const std::shared_ptr<C2ReflectorHelper> &helper, bool isSecure = false)
        : VdecCommonParamIntf(
                helper,
                ((isSecure == true)? SECURE_COMPONENT_NAME:COMPONENT_NAME),
                C2Component::KIND_DECODER,
                C2Component::DOMAIN_VIDEO,
                MEDIA_MIMETYPE_VIDEO_VP9,
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
                        C2Config::PROFILE_VP9_0, C2Config::LEVEL_VP9_5))
                .withFields({
                    C2F(mProfileLevel, profile).oneOf({
                            C2Config::PROFILE_VP9_0,
#ifndef UNSUPPORT_10BIT
                            C2Config::PROFILE_VP9_2,
#endif
                    }),
                    C2F(mProfileLevel, level).oneOf({
                            C2Config::LEVEL_VP9_1,
                            C2Config::LEVEL_VP9_1_1,
                            C2Config::LEVEL_VP9_2,
                            C2Config::LEVEL_VP9_2_1,
                            C2Config::LEVEL_VP9_3,
                            C2Config::LEVEL_VP9_3_1,
                            C2Config::LEVEL_VP9_4,
                            C2Config::LEVEL_VP9_4_1,
                            C2Config::LEVEL_VP9_5,
                            C2Config::LEVEL_VP9_5_1,
                            C2Config::LEVEL_VP9_5_2,
                            C2Config::LEVEL_VP9_6,
                    })
                })
                .withSetter(ProfileLevelSetter, mStreamSize)
                .build());

        addHdrStaticInfo();

        /* HDR :: Dynamic Info */
        {
            mHdrDynamicInfoInput = C2StreamHdrDynamicMetadataInfo::input::AllocShared(0, 0u,
                                            C2Config::HDR_DYNAMIC_METADATA_TYPE_SMPTE_2094_40);
            addParameter(
                    DefineParam(mHdrDynamicInfoInput, C2_PARAMKEY_INPUT_HDR_DYNAMIC_INFO)
                    .withDefault(mHdrDynamicInfoInput)
                    .withFields({
                        C2F(mHdrDynamicInfoInput, m.type_).oneOf({ HDR_DYNAMIC_METADATA_TYPE_SMPTE_2094_40 }),
                        C2F(mHdrDynamicInfoInput, m.data).any(),
                    })
                    .withSetter(HdrDynamicInfoInputSetter)
                    .build());

            mHdrDynamicInfoOutput = C2StreamHdrDynamicMetadataInfo::output::AllocShared(0, 0u,
                                            C2Config::HDR_DYNAMIC_METADATA_TYPE_SMPTE_2094_40);
            addParameter(
                    DefineParam(mHdrDynamicInfoOutput, C2_PARAMKEY_OUTPUT_HDR_DYNAMIC_INFO)
                    .withDefault(mHdrDynamicInfoOutput)
                    .withFields({
                        C2F(mHdrDynamicInfoOutput, m.type_).oneOf({ HDR_DYNAMIC_METADATA_TYPE_SMPTE_2094_40 }),
                        C2F(mHdrDynamicInfoOutput, m.data).any(),
                    })
                    .withSetter(HdrDynamicInfoOutputSetter)
                    .build());
        }

        /* extension feature : cropping */
        addCroppingParam(8192, 8192, 8192, 8192, 2);

        /* extension feature : compressed color */
        addCompressedColor();

        /* extension feature : image convert */
        addImageConvertModeParam(mImageConvert, mImageConvertMode, mImageConvertPixelFormat, mImageConvertControlFrame,
                                 cnvImageConvert, cnvImageConvertMode, cnvImageConvertPixelFormat, cnvImageConvertControlFrame);
    }

    virtual ~Vp9DecParamIntf() = default;

    /* getter functions */
    std::shared_ptr<C2StreamHdrDynamicMetadataInfo::output> getHdrDynamicInfo() {
        return mHdrDynamicInfoOutput;
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

        auto intfImpl = std::static_pointer_cast<Vp9DecParamIntf>(intf);

        if (intfImpl->getImageConvert()) {
            auto id = intfImpl->findFilterID("hdr2sdr");
            if (id != 0) {  /* target filter is valid */
                auto filterParam = std::make_shared<ExynosFilterParam<ParamHDR2SDR>>();
                auto param       = std::static_pointer_cast<ExynosParam<ParamHDR2SDR>>(filterParam->getBaseParam());

                param->m.enable = 1;

                StaticExynosLog(Level::Essential, "Vp9DecParamIntf", "[%s] image convert is enabled", __FUNCTION__);

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

        auto intfImpl = std::static_pointer_cast<Vp9DecParamIntf>(intf);

        auto id = intfImpl->findFilterID("hdr2sdr");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamHDR2SDRMode>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamHDR2SDRMode>>(filterParam->getBaseParam());

            param->m.type = intfImpl->getImageConvertMode();

            StaticExynosLog(Level::Essential, "Vp9DecParamIntf", "[%s] image convert mode(%d)", __FUNCTION__, param->m.type);

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

        auto intfImpl = std::static_pointer_cast<Vp9DecParamIntf>(intf);

        auto id = intfImpl->findFilterID("hdr2sdr");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamHDR2SDRPixelFormat>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamHDR2SDRPixelFormat>>(filterParam->getBaseParam());

            param->m.format = intfImpl->getImageConvertPixelFormat();

            StaticExynosLog(Level::Essential, "Vp9DecParamIntf", "[%s] image convert format(0x%x)", __FUNCTION__, param->m.format);

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

        auto intfImpl = std::static_pointer_cast<Vp9DecParamIntf>(intf);

        auto count = intfImpl->getImageConvertControlFrame();

        return DecCommonCnv::cnvImageConvertControlFrameCommon(intf, count);
    }

private:
    std::shared_ptr<C2StreamHdrDynamicMetadataInfo::input>   mHdrDynamicInfoInput;   /* from external */
    std::shared_ptr<C2StreamHdrDynamicMetadataInfo::output>  mHdrDynamicInfoOutput;  /* bypass */

    /* extension feature : image convert */
    std::shared_ptr<C2ExynosPortImageConvertSetting::output>              mImageConvert;  /* for hdr2sdr */
    std::shared_ptr<C2ExynosPortImageConvertModeSetting::output>          mImageConvertMode;
    std::shared_ptr<C2ExynosPortImageConvertPixelFormatSetting::output>   mImageConvertPixelFormat;
    std::shared_ptr<C2ExynosPortImageConvertControlFrameSetting::output>  mImageConvertControlFrame;

    friend class ExynosC2Vp9DecComponent;
};

ExynosC2Vp9DecComponent::ExynosC2Vp9DecComponent(c2_node_id_t id, const std::shared_ptr<Vp9DecParamIntf> &intfImpl)
    : ExynosC2DecComponent(std::make_shared<ExynosC2ComponentInterface>(intfImpl->mName->m.value, id, intfImpl)) {

    mObjName    = "ExynosC2Vp9DecComponent";
    mbLogOff    = false;
    mParamIntf  = std::static_pointer_cast<CommonParamIntf>(intfImpl);

    mThreadPool->setObjName(mObjName);
    return;
}

bool ExynosC2Vp9DecComponent::init(std::weak_ptr<C2Component> wkComponent) {
    ExynosLogFunctionTrace();

    /* TODO : secure */
    mFilterManager = std::make_shared<ExynosC2FilterManager>(mObjName, wkComponent);

    {
        Vp9DecParamIntf::Lock lock = mParamIntf->lock();

        mFilter = mFilterManager->loadFilterModules((mParamIntf->mIsSecure == true)? listSecureFilterInfo:listFilterInfo);
    }

    return setCallback(mFilterManager, mFilterListener);
}

void ExynosC2Vp9DecComponent::onUpdateC2Config(
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

    auto vp9DecIntf = std::static_pointer_cast<Vp9DecParamIntf>(mParamIntf);
#if 0
    if ((buffer->mParams.get() != nullptr) &&
        (!buffer->mParams->empty())) {
        // Todo
    }
#endif
    if (c2buffer.get() != nullptr) {
        /* HDR Static */
        {
            std::shared_ptr<C2StreamHdrStaticInfo::output> codedStaticInfo = nullptr;
            {
                Vp9DecParamIntf::Lock lock = vp9DecIntf->lock();

                codedStaticInfo = vp9DecIntf->getHdrStaticInfo();
            }

            if (codedStaticInfo.get() != nullptr) {
                ExynosLogI("[%s] HDR :: static info[framework] r(%f, %f), g(%f, %f), b(%f, %f), w(%f, %f), max(%f), min(%f), cll(%f), fall(%f)", __FUNCTION__,
                    codedStaticInfo->mastering.red.x, codedStaticInfo->mastering.red.y,
                    codedStaticInfo->mastering.green.x, codedStaticInfo->mastering.green.y,
                    codedStaticInfo->mastering.blue.x, codedStaticInfo->mastering.blue.y,
                    codedStaticInfo->mastering.white.x, codedStaticInfo->mastering.white.y,
                    codedStaticInfo->mastering.maxLuminance, codedStaticInfo->mastering.minLuminance,
                    codedStaticInfo->maxCll, codedStaticInfo->maxFall);

                c2buffer->setInfo(codedStaticInfo);
            }
        }

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

                        c2_status_t err = vp9DecIntf->config({ outParam.get() }, C2_MAY_BLOCK, &failures);
                        if (err == C2_OK) {
                            Vp9DecParamIntf::Lock lock = vp9DecIntf->lock();

                            auto hdrDynamicInfo = vp9DecIntf->getHdrDynamicInfo();

                            c2buffer->setInfo(hdrDynamicInfo);

                            /* for android.media.cts.DecoderTest#testVp9Hdr10PlusMetaData
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

c2_status_t ExynosC2Vp9DecComponent::onSetup() {
    ExynosLogFunctionTrace();

    c2_status_t ret = C2_OK;

    ret = ExynosC2DecComponent::onSetup();
    if (ret != C2_OK) {
        return ret;
    }

    auto vp9DecIntf = std::static_pointer_cast<Vp9DecParamIntf>(mParamIntf);

    /* crop info about bitstream */
    vp9DecIntf->addStreamCrop(8192, 8192);

    std::vector<C2Param::Index> indices;

    indices.push_back(C2StreamColorAspectsInfo::output::PARAM_TYPE);           /* color aspects */
    indices.push_back(C2StreamHdrStaticInfo::output::PARAM_TYPE);              /* hdr static */
    indices.push_back(C2ExynosPortImageConvertSetting::output::PARAM_TYPE);    /* image convert */
    indices.push_back(C2ExynosPortCompressedColorSetting::output::PARAM_TYPE); /* compressed color */

    return makeFilterParam(indices);
}

c2_status_t ExynosC2Vp9DecComponent::onFlush() {
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

class ExynosC2Vp9DecFactory : public C2ComponentFactory {
public:
    ExynosC2Vp9DecFactory(bool isSecure = false)
        : mIsSecure(isSecure),
          mHelper(std::static_pointer_cast<C2ReflectorHelper>(android::GetCodec2ExynosComponentStore()->getParamReflector())) {
        SetStaticExynosLogLevel();
    }

    c2_status_t createComponent(
        c2_node_id_t id,
        std::shared_ptr<C2Component> * const component,
        std::function<void(C2Component*)> deleter) override {
        using __component_type = ExynosC2ComponentWrapper<ExynosC2Vp9DecComponent,
                                                          ExynosC2Vp9DecComponent::Vp9DecParamIntf>;

        *component = std::shared_ptr<C2Component>(
                        new __component_type(id, std::make_shared<ExynosC2Vp9DecComponent::Vp9DecParamIntf>(mHelper, mIsSecure)),
                                                    deleter);

        auto vp9Component = std::static_pointer_cast<__component_type>(*component);

        vp9Component->init((*component));

        return C2_OK;
    }

    c2_status_t createInterface(
        c2_node_id_t id,
        std::shared_ptr<C2ComponentInterface> * const interface,
        std::function<void(C2ComponentInterface*)> deleter) override {
        *interface = std::shared_ptr<C2ComponentInterface>(
                        new ExynosC2ComponentInterface(((mIsSecure == false)? COMPONENT_NAME: SECURE_COMPONENT_NAME),
                                                       id, std::make_shared<ExynosC2Vp9DecComponent::Vp9DecParamIntf>(mHelper, mIsSecure)), deleter);

        return C2_OK;
    }

    virtual ~ExynosC2Vp9DecFactory() override = default;

private:
    bool mIsSecure;
    std::shared_ptr<C2ReflectorHelper> mHelper;
};

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((cfi_canonical_jump_table))
EXYNOS_EXPORT_REF extern "C" ::C2ComponentFactory* CreateCodec2Factory() {
    return new ExynosC2Vp9DecFactory();
}

__attribute__((cfi_canonical_jump_table))
EXYNOS_EXPORT_REF extern "C" ::C2ComponentFactory* CreateSecureCodec2Factory() {
    return new ExynosC2Vp9DecFactory(true);
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

