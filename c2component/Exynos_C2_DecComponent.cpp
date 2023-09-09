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
#define __C2_GENERATE_GLOBAL_VARS__

#include <Codec2Mapper.h>
#include <media/stagefright/foundation/ColorUtils.h>

#include "Exynos_C2_Component.h"
#include "Exynos_C2_DecComponent.h"
#include "C2ParamUtility.h"
#include "Exynos_C2_ConfigUpdater.h"

#include "Exynos_CodecBase_Filter.h"

#define LOG_ON
#include "ExynosLog.h"
#undef LOG_TAG
#define LOG_TAG "ExynosC2DecComponent"

ExynosC2DecComponent::VdecCommonParamIntf::VdecCommonParamIntf(
    const std::shared_ptr<C2ReflectorHelper> &reflector,
    C2String name,
    C2Component::kind_t kind,
    C2Component::domain_t domain,
    C2String mediaType,
    ExynosC2FilterManager::FilterModuleInfoList listFilterInfo) : CommonParamIntf(reflector, name, kind, domain, mediaType, listFilterInfo) {
    /* kind of output */
    {
        C2ChromaOffsetStruct locations[1] = { C2ChromaOffsetStruct::ITU_YUV_420_0() };
        std::shared_ptr<C2StreamColorInfo::output> defaultColorInfo =
            C2StreamColorInfo::output::AllocShared(
                    1u, 0u, 8u /* bitDepth */, C2Color::YUV_420);
        memcpy(defaultColorInfo->m.locations, locations, sizeof(locations));

        defaultColorInfo =
            C2StreamColorInfo::output::AllocShared(
                    { C2ChromaOffsetStruct::ITU_YUV_420_0() },
                    0u, 8u /* bitDepth */, C2Color::YUV_420);
        mReflector->addStructDescriptors<C2ChromaOffsetStruct>();

        addParameter(
                DefineParam(mColorInfo, C2_PARAMKEY_CODED_COLOR_INFO)
                .withConstValue(defaultColorInfo)
                .build());
    }

    /* internal */
    addParameter(
            DefineParam(mMaxDPBNumInfo, C2EXYNOS_PARAMKEY_MAX_DPB_NUM)
            .withDefault(new C2ExynosStreamMaxDPBNumInfo::output(0u, DEFAULT_DPB))
            .withFields({ C2F(mMaxDPBNumInfo, value).inRange(0, 32u) })
            .withSetter(Setter<C2ExynosStreamMaxDPBNumInfo::output>::NonStrictValueWithNoDeps)
            .build());

    addParameter(
            DefineParam(mExtraBufferNumInfo, C2EXYNOS_PARAMKEY_EXTRA_BUFFER_NUM)
            .withDefault(new C2ExynosPortExtraBufferNumInfo::output(0u))
            .withFields({ C2F(mExtraBufferNumInfo, value).inRange(0, 32u) })
            .withSetter(Setter<C2ExynosPortExtraBufferNumInfo::output>::NonStrictValueWithNoDeps)
            .build());

    /* Color Aspects */
    {
        addParameter(
                DefineParam(mDefaultColorAspects, C2_PARAMKEY_DEFAULT_COLOR_ASPECTS)
                .withDefault(new C2StreamColorAspectsTuning::output(
                        0u, C2Color::RANGE_UNSPECIFIED, C2Color::PRIMARIES_UNSPECIFIED,
                        C2Color::TRANSFER_UNSPECIFIED, C2Color::MATRIX_UNSPECIFIED))
                .withFields({
                    C2F(mDefaultColorAspects, range).inRange(
                                C2Color::RANGE_UNSPECIFIED,     C2Color::RANGE_OTHER),
                    C2F(mDefaultColorAspects, primaries).inRange(
                                C2Color::PRIMARIES_UNSPECIFIED, C2Color::PRIMARIES_OTHER),
                    C2F(mDefaultColorAspects, transfer).inRange(
                                C2Color::TRANSFER_UNSPECIFIED,  C2Color::TRANSFER_OTHER),
                    C2F(mDefaultColorAspects, matrix).inRange(
                                C2Color::MATRIX_UNSPECIFIED,    C2Color::MATRIX_OTHER)
                })
                .withSetter(DecCommonSetter::DefaultColorAspectsSetter)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mDefaultColorAspects)), nullptr);

        addParameter(
                DefineParam(mCodedColorAspects, C2_PARAMKEY_VUI_COLOR_ASPECTS)
                .withDefault(new C2StreamColorAspectsInfo::input(
                        0u, C2Color::RANGE_UNSPECIFIED, C2Color::PRIMARIES_UNSPECIFIED,
                        C2Color::TRANSFER_UNSPECIFIED, C2Color::MATRIX_UNSPECIFIED))
                .withFields({
                    C2F(mCodedColorAspects, range).inRange(
                                C2Color::RANGE_UNSPECIFIED,     C2Color::RANGE_OTHER),
                    C2F(mCodedColorAspects, primaries).inRange(
                                C2Color::PRIMARIES_UNSPECIFIED, C2Color::PRIMARIES_OTHER),
                    C2F(mCodedColorAspects, transfer).inRange(
                                C2Color::TRANSFER_UNSPECIFIED,  C2Color::TRANSFER_OTHER),
                    C2F(mCodedColorAspects, matrix).inRange(
                                C2Color::MATRIX_UNSPECIFIED,    C2Color::MATRIX_OTHER)
                })
                .withSetter(DecCommonSetter::CodedColorAspectsSetter)
                .build());

        addParameter(
                DefineParam(mColorAspects, C2_PARAMKEY_COLOR_ASPECTS)
                .withDefault(new C2StreamColorAspectsInfo::output(
                        0u, C2Color::RANGE_UNSPECIFIED, C2Color::PRIMARIES_UNSPECIFIED,
                        C2Color::TRANSFER_UNSPECIFIED, C2Color::MATRIX_UNSPECIFIED))
                .withFields({
                    C2F(mColorAspects, range).inRange(
                                C2Color::RANGE_UNSPECIFIED,     C2Color::RANGE_OTHER),
                    C2F(mColorAspects, primaries).inRange(
                                C2Color::PRIMARIES_UNSPECIFIED, C2Color::PRIMARIES_OTHER),
                    C2F(mColorAspects, transfer).inRange(
                                C2Color::TRANSFER_UNSPECIFIED,  C2Color::TRANSFER_OTHER),
                    C2F(mColorAspects, matrix).inRange(
                                C2Color::MATRIX_UNSPECIFIED,    C2Color::MATRIX_OTHER)
                })
                .withSetter(DecCommonSetter::ColorAspectsSetter, mDefaultColorAspects, mCodedColorAspects)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mColorAspects)), DecCommonCnv::cnvColorAspects,
                                { mDefaultColorAspects, mCodedColorAspects });
    }

    addParameter(
            DefineParam(mLowLatency, C2_PARAMKEY_LOW_LATENCY_MODE)
            .withDefault(new C2GlobalLowLatencyModeTuning(C2_FALSE))
            .withFields({C2F(mLowLatency, value).oneOf({ C2_FALSE, C2_TRUE })})
            .withSetter(Setter<decltype(*mLowLatency)>::NonStrictValueWithNoDeps)
            .build());

    /* allocator id : TODO : unused on swc2codec */
    {
#ifndef USE_BUFFERQUEUE
        C2Allocator::id_t surfaceAllocator = C2PlatformAllocatorStore::GRALLOC;
#else
        C2Allocator::id_t surfaceAllocator = C2PlatformAllocatorStore::BUFFERQUEUE;
#endif

        addParameter(DefineParam(mOutputSurfaceAllocatorId, C2_PARAMKEY_OUTPUT_SURFACE_ALLOCATOR)
                .withConstValue(new C2PortSurfaceAllocatorTuning::output(surfaceAllocator))
                .build());
    }

    /* extension feature : scaling and positioning */
    {
        /* position info about scaled picture. it is used as csc's output crop */
        addParameter(
                DefineParam(mPicturePositInfo, C2_PARAMKEY_SCALED_CROP_RECT)
                .withDefault(new C2StreamScaledCropRectTuning::output(0u, C2Rect(0, 0)))
                .withFields({
                    C2F(mPicturePositInfo, width).inRange(0, 8192, 2),
                    C2F(mPicturePositInfo, height).inRange(0, 8192, 2),
                    C2F(mPicturePositInfo, left).inRange(0, 8192, 2),
                    C2F(mPicturePositInfo, top).inRange(0, 8192, 2),
                })
                .withSetter(DecCommonSetter::CropRectSetter<C2StreamScaledCropRectTuning::output>)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mPicturePositInfo)), DecCommonCnv::cnvOutputCrop);

        /* size about scaled picture. if it is set, scaling is required. it could be used as csc's output */
        addParameter(
                DefineParam(mPictureSize, C2_PARAMKEY_SCALED_PICTURE_SIZE)
                .withDefault(new C2StreamScaledPictureSizeTuning::output(0u, 0, 0))
                .withFields({
                    C2F(mPictureSize, width).inRange(0, 8192, 2),
                    C2F(mPictureSize, height).inRange(0, 8192, 2),
                })
                .withSetter(DecCommonSetter::ScaleSizeSetter<C2StreamScaledPictureSizeTuning::output>)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mPictureSize)), DecCommonCnv::cnvScaleSize);
    }

    /* extension feature : reordering timestamp */
    addParameter(
            DefineParam(mReorderTimestamp, C2EXYNOS_PARAMKEY_REORDER_TIMESTAMP)
            .withDefault(new C2ExynosPortReorderTimestampSetting::output(Off))
            .withFields({ C2F(mReorderTimestamp, value).inRange(Off, On) })
            .withSetter(DecCommonSetter::ReorderTimestampSetter)
            .build());

    /* extension feature : buffer copy */
    addParameter(
            DefineParam(mBufferCopy, C2EXYNOS_PARAMKEY_CSC_BUFFER_COPY)
            .withDefault(new C2ExynosPortCSCBufferCopySetting::output(Off))
            .withFields({ C2F(mBufferCopy, value).inRange(Off, On) })
            .withSetter(DecCommonSetter::BufferCopySetter)
            .build());
    addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mBufferCopy)), DecCommonCnv::cnvBufferCopy);

    /* extension feature : black bar */
    {
        addParameter(
                DefineParam(mBlackBar, C2EXYNOS_PARAMKEY_BLACK_BAR)
                .withDefault(new C2ExynosPortBlackBarSetting::output(Off))
                .withFields({ C2F(mBlackBar, value).inRange(Off, On) })
                .withSetter(DecCommonSetter::BlackBarSetter)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mBlackBar)), DecCommonCnv::cnvBlackBar);

        addParameter(
                DefineParam(mBlackBarInfo, C2EXYNOS_PARAMKEY_BLACK_BAR_INFO)
                .withDefault(new C2ExynosStreamBlackBarInfo::output(0u, C2Rect(0, 0)))
                .withFields({
                    C2F(mBlackBarInfo, width).inRange(0, 8192, 2),
                    C2F(mBlackBarInfo, height).inRange(0, 8192, 2),
                    C2F(mBlackBarInfo, left).inRange(0, 8192, 2),
                    C2F(mBlackBarInfo, top).inRange(0, 8192, 2),
                })
                .withSetter(DecCommonSetter::CropRectSetter<C2ExynosStreamBlackBarInfo::output>)
                .build());
    }

    /* extension feature : thumbnail mode */
    {
        addParameter(
                DefineParam(mThumbnailMode, C2EXYNOS_PARAMKEY_THUMBNAIL_MODE)
                .withDefault(new C2ExynosPortThumbnailModeSetting::output(Off))
                .withFields({ C2F(mThumbnailMode, value).inRange(Off, On) })
                .withSetter(Setter<decltype(*mThumbnailMode)>::StrictValueWithNoDeps)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mThumbnailMode)), DecCommonCnv::cnvThumbnailMode);
    }
}

