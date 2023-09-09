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
#ifndef EXYNOS_EXTERNAL_FILTER_H
#define EXYNOS_EXTERNAL_FILTER_H

#include <string>
#include <memory>

#include "Exynos_Filter.h"
#include "ExynosDef.h"

#define LOG_ON
#include "ExynosLog.h"

class ExynosExternalImpl : public ExynosLog {
public:
    ExynosExternalImpl(std::string name) : ExynosLog(name + "-Impl") {
        mbLogOff = false;
    }

    virtual ~ExynosExternalImpl() = default;

    virtual bool load() = 0;
    virtual void unload() = 0;
};

class ExynosExternalFilter : public ExynosFilterBase/*, public std::enable_shared_from_this<ExynosExternalFilter>*/ {
public:
    ExynosExternalFilter(uint32_t id, bool isSecure = false) : ExynosFilterBase(id, isSecure) {
        mObjName = "ExynosExternalFilter";
        mbLogOff = false;
        mThreadPool->setObjName(mObjName);

        mInitialized    = false;
        mExternalImpl   = nullptr;
    }

    virtual ~ExynosExternalFilter() {
        mExternalImpl.reset();
    }

protected:
    /* function for ExynosExternalFilter's child class */
    virtual bool onStart() = 0;
    virtual bool onReset() = 0;
    virtual bool onProcess(std::shared_ptr<ExynosBuffer> buffer) = 0;
    virtual void onApplyConfig(std::shared_ptr<ExynosParams> params) = 0;

    /* override function on ExynosListerInterface */

    /* override function on ExynosFilterBase. function for thread pool owned by self */
    bool doStart() override;
    bool doStop() override;
    bool doFlush() override;
    bool doReset() override;

    bool doProcess(std::shared_ptr<ExynosBuffer> buffer) override;

    /* add function for ExynosExternalFilter */

    bool mInitialized;
    std::shared_ptr<ExynosExternalImpl> mExternalImpl;

private:
};

#endif // EXYNOS_EXTERNAL_FILTER_H
