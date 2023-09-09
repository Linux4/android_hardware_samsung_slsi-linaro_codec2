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
#ifndef EXYNOS_BUFFER_ALLOCATOR_H
#define EXYNOS_BUFFER_ALLOCATOR_H

#include <C2PlatformSupport.h>
#include <C2Component.h>
#include <C2Buffer.h>
#include <C2BufferPriv.h>
#include <memory>
#include <variant>
#include <utility>

#include "ExynosDef.h"
#include "ExynosBuffer.h"
#include "ExynosETC.h"

#define LOG_ON
#include "ExynosLog.h"


class BufferCount {
public:
    BufferCount() {
        count = 0;
    }

    ~BufferCount() = default;

    int get() {
        return count;
    }

    int inc() {
        std::lock_guard<std::mutex> lock(mutex);

        return ++count;
    }

    int dec() {
        std::lock_guard<std::mutex> lock(mutex);

        return --count;
    }

private:
    std::mutex mutex;
    int count;
};

class ExynosBufferAllocator : public ExynosLog {
public:
    ~ExynosBufferAllocator() = default;

    BufferAllocRetType alloc(AllocArg &argument);
    void free(std::shared_ptr<ExynosBuffer> buffer);
    int getAllocCount() {
        return (mBufferCount.get() != nullptr)? mBufferCount->get():0;
    }

    static std::optional<std::shared_ptr<ExynosBuffer>> importC2Buffer(std::shared_ptr<C2Buffer> c2buffer);
    static std::optional<std::shared_ptr<C2Buffer>> exportC2Buffer(std::shared_ptr<ExynosBuffer> buffer);
    static C2BlockPool::local_id_t getBlockPoolID(android::C2PlatformAllocatorStore::id_t allocStoreID);
    static android::C2PlatformAllocatorStore::id_t getAllocatorID(android::C2PlatformAllocatorStore::id_t allocStoreID);
    static std::shared_ptr<ExynosBufferAllocator> makeAllocator(std::shared_ptr<const C2Component> component, android::C2PlatformAllocatorStore::id_t allocStoreID, C2BlockPool::local_id_t poolID, C2MemoryUsage usage, std::shared_ptr<C2BlockPool> *blockPool);

private:
    ExynosBufferAllocator(std::shared_ptr<C2BlockPool> blockPool, android::C2PlatformAllocatorStore::id_t allocStoreID, C2MemoryUsage usage);

    std::weak_ptr<C2BlockPool>                  mBlockPool;
    android::C2PlatformAllocatorStore::id_t     mAllocStoreID;
    C2MemoryUsage                               mUsage;

    std::shared_ptr<BufferCount> mBufferCount;

    /* disable default constructor */
    ExynosBufferAllocator() = delete;
};

inline std::optional<std::pair<std::shared_ptr<ExynosBuffer>, std::shared_ptr<C2Buffer>>> allocatBuffer(
                                                std::shared_ptr<ExynosBufferAllocator> allocator, AllocArg arg) {
    if (allocator.get() == nullptr) {
        StaticExynosLog(Level::Error, "", "[%s] allocator() is invalid", __FUNCTION__);
        return std::nullopt;
    }

    /* this buffer will be freed by default destructor */
    auto ret = allocator->alloc(arg);

    if ((ret.first != EXYNOS_ERROR_NONE) ||
        (ret.second.get() == nullptr)) {
        StaticExynosLog(Level::Error, "", "[%s] alloc() is failed", __FUNCTION__);
        return std::nullopt;
    }

    auto optC2Buffer = ExynosBufferAllocator::exportC2Buffer(ret.second);

    if (!optC2Buffer) {
        StaticExynosLog(Level::Error, "", "[%s] exportC2Buffer() is failed", __FUNCTION__);
        allocator->free(ret.second);
        return std::nullopt;
    }

    return { std::make_pair(ret.second, *optC2Buffer) };
}

#undef LOG_ON

#endif // EXYNOS_BUFFER_ALLOCATOR_H
