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
#include <chrono>
#include "Exynos_CodecDecBase_Filter.h"

#define LOG_ON
#include "ExynosLog.h"
#define LOG_TAG "ExynosCodecDecBaseFilter"

#define MAX_ALLOC_DELAY_TIME 10  /* timeout about buffer allocation */
#define FRAME_DROP_TIME_LIMIT 40
#define SW_OVERHEAD_TIME 10

bool ExynosCodecDecBaseFilter::onStart() {
    ExynosLogFunctionTrace();

    mAllocDuration.clear();
    mDelayableTimeMs = 0;

    return true;
}

bool ExynosCodecDecBaseFilter::onSetup(std::shared_ptr<ExynosBuffer> buffer) {
    ExynosLogFunctionTrace();

    if (buffer.get() == nullptr) {
        /* invalid parameter */
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return false;
    }

    if (buffer->getFlags() & ExynosBuffer::REPLICA) {
        ExynosLogD("[%s] discards replica input(%p)", __FUNCTION__, buffer.get());
        return false;
    }

    ExynosBufferInfo input;
    ExynosBufferInfo::reset(input);

    onSetInputBufferInfo(input, buffer);

    {
        auto shCodec = mCodec;

        if (shCodec.get() == nullptr) {
            /* obj is released */
            ExynosLogT("[%s] obj is released", __FUNCTION__);
            return false;
        }

        auto err = shCodec->setup(input);
        if (err == false) {
            ExynosLogE("[%s] setup() is failed", __FUNCTION__);
            return false;
        }

        err = shCodec->getOutputInfo(mWidth, mHeight, mNumMinDPB, mFormat);
        if (err == false) {
            ExynosLogE("[%s] getOutputInfo() is failed", __FUNCTION__);
            return false;
        }

        err = shCodec->getOutCropInfo(mOutCrop);
        if (err == false) {
            ExynosLogE("[%s] getOutCropInfo() is failed", __FUNCTION__);
            return false;
        }
    }

    /* generate a param for requesting configuration update */
    {
        auto param = std::make_shared<ExynosParam<ParamRequestParamUpdate>>();
        param->m.flags = (ExynosRequestedParamFlag)(REQUESTED_TYPE_PIC_SIZE |
                                                    REQUESTED_TYPE_MAX_PIC_SIZE |
                                                    REQUESTED_TYPE_MIN_DPB_CNT);

        auto filterParam = std::make_shared<ExynosFilterParam<ParamRequestParamUpdate>>(param);
        filterParam->registTargetFilter(TARGET_OWNER_COMPONENT);

        auto filterParams = std::static_pointer_cast<ExynosFilterParams>(buffer->mParams);
        filterParams->addParam(filterParam);
    }

    /* update output delay, if it was requested */
    {
        auto filterParams = std::static_pointer_cast<ExynosFilterParams>(buffer->mParams);

        auto updateFilterParam = filterParams->getParam(ExynosParamIndex::FuncOutputDelayUpdate, mID);
        if (updateFilterParam.get() != nullptr) {
            auto updateParam = std::static_pointer_cast<ExynosParam<FuncParamOutputDelayUpdate>>(updateFilterParam->getBaseParam());

            auto update = updateParam->m.func;

            if (update.get() != nullptr) {
                auto filterParam = std::make_shared<ExynosFilterParam<ParamOutputDelay>>();
                auto param = std::static_pointer_cast<ExynosParam<ParamOutputDelay>>(filterParam->getBaseParam());

                param->m.delay = mNumMinDPB;
                filterParam->registTargetFilter(TARGET_OWNER_COMPONENT);

                auto filterParams = std::make_shared<ExynosFilterParams>();
                filterParams->addParam(filterParam);

                (*update)(filterParams);
            }
        }
    }

    /* TODO : ret value handling */
    /* mDoResult.emplace_back(std::move(err)); */

    return true;
}

