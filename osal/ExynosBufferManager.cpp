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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ExynosBufferManager.h"

#define GARBAGE_CLEANUP_BASELINE 32

fds_t BufferFdManager::getFixedFds(std::shared_ptr<ExynosBuffer> buffer) {
    fds_t fds;
    memset(&fds, 0, sizeof(fds_t));

    if (buffer.get() == nullptr) {
        return fds;
    }

    auto handle = buffer->handle();
    auto plane = buffer->plane();
    unsigned long nFD[BASE_BUFFER_MAX_PLANES] = {0, 0, 0};

    for (int i = 0; i < plane; i++) {
        nFD[i]       = handle->data[i];
        StaticExynosLog(Level::Trace, "BufferFdManager", "[%s] buffer info : fd(%d)", __FUNCTION__, nFD[i]);
    }

    auto optKey = buffer->getStIno();
    if (!optKey) {
        fds.key = getStIno(nFD[0]);
        buffer->setStIno(fds.key);
    } else {
        fds.key = *optKey;
    }

    int count = 0;

    if (mFdMap.size() > GARBAGE_CLEANUP_BASELINE) {
        /* clear reserved FDs */
        int cnt = 0;

        for (auto it = mFdMap.begin(); it != mFdMap.end(); ) {
            if (it->second.dupCount > 0) {
                it++;
                continue;
            }

            for (int i = 0; i < it->second.plane; i++) {
                close(it->second.fd[i]);
            }

            it = mFdMap.erase(it);
            cnt++;
        }

        StaticExynosLog(Level::Trace, "BufferFdManager", "[%s] clear reserved FDs (%d)", __FUNCTION__, cnt);
    }

    auto search = mFdMap.find(fds.key);
    if (search == mFdMap.end()) {
        fds.plane = plane;
        count = fds.dupCount = 1;
        for (int i = 0; i< fds.plane; i++) {
            fds.fd[i] = dup(nFD[i]);
        }
        mFdMap.emplace(fds.key, fds);
    } else {
        fds.plane = search->second.plane;
        for (int i = 0; i < search->second.plane; i++) {
            if (fixedFd(nFD[i], search->second.fd[i]) >= 0) {
                fds.fd[i] = search->second.fd[i];
            } else {
                StaticExynosLog(Level::Trace, "BufferFdManager", "[%s] Someone intercepted fd:%d.", __FUNCTION__, search->second.fd[i]);
                fds.fd[i] = search->second.fd[i] = dup(nFD[i]);
                StaticExynosLog(Level::Trace, "BufferFdManager", "[%s] New fixed fd:%d.", __FUNCTION__, search->second.fd[i]);
            }
        }
        search->second.dupCount++;
        count = search->second.dupCount;
    }

    StaticExynosLog(Level::Trace, "BufferFdManager", "[%s] getFixedFds inode number:%zu, fd[0]:%d, fd[1]:%d, fd[2]:%d, plane:%d, mapSize:%zu, dupCount:%d",
                                __FUNCTION__, fds.key, fds.fd[0], fds.fd[1], fds.fd[2], fds.plane, mFdMap.size(), count);

    return fds;
}

void BufferFdManager::returnFixedFds(uint64_t key) {
    auto search = mFdMap.find(key);
    if (search != mFdMap.end()) {
        if (search->second.dupCount == 1) {
            for (int i = 0; i < search->second.plane; i++) {
                if (fixedFd(STDOUT_FILENO, search->second.fd[i]) < 0) {
                    search->second.fd[i] = -1;

                    for (int i = 0; i < search->second.plane; i++) {
                        if (search->second.fd[i] >= 0)
                            close(search->second.fd[i]);
                    }

                    mFdMap.erase(search);
                    StaticExynosLog(Level::Trace, "BufferFdManager", "[%s] Someone intercepted fd. keep failed.", __FUNCTION__);

                    return;
                }
            }
        }
        search->second.dupCount--;
        search->second.dupCount = (search->second.dupCount < 0) ? 0 : search->second.dupCount;

        StaticExynosLog(Level::Trace, "BufferFdManager", "[%s] returnFixedFds inode number:%zu, dupCount:%d", __FUNCTION__, key, search->second.dupCount);
    }
}

void BufferFdManager::returnFixedFds(std::shared_ptr<ExynosBuffer> buffer) {
    if (buffer.get() == nullptr) {
        StaticExynosLog(Level::Error, "BufferFdManager", "[%s] ExynosBuffer is nullptr", __FUNCTION__);
        return;
    }

    auto handle = buffer->handle();
    auto plane = buffer->plane();
    unsigned long nFD[BASE_BUFFER_MAX_PLANES] = {0, 0, 0};

    for (int i = 0; i < plane; i++) {
        nFD[i]       = handle->data[i];
        StaticExynosLog(Level::Trace, "BufferFdManager", "[%s] output buffer info : fd(%d)", __FUNCTION__, nFD[i]);
    }

    struct stat statInfo;
    fstat(nFD[0], &statInfo);
    uint64_t key = (uint64_t)statInfo.st_ino;

    returnFixedFds(key);
}

