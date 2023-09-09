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
#ifndef EXYNOS_CODECENCBASE_FILTER_H
#define EXYNOS_CODECENCBASE_FILTER_H

#include <memory>

#include "Exynos_CodecBase_Filter.h"

#define LOG_ON
#include "ExynosLog.h"

#define DEFAULT_OUTBUF_NUM 5

class ExynosCodecEncBaseFilter : public ExynosCodecBaseFilter {
public:
    ExynosCodecEncBaseFilter(uint32_t id, bool isSecure = false) : ExynosCodecBaseFilter(id, isSecure), mNumOutput(DEFAULT_OUTBUF_NUM) {
        mObjName = "ExynosCodecEncBaseFilter";
        mThreadPool->setObjName(mObjName);

        mCSD = nullptr;
    }

    ~ExynosCodecEncBaseFilter() = default;

protected:
    /* override function on ExynosCodecBaseFilter */
    bool onStart() override;
    bool onSetup(std::shared_ptr<ExynosBuffer> buffer) override;
    bool onStop() override;
    bool onFlush() override;
    void onApplyConfig(ExynosBufferInfo &info, std::shared_ptr<ExynosParams> params) override;
    virtual void onAdditionalWorkForParam(ExynosParamIndex index, std::shared_ptr<ExynosParamBase> param) override;
    virtual bool onProcessDone(ExynosBufferInfo input, ExynosBufferInfo output) override;
    bool onFillOutBuffers() override;
    int onCheckNeedMoreBuffer() override;
    bool onSetInputBufferInfo(ExynosBufferInfo &input, std::shared_ptr<ExynosBuffer> buffer) override;

    uint32_t mNumOutput;
    std::shared_ptr<ExynosBuffer> mCSD;

private:
    /* add function for ExynosCodecEncBaseFilter */
    bool allocOutBuffer();
};

#endif // EXYNOS_CODECENCBASE_FILTER_H
