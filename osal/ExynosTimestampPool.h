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
#ifndef EXYNOS_TIMESTAMP_POOL_H
#define EXYNOS_TIMESTAMP_POOL_H

#include <list>
#include <mutex>
#include <C2.h>

#define LOG_ON
#include "ExynosLog.h"

class ExynosTimestampPool : public ExynosLog {
public:
    ExynosTimestampPool() : ExynosLog("ExynosTimestampPool") {
    }

    ~ExynosTimestampPool() {
        std::lock_guard<std::mutex> lock(mListMutex);

        mTimestamps.clear();
    }

    void addTimestamp(c2_cntr64_t ts) {
        std::lock_guard<std::mutex> lock(mListMutex);

        mTimestamps.push_back(ts);
    }

    c2_cntr64_t getTimestamp(bool sort = false) {
        std::lock_guard<std::mutex> lock(mListMutex);

        c2_cntr64_t ts;

        if (mTimestamps.size() > 0) {
            if (sort) {
                mTimestamps.sort();  /* ascending */
            }

            ts = mTimestamps.front();
            mTimestamps.pop_front();

            if (mLatestTimestamp > ts) {
                ts = mLatestTimestamp;  /* ts should be bigger than latest ts */
            } else {
                mLatestTimestamp = ts;  /* update */
            }
        } else {
            ts = mLatestTimestamp;
        }

        return ts;
    }

    void calibrateTimestamp(c2_cntr64_t ts) {
        std::lock_guard<std::mutex> lock(mListMutex);

        mTimestamps.sort();

        /* remove timestamps smaller than basis timestamp for sync based on codec standard */
        auto criterionIt = std::find(mTimestamps.begin(), mTimestamps.end(), ts);
        if (criterionIt != mTimestamps.end()) {
            mTimestamps.erase(mTimestamps.begin(), criterionIt);
        } else {
            for (auto it = mTimestamps.begin(); it != mTimestamps.end(); it++) {
                if (*it < ts) {
                    it = mTimestamps.erase(it);
                }
            }
        }
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mListMutex);

        mTimestamps.clear();
        mLatestTimestamp = 0;
    }

private:
    std::mutex              mListMutex;
    std::list<c2_cntr64_t>  mTimestamps;
    c2_cntr64_t             mLatestTimestamp;
};

#endif // EXYNOS_TIMESTAMP_POOL_H
