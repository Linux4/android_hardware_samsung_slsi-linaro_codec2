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
#ifndef EXYNOS_FILTER_H
#define EXYNOS_FILTER_H

#include <list>
#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <utility>

#include "ExynosQueue.h"
#include "ExynosThreadPool.h"
#include "ExynosBuffer.h"
#include "ExynosDef.h"
#include "ExynosFilterParam.h"
#include "ExynosListener.h"
#include "ExynosETC.h"

#define LOG_ON
#include "ExynosLog.h"

class ExynosFilter {
public:
    enum class AllocMode : uint32_t {
        PreferPerformance,
        PreferResources,
    };

    virtual ~ExynosFilter() = default;

    class FilterWork {
    public:
        FilterWork() : inputIndex(0) {
            buffers.clear();
        }

        ~FilterWork() {
            buffers.clear();
        }

        std::list<std::shared_ptr<ExynosBuffer>> buffers;

        int inputIndex = 0;  /* an index of buffer which should be used */
        bool isDrain = false;
    };

    class FilterListener : public ExynosLog {
    public:
        static std::shared_ptr<FilterListener> makeListener(std::function<bool(std::unique_ptr<FilterWork>)> callback,
                                                            std::string name = "FilterListener") {
#if 0
            auto listener = std::make_shared<FilterListener>(FilterListener(owner));
#else
            auto delfunc = [](FilterListener *p) {
                               delete p;
                           };

            auto listener = std::shared_ptr<FilterListener>(new FilterListener(std::move(callback), name),
                                                            std::move(delfunc));
#endif
            return listener;
        }

        ~FilterListener() = default;

        bool workDone(std::unique_ptr<FilterWork> work) {
            ExynosLogFunctionTrace();

            std::lock_guard<std::mutex> lock(mMutex);

            if ((work.get() != nullptr) &&
                (mCallback != nullptr)) {
                return mCallback(std::move(work));
            }

            return true;
        }

    private:
        FilterListener(std::function<bool(std::unique_ptr<FilterWork>)> callback,
                       std::string name) : ExynosLog(name + "-FilterListener"),
                                           mCallback(std::move(callback)) {
        }

        std::mutex mMutex;
        std::function<bool(std::unique_ptr<FilterWork>)> const mCallback;

        /* disable default constructor */
        FilterListener() = delete;
    };

    /* add function for ExynosFilter */
    virtual bool setNext(std::shared_ptr<ExynosFilter> nextFilter) = 0;
    virtual bool setCallback(std::shared_ptr<FilterListener> listener) = 0;
    virtual bool setAllocator(std::shared_ptr<BufferAllocFnType> allocfunc) = 0;
    virtual bool setAllocMode(AllocMode mode) = 0;
    virtual bool queueWork(std::unique_ptr<FilterWork> work) = 0;

    /* function for real time resource */
    virtual bool checkRealTimeResource(int32_t width, int32_t height, int32_t operatingRate) {
        UNUSED(width);
        UNUSED(height);
        UNUSED(operatingRate);

        /* TODO */

        return true;
    }

    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual bool flush() = 0;
    virtual bool reset() = 0;
    virtual bool release() = 0;
};

