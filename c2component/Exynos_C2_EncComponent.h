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
#ifndef EXYNOS_C2_ENCCOMPONENT_H
#define EXYNOS_C2_ENCCOMPONENT_H

#include <vector>
#include <list>
#include <memory>

#include <system/graphics.h>
#include <Codec2Mapper.h>
#include "exynos_format.h"

#include "Exynos_C2_Component.h"
#include "ExynosBufferAllocator.h"

#define LOG_ON
#include "ExynosLog.h"

#ifdef USE_SW_CSC
#define DEFAULT_CSC_MEMORY_USAGE (android::C2AndroidMemoryUsage::HW_CODEC_WRITE |\
                                  android::C2AndroidMemoryUsage::HW_CODEC_READ |\
                                  C2MemoryUsage::CPU_READ)
#else
#define DEFAULT_CSC_MEMORY_USAGE (android::C2AndroidMemoryUsage::HW_CODEC_WRITE |\
                                  android::C2AndroidMemoryUsage::HW_CODEC_READ)
#endif

#define DEFAULT_ENC_MEMORY_USAGE (android::C2AndroidMemoryUsage::HW_CODEC_WRITE |\
                                  C2MemoryUsage::CPU_READ)

#define DEFAULT_SECURE_ENC_MEMORY_USAGE (android::C2AndroidMemoryUsage::HW_CODEC_WRITE |\
                                         C2MemoryUsage::WRITE_PROTECTED |\
                                         C2MemoryUsage::READ_PROTECTED)

inline void ParseGop(
        const C2StreamGopTuning::output &gop,
        uint32_t *syncInterval, uint32_t *iInterval, uint32_t *maxBframes) {
    uint32_t syncInt = 1;
    uint32_t iInt = 1;
    for (size_t i = 0; i < gop.flexCount(); ++i) {
        const C2GopLayerStruct &layer = gop.m.values[i];
        if (layer.count == UINT32_MAX) {
            syncInt = 0;
        } else if (syncInt <= UINT32_MAX / (layer.count + 1)) {
            syncInt *= (layer.count + 1);
        }
        if ((layer.type_ & I_FRAME) == 0) {
            if (layer.count == UINT32_MAX) {
                iInt = 0;
            } else if (iInt <= UINT32_MAX / (layer.count + 1)) {
                iInt *= (layer.count + 1);
            }
        }
        if (layer.type_ == C2Config::picture_type_t(P_FRAME | B_FRAME) && maxBframes) {
            *maxBframes = layer.count;
        }
    }
    if (syncInterval) {
        *syncInterval = syncInt;
    }
    if (iInterval) {
        *iInterval = iInt;
    }
}

class ExynosC2EncComponent : public ExynosC2Component/*, public std::enable_shared_from_this<ExynosC2EncComponent>*/ {
public:
    class VencCommonParamIntf;

    ExynosC2EncComponent(const std::shared_ptr<C2ComponentInterface> &intf) : ExynosC2Component(intf) {
        mObjName = "ExynosC2EncComponent";
        ExynosMutex<ComponentState>::LockObj comp(mStateMutex);

        mThreadPool->setObjName(mObjName);
#ifdef USE_DEFFERING_WORK_QUEUE
        mDeferringWorkQueue = std::make_shared<ExynosMutex<ExynosQueue<std::shared_ptr<WorkQueueElement>>>>();
#endif

        mWidth = 0;
        mHeight = 0;
        mActualFormat = 0;
    }

    ~ExynosC2EncComponent() = default;

protected:
    /* override function on ExynosC2Component */
    c2_status_t onStart() override;
    c2_status_t onSetup() override;
    bool        onQueue(std::shared_ptr<WorkQueueElement> workElement) override;
    void onUpdateC2Work(std::unique_ptr<C2Work> &c2work,
                        std::shared_ptr<ExynosBuffer> inBuffer = nullptr,
                        std::shared_ptr<ExynosBuffer> outBuffer = nullptr) override;
    virtual void onUpdateC2Config(std::shared_ptr<C2Buffer> c2buffer, std::shared_ptr<ExynosBuffer> buffer,
                                  std::vector<std::unique_ptr<C2Param>> &inConfig,
                                  std::vector<std::unique_ptr<C2Param>> &outConfig);
    void setBlockPool() override;
    std::optional<std::shared_ptr<ExynosBuffer>> getExynosBuffer(C2FrameData &input) override;

    /* function for ExynosC2EncComponent's child class */
    void updateC2Config_Subscribes(std::shared_ptr<C2Buffer> c2buffer,
                                   std::shared_ptr<ExynosBuffer> buffer);
    void updateC2Config_AverageQp(std::shared_ptr<C2Buffer> c2buffer,
                                  std::shared_ptr<ExynosBuffer> buffer);

