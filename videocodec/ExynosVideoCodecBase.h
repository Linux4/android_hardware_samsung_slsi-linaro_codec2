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
#ifndef EXYNOS_VIDEO_CODEC_BASE_H
#define EXYNOS_VIDEO_CODEC_BASE_H

#include <memory>
#include <mutex>
#include <atomic>
#include <map>
#include <string>
#include <sys/mman.h>

#include "ExynosBuffer.h"
#include "ExynosBufferManager.h"

#include "ExynosVideoApi.h"

#define MAX_TAG_NUM 64 /* VIDEO_BUFFER_MAX_NUM(32) * 2 */


class ExynosPort {
public:
    enum OnOff {
        Off = 0,
        On  = 1,
    };

    enum Port {
        Input   = 0,
        Output  = 1,
        MaxPort = 2,
    };

    ExynosPort() : mIsConfigured(false), mIsOn(false), mBufManager(nullptr) {
    }

    ~ExynosPort() = default;

    bool mIsConfigured;
    std::mutex mStreamMutex;
    std::atomic_bool mIsOn;
    std::shared_ptr<SharedPtrManager> mBufManager;

protected:
private:
};

class ExynosStreamPort {
public:
    ExynosStreamPort() = default;

    virtual ~ExynosStreamPort() = default;

    virtual ExynosErrorType portStop(ExynosPort::Port port) = 0;

    bool isConfigured(ExynosPort::Port port = ExynosPort::MaxPort) {
        if (port == ExynosPort::MaxPort) {
            return (mExynosPort[ExynosPort::Input].mIsConfigured && mExynosPort[ExynosPort::Output].mIsConfigured);
        }

        return (mExynosPort[port].mIsConfigured);
    }

    ExynosErrorType getBufCount(ExynosPort::Port port, int &cnt) {
        cnt = 0;

        if (mExynosPort[port].mBufManager.get() != nullptr) {
            cnt = mExynosPort[port].mBufManager->size();
        }

        return EXYNOS_ERROR_NONE;
    }

    virtual ExynosErrorType getRefBufCount(int &cnt) {
        cnt = -1;

        return EXYNOS_ERROR_NONE;
    }

    virtual ExynosErrorType getRegBufCount(int &cnt) {
        cnt = -1;

        return EXYNOS_ERROR_NONE;
    }

    bool isStreamOn(ExynosPort::Port port) {
        std::lock_guard<std::mutex> lock(mExynosPort[port].mStreamMutex);
        return mExynosPort[port].mIsOn;
    }

protected:
    virtual ExynosErrorType streamOnOff(ExynosPort::Port port, ExynosPort::OnOff eSwitch) = 0;

    ExynosPort mExynosPort[ExynosPort::MaxPort];

private:
};

class ExynosVideoCodecBase : public ExynosLog, public ExynosStreamPort, public std::enable_shared_from_this<ExynosVideoCodecBase> {
public:
    enum class Type : int {
        UNKNOWN,
        H264,
        H264_SECURE,
        HEVC,
        HEVC_SECURE,
        MPEG4,
        H263,
        VP8,
        VP9,
        VP9_SECURE,
        MPEG2,
        VC1,
        WMV,
        AV1,
        AV1_SECURE,
        H264WFD,
        H264WFD_SECURE,
        HEVCWFD,
        HEVCWFD_SECURE,

        CODECMAX,
    };

    ExynosVideoCodecBase() = default;
    virtual ~ExynosVideoCodecBase() = default;

    /* function for ExynosVideoCodecBase's child class */
    virtual ExynosErrorType init() = 0;
    virtual void deinit() = 0;
    virtual ExynosErrorType srcSetup(ExynosBufferInfo input) = 0;
    virtual ExynosErrorType dstSetup() = 0;
    virtual ExynosErrorType srcEnqueue(ExynosBufferInfo &buf) = 0;
    virtual ExynosErrorType srcDequeue(ExynosBufferInfo &buf) = 0;
    virtual ExynosErrorType dstEnqueue(ExynosBufferInfo &buf) = 0;
    virtual ExynosErrorType dstDequeue(ExynosBufferInfo &buf) = 0;
    virtual ExynosErrorType waitBuffer(ExynosPort::Port port, bool &isAvail) = 0;
    virtual ExynosErrorType stopWaitBuffer() = 0;
    virtual ExynosErrorType resetWaitBuffer() = 0;
    virtual std::string getObjName() = 0;
    virtual ExynosErrorType checkRealTimeResource(int32_t width, int32_t height, int32_t operatingRate) = 0;

    /* add function for ExynosVideoCodecBase */
    virtual bool isSBWCEnable() = 0;

protected:
    uint32_t getVideoFormat(uint32_t pixel);
    uint32_t getPixelFormat(uint32_t video);
    uint32_t getPlaneCnt(uint32_t video);
    uint32_t getDataType(uint32_t video);

private:
};

typedef struct CodecInfo_t {
    std::string             objName;
    ExynosVideoCodingType   eCodecType;
    ExynosVideoSecurityType eSecurityType;
    ExynosVideoBoolType     bOTFMode;
} CodecInfo;

class CodecInfoRegistry {
public:
    CodecInfoRegistry() = default;
    virtual ~CodecInfoRegistry() {
        mCodecInfo->clear();
        mCodecInfo.reset();
    }

    virtual void registCodecInfo() = 0;

    CodecInfo getCodecInfo(ExynosVideoCodecBase::Type type) {
        CodecInfo unknownInfo = { "empty", VIDEO_CODING_UNKNOWN, VIDEO_NORMAL, VIDEO_FALSE };

        if (mCodecInfo.get() == nullptr) {
            return unknownInfo;
        }

        auto search = mCodecInfo->find(type);

        if (search == mCodecInfo->end()) {
            StaticExynosLog(Level::Error, "CodecInfoRegistry", "[%s] Not found Codec Info", __FUNCTION__);
            return unknownInfo;
        }

            return search->second;
    }

protected:
    std::shared_ptr<std::map<ExynosVideoCodecBase::Type, CodecInfo>> mCodecInfo;

private:
};

#endif // EXYNOS_VIDEO_CODEC_BASE_H