class ExynosFilterBase : public ExynosFilter,
                         public ExynosListenerInterface,
                         public ExynosLog,
                         public std::enable_shared_from_this<ExynosFilterBase> {
public:
    ExynosFilterBase(uint32_t id, bool isSecure = false) : ExynosLog("ExynosFilter"),
                     mThreadPool(std::make_shared<ExynosThreadPool>(1, mObjName)),
                     mID(id), mIsSecure(isSecure), mAllocMode(AllocMode::PreferPerformance) {
        ExynosLogFunctionTrace();
        mPendingParams.reset();
        mDoResult.clear();
        mQuitWork = false;
        mDebug = EXYNOS_DEBUG_NONE;
    }

    virtual ~ExynosFilterBase() {
        ExynosLogFunctionTrace();

        mThreadPool.reset();
        mFilterListener.reset();
        mWorkInfos.clear();
        mPendingParams.reset();
        mDoResult.clear();
    }

    /* override function on ExynosFilter */
    bool setNext(std::shared_ptr<ExynosFilter> nextFilter) override;
    bool setCallback(std::shared_ptr<FilterListener> listener) override;
    bool setAllocator(std::shared_ptr<BufferAllocFnType> allocfunc) override;
    bool setAllocMode(AllocMode mode) override {
        mAllocMode = mode;
        return true;
    };
    bool queueWork(std::unique_ptr<FilterWork> work) override;
    bool start() override;
    bool stop() override;
    bool flush() override;
    bool reset() override;
    bool release() override;

protected:
    class FilterWorkInfo {
    public:
        FilterWorkInfo(int in, int out, std::unique_ptr<FilterWork> obj) : inDataNum(in),
                                                                           outDataNum(out),
                                                                           work(std::move(obj)) {
        }

        ~FilterWorkInfo() = default;

        int inDataNum = 0;  /* a number of input buffers which should be processed */
        int outDataNum = 0; /* a number of output buffers processed
                             * it will be a number of input buffers in the next filter
                             */

        std::unique_ptr<FilterWork> work;
    };

    /* it will be implemented by ExynosFilter's child class function on ExynosListerInterface */
    virtual bool processDone(ExynosBufferInfo input, ExynosBufferInfo output) override;
    virtual void eventReceived(std::shared_ptr<ExynosListenerEvent> event) override;

    /* function for thread pool owned by self. it will be implemented by ExynosFilter's child class */
    virtual bool doStart() = 0;
    virtual bool doStop() = 0;
    virtual bool doFlush() = 0;
    virtual bool doReset() = 0;
    virtual bool doProcess(std::shared_ptr<ExynosBuffer> buffer) = 0;
    virtual bool doWorkDone(std::unique_ptr<FilterWork> work);
    virtual bool forceExit() {
        mQuitWork = true;
        return true;
    }
    virtual bool forceExitDone() {
        mQuitWork = false;
        return true;
    }

    /* add function for ExynosFilter */
    std::shared_ptr<FilterWorkInfo> obtainWorkInfo(std::shared_ptr<ExynosBuffer> inbuffer,
                                                   std::shared_ptr<ExynosBuffer> outbuffer = nullptr,
                                                   bool internalRelease = false);
    void reuseWorkInfo(std::shared_ptr<FilterWorkInfo> workInfo);
    bool workDone(std::unique_ptr<FilterWork> work);
    bool bypassBuffer(std::shared_ptr<ExynosBuffer> buffer);
    std::optional<std::shared_ptr<ExynosBuffer>> allocBuffer(AllocArg arg);  /* blocking */

    std::shared_ptr<ExynosThreadPool> mThreadPool;
    std::shared_ptr<ExynosListener> mListener;  /* owns listener */
    std::weak_ptr<BufferAllocFnType> mFnBufferAlloc;
    std::vector<std::future<bool>> mDoResult;  /* TODO : ret value handling */

    uint32_t    mID;  /* identity */
    bool        mIsSecure;
    bool        mQuitWork;
    AllocMode   mAllocMode;

    ExynosDebugType mDebug;

private:
    /* function for thread pool owned by self */
    bool doQueueWork(std::shared_ptr<std::unique_ptr<FilterWork>> shWork);

    std::mutex mMutex;
    std::shared_ptr<FilterListener> mFilterListener;  /* owns listener to communicate with other filter */
    std::weak_ptr<ExynosFilter> mSrcFilter;
    std::weak_ptr<ExynosFilter> mNextFilter;
    std::weak_ptr<FilterListener> mNotify;

    std::mutex mWorkInfoMutex;
    ExynosQueue<std::shared_ptr<FilterWorkInfo>> mWorkInfos;

    std::mutex mPendingParamsMutex;
    std::shared_ptr<ExynosFilterParams> mPendingParams;
};

template<typename T>
class ExynosFilterWrapper : public ExynosFilter {
public:
    ExynosFilterWrapper(uint32_t id, bool isSecure = false) {
        mExynosFilter = std::make_shared<T>(id, isSecure);
    }

    ~ExynosFilterWrapper() {
        mExynosFilter.reset();
    }

    /* override function on ExynosFilter */
    bool setNext(std::shared_ptr<ExynosFilter> nextFilter) {
        return mExynosFilter->setNext(nextFilter);
    }

    bool setCallback(std::shared_ptr<ExynosFilter::FilterListener> listener) {
        return mExynosFilter->setCallback(listener);
    }

    bool setAllocator(std::shared_ptr<BufferAllocFnType> allocfunc) {
        return mExynosFilter->setAllocator(allocfunc);
    }

    bool setAllocMode(AllocMode mode) {
        return mExynosFilter->setAllocMode(mode);
    }

    bool queueWork(std::unique_ptr<ExynosFilter::FilterWork> work) {
        return mExynosFilter->queueWork(std::move(work));
    }

    bool start() {
        return mExynosFilter->start();
    }

    bool stop() {
        return mExynosFilter->stop();
    }

    bool reset() {
        return mExynosFilter->reset();
    }

    bool flush() {
        return mExynosFilter->flush();
    }

    bool release() {
        return mExynosFilter->release();
    }

    std::shared_ptr<ExynosFilter> getCoreFilter() {
        return mExynosFilter;
    }

private:
    std::shared_ptr<T> mExynosFilter = nullptr;
};

#endif // EXYNOS_FILTER_H
