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
#ifndef EXYNOS_THREAD_POOL_H
#define EXYNOS_THREAD_POOL_H

#include <memory>
#include <thread>
#include <vector>

#include <mutex>
#include <list>
#include <condition_variable>

#include <future>
#include <functional>
#include <stdexcept>
#include <chrono>
#include <string>
#include <utility>

#include "ExynosDef.h"

// #define THREAD_POOL_LOG_ON
#include "ExynosLog.h"

#ifdef THREAD_POOL_LOG_ON
#define LOG_ONOFF false
#else
#define LOG_ONOFF true
#endif

#define MAX_FUTURE_TIMEOUT_MS    0 // ms

class ExynosThreadPool : public ExynosLog {
public:
    inline ExynosThreadPool(size_t num = 1, std::string name = "ExynosThreadPool") : ExynosThreadPool(false, num, name) {
    }

    inline ExynosThreadPool(bool useSession = false, size_t num = 1, std::string name = "ExynosThreadPool") : ExynosLog(name + "-ThreadPool"),
                                                                                     mExit(false),
                                                                                     mIsSessionMode(useSession) {
        mbLogOff = LOG_ONOFF;

        ExynosLogFunctionTrace();

        mSessionNumber = std::make_shared<SessionNumber>();

        for (size_t i = 0; i < num; i++) {
            /* create a thread */
            mThreads.emplace_back(
                [this]() {
                    while (!this->mExit) {
                        std::shared_ptr<TASK_PACKAGE> task = nullptr;
                        {
                            std::unique_lock<std::mutex> lock(this->mTaskMutex);

                            auto condfunc = [this]()->bool {
                                                bool ret = true;

                                                /* wait for getting a task */
                                                if ((!this->mExit) &&
                                                    (this->mTasks.empty())) {
                                                    ret = false;
                                                }

                                                return ret;
                                            };

                            this->mTaskCondition.wait(lock, std::move(condfunc));

                            if ((this->mExit) &&
                                (this->mTasks.empty())) {
                                break;
                            }

                            /* get a task */
                            if (mIsSessionMode) {
                                for (auto it = this->mTasks.begin(); it != this->mTasks.end(); it++) {
                                    if ((*it)->mSession == this->mSessionNumber->getCur()) {
                                        task = std::move(*it);
                                        this->mTasks.erase(it);
                                        break;
                                    }
                                }
                            } else {
                                task = std::move(this->mTasks.front());
                                this->mTasks.pop_front();
                            }
                        }

                        /* run a task */
                        if (task.get() != nullptr) {
                            ExynosLogV("[%s] run a task :: name(%s)", __FUNCTION__, (task->mName->size() > 0)? task->mName->c_str():"unnamed");

                            std::function<void(bool)> fn = std::move(task->mFn);
                            fn(false);  /* run */
                        }
                        std::this_thread::yield();
                    }
                });
        }
    }

    inline ~ExynosThreadPool() {
        ExynosLogFunctionTrace();

        stop();
        mThreads.clear();
        mSessionNumber.reset();
    }

    inline void flush() {
        ExynosLogFunctionTrace();

        std::lock_guard<std::mutex> lock(mCancelMutex);
        {
            std::unique_lock<std::mutex> lock(mTaskMutex);

            /* remove all piled elements */
            while (!mTasks.empty()) {
                auto task = std::move(this->mTasks.front());
                mTasks.pop_front();

                if (task.get() != nullptr) {
                    ExynosLogT("[%s] discard a task :: name(%s)", __FUNCTION__, (task->mName->size() > 0)? task->mName->c_str():"unnamed");

                    std::function<void(bool)> fn = std::move(task->mFn);
                    fn(true);  /* discard */
                }
            }
        }
    }

    inline void stop() {
        ExynosLogFunctionTrace();

        {
            std::unique_lock<std::mutex> lock(mTaskMutex);
            mExit = true;
        }

        /* awake all threads */
        mTaskCondition.notify_all();

        if (mThreads.size() <= 0) {
            /* obj is already released */
            return;
        }

        /* wait for thread termination */
        for (std::thread &thread : mThreads) {
            if (thread.get_id() != std::this_thread::get_id()) {  /* prevent a deadlock from resource release on shared ptr */
                if (thread.joinable() == true) {
                    thread.join();
                }

                mThreads.erase(mThreads.begin());
            }
        }

        flush();
    }

    template<class F, class... Args>
    decltype(auto) post(F &&f, Args&&... args);

