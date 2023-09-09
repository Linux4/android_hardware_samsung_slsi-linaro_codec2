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

#include <system/graphics.h>
#include "exynos_format.h"
#include "ExynosVideoCodecCommon.h"

#define LOG_ON
#include "ExynosLog.h"


/* dec specific function  */
ExynosVideoFrameStatusType ExynosVideoCodecCommon::getDisplayStatus(ExynosVideoExtraInfo &extraInfo) {
    UNUSED(extraInfo);
    return VIDEO_FRAME_STATUS_UNKNOWN;
}


/* enc specific function  */
bool ExynosVideoCodecCommon::needToChangeResolution(ExynosBufferInfo &buf) {
    UNUSED(buf);
    return false;
}

ExynosErrorType ExynosVideoCodecCommon::changeResolution(ExynosBufferInfo &buf) {
    UNUSED(buf);
    return EXYNOS_ERROR_UNKNOWN;
}

void ExynosVideoCodecCommon::setDRCRequired(bool bDRCRequired) {
    UNUSED(bDRCRequired);
    return;
}

bool ExynosVideoCodecCommon::getDRCRequired() {
    return false;
}

void ExynosVideoCodecCommon::applyExtraInfo(ExynosBufferInfo &buf) {
    UNUSED(buf);
    return;
}

void ExynosVideoCodecCommon::setDropControl() {
    return;
}

void ExynosVideoCodecCommon::runPerformance() {
    return;
}

int ExynosVideoCodecCommon::setFrameTag(std::shared_ptr<CodecImpl> codecImpl) {
    if (!mExynosPort[ExynosPort::Input].mIsConfigured) {
        ExynosLogE("[%s] src is not configured", __FUNCTION__);
        return -1;
    }

    return CodecSpecific_Set_FrameTag(codecImpl);
}

int ExynosVideoCodecCommon::incFrameTag(std::shared_ptr<CodecImpl> codecImpl) {
    codecImpl->mTagNum++;
    codecImpl->mTagNum %= MAX_TAG_NUM;

    return codecImpl->mTagNum;
}

int ExynosVideoCodecCommon::curFrameTag(std::shared_ptr<CodecImpl> codecImpl) {
    return codecImpl->mTagNum;
}

int ExynosVideoCodecCommon::getFrameTag(std::shared_ptr<CodecImpl> codecImpl) {
    if (!mExynosPort[ExynosPort::Output].mIsConfigured) {
        ExynosLogE("[%s] dst is not configured", __FUNCTION__);
        return -1;
    }

    return CodecSpecific_Get_FrameTag(codecImpl);
}


/* common code */
bool ExynosVideoCodecCommon::setCodecImpl(std::shared_ptr<CodecImpl> codecImpl, bool bEncode) {
    if (codecImpl.get() == nullptr) {
        return false;
    }

    mCommonCodecImpl = codecImpl;
    mIsEncode = bEncode;

    return true;
}

std::string ExynosVideoCodecCommon::getObjName() {
    auto shPtr = GET_SHARED_PTR(mCommonCodecImpl);

    return (shPtr.get() != nullptr) ? shPtr->getObjName() : "Unknown";
}

