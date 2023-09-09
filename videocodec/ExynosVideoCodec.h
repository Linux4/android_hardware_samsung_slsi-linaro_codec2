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
#ifndef EXYNOS_VIDEO_CODEC_H
#define EXYNOS_VIDEO_CODEC_H

#include <memory>
#include <mutex>
#include <atomic>

#include "ExynosThreadPool.h"
#include "ExynosQueue.h"
#include "ExynosMutex.h"
#include "ExynosSignal.h"
#include "ExynosListener.h"
#include "ExynosDef.h"

#include "ExynosVideoCodecBase.h"

#define LOG_ON
#include "ExynosLog.h"


class ExynosVideoCodecEvent : public ExynosListenerEvent {
public:
    enum : type_event_t {
        EventResolutionChanged = EventExtensionStart,
    };

    ExynosVideoCodecEvent(type_event_t type) : ExynosListenerEvent(type) { }
    virtual ~ExynosVideoCodecEvent() = default;
};

class ExynosVideoCodec : public ExynosLog, public std::enable_shared_from_this<ExynosVideoCodec> {
public:
    enum class Type : int {
        UNKNOWN,
        DECODE_START    = 1,
        H264DEC         = DECODE_START,
        HEVCDEC,
        MPEG4DEC,
        H263DEC,
        VP8DEC,
        VP9DEC,
        MPEG2DEC,
        VC1DEC,
        WMVDEC,
        AV1DEC,

        SECURE_DEC_START = 15,
        H264DEC_SECURE   = SECURE_DEC_START,
        HEVCDEC_SECURE,
        VP9DEC_SECURE,
        AV1DEC_SECURE,

        ENCODE_START     = 30,
        H264ENC          = ENCODE_START,
        HEVCENC,
        MPEG4ENC,
        H263ENC,
        VP8ENC,
        VP9ENC,
        H264WFDENC,
        HEVCWFDENC,

        SECURE_ENC_START = 45,
        H264ENC_SECURE   = SECURE_ENC_START,
        HEVCENC_SECURE,
        H264WFDENC_SECURE,
        HEVCWFDENC_SECURE,

        CODECMAX,
    };

    ExynosVideoCodec(Type type);

    virtual ~ExynosVideoCodec();

    /* add function for ExynosVideoCodec */
    bool setCallback(std::shared_ptr<ExynosListener> listener);
    bool setup(ExynosBufferInfo input);
    bool resetup();
    bool getOutputInfo(uint32_t &width, uint32_t &height, uint32_t &count);
    bool getOutputInfo(uint32_t &width, uint32_t &height, uint32_t &count, int &format);
    bool getOutCropInfo(CropInfo &crop);
    int  getReqOutBufCount();
    int  getInBufCount();
    int  getOutBufCount();
    int  getRefBufCount();
    int  getRegBufCount();
    int  getExtraBufNum();
    ExynosVideoCodec::Type getCodecType();

    virtual bool inputEnqueue(ExynosBufferInfo input);
    virtual bool outputEnqueue(ExynosBufferInfo output);
    virtual bool flush();
    virtual bool waitDequeue(bool isInput);

    bool inputDequeue();
    bool outputDequeue();
    bool isSBWCEnable();

    bool checkRealTimeResource(int32_t width, int32_t height, int32_t operatingRate);

protected:
    virtual bool clearOutputBuffers();

    std::shared_ptr<ExynosVideoCodecBase> mCodec;
    ExynosMutex<ExynosMap<uint32_t, ExynosBufferInfo>> mInputs;  // key: Tag
    ExynosMutex<ExynosMap<uint64_t, ExynosBufferInfo>> mOutputs; // key: ExynosBuffer address

    bool mIsEncoder;
    std::atomic<bool> mFlush;
    std::shared_ptr<ExynosSignal> mSyncSignal;

private:
    /* add function for ExynosVideoCodec */

    Type mType = Type::UNKNOWN;
    std::weak_ptr<ExynosListener> mNotify;

    typedef struct BufferInfo {
        uint32_t nWidth;
        uint32_t nHeight;
        CropInfo stCrop;
        uint32_t nCnt;
        int nFormat;
    } BufferInfo;
    ExynosMutex<BufferInfo> mBufferInfo;

    /* disable default constructor */
    ExynosVideoCodec() = delete;
};

class ParallelProcessingVideoCodec : public ExynosVideoCodec/*, public std::enable_shared_from_this<ExynosVideoCodec>*/ {
public:
    ParallelProcessingVideoCodec(Type type);
    ~ParallelProcessingVideoCodec();

    bool inputEnqueue(ExynosBufferInfo input) override;
    bool outputEnqueue(ExynosBufferInfo output) override;
    bool flush() override;
    void stopAllThreadPool();

protected:
    bool clearOutputBuffers() override;

private:
    /* function for thread pool owned by self */
    int getAvailPipeInputCount();

    bool doPipeInputEnqueue(ExynosBufferInfo input);
    bool doInputEnqueue(ExynosBufferInfo input);
    bool doOutputEnqueue(ExynosBufferInfo output);
    bool doWaitInputDequeue();
    bool doWaitOutputDequeue();
    bool doFlush();
    bool doStop();
    bool doPortStop(ExynosPort::Port port);

    /* add function for ExynosVideoCodec */
    bool waitDequeue(bool isInput) override;

    ExynosMutex<ExynosQueue<ExynosBufferInfo>> mPipeInputs;

    std::shared_ptr<ExynosThreadPool> mEnqueueThread;
    std::shared_ptr<ExynosThreadPool> mInputDequeueThread;
    std::shared_ptr<ExynosThreadPool> mOutputDequeueThread;

    std::shared_ptr<uint32_t> mInputFeedTaskCnt;

    ParallelProcessingVideoCodec() = delete;
};

#undef LOG_ON

#endif // EXYNOS_VIDEO_CODEC_H