c2_status_t ExynosC2DecComponent::onStart() {
    ExynosLogFunctionTrace();

    c2_status_t ret = C2_OK;

    if (mCompRes.get() == nullptr) {
        /* try to get a resource from manager */
        auto rm = ExynosC2ComponentRM::getInstance();

        auto vdecIntf = std::static_pointer_cast<VdecCommonParamIntf>(mParamIntf);

        mCompRes = rm->getResource((vdecIntf->mIsSecure)? ExynosC2ComponentRM::ResourceType::SECURE:
                                                          ExynosC2ComponentRM::ResourceType::DECODER);
        if (mCompRes.get() == nullptr) {
            ExynosLogE("[%s] getResource() is failed", __FUNCTION__);
            return C2_NO_MEMORY;
        }

        ExynosLogI("[%s] resource is obtained (%u)", __FUNCTION__, mCompRes.use_count());
    }

    ret = onSetup();
    if (ret != C2_OK) {
        ExynosLogE("[%s] onSetup() is failed", __FUNCTION__);
        mCompRes = nullptr;
        return ret;
    }

    if (mFilter.expired()) {
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        mCompRes = nullptr;
        return C2_NOT_FOUND;
    }

    auto shFilter = mFilter.lock();

    if (shFilter.get() == nullptr) {
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        mCompRes = nullptr;
        return C2_NOT_FOUND;
    }

    shFilter->start();

    /* init repilica buffer allocator */
    mReplicaBufferAllocator =
        ExynosBufferAllocator::makeAllocator(shared_from_this(),
                                             C2PlatformAllocatorStore::ION, C2BlockPool::BASIC_LINEAR,
                                             C2MemoryUsage(C2MemoryUsage::CPU_READ | android::C2AndroidMemoryUsage::HW_CODEC_READ),
                                             &mReplicaInputBlockPool);
    if (mReplicaBufferAllocator.get() == nullptr) {
        ExynosLogE("[%s] makeAllocator() for replica is failed", __FUNCTION__);
    } else {
        if (mReplicaInputBlockPool.get() == nullptr) {
            ExynosLogE("[%s] replicaInputblockPool() is invalid", __FUNCTION__);
        }
    }

    {
        auto vdecIntf = std::static_pointer_cast<VdecCommonParamIntf>(mParamIntf);

        VdecCommonParamIntf::Lock lock = vdecIntf->lock();

        mMinOutputDelay      = vdecIntf->getActualOutputDelay();
        mUseReorderTimestamp = vdecIntf->getReorderTimestamp();
        mStreamUsage         = vdecIntf->getStreamUsage();

        auto reorderKey   = vdecIntf->getReorderKey();
        mUseCustomOrdinal = (reorderKey->value == C2Config::ordinal_key_t::CUSTOM)? true:false;

        auto resolution       = vdecIntf->getStreamSize();
        auto operateRate      = vdecIntf->getOperateRate();
        auto realTimePriority = vdecIntf->getRealTimePriority();

        if ((realTimePriority == 0) && (operateRate > 0)) {
            auto filterWrapper = std::static_pointer_cast<ExynosFilterWrapper<ExynosFilter>>(mFilterManager->getFilter("dec"));
            if (filterWrapper.get() != nullptr) {
                auto filter = filterWrapper->getCoreFilter();
                if (filter.get() != nullptr) {
                    if (filter->checkRealTimeResource(resolution->width, resolution->height, operateRate)) {
                        ExynosLogD("[%s] real time resource is available", __FUNCTION__);
                    } else {
                        ExynosLogE("[%s] Failed to get real time resource", __FUNCTION__);
                        return C2_NOT_FOUND;
                    }
                }
            }
        }
    }

    mHasSavedEOS = false;

    startInputBufferFlowControl();

    return ret;
}

