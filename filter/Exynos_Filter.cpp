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
#include "Exynos_Filter.h"

#define LOG_ON
#include "ExynosLog.h"
#define LOG_TAG "ExynosFilter"

bool ExynosFilterBase::setNext(std::shared_ptr<ExynosFilter> nextFilter) {
    ExynosLogFunctionTrace();

    if (!CHECK_SHARED_PTR(nextFilter)) {
        return false;
    }

    /* create my listener and set it as callback to source filter */
    auto callbackfunc =
        [wkOwner = std::weak_ptr<ExynosFilter>(nextFilter)](std::unique_ptr<FilterWork> work)->bool {
            auto shOwner = GET_SHARED_PTR(wkOwner);
            if (shOwner.get() != nullptr) {
                return shOwner->queueWork(std::move(work));
            }

            return false;
        };

    mFilterListener = FilterListener::makeListener(std::move(callbackfunc), mObjName);
    if (!CHECK_SHARED_PTR(mFilterListener)) {
        return false;
    }

    bool ret = setCallback(mFilterListener);
    if (ret == false) {
        ExynosLogE("[%s] setCallback() is failed", __FUNCTION__);
        mFilterListener.reset();
        return false;
    }

    mNextFilter = nextFilter;

    return true;
}

bool ExynosFilterBase::setCallback(std::shared_ptr<FilterListener> listener) {
    ExynosLogFunctionTrace();

    if (!CHECK_SHARED_PTR(listener)) {
        return false;
    }

    mNotify = listener;

    return true;
}

bool ExynosFilterBase::setAllocator(std::shared_ptr<BufferAllocFnType> allocfunc) {
    ExynosLogFunctionTrace();

    mFnBufferAlloc = allocfunc;

    return true;
}

bool ExynosFilterBase::queueWork(std::unique_ptr<FilterWork> work) {
    ExynosLogFunctionTrace();

    if (!CHECK_UNIQUE_PTR(work)) {
        return false;
    }

    auto shThreadPool = mThreadPool;

    if (!CHECK_SHARED_PTR(shThreadPool)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mMutex);

    /* wrap to shared_ptr from unique_ptr */
    auto shWork = wrapShared<std::unique_ptr<FilterWork>>(std::move(work));

    shThreadPool->toss(std::string("ExynosFilter::doQueueWork"),
                                  weak_pointer_bind(false, &ExynosFilterBase::doQueueWork, weak_from_this(), shWork));

    /* TODO : ret value handling */
    /* mDoResult.emplace_back(std::move(ret)); */

    return true;
}

bool ExynosFilterBase::start() {
    ExynosLogFunctionTrace();

    bool ret = true;

    auto shThreadPool = mThreadPool;

    if (!CHECK_SHARED_PTR(shThreadPool)) {
        return false;
    }

    {
        auto err = shThreadPool->post(std::string("ExynosFilter::doStart"),
                                      weak_pointer_bind(false, &ExynosFilterBase::doStart, weak_from_this()));

        /* wait for getting a result */
        ret = WaitGetResultFromFuture(err, false);
        if (ret == false) {
            ExynosLogE("[%s] doStart() is timed out", __FUNCTION__);
        }
    }

    if (ret) {
        std::shared_ptr<ExynosFilter> shNextFilter = GET_SHARED_PTR_NOLOG(mNextFilter);

        if (shNextFilter.get() != nullptr) {
            ret = shNextFilter->start();
        }
    }

    return ret;
}

bool ExynosFilterBase::stop() {
    ExynosLogFunctionTrace();

    bool ret = true;

    auto shThreadPool = mThreadPool;

    if (!CHECK_SHARED_PTR(shThreadPool)) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mMutex);

        shThreadPool->flush();  /* clear tasks piled on threadpool */
        forceExit();

        auto err = shThreadPool->post(std::string("ExynosFilter::doStop"),
                                      weak_pointer_bind(false, &ExynosFilterBase::doStop, weak_from_this()));

        /* wait for getting a result */
        ret = WaitGetResultFromFuture(err, false);
        if (ret == false) {
            ExynosLogE("[%s] doStop() is timed out", __FUNCTION__);
        }

        forceExitDone();
    }

    mWorkInfos.clear();

    {
        std::lock_guard<std::mutex> lock(mPendingParamsMutex);

        if (mPendingParams.get() != nullptr) {
            mPendingParams->clear();
            mPendingParams.reset();
        }
    }

    if (ret) {
        std::shared_ptr<ExynosFilter> shNextFilter = GET_SHARED_PTR_NOLOG(mNextFilter);

        if (shNextFilter.get() != nullptr) {
            ret = shNextFilter->stop();
        }
    }

    return ret;
}