    /* for replica buffer */
    int mWidth;
    int mHeight;
    int mActualFormat;

private:
    /* disable default constructor */
    ExynosC2EncComponent() = delete;
};

class ExynosC2EncComponent::VencCommonParamIntf : public ExynosC2Component::CommonParamIntf {
public:
    VencCommonParamIntf(
        const std::shared_ptr<C2ReflectorHelper> &reflector,
        C2String name,
        C2Component::kind_t kind,
        C2Component::domain_t domain,
        C2String mediaType,
        ExynosC2FilterManager::FilterModuleInfoList listFilterInfo);

    virtual ~VencCommonParamIntf() = default;

    /* getter functions */
    std::shared_ptr<C2StreamPictureSizeInfo::input> getStreamSize() const { return mStreamSize; }
    uint32_t getPixelFormat() const { return mPixelFormat->value; }
    uint32_t getActualFormat() const { return mActualFormat->value; }
    std::shared_ptr<C2StreamProfileLevelInfo::output> getProfileLevelInfo() const { return mProfileLevel; }
    float    getFrameRate() const { return mFrameRate->value; }
    uint32_t getBitrate() const { return mBitrate->value; }
    uint32_t getBitrateMode() { return mBitrateMode->value; }
    uint32_t getVendorBitrateMode() { return mVendorBitrateMode->value; }
    uint64_t getInternalUsage() const { return mInternalUsage->value; }
    uint32_t getHdrFormat() const {
        if (mHdrFormat->value == C2Config::hdr_format_t::UNKNOWN) {
            return C2Config::hdr_format_t::SDR;
        }

        return mHdrFormat->value;
    }

    bool getStartIDRFrame() const {
        bool ret = mStartIDRFrame->value;
        mStartIDRFrame->value = false;  /* reset */
        return ret;
    }

    bool getRequestSyncFrame() const {
        bool ret = mRequestSyncFrame->value;
        mRequestSyncFrame->value = false;  /* reset */
        return ret;
    }

    uint32_t getIDRPeriod() const {
        if ((mSyncFramePeriod->value < 0) || (mSyncFramePeriod->value == INT64_MAX)) {
            return 0;
        }

        /* mSyncFramePeriod is time domain */
        return c2_max(c2_min((double)(mFrameRate->value * (mSyncFramePeriod->value / 1E6) + 0.5), double(UINT32_MAX)), 1.0);
    }

    std::shared_ptr<C2StreamIntraRefreshTuning::output> getIntraRefresh() const { return mIntraRefresh; }

    CropInfo getInputCrop() const {
        CropInfo crop;

        crop.nWidth     = mPictureCropInfo->width;
        crop.nHeight    = mPictureCropInfo->height;
        crop.nLeft      = mPictureCropInfo->left;
        crop.nTop       = mPictureCropInfo->top;

        return crop;
    }

    std::shared_ptr<C2StreamScaledPictureSizeTuning::input> getPictureSize() const { return mPictureSize; }

    std::shared_ptr<C2StreamColorAspectsInfo::input> getColorAspects_l() { return mColorAspects; }

    uint32_t getDataSpace() { return mCSCDataSpace->value; }

    std::shared_ptr<C2ExynosPortQpRangeTuning::output> getQpRange() { return mQpRange; }

    bool getDropControl() { return (mDropControl->value == On)? true:false; }

    bool getDynamicFramerate() { return (mDynamicFramerate->value == 0)? true:false; }

    bool getAverageQp() { return (mAverageQp->value == On)? true:false; }

    uint32_t getMinQuality() { return (uint32_t)mMinQuality->value; }

    std::shared_ptr<C2ExynosStreamPMVTuning::output> getPMV() { return mPMV; }

protected:
    void addEncodeDefaultParam(struct C2PictureSizeStruct def, struct C2PictureSizeStruct min, struct C2PictureSizeStruct max) {
        addParameter(
                DefineParam(mAttribute, C2_PARAMKEY_COMPONENT_ATTRIBUTES)
                .withConstValue(new C2ComponentAttributesSetting(C2Component::ATTRIB_IS_TEMPORAL))
                .build());

        /* input size about raw data. it is same as input of MFC D/D */
        addParameter(
            DefineParam(mStreamSize, C2_PARAMKEY_PICTURE_SIZE)
            .withDefault(new C2StreamPictureSizeInfo::input(0u, def.width, def.height))
            .withFields({
                C2F(mStreamSize, width).inRange(min.width, max.width, 2),
                C2F(mStreamSize, height).inRange(min.height, max.height, 2),
            })
            .withSetter(EncCommonSetter::SizeSetter)
            .build());
        // addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mStreamSize)), cnvStreamSize);
    }

