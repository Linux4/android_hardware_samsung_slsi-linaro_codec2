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
#include "Exynos_Av1Dec_Filter.h"

#define LOG_ON
#include "ExynosLog.h"
#define LOG_TAG "ExynosAv1DecFilter"

bool ExynosAv1DecFilter::onSetup(std::shared_ptr<ExynosBuffer> buffer) {
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
        /* in case of AV1, there isn't CSD */
        if ((buffer->mParams.get() != nullptr) &&
            (!buffer->mParams->empty())) {
            auto params = std::static_pointer_cast<ExynosFilterParams>(buffer->mParams);

            if (mPiledParams.get() == nullptr) {
                mPiledParams = std::make_shared<ExynosFilterParams>();
            }

            mPiledParams->append(params);
        }

        return false;
    }

    if (mPiledParams.get() != nullptr) {
        auto params = std::static_pointer_cast<ExynosFilterParams>(buffer->mParams);

        params->append(mPiledParams);

        mPiledParams.reset();
    }

    return ExynosCodecDecBaseFilter::onSetup(buffer);
}

bool ExynosAv1DecFilter::onProcess(std::shared_ptr<ExynosBuffer> buffer) {
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
        /* in case of AV1, there isn't CSD */
        return false;
    }

    if (mPiledParams.get() != nullptr) {
        auto params = std::static_pointer_cast<ExynosFilterParams>(buffer->mParams);

        params->append(mPiledParams);

        mPiledParams.reset();
    }

    return ExynosCodecDecBaseFilter::onProcess(buffer);
}

bool ExynosAv1DecFilter::onProcessDone(ExynosBufferInfo input, ExynosBufferInfo output) {
    ExynosLogFunctionTrace();

    std::shared_ptr<ExynosBuffer> inbuffer = input.obj;

    if (inbuffer.get() == nullptr) {
        /* TODO : event handling */
        ExynosLogV("[%s] invalid parameter", __FUNCTION__);
    }

    auto ret = ExynosCodecDecBaseFilter::onProcessDone(input, output);
    if (ret == false) {
        /* TODO */
        return false;
    }

    /* apply configurations generated from video codec */
    {
        auto filterParams = std::static_pointer_cast<ExynosFilterParams>(inbuffer->mParams);

        /* film grain info */
        {
            auto param = std::static_pointer_cast<ExynosParam<ParamFilmGrainInfo>>(
                                output.params.getParam(ExynosParamIndex::FilmGrainInfoIndex));
            if (param.get() != nullptr) {
                auto filterParam = std::make_shared<ExynosFilterParam<ParamFilmGrainInfo>>(param);
                filterParam->registTargetFilter(TARGET_FILTER_ALL);  /* TODO */

                filterParams->addParam(filterParam);

                ExynosLogD("[%s] film grain info : apply(%d), update(%d)", __FUNCTION__,
                            param->m.info.apply_grain, param->m.info.update_grain);
            }
        }
    }

    return true;
}