bool ExynosCodecDecBaseFilter::onStop() {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;

    if (shCodec.get() == nullptr) {
        /* obj is released */
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return false;
    }

    auto err = shCodec->flush();
    if (err == false) {
        ExynosLogE("[%s] flush() is failed", __FUNCTION__);
        return false;
    }

    return true;
}

bool ExynosCodecDecBaseFilter::onFlush() {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;

    if (shCodec.get() == nullptr) {
        /* obj is released */
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return false;
    }

    auto err = shCodec->flush();
    if (err == false) {
        ExynosLogE("[%s] flush() is failed", __FUNCTION__);
        return false;
    }

    mAllocDuration.clear();
    mDelayableTimeMs = 0;
    mRequestUpdate = true;

    return true;
}

void ExynosCodecDecBaseFilter::onApplyConfig(
    ExynosBufferInfo                &info,
    std::shared_ptr<ExynosParams>    params) {
    ExynosLogFunctionTrace();

    if ((params.get() == nullptr) ||
        params->empty()) {
        /* there is nothing to change */
        return;
    }

    ExynosCodecBaseFilter::onApplyConfig(info, params);

    auto filterParams = std::static_pointer_cast<ExynosFilterParams>(params);

    std::vector<ExynosParamIndex> decIndexes;

    if (!mIsConfigured) {
        decIndexes = {
            ExynosParamIndex::PixelFormatIndex,
            ExynosParamIndex::DecodeOrderIndex,
            ExynosParamIndex::ThumbnailModeIndex,
            ExynosParamIndex::CompressedColorIndex,

            ExynosParamIndex::DisplayDelayIndex,    /* h264, hevc */
        };
    }

    decIndexes.push_back(ExynosParamIndex::BlackBarIndex);

    applyFilterParams(info, decIndexes, filterParams);

    return;
}


void ExynosCodecDecBaseFilter::onAdditionalWorkForParam(
    ExynosParamIndex                    index,
    std::shared_ptr<ExynosParamBase>    param) {
    ExynosLogFunctionTrace();

    ExynosCodecBaseFilter::onAdditionalWorkForParam(index, param);

    return;
}

bool ExynosCodecDecBaseFilter::onProcessDone(ExynosBufferInfo input, ExynosBufferInfo output) {
    ExynosLogFunctionTrace();

    std::shared_ptr<ExynosBuffer> inbuffer = input.obj;

    if (inbuffer.get() == nullptr) {
        /* TODO : event handling */
        ExynosLogV("[%s] invalid parameter", __FUNCTION__);
    }

    std::shared_ptr<ExynosBuffer> outbuffer = output.obj;

    auto filterParams = std::static_pointer_cast<ExynosFilterParams>(inbuffer->mParams);

    /*
     * for updating max number of output buffer
     * - mRequestUpdate : configurations could be missing while flushing
     * - forceUpdate    : CSD had been flushed. update some information forcefully
     * - needUpdate     : need/try to update an output dealy
     */
    bool forceUpdate = (inbuffer->mImageInfo.eFrameInfo & FrameInfo::CodecSpecificData);
    bool needUpdate = ((mNumMaxOutput < mNumMinDPB) || mRequestUpdate || forceUpdate);

    if (forceUpdate) {
        auto filterParam = std::make_shared<ExynosFilterParam<ParamRequestParamUpdate>>();
        filterParam->registTargetFilter(TARGET_OWNER_COMPONENT);

        auto param       = std::static_pointer_cast<ExynosParam<ParamRequestParamUpdate>>(filterParam->getBaseParam());
        param->m.flags = (ExynosRequestedParamFlag)(REQUESTED_TYPE_PIC_SIZE |
                                                    REQUESTED_TYPE_MAX_PIC_SIZE |
                                                    REQUESTED_TYPE_MIN_DPB_CNT);
        filterParams->addParam(filterParam);
        ExynosLogV("[%s] request to update information at CSD", __FUNCTION__);
    }

    if (needUpdate) {  /* number of required DPB is over than max num for allocating on filter */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamOutputDelay>>();
        filterParam->registTargetFilter(TARGET_OWNER_COMPONENT);

        auto param = std::static_pointer_cast<ExynosParam<ParamOutputDelay>>(filterParam->getBaseParam());
        param->m.delay = mNumMinDPB;
        filterParams->addParam(filterParam);

        ExynosLogD("[%s] max number of output buffer: change to %d from %d", __FUNCTION__, mNumMaxOutput, mNumMinDPB);
        mNumMaxOutput = mNumMinDPB;
    }

    /* apply configurations generated from video codec */
    {
        /* black bar info */
        {
            auto param = std::static_pointer_cast<ExynosParam<ParamBlackBarInfo>>(output.params.getParam(ExynosParamIndex::BlackBarInfoIndex));
            if (param.get() != nullptr) {
                auto filterParam = std::make_shared<ExynosFilterParam<ParamBlackBarInfo>>(param);
                filterParam->registTargetFilter(TARGET_OWNER_COMPONENT);

                filterParams->addParam(filterParam);

                ExynosLogV("[%s] black bar info is (t:%d, l:%d, w:%d, h:%d)", __FUNCTION__,
                            param->m.rect.nTop, param->m.rect.nLeft, param->m.rect.nWidth, param->m.rect.nHeight);
            }
        }
    }

    if (outbuffer.get() != nullptr) {
        outbuffer->mImageInfo = output.stImageInfo;
    }

    mRequestUpdate = false;

    return true;
}

