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
#ifndef EXYNOS_C2_DECCOMPONENT_H
#define EXYNOS_C2_DECCOMPONENT_H

#include <list>
#include <vector>
#include <memory>

#include <Codec2Mapper.h>
#include <media/stagefright/foundation/ColorUtils.h>

#include <system/graphics.h>
#include "exynos_format.h"

#include "Exynos_C2_Component.h"
#include "ExynosBufferAllocator.h"

#define LOG_ON
#include "ExynosLog.h"

#define SMOOTHNESS_FACTOR 4   /* it depends on CCodecBufferChannel */
#define DEFAULT_OUTPUT_DELAY 8
#define MIN_OUTPUT_DELAY 1u

#define DEFAULT_DPB 0
#define EXTRA_DPB 5     /* it depdens on VideoCodec */

#define UHD_IMAGE_SIZE (4096 * 2304)

#define MIN_INPUT_SIZE (7 * 1024 * 1024)
#define MAX_INPUT_SIZE (15 * 1024 * 1024)
#define GET_INPUT_MAX_SIZE(w, h) (((w * h) <= (4096 * 2160))? MIN_INPUT_SIZE:MAX_INPUT_SIZE)
#define LIMITED_SECURE_INPUT_SIZE (1024 * 1024 * 5 / 2)  /* 2.5MB : For projects which has small vstream heap */

#define DEFAULT_DEC_MEMORY_USAGE (android::C2AndroidMemoryUsage::HW_CODEC_WRITE |\
                                  android::C2AndroidMemoryUsage::HW_TEXTURE_READ)

#define DEFAULT_SECURE_DEC_MEMORY_USAGE (android::C2AndroidMemoryUsage::HW_CODEC_WRITE |\
                                         android::C2AndroidMemoryUsage::HW_TEXTURE_READ |\
                                         C2MemoryUsage::WRITE_PROTECTED |\
                                         C2MemoryUsage::READ_PROTECTED)

class InputBufferFlowControl {
public:
    InputBufferFlowControl() {
        allClearPendingInputs();
        mMaxInputCount = SMOOTHNESS_FACTOR;
        mMinOutputDelay = 0;
        mTag = 0;
    }

    ~InputBufferFlowControl() {
        allClearPendingInputs();
    }

    bool addPendingInput(std::weak_ptr<ExynosC2Component> wkComponent, std::shared_ptr<ExynosBuffer> buffer,
                         std::shared_ptr<ExynosC2Component::WorkQueueElement> workElement);
    void removePendingInput(std::shared_ptr<ExynosC2Component::WorkQueueElement> element);
    void clearPendingInputs(int32_t count);
    int32_t getPendingInputsCount();
    bool setConsumedPendingInput(std::weak_ptr<ExynosC2Component::WorkQueueElement> workElement);
    bool releasePendingInputs(std::weak_ptr<ExynosC2Component> weakComp, uint32_t outputDelay);
    void allClearPendingInputs();

    void startInputBufferFlowControl() {
        mMaxInputCount = SMOOTHNESS_FACTOR;
        mTag = 0;
    }

protected:
    class PendingInput {
    public:
        PendingInput(std::weak_ptr<ExynosC2Component::WorkQueueElement> element) : mIsConsumed(false), mWorkElement(element) {
        }

        ~PendingInput() = default;

        std::atomic<bool> mIsConsumed;
        std::weak_ptr<ExynosC2Component::WorkQueueElement> mWorkElement;

    private:
        /* disable default constructor */
        PendingInput() = delete;
    };

    uint32_t mMinOutputDelay; // value update ExynosC2DecComponent

private:
    int32_t mMaxInputCount;
    uint64_t mTag;

    ExynosMutex<std::list<PendingInput>> mPendingInputs;  /* controls input release */
};

class ExynosC2DecComponent : public ExynosC2Component, public InputBufferFlowControl/*, public std::enable_shared_from_this<ExynosC2DecComponent>*/ {
public:
    class VdecCommonParamIntf;