bool ExynosFilterBase::flush() {
    ExynosLogFunctionTrace();

    bool ret = true;

    auto shThreadPool = mThreadPool;

    if (!CHECK_SHARED_PTR(shThreadPool)) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mMutex);

        shThreadPool->flush();  /* clear tasks piled on threadpool */
        forceExit();

        auto err = shThreadPool->post(std::string("ExynosFilter::doFlush"),
                                      weak_pointer_bind(false, &ExynosFilterBase::doFlush, weak_from_this()));

        /* wait for getting a result */
        ret = WaitGetResultFromFuture(err, false);
        if (ret == false) {
            ExynosLogE("[%s] doFlush() is timed out", __FUNCTION__);
        }

        /*
         * it is for sync with flush.
         * although task(doFlush) was processed, there would be run task(doWorkDone) which is
         * generated by previous task(doProcess).
         * thus, creates a task for sync with flush.
         */
        {
            auto func = [name = mObjName]()->bool {
                            StaticExynosLog(Level::Debug, name.c_str(), "[doFlushDone] flush is completed");
                            return true;
                        };

            auto err = shThreadPool->post(std::string("ExynosFilter::doFlushDone"), std::move(func));

            /* wait for getting a result */
            ret = WaitGetResultFromFuture(err, false);
            if (ret == false) {
                ExynosLogE("[%s] doFlushDone() is timed out", __FUNCTION__);
            }
        }

        forceExitDone();
    }

    mWorkInfos.clear();

    {
        std::lock_guard<std::mutex> lock(mPendingParamsMutex);

        if (mPendingParams.get() != nullptr) {
            mPendingParams->clear();
            mPendingParams.reset();
        }
    }

    if (ret) {
        std::shared_ptr<ExynosFilter> shNextFilter = GET_SHARED_PTR_NOLOG(mNextFilter);

        if (shNextFilter.get() != nullptr) {
            ret = shNextFilter->flush();
       }
    }

    return ret;
}

bool ExynosFilterBase::reset() {
    ExynosLogFunctionTrace();

    bool ret = true;

    auto shThreadPool = mThreadPool;

    if (!CHECK_SHARED_PTR(shThreadPool)) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mMutex);

        shThreadPool->flush();  /* clear tasks piled on threadpool */

        forceExit();

        auto err = shThreadPool->post(std::string("ExynosFilter::doReset"),
                                      weak_pointer_bind(false, &ExynosFilterBase::doReset, weak_from_this()));

        /* wait for getting a result */
        ret = WaitGetResultFromFuture(err, false);
        if (ret == false) {
            ExynosLogE("[%s] doReset() is timed out", __FUNCTION__);
        }

        forceExitDone();
    }

    mWorkInfos.clear();

    {
        std::lock_guard<std::mutex> lock(mPendingParamsMutex);

        if (mPendingParams.get() != nullptr) {
            mPendingParams->clear();
            mPendingParams.reset();
        }
    }

    if (ret) {
        std::shared_ptr<ExynosFilter> shNextFilter = GET_SHARED_PTR_NOLOG(mNextFilter);

        if (shNextFilter.get() != nullptr) {
            ret = shNextFilter->reset();
        }
    }

    return ret;
}

