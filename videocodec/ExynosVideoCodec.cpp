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
#include <string>
#include <unistd.h>

#include "ExynosVideoCodec.h"
#include "ExynosVideoCodecDec.h"
#include "ExynosVideoCodecEnc.h"

#include "ExynosVideoApi.h"

#define LOG_ON
#include "ExynosLog.h"
#define LOG_TAG "ExynosVideoCodec"

#define TODO(willUse) (void)willUse
#define MAX_VIDEO_INPUT_BUFFER 25  /* VIDEO_BUFFER_MAX_NUM(32) */
#define WAIT_INPUT_COMPLETION_TIMEOUT_MS 5


ExynosVideoCodecBase::Type cnvCodecType(ExynosVideoCodec::Type type) {
    ExynosVideoCodecBase::Type ret = ExynosVideoCodecBase::Type::UNKNOWN;

    switch (type) {
    case ExynosVideoCodec::Type::H264DEC:
    case ExynosVideoCodec::Type::H264ENC:
    {
        ret = ExynosVideoCodecBase::Type::H264;
    }
        break;
    case ExynosVideoCodec::Type::H264DEC_SECURE:
        [[fallthrough]];
    case ExynosVideoCodec::Type::H264ENC_SECURE:
    {
        ret = ExynosVideoCodecBase::Type::H264_SECURE;
    }
        break;
    case ExynosVideoCodec::Type::HEVCDEC:
    case ExynosVideoCodec::Type::HEVCENC:
    {
        ret = ExynosVideoCodecBase::Type::HEVC;
    }
        break;
    case ExynosVideoCodec::Type::HEVCDEC_SECURE:
        [[fallthrough]];
    case ExynosVideoCodec::Type::HEVCENC_SECURE:
    {
        ret = ExynosVideoCodecBase::Type::HEVC_SECURE;
    }
        break;
    case ExynosVideoCodec::Type::MPEG4DEC:
    case ExynosVideoCodec::Type::MPEG4ENC:
    {
        ret = ExynosVideoCodecBase::Type::MPEG4;
    }
        break;
    case ExynosVideoCodec::Type::H263DEC:
    case ExynosVideoCodec::Type::H263ENC:
    {
        ret = ExynosVideoCodecBase::Type::H263;
    }
        break;
    case ExynosVideoCodec::Type::VP8DEC:
    case ExynosVideoCodec::Type::VP8ENC:
    {
        ret = ExynosVideoCodecBase::Type::VP8;
    }
        break;
    case ExynosVideoCodec::Type::VP9DEC:
    case ExynosVideoCodec::Type::VP9ENC:
    {
        ret = ExynosVideoCodecBase::Type::VP9;
    }
        break;
    case ExynosVideoCodec::Type::VP9DEC_SECURE:
    {
        ret = ExynosVideoCodecBase::Type::VP9_SECURE;
    }
        break;
    case ExynosVideoCodec::Type::MPEG2DEC:
    {
        ret = ExynosVideoCodecBase::Type::MPEG2;
    }
        break;
    case ExynosVideoCodec::Type::VC1DEC:
    {
        ret = ExynosVideoCodecBase::Type::VC1;
    }
        break;
    case ExynosVideoCodec::Type::WMVDEC:
    {
        ret = ExynosVideoCodecBase::Type::WMV;
    }
        break;
    case ExynosVideoCodec::Type::AV1DEC:
    {
        ret = ExynosVideoCodecBase::Type::AV1;
    }
        break;
    case ExynosVideoCodec::Type::AV1DEC_SECURE:
    {
        ret = ExynosVideoCodecBase::Type::AV1_SECURE;
    }
        break;
    case ExynosVideoCodec::Type::H264WFDENC:
    {
        ret = ExynosVideoCodecBase::Type::H264WFD;
    }
        break;
    case ExynosVideoCodec::Type::H264WFDENC_SECURE:
    {
        ret = ExynosVideoCodecBase::Type::H264WFD_SECURE;
    }
        break;
    case ExynosVideoCodec::Type::HEVCWFDENC:
    {
        ret = ExynosVideoCodecBase::Type::HEVCWFD;
    }
        break;
    case ExynosVideoCodec::Type::HEVCWFDENC_SECURE:
    {
        ret = ExynosVideoCodecBase::Type::HEVCWFD_SECURE;
    }
        break;

    default:
        break;
    }

    return ret;
}

bool isEncode(ExynosVideoCodec::Type type) {
    return (type >= ExynosVideoCodec::Type::ENCODE_START)? true: false;
}

std::pair<std::shared_ptr<ExynosVideoCodecBase>, std::string> CreateCodec(ExynosVideoCodecBase::Type type, bool isEncode) {
    std::shared_ptr<ExynosVideoCodecBase> codec = nullptr;
    std::string objName;

    if ((type <= ExynosVideoCodecBase::Type::UNKNOWN) ||
        (type >= ExynosVideoCodecBase::Type::CODECMAX)) {
        objName = "Unknown";
        goto EXIT;
    }

    if (isEncode == false) {
        codec = std::static_pointer_cast<ExynosVideoCodecBase>(std::make_shared<ExynosVideoCodecDec>(type));
    } else {
        codec = std::static_pointer_cast<ExynosVideoCodecBase>(std::make_shared<ExynosVideoCodecEnc>(type));
    }

    objName = codec->getObjName();

EXIT:
    return (std::make_pair(codec, objName));
}


ExynosVideoCodec::ExynosVideoCodec(ExynosVideoCodec::Type type) : mIsEncoder(false),
                                                                  mFlush(false),
                                                                  mType(type) {
    mbLogOff = false;
    mIsEncoder = isEncode(mType);

    auto codecInfo = CreateCodec(cnvCodecType(mType), mIsEncoder);
    mCodec = codecInfo.first;
    mObjName = codecInfo.second;

    mSyncSignal = std::make_shared<ExynosSignal>();
}

