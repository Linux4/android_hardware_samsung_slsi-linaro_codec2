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
#ifndef EXYNOS_GDC_WRAPPER_H
#define EXYNOS_GDC_WRAPPER_H

#include <string>
#include <memory>

#include "ExynosDef.h"
#include "ExynosBuffer.h"

#define LOG_ON
#include "ExynosLog.h"

class ExynosGDCWrapper : public ExynosLog {
public:
    ExynosGDCWrapper(std::string name) : ExynosLog(name + "-GDC") {
        mImpl = nullptr;
        mbLogOff = false;
    }

    ~ExynosGDCWrapper() {
        if (mImpl.get() != nullptr) {
            mImpl.reset();
        }
    }

    bool process(ExynosBufferInfo input, ExynosBufferInfo output);
    bool flush();

private:
    class GDCImpl;
    std::shared_ptr<GDCImpl> mImpl;

    ExynosGDCWrapper() = delete;
};

#undef LOG_ON

#endif // EXYNOS_GDC_WRAPPER_H

