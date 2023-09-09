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
#include <map>
#include <log/log.h>
#include <cutils/properties.h>

#include "ExynosDef.h"
#include "ExynosLog.h"

static std::map<std::string, unsigned int> LevelMap = {
    { std::string("disable"),   Level::Unknown },
    { std::string("default"),   (Level::Error | Level::Warning | Level::Info) },
    { std::string("error"),     Level::Error },
    { std::string("warning"),   (Level::Error | Level::Warning) },
    { std::string("info"),      (Level::Error | Level::Warning | Level::Info) },
    { std::string("essential"), (Level::Error | Level::Warning | Level::Info | Level::Essential) },
    { std::string("debug"),     (Level::Error | Level::Warning | Level::Info | Level::Essential | Level::Debug) },
    { std::string("trace"),     (Level::Error | Level::Warning | Level::Info | Level::Essential | Level::Debug | Level::Trace) }
};

static unsigned int gLogLevel = (Level::Error | Level::Warning | Level::Info);  /* default */

/* static */
bool ExynosTraceObject::isFunctionTrace() {
    return (gLogLevel & Level::Trace);
}

ExynosLog::ExynosLog() {
    mLogLevel = gLogLevel;
}

ExynosLog::ExynosLog(std::string name) : mObjName(name) {
    mLogLevel = gLogLevel;
}

void ExynosLog::ExynosLogPrint(Level level, const char *msg, ...) {
    if ((mbLogOff == true) ||
        !(mLogLevel & level)) {
        /* discard log message */
        return;
    }

    va_list argptr;
    va_start(argptr, msg);

    switch (level) {
    case Level::Error:
        __android_log_vprint(ANDROID_LOG_ERROR, mObjName.c_str(), msg, argptr);
        break;
    case Level::Warning:
        __android_log_vprint(ANDROID_LOG_WARN, mObjName.c_str(), msg, argptr);
        break;
    case Level::Info:
        __android_log_vprint(ANDROID_LOG_INFO, mObjName.c_str(), msg, argptr);
        break;
    case Level::Essential:
        __android_log_vprint(ANDROID_LOG_INFO, mObjName.c_str(), msg, argptr);
        break;
    case Level::Debug:
        __android_log_vprint(ANDROID_LOG_DEBUG, mObjName.c_str(), msg, argptr);
        break;
    case Level::Trace:
        __android_log_vprint(ANDROID_LOG_VERBOSE, mObjName.c_str(), msg, argptr);
        break;
    default:
        __android_log_vprint(ANDROID_LOG_VERBOSE, mObjName.c_str(), msg, argptr);
        break;
    }

    va_end(argptr);

    return;
}

void StaticExynosLogSkip(Level level, const char *objName, const char *msg, ...) {
    va_list argptr;
    va_start(argptr, msg);
    (void)(level);
    (void)(objName);
    (void)(msg);
    (void)(argptr);
    va_end(argptr);
    return;
}

void StaticExynosLogUpdateLevel() {
    char prop[PROPERTY_VALUE_MAX] = { 0, };

    property_get("vendor.debug.c2.level", prop, "default");

    auto it = LevelMap.find(std::string(prop));

    if (it != LevelMap.end()) {
        gLogLevel = it->second;
    } else {
        gLogLevel = (Level::Error | Level::Warning | Level::Info);
    }
}

void StaticExynosLogPrint(Level level, const char *objName, const char *msg, ...) {
    if (!(gLogLevel & level)) {
        /* discard log message */
        return;
    }

    va_list argptr;
    va_start(argptr, msg);

    switch (level) {
    case Level::Error:
        __android_log_vprint(ANDROID_LOG_ERROR, objName, msg, argptr);
        break;
    case Level::Warning:
        __android_log_vprint(ANDROID_LOG_WARN, objName, msg, argptr);
        break;
    case Level::Info:
        __android_log_vprint(ANDROID_LOG_INFO, objName, msg, argptr);
        break;
    case Level::Essential:
        __android_log_vprint(ANDROID_LOG_INFO, objName, msg, argptr);
        break;
    case Level::Debug:
        __android_log_vprint(ANDROID_LOG_DEBUG, objName, msg, argptr);
        break;
    case Level::Trace:
        __android_log_vprint(ANDROID_LOG_VERBOSE, objName, msg, argptr);
        break;
    default:
        __android_log_vprint(ANDROID_LOG_VERBOSE, objName, msg, argptr);
        break;
    }

    va_end(argptr);

    return;
}

