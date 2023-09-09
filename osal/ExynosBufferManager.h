/*
 *
 * Copyright 2020 Samsung Electronics S.LSI Co. LTD
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
#ifndef EXYNOS_BUFFER_MANAGER_H
#define EXYNOS_BUFFER_MANAGER_H

#include <memory>
#include <mutex>
#include <atomic>
#include <map>
#include <sys/mman.h>

#include "ExynosQueue.h"
#include "ExynosMutex.h"
#include "ExynosDef.h"
#include "ExynosBuffer.h"

#define LOG_ON
#include "ExynosLog.h"

#define MAX_PLANE 3

typedef struct fds_t {
    int dupCount;
    int plane;
    int fd[MAX_PLANE];
    uint64_t key;
} fds_t;

class BufferFdManager {
public:
    BufferFdManager() {
    }

    ~BufferFdManager() {
        allFdClear();
    }

    fds_t getFixedFds(std::shared_ptr<ExynosBuffer> buffer);
    void returnFixedFds(uint64_t key);
    void returnFixedFds(std::shared_ptr<ExynosBuffer> buffer);
    void allFdClear();
    static uint64_t getStIno(int fd);

private:
    int fixedFd(int orgFd, int targetFd);

    std::map<uint64_t, fds_t> mFdMap;
};


class SharedPtrManager : public BufferFdManager {
public:
    SharedPtrManager() {
        mFixedFDEnable = false;
    }

    SharedPtrManager(bool bFixedFDEnable) {
        mFixedFDEnable = bFixedFDEnable;
    }

    virtual ~SharedPtrManager() {
        mQueue.clear();
    }

    virtual void* swapSharedPtrToPtr(std::shared_ptr<ExynosBuffer> shPtr, fds_t &fds);
    virtual void* swapSharedPtrToPtr(std::shared_ptr<ExynosBuffer> shPtr, fds_t &fds, ExynosParams params);
    virtual std::shared_ptr<ExynosBuffer> swapPtrToSharedPtr(void *ptr, bool bDelayReturn = false);
    virtual std::shared_ptr<ExynosBuffer> swapPtrToSharedPtr(void *ptr, ExynosParams &params, bool bDelayReturn = false);
    virtual void eraseSharedPtr(std::shared_ptr<ExynosBuffer> shPtr);
    virtual void reset();
    virtual int size();
    virtual int refSize();

    void delayReturn(uint64_t key);

private:
    std::mutex mMutex;
    typedef struct  swap_data_t {
        std::shared_ptr<ExynosBuffer> mShPtr;
        ExynosParams params;
    } swap_data;

    bool mFixedFDEnable;

    ExynosMap<uint64_t, std::shared_ptr<swap_data>> mQueue;
};

#endif // EXYNOS_BUFFER_MANAGER_H
