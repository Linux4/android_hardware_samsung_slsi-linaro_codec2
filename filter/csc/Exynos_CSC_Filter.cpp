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
#include <chrono>
#include <thread>

#include <system/graphics.h>
#include "exynos_format.h"

#include "Exynos_CSC_Filter.h"
#include "ExynosCSC.h"
#include "ExynosGraphicBuffer.h"

#define LOG_ON
#include "ExynosLog.h"
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "ExynosCSCFilter"

#define MAX_ALLOC_BUFFER_NUM EXTRA_INTERNAL_BUFFER_NUM

static void setPortInfo(
    PortInfo   &portInfo,
    int         width,
    int         height,
    int         stride,
    int         format,
    CropInfo    crop) {
    portInfo.width  = width;
    portInfo.height = height;
    portInfo.stride = stride;
    portInfo.format = format;
    portInfo.crop   = crop;
}

static void getPortInfo(
    uint32_t &width,
    uint32_t &height,
    uint32_t &stride,
    uint32_t &format,
    CropInfo &crop,
    PortInfo  portInfo) {
    width  = portInfo.width;
    height = portInfo.height;
    stride = portInfo.stride;
    format = portInfo.format;
    crop   = portInfo.crop;
}

static void setCropInfo(
    CropInfo   &crop,
    int         left,
    int         top,
    int         width,
    int         height) {
    crop.nLeft   = left;
    crop.nTop    = top;
    crop.nWidth  = width;
    crop.nHeight = height;
}

static bool setPortBufferInfo(
    ExynosBufferInfo                &bufferInfo,
    PortInfo                        &portInfo,
    std::shared_ptr<ExynosBuffer>    buffer,
    int                              dataspace,
    ImageInfo                        imageInfo,
    Port                             port,
    bool                             useFormat = false) {
    auto flags = buffer->getFlags();

    if (port == Port::Input) {
        bufferInfo.eDataInfo = DataInfo::SingleData;

        if ((buffer->width() < portInfo.stride) ||
            (buffer->height() < portInfo.height)) {
            StaticExynosLog(Level::Error, "ExynosCSCFilter", "[%s] size of buffer is small : (%d x %d) vs (%d x %d)",
                                __FUNCTION__, buffer->width(), buffer->height(), portInfo.stride, portInfo.height);
            return false;
        }

        if (useFormat) {
            /* need to update actual format */
            auto meta = buffer->metadata();

            if ((meta != nullptr) &&
                (meta->eType & VIDEO_INFO_TYPE_CHECK_PIXEL_FORMAT)) {
                portInfo.format = meta->nPixelFormat;
                flags = 0;  /* W/A : in case of SBWC, never be GPU_TEXTURE */
            }
        }
    } else {
        bufferInfo.eDataInfo              = DataInfo::NoData;
        bufferInfo.stImageInfo.eFrameInfo = imageInfo.eFrameInfo;
        bufferInfo.stImageInfo.nPoc       = imageInfo.nPoc;
        bufferInfo.stImageInfo.nTimeStamp = imageInfo.nTimeStamp;
        portInfo.stride                   = buffer->mImageInfo.nStride;   /* must follow graphic buffer's stride */
        portInfo.width                    = buffer->mImageInfo.nWidth;
    }

    getPortInfo(bufferInfo.stImageInfo.nWidth, bufferInfo.stImageInfo.nHeight, bufferInfo.stImageInfo.nStride,
                bufferInfo.stImageInfo.nFormat, bufferInfo.stImageInfo.stCropInfo, portInfo);

    bufferInfo.stImageInfo.stHDRInfo  = imageInfo.stHDRInfo;
    bufferInfo.stImageInfo.nDataSpace = dataspace;

    auto vstride = vendor::graphics::ExynosGraphicBufferMeta::get_vstride(buffer->handle());

    ExynosUtils::GetYUVPlaneInfo(bufferInfo.stImageInfo.nStride, vstride, bufferInfo.stImageInfo.nFormat,
                                 bufferInfo.nPlane, bufferInfo.nAllocLen, flags);

    auto handle = buffer->handle();

    for (int i = 0; i < bufferInfo.nPlane; i++) {
        bufferInfo.nFD[i] = handle->data[i];
        StaticExynosLog(Level::Debug, "ExynosCSCFilter", "[%s] %s buffer info : fd(%d)",
                            __FUNCTION__, (port == Port::Input)? "input":"output", bufferInfo.nFD[i]);
    }

    bufferInfo.obj = buffer;

    StaticExynosLog(Level::Debug, "ExynosCSCFilter", "[%s] %s w:%d, h:%d, s:%d, cl:%d, ct:%d, cw:%d, ch:%d, f:0x%x, dataspace:0x%x",
                    __FUNCTION__,
                    (port == Port::Input)? "input":"output",
                    bufferInfo.stImageInfo.nWidth, bufferInfo.stImageInfo.nHeight, bufferInfo.stImageInfo.nStride,
                    bufferInfo.stImageInfo.stCropInfo.nLeft, bufferInfo.stImageInfo.stCropInfo.nTop,
                    bufferInfo.stImageInfo.stCropInfo.nWidth, bufferInfo.stImageInfo.stCropInfo.nHeight,
                    bufferInfo.stImageInfo.nFormat, dataspace);

    return true;
}

