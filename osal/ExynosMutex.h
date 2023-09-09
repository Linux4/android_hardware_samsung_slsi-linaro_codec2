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
#ifndef EXYNOS_MUTEX_H
#define EXYNOS_MUTEX_H

#include <mutex>

template<class T>
class ExynosMutex {
public:
    template<typename ...Args>
    ExynosMutex(Args... args): mObj(args...) { }

    ~ExynosMutex() = default;

    class LockObj {
    public:
        LockObj(ExynosMutex<T> &mutex): mIsLocked(false),
                                        mMutex(mutex.mMutex),
                                        mObj(&(mutex.mObj)) {
            lock();
        }

        LockObj(LockObj &&obj): mIsLocked(obj.mIsLocked),
                                mMutex(obj.mMutex),
                                mObj(obj.mObj) {
            obj.mIsLocked = false;
            obj.mObj = nullptr;

            lock();
        }

        ~LockObj() {
            unlock();
        }

        void lock() {
            if (mIsLocked == false) {
                mMutex.lock();
                mIsLocked = true;
            }
        }

        void unlock() {
            if (mIsLocked == true) {
                mIsLocked = false;
                mMutex.unlock();
            }
        }

        T* get() const {
            return (mIsLocked == true)? mObj:nullptr;
        }

        T* operator->() const {
            return (mIsLocked == true)? mObj:nullptr;
        }

        T& operator*() const {
            return (mIsLocked == true)? (*mObj):(*(T*)nullptr);
        }

    private:
        bool mIsLocked;
        std::mutex &mMutex;
        T *mObj;

        /* disable copy constructors */
        LockObj(const LockObj&) = delete;
        void operator=(const LockObj&) = delete;
    };

    LockObj lock() {
        return LockObj(*this);
    }

private:
    friend class LockObj;

    std::mutex mMutex;
    T mObj;

    /* disable copy constructors */
    ExynosMutex(const ExynosMutex<T>&) = delete;
    void operator=(const ExynosMutex<T>&) = delete;
};

#endif // EXYNOS_MUTEX_H
