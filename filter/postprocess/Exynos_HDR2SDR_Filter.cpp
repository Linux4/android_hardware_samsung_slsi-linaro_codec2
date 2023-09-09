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

#include "Exynos_HDR2SDR_Filter.h"
#include "VendorVideoAPI.h"

#define LOG_ON
#include "ExynosLog.h"
#define LOG_TAG "ExynosHDR2SDRFilter"

#define MAX_ALLOC_BUFFER_NUM EXTRA_INTERNAL_BUFFER_NUM

/* it depends on codec standard(CTA.861.3) */
#define RANGE_LIMITED 2
#define PRIMARIES_BT709 1
#define PRIMARIES_BT2020 9
#define TRANSFER_BT709 1
#define TRANSFER_ST2084 16
#define MATRIX_COEFF_BT709 1
#define MATRIX_COEFF_BT2020 9

#define CHECK_HDR10(f, r, p, t, c) \
    ((f == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M) && \
     (r == RANGE_LIMITED) && \
     (p == PRIMARIES_BT2020) && \
     (t == TRANSFER_ST2084) && \
     (c == MATRIX_COEFF_BT2020))

constexpr char LIB_NAME[]           = "libImageFormatConverter.so";
constexpr char LIB_FN_NAME_INIT[]   = "CL_HDR2SDR_ARM_init";
constexpr char LIB_FN_NAME_DEINIT[] = "CL_HDR2SDR_ARM_deinit";
constexpr char LIB_FN_NAME_RUN[]    = "CL_HDR2SDR_ARM_convert1";

typedef enum _Image_ColorFormat {
    INVALID         = -1,
    TP10_UBWC_QCOM  = 0,
    P010_LINEAR     = 1,
    NV12            = 2,
    RGBA            = 3,
    YV12            = 4,
    NV21            = 5,
} Image_ColorFormat;

typedef enum _ALGO_t {
    NONE    = 0,
    NORMAL  = 1,
    HI_JACK = 2,
} ALGO_t;

typedef struct _MULTIPLANE_t {
    int     handle;
    void   *host_ptr;
    int     size;
} MULTIPLANE_t;

typedef struct _HDR2SDR_config_params_t {
    Image_ColorFormat color_format;
    MULTIPLANE_t buffer[3];
} HDR2SDR_config_params_t;

typedef struct _HDR10_STATIC_INFO {
    unsigned int max_display_luminance; // in 0.0001 cd/m^2
    unsigned int min_display_luminance; // in 0.0001 cd/m^2
    unsigned int max_content_light; // in cd/m^2
    unsigned int max_pic_average_light; // in cd/m^2
} HDR10_STATIC_INFO;

typedef struct _HDR10PLUS_DYNAMIC_INFO {
    unsigned char country_code;              // 0xB5
    unsigned short provider_code;            // 0x003C
    unsigned short provider_oriented_code;   // 0x0001

    unsigned char application_identifier;
    unsigned char application_version;

    unsigned int display_max_luminance;
    // unsigned char targeted_system_display_actual_peak_luminance_flag = 0;
    // --------------------------------------------------------------------
    /* window = 1, fixed point at this moment */
    unsigned int  maxscl[3];
    unsigned int  avg_maxrgb;
    unsigned char num_maxrgb_percentiles;
    unsigned char maxrgb_percentages[15];
    unsigned int  maxrgb_percentiles[15];
    // unsigned char fraction_bright_pixels = 1;
    // --------------------------------------------------------------------
    // unsigned char mastering_display_actual_peak_luminance_flag = 0;
    // --------------------------------------------------------------------
    /* window = 1, fixed point at this moment */
    unsigned char  tone_mapping_flag;         // 1
    unsigned short knee_point_x;               //[ ~ 4095]
    unsigned short knee_point_y;               //[ ~ 4095]
    unsigned char  num_bezier_curve_anchors;   // 10 [ ~ 15]
    unsigned short bezier_curve_anchors[15];
    // unsigned char color_saturation_mapping_flag = 0;
    // --------------------------------------------------------------------
} HDR10PLUS_DYNAMIC_INFO;

typedef bool (*ConvertInitFunc)(const unsigned int width, const unsigned int height, const ALGO_t algo, void **user_data, const bool usage);
typedef void (*ConvertDeinitFunc)(void *user_data);
typedef bool (*ConvertRunFunc)(HDR2SDR_config_params_t *In_params, HDR2SDR_config_params_t *Out_params,
                                HDR10PLUS_DYNAMIC_INFO *dy, HDR10_STATIC_INFO *st, void *user_data, uint32_t frame);