ExynosVideoCodec::~ExynosVideoCodec() {
    ExynosLogFunctionTrace();

    {
        ExynosMutex<ExynosMap<uint32_t, ExynosBufferInfo>>::LockObj inputs(mInputs);
        inputs->clear();
    }

    {
        ExynosMutex<ExynosMap<uint64_t, ExynosBufferInfo>>::LockObj outputs(mOutputs);
        outputs->clear();
    }

    mSyncSignal.reset();

    if (mCodec.get() != nullptr) {
        auto shCodec = mCodec;

        if (shCodec.get() != nullptr) {
            shCodec->portStop(ExynosPort::Input);
            shCodec->portStop(ExynosPort::Output);
            shCodec->deinit();
        }
    }

    mCodec.reset();
}

bool ExynosVideoCodec::setCallback(std::shared_ptr<ExynosListener> listener) {
    ExynosLogFunctionTrace();

    if (listener.get() == nullptr) {
        /* invalid parameter */
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return false;
    }

    mNotify = listener;

    return true;
}

/* blocking function */
bool ExynosVideoCodec::setup(ExynosBufferInfo input) {
    ExynosLogFunctionTrace();

    /* TODO : check input validation */
    auto shCodec = mCodec;
    if (!CHECK_SHARED_PTR(shCodec)) {
        return false;
    }

    if (!mIsEncoder) {
        /* specific features for decoder */
        auto codec = static_cast<ExynosVideoCodecDec *>(shCodec.get());

        auto err = codec->srcSetup(input);
        if (err != EXYNOS_ERROR_NONE) {
            std::shared_ptr<ExynosBuffer> buffer = input.obj;
            if (buffer.get() != nullptr) {
                auto info = (FrameInfo)(input.stImageInfo.eFrameInfo | FrameInfo::CorruptedFrame);

                /* W/A for raising an unsupported feature error */
                if (err == EXYNOS_ERROR_NOT_SUPPORT) {
                    info = (FrameInfo)(info | FrameInfo::CodecSpecificData);
                }

                buffer->mImageInfo.eFrameInfo = info;
            }

            ExynosLogE("[%s] srcSetup() is failed", __FUNCTION__);
            return false;
        }

        {
            ExynosMutex<BufferInfo>::LockObj bufferInfo(mBufferInfo);

            err = codec->getResolution(bufferInfo->nWidth, bufferInfo->nHeight, bufferInfo->stCrop);
            if (err != EXYNOS_ERROR_NONE) {
                ExynosLogE("[%s] getResolution() is failed", __FUNCTION__);
                return false;
            }

            int cnt = 0;
            err = codec->getDPBCount(cnt);
            if (err != EXYNOS_ERROR_NONE) {
                ExynosLogE("[%s] getDPBCount() is failed", __FUNCTION__);
                return false;
            }
            bufferInfo->nCnt = cnt;

            err = codec->getColorFormat(bufferInfo->nFormat);
            if (err != EXYNOS_ERROR_NONE) {
                ExynosLogE("[%s] getColorFormat() is failed", __FUNCTION__);
                return false;
            }
        }

        err = codec->dstSetup();
        if (err != EXYNOS_ERROR_NONE) {
            ExynosLogE("[%s] dstSetup() is failed", __FUNCTION__);
            return false;
        }
    } else {
        /* specific features for encoder */
        auto codec = static_cast<ExynosVideoCodecEnc *>(shCodec.get());

        auto err = codec->srcSetup(input);
        if (err != EXYNOS_ERROR_NONE) {
            ExynosLogE("[%s] srcSetup() is failed", __FUNCTION__);
            return false;
        }

        {
            ExynosMutex<BufferInfo>::LockObj bufferInfo(mBufferInfo);

            /* getResolution */
            bufferInfo->nWidth     = input.stImageInfo.nWidth;
            bufferInfo->nHeight    = input.stImageInfo.nHeight;
            bufferInfo->stCrop     = input.stImageInfo.stCropInfo;

            /* getDPBCount */
            bufferInfo->nCnt       = DEFAULT_NUM_OUTPUT_BUFFER;
        }

        err = codec->dstSetup();
        if (err != EXYNOS_ERROR_NONE) {
            ExynosLogE("[%s] dstSetup() is failed", __FUNCTION__);
            return false;
        }
    }

    return true;
}

bool ExynosVideoCodec::resetup() {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;
    if (!CHECK_SHARED_PTR(shCodec)) {
        return false;
    }

    if (!mIsEncoder) {
        /* clear all taks on output dequeue thread */
        clearOutputBuffers();

        /* specific features for decoder */
        auto codec = static_cast<ExynosVideoCodecDec *>(shCodec.get());

        auto err = codec->updateResolution();
        if (err != EXYNOS_ERROR_NONE) {
            ExynosLogE("[%s] updateResolution() is failed", __FUNCTION__);
            return false;
        }

        {
            ExynosMutex<BufferInfo>::LockObj info(mBufferInfo);
            BufferInfo oldInfo;

            /* backup old info */
            memcpy(&oldInfo, info.get(), sizeof(BufferInfo));

            err = codec->getResolution(info->nWidth, info->nHeight, info->stCrop);
            if (err != EXYNOS_ERROR_NONE) {
                ExynosLogE("[%s] getResolution() is failed", __FUNCTION__);
                return false;
            }

            int cnt = 0;
            err = codec->getDPBCount(cnt);
            if (err != EXYNOS_ERROR_NONE) {
                ExynosLogE("[%s] getDPBCount() is failed", __FUNCTION__);
                return false;
            }
            info->nCnt = cnt;

            err = codec->getColorFormat(info->nFormat);
            if (err != EXYNOS_ERROR_NONE) {
                ExynosLogE("[%s] getColorFormat() is failed", __FUNCTION__);
                return false;
            }

            if (!memcmp(info.get(), &oldInfo, sizeof(BufferInfo))) {
                /* nothing is changed */
                return true;
            }
        }

        err = codec->dstSetup();
        if (err != EXYNOS_ERROR_NONE) {
            ExynosLogE("[%s] dstSetup() is failed", __FUNCTION__);
            return false;
        }
    } else {
        /* specific features for encoder */
        auto codec = static_cast<ExynosVideoCodecEnc *>(shCodec.get());

        (void)codec;
    }

    return true;
}

