/*
 *
 * Copyright 2021 Samsung Electronics S.LSI Co. LTD
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

#include "Exynos_HDR10PlusStatEnc_Filter.h"

#define LOG_ON
#include "ExynosLog.h"
#define LOG_TAG "ExynosHDR10PlusStatEncFilter"

#define MAX_ALLOC_BUFFER_NUM EXTRA_INTERNAL_BUFFER_NUM

#ifndef HDR_DYNAMIC_META_LIB
#define LIB_NAME "libHDRMetaGenerator.so"
#else
#define LIB_NAME HDR_DYNAMIC_META_LIB
#endif  // HDR_DYNAMIC_META_LIB

#include "Hdr10PlusMetaInterface.h"

class ExynosHDR10PlusStatEncImpl : public ExynosExternalImpl {
public:
    ExynosHDR10PlusStatEncImpl(std::string name) : ExynosExternalImpl(name) {
        mbLogOff = false;

        mHandle      = nullptr;
        mCreate      = nullptr;
        mDestroy     = nullptr;
        mInterface   = nullptr;

        mIsConfigured = false;
    }

    ~ExynosHDR10PlusStatEncImpl() {
        unload();
    }

    bool load() override;
    void unload() override;
    bool run(std::shared_ptr<ExynosBuffer> buffer, HDRStatInfo &stat);
    bool run(std::shared_ptr<ExynosBuffer> buffer, HDRStatInfo &stat,
                ExynosHdrDynamicInfo &dynamic);
private:
    bool init();
    void deinit();
    bool writeToSEI(std::shared_ptr<ExynosBuffer> buffer, HDRStatInfo &stat,
                        ExynosHdrData_ST2094_40 &info);

    void                            *mHandle;
    CreateHdr10PlusMetaGenIntfFunc   mCreate;
    DestroyHdr10PlusMetaGenIntfFunc  mDestroy;
    Hdr10PlusMetaInterface          *mInterface;

    bool mIsConfigured;
};

bool ExynosHDR10PlusStatEncImpl::load() {
    ExynosLogFunctionTrace();

    if (mHandle != nullptr) {
        /* lib is already loaded */
        return true;
    }

    mHandle = dlopen(LIB_NAME, RTLD_NOW | RTLD_GLOBAL);
    if (mHandle == nullptr) {
        ExynosLogD("[%s] dlopen(%s) is failed. reason(%s)", __FUNCTION__, LIB_NAME, dlerror());
        return false;
    }

    mCreate  = (CreateHdr10PlusMetaGenIntfFunc)dlsym(mHandle, LIB_FN_NAME_CREATE);
    mDestroy = (DestroyHdr10PlusMetaGenIntfFunc)dlsym(mHandle, LIB_FN_NAME_DESTROY);

    if ((mCreate == nullptr) ||
        (mDestroy == nullptr)) {
        mCreate  = nullptr;
        mDestroy = nullptr;

        dlclose(mHandle);
        mHandle = nullptr;

        ExynosLogE("[%s] dlsym() is failed", __FUNCTION__);
        return false;
    }

    mInterface = (Hdr10PlusMetaInterface *)mCreate();
    if (mInterface == nullptr) {
        mCreate  = nullptr;
        mDestroy = nullptr;

        dlclose(mHandle);
        mHandle = nullptr;

        ExynosLogE("[%s] CreateHdr10PlusMetaGenIntfFactory() is failed", __FUNCTION__);
        return false;
    }

    return true;
}

void ExynosHDR10PlusStatEncImpl::unload() {
    ExynosLogFunctionTrace();

    if (mHandle != nullptr) {
        if (mIsConfigured) {
            deinit();
        }

        if ((mInterface != nullptr) &&
            (mDestroy != nullptr)) {
            mDestroy(mInterface);
            mInterface = nullptr;
        }

        mCreate  = nullptr;
        mDestroy = nullptr;

        dlclose(mHandle);
        mHandle = nullptr;
    }

    return;
}

bool ExynosHDR10PlusStatEncImpl::run(
    std::shared_ptr<ExynosBuffer> buffer,
    HDRStatInfo &stat) {
    ExynosLogFunctionTrace();

    if (mInterface == nullptr) {
        /* run() is invalid */
        return false;
    }

    if (!mIsConfigured) {
        if (!init()) {
            ExynosLogE("[%s] init() is failed", __FUNCTION__);
            return false;
        }
    }

    ExynosHdrData_ST2094_40 dynamicMeta;
    memset(&dynamicMeta, 0, sizeof(ExynosHdrData_ST2094_40));

    ExynosLogD("[%s] use statistics info and generate HDR dynamic info", __FUNCTION__);

    if (mInterface->getHdr10PlusMetadata(&stat.sHdrDynamicStatInfo, &dynamicMeta) != 0) {
        ExynosLogE("[%s] getHdr10PlusMetadata() is failed", __FUNCTION__);
        return false;
    }

    /* for update it to client */
    {
        auto filterParams = std::static_pointer_cast<ExynosFilterParams>(buffer->mParams);
        if (filterParams.get() != nullptr) {
            auto filterParam = std::make_shared<ExynosFilterParam<ParamHdrDynamicInfo>>();
            filterParam->registTargetFilter(TARGET_OWNER_COMPONENT);

            auto param = std::static_pointer_cast<ExynosParam<ParamHdrDynamicInfo>>(filterParam->getBaseParam());

            ExynosHdrDynamicInfo &DY = param->m.DY;
            DY.valid = 1;  /* mark for processing */

            memcpy(&DY.data, &dynamicMeta, sizeof(ExynosHdrData_ST2094_40));

            filterParams->addParam(filterParam);

            ExynosLogV("[%s] add hdr10+ dynamic info", __FUNCTION__);
        }
    }

    return writeToSEI(buffer, stat, dynamicMeta);
}

