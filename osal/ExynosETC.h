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
#ifndef EXYNOS_ETC_H
#define EXYNOS_ETC_H

#include <string>
#include <memory>
#include <utility>

#include "ExynosDef.h"
#include "ExynosLog.h"

#define STR_INDIR(x) #x
#define TO_STRING(x) STR_INDIR(x)

#define CHECK_POINTER(x) ({                 \
    if (x == NULL) {                        \
        StaticExynosLog(Level::Error, "Check", "[%s] : [%s] Line %s, Name: %s", __FILE__, __FUNCTION__, TO_STRING(__LINE__), TO_STRING(x)); \
    }                                       \
    (x != NULL);                            \
})

#define CHECK_EQUAL(x, y) ({                \
    if (x != y) {                           \
        StaticExynosLog(Level::Error, "Check", "[%s] : [%s] Line %s, %s vs %s, not equal", __FILE__, __FUNCTION__, TO_STRING(__LINE__), TO_STRING(x), TO_STRING(y)); \
    }                                       \
    (x == y);                               \
})

template<typename T>
inline std::optional<std::shared_ptr<T>> checkSharedPtr(std::shared_ptr<T> &shPtr, std::string str, bool bLogOn) {
    if (shPtr.get() == nullptr) {
        if (bLogOn) {
            StaticExynosLog(Level::Error, "Check", "%s, obj is released", str.c_str());
        }

        return std::nullopt;
    }

    return { shPtr };
}

#define CHECK_SHARED_PTR(x) ({              \
    std::string ___full;                    \
    ___full.append("[");                    \
    ___full.append(__FILE__);               \
    ___full.append("] : [");                \
    ___full.append(__FUNCTION__);           \
    ___full.append("] Line: ");             \
    ___full.append(TO_STRING(__LINE__));    \
    ___full.append(", Name: ");             \
    ___full.append(TO_STRING(x));           \
    checkSharedPtr(x, ___full, true);       \
})

#define CHECK_SHARED_PTR_NOLOG(x) ({        \
    std::string ___full;                    \
    checkSharedPtr(x, ___full, false);      \
})

template<typename T>
inline bool checkUniquePtr(std::unique_ptr<T> &uqPtr, std::string str) {
    if (uqPtr.get() == nullptr) {
        StaticExynosLog(Level::Error, "Check", "%s, obj is released", str.c_str());
        return false;
    }

    return true;
}

#define CHECK_UNIQUE_PTR(x) ({              \
    std::string ___full;                    \
    ___full.append("[");                    \
    ___full.append(__FILE__);               \
    ___full.append("] : [");                \
    ___full.append(__FUNCTION__);           \
    ___full.append("] Line: ");             \
    ___full.append(TO_STRING(__LINE__));    \
    ___full.append(", Name: ");             \
    ___full.append(TO_STRING(x));           \
    checkUniquePtr(x, ___full);             \
})

template<typename T>
inline std::shared_ptr<T> getSharedPtrFromWeakPtr(std::weak_ptr<T> const &wkPtr, std::string str, bool bLogOn) {
    if (wkPtr.expired() == true) {
        if (bLogOn)
            StaticExynosLog(Level::Error, "Check", "%s, weak ptr expired", str.c_str());
        return nullptr;
    }

    auto shPtr = wkPtr.lock();
    if (shPtr.get() == nullptr) {
        if (bLogOn)
            StaticExynosLog(Level::Error, "Check", "%s, shared ptr is nullptr", str.c_str());
        return nullptr;
    }

    return shPtr;
}

#define GET_SHARED_PTR(x) ({                     \
    std::string ___full;                         \
    ___full.append("[");                         \
    ___full.append(__FILE__);                    \
    ___full.append("] : [");                     \
    ___full.append(__FUNCTION__);                \
    ___full.append("] Line: ");                  \
    ___full.append(TO_STRING(__LINE__));         \
    ___full.append(", Name: ");                  \
    ___full.append(TO_STRING(x));                \
    getSharedPtrFromWeakPtr(x, ___full, true);   \
})

#define GET_SHARED_PTR_NOLOG(x) ({                \
    std::string ___full;                          \
    getSharedPtrFromWeakPtr(x, ___full, false);   \
})


template<typename T>
inline std::shared_ptr<T> make_shared_array(int size) {
    return std::shared_ptr<T>(new T[size], [](T *p) { delete[] p; });
}

template<typename T>
inline std::shared_ptr<T> make_shared_buffer(int size) {
    return std::shared_ptr<T>((T *)malloc(sizeof(T)*size), [](T *p) { free(p); });
}

template<class T>
inline std::shared_ptr<T> wrapShared(T t) {
    return std::make_shared<T>(std::move(t));
}

template<class T>
inline std::shared_ptr<T> copyShared(std::shared_ptr<T> t) {
    std::shared_ptr<T> tmp = t;
    return tmp;
}

template<typename R, typename T>
R check_weak_pointer_and_run(R ret, std::function<R(std::shared_ptr<T>)> f, std::weak_ptr<T> weak) {
    if (!weak.expired()) {
        auto ptr = weak.lock();
        if (ptr.get() != nullptr) {
            return f(ptr);
        }
    }

    return std::forward<R>(ret);
}

template<class R, class F, class G>
inline std::function<R()> weak_pointer_bind(R ret, F&& f, std::weak_ptr<G> weak) {
    std::function<R()> retFunc = [ret = (R)ret, f = std::forward<F>(f), weak = (std::weak_ptr<G>)weak]() -> R {
                                        if (!weak.expired()) {
                                            auto ptr = weak.lock();
                                            if (ptr.get() != nullptr) {
                                                return (ptr.get()->*f)();
                                            }
                                        }

                                        return (ret);
                                    };
    return retFunc;
}