bool ExynosVideoCodec::getOutputInfo(
    uint32_t &width,
    uint32_t &height,
    uint32_t &count) {
    ExynosLogFunctionTrace();

    int format;

    return getOutputInfo(width, height, count, format);
}

bool ExynosVideoCodec::getOutputInfo(
    uint32_t &width,
    uint32_t &height,
    uint32_t &count,
    int      &format) {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;
    if (!CHECK_SHARED_PTR(shCodec)) {
        return false;
    }

    if (!shCodec->isConfigured(ExynosPort::MaxPort)) {
        ExynosLogE("[%s] codec is not configured yet", __FUNCTION__);
        return false;
    }

    {
        ExynosMutex<BufferInfo>::LockObj bufferInfo(mBufferInfo);

        width           = bufferInfo->nWidth;
        height          = bufferInfo->nHeight;
#ifdef USE_PERFORMANCE_MODE
        count           = VIDEO_BUFFER_MAX_NUM;
#else
        count           = bufferInfo->nCnt;
#endif
        format          = bufferInfo->nFormat;
    }

    return true;
}

bool ExynosVideoCodec::getOutCropInfo(CropInfo &crop) {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;
    if (!CHECK_SHARED_PTR(shCodec)) {
        return false;
    }

    if (!shCodec->isConfigured(ExynosPort::MaxPort)) {
        ExynosLogE("[%s] codec is not configured yet", __FUNCTION__);
        return false;
    }

    {
        ExynosMutex<BufferInfo>::LockObj bufferInfo(mBufferInfo);

        crop = bufferInfo->stCrop;
    }

    return true;
}

int ExynosVideoCodec::getReqOutBufCount() {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;
    if (!CHECK_SHARED_PTR(shCodec)) {
        return 0;
    }

    int output_enqueue_cnt = getOutBufCount();
    int output_dupBuff_cnt = getRefBufCount();

    ExynosMutex<BufferInfo>::LockObj bufferInfo(mBufferInfo);
    ExynosLogV("[%s] outbuffer : max(%d) - (owns(total:%d) - dupBufs(%d)) = need(%d)", __FUNCTION__,
                bufferInfo->nCnt, output_enqueue_cnt, output_dupBuff_cnt,
                bufferInfo->nCnt - (output_enqueue_cnt - output_dupBuff_cnt));

    return (bufferInfo->nCnt - (output_enqueue_cnt - output_dupBuff_cnt));
}

int ExynosVideoCodec::getInBufCount() {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;
    if (!CHECK_SHARED_PTR(shCodec)) {
        return 0;
    }

    int cnt = 0;

    if (shCodec->getBufCount(ExynosPort::Input, cnt) != EXYNOS_ERROR_NONE) {
        ExynosLogE("[%s] input getBufCount() is failed", __FUNCTION__);
        return 0;
    }

    return cnt;
}

int ExynosVideoCodec::getOutBufCount() {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;
    if (!CHECK_SHARED_PTR(shCodec)) {
        return 0;
    }

    int cnt = 0;

    if (shCodec->getBufCount(ExynosPort::Output, cnt) != EXYNOS_ERROR_NONE) {
        ExynosLogE("[%s] output getBufCount() is failed", __FUNCTION__);
        return 0;
    }

    return cnt;
}

int ExynosVideoCodec::getRefBufCount() {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;
    if (!CHECK_SHARED_PTR(shCodec)) {
        return 0;
    }

    int cnt = -1;

    if (!mIsEncoder) {
        /* only decoder has extra buffer */
        if (shCodec->getRefBufCount(cnt) != EXYNOS_ERROR_NONE) {
            ExynosLogE("[%s] getRefBufCount() is failed", __FUNCTION__);
            return -1;
        }
    }

    return cnt;
}

int ExynosVideoCodec::getRegBufCount() {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;
    if (!CHECK_SHARED_PTR(shCodec)) {
        return 0;
    }

    int cnt = -1;

    if (!mIsEncoder) {
        /* only decoder has extra buffer */
        if (shCodec->getRegBufCount(cnt) != EXYNOS_ERROR_NONE) {
            ExynosLogE("[%s] getRefBufCount() is failed", __FUNCTION__);
            return -1;
        }
    }

    return cnt;
}

int ExynosVideoCodec::getExtraBufNum() {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;
    if (!CHECK_SHARED_PTR(shCodec)) {
        return 0;
    }

    int cnt = 0;

    if (!mIsEncoder) {
        /* only decoder has extra buffer */
        auto codec = static_cast<ExynosVideoCodecDec *>(shCodec.get());

        if (codec->getExtraBufNum(cnt) != EXYNOS_ERROR_NONE) {
            ExynosLogE("[%s] getExtraBufNum() is failed", __FUNCTION__);
            return 0;
        }
    }

    return cnt;
}

ExynosVideoCodec::Type ExynosVideoCodec::getCodecType() {
    ExynosLogFunctionTrace();

    return mType;
}

bool ExynosVideoCodec::inputEnqueue(ExynosBufferInfo input) {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;
    if (!CHECK_SHARED_PTR(shCodec)) {
        return false;
    }

    if (!shCodec->isConfigured(ExynosPort::Input)) {
        ExynosLogE("[%s] codec is not configured yet", __FUNCTION__);
        return false;
    }

    ExynosMutex<ExynosMap<uint32_t, ExynosBufferInfo>>::LockObj inputs(mInputs);

    ExynosErrorType err = EXYNOS_ERROR_NONE;

    err = shCodec->srcEnqueue(input);
    if (err != EXYNOS_ERROR_NONE) {
        if (err != EXYNOS_ERROR_NO_BUFFER_PROCESS) {
            ExynosLogE("[%s] srcEnqueue() is failed", __FUNCTION__);
        }

        /* return an input */
        {
            auto shNotify = GET_SHARED_PTR(mNotify);
            if (!CHECK_SHARED_PTR(shNotify)) {
                return false;
            }

            /* set input info */
            input.eDataInfo = DataInfo::CorruptedData;

            /* empty output */
            ExynosBufferInfo output;
            ExynosBufferInfo::reset(output);

            output.eDataInfo = DataInfo::NoData;
            output.obj       = nullptr;
            output.nID       = -1;

            shNotify->processDone(input, output);
        }

        return false;
    }

    /* if there is info got same ID, discard and overwrite new info */
    inputs->enqueue((uint32_t)input.nID, std::move(input));

    ExynosLogV("[%s] input count: %d", __FUNCTION__ , inputs->size());

    return true;
}