    template<class F, class... Args>
    decltype(auto) post(F &&f, std::function<void()> notify, Args&&... args);

    template<class F, class... Args>
    decltype(auto) post(std::string name, F &&f, Args&&... args);

    template<class F, class... Args>
    decltype(auto) post(std::string name, F &&f, std::function<void()> notify, Args&&... args);

    template<class F, class... Args>
    decltype(auto) postToCurSession(F &&f, Args&&... args);

    template<class F, class... Args>
    decltype(auto) postToCurSession(std::string name, F &&f, Args&&... args);

    template<class F, class... Args>
    bool toss(F &&f, Args&&... args);

    template<class F, class... Args>
    bool toss(F &&f, std::function<void()> notify, Args&&... args);

    template<class F, class... Args>
    bool toss(std::string name, F &&f, Args&&... args);

    template<class F, class... Args>
    bool toss(std::string name, F &&f, std::function<void()> notify, Args&&... args);

    template<class F, class... Args>
    bool tossToCurSession(F &&f, Args&&... args);

    template<class F, class... Args>
    bool tossToCurSession(std::string name, F &&f, Args&&... args);

    inline void incNextSession() {
        if ((mIsSessionMode) &&
            (mSessionNumber.get() != nullptr)) {
            mSessionNumber->incNext();
        }
    }

    inline void incCurSession() {
        if ((mIsSessionMode) &&
            (mSessionNumber.get() != nullptr)) {
            mSessionNumber->incCur();
        }
    }

    inline uint64_t getCurSession() {
        if ((mIsSessionMode) &&
            (mSessionNumber.get() != nullptr)) {
            mSessionNumber->getCur();
        }

        return 0;
    }

    inline void clearSession() {
        if ((mIsSessionMode) &&
            (mSessionNumber.get() != nullptr)) {
            mSessionNumber->clear();
        }
    }

    void setObjName(std::string name) {
        mObjName = name + "-ThreadPool";
    }

private:
    class TASK_PACKAGE {
    public:
        TASK_PACKAGE(std::function<void(bool)> fn, std::shared_ptr<std::string> name) : mFn(std::move(fn)), mName(name) {
            mSession = 0;
        }

        std::function<void(bool)> mFn;
        std::shared_ptr<std::string> mName;
        uint64_t mSession;
    };

    std::shared_ptr<TASK_PACKAGE> makeTaskPackge(std::function<void(bool)> wrappedTask,
                                                 std::function<void()> notify, std::string name) {
        std::shared_ptr<TASK_PACKAGE> deliverTask = nullptr;

        if (notify == nullptr) {
            deliverTask = std::shared_ptr<TASK_PACKAGE>(new ExynosThreadPool::TASK_PACKAGE(std::move(wrappedTask),
                                                        std::make_shared<std::string>(name)));
        } else {
            auto delfunc = [notify](ExynosThreadPool::TASK_PACKAGE *p) {
                                if (notify != nullptr) {
                                    notify();
                                }

                                delete(p);
                           };

            deliverTask = std::shared_ptr<TASK_PACKAGE>(new ExynosThreadPool::TASK_PACKAGE(std::move(wrappedTask),
                                                        std::make_shared<std::string>(name)), std::move(delfunc));
        }

        return deliverTask;
    }

    bool pushTaskPackge(std::shared_ptr<TASK_PACKAGE> deliverTask, uint64_t session) {
        std::lock_guard<std::mutex> lock(mCancelMutex);

        /* push a task to list */
        {
            std::unique_lock<std::mutex> lock(mTaskMutex);

            if (mIsSessionMode) {
                deliverTask->mSession = session;
            }

            if (!mExit) {
                mTasks.push_back(std::move(deliverTask));
            } else {
                return false;
            }
        }

        mTaskCondition.notify_one();

        return true;
    }

    class SessionNumber {
    public:
        SessionNumber() : mCurSession(0), mNextSession(0) {
        }

        ~SessionNumber() = default;

        void incCur() {
            std::lock_guard<std::mutex> lock(mMutex);

            mCurSession++;
            return;
        }

        void incNext() {
            std::lock_guard<std::mutex> lock(mMutex);

            mNextSession++;
            return;
        }

        uint64_t getCur() {
            std::lock_guard<std::mutex> lock(mMutex);

            return mCurSession;
        }

        uint64_t getNext() {
            std::lock_guard<std::mutex> lock(mMutex);

            return mNextSession;
        }

