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
#include "ExynosBufferAllocator.h"
#include "ExynosETC.h"

#define LOG_ON
#include "ExynosLog.h"
#undef LOG_TAG
#define LOG_TAG "ExynosC2Component"

#define OUTPUT_START_INDEX 1

ExynosC2Component::CommonParamIntf::CommonParamIntf(
    const std::shared_ptr<C2ReflectorHelper> &reflector,
    C2String name,
    C2Component::kind_t kind,
    C2Component::domain_t domain,
    C2String mediaType,
    ExynosC2FilterManager::FilterModuleInfoList listFilterInfo,
    std::vector<C2String> aliases) : C2InterfaceHelper(reflector), ExynosLog(name + "_Interface") {
    setDerivedInstance(this);

    mIsSecure = (name.find(".secure") != std::string::npos)? true:false;
    mListFilterInfo = listFilterInfo;

    {
        ExynosMutex<std::shared_ptr<ExynosFilterParams>>::LockObj filterParams(mFilterParams);

        (*filterParams) = std::make_shared<ExynosFilterParams>();
    }

    addParameter(
            DefineParam(mName, C2_PARAMKEY_COMPONENT_NAME)
            .withConstValue(AllocSharedString<C2ComponentNameSetting>(name.c_str()))
            .build());

    if (aliases.size()) {
        C2String joined;
        for (const C2String &alias : aliases) {
            if (joined.length()) {
                joined += ",";
            }
            joined += alias;
        }
        addParameter(
                DefineParam(mAliases, C2_PARAMKEY_COMPONENT_ALIASES)
                .withConstValue(AllocSharedString<C2ComponentAliasesSetting>(joined.c_str()))
                .build());
    }

    addParameter(
            DefineParam(mKind, C2_PARAMKEY_COMPONENT_KIND)
            .withConstValue(new C2ComponentKindSetting(kind))
            .build());

    addParameter(
            DefineParam(mDomain, C2_PARAMKEY_COMPONENT_DOMAIN)
            .withConstValue(new C2ComponentDomainSetting(domain))
            .build());

    if (mIsSecure == true) {
        addParameter(
                DefineParam(mSecureModeTuning, C2_PARAMKEY_SECURE_MODE)
                .withConstValue(new C2SecureModeTuning(C2Config::SM_READ_PROTECTED))
                .build());
    }

    /* single stream */
    addParameter(
            DefineParam(mInputStreamCount, C2_PARAMKEY_INPUT_STREAM_COUNT)
            .withConstValue(new C2PortStreamCountTuning::input(1))
            .build());

    addParameter(
            DefineParam(mOutputStreamCount, C2_PARAMKEY_OUTPUT_STREAM_COUNT)
            .withConstValue(new C2PortStreamCountTuning::output(1))
            .build());

    // set up buffer formats and allocators

    // default to linear buffers and no media type
    C2BufferData::type_t rawBufferType = C2BufferData::LINEAR;
    C2String rawMediaType;
    C2Allocator::id_t rawAllocator = C2AllocatorStore::DEFAULT_LINEAR;
    C2BlockPool::local_id_t rawPoolId = C2BlockPool::BASIC_LINEAR;
    C2BufferData::type_t codedBufferType = C2BufferData::LINEAR;
    int poolMask = GetCodec2PoolMask();
    C2Allocator::id_t preferredLinearId = GetPreferredLinearAllocatorId(poolMask);
    C2Allocator::id_t codedAllocator = preferredLinearId;
    C2BlockPool::local_id_t codedPoolId = C2BlockPool::BASIC_LINEAR;

    switch (domain) {
        case C2Component::DOMAIN_IMAGE: [[fallthrough]];
        case C2Component::DOMAIN_VIDEO:
        case C2Component::DOMAIN_OTHER:
            // TODO: should we define raw image? The only difference is timestamp handling
            rawBufferType = C2BufferData::GRAPHIC;
            rawMediaType = MEDIA_MIMETYPE_VIDEO_RAW;
            rawAllocator = C2PlatformAllocatorStore::GRALLOC;
            rawPoolId = C2BlockPool::BASIC_GRAPHIC;
            break;
        default:
            break;
    }
    bool isEncoder = kind == C2Component::KIND_ENCODER;

    // handle raw decoders
    if (mediaType == rawMediaType) {
        codedBufferType = rawBufferType;
        codedAllocator = rawAllocator;
        codedPoolId = rawPoolId;
    }

    addParameter(
            DefineParam(mInputFormat, C2_PARAMKEY_INPUT_STREAM_BUFFER_TYPE)
            .withConstValue(new C2StreamBufferTypeSetting::input(
                    0u, isEncoder ? rawBufferType : codedBufferType))
            .build());

    addParameter(
            DefineParam(mInputMediaType, C2_PARAMKEY_INPUT_MEDIA_TYPE)
            .withConstValue(AllocSharedString<C2PortMediaTypeSetting::input>(
                    isEncoder ? rawMediaType : mediaType))
            .build());

    addParameter(
            DefineParam(mOutputFormat, C2_PARAMKEY_OUTPUT_STREAM_BUFFER_TYPE)
            .withConstValue(new C2StreamBufferTypeSetting::output(
                    0u, isEncoder ? codedBufferType : rawBufferType))
            .build());

    addParameter(
            DefineParam(mOutputMediaType, C2_PARAMKEY_OUTPUT_MEDIA_TYPE)
            .withConstValue(AllocSharedString<C2PortMediaTypeSetting::output>(
                    isEncoder ? mediaType : rawMediaType))
            .build());

    C2Allocator::id_t inputAllocators[1] = { isEncoder ? rawAllocator : codedAllocator };
    C2Allocator::id_t outputAllocators[1] = { isEncoder ? codedAllocator : rawAllocator };
    C2BlockPool::local_id_t outputPoolIds[1] = { isEncoder ? codedPoolId : rawPoolId };

    addParameter(
            DefineParam(mInputAllocators, C2_PARAMKEY_INPUT_ALLOCATORS)
            .withDefault(C2PortAllocatorsTuning::input::AllocShared(inputAllocators))
            .withFields({ C2F(mInputAllocators, m.values[0]).any(),
                          C2F(mInputAllocators, m.values).inRange(0, 1) })
            .withSetter(Setter<C2PortAllocatorsTuning::input>::NonStrictValuesWithNoDeps)
            .build());

    addParameter(
            DefineParam(mOutputAllocators, C2_PARAMKEY_OUTPUT_ALLOCATORS)
            .withDefault(C2PortAllocatorsTuning::output::AllocShared(outputAllocators))
            .withFields({ C2F(mOutputAllocators, m.values[0]).any(),
                          C2F(mOutputAllocators, m.values).inRange(0, 1) })
            .withSetter(Setter<C2PortAllocatorsTuning::output>::NonStrictValuesWithNoDeps)
            .build());

    addParameter(
            DefineParam(mOutputPoolIds, C2_PARAMKEY_OUTPUT_BLOCK_POOLS)
            .withDefault(C2PortBlockPoolsTuning::output::AllocShared(outputPoolIds))
            .withFields({ C2F(mOutputPoolIds, m.values[0]).any(),
                          C2F(mOutputPoolIds, m.values).inRange(0, 1) })
            .withSetter(Setter<C2PortBlockPoolsTuning::output>::NonStrictValuesWithNoDeps)
            .build());

    // add stateless params
    addParameter(
            DefineParam(mSubscribedParamIndices, C2_PARAMKEY_SUBSCRIBED_PARAM_INDICES)
            .withDefault(C2SubscribedParamIndicesTuning::AllocShared(0u))
            .withFields({ C2F(mSubscribedParamIndices, m.values[0]).any(),
                          C2F(mSubscribedParamIndices, m.values).any() })
            .withSetter(CommonSetter::SubscribedParamIndicesSetter)
            .build());

    /* TODO

    addParameter(
            DefineParam(mCurrentWorkOrdinal, C2_PARAMKEY_CURRENT_WORK)
            .withDefault(new C2CurrentWorkTuning())
            .withFields({ C2F(mCurrentWorkOrdinal, m.timeStamp).any(),
                          C2F(mCurrentWorkOrdinal, m.frameIndex).any(),
                          C2F(mCurrentWorkOrdinal, m.customOrdinal).any() })
            .withSetter(Setter<C2CurrentWorkTuning>::NonStrictValuesWithNoDeps)
            .build());

    addParameter(
            DefineParam(mLastInputQueuedWorkOrdinal, C2_PARAMKEY_LAST_INPUT_QUEUED)
            .withDefault(new C2LastWorkQueuedTuning::input())
            .withFields({ C2F(mLastInputQueuedWorkOrdinal, m.timeStamp).any(),
                          C2F(mLastInputQueuedWorkOrdinal, m.frameIndex).any(),
                          C2F(mLastInputQueuedWorkOrdinal, m.customOrdinal).any() })
            .withSetter(Setter<C2LastWorkQueuedTuning::input>::NonStrictValuesWithNoDeps)
            .build());

    addParameter(
            DefineParam(mLastOutputQueuedWorkOrdinal, C2_PARAMKEY_LAST_OUTPUT_QUEUED)
            .withDefault(new C2LastWorkQueuedTuning::output())
            .withFields({ C2F(mLastOutputQueuedWorkOrdinal, m.timeStamp).any(),
                          C2F(mLastOutputQueuedWorkOrdinal, m.frameIndex).any(),
                          C2F(mLastOutputQueuedWorkOrdinal, m.customOrdinal).any() })
            .withSetter(Setter<C2LastWorkQueuedTuning::output>::NonStrictValuesWithNoDeps)
            .build());

    std::shared_ptr<C2OutOfMemoryTuning> mOutOfMemory;

    std::shared_ptr<C2PortConfigCounterTuning::input> mInputConfigCounter;
    std::shared_ptr<C2PortConfigCounterTuning::output> mOutputConfigCounter;
    std::shared_ptr<C2ConfigCounterTuning> mDirectConfigCounter;

    */

    addParameter(
            DefineParam(mOperateRateTuning, C2_PARAMKEY_OPERATING_RATE)
            .withDefault(new C2OperatingRateTuning(0))
            .withFields({ C2F(mOperateRateTuning, value).any() })
            .withSetter(CommonSetter::OperateRateSetter)
            .build());
    addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mOperateRateTuning)), CommonCnv::cnvOperateRate);

    addParameter(
        DefineParam(mRealTimePriorityTuning, C2_PARAMKEY_PRIORITY)
        .withDefault(new C2RealTimePriorityTuning(0))
        .withFields({ C2F(mRealTimePriorityTuning, value).any() })
        .withSetter(CommonSetter::RealTimePrioritySetter)
        .build());
    addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mRealTimePriorityTuning)), CommonCnv::cnvRealTimePriority);
}