ExynosErrorType ExynosVideoCodecCommon::commonSrcEnqueue(std::shared_ptr<CodecImpl> codecImpl, ExynosBufferInfo &buf, bool bEncode) {
    ExynosLogFunctionTrace();

    if (isValidCodecHandle() == false) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return EXYNOS_ERROR_BAD_STATE;
    }

    if (!mExynosPort[ExynosPort::Input].mIsConfigured) {
        ExynosLogE("[%s] src is not configured", __FUNCTION__);
        return EXYNOS_ERROR_BAD_STATE;
    }

    ExynosVideoBuffer buffer;
    memset(&buffer, 0, sizeof(buffer));

    if (bEncode == true) {
        /* check a resolution and color format
         * dynamic resolution change
         */
        if (needToChangeResolution(buf) ||
            getDRCRequired()) {
            setDRCRequired(false);
            auto err = changeResolution(buf);
            if (err != EXYNOS_ERROR_NONE) {
                ExynosLogE("[%s] changeResolution() is failed", __FUNCTION__);
                return EXYNOS_ERROR_CHANGE_RESOLUTION;
            }
        }

        applyExtraInfo(buf);

        buffer.nPlaneCnt = buf.nPlane;
    } else {
        /* only valid one buffer for bitstream */
        buffer.nPlaneCnt = 1;
    }

    /* apply configurations */
    applyConfig(buf.params);

    if (codecImpl->mVideoInstInfo.bOTFMode == VIDEO_TRUE) {
        /* low latency WFD does not queue/dequeue buffer */
        return EXYNOS_ERROR_NO_BUFFER_PROCESS;
    }

    buf.nID = setFrameTag(codecImpl);

    for (int i = 0; i < buffer.nPlaneCnt; i++) {
        buffer.planes[i].addr       = buf.pBuffer[i];
        buffer.planes[i].fd         = buf.nFD[i];
        buffer.planes[i].allocSize  = buf.nAllocLen[i];
        buffer.planes[i].dataSize   = buf.nDataSize[i];
    }
    buffer.extraInfo.nTimeStamp = buf.stImageInfo.nTimeStamp;
    buffer.extraInfo.nFlags     = 0;

    if (bEncode == false) {
        // for dec
        if (buf.stImageInfo.eFrameInfo & CodecSpecificData) {
            buffer.extraInfo.nFlags |= VIDEO_FLAG_CSD;
        }
    } else {
        // for enc
        if ((buf.obj.get() != nullptr) &&
            (buf.obj->getFlags() & ExynosBuffer::REPLICA)) {
            buffer.extraInfo.nFlags |= VIDEO_FLAG_REPLICA_DATA;
            setDRCRequired(true);  /* next frame should be started as new stream */
        }
    }

    if (buffer.planes[0].dataSize == 0) {
        buffer.extraInfo.nFlags |= VIDEO_FLAG_EMPTY_DATA;
    }
    if (buf.stImageInfo.eFrameInfo & FrameInfo::EndOfStream) {
        buffer.extraInfo.nFlags |= VIDEO_FLAG_EOS;
    }

    fds_t fds;
    buffer.extraInfo.pBuffer = mExynosPort[ExynosPort::Input].mBufManager->swapSharedPtrToPtr(buf.obj, fds);
    for (int i = 0; i < fds.plane; i++) {
        buffer.planes[i].addr = (void *)(unsigned long long)(fds.fd[i]);
        buffer.planes[i].fd = fds.fd[i];
    }

    /* TODO : buffer.extraInfo.pPrivate = ptr (private data buffer) */

    if (bEncode == true) {
        setDropControl();
        runPerformance();
    }

    auto err = (codecImpl->mInBufOps).Enqueue(codecImpl->mHandle, &buffer);
    if (err != VIDEO_ERROR_NONE) {
        mExynosPort[ExynosPort::Input].mBufManager->eraseSharedPtr(buf.obj);
        ExynosLogE("[%s] inbuf : ExtensionEnqueue() is failed / fd:%d, ptr:%p, size:%d", __FUNCTION__,
                    buf.nFD[0], buf.obj.get(), buf.nDataSize[0]);
        return EXYNOS_ERROR_UNKNOWN;
    }

    ExynosLogD("[%s] input : enqueue / fd(%d), ptr(%p), size(%d), ts(%lld), id(%d)", __FUNCTION__,
                    buf.nFD[0], buf.obj.get(), buf.nDataSize[0], buf.stImageInfo.nTimeStamp, curFrameTag(codecImpl));

    incFrameTag(codecImpl);


    auto ret = streamOnOff(ExynosPort::Input, ExynosPort::On);
    if (ret != EXYNOS_ERROR_NONE) {
        return ret;
    }

    return EXYNOS_ERROR_NONE;
}

