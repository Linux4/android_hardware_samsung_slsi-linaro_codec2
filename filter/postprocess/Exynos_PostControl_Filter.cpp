/*
 *
 * Copyright 2020 Samsung Electronics S.LSI Co. LTD
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
#include <dlfcn.h>

#include <system/graphics.h>
#include "exynos_format.h"

#include "Exynos_PostControl_Filter.h"

#define LOG_ON
#include "ExynosLog.h"
#define LOG_TAG "ExynosPostCtrlFilter"

/* HDR2SDR */
constexpr char HDR2SDR_LIB_NAME[]          = "libImageFormatConverter.so";

#ifdef USE_QUERY_HDR2SDR
constexpr char HDR2SDR_LIB_FN_NAME_QUERY[] = "CL_HDR2SDR_ARM_query";
#endif

typedef bool (*ConvertQueryFunc)();


/* FILMGRAIN */
constexpr char FILMGRAIN_LIB_NAME[] = "libFilmGrainNoise.so";


class ExynosPostCtrlFilter::ExynosHDR2SDRImpl : public ExynosLog {
public:
    ExynosHDR2SDRImpl(std::string name) : ExynosLog(name + "-Impl") {
        mbLogOff = false;

        mHandle      = nullptr;
        mQuery       = nullptr;

        mIsChecked  = false;
        mIsRequired = false;
    }

    ~ExynosHDR2SDRImpl() {
        unload();
    }

    bool load();
    void unload();
    bool query();

private:
    void                *mHandle;
    ConvertQueryFunc     mQuery;

    bool mIsChecked;
    bool mIsRequired;
};

bool ExynosPostCtrlFilter::ExynosHDR2SDRImpl::load() {
    ExynosLogFunctionTrace();

    if (mHandle != nullptr) {
        /* lib is already loaded */
        return true;
    }

    mHandle = dlopen(HDR2SDR_LIB_NAME, RTLD_NOW | RTLD_GLOBAL);
    if (mHandle == nullptr) {
        ExynosLogD("[%s] dlopen(%s) is failed. reason(%s)", __FUNCTION__, HDR2SDR_LIB_NAME, dlerror());
        return false;
    }

#ifdef USE_QUERY_HDR2SDR
    mQuery = (ConvertQueryFunc)dlsym(mHandle, HDR2SDR_LIB_FN_NAME_QUERY);

    if (mQuery == nullptr) {
        dlclose(mHandle);
        mHandle = nullptr;

        ExynosLogE("[%s] dlsym() is failed", __FUNCTION__);
        return false;
    }
#endif

    return true;
}

void ExynosPostCtrlFilter::ExynosHDR2SDRImpl::unload() {
    ExynosLogFunctionTrace();

    if (mHandle != nullptr) {
        mQuery = nullptr;

        dlclose(mHandle);
        mHandle = nullptr;
    }

    mIsChecked  = false;
    mIsRequired = false;
}

bool ExynosPostCtrlFilter::ExynosHDR2SDRImpl::query() {
    ExynosLogFunctionTrace();

    if (!mIsChecked) {
        mIsRequired = false;

        if (mQuery != nullptr) {
            mIsRequired = mQuery();
        }

        mIsChecked = true;
    }

    return mIsRequired;
}

class ExynosPostCtrlFilter::ExynosFilmGrainImpl : public ExynosLog {
public:
    ExynosFilmGrainImpl(std::string name) : ExynosLog(name + "-Impl") {
        mbLogOff = false;
        mHandle = nullptr;
    }

    ~ExynosFilmGrainImpl() {
        unload();
    }

    bool load() {
        ExynosLogFunctionTrace();

        if (mHandle != nullptr) {
            /* lib is already loaded */
            return true;
        }

        if (!ExynosUtils::GetFilmgrainType()) {
            return true;
        }

        mHandle = dlopen(FILMGRAIN_LIB_NAME, RTLD_NOW | RTLD_GLOBAL);
        if (mHandle == nullptr) {
            ExynosLogD("[%s] dlopen(%s) is failed. reason(%s)", __FUNCTION__, FILMGRAIN_LIB_NAME, dlerror());
            return false;
        }

        return true;
    }

