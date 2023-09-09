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
#ifndef EXYNOS_CODECDECBASE_FILTER_H
#define EXYNOS_CODECDECBASE_FILTER_H

#include <memory>

#include "Exynos_CodecBase_Filter.h"
#include "ExynosDurationCalc.h"

#define LOG_ON
#include "ExynosLog.h"

#define DEFAULT_OUTBUF_NUM 8

class ExynosCodecDecBaseFilter : public ExynosCodecBaseFilter {
public:
    ExynosCodecDecBaseFilter(uint32_t id, bool isSecure = false) : ExynosCodecBaseFilter(id, isSecure), mNumMinDPB(DEFAULT_OUTBUF_NUM) {
        mObjName = "ExynosCodecDecBaseFilter";
        mThreadPool->setObjName(mObjName);

        memset(&mOutCrop, 0, sizeof(mOutCrop));
        mFormat = 0;
        mIsDecodeOrder = false;
        mRequestUpdate = false;
        mDelayableTimeMs = 0;
        mAllocDuration.clear();
    }

    virtual ~ExynosCodecDecBaseFilter() = default;

protected:
    /* function for ExynosCodecDecBaseFilter's child class */

    /* override function on ExynosCodecBaseFilter */
    bool onStart() override;
    virtual bool onSetup(std::shared_ptr<ExynosBuffer> buffer) override;
    bool onStop() override;
    bool onFlush() override;
    virtual void onApplyConfig(ExynosBufferInfo &info, std::shared_ptr<ExynosParams> params) override;
    virtual void onAdditionalWorkForParam(ExynosParamIndex index, std::shared_ptr<ExynosParamBase> param) override;
    virtual bool onProcessDone(ExynosBufferInfo input, ExynosBufferInfo output) override;
    virtual void onEventReceived(std::shared_ptr<ExynosListenerEvent> event) override;
    bool onFillOutBuffers() override;
    int onCheckNeedMoreBuffer() override;
    bool onSetInputBufferInfo(ExynosBufferInfo &input, std::shared_ptr<ExynosBuffer> buffer) override;

    bool outputBufferEnqueue(std::shared_ptr<ExynosBuffer> buffer);


    CropInfo mOutCrop;
    uint32_t mNumMinDPB;
    int      mFormat;
    bool     mIsDecodeOrder;
    bool     mRequestUpdate;

private:
    /* add function for ExynosCodecDecBaseFilter */
    bool allocOutBuffer();
    bool reconfigOutput();

    std::mutex          mReconfigMutex;
    ExynosDurationCalc  mAllocDuration;
    int32_t             mDelayableTimeMs;
};

#endif // EXYNOS_CODECDECBASE_FILTER_H