static void updateHdrDynamicInfo(
    HDR10PLUS_DYNAMIC_INFO &dstInfo,
    ExynosHdrDynamicInfo   *pSrcInfo) {
    dstInfo.country_code           = pSrcInfo->data.country_code;
    dstInfo.provider_code          = pSrcInfo->data.provider_code;
    dstInfo.provider_oriented_code = pSrcInfo->data.provider_oriented_code;
    dstInfo.application_identifier = pSrcInfo->data.application_identifier;
    dstInfo.application_version    = pSrcInfo->data.application_version;

#ifdef USE_FULL_ST2094_40
    dstInfo.display_max_luminance  = pSrcInfo->data.targeted_system_display_maximum_luminance;

    for (int i = 0; i < 3; i++) {
        dstInfo.maxscl[i] = pSrcInfo->data.maxscl[0][i];
    }
    dstInfo.avg_maxrgb = pSrcInfo->data.average_maxrgb[0];

    dstInfo.num_maxrgb_percentiles = pSrcInfo->data.num_maxrgb_percentiles[0];
    for (int i = 0; i < dstInfo.num_maxrgb_percentiles; i++) {
        dstInfo.maxrgb_percentages[i] = pSrcInfo->data.maxrgb_percentages[0][i];
        dstInfo.maxrgb_percentiles[i] = pSrcInfo->data.maxrgb_percentiles[0][i];
    }

    dstInfo.tone_mapping_flag          = pSrcInfo->data.tone_mapping.tone_mapping_flag[0];
    dstInfo.knee_point_x               = pSrcInfo->data.tone_mapping.knee_point_x[0];
    dstInfo.knee_point_y               = pSrcInfo->data.tone_mapping.knee_point_y[0];
    dstInfo.num_bezier_curve_anchors   = pSrcInfo->data.tone_mapping.num_bezier_curve_anchors[0];

    for (int i = 0; i < dstInfo.num_bezier_curve_anchors; i++) {
        dstInfo.bezier_curve_anchors[i] = pSrcInfo->data.tone_mapping.bezier_curve_anchors[0][i];
    }
#else
    dstInfo.display_max_luminance = pSrcInfo->data.display_maximum_luminance;

    for (int i = 0; i < 3; i++) {
        dstInfo.maxscl[i] = pSrcInfo->data.maxscl[i];
    }

    (void)dstInfo.avg_maxrgb;  /* TODO : it can be supported since hdr10+ full spec support */
    dstInfo.num_maxrgb_percentiles = pSrcInfo->data.num_maxrgb_percentiles;
    for (int i = 0; i < dstInfo.num_maxrgb_percentiles; i++) {
        dstInfo.maxrgb_percentages[i] = pSrcInfo->data.maxrgb_percentages[i];
        dstInfo.maxrgb_percentiles[i] = pSrcInfo->data.maxrgb_percentiles[i];
    }

    dstInfo.tone_mapping_flag        = pSrcInfo->data.tone_mapping.tone_mapping_flag;
    dstInfo.knee_point_x             = pSrcInfo->data.tone_mapping.knee_point_x;
    dstInfo.knee_point_y             = pSrcInfo->data.tone_mapping.knee_point_y;
    dstInfo.num_bezier_curve_anchors = pSrcInfo->data.tone_mapping.num_bezier_curve_anchors;

    for (int i = 0; i < dstInfo.num_bezier_curve_anchors; i++) {
        dstInfo.bezier_curve_anchors[i] = pSrcInfo->data.tone_mapping.bezier_curve_anchors[i];
    }
#endif // USE_FULL_ST2094_40

    return;
}

class ExynosHDR2SDRImpl : public ExynosExternalImpl {
public:
    ExynosHDR2SDRImpl(std::string name) : ExynosExternalImpl(name) {
        mbLogOff = false;

        mHandle      = nullptr;
        mInit        = nullptr;
        mDeinit      = nullptr;
        mRun         = nullptr;
        mUserDataPtr = nullptr;

        mWidth = 0;
        mHeight = 0;
        mMode = 1;
        memset(&mStaticInfo, 0, sizeof(mStaticInfo));
        memset(&mDynamicInfo, 0, sizeof(mDynamicInfo));
        mHasDynamicInfo = false;
        mIsConfigured = false;
    }