void ExynosCodecDecBaseFilter::onEventReceived(std::shared_ptr<ExynosListenerEvent> event) {
    ExynosLogFunctionTrace();

    if (event.get() == nullptr) {
        /* invalid parameter */
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return;
    }

    switch ((int)event->mType) {
    case ExynosVideoCodecEvent::EventResolutionChanged:
    {
        /* reconfigure output */
        reconfigOutput();
    }
        break;
    default:
        /* TODO */
        break;
    }
}

bool ExynosCodecDecBaseFilter::onFillOutBuffers() {
    ExynosLogFunctionTrace();

    std::lock_guard<std::mutex> lock(mReconfigMutex);  /* wait while reconfiguring output */

    allocOutBuffer();  /* allocate an output buffer */

    return true;
}

int ExynosCodecDecBaseFilter::onCheckNeedMoreBuffer() {
    auto shCodec = mCodec;

    if (shCodec.get() == nullptr) {
        /* obj is released */
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return 0;
    }

    int32_t reqOutBufCount = shCodec->getReqOutBufCount();
    int32_t reqTaskCnt = ((int32_t)mReqAllocCnt.use_count()) - 1;

    ExynosLogV("[%s] required: %d, requested: %d", __FUNCTION__, reqOutBufCount, reqTaskCnt);

    int count = (int)((reqOutBufCount > reqTaskCnt) ? (reqOutBufCount - reqTaskCnt) : 0);

    ExynosLogV("[%s] return count:%d", __FUNCTION__, count);

    return count;
}

bool ExynosCodecDecBaseFilter::onSetInputBufferInfo(
    ExynosBufferInfo                &input,
    std::shared_ptr<ExynosBuffer>    buffer) {
    ExynosLogFunctionTrace();

    auto handle = buffer->handle();

    input.eDataInfo     = DataInfo::UnusedData;
    input.nFD[0]        = handle->data[0];
    input.pBuffer[0]    = (void *)(unsigned long long)handle->data[0];  /* TODO */
    input.nAllocLen[0]  = buffer->size();
    input.nDataSize[0]  = buffer->getDataLen();
    input.nPlane        = 1;
    input.obj           = buffer;
    input.nID           = -1;

    input.stImageInfo   = buffer->mImageInfo;

    /* apply configurations */
    onApplyConfig(input, buffer->mParams);

    return true;
}

