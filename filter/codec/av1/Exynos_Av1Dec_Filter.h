/*
 *
 * Copyright 2020 Samsung Electronics S.LSI Co. LTD
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
#ifndef EXYNOS_AV1DEC_FILTER_H
#define EXYNOS_AV1DEC_FILTER_H

#include <memory>

#include "Exynos_CodecDecBase_Filter.h"

#define LOG_ON
#include "ExynosLog.h"

class ExynosAv1DecFilter : public ExynosCodecDecBaseFilter /*, public std::enable_shared_from_this<ExynosAv1DecFilter>*/ {
public:
    ExynosAv1DecFilter(uint32_t id, bool isSecure = false) : ExynosCodecDecBaseFilter(id, isSecure) {
        mObjName = "ExynosAv1DecFilter";
        mbLogOff = false;
        mThreadPool->setObjName(mObjName);
        mCodecType = ((mIsSecure == true)? ExynosVideoCodec::Type::AV1DEC_SECURE:ExynosVideoCodec::Type::AV1DEC);

        mPiledParams = nullptr;
    }

    ~ExynosAv1DecFilter() = default;

private:
    /* override function on ExynosCodecBaseFilter */
    bool onSetup(std::shared_ptr<ExynosBuffer> buffer) override;
    bool onProcess(std::shared_ptr<ExynosBuffer> buffer) override;
    bool onProcessDone(ExynosBufferInfo input, ExynosBufferInfo output) override;

    /* add function for ExynosAv1DecFilter */

    std::shared_ptr<ExynosFilterParams> mPiledParams;
};

#endif // EXYNOS_AV1DEC_FILTER_H