c2_status_t ExynosC2Component::setListener_vb(
    const std::shared_ptr<Listener> &listener,
    c2_blocking_t mayBlock) {
    ExynosLogFunctionTrace();

    ExynosMutex<ComponentState>::LockObj comp(mStateMutex);

    if (comp->mState == State::RUNNING) {
        if (listener.get() != nullptr) {
            return C2_BAD_STATE;
        } else if (!mayBlock) {
            return C2_BLOCKING;
        }
    }

    mCallbackListener = listener;

    return C2_OK;
}

c2_status_t ExynosC2Component::queue_nb(std::list<std::unique_ptr<C2Work>> * const items) {
    ExynosLogFunctionTrace();

    ExynosMutex<ComponentState>::LockObj comp(mStateMutex);

    if ((comp->mState == State::UNLOADED) ||
        (comp->mState == State::LOADED)) {
        ExynosLogE("[%s] invalid state(%d)", __FUNCTION__, (int)comp->mState);
        return C2_BAD_STATE;
    }

    int cnt = (!items->empty())? items->size():0;

    {
        ExynosMutex<ExynosQueue<std::shared_ptr<WorkQueueElement>>>::LockObj workQ(mWorkQueue);
        /* items can have a lot of C2Works */
        while (!items->empty()) {
            /* get a C2Work */
            auto element = wrapShared<WorkQueueElement>(std::move(items->front()));

            workQ->enqueue(element);
            items->pop_front();

            ExynosLogV("[%s] c2work count: %d", __FUNCTION__, ++mC2WorkCount);
        }
    }

    inputProcessTrigger(cnt);

    /* TODO : ret value handling */

    return C2_OK;
}

