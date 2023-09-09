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
#include "Exynos_CodecBase_Filter.h"

#define LOG_ON
#include "ExynosLog.h"
#define LOG_TAG "ExynosCodecBaseFilter"


static std::optional<std::shared_ptr<ExynosParamBase>> getExynosParam(
    std::shared_ptr<ExynosFilterParams> filterParams,
    ExynosParamIndex                    index,
    uint32_t                            id) {
    if (filterParams.get() != nullptr) {
        auto filterParam = filterParams->getParam(index, id);

        if (filterParam.get() != nullptr) {
            return { filterParam->getBaseParam() };
        }
    }

    return std::nullopt;
}

bool ExynosCodecBaseFilter::onProcess(std::shared_ptr<ExynosBuffer> buffer) {
    ExynosLogFunctionTrace();

    if (buffer.get() == nullptr) {
        /* invalid parameter */
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return false;
    }

    if (mIsConfigured == false) {
        /* TODO : error handling */
        ExynosLogE("[%s] it is not configured", __FUNCTION__);
        return false;
    }

    ExynosBufferInfo input;
    ExynosBufferInfo::reset(input);

    onSetInputBufferInfo(input, buffer);

    /* enqueue an input */
    {
        auto shCodec = mCodec;

        if (shCodec.get() == nullptr) {
            /* obj is released */
            ExynosLogT("[%s] obj is released", __FUNCTION__);
            return false;
        }

        auto err = shCodec->inputEnqueue(input);
        if (err == false) {
            ExynosLogE("[%s] inputEnqueue() is failed", __FUNCTION__);
            return false;
        }
    }

    return true;
}

void ExynosCodecBaseFilter::onApplyConfig(
    ExynosBufferInfo                &info,
    std::shared_ptr<ExynosParams>    params) {
    ExynosLogFunctionTrace();

    if ((params.get() == nullptr) ||
        params->empty()) {
        /* there is nothing to change */
        return;
    }

    auto filterParams = std::static_pointer_cast<ExynosFilterParams>(params);

    std::vector<ExynosParamIndex> comIndexes = {
        ExynosParamIndex::OperatingRateIndex,
        ExynosParamIndex::ColorAspectsIndex,
        ExynosParamIndex::RealTimePriorityIndex,
    };

    applyFilterParams(info, comIndexes, filterParams);

    return;
}

void ExynosCodecBaseFilter::onEventReceived(std::shared_ptr<ExynosListenerEvent> event) {
    ExynosLogFunctionTrace();

    if (event.get() == nullptr) {
        /* invalid parameter */
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return;
    }
}

void ExynosCodecBaseFilter::onAdditionalWorkForParam(
    ExynosParamIndex                    index,
    std::shared_ptr<ExynosParamBase>    param) {
    switch (index) {
    case ExynosParamIndex::OperatingRateIndex:
    {
        auto childParam = std::static_pointer_cast<ExynosParam<ParamOperatingRate>>(param);

        ExynosLogI("[%s] operating rate : %d", __FUNCTION__, childParam->m.value);

        mOperatingRate = childParam->m.value;
    }
        break;
    case ExynosParamIndex::ColorAspectsIndex:
    {
        auto childParam = std::static_pointer_cast<ExynosParam<ParamColorAspects>>(param);

        ExynosLogD("[%s] color aspects : r(%d), p(0x%x), t(0x%x), m(0x%x)", __FUNCTION__,
                    childParam->m.range, childParam->m.primaries, childParam->m.transfer, childParam->m.matrix);
    }
        break;
    default:
        break;
    }

    return;
}

bool ExynosCodecBaseFilter::doFillOutBuffers() {
    ExynosLogFunctionTrace();

    if (mIsConfigured == false) {
        ExynosLogT("[%s] it is not configured", __FUNCTION__);
        return false;
    }

    return onFillOutBuffers();
}