bool ExynosCodecDecBaseFilter::allocOutBuffer() {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;

    if (shCodec.get() == nullptr) {
        /* obj is released */
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return false;
    }

    /* TODO : set attribute on buffer */
    GraphicBufferAttribute attr;
    attr.mWidth  = mWidth;
    attr.mHeight = mHeight;
    attr.mFormat = mFormat;
    attr.mUsage  = 0;

    AllocArg arg;
    arg.attr        = attr;
    arg.limit       = mNumMinDPB;
    arg.checkLimit  = nullptr;
    arg.allocCount  = 0;

    int32_t refBufCount    = shCodec->getRefBufCount();
    int32_t regBufCount    = shCodec->getRegBufCount();
    int32_t inBufCount     = shCodec->getInBufCount();
    int32_t outBufCount    = shCodec->getOutBufCount();
    int32_t reqOutBufCount = shCodec->getReqOutBufCount();

    arg.checkLimit = [refCount = refBufCount, reqCount = reqOutBufCount, minCount = mNumMinDPB](int32_t allocCount, int32_t limit)->int32_t {
                        if ((limit > 0) &&
                            (allocCount >= limit)) {
                            if (((allocCount - refCount) < minCount) &&
                                (reqCount > 0)) {
                                /* allocated buffer is not able to be used
                                 * until referencing is finished.
                                 * then, decoding could be stuck
                                 * in this case, alloc should be done since min count is statisfied
                                 */
                                StaticExynosLog(Level::Debug, LOG_TAG, "[checkLimit] release limit(%d) for feeding non-ref DPB (alloc:%d, ref:%d, min:%d)",
                                                    limit, allocCount, refCount, minCount);
                                return 0;
                            }
                        }

                        return limit;
                    };

    ExynosLogD("[%s] getRefBufCount:%d, getRegBufCount:%d, getInBufCount:%d, getOutBufCount:%d, getReqOutBufCount:%d",
                __FUNCTION__, refBufCount, regBufCount, inBufCount, outBufCount, reqOutBufCount);

    if (mFnBufferAlloc.expired()) {
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return false;
    }

    std::shared_ptr<BufferAllocFnType> allocFunc = mFnBufferAlloc.lock();

    if (allocFunc.get() == nullptr) {
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return false;
    }

    /* allocate an output buffer */
    ExynosLogV("[%s] allocate output buffer : width(%d), height(%d), format(0x%x), limit(%d)", __FUNCTION__,
                attr.mWidth, attr.mHeight, attr.mFormat, arg.limit);

    if (mAllocDuration.getLimit() != mNumMinDPB) {
        /* update limit */
        mAllocDuration.setLimit(mNumMinDPB);
    }

    if (mDelayableTimeMs > 0) {
        if (mOperatingRate > 0) {
            int32_t operatingTimsMs = MAX(((1000 / (int32_t)mOperatingRate) - MAX_ALLOC_DELAY_TIME), 0);

            ExynosLogV("[%s] delayable time(%d ms) vs operating time(%d ms)", __FUNCTION__, mDelayableTimeMs, operatingTimsMs);

            mDelayableTimeMs = MAX(MIN(operatingTimsMs, mDelayableTimeMs), 0);
        }

        /* all allocation is succeed
         * it has a chance to take a rest for resource management
         */
        ExynosLogV("[%s] output buffer is enough. sleep for %d ms", __FUNCTION__, mDelayableTimeMs);

        std::this_thread::sleep_for(mDelayableTimeMs * 1ms);
        mDelayableTimeMs = 0;
    }

    auto ret = (*allocFunc)(arg);
    if (ret.first == EXYNOS_ERROR_TRY_AGAIN) {
        if (onCheckNeedMoreBuffer() > 0) {
            /* if allocating a buffer is failed continuously many times,
             * it is considered to get being "pause".
             * therefore, quit to request allocating a buffer.
             */
            if (mNumMaxOutput > 0) {  /* since number of DPB is decided */
                /* except for display order decoding in resource mode(buffer queue)
                 * set timeout about retry.
                 */
                if ((mIsDecodeOrder) ||
                    (mAllocMode != AllocMode::PreferResources)) {
                    if ((*mReqAllocCnt) > (ALLOC_RETRY_TIME / WAIT_ALLOC_TIME)) {
                        return false;
                    }
                }

                (*mReqAllocCnt)++;  /* mark */
            }

            int32_t reqTaskCnt = ((int32_t)mReqAllocCnt.use_count()) - 1;
            if (reqTaskCnt <= 0) {
                /* try again. allocating a buffer could be failed due to resource limitation */
                ExynosLogV("[%s] outbuffer is invalid. try again with doFillOutbuffers(), requested: %d", __FUNCTION__, reqTaskCnt);

                fillOutBuffers(1);
            } else {
                ExynosLogT("[%s] there is no available output buffer, requested: %d", __FUNCTION__, reqTaskCnt);
            }
        }

        return false;
    } else if (ret.first != EXYNOS_ERROR_NONE) {
        ExynosLogE("[%s] alloc() is failed. ret:0x%x", __FUNCTION__, ret.first);
        return false;
    }

    std::shared_ptr<ExynosBuffer> outbuffer = ret.second;

    (*mReqAllocCnt) = 0;  /* clear */

    auto adjustDelayFunc = [&]()->int32_t {
                                /* get delayable time based on history */
                                int32_t delay = mAllocDuration.getAverage(FRAME_DROP_TIME_LIMIT, MAX_ALLOC_DELAY_TIME);
                                int32_t adjustTime = MAX_ALLOC_DELAY_TIME + SW_OVERHEAD_TIME;

                                delay = delay - adjustTime;

                                return (delay > 0)? delay:0;
                            };

    mDelayableTimeMs = adjustDelayFunc();

    if (inBufCount >= (arg.allocCount - refBufCount)) {
        mDelayableTimeMs = 0;
    }

    outputBufferEnqueue(outbuffer);

    fillOutBuffers(onCheckNeedMoreBuffer());

    return true;
}