c2_status_t ExynosC2Component::announce_nb(const std::vector<C2WorkOutline> &items) {
    /* TODO */
    (void)items;

    return C2_OMITTED;
}

c2_status_t ExynosC2Component::flush_sm(
    flush_mode_t flushMode,
    std::list<std::unique_ptr<C2Work>> * const flushedWork) {
    ExynosLogFunctionTrace();

    /* TODO */
    (void)flushMode;

    ExynosMutex<ComponentState>::LockObj comp(mStateMutex);

    ExynosLogV("[%s] start >> c2work count: %d", __FUNCTION__, (int)mC2WorkCount);

    if ((comp->mState == State::UNLOADED) ||
        (comp->mState == State::LOADED)) {
        ExynosLogE("[%s] invalid state(%d)", __FUNCTION__, (int)comp->mState);
        return C2_BAD_STATE;
    }

    mPendingFlushCount++;

    ExynosMutex<ExynosQueue<std::shared_ptr<WorkQueueElement>>>::LockObj workQ(mWorkQueue);

    flushQueue(workQ, flushedWork);

#ifdef USE_DEFFERING_WORK_QUEUE
    /* deffering queue */
    if (mDeferringWorkQueue.get() != nullptr) {
        ExynosMutex<ExynosQueue<std::shared_ptr<WorkQueueElement>>>::LockObj defferingWorkQ(*mDeferringWorkQueue.get());

    flushQueue(defferingWorkQ, flushedWork);
    }
#endif

    /* pending input */
    /* pending input refers WorkQueueElement.
     * so, it should be processed before clearing pendingWorkQueue.
     */
    PreFlush();

    /* pending queue */
    ExynosMutex<ExynosQueue<std::shared_ptr<WorkQueueElement>>>::LockObj pendingWorkQ(mPendingWorkQueue);

    flushQueue(pendingWorkQ, flushedWork);

    mC2WorkCount -= flushedWork->size();
    ExynosLogV("[%s] finish >> c2work count: %d", __FUNCTION__, (int)mC2WorkCount);

    auto shThreadPool = mThreadPool;

    if (shThreadPool.get() == nullptr) {
        /* obj is released */
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return C2_CORRUPTED;
    }

    shThreadPool->flush();
    shThreadPool->clearSession();

    /* doFlush task could be discarded by other flush_sm() called as rapidly
     * so, uses the notification
     */
    std::function<void()> notifyfunc =
        [wkOwner = weak_from_this()]() {
            if (wkOwner.expired()) {
                return;
            }

            auto shOwner = wkOwner.lock();

            if (shOwner.get() != nullptr) {
                shOwner->mPendingFlushCount--;
            }
        };

    shThreadPool->toss(std::string("ExynosC2Component::doFlush"),
                                  weak_pointer_bind(false, &ExynosC2Component::doFlush, weak_from_this()),
                                  std::move(notifyfunc));

    /* TODO : ret value handling */

    return C2_OK;
}

c2_status_t ExynosC2Component::drain_nb(drain_mode_t drainMode) {
    ExynosLogFunctionTrace();

    ExynosMutex<ComponentState>::LockObj comp(mStateMutex);

    if (drainMode == DRAIN_CHAIN) {
        ExynosLogE("[%s] drainMode(%d) is not supported", __FUNCTION__, (int)drainMode);
        return C2_OMITTED;
    }

    if (comp->mState != State::RUNNING) {
        ExynosLogE("[%s] invalid state(%d)", __FUNCTION__, (int)comp->mState);
        return C2_BAD_STATE;
    }

    auto element = wrapShared<WorkQueueElement>(drainMode);

    {
        ExynosMutex<ExynosQueue<std::shared_ptr<WorkQueueElement>>>::LockObj workQ(mWorkQueue);

        workQ->enqueue(element);
    }

    auto shThreadPool = mThreadPool;

    if (shThreadPool.get() == nullptr) {
        /* obj is released */
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return C2_CORRUPTED;
    }

    shThreadPool->toss(std::string("ExynosC2Component::doQueue"),
                                  weak_pointer_bind(false, &ExynosC2Component::doQueue, weak_from_this()));

    shThreadPool->incNextSession();

    return C2_OK;
}

c2_status_t ExynosC2Component::start() {
    ExynosLogFunctionTrace();

    ExynosMutex<ComponentState>::LockObj comp(mStateMutex);

    if (comp->mState != State::LOADED) {
        return C2_BAD_STATE;
    }

    if (comp->mInit == false) {
        auto shThreadPool = mThreadPool;

        if (shThreadPool.get() == nullptr) {
            /* obj is released */
            ExynosLogT("[%s] obj is released", __FUNCTION__);
            return C2_CORRUPTED;
        }

        auto err = shThreadPool->post(std::string("ExynosC2Component::doStart"),
                                      weak_pointer_bind(false, &ExynosC2Component::doStart, weak_from_this()));

        /* wait for getting a result */
        auto ret = WaitGetResultFromFuture(err, false);
        if (ret == false) {
            ExynosLogE("[%s] doStart() is failed", __FUNCTION__);
            return C2_NO_MEMORY;
        }

        comp->mInit = true;
    }

    comp->mState = State::RUNNING;

    return C2_OK;
}

