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
#ifndef EXYNOS_H263DEC_FILTER_H
#define EXYNOS_H263DEC_FILTER_H

#include "Exynos_CodecDecBase_Filter.h"

#define LOG_ON
#include "ExynosLog.h"

class ExynosH263DecFilter : public ExynosCodecDecBaseFilter /*, public std::enable_shared_from_this<ExynosH263DecFilter>*/ {
public:
    ExynosH263DecFilter(uint32_t id, bool isSecure) : ExynosCodecDecBaseFilter(id, isSecure) {
        mObjName = "ExynosH263DecFilter";
        mbLogOff = false;
        mThreadPool->setObjName(mObjName);
        mCodecType = ExynosVideoCodec::Type::H263DEC;
    }

    ~ExynosH263DecFilter() = default;

private:
    /* override function on ExynosCodecBaseFilter */

    /* add function for ExynosH263DecFilter */
};

#endif // EXYNOS_H263DEC_FILTER_H