bool ExynosVideoCodec::outputEnqueue(ExynosBufferInfo output) {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;
    if (!CHECK_SHARED_PTR(shCodec)) {
        return false;
    }

    if (!shCodec->isConfigured(ExynosPort::Output)) {
        ExynosLogE("[%s] codec is not configured yet", __FUNCTION__);
        return false;
    }

    ExynosMutex<ExynosMap<uint64_t, ExynosBufferInfo>>::LockObj outputs(mOutputs);

    if (shCodec->dstEnqueue(output) != EXYNOS_ERROR_NONE) {
        ExynosLogE("[%s] dstEnqueue() is failed", __FUNCTION__);
        return false;
    }

    outputs->enqueue((uint64_t)output.obj.get(), std::move(output));

    ExynosLogV("[%s] output count: %d", __FUNCTION__ , outputs->size());

    return true;
}

bool ExynosVideoCodec::flush() {
    ExynosLogFunctionTrace();

    auto flushing = bool_guard<std::atomic<bool>>(mFlush, true);

    auto shCodec = mCodec;
    if (!CHECK_SHARED_PTR(shCodec)) {
        return false;
    }

    if (!shCodec->isConfigured(ExynosPort::MaxPort)) {
        ExynosLogV("[%s] codec is not configured yet", __FUNCTION__);
        return true;
    }

    /* wake up dequeue thread using poll */
    shCodec->stopWaitBuffer();

    shCodec->resetWaitBuffer();

    {
        ExynosMutex<ExynosMap<uint32_t, ExynosBufferInfo>>::LockObj inputs(mInputs);
        inputs->clear();
    }

    {
        ExynosMutex<ExynosMap<uint64_t, ExynosBufferInfo>>::LockObj outputs(mOutputs);
        outputs->clear();
    }

    mSyncSignal->clear();

    /* TODO : error handling */

    return true;
}

bool ExynosVideoCodec::clearOutputBuffers() {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;
    if (!CHECK_SHARED_PTR(shCodec)) {
        return false;
    }

    /* stop */
    auto err = shCodec->portStop(ExynosPort::Output);

    if (err != EXYNOS_ERROR_NONE) {
        ExynosLogE("[%s] dstStop() is failed. err(0x%x)", __FUNCTION__, err);
        return false;
    }

    /* clear all buffers */
    ExynosMutex<ExynosMap<uint64_t, ExynosBufferInfo>>::LockObj outputs(mOutputs);

    outputs->clear();

    return true;
}

bool ExynosVideoCodec::waitDequeue(bool isInput) {
    ExynosLogFunctionTrace();

    if (isInput) {
        return inputDequeue();
    } else {
        return outputDequeue();
    }

    return false;
}

bool ExynosVideoCodec::inputDequeue() {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;
    if (!CHECK_SHARED_PTR(shCodec)) {
        return false;
    }

    if (!shCodec->isConfigured(ExynosPort::Input)) {
        ExynosLogE("[%s] codec is not configured yet", __FUNCTION__);
        return false;
    }

    if (!shCodec->isStreamOn(ExynosPort::Input)) {
        return false;
    }

    ExynosBufferInfo buffer;
    ExynosBufferInfo::reset(buffer);

    auto err = shCodec->srcDequeue(buffer);

    if (err != EXYNOS_ERROR_NONE) {
        ExynosLogE("[%s] srcDequeue() is failed", __FUNCTION__);
        return false;
    }

    if (buffer.obj.get() == nullptr) {
        return false;
    }

    {
        ExynosMutex<BufferInfo>::LockObj bufferInfo(mBufferInfo);

        if (!buffer.obj->destroy(bufferInfo->nCnt)) {
            ExynosLogE("[%s] destroy() is failed", __FUNCTION__);
        }
    }

    /*
     * consumed only : no output
     * call processDone() with empty output
     */
    if ((buffer.eDataInfo == DataInfo::UsedData) ||
        (buffer.eDataInfo == DataInfo::CorruptedData)) {
        auto condfunc = [buffer](ExynosBufferInfo &e)->bool {
                            if (buffer.obj.get() == e.obj.get()) {
                                return true;
                            }

                            return false;
                        };

        ExynosBufferInfo input;
        ExynosBufferInfo::reset(input);

        {
            ExynosMutex<ExynosMap<uint32_t, ExynosBufferInfo>>::LockObj inputs(mInputs);

            if (inputs->dequeue(input, std::move(condfunc)) == false) {
                ExynosLogV("[%s] can not find an input buffer(%p) in mInputs", __FUNCTION__, buffer.obj.get());

                inputs.unlock();

                return false;
            }

            ExynosLogT("[%s] input count: %d", __FUNCTION__ , inputs->size());
        }

        input.eDataInfo = buffer.eDataInfo;

        auto shNotify = GET_SHARED_PTR(mNotify);
        if (!CHECK_SHARED_PTR(shNotify)) {
            return false;
        }

        /* empty output */
        ExynosBufferInfo output;
        ExynosBufferInfo::reset(output);

        output.eDataInfo = DataInfo::NoData;
        output.obj       = nullptr;
        output.nID       = -1;

        shNotify->processDone(input, output);
    } else {
        /* notify input processing is finished */
        mSyncSignal->notify();
    }

    return true;
}

