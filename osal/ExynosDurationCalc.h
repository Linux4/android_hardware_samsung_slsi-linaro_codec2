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
#ifndef EXYNOS_DURATION_CALC_H
#define EXYNOS_DURATION_CALC_H

#include <list>
#include <chrono>
#include <mutex>

#include "ExynosLog.h"

using std::chrono::system_clock;

#define DEFAULT_DURATION_LIMIT 9
#define MIN_DURATION_LIMIT 1

class ExynosDurationCalc {
public:
    ExynosDurationCalc() {
        mLimit = DEFAULT_DURATION_LIMIT;

        clear();
    }

    ~ExynosDurationCalc() = default;

    int32_t getAverage(int32_t confines, int32_t overhead = 0) {
        std::lock_guard<std::mutex> lock(mMutex);

        if (mList.size() == 0) {
            mLatestTime = std::chrono::system_clock::now();
        }

        system_clock::time_point currentTime = std::chrono::system_clock::now();
        auto duration = ((int32_t)(std::chrono::duration<double, std::milli>(currentTime - mLatestTime).count()));

        duration = (duration > (mLatestDuration + confines))? (mLatestDuration + confines):duration;

        if ((mLatestDuration - duration) > overhead) {
            /* for responsibility */
            duration = 0;
            mLatestDuration = 0;
            mList.clear();
        }

        mList.push_back(duration);

        /* calculate the average of duration */
        int32_t sum = 0;
        for (int32_t element : mList) {
            sum += element;
        }
        mLatestDuration = sum / mList.size();

        while (mList.size() >= mLimit) {
            mList.pop_front();  /* remove old duration */
        }

        mLatestTime = currentTime;

        StaticExynosLog(Level::Trace, "ExynosDurationCalc", "[%s] average (%d)", __FUNCTION__, mLatestDuration);

        return mLatestDuration;
    }

    void setLimit(int32_t limit) {
        {
            std::lock_guard<std::mutex> lock(mMutex);

            mLimit = (limit > 0)? limit:MIN_DURATION_LIMIT;
        }

        clear();
    }

    int32_t getLimit() {
        std::lock_guard<std::mutex> lock(mMutex);

        return mLimit;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mMutex);

        mList.clear();
        mLatestDuration = 0;
        mLatestTime = std::chrono::system_clock::now();
    }

private:
    std::mutex mMutex;
    int32_t mLimit;
    std::list<int32_t> mList;

    system_clock::time_point mLatestTime;
    int32_t mLatestDuration;
};

#endif // EXYNOS_DURATION_CALC