bool ExynosHDR10PlusStatEncImpl::run(
    std::shared_ptr<ExynosBuffer> buffer,
    HDRStatInfo &stat,
    ExynosHdrDynamicInfo &dynamic) {
    ExynosLogFunctionTrace();

    ExynosLogD("[%s] use HDR dynamic info passed", __FUNCTION__);

    return writeToSEI(buffer, stat, dynamic.data);
}

bool ExynosHDR10PlusStatEncImpl::writeToSEI(
    std::shared_ptr<ExynosBuffer> buffer,
    HDRStatInfo &stat,
    ExynosHdrData_ST2094_40 &info) {
    if (buffer.get() == nullptr) {
        return false;
    }

    BufferAddressInfo addrInfo;
    if (!buffer->map(addrInfo)) {
        ExynosLogE("[%s] map() is failed", __FUNCTION__);
        return false;
    }

    auto size = Exynos_sei_write(&info, stat.hdr10_plus_stat_sei_size,
                                    (unsigned char *)(addrInfo.plane[0]) + stat.hdr10_plus_stat_offset);

    buffer->unmap();

    if (size != stat.hdr10_plus_stat_sei_size) {
        ExynosLogE("[%s] Exynos_sei_write() is failed", __FUNCTION__);
        return false;
    }

    return true;
}

bool ExynosHDR10PlusStatEncImpl::init() {
    ExynosLogFunctionTrace();

    if (mHandle == nullptr) {
        /* it is not supported */
        return false;
    }

    if (mInterface != nullptr) {
        mInterface->initHdr10PlusMeta();
        mIsConfigured = true;

        return true;
    }

    return false;
}

void ExynosHDR10PlusStatEncImpl::deinit() {
    ExynosLogFunctionTrace();

    if (mIsConfigured) {
        mIsConfigured = false;
    }

    return;
}

bool ExynosHDR10PlusStatEncFilter::onStart() {
    ExynosLogFunctionTrace();

    if (mExternalImpl.get() == nullptr) {
        mExternalImpl = std::make_shared<ExynosHDR10PlusStatEncImpl>(mObjName);
    }

    return true;
}

bool ExynosHDR10PlusStatEncFilter::onReset() {
    ExynosLogFunctionTrace();

    memset(&mStatInfo, 0, sizeof(mStatInfo));
    memset(&mDynamicInfo, 0, sizeof(mDynamicInfo));

    return true;
}

bool ExynosHDR10PlusStatEncFilter::onProcess(std::shared_ptr<ExynosBuffer> buffer) {
    ExynosLogFunctionTrace();

    if (buffer.get() == nullptr) {
        /* invalid parameter */
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return false;
    }

    auto mHDR10PlusStatEncImpl = std::static_pointer_cast<ExynosHDR10PlusStatEncImpl>(mExternalImpl);

    if ((!mHasStatInfo) ||
        (mHDR10PlusStatEncImpl.get() == nullptr) ||
        (buffer->mImageInfo.eFrameInfo & FrameInfo::CodecSpecificData)) {
        ExynosLogV("[%s] bypass", __FUNCTION__);

        return bypassBuffer(buffer);
    }

    bool ret = false;

    /* trust existing dynamic info */
    if (mHasDynamicInfo) {
        ret = mHDR10PlusStatEncImpl->run(buffer, mStatInfo, mDynamicInfo);
    } else {
        ret = mHDR10PlusStatEncImpl->run(buffer, mStatInfo);
    }

    if (!ret) {
        ExynosLogE("[%s] run() is failed", __FUNCTION__);
        return false;
    }

    return bypassBuffer(buffer);
}

void ExynosHDR10PlusStatEncFilter::onApplyConfig(std::shared_ptr<ExynosParams> params) {
    ExynosLogFunctionTrace();

    if ((params.get() == nullptr) ||
        params->empty()) {
        /* there is nothing to change */
        return;
    }

    auto filterParams = std::static_pointer_cast<ExynosFilterParams>(params);

    {
        mHasStatInfo = false;

        auto filterParam = filterParams->getParam(ExynosParamIndex::HDR10PlusStatInfoIndex, mID);
        if (filterParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamHDR10PlusStatInfo>>(filterParam->getBaseParam());

            ExynosLogV("[%s] has hdr10+ statistics info", __FUNCTION__);

            memcpy(&mStatInfo, &(param->m.info), sizeof(mStatInfo));

            mHasStatInfo = true;
        }
    }

    {
        mHasDynamicInfo = false;

        auto filterParam = filterParams->getParam(ParamHdrDynamicInfo::INDEX, mID);
        if (filterParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamHdrDynamicInfo>>(filterParam->getBaseParam());

            ExynosLogV("[%s] has hdr10+ dynamic info", __FUNCTION__);

            memcpy(&mDynamicInfo, &(param->m.DY), sizeof(mDynamicInfo));

            param->m.DY.valid = 0;  /* mark for the completion */

            mHasDynamicInfo = true;
        }
    }

    return;
}