bool ExynosFilterBase::release() {
    ExynosLogFunctionTrace();

    bool ret = true;

    auto shThreadPool = mThreadPool;

    if (!CHECK_SHARED_PTR(shThreadPool)) {
        return false;
    }

    {
        reset();

        std::lock_guard<std::mutex> lock(mMutex);

        shThreadPool->stop();  /* directly terminate a thread pool instead of delegation */
//      mThreadPool.reset();
    }

    if (ret) {
        std::shared_ptr<ExynosFilter> shNextFilter = GET_SHARED_PTR_NOLOG(mNextFilter);

        if (shNextFilter.get() != nullptr) {
            ret = shNextFilter->release();
        }

        mNextFilter.reset();
    }

    return ret;
}

bool ExynosFilterBase::processDone(ExynosBufferInfo input, ExynosBufferInfo output) {
    ExynosLogFunctionTrace();

    if (input.obj.get() == nullptr) {
        /* TODO : error handling */
        ExynosLogE("[%s] input is invalid", __FUNCTION__);
        return false;
    }

    if (output.obj.get() == nullptr) {
        /* TODO : error handling */
        ExynosLogE("[%s] output is invalid", __FUNCTION__);
        return false;
    }

    std::shared_ptr<ExynosBuffer> inbuffer = input.obj;

    ExynosLogD("[%s] inbuffer : fd(%d), ptr(%p)", __FUNCTION__,
                (inbuffer->handle())->data[0], inbuffer.get());

    std::shared_ptr<ExynosBuffer> outbuffer = output.obj;

    ExynosLogD("[%s] outbuffer : fd(%d), ptr(%p)", __FUNCTION__,
                (outbuffer->handle())->data[0], outbuffer.get());

    if (mDebug & EXYNOS_DEBUG_OUTPUT) {
        ExynosBuffer::dump(outbuffer, mDebug, (mObjName + "-output"));
    }

    /* get workInfo and save output buffer to it */
    auto workInfo = obtainWorkInfo(inbuffer, outbuffer, ((input.eDataInfo != UnusedData) && (mID != FIRST_FILTER_ID)));

    if (workInfo.get() == nullptr) {
        /* TODO : error handling */
        ExynosLogV("[%s] obtainWorkInfo() is failed", __FUNCTION__);
        return false;
    }

    if ((output.eDataInfo == DataInfo::MultiData) ||
        (workInfo->inDataNum > workInfo->outDataNum)) {
        reuseWorkInfo(workInfo);
    } else {
        /* delegation about doWorkDone could not be procced
         * if buffer allocation is failed continuously
         * so, it must be called directly
         */
        return doWorkDone(std::move(workInfo->work));
    }

    return true;
}

void ExynosFilterBase::eventReceived(std::shared_ptr<ExynosListenerEvent> event) {
    ExynosLogFunctionTrace();

    if (event.get() == nullptr) {
        /* invalid parameter */
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return;
    }

    /* TODO */
    return;
}

bool ExynosFilterBase::doWorkDone(std::unique_ptr<FilterWork> work) {
    ExynosLogFunctionTrace();

    if (work.get() == nullptr) {
        /* invalid parameter */
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return false;
    }

    workDone(std::move(work));

    return true;
}

