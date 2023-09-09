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
#ifndef EXYNOS_C2_COMPONENT_H
#define EXYNOS_C2_COMPONENT_H

#include <functional>
#include <memory>
#include <list>
#include <limits>
#include <string>
#include <utility>
#include <algorithm>
#include <map>
#include <vector>

#include <C2Component.h>
#include <C2Config.h>
#include <util/C2InterfaceHelper.h>
#include <media/stagefright/foundation/MediaDefs.h>

#include <C2ComponentFactory.h>
#include <C2PlatformSupport.h>

#include "C2ExynosSupport.h"
#include "C2ExynosParam.h"
#include "Exynos_C2_Utils.h"
#include "Exynos_C2_FilterManager.h"
#include "Exynos_C2_Ordinal.h"
#include "ExynosMutex.h"
#include "Exynos_C2_ComponentRM.h"
#include "Exynos_C2_IntfSetter.h"
#include "Exynos_C2_FilterParamCnv.h"

#define LOG_ON
#include "ExynosLog.h"

class ExynosC2Component : public C2Component, public ExynosLog, public std::enable_shared_from_this<ExynosC2Component> {
public:
    /*
     * Common Codec 2.0 parameters for all components.
     */
    class CommonParamIntf;
    class WorkQueueElement {
    public:
        static constexpr uint32_t NO_DRAIN = ~0u;

        WorkQueueElement(std::unique_ptr<C2Work> work) : mC2Work(std::move(work)) {
            mTag = 0;
        }

        WorkQueueElement(uint32_t drainMode) : mDrainMode(drainMode) {
            mC2Work = std::make_unique<C2Work>();
            mC2Work->worklets.emplace_back(new C2Worklet);
            mTag = 0;
        }

        uint32_t mDrainMode = NO_DRAIN;
        std::unique_ptr<C2Work> mC2Work = nullptr;
        uint64_t mTag;

    private:
        /* disable default constructor */
        WorkQueueElement() = delete;
    };

    ExynosC2Component(const std::shared_ptr<C2ComponentInterface> &intf) : ExynosLog("ExynosC2Component"),
                                                                           mStateMutex(ExynosMutex<ComponentState>()),
                                                                           mThreadPool(std::make_shared<ExynosThreadPool>(true, 1, mObjName)),
                                                                           mCallbackListener(nullptr), mIsAfterEOS(false),
                                                                           mIntf(intf), mC2WorkCount(0), mPendingFlushCount(0) {
        ExynosLogFunctionTrace();

        mUseCustomOrdinal = false;
        mCompRes = nullptr;
    }

    virtual ~ExynosC2Component() {
        ExynosLogFunctionTrace();

        mThreadPool->stop();

        mCallbackListener.reset();
        mThreadPool.reset();

        mFilterListener.reset();
        mFilterManager.reset();

        {
            ExynosMutex<ExynosQueue<std::shared_ptr<WorkQueueElement>>>::LockObj workQ(mWorkQueue);

            workQ->clear();
        }
        {
            ExynosMutex<ExynosQueue<std::shared_ptr<WorkQueueElement>>>::LockObj pendingWorkQ(mPendingWorkQueue);

            pendingWorkQ->clear();
        }

        mCompRes = nullptr;
    }

    /* override function on C2Component */
    c2_status_t setListener_vb(const std::shared_ptr<Listener> &listener, c2_blocking_t mayBlock) override;
    c2_status_t queue_nb(std::list<std::unique_ptr<C2Work>> * const items) override;
    c2_status_t announce_nb(const std::vector<C2WorkOutline> &items) override;
    c2_status_t flush_sm(flush_mode_t mode, std::list<std::unique_ptr<C2Work>> * const flushedWork) override;
    c2_status_t drain_nb(drain_mode_t mode) override;
    c2_status_t start() override;
    c2_status_t stop() override;
    c2_status_t reset() override;
    c2_status_t release() override;
    std::shared_ptr<C2ComponentInterface> intf() override;

    /* add function for ExynosC2Component */
    bool filterWorkDone(std::unique_ptr<ExynosFilter::FilterWork> work);
    void errorNotify(c2_status_t c2err);