void BufferFdManager::allFdClear() {
    StaticExynosLog(Level::Trace, "[%s] BufferFdManager", "[%s] allFdClear++", __FUNCTION__);
    if (mFdMap.size() > 0) {
        for (auto it = mFdMap.begin(); it != mFdMap.end(); ) {
            for (int i = 0; i < it->second.plane; i++) {
                close(it->second.fd[i]);
            }
            it = mFdMap.erase(it);
        }
    }
    StaticExynosLog(Level::Trace, "BufferFdManager", "[%s] allFdClear--", __FUNCTION__);
}

int BufferFdManager::fixedFd(int orgFd, int targetFd) {
    int ret = -1;

    if ((orgFd < 0) || (targetFd < 0))
        return ret;

#if 1
    if (dup2(orgFd, targetFd) >= 0) {
        ret = targetFd;
    }
#else
    auto dupFd = dup(orgFd);
    if (dup2(dupFd, targetFd) >= 0) {
        ret = targetFd;
    }
    close(dupFd);
#endif

    return ret;
}

uint64_t BufferFdManager::getStIno(int fd) {
    struct stat statInfo;

    fstat(fd, &statInfo);

    return (uint64_t)statInfo.st_ino;
}


void* SharedPtrManager::swapSharedPtrToPtr(std::shared_ptr<ExynosBuffer> shPtr, fds_t &fds) {
    std::lock_guard<std::mutex> lock(mMutex);
    if (shPtr.get() != nullptr) {
        auto swapData = std::make_shared<swap_data>();
        swapData.get()->mShPtr = shPtr;
        mQueue.enqueue((uint64_t)shPtr.get(), swapData);
    }

    memset(&fds, 0, sizeof(fds));

    if (mFixedFDEnable == true) {
        fds = getFixedFds(shPtr);
    }

    return static_cast<void*>(shPtr.get());
}

void* SharedPtrManager::swapSharedPtrToPtr(std::shared_ptr<ExynosBuffer> shPtr, fds_t &fds, ExynosParams params) {
    std::lock_guard<std::mutex> lock(mMutex);
    if (shPtr.get() != nullptr) {
        auto swapData = std::make_shared<swap_data>();
        swapData.get()->mShPtr = shPtr;
        swapData.get()->params = params;

        mQueue.enqueue((uint64_t)shPtr.get(), swapData);
    }

    memset(&fds, 0, sizeof(fds));

    if (mFixedFDEnable == true) {
        fds = getFixedFds(shPtr);
    }

    return static_cast<void*>(shPtr.get());
}

std::shared_ptr<ExynosBuffer> SharedPtrManager::swapPtrToSharedPtr(void *ptr, bool bDelayReturn) {
    std::lock_guard<std::mutex> lock(mMutex);
    std::shared_ptr<swap_data> swapData = nullptr;
    std::shared_ptr<ExynosBuffer> shPtr = nullptr;

    if (ptr != nullptr) {
        if (true == mQueue.dequeue((uint64_t)ptr, swapData)) {
            if (swapData.get() != nullptr) {
                shPtr = swapData.get()->mShPtr;
            }
        }
    }

    if (mFixedFDEnable == false) {
        goto EXIT;
    }

    if ((shPtr.get() != nullptr) &&
        (bDelayReturn == false)) {
        returnFixedFds(shPtr);
    }

EXIT:
    return shPtr;
}

std::shared_ptr<ExynosBuffer> SharedPtrManager::swapPtrToSharedPtr(void *ptr, ExynosParams &params, bool bDelayReturn) {
    std::lock_guard<std::mutex> lock(mMutex);
    std::shared_ptr<swap_data> swapData = nullptr;
    std::shared_ptr<ExynosBuffer> shPtr = nullptr;

    if (ptr != nullptr) {
        if (true == mQueue.dequeue((uint64_t)ptr, swapData)) {
            if (swapData.get() != nullptr) {
                shPtr = swapData.get()->mShPtr;
                params = swapData.get()->params;
            }
        }
    }

    if (mFixedFDEnable == false) {
        goto EXIT;
    }

    if ((shPtr.get() != nullptr) &&
        (bDelayReturn == false)) {
        returnFixedFds(shPtr);
    }

EXIT:
    return shPtr;
}

void SharedPtrManager::eraseSharedPtr(std::shared_ptr<ExynosBuffer> shPtr) {
    std::lock_guard<std::mutex> lock(mMutex);
    std::shared_ptr<swap_data> temp = nullptr;

    if (shPtr != nullptr) {
        mQueue.dequeue((uint64_t)shPtr.get(), temp);
    }

    if (mFixedFDEnable == true) {
        returnFixedFds(shPtr);
    }
}

void SharedPtrManager::reset() {
    std::lock_guard<std::mutex> lock(mMutex);
    mQueue.clear();


    if (mFixedFDEnable == true) {
        allFdClear();
    }
}

int SharedPtrManager::size() {
    std::lock_guard<std::mutex> lock(mMutex);
    return mQueue.size();
}

int SharedPtrManager::refSize() {
    return -1;
}

void SharedPtrManager::delayReturn(uint64_t key) {
    if (mFixedFDEnable == true) {
        returnFixedFds(key);
    }
}

