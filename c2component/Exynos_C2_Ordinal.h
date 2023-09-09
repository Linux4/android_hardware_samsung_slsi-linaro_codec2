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
#ifndef EXYNOS_C2_ORDINAL_H
#define EXYNOS_C2_ORDINAL_H

#include <mutex>

#include "ExynosTimestampPool.h"

#define LOG_ON
#include "ExynosLog.h"

class ExynosC2Ordinal : public ExynosLog {
public:
    ExynosC2Ordinal() : ExynosLog("ExynosC2Ordinal") {
        clear();
    }

    ~ExynosC2Ordinal() {
        clear();
    }

    /* basically ordinal is decied by timestamp */
    void addTimestamp(c2_cntr64_t timestamp) {
        std::lock_guard<std::mutex> lock(mMutex);

        mTimestamps.addTimestamp(timestamp);
    }

    void adjustTimestamp(c2_cntr64_t timestamp) {
        std::lock_guard<std::mutex> lock(mMutex);

        mTimestamps.calibrateTimestamp(timestamp);
    }

    /* update the base index */
    void adjustFrameIndex() {
        std::lock_guard<std::mutex> lock(mMutex);

        mBaseFrameIndex = mFrameIndex + 1;
    }

    /* return ordinal based on timestamp */
    c2_cntr64_t getOrdinal(bool sort = false) {
        std::lock_guard<std::mutex> lock(mMutex);

        c2_cntr64_t ordinal = 0;

        ordinal = mTimestamps.getTimestamp(sort);

        return ordinal;
    }

    /* return ordinal based on frame index */
    c2_cntr64_t getOrdinal(int index) {
        std::lock_guard<std::mutex> lock(mMutex);

        c2_cntr64_t ordinal = 0;

        ordinal = (index < 0)? (mFrameIndex + 1):(mBaseFrameIndex + index);

        if (mFrameIndex < ordinal) {
            mFrameIndex = ordinal;
        }

        mTimestamps.clear();

        return ordinal;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mMutex);

        mTimestamps.clear();
        mFrameIndex     = 0;
        mBaseFrameIndex = 0;
    }

private:
    std::mutex              mMutex;
    ExynosTimestampPool     mTimestamps;
    c2_cntr64_t             mFrameIndex;
    c2_cntr64_t             mBaseFrameIndex;
};

#endif // EXYNOS_C2_ORDINAL_H