    ExynosC2DecComponent(const std::shared_ptr<C2ComponentInterface> &intf) : ExynosC2Component(intf) {
        mObjName = "ExynosC2DecComponent";
        mThreadPool->setObjName(mObjName);

        mHasSavedEOS = false;
        mUseReorderTimestamp = false;
        mStreamUsage = 0u;
    }

    virtual ~ExynosC2DecComponent() {
    }

protected:
    /* override function on ExynosC2Component */
    virtual c2_status_t onStart() override;
    c2_status_t onSetup() override;
    bool        onQueue(std::shared_ptr<WorkQueueElement> workElement) override;
    c2_status_t onStop() override;
    c2_status_t onFlush() override;
    c2_status_t onReset() override;
    void onWorkDone(std::shared_ptr<WorkQueueElement> workElement) override;
    void onUpdateC2Work(std::unique_ptr<C2Work> &c2work,
                        std::shared_ptr<ExynosBuffer> inBuffer = nullptr,
                        std::shared_ptr<ExynosBuffer> outBuffer = nullptr) override;
    virtual void onUpdateC2Config(std::shared_ptr<C2Buffer> c2buffer, std::shared_ptr<ExynosBuffer> buffer,
                                  std::vector<std::unique_ptr<C2Param>> &inConfig,
                                  std::vector<std::unique_ptr<C2Param>> &outConfig);
    void setBlockPool() override;
    std::optional<std::shared_ptr<ExynosBuffer>> getExynosBuffer(C2FrameData &input) override;
    void sendC2Work(std::unique_ptr<C2Work> c2work) override;
    void PreFlush() override;
    int32_t getC2WorkCount() override {
        return ExynosC2Component::getC2WorkCount();
    }

    /* function for ExynosC2DecComponent's child class */
    // TODO

    bool updateC2Config_StreamSize(std::vector<std::unique_ptr<C2Param>> &outConfig,
                                   std::shared_ptr<ExynosBuffer> buffer,
                                   bool forceUpdate);
    void updateC2Config_MaxStreamSize(std::vector<std::unique_ptr<C2Param>> &outConfig,
                                      std::shared_ptr<ExynosBuffer> buffer);
    void updateC2Config_CropInfo(std::vector<std::unique_ptr<C2Param>> &outConfig,
                                 std::shared_ptr<ExynosBuffer> buffer);
    void updateC2Config_OutputDelay(std::vector<std::unique_ptr<C2Param>> &outConfig,
                                    std::shared_ptr<ExynosFilterParams> filterParams,
                                    bool forceUpdate);
    void updateC2Config_ExtraBufferNum(std::vector<std::unique_ptr<C2Param>> &outConfig,
                                       std::shared_ptr<ExynosBuffer> buffer);
    void updateC2Config_BlackBarInfo(std::vector<std::unique_ptr<C2Param>> &outConfig,
                                     std::shared_ptr<ExynosBuffer> buffer);
    void updateC2Config_ReorderDepth(std::vector<std::unique_ptr<C2Param>> &outConfig,
                                     std::shared_ptr<ExynosParams> params);
    void updateC2Buffer_ColorAspects(std::shared_ptr<C2Buffer> c2buffer,
                                     std::shared_ptr<ExynosBuffer> buffer);
    // for HEVC & AV1
    void updateC2Buffer_HDRInfo(std::shared_ptr<C2Buffer> c2buffer,
                                std::shared_ptr<ExynosBuffer> buffer);

private:
    void addFilterParam_OutputDelayUpdate(std::shared_ptr<ExynosBuffer> buffer,
                                          std::shared_ptr<WorkQueueElement> element);
    void applyCustomOrdinal(std::unique_ptr<C2Work> &c2work,
                            std::unique_ptr<C2Worklet> &worklet,
                            std::shared_ptr<ExynosBuffer> outBuffer);

    std::atomic_bool mHasSavedEOS;
    bool mUseReorderTimestamp;
    uint64_t mStreamUsage;

    friend bool InputBufferFlowControl::releasePendingInputs(std::weak_ptr<ExynosC2Component> weakComp, uint32_t outputDelay);