std::shared_ptr<ExynosFilterBase::FilterWorkInfo> ExynosFilterBase::obtainWorkInfo(
    std::shared_ptr<ExynosBuffer> inbuffer,
    std::shared_ptr<ExynosBuffer> outbuffer,
    bool                          internalRelease) {
    ExynosLogFunctionTrace();

    std::lock_guard<std::mutex> lock(mWorkInfoMutex);

    std::shared_ptr<FilterWorkInfo> workInfo = nullptr;

    if (!CHECK_SHARED_PTR(inbuffer)) {
        return workInfo;
    }

    /* find workInfo which has a buffer same as inbuffer */
    auto condfunc = [inbuffer](std::shared_ptr<FilterWorkInfo> elment)->bool {
                        for (auto &buffer : elment->work->buffers) {
                            if (buffer.get() == inbuffer.get()) {
                                return true;
                            }
                        }

                        return false;
                    };

    /* get workInfo and set outbuffer, if outbuffer is valid */
    if (mWorkInfos.dequeue(workInfo, std::move(condfunc))) {
        if (CHECK_SHARED_PTR_NOLOG(workInfo)) {
            if (CHECK_SHARED_PTR_NOLOG(outbuffer)) {
                {
                    std::lock_guard<std::mutex> lock(mPendingParamsMutex);

                    if (CHECK_SHARED_PTR_NOLOG(mPendingParams)) {
                        /* update latest params to pending params */
                        mPendingParams->append(std::static_pointer_cast<ExynosFilterParams>(inbuffer->mParams));

                        /* update pending params to input */
                        auto filterParams = std::static_pointer_cast<ExynosFilterParams>(inbuffer->mParams);
                        filterParams->append(mPendingParams);

                        mPendingParams.reset();
                    }
                }

                outbuffer->mParams = inbuffer->mParams;  /* pass params to next filter */

                if (internalRelease) {
                    size_t bufferCount = workInfo->work->buffers.size();
                    auto it = workInfo->work->buffers.begin();
                    std::advance(it, (bufferCount - workInfo->inDataNum));

                    workInfo->work->buffers.erase(it);
                    workInfo->work->inputIndex--;
                }

                workInfo->work->buffers.push_back(outbuffer);
                workInfo->outDataNum++;
            } else {
                if ((CHECK_SHARED_PTR_NOLOG(inbuffer->mParams)) &&
                    (!inbuffer->mParams->empty())) {
                    if (inbuffer->mImageInfo.eFrameInfo & FrameInfo::CodecSpecificData) {
                        outbuffer = std::make_shared<ExynosBuffer>();
                        outbuffer->setFlags(ExynosBuffer::CONFIGONLY | ExynosBuffer::REPLICA);
                        outbuffer->mParams = inbuffer->mParams;  /* pass params to next filter */

                        workInfo->work->buffers.push_back(outbuffer);
                        workInfo->outDataNum++;

                        ExynosLogD("[%s] create buffer for configurations(num:%d)", __FUNCTION__, outbuffer->mParams->size());
                    } else {
                        std::lock_guard<std::mutex> lock(mPendingParamsMutex);

                        if (!CHECK_SHARED_PTR_NOLOG(mPendingParams)) {
                            mPendingParams = std::make_shared<ExynosFilterParams>();
                        }

                        mPendingParams->append(std::static_pointer_cast<ExynosFilterParams>(inbuffer->mParams));

                        ExynosLogD("[%s] keep params (%d)", __FUNCTION__, mPendingParams->size());
                    }
                }
            }
        }
    }

    return workInfo;
}

void ExynosFilterBase::reuseWorkInfo(std::shared_ptr<FilterWorkInfo> workInfo) {
    ExynosLogFunctionTrace();

    std::lock_guard<std::mutex> lock(mWorkInfoMutex);

    if (!CHECK_SHARED_PTR(workInfo)) {
        return;
    }

    ExynosLogV("[%s] filter work: %p", __FUNCTION__, workInfo->work.get());
    mWorkInfos.enqueue(workInfo);
}

bool ExynosFilterBase::workDone(std::unique_ptr<FilterWork> work) {
    ExynosLogFunctionTrace();

    if (!CHECK_UNIQUE_PTR(work)) {
        return false;
    }

    /* TODO : ret value handling */
    /* mDoResult.emplace_back(std::move(ret)); */

    auto notify = GET_SHARED_PTR(mNotify);

    if (!CHECK_SHARED_PTR(notify)) {
        return false;
    }

    ExynosLogD("[%s] filter work : ptr(%p)", __FUNCTION__, work.get());
    return notify->workDone(std::move(work));
}

bool ExynosFilterBase::bypassBuffer(std::shared_ptr<ExynosBuffer> buffer) {
    ExynosLogFunctionTrace();

    if (buffer.get() == nullptr) {
        /* invalid parameter */
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return false;
    }

    auto workInfo = obtainWorkInfo(buffer, buffer);

    if (workInfo.get() == nullptr) {
        /* TODO : error handling */
        ExynosLogV("[%s] obtainWorkInfo() is failed", __FUNCTION__);
        return false;
    }

    if (workInfo->inDataNum > workInfo->outDataNum) {
        reuseWorkInfo(workInfo);
        return true;
    }

    workInfo->work->inputIndex -= workInfo->inDataNum;

    for (int i = 0; i < workInfo->outDataNum; i++) {
        workInfo->work->buffers.pop_back();
    }

    /* delegation about doWorkDone could not be procced
     * if buffer allocation is failed continuously
     * so, it must be called directly
     */
    return doWorkDone(std::move(workInfo->work));
}