c2_status_t ExynosC2DecComponent::onSetup() {
    ExynosLogFunctionTrace();

    c2_status_t ret = C2_OK;

    std::vector<C2Param::Index> indices;

    indices.push_back(C2StreamPixelFormatInfo::output::PARAM_TYPE);         /* pixel format */

    ret = makeFilterParam(indices);

    /* send Filter list info for control filter */
    auto targetID = mParamIntf->findFilterID("control");
    if (targetID > 0) {
        auto filterParam = std::make_shared<ExynosFilterParam<ParamFilterListInfo>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamFilterListInfo>>(filterParam->getBaseParam());

        param->m.wkInfo = mFilterManager->getFilterListInfo();
        filterParam->registTargetFilter(targetID);

        mParamIntf->keepFilterParam({std::static_pointer_cast<ExynosFilterParamBase>(filterParam)});
    }

    return ret;
}

bool ExynosC2DecComponent::onQueue(std::shared_ptr<WorkQueueElement> workElement) {
    if (workElement.get() == nullptr) {
        /* invalid parameter */
        return false;
    }

    if ((workElement->mC2Work->input.buffers.empty()) &&
        !(workElement->mC2Work->input.flags & C2FrameData::FLAG_END_OF_STREAM)) {
        /* except for EOS, c2buffer(null) is not allowed */
        ExynosLogD("[%s] c2work has an invalid buffer with flag(0x%x)", __FUNCTION__,
                        workElement->mC2Work->input.flags);
        return false;
    }

    /* get an input buffer */
    auto optBuffer = getExynosBuffer(workElement->mC2Work->input);

    if (!optBuffer) {
        return false;
    }

    {
        auto vdecIntf = std::static_pointer_cast<VdecCommonParamIntf>(mParamIntf);

        /* output delay has not been updated yet */
        if (vdecIntf->getMaxDPBNumInfo() == DEFAULT_DPB) {
            /* make configuration for updating output delay */
            addFilterParam_OutputDelayUpdate(*optBuffer, workElement);
        }
    }

    /* register input to pending inputs */
    addPendingInput(weak_from_this(), *optBuffer, workElement);

    return queueFilterWork(workElement, *optBuffer);
}