bool ExynosCSCFilter::doStart() {
    ExynosLogFunctionTrace();

    if (mCSC.get() == nullptr) {
#ifndef USE_SW_CSC
        mCSC = std::make_shared<ExynosCSC>(mObjName, ((mIsSecure == true)? ExynosCSC::Type::HW_SECURE:ExynosCSC::Type::HW));
#else
        mCSC = std::make_shared<ExynosCSC>(mObjName, ((mIsSecure == true)? ExynosCSC::Type::UNUSED:ExynosCSC::Type::SW));
#endif

        mUseCropping    = false;
        mUsePositioning = false;
        mUseScaling     = false;
        mUseFormat      = false;
        mUseBufferCopy  = false;
        mFormat         = 0;  /* HAL_PIXEL_FORMAT_UNDEFINED */
        mDataSpace      = HAL_DATASPACE_BT601_625;
    }

    mDebug = ExynosUtils::GetDebugType(mObjName);

    return true;
}

bool ExynosCSCFilter::doStop() {
    ExynosLogFunctionTrace();

    /* TODO */

    return true;
}

bool ExynosCSCFilter::doFlush() {
    ExynosLogFunctionTrace();

    /* TODO */

    return true;
}

bool ExynosCSCFilter::doReset() {
    ExynosLogFunctionTrace();

    mCSC.reset();

    return true;
}

