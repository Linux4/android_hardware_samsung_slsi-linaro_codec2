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
#ifndef EXYNOS_FILTER_MANAGER_H
#define EXYNOS_FILTER_MANAGER_H

#include <mutex>
#include <list>
#include <memory>
#include <string>
#include <functional>
#include <map>
#include <utility>

#include "ExynosBuffer.h"
#include "ExynosDef.h"
#include "ExynosBufferAllocator.h"
#include "ExynosETC.h"
#include "Exynos_Filter.h"

// #define FILTER_MANAGER_LOG_ON
#include "ExynosLog.h"

#ifdef FILTER_MANAGER_LOG_ON
#define LOG_ONOFF false
#else
#define LOG_ONOFF true
#endif

class ExynosSelectorFilter : public ExynosFilter, ExynosLog, std::enable_shared_from_this<ExynosSelectorFilter> {
public:
    ExynosSelectorFilter(std::shared_ptr<ExynosFilter> defaultFilter, std::shared_ptr<ExynosFilter> subFilter) {
        mWorkerList.clear();
        mWorkerList.insert({"defaultFilter", defaultFilter});
        mWorkerList.insert({"subFilter", subFilter});

        mWorker = defaultFilter;
    }

    bool changeWorker(std::string key) {
        if (mWorkerList.find(key) == mWorkerList.end()) {
            return false;
        }

        mWorker = mWorkerList.find(key)->second;

        return true;
    }

    ~ExynosSelectorFilter() = default;

    bool setNext(std::shared_ptr<ExynosFilter> nextFilter) override {
        ExynosLogFunctionTrace();

        if (!CHECK_SHARED_PTR(nextFilter)) {
            return false;
        }

        bool ret = true;

        for (auto element = mWorkerList.begin(); element != mWorkerList.end(); element++) {
            ret = element->second->setNext(nextFilter);
            if (ret == false) {
                break;
            }
        }

        return ret;
    }

    bool setCallback(std::shared_ptr<FilterListener> listener) override {
        bool ret = true;

        for (auto element = mWorkerList.begin(); element != mWorkerList.end(); element++) {
            ret = element->second->setCallback(listener);
            if (ret == false) {
                break;
            }
        }

        return ret;
    }

    bool setAllocator(std::shared_ptr<BufferAllocFnType> allocfunc) override {
        bool ret = true;

        for (auto element = mWorkerList.begin(); element != mWorkerList.end(); element++) {
            ret = element->second->setAllocator(allocfunc);
            if (ret == false) {
                break;
            }
        }

        return ret;
    }

    bool setAllocMode(AllocMode mode) override {
        bool ret = true;

        for (auto element = mWorkerList.begin(); element != mWorkerList.end(); element++) {
            ret = element->second->setAllocMode(mode);
            if (ret == false) {
                break;
            }
        }

        return ret;
    }

    bool queueWork(std::unique_ptr<FilterWork> work) override {
        return mWorker->queueWork(std::move(work));
    }

    /* function for real time resource */
    bool checkRealTimeResource(int32_t width, int32_t height, int32_t operatingRate) override {
        return mWorker->checkRealTimeResource(width, height, operatingRate);
    }

    bool start() override {
        return mWorker->start();
    }

    bool stop() override {
        return mWorker->stop();
    }

    bool flush() override {
        return mWorker->flush();
    }

    bool reset() override {
        return mWorker->reset();
    }

    bool release() override {
        return mWorker->release();
    }

private:
    std::shared_ptr<ExynosFilter> mWorker;
    std::map<std::string, std::shared_ptr<ExynosFilter>> mWorkerList;
    std::shared_ptr<ExynosFilter::FilterListener> mSelectorListener;

    ExynosSelectorFilter() = delete;
};

template<class T, class R>
class ExynosExtentionFilter : public ExynosSelectorFilter {
public:
    ExynosExtentionFilter(uint32_t id, bool secure) : ExynosSelectorFilter(std::static_pointer_cast<ExynosFilter>(std::make_shared<T>(id, secure)),
                                                                           std::static_pointer_cast<ExynosFilter>(std::make_shared<R>(id, secure))) {
    }

    ~ExynosExtentionFilter() = default;

private:
    ExynosExtentionFilter() = delete;
};

class ExynosC2FilterManager : public ExynosLog {
public:
    class FilterModule {
    public:
        FilterModule(std::string name, std::shared_ptr<ExynosFilter> filter,
                     C2PlatformAllocatorStore::id_t allocStoreID,
                     C2MemoryUsage usage)
            : mName(name), mFilter(filter), mAllocStoreID(allocStoreID), mUsage(usage) {
        }

        ~FilterModule() {
            mFilter.reset();
            mOutputBlockPool.reset();
            mBufferAllocator.reset();
            mAllocFn.reset();
        }