ExynosErrorType ExynosVideoCodecCommon::commonDstEnqueue(std::shared_ptr<CodecImpl> codecImpl, ExynosBufferInfo &buf, bool bEncode) {
    ExynosLogFunctionTrace();

    if (isValidCodecHandle() == false) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return EXYNOS_ERROR_BAD_STATE;
    }

    if (!mExynosPort[ExynosPort::Output].mIsConfigured) {
        ExynosLogE("[%s] dst is not configured", __FUNCTION__);
        return EXYNOS_ERROR_BAD_STATE;
    }

    ExynosVideoBuffer buffer;

    memset(&buffer, 0, sizeof(buffer));

    if (bEncode == true) {
        /* only valid one buffer for bitstream */
        buffer.nPlaneCnt = 1;
    } else {
        buffer.nPlaneCnt = buf.nPlane;
    }

    for (int i = 0; i < buffer.nPlaneCnt; i++) {
        buffer.planes[i].addr       = buf.pBuffer[i];
        buffer.planes[i].fd         = buf.nFD[i];
        buffer.planes[i].allocSize  = buf.nAllocLen[i];
        buffer.planes[i].dataSize   = buf.nDataSize[i];
    }

    fds_t fds;
    buffer.extraInfo.pBuffer = mExynosPort[ExynosPort::Output].mBufManager->swapSharedPtrToPtr(buf.obj, fds);
    for (int i = 0; i < fds.plane; i++) {
        buffer.planes[i].addr = (void *)(unsigned long long)(fds.fd[i]);
        buffer.planes[i].fd = fds.fd[i];
    }

    auto err = (codecImpl->mOutBufOps).Enqueue(codecImpl->mHandle, &buffer);
    if (err != VIDEO_ERROR_NONE) {
        mExynosPort[ExynosPort::Output].mBufManager->eraseSharedPtr(buf.obj);
        ExynosLogE("[%s] outbuf : ExtensionEnqueue() is failed / fd:%d, ptr:%p", __FUNCTION__,
                    buf.nFD[0], buf.obj.get());

        return EXYNOS_ERROR_UNKNOWN;
    }

    ExynosLogD("[%s] output : enqueue / fd(%d), ptr(%p)", __FUNCTION__, buf.nFD[0], buf.obj.get());

    auto ret = streamOnOff(ExynosPort::Output, ExynosPort::On);
    if (ret != EXYNOS_ERROR_NONE) {
        return ret;
    }

    return EXYNOS_ERROR_NONE;
}

