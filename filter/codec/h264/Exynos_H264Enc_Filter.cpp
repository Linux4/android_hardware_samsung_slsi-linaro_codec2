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
#include "Exynos_H264Enc_Filter.h"

#define LOG_ON
#include "ExynosLog.h"
#define LOG_TAG "ExynosH264EncFilter"

void ExynosH264EncFilter::onApplyConfig(ExynosBufferInfo &info, std::shared_ptr<ExynosParams> params) {
    ExynosLogFunctionTrace();

    if ((params.get() == nullptr) ||
        params->empty()) {
        /* there is nothing to change */
        return;
    }

    ExynosCodecEncBaseFilter::onApplyConfig(info, params);

    auto filterParams = std::static_pointer_cast<ExynosFilterParams>(params);

    std::vector<ExynosParamIndex> h264encIndexes = {
        ExynosParamIndex::SliceSizeIndex,
        ExynosParamIndex::EntropyModeIndex,
        ExynosParamIndex::MaxIFrameSizeIndex,

        /* skype */
        ExynosParamIndex::SkypeLowLatencyIndex,
        ExynosParamIndex::SkypeInputControlIndex,
        ExynosParamIndex::SkypeFrameQpIndex,
        ExynosParamIndex::SkypeLTRCountIndex,
        ExynosParamIndex::SkypeLTRIndex,
        ExynosParamIndex::SkypeSARIndex,
        ExynosParamIndex::SkypeBaseLayerPidIndex,
        ExynosParamIndex::MaxLayeringIndex,
    };

    applyFilterParams(info, h264encIndexes, filterParams);

    return;
}