c2_status_t ExynosC2Component::stop() {
    ExynosLogFunctionTrace();

    ExynosMutex<ComponentState>::LockObj comp(mStateMutex);

    if ((comp->mState != State::RUNNING) &&
        (comp->mState != State::ERROR)) {
        ExynosLogE("[%s] invalid state(%d)", __FUNCTION__, (int)comp->mState);
        return C2_BAD_STATE;
    }

    comp->mState = State::LOADED;

    if (comp->mInit != false) {
        auto shThreadPool = mThreadPool;

        if (shThreadPool.get() == nullptr) {
            /* obj is released */
            ExynosLogT("[%s] obj is released", __FUNCTION__);
            return C2_CORRUPTED;
        }

        shThreadPool->flush();
        shThreadPool->clearSession();

        auto err = shThreadPool->post(std::string("ExynosC2Component::doStop"),
                                      weak_pointer_bind(false, &ExynosC2Component::doStop, weak_from_this()));

        /* wait for getting a result */
        auto ret = WaitGetResultFromFuture(err, false);
        if (ret == false) {
            ExynosLogE("[%s] doStop() is timed out", __FUNCTION__);
        }

        comp->mInit = false;
    }

    {
        /* work queue */
        ExynosMutex<ExynosQueue<std::shared_ptr<WorkQueueElement>>>::LockObj workQ(mWorkQueue);

        if (!workQ->empty()) {
            workQ->clear();
        }
#ifdef USE_DEFFERING_WORK_QUEUE
        /* deffering queue */
        if (mDeferringWorkQueue.get() != nullptr) {
            ExynosMutex<ExynosQueue<std::shared_ptr<WorkQueueElement>>>::LockObj defferingWorkQ(*mDeferringWorkQueue.get());

            if (!defferingWorkQ->empty()) {
                defferingWorkQ->clear();
            }
        }
#endif
        /* pending queue */
        ExynosMutex<ExynosQueue<std::shared_ptr<WorkQueueElement>>>::LockObj pendingWorkQ(mPendingWorkQueue);

        if (!pendingWorkQ->empty()) {
            pendingWorkQ->clear();
        }

        mC2WorkCount = 0;
        ExynosLogV("[%s] c2work count: %d", __FUNCTION__, (int)mC2WorkCount);
    }

    mOrdinal.clear();

    return C2_OK;
}

c2_status_t ExynosC2Component::reset() {
    ExynosLogFunctionTrace();

    ExynosMutex<ComponentState>::LockObj comp(mStateMutex);

    if (comp->mInit != false) {
        if (comp->mState == State::RUNNING) {
            comp.unlock();
            stop();
            comp.lock();
        }

        auto shThreadPool = mThreadPool;

        if (shThreadPool.get() == nullptr) {
            /* obj is released */
            ExynosLogT("[%s] obj is released", __FUNCTION__);
            return C2_CORRUPTED;
        }

        shThreadPool->flush();
        shThreadPool->clearSession();

        auto err = shThreadPool->post(std::string("ExynosC2Component::doReset"),
                                      weak_pointer_bind(false, &ExynosC2Component::doReset, weak_from_this()));

        /* wait for getting a result */
        auto ret = WaitGetResultFromFuture(err, false);
        if (ret == false) {
            ExynosLogE("[%s] doReset() is timed out", __FUNCTION__);
        }

        comp->mInit = false;
    }

    comp->mState = State::LOADED;
    mOrdinal.clear();

    return C2_OK;
}

c2_status_t ExynosC2Component::release() {
    ExynosLogFunctionTrace();

    ExynosMutex<ComponentState>::LockObj comp(mStateMutex);

    if (comp->mState != State::LOADED) {
        switch (comp->mState) {
        case State::RUNNING:
        {
            /* TODO : is it really need to support?
             * according to the spec on c2component,
             * This method MUST be supported in "stopped" state.
             * retval(C2_BAD_STATE) means the component is running.
             */
            comp.unlock();
            stop();
            comp.lock();
        }
            break;
        case State::UNLOADED:
        {
            /* release() is already handled
             * there is nothing to do
             */
            return C2_OK;
        }
            break;
        default:
        {
            ExynosLogE("[%s] invalid state(%d)", __FUNCTION__, (int)comp->mState);
            return C2_BAD_STATE;
        }
            break;
        }
    }

    comp->mState = State::UNLOADED;

    auto shThreadPool = mThreadPool;

    if (shThreadPool.get() == nullptr) {
        /* obj is released */
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return C2_CORRUPTED;
    }

    shThreadPool->flush();
    shThreadPool->clearSession();

    auto err = shThreadPool->post(std::string("ExynosC2Component::doRelease"),
                                  weak_pointer_bind(false, &ExynosC2Component::doRelease, weak_from_this()));

    /* wait for getting a result */
    auto ret = WaitGetResultFromFuture(err, false);
    if (ret == false) {
        ExynosLogE("[%s] doRelease() is timed out", __FUNCTION__);
    }

    shThreadPool->stop();

    ExynosLogI("[%s] component is released", __FUNCTION__);

    return C2_OK;
}

std::shared_ptr<C2ComponentInterface> ExynosC2Component::intf() {
    return mIntf;
}

bool ExynosC2Component::filterWorkDone(std::unique_ptr<ExynosFilter::FilterWork> work) {
    ExynosLogFunctionTrace();

    if (work.get() == nullptr) {
        /* invalid parameter */
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return false;
    }

    /* TODO : state check */

    auto shThreadPool = mThreadPool;

    if (shThreadPool.get() == nullptr) {
        /* obj is released */
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return false;
    }

    auto shWork = wrapShared<std::unique_ptr<ExynosFilter::FilterWork>>(std::move(work));

    auto func = [wkOwner = weak_from_this(), shWork]()->bool {
                    if (wkOwner.expired()) {
                        return false;
                    }

                    auto shOwner = wkOwner.lock();

                    if (shOwner.get() != nullptr) {
                        if (shOwner->mPendingFlushCount > 0) {
                            return false;
                        }

                        return shOwner->doFilterWorkDone(std::move(*(shWork.get())));
                    }

                    return false;
                };

    shThreadPool->tossToCurSession(std::string("ExynosC2Component::doFilterWorkDone"), std::move(func));

    /* TODO : ret value handling */

    return true;
}

bool ExynosC2Component::setCallback(std::shared_ptr<ExynosC2FilterManager> filterManager,
                 std::shared_ptr<ExynosFilter::FilterListener> filterListener) {
    ExynosLogFunctionTrace();

    /* set callback */
    auto callbackfunc = [wkOwner = weak_from_this()](std::unique_ptr<ExynosFilter::FilterWork> work)->bool {
                            if (!wkOwner.expired()) {
                                auto shOwner = wkOwner.lock();

                                if (shOwner.get() != nullptr) {
                                    return shOwner->filterWorkDone(std::move(work));
                                }
                            }

                            return false;
                        };

    filterListener = ExynosFilter::FilterListener::makeListener(std::move(callbackfunc), mObjName);

    filterManager->setCallback(filterListener);

    return true;
}

