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
#include <chrono>
#include "Exynos_CodecEncBase_Filter.h"

#define LOG_ON
#include "ExynosLog.h"
#define LOG_TAG "ExynosCodecEncBaseFilter"

bool ExynosCodecEncBaseFilter::onStart() {
    ExynosLogFunctionTrace();

    return true;
}

bool ExynosCodecEncBaseFilter::onSetup(std::shared_ptr<ExynosBuffer> buffer) {
    ExynosLogFunctionTrace();

    if (buffer.get() == nullptr) {
        /* invalid parameter */
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return false;
    }

    ExynosBufferInfo input;
    ExynosBufferInfo::reset(input);

    onSetInputBufferInfo(input, buffer);

    input.nID = -1;

    auto shCodec = mCodec;

    if (shCodec.get() == nullptr) {
        /* obj is released */
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return false;
    }

    auto err = shCodec->setup(input);
    if (err == false) {
        ExynosLogE("[%s] setup() is failed", __FUNCTION__);
        return false;
    }

    err = shCodec->getOutputInfo(mWidth, mHeight, mNumOutput);
    if (err == false) {
        ExynosLogE("[%s] getOutputInfo() is failed", __FUNCTION__);
        return false;
    }

    return true;
}

bool ExynosCodecEncBaseFilter::onStop() {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;

    if (shCodec.get() == nullptr) {
        /* obj is released */
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return false;
    }

    auto err = shCodec->flush();
    if (err == false) {
        ExynosLogE("[%s] flush() is failed", __FUNCTION__);
        return false;
    }

    return true;
}

bool ExynosCodecEncBaseFilter::onFlush() {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;

    if (shCodec.get() == nullptr) {
        /* obj is released */
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return false;
    }

    auto err = shCodec->flush();
    if (err == false) {
        ExynosLogE("[%s] flush() is failed", __FUNCTION__);
        return false;
    }

    return true;
}

void ExynosCodecEncBaseFilter::onApplyConfig(
    ExynosBufferInfo                &info,
    std::shared_ptr<ExynosParams>    params) {
    ExynosLogFunctionTrace();

    if ((params.get() == nullptr) ||
        params->empty()) {
        /* there is nothing to change */
        return;
    }

    ExynosCodecBaseFilter::onApplyConfig(info, params);

    auto filterParams = std::static_pointer_cast<ExynosFilterParams>(params);

    std::vector<ExynosParamIndex> encIndexes = {
        ExynosParamIndex::ProfileLevelIndex,
        ExynosParamIndex::BitrateModeIndex,
        ExynosParamIndex::BitrateIndex,
        ExynosParamIndex::FramerateIndex,
        ExynosParamIndex::IDRPeriodIndex,
        ExynosParamIndex::IntraRefreshIndex,
        ExynosParamIndex::QpRangeIndex,
        ExynosParamIndex::DropControlIndex,
        ExynosParamIndex::DynamicFramerateIndex,
        ExynosParamIndex::FrameQpIndex,
        ExynosParamIndex::AverageQpIndex,
        ExynosParamIndex::MinQualityIndex,
        ExynosParamIndex::PMVIndex,
        ExynosParamIndex::IntraVOPRefreshIndex,

        ExynosParamIndex::PrependHeaderModeIndex,   /* h264, hevc */
        ExynosParamIndex::IFrameRatioIndex,         /* h264, hevc */
        ExynosParamIndex::ChromaQpOffsetIndex,      /* h264, hevc */
        ExynosParamIndex::BFrameIndex,              /* h264, hevc, mpeg4 */
        ExynosParamIndex::LayeringIndex,            /* h264, hevc, vp8, vp9 */
        ExynosParamIndex::RefPFramesIndex,          /* h264, hevc, vp8, vp9 */
    };

    applyFilterParams(info, encIndexes, filterParams);

    return;
}

void ExynosCodecEncBaseFilter::onAdditionalWorkForParam(
    ExynosParamIndex                    index,
    std::shared_ptr<ExynosParamBase>    param) {
    ExynosLogFunctionTrace();

    ExynosCodecBaseFilter::onAdditionalWorkForParam(index, param);

    return;
}