c2_status_t ExynosC2DecComponent::onStop() {
    ExynosLogFunctionTrace();

    auto ret = ExynosC2Component::onStop();
    if (C2_OK != ret) {
        return ret;
    }

    allClearPendingInputs();

    return ret;
}

c2_status_t ExynosC2DecComponent::onFlush() {
    ExynosLogFunctionTrace();

    auto ret = ExynosC2Component::onFlush();
    if (C2_OK != ret) {
        return ret;
    }

    mHasSavedEOS = false;

    /* send Filter list info for control filter */
    auto targetID = mParamIntf->findFilterID("control");
    if (targetID > 0) {
        auto filterParam = std::make_shared<ExynosFilterParam<ParamFilterListInfo>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamFilterListInfo>>(filterParam->getBaseParam());

        param->m.wkInfo = mFilterManager->getFilterListInfo();
        filterParam->registTargetFilter(targetID);

        mParamIntf->keepFilterParam({std::static_pointer_cast<ExynosFilterParamBase>(filterParam)});
    }

    return ret;
}

c2_status_t ExynosC2DecComponent::onReset() {
    ExynosLogFunctionTrace();

    auto ret = ExynosC2Component::onReset();
    if (C2_OK != ret) {
        return ret;
    }

    allClearPendingInputs();

    return ret;
}

void ExynosC2DecComponent::onWorkDone(std::shared_ptr<WorkQueueElement> workElement) {
    ExynosLogFunctionTrace();

    if (workElement.get() == nullptr) {
        /* obj is released */
        return;
    }

    removePendingInput(workElement);

    return;
}

void ExynosC2DecComponent::onUpdateC2Work(
    std::unique_ptr<C2Work>         &c2work,
    std::shared_ptr<ExynosBuffer>    inBuffer,
    std::shared_ptr<ExynosBuffer>    outBuffer) {
    ExynosLogFunctionTrace();

    /* exynosbuffer to c2buffer */
    std::shared_ptr<C2Buffer> c2buffer = nullptr;

    if ((outBuffer.get() != nullptr) &&
        (!(outBuffer->getFlags() & ExynosBuffer::CONFIGONLY))) {
        c2buffer = *(ExynosBufferAllocator::exportC2Buffer(outBuffer));
    }

    uint32_t flags = 0;

    if (mIsAfterEOS) {
        c2work->result = C2_BAD_VALUE;
        ExynosLogV("[%s] after EOS", __FUNCTION__);
        return;
    }

    /* TODO : flags handling */
    if ((c2work->input.flags & C2FrameData::FLAG_END_OF_STREAM) ||
        (mHasSavedEOS)) {
        if ((outBuffer.get() != nullptr) &&
            (outBuffer->mImageInfo.eFrameInfo & FrameInfo::Bframe)) {
            ExynosLogD("[%s] keep EOS flag", __FUNCTION__);
            mHasSavedEOS = true;
        } else {
            flags |= C2FrameData::FLAG_END_OF_STREAM;
            ExynosLogI("[%s] output detects EOS !!", __FUNCTION__);
            mIsAfterEOS = true;
            mHasSavedEOS = false;
        }
    }

    auto it = c2work->worklets.begin();

    /*
     * !! caution !!
     * c2 client always uses first worklet forcefully.
     */
    std::advance(it, c2work->workletsProcessed);
    if (it == c2work->worklets.end()) {
        c2work->worklets.emplace_back(new C2Worklet);

        it = c2work->worklets.begin();
        std::advance(it, c2work->workletsProcessed);
    }

    /* update information */
    (*it)->output.flags = (C2FrameData::flags_t)flags;
    (*it)->output.buffers.clear();

    if (outBuffer.get() != nullptr) {
        onUpdateC2Config(c2buffer, outBuffer, c2work->input.configUpdate, (*it)->output.configUpdate);

        if (outBuffer->getFlags() & ExynosBuffer::CONFIGONLY) {
            /* it doesn't need no more */
            outBuffer.reset();
        }
    }

    if (c2buffer.get() != nullptr) {
        (*it)->output.buffers.push_back(c2buffer);
    }

    (*it)->output.ordinal = c2work->input.ordinal;

    if (mUseCustomOrdinal) {
        applyCustomOrdinal(c2work, (*it), outBuffer);
    }

    auto updateFlags = [](C2FrameData::flags_t flags)->C2FrameData::flags_t {
        return (C2FrameData::flags_t)(((flags & C2FrameData::FLAG_END_OF_STREAM)? C2FrameData::FLAG_END_OF_STREAM:0) |
                                C2FrameData::FLAG_DROP_FRAME);
    };

    if ((outBuffer.get() != nullptr) &&
        (outBuffer->mImageInfo.eFrameInfo & FrameInfo::CorruptedFrame)) {
        (*it)->output.buffers.clear();
        (*it)->output.flags = updateFlags((*it)->output.flags);
        ExynosLogW("[%s] corrupted frame. it will be dropped", __FUNCTION__);
    }

    c2work->workletsProcessed++;
    c2work->result = C2_OK;

    if (inBuffer.get() == nullptr) {
        return;
    }

    if (inBuffer->getFlags() & ExynosBuffer::REPLICA) {
        c2work->input.buffers.clear();
        c2work->worklets.front()->output.buffers.clear();
    }

    if (inBuffer->mImageInfo.eFrameInfo & FrameInfo::CorruptedFrame) {
        ExynosLogW("[%s] input is corrupted", __FUNCTION__);
        if (inBuffer->mImageInfo.eFrameInfo & FrameInfo::CodecSpecificData) {
            c2work->result = C2_CORRUPTED;
        }
    }

    return;
}

