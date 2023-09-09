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
#ifndef EXYNOS_VP9ENC_FILTER_H
#define EXYNOS_VP9ENC_FILTER_H

#include "Exynos_CodecEncBase_Filter.h"

#define LOG_ON
#include "ExynosLog.h"

class ExynosVp9EncFilter : public ExynosCodecEncBaseFilter/*, public std::enable_shared_from_this<ExynosVp9EncFilter>*/ {
public:
    ExynosVp9EncFilter(uint32_t id, bool isSecure = false) : ExynosCodecEncBaseFilter(id, isSecure) {
        mObjName = "ExynosVp9EncFilter";
        mbLogOff = false;
        mThreadPool->setObjName(mObjName);
        mCodecType = ExynosVideoCodec::Type::VP9ENC;
    }

    ~ExynosVp9EncFilter() = default;

private:
    /* override function on ExynosCodecBaseFilter */

    /* add function for ExynosVp9EncFilter */
};

#endif // EXYNOS_VP9ENC_FILTER_H