bool ExynosVideoCodec::outputDequeue() {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;
    if (!CHECK_SHARED_PTR(shCodec)) {
        return false;
    }

    if (!shCodec->isConfigured(ExynosPort::Output)) {
        ExynosLogE("[%s] codec is not configured yet", __FUNCTION__);
        return false;
    }

    if (!shCodec->isStreamOn(ExynosPort::Output)) {
        return false;
    }

    ExynosBufferInfo input, output;
    ExynosBufferInfo::reset(input);
    ExynosBufferInfo::reset(output);

    /* dequeue an output buffer */
    {
        ExynosBufferInfo buffer;
        ExynosBufferInfo::reset(buffer);

        auto err = shCodec->dstDequeue(buffer);

        if (err != EXYNOS_ERROR_NONE) {
            if (err == EXYNOS_ERROR_CHANGE_RESOLUTION) {
                auto shNotify = GET_SHARED_PTR(mNotify);
                if (!CHECK_SHARED_PTR(shNotify)) {
                    return false;
                }

                auto event = std::make_shared<ExynosVideoCodecEvent>(ExynosVideoCodecEvent::EventResolutionChanged);

                shNotify->notifyEvent(event);

                return true;
            }

            ExynosLogE("[%s] dstDequeue() is failed", __FUNCTION__);
            return false;
        }

        if (buffer.obj.get() == nullptr) {
            return false;
        }

        auto condfunc = [buffer](ExynosBufferInfo &e)->bool {
                            if (buffer.obj.get() == e.obj.get()) {
                                return true;
                            }

                            return false;
                        };

        ExynosLogD("[%s] outbuffer : ptr(%p)", __FUNCTION__, buffer.obj.get());

        {
            ExynosMutex<ExynosMap<uint64_t, ExynosBufferInfo>>::LockObj outputs(mOutputs);

            if (outputs->dequeue((uint64_t)buffer.obj.get(), output) == false) {
                ExynosLogV("[%s] can not find an output buffer(%p) in mOutputs", __FUNCTION__, buffer.obj.get());
                return false;
            }

            ExynosLogT("[%s] output count: %d", __FUNCTION__ , outputs->size());
        }

        output.eDataInfo              = buffer.eDataInfo;
        output.stImageInfo.eFrameInfo = buffer.stImageInfo.eFrameInfo;
        output.nID      = buffer.nID;
        output.params   = buffer.params;

        if (!mIsEncoder) {
            output.stImageInfo = buffer.stImageInfo;
        }

        for (int i = 0; i < buffer.nPlane; i++) {
            output.nDataSize[i] = buffer.nDataSize[i];
        }
    }

    if ((mIsEncoder == true) &&
        (output.stImageInfo.eFrameInfo & FrameInfo::CodecSpecificData)) {
        /* it doesn't have a matchable input buffer */
        input.eDataInfo = DataInfo::NoData;
        input.obj       = nullptr;
        input.nID       = -1;
    } else {
        /* find an input buffer which has same ID */
        ExynosBufferInfo buffer;
        ExynosBufferInfo::reset(buffer);

        buffer.nID = output.nID;

        if (output.eDataInfo == DataInfo::SingleData) {
            if (!mSyncSignal->wait_for(WAIT_INPUT_COMPLETION_TIMEOUT_MS)) {
                ExynosLogV("[%s] waiting for input completion was timeout", __FUNCTION__);
            }
        }

        /* find an input which has same ExynosBuffer with output */
        ExynosMutex<ExynosMap<uint32_t, ExynosBufferInfo>>::LockObj inputs(mInputs);

        if (inputs->dequeue((uint32_t)buffer.nID, input) == false) {
            if (!mIsEncoder &&
                (output.stImageInfo.eFrameInfo & InterlacedFrame)) {
                /* it is for preventing to miss an output
                 * when inputs are configured as separated field(top or bottom)
                 * and output has ID which is first field.
                 * retry to find an input via next ID.
                 */
                buffer.nID = (output.nID + 1) % MAX_TAG_NUM;

                if (inputs->dequeue((uint32_t)buffer.nID, input) == false) {
                    ExynosLogV("[%s] can not find an input buffer(%d) in mInputs", __FUNCTION__, buffer.nID);
                    return false;
                }
            } else {
                ExynosLogV("[%s] can not find an input buffer(%d) in mInputs", __FUNCTION__, buffer.nID);
                return false;
            }
        }

        if (input.obj->getFlags() & ExynosBuffer::REPLICA) {
            output.obj->setFlags(ExynosBuffer::REPLICA);
        }

        input.eDataInfo = DataInfo::UsedData;

        input.obj->setDestroyNotify(nullptr);  /* it does not need anymore */

        /* TODO : check it is whether multi or not(like as Packed P/B) */
        if (output.eDataInfo == DataInfo::MultiData) {
            ExynosLogV("[%s] input(exynos buffer:%p) wait for more outputs", __FUNCTION__, input.obj.get());
            inputs->enqueue((uint32_t)buffer.nID, input);  /* input will be needed for next output */
        }
    }

    auto shNotify = GET_SHARED_PTR(mNotify);
    if (!CHECK_SHARED_PTR(shNotify)) {
        return false;
    }

    shNotify->processDone(input, output);

    return true;
}

bool ExynosVideoCodec::isSBWCEnable() {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;
    if (!CHECK_SHARED_PTR(shCodec)) {
        return false;
    }

    return shCodec->isSBWCEnable();
}

bool ExynosVideoCodec::checkRealTimeResource(
    int32_t width,
    int32_t height,
    int32_t operatingRate) {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;
    if (!CHECK_SHARED_PTR(shCodec)) {
        return false;
    }

    auto err = shCodec->checkRealTimeResource(width, height, operatingRate);
    if (err != EXYNOS_ERROR_NONE) {
        ExynosLogE("[%s] check real time resource is failed", __FUNCTION__);
        return false;
    }

    return true;
}

ParallelProcessingVideoCodec::ParallelProcessingVideoCodec(ExynosVideoCodec::Type type) : ExynosVideoCodec(type) {
    mbLogOff = false;

    mEnqueueThread       = std::make_shared<ExynosThreadPool>(1, mObjName + "-Enqueue");
    mInputDequeueThread  = std::make_shared<ExynosThreadPool>(1, mObjName + "-InputDequeue");
    mOutputDequeueThread = std::make_shared<ExynosThreadPool>(1, mObjName + "-OutputDequeue");

    mInputFeedTaskCnt = std::make_shared<uint32_t>(0);
}