template<class R, class F, class G, class... Args>
inline std::function<R()> weak_pointer_bind(R ret, F&& f, std::weak_ptr<G> weak, Args&&... args) {
#ifdef USE_C2A_VERSION
    std::function<R()> retFunc = [ret = (R)ret, f = std::forward<F>(f), weak = (std::weak_ptr<G>)weak, ... args = std::forward<Args>(args)]() -> R {
                                        if (!weak.expired()) {
                                            auto ptr = weak.lock();
                                            if (ptr.get() != nullptr) {
                                                return (ptr.get()->*f)(args ...);
                                            }
                                        }

                                        return (ret);
                                    };
    return retFunc;
#else
    std::function<R()> retFunc = [ret = (R)ret, f = std::forward<F>(f), weak = (std::weak_ptr<G>)weak, args = std::make_tuple(std::forward<Args>(args) ...)]() -> R {
                                        if (!weak.expired()) {
                                            auto ptr = weak.lock();
                                            if (ptr.get() != nullptr) {
                                                return std::apply([ptr, f](auto&& ... args) mutable -> decltype(auto) {
                                                                                return (ptr.get()->*f)(args ...);
                                                                            }, std::move(args));
                                            }
                                        }

                                        return (ret);
                                    };
    return retFunc;
#endif
}

// Consider a class template partial specialization
template<typename T>
void check_weak_pointer_and_run(std::function<void(std::shared_ptr<T>)> f, std::weak_ptr<T> weak) {
    if (!weak.expired()) {
        auto ptr = weak.lock();
        if (ptr.get() != nullptr) {
            return f(ptr);
        }
    }

    return;
}

template<class F, class G>
inline std::function<void()> weak_pointer_bind(F&& f, std::weak_ptr<G> weak) {
    std::function<void()> retFunc = [f = std::forward<F>(f), weak = (std::weak_ptr<G>)weak]() {
                                        if (!weak.expired()) {
                                            auto ptr = weak.lock();
                                            if (ptr.get() != nullptr) {
                                                return (ptr.get()->*f)();
                                            }
                                        }

                                        return;
                                    };
    return retFunc;
}

template<class F, class G, class... Args>
inline std::function<void()> weak_pointer_bind(F&& f, std::weak_ptr<G> weak, Args&&... args) {
#ifdef USE_C2A_VERSION
    std::function<void()> retFunc = [f = std::forward<F>(f), weak = (std::weak_ptr<G>)weak, ... args = std::forward<Args>(args)]() {
                                        if (!weak.expired()) {
                                            auto ptr = weak.lock();
                                            if (ptr.get() != nullptr) {
                                                return (ptr.get()->*f)(args ...);
                                            }
                                        }

                                        return;
                                    };
    return retFunc;
#else
    std::function<void()> retFunc = [f = std::forward<F>(f), weak = (std::weak_ptr<G>)weak, args = std::make_tuple(std::forward<Args>(args) ...)]() {
                                        if (!weak.expired()) {
                                            auto ptr = weak.lock();
                                            if (ptr.get() != nullptr) {
                                                return std::apply([ptr, f](auto&& ... args) mutable {
                                                                                return (ptr.get()->*f)(args ...);
                                                                            }, std::move(args));
                                            }
                                        }

                                        return;
                                    };
    return retFunc;
#endif
}

template<typename T>
class bool_guard {
public:
    bool_guard(T &variable, bool val) {
        mVariable = &variable;
        /* store */
        (*mVariable) = val;
    }

    ~bool_guard() {
        /* restore */
        (*mVariable) = !(*mVariable);
    }

private:
    T *mVariable;

    bool_guard() = delete;
};

namespace ExynosUtils {
    void GetYUVPlaneInfo(int width, int height, int format, int &cnt, unsigned int size[BASE_BUFFER_MAX_PLANES], uint64_t flags = 0);
    void GetYUVImageInfo(int width, int height, int format, int &cnt, unsigned int size[BASE_BUFFER_MAX_PLANES], uint64_t flags = 0);
    int GetPlaneCnt(int format);
    int GetImagePlaneCnt(int format);
    bool CheckRGB(const int format);
    bool CheckFullRange(const int dataspace);
    bool CheckBT601(const int dataspace);
    bool CheckBT709(const int dataspace);
    bool CheckBT2020(const int dataspace);
    bool CheckCompressedFormat(const int format);
    bool Check10BitFormat(const int format);
    int GetSupportedDataspaceOnGPU();
    void GetColorAspectsFromDataspace(int &range, int &primaries, int &transfer, int &matrix, const int dataspace);
    int GetDefaultDataSpaceIfNeeded(int width, int height);
    int GetDataspaceFromColorAspects(int range, int primaries, int transfer, int matrix);
    int GetDataspaceFromColorAspects(int width, int height, const ExynosColorAspects &CA);
    uint32_t GetOutputSizeForEnc(uint32_t width, uint32_t height);
    uint32_t GetOutputSizeForEncSecure(uint32_t width, uint32_t height);
    ExynosDebugType GetDebugType(const std::string& name);
    uint32_t GetCompressedColorType();
    uint32_t GetMinQuality();
    bool GetFilmgrainType();
    uint64_t GetUsageType();
}; // namespace ExynosUtils

#endif // EXYNOS_ETC_H
