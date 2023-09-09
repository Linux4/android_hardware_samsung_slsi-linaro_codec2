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
#include <thread>
#include <chrono>

#include <system/graphics.h>
#include "exynos_format.h"

#include "Exynos_FilmGrain_Filter.h"

#define LOG_ON
#include "ExynosLog.h"
#define LOG_TAG "ExynosFilmGrainFilter"

#define MAX_ALLOC_BUFFER_NUM EXTRA_INTERNAL_BUFFER_NUM

constexpr char LIB_NAME[]             = "libFilmGrainNoise.so";
constexpr char LIB_FN_NAME_CREATE[]   = "CreateFilmGrainNoiseFactory";
constexpr char LIB_FN_NAME_DESTROY[]  = "DestroyFilmGrainNoiseFactory";

#define LUM_SCALING_POS_SIZE 14
#define CHR_SCALING_POS_SIZE 10
#define LUM_AR_COEF_SIZE 24
#define CHR_AR_COEF_SIZE 25

enum class eDevType {
    CPU,
    GPU
};

enum class eImgFormat {
    YV12,
    NV12
};

enum class eSecureMode {
    Disable,
    Enable
};

typedef struct FilmGrainNoiseConfig {
    int PicSizeX;
    int PicSizeY;
    int BitDepth;
    int ApplyGrain;

    int NumPosY;
    int NumPosCb;
    int NumPosCr;
    int ScalingShift;
    uint8_t ScalingPosY0[LUM_SCALING_POS_SIZE];   // x
    uint8_t ScalingPosY1[LUM_SCALING_POS_SIZE];   // y
    uint8_t ScalingPosCb0[CHR_SCALING_POS_SIZE];  // x
    uint8_t ScalingPosCb1[CHR_SCALING_POS_SIZE];  // y
    uint8_t ScalingPosCr0[CHR_SCALING_POS_SIZE];  // x
    uint8_t ScalingPosCr1[CHR_SCALING_POS_SIZE];  // y

    int8_t ArCoeffY[LUM_AR_COEF_SIZE];
    int8_t ArCoeffCb[CHR_AR_COEF_SIZE];
    int8_t ArCoeffCr[CHR_AR_COEF_SIZE];
    int ArCoeffLag;
    int ArCoeffShift;

    int CbMult;
    int CbLumaMult;
    int CbOffset;
    int CrMult;
    int CrLumaMult;
    int CrOffset;

    int OverlapFlag;
    int ClipToRestrictedRange;
    int ChromaScalingFromLuma;
    int GrainScaleShift;
    uint16_t RandomSeed;
    int McIdentity;

    void *SrcY;
    void *SrcU;
    void *SrcV;
    void *SrcUV;  // interleaved UV of NV21
    void *DstY;
    void *DstU;
    void *DstV;
    void *DstUV;  // interleaved UV of NV21

    int SrcYFd;
    int SrcUFd;
    int SrcVFd;
    int SrcUVFd;  // interleaved UV of NV21
    int DstYFd;
    int DstUFd;
    int DstVFd;
    int DstUVFd;  // interleaved UV of NV21
} FilmGrainNoiseConfig;

class FilmGrainNoiseInterface {
public:
    virtual ~FilmGrainNoiseInterface() = default;
    virtual int Init(int bitDepth, eImgFormat imgFormat, eDevType devType, eSecureMode secureMode) = 0;
    virtual int Deinit() = 0;
    virtual int Run(FilmGrainNoiseConfig *Config) = 0;
};

typedef FilmGrainNoiseInterface *(*CreateFilmGrainIntfFunc)();
typedef void (*DestroyFilmGrainFunc)(FilmGrainNoiseInterface *factory);

class ExynosFilmGrainImpl : public ExynosExternalImpl {
public:
    struct InitConfig {
        int         bitdepth;
        eImgFormat  imgFormat;
        eDevType    devType;
        eSecureMode secureMode;
    };

    ExynosFilmGrainImpl(std::string name, bool isSecure = false) : ExynosExternalImpl(name) {
        mbLogOff = false;

        mHandle      = nullptr;
        mCreate      = nullptr;
        mDestroy     = nullptr;
        mInterface   = nullptr;

        memset(&mInitConfig, 0, sizeof(mInitConfig));
        mIsConfigured = false;
        mIsSecure = isSecure;
    }

    ~ExynosFilmGrainImpl() {
        unload();
    }

    bool load() override;
    void unload() override;
    bool run(ExynosBufferInfo &input, ExynosBufferInfo &output, FilmGrainInfo &info);

private:
    bool init(InitConfig config);
    void deinit();

