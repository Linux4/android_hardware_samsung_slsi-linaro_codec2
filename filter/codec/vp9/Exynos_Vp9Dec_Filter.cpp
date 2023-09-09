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
#include "Exynos_Vp9Dec_Filter.h"

#define LOG_ON
#include "ExynosLog.h"
#define LOG_TAG "ExynosVp9DecFilter"

void ExynosVp9DecFilter::onApplyConfig(ExynosBufferInfo &info, std::shared_ptr<ExynosParams> params) {
    ExynosLogFunctionTrace();

    if ((params.get() == nullptr) ||
        params->empty()) {
        /* there is nothing to change */
        return;
    }

    ExynosCodecDecBaseFilter::onApplyConfig(info, params);

    auto filterParams = std::static_pointer_cast<ExynosFilterParams>(params);

    std::vector<ExynosParamIndex> vp9decIndexes = {
        ExynosParamIndex::HDRStaticInfoIndex,
    };

    applyFilterParams(info, vp9decIndexes, filterParams);

    return;
}

bool ExynosVp9DecFilter::onSetup(std::shared_ptr<ExynosBuffer> buffer) {
    ExynosLogFunctionTrace();

    if (buffer.get() == nullptr) {
        /* invalid parameter */
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return false;
    }

    if (buffer->getFlags() & ExynosBuffer::REPLICA) {
        ExynosLogD("[%s] discards empty input(%p)", __FUNCTION__, buffer.get());
        return false;
    }

    if (buffer->mImageInfo.eFrameInfo & FrameInfo::CodecSpecificData) {
        /* in case of VP9, CSD should be discarded */
        if ((buffer->mParams.get() != nullptr) &&
            (!buffer->mParams->empty())) {
            auto params = std::static_pointer_cast<ExynosFilterParams>(buffer->mParams);

            if (mPiledParams.get() == nullptr) {
                mPiledParams = std::make_shared<ExynosFilterParams>();
            }

            mPiledParams->append(params);
        }

        ExynosLogD("[%s] discards CSD for vp9 stream(flag: 0x%x)",
                        __FUNCTION__, buffer->mImageInfo.eFrameInfo);
        return false;
    }

    if (mPiledParams.get() != nullptr) {
        auto params = std::static_pointer_cast<ExynosFilterParams>(buffer->mParams);

        params->append(mPiledParams);

        mPiledParams.reset();
    }

    return ExynosCodecDecBaseFilter::onSetup(buffer);
}

bool ExynosVp9DecFilter::onProcess(std::shared_ptr<ExynosBuffer> buffer) {
    ExynosLogFunctionTrace();

    if (buffer.get() == nullptr) {
        /* invalid parameter */
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return false;
    }

    if (mIsConfigured == false) {
        /* TODO : error handling */
        ExynosLogE("[%s] it is not configured", __FUNCTION__);
        return false;
    }

    if (buffer->mImageInfo.eFrameInfo & FrameInfo::CodecSpecificData) {
        /* in case of VP9, CSD should be discarded */
        return false;
    }

    if (mPiledParams.get() != nullptr) {
        auto params = std::static_pointer_cast<ExynosFilterParams>(buffer->mParams);

        params->append(mPiledParams);

        mPiledParams.reset();
    }

    return ExynosCodecBaseFilter::onProcess(buffer);
}