bool ExynosCodecBaseFilter::fillOutBuffers(uint32_t num) {
    ExynosLogFunctionTrace();

    if (mIsConfigured == false) {
        ExynosLogT("[%s] filter was stopped. will not create a task(doFillOutBuffers)", __FUNCTION__);
        return true;
    }

    auto shThreadPool = mAllocThreadPool;

    if (shThreadPool.get() == nullptr) {
        /* obj is released */
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return false;
    }

    /* fill output buffers as much as number of required output */
    if (num == 0) {
        num = onCheckNeedMoreBuffer();
    }

    for (int i = 0; i < num; i++) {
        auto func = [wkOwner = weak_from_this(), reqTaskCnt = mReqAllocCnt]() mutable ->bool {
                        // UNUSED(reqTaskCnt);  /* the purpose of this variable is just holding ref. count */
                        reqTaskCnt.reset();  /* release a ref. count before calling doFillOutBuffers() */

                        if (wkOwner.expired()) {
                            return false;
                        }

                        auto shOwner = std::static_pointer_cast<ExynosCodecBaseFilter>(wkOwner.lock());

                        if (shOwner.get() != nullptr) {
                            return shOwner->doFillOutBuffers();
                        }

                        return false;
                    };

        shThreadPool->toss(std::string("ExynosCodecBaseFilter::doFillOutBuffers"), std::move(func));
    }

    return true;
}

void ExynosCodecBaseFilter::applyFilterParams(
    ExynosBufferInfo                    &info,
    std::vector<ExynosParamIndex>        indexList,
    std::shared_ptr<ExynosFilterParams>  filterParams) {
    ExynosLogFunctionTrace();

    for (int i = 0; i < indexList.size(); i++) {
        auto optBaseParam = getExynosParam(filterParams, indexList[i], mID);
        if (optBaseParam != std::nullopt) {
            std::shared_ptr<ExynosParamBase> param = (*optBaseParam);

            ExynosLogV("[%s] %s", __FUNCTION__, param->name().c_str());

            onAdditionalWorkForParam(indexList[i], param);

            info.params.addParam(param);  /* it will be processed by videocodec */
        }
    }

    return;
}

bool ExynosCodecBaseFilter::processDone(ExynosBufferInfo input, ExynosBufferInfo output) {
    ExynosLogFunctionTrace();

    /* process specialized features on each codec */
    if (onProcessDone(input, output) != true) {
        /* TODO : error handling */
        ExynosLogE("[%s] onProcessDone() is failed", __FUNCTION__);
        return false;
    }

    std::shared_ptr<ExynosBuffer> outbuffer = nullptr;
    if (output.eDataInfo != DataInfo::NoData) {
        /* set buffer if data on buffer is valid */
        outbuffer = output.obj;
    }

    if ((input.eDataInfo == DataInfo::NoData) &&
        (input.obj.get() == nullptr)) {
        /* only process output */
        return true;
    }

    if (outbuffer.get() != nullptr) {
        ExynosLogD("[%s] outbuffer : fd(%d), ptr(%p)", __FUNCTION__,
                    (outbuffer->handle())->data[0], outbuffer.get());

        if (mDebug & EXYNOS_DEBUG_OUTPUT) {
            ExynosBuffer::dump(outbuffer, mDebug, (mObjName + "-output"));
        }
    }

    std::shared_ptr<ExynosBuffer> inbuffer = input.obj;

    if (inbuffer.get() == nullptr) {
        /* TODO : error handling */
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return false;
    }

    ExynosLogD("[%s] inbuffer : fd(%d), ptr(%p)", __FUNCTION__,
                (inbuffer->handle() == nullptr)? -1:inbuffer->handle()->data[0], inbuffer.get());

    if (input.eDataInfo == DataInfo::CorruptedData) {
        inbuffer->mImageInfo.eFrameInfo = FrameInfo::CorruptedFrame;
    }

    /* get workInfo and save output buffer to it */
    auto workInfo = obtainWorkInfo(inbuffer, outbuffer);

    if (workInfo.get() == nullptr) {
        ExynosLogV("[%s] obtainWorkInfo() is failed", __FUNCTION__);
        fillOutBuffers();  /* for processing piled inputs in video codec */
        return false;
    }

    ExynosLogD("[%s] filter work : ptr(%p)", __FUNCTION__, workInfo->work.get());

    if (output.eDataInfo == DataInfo::MultiData) {
        /* decoder : one input creates multiple outputs
         * encoder : CSD
         */
        reuseWorkInfo(workInfo);
    } else {
        auto shThreadPool = mThreadPool;

        if (shThreadPool.get() == nullptr) {
            /* obj is released */
            ExynosLogT("[%s] obj is released", __FUNCTION__);
            return false;
        }

        fillOutBuffers();  /* it must be here for preventing invalid request after stop */

        /* return a work */
        auto func = [wkOwner = weak_from_this(), workInfo]()->bool {
                        if (wkOwner.expired()) {
                            return true;
                        }

                        auto shOwner = std::static_pointer_cast<ExynosCodecBaseFilter>(wkOwner.lock());

                        if (shOwner.get() != nullptr) {
                            return shOwner->doWorkDone(std::move(workInfo->work));
                        }

                        return false;
                    };

        shThreadPool->toss(std::string("ExynosCodecBaseFilter::doWorkDone"), std::move(func));

        /* TODO : ret value handling */
        /* mDoResult.emplace_back(std::move(ret)); */
    }

    return true;
}

