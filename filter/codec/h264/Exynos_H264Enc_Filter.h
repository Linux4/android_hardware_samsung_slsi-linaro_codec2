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
#ifndef EXYNOS_H264ENC_FILTER_H
#define EXYNOS_H264ENC_FILTER_H

#include <memory>

#include "Exynos_CodecEncBase_Filter.h"

#define LOG_ON
#include "ExynosLog.h"

class ExynosH264EncFilter : public ExynosCodecEncBaseFilter/*, public std::enable_shared_from_this<ExynosH264EncFilter>*/ {
public:
    ExynosH264EncFilter(uint32_t id, bool isSecure = false) : ExynosCodecEncBaseFilter(id, isSecure) {
        mObjName = "ExynosH264EncFilter";
        mbLogOff = false;
        mThreadPool->setObjName(mObjName);
        mCodecType = ((mIsSecure == true)? ExynosVideoCodec::Type::H264ENC_SECURE:ExynosVideoCodec::Type::H264ENC);
    }

    ~ExynosH264EncFilter() = default;

private:
    /* override function on ExynosCodecBaseFilter */
    void onApplyConfig(ExynosBufferInfo &info, std::shared_ptr<ExynosParams> params) override;

    /* add function for ExynosH264EncFilter */
};

#endif // EXYNOS_H264ENC_FILTER_H