bool ExynosCSCFilter::doProcess(std::shared_ptr<ExynosBuffer> buffer) {
    ExynosLogFunctionTrace();

    if (buffer.get() == nullptr) {
        /* invalid parameter */
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return false;
    }

    ExynosLogD("[%s] exynos buffer(%p)", __FUNCTION__, buffer.get());

    PortInfo srcInfo, dstInfo;

    memset(&srcInfo, 0, sizeof(srcInfo));
    memset(&dstInfo, 0, sizeof(dstInfo));

    setPortInfo(srcInfo, buffer->mImageInfo.nWidth, buffer->mImageInfo.nHeight, buffer->mImageInfo.nStride,
                buffer->mImageInfo.nFormat, buffer->mImageInfo.stCropInfo);
    setPortInfo(dstInfo, srcInfo.width, srcInfo.height, srcInfo.stride, srcInfo.format, srcInfo.crop);

    /* apply configurations */
    applyConfig(buffer->mParams);

    if (mUseCropping) {
        srcInfo.crop = mCrop;
        memset(&dstInfo.crop, 0, sizeof(dstInfo.crop));
    }

    if (mUsePositioning) {
        dstInfo.crop = mPosit;
    }

    if (mUseScaling) {
        dstInfo.stride  = mScale.width;
        dstInfo.height  = mScale.height;

        if (!mUsePositioning) {
            setCropInfo(dstInfo.crop, 0, 0, dstInfo.width, dstInfo.height);
        }
    }

    if (mUseFormat) {
        if (mFormat == HAL_PIXEL_FORMAT_YCBCR_420_888) {
            /* this is for decoder case
             * in case of encoder, YCbCr_P010_M never be used with YCBCR_420_888
             */
            if (ExynosUtils::Check10BitFormat(srcInfo.format)) {
                /* 420_888 means that
                 * output should be treated as 8bit YUV
                 */
                dstInfo.format = HAL_PIXEL_FORMAT_YV12;
            }
        } else if (mFormat == HAL_PIXEL_FORMAT_YCBCR_P010) {
            if (!ExynosUtils::Check10BitFormat(srcInfo.format)) {
                ExynosLogW("[%s] P010 is requested but data is not 10bit(0x%x)", __FUNCTION__, srcInfo.format);
            } else {
                dstInfo.format = mFormat;
            }
        } else {
            dstInfo.format = mFormat;
        }
    }

    if (mDebug & EXYNOS_DEBUG_INPUT) {
        ExynosBuffer::dump(buffer, mDebug, (mObjName + "-input"));
    }

    bool ret = false;

    if ((buffer->getFlags() & ExynosBuffer::REPLICA) ||
        ((!mUseBufferCopy) &&
         (!memcmp(&srcInfo, &dstInfo, sizeof(PortInfo))))) {
        ret = bypassBuffer(buffer);
        ExynosLogV("[%s] bypass", __FUNCTION__);
    } else {
        if (mCSC.get() == nullptr) {
            ExynosLogE("[%s] CSC filter is not started", __FUNCTION__);
            return false;
        }

        ExynosBufferInfo input, output;
        ExynosBufferInfo::reset(input);
        ExynosBufferInfo::reset(output);

        int dataspace = mDataSpace;

        /* set input info */
        if (!setPortBufferInfo(input, srcInfo, buffer, dataspace, buffer->mImageInfo, Port::Input, mUseFormat)) {
            return false;
        }

        /* allocate output buffer & set output info */
        GraphicBufferAttribute attr;

        attr.mWidth  = (mUseScaling)? dstInfo.stride:dstInfo.width;
        attr.mHeight = dstInfo.height;
        attr.mFormat = dstInfo.format;
        attr.mUsage  = buffer->usage();

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

        setPortBufferInfo(output, dstInfo, outbuffer, dataspace, buffer->mImageInfo, Port::Output);

        /* Below memcpy should be done before process()
         * because ExynosCSC can overwrite metadata of output.
         */
        auto src = input.obj->metadata();
        auto dst = output.obj->metadata();

        if ((src != nullptr) &&
            (dst != nullptr)) {
            memcpy(dst, src, sizeof(ExynosVideoMeta));
        }

        if (mOperatingRate > 0) {
            mCSC->setOperatingRate(mOperatingRate);
        } else if ((mRealTimePriority == 0) && (mFramerate > 0)) {
            /* TODO :
             * In the case of vpx enc,
             * the framerate can be zero for dynamic framerate feature.
             * it may require that the enc filter inform actual framerate to c2 comp
             * and update it as such framerate here
             */
            mCSC->setOperatingRate(mFramerate);
        }

        ret = mCSC->process(input, output);
        if (!ret) {
            ExynosLogE("[%s] process() is failed", __FUNCTION__);
            return false;
        }

        input.eDataInfo  = DataInfo::UsedData;
        output.eDataInfo = DataInfo::SingleData;

        /* update output information to buffer */
        outbuffer->mImageInfo = output.stImageInfo;

        ret = processDone(input, output);
    }

    return ret;
}