    ~ExynosHDR2SDRImpl() {
        unload();
    }

    bool load() override;
    void unload() override;
    bool query(std::shared_ptr<ExynosBuffer> buffer);
    bool run(std::shared_ptr<ExynosBuffer> input, std::shared_ptr<ExynosBuffer> output, uint32_t mode, uint32_t frame);

private:
    bool init(uint32_t width, uint32_t height, uint32_t mode);
    void deinit();

    void                *mHandle;
    ConvertInitFunc      mInit;
    ConvertDeinitFunc    mDeinit;
    ConvertRunFunc       mRun;

    uint32_t  mWidth;
    uint32_t  mHeight;
    uint32_t  mMode;

    HDR10_STATIC_INFO      mStaticInfo;
    HDR10PLUS_DYNAMIC_INFO mDynamicInfo;
    bool                   mHasDynamicInfo;

    void *mUserDataPtr;
    bool mIsConfigured;
};

bool ExynosHDR2SDRImpl::load() {
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

    mInit   = (ConvertInitFunc)dlsym(mHandle, LIB_FN_NAME_INIT);
    mDeinit = (ConvertDeinitFunc)dlsym(mHandle, LIB_FN_NAME_DEINIT);
    mRun    = (ConvertRunFunc)dlsym(mHandle, LIB_FN_NAME_RUN);

    if ((mInit   == nullptr) ||
        (mDeinit == nullptr) ||
        (mRun    == nullptr)) {
        mInit   = nullptr;
        mDeinit = nullptr;
        mRun    = nullptr;

        dlclose(mHandle);
        mHandle = nullptr;

        ExynosLogE("[%s] dlsym() is failed", __FUNCTION__);
        return false;
    }

    return true;
}

void ExynosHDR2SDRImpl::unload() {
    ExynosLogFunctionTrace();

    if (mHandle != nullptr) {
        if (mIsConfigured) {
            deinit();
        }

        mInit   = nullptr;
        mDeinit = nullptr;
        mRun    = nullptr;

        dlclose(mHandle);
        mHandle = nullptr;
    }

    return;
}

bool ExynosHDR2SDRImpl::query(std::shared_ptr<ExynosBuffer> buffer) {
    ExynosLogFunctionTrace();

    if (buffer.get() == nullptr) {
        /* error handling */
        ExynosLogE("[%s] input is invalid", __FUNCTION__);
        return false;
    }

    ExynosColorAspects &CA = buffer->mImageInfo.stHDRInfo.sColorAspects;

    uint32_t format = buffer->mImageInfo.nFormat;

    auto meta = buffer->metadata();
    if (meta != nullptr) {
        if (meta->eType & VIDEO_INFO_TYPE_CHECK_PIXEL_FORMAT) {
            format = meta->nPixelFormat;
        }
    }

    if (!(buffer->mImageInfo.stHDRInfo.eValidInfo & VIDEO_INFO_TYPE_COLOR_ASPECTS) ||
        !CHECK_HDR10(format, CA.mRange, CA.mPrimaries, CA.mTransfer, CA.mMatrixCoeffs)) {
        /* it is not HDR */
        ExynosLogD("[%s] input is not HDR", __FUNCTION__);
        return false;
    }

    ExynosLogD("[%s] input(fd(%d), ptr(%p)) is HDR", __FUNCTION__,
                    (buffer->handle())->data[0], buffer.get());

    return true;
}