ParallelProcessingVideoCodec::~ParallelProcessingVideoCodec() {
    {
        ExynosMutex<ExynosQueue<ExynosBufferInfo>>::LockObj pipeInputs(mPipeInputs);
        pipeInputs->clear();
    }

    if (mEnqueueThread.get() != nullptr) {
        mEnqueueThread->stop();
        mEnqueueThread.reset();
    }

    if (mInputDequeueThread.get() != nullptr) {
        mInputDequeueThread->stop();
        mInputDequeueThread.reset();
    }

    if (mOutputDequeueThread.get() != nullptr) {
        mOutputDequeueThread->stop();
        mOutputDequeueThread.reset();
    }
}

int ParallelProcessingVideoCodec::getAvailPipeInputCount() {
    int availCount = 0;

    int limit = MAX_VIDEO_INPUT_BUFFER;

#if 0
    /* in case of dec, limit is restricted as min DPB */
    if (!mIsEncoder) {
        ExynosMutex<BufferInfo>::LockObj bufferInfo(mBufferInfo);

        limit = bufferInfo->nCnt - getExtraBufNum();
    }
#endif

    int queuedCount = getInBufCount();

    availCount = (queuedCount < limit)? (limit - queuedCount):0;

    ExynosLogV("[%s] limit(%d) - queued(%d) = avail(%d)", __FUNCTION__,
                    limit, queuedCount, availCount);

    return availCount;
}

bool ParallelProcessingVideoCodec::clearOutputBuffers() {
    ExynosLogFunctionTrace();

#if 0
    auto shCodec = mCodec;
    if (!CHECK_SHARED_PTR(shCodec)) {
        return false;
    }

    /* stop */
    auto err = shCodec->portStop(ExynosPort::Output);

    if (err != EXYNOS_ERROR_NONE) {
        ExynosLogE("[%s] dstStop() is failed. err(0x%x)", __FUNCTION__, err);
        return false;
    }

    auto shDequeueThread = mOutputDequeueThread;

    /* clear tasks */
    if (shDequeueThread.get() != nullptr) {
        shDequeueThread->flush();
    }

    /* clear all buffers */
    ExynosMutex<ExynosMap<uint64_t, ExynosBufferInfo>>::LockObj outputs(mOutputs);

    outputs->clear();
#else
    ExynosVideoCodec::clearOutputBuffers();

    auto shDequeueThread = mOutputDequeueThread;

    /* clear tasks */
    if (shDequeueThread.get() != nullptr) {
        shDequeueThread->flush();
    }
#endif

    return true;
}

bool ParallelProcessingVideoCodec::inputEnqueue(ExynosBufferInfo input) {
    ExynosLogFunctionTrace();

    auto shEnqueueThread = mEnqueueThread;
    if (!CHECK_SHARED_PTR(shEnqueueThread)) {
        return false;
    }

#if 0  /* it does not need to check
        * because that inputEnqueue() will not be called while flushing
        * caller is same
        */
    if (mFlush) {
        /* being flushing. nothing to do */
        return true;
    }

#endif

    auto func = [wkOwner = weak_from_this(), reqTaskCnt = mInputFeedTaskCnt](ExynosBufferInfo input) mutable ->bool {
                    reqTaskCnt.reset();  /* release a ref. count before calling doPipeInputEnqueue() */

                    auto shOwner = std::static_pointer_cast<ParallelProcessingVideoCodec>(GET_SHARED_PTR(wkOwner));
                    if (CHECK_SHARED_PTR(shOwner)) {
                        return shOwner->doPipeInputEnqueue(input);
                    }

                    return false;
                };

    shEnqueueThread->toss(std::string("ParallelProcessingVideoCodec::doPipeInputEnqueue"), std::move(func), input);

    /* TODO : error handling */

    return true;
}

bool ParallelProcessingVideoCodec::outputEnqueue(ExynosBufferInfo output) {
    ExynosLogFunctionTrace();

    auto DequeueRemainbuffer = [&]()->bool {
            /* although enqueing output buffer is failed,
             * if there are output buffers queueud,
             * try to dequeue.
             */
            auto shCodec = mCodec;

            int cnt = 0;

            if (!CHECK_SHARED_PTR(shCodec)) {
                return false;
            }

            if (shCodec->getBufCount(ExynosPort::Output, cnt) != EXYNOS_ERROR_NONE) {
                ExynosLogE("[%s] output getBufCount() is failed", __FUNCTION__);
                return false;
            }

            if (cnt > 0) {
                waitDequeue(false);
                ExynosLogW("[%s] doOutputEnqueue() is failed. but, try to calling waitDequeue()", __FUNCTION__);
            }

        return true;
    };

    /* need to stop queueing an output while flushing */
    if (mFlush) {
        /* nothing to do */
        return true;
    }

    auto shEnqueueThread = mEnqueueThread;
    if (!CHECK_SHARED_PTR(shEnqueueThread)) {
        return false;
    }

    {
        std::weak_ptr<ParallelProcessingVideoCodec> wkThis = std::static_pointer_cast<ParallelProcessingVideoCodec>(shared_from_this());
        auto ret = shEnqueueThread->post(std::string("ParallelProcessingVideoCodec::doOutputEnqueue"),
                                         weak_pointer_bind(false, &ParallelProcessingVideoCodec::doOutputEnqueue, wkThis, output));
        if (WaitGetResultFromFuture(ret, false) == true) {
            /* send a command to a dequeue thread */
            waitDequeue(false);

            return true;
        } else {
            DequeueRemainbuffer();
        }
    }

    ExynosLogE("[%s] doOutputEnqueue() is timed out", __FUNCTION__);

    return false;
}

