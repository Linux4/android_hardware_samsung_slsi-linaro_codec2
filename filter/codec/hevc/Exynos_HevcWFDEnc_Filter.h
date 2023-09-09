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
#ifndef EXYNOS_HEVCWFDENC_FILTER_H
#define EXYNOS_HEVCWFDENC_FILTER_H

#include <memory>

#include "Exynos_CodecEncBase_Filter.h"

#define LOG_ON
#include "ExynosLog.h"

class ExynosHevcWFDEncFilter : public ExynosCodecEncBaseFilter/*, public std::enable_shared_from_this<ExynosHevcWFDEncFilter>*/ {
public:
    ExynosHevcWFDEncFilter(uint32_t id, bool isSecure = false) : ExynosCodecEncBaseFilter(id, isSecure) {
        mObjName = "ExynosHevcWFDEncFilter";
        mbLogOff = false;
        mThreadPool->setObjName(mObjName);
        mCodecType = ((mIsSecure == true)? ExynosVideoCodec::Type::HEVCWFDENC_SECURE:ExynosVideoCodec::Type::HEVCWFDENC);
    }

    ~ExynosHevcWFDEncFilter() = default;

private:
    /* override function on ExynosCodecBaseFilter */
    void onApplyConfig(ExynosBufferInfo &info, std::shared_ptr<ExynosParams> params) override;

    /* add function for ExynosHevcWFDEncFilter */
};

#endif // EXYNOS_HEVCWFDENC_FILTER_H
