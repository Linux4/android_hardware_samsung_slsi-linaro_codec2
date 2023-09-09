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
#include <chrono>
#include <memory>
#include <cutils/properties.h>
#include <hidl/HidlSupport.h>

#include "ExynosPerformance.h"
#include "OperatorFactory.h"

#define LOG_ON
#include "ExynosLog.h"
#define LOG_TAG "ExynosPerformance"

using std::chrono::system_clock;
using namespace android;
using ::android::hardware::hidl_string;

class ExynosPerformance::ExynosPerformanceImpl : public ExynosLog {
public:
    ExynosPerformanceImpl(bool encoder = false): ExynosLog("ExynosPerformance") {
        mIsEncoder = encoder;
        mTimeSlice = property_get_int32("ro.vendor.power.timeslice", 1000/* default: 1000ms */);

        mEpic = std::shared_ptr<epic::IEpicOperator>(static_cast<epic::IEpicOperator *>(
                    createOperator((mIsEncoder)? eVideoEncoding:eVideoDecoding)));
        if (mEpic.get() != nullptr) {
            if (!mIsEncoder) {
                mLatestTime = std::chrono::system_clock::now();
                mEpic->doAction(eAcquire, nullptr);
                StaticExynosLog(Level::Trace, "ExynosPerformanceImpl", "[%s] EpicOperator is acquired", __FUNCTION__);
            }
        } else {
            StaticExynosLog(Level::Trace, "ExynosPerformanceImpl", "[%s] EpicOperator is invalid", __FUNCTION__);
        }
    }

    ~ExynosPerformanceImpl() {
        if (mEpic.get() != nullptr) {
            if (!mIsEncoder) {
                mEpic->doAction(eRelease, nullptr);
                StaticExynosLog(Level::Trace, "ExynosPerformanceImpl", "[%s] EpicOperator is realesed", __FUNCTION__);
            }
        }
    }

    void notify(uint32_t num, uint32_t fps) {
        if (mEpic.get() != nullptr) {
            if (mIsEncoder) {
                system_clock::time_point currentTime = std::chrono::system_clock::now();
                auto duration = ((int32_t)(std::chrono::duration<double, std::milli>(currentTime - mLatestTime).count()));

                int vectors[] = { 1, 2, 4, 8, 16, 32, 64 };
                int threshold = vectors[0];

                for (int i = 0; i < (int)(sizeof(vectors)/sizeof(vectors[0])); i++) {
                    if ((vectors[i] * 1000/*ms*/ / (fps? fps:1)) > mTimeSlice) {
                        /* interval := (vectors[i] * (1000 / fps)ms) > timeslice
                         * it can not be available to inform a hint within timeslice
                         */
                        break;
                    }

                    threshold = vectors[i];
                }

                if ((mTimeSlice <= duration) ||
                    ((num % threshold) == 0)) {
                    mLatestTime = std::chrono::system_clock::now();
                    mEpic->doAction(eAcquire, nullptr);
                    StaticExynosLog(Level::Trace, "ExynosPerformanceImpl", "[%s] EpicOperator is requested", __FUNCTION__);
                }
            }
        }
    }

private:
    bool mIsEncoder;
    int32_t mTimeSlice;
    system_clock::time_point mLatestTime;

    std::shared_ptr<epic::IEpicOperator> mEpic;
};

ExynosPerformance::ExynosPerformance(bool encoder) {
    mImpl = std::make_shared<ExynosPerformanceImpl>(encoder);
}

ExynosPerformance::~ExynosPerformance() {
    mImpl.reset();
}

void ExynosPerformance::notify(
    uint32_t num,
    uint32_t fps) {
    mImpl->notify(num, fps);

    return;
}