c2_status_t ExynosC2Component::onRelease() {
    ExynosLogFunctionTrace();

    c2_status_t ret = C2_OK;

    if (mFilter.expired()) {
        return ret;
    }

    auto shFilter = mFilter.lock();

    if (shFilter.get() != nullptr) {
        shFilter->release();
    }

    mCompRes = nullptr;

    return ret;
}

void ExynosC2Component::onWorkDone(std::shared_ptr<WorkQueueElement> workElement) {
    ExynosLogFunctionTrace();

    if (workElement.get() == nullptr) {
        /* obj is released */
        return;
    }

    /* nothing to do */

    return;
}

bool ExynosC2Component::queueFilterWork(
    std::shared_ptr<WorkQueueElement>   workElement,
    std::shared_ptr<ExynosBuffer>       buffer) {
    ExynosLogFunctionTrace();

    if ((workElement.get() == nullptr) ||
        (buffer.get() == nullptr)) {
        /* invalid parameter */
        return false;
    }

    if (mFilter.expired()) {
        return false;
    }

    auto shFilter = mFilter.lock();

    if (shFilter.get() == nullptr) {
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return false;
    }

    ExynosLogD("[%s] input flag : 0x%x", "doQueue", workElement->mC2Work->input.flags);

    if (buffer->mParams.get() == nullptr) {
        buffer->mParams = std::static_pointer_cast<ExynosParams>(std::make_shared<ExynosFilterParams>());
    }

    /* configurations */
    {
        auto params = std::make_shared<ExynosFilterParams>();

        /* configurations piled by config_vb() */
        {
            auto paramIntf = std::static_pointer_cast<ExynosC2Component::CommonParamIntf>(mParamIntf);

            CommonParamIntf::Lock lock = paramIntf->lock();

            auto piledFilterParams = paramIntf->getPiledFilterParams();

            if (!piledFilterParams->empty()) {
                ExynosLogD("[%s] there is piled param(num: %d)", "doQueue", piledFilterParams->size());

                params = piledFilterParams;
            }
        }

        /* configurations on c2work */
        if (workElement->mC2Work->input.configUpdate.size()) {
            ExynosLogD("[%s] need to update configs(num: %d)", "doQueue",
                        workElement->mC2Work->input.configUpdate.size());

            onApplyConfig(workElement->mC2Work->input.configUpdate, std::static_pointer_cast<ExynosFilterParams>(params));
        }

        auto filterParams = std::static_pointer_cast<ExynosFilterParams>(buffer->mParams);
        filterParams->append(params);
    }

    /* set flags on C2Buffer to ExynosBuffer */
    if (workElement->mC2Work->input.flags & C2FrameData::FLAG_CODEC_CONFIG) {
        buffer->mImageInfo.eFrameInfo |= FrameInfo::CodecSpecificData;
        ExynosLogD("[%s] input detects CSD flag", "doQueue");
    }

    if (workElement->mC2Work->input.flags & C2FrameData::FLAG_END_OF_STREAM) {
        buffer->mImageInfo.eFrameInfo |= FrameInfo::EndOfStream;
        ExynosLogI("[%s] input detects EOS flag !!", "doQueue");
    }

    buffer->mImageInfo.nTimeStamp = workElement->mC2Work->input.ordinal.timestamp.peekull();

    /* create and update a filter work */
    auto work = std::make_unique<ExynosFilter::FilterWork>();

    work->buffers.push_back(buffer);
    work->inputIndex = 0;
    work->isDrain = (workElement->mDrainMode != WorkQueueElement::NO_DRAIN)? true:false;

    ExynosLogD("[%s] input timestamp: %lld", "doQueue", workElement->mC2Work->input.ordinal.timestamp);

    if (mUseCustomOrdinal) {
        if ((!(workElement->mC2Work->input.flags & C2FrameData::FLAG_END_OF_STREAM)) ||
            ((!(buffer->getFlags() & ExynosBuffer::REPLICA)) &&
             (workElement->mC2Work->input.ordinal.timestamp.peekll() > 0))) {
            mOrdinal.addTimestamp(workElement->mC2Work->input.ordinal.timestamp);
        }
    }

    /* send an filter work to filter component */
    shFilter->queueWork(std::move(work));

    ExynosLogD("[%s] queueWork: c2buffer: %p, exynos buffer: %p", "doQueue",
                (workElement->mC2Work->input.buffers.empty())? nullptr:workElement->mC2Work->input.buffers[0].get(), buffer.get());

    {
        ExynosMutex<ExynosQueue<std::shared_ptr<WorkQueueElement>>>::LockObj pendingWorkQ(mPendingWorkQueue);

        pendingWorkQ->enqueue(workElement);
    }

    return true;
}

std::unique_ptr<C2Work> ExynosC2Component::cloneC2Work(std::unique_ptr<C2Work> &org) {
    ExynosLogFunctionTrace();

    std::unique_ptr<C2Work> work(new C2Work);

    work->input.flags   = org->input.flags;
    work->input.ordinal = org->input.ordinal;

    work->worklets.emplace_back(new C2Worklet);
    work->worklets.front()->output.flags = (C2FrameData::flags_t)C2FrameData::FLAG_INCOMPLETE;
    work->worklets.front()->output.buffers.clear();
    work->worklets.front()->output.ordinal = work->input.ordinal;

    work->workletsProcessed = 1u;

    work->result = C2_OK;

    if (org->worklets.front()->output.buffers.size()) {
        auto it = org->worklets.front()->output.buffers.begin();

        work->worklets.front()->output.buffers.push_back(*it);
        org->worklets.front()->output.buffers.erase(it);
    }

    ExynosLogT("[%s] c2work(%p) is cloned to %p", __FUNCTION__, org.get(), work.get());

    return (work);
}

void ExynosC2Component::sendC2Work(std::unique_ptr<C2Work> c2work) {
    std::shared_ptr<C2Component::Listener> listener = mCallbackListener;

    if (listener.get() != nullptr) {
        std::list<std::unique_ptr<C2Work>> items;

        if (!c2work->worklets.empty()) {
            ExynosLogD("[sendC2Work] c2work: %p", c2work.get());
            ExynosLogD("[sendC2Work] output flag : 0x%x", c2work->worklets.front()->output.flags);
            ExynosLogD("[sendC2Work] output timestamp: %lld", c2work->worklets.front()->output.ordinal.timestamp);
            ExynosLogD("[sendC2Work] output customOrdinal: %lld", c2work->worklets.front()->output.ordinal.customOrdinal);
            ExynosLogV("[sendC2Work] input buffers: %d, output buffers: %d",
                                c2work->input.buffers.size(), c2work->worklets.front()->output.buffers.size());
        }

        items.push_back(std::move(c2work));

        listener->onWorkDone_nb(shared_from_this(), std::move(items));
    }
}