ExynosErrorType ExynosVideoCodecCommon::commonSrcDequeue(std::shared_ptr<CodecImpl> codecImpl, ExynosBufferInfo &buf, bool bEncode) {
    ExynosLogFunctionTrace();

    if (isValidCodecHandle() == false) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return EXYNOS_ERROR_BAD_STATE;
    }

    if (!mExynosPort[ExynosPort::Input].mIsConfigured) {
        ExynosLogE("[%s] src is not configured", __FUNCTION__);
        return EXYNOS_ERROR_BAD_STATE;
    }

    if (!isStreamOn(ExynosPort::Input)) {
        ExynosLogT("[%s] a state of src is stream off", __FUNCTION__);
        return EXYNOS_ERROR_BAD_STATE;
    }

    buf.eDataInfo   = DataInfo::NoData;
    buf.obj         = nullptr;
    buf.nID         = -1;

    ExynosVideoBuffer buffer;

    memset(&buffer, 0, sizeof(buffer));

    if ((codecImpl->mInBufOps).Dequeue(codecImpl->mHandle, &buffer) != VIDEO_ERROR_NONE) {
        if (!isStreamOn(ExynosPort::Input)) {
            ExynosLogT("[%s] inbuf : ExtensionDequeue() is returned by Stop()", __FUNCTION__);
            return EXYNOS_ERROR_NONE;
        }

        ExynosLogE("[%s] inbuf : ExtensionDequeue() is failed", __FUNCTION__);
        return EXYNOS_ERROR_UNKNOWN;
    }

    buf.nPlane          = buffer.nPlaneCnt;
    for (int i = 0; i < buf.nPlane; i++) {
        buf.nFD[i]       = buffer.planes[i].fd;
        buf.nAllocLen[i] = buffer.planes[i].allocSize;
    }
    buf.obj = mExynosPort[ExynosPort::Input].mBufManager->swapPtrToSharedPtr(buffer.extraInfo.pBuffer);

    ExynosLogD("[%s] input : dequeue / fd(%d), ptr(%p)", __FUNCTION__, buf.nFD[0], buf.obj.get());

    /* TODO : check a type of input data */
    buf.eDataInfo = DataInfo::UnusedData;  /* it will be returned with output */

    auto frameType = (!bEncode)? buffer.extraInfo.specific.dec.frameType:buffer.extraInfo.specific.enc.frameType;

    if (!bEncode) {
        /*
         * MFC D/D could not mark CONSUMED_ONLY in case of CSD
         * need to check frame info
         */
        if ((buf.obj.get() != nullptr) &&
            (buf.obj->mImageInfo.eFrameInfo & FrameInfo::CodecSpecificData)) {
            ExynosLogD("[%s] CSD buffer is processed", __FUNCTION__);
            buf.eDataInfo = DataInfo::UsedData;  /* no generates output */
        }
    }

    if (frameType & VIDEO_FRAME_CONSUMED_ONLY) {
        buf.eDataInfo = DataInfo::UsedData;  /* no generates output */
        ExynosLogD("[%s] input data doesn't have output data", __FUNCTION__);

        if (!bEncode &&
            (frameType & VIDEO_FRAME_CORRUPT)) {
            buf.eDataInfo = DataInfo::CorruptedData;  /* notify an error */

            ExynosLogD("[%s] input is corrupted", __FUNCTION__);
        }
    } else {
        ExynosLogV("[%s] input data has valid output data", __FUNCTION__);
    }

    return EXYNOS_ERROR_NONE;
}

ExynosErrorType ExynosVideoCodecCommon::commonDstDequeue(std::shared_ptr<CodecImpl> codecImpl, ExynosBufferInfo &buf, bool bEncode) {
    ExynosLogFunctionTrace();

    if (isValidCodecHandle() == false) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return EXYNOS_ERROR_BAD_STATE;
    }

    if (!mExynosPort[ExynosPort::Output].mIsConfigured) {
        ExynosLogE("[%s] dst is not configured", __FUNCTION__);
        return EXYNOS_ERROR_BAD_STATE;
    }

    if (!isStreamOn(ExynosPort::Output)) {
        ExynosLogT("[%s] a state of dst is stream off", __FUNCTION__);
        return EXYNOS_ERROR_BAD_STATE;
    }

    buf.eDataInfo              = DataInfo::NoData;
    buf.obj                    = nullptr;
    buf.nID                    = -1;

    memset(&buf.stImageInfo, 0, sizeof(buf.stImageInfo));
    buf.stImageInfo.eFrameInfo = FrameInfo::UnknownFrame;

    ExynosVideoBuffer buffer;

    memset(&buffer, 0, sizeof(buffer));

    if ((codecImpl->mOutBufOps).Dequeue(codecImpl->mHandle, &buffer) != VIDEO_ERROR_NONE) {
        if (!isStreamOn(ExynosPort::Output)) {
            ExynosLogT("[%s] outbuf : ExtensionDequeue() is returned by Stop()", __FUNCTION__);
            return EXYNOS_ERROR_NONE;
        }

        ExynosLogE("[%s] outbuf : ExtensionDequeue() is failed", __FUNCTION__);
        return EXYNOS_ERROR_UNKNOWN;
    }

    auto displayStatus = getDisplayStatus(buffer.extraInfo);
    ExynosLogD("[%s] display status : %x", __FUNCTION__, displayStatus);

    if (displayStatus == VIDEO_FRAME_STATUS_CHANGE_RESOL) {
        ExynosLogD("[%s] Dynamic Resolution Changed", __FUNCTION__);
        return EXYNOS_ERROR_CHANGE_RESOLUTION;
    }

    if (checkDstDequeueStatus(displayStatus, bEncode) == true) {
        buf.nPlane = buffer.nPlaneCnt;
        for (int i = 0; i < buf.nPlane; i++) {
            buf.nFD[i]          = buffer.planes[i].fd;
            buf.nAllocLen[i]    = buffer.planes[i].allocSize;
            buf.nDataSize[i]    = buffer.planes[i].dataSize;
        }
        buf.obj = mExynosPort[ExynosPort::Output].mBufManager->swapPtrToSharedPtr(buffer.extraInfo.pBuffer);

        buf.eDataInfo = DataInfo::SingleData;  /* singe video image */

        updateBufferInfo(buffer, buf);
    } else {
        ExynosLogV("[%s] no valid output. display status : %x", __FUNCTION__, displayStatus);
        return EXYNOS_ERROR_UNKNOWN;
    }

    return EXYNOS_ERROR_NONE;
}

