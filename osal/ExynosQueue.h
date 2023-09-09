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
#ifndef EXYNOS_QUEUE_H
#define EXYNOS_QUEUE_H

#include <mutex>
#include <list>
#include <unordered_map>
#include <functional>
#include <utility>

template<class T>
class ExynosQueue {
public:
    ExynosQueue() = default;

    ~ExynosQueue() {
        std::lock_guard<std::mutex> lock(mMutex);

        mList.clear();
    }

    void enqueue(T &element, std::function<bool(T&)> condfunc = nullptr) {
        std::lock_guard<std::mutex> lock(mMutex);

        if ((!mList.empty()) &&
            (condfunc != nullptr)) {
            for (auto it = mList.begin(); it != mList.end(); ) {
                if (condfunc(*it) == true) {
                    it = mList.erase(it);
                } else {
                    it++;
                }
            }
        }

        mList.push_back(element);
    }

    void enqueue(T &&element, std::function<bool(T&)> condfunc = nullptr) {
        std::lock_guard<std::mutex> lock(mMutex);

        if ((!mList.empty()) &&
            (condfunc != nullptr)) {
            for (auto it = mList.begin(); it != mList.end(); ) {
                if (condfunc(*it) == true) {
                    it = mList.erase(it);
                } else {
                    it++;
                }
            }
        }

        mList.push_back(std::move(element));
    }

    bool dequeue(T &element, std::function<bool(T&)> condfunc = nullptr) {
        std::lock_guard<std::mutex> lock(mMutex);

        if (!mList.empty()) {
            if (condfunc != nullptr) {
                for (auto it = mList.begin(); it != mList.end(); it++) {
                    if (condfunc(*it) == true) {
                        element = (*it);
                        mList.erase(it);
                        return true;
                    }
                }
            } else {
                element = mList.front();
                mList.pop_front();
                return true;
            }
        }

        return false;
    }

    bool front(T &element) {
        return find(element);
    }

    bool find(T &element, std::function<bool(T&)> condfunc = nullptr) {
        std::lock_guard<std::mutex> lock(mMutex);

        if (!mList.empty()) {
            if (condfunc != nullptr) {
                for (auto it = mList.begin(); it != mList.end(); it++) {
                    if (condfunc(*it) == true) {
                        element = (*it);
                        return true;
                    }
                }
            } else {
                element = mList.front();
                return true;
            }
        }

        return false;
    }

    int size() {
        std::lock_guard<std::mutex> lock(mMutex);

        return mList.size();
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mMutex);

        return mList.empty();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mMutex);

        mList.clear();
    }

private:
    std::mutex mMutex;
    std::list<T> mList;
};

template<class T, class R>
class ExynosMap {
public:
    ExynosMap() = default;

    ~ExynosMap() {
        std::lock_guard<std::mutex> lock(mMutex);

        mMap.clear();
    }

    void enqueue(T key, R &element) {
        std::lock_guard<std::mutex> lock(mMutex);

        mMap.insert_or_assign(key, element);
    }

    void enqueue(T key, R &&element) {
        std::lock_guard<std::mutex> lock(mMutex);

        mMap.insert_or_assign(key, std::move(element));
    }

    bool dequeue(T key, R &element) {
        std::lock_guard<std::mutex> lock(mMutex);

        if (!mMap.empty()) {
            auto node = mMap.extract(key);
            if (!node.empty()) {
                element = std::move(node.mapped());
                return true;
            }
        }

        return false;
    }

    bool dequeue(R &element, std::function<bool(R&)> condfunc) {
        std::lock_guard<std::mutex> lock(mMutex);

        if (!mMap.empty()) {
            if (condfunc != nullptr) {
                for (auto it = mMap.begin(); it != mMap.end(); it++) {
                    if (condfunc(it->second) == true) {
                        auto node = mMap.extract(it);
                        if (!node.empty()) {
                            element = std::move(node.mapped());
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    int size() {
        std::lock_guard<std::mutex> lock(mMutex);

        return mMap.size();
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mMutex);

        return mMap.empty();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mMutex);

        mMap.clear();
    }

private:
    std::mutex mMutex;
    std::unordered_map<T, R> mMap;
};

#endif // EXYNOS_QUEUE_H