bool ExynosHDR2SDRImpl::run(
    std::shared_ptr<ExynosBuffer> input,
    std::shared_ptr<ExynosBuffer> output,
    uint32_t                      mode,
    uint32_t                      frame) {
    ExynosLogFunctionTrace();

    if (mRun == nullptr) {
        /* run() is invalid */
        return false;
    }

    uint32_t width  = input->mImageInfo.nStride;
    uint32_t height = input->mImageInfo.nHeight;

    if (init(width, height, mode) == false) {
        ExynosLogE("[%s] init(%d, %d) is failed", __FUNCTION__, width, height);
        return false;
    }

    HDR2SDR_config_params_t inConfig, outConfig;
    memset(&inConfig, 1, sizeof(inConfig));
    memset(&outConfig, 1, sizeof(outConfig));

    BufferAddressInfo inAddrInfo, outAddrInfo;
    memset(&inAddrInfo, 0, sizeof(inAddrInfo));
    memset(&outAddrInfo, 0, sizeof(outAddrInfo));

    /* set input info */
    {
        if (input->map(inAddrInfo) == false) {
            ExynosLogE("[%s] map() is failed", __FUNCTION__);
            return false;
        }

        auto handle = input->handle();

        inConfig.color_format          = P010_LINEAR;
        inConfig.buffer[0].handle      = handle->data[0];
        inConfig.buffer[0].size        = inAddrInfo.size[0];
        inConfig.buffer[0].host_ptr    = inAddrInfo.plane[0];
        inConfig.buffer[1].handle      = handle->data[1];
        inConfig.buffer[1].size        = inAddrInfo.size[1];
        inConfig.buffer[1].host_ptr    = inAddrInfo.plane[1];

        if (input->mImageInfo.stHDRInfo.eValidInfo & VIDEO_INFO_TYPE_HDR_STATIC) {
            ExynosHdrStaticInfo &ST = input->mImageInfo.stHDRInfo.sHdrStaticInfo;

            mStaticInfo.min_display_luminance   = ST.sType1.mMinDisplayLuminance;
            mStaticInfo.max_display_luminance   = ST.sType1.mMaxDisplayLuminance;
            mStaticInfo.max_content_light       = ST.sType1.mMaxContentLightLevel;
            mStaticInfo.max_pic_average_light   = ST.sType1.mMaxFrameAverageLightLevel;

            ExynosLogD("[%s] update HDR static info", __FUNCTION__);
        }

        if (input->mImageInfo.stHDRInfo.eValidInfo & VIDEO_INFO_TYPE_HDR_DYNAMIC) {
            ExynosHdrDynamicInfo *pDY = nullptr;

            if (input->mImageInfo.stHDRInfo.sHdrDynamicBlob.nSize > 0) {
                ExynosHdrDynamicBlob *DYNAMIC_BLOB = &(input->mImageInfo.stHDRInfo.sHdrDynamicBlob);

                pDY = &(input->mImageInfo.stHDRInfo.sHdrDynamicInfo);
                pDY->valid = 0;

                auto err = Exynos_parsing_user_data_registered_itu_t_t35(pDY, (char *)DYNAMIC_BLOB->pData);
                if (err < 0) {
                    ExynosLogE("[%s] converting HDR dynamic info(size:%zu) is failed",
                                    __FUNCTION__, input->mImageInfo.stHDRInfo.sHdrDynamicBlob.nSize);
                } else {
                    pDY->valid = 1;
                }
            } else {
                pDY = &(input->mImageInfo.stHDRInfo.sHdrDynamicInfo);
            }

            if (pDY->valid != 0) {
                HDR10PLUS_DYNAMIC_INFO info;
                memset(&info, 0, sizeof(info));

                updateHdrDynamicInfo(info, pDY);

                if (memcmp(&mDynamicInfo, &info, sizeof(info))) {
                    mHasDynamicInfo = true;
                    memcpy(&mDynamicInfo, &info, sizeof(info));
                    ExynosLogD("[%s] update HDR dynamic info", __FUNCTION__);
                }
            }
        }
    }

    /* set output info */
    {
        if (output->map(outAddrInfo) == false) {
            ExynosLogE("[%s] map() is failed", __FUNCTION__);
            return false;
        }

        auto handle = output->handle();

        switch (output->format()) {
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M:
            outConfig.color_format          = P010_LINEAR;
            outConfig.buffer[0].handle      = handle->data[0];
            outConfig.buffer[0].size        = outAddrInfo.size[0];
            outConfig.buffer[0].host_ptr    = outAddrInfo.plane[0];
            outConfig.buffer[1].handle      = handle->data[1];
            outConfig.buffer[1].size        = outAddrInfo.size[1];
            outConfig.buffer[1].host_ptr    = outAddrInfo.plane[1];
            break;
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
            outConfig.color_format          = NV12;
            outConfig.buffer[0].handle      = handle->data[0];
            outConfig.buffer[0].size        = outAddrInfo.size[0];
            outConfig.buffer[0].host_ptr    = outAddrInfo.plane[0];
            outConfig.buffer[1].handle      = handle->data[1];
            outConfig.buffer[1].size        = outAddrInfo.size[1];
            outConfig.buffer[1].host_ptr    = outAddrInfo.plane[1];
            break;
        default:
            ExynosLogE("[%s] format(0x%x) is invalid", __FUNCTION__, output->format());
            break;
        }
    }

    auto ret = mRun(&inConfig, &outConfig, ((mHasDynamicInfo)? &mDynamicInfo:NULL), &mStaticInfo, mUserDataPtr, frame);

    input->unmap();
    output->unmap();

    return ret;
}

