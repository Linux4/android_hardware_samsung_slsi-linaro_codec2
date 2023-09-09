/*
 *
 * Copyright 2021 Samsung Electronics S.LSI Co. LTD
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
#ifndef EXYNOS_SIGNAL_H
#define EXYNOS_SIGNAL_H

#include <condition_variable>
#include <utility>
#include <chrono>
#include <mutex>

#include "ExynosLog.h"

using namespace std::chrono_literals;

class ExynosSignal {
public:
    ExynosSignal() {
        mCount = 0;
    }

    ~ExynosSignal() {
        mSignal.notify_all();
        mCount = 0;
    }

    bool wait_for(uint32_t time) {
        std::unique_lock<std::mutex> lock(mMutex);

        if (mCount > 0) {
            mCount--;
            return true;
        }

        auto condfunc = [this]()->bool {
                            return (mCount > 0)? true:false;
                        };

        auto ret = mSignal.wait_for(lock, (time * 1ms), std::move(condfunc));
        mCount = (mCount > 0)? (mCount - 1):0;

        return ret;
    }

    void notify() {
        std::unique_lock<std::mutex> lock(mMutex);
        mCount++;
        mSignal.notify_one();
    }

    void clear() {
        std::unique_lock<std::mutex> lock(mMutex);
        mCount = 0;
    }

private:
    std::mutex mMutex;
    std::condition_variable mSignal;
    int32_t mCount;
};

#endif  // EXYNOS_SIGNAL_H