void ExynosC2DecComponent::onUpdateC2Config(
    std::shared_ptr<C2Buffer>               c2buffer,
    std::shared_ptr<ExynosBuffer>           buffer,
    std::vector<std::unique_ptr<C2Param>>  &inConfig,
    std::vector<std::unique_ptr<C2Param>>  &outConfig) {
    ExynosLogFunctionTrace();

    UNUSED(inConfig);

    if (buffer.get() == nullptr) {
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return;
    }

    if (mParamIntf.get() == nullptr) {
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return;
    }

    auto vdecIntf = std::static_pointer_cast<VdecCommonParamIntf>(mParamIntf);

    auto filterParams = std::static_pointer_cast<ExynosFilterParams>(buffer->mParams);

    ExynosRequestedParamFlag requestUpdate = REQUESTED_TYPE_NONE;

    /* requested param update */
    if (filterParams.get() != nullptr) {
        auto filterParam = filterParams->getParam(ExynosParamIndex::RequestParamUpdateIndex, TARGET_OWNER_COMPONENT);

        if (filterParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamRequestParamUpdate>>(filterParam->getBaseParam());

            requestUpdate = param->m.flags;
        }
    }

    if (c2buffer.get() != nullptr) {
        if (updateC2Config_StreamSize(outConfig, buffer, (requestUpdate & REQUESTED_TYPE_PIC_SIZE)? true:false)) {
            updateC2Config_MaxStreamSize(outConfig, buffer);
        }

        updateC2Config_CropInfo(outConfig, buffer);

        updateC2Buffer_ColorAspects(c2buffer, buffer);
    }

    if ((filterParams.get() != nullptr) &&
        (!filterParams->empty())) {
        updateC2Config_OutputDelay(outConfig, filterParams, (requestUpdate & REQUESTED_TYPE_MIN_DPB_CNT)? true:false);

        /* extra buffer for internally used */
        updateC2Config_ExtraBufferNum(outConfig, buffer);

        if (c2buffer.get() != nullptr) {
            updateC2Config_BlackBarInfo(outConfig, buffer);
        }

        updateC2Config_ReorderDepth(outConfig, buffer->mParams);
    }

    return;
}

void ExynosC2DecComponent::setBlockPool() {
    ExynosLogFunctionTrace();

    VdecCommonParamIntf::Lock lock = mParamIntf->lock();

    mFilterManager->setBlockPool((mParamIntf->mOutputPoolIds->flexCount() > 0)?
                                      mParamIntf->mOutputPoolIds->m.values[0]:C2BlockPool::BASIC_GRAPHIC,
                                 mStreamUsage);
    return;
}

std::optional<std::shared_ptr<ExynosBuffer>> ExynosC2DecComponent::getExynosBuffer(C2FrameData &input) {
    ExynosLogFunctionTrace();

    if (input.buffers.empty()) {
        /* allocate input buffer for EOS */
        LinearBufferAttribute attr;
        attr.mSize  = 1024;  /* TODO : size */

        AllocArg arg;
        arg.attr        = attr;
        arg.limit       = 0;
        arg.checkLimit  = nullptr;
        arg.allocCount  = 0;

        auto buff = allocatBuffer(mReplicaBufferAllocator, arg);
        if (!buff) {
            return std::nullopt;
        }

        input.buffers.push_back((*buff).second);

        ((*buff).first)->setDataLen(0);  /* empty data */
        ((*buff).first)->setFlags(ExynosBuffer::REPLICA);  /* replica buffer */

        ExynosLogD("[%s] replica buffer is allocated(C2Buffer:%p, ExynosBuffer:%p)", __FUNCTION__, ((*buff).second).get(), ((*buff).first).get());

        return { (*buff).first };
    } else {
        std::shared_ptr<C2Buffer> c2Buffer = input.buffers[0];

        /* C2Buffer to ExynosBuffer */
        auto optBuffer = ExynosBufferAllocator::importC2Buffer(c2Buffer);

        if (!optBuffer) {
            ExynosLogE("[%s] exynos buffer is invalid. c2buffer(%p)", __FUNCTION__, c2Buffer.get());
            return std::nullopt;
        }

        ExynosLogD("[%s] buffer: %p", __FUNCTION__, (*optBuffer).get());

        return optBuffer;
    }

    return std::nullopt;
}

bool ExynosC2DecComponent::updateC2Config_StreamSize(
    std::vector<std::unique_ptr<C2Param>>  &outConfig,
    std::shared_ptr<ExynosBuffer>           buffer,
    bool                                    forceUpdate) {
    return StaticUpdateC2Config::updateC2Config_StreamSize(mParamIntf, outConfig, buffer, forceUpdate);
}

void ExynosC2DecComponent::updateC2Config_MaxStreamSize(
    std::vector<std::unique_ptr<C2Param>>  &outConfig,
    std::shared_ptr<ExynosBuffer>           buffer) {
    return StaticUpdateC2Config::updateC2Config_MaxStreamSize(mParamIntf, outConfig, buffer);
}

void ExynosC2DecComponent::updateC2Config_CropInfo(
    std::vector<std::unique_ptr<C2Param>>  &outConfig,
    std::shared_ptr<ExynosBuffer>           buffer) {
    return StaticUpdateC2Config::updateC2Config_CropInfo(mParamIntf, outConfig, buffer);
}

