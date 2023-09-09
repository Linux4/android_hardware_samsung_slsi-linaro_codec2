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
#include <unistd.h>
#include <linux/ion.h>
#include <ion/ion.h>
#include <string>
#include <fcntl.h>
#include <algorithm>

#include "ExynosIONUtils.h"
#include "ExynosETC.h"

#include "hardware/exynos/ion.h"

#define LOG_ON
#include "ExynosLog.h"
#define LOG_TAG "ExynosIONUtils"

///////////////////////////////////////////////////////////////////////////////
/* graphics/base/libion/ion.c */
// #define MAX_HEAP_NAME 32

/* graphics/base/libion/ion_uapi.h */
struct ion_heap_data {
    char name[ExynosIONUtils::MAX_HEAP_NAME];
    __u32 type;
    __u32 heap_id;
    __u32 size;       /* reserved 0 */
    __u32 heap_flags; /* reserved 1 */
    __u32 reserved2;
};

struct ion_heap_query {
    __u32 cnt;
    __u32 reserved0;
    __u64 heaps;
    __u32 reserved1;
    __u32 reserved2;
};
///////////////////////////////////////////////////////////////////////////////

#define ION_EXYNOS_HEAP_NAME_SYSTEM "ion_system_heap"
#define ION_EXYNOS_HEAP_NAME_VIDEO_STREAM "vstream_heap"  /* ion */
#define DMA_EXYNOS_HEAP_NAME_VIDEO_STREAM_SECURE "system-secure-vstream-secure"  /* dmaheap */
#define DMA_EXYNOS_HEAP_NAME_SYSTEM "system"
#define DMA_EXYNOS_HEAP_NAME_SYSTEM_UNCACHED "system-uncached"

static uint32_t getHeapMask(const char *heapName) {
    uint32_t ret = 0;

    if (heapName == nullptr) {
        StaticExynosLog(Level::Error, LOG_TAG, "[%s] heap name is invalid", __FUNCTION__);
        return ret;
    }

    int ionFd = ion_open();
    if (ionFd < 0) {
        StaticExynosLog(Level::Error, LOG_TAG, "[%s] ion_open() is failed", __FUNCTION__);
        return ret;
    }

    if (ion_is_legacy(ionFd)) {
        ion_close(ionFd);

        if (std::string(heapName) == std::string(ION_EXYNOS_HEAP_NAME_VIDEO_STREAM)) {
            return EXYNOS_ION_HEAP_VIDEO_STREAM_MASK;
        }

        return EXYNOS_ION_HEAP_SYSTEM_MASK;
    }

    int heapCnt = 0;
    if ((ion_query_heap_cnt(ionFd, &heapCnt) < 0) ||
        (heapCnt <= 0)) {
        StaticExynosLog(Level::Error, LOG_TAG, "[%s] ion_query_heap_cnt() is failed", __FUNCTION__);
        ion_close(ionFd);
        return ret;
    }

    auto ionHeapData = make_shared_array<struct ion_heap_data>(heapCnt);
    if (ionHeapData.get() == nullptr) {
        StaticExynosLog(Level::Error, LOG_TAG, "[%s] can not alloc ionHeapData", __FUNCTION__);
        ion_close(ionFd);
        return ret;
    }
    memset(ionHeapData.get(), 0, sizeof(struct ion_heap_data) * heapCnt);

    auto query = std::make_shared<struct ion_heap_query>();

    memset(query.get(), 0, sizeof(struct ion_heap_query));
    query.get()->cnt    = heapCnt;
    query.get()->heaps  = reinterpret_cast<__u64>(ionHeapData.get());

    if (ion_query_get_heaps(ionFd, heapCnt, ionHeapData.get()) < 0) {
        StaticExynosLog(Level::Error, LOG_TAG, "[%s] ion_query_get_heaps() is failed", __FUNCTION__);
        ion_close(ionFd);
        return ret;
    }

    for (int i = 0; i < heapCnt; i++) {
        auto heap = ionHeapData.get()[i];

        if (strncmp(heap.name, heapName, strlen(heapName)) == 0) {
            ret = (1 << heap.heap_id);
            StaticExynosLog(Level::Debug, LOG_TAG, "[%s] find heap(%s) : id(%d), mask(0x%x)", __FUNCTION__, heapName, heap.heap_id, ret);
            break;
        }
    }

    ion_close(ionFd);

    return ret;
}

C2R ExynosIONUtils::setIonUsage(C2InterfaceHelper::C2P<C2StoreIonUsageInfo> &me) {
    me.set().heapMask     = getHeapMask(ION_EXYNOS_HEAP_NAME_SYSTEM);
    me.set().allocFlags   = 0;
    me.set().minAlignment = 16;

    if ((me.v.usage == C2MemoryUsage::WRITE_PROTECTED) ||
        (me.v.usage == C2MemoryUsage::READ_PROTECTED)) {
        me.set().heapMask   = getHeapMask(ION_EXYNOS_HEAP_NAME_VIDEO_STREAM);
        me.set().allocFlags = (me.v.allocFlags | ION_FLAG_PROTECTED);
    }

    StaticExynosLog(Level::Debug, LOG_TAG, "[%s] usage(0x%x), capacity(%d), heapmask(0x%x), flags(0x%x), alignment(%d)", __FUNCTION__,
                        me.v.usage, me.v.capacity, me.v.heapMask, me.v.allocFlags, me.v.minAlignment);

    return C2R::Ok();
}

uint32_t ExynosIONUtils::getIonUsageMask() {
    uint32_t mask = ION_FLAG_CACHED |
                    ION_FLAG_CACHED_NEEDS_SYNC |
                    ION_FLAG_PROTECTED;
    return mask;
}

C2R ExynosIONUtils::setDmaUsage(C2InterfaceHelper::C2P<C2StoreDmaBufUsageInfo> &me) {
    const char *heapName = nullptr;
    int32_t allocFlags = 0;

    if (me.v.m.usage & (C2MemoryUsage::WRITE_PROTECTED | C2MemoryUsage::READ_PROTECTED)) {
        heapName    = DMA_EXYNOS_HEAP_NAME_VIDEO_STREAM_SECURE;
        allocFlags  = 0;
    } else if (me.v.m.usage & (C2MemoryUsage::CPU_READ | C2MemoryUsage::CPU_WRITE)) {
        heapName    = DMA_EXYNOS_HEAP_NAME_SYSTEM;
        allocFlags  = O_RDWR;
    } else {
        heapName    = DMA_EXYNOS_HEAP_NAME_SYSTEM_UNCACHED;
        allocFlags  = O_RDWR;
    }

    memset(me.set().m.heapName, 0, me.v.flexCount());
    strncpy(me.set().m.heapName, heapName, std::min(me.v.flexCount(), strlen(heapName)));
    me.set().m.allocFlags = allocFlags;

    StaticExynosLog(Level::Debug, LOG_TAG, "[%s] usage(0x%x), capacity(%d), heapName(%s), flags(0x%x)", __FUNCTION__,
                        me.v.m.usage, me.v.m.capacity, me.v.m.heapName, me.v.m.allocFlags);

    return C2R::Ok();
}

uint32_t ExynosIONUtils::getDmaUsageMask() {
    uint32_t mask = O_RDONLY |
                    O_WRONLY |
                    O_RDWR;
    return mask;
}