void ExynosCodecBaseFilter::eventReceived(std::shared_ptr<ExynosListenerEvent> event) {
    ExynosLogFunctionTrace();

    if (event.get() == nullptr) {
        /* invalid parameter */
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return;
    }

    switch ((int)event->mType) {
    case ExynosListenerEvent::EventError:
        /* TODO */
        break;
    default:
        onEventReceived(event);
        break;
    }
}

bool ExynosCodecBaseFilter::doStart() {
    ExynosLogFunctionTrace();

    if ((!mIsConfigured) &&
        (mCodec.get() == nullptr)) {
        /* initialize resources once */
        mCodec = std::make_shared<ParallelProcessingVideoCodec>(mCodecType);

        if (mCodec.get() == nullptr) {
            ExynosLogE("[%s] create ParallelProcessingVideoCodec(%x) is failed", __FUNCTION__, mCodecType);
            return false;
        }

        auto owner = shared_from_this();
        if (owner.get() == nullptr) {
            /* obj is released */
            ExynosLogT("[%s] obj is released", __FUNCTION__);
            return false;
        }

        mListener = ExynosListener::makeListener(std::static_pointer_cast<ExynosListenerInterface>(owner),
                                                 mObjName);

        if (mCodec->setCallback(mListener) == false) {
            ExynosLogE("[%s] setCallback() is failed", __FUNCTION__);
            doStop();
            return false;
        }
    }

    if (mAllocThreadPool.get() == nullptr) {
        mAllocThreadPool = std::make_shared<ExynosThreadPool>(1, mObjName + "-Alloc");
    }

    mDebug = ExynosUtils::GetDebugType(mObjName);

    mAllocThreadPool->flush();

    (*mReqAllocCnt) = 0;  /* clear */

    auto ret = onStart();

    return ret;
}

bool ExynosCodecBaseFilter::doStop() {
    ExynosLogFunctionTrace();

    auto shAllocThreadPool = mAllocThreadPool;
    if (shAllocThreadPool.get() != nullptr) {
        shAllocThreadPool->flush();

        std::weak_ptr<ExynosCodecBaseFilter> wkCodecFilter = std::static_pointer_cast<ExynosCodecBaseFilter>(shared_from_this());
        auto err = shAllocThreadPool->post(std::string("ExynosCodecBaseFilter::doStopSync"),
                                           weak_pointer_bind(false, &ExynosCodecBaseFilter::doStopSync, wkCodecFilter));

        /* wait for getting a result */
        auto ret = WaitGetResultFromFuture(err, false);
        if (ret == false) {
            ExynosLogE("[%s] doStopSync() is timed out", __FUNCTION__);
        }

        return ret;
    }

    return onStop();
}

