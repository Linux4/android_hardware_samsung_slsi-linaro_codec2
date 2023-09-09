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
#ifndef EXYNOS_POST_CONTROL_FILTER_H
#define EXYNOS_POST_CONTROL_FILTER_H

#include <memory>
#include <map>
#include <string>

#include "Exynos_Filter.h"
#include "ExynosDef.h"

#define LOG_ON
#include "ExynosLog.h"


class ExynosPostCtrlFilter : public ExynosFilterBase/*, public std::enable_shared_from_this<ExynosPostCtrlFilter>*/ {
public:
    class ExynosHDR2SDRImpl;
    class ExynosFilmGrainImpl;

    ExynosPostCtrlFilter(uint32_t id, bool isSecure = false) : ExynosFilterBase(id, isSecure) {
        mObjName = "ExynosPostCtrlFilter";
        mbLogOff = false;
        mThreadPool->setObjName(mObjName);

        mUsedFilterNum = 0;
        mCSCFilterID = -1;
        mUseCSC = false;
        mHDR2SDRFilterID = -1;
        mUseHDR2SDR = false;
        mFilmGrainFilterID = -1;
        mUseFilmGrain = false;
        mHasFilmGrain = false;
        mHDR2SDRImpl = nullptr;
        mFilmGrainImpl = nullptr;
    }

    ~ExynosPostCtrlFilter() = default;

private:
    /* override function on ExynosListerInterface */

    /* override function on ExynosFilterBase. function for thread pool owned by self */
    bool doStart() override;
    bool doStop() override;
    bool doFlush() override;
    bool doReset() override;
    bool doProcess(std::shared_ptr<ExynosBuffer> buffer) override;

    /* add function for ExynosPostCtrlFilter */
    void applyConfig(std::shared_ptr<ExynosParams> params);

    /* configurations */
    std::weak_ptr<std::map<std::string, int>> mFilterInfo;

    int32_t mUsedFilterNum;
    int32_t mCSCFilterID;
    bool    mUseCSC;
    int32_t mHDR2SDRFilterID;
    bool    mUseHDR2SDR;
    int32_t mFilmGrainFilterID;
    bool    mUseFilmGrain;
    bool    mHasFilmGrain;
    std::shared_ptr<ExynosHDR2SDRImpl> mHDR2SDRImpl;
    std::shared_ptr<ExynosFilmGrainImpl> mFilmGrainImpl;
};

#endif // EXYNOS_POST_CONTROL_FILTER_H
