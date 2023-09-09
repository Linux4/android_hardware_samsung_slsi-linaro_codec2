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
#ifndef EXYNOS_CSC_H
#define EXYNOS_CSC_H

#include <string>
#include <memory>

#include "ExynosDef.h"
#include "ExynosBuffer.h"

#define LOG_ON
#include "ExynosLog.h"

class CSCImpl : public ExynosLog {
public:
    CSCImpl() : ExynosLog("default") {
        mbLogOff = false;
    }

    virtual ~CSCImpl() = default;

    virtual bool run(ExynosBufferInfo &input, ExynosBufferInfo &output) = 0;
    virtual void setOperatingRate(uint32_t operatingRate) { UNUSED(operatingRate); }

private:
    virtual void updateActualDataSpace(unsigned int &dataspace) = 0;
};

class ExynosCSC : public ExynosLog {
public:
    enum class Type : int {
        UNUSED,
        SW,
        HW,
        HW_SECURE,
    };

    ExynosCSC(std::string name, Type type = Type::SW);

    ~ExynosCSC() {
        if (mImpl.get() != nullptr) {
            mImpl.reset();
        }
    }

    // bool setRotationInfo();
    bool process(ExynosBufferInfo input, ExynosBufferInfo output);
    void setOperatingRate(uint32_t operatingRate);

private:
    std::shared_ptr<CSCImpl> mImpl;

    ExynosCSC() = delete;
};

#undef LOG_ON

#endif // EXYNOS_CSC_H