void ExynosCSCFilter::applyConfig(std::shared_ptr<ExynosParams> params) {
    ExynosLogFunctionTrace();

    if ((params.get() == nullptr) ||
        params->empty()) {
        /* there is nothing to change */
        return;
    }

    auto filterParams = std::static_pointer_cast<ExynosFilterParams>(params);

    /* output format */
    {
        auto filterParam = filterParams->getParam(ExynosParamIndex::ActualFormatIndex, mID);
        if (filterParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamActualFormat>>(filterParam->getBaseParam());

            if (param->m.format != HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED) {
                mUseFormat  = true;
                mFormat     = param->m.format;
            }

            ExynosLogD("[%s] ActualFormat : 0x%x", __FUNCTION__, mFormat);
        }
    }

    /* cropping */
    {
        auto filterParam = filterParams->getParam(ExynosParamIndex::InputCropIndex, mID);
        if (filterParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamInputCrop>>(filterParam->getBaseParam());
            /* TODO : validation */
            ExynosLogD("[%s] InputCrop", __FUNCTION__);
            mUseCropping    = true;
            mCrop           = param->m.crop;
        }
    }

    /* positioning */
    {
        auto filterParam = filterParams->getParam(ExynosParamIndex::OutputCropIndex, mID);
        if (filterParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamOutputCrop>>(filterParam->getBaseParam());
            /* TODO : validation */
            ExynosLogD("[%s] OutputCrop", __FUNCTION__);
            mUsePositioning = true;
            mPosit          = param->m.crop;
        }
    }

    /* scaling up/down */
    {
        auto filterParam = filterParams->getParam(ExynosParamIndex::OutputFrameInfoIndex, mID);
        if (filterParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamOutputFrameInfo>>(filterParam->getBaseParam());
            /* TODO : validation */
            ExynosLogD("[%s] OutputFrame", __FUNCTION__);
            mUseScaling     = true;
            mScale.width    = param->m.width;
            mScale.height   = param->m.height;
        }
    }

    /* dataspace */
    {
        auto filterParam = filterParams->getParam(ExynosParamIndex::DataSpaceIndex, mID);
        if (filterParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamDataSpace>>(filterParam->getBaseParam());
            /* TODO : validation */
            mDataSpace = param->m.dataspace;

            ExynosLogD("[%s] DataSpace : 0x%x", __FUNCTION__, mDataSpace);
        }
    }

    /* buffer copy */
    {
        auto filterParam = filterParams->getParam(ExynosParamIndex::BufferCopyIndex, mID);
        if (filterParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamBufferCopy>>(filterParam->getBaseParam());

            mUseBufferCopy = (param->m.enable == On)? true:false;
            ExynosLogD("[%s] Buffer Copy is %s", __FUNCTION__, (param->m.enable == On)? "enabled":"disabled");
        }
    }

    /* operating rate */
    {
        auto filterParam =  filterParams->getParam(ExynosParamIndex::OperatingRateIndex, mID);
        if (filterParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamOperatingRate>>(filterParam->getBaseParam());
            mOperatingRate = param->m.value;
            ExynosLogD("[%s] operating rate is %d", __FUNCTION__, (param->m.value));
        }
    }

    /* real time priority */
    {
        auto filterParam =  filterParams->getParam(ExynosParamIndex::RealTimePriorityIndex, mID);
        if (filterParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamRealTimePriority>>(filterParam->getBaseParam());
            mRealTimePriority = param->m.value;
            ExynosLogD("[%s] real time priority is %d", __FUNCTION__, (param->m.value));
        }
    }

    /* framerate */
    {
        auto filterParam =  filterParams->getParam(ExynosParamIndex::FramerateIndex, mID);
        if (filterParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamFramerate>>(filterParam->getBaseParam());
            mFramerate = param->m.framerate;
            ExynosLogD("[%s] framerate is %d", __FUNCTION__, (param->m.framerate));
        }
    }

    return;
}