ExynosErrorType ExynosVideoCodecCommon::commonWaitBuffer(std::shared_ptr<CodecImpl> codecImpl, ExynosPort::Port port, bool &isAvail) {
    ExynosLogFunctionTrace();

    ExynosVideoBufferOps &inBufOps = codecImpl->mInBufOps;
    ExynosVideoBufferOps &outBufOps = codecImpl->mOutBufOps;

    auto handle = codecImpl->mHandle;

    isAvail = false;

    if (handle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return EXYNOS_ERROR_BAD_STATE;
    }

    ExynosVideoPollType avail = POLL_TYPE_NONE;

    auto err = VIDEO_ERROR_NONE;
    if (port == ExynosPort::Input) {
        err = inBufOps.Wait_Buffer(handle, &avail);
    } else {
        err = outBufOps.Wait_Buffer(handle, &avail);
    }

    if (err == VIDEO_ERROR_TRY_AGAIN) {
        return EXYNOS_ERROR_TRY_AGAIN;
    }

    if (err != VIDEO_ERROR_NONE) {
        return EXYNOS_ERROR_UNKNOWN;
    }

    if (port == ExynosPort::Input) {
        isAvail = (avail & POLL_TYPE_SRC)? true:false;
    } else {
        isAvail = (avail & POLL_TYPE_DST)? true:false;
    }

    if (avail & POLL_TYPE_USER) {
        isAvail = false;
        ExynosLogD("[%s] user poll event : no available buffer", __FUNCTION__);
    }

    return EXYNOS_ERROR_NONE;
}

ExynosErrorType ExynosVideoCodecCommon::commonStopWaitBuffer(std::shared_ptr<CodecImpl> codecImpl) {
    ExynosLogFunctionTrace();

    ExynosVideoErrorType err = CodecSpecific_Stop_Wait_Buffer(codecImpl);
    if (err != VIDEO_ERROR_NONE) {
        return EXYNOS_ERROR_UNKNOWN;
    }

    return EXYNOS_ERROR_NONE;
}

ExynosErrorType ExynosVideoCodecCommon::commonResetWaitBuffer(std::shared_ptr<CodecImpl> codecImpl) {
    ExynosLogFunctionTrace();

    ExynosVideoErrorType err = CodecSpecific_Reset_Wait_Buffer(codecImpl);
    if (err != VIDEO_ERROR_NONE) {
        return EXYNOS_ERROR_UNKNOWN;
    }

    return EXYNOS_ERROR_NONE;
}

ExynosErrorType ExynosVideoCodecCommon::commonStop(std::shared_ptr<CodecImpl> codecImpl, ExynosPort::Port port) {
    ExynosLogFunctionTrace();

    auto handle = codecImpl->mHandle;

    if (handle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return EXYNOS_ERROR_BAD_STATE;
    }

    streamOnOff(port, ExynosPort::Off);

    if (mExynosPort[port].mBufManager.get() != nullptr) {
        ExynosLogT("[%s] %s mBufManager size: %d", __FUNCTION__,
                        (port == ExynosPort::Input)? "Src": "Dst",
                        mExynosPort[port].mBufManager->size());
        mExynosPort[port].mBufManager->reset();
    }

    return EXYNOS_ERROR_NONE;
}

