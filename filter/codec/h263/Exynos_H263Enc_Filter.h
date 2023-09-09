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
#ifndef EXYNOS_H263ENC_FILTER_H
#define EXYNOS_H263ENC_FILTER_H

#include "Exynos_CodecEncBase_Filter.h"

#define LOG_ON
#include "ExynosLog.h"

class ExynosH263EncFilter : public ExynosCodecEncBaseFilter/*, public std::enable_shared_from_this<ExynosH263EncFilter>*/ {
public:
    ExynosH263EncFilter(uint32_t id, bool isSecure = false) : ExynosCodecEncBaseFilter(id, isSecure) {
        mObjName = "ExynosH263EncFilter";
        mbLogOff = false;
        mThreadPool->setObjName(mObjName);
        mCodecType = ExynosVideoCodec::Type::H263ENC;
    }

    ~ExynosH263EncFilter() = default;

private:
    /* override function on ExynosCodecBaseFilter */

    /* add function for ExynosH263EncFilter */
};

#endif // EXYNOS_H263ENC_FILTER_H