    void                    *mHandle;
    CreateFilmGrainIntfFunc  mCreate;
    DestroyFilmGrainFunc     mDestroy;
    FilmGrainNoiseInterface *mInterface;

    struct InitConfig mInitConfig;
    bool mIsConfigured;

    bool mIsSecure;
};

bool ExynosFilmGrainImpl::load() {
    ExynosLogFunctionTrace();

    if (mHandle != nullptr) {
        /* lib is already loaded */
        return true;
    }

    if (!ExynosUtils::GetFilmgrainType()) {
        return false;
    }

    mHandle = dlopen(LIB_NAME, RTLD_NOW | RTLD_GLOBAL);
    if (mHandle == nullptr) {
        ExynosLogD("[%s] dlopen(%s) is failed. reason(%s)", __FUNCTION__, LIB_NAME, dlerror());
        return false;
    }

    mCreate  = (CreateFilmGrainIntfFunc)dlsym(mHandle, LIB_FN_NAME_CREATE);
    mDestroy = (DestroyFilmGrainFunc)dlsym(mHandle, LIB_FN_NAME_DESTROY);

    if ((mCreate == nullptr) ||
        (mDestroy == nullptr)) {
        mCreate  = nullptr;
        mDestroy = nullptr;

        dlclose(mHandle);
        mHandle = nullptr;

        ExynosLogE("[%s] dlsym() is failed", __FUNCTION__);
        return false;
    }

    mInterface = (FilmGrainNoiseInterface *)mCreate();
    if (mInterface == nullptr) {
        mCreate  = nullptr;
        mDestroy = nullptr;

        dlclose(mHandle);
        mHandle = nullptr;

        ExynosLogE("[%s] CreateFilmGrainNoiseFactory() is failed", __FUNCTION__);
        return false;
    }

    return true;
}