int32_t ExynosC2Component::getC2WorkCount() {
    ExynosLogFunctionTrace();

    ExynosMutex<ExynosQueue<std::shared_ptr<WorkQueueElement>>>::LockObj workQueue(mWorkQueue);
    ExynosMutex<ExynosQueue<std::shared_ptr<WorkQueueElement>>>::LockObj pendingWorkQueue(mPendingWorkQueue);

    return (workQueue->size() + pendingWorkQueue->size());
}

c2_status_t ExynosC2Component::onStop() {
    ExynosLogFunctionTrace();

    c2_status_t ret = C2_OK;

    if (mFilter.expired()) {
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return C2_NOT_FOUND;
    }

    auto shFilter = mFilter.lock();

    if (shFilter.get() == nullptr) {
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return C2_NOT_FOUND;
    }

    shFilter->stop();

    /* clear the buffer allocator to filters */
    mFilterManager->clearBlockPool();

    /* deinit replica buffer allocator */
    mReplicaInputBlockPool.reset();
    mReplicaBufferAllocator.reset();

    return ret;
}

c2_status_t ExynosC2Component::onApplyConfig(
    std::vector<std::unique_ptr<C2Param>> &c2config,
    std::shared_ptr<ExynosFilterParams>     params) {
    ExynosLogFunctionTrace();

    if (params.get() == nullptr) {
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return C2_BAD_VALUE;
    }

    if (mParamIntf.get() == nullptr) {
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return C2_BAD_VALUE;
    }

    std::vector<C2Param*> c2params;
    std::vector<C2Param::Index> heapParamIndices;
    for (auto it = c2config.begin(); it != c2config.end(); it++) {
        C2Param *c2param = (*it).get();
        if (c2param != nullptr) {
            c2params.push_back(c2param);
            heapParamIndices.push_back(c2param->index());
        }
    }

    std::vector<std::unique_ptr<C2SettingResult>> failures;
    auto err = mParamIntf->config(c2params, C2_MAY_BLOCK, &failures);
    if (err != C2_OK) {
        return err;
    }

    std::vector<std::unique_ptr<C2Param>> queryParams;
    err = mParamIntf->query({}, heapParamIndices, C2_MAY_BLOCK, &queryParams);

    if ((err == C2_OK) && (!queryParams.empty())) {
        CommonParamIntf::Lock lock = mParamIntf->lock();
        for (auto it = queryParams.begin(); it != queryParams.end(); it++) {
            C2Param * const c2param = (*it).get();

            if (c2param != nullptr) {
                auto param = mParamIntf->cnvC2ParamToFilterParam(*c2param, mParamIntf);
                if (!param.empty()) {
                    for (std::shared_ptr<ExynosFilterParamBase> &element : param) {
                        ExynosLogD("[%s] add filter param(index:%x, name:%s)", __FUNCTION__,
                                        element->index(), element->name().c_str());
                    }

                    params->addParam(param);
                }
            }
        }
    }

    return C2_OK;
}

c2_status_t ExynosC2Component::onFlush() {
    ExynosLogFunctionTrace();

    c2_status_t ret = C2_OK;

    if (mFilter.expired()) {
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return C2_NOT_FOUND;
    }

    auto shFilter = mFilter.lock();

    if (shFilter.get() == nullptr) {
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return C2_NOT_FOUND;
    }

    shFilter->flush();

    return ret;
}

c2_status_t ExynosC2Component::onReset() {
    ExynosLogFunctionTrace();

    c2_status_t ret = C2_OK;

    if (mFilter.expired()) {
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return C2_NOT_FOUND;
    }

    auto shFilter = mFilter.lock();

    if (shFilter.get() == nullptr) {
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return C2_NOT_FOUND;
    }

    shFilter->reset();

    /* clear the buffer allocator to filters */
    mFilterManager->clearBlockPool();

    /* deinit replica buffer allocator */
    mReplicaInputBlockPool.reset();
    mReplicaBufferAllocator.reset();

    ExynosLogI("[%s] resource is released (%u)", __FUNCTION__, (mCompRes.use_count() - 1));

    mCompRes = nullptr;

    return ret;
}

bool ExynosC2Component::doQueue() {
    ExynosLogFunctionTrace();

    /* TODO : state check */

    setBlockPool();

    if (mFilter.expired()) {
        return false;
    }

    auto shFilter = mFilter.lock();

    if (shFilter.get() == nullptr) {
        /* obj is released */
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return false;
    }

    std::shared_ptr<WorkQueueElement> element = nullptr;

    ExynosMutex<ExynosQueue<std::shared_ptr<WorkQueueElement>>>::LockObj workQ(mWorkQueue);

    if (workQ->empty()) {
        ExynosLogT("[%s] mWorkQueue is empty", __FUNCTION__);
        return false;
    }

    if (workQ->dequeue(element) == false) {
        /* invalid state */
        ExynosLogE("[%s] mWorkQueue.dequeue() is failed", __FUNCTION__);
        return false;
    }

    if ((element.get() != nullptr) &&
        (element->mC2Work.get() != nullptr)) {
        ExynosLogD("[%s] c2work: %p, c2buffer: %p", __FUNCTION__, element->mC2Work.get(),
                    (element->mC2Work->input.buffers.empty())? nullptr:element->mC2Work->input.buffers[0].get());

        if (element->mC2Work->worklets.empty()) {
            element->mC2Work->worklets.emplace_back(new C2Worklet);
        }

        /* initialize information for output */
        element->mC2Work->result = C2_OK;
        element->mC2Work->workletsProcessed = 0u;
        element->mC2Work->worklets.front()->output.flags = element->mC2Work->input.flags;

        if ((element->mC2Work->input.buffers.empty()) ||
            (element->mC2Work->input.buffers[0].get() == nullptr))  {
            ExynosLogV("[%s] c2work has null c2buffer with flag(0x%x)", __FUNCTION__,
                            element->mC2Work->input.flags);
            element->mC2Work->input.buffers.clear();
        }

        if (!onQueue(element)) {
            std::shared_ptr<C2Component::Listener> listener = mCallbackListener;

            if (listener.get() != nullptr) {
                std::list<std::unique_ptr<C2Work>> items;

                ExynosLogD("[sendC2Work] c2work: %p", element->mC2Work.get());
                ExynosLogD("[sendC2Work] output flag : 0x%x", element->mC2Work->input.flags);
                ExynosLogD("[sendC2Work] output timestamp: %lld", element->mC2Work->input.ordinal.timestamp);

                items.push_back(std::move(element->mC2Work));

                listener->onWorkDone_nb(shared_from_this(), std::move(items));
            }

            ExynosLogV("[%s] c2work count: %d", __FUNCTION__, --mC2WorkCount);

            return true;
        }
    }

    /* TODO */
    /* case 2. drain mode */
    /* how can send drain info to filter? if using job obj? */

    return true;
}