bool ExynosCodecEncBaseFilter::onProcessDone(ExynosBufferInfo input, ExynosBufferInfo output) {
    ExynosLogFunctionTrace();

    std::shared_ptr<ExynosBuffer> inbuffer = input.obj;

    if (inbuffer.get() == nullptr) {
        /* in case of CSD, input is invalid */
        if ((input.eDataInfo == DataInfo::NoData) &&
            (output.stImageInfo.eFrameInfo & FrameInfo::CodecSpecificData)) {
            /* save an output buffer to mCSD */
            mCSD = output.obj;

            if (mCSD.get() == nullptr) {
                ExynosLogE("[%s] invalid parameter", __FUNCTION__);
                return false;
            }

            mCSD->mImageInfo = output.stImageInfo;
            mCSD->setDataLen(output.nDataSize[0]);

            if (mDebug & EXYNOS_DEBUG_OUTPUT) {
                ExynosBuffer::dump(mCSD, mDebug, (mObjName + "-output"));
            }

            ExynosLogD("[%s] keep a CSD buffer. it will be sent with next output", __FUNCTION__);
            return true;
        }

        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return false;
    }

    /* add CSD to current filter work */
    if (mCSD.get() != nullptr) {
        auto workInfo = obtainWorkInfo(inbuffer, mCSD);
        if (workInfo.get() == nullptr) {
            ExynosLogV("[%s] obtainWorkInfo() is failed", __FUNCTION__);
            return false;
        }

        ExynosLogD("[%s] set a CSD buffer. buffer(%p)/length(%d)", __FUNCTION__, mCSD.get(), mCSD->getDataLen());

        reuseWorkInfo(workInfo);

        mCSD = nullptr;
    }

    /* for updating max number of output buffer */
    if (mNumMaxOutput < mNumOutput) {  /* number of required output is over than max num for allocating on filter */
        ExynosLogD("[%s] max number of output buffer: change to %d from %d", __FUNCTION__, mNumMaxOutput, mNumOutput);
        mNumMaxOutput = mNumOutput;
    }

    /* apply configurations generated from video codec */
    {
        auto filterParams = std::static_pointer_cast<ExynosFilterParams>(inbuffer->mParams);

        /* average qp info */
        {
            auto param = std::static_pointer_cast<ExynosParam<ParamAverageQpInfo>>(
                                    output.params.getParam(ExynosParamIndex::AverageQpInfoIndex));
            if (param.get() != nullptr) {
                auto filterParam = std::make_shared<ExynosFilterParam<ParamAverageQpInfo>>(param);
                filterParam->registTargetFilter(TARGET_OWNER_COMPONENT);

                filterParams->addParam(filterParam);

                ExynosLogV("[%s] average qp is %d", __FUNCTION__, param->m.value);
            }
        }
    }

    std::shared_ptr<ExynosBuffer> outbuffer = output.obj;

    if (outbuffer.get() != nullptr) {
        outbuffer->mImageInfo = output.stImageInfo;
        outbuffer->setDataLen(output.nDataSize[0]);
    }

    return true;
}

bool ExynosCodecEncBaseFilter::onFillOutBuffers() {
    ExynosLogFunctionTrace();

    allocOutBuffer();  /* allocate an output buffer */

    return true;
}

int ExynosCodecEncBaseFilter::onCheckNeedMoreBuffer() {
    auto shCodec = mCodec;

    if (shCodec.get() == nullptr) {
        /* obj is released */
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return 0;
    }

    auto inBufCnt  = shCodec->getInBufCount();
    auto outBufCnt = shCodec->getOutBufCount();

    /* in case of PreferResources, allocates minimal DPB except for extra */
    auto adjustmentFactor = ((mAllocMode == AllocMode::PreferResources)? shCodec->getExtraBufNum():0);

    auto reqBufCnt = shCodec->getReqOutBufCount() - adjustmentFactor;

    int32_t reqTaskCnt = ((int32_t)mReqAllocCnt.use_count()) - 1;

    ExynosLogV("[%s] input: %d, output: %d, required: %d, requested: %d", __FUNCTION__, inBufCnt, outBufCnt, reqBufCnt, reqTaskCnt);

    return (reqBufCnt < reqTaskCnt)? 0:(reqBufCnt - reqTaskCnt);
}