std::optional<std::shared_ptr<ExynosBuffer>> ExynosFilterBase::allocBuffer(AllocArg arg) {
    ExynosLogFunctionTrace();

    auto allocator = GET_SHARED_PTR(mFnBufferAlloc);
    if (allocator == nullptr) {
        return std::nullopt;
    }

    /* allocate a buffer */
    std::shared_ptr<ExynosBuffer> buffer = nullptr;

    while (!mQuitWork) {
        auto ret = (*allocator)(arg);
        if (ret.first == EXYNOS_ERROR_TRY_AGAIN) {
            continue;
        }

        buffer = ret.second;
        break;
    }

    if (buffer.get() == nullptr) {
        if (!mQuitWork) {
            ExynosLogE("[%s] buffer allocation is failed", __FUNCTION__);
        }

        return std::nullopt;
    }

    return { buffer };
}

bool ExynosFilterBase::doQueueWork(std::shared_ptr<std::unique_ptr<FilterWork>> shWork) {
    ExynosLogFunctionTrace();

    if (!CHECK_SHARED_PTR(shWork)) {
        return false;
    }

    auto shThreadPool = mThreadPool;

    if (!CHECK_SHARED_PTR(shThreadPool)) {
        return false;
    }

    /* unwrap to unique_ptr from shared_ptr */
    std::unique_ptr<FilterWork> work = std::move(*(shWork.get()));

    /* process a work */
    size_t bufferCount = work->buffers.size();

    if (bufferCount <= (size_t)work->inputIndex) {
        /* TODO : error handling */
        ExynosLogV("[%s] index(%d) >= buffer count(%d). prev filter might not generate output", __FUNCTION__, work->inputIndex, bufferCount);
        workDone(std::move(work));
        return false;
    }

    auto workInfo = std::make_shared<FilterWorkInfo>((bufferCount - work->inputIndex), 0, std::move(work));

    /* save workInfo */
    mWorkInfos.enqueue(workInfo);

    ExynosLogD("[%s] filter work : ptr(%p)", __FUNCTION__, workInfo->work.get());

#if 0
    // ?´ê²½???¤ë¥¸ thread?ì„œ filterJob??ë³€ê²½í•˜???¤ë¥˜ ë°œìƒ
    // for loop ê³¼ì • ì¤?next step?ì„œ ì¶”ê??˜ëŠ” ë²„í¼ê°€ ë¬¸ì œ
    // ?¥í›„ filterJob struct ?˜ì • ?„ìš”.
    // filterInternalJob->filterJob->inputIndex++; ?´ë?ë¶?ë¬¸ì œ ?ˆìŒ
    auto it = workInfo->work->buffers.begin();
    for (std::advance(it, filterInternalJob->filterJob->inputIndex); it != filterInternalJob->filterJob->buffers.end(); it++) {
#else
    /* get a buffer and process it */
    auto it = workInfo->work->buffers.begin();
    std::advance(it, workInfo->work->inputIndex);

    workInfo->work->inputIndex += workInfo->inDataNum;
    for (int i = 0; i < workInfo->inDataNum; i++) {
#endif
        std::shared_ptr<ExynosBuffer> buffer = *it;

        it++;

        /* delegation may occurs dead lock problem while DRC */
        auto err = doProcess(buffer);
        if (err == false) {
            auto condfunc = [inbuffer = buffer](std::shared_ptr<FilterWorkInfo> elment)->bool {
                                for (auto &buffer : elment->work->buffers) {
                                    if (buffer.get() == inbuffer.get()) {
                                        return true;
                                    }
                                }

                                return false;
                            };

            std::shared_ptr<FilterWorkInfo> workInfo = nullptr;

            if (mWorkInfos.dequeue(workInfo, std::move(condfunc))) {
                workInfo.reset();
            }
        }
    }

    return true;
}