void ExynosC2DecComponent::updateC2Config_OutputDelay(
    std::vector<std::unique_ptr<C2Param>>  &outConfig,
    std::shared_ptr<ExynosFilterParams>     filterParams,
    bool                                    forceUpdate) {
    return StaticUpdateC2Config::updateC2Config_OutputDelay(mParamIntf, outConfig, filterParams, mMinOutputDelay, forceUpdate);
}

void ExynosC2DecComponent::updateC2Config_ExtraBufferNum(
    std::vector<std::unique_ptr<C2Param>>  &outConfig,
    std::shared_ptr<ExynosBuffer>           buffer) {
    return StaticUpdateC2Config::updateC2Config_ExtraBufferNum(mParamIntf, outConfig, buffer);
}

void ExynosC2DecComponent::updateC2Config_BlackBarInfo(
    std::vector<std::unique_ptr<C2Param>>  &outConfig,
    std::shared_ptr<ExynosBuffer>           buffer) {
    return StaticUpdateC2Config::updateC2Config_BlackBarInfo(mParamIntf, outConfig, buffer);
}

void ExynosC2DecComponent::updateC2Config_ReorderDepth(
    std::vector<std::unique_ptr<C2Param>>  &outConfig,
    std::shared_ptr<ExynosParams>           params) {
    return  StaticUpdateC2Config::updateC2Config_ReorderDepth(mParamIntf, outConfig, params);
}

void ExynosC2DecComponent::updateC2Buffer_ColorAspects(
    std::shared_ptr<C2Buffer>               c2buffer,
    std::shared_ptr<ExynosBuffer>           buffer) {
    return  StaticUpdateC2Config::updateC2Buffer_ColorAspects(mParamIntf, c2buffer, buffer);
}

void ExynosC2DecComponent::updateC2Buffer_HDRInfo(
    std::shared_ptr<C2Buffer>       c2buffer,
    std::shared_ptr<ExynosBuffer>   buffer) {
    return StaticUpdateC2Config::updateC2Buffer_HDRInfo(mParamIntf, c2buffer, buffer);
}

void ExynosC2DecComponent::sendC2Work(std::unique_ptr<C2Work> c2work) {
    ExynosLogFunctionTrace();

    ExynosC2Component::sendC2Work(std::move(c2work));

    auto shThreadPool = mThreadPool;
    if (shThreadPool.get() != nullptr) {
        auto func = [wkOwner = weak_from_this(), outputDelay = mMinOutputDelay]()->bool {
                        if (!wkOwner.expired()) {
                            auto shOwner = std::static_pointer_cast<ExynosC2DecComponent>(wkOwner.lock());
                            if (shOwner.get() != nullptr) {
                                shOwner->releasePendingInputs(wkOwner, outputDelay);
                            }
                        }

                        return true;
                    };

        /* send a task */
        shThreadPool->toss(std::string("ExynosC2DecComponent::releasePendingInputs"), std::move(func));
    }

    return;
}

void ExynosC2DecComponent::PreFlush() {
    ExynosLogFunctionTrace();

    return allClearPendingInputs();
}

void ExynosC2DecComponent::addFilterParam_OutputDelayUpdate(std::shared_ptr<ExynosBuffer> buffer,
                                                            std::shared_ptr<WorkQueueElement> element) {
    ExynosLogFunctionTrace();

    if ((buffer.get() == nullptr) ||
        (element.get() == nullptr)) {
        /* invalid parameter */
        return;
    }

    if (element->mC2Work.get() == nullptr) {
        /* workElement has null c2work */
        return;
    }

    using __function__ = std::function<void(std::shared_ptr<ExynosParams>)>;

    auto clonedC2Work = cloneC2Work(element->mC2Work);
    if (clonedC2Work.get() == nullptr) {
        StaticExynosLog(Level::Trace, "ExynosC2DecComponent", "[processCSDInfo] c2work is null");
        return;
    }

    auto shClonedC2Work = wrapShared<std::unique_ptr<C2Work>>(std::move(clonedC2Work));

    std::shared_ptr<__function__> updatefunc = std::make_shared<__function__>(
        [wkOwner = weak_from_this(), shClonedC2Work](std::shared_ptr<ExynosParams> params) {
            if (wkOwner.expired() ||
                (params.get() == nullptr)) {
                /* invalid parameter */
                return;
            }

            auto shOwner = std::static_pointer_cast<ExynosC2DecComponent>(wkOwner.lock());
            if (shOwner.get() == nullptr) {
                /* obj is released */
                return;
            }

            auto func = [wkOwner = (std::weak_ptr<ExynosC2DecComponent>)shOwner, shWork = std::move(shClonedC2Work), params]() {
                            auto shOwner = GET_SHARED_PTR_NOLOG(wkOwner);
                            auto c2work = std::move(*shWork);
                            if (!CHECK_SHARED_PTR_NOLOG(shOwner)) {
                                /* invalid parameter or obj is released */
                                return;
                            }

                            auto filterParams = std::static_pointer_cast<ExynosFilterParams>(params);

                            /* send a cloned c2work which includes output delay configuration */
                            shOwner->updateC2Config_OutputDelay(c2work->worklets.front()->output.configUpdate, filterParams, true);

                            StaticExynosLog(Level::Info, "ExynosC2DecComponent",
                                                "[processCSDInfo] update configurations on c2work(%p)",
                                                c2work.get());

                            shOwner->sendC2Work(std::move(c2work));

                            return;
                        };

            auto shThreadPool = shOwner->mThreadPool;
            if (shThreadPool.get() == nullptr) {
                /* obj is released */
                return;
            }

            shThreadPool->tossToCurSession(std::string("ExynosC2Component::doFilterWorkDone"), std::move(func));

            return;
        });

    if (buffer->mParams.get() == nullptr) {
        buffer->mParams = std::static_pointer_cast<ExynosParams>(std::make_shared<ExynosFilterParams>());
    }

    auto filterParams = std::static_pointer_cast<ExynosFilterParams>(buffer->mParams);

    auto intfImpl = std::static_pointer_cast<VdecCommonParamIntf>(mParamIntf);
    auto id = intfImpl->findFilterID("dec");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<FuncParamOutputDelayUpdate>>();
        auto param       = std::static_pointer_cast<ExynosParam<FuncParamOutputDelayUpdate>>(filterParam->getBaseParam());

        param->m.func = updatefunc;

        filterParam->registTargetFilter(id);
        filterParams->addParam(filterParam);
    }

    return;
}