        void clear() {
            std::lock_guard<std::mutex> lock(mMutex);

            mCurSession = 0;
            mNextSession = 0;
            return;
        }

    private:
        std::mutex mMutex;
        uint64_t mCurSession;
        uint64_t mNextSession;
    };

    template<class F, class... Args>
    decltype(auto) postToSession(uint64_t session, std::string name, F &&f,
                                    std::function<void()> notify, Args&&... args);

    template<class F, class... Args>
    bool tossToSession(uint64_t session, std::string name, F &&f,
                                    std::function<void()> notify, Args&&... args);

    bool mExit;
    std::vector<std::thread> mThreads;

    std::mutex mTaskMutex;
    std::condition_variable mTaskCondition;
    std::list<std::shared_ptr<TASK_PACKAGE>> mTasks;

    bool mIsSessionMode;
    std::shared_ptr<SessionNumber> mSessionNumber = nullptr;

    std::mutex mCancelMutex;
};

template<class F, class... Args>
decltype(auto) ExynosThreadPool::post(F &&f, Args&&... args) {
    std::string name = "unnamed";
    std::function<void()> notify = nullptr;

    return post(name, std::forward<F>(f), std::move(notify), std::forward<Args>(args)...);
}

template<class F, class... Args>
decltype(auto) ExynosThreadPool::post(F &&f, std::function<void()> notify, Args&&... args) {
    std::string name = "unnamed";
    return post(name, std::forward<F>(f), std::move(notify), std::forward<Args>(args)...);
}

template<class F, class... Args>
decltype(auto) ExynosThreadPool::post(std::string name, F &&f, Args&&... args) {
    std::function<void()> notify = nullptr;

    return postToSession(mSessionNumber->getNext(), name, std::forward<F>(f), std::move(notify), std::forward<Args>(args)...);
}

template<class F, class... Args>
decltype(auto) ExynosThreadPool::post(std::string name, F &&f, std::function<void()> notify, Args&&... args) {
    return postToSession(mSessionNumber->getNext(), name, std::forward<F>(f), std::move(notify), std::forward<Args>(args)...);
}

template<class F, class... Args>
decltype(auto) ExynosThreadPool::postToCurSession(F &&f, Args&&... args) {
    std::string name = "unnamed";
    std::function<void()> notify = nullptr;

    return postToSession(mSessionNumber->getCur(), name, std::forward<F>(f), std::forward<Args>(args)...);
}

template<class F, class... Args>
decltype(auto) ExynosThreadPool::postToCurSession(std::string name, F &&f, Args&&... args) {
    std::function<void()> notify = nullptr;

    return postToSession(mSessionNumber->getCur(), name, std::forward<F>(f), std::move(notify), std::forward<Args>(args)...);
}

template<class F, class... Args>
bool ExynosThreadPool::toss(F &&f, Args&&... args) {
    std::string name = "unnamed";
    std::function<void()> notify = nullptr;

    return toss(name, std::forward<F>(f), std::move(notify), std::forward<Args>(args)...);
}

template<class F, class... Args>
bool ExynosThreadPool::toss(F &&f, std::function<void()> notify, Args&&... args) {
    std::string name = "unnamed";
    return toss(name, std::forward<F>(f), std::move(notify), std::forward<Args>(args)...);
}

template<class F, class... Args>
bool ExynosThreadPool::toss(std::string name, F &&f, Args&&... args) {
    std::function<void()> notify = nullptr;

    return tossToSession(mSessionNumber->getNext(), name, std::forward<F>(f), std::move(notify), std::forward<Args>(args)...);
}

template<class F, class... Args>
bool ExynosThreadPool::toss(std::string name, F &&f, std::function<void()> notify, Args&&... args) {
    return tossToSession(mSessionNumber->getNext(), name, std::forward<F>(f), std::move(notify), std::forward<Args>(args)...);
}

template<class F, class... Args>
bool ExynosThreadPool::tossToCurSession(F &&f, Args&&... args) {
    std::string name = "unnamed";
    std::function<void()> notify = nullptr;

    return tossToSession(mSessionNumber->getCur(), name, std::forward<F>(f), std::forward<Args>(args)...);
}

template<class F, class... Args>
bool ExynosThreadPool::tossToCurSession(std::string name, F &&f, Args&&... args) {
    std::function<void()> notify = nullptr;

    return tossToSession(mSessionNumber->getCur(), name, std::forward<F>(f), std::move(notify), std::forward<Args>(args)...);
}