    void unload() {
        ExynosLogFunctionTrace();

        if (mHandle != nullptr) {
            dlclose(mHandle);
            mHandle = nullptr;
        }
    }

    bool query() {
        ExynosLogFunctionTrace();

        return (mHandle != nullptr)? true:false;
    }

private:
    void *mHandle;
};

static void setActualFormatToCSCForHdr2Sdr(
    std::shared_ptr<ExynosFilterParams>  filterParams,
    uint32_t                             format,
    int32_t                              CSCFilterID,
    bool                                &bUseCSC) {
    bUseCSC = true;

    /* converting to P010M format for HDR2SDR */
    if ((format == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC) ||
        (format == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC) ||
        (format == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M) ||
        (format == HAL_PIXEL_FORMAT_YCBCR_P010)) {
        auto param = std::make_shared<ExynosFilterParam<ParamActualFormat>>();
        auto baseParam = std::static_pointer_cast<ExynosParam<ParamActualFormat>>(param->getBaseParam());

        baseParam->m.format = HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M;

        param->registTargetFilter(CSCFilterID);
        filterParams->addParam(param);
    }

    auto filterParam = filterParams->getParam(ExynosParamIndex::ActualFormatIndex, CSCFilterID);

    if (filterParam.get() != nullptr) {
        auto param = std::static_pointer_cast<ExynosParam<ParamActualFormat>>(filterParam->getBaseParam());

        /* (backward compatiblity: ~ android R) ignore thumbnail scenario in hdr2sdr
         * since android S, format will be YCBCR_420_888.
         */
        if (param->m.format == HAL_PIXEL_FORMAT_YV12) {
            /* update to src format */
            auto updateFilterParam = std::make_shared<ExynosFilterParam<ParamActualFormat>>();
            updateFilterParam->registTargetFilter(CSCFilterID);

            auto updateParam = std::static_pointer_cast<ExynosParam<ParamActualFormat>>(updateFilterParam->getBaseParam());
            updateParam->m.format = HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED;

            filterParams->addParam(updateFilterParam);

            StaticExynosLog(Level::Debug, "ExynosPostCtrlFilter", "[%s] actual format on CSC is changed to 0x%x from 0x%x",
                            __FUNCTION__, updateParam->m.format, param->m.format);
        }
    }

    return;
}

static void setActualFormatToCSCForFilmGrain(
    std::shared_ptr<ExynosFilterParams>  filterParams,
    uint32_t                             format,
    int32_t                              CSCFilterID,
    bool                                &bUseCSC) {
    /* if the format is YV12, change to YV12M */
    auto filterParam = filterParams->getParam(ExynosParamIndex::ActualFormatIndex, CSCFilterID);

    if (filterParam.get() != nullptr) {
        auto param = std::static_pointer_cast<ExynosParam<ParamActualFormat>>(filterParam->getBaseParam());

        /* (backward compatiblity: ~ android R) for thumbnail scenario
         * since android S, format will be YCBCR_420_888.
         */
        if (param->m.format == HAL_PIXEL_FORMAT_YV12) {
            /* update to YV12M */
            auto updateFilterParam = std::make_shared<ExynosFilterParam<ParamActualFormat>>();
            updateFilterParam->registTargetFilter(CSCFilterID);

            auto updateParam = std::static_pointer_cast<ExynosParam<ParamActualFormat>>(updateFilterParam->getBaseParam());
            updateParam->m.format = HAL_PIXEL_FORMAT_EXYNOS_YV12_M;

            filterParams->addParam(updateFilterParam);

            StaticExynosLog(Level::Debug, "ExynosPostCtrlFilter", "[%s] actual format on CSC is changed to 0x%x from 0x%x",
                            __FUNCTION__, updateParam->m.format, param->m.format);
            bUseCSC = true;
        }
    }

    /* (W/A) converting to decompressed format
     * generally, MFC never use SBWC format when filmgrain is valid
     * (W/A) converting to multi-fd format
     */
    auto reqFormat = [format]()->int {
        int reqFormat = format;

        switch (format) {
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC:
            [[fallthrough]];
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC:
            [[fallthrough]];
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN:
            reqFormat = HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M;
            break;
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC:
            [[fallthrough]];
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC:
            [[fallthrough]];
        case HAL_PIXEL_FORMAT_YCBCR_P010:
            reqFormat = HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M;
            break;
        default:
            break;
        }

        return reqFormat;
    }();

    if (format != reqFormat) {
        auto param = std::make_shared<ExynosFilterParam<ParamActualFormat>>();
        auto baseParam = std::static_pointer_cast<ExynosParam<ParamActualFormat>>(param->getBaseParam());

        baseParam->m.format = reqFormat;

        param->registTargetFilter(CSCFilterID);
        filterParams->addParam(param);
        bUseCSC = true;
    }

    return;
}