bool ExynosVideoCodecCommon::commonIsSBWCEnable(std::shared_ptr<CodecImpl> codecImpl) {
    if ((codecImpl->mGeometry->eFilledDataType == DATA_8BIT_SBWC) ||
        (codecImpl->mGeometry->eFilledDataType == DATA_10BIT_SBWC)) {
        return true;
    } else {
        return false;
    }
}

ExynosErrorType ExynosVideoCodecCommon::commonStreamOnOff(std::shared_ptr<CodecImpl> codecImpl, ExynosPort::Port port, ExynosPort::OnOff eSwitch) {
    ExynosLogFunctionTrace();

    std::lock_guard<std::mutex> lock(mExynosPort[port].mStreamMutex);
    ExynosErrorType ret = EXYNOS_ERROR_NONE;

    ExynosVideoBufferOps    &inBufOps  = codecImpl->mInBufOps;
    ExynosVideoBufferOps    &outBufOps = codecImpl->mOutBufOps;

    auto handle = codecImpl->mHandle;

    if (handle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return EXYNOS_ERROR_BAD_STATE;
    }

    if (eSwitch == ExynosPort::Off) {
        if (mExynosPort[port].mIsOn) {
            mExynosPort[port].mIsOn = false;

            if (port == ExynosPort::Input) {
                inBufOps.Stop(handle);
            } else {
                outBufOps.Stop(handle);
            }

            ExynosLogI("[%s] %s : stream off", __FUNCTION__, (port == ExynosPort::Input)? "Src": "Dst");
        }
    } else if (eSwitch == ExynosPort::On) {
        if (!mExynosPort[port].mIsOn) {
            ExynosLogI("[%s] %s : stream on", __FUNCTION__, (port == ExynosPort::Input)? "Src": "Dst");

            /* input stream on */
            ExynosVideoErrorType err = VIDEO_ERROR_NONE;
            if (port == ExynosPort::Input) {
                err = inBufOps.Run(handle);
            } else {
                err = outBufOps.Run(handle);
            }
            if (err != VIDEO_ERROR_NONE) {
                ExynosLogE("[%s] %s : Run() is failed", __FUNCTION__, (port == ExynosPort::Input)? "inbuf": "outbuf");
                return EXYNOS_ERROR_UNKNOWN;
            }

            mExynosPort[port].mIsOn = true;
        }
    }

    return ret;
}

FrameInfoType ExynosVideoCodecCommon::getFrameInfoType(int frameType) {
    FrameInfoType ret;
    switch (frameType & VIDEO_FRAME_TYPE_MASK) {
    case VIDEO_FRAME_IDR:
    {
        ret = FrameInfo::IDRframe;
        StaticExynosLog(Level::Debug, "FrameType", "[%s] frame type : IDR frame", __FUNCTION__);
    }
        break;
    case VIDEO_FRAME_I:
    {
        ret = FrameInfo::Iframe;
        StaticExynosLog(Level::Debug, "FrameType", "[%s] frame type : I frame", __FUNCTION__);
    }
        break;
    case VIDEO_FRAME_P:
    {
        ret = FrameInfo::Pframe;
        StaticExynosLog(Level::Debug, "FrameType", "[%s] frame type : P frame", __FUNCTION__);
    }
        break;
    case VIDEO_FRAME_B:
    {
        ret = FrameInfo::Bframe;
        StaticExynosLog(Level::Debug, "FrameType", "[%s] frame type : B frame", __FUNCTION__);
    }
        break;
    default:
    {
        ret = FrameInfo::UnknownFrame;
        StaticExynosLog(Level::Debug, "FrameType", "[%s] frame type : Unknown frame", __FUNCTION__);
    }
        break;
    }

    return ret;
}

