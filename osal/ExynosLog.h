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
#ifndef EXYNOS_LOG_H
#define EXYNOS_LOG_H

#include <string>
#include <stdarg.h>
#include <cstdarg>
#include <memory>

enum Level {
    Unknown   = 0,
    Error     = 1 << 0,
    Warning   = 1 << 1,
    Info      = 1 << 2,
    Essential = 1 << 3,
    Debug     = 1 << 4,
    Trace     = 1 << 5,
};

class ExynosLog {
public:
    ExynosLog();
    ExynosLog(std::string name);
    ~ExynosLog() = default;

    void ExynosLogPrint(Level level, const char *msg, ...);

protected:
    std::string mObjName = "\0";
    bool mbLogOff = true;

private:
    unsigned int mLogLevel;
};

class ExynosTraceObject : public ExynosLog {
public:
    ExynosTraceObject(bool bLogOff, const std::string objName, const char *funcName, const char *info) : ExynosLog(objName),
                                                                                                         mFuncName(funcName),
                                                                                                         mInfo(info) {
        mbLogOff = bLogOff;

        if (!mInfo.empty()) {
            ExynosLogPrint(Level::Trace, "[%s] In - info:%s", mFuncName.c_str(), mInfo.c_str());
        } else {
            ExynosLogPrint(Level::Trace, "[%s] In", mFuncName.c_str());
        }
    }

    ~ExynosTraceObject() {
        if (!mInfo.empty()) {
            ExynosLogPrint(Level::Trace, "[%s] Out - info:%s", mFuncName.c_str(), mInfo.c_str());
        } else {
            ExynosLogPrint(Level::Trace, "[%s] out", mFuncName.c_str());
        }
    }

    static bool isFunctionTrace();

private:
    std::string mFuncName;
    std::string mInfo;

    /* disable copy constructors */
    ExynosTraceObject() = delete;
};

#define CREATE_FUNCTRACE_OBJ(logoff, name) std::shared_ptr<ExynosTraceObject> trace_obj ## __LINE__ = nullptr; trace_obj ## __LINE__ = \
    (ExynosTraceObject::isFunctionTrace() == false)? nullptr: std::make_shared<ExynosTraceObject>(logoff, name, __FUNCTION__, "");
#define CREATE_FUNCTRACE_OBJ_WITH_INFO(logoff, name, info) std::shared_ptr<ExynosTraceObject> trace_obj ## __LINE__ = nullptr; trace_obj ## __LINE__ = \
    (ExynosTraceObject::isFunctionTrace() == false)? nullptr: std::make_shared<ExynosTraceObject>(logoff, name, __FUNCTION__, info);

void StaticExynosLogSkip(Level level, const char *objName, const char *msg, ...);
void StaticExynosLogUpdateLevel();
void StaticExynosLogPrint(Level level, const char *objName, const char *msg, ...);

#endif // EXYNOS_LOG_H

/* COMPILE TIME */
#undef StaticExynosLog
#undef SetStaticExynosLogLevel
#undef StaticExynosLogFunctionTrace
#undef ExynosLogE
#undef ExynosLogW
#undef ExynosLogI
#undef ExynosLogV
#undef ExynosLogD
#undef ExynosLogT
#undef ExynosLogFunctionTrace
#undef ExynosLogFunctionTraceWithInfo

#if defined(LOG_ON) || defined(THREAD_POOL_LOG_ON) || defined(FILTER_MANAGER_LOG_ON)
#define StaticExynosLog(level, objName, ...) StaticExynosLogPrint(level, objName, __VA_ARGS__)
#define SetStaticExynosLogLevel() StaticExynosLogUpdateLevel()
#define StaticExynosLogFunctionTrace(objName) CREATE_FUNCTRACE_OBJ(true, objName)

#define ExynosLogE(...) ExynosLogPrint(Level::Error,     __VA_ARGS__)
#define ExynosLogW(...) ExynosLogPrint(Level::Warning,   __VA_ARGS__)
#define ExynosLogI(...) ExynosLogPrint(Level::Info,      __VA_ARGS__)
#define ExynosLogD(...) ExynosLogPrint(Level::Essential, __VA_ARGS__)
#define ExynosLogV(...) ExynosLogPrint(Level::Debug,     __VA_ARGS__)
#define ExynosLogT(...) ExynosLogPrint(Level::Trace,     __VA_ARGS__)

#define ExynosLogFunctionTrace() CREATE_FUNCTRACE_OBJ(mbLogOff, mObjName)
#define ExynosLogFunctionTraceWithInfo(info) CREATE_FUNCTRACE_OBJ_WITH_INFO(mbLogOff, mObjName, info)
#else
#define StaticExynosLog(level, objName, ...) StaticExynosLogPrint(level, objName, __VA_ARGS__)
#define SetStaticExynosLogLevel() StaticExynosLogUpdateLevel()
#define StaticExynosLogFunctionTrace(objName) \
        do {                                          \
            (void)(objName);                          \
        } while (0)

#define ExynosLogE(...) ExynosLogPrint(Level::Error,                __VA_ARGS__)
#define ExynosLogW(...) ExynosLogPrint(Level::Warning,              __VA_ARGS__)
#define ExynosLogI(...) StaticExynosLogSkip(Level::Info, "",        __VA_ARGS__)
#define ExynosLogD(...) StaticExynosLogSkip(Level::Essential, "",   __VA_ARGS__)
#define ExynosLogV(...) StaticExynosLogSkip(Level::Debug, "",       __VA_ARGS__)
#define ExynosLogT(...) StaticExynosLogSkip(Level::Trace, "",       __VA_ARGS__)

#define ExynosLogFunctionTrace()
#define ExynosLogFunctionTraceWithInfo(info) ((void)info)
#endif  // LOG_ON || THREAD_POOL_LOG_ON || FILTER_MANAGER_LOG_ON

