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
#include "Exynos_External_Filter.h"

#define LOG_ON
#include "ExynosLog.h"
#define LOG_TAG "ExynosExternalFilter"


bool ExynosExternalFilter::doStart() {
    ExynosLogFunctionTrace();

    if ((!onStart()) ||
        (mExternalImpl.get() == nullptr)) {
        ExynosLogE("[%s] onStart() is failed", __FUNCTION__);
        return false;
    }

    if (!mInitialized) {
        if (mExternalImpl->load()) {
            mInitialized = true;
        }
    }

    mDebug = ExynosUtils::GetDebugType(mObjName);

    return true;
}

bool ExynosExternalFilter::doStop() {
    ExynosLogFunctionTrace();

    if (mExternalImpl.get() != nullptr) {
        if (mInitialized) {
            mExternalImpl->unload();
            mInitialized = false;
        }
    }

    return true;
}

bool ExynosExternalFilter::doFlush() {
    ExynosLogFunctionTrace();

    /* TODO */

    return true;
}

bool ExynosExternalFilter::doReset() {
    ExynosLogFunctionTrace();

    onReset();

    mExternalImpl.reset();
    mInitialized = false;

    return true;
}

bool ExynosExternalFilter::doProcess(std::shared_ptr<ExynosBuffer> buffer) {
    ExynosLogFunctionTrace();

    if (buffer.get() == nullptr) {
        /* invalid parameter */
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return false;
    }

    ExynosLogD("[%s] exynos buffer(%p)", __FUNCTION__, buffer.get());

    if (!(buffer->getFlags() & ExynosBuffer::REPLICA)) {
        if (mDebug & EXYNOS_DEBUG_INPUT) {
            ExynosBuffer::dump(buffer, mDebug, (mObjName + "-input"));
        }
    }

    /* apply configurations */
    onApplyConfig(buffer->mParams);

    bool ret = false;

    if ((!mInitialized) ||
        (buffer->getFlags() & ExynosBuffer::REPLICA)) {
        ret = bypassBuffer(buffer);

        ExynosLogV("[%s] bypass", __FUNCTION__);
    } else {
        ret = onProcess(buffer);
    }

    return ret;
}