template<class F, class... Args>
decltype(auto) ExynosThreadPool::postToSession(
    uint64_t session,
    std::string name,
    F &&f,
    std::function<void()> notify,
    Args&&... args) {
    ExynosLogFunctionTraceWithInfo(name.c_str());

    using __type_of_return = std::invoke_result_t<F, Args...>;

#ifdef USE_C2A_VERSION
    auto mainTask = std::make_shared<std::packaged_task<__type_of_return(bool)>>(
                            [f = std::forward<F>(f), ... args = std::forward<Args>(args)](bool bCancel) mutable -> __type_of_return {
                               if (bCancel)  /* discard this work */
                                   return __type_of_return(0);

                                return f(args ...);  /* run */
                        });
#else
    auto mainTask = std::make_shared<std::packaged_task<__type_of_return(bool)>>(
                            [f = std::forward<F>(f), args = std::make_tuple(std::forward<Args>(args) ...)](bool bCancel) mutable -> __type_of_return {
                               if (bCancel)  /* discard this work */
                                   return __type_of_return(0);

                                return std::apply([f = std::forward<F>(f)](auto&& ... args) mutable -> decltype(auto) {
                                                        return f(args ...);  /* run */
                                                    }, std::move(args));
                        });
#endif

    /*
     * get a result undecided
     * a result decided will be obtained using result.get()
     */
    std::future<__type_of_return> result = mainTask->get_future();

    /* abstract tasks */
    auto voidReturnFunction = [mainTask](bool bCancel) {
                                   (*mainTask)(bCancel);
                               };

    /* create abstracted task */
    std::function<void(bool)> wrappedTask(std::move(voidReturnFunction));

    std::shared_ptr<TASK_PACKAGE> deliverTask = makeTaskPackge(std::move(wrappedTask), notify, name);

    if (false == pushTaskPackge(deliverTask, session)) {
        std::future<__type_of_return> err;
        return err;
    }

    return result;
}

template<class F, class... Args>
bool ExynosThreadPool::tossToSession(
    uint64_t session,
    std::string name,
    F &&f,
    std::function<void()> notify,
    Args&&... args) {
    ExynosLogFunctionTraceWithInfo(name.c_str());

    using __type_of_return = std::invoke_result_t<F, Args...>;

#ifdef USE_C2A_VERSION
    auto mainTask = std::make_shared<std::function<__type_of_return(bool)>>(
                            [f = std::forward<F>(f), ... args = std::forward<Args>(args)](bool bCancel) mutable -> __type_of_return {
                               if (bCancel)  /* discard this work */
                                   return __type_of_return(0);

                                return f(args ...);  /* run */
                        });
#else
    auto mainTask = std::make_shared<std::function<__type_of_return(bool)>>(
                            [f = std::forward<F>(f), args = std::make_tuple(std::forward<Args>(args) ...)](bool bCancel) mutable -> __type_of_return {
                               if (bCancel)  /* discard this work */
                                   return __type_of_return(0);

                                return std::apply([f = std::forward<F>(f)](auto&& ... args) mutable -> decltype(auto) {
                                                        return f(args ...);  /* run */
                                                    }, std::move(args));
                        });
#endif

    /* abstract tasks */
    auto voidReturnFunction = [mainTask](bool bCancel) {
                                   (*mainTask)(bCancel);
                               };

    /* create abstracted task */
    std::function<void(bool)> wrappedTask(std::move(voidReturnFunction));

    std::shared_ptr<TASK_PACKAGE> deliverTask = makeTaskPackge(std::move(wrappedTask), notify, name);

    return pushTaskPackge(deliverTask, session);
}

template<class T>
inline auto WaitGetResultFromFuture(std::future<T> &ret, T err, uint32_t ms = MAX_FUTURE_TIMEOUT_MS)
->decltype(auto) {
    StaticExynosLogFunctionTrace("ExynosThreadPool");

    if (ret.valid() == true) {
        if (ms == 0) {  /* blocking */
            return ret.get();
        } else {  /* set timedout */
            auto status = ret.wait_for(std::chrono::milliseconds(ms));

            if (status == std::future_status::ready) {
                return ret.get();
            }

            /* can not get a result within given time */
            // if ((status == std::future_status::deferred) ||
            //     (status == std::future_status::timeout)) {
                return err;
            // }
        }
    }

    return err;
}

#undef THREAD_POOL_LOG_ON
#undef LOG_ON
#undef LOG_ONOFF

#endif // EXYNOS_THREAD_POOL_H
