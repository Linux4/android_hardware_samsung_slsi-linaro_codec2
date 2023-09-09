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
#include "Exynos_GDC_Filter.h"

#define LOG_ON
#include "ExynosLog.h"
#define LOG_TAG "ExynosGDCFilter"

#include "ExynosGraphicBuffer.h"

#define MAX_ALLOC_BUFFER_NUM EXTRA_INTERNAL_BUFFER_NUM

static void setBufferInfo(ExynosBufferInfo &info, std::shared_ptr<ExynosBuffer> buffer, bool src = true) {
    if (buffer.get() == nullptr) {
        return;
    }

    info.stImageInfo.nWidth     = buffer->mImageInfo.nWidth;
    info.stImageInfo.nHeight    = buffer->mImageInfo.nHeight;
    info.stImageInfo.nStride    = buffer->mImageInfo.nStride;
    info.stImageInfo.nFormat    = buffer->mImageInfo.nFormat;

    ExynosUtils::GetYUVPlaneInfo(info.stImageInfo.nStride, info.stImageInfo.nHeight, info.stImageInfo.nFormat, info.nPlane, info.nAllocLen,
                                 buffer->getFlags());

    auto handle = buffer->handle();
    for (int i = 0; i < info.nPlane; i++) {
        info.nFD[i] = handle->data[i];
        StaticExynosLog(Level::Debug, "ExynosGDCFilter", "[%s] %s buffer info : fd(%d)", __FUNCTION__, (src)? "src":"dst", info.nFD[i]);
    }

    info.obj = buffer;

    StaticExynosLog(Level::Essential, "ExynosGDCFilter", "[%s] %s buffer (w:%d, h:%d, s:%d, f:0x%x)", __FUNCTION__,
                            (src)? "src":"dst",
                            info.stImageInfo.nWidth, info.stImageInfo.nHeight, info.stImageInfo.nStride,
                            info.stImageInfo.nFormat);

    return;
}

bool ExynosGDCFilter::doStart() {
    ExynosLogFunctionTrace();

    if (mGDC.get() == nullptr) {
        mGDC = std::make_shared<ExynosGDCWrapper>(mObjName);
    }

    mDebug = ExynosUtils::GetDebugType(mObjName);

    return true;
}

bool ExynosGDCFilter::doStop() {
    ExynosLogFunctionTrace();

    if (mGDC.get() != nullptr) {
        mGDC->flush();
    }

    return true;
}

bool ExynosGDCFilter::doFlush() {
    ExynosLogFunctionTrace();

    if (mGDC.get() != nullptr) {
        mGDC->flush();
    }

    return true;
}

bool ExynosGDCFilter::doReset() {
    ExynosLogFunctionTrace();

    mGDC.reset();

    return true;
}

bool ExynosGDCFilter::doProcess(std::shared_ptr<ExynosBuffer> buffer) {
    ExynosLogFunctionTrace();

    if (buffer.get() == nullptr) {
        /* invalid parameter */
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return false;
    }

    ExynosLogD("[%s] exynos buffer(%p)", __FUNCTION__, buffer.get());

    bool ret = false;

    auto isNeedToProcess =
        [buffer]()->bool {
            if (buffer->getFlags() & ExynosBuffer::REPLICA) {
                return false;
            }

            auto meta = buffer->metadata();
            if (meta == nullptr) {
                return false;
            }

            if ((meta->eType & VIDEO_INFO_TYPE_GDC_OTF) &&
                (meta->data.enc.nIsGdcOTF == 1)) {
                return true;
            }

            return false;
        }();

    if (!isNeedToProcess) {
        ret = bypassBuffer(buffer);
        ExynosLogV("[%s] bypass", __FUNCTION__);
    } else {
        if (mGDC.get() == nullptr) {
            ExynosLogE("[%s] GDC filter is not started", __FUNCTION__);
            return false;
        }

        auto meta = buffer->metadata();
        if (meta == nullptr) {
            ExynosLogE("[%s] unknown error", __FUNCTION__);
            return false;
        }

        if (!(meta->eType & VIDEO_INFO_TYPE_GDC_OTF)) {
            ExynosLogE("[%s] unknown error", __FUNCTION__);
            return false;
        }

        ExynosBufferInfo input, output;
        ExynosBufferInfo::reset(input);
        ExynosBufferInfo::reset(output);
#if 0
        /* apply configurations */
        applyConfig(buffer->mParams);  // need?
#endif
        /* set input info */
        {
            input.eDataInfo = DataInfo::SingleData;
            setBufferInfo(input, buffer);
        }

        /* allocate output buffer & set output info */
        GraphicBufferAttribute attr;

        attr.mWidth  = buffer->mImageInfo.stCropInfo.nWidth;
        attr.mHeight = buffer->mImageInfo.stCropInfo.nHeight;
        attr.mFormat = buffer->format();
        attr.mUsage  = buffer->usage();
        if (attr.mUsage &  vendor::graphics::ExynosGraphicBufferUsage::GDC_MODE) {
            attr.mUsage &= ~(vendor::graphics::ExynosGraphicBufferUsage::GDC_MODE);
        }

        AllocArg arg;
        arg.attr        = attr;
        arg.limit       = (mAllocMode == AllocMode::PreferResources)? MAX_ALLOC_BUFFER_NUM:0;
        arg.checkLimit  = nullptr;
        arg.allocCount  = 0;

        auto optBuffer = allocBuffer(arg);
        if (!optBuffer) {
            return false;
        }
        std::shared_ptr<ExynosBuffer> outbuffer = *optBuffer;

        output.eDataInfo = DataInfo::NoData;
        setBufferInfo(output, outbuffer, false);

        output.stImageInfo.eFrameInfo = buffer->mImageInfo.eFrameInfo;
        output.stImageInfo.stHDRInfo  = buffer->mImageInfo.stHDRInfo;
        output.stImageInfo.nPoc       = buffer->mImageInfo.nPoc;
        output.stImageInfo.nTimeStamp = buffer->mImageInfo.nTimeStamp;

        ret = mGDC->process(input, output);
        if (!ret) {
            ExynosLogE("[%s] process() is failed", __FUNCTION__);
            return false;
        }

        input.eDataInfo  = DataInfo::UnusedData;  /* need to keep this buffer */
        output.eDataInfo = DataInfo::SingleData;

        /* update output information to buffer */
        outbuffer->mImageInfo = output.stImageInfo;

        auto src = input.obj->metadata();
        auto dst = output.obj->metadata();
        if ((src != nullptr) &&
            (dst != nullptr)) {
            memcpy(dst, src, sizeof(ExynosVideoMeta));
        }

        ret = processDone(input, output);
    }

    return ret;
}
#if 0
void ExynosGDCFilter::applyConfig(std::shared_ptr<ExynosParams> params) {
    ExynosLogFunctionTrace();

    if ((params.get() == nullptr) ||
        params->empty()) {
        /* there is nothing to change */
        return;
    }

    auto filterParams = std::static_pointer_cast<ExynosFilterParams>(params);

    return;
}
#endif
