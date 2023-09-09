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
#ifndef EXYNOS_CSC_FILTER_H
#define EXYNOS_CSC_FILTER_H

#include <memory>

#include "Exynos_Filter.h"
#include "ExynosCSC.h"
#include "ExynosDef.h"

#define LOG_ON
#include "ExynosLog.h"

enum Port {
    Input   = 0,
    Output  = 1,
    MaxPort = 2,
};

typedef struct ScalingInfo {
    int width;
    int height;
} ScalingInfo;

typedef struct PortInfo {
    int width;
    int height;
    int stride;
    int format;
    CropInfo crop;
} PortInfo;

class ExynosCSCFilter : public ExynosFilterBase/*, public std::enable_shared_from_this<ExynosCSCFilter>*/ {
public:
    ExynosCSCFilter(uint32_t id, bool isSecure = false) : ExynosFilterBase(id, isSecure) {
        mObjName = "ExynosCSCFilter";
        mbLogOff = false;
        mThreadPool->setObjName(mObjName);

        mUseCropping = false;
        memset(&mCrop, 0, sizeof(mCrop));
        mUsePositioning = false;
        memset(&mPosit, 0, sizeof(mPosit));
        mUseScaling = false;
        memset(&mScale, 0, sizeof(mScale));
        mUseFormat = false;
        mFormat = 0;
        mDataSpace = 0;
        mUseBufferCopy = false;
        mOperatingRate = 0;
        mRealTimePriority = 0;
        mFramerate = 0;

        mCSC = nullptr;
    }

    ~ExynosCSCFilter() = default;

private:
    /* override function on ExynosListerInterface */

    /* override function on ExynosFilterBase. function for thread pool owned by self */
    bool doStart() override;
    bool doStop() override;
    bool doFlush() override;
    bool doReset() override;
    bool doProcess(std::shared_ptr<ExynosBuffer> buffer) override;

    /* add function for ExynosCSCFilter */
    void applyConfig(std::shared_ptr<ExynosParams> params);

    /* configurations */
    bool mUseCropping;
    CropInfo mCrop;

    bool mUsePositioning;
    CropInfo mPosit;

    bool mUseScaling;
    ScalingInfo mScale;

    bool mUseFormat;
    int mFormat;

    int mDataSpace;

    bool mUseBufferCopy;

    int mOperatingRate;
    int mRealTimePriority;
    int mFramerate;

    std::shared_ptr<ExynosCSC> mCSC;  /* TODO : change to unique_ptr ? */
};

#endif // EXYNOS_CSC_FILTER_H
