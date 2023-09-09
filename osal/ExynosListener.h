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
#ifndef EXYNOS_LISTENER_H
#define EXYNOS_LISTENER_H

#include <memory>
#include <utility>
#include <string>

#include "ExynosDef.h"
#include "ExynosBuffer.h"

#define LOG_ON
#include "ExynosLog.h"

class ExynosListenerEvent {
public:
    typedef int32_t type_event_t;

    enum : type_event_t {
        EventError = 0,
        EventExtensionStart,
    };

    ExynosListenerEvent(type_event_t type = EventError) : mType(type) { }
    virtual ~ExynosListenerEvent() = default;

    type_event_t mType;
};

class ExynosListenerInterface {
public:
    ExynosListenerInterface() = default;
    virtual ~ExynosListenerInterface() = default;

    virtual bool processDone(ExynosBufferInfo input, ExynosBufferInfo output) = 0;
    virtual void eventReceived(std::shared_ptr<ExynosListenerEvent> event) = 0;
};

class ExynosListener : public ExynosLog {
public:
    static std::shared_ptr<ExynosListener> makeListener(std::weak_ptr<ExynosListenerInterface> const owner,
                                                        std::string name = "ExynosListener") {
        auto delfunc = [](ExynosListener *p) {
                           delete p;
                       };

        auto listener = std::shared_ptr<ExynosListener>(new ExynosListener(owner, name),
                                                        std::move(delfunc));

        return listener;
    }

    ~ExynosListener() = default;

    bool processDone(ExynosBufferInfo input, ExynosBufferInfo output) {
        ExynosLogFunctionTrace();

        std::lock_guard<std::mutex> lock(mMutex);

        if (mOwner.expired()) {
            return false;
        }

        auto shOwner = mOwner.lock();

        if (shOwner.get() != nullptr) {
            return shOwner->processDone(input, output);
        }

        return false;
    }

    void notifyEvent(std::shared_ptr<ExynosListenerEvent> event) {
        ExynosLogFunctionTrace();

        std::lock_guard<std::mutex> lock(mMutex);

        if (mOwner.expired()) {
            return;
        }

        auto shOwner = mOwner.lock();

        if (shOwner.get() != nullptr) {
            shOwner->eventReceived(event);
        }
    }

    void setName(std::string name) {
        mObjName = name + "-ExynosListener";
    }

private:
    ExynosListener(std::weak_ptr<ExynosListenerInterface> const owner, std::string name = "ExynosListener")
        : ExynosLog(name + "-ExynosListener"),
          mOwner(owner) {
          mbLogOff = false;
    }

    std::weak_ptr<ExynosListenerInterface> const mOwner;
    std::mutex mMutex;

    /* disable default constructor */
    ExynosListener() = delete;
};

#undef LOG_ON

#endif // EXYNOS_LISTENER_H
