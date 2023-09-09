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
#ifndef EXYNOS_FILTER_PARAM_H
#define EXYNOS_FILTER_PARAM_H

#include <list>
#include <string>
#include <map>
#include <memory>

#include "ExynosDef.h"
#include "ExynosParam.h"

#include "ExynosLog.h"

#define TARGET_FILTER_ALL       0xFFFFFFFF
#define TARGET_OWNER_COMPONENT  0x00000000
#define FIRST_FILTER_ID 1

class ExynosFilterParamBase {
public:
    ExynosFilterParamBase() {
        mBase = nullptr;
    }

    virtual ~ExynosFilterParamBase() {
        mBase.reset();
    }

    virtual ParamIndex index() = 0;

    virtual std::string name() = 0;

    virtual bool registTargetFilter(uint32_t filter) = 0;

    virtual bool isOwner(uint32_t filter) = 0;

    std::shared_ptr<ExynosParamBase> getBaseParam() {
        return mBase;
    }

protected:
    std::shared_ptr<ExynosParamBase> mBase;
};

template<typename T>
class ExynosFilterParam : public ExynosFilterParamBase {
public:
    ExynosFilterParam() : mTargetFilter(0) {
        mBase = std::static_pointer_cast<ExynosParamBase>(std::make_shared<ExynosParam<T>>());
    }

    ExynosFilterParam(const std::shared_ptr<ExynosParam<T>> obj) : mTargetFilter(0) {
        mBase = std::static_pointer_cast<ExynosParamBase>(std::make_shared<ExynosParam<T>>(*obj.get()));
    }

    ~ExynosFilterParam() {
        mBase.reset();
    }

    ParamIndex index() override {
        return T::INDEX;
    }

    std::string name() override {
        return std::string(T::NAME);
    }

    bool registTargetFilter(uint32_t filter) override {
        uint32_t mask = (filter == TARGET_FILTER_ALL)? TARGET_FILTER_ALL:(1 << filter);

        if (mask == 0) {  /* underflow */
            return false;
        }

        mTargetFilter |= mask;

        return true;
    }

    bool isOwner(uint32_t filter) override {
        return (mTargetFilter & (1 << filter))? true:false;
    }

private:
     uint32_t mTargetFilter;
};

class ExynosFilterParams : public ExynosParams {
public:
    ExynosFilterParams() {
        mFilterParams.clear();
    }

    ~ExynosFilterParams() {
        mFilterParams.clear();
    }

    ExynosFilterParams& operator=(const ExynosFilterParams &rhs) {
        this->mParams = rhs.mParams;
        this->mFilterParams = rhs.mFilterParams;

        return *this;
    }

    void append(std::shared_ptr<ExynosFilterParams> params) {
        if (params.get() != nullptr) {
            for (auto it = params->mFilterParams.begin(); it != params->mFilterParams.end(); it++) {
                addParam(it->second);
            }
        }
    }

    bool addParam(std::list<std::shared_ptr<ExynosFilterParamBase>> params) {
        if (!params.empty()) {
            for (std::shared_ptr<ExynosFilterParamBase> &param : params) {
                if (addParam(param) == false) {
                    return false;
                }
            }

            return true;
        }

        return false;
    }

    bool addParam(std::shared_ptr<ExynosFilterParamBase> param) {
        if ((param.get() == nullptr) ||
            (param->getBaseParam() == nullptr)) {
            return false;
        }

        auto baseParam = param->getBaseParam();

        if (!ExynosParams::addParam(baseParam)) {
            return false;
        }

        /* already has a key. change latest value */
        auto ret = mFilterParams.insert_or_assign(baseParam->index(), param);
        if ((ret.second == false) && (ret.first != mFilterParams.end())) {
            StaticExynosLog(Level::Trace, "ExynosFilterParams", "[%s] already has key(%d) replace value in FilterParams", __FUNCTION__, baseParam->index());
            return true;
        }

        return ret.second;
    }

    std::shared_ptr<ExynosFilterParamBase> getParam(ParamIndex index, uint32_t filter) {
        auto it = mFilterParams.find(index);

        if (it != mFilterParams.end()) {
            auto param = it->second;

            if (param.get() != nullptr) {
                if (param->isOwner(filter)) {
                    return param;
                }
            }
        }

        return nullptr;
    }

    ParamIndex queryParam(uint32_t filter, int begin) {
        auto it = mFilterParams.begin();

        std::advance(it, begin);

        if (it != mFilterParams.end()) {
            auto param = it->second;

            if (param->isOwner(filter))
                return it->first;
        }

        return ExynosParamIndex::UnknownIndex;
    }

    bool empty() override {
        return mFilterParams.empty();
    }

    int size() override {
        return mFilterParams.size();
    }

    void clear() override {
        mFilterParams.clear();
        mParams.clear();
    }

private:
    std::map<ParamIndex, std::shared_ptr<ExynosFilterParamBase>> mFilterParams;
};

#endif // EXYNOS_FILTER_PARAM_H