        template<typename T>
        static std::shared_ptr<FilterModule> makeFilterModule(uint32_t id, std::string name,
                                                              C2PlatformAllocatorStore::id_t allocStoreID,
                                                              C2MemoryUsage usage) {
            bool secure = (name.find(".secure") != std::string::npos)? true:false;

            auto filter = std::static_pointer_cast<ExynosFilter>(std::make_shared<ExynosFilterWrapper<T>>(id, secure));

            return std::make_shared<FilterModule>(name, filter, allocStoreID, usage);
        }

    private:
        friend class ExynosC2FilterManager;

        std::string mName;

        std::shared_ptr<ExynosFilter> mFilter;

        C2PlatformAllocatorStore::id_t          mAllocStoreID;
        C2MemoryUsage                           mUsage;
        std::shared_ptr<C2BlockPool>            mOutputBlockPool;
        std::shared_ptr<ExynosBufferAllocator>  mBufferAllocator;
        std::shared_ptr<BufferAllocFnType>      mAllocFn;

        /* disable default constructor */
        FilterModule() = delete;
    };

    typedef struct FilterModuleInfo {
        std::string name;
        std::function<std::shared_ptr<FilterModule>(uint32_t, std::string, C2PlatformAllocatorStore::id_t, C2MemoryUsage)> createFilterModule;
        C2PlatformAllocatorStore::id_t allocStoreID;
        C2MemoryUsage                  usage;
    } FilterModuleInfo;

    typedef std::list<FilterModuleInfo> FilterModuleInfoList;

    ExynosC2FilterManager(std::string name, std::weak_ptr<C2Component> c2component) : ExynosLog(name + "-FilterManager") {
        mbLogOff = LOG_ONOFF;
        mC2Component = c2component;
        mFilterListInfo.reset();
    }

    ~ExynosC2FilterManager() {
        mFilterModules.clear();
        mFilterListener.reset();
        mC2Component.reset();
        mFilterListInfo.reset();
    }

    std::shared_ptr<ExynosFilter> loadFilterModules(FilterModuleInfoList info) {
        ExynosLogFunctionTrace();
        std::unique_lock<std::mutex> lock(mMutex);

        mFilterListInfo = std::make_shared<std::map<std::string, int>>();

        for (auto it = info.begin(); it != info.end(); it++) {
            if (it->createFilterModule == nullptr) {
                ExynosLogE("[%s] createFilterModule is nullptr", __FUNCTION__);
                mFilterModules.clear();
                return nullptr;
            }

            int filterID = mFilterModules.size() + FIRST_FILTER_ID;  /* ID(0) is reserved for owner component */

            auto module = it->createFilterModule(filterID,  /* ID(0) is reserved for owner component */
                                                 it->name, it->allocStoreID, it->usage);
            if ((module.get() == nullptr) ||
                (module->mFilter.get() == nullptr)) {
                ExynosLogE("[%s] createFilterModule(%s, %x) is failed", __FUNCTION__, it->name.c_str(), it->allocStoreID);
                mFilterModules.clear();
                return nullptr;
            }
            mFilterListInfo->emplace(it->name, filterID);

            /* connect filters */
            if (!mFilterModules.empty()) {
                /* set a source filter */
                auto srcFilter = mFilterModules.back()->mFilter;

                if (!srcFilter->setNext(module->mFilter)) {
                    ExynosLogE("[%s] setNext() is failed", __FUNCTION__);
                    mFilterModules.clear();
                    return nullptr;
                }
            }

            if (mFilterListener.get() != nullptr) {
                if (!module->mFilter->setCallback(mFilterListener)) {
                    ExynosLogE("[%s] setCallback() is failed", __FUNCTION__);
                    mFilterModules.clear();
                    return nullptr;
                }
            }

            mFilterModules.push_back(module);
        }

        return (mFilterModules.empty()? nullptr: mFilterModules.front()->mFilter);
    }

    void setCallback(std::shared_ptr<ExynosFilter::FilterListener> listener) {
        ExynosLogFunctionTrace();
        std::unique_lock<std::mutex> lock(mMutex);

        if (!mFilterModules.empty()) {
            auto filter = mFilterModules.back()->mFilter;

            if (!filter->setCallback(listener)) {
                ExynosLogE("[%s] setCallback() is failed", __FUNCTION__);
                mFilterModules.clear();
                return;
            }
        }

        mFilterListener = listener;
    }

