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
#ifndef EXYNOS_HDR10PLUSSTATENC_FILTER_H
#define EXYNOS_HDR10PLUSSTATENC_FILTER_H

#include <memory>

#include "Exynos_External_Filter.h"
#include "ExynosDef.h"
#include "VendorVideoAPI.h"

#define LOG_ON
#include "ExynosLog.h"

class ExynosHDR10PlusStatEncFilter : public ExynosExternalFilter {
public:
    ExynosHDR10PlusStatEncFilter(uint32_t id, bool isSecure = false) : ExynosExternalFilter(id, isSecure) {
        mObjName = "ExynosHDR10PlusStatEncFilter";
        mbLogOff = false;
        mThreadPool->setObjName(mObjName);

        memset(&mStatInfo, 0, sizeof(mStatInfo));
        mHasStatInfo = false;

        memset(&mDynamicInfo, 0, sizeof(mDynamicInfo));
        mHasDynamicInfo = false;
    }

    ~ExynosHDR10PlusStatEncFilter() = default;

private:
    /* override function on ExynosExternalFilter */
    bool onStart() override;
    bool onReset() override;
    bool onProcess(std::shared_ptr<ExynosBuffer> buffer) override;
    void onApplyConfig(std::shared_ptr<ExynosParams> params) override;

    /* configurations */
    bool         mHasStatInfo;
    HDRStatInfo  mStatInfo;

    bool                 mHasDynamicInfo;
    ExynosHdrDynamicInfo mDynamicInfo;
};

#endif // EXYNOS_HDR10PLUSSTATENC_FILTER_H