bool ExynosPostCtrlFilter::doStart() {
    ExynosLogFunctionTrace();

    if (mHDR2SDRImpl.get() == nullptr) {
        mHDR2SDRImpl = std::make_shared<ExynosHDR2SDRImpl>(mObjName);

        mHDR2SDRImpl->load();
    }

    if (mFilmGrainImpl.get() == nullptr) {
        mFilmGrainImpl = std::make_shared<ExynosFilmGrainImpl>(mObjName);

        mFilmGrainImpl->load();
    }

    return true;
}

bool ExynosPostCtrlFilter::doStop() {
    ExynosLogFunctionTrace();

    if (mHDR2SDRImpl.get() != nullptr) {
        mHDR2SDRImpl->unload();
    }

    if (mFilmGrainImpl.get() != nullptr) {
        mFilmGrainImpl->unload();
    }

    return true;
}

bool ExynosPostCtrlFilter::doFlush() {
    ExynosLogFunctionTrace();

    /* TODO */

    return true;
}

bool ExynosPostCtrlFilter::doReset() {
    ExynosLogFunctionTrace();

    mUsedFilterNum = 0;
    mHDR2SDRImpl.reset();
    mUseHDR2SDR = false;
    mFilmGrainImpl.reset();
    mUseFilmGrain = false;
    mHasFilmGrain = false;

    return true;
}

bool ExynosPostCtrlFilter::doProcess(std::shared_ptr<ExynosBuffer> buffer) {
    ExynosLogFunctionTrace();

    if (buffer.get() == nullptr) {
        /* invalid parameter */
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return false;
    }

    /* apply configurations */
    applyConfig(buffer->mParams);

    auto filterParams = std::static_pointer_cast<ExynosFilterParams>(buffer->mParams);
    if (filterParams.get() == nullptr) {
        ExynosLogD("[%s] filterParams is invalid. make filterParams", __FUNCTION__);

        filterParams = std::make_shared<ExynosFilterParams>();

        buffer->mParams = std::static_pointer_cast<ExynosParams>(filterParams);
    }

    auto format = buffer->format();

    /* To HDR2SDRFilter */
    if ((!mUseHDR2SDR) &&
        ((mHDR2SDRFilterID > 0) &&
         (mHDR2SDRImpl.get() != nullptr) &&
         (mHDR2SDRImpl->query() == true))) {
        auto param = std::make_shared<ExynosFilterParam<ParamHDR2SDR>>();
        auto baseParam = std::static_pointer_cast<ExynosParam<ParamHDR2SDR>>(param->getBaseParam());

        baseParam->m.enable = 1;

        ExynosLogD("[%s] hdr2sdr is used", __FUNCTION__);

        param->registTargetFilter(mHDR2SDRFilterID);
        filterParams->addParam(param);

        mUseHDR2SDR = true;
    }

    /* To CSCFilter */
    if (mCSCFilterID > 0) {
        if (mUseHDR2SDR) {
            setActualFormatToCSCForHdr2Sdr(filterParams, format, mCSCFilterID, mUseCSC);
        }

        if (mHasFilmGrain) {
            setActualFormatToCSCForFilmGrain(filterParams, format, mCSCFilterID, mUseCSC);
        }
    }

    /* for increasing number of internal buffer */
    int usedFilterNum = (mUseCSC && mUseFilmGrain)? 2:((mUseCSC || mUseFilmGrain)? 1:0);
    if (mUsedFilterNum < usedFilterNum) {
        auto param = std::make_shared<ExynosFilterParam<ParamEnabledFilterNum>>();
        auto baseParam = std::static_pointer_cast<ExynosParam<ParamEnabledFilterNum>>(param->getBaseParam());

        baseParam->m.num = usedFilterNum;
        mUsedFilterNum = usedFilterNum;

        param->registTargetFilter(TARGET_OWNER_COMPONENT);
        filterParams->addParam(param);
    }

    return bypassBuffer(buffer);
}

