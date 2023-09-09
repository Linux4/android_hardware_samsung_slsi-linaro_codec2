/*
 *
 * Copyright 2019 Samsung Electronics S.LSI Co. LTD
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
#include "Exynos_HevcEnc_Filter.h"

#define LOG_ON
#include "ExynosLog.h"
#define LOG_TAG "ExynosHevcEncFilter"

void ExynosHevcEncFilter::onApplyConfig(ExynosBufferInfo &info, std::shared_ptr<ExynosParams> params) {
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
        ExynosParamIndex::HDREncodingIndex,
        ExynosParamIndex::HDRStaticInfoIndex,
        ExynosParamIndex::HDRDynamicInfoIndex,
        ExynosParamIndex::HDR10PlusStatIndex,
        ExynosParamIndex::MaxIFrameSizeIndex,
    };

    applyFilterParams(info, hevcencIndexes, filterParams);

    return;
}

bool ExynosHevcEncFilter::onProcessDone(ExynosBufferInfo input, ExynosBufferInfo output) {
    ExynosLogFunctionTrace();

    auto ret = ExynosCodecEncBaseFilter::onProcessDone(input, output);
    if (ret == false) {
        /* TODO */
        return false;
    }

    std::shared_ptr<ExynosBuffer> inbuffer = input.obj;

    if (inbuffer.get() != nullptr) {
        /* apply configurations generated from video codec */
        if (inbuffer->mParams.get() != nullptr) {
            auto filterParams = std::static_pointer_cast<ExynosFilterParams>(inbuffer->mParams);

            /* hdr10plus statistic info */
            bool hasStatistics = false;
            {
                auto param = std::static_pointer_cast<ExynosParam<ParamHDR10PlusStatInfo>>(
                                output.params.getParam(ExynosParamIndex::HDR10PlusStatInfoIndex));
                if (param.get() != nullptr) {
                    auto filterParam = std::make_shared<ExynosFilterParam<ParamHDR10PlusStatInfo>>(param);

                    filterParam->registTargetFilter(TARGET_FILTER_ALL); /* TODO */

                    filterParams->addParam(filterParam);

                    hasStatistics = true;
                    ExynosLogV("[%s] hdr10plus statistic info", __FUNCTION__);
                }
            }

            /* hdr10+ dynamic info
             * it should be handled by stat filter, if stat is enabled.
             */
            if (hasStatistics) {
                auto filterParam = filterParams->getParam(ParamHdrDynamicInfo::INDEX, mID);
                if (filterParam.get() != nullptr) {
                    /* user gives HDR dynamic info */
                    filterParam->registTargetFilter(TARGET_FILTER_ALL);

                    ExynosLogV("[%s] add hdr10+ dynamic info(user)", __FUNCTION__);
                } else {
                    /* vendor gives HDR dynamic info */
                    auto param = std::static_pointer_cast<ExynosParam<ParamHdrDynamicInfo>>(
                                    input.params.getParam(ParamHdrDynamicInfo::INDEX));
                    if (param.get() != nullptr) {
                        auto filterParam = std::make_shared<ExynosFilterParam<ParamHdrDynamicInfo>>(param);

                        filterParam->registTargetFilter(TARGET_FILTER_ALL); /* TODO */

                        filterParams->addParam(filterParam);

                        ExynosLogV("[%s] add hdr10+ dynamic info(vendor)", __FUNCTION__);
                    }
                }
            }
        }
    }

    return true;
}