    template<class F, class G, class... Args>
    bool sendTesk(std::string logTag, F&& f, std::weak_ptr<G> weak, Args&&... args) {
        auto shThreadPool = mThreadPool;
        if (shThreadPool.get() == nullptr) {
            /* obj is released */
            return false;
        }

        shThreadPool->toss(logTag, weak_pointer_bind(false, f, weak, args...));

        return true;
    }

protected:
    enum class State: int {
        UNLOADED,
        LOADED,
        RUNNING,
        TRIPPED,
        ERROR,
    };

    class ComponentState {
    public:
        ComponentState() : mInit(false), mState(State::LOADED) {
        }

        ~ComponentState() = default;

        bool mInit;
        State mState;  /* TODO : need "std::atomic<State> mState;" ?? */
    };

    /* function for ExynosC2Component's child class */
    virtual bool setCallback(std::shared_ptr<ExynosC2FilterManager> filterManager,
                     std::shared_ptr<ExynosFilter::FilterListener> filterListener);
    virtual c2_status_t onStart() = 0;
    virtual c2_status_t onStop();
    virtual c2_status_t onSetup() = 0;
    virtual bool        onQueue(std::shared_ptr<WorkQueueElement> workElement) = 0;
    virtual c2_status_t onApplyConfig(std::vector<std::unique_ptr<C2Param>> &c2config, std::shared_ptr<ExynosFilterParams> params);
    virtual c2_status_t onFlush();
    virtual c2_status_t onReset();
    virtual c2_status_t onRelease();
    virtual void onWorkDone(std::shared_ptr<WorkQueueElement> workElement);
    virtual void onUpdateC2Work(std::unique_ptr<C2Work> &c2work,
                                std::shared_ptr<ExynosBuffer> inBuffer = nullptr,
                                std::shared_ptr<ExynosBuffer> outBuffer = nullptr) = 0;

    virtual void setBlockPool() = 0;
    virtual std::optional<std::shared_ptr<ExynosBuffer>> getExynosBuffer(C2FrameData &input) = 0;
    virtual void PreFlush() {
        return;
    }

    bool queueFilterWork(std::shared_ptr<WorkQueueElement> workElement, std::shared_ptr<ExynosBuffer> exynosBuffer);
    std::unique_ptr<C2Work> cloneC2Work(std::unique_ptr<C2Work> &org);
    virtual void sendC2Work(std::unique_ptr<C2Work> c2work);
    virtual int32_t getC2WorkCount();
    std::shared_ptr<WorkQueueElement> findWorkElement(std::shared_ptr<C2Buffer> c2buffer);
    c2_status_t makeFilterParam(std::vector<C2Param::Index> &indices);

    ExynosMutex<ComponentState>         mStateMutex;
    std::shared_ptr<CommonParamIntf>    mParamIntf;

    ExynosC2ComponentRM::ComponentResource mCompRes;

    std::shared_ptr<ExynosThreadPool>   mThreadPool;
    std::shared_ptr<Listener>           mCallbackListener;  /* c2 client */

#ifdef USE_DEFFERING_WORK_QUEUE
    /* waiting for available c2buffer coming, defer WorkQueueElement processing */
    std::shared_ptr<ExynosMutex<ExynosQueue<std::shared_ptr<WorkQueueElement>>>> mDeferringWorkQueue;
#endif

    std::shared_ptr<ExynosC2FilterManager>          mFilterManager;
    std::weak_ptr<ExynosFilter>                     mFilter;
    std::shared_ptr<ExynosFilter::FilterListener>   mFilterListener;

    /* replica input buffer */
    std::shared_ptr<C2BlockPool> mReplicaInputBlockPool;
    std::shared_ptr<ExynosBufferAllocator> mReplicaBufferAllocator;

    std::atomic_bool mIsAfterEOS;

    bool             mUseCustomOrdinal;
    ExynosC2Ordinal  mOrdinal;

private:
    /* function for thread pool owned by self */
    bool doQueue();
    bool doFlush();
    bool doStart();
    bool doStop();
    bool doReset();
    bool doRelease();
    void doNotifyError(c2_status_t err);
    bool doFilterWorkDone(std::unique_ptr<ExynosFilter::FilterWork> work);