void ExynosC2DecComponent::applyCustomOrdinal(std::unique_ptr<C2Work> &c2work,
                                                std::unique_ptr<C2Worklet> &worklet,
                                                std::shared_ptr<ExynosBuffer> outBuffer) {
    if (outBuffer.get() != nullptr) {
        if ((outBuffer->mImageInfo.eFrameInfo & FrameInfo::IDRframe) ||
            (outBuffer->mImageInfo.eFrameInfo & FrameInfo::Iframe)) {
            /* trust timestamp on IDR/I frame is correct */
            mOrdinal.adjustTimestamp(c2work->input.ordinal.timestamp);
            mOrdinal.adjustFrameIndex();
        }

        if (mUseReorderTimestamp) {
            auto timestamp = mOrdinal.getOrdinal(true);

            if ((c2work->worklets.front()->output.flags & C2FrameData::FLAG_END_OF_STREAM) &&
                (c2work->input.ordinal.timestamp.peekll() <= 0)) {
                timestamp = c2work->input.ordinal.timestamp;
            }

            worklet->output.ordinal.customOrdinal = c2work->input.ordinal.customOrdinal = timestamp;
        } else {
            worklet->output.ordinal.customOrdinal = mOrdinal.getOrdinal((int)outBuffer->mImageInfo.nPoc);
        }
    } else {
        if (mUseReorderTimestamp) {
            auto timestamp = mOrdinal.getOrdinal(true);

            if ((c2work->worklets.front()->output.flags & C2FrameData::FLAG_END_OF_STREAM) &&
                (c2work->input.ordinal.timestamp.peekll() <= 0)) {
                timestamp = c2work->input.ordinal.timestamp;
            }

            worklet->output.ordinal.customOrdinal = c2work->input.ordinal.customOrdinal = timestamp;
        }
    }
}

bool InputBufferFlowControl::addPendingInput(
    std::weak_ptr<ExynosC2Component> wkComponent, std::shared_ptr<ExynosBuffer> buffer,
    std::shared_ptr<ExynosC2Component::WorkQueueElement> workElement) {
    StaticExynosLogFunctionTrace("InputBufferFlowControl");

    if ((buffer.get() == nullptr) ||
        (workElement.get() == nullptr)) {
        /* invalid parameter */
        return false;
    }

    ExynosMutex<std::list<PendingInput>>::LockObj pendingInputs(mPendingInputs);

    std::shared_ptr<std::function<bool(uint32_t)>> relfunc = std::make_shared<std::function<bool(uint32_t)>>(
        [wkComp = wkComponent, wkElement = (std::weak_ptr<ExynosC2Component::WorkQueueElement>)workElement](uint32_t outputDelay)->bool {
            if (wkComp.expired()) {
                /* obj is released */
                return false;
            }

            auto shDecComp = std::static_pointer_cast<ExynosC2DecComponent>(wkComp.lock());
            if (shDecComp.get() == nullptr) {
                return false;
            }

            shDecComp->setConsumedPendingInput(wkElement);

            /* send a task for release inputs */
            auto wkDecComp = std::weak_ptr<ExynosC2DecComponent>(shDecComp);
            shDecComp->sendTesk(std::string("ExynosC2DecComponent::releasePendingInputs"),
                                &ExynosC2DecComponent::releasePendingInputs, wkDecComp, wkDecComp, outputDelay);

            return true;
        });

    workElement->mTag = mTag++ % std::numeric_limits<unsigned int>::max();

    buffer->setMark(workElement->mTag);
    buffer->setDestroyNotify(relfunc);

    StaticExynosLog(Level::Essential, "InputBufferFlowControl",
                    "[%s] exynos buffer: %p, tag: %llu", __FUNCTION__, buffer.get(), workElement->mTag);

    pendingInputs->emplace_back(workElement);

    return true;
}

void InputBufferFlowControl::removePendingInput(std::shared_ptr<ExynosC2Component::WorkQueueElement> element) {
    StaticExynosLogFunctionTrace("InputBufferFlowControl");

    ExynosMutex<std::list<PendingInput>>::LockObj pendingInputs(mPendingInputs);

    for (auto it = pendingInputs->begin(); it != pendingInputs->end(); ) {
        if ((*it).mWorkElement.expired()) {
            /* work is already returned */
            it = pendingInputs->erase(it);
        } else {
            auto shWork = (*it).mWorkElement.lock();

            /* find a pending input which has same c2work */
            if ((shWork.get() != nullptr) &&
                (element.get() != nullptr) &&
                (element.get() == shWork.get())) {
                StaticExynosLog(Level::Debug, "ExynosC2DecComponent",
                                    "[removePendingInput] c2work(%p)'s inputs are released, tag: %d",
                                    shWork->mC2Work.get(), shWork->mTag);

                it = pendingInputs->erase(it);
            } else {
                ++it;
            }
        }
    }

    return;
}

