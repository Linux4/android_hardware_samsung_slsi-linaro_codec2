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
#include "Exynos_HevcWFDEnc_Filter.h"

#define LOG_ON
#include "ExynosLog.h"
#define LOG_TAG "ExynosHevcWFDEncFilter"

void ExynosHevcWFDEncFilter::onApplyConfig(ExynosBufferInfo &info, std::shared_ptr<ExynosParams> params) {
    ExynosLogFunctionTrace();

    if ((params.get() == nullptr) ||
        params->empty()) {
        /* there is nothing to change */
        return;
    }

    ExynosCodecEncBaseFilter::onApplyConfig(info, params);

    auto filterParams = std::static_pointer_cast<ExynosFilterParams>(params);

    std::vector<ExynosParamIndex> hevcencIndexes = {
        ExynosParamIndex::GeneralPBIndex,
        ExynosParamIndex::MaxIFrameSizeIndex,
    };

    applyFilterParams(info, hevcencIndexes, filterParams);

    return;
}