    bool inputProcessTrigger(int workCount);
    bool flushQueue(ExynosMutex<ExynosQueue<std::shared_ptr<WorkQueueElement>>>::LockObj &srcQ,
                    std::list<std::unique_ptr<C2Work>> * const flushedWork);

    ExynosMutex<ExynosQueue<std::shared_ptr<WorkQueueElement>>> mWorkQueue;
    ExynosMutex<ExynosQueue<std::shared_ptr<WorkQueueElement>>> mPendingWorkQueue;

    const std::shared_ptr<C2ComponentInterface> mIntf;

    std::atomic_int mC2WorkCount;
    std::atomic_int mPendingFlushCount;

    /* disable default constructor */
    ExynosC2Component() = delete;
};

class ExynosC2Component::CommonParamIntf : public C2InterfaceHelper, public ExynosLog {
public:
    CommonParamIntf(const std::shared_ptr<C2ReflectorHelper> &helper,
                    C2String name,
                    C2Component::kind_t kind,
                    C2Component::domain_t domain,
                    C2String mediaType,
                    ExynosC2FilterManager::FilterModuleInfoList listFilterInfo,
                    std::vector<C2String> aliases = std::vector<C2String>());

    virtual ~CommonParamIntf() {
        ExynosMutex<std::shared_ptr<ExynosFilterParams>>::LockObj filterParams(mFilterParams);

        (*filterParams).reset();
    }

    bool mIsSecure;

    std::shared_ptr<C2ApiLevelSetting>      mApiLevel;
    std::shared_ptr<C2ApiFeaturesSetting>   mApiFeatures;

    std::shared_ptr<C2PlatformLevelSetting>     mPlatformLevel;
    std::shared_ptr<C2PlatformFeaturesSetting>  mPlatformFeatures;

    std::shared_ptr<C2ComponentNameSetting>         mName;
    std::shared_ptr<C2ComponentAliasesSetting>      mAliases;
    std::shared_ptr<C2ComponentKindSetting>         mKind;
    std::shared_ptr<C2ComponentDomainSetting>       mDomain;
    std::shared_ptr<C2ComponentAttributesSetting>   mAttribute;
    std::shared_ptr<C2ComponentTimeStretchTuning>   mTimeStretch;

    std::shared_ptr<C2PortMediaTypeSetting::input>      mInputMediaType;
    std::shared_ptr<C2PortMediaTypeSetting::output>     mOutputMediaType;

    std::shared_ptr<C2StreamBufferTypeSetting::input>   mInputFormat;
    std::shared_ptr<C2StreamBufferTypeSetting::output>  mOutputFormat;

    std::shared_ptr<C2PortRequestedDelayTuning::input>  mRequestedInputDelay;
    std::shared_ptr<C2PortRequestedDelayTuning::output> mRequestedOutputDelay;
    std::shared_ptr<C2RequestedPipelineDelayTuning>     mRequestedPipelineDelay;

    std::shared_ptr<C2PortActualDelayTuning::input>         mActualInputDelay;
    std::shared_ptr<C2PortActualDelayTuning::output>        mActualOutputDelay;
    std::shared_ptr<C2PortReorderBufferDepthTuning::output> mReorderDepth;
    std::shared_ptr<C2PortReorderKeySetting::output>        mReorderKey;
    std::shared_ptr<C2ActualPipelineDelayTuning>            mActualPipelineDelay;

    std::shared_ptr<C2StreamMaxReferenceAgeTuning::input>       mMaxInputReferenceAge;
    std::shared_ptr<C2StreamMaxReferenceCountTuning::input>     mMaxInputReferenceCount;
    std::shared_ptr<C2StreamMaxReferenceAgeTuning::output>      mMaxOutputReferenceAge;
    std::shared_ptr<C2StreamMaxReferenceCountTuning::output>    mMaxOutputReferenceCount;

    std::shared_ptr<C2MaxPrivateBufferCountTuning>      mMaxPrivateBufferCount;
    std::shared_ptr<C2PortStreamCountTuning::input>     mInputStreamCount;
    std::shared_ptr<C2PortStreamCountTuning::output>    mOutputStreamCount;

    std::shared_ptr<C2SubscribedParamIndicesTuning> mSubscribedParamIndices;

