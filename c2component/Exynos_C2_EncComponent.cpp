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
#include "Exynos_C2_Component.h"
#include "Exynos_C2_EncComponent.h"
#include "Exynos_CodecBase_Filter.h"

#include "ExynosETC.h"

#define LOG_ON
#include "ExynosLog.h"
#undef LOG_TAG
#define LOG_TAG "ExynosC2EncComponent"

ExynosC2EncComponent::VencCommonParamIntf::VencCommonParamIntf(
    const std::shared_ptr<C2ReflectorHelper> &reflector,
    C2String name,
    C2Component::kind_t kind,
    C2Component::domain_t domain,
    C2String mediaType,
    ExynosC2FilterManager::FilterModuleInfoList listFilterInfo) : CommonParamIntf(reflector, name, kind, domain, mediaType, listFilterInfo) {
    addParameter(
            DefineParam(mUsage, C2_PARAMKEY_INPUT_STREAM_USAGE)
            .withConstValue(new C2StreamUsageTuning::input(
                    0u, (uint64_t)(/*C2MemoryUsage::CPU_READ | */android::C2AndroidMemoryUsage::HW_CODEC_READ)))
            .build());

    addParameter(
            DefineParam(mInternalUsage, C2EXYNOS_PARAMKEY_INTERNAL_STREAM_USAGE)
            .withDefault(new C2ExynosStreamInternalUsageTuning::input(0u, 0))
            .withFields({C2F(mInternalUsage, value).any()})
            .withSetter(Setter<decltype(*mInternalUsage)>::NonStrictValueWithNoDeps)
            .build());

    /* request to restart stream with IDR */
    addParameter(
            DefineParam(mStartIDRFrame, C2EXYNOS_PARAMKEY_START_IDR_FRAME)
            .withDefault(new C2ExynosStreamStartIDRFrameTuning::output(0u, C2_FALSE))
            .withFields({C2F(mStartIDRFrame, value).oneOf({ C2_FALSE, C2_TRUE }) })
            .withSetter(Setter<decltype(*mStartIDRFrame)>::NonStrictValueWithNoDeps)
            .build());
    addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mStartIDRFrame)), EncCommonCnv::cnvStartIDRFrame);

    /* request sync frame */
    addParameter(
            DefineParam(mRequestSyncFrame, C2_PARAMKEY_REQUEST_SYNC_FRAME)
            .withDefault(new C2StreamRequestSyncFrameTuning::output(0u, C2_FALSE))
            .withFields({C2F(mRequestSyncFrame, value).oneOf({ C2_FALSE, C2_TRUE }) })
            .withSetter(Setter<decltype(*mRequestSyncFrame)>::NonStrictValueWithNoDeps)
            .build());
    addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mRequestSyncFrame)), EncCommonCnv::cnvRequestSyncFrame);

    addParameter(
        DefineParam(mFrameRate, C2_PARAMKEY_FRAME_RATE)
        .withDefault(new C2StreamFrameRateInfo::output(0u, 15.))
        .withFields({C2F(mFrameRate, value).greaterThan(0.)}) /* TODO: limitation */
        .withSetter(Setter<decltype(*mFrameRate)>::NonStrictValueWithNoDeps)
        .build());
    addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mFrameRate)), EncCommonCnv::cnvFrameRate);

    /* timebased interval of IDR */
    addParameter(
            DefineParam(mSyncFramePeriod, C2_PARAMKEY_SYNC_FRAME_INTERVAL)
            .withDefault(new C2StreamSyncFrameIntervalTuning::output(0u, 1000000))
            .withFields({C2F(mSyncFramePeriod, value).any()})
            .withSetter(Setter<decltype(*mSyncFramePeriod)>::StrictValueWithNoDeps)
            .build());
    addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mSyncFramePeriod)), EncCommonCnv::cnvSyncFramePeriod,
                            { mFrameRate });

    addParameter(
            DefineParam(mIntraRefresh, C2_PARAMKEY_INTRA_REFRESH)
            .withDefault(new C2StreamIntraRefreshTuning::output(
                    0u, C2Config::INTRA_REFRESH_DISABLED, 0.))
            .withFields({
                C2F(mIntraRefresh, mode).oneOf({
                    C2Config::INTRA_REFRESH_DISABLED,
                    C2Config::INTRA_REFRESH_ARBITRARY}),  /* cyclic */
                C2F(mIntraRefresh, period).any()
            })
            .withSetter(EncCommonSetter::IntraRefreshSetter)
            .build());
    addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mIntraRefresh)), EncCommonCnv::cnvIntraRefresh);

    /* HDR :: Color Aspects */
    {
        addParameter(
                DefineParam(mDataSpace, C2_PARAMKEY_DATA_SPACE)
                .withDefault(new C2StreamDataSpaceInfo::input(0u, HAL_DATASPACE_UNKNOWN))
                .withFields({C2F(mDataSpace, value).oneOf({
                                    HAL_DATASPACE_UNKNOWN,
                                    HAL_DATASPACE_SRGB_LINEAR,
                                    HAL_DATASPACE_V0_SRGB_LINEAR,
                                    HAL_DATASPACE_SRGB,
                                    HAL_DATASPACE_V0_SRGB,
                                    HAL_DATASPACE_JFIF,
                                    HAL_DATASPACE_V0_JFIF,
                                    HAL_DATASPACE_BT601_625,
                                    HAL_DATASPACE_V0_BT601_625,
                                    HAL_DATASPACE_BT601_525,
                                    HAL_DATASPACE_V0_BT601_525,
                                    HAL_DATASPACE_BT709,
                                    HAL_DATASPACE_V0_BT709,
                                    (HAL_DATASPACE_STANDARD_BT709 | HAL_DATASPACE_TRANSFER_SMPTE_170M | HAL_DATASPACE_RANGE_FULL),
                                    HAL_DATASPACE_BT2020,
                                    HAL_DATASPACE_BT2020_PQ,
                                    HAL_DATASPACE_BT2020_ITU,
                                    HAL_DATASPACE_BT2020_ITU_PQ,
                                    HAL_DATASPACE_BT2020_ITU_HLG,
                                    HAL_DATASPACE_BT2020_HLG})})
                .withSetter(EncCommonSetter::DataSpaceSetter)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mDataSpace)), nullptr);

        addParameter(
                DefineParam(mColorAspects, C2_PARAMKEY_COLOR_ASPECTS)
                .withDefault(new C2StreamColorAspectsInfo::input(
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
                .withSetter(EncCommonSetter::ColorAspectsSetter, mDataSpace)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mColorAspects)), EncCommonCnv::cnvColorAspects,
                                { mDataSpace });

        addParameter(
                DefineParam(mCSCDataSpace, C2EXYNOS_PARAMKEY_CSC_DATA_SPACE)
                .withDefault(new C2ExynosStreamCSCDataSpaceInfo::input(0u, HAL_DATASPACE_UNKNOWN))
                .withFields({C2F(mCSCDataSpace, value).oneOf({
                                    HAL_DATASPACE_UNKNOWN,
                                    HAL_DATASPACE_SRGB_LINEAR,
                                    HAL_DATASPACE_V0_SRGB_LINEAR,
                                    HAL_DATASPACE_SRGB,
                                    HAL_DATASPACE_V0_SRGB,
                                    HAL_DATASPACE_JFIF,
                                    HAL_DATASPACE_V0_JFIF,
                                    HAL_DATASPACE_BT601_625,
                                    HAL_DATASPACE_V0_BT601_625,
                                    HAL_DATASPACE_BT601_525,
                                    HAL_DATASPACE_V0_BT601_525,
                                    HAL_DATASPACE_BT709,
                                    HAL_DATASPACE_V0_BT709,
                                    (HAL_DATASPACE_STANDARD_BT709 | HAL_DATASPACE_TRANSFER_SMPTE_170M | HAL_DATASPACE_RANGE_FULL),
                                    HAL_DATASPACE_BT2020,
                                    HAL_DATASPACE_BT2020_PQ,
                                    HAL_DATASPACE_BT2020_ITU,
                                    HAL_DATASPACE_BT2020_ITU_PQ,
                                    HAL_DATASPACE_BT2020_ITU_HLG,
                                    HAL_DATASPACE_BT2020_HLG})})
                .withSetter(EncCommonSetter::CSCDataSpaceSetter, mColorAspects)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mCSCDataSpace)), EncCommonCnv::cnvCSCDataSpace,
                                { mColorAspects });
    }

    /* extension feature : drop control */
    {
        addParameter(
                DefineParam(mDropControl, C2EXYNOS_PARAMKEY_DROP_CONTROL)
                .withDefault(new C2ExynosPortDropControlSetting::output(On))
                .withFields({ C2F(mDropControl, value).inRange(Off, On) })
                .withSetter(Setter<decltype(*mDropControl)>::StrictValueWithNoDeps)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mDropControl)), EncCommonCnv::cnvDropControl);
    }

    /* extension feature : dynamic framerate */
    {
        addParameter(
                DefineParam(mDynamicFramerate, C2EXYNOS_PARAMKEY_DYNAMIC_FRAMERATE)
                .withDefault(new C2ExynosPortDynamicFramerateSetting::output(0))  /* enable */
                .withFields({ C2F(mDynamicFramerate, value).inRange(0, 1) })
                .withSetter(Setter<decltype(*mDynamicFramerate)>::StrictValueWithNoDeps)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mDynamicFramerate)), EncCommonCnv::cnvDynamicFramerate);
    }

    /* extension feature : average qp */
    {
        addParameter(
                DefineParam(mAverageQp, C2EXYNOS_PARAMKEY_AVERAGE_QP)
                .withDefault(new C2ExynosPortAverageQpSetting::output(Off))
                .withFields({ C2F(mAverageQp, value).inRange(Off, On) })
                .withSetter(Setter<decltype(*mAverageQp)>::StrictValueWithNoDeps)
                .build());
        addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mAverageQp)), EncCommonCnv::cnvAverageQp);
    }

    /* min quality */
    addParameter(
            DefineParam(mMinQuality, C2_PARAMKEY_ENCODING_QUALITY_LEVEL)
            .withDefault(new C2EncodingQualityLevel(C2PlatformConfig::encoding_quality_level_t::NONE))
            .withFields({ C2F(mMinQuality, value).oneOf({
                                C2PlatformConfig::encoding_quality_level_t::NONE,
                                C2PlatformConfig::encoding_quality_level_t::S_HANDHELD})})
            .withSetter(EncCommonSetter::MinQualitySetter)
            .build());
    addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mMinQuality)), EncCommonCnv::cnvMinQuality);
}