bool ExynosC2Component::doFlush() {
    ExynosLogFunctionTrace();

    /* TODO : state check */

    c2_status_t c2err = onFlush();

    if (c2err != C2_OK) {
        errorNotify(c2err);

        return false;
    }

    mIsAfterEOS = false;
    mOrdinal.clear();

    return true;
}

bool ExynosC2Component::doStart() {
    ExynosLogFunctionTrace();

    /* TODO : state check */
    mIsAfterEOS = false;

    c2_status_t c2err = onStart();

    if (c2err != C2_OK) {
        errorNotify(c2err);

        return false;
    }

    mOrdinal.clear();

    return true;
}

bool ExynosC2Component::doStop() {
    ExynosLogFunctionTrace();
    /* TODO : state check */

    mPendingFlushCount = 0;

    c2_status_t c2err = onStop();

    if (c2err != C2_OK) {
        errorNotify(c2err);

        return false;
    }

    return true;
}

bool ExynosC2Component::doReset() {
    ExynosLogFunctionTrace();

    mPendingFlushCount = 0;

    /* TODO : state check */

    c2_status_t c2err = onReset();

    if (c2err != C2_OK) {
        errorNotify(c2err);

        return false;
    }

    return true;
}

bool ExynosC2Component::doRelease() {
    ExynosLogFunctionTrace();

    /* TODO : state check */

    c2_status_t c2err = onRelease();

    if (c2err != C2_OK) {
        errorNotify(c2err);

        return false;
    }

    return true;
}

void ExynosC2Component::doNotifyError(c2_status_t c2err) {
    ExynosLogFunctionTrace();

    if (c2err != C2_OK) {
        std::shared_ptr<C2Component::Listener> listener = mCallbackListener;

        if (listener.get() != nullptr) {
            listener->onError_nb(shared_from_this(), c2err);
        }
    }

    return;
}

bool ExynosC2Component::doFilterWorkDone(std::unique_ptr<ExynosFilter::FilterWork> work) {
    ExynosLogFunctionTrace();

    if (work.get() == nullptr) {
        /* invalid parameter */
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return false;
    }

    /* exynosbuffer to c2buffer */
    std::shared_ptr<ExynosBuffer> inBuffer = work->buffers.front();
    auto optC2Buffer                       = ExynosBufferAllocator::exportC2Buffer(inBuffer);
    std::shared_ptr<uint64_t>     tag      = inBuffer->getMark();

    /* set condfunc for finding a c2work */
    std::function<bool(std::shared_ptr<WorkQueueElement> &e)> condfunc = nullptr;

    if (tag.get() != nullptr) {
        /* c2buffer could be already returned, then uses the tag */
        ExynosLogD("[%s] c2buffer: %p, exynos buffer: %p, tag: %llu", __FUNCTION__, inBuffer.get(), (*optC2Buffer).get(), (*tag));

        condfunc = [tag](std::shared_ptr<WorkQueueElement> &e)->bool {
                       return (e->mTag == (*tag))? true:false;
                   };
    } else if (optC2Buffer) {
        ExynosLogD("[%s] c2buffer: %p, exynos buffer: %p", __FUNCTION__, inBuffer.get(), (*optC2Buffer).get());

        condfunc = [c2Buffer = (*optC2Buffer)](std::shared_ptr<WorkQueueElement> &e)->bool {
                       return (e->mC2Work->input.buffers[0].get() == c2Buffer.get())? true:false;
                   };
    } else {
        ExynosLogE("[%s] c2buffer is invalid. exynos buffer: %p", __FUNCTION__, inBuffer.get());

        return false;
    }

    /* find a c2work which owns same c2buffer in pending queue */
    std::shared_ptr<WorkQueueElement> element = nullptr;

    ExynosMutex<ExynosQueue<std::shared_ptr<WorkQueueElement>>>::LockObj pendingWorkQ(mPendingWorkQueue);

    if (pendingWorkQ->dequeue(element, std::move(condfunc)) == false) {
        ExynosLogV("[%s] mPendingWorkQueue.dequeue() is failed", __FUNCTION__);
        return false;
    }

    ExynosLogD("[%s] mPendingWorkQueue size: %d", __FUNCTION__, pendingWorkQ->size());

    onWorkDone(element);

    /* initialize information on c2work */
    auto c2work = std::move(element->mC2Work);
    c2work->workletsProcessed = 0;
    c2work->result = C2_OK;
    element->mC2Work.reset();
    element.reset();  /* free a WorkQueueElement */

    /*
     * the list in filter work will have buffers
     * configured as input buffer + [temporal buffers] + output buffers sequentially.
     * skip temporal buffers and update output buffers to c2work.
     */
    if (work->buffers.size() > work->inputIndex) {
        auto it = work->buffers.begin();
        for (std::advance(it, work->inputIndex); it != work->buffers.end(); it++) {
            /* update output buffer to c2work */
            std::shared_ptr<ExynosBuffer> outBuffer = (*it);

            if ((outBuffer.get() != nullptr) &&
                (!(outBuffer->getFlags() & ExynosBuffer::CONFIGONLY))) {
                ExynosLogD("[%s] fd(%d), ptr(%p), index(%d)", __FUNCTION__,
                            (outBuffer->handle())->data[0], outBuffer.get(), work->inputIndex);
            } else {
                ExynosLogD("[%s] fd(-1), ptr(%p), index(%d)", __FUNCTION__, outBuffer.get(), work->inputIndex);
            }

            onUpdateC2Work(c2work, inBuffer, outBuffer);

            /* send cloned C2Work with an output buffer */
            if (std::next(it, 1) != work->buffers.end()) {
                auto worklet = c2work->worklets.begin();

                if (!(*worklet)->output.buffers.empty()) {
                    sendC2Work(cloneC2Work(c2work));
                }
            }

            auto params = std::static_pointer_cast<ExynosFilterParams>(outBuffer->mParams);

            outBuffer->mParams.reset();
            params.reset();
        }
    } else {
        onUpdateC2Work(c2work, inBuffer);
    }

    if (work->isDrain) {
        auto shThreadPool = mThreadPool;

        if (shThreadPool.get() == nullptr) {
            /* obj is released */
            ExynosLogT("[%s] obj is released", __FUNCTION__);
            return false;
        }

        auto old = shThreadPool->getCurSession();

        shThreadPool->incCurSession();

        ExynosLogD("[%s] detect drain. session(%llu) is end and start new session(%llu)",
                    __FUNCTION__, old, shThreadPool->getCurSession());
    } else {
#if 0
        /* data with EOS : send cloned c2work(data) and c2work(EOS) */
        if ((c2work->worklets.front()->output.flags & C2FrameData::FLAG_END_OF_STREAM) &&
            (c2work->worklets.front()->output.buffers.size() != 0) &&
            (c2work->input.buffers.size() != 0)) {
            sendC2Work(cloneC2Work(c2work));
        }
#endif
        sendC2Work(std::move(c2work));
    }

    work.reset();  /* free a filter work */

    ExynosLogV("[%s] c2work count: %d", __FUNCTION__, --mC2WorkCount);

    return true;
}