    /* disable default constructor */
    ExynosC2DecComponent() = delete;
};

class ExynosC2DecComponent::VdecCommonParamIntf : public ExynosC2Component::CommonParamIntf {
public:
    VdecCommonParamIntf(
        const std::shared_ptr<C2ReflectorHelper> &reflector,
        C2String name,
        C2Component::kind_t kind,
        C2Component::domain_t domain,
        C2String mediaType,
        ExynosC2FilterManager::FilterModuleInfoList listFilterInfo);

    virtual ~VdecCommonParamIntf() = default;

    /* getter functions */
    uint64_t getStreamUsage() const { return mStreamUsage->value; }

    uint32_t getMaxDPBNumInfo() const { return mMaxDPBNumInfo->value; }

    uint32_t getActualOutputDelay() {
        return (mActualOutputDelay.get() != nullptr)? mActualOutputDelay->value:0;
    }

    std::shared_ptr<C2StreamPictureSizeInfo::output> getStreamSize()     const { return mStreamSize; }
    std::shared_ptr<C2StreamMaxPictureSizeTuning::output> getMaxStreamSize()     const { return mMaxStreamSize; }
    std::shared_ptr<C2StreamCropRectInfo::output>    getStreamCropInfo() const { return mStreamCropInfo; }

    uint32_t getPixelFormat() const { return mPixelFormat->value; }
    uint32_t getActualFormat() const { return mActualFormat->value; }

    std::shared_ptr<C2StreamColorAspectsInfo::output> getColorAspects_l() {
        return mColorAspects;
    }

    CropInfo getInputCrop() const {
        CropInfo crop;

        crop.nWidth     = mPictureCropInfo->width;
        crop.nHeight    = mPictureCropInfo->height;
        crop.nLeft      = mPictureCropInfo->left;
        crop.nTop       = mPictureCropInfo->top;

        return crop;
    }
    CropInfo getOutputCrop() const {
        CropInfo crop;

        crop.nWidth     = mPicturePositInfo->width;
        crop.nHeight    = mPicturePositInfo->height;
        crop.nLeft      = mPicturePositInfo->left;
        crop.nTop       = mPicturePositInfo->top;

        return crop;
    }

    std::shared_ptr<C2StreamHdrStaticInfo::output> getHdrStaticInfo(bool internal = false) {
        if (internal ||
            (mHdrStaticValidationInfo->value == C2_TRUE)) {
            if (!internal) {
                mHdrStaticValidationInfo->value = C2_FALSE;
            }

            return mHdrStaticInfo;
        }

        return nullptr;
    }

    std::shared_ptr<C2StreamScaledPictureSizeTuning::output> getPictureSize() const { return mPictureSize; }

    bool getReorderTimestamp() const {
        if (mReorderTimestamp.get() != nullptr) {
            return (mReorderTimestamp->value == On)? true:false;
        }

        return false;
    }

    std::shared_ptr<C2ExynosPortDecodeOrderSetting::output> getDecodeOrder() const { return mDecodeOrder; }

    std::shared_ptr<C2ExynosPortCSCBufferCopySetting::output> getBufferCopy() const { return mBufferCopy; }

    std::shared_ptr<C2PortReorderKeySetting::output> getReorderKey() const { return mReorderKey; }

    std::shared_ptr<C2ExynosPortBlackBarSetting::output> getBlackBar() const { return mBlackBar; }

    std::shared_ptr<C2ExynosPortThumbnailModeSetting::output> getThumbnailMode() const { return mThumbnailMode; }