bool ParallelProcessingVideoCodec::flush() {
    ExynosLogFunctionTrace();

    auto flushing = bool_guard<std::atomic<bool>>(mFlush, true);

    auto shCodec = mCodec;
    if (!CHECK_SHARED_PTR(shCodec)) {
        return false;
    }

    if (!shCodec->isConfigured(ExynosPort::MaxPort)) {
        ExynosLogV("[%s] codec is not configured yet", __FUNCTION__);
        return true;
    }

    auto shEnqueueThread = mEnqueueThread;
    if (!CHECK_SHARED_PTR(shEnqueueThread)) {
        return false;
    }

    {
        std::weak_ptr<ParallelProcessingVideoCodec> wkThis = std::static_pointer_cast<ParallelProcessingVideoCodec>(shared_from_this());
        auto ret = shEnqueueThread->post(std::string("ParallelProcessingVideoCodec::doFlush"),
                                         weak_pointer_bind(false, &ParallelProcessingVideoCodec::doFlush, wkThis));
        if (WaitGetResultFromFuture(ret, false) == false) {
            /* send a command to a dequeue thread */
            ExynosLogE("[%s] doFlush() is timed out", __FUNCTION__);
            return false;
        }
    }

    {
        ExynosMutex<ExynosQueue<ExynosBufferInfo>>::LockObj pipeInputs(mPipeInputs);
        pipeInputs->clear();
    }

    {
        ExynosMutex<ExynosMap<uint32_t, ExynosBufferInfo>>::LockObj inputs(mInputs);
        inputs->clear();
    }

    {
        ExynosMutex<ExynosMap<uint64_t, ExynosBufferInfo>>::LockObj outputs(mOutputs);
        outputs->clear();
    }

    /* TODO : error handling */

    return true;
}

void ParallelProcessingVideoCodec::stopAllThreadPool() {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;
    if (!CHECK_SHARED_PTR(shCodec)) {
        return;
    }

    /* wake up dequeue thread using poll */
    shCodec->stopWaitBuffer();

    mEnqueueThread->stop();
    mInputDequeueThread->stop();
    mOutputDequeueThread->stop();

    mCodec->deinit();
    mCodec.reset();

    return;
}

bool ParallelProcessingVideoCodec::doPipeInputEnqueue(ExynosBufferInfo input) {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;
    if (!CHECK_SHARED_PTR(shCodec)) {
        return false;
    }

    if (!shCodec->isConfigured(ExynosPort::Input)) {
        ExynosLogE("[%s] codec is not configured yet", __FUNCTION__);
        return false;
    }

    ExynosMutex<ExynosQueue<ExynosBufferInfo>>::LockObj pipeInputs(mPipeInputs);

    /* pile up input to pipeline */
    if (input.obj.get() != nullptr) {
        pipeInputs->enqueue(std::move(input));
    }

    int availCount = getAvailPipeInputCount();
    int queuedCount = 0;

    for (int i = 0; (i < availCount) && (pipeInputs->size() > 0); i++) {
        ExynosBufferInfo pendingInput;
        ExynosBufferInfo::reset(pendingInput);

        if (mIsEncoder) {
            /* specific features for encoder */
            auto codec = static_cast<ExynosVideoCodecEnc *>(shCodec.get());

            if (!pipeInputs->front(pendingInput)) {
                /* failed to get first element */
                break;
            }

            if (!codec->availableToEnqueue(pendingInput)) {
                /* enqueuing is not available. retry to enqueue at next time */
                ExynosLogV("[%s] wait for input consumption. piled Inputs: %d", __FUNCTION__, pipeInputs->size());
                break;
            }
        }

        if (!pipeInputs->dequeue(pendingInput)) {
            break;
        }

        if (pendingInput.obj.get() == nullptr) {
            break;
        }

        auto ret = doInputEnqueue(pendingInput);
        if (ret == true) {
            /* send a command to a dequeue thread */
            waitDequeue(true);
            queuedCount++;
        }
    }

    if (queuedCount > 0) {
        ExynosLogV("[%s] newly queued Inputs: %d, piled Inputs: %d", __FUNCTION__, queuedCount, pipeInputs->size());
    }

    return true;
}

bool ParallelProcessingVideoCodec::doInputEnqueue(ExynosBufferInfo input) {
    ExynosLogFunctionTrace();

    return ExynosVideoCodec::inputEnqueue(input);
}

bool ParallelProcessingVideoCodec::doOutputEnqueue(ExynosBufferInfo output) {
    ExynosLogFunctionTrace();

    return ExynosVideoCodec::outputEnqueue(output);
}

bool ParallelProcessingVideoCodec::doWaitInputDequeue() {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;
    if (!CHECK_SHARED_PTR(shCodec)) {
        return false;
    }

    if (!shCodec->isConfigured(ExynosPort::Input)) {
        ExynosLogE("[%s] codec is not configured yet", __FUNCTION__);
        return false;
    }

    if (!shCodec->isStreamOn(ExynosPort::Input)) {
        return false;
    }

    if ((getCodecType() == ExynosVideoCodec::Type::H264WFDENC) ||
        (getCodecType() == ExynosVideoCodec::Type::H264WFDENC_SECURE) ||
        (getCodecType() == ExynosVideoCodec::Type::HEVCWFDENC) ||
        (getCodecType() == ExynosVideoCodec::Type::HEVCWFDENC_SECURE)) {
        return false;
    }

    bool hasInput = false;

    ExynosErrorType err = EXYNOS_ERROR_NONE;

    do {
        err = shCodec->waitBuffer(ExynosPort::Input, hasInput);
        if (err == EXYNOS_ERROR_TRY_AGAIN) {
            ExynosLogT("[%s] srcWaitBuffer() : try again", __FUNCTION__);
        }
    } while (err == EXYNOS_ERROR_TRY_AGAIN);

    if (err != EXYNOS_ERROR_NONE) {
        ExynosLogT("[%s] srcWaitBuffer() is failed", __FUNCTION__);
        return false;
    }

    bool ret = true;
    if (hasInput == true) {
        ExynosLogT("[%s] input is available", __FUNCTION__);

        ret = inputDequeue();  /* don't delegate, poll() and deqbuf() must be called at one way */

        /* feeding empty input for enqueue trigger at next time */
        auto feedfunc = [&]() {
                            if ((getAvailPipeInputCount() > 0) &&
                                ((int32_t)mInputFeedTaskCnt.use_count() <= 1)) {
                                bool avail = [&]()->bool {
                                                 ExynosMutex<ExynosQueue<ExynosBufferInfo>>::LockObj pipeInputs(mPipeInputs);

                                                 StaticExynosLog(Level::Trace, "ParallelProcessingVideoCodec", "[%s] pipelineInputs: %d",
                                                                    __FUNCTION__, pipeInputs->size());

                                                 return pipeInputs->empty()? false:true;
                                             }();

                                if (avail) {
                                    ExynosBufferInfo empty;
                                    ExynosBufferInfo::reset(empty);

                                    empty.obj = nullptr;

                                    auto shEnqueueThread = mEnqueueThread;
                                    if (!CHECK_SHARED_PTR(shEnqueueThread)) {
                                        return;
                                    }

                                    std::weak_ptr<ParallelProcessingVideoCodec> wkThis = std::static_pointer_cast<ParallelProcessingVideoCodec>(shared_from_this());
                                    shEnqueueThread->toss(std::string("ParallelProcessingVideoCodec::doPipeInputEnqueue"),
                                                                     weak_pointer_bind(false, &ParallelProcessingVideoCodec::doPipeInputEnqueue, wkThis, empty));
                                }
                            }

                            return;
                        };
        feedfunc();
    }

    return ret;
}

