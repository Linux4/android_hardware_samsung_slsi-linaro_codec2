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
#ifndef EXYNOS_GDC_FILTER_H
#define EXYNOS_GDC_FILTER_H

#include <memory>

#include "Exynos_Filter.h"
#include "ExynosGDCWrapper.h"
#include "ExynosDef.h"

#define LOG_ON
#include "ExynosLog.h"


class ExynosGDCFilter : public ExynosFilterBase/*, public std::enable_shared_from_this<ExynosGDCFilter>*/ {
public:
    ExynosGDCFilter(uint32_t id, bool isSecure = false) : ExynosFilterBase(id, isSecure) {
        mObjName = "ExynosGDCFilter";
        mbLogOff = false;
        mThreadPool->setObjName(mObjName);

        mGDC = nullptr;
    }

    ~ExynosGDCFilter() = default;

private:
    /* override function on ExynosListerInterface */

    /* override function on ExynosFilterBase. function for thread pool owned by self */
    bool doStart() override;
    bool doStop() override;
    bool doFlush() override;
    bool doReset() override;
    bool doProcess(std::shared_ptr<ExynosBuffer> buffer) override;

    /* add function for ExynosGDCFilter */
#if 0
    void applyConfig(std::shared_ptr<ExynosParams> params);
#endif

    std::shared_ptr<ExynosGDCWrapper> mGDC;
};

#endif // EXYNOS_GDC_FILTER_H
