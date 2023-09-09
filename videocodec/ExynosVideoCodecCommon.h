/*
 *
 * Copyright 2021 Samsung Electronics S.LSI Co. LTD
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
#ifndef EXYNOS_VIDEO_CODEC_COMMON_H
#define EXYNOS_VIDEO_CODEC_COMMON_H

#include <string>
#include <memory>

#include "ExynosVideoCodecBase.h"

#ifdef USE_PERFORMANCE
#include "ExynosPerformance.h"
#endif

#define LOG_ON
#include "ExynosLog.h"


class CodecImpl : public ExynosLog {
public:
    CodecImpl(std::string name, bool encoder = false) : ExynosLog(name), mHandle(nullptr), mGeometry(nullptr), mTagNum(0) {
#ifdef USE_PERFORMANCE
        mPerf = std::make_shared<ExynosPerformance>(encoder);
#else
        (void)encoder;
#endif
    }

    virtual ~CodecImpl() = default;

    std::string getObjName() {
        return mObjName;
    }

    ExynosVideoInstInfo mVideoInstInfo;
    std::variant<ExynosVideoDecOps, ExynosVideoEncOps> mCommonOps;
    ExynosVideoBufferOps mInBufOps;
    ExynosVideoBufferOps mOutBufOps;

    void *mHandle;

    ExynosVideoGeometry  *mGeometry;

#ifdef USE_PERFORMANCE
    std::shared_ptr<ExynosPerformance> mPerf;
#endif

    uint32_t mTagNum;
};

class ExynosVideoCodecCommon : public ExynosVideoCodecBase {
public:
    ExynosVideoCodecCommon() = default;

    virtual ~ExynosVideoCodecCommon() = default;

#if 0
    /* override common function on ExynosVideoCodecBase */
    virtual ExynosErrorType init() = 0;
    virtual void deinit() = 0;
    virtual ExynosErrorType srcSetup(ExynosBufferInfo input) = 0;
    virtual ExynosErrorType dstSetup() = 0;
#endif
    virtual std::string getObjName() override;

protected:
    virtual bool isValidCodecHandle() = 0;
    virtual void applyConfig(ExynosParams &params) = 0;
    virtual void updateBufferInfo(ExynosVideoBuffer &buffer, ExynosBufferInfo &buf) = 0;

    /* for common API */
    virtual int CodecSpecific_Set_FrameTag(std::shared_ptr<CodecImpl> codecImpl) = 0;
    virtual int CodecSpecific_Get_FrameTag(std::shared_ptr<CodecImpl> codecImpl) = 0;
    virtual ExynosVideoErrorType CodecSpecific_Stop_Wait_Buffer(std::shared_ptr<CodecImpl> codecImpl) = 0;
    virtual ExynosVideoErrorType CodecSpecific_Reset_Wait_Buffer(std::shared_ptr<CodecImpl> codecImpl) = 0;

    /* dec specific function  */
    virtual ExynosVideoFrameStatusType getDisplayStatus(ExynosVideoExtraInfo &extraInfo);

    /* enc specific function  */
    virtual bool needToChangeResolution(ExynosBufferInfo &buf);
    virtual ExynosErrorType changeResolution(ExynosBufferInfo &buf);
    virtual void setDRCRequired(bool bDRCRequired);
    virtual bool getDRCRequired();
    virtual void applyExtraInfo(ExynosBufferInfo &buf);
    virtual void setDropControl();
    virtual void runPerformance();

    /* common code */
    ExynosErrorType srcEnqueue(ExynosBufferInfo &buf) override;
    ExynosErrorType srcDequeue(ExynosBufferInfo &buf) override;
    ExynosErrorType dstEnqueue(ExynosBufferInfo &buf) override;
    ExynosErrorType dstDequeue(ExynosBufferInfo &buf) override;
    ExynosErrorType waitBuffer(ExynosPort::Port port, bool &isAvail) override;
    ExynosErrorType stopWaitBuffer() override;
    ExynosErrorType resetWaitBuffer() override;
    virtual ExynosErrorType portStop(ExynosPort::Port port) override;
    ExynosErrorType streamOnOff(ExynosPort::Port port, ExynosPort::OnOff eSwitch) override;
    bool isSBWCEnable() override;

    int setFrameTag(std::shared_ptr<CodecImpl> codecImpl);
    int incFrameTag(std::shared_ptr<CodecImpl> codecImpl);
    int curFrameTag(std::shared_ptr<CodecImpl> codecImpl);
    int getFrameTag(std::shared_ptr<CodecImpl> codecImpl);

    FrameInfoType getFrameInfoType(int frameType);
    bool checkDstDequeueStatus(ExynosVideoFrameStatusType displayStatus, bool isEncode);

    bool setCodecImpl(std::shared_ptr<CodecImpl> codecImpl, bool bEncode);

private:
    ExynosErrorType commonSrcEnqueue(std::shared_ptr<CodecImpl> codecImpl, ExynosBufferInfo &buf, bool bEncode = false);
    ExynosErrorType commonDstEnqueue(std::shared_ptr<CodecImpl> codecImpl, ExynosBufferInfo &buf, bool bEncode = false);
    ExynosErrorType commonSrcDequeue(std::shared_ptr<CodecImpl> codecImpl, ExynosBufferInfo &buf, bool bEncode = false);
    ExynosErrorType commonDstDequeue(std::shared_ptr<CodecImpl> codecImpl, ExynosBufferInfo &buf, bool bEncode = false);

    ExynosErrorType commonWaitBuffer(std::shared_ptr<CodecImpl> codecImpl, ExynosPort::Port port, bool &isAvail);
    ExynosErrorType commonStopWaitBuffer(std::shared_ptr<CodecImpl> codecImpl);
    ExynosErrorType commonResetWaitBuffer(std::shared_ptr<CodecImpl> codecImpl);
    ExynosErrorType commonStop(std::shared_ptr<CodecImpl> codecImpl, ExynosPort::Port port);
    ExynosErrorType commonStreamOnOff(std::shared_ptr<CodecImpl> codecImpl, ExynosPort::Port port, ExynosPort::OnOff eSwitch);

    bool commonIsSBWCEnable(std::shared_ptr<CodecImpl> codecImpl);

    std::weak_ptr<CodecImpl> mCommonCodecImpl;
    bool mIsEncode;
};

#endif // EXYNOS_VIDEO_CODEC_H