bool ExynosHDR2SDRImpl::init(
    uint32_t width,
    uint32_t height,
    uint32_t mode) {
    ExynosLogFunctionTrace();

    if (mHandle == nullptr) {
        /* it is not supported */
        return false;
    }

    if (mIsConfigured) {
        if ((mWidth == width) &&
            (mHeight == height) &&
            (mMode == mode)) {
            /* already initialized */
            return true;
        } else {
            deinit();
        }
    }

    if (mInit != nullptr) {
        if (mInit(width, height, ((mode == 0)? NORMAL:HI_JACK), &mUserDataPtr, true) != true) {
            ExynosLogE("[%s] Init(%d, %d) is failed", __FUNCTION__, width, height);
            return false;
        }

        mWidth = width;
        mHeight = height;
        mMode = mode;
        mIsConfigured = true;

        return true;
    }

    return false;
}

void ExynosHDR2SDRImpl::deinit() {
    ExynosLogFunctionTrace();

    if (mIsConfigured) {
        if (mDeinit != nullptr) {
            mDeinit(mUserDataPtr);
            mUserDataPtr = nullptr;
        }

        mIsConfigured  = false;
    }

    return;
}

ExynosHDR2SDRFilter::ExynosHDR2SDRFilter(uint32_t id, bool isSecure) : ExynosExternalFilter(id, isSecure) {
    mObjName = "ExynosHDR2SDRFilter";
    mbLogOff = false;
    mThreadPool->setObjName(mObjName);

    mIsEnabled = false;
    mMode = 1;
    mFormat = HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M;
    mFrameCount = 0;
}

bool ExynosHDR2SDRFilter::onStart() {
    ExynosLogFunctionTrace();

    if (mExternalImpl.get() == nullptr) {
        mExternalImpl = std::make_shared<ExynosHDR2SDRImpl>(mObjName);
    }

    return true;
}

bool ExynosHDR2SDRFilter::onReset() {
    ExynosLogFunctionTrace();

    return true;
}