c2_status_t ExynosC2EncComponent::onStart() {
    ExynosLogFunctionTrace();

    c2_status_t ret = C2_OK;

    auto vencIntf = std::static_pointer_cast<VencCommonParamIntf>(mParamIntf);

    if (mCompRes.get() == nullptr) {
        /* try to get a resource from manager */
        auto rm = ExynosC2ComponentRM::getInstance();

        mCompRes = rm->getResource((vencIntf->mIsSecure)? ExynosC2ComponentRM::ResourceType::SECURE:
                                                          ExynosC2ComponentRM::ResourceType::ENCODER);
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
        return C2_NOT_FOUND;
    }

    auto shFilter = mFilter.lock();

    if (shFilter.get() == nullptr) {
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        mCompRes = nullptr;
        return C2_NOT_FOUND;
    }

    shFilter->start();

    /* init replica buffer allocator */
    auto usage = C2MemoryUsage(android::C2AndroidMemoryUsage::HW_CODEC_READ |\
                              ((vencIntf->mIsSecure == true) ? C2MemoryUsage::READ_PROTECTED | C2MemoryUsage::WRITE_PROTECTED :\
                                                               C2MemoryUsage::CPU_READ));
    mReplicaBufferAllocator =
        ExynosBufferAllocator::makeAllocator(shared_from_this(),
                                             C2PlatformAllocatorStore::GRALLOC, C2BlockPool::BASIC_GRAPHIC,
                                             usage,
                                             &mReplicaInputBlockPool);
    if (mReplicaBufferAllocator.get() == nullptr) {
        ExynosLogE("[%s] makeAllocator() for replica is failed", __FUNCTION__);
    } else {
        if (mReplicaInputBlockPool.get() == nullptr) {
            ExynosLogE("[%s] replicaInputblockPool() is invalid", __FUNCTION__);
        }
    }

    {
        auto resolution       = vencIntf->getStreamSize();
        auto operateRate      = vencIntf->getOperateRate();
        auto realTimePriority = vencIntf->getRealTimePriority();

        if ((realTimePriority == 0) && (operateRate > 0)) {
            auto filterWrapper = std::static_pointer_cast<ExynosFilterWrapper<ExynosFilter>>(mFilterManager->getFilter("enc"));
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

    return ret;
}


c2_status_t ExynosC2EncComponent::onSetup() {
    ExynosLogFunctionTrace();

    c2_status_t ret = C2_OK;

    std::vector<C2Param::Index> indices;

    indices.push_back(C2StreamProfileLevelInfo::output::PARAM_TYPE);        /* profile & level */
    indices.push_back(C2StreamColorAspectsInfo::input::PARAM_TYPE);         /* color aspects */
    indices.push_back(C2ExynosPortQpRangeTuning::output::PARAM_TYPE);       /* qp range */
    indices.push_back(C2ExynosPortAverageQpSetting::output::PARAM_TYPE);    /* average qp */
    indices.push_back(C2EncodingQualityLevel::PARAM_TYPE);                  /* min quality */
    indices.push_back(C2ExynosPortDropControlSetting::output::PARAM_TYPE);  /* drop control */

    ret = makeFilterParam(indices);

    auto param = EncCommonCnv::cnvSubscribedParam(mParamIntf);
    mParamIntf->keepFilterParam(param);

    mActualFormat = 0;  /* clear actual format */

    return ret;
}

bool ExynosC2EncComponent::onQueue(std::shared_ptr<WorkQueueElement> workElement) {
    if (workElement.get() == nullptr) {
        /* invalid parameter */
        return false;
    }

    /* get an input buffer */
    auto optBuffer = getExynosBuffer(workElement->mC2Work->input);

    if (!optBuffer) {
#ifdef USE_DEFFERING_WORK_QUEUE
        /* if component supports deferring work processing,
         * mDeferringWorkQueue will be valid.
         * it could be used for specific case such as starting with "null buffer" in encoder.
         */
        if (mDeferringWorkQueue.get() != nullptr) {
            ExynosMutex<ExynosQueue<std::shared_ptr<WorkQueueElement>>>::LockObj defferingWorkQ(*mDeferringWorkQueue.get());

            defferingWorkQ->enqueue(workElement);

            ExynosLogI("[%s] exynos buffer is invalid. defer c2work(%p)", __FUNCTION__, workElement->mC2Work.get());

            return true;
        }
#endif
        ExynosLogE("[%s] importC2Buffer() is failed", __FUNCTION__);
        return false;
    }

#ifdef USE_DEFFERING_WORK_QUEUE
    /* if there is deferred work, process it first */
    if (mDeferringWorkQueue.get() != nullptr) {
        ExynosMutex<ExynosQueue<std::shared_ptr<WorkQueueElement>>>::LockObj defferingWorkQ(*mDeferringWorkQueue.get());

        while (!defferingWorkQ->empty()) {
            std::shared_ptr<WorkQueueElement> defferedElement = nullptr;

            if (defferingWorkQ->dequeue(defferedElement)) {
                auto optDefferedBuffer = getExynosBuffer(defferedElement->mC2Work->input);

                if (!optDefferedBuffer) {
                    ExynosLogE("[%s] getExynosBuffer() is failed", __FUNCTION__);
                    return false;
                }

                ExynosLogI("[%s] found deffered c2work(%p)", __FUNCTION__, defferedElement->mC2Work.get());

                queueFilterWork(defferedElement, *optDefferedBuffer);
            }
        }
    }
#endif

    return queueFilterWork(workElement, *optBuffer);
}

void ExynosC2EncComponent::onUpdateC2Work(
    std::unique_ptr<C2Work>         &c2work,
    std::shared_ptr<ExynosBuffer>    inBuffer,
    std::shared_ptr<ExynosBuffer>    outBuffer) {
    ExynosLogFunctionTrace();

    /* exynosbuffer to c2buffer */
    std::shared_ptr<C2Buffer> c2buffer = nullptr;

    if (outBuffer.get() != nullptr) {
        c2buffer = *(ExynosBufferAllocator::exportC2Buffer(outBuffer));

        /* in case of CSD, update C2StreamInitDataInfo */
        if (outBuffer->mImageInfo.eFrameInfo & FrameInfo::CodecSpecificData) {
            if (outBuffer->getDataLen() > 0) {
                std::unique_ptr<C2StreamInitDataInfo::output> csd =
                                    C2StreamInitDataInfo::output::AllocUnique(outBuffer->getDataLen(), 0u);
                if (csd == nullptr) {
                    ExynosLogE("[%s] AllocUnique() for CSD is failed", __FUNCTION__);
                    c2work->result = C2_NO_MEMORY;
                    return;
                }

                C2ReadView      view    = c2buffer->data().linearBlocks().front().map().get();
                const uint8_t  *pHeader = view.data();

                if (pHeader == nullptr) {
                    ExynosLogE("[%s] addr about header buffer is invalid", __FUNCTION__);
                    c2work->result = C2_NO_MEMORY;
                    return;
                }

                memcpy((void *)(csd->m.value), pHeader, outBuffer->getDataLen());

                c2work->worklets.front()->output.configUpdate.push_back(std::move(csd));

                ExynosLogI("[%s] set CSD to configuration on output.", __FUNCTION__);
            } else {
                ExynosLogD("[%s] generating CSD is disabled", __FUNCTION__);
            }

            return;
        }

        /* in case of IDR frame, update C2StreamPictureTypeMaskInfo */
        if ((outBuffer->mImageInfo.eFrameInfo & FrameInfo::IDRframe) &&
            (!(outBuffer->getFlags() & ExynosBuffer::REPLICA))) {
            ExynosLogI("[%s] got SYNC FRAME", __FUNCTION__);
            c2buffer->setInfo(std::make_shared<C2StreamPictureTypeMaskInfo::output>(0u /* stream id */, C2Config::SYNC_FRAME));
        }
    }

    uint32_t flags = 0;

    /* TODO : flags handling */
    if ((c2work->input.flags & C2FrameData::FLAG_END_OF_STREAM)
#if 0  /* TODO : check timestamp */
        /* && (c2_cntr64_t(index ) == work->input.ordinal.frameIndex)*/ /*index is timestamp*/
#endif
        ) {
        flags |= C2FrameData::FLAG_END_OF_STREAM;
        mIsAfterEOS = true;
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
    }

    if (c2buffer.get() != nullptr) {
        (*it)->output.buffers.push_back(c2buffer);
    }

    (*it)->output.ordinal = c2work->input.ordinal;

    if (mUseCustomOrdinal) {
        if (outBuffer.get() != nullptr) {
            if ((outBuffer->mImageInfo.eFrameInfo & FrameInfo::IDRframe) ||
                (outBuffer->mImageInfo.eFrameInfo & FrameInfo::Iframe)) {
                /* trust timestamp on IDR/I frame is correct */
                mOrdinal.adjustTimestamp(c2work->input.ordinal.timestamp);
            }

            (*it)->output.ordinal.customOrdinal = mOrdinal.getOrdinal();
        }
    }

    if ((inBuffer.get() != nullptr) &&
        (inBuffer->getFlags() & ExynosBuffer::REPLICA)) {
        c2work->input.buffers.clear();
        c2work->worklets.front()->output.buffers.clear();
    }

    c2work->workletsProcessed++;
    c2work->result = C2_OK;

    return;
}

void ExynosC2EncComponent::onUpdateC2Config(
    std::shared_ptr<C2Buffer>               c2buffer,
    std::shared_ptr<ExynosBuffer>           buffer,
    std::vector<std::unique_ptr<C2Param>>  &inConfig,
    std::vector<std::unique_ptr<C2Param>>  &outConfig) {
    ExynosLogFunctionTrace();

    UNUSED(inConfig);
    UNUSED(outConfig);

    if (buffer.get() == nullptr) {
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return;
    }

    if (mParamIntf.get() == nullptr) {
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return;
    }

    auto vencIntf = std::static_pointer_cast<VencCommonParamIntf>(mParamIntf);

    auto filterParams = std::static_pointer_cast<ExynosFilterParams>(buffer->mParams);

    if ((filterParams.get() != nullptr) &&
        (!filterParams->empty())) {
        if (c2buffer.get() != nullptr) {
            updateC2Config_Subscribes(c2buffer, buffer);
            updateC2Config_AverageQp(c2buffer, buffer);
        }
    }

    return;
}

void ExynosC2EncComponent::setBlockPool() {
    ExynosLogFunctionTrace();

    VencCommonParamIntf::Lock lock = mParamIntf->lock();

    mFilterManager->setBlockPool((mParamIntf->mOutputPoolIds->flexCount() > 0)?
                                      mParamIntf->mOutputPoolIds->m.values[0]:C2BlockPool::BASIC_LINEAR);
    return;
}

std::optional<std::shared_ptr<ExynosBuffer>> ExynosC2EncComponent::getExynosBuffer(C2FrameData &input) {
    ExynosLogFunctionTrace();

    static bool bRequiredIDR = false;

    if (input.buffers.empty()) {
        /* allocate input buffer for EOS */
        GraphicBufferAttribute attr;

        std::shared_ptr<VencCommonParamIntf> vencIntf = std::static_pointer_cast<VencCommonParamIntf>(mParamIntf);

        if (vencIntf.get() == nullptr) {
            ExynosLogE("[%s] mParamIntf is invalid", __FUNCTION__);
            return std::nullopt;
        }

        auto format = mActualFormat;
        uint64_t usage = 0;
        {
            VencCommonParamIntf::Lock lock = vencIntf->lock();

            usage = vencIntf->getInternalUsage();

            /* if actual format is not updated, can not allocate replica buffer */
            if (mActualFormat == 0) {  /* HAL_PIXEL_FORMAT_UNDEFINED */
                auto size = vencIntf->getStreamSize();
                mWidth = size->width;
                mHeight = size->height;

                bRequiredIDR = true;  /* empty data could be encoded. so, next input should be started as IDR */

                /* it should use a suitable format depended on the hdr format */
                auto hdrFormat = vencIntf->getHdrFormat();

                format = (hdrFormat == C2Config::hdr_format_t::SDR)?
                                HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M;
            }
        }

        attr.mWidth  = mWidth;
        attr.mHeight = mHeight;
        attr.mFormat = format;
        attr.mUsage  = usage;

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

        mWidth  = (*optBuffer)->mImageInfo.stCropInfo.nWidth;
        mHeight = (*optBuffer)->mImageInfo.stCropInfo.nHeight;

        /* previous input is replica */
        if (bRequiredIDR) {
            std::unique_ptr<C2ExynosStreamStartIDRFrameTuning::output> startIDR =
                std::make_unique<C2ExynosStreamStartIDRFrameTuning::output>(0u, C2_TRUE);
            input.configUpdate.push_back(std::move(startIDR));
            bRequiredIDR = false;
        }

        /* update actual format, if actual format is not updated */
        if (mActualFormat == 0) {  /* HAL_PIXEL_FORMAT_UNDEFINED */
            std::unique_ptr<C2ExynosCSCOutputPixelFormatInfo> actualFormat =
                std::make_unique<C2ExynosCSCOutputPixelFormatInfo>((*optBuffer)->format());

            std::vector<std::unique_ptr<C2SettingResult>> failures;

            auto err = mParamIntf->config({ actualFormat.get() }, C2_MAY_BLOCK, &failures);
            if (err == C2_OK) {
                std::shared_ptr<VencCommonParamIntf> vencIntf = std::static_pointer_cast<VencCommonParamIntf>(mParamIntf);

                if (vencIntf.get() == nullptr) {
                    ExynosLogE("[%s] mParamIntf is invalid", __FUNCTION__);
                    return std::nullopt;
                }

                VencCommonParamIntf::Lock lock = vencIntf->lock();
                actualFormat->value = mActualFormat = vencIntf->getActualFormat();

                input.configUpdate.push_back(std::move(actualFormat));
            } else {
                ExynosLogE("[%s] config() for actual format is failed", __FUNCTION__);
            }

            std::unique_ptr<C2ExynosStreamInternalUsageTuning::input> internalUsage =
                std::make_unique<C2ExynosStreamInternalUsageTuning::input>(0u, (*optBuffer)->usage());

            failures.clear();

            err = mParamIntf->config({internalUsage.get()}, C2_MAY_BLOCK, &failures);
            if (err != C2_OK) {
                ExynosLogE("[%s] config() for internal usage is failed", __FUNCTION__);
            }
        }

        ExynosLogD("[%s] buffer: %p", __FUNCTION__, (*optBuffer).get());

        return optBuffer;
    }

    return std::nullopt;
}

void ExynosC2EncComponent::updateC2Config_Subscribes(
    std::shared_ptr<C2Buffer>       c2buffer,
    std::shared_ptr<ExynosBuffer>   buffer) {
    if ((c2buffer.get() == nullptr) ||
        (buffer.get() == nullptr)) {
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return;
    }

    if (buffer->mParams.get() == nullptr) {
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return;
    }

    auto vencIntf = std::static_pointer_cast<VencCommonParamIntf>(mParamIntf);
    auto subscribes = vencIntf->getSubscribes();

    auto filterParams = std::static_pointer_cast<ExynosFilterParams>(buffer->mParams);

    for (size_t i = 0; i < subscribes->flexCount(); ++i) {
        auto &indices = subscribes->m.values[i];

        switch (indices) {
        case C2StreamPictureTypeInfo::output::PARAM_TYPE:
        {
            C2Config::picture_type_t type = (C2Config::picture_type_t)0;

            switch (buffer->mImageInfo.eFrameInfo & FrameInfo::FRAME_INFO_MASK) {
            case FrameInfo::IDRframe:
                type = C2Config::SYNC_FRAME;
                break;
            case FrameInfo::Iframe:
                type = C2Config::I_FRAME;
                break;
            case FrameInfo::Pframe:
                type = C2Config::P_FRAME;
                break;
            case FrameInfo::Bframe:
                type = C2Config::B_FRAME;
                break;
            default:
                break;
            }

            c2buffer->setInfo(std::make_shared<C2StreamPictureTypeInfo::output>(0u /* stream id */, type));
            ExynosLogV("[%s] frame type is 0x%x", __FUNCTION__, type);
        }
            break;
        case C2AndroidStreamAverageBlockQuantizationInfo::output::PARAM_TYPE:
        {
            auto filterParam = filterParams->getParam(ExynosParamIndex::AverageQpInfoIndex, TARGET_OWNER_COMPONENT);
            if (filterParam.get() != nullptr) {
                auto param = std::static_pointer_cast<ExynosParam<ParamAverageQpInfo>>(filterParam->getBaseParam());
                auto averageQp = std::make_shared<C2AndroidStreamAverageBlockQuantizationInfo::output>(0, param->m.value);

                c2buffer->setInfo(averageQp);
                ExynosLogV("[%s] average qp is %d", __FUNCTION__, averageQp->value);
            }
        }
            break;
        default:
            break;
        }
    }

    return;
}

void ExynosC2EncComponent::updateC2Config_AverageQp(
    std::shared_ptr<C2Buffer>       c2buffer,
    std::shared_ptr<ExynosBuffer>   buffer) {
    if ((c2buffer.get() == nullptr) ||
        (buffer.get() == nullptr)) {
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return;
    }

    if (buffer->mParams.get() == nullptr) {
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return;
    }

    auto vencIntf = std::static_pointer_cast<VencCommonParamIntf>(mParamIntf);

    if (vencIntf->getAverageQp()) {
        auto filterParams = std::static_pointer_cast<ExynosFilterParams>(buffer->mParams);

        auto filterParam = filterParams->getParam(ExynosParamIndex::AverageQpInfoIndex, TARGET_OWNER_COMPONENT);
        if (filterParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamAverageQpInfo>>(filterParam->getBaseParam());

            auto averageQp = std::make_shared<C2ExynosStreamAverageQpInfo::output>(0, param->m.value);

            c2buffer->setInfo(averageQp);

            ExynosLogD("[%s] average qp is %d", __FUNCTION__, averageQp->value);
        }
    }

    return;
}