bool ExynosCodecBaseFilter::doFlush() {
    ExynosLogFunctionTrace();

    auto shAllocThreadPool = mAllocThreadPool;
    if (shAllocThreadPool.get() != nullptr) {
        shAllocThreadPool->flush();

        std::weak_ptr<ExynosCodecBaseFilter> wkCodecFilter = std::static_pointer_cast<ExynosCodecBaseFilter>(shared_from_this());
        auto err = shAllocThreadPool->post(std::string("ExynosCodecBaseFilter::doFlushSync"),
                                           weak_pointer_bind(false, &ExynosCodecBaseFilter::doFlushSync, wkCodecFilter));

        /* wait for getting a result */
        auto ret = WaitGetResultFromFuture(err, false);
        if (ret == false) {
            ExynosLogE("[%s] doFlushSync() is timed out", __FUNCTION__);
        }

        return ret;
    }

    return onFlush();
}

bool ExynosCodecBaseFilter::doReset() {
    ExynosLogFunctionTrace();

    mListener.reset();

    if (mAllocThreadPool.get() != nullptr) {
        mAllocThreadPool->flush();
        mAllocThreadPool->stop();
        mAllocThreadPool.reset();
    }

    if (mCodec.get() != nullptr) {
        mCodec->stopAllThreadPool();
        mCodec.reset();
    }

    mIsConfigured = false;
    mOperatingRate = 0;

    return true;
}

bool ExynosCodecBaseFilter::doProcess(std::shared_ptr<ExynosBuffer> buffer) {
    ExynosLogFunctionTrace();

    if (buffer.get() == nullptr) {
        /* invalid parameter */
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return false;
    }

    if (mDebug & EXYNOS_DEBUG_INPUT) {
        ExynosBuffer::dump(buffer, mDebug, (mObjName + "-input"));
    }

    bool ret = false;

    auto retWorkFunc = [&](std::shared_ptr<ExynosBuffer> buffer) {
                            auto workInfo = obtainWorkInfo(buffer);

                            if (workInfo != nullptr) {
                                auto shThreadPool = mThreadPool;

                                if (shThreadPool.get() == nullptr) {
                                    /* obj is released */
                                    ExynosLogT("[%s] obj is released", __FUNCTION__);
                                    return;
                                }

                                auto func = [wkOwner = weak_from_this(), workInfo]()->bool {
                                                if (wkOwner.expired()) {
                                                    return true;
                                                }

                                                auto shOwner = std::static_pointer_cast<ExynosCodecBaseFilter>(wkOwner.lock());

                                                if (shOwner.get() != nullptr) {
                                                    return shOwner->doWorkDone(std::move(workInfo->work));
                                                }

                                                return false;
                                            };

                                shThreadPool->toss(std::string("ExynosCodecBaseFilter::doWorkDone"), std::move(func));
                            }
                        };

    if (mIsConfigured == false) {
        if (onSetup(buffer) != true) {
            retWorkFunc(buffer);
            return false;
        }

        mIsConfigured = true;
    }

    if (onProcess(buffer) != true) {
        retWorkFunc(buffer);
        return false;
    }

    ret = fillOutBuffers();

    return ret;
}

bool ExynosCodecBaseFilter::doWorkDone(std::unique_ptr<FilterWork> work) {
    ExynosLogFunctionTrace();

    bool ret = ExynosFilterBase::doWorkDone(std::move(work));
    /* now, work is get to be invalid */

    return ret;
}

bool ExynosCodecBaseFilter::doStopSync() {
    ExynosLogFunctionTrace();

    auto ret = onStop();

    auto shAllocThreadPool = mAllocThreadPool;
    if (shAllocThreadPool.get() != nullptr) {
        /* clear tasks again since
         * it could be piled by the task
         * what run just before doStopSync
         */
        shAllocThreadPool->flush();
    }

    return ret;
}

bool ExynosCodecBaseFilter::doFlushSync() {
    ExynosLogFunctionTrace();

    return onFlush();
}

bool ExynosCodecBaseFilter::checkRealTimeResource(
    int32_t width,
    int32_t height,
    int32_t operatingRate) {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;

    if (shCodec.get() == nullptr) {
        /* obj is released */
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return false;
    }

    auto err = shCodec->checkRealTimeResource(width, height, operatingRate);
    if (err == false) {
        ExynosLogE("[%s] check real time resource is failed", __FUNCTION__);
        return false;
    }

    return true;
}