bool ExynosCodecDecBaseFilter::reconfigOutput() {
    ExynosLogFunctionTrace();

    std::lock_guard<std::mutex> lock(mReconfigMutex);

    auto shCodec = mCodec;

    if (shCodec.get() == nullptr) {
       /* obj is released */
       ExynosLogT("[%s] obj is released", __FUNCTION__);
       return false;
    }

    shCodec->resetup();

    shCodec->getOutputInfo(mWidth, mHeight, mNumMinDPB, mFormat);
    shCodec->getOutCropInfo(mOutCrop);

    mAllocDuration.clear();
    mDelayableTimeMs = 0;

    auto ret = fillOutBuffers();

    return ret;
}

bool ExynosCodecDecBaseFilter::outputBufferEnqueue(std::shared_ptr<ExynosBuffer> outbuffer) {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;

    if (shCodec.get() == nullptr) {
        /* obj is released */
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return false;
    }

    ExynosBufferInfo output;
    ExynosBufferInfo::reset(output);

    output.eDataInfo    = DataInfo::NoData;
    output.stImageInfo  = outbuffer->mImageInfo;

    ExynosUtils::GetYUVPlaneInfo(output.stImageInfo.nStride, output.stImageInfo.nHeight, output.stImageInfo.nFormat,
                                 output.nPlane, output.nAllocLen, outbuffer->getFlags());

    ExynosLogV("[%s] output buffer info : number of plane(%d)", __FUNCTION__, output.nPlane);

    auto handle = outbuffer->handle();

    for (int i = 0; i < output.nPlane; i++) {
        output.nFD[i]       = handle->data[i];
        output.pBuffer[i]   = (void *)(unsigned long long)handle->data[i];  /* TODO */
        output.nDataSize[i] = 0;
        ExynosLogV("[%s] output buffer info : fd(%d)", __FUNCTION__, output.nFD[i]);
    }

    output.obj = outbuffer;
    output.nID = -1;

    ExynosLogD("[%s] allocate output buffer : fd(%d), ptr(%p)", __FUNCTION__,
                output.nFD[0], output.obj.get());

    /* enqueue an output */
    auto err = shCodec->outputEnqueue(output);
    if (err != true) {
        /* TODO : error handling */
        ExynosLogE("[%s] outputEnqueue() is failed", __FUNCTION__);
        return false;
    }

    return true;
}