    std::shared_ptr<C2PortSuggestedBufferCountTuning::input>    mSuggestedInputBufferCount;
    std::shared_ptr<C2PortSuggestedBufferCountTuning::output>   mSuggestedOutputBufferCount;

    std::shared_ptr<C2CurrentWorkTuning>            mCurrentWorkOrdinal;
    std::shared_ptr<C2LastWorkQueuedTuning::input>  mLastInputQueuedWorkOrdinal;
    std::shared_ptr<C2LastWorkQueuedTuning::output> mLastOutputQueuedWorkOrdinal;

    std::shared_ptr<C2PortAllocatorsTuning::input>        mInputAllocators;
    std::shared_ptr<C2PortAllocatorsTuning::output>       mOutputAllocators;
    std::shared_ptr<C2PrivateAllocatorsTuning>            mPrivateAllocators;
    std::shared_ptr<C2PortSurfaceAllocatorTuning::output> mOutputSurfaceAllocatorId;

    std::shared_ptr<C2PortBlockPoolsTuning::output> mOutputPoolIds;
    std::shared_ptr<C2PrivateBlockPoolsTuning>      mPrivatePoolIds;

    std::shared_ptr<C2TrippedTuning>        mTripped;

    std::shared_ptr<C2OutOfMemoryTuning>    mOutOfMemory;

    std::shared_ptr<C2PortConfigCounterTuning::input>   mInputConfigCounter;
    std::shared_ptr<C2PortConfigCounterTuning::output>  mOutputConfigCounter;
    std::shared_ptr<C2ConfigCounterTuning>              mDirectConfigCounter;

    std::shared_ptr<C2SecureModeTuning> mSecureModeTuning;

    std::shared_ptr<C2OperatingRateTuning> mOperateRateTuning;
    std::shared_ptr<C2RealTimePriorityTuning> mRealTimePriorityTuning;

    uint32_t findFilterID(std::string name) {
        if (mListFilterInfo.empty()) {
            StaticExynosLog(Level::Error, "CommonParamIntf", "[%s] filter info is invalid", __FUNCTION__);
            return 0;
        }

        uint32_t i = 1;  /* number is started with 1. 0 means "owner" */
        for (auto &element : mListFilterInfo) {
            if (element.name.find(name) != std::string::npos) {
                return i;
            }

            i++;
        }

        return 0;
    }

    /* getter functions */
    std::shared_ptr<C2SubscribedParamIndicesTuning> getSubscribes() const { return mSubscribedParamIndices; }
    int32_t getOperateRate() const { return (int32_t)mOperateRateTuning->value; }
    int32_t getRealTimePriority() const { return (int32_t)mRealTimePriorityTuning->value; }

    /* generate filter param */
    void addFilterParamSetter(C2Param &c2param, FilterParamSetterFunc func) {
        std::list<uint32_t> deps;
        deps.clear();

        FilterParamSetterInfo setterInfo = { func, deps };

        mFilterParamSetters.insert(std::make_pair(c2param.index(), setterInfo));
    }

    void addFilterParamSetter(C2Param &c2param,
                              FilterParamSetterFunc func, std::list<std::shared_ptr<C2Param>> deps) {
        addFilterParamSetter(c2param, func);

        /* update dependency */
        if (!deps.empty()) {
            for (auto &d : deps) {
                auto setter = mFilterParamSetters.find(d->index());
                if (setter != mFilterParamSetters.end()) {
                    StaticExynosLog(Level::Debug, "CommonParamIntf", "[%s] reigster dependency(0x%x) to param(0x%x)",
                                        __FUNCTION__, c2param.index(), d->index());
                    setter->second.deps.push_back(c2param.index());
                }
            }
        }
    }

