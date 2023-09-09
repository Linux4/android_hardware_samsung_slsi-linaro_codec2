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
#ifndef EXYNOS_C2_UTILS_H
#define EXYNOS_C2_UTILS_H

#include <string>
#include <memory>

#include <C2Component.h>
#include <C2Config.h>
#include <util/C2InterfaceHelper.h>
#include <media/stagefright/foundation/MediaDefs.h>

using namespace android;

template<typename T, typename ...Args>
std::shared_ptr<T> AllocSharedString(const Args(&... args), const char *str) {
    size_t len = strlen(str) + 1;
    std::shared_ptr<T> ret = T::AllocShared(len, args...);
    strcpy(ret->m.value, str);
    return ret;
}

template<typename T, typename ...Args>
std::shared_ptr<T> AllocSharedString(const Args(&... args), const std::string &str) {
    std::shared_ptr<T> ret = T::AllocShared(str.length() + 1, args...);
    strcpy(ret->m.value, str.c_str());
    return ret;
}

template <typename T>
struct Setter {
    typedef typename std::remove_reference<T>::type type;

    static C2R NonStrictValueWithNoDeps(
            bool mayBlock, C2InterfaceHelper::C2P<type> &me) {
        (void)mayBlock;
        return me.F(me.v.value).validatePossible(me.v.value);
    }

    static C2R NonStrictValuesWithNoDeps(
            bool mayBlock, C2InterfaceHelper::C2P<type> &me) {
        (void)mayBlock;
        C2R res = C2R::Ok();
        for (size_t ix = 0; ix < me.v.flexCount(); ++ix) {
            res.plus(me.F(me.v.m.values[ix]).validatePossible(me.v.m.values[ix]));
        }
        return res;
    }

    static C2R StrictValueWithNoDeps(
            bool mayBlock,
            const C2InterfaceHelper::C2P<type> &old,
            C2InterfaceHelper::C2P<type> &me) {
        (void)mayBlock;
        if (!me.F(me.v.value).supportsNow(me.v.value)) {
            me.set().value = old.v.value;
        }
        return me.F(me.v.value).validatePossible(me.v.value);
    }
};

#endif // EXYNOS_C2_UTILS_H