bool ExynosVideoCodecCommon::checkDstDequeueStatus(ExynosVideoFrameStatusType displayStatus, bool isEncode) {
    if (isEncode == false) {
        if ((displayStatus == VIDEO_FRAME_STATUS_DISPLAY_DECODING) ||
            (displayStatus == VIDEO_FRAME_STATUS_DISPLAY_ONLY) ||
            (displayStatus == VIDEO_FRAME_STATUS_DECODING_ONLY) ||  /* TODO */
            (displayStatus == VIDEO_FRAME_STATUS_DECODING_FINISHED) ||
            (displayStatus == VIDEO_FRAME_STATUS_LAST_FRAME) ||
            (displayStatus == VIDEO_FRAME_STATUS_DISPLAY_INTER_RESOL_CHANGE)) {  /* VP9 */
            return true;
        }

        return false;
    }

    return true;
}

ExynosErrorType ExynosVideoCodecCommon::srcEnqueue(ExynosBufferInfo &buf) {
    auto shPtr = GET_SHARED_PTR(mCommonCodecImpl);

    return (shPtr.get() == nullptr) ? EXYNOS_ERROR_UNKNOWN : commonSrcEnqueue(shPtr, buf, mIsEncode);
}

ExynosErrorType ExynosVideoCodecCommon::srcDequeue(ExynosBufferInfo &buf) {
    auto shPtr = GET_SHARED_PTR(mCommonCodecImpl);

    return (shPtr.get() == nullptr) ? EXYNOS_ERROR_UNKNOWN : commonSrcDequeue(shPtr, buf, mIsEncode);
}

ExynosErrorType ExynosVideoCodecCommon::dstEnqueue(ExynosBufferInfo &buf) {
    auto shPtr = GET_SHARED_PTR(mCommonCodecImpl);

    return (shPtr.get() == nullptr) ? EXYNOS_ERROR_UNKNOWN : commonDstEnqueue(shPtr, buf, mIsEncode);
}

ExynosErrorType ExynosVideoCodecCommon::dstDequeue(ExynosBufferInfo &buf) {
    auto shPtr = GET_SHARED_PTR(mCommonCodecImpl);

    return (shPtr.get() == nullptr) ? EXYNOS_ERROR_UNKNOWN : commonDstDequeue(shPtr, buf, mIsEncode);
}

ExynosErrorType ExynosVideoCodecCommon::waitBuffer(ExynosPort::Port port, bool &isAvail) {
    auto shPtr = GET_SHARED_PTR(mCommonCodecImpl);

    return (shPtr.get() == nullptr) ? EXYNOS_ERROR_UNKNOWN : commonWaitBuffer(shPtr, port, isAvail);
}

ExynosErrorType ExynosVideoCodecCommon::stopWaitBuffer() {
    auto shPtr = GET_SHARED_PTR(mCommonCodecImpl);

    return (shPtr.get() == nullptr) ? EXYNOS_ERROR_UNKNOWN : commonStopWaitBuffer(shPtr);
}

ExynosErrorType ExynosVideoCodecCommon::resetWaitBuffer() {
    auto shPtr = GET_SHARED_PTR(mCommonCodecImpl);

    return (shPtr.get() == nullptr) ? EXYNOS_ERROR_UNKNOWN : commonResetWaitBuffer(shPtr);
}

ExynosErrorType ExynosVideoCodecCommon::portStop(ExynosPort::Port port) {
    auto shPtr = GET_SHARED_PTR(mCommonCodecImpl);

    return (shPtr.get() == nullptr) ? EXYNOS_ERROR_UNKNOWN : commonStop(shPtr, port);
}

bool ExynosVideoCodecCommon::isSBWCEnable() {
    auto shPtr = GET_SHARED_PTR(mCommonCodecImpl);

    return (shPtr.get() == nullptr) ? EXYNOS_ERROR_UNKNOWN : commonIsSBWCEnable(shPtr);
}

ExynosErrorType ExynosVideoCodecCommon::streamOnOff(ExynosPort::Port port, ExynosPort::OnOff eSwitch) {
    auto shPtr = GET_SHARED_PTR(mCommonCodecImpl);

    return (shPtr.get() == nullptr) ? EXYNOS_ERROR_UNKNOWN : commonStreamOnOff(shPtr, port, eSwitch);
}