    FilterParamSetterRet cnvC2ParamToFilterParam(C2Param &c2param,
                                                 std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet param;
        param.clear();

        auto setter = mFilterParamSetters.find(c2param.index());
        if (setter == mFilterParamSetters.end()) {
            /* can't find setter */
            param.clear();
            return param;
        }

        if (setter->second.func != nullptr) {
            param = setter->second.func(intf);
        }

        /* update dependencies */
        if (!(setter->second.deps.empty())) {
            for (auto &d : setter->second.deps) {
                auto dSetter = mFilterParamSetters.find(d);

                if (dSetter != mFilterParamSetters.end()) {
                    StaticExynosLog(Level::Debug, "CommonParamIntf", "[%s] update dependency(0x%x) by changes(0x%x)",
                                        __FUNCTION__, d, c2param.index());

                    if (dSetter->second.func != nullptr) {
                        auto ret = dSetter->second.func(intf);

                        if (!ret.empty()) {
                            std::copy(ret.begin(), ret.end(), std::inserter(param, param.end()));
                        }
                    }
                }
            }
        }

        return param;
    }

    bool keepFilterParam(FilterParamSetterRet param) {
        if (param.empty()) {
            /* invalid parameter */
            return false;
        }

        ExynosMutex<std::shared_ptr<ExynosFilterParams>>::LockObj filterParams(mFilterParams);

        if ((*filterParams).get() != nullptr) {
            return (*filterParams)->addParam(param);
        }

        return false;
    }

    std::shared_ptr<ExynosFilterParams> getPiledFilterParams() {
        std::shared_ptr<ExynosFilterParams> ret = std::make_shared<ExynosFilterParams>();

        ExynosMutex<std::shared_ptr<ExynosFilterParams>>::LockObj filterParams(mFilterParams);

        if ((*filterParams).get() != nullptr) {
            (*ret) = (*(*filterParams));
            (*filterParams)->clear();
        }

        return ret;
    }

protected:
    ExynosC2FilterManager::FilterModuleInfoList mListFilterInfo;
    std::map<uint32_t, FilterParamSetterInfo> mFilterParamSetters;

private:
    ExynosMutex<std::shared_ptr<ExynosFilterParams>> mFilterParams;
};

class ExynosC2ComponentInterface : public C2ComponentInterface , public ExynosLog {
public:
    ExynosC2ComponentInterface(const char *name, c2_node_id_t id,
                               const std::shared_ptr<C2InterfaceHelper> &impl) : ExynosLog("ExynosC2ComponentInterface"),
                                                                                 mName(name),
                                                                                 mId(id),
                                                                                 mImpl(impl) {
    }

    ~ExynosC2ComponentInterface() override = default;

    /* override function on C2ComponentInteface */
    C2String getName() const override {
        return mName;
    }

    c2_node_id_t getId() const override {
        return mId;
    }

    c2_status_t query_vb(const std::vector<C2Param*> &stackParams,
                         const std::vector<C2Param::Index> &heapParamIndices,
                         c2_blocking_t mayBlock,
                         std::vector<std::unique_ptr<C2Param>> * const heapParams) const override {
        ExynosLogFunctionTrace();

        if (mImpl.get() == nullptr) {
            return C2_CANNOT_DO;
        }

        return mImpl->query(stackParams, heapParamIndices, mayBlock, heapParams);
    }

    c2_status_t config_vb(const std::vector<C2Param*> &params,
                          c2_blocking_t mayBlock,
                          std::vector<std::unique_ptr<C2SettingResult>> * const failures) override {
        ExynosLogFunctionTrace();

        if (mImpl.get() == nullptr) {
            return C2_CANNOT_DO;
        }

        /* apply c2param and save filterparam corresponding each c2param */
        c2_status_t ret = C2_OK;

        auto err = mImpl->config(params, mayBlock, failures);
        if (err != C2_OK) {
            return err;
        }

        std::vector<C2Param::Index> heapParamIndices;
        for (auto it = params.begin(); it != params.end(); it++) {
            C2Param * const c2param = (*it);
            if (c2param != nullptr) {
                heapParamIndices.push_back(c2param->index());
            }
        }

        std::vector<std::unique_ptr<C2Param>> queryParams;
        err = mImpl->query({}, heapParamIndices, C2_MAY_BLOCK, &queryParams);

        if ((err == C2_OK) && (!queryParams.empty())) {
            C2InterfaceHelper::Lock lock = mImpl->lock();
            auto paramIntf = std::static_pointer_cast<ExynosC2Component::CommonParamIntf>(mImpl);

            for (auto it = queryParams.begin(); it != queryParams.end(); it++) {
                C2Param * const c2param = (*it).get();

                if (c2param != nullptr) {
                    auto param = paramIntf->cnvC2ParamToFilterParam(*c2param, paramIntf);
                    if (!param.empty()) {
                        for (std::shared_ptr<ExynosFilterParamBase> &element : param) {
                            ExynosLogD("[%s] add filter param(index:%x, name:%s)", __FUNCTION__,
                                            element->index(), element->name().c_str());
                        }

                        paramIntf->keepFilterParam(param);
                    }
                }
            }
        }

        return ret;
    }