void InputBufferFlowControl::clearPendingInputs(int32_t count) {
    StaticExynosLogFunctionTrace("InputBufferFlowControl");

    ExynosMutex<std::list<PendingInput>>::LockObj pendingInputs(mPendingInputs);

    auto clearfunc = [](std::weak_ptr<ExynosC2Component::WorkQueueElement> wkWork)->void {
                        if (!wkWork.expired()) {
                            auto shWork = wkWork.lock();
                            if ((shWork.get() != nullptr) &&
                                (shWork->mC2Work.get() != nullptr)) {
                                if (shWork->mC2Work->input.buffers.size() > 0) {
                                    shWork->mC2Work->input.buffers.clear();
                                }
                                StaticExynosLog(Level::Debug, "ExynosC2DecComponent",
                                                    "[clearPendingInputs] c2work(%p)'s inputs are released, tag: %d",
                                                    shWork->mC2Work.get(), shWork->mTag);
                            }
                        }

                        return;
                    };

    for (int i = 0; i < count; i++) {
        if (pendingInputs->empty()) {
            break;
        }

        for (auto it = pendingInputs->begin(); it != pendingInputs->end(); ) {
            if ((*it).mWorkElement.expired()) {
                /* work is already returned */
                it = pendingInputs->erase(it);
            } else {
                if ((*it).mIsConsumed) {
                    /* input buffer can be released */
                    clearfunc((*it).mWorkElement);

                    it = pendingInputs->erase(it);
                    break;
                }

                ++it;
            }
        }
    }

    return;
}

int32_t InputBufferFlowControl::getPendingInputsCount() {
    StaticExynosLogFunctionTrace("InputBufferFlowControl");

    ExynosMutex<std::list<PendingInput>>::LockObj pendingInputs(mPendingInputs);

    for (auto it = pendingInputs->begin(); it != pendingInputs->end(); ) {
        if ((*it).mWorkElement.expired()) {
            /* work is already returned */
            it = pendingInputs->erase(it);
        } else {
            ++it;
        }
    }

    return pendingInputs->size();
}

bool InputBufferFlowControl::setConsumedPendingInput(std::weak_ptr<ExynosC2Component::WorkQueueElement> workElement) {
    StaticExynosLogFunctionTrace("InputBufferFlowControl");

    if (workElement.expired()) {
        /* obj is released */
        return false;
    }

    ExynosMutex<std::list<PendingInput>>::LockObj pendingInputs(mPendingInputs);

    for (auto it = pendingInputs->begin(); it != pendingInputs->end(); ) {
        if ((*it).mWorkElement.expired()) {
            /* work is already returned */
            it = pendingInputs->erase(it);
        } else {
            auto shPendingInputWork = (*it).mWorkElement.lock();
            auto shWork = workElement.lock();

            if ((shPendingInputWork.get() != nullptr) &&
                (shPendingInputWork->mC2Work.get() != nullptr) &&
                (shWork.get() != nullptr) &&
                (shWork->mC2Work.get() != nullptr)) {
                /* find a pending input which has same c2work */
                if (shPendingInputWork->mC2Work.get() == shWork->mC2Work.get()) {
                    (*it).mIsConsumed = true;
                    StaticExynosLog(Level::Essential, "InputBufferFlowControl",
                                    "[%s] c2work(%p)'s inputs are consumed, tag :%d", __FUNCTION__,
                                    shWork->mC2Work.get(), shWork->mTag);
                    return true;
                }
            }

            ++it;
        }
    }

    /* can not find a pending input. it could be already returned */
    return false;
}

bool InputBufferFlowControl::releasePendingInputs(
    std::weak_ptr<ExynosC2Component>    weakComp,
    uint32_t                            outputDelay) {
    StaticExynosLogFunctionTrace("InputBufferFlowControl");

    auto shDecComp = std::static_pointer_cast<ExynosC2DecComponent>(GET_SHARED_PTR_NOLOG(weakComp));
    if (!CHECK_SHARED_PTR_NOLOG(shDecComp)) {
        return false;
    }

    int32_t numC2Works = shDecComp->getC2WorkCount();
    int32_t maxC2Works = ((outputDelay < EXTRA_DPB)? outputDelay:(outputDelay - EXTRA_DPB)) +
                            SMOOTHNESS_FACTOR;
    int32_t permittedC2works = maxC2Works - numC2Works;

    int32_t numPendingInputs = getPendingInputsCount();
    if ((mMaxInputCount == SMOOTHNESS_FACTOR) &&  /* default */
        ((mMaxInputCount - numPendingInputs) < 0)) {  /* it means that user doesn't use array mode(4) */
        mMaxInputCount = SMOOTHNESS_FACTOR * 2;
        StaticExynosLog(Level::Essential, "InputBufferFlowControl",
                        "[%s] increase expected max input count(%d)", __FUNCTION__, mMaxInputCount);
    }
    int32_t expectedC2Works = mMaxInputCount - numPendingInputs;  /* framework will send */

    StaticExynosLog(Level::Trace, "InputBufferFlowControl",
                "[%s] current(%d), max(%d), pending(%d), permit(%d), expected(%d)", __FUNCTION__,
                numC2Works, maxC2Works, numPendingInputs, permittedC2works, expectedC2Works);

    auto count = permittedC2works - expectedC2Works;

    if (count > 0) {
        StaticExynosLog(Level::Essential, "InputBufferFlowControl",
                        "[%s] release pending inputs(%d)", __FUNCTION__, count);
        clearPendingInputs(count);
    }

    return true;
}

void InputBufferFlowControl::allClearPendingInputs() {
    StaticExynosLogFunctionTrace("InputBufferFlowControl");

    ExynosMutex<std::list<PendingInput>>::LockObj pendingInputs(mPendingInputs);
    pendingInputs->clear();

    return;
}