    std::shared_ptr<C2ExynosPortCompressedColorSetting::output> getCompressedColor() const { return mCompressedColor; }

protected:
    void addDecodeDefaultParam(uint32_t minWSize, uint32_t minHSize, uint32_t maxWSize, uint32_t maxHSize, uint32_t maxBufferSize,
                         C2R (*maxPictureSizeSetter)(bool mayBlock, C2P<C2StreamMaxPictureSizeTuning::output> &me,
                                                     const C2P<C2StreamPictureSizeInfo::output> &size),
                                bool support10bit = false) {
        addParameter(
                DefineParam(mAttribute, C2_PARAMKEY_COMPONENT_ATTRIBUTES)
                .withConstValue(new C2ComponentAttributesSetting(C2Component::ATTRIB_IS_TEMPORAL))
                .build());

        /* output size about bitstream. it could be same as output of MFC D/D */
        addParameter(
                DefineParam(mStreamSize, C2_PARAMKEY_PICTURE_SIZE)
                .withDefault(new C2StreamPictureSizeInfo::output(0u, 320, 240))
                .withFields({
                    C2F(mStreamSize, width).inRange(minWSize, maxWSize, 1),
                    C2F(mStreamSize, height).inRange(minHSize, maxHSize, 1),
                })
                .withSetter(DecCommonSetter::SizeSetter)
                .build());

        /* supported max stream size */
        addParameter(
                DefineParam(mMaxStreamSize, C2_PARAMKEY_MAX_PICTURE_SIZE)
                .withDefault(new C2StreamMaxPictureSizeTuning::output(0u, 0, 0))
                .withFields({
                    C2F(mStreamSize, width).inRange(minWSize, maxWSize, 1),
                    C2F(mStreamSize, height).inRange(minHSize, maxHSize, 1),
                })
                .withSetter(maxPictureSizeSetter, mStreamSize)
                .build());

        /* size of input buffer */
        addParameter(
                DefineParam(mMaxInputSize, C2_PARAMKEY_INPUT_MAX_BUFFER_SIZE)
                .withDefault(new C2StreamMaxBufferSizeInfo::input(0u, maxBufferSize))
                .withFields({
                    C2F(mMaxInputSize, value).any(),
                })
                .calculatedAs(((mIsSecure)? DecCommonSetter::MaxInputSizeSetterForSecure:DecCommonSetter::MaxInputSizeSetter), mMaxStreamSize)
                .build());
        {
            std::vector<unsigned int> outputFormatList;

            if (!mIsSecure) {
                outputFormatList = { HAL_PIXEL_FORMAT_YCBCR_420_888,
                                     HAL_PIXEL_FORMAT_YV12,
                                     /* HAL_PIXEL_FORMAT_RGBA_8888, */ /* TODO : need? */
                                     HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN,
                                     HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M,
                                     HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M };

                if (support10bit) {
                    outputFormatList.push_back(HAL_PIXEL_FORMAT_YCBCR_P010);
                }
            }

            outputFormatList.push_back(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED);

            addParameter(
                DefineParam(mPixelFormat, C2_PARAMKEY_PIXEL_FORMAT)
                .withDefault(new C2StreamPixelFormatInfo::output(0u, HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED))
                .withFields({C2F(mPixelFormat, value).oneOf(outputFormatList)})
                .withSetter(DecCommonSetter::PixelFormatSetter)
                .build());
            addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mPixelFormat)), DecCommonCnv::cnvPixelFormat);
        }

        addParameter(
            DefineParam(mActualFormat, C2EXYNOS_PARAMKEY_CSC_OUTPUT_PIXEL_FORMAT)
            .withDefault(new C2ExynosCSCOutputPixelFormatInfo(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED))
            .withFields({C2F(mActualFormat, value).oneOf({
                                HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED,
                                HAL_PIXEL_FORMAT_YCBCR_420_888,
                                HAL_PIXEL_FORMAT_YCBCR_P010,
                                HAL_PIXEL_FORMAT_YV12,
                                HAL_PIXEL_FORMAT_EXYNOS_YV12_M,
                                HAL_PIXEL_FORMAT_RGBA_8888,
                                HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN,
                                HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M,
                                HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M})})
            .withSetter(DecCommonSetter::ActualFormatSetter, mPixelFormat)
            .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mActualFormat)), DecCommonCnv::cnvActualFormat,
                            { mPixelFormat });

        addParameter(
                DefineParam(mUsage, C2_PARAMKEY_OUTPUT_STREAM_USAGE)
                .withDefault(new C2StreamUsageTuning::output(0u, 0))
                .withFields({ C2F(mUsage, value).any() })
                .withSetter(DecCommonSetter::UsageSetter)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mUsage)), nullptr);

        addParameter(
                DefineParam(mStreamUsage, C2EXYNOS_PARAMKEY_VENDOR_OUTPUT_STREAM_USAGE)
                .withDefault(new C2ExynosStreamOutputStreamSetting::output(0u, ExynosUtils::GetUsageType()))
                .withFields({ C2F(mStreamUsage, value).any() })
                .withSetter(DecCommonSetter::StreamUsageSetter, mPixelFormat, mUsage)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mStreamUsage)), nullptr);
    }

    void addDecodeOrder(bool enable) {
        addParameter(
                DefineParam(mDecodeOrder, C2EXYNOS_PARAMKEY_DECODE_ORDER)
                .withDefault(new C2ExynosPortDecodeOrderSetting::output(enable? On:Off))
                .withFields({ C2F(mDecodeOrder, value).inRange(Off, On) })
                .withSetter(DecCommonSetter::DecodeOrderSetter, mReorderTimestamp)
                .build());
        return;
    }

    void addReorderDepth(C2Uint32Value depth) {
        addParameter(
                DefineParam(mReorderDepth, C2_PARAMKEY_OUTPUT_REORDER_DEPTH)
                .withDefault(new C2PortReorderBufferDepthTuning::output(depth))
                .withFields({ C2F(mReorderDepth, value).inRange(0, 32) })
                .withSetter(DecCommonSetter::ReorderDepthSetter, mStreamSize, mDecodeOrder, mMaxDPBNumInfo)
                .build());
        return;
    }

    void addOutputDelay(C2Uint32Value delay) {
        addParameter(
                DefineParam(mActualOutputDelay, C2_PARAMKEY_OUTPUT_DELAY)
                .withDefault(new C2PortActualDelayTuning::output(delay))
                .withFields({C2F(mActualOutputDelay, value).inRange(0, 32u)})
                .withSetter(DecCommonSetter::ActualOutputDelaySetter, mMaxDPBNumInfo)
                .build());
        return;
    }

    template<typename ... Deps>
    void addOutputDelay(
        C2Uint32Value delay,
        C2R (*setter)(bool, C2P<C2PortActualDelayTuning::output> &, const C2P<Deps> &...),
        std::shared_ptr<Deps>& ... deps) {
        addParameter(
                DefineParam(mActualOutputDelay, C2_PARAMKEY_OUTPUT_DELAY)
                .withDefault(new C2PortActualDelayTuning::output(delay))
                .withFields({C2F(mActualOutputDelay, value).inRange(0, 32u)})
                .withSetter(setter, deps...)
                .build());
        return;
    }

    template<typename ... Deps>
    void addReorderKey(
        C2Config::ordinal_key_t key,
        C2R (*setter)(bool, C2P<C2PortReorderKeySetting::output> &, const C2P<Deps> &...),
        std::shared_ptr<Deps>& ... deps) {
        addParameter(
                DefineParam(mReorderKey, C2_PARAMKEY_OUTPUT_REORDER_KEY)
                .withDefault(new C2PortReorderKeySetting::output(key))
                .withFields({ C2F(mReorderKey, value).inRange(C2Config::ordinal_key_t::ORDINAL,
                                                              C2Config::ordinal_key_t::CUSTOM) })
                .withSetter(setter, deps...)
                .build());
        return;
    }

    void addStreamCrop(uint32_t maxWidth, uint32_t maxHeight) {
        uint32_t width  = 0;
        uint32_t height = 0;

        if (mStreamSize.get() != nullptr) {
            width  = mStreamSize->width;
            height = mStreamSize->height;
        } else {
            StaticExynosLog(Level::Error, "VdecCommonParamIntf", "[%s] mStreamSize is invalid", __FUNCTION__);
        }

        addParameter(
                DefineParam(mStreamCropInfo, C2_PARAMKEY_CROP_RECT)
                .withDefault(new C2StreamCropRectInfo::output(0u, C2Rect(width, height)))
                .withFields({
                    C2F(mStreamCropInfo, width).inRange(0, maxWidth, 2),
                    C2F(mStreamCropInfo, height).inRange(0, maxHeight, 2),
                    C2F(mStreamCropInfo, left).inRange(0, maxWidth, 2),
                    C2F(mStreamCropInfo, top).inRange(0, maxHeight, 2),
                })
                .withSetter(DecCommonSetter::CropRectSetter<C2StreamCropRectInfo::output>)
                .build());

        return;
    }

    void addCompressedColor() {
        addParameter(
                DefineParam(mCompressedColor, C2EXYNOS_PARAMKEY_COMPRESSED_COLOR)
                .withDefault(new C2ExynosPortCompressedColorSetting::output(ExynosUtils::GetCompressedColorType()))
                .withFields({ C2F(mCompressedColor, value).oneOf({
                                  VendorC2Config::COMPRESSED_COLOR_NONE,
                                  VendorC2Config::COMPRESSED_COLOR_LOSSLESS})
                })
                .withSetter(DecCommonSetter::CompressedColorSetter, mStreamUsage)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mCompressedColor)), DecCommonCnv::cnvCompressedColor,
                                { mPixelFormat, mStreamUsage });

        return;
    }

    void addImageConvertModeParam(std::shared_ptr<C2ExynosPortImageConvertSetting::output> &imageConvert,
                                std::shared_ptr<C2ExynosPortImageConvertModeSetting::output> &imageConvertMode,
                                std::shared_ptr<C2ExynosPortImageConvertPixelFormatSetting::output> &imageConvertPixelFormat,
                                std::shared_ptr<C2ExynosPortImageConvertControlFrameSetting::output> &imageConvertControlFrame,
                                FilterParamSetterFunc cnvImageConvert,
                                FilterParamSetterFunc cnvImageConvertMode,
                                FilterParamSetterFunc cnvImageConvertPixelFormat,
                                FilterParamSetterFunc cnvImageConvertControlFrame) {
        addParameter(
                DefineParam(imageConvert, C2EXYNOS_PARAMKEY_IMAGE_CONVERT)
                .withDefault(new C2ExynosPortImageConvertSetting::output(Off))
                .withFields({C2F(imageConvert, value).inRange(Off, On)})
                .withSetter(Setter<decltype(*imageConvert)>::StrictValueWithNoDeps)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(imageConvert)), cnvImageConvert);

        addParameter(
                DefineParam(imageConvertMode, C2EXYNOS_PARAMKEY_IMAGE_CONVERT_MODE)
                .withDefault(new C2ExynosPortImageConvertModeSetting::output(1u))
                .withFields({C2F(imageConvertMode, value).inRange(0u, 1u)})
                .withSetter(Setter<decltype(*imageConvertMode)>::StrictValueWithNoDeps)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(imageConvertMode)), cnvImageConvertMode);

        addParameter(
                DefineParam(imageConvertPixelFormat, C2EXYNOS_PARAMKEY_IMAGE_CONVERT_PIXEL_FORMAT)
                .withDefault(new C2ExynosPortImageConvertPixelFormatSetting::output(HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M))
                .withFields({C2F(imageConvertPixelFormat, value).oneOf({
                                    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M,
                                    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M})})
                .withSetter(Setter<decltype(*imageConvertPixelFormat)>::StrictValueWithNoDeps)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(imageConvertPixelFormat)), cnvImageConvertPixelFormat);

        addParameter(
                DefineParam(imageConvertControlFrame, C2EXYNOS_PARAMKEY_IMAGE_CONVERT_CONTROL_FRAME)
                .withDefault(new C2ExynosPortImageConvertControlFrameSetting::output(0))
                .withFields({C2F(imageConvertControlFrame, value).any()})
                .withSetter(Setter<decltype(*imageConvertControlFrame)>::StrictValueWithNoDeps)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(imageConvertControlFrame)), cnvImageConvertControlFrame);
    }

    void addCroppingParam(uint32_t maxLSize, uint32_t maxTSize, uint32_t maxWSize, uint32_t maxHSize, uint32_t step) {
        /* extension feature : cropping */
        addParameter(
                DefineParam(mPictureCropInfo, C2_PARAMKEY_SCALED_CROP_RECT)
                .withDefault(new C2StreamScaledCropRectTuning::input(0u, C2Rect(0, 0)))
                .withFields({
                    C2F(mPictureCropInfo, width).inRange(0, maxWSize, step),
                    C2F(mPictureCropInfo, height).inRange(0, maxHSize, step),
                    C2F(mPictureCropInfo, left).inRange(0, maxLSize, step),
                    C2F(mPictureCropInfo, top).inRange(0, maxTSize, step),
                })
                .withSetter(DecCommonSetter::CropRectSetter<C2StreamScaledCropRectTuning::input, C2StreamPictureSizeInfo::output>, mStreamSize)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mPictureCropInfo)), DecCommonCnv::cnvInputCrop,
                                { mStreamSize });
    }

    void addHdrStaticInfo() {
        mReflector->addStructDescriptors<C2MasteringDisplayColorVolumeStruct, C2ColorXyStruct>();

        mHdrStaticInfo = std::make_shared<C2StreamHdrStaticInfo::output>();

        /* sample BT.2020 */
        mHdrStaticInfo->mastering.red   = {0.708, 0.292};
        mHdrStaticInfo->mastering.green = {0.170, 0.797};
        mHdrStaticInfo->mastering.blue  = {0.131, 0.046};
        mHdrStaticInfo->mastering.white = {0.3127, 0.3290};
        mHdrStaticInfo->mastering.maxLuminance = 1000;
        mHdrStaticInfo->mastering.minLuminance = 0.1;
        mHdrStaticInfo->maxCll = 1000;
        mHdrStaticInfo->maxFall = 120;

        /* TODO */
        mHdrStaticInfo->mastering.minLuminance = 0;  /* SMPTE2086. default value for marking invalidation */
        mHdrStaticInfo->maxCll = 0;  /* CTA861-3. default value for marking invalidation */
        mHdrStaticInfo->maxFall = 65536;  /* format. default value for marking invalidation */

        addParameter(
                DefineParam(mHdrStaticInfo, C2_PARAMKEY_HDR_STATIC_INFO)
                .withDefault(mHdrStaticInfo)
                .withSetter(DecCommonSetter::CodedHdrStaticInfoSetter)
                .withFields({
                    C2F(mHdrStaticInfo, mastering.red.x).any(), // .inRange(0, 1),
                    C2F(mHdrStaticInfo, mastering.red.y).any(), // .inRange(0, 1),
                    C2F(mHdrStaticInfo, mastering.green.x).any(), // .inRange(0, 1),
                    C2F(mHdrStaticInfo, mastering.green.y).any(), // .inRange(0, 1),
                    C2F(mHdrStaticInfo, mastering.blue.x).any(), // .inRange(0, 1),
                    C2F(mHdrStaticInfo, mastering.blue.y).any(), // .inRange(0, 1),
                    C2F(mHdrStaticInfo, mastering.white.x).any(), // .inRange(0, 1),
                    C2F(mHdrStaticInfo, mastering.white.y).any(), // .inRange(0, 1),
                    C2F(mHdrStaticInfo, mastering.maxLuminance).any(), // .inRange(0, 65535),
                    C2F(mHdrStaticInfo, mastering.minLuminance).any(), // .inRange(0, 6.5535),
                    C2F(mHdrStaticInfo, maxCll).any(), // .inRange(0, 65535),
                    C2F(mHdrStaticInfo, maxFall).any(), // .inRange(0, 65535),
                })
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mHdrStaticInfo)), DecCommonCnv::cnvHdrStaticInfo);

        addParameter(
                DefineParam(mHdrStaticValidationInfo, C2EXYNOS_PARAMKEY_HDR_STATIC_VALIDATION_INFO)
                .withDefault(new C2ExynosStreamHdrStaticValidationInfo::output(0u, C2_FALSE))
                .withSetter(DecCommonSetter::CodedHdrStaticValidationInfoSetter, mHdrStaticInfo)
                .withFields({
                    C2F(mHdrStaticValidationInfo, value).oneOf({ C2_FALSE, C2_TRUE })
                })
                .build());
    }

    /* bitstream info */
    std::shared_ptr<C2StreamProfileLevelInfo::input>        mProfileLevel;
    std::shared_ptr<C2StreamMaxBufferSizeInfo::input>       mMaxInputSize;
    std::shared_ptr<C2StreamMaxPictureSizeTuning::output>   mMaxStreamSize;
    std::shared_ptr<C2StreamPictureSizeInfo::output>        mStreamSize;
    std::shared_ptr<C2StreamCropRectInfo::output>           mStreamCropInfo;
    std::shared_ptr<C2ExynosStreamMaxDPBNumInfo::output>    mMaxDPBNumInfo;

    std::shared_ptr<C2ExynosPortExtraBufferNumInfo::output> mExtraBufferNumInfo;

    std::shared_ptr<C2GlobalLowLatencyModeTuning>           mLowLatency;

    /* extension feature : cropping info */
    std::shared_ptr<C2StreamScaledCropRectTuning::input>        mPictureCropInfo;    /* CSC's input crop */

    /* extension feature : scaling & positioning info */
    std::shared_ptr<C2StreamScaledCropRectTuning::output>       mPicturePositInfo;  /* CSC's output crop */
    std::shared_ptr<C2StreamScaledPictureSizeTuning::output>    mPictureSize;       /* CSC's output */

    /* kind of output */
    std::shared_ptr<C2StreamColorInfo::output>                    mColorInfo;
    std::shared_ptr<C2StreamPixelFormatInfo::output>              mPixelFormat;
    std::shared_ptr<C2ExynosCSCOutputPixelFormatInfo>             mActualFormat;  /* CSC's output format */
    std::shared_ptr<C2StreamUsageTuning::output>                  mUsage;
    std::shared_ptr<C2ExynosStreamOutputStreamSetting::output>    mStreamUsage;

    /* color aspects */
    std::shared_ptr<C2StreamColorAspectsTuning::output> mDefaultColorAspects;  /* framework's color aspects */
    std::shared_ptr<C2StreamColorAspectsInfo::input>    mCodedColorAspects;    /* bitstream's color aspects */
    std::shared_ptr<C2StreamColorAspectsInfo::output>   mColorAspects;         /* decided color aspects */

    /* hdr static */
    std::shared_ptr<C2ExynosStreamHdrStaticValidationInfo::output> mHdrStaticValidationInfo;
    std::shared_ptr<C2StreamHdrStaticInfo::output> mHdrStaticInfo;  /* from bitstream or external */

    /* extension feature : reorder timestamp */
    std::shared_ptr<C2ExynosPortReorderTimestampSetting::output> mReorderTimestamp;

    /* extension feature : decode order decoding */
    std::shared_ptr<C2ExynosPortDecodeOrderSetting::output> mDecodeOrder;

    /* extension feature : buffer copy */
    std::shared_ptr<C2ExynosPortCSCBufferCopySetting::output> mBufferCopy;

    /* extension feature : black bar */
    std::shared_ptr<C2ExynosPortBlackBarSetting::output> mBlackBar;
    std::shared_ptr<C2ExynosStreamBlackBarInfo::output>  mBlackBarInfo;

    /* extension feature : thumbnail mode */
    std::shared_ptr<C2ExynosPortThumbnailModeSetting::output> mThumbnailMode;

    /* extension feature : compressed color */
    std::shared_ptr<C2ExynosPortCompressedColorSetting::output> mCompressedColor;

    friend class ExynosC2DecComponent;
};

#endif // EXYNOS_C2_DECCOMPONENT_H