void ExynosPostCtrlFilter::applyConfig(std::shared_ptr<ExynosParams> params) {
    ExynosLogFunctionTrace();

    if ((params.get() == nullptr) ||
        params->empty()) {
        /* there is nothing to change */
        return;
    }

    auto filterParams = std::static_pointer_cast<ExynosFilterParams>(params);

    {
        auto filterParam = filterParams->getParam(ExynosParamIndex::FilterListInfoIndex, mID);
        if (filterParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamFilterListInfo>>(filterParam->getBaseParam());

            ExynosLogD("[%s] get the list of filters", __FUNCTION__);

            mFilterInfo = param->m.wkInfo;

            if (!mFilterInfo.expired()) {
                auto shFilterInfo = mFilterInfo.lock();

                /* check HDR2SDR filter */
                auto info = shFilterInfo->find("hdr2sdr");
                if (info != shFilterInfo->end()) {
                    mHDR2SDRFilterID = info->second;
                }

                /* check FilmGrain filter */
                info = shFilterInfo->find("filmgrain");
                if (info == shFilterInfo->end()) {
                    info = shFilterInfo->find("filmgrain.secure");
                }

                if (info != shFilterInfo->end()) {
                    mFilmGrainFilterID = info->second;
                }

                /* get ID of CSC filter */
                info = shFilterInfo->find("csc");
                if (info == shFilterInfo->end()) {
                    info = shFilterInfo->find("csc.secure");
                }

                if (info != shFilterInfo->end()) {
                    mCSCFilterID = info->second;
                }
            }
        }
    }

    {
        mHasFilmGrain = false;
        if ((mFilmGrainFilterID > 0) &&
            (mFilmGrainImpl.get() != nullptr)) {
            auto filterParam = filterParams->getParam(ExynosParamIndex::FilmGrainInfoIndex, mID);
            if (filterParam.get() != nullptr) {
                mUseFilmGrain = true;
                mHasFilmGrain = true;

                ExynosLogD("[%s] has film grain info", __FUNCTION__);

                if (!mFilmGrainImpl->query()) {
                    mUseFilmGrain = false;
                    mHasFilmGrain = false;
                    ExynosLogD("[%s] film grain is not supported", __FUNCTION__);
                }
            }
        }
    }

    if ((mHDR2SDRFilterID > 0) &&
        (mHDR2SDRImpl.get() != nullptr)) {
        auto filterParam = filterParams->getParam(ExynosParamIndex::HDR2SDRIndex, mHDR2SDRFilterID);
        if (filterParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamHDR2SDR>>(filterParam->getBaseParam());

            mUseHDR2SDR = (param->m.enable != 0)? true:false;

            if (mUseHDR2SDR) {
                ExynosLogD("[%s] hdr2sdr is used", __FUNCTION__);
            }
        }
    }

    return;
}