    bool setBlockPool(C2BlockPool::local_id_t poolID, uint64_t usage = 0u) {
        ExynosLogFunctionTrace();
        std::unique_lock<std::mutex> lock(mMutex);

        if (mC2Component.expired()) {
            ExynosLogT("[%s] obj is released", __FUNCTION__);
            return false;
        }

        std::shared_ptr<C2Component> component = mC2Component.lock();

        if (component.get() == nullptr) {
            ExynosLogE("[%s] invalid parameter", __FUNCTION__);
            return false;
        }

        for (auto &element : mFilterModules) {
            if (element->mFilter.get() == nullptr) {
                ExynosLogE("[%s] filter is invalid", __FUNCTION__);
                element->mBufferAllocator.reset();
                element->mOutputBlockPool.reset();
                element->mAllocFn.reset();
                return false;
            }

            if (element->mOutputBlockPool.get() != nullptr) {
                /* pool is already allocated. skip */
                continue;
            }

            if (element->mBufferAllocator.get() != nullptr) {
                element->mBufferAllocator.reset();
            }

            if (element->mAllocFn.get() != nullptr) {
                element->mAllocFn.reset();
            }

            /* create a buffer allocator & blockPool */
            std::shared_ptr<C2BlockPool> blockPool = nullptr;
            auto allocator = ExynosBufferAllocator::makeAllocator(
                                    component, element->mAllocStoreID,
                                    poolID,
                                    C2MemoryUsage(usage, element->mUsage.expected), &blockPool);

            element->mBufferAllocator = allocator;
            element->mOutputBlockPool = blockPool;

            std::weak_ptr<ExynosBufferAllocator> wkAllocator = element->mBufferAllocator;

            std::shared_ptr<BufferAllocFnType> allocFunc = std::make_shared<BufferAllocFnType>(
                [wkAllocator](AllocArg &arg)->BufferAllocRetType {
                    if (!wkAllocator.expired()) {
                        auto shAllocator = wkAllocator.lock();

                        if (shAllocator.get() != nullptr) {
                            StaticExynosLog(Level::Debug, "ExynosBufferAllocator", "[allocFunc] limit:(%d), alloc count:(%d)",
                                                arg.limit, shAllocator->getAllocCount());

                            return shAllocator->alloc(arg);
                        }
                    }

                    return std::make_pair(EXYNOS_ERROR_BAD_STATE, nullptr);
                });

            if (!element->mFilter->setAllocator(allocFunc)) {
                ExynosLogE("[%s] %s filter : setAllocator() is failed", __FUNCTION__, element->mName.c_str());
                return false;
            }

            element->mAllocFn = allocFunc;

            auto mode = (element->mAllocStoreID == C2PlatformAllocatorStore::BUFFERQUEUE)?
                                ExynosFilter::AllocMode::PreferResources:ExynosFilter::AllocMode::PreferPerformance;
            if (!element->mFilter->setAllocMode(mode)) {
                ExynosLogE("[%s] %s filter : setAllocMode() is failed", __FUNCTION__, element->mName.c_str());
                return false;
            }
        }

        return true;
    }

    void clearBlockPool() {
        ExynosLogFunctionTrace();
        std::lock_guard<std::mutex> lock(mMutex);

        for (auto &element : mFilterModules) {
            std::shared_ptr<BufferAllocFnType> nullFunc = nullptr;

            if (element->mFilter.get() != nullptr) {
                element->mFilter->setAllocator(nullFunc);
            }

            element->mBufferAllocator.reset();
            element->mOutputBlockPool.reset();
            element->mAllocFn.reset();
        }
    }

    std::shared_ptr<ExynosFilter> getFilter() {
        std::lock_guard<std::mutex> lock(mMutex);

        if (!mFilterModules.empty()) {
            return mFilterModules.front()->mFilter;
        }

        return nullptr;
    }

    std::shared_ptr<ExynosFilter> getFilter(std::string name) {
        std::lock_guard<std::mutex> lock(mMutex);

        if (!mFilterModules.empty()) {
            for (auto it = mFilterModules.begin(); it != mFilterModules.end(); it++) {
                if (((*it)->mName).find(name) != std::string::npos) {
                    return (*it)->mFilter;
                }
            }
        }

        return nullptr;
    }

    std::weak_ptr<std::map<std::string, int>> getFilterListInfo() {
        return mFilterListInfo;
    }

    void changeToSubFilter(std::string name) {
        std::lock_guard<std::mutex> lock(mMutex);

        for (auto &element : mFilterModules) {
            if (element->mName.find(name) != std::string::npos) {
                auto eleFilter = static_pointer_cast<ExynosFilterWrapper<ExynosFilter>>(element->mFilter);
                auto coreFilter = eleFilter->getCoreFilter();
                auto selectorFilter = std::static_pointer_cast<ExynosSelectorFilter>(coreFilter);
                selectorFilter->changeWorker("subFilter");
            }
        }
    }

private:
    std::mutex mMutex;

    std::list<std::shared_ptr<FilterModule>>        mFilterModules;
    std::shared_ptr<ExynosFilter::FilterListener>   mFilterListener;
    std::weak_ptr<C2Component>                      mC2Component;

    std::shared_ptr<std::map<std::string, int>>     mFilterListInfo;

    /* disable default constructor */
    ExynosC2FilterManager() = delete;
};

#undef FILTER_MANAGER_LOG_ON
#undef LOG_ON

#endif // EXYNOS_FILTER_MANAGER_H
