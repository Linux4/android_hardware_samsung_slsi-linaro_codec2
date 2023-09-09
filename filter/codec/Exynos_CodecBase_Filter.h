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
#ifndef EXYNOS_CODECBASE_FILTER_H
#define EXYNOS_CODECBASE_FILTER_H

#include <memory>
#include <chrono>
#include <vector>

#include "Exynos_Filter.h"
#include "ExynosVideoCodec.h"
#include "ExynosMutex.h"
#include "ExynosDef.h"

#define LOG_ON
#include "ExynosLog.h"

using namespace std::chrono_literals;
#define ALLOC_RETRY_TIME 1000ms
#define WAIT_ALLOC_TIME 10ms

class ExynosCodecBaseFilter : public ExynosFilterBase/*, public std::enable_shared_from_this<ExynosCodecBaseFilter>*/ {
public:
    ExynosCodecBaseFilter(uint32_t id, bool isSecure = false) : ExynosFilterBase(id, isSecure), mIsConfigured(false), mCodec(nullptr) {
        mObjName = "ExynosCodecBaseFilter";
        mThreadPool->setObjName(mObjName);

        mWidth = 0;
        mHeight = 0;
        mNumMaxOutput = 0;
        mCodecType = ExynosVideoCodec::Type::UNKNOWN;
        mOperatingRate = 0;

        mReqAllocCnt = std::make_shared<uint32_t>(0);
    }

    virtual ~ExynosCodecBaseFilter() {
        mListener.reset();

        if (mAllocThreadPool.get() != nullptr) {
            mAllocThreadPool->flush();
            mAllocThreadPool->stop();
            mAllocThreadPool.reset();
        }

        if (mCodec.get() != nullptr) {
            mCodec->stopAllThreadPool();
            mCodec.reset();
        }
    }

protected:
    /* function for ExynosCodecBaseFilter's child class */
    virtual bool onStart() = 0;
    virtual bool onSetup(std::shared_ptr<ExynosBuffer> buffer) = 0;
    virtual bool onStop() = 0;
    virtual bool onFlush() = 0;
    virtual bool onProcess(std::shared_ptr<ExynosBuffer> input);
    virtual void onApplyConfig(ExynosBufferInfo &info, std::shared_ptr<ExynosParams> params);
    virtual void onAdditionalWorkForParam(ExynosParamIndex index, std::shared_ptr<ExynosParamBase> param);
    virtual bool onProcessDone(ExynosBufferInfo input, ExynosBufferInfo output) = 0;
    virtual void onEventReceived(std::shared_ptr<ExynosListenerEvent> event);
    virtual bool onFillOutBuffers() = 0;
    virtual int  onCheckNeedMoreBuffer() = 0;
    virtual bool onSetInputBufferInfo(ExynosBufferInfo &input, std::shared_ptr<ExynosBuffer> buffer) = 0;

    bool fillOutBuffers(uint32_t num = 0);
    void applyFilterParams(ExynosBufferInfo &info, std::vector<ExynosParamIndex> indexList,
                           std::shared_ptr<ExynosFilterParams> filterParams);

    /* function for thread pool owned by self */
    bool doFillOutBuffers();

    /* graphic buffer : DEC(output), ENC(input) */
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mNumMaxOutput;
    uint32_t mOperatingRate;

    bool mIsConfigured;

    ExynosVideoCodec::Type mCodecType;
    std::shared_ptr<ParallelProcessingVideoCodec> mCodec;

    std::shared_ptr<ExynosThreadPool> mAllocThreadPool;

    /* for managing number of task for allocating output buffer.
     * uses attribute about reference count about shared pointer(smart pointer)
     */
    std::shared_ptr<uint32_t> mReqAllocCnt;

private:
    /* override function on ExynosListerInterface */
    bool processDone(ExynosBufferInfo input, ExynosBufferInfo output) override;
    void eventReceived(std::shared_ptr<ExynosListenerEvent> event) override;

    /* override function on ExynosFilterBase. function for thread pool owned by self */
    bool doStart() override;
    bool doStop() override;
    bool doReset() override;
    bool doFlush() override;
    bool doProcess(std::shared_ptr<ExynosBuffer> buffer) override;
    bool doWorkDone(std::unique_ptr<FilterWork> work) override;

    /* sync with alloc thread pool */
    bool doStopSync();
    bool doFlushSync();

    bool checkRealTimeResource(int32_t width, int32_t height, int32_t operatingRate) override;
};

#endif // EXYNOS_CODECBASE_FILTER_H