bool ParallelProcessingVideoCodec::doWaitOutputDequeue() {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;
    if (!CHECK_SHARED_PTR(shCodec)) {
        return false;
    }

    if (!shCodec->isConfigured(ExynosPort::Output)) {
        ExynosLogE("[%s] codec is not configured yet", __FUNCTION__);
        return false;
    }

    if (!shCodec->isStreamOn(ExynosPort::Output)) {
        return false;
    }

    bool hasOutput = false;

    ExynosErrorType err = EXYNOS_ERROR_NONE;

    do {
        err = shCodec->waitBuffer(ExynosPort::Output, hasOutput);
        if (err == EXYNOS_ERROR_TRY_AGAIN) {
            ExynosLogT("[%s] dstWaitBuffer() : try again", __FUNCTION__);
        }
    } while (err == EXYNOS_ERROR_TRY_AGAIN);

    if (err != EXYNOS_ERROR_NONE) {
        ExynosLogT("[%s] dstWaitBuffer() is failed", __FUNCTION__);
        return false;
    }

    if (hasOutput == true) {
        ExynosLogT("[%s] output is available", __FUNCTION__);

        return outputDequeue();  /* don't delegate, poll() and deqbuf() must be called at one way */
    }

    return true;
}

bool ParallelProcessingVideoCodec::doFlush() {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;
    if (!CHECK_SHARED_PTR(shCodec)) {
        return false;
    }

    if (!shCodec->isConfigured(ExynosPort::MaxPort)) {
        ExynosLogV("[%s] codec is not configured yet", __FUNCTION__);
        return true;
    }

    /* TODO : set flag */

    /* wake up dequeue thread using poll */
    shCodec->stopWaitBuffer();

    std::weak_ptr<ParallelProcessingVideoCodec> wkThis = std::static_pointer_cast<ParallelProcessingVideoCodec>(shared_from_this());

    /* input dequeue thread */
    for (int i = 0; i < ExynosPort::MaxPort; i++) {
        ExynosPort::Port port = (ExynosPort::Port) i;
        if (shCodec->isStreamOn(port)) {
            auto shDequeueThread = (port == ExynosPort::Input)? mInputDequeueThread: mOutputDequeueThread;
            if (!CHECK_SHARED_PTR(shDequeueThread)) {
                return false;
            }

            /* send a task for stream off */
            {
                auto ret = shDequeueThread->post(std::string("ExynosVideoCodec::doPortStop"),
                                                 weak_pointer_bind(false, &ParallelProcessingVideoCodec::doPortStop, wkThis, port));
                if (WaitGetResultFromFuture(ret, false) == false) {
                    if (mIsEncoder ||
                        (port != ExynosPort::Output) ||
                        (shCodec->isStreamOn(port))) {  /* in case of decoder, doPortStop(output) could be processed while DRC handling */
                        ExynosLogE("[%s] do%sputStop() is timed out", __FUNCTION__, (port == ExynosPort::Input)? "In":"Out");
                        return false;
                    }
                }
            }
        }
    }

    shCodec->resetWaitBuffer();
    mSyncSignal->clear();

    /* TODO : notify flush done? */

    return true;
}

bool ParallelProcessingVideoCodec::doStop() {
    ExynosLogFunctionTrace();

    return doFlush();
}

bool ParallelProcessingVideoCodec::doPortStop(ExynosPort::Port port) {
    ExynosLogFunctionTrace();

    auto shCodec = mCodec;
    if (!CHECK_SHARED_PTR(shCodec)) {
        return false;
    }

    auto err = shCodec->portStop(port);

    if (err != EXYNOS_ERROR_NONE) {
        return false;
    }

    if (port == ExynosPort::Input) {
        ExynosMutex<ExynosMap<uint32_t, ExynosBufferInfo>>::LockObj inputs(mInputs);
        inputs->clear();
    } else {
        ExynosMutex<ExynosMap<uint64_t, ExynosBufferInfo>>::LockObj outputs(mOutputs);
        outputs->clear();
    }

    return true;
}

bool ParallelProcessingVideoCodec::waitDequeue(bool isInput) {
    ExynosLogFunctionTrace();

    std::weak_ptr<ParallelProcessingVideoCodec> wkThis = std::static_pointer_cast<ParallelProcessingVideoCodec>(shared_from_this());

    if (isInput) {
        auto shDequeueThread = mInputDequeueThread;
        if (!CHECK_SHARED_PTR(shDequeueThread)) {
            return false;
        }

        {
            shDequeueThread->toss(std::string("ExynosVideoCodec::doWaitInputDequeue"),
                                             weak_pointer_bind(false, &ParallelProcessingVideoCodec::doWaitInputDequeue, wkThis));
            /* TODO : ret value handling */
        }
    } else {
        auto shDequeueThread = mOutputDequeueThread;
        if (!CHECK_SHARED_PTR(shDequeueThread)) {
            return false;
        }

        {
            shDequeueThread->toss(std::string("ExynosVideoCodec::doWaitOutputDequeue"),
                                             weak_pointer_bind(false, &ParallelProcessingVideoCodec::doWaitOutputDequeue, wkThis));
            /* TODO : ret value handling */
        }
    }

    return true;
}