void ExynosFilmGrainImpl::unload() {
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

bool ExynosFilmGrainImpl::run(
    ExynosBufferInfo &input,
    ExynosBufferInfo &output,
    FilmGrainInfo    &info) {
    ExynosLogFunctionTrace();

    if (mInterface == nullptr) {
        /* interface is invalid */
        return false;
    }

    if ((input.stImageInfo.nFormat != HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M) &&
        (input.stImageInfo.nFormat != HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M) &&
        (input.stImageInfo.nFormat != HAL_PIXEL_FORMAT_EXYNOS_YV12_M)) {
        ExynosLogE("[%s] format(0x%x) is not supported", __FUNCTION__, input.stImageInfo.nFormat);
        return false;
    }

    FilmGrainNoiseConfig config;
    memset(&config, 0, sizeof(config));

    config.PicSizeX = input.stImageInfo.nStride;
    config.PicSizeY = input.stImageInfo.nHeight;
    config.BitDepth = (input.stImageInfo.nFormat == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M)? 10:8;

    if (!mIsConfigured) {
        InitConfig initConfig;

        initConfig.bitdepth     = config.BitDepth;
        initConfig.imgFormat    = (input.stImageInfo.nFormat == HAL_PIXEL_FORMAT_EXYNOS_YV12_M)? eImgFormat::YV12:eImgFormat::NV12;
        initConfig.devType      = eDevType::GPU;
        initConfig.secureMode   = mIsSecure? eSecureMode::Enable:eSecureMode::Disable;

        if (init(initConfig) == false) {
            return false;
        }
    }

    config.ApplyGrain   = info.apply_grain;
    config.NumPosY      = info.num_y_points;
    config.NumPosCb     = info.num_cb_points;
    config.NumPosCr     = info.num_cr_points;
    config.ScalingShift = info.grain_scaling_minus_8;

    for (int i = 0; i < LUM_SCALING_POS_SIZE; i++) {
        config.ScalingPosY0[i]= info.point_y_value[i];
        config.ScalingPosY1[i]= info.point_y_scaling[i];
    }

    for (int i = 0; i < CHR_SCALING_POS_SIZE; i++) {
        config.ScalingPosCb0[i]= info.point_cb_value[i];
        config.ScalingPosCb1[i]= info.point_cb_scaling[i];

        config.ScalingPosCr0[i]= info.point_cr_value[i];
        config.ScalingPosCr1[i]= info.point_cr_scaling[i];
    }

    for (int i = 0; i < LUM_AR_COEF_SIZE; i++) {
        config.ArCoeffY[i] = info.ar_coeffs_y_plus_128[i];
    }

    for (int i = 0; i < CHR_AR_COEF_SIZE; i++) {
        config.ArCoeffCb[i] = info.ar_coeffs_cb_plus_128[i];

        config.ArCoeffCr[i] = info.ar_coeffs_cr_plus_128[i];
    }

    config.ArCoeffLag       = info.ar_coeff_lag;
    config.ArCoeffShift     = info.ar_coeff_shift_minus_6;

    config.CbMult       = info.cb_mult;
    config.CbLumaMult   = info.cb_luma_mult;
    config.CbOffset     = info.cb_offset;
    config.CrMult       = info.cr_mult;
    config.CrLumaMult   = info.cr_luma_mult;
    config.CrOffset     = info.cr_offset;

    config.OverlapFlag              = info.overlap_flag;
    config.ClipToRestrictedRange    = info.clip_to_restricted_range;
    config.ChromaScalingFromLuma    = info.chroma_scaling_from_luma;
    config.GrainScaleShift          = info.grain_scale_shift;
    config.RandomSeed               = info.grain_seed;
    config.McIdentity               = info.mc_identity;

    switch (input.stImageInfo.nFormat) {
    case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
        config.SrcYFd  = input.nFD[0];
        config.SrcVFd  = input.nFD[1];
        config.SrcUFd  = input.nFD[2];
        break;
    default:
        config.SrcYFd  = input.nFD[0];
        config.SrcUVFd = input.nFD[1];
        break;
    }

    switch (output.stImageInfo.nFormat) {
    case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
        config.DstYFd  = output.nFD[0];
        config.DstVFd  = output.nFD[1];
        config.DstUFd  = output.nFD[2];
        break;
    default:
        config.DstYFd  = output.nFD[0];
        config.DstUVFd = output.nFD[1];
        break;
    }

    if (mInterface->Run(&config) != 0) {
        ExynosLogE("[%s] Run() is failed", __FUNCTION__);
        deinit();  /* requirement */
        return false;
    }

    ExynosLogD("[%s] film grain is applied", __FUNCTION__);

    return true;
}

bool ExynosFilmGrainImpl::init(InitConfig config) {
    ExynosLogFunctionTrace();

    if (mHandle == nullptr) {
        /* it is not supported */
        return false;
    }

    if (mIsConfigured) {
        if (memcmp(&config, &mInitConfig, sizeof(config))) {
            /* already initialized */
            return true;
        } else {
            deinit();
        }
    }

    if (mInterface != nullptr) {
        if (mInterface->Init(config.bitdepth, config.imgFormat, config.devType, config.secureMode) != 0) {
            ExynosLogE("[%s] init(bit:%d, format:0x%x, type:%d) is failed", __FUNCTION__,
                            config.bitdepth, config.imgFormat, config.devType);
            memset(&mInitConfig, 0, sizeof(mInitConfig));
            mInterface->Deinit();  /* requirement */
            return false;
        }

        memcpy(&mInitConfig, &config, sizeof(mInitConfig));
        mIsConfigured = true;

        return true;
    }

    return false;
}

void ExynosFilmGrainImpl::deinit() {
    ExynosLogFunctionTrace();

    if (mIsConfigured) {
        if (mInterface != nullptr) {
            if (mInterface->Deinit() != 0) {
                ExynosLogE("[%s] deinit() is failed", __FUNCTION__);
            }
        }

        memset(&mInitConfig, 0, sizeof(mInitConfig));
        mIsConfigured  = false;
    }

    return;
}

bool ExynosFilmGrainFilter::onStart() {
    ExynosLogFunctionTrace();

    if (mExternalImpl.get() == nullptr) {
        mExternalImpl = std::make_shared<ExynosFilmGrainImpl>(mObjName, mIsSecure);
    }

    return true;
}

bool ExynosFilmGrainFilter::onReset() {
    ExynosLogFunctionTrace();

    memset(&mInfo, 0, sizeof(mInfo));

    return true;
}

bool ExynosFilmGrainFilter::onProcess(std::shared_ptr<ExynosBuffer> buffer) {
    ExynosLogFunctionTrace();

    if (buffer.get() == nullptr) {
        /* invalid parameter */
        ExynosLogE("[%s] invalid parameter", __FUNCTION__);
        return false;
    }

    bool ret = false;

    if ((mInfo.apply_grain == 0) ||
        (mExternalImpl.get() == nullptr)) {
        ret = bypassBuffer(buffer);

        ExynosLogV("[%s] bypass", __FUNCTION__);

        return ret;
    }

    ExynosBufferInfo input, output;
    ExynosBufferInfo::reset(input);
    ExynosBufferInfo::reset(output);

    /* set input info */
    {
        memcpy(&(input.stImageInfo), &(buffer->mImageInfo), sizeof(input.stImageInfo));

        ExynosUtils::GetYUVPlaneInfo(input.stImageInfo.nStride, input.stImageInfo.nHeight, input.stImageInfo.nFormat,
                                     input.nPlane, input.nAllocLen, buffer->getFlags());

        auto handle = buffer->handle();

        for (int i = 0; i < input.nPlane; i++) {
            input.nFD[i] = handle->data[i];
            ExynosLogV("[%s] input buffer info : fd(%d)", __FUNCTION__, input.nFD[i]);
        }

        input.obj = buffer;
    }

    /* allocate output buffer & set output info */
    GraphicBufferAttribute attr;

    attr.mWidth  = input.stImageInfo.nWidth;
    attr.mHeight = input.stImageInfo.nHeight;
    attr.mFormat = input.stImageInfo.nFormat;
    attr.mUsage  = buffer->usage();

    if (input.stImageInfo.nFormat == HAL_PIXEL_FORMAT_EXYNOS_YV12_M) {
        attr.mFormat = HAL_PIXEL_FORMAT_YV12;
    }

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

    /* set output info */
    {
        output.eDataInfo                = DataInfo::NoData;
        output.stImageInfo.eFrameInfo   = buffer->mImageInfo.eFrameInfo;
        output.stImageInfo.nWidth       = input.stImageInfo.nWidth;
        output.stImageInfo.nHeight      = input.stImageInfo.nHeight;
        output.stImageInfo.nStride      = outbuffer->mImageInfo.nStride;  /* must follow graphic buffer's stirde */
        output.stImageInfo.nFormat      = outbuffer->format();

        output.stImageInfo.stCropInfo = input.stImageInfo.stCropInfo;
        output.stImageInfo.stHDRInfo  = input.stImageInfo.stHDRInfo;
        output.stImageInfo.nDataSpace = input.stImageInfo.nDataSpace;
        output.stImageInfo.nPoc       = input.stImageInfo.nPoc;
        output.stImageInfo.nTimeStamp = input.stImageInfo.nTimeStamp;

        ExynosUtils::GetYUVPlaneInfo(output.stImageInfo.nStride, output.stImageInfo.nHeight, output.stImageInfo.nFormat,
                                     output.nPlane, output.nAllocLen, buffer->getFlags());

        auto handle = outbuffer->handle();

        for (int i = 0; i < output.nPlane; i++) {
            output.nFD[i] = handle->data[i];
            ExynosLogV("[%s] output buffer info : fd(%d)", __FUNCTION__, output.nFD[i]);
        }

        output.obj = outbuffer;
    }

    auto mFilmGrainImpl = std::static_pointer_cast<ExynosFilmGrainImpl>(mExternalImpl);

    if (mFilmGrainImpl->run(input, output, mInfo)) {
        input.eDataInfo  = DataInfo::UsedData;
        output.eDataInfo = DataInfo::SingleData;

        /* update output information to buffer */
        outbuffer->mImageInfo = output.stImageInfo;

        auto src = input.obj->metadata();
        auto dst = output.obj->metadata();

        if ((src != nullptr) &&
            (dst != nullptr)) {
            memcpy(dst, src, sizeof(ExynosVideoMeta));
        }

        return processDone(input, output);
    }

    /* not processed */
    ret = bypassBuffer(buffer);

    ExynosLogV("[%s] bypass", __FUNCTION__);

    return ret;
}

void ExynosFilmGrainFilter::onApplyConfig(std::shared_ptr<ExynosParams> params) {
    ExynosLogFunctionTrace();

    if ((params.get() == nullptr) ||
        params->empty()) {
        /* there is nothing to change */
        return;
    }

    auto filterParams = std::static_pointer_cast<ExynosFilterParams>(params);

    {
        auto filterParam = filterParams->getParam(ExynosParamIndex::FilmGrainInfoIndex, mID);
        if (filterParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamFilmGrainInfo>>(filterParam->getBaseParam());

            ExynosLogD("[%s] film grain info : apply(%d), update(%d)", __FUNCTION__,
                        param->m.info.apply_grain, param->m.info.update_grain);

            memcpy(&mInfo, &(param->m.info), sizeof(mInfo));
        }
    }

    return;
}

