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
#ifndef EXYNOS_C2_COMPONENT_RM_H
#define EXYNOS_C2_COMPONENT_RM_H

#include <mutex>
#include <list>
#include <memory>

#include <media/stagefright/xmlparser/MediaCodecsXmlParser.h>

#define LOG_ON
#include "ExynosLog.h"

class ExynosC2ComponentRM : public ExynosLog {
public:
    enum ResourceType : uint32_t {
        DECODER = 0,
        ENCODER,
        SECURE,
        RESOURCE_MAX
    };


    using ComponentResource = std::shared_ptr<uint32_t>;

    ~ExynosC2ComponentRM() = default;

    static std::shared_ptr<ExynosC2ComponentRM> getInstance();
    ComponentResource getResource(ResourceType type);

private:
    ExynosC2ComponentRM();

    std::mutex mMutex;
    ComponentResource mResources[RESOURCE_MAX];

    const char *mResourceNames[RESOURCE_MAX] = {
        "Decoder",
        "Encoder",
        "Secure",
    };

    ExynosC2ComponentRM(const ExynosC2ComponentRM&) = delete;
    ExynosC2ComponentRM& operator= (const ExynosC2ComponentRM&) = delete;
};

class ExynosC2ComponentInfo : public ExynosLog {
public:
    ~ExynosC2ComponentInfo() = default;

    static const std::shared_ptr<ExynosC2ComponentInfo> getInstance();

    android::MediaCodecsXmlParser::CodecMap& getCodecMap() {
        return (*(mCodecMap.get()));
    }

private:
    ExynosC2ComponentInfo();

    std::shared_ptr<android::MediaCodecsXmlParser::CodecMap>    mCodecMap;
    std::shared_ptr<android::MediaCodecsXmlParser::RoleMap>     mRoleMap;

    ExynosC2ComponentInfo(const ExynosC2ComponentInfo&) = delete;
    ExynosC2ComponentInfo& operator= (const ExynosC2ComponentInfo&) = delete;
};

#endif // EXYNOS_C2_COMPONENT_RM_H