    c2_status_t createTunnel_sm(c2_node_id_t) override {
        return C2_OMITTED;
    }

    c2_status_t releaseTunnel_sm(c2_node_id_t) override {
        return C2_OMITTED;
    }

    c2_status_t querySupportedParams_nb(std::vector<std::shared_ptr<C2ParamDescriptor>> * const params) const override {
        ExynosLogFunctionTrace();

        if (mImpl.get() == nullptr) {
            return C2_CANNOT_DO;
        }

        return mImpl->querySupportedParams(params);
    }

    c2_status_t querySupportedValues_vb(std::vector<C2FieldSupportedValuesQuery> &fields,
                                        c2_blocking_t mayBlock) const override {
        ExynosLogFunctionTrace();

        if (mImpl.get() == nullptr) {
            return C2_CANNOT_DO;
        }

        return mImpl->querySupportedValues(fields, mayBlock);
    }

private:
    C2String mName;
    const c2_node_id_t mId;
    const std::shared_ptr<C2InterfaceHelper> mImpl;

    /* disable default constructors */
    ExynosC2ComponentInterface() = delete;
};

template<typename T, typename U>
class ExynosC2ComponentWrapper : public C2Component, public ExynosLog {
public:
    ExynosC2ComponentWrapper(c2_node_id_t id, const std::shared_ptr<U> &intfImpl) : ExynosLog("ExynosC2ComponentWrapper") {
        ExynosLogFunctionTrace();

        mExynosC2Component = std::make_shared<T>(id, intfImpl);
    }

    ~ExynosC2ComponentWrapper() {
        ExynosLogFunctionTrace();

        mExynosC2Component->release();

        mExynosC2Component.reset();
    }

    /* override function on C2Component */
    c2_status_t setListener_vb(const std::shared_ptr<Listener> &listener, c2_blocking_t mayBlock) override {
        ExynosLogFunctionTrace();
        return mExynosC2Component->setListener_vb(listener, mayBlock);
    }

    c2_status_t queue_nb(std::list<std::unique_ptr<C2Work>> * const items) override {
        ExynosLogFunctionTrace();
        return mExynosC2Component->queue_nb(items);
    }

    c2_status_t announce_nb(const std::vector<C2WorkOutline> &items) {
        ExynosLogFunctionTrace();
        return mExynosC2Component->announce_nb(items);
    }

    c2_status_t flush_sm(flush_mode_t mode, std::list<std::unique_ptr<C2Work>> * const flushedWork) {
        ExynosLogFunctionTrace();
        return mExynosC2Component->flush_sm(mode, flushedWork);
    }

    c2_status_t drain_nb(drain_mode_t mode) {
        ExynosLogFunctionTrace();
        return mExynosC2Component->drain_nb(mode);
    }

    c2_status_t start() {
        ExynosLogFunctionTrace();
        return mExynosC2Component->start();
    }

    c2_status_t stop() {
        ExynosLogFunctionTrace();
        return mExynosC2Component->stop();
    }

    c2_status_t reset() {
        ExynosLogFunctionTrace();
        return mExynosC2Component->reset();
    }

    c2_status_t release() {
        ExynosLogFunctionTrace();
        return mExynosC2Component->release();
    }
    std::shared_ptr<C2ComponentInterface> intf() {
        ExynosLogFunctionTrace();
        return mExynosC2Component->intf();
    }

    /* add function for ExynosC2Component */
    bool init(std::weak_ptr<C2Component> wkComponent) {
        return mExynosC2Component->init(wkComponent);
    }

private:
    std::shared_ptr<T> mExynosC2Component = nullptr;

    ExynosC2ComponentWrapper() = delete;
};

#undef LOG_ON

#endif // EXYNOS_C2_COMPONENT_H