bool ExynosCodecEncBaseFilter::onSetInputBufferInfo(
    ExynosBufferInfo                &input,
    std::shared_ptr<ExynosBuffer>    buffer) {
    ExynosLogFunctionTrace();

    input.eDataInfo   = DataInfo::UnusedData;
    input.stImageInfo = buffer->mImageInfo;

    ExynosUtils::GetYUVPlaneInfo(input.stImageInfo.nStride, input.stImageInfo.nHeight, input.stImageInfo.nFormat,
                                 input.nPlane, input.nAllocLen, buffer->getFlags());

    ExynosLogV("[%s] input buffer info : number of plane(%d)", __FUNCTION__, input.nPlane);

    auto handle = buffer->handle();

    for (int i = 0; i < input.nPlane; i++) {
        input.nFD[i]       = handle->data[i];
        input.pBuffer[i]   = (void *)(unsigned long long)handle->data[i];  /* TODO */
        input.nDataSize[i] = input.nAllocLen[i];

        if ((input.stImageInfo.eFrameInfo & FrameInfo::EndOfStream) &&
            (buffer->getFlags() & ExynosBuffer::REPLICA)) {
            input.nDataSize[i] = 0;
        }

        ExynosLogD("[%s] input buffer info : fd(%d), size(%d), len(%d)", __FUNCTION__,
                        input.nFD[i], input.nAllocLen[i], input.nDataSize[i]);
    }

    ExynosLogD("[%s] resolution(%d x %d), format(0x%x)", __FUNCTION__,
                    input.stImageInfo.nWidth, input.stImageInfo.nHeight, input.stImageInfo.nFormat);

    input.obj = buffer;

    /* apply configurations */
    onApplyConfig(input, buffer->mParams);

    return true;
}

bool ExynosCodecEncBaseFilter::allocOutBuffer() {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;

    if (shCodec.get() == nullptr) {
        /* obj is released */
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return false;
    }

    /* TODO : set attribute on buffer */
    LinearBufferAttribute attr;
    attr.mSize = (mIsSecure == true) ? ExynosUtils::GetOutputSizeForEncSecure(mWidth, mHeight)
                                       : ExynosUtils::GetOutputSizeForEnc(mWidth, mHeight);

    AllocArg arg;
    arg.attr        = attr;
    arg.limit       = 0;
    arg.checkLimit  = nullptr;
    arg.allocCount  = 0;

    if (mFnBufferAlloc.expired()) {
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return false;
    }

    std::shared_ptr<BufferAllocFnType> allocFunc = mFnBufferAlloc.lock();

    if (allocFunc.get() == nullptr) {
        ExynosLogT("[%s] obj is released", __FUNCTION__);
        return false;
    }

    /* allocate an output buffer */
    auto ret = (*allocFunc)(arg);
    if (ret.first == EXYNOS_ERROR_TRY_AGAIN) {
        if (onCheckNeedMoreBuffer() > 0) {
            /* if allocating a buffer is failed continuously many times,
             * it is considered to get being "pause".
             * therefore, quit to request allocating a buffer.
             */
            if (mNumOutput > 0) {  /* since number of output is decided */
                if ((*mReqAllocCnt) > (ALLOC_RETRY_TIME / WAIT_ALLOC_TIME)) {
                    return false;
                }

                (*mReqAllocCnt)++;  /* mark */
            }

            /* try again. allocating a buffer could be failed due to resource limitation */
            ExynosLogV("[%s] outbuffer is invalid. try again with doFillOutbuffers()", __FUNCTION__);

            int32_t reqTaskCnt = ((int32_t)mReqAllocCnt.use_count()) - 1;
            if (reqTaskCnt <= 0) {
                /* try again. allocating a buffer could be failed due to resource limitation */
                ExynosLogV("[%s] outbuffer is invalid. try again with doFillOutbuffers(), requested: %d", __FUNCTION__, reqTaskCnt);

                fillOutBuffers(1);
            } else {
                ExynosLogT("[%s] there is no available output buffer, requested: %d", __FUNCTION__, reqTaskCnt);
            }
        }

        return false;
    } else if (ret.first != EXYNOS_ERROR_NONE) {
        ExynosLogE("[%s] alloc() is failed. ret:0x%x", __FUNCTION__, ret.first);
        return false;
    }

    std::shared_ptr<ExynosBuffer> outbuffer = ret.second;

    (*mReqAllocCnt) = 0;  /* clear */

    ExynosBufferInfo output;
    ExynosBufferInfo::reset(output);

    auto handle = outbuffer->handle();

    ExynosLogV("[%s] output buffer info : num fd(%d), size(%d)", __FUNCTION__, handle->numFds, attr.mSize);

    output.eDataInfo    = DataInfo::NoData;
    output.nFD[0]       = handle->data[0];
    output.pBuffer[0]   = (void *)(unsigned long long)handle->data[0];  /* TODO */

    output.nAllocLen[0] = outbuffer->size();
    output.nDataSize[0] = 0;
    output.nPlane       = 1;

    output.obj = outbuffer;
    output.nID = -1;

    ExynosLogD("[%s] allocate output buffer : fd(%d), ptr(%p)", __FUNCTION__,
                 output.nFD[0], output.obj.get());

    /* enqueue an output */
    auto err = shCodec->outputEnqueue(output);
    if (err != true) {
        /* TODO : error handling */
        ExynosLogE("[%s] outputEnqueue() is failed", __FUNCTION__);
        return false;
    }

    return true;
}