    void addActualFormatParam(bool support10bit = false) {
        addParameter(
                DefineParam(m10BitSupport, C2EXYNOS_PARAMKEY_ENC_INPUT_10BIT_SUPPORT)
                .withConstValue(new C2Exynos10BitSupportInfo(support10bit))
                .build());

        addHdrFormat();

        {
            std::vector<unsigned int> inputFormatList;

            inputFormatList = { HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED,
                                HAL_PIXEL_FORMAT_YCBCR_420_888,
                                HAL_PIXEL_FORMAT_YV12,
                                HAL_PIXEL_FORMAT_YCrCb_420_SP,
                                HAL_PIXEL_FORMAT_RGBA_8888,
                                HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN,
                                HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M,
                                HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M };

            if (support10bit) {
                inputFormatList.push_back(HAL_PIXEL_FORMAT_YCBCR_P010);
#ifndef USE_SW_CSC
                inputFormatList.push_back(HAL_PIXEL_FORMAT_RGBA_1010102);
#endif
            }

            addParameter(
                DefineParam(mPixelFormat, C2_PARAMKEY_PIXEL_FORMAT)
                .withDefault(new C2StreamPixelFormatInfo::input(0u, HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED))
                .withFields({C2F(mPixelFormat, value).oneOf(inputFormatList)})
                .withSetter(EncCommonSetter::PixelFormatSetter)
                .build());
        }

        addParameter(
            DefineParam(mActualFormat, C2EXYNOS_PARAMKEY_CSC_OUTPUT_PIXEL_FORMAT)
            .withDefault(new C2ExynosCSCOutputPixelFormatInfo(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED))
            .withFields({C2F(mActualFormat, value).oneOf({
                                HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED,
                                HAL_PIXEL_FORMAT_YCBCR_420_888,
                                HAL_PIXEL_FORMAT_YV12,
                                HAL_PIXEL_FORMAT_EXYNOS_YV12_M,
                                HAL_PIXEL_FORMAT_RGBA_8888,
#ifndef USE_SW_CSC
                                HAL_PIXEL_FORMAT_RGBA_1010102,
#endif
                                HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN,
                                HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M,
                                HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M,
                                HAL_PIXEL_FORMAT_YCBCR_P010,
                                HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M,
                                HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC,
                                HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC,
#ifdef USE_SUPPORT_GPU_SBWC
                                HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_256_SBWC,
                                HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_256_SBWC,
#endif
                                HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC,
                                HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L40,
                                HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L60,
                                HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L80,
                                HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC,
                                HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L50,
                                HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L75,
                                HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_32_SBWC_L,
                                HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_64_SBWC_L,
                                HAL_PIXEL_FORMAT_EXYNOS_420_SPN_32_SBWC_L,
                                HAL_PIXEL_FORMAT_EXYNOS_420_SPN_64_SBWC_L,
                                HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_10B_32_SBWC_L,
                                HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_10B_64_SBWC_L,
                                HAL_PIXEL_FORMAT_EXYNOS_420_SPN_10B_32_SBWC_L,
                                HAL_PIXEL_FORMAT_EXYNOS_420_SPN_10B_64_SBWC_L})})
            .withSetter(EncCommonSetter::ActualFormatSetter, mPixelFormat, mCSCDataSpace, m10BitSupport)
            .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mActualFormat)), EncCommonCnv::cnvActualFormat,
                                { mPixelFormat, mCSCDataSpace });
    }

    void addPrependHeaderModeParam(std::shared_ptr<C2PrependHeaderModeSetting> &prependHeaderMode,
                                   FilterParamSetterFunc func) {
        addParameter(
            DefineParam(prependHeaderMode, C2_PARAMKEY_PREPEND_HEADER_MODE)
            .withDefault(new C2PrependHeaderModeSetting(C2Config::PREPEND_HEADER_TO_NONE))
            .withFields({C2F(prependHeaderMode, value).oneOf({
                            C2Config::PREPEND_HEADER_TO_NONE,
                            C2Config::PREPEND_HEADER_ON_CHANGE,
                            C2Config::PREPEND_HEADER_TO_ALL_SYNC})
            })
            .withSetter(Setter<decltype(*prependHeaderMode)>::StrictValueWithNoDeps)
            .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(prependHeaderMode)), func);
    }

    template<typename ... Deps>
    void addBitrateModeParam(const std::vector<uint32_t> values,
                             C2R (*setter)(bool, C2P<C2ExynosStreamBitrateModeTuning::output> &, const C2P<Deps> &...),
                             std::shared_ptr<Deps>& ... deps) {
        std::vector<uint32_t> supportedValues = { C2Config::BITRATE_VARIABLE,
                                                  C2Config::BITRATE_IGNORE,
                                                  C2Config::BITRATE_CONST };
        if (!values.empty()) {
            supportedValues.insert(supportedValues.end(), values.begin(), values.end());
        }

        std::list<std::shared_ptr<C2Param>> listDeps = { std::static_pointer_cast<C2Param>(deps)... };

        addParameter(
                DefineParam(mBitrateMode, C2_PARAMKEY_BITRATE_MODE)
                .withDefault(new C2StreamBitrateModeTuning::output(
                        0u, (C2Config::bitrate_mode_t)supportedValues[0]))
                .withFields({
                    C2F(mBitrateMode, value).oneOf(supportedValues)
                })
                .withSetter(setter, deps...)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mBitrateMode)), EncCommonCnv::cnvBitrateMode,
                                listDeps);

        int32_t mode = 0;
        C2Mapper::map((C2Config::bitrate_mode_t)supportedValues[0], &mode);
        addParameter(
                DefineParam(mVendorBitrateMode, C2EXYNOS_PARAMKEY_BITRATE_MODE)  /* it will be deprecated. setting for vendor's define */
                .withDefault(new C2ExynosStreamBitrateModeTuning::input(0u, (C2Config::bitrate_mode_t)mode))
                .withFields({
                    C2F(mVendorBitrateMode, value).any()
                })
                .withSetter(EncCommonSetter::VendorBitrateModeSetter)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mVendorBitrateMode)), EncCommonCnv::cnvVendorBitrateMode);
    }

    void addBitrateParam(std::shared_ptr<C2StreamBitrateInfo::output> &bitrate,
                         uint64_t defaultValue, uint64_t rangeMin, uint64_t rangeMax,
                         C2R (*setterFunc)(bool mayBlock, C2P<C2StreamBitrateInfo::output> &me),
                         FilterParamSetterFunc func) {
        addParameter(
                DefineParam(bitrate, C2_PARAMKEY_BITRATE)
                .withDefault(new C2StreamBitrateInfo::output(0u, defaultValue))
                .withFields({C2F(bitrate, value).inRange(rangeMin, rangeMax)})
                .withSetter(setterFunc)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(bitrate)), func);
    }

    template<typename ... Deps>
    void addBitrateParamExt(std::shared_ptr<C2StreamBitrateInfo::output> &bitrate,
                         uint64_t defaultValue, uint64_t rangeMin, uint64_t rangeMax,
                         C2R (*setterFunc)(bool, C2P<C2StreamBitrateInfo::output> &, const C2P<Deps> &...),
                         FilterParamSetterFunc func,
                         std::shared_ptr<Deps>& ... deps) {
        std::list<std::shared_ptr<C2Param>> listDeps = { std::static_pointer_cast<C2Param>(deps)... };

        addParameter(
                DefineParam(bitrate, C2_PARAMKEY_BITRATE)
                .withDefault(new C2StreamBitrateInfo::output(0u, defaultValue))
                .withFields({C2F(bitrate, value).inRange(rangeMin, rangeMax)})
                .withSetter(setterFunc, deps...)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(bitrate)), func, listDeps);
    }

    void addTemporalLayeringParam(std::shared_ptr<C2StreamTemporalLayeringTuning::output> &layering,
                                  std::shared_ptr<C2StreamBitrateInfo::output> &bitrate,
                                  uint64_t layerCountMaxValue,
                                  uint64_t bLayerCountMaxValue,
                                  C2R (*setterFunc)(bool mayBlock, C2P<C2StreamTemporalLayeringTuning::output>& me),
                                  FilterParamSetterFunc func) {
        addParameter(
            DefineParam(layering, C2_PARAMKEY_TEMPORAL_LAYERING)
                .withDefault(C2StreamTemporalLayeringTuning::output::AllocShared(0u, 0, 0, 0))
                .withFields({
                    C2F(layering, m.layerCount).inRange(0, layerCountMaxValue),
                    C2F(layering, m.bLayerCount).inRange(0, bLayerCountMaxValue),
                    C2F(layering, m.bitrateRatios).inRange(0., 1.)
                })
                .withSetter(setterFunc)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(layering)), func,
                                { bitrate });
    }

    void addCroppingScalingParam(uint32_t maxLSize, uint32_t maxTSize, uint32_t maxWSize, uint32_t maxHSize, uint32_t step) {
        /* extension feature : cropping and scaling */
        {
            /* crop info about raw data. it could be used as csc's input */
            addParameter(
                    DefineParam(mPictureCropInfo, C2_PARAMKEY_SCALED_CROP_RECT)
                    .withDefault(new C2StreamScaledCropRectTuning::input(0u, C2Rect(0, 0)))
                    .withFields({
                        C2F(mPictureCropInfo, width).inRange(0, maxWSize, step),
                        C2F(mPictureCropInfo, height).inRange(0, maxHSize, step),
                        C2F(mPictureCropInfo, left).inRange(0, maxLSize, step),
                        C2F(mPictureCropInfo, top).inRange(0, maxTSize, step),
                    })
                    .withSetter(EncCommonSetter::CropRectSetter<C2StreamScaledCropRectTuning::input, C2StreamPictureSizeInfo::input>, mStreamSize)
                    .build());
            addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mPictureCropInfo)), EncCommonCnv::cnvInputCrop,
                                    { mStreamSize });

            /* size about scaled picture. if it is set, scaling is required. it could be used as csc's output */
            addParameter(
                    DefineParam(mPictureSize, C2_PARAMKEY_SCALED_PICTURE_SIZE)
                    .withDefault(new C2StreamScaledPictureSizeTuning::input(0u, 0, 0))
                    .withFields({
                        C2F(mPictureSize, width).inRange(0, maxWSize, step),
                        C2F(mPictureSize, height).inRange(0, maxHSize, step),
                    })
                    .withSetter(EncCommonSetter::ScaleSizeSetter<C2StreamScaledPictureSizeTuning::input>)
                    .build());
            addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mPictureSize)), EncCommonCnv::cnvScaleSize);
        }
    }

    void addQpRangeParam(uint32_t minISize, uint32_t maxISize,
                         uint32_t minPSize, uint32_t maxPSize,
                         uint32_t minBSize, uint32_t maxBSize,
                         C2R (*setter)(bool, C2P<C2StreamPictureQuantizationTuning::output> &)) {
        /* quantization */
        {
            addParameter(
                    DefineParam(mQuantization, C2_PARAMKEY_PICTURE_QUANTIZATION)
                    .withDefault(C2StreamPictureQuantizationTuning::output::AllocShared(0, 0u))
                    .withFields({
                        C2F(mQuantization, m.values[0].type_).oneOf({
                                C2Config::picture_type_t(I_FRAME),
                                C2Config::picture_type_t(P_FRAME),
                                C2Config::picture_type_t(B_FRAME)}),
                        C2F(mQuantization, m.values[0].min).any(),
                        C2F(mQuantization, m.values[0].max).any()
                    })
                    .withSetter(setter)
                    .build());
            addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mQuantization)), nullptr);
        }

        /* extension feature : qp range */
        {
            addParameter(
                    DefineParam(mQpRange, C2EXYNOS_PARAMKEY_QP_RANGE)
                    .withDefault(new C2ExynosPortQpRangeTuning::output(minISize, maxISize, minPSize, maxPSize, minBSize, maxBSize))
                    .withFields({
                        C2F(mQpRange, I_minQP).any(),
                        C2F(mQpRange, I_maxQP).any(),
                        C2F(mQpRange, P_minQP).any(),
                        C2F(mQpRange, P_maxQP).any(),
                        C2F(mQpRange, B_minQP).any(),
                        C2F(mQpRange, B_maxQP).any(),
                    })
                    .withSetter(EncCommonSetter::QpRangeSetter, mQuantization)
                    .build());
            addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mQpRange)), EncCommonCnv::cnvQpRange);
        }
    }

    template<typename ... Deps>
    void addGopActualInputDelayParam(std::shared_ptr<C2StreamGopTuning::output> &gop,
                                std::shared_ptr<C2PortActualDelayTuning::input> &actualInputDelay,
                                C2R (*gopSetter)(bool mayBlock, C2P<C2StreamGopTuning::output> &me),
                                FilterParamSetterFunc func,
                                uint32_t defaultB, uint32_t maxB,
                                C2R (*inputDelaySetter)(bool, C2P<C2PortActualDelayTuning::input> &,
                                                        const C2P<C2StreamGopTuning::output> &, const C2P<Deps> &...),
                                std::shared_ptr<Deps>& ... deps) {
        addParameter(
                DefineParam(gop, C2_PARAMKEY_GOP)
                .withDefault(C2StreamGopTuning::output::AllocShared(
                        0 /* flexCount */, 0u /* stream */))
                .withFields({C2F(gop, m.values[0].type_).any(),
                             C2F(gop, m.values[0].count).any()})
                .withSetter(gopSetter)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(gop)), func);

        addParameter(
                DefineParam(actualInputDelay, C2_PARAMKEY_INPUT_DELAY)
                .withDefault(new C2PortActualDelayTuning::input(defaultB))
                .withFields({C2F(actualInputDelay, value).inRange(0, maxB)})
                .calculatedAs(inputDelaySetter, gop, deps...)
                .build());
    }

    template<typename ... Deps>
    void addRefPframesParam(std::shared_ptr<C2ExynosPortRefPframeSetting::output> &refPFrames,
                         C2R (*setter)(bool, C2P<C2ExynosPortRefPframeSetting::output> &, const C2P<Deps> &...),
                         FilterParamSetterFunc func,
                         std::shared_ptr<Deps>& ... deps) {
        /* extension feature : ref p frames */
        {
            addParameter(
                    DefineParam(refPFrames, C2EXYNOS_PARAMKEY_REF_P_FRAMES)
                    .withDefault(new C2ExynosPortRefPframeSetting::output(1))
                    .withFields({ C2F(refPFrames, value).inRange(1, 2) })
                    .withSetter(setter, deps...)
                    .build());
            addFilterParamSetter(*(std::static_pointer_cast<C2Param>(refPFrames)), func);
        }
    }

    void addIFrameRatioParam(std::shared_ptr<C2ExynosStreamIFrameRatioTunning::output> &iFrameRatio, FilterParamSetterFunc cnvIFrameRatio) {
        /* i-frame ratio */
        addParameter(
                DefineParam(iFrameRatio, C2EXYNOS_PARAMKEY_I_FRAME_RATIO)
                .withDefault(new C2ExynosStreamIFrameRatioTunning::output(0u, 0))
                .withFields({ C2F(iFrameRatio, value).inRange(15, 50) })
                .withSetter(Setter<decltype(*iFrameRatio)>::StrictValueWithNoDeps)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(iFrameRatio)), cnvIFrameRatio);
    }

    void addChromaQpOffsetParam(std::shared_ptr<C2ExynosStreamChromaQpOffsetSetting::output> &chromaQpOffset,
                                C2R (*chromaQpOffsetSetter)(bool, C2P<C2ExynosStreamChromaQpOffsetSetting::output>&),
                                FilterParamSetterFunc cnvChromaQpOffset) {
        /* chroma qp offset */
        addParameter(
                DefineParam(chromaQpOffset, C2EXYNOS_PARAMKEY_CHROMA_QP_OFFSET)
                .withDefault(new C2ExynosStreamChromaQpOffsetSetting::output(0u, 0, 0))
                .withFields({
                    C2F(chromaQpOffset, Cb).inRange(-12, 12),
                    C2F(chromaQpOffset, Cr).inRange(-12, 12),
                })
                .withSetter(chromaQpOffsetSetter)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(chromaQpOffset)), cnvChromaQpOffset);
    }

    void addPMV(bool global = false) {
        addParameter(
                DefineParam(mPMV, C2EXYNOS_PARAMKEY_PMV)
                .withDefault(new C2ExynosStreamPMVTuning::output(0u, VendorC2Config::PMV_DISABLED, 0, 0, 0, 0))
                .withFields({
                    C2F(mPMV, Mode).inRange(VendorC2Config::PMV_DISABLED,
                                            (global)? VendorC2Config::PMV_GLOBAL:VendorC2Config::PMV_LOCAL),
                    C2F(mPMV, HorizonL0).any(),
                    C2F(mPMV, HorizonL1).any(),
                    C2F(mPMV, VerticalL0).any(),
                    C2F(mPMV, VerticalL1).any(),
                })
                .withSetter(EncCommonSetter::PMVSetter)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mPMV)), EncCommonCnv::cnvPMV);

        return;
    }

    void addHdrFormat() {
        std::vector<uint32_t> hdrFormatList;
        hdrFormatList = {
            C2Config::hdr_format_t::UNKNOWN,
            C2Config::hdr_format_t::SDR };

        if ((m10BitSupport.get() != nullptr) &&
            (m10BitSupport->value)) {
            hdrFormatList.insert(hdrFormatList.end(), { C2Config::hdr_format_t::HLG,
                                                        C2Config::hdr_format_t::HDR10,
                                                        C2Config::hdr_format_t::HDR10_PLUS});
        }

        addParameter(
                DefineParam(mHdrFormat, C2_PARAMKEY_HDR_FORMAT)
                .withDefault(new C2StreamHdrFormatInfo::output(0u, C2Config::hdr_format_t::UNKNOWN))
                .withFields({C2F(mHdrFormat, value).oneOf(hdrFormatList)})
                .withSetter(EncCommonSetter::HdrFormatSetter)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mHdrFormat)), EncCommonCnv::cnvHdrFormat);
    }

    void addHdrStaticInfo(std::shared_ptr<C2StreamHdrStaticInfo::input> &hdrStaticInfo,
                            FilterParamSetterFunc cnvHdrStaticInfo) {
        mReflector->addStructDescriptors<C2MasteringDisplayColorVolumeStruct, C2ColorXyStruct>();

        hdrStaticInfo = std::make_shared<C2StreamHdrStaticInfo::input>();

        /* sample BT.2020 */
        hdrStaticInfo->mastering.red   = {0.708, 0.292};
        hdrStaticInfo->mastering.green = {0.170, 0.797};
        hdrStaticInfo->mastering.blue  = {0.131, 0.046};
        hdrStaticInfo->mastering.white = {0.3127, 0.3290};
        hdrStaticInfo->mastering.maxLuminance = 1000;
        hdrStaticInfo->mastering.minLuminance = 0.1;
        hdrStaticInfo->maxCll = 1000;
        hdrStaticInfo->maxFall = 120;

        /* TODO */
        hdrStaticInfo->mastering.minLuminance = 0;  /* SMPTE2086. default value for marking invalidation */
        hdrStaticInfo->maxCll = 0;  /* CTA861-3. default value for marking invalidation */
        hdrStaticInfo->maxFall = 65536;  /* format. default value for marking invalidation */

        addParameter(
                DefineParam(hdrStaticInfo, C2_PARAMKEY_HDR_STATIC_INFO)
                .withDefault(hdrStaticInfo)
                .withSetter(EncCommonSetter::HdrStaticInfoSetter)
                .withFields({
                    C2F(hdrStaticInfo, mastering.red.x).any(), // .inRange(0, 1),
                    C2F(hdrStaticInfo, mastering.red.y).any(), // .inRange(0, 1),
                    C2F(hdrStaticInfo, mastering.green.x).any(), // .inRange(0, 1),
                    C2F(hdrStaticInfo, mastering.green.y).any(), // .inRange(0, 1),
                    C2F(hdrStaticInfo, mastering.blue.x).any(), // .inRange(0, 1),
                    C2F(hdrStaticInfo, mastering.blue.y).any(), // .inRange(0, 1),
                    C2F(hdrStaticInfo, mastering.white.x).any(), // .inRange(0, 1),
                    C2F(hdrStaticInfo, mastering.white.y).any(), // .inRange(0, 1),
                    C2F(hdrStaticInfo, mastering.maxLuminance).any(), // .inRange(0, 65535),
                    C2F(hdrStaticInfo, mastering.minLuminance).any(), // .inRange(0, 6.5535),
                    C2F(hdrStaticInfo, maxCll).any(), // .inRange(0, 65535),
                    C2F(hdrStaticInfo, maxFall).any(), // .inRange(0, 65535),
                })
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(hdrStaticInfo)), cnvHdrStaticInfo);
    }

    void addHdrDynamicInfo(std::shared_ptr<C2StreamHdrDynamicMetadataInfo::input> &hdrDynamicInfoInput,
                            std::shared_ptr<C2StreamHdrDynamicMetadataInfo::output> &hdrDynamicInfoOutput,
                            FilterParamSetterFunc cnvHdrDynamicInfo = nullptr) {
        hdrDynamicInfoInput = C2StreamHdrDynamicMetadataInfo::input::AllocShared(0, 0u,
                                        C2Config::HDR_DYNAMIC_METADATA_TYPE_SMPTE_2094_40);
        addParameter(
                DefineParam(hdrDynamicInfoInput, C2_PARAMKEY_INPUT_HDR_DYNAMIC_INFO)
                .withDefault(hdrDynamicInfoInput)
                .withFields({
                    C2F(hdrDynamicInfoInput, m.type_).oneOf({ HDR_DYNAMIC_METADATA_TYPE_SMPTE_2094_40 }),
                    C2F(hdrDynamicInfoInput, m.data).any(),
                })
                .withSetter(EncCommonSetter::HdrDynamicInfoInputSetter)
                .build());
        if (cnvHdrDynamicInfo != nullptr) {
            addFilterParamSetter(*(std::static_pointer_cast<C2Param>(hdrDynamicInfoInput)), cnvHdrDynamicInfo);
        }

        hdrDynamicInfoOutput = C2StreamHdrDynamicMetadataInfo::output::AllocShared(0, 0u,
                                        C2Config::HDR_DYNAMIC_METADATA_TYPE_SMPTE_2094_40);
        addParameter(
                DefineParam(hdrDynamicInfoOutput, C2_PARAMKEY_OUTPUT_HDR_DYNAMIC_INFO)
                .withDefault(hdrDynamicInfoOutput)
                .withFields({
                    C2F(hdrDynamicInfoOutput, m.type_).oneOf({ HDR_DYNAMIC_METADATA_TYPE_SMPTE_2094_40 }),
                    C2F(hdrDynamicInfoOutput, m.data).any(),
                })
                .withSetter(EncCommonSetter::HdrDynamicInfoOutputSetter)
                .build());
    }

    void addMaxIFrameSize(std::shared_ptr<C2ExynosStreamMaxIFrameSizeTuning::output> &maxIFrameSize,
                          FilterParamSetterFunc cnvMaxIFrameSize) {
        addParameter(
                DefineParam(maxIFrameSize, C2EXYNOS_PARAMKEY_MAX_I_FRAME_SIZE)
                .withDefault(new C2ExynosStreamMaxIFrameSizeTuning::output(0u, 0u))
                .withFields({ C2F(maxIFrameSize, value).any() })
                .withSetter(Setter<decltype(*maxIFrameSize)>::NonStrictValueWithNoDeps)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(maxIFrameSize)), cnvMaxIFrameSize);

        return;
    }

    /* kind of input */
    std::shared_ptr<C2StreamUsageTuning::input> mUsage;
    std::shared_ptr<C2ExynosStreamInternalUsageTuning::input> mInternalUsage;
    std::shared_ptr<C2StreamPixelFormatInfo::input>           mPixelFormat;
    std::shared_ptr<C2ExynosCSCOutputPixelFormatInfo>         mActualFormat;  /* CSC's output format */
    std::shared_ptr<C2Exynos10BitSupportInfo>                 m10BitSupport;

    /* image info */
    std::shared_ptr<C2StreamPictureSizeInfo::input> mStreamSize;

    /* bitstream info */
    std::shared_ptr<C2StreamProfileLevelInfo::output>           mProfileLevel;
    std::shared_ptr<C2StreamFrameRateInfo::output>              mFrameRate;
    std::shared_ptr<C2StreamBitrateInfo::output>                mBitrate;
    std::shared_ptr<C2StreamBitrateModeTuning::output>          mBitrateMode;
    std::shared_ptr<C2ExynosStreamBitrateModeTuning::input>     mVendorBitrateMode;  /* it will be deprecated */
    std::shared_ptr<C2StreamRequestSyncFrameTuning::output>     mRequestSyncFrame;
    std::shared_ptr<C2StreamSyncFrameIntervalTuning::output>    mSyncFramePeriod;
    std::shared_ptr<C2StreamIntraRefreshTuning::output>         mIntraRefresh;
    std::shared_ptr<C2StreamPictureQuantizationTuning::output>  mQuantization;

    std::shared_ptr<C2ExynosStreamStartIDRFrameTuning::output>  mStartIDRFrame;

    /* extension feature : cropping info */
    std::shared_ptr<C2StreamScaledCropRectTuning::input>    mPictureCropInfo;   /* CSC's input crop */
    std::shared_ptr<C2StreamScaledPictureSizeTuning::input> mPictureSize;       /* CSC's output */

    /* HDR :: color aspects */
    std::shared_ptr<C2StreamDataSpaceInfo::input>           mDataSpace;
    std::shared_ptr<C2StreamColorAspectsInfo::input>        mColorAspects;
    std::shared_ptr<C2ExynosStreamCSCDataSpaceInfo::input>  mCSCDataSpace;

    std::shared_ptr<C2StreamHdrFormatInfo::output>          mHdrFormat;

    /* extension feature : qp range */
    std::shared_ptr<C2ExynosPortQpRangeTuning::output> mQpRange;

    /* extension feature : drop control */
    std::shared_ptr<C2ExynosPortDropControlSetting::output> mDropControl;

    /* extension feature : dynamic framerate */
    std::shared_ptr<C2ExynosPortDynamicFramerateSetting::output> mDynamicFramerate;

    /* extension feature : average qp */
    std::shared_ptr<C2ExynosPortAverageQpSetting::output> mAverageQp;

    /* extension feature : PMV */
    std::shared_ptr<C2ExynosStreamPMVTuning::output> mPMV;

    std::shared_ptr<C2EncodingQualityLevel> mMinQuality;
};

#endif // EXYNOS_C2_ENCCOMPONENT_H