bool ExynosHDR2SDRFilter::onProcess(std::shared_ptr<ExynosBuffer> buffer) {
    ExynosLogFunctionTrace();

    if (buffer.get() == nullptr) {
        /* invalid parameter */
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return false;
    }

    auto mHDR2SDRImpl = std::static_pointer_cast<ExynosHDR2SDRImpl>(mExternalImpl);

    if ((!mIsEnabled) ||
        (mHDR2SDRImpl.get() == nullptr) ||
        (!mHDR2SDRImpl->query(buffer))) {
        auto ret = bypassBuffer(buffer);

        ExynosLogV("[%s] bypass", __FUNCTION__);

        return ret;
    }

    /* allocate output buffer & set output info */
    GraphicBufferAttribute attr;

    attr.mWidth  = buffer->mImageInfo.nWidth;
    attr.mHeight = buffer->mImageInfo.nHeight;
    attr.mFormat = mFormat;
    attr.mUsage  = buffer->usage();

    AllocArg arg;
    arg.attr        = attr;
    arg.limit       = MAX_ALLOC_BUFFER_NUM;
    arg.checkLimit  = nullptr;
    arg.allocCount  = 0;

    auto optBuffer = allocBuffer(arg);
    if (!optBuffer) {
        return false;
    }
    std::shared_ptr<ExynosBuffer> outbuffer = *optBuffer;

    auto ret = mHDR2SDRImpl->run(buffer, outbuffer, mMode, mFrameCount);
    /* update output information to buffer */
    {
        outbuffer->mImageInfo.eFrameInfo    = buffer->mImageInfo.eFrameInfo;
        outbuffer->mImageInfo.stCropInfo    = buffer->mImageInfo.stCropInfo;
        outbuffer->mImageInfo.stHDRInfo     = buffer->mImageInfo.stHDRInfo;
        outbuffer->mImageInfo.nDataSpace    = buffer->mImageInfo.nDataSpace;
        outbuffer->mImageInfo.nPoc          = buffer->mImageInfo.nPoc;
        outbuffer->mImageInfo.nTimeStamp    = buffer->mImageInfo.nTimeStamp;

        auto src = buffer->metadata();
        auto dst = outbuffer->metadata();

        if ((src != nullptr) &&
            (dst != nullptr)) {
            memcpy(dst, src, sizeof(ExynosVideoMeta));
        }
    }

    if (ret) {
        /* from now on, it is SDR */
        const uint32_t clearInfo = ~(VIDEO_INFO_TYPE_COLOR_ASPECTS | VIDEO_INFO_TYPE_HDR_STATIC | VIDEO_INFO_TYPE_HDR_DYNAMIC);
        HDRInfo &hdrInfo = outbuffer->mImageInfo.stHDRInfo;
        hdrInfo.eChangedInfo = (hdrInfo.eChangedInfo & clearInfo) | VIDEO_INFO_TYPE_COLOR_ASPECTS;
        hdrInfo.eValidInfo   = (hdrInfo.eValidInfo & clearInfo) | VIDEO_INFO_TYPE_COLOR_ASPECTS;
        hdrInfo.sColorAspects.mRange        = RANGE_LIMITED;
        hdrInfo.sColorAspects.mPrimaries    = PRIMARIES_BT709;
        hdrInfo.sColorAspects.mTransfer     = TRANSFER_BT709;
        hdrInfo.sColorAspects.mMatrixCoeffs = MATRIX_COEFF_BT709;

        auto meta = outbuffer->metadata();
        if (meta != nullptr) {
            meta->eType = (ExynosVideoInfoType)(meta->eType & clearInfo);
        }

        ExynosLogD("[%s] buffer(fd(%d), ptr(%p)) is SDR", __FUNCTION__,
                    (outbuffer->handle())->data[0], outbuffer.get());
    } else {
        outbuffer->mImageInfo.eFrameInfo = FrameInfo::CorruptedFrame;

        ExynosLogD("[%s] buffer(fd(%d), ptr(%p)) does not have valid data", __FUNCTION__,
                    (outbuffer->handle())->data[0], outbuffer.get());
    }

    ExynosBufferInfo input, output;
    ExynosBufferInfo::reset(input);
    ExynosBufferInfo::reset(output);

    input.eDataInfo  = DataInfo::UsedData;
    input.obj  = buffer;

    output.eDataInfo = DataInfo::SingleData;
    output.obj = outbuffer;

    return processDone(input, output);
}

void ExynosHDR2SDRFilter::onApplyConfig(std::shared_ptr<ExynosParams> params) {
    ExynosLogFunctionTrace();

    if ((params.get() == nullptr) ||
        params->empty()) {
        /* there is nothing to change */
        return;
    }

    auto filterParams = std::static_pointer_cast<ExynosFilterParams>(params);

    {
        auto filterParam = filterParams->getParam(ExynosParamIndex::HDR2SDRIndex, mID);
        if (filterParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamHDR2SDR>>(filterParam->getBaseParam());

            if (param->m.enable != 0) {
                mIsEnabled = true;
                ExynosLogD("[%s] hdr2sdr is enabled", __FUNCTION__);
            }
        }
    }

    {
        auto filterParam = filterParams->getParam(ExynosParamIndex::HDR2SDRModeIndex, mID);
        if (filterParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamHDR2SDRMode>>(filterParam->getBaseParam());

            mMode = param->m.type;
            ExynosLogD("[%s] hdr2sdr mode: %d", __FUNCTION__, mMode);
        }
    }

    {
        auto filterParam = filterParams->getParam(ExynosParamIndex::HDR2SDRPixelFormatIndex, mID);
        if (filterParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamHDR2SDRPixelFormat>>(filterParam->getBaseParam());

            if (param->m.format != 0) {
                mFormat = param->m.format;
            }

            ExynosLogD("[%s] hdr2sdr format: 0x%x", __FUNCTION__, mFormat);
        }
    }

    {
        auto filterParam = filterParams->getParam(ExynosParamIndex::HDR2SDRFrameCountIndex, mID);
        if (filterParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamHDR2SDRFrameCount>>(filterParam->getBaseParam());

            if (param->m.value > 0) {
                mFrameCount = param->m.value;
            }

            ExynosLogD("[%s] hdr2sdr frame count: %d", __FUNCTION__, mFrameCount);
        }
    }

    return;
}

