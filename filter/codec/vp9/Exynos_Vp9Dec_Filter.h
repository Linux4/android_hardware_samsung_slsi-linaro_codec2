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
#ifndef EXYNOS_VP9DEC_FILTER_H
#define EXYNOS_VP9DEC_FILTER_H

#include <memory>

#include "Exynos_CodecDecBase_Filter.h"

#define LOG_ON
#include "ExynosLog.h"

class ExynosVp9DecFilter : public ExynosCodecDecBaseFilter /*, public std::enable_shared_from_this<ExynosVp9DecFilter>*/ {
public:
    ExynosVp9DecFilter(uint32_t id, bool isSecure = false) : ExynosCodecDecBaseFilter(id, isSecure) {
        mObjName = "ExynosVp9DecFilter";
        mbLogOff = false;
        mThreadPool->setObjName(mObjName);
        mCodecType = ((mIsSecure == true)? ExynosVideoCodec::Type::VP9DEC_SECURE:ExynosVideoCodec::Type::VP9DEC);

        mPiledParams = nullptr;
    }

    ~ExynosVp9DecFilter() = default;

private:
    /* override function on ExynosCodecBaseFilter */
    void onApplyConfig(ExynosBufferInfo &info, std::shared_ptr<ExynosParams> params) override;
    bool onSetup(std::shared_ptr<ExynosBuffer> buffer) override;
    bool onProcess(std::shared_ptr<ExynosBuffer> buffer) override;

    /* add function for ExynosVp9DecFilter */

    std::shared_ptr<ExynosFilterParams> mPiledParams;
};

#endif // EXYNOS_VP9DEC_FILTER_H