std::shared_ptr<ExynosC2Component::WorkQueueElement> ExynosC2Component::findWorkElement(std::shared_ptr<C2Buffer> c2buffer) {
    ExynosLogFunctionTrace();

    if (c2buffer.get() == nullptr) {
        /* invalid parameter */
        return nullptr;
    }

    std::shared_ptr<WorkQueueElement> element = nullptr;

    auto condfunc = [c2buffer](std::shared_ptr<WorkQueueElement> &e)->bool {
                        return (e->mC2Work->input.buffers[0].get() == c2buffer.get())? true:false;
                    };

    ExynosMutex<ExynosQueue<std::shared_ptr<WorkQueueElement>>>::LockObj pendingWorkQ(mPendingWorkQueue);

    if (pendingWorkQ->find(element, std::move(condfunc)) == false) {
        /* can not find */
        return nullptr;
    }

    return element;
}

bool ExynosC2Component::inputProcessTrigger(int workCount) {
    ExynosLogFunctionTrace();

    if (workCount > 0) {
        auto func = [wkOwner = weak_from_this(), workCount]() {
                        if (wkOwner.expired()) {
                            return;
                        }

                        auto shOwner = wkOwner.lock();

                        if (shOwner.get() != nullptr) {
                            if (shOwner->mPendingFlushCount > 0) {
                                return;
                            }

                            auto isEmpty = [shOwner]()->bool {
                                               ExynosMutex<ExynosQueue<std::shared_ptr<WorkQueueElement>>>::LockObj workQ(shOwner->mWorkQueue);
                                               StaticExynosLog(Level::Debug, "ExynosC2Component::queue_nb", "[%s] workQueue size: %d", __FUNCTION__, workQ->size());
                                               return workQ->empty();
                                           };

                            for (int i = 0; (i < workCount) && (!isEmpty()); i++) {
                                shOwner->doQueue();
                            }
                        }
                    };

        auto shThreadPool = mThreadPool;

        if (shThreadPool.get() == nullptr) {
            /* obj is released */
            ExynosLogT("[%s] obj is released", __FUNCTION__);
            return C2_CORRUPTED;
        }

        /* send a task */
        shThreadPool->toss(std::string("ExynosC2Component::doQueue"), std::move(func));
    }

    return true;
}

bool ExynosC2Component::flushQueue(ExynosMutex<ExynosQueue<std::shared_ptr<WorkQueueElement>>>::LockObj &srcQ,
                std::list<std::unique_ptr<C2Work>> * const flushedWork) {
    ExynosLogFunctionTrace();

    while (!srcQ->empty()) {
        std::shared_ptr<WorkQueueElement> element;

        if (srcQ->dequeue(element)) {
            std::unique_ptr<C2Work> work = std::move(element->mC2Work);
            element->mC2Work.reset();
            element.reset();

            if (work.get() != nullptr) {
                if (!(work->input.flags & C2FrameData::FLAG_CODEC_CONFIG)) {
                    work->input.buffers.clear();
                }
                if (!work->worklets.empty()) {
                    work->worklets.front()->output.buffers.clear();
                    work->worklets.clear();
                }

                work->result = C2_NOT_FOUND;
                flushedWork->push_back(std::move(work));
            }
        }
    }

    return true;
}

c2_status_t ExynosC2Component::makeFilterParam(std::vector<C2Param::Index> &indices) {
    ExynosLogFunctionTrace();

    c2_status_t ret = C2_OK;

    std::vector<std::unique_ptr<C2Param>> params;

    ret = mParamIntf->query({}, indices, C2_MAY_BLOCK, &params);
    if (ret == C2_OK) {
        CommonParamIntf::Lock lock = mParamIntf->lock();

        for (std::unique_ptr<C2Param> &element : params) {
            auto c2param = element.get();

            auto param = mParamIntf->cnvC2ParamToFilterParam(*c2param, mParamIntf);

            mParamIntf->keepFilterParam(param);
        }
    }

    return ret;
}

void ExynosC2Component::errorNotify(c2_status_t c2err) {
    auto shThreadPool = mThreadPool;

    if (!CHECK_SHARED_PTR(shThreadPool)) {
        return;
    }

    shThreadPool->tossToCurSession(std::string("ExynosC2Component::doNotifyError"),
                                  weak_pointer_bind(&ExynosC2Component::doNotifyError, weak_from_this(), c2err));

    /* TODO : ret value handling */
}

