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
#include <string>
#include <map>
#include <system/graphics.h>
#include "exynos_format.h"

#include "ExynosGraphicBuffer.h"

#include "ExynosVideoCodecDec.h"
#include "ExynosVideoApi.h"
#include "ExynosVideoCodecCommon.h"

#define LOG_ON
#include "ExynosLog.h"

#define TODO(willUse) (void)willUse

#define EXTRA_DPB_NUM 5
#define USE_DPB_MANAGER  /* in order to hold a graphic blob, manage ref. DPB information */

#define UHD_IMAGE_SIZE (4096 * 2304)

#ifdef USE_FLEXIBLE_P010
#define DEFAULT_PIXEL_FORMAT HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN
#else
#define DEFAULT_PIXEL_FORMAT HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M
#endif

constexpr bool INPUT_FIXED_FD_ENABLE = true;
constexpr bool OUTPUT_FIXED_FD_ENABLE = true;

static ExynosVideoCodingType getVc1FormatType(void *pInputStream) {
    ExynosVideoCodingType ret = VIDEO_CODING_UNKNOWN;
    char *pCheckBuffer = (char *)pInputStream;

    if (pInputStream == NULL) {
        StaticExynosLog(Level::Error, "[%s] pInputStream is null", __FUNCTION__);
        return ret;
    }

    if ((pCheckBuffer[3] == 0xC5) &&
        (pCheckBuffer[4] == 0x04)) {
        StaticExynosLog(Level::Info, "[%s] WMV_FORMAT_WMV3", __FUNCTION__);
        ret = VIDEO_CODING_VC1_RCV;
    } else if (((pCheckBuffer[3] == 0x01) &&
                    (pCheckBuffer[4] == 0x0F)) ||
               ((pCheckBuffer[2] == 0x01) &&
                    (pCheckBuffer[3] == 0x0F))) {
        StaticExynosLog(Level::Info, "[%s] WMV_FORMAT_VC1", __FUNCTION__);
        ret = VIDEO_CODING_VC1;
    } else {
        StaticExynosLog(Level::Warning, "[%s] WMV_FORMAT_UNKNOWN", __FUNCTION__);
    }

    return ret;
}

static void updateHdrDynamicInfo(
    ExynosHdrDynamicInfo &dstInfo,
    ExynosVideoHdrInfo   &srcInfo) {
    dstInfo.valid = (srcInfo.eChangedType & HDR_INFO_DYNAMIC_META)? 1:0;

    dstInfo.data.country_code = srcInfo.sHdrDynamic.itu_t_t35_country_code;

    dstInfo.data.country_code              = srcInfo.sHdrDynamic.itu_t_t35_country_code;
    dstInfo.data.provider_code             = srcInfo.sHdrDynamic.itu_t_t35_terminal_provider_code;
    dstInfo.data.provider_oriented_code    = srcInfo.sHdrDynamic.itu_t_t35_terminal_provider_oriented_code;
    dstInfo.data.application_identifier    = srcInfo.sHdrDynamic.application_identifier;
    dstInfo.data.application_version       = srcInfo.sHdrDynamic.application_version;

#ifdef USE_FULL_ST2094_40
    dstInfo.data.num_windows               = srcInfo.sHdrDynamic.num_windows;
    int i, j;

#if 0  /* These infos are supported at blob type data, but not ExynosVideHdrDynamic type data.
        * So, if below data is supported by MFC Driver, annotation could be removed.
        */
    for (i = 1; i < dstInfo.data.num_windows; i++) {
        dstInfo.data.window_upper_left_corner_x[i - 1]      = srcInfo.sHdrDynamic.window_upper_left_corner_x[i - 1];
        dstInfo.data.window_upper_left_corner_y[i - 1]      = srcInfo.sHdrDynamic.window_upper_left_corner_y[i - 1];
        dstInfo.data.window_lower_right_corner_x[i - 1]     = srcInfo.sHdrDynamic.window_lower_right_corner_x[i - 1];
        dstInfo.data.window_lower_right_corner_y[i - 1]     = srcInfo.sHdrDynamic.window_lower_right_corner_y[i - 1];
        dstInfo.data.center_of_ellipse_x[i - 1]             = srcInfo.sHdrDynamic.center_of_ellipse_x[i - 1];
        dstInfo.data.center_of_ellipse_y[i - 1]             = srcInfo.sHdrDynamic.center_of_ellipse_y[i - 1];
        dstInfo.data.rotation_angle[i - 1]                  = srcInfo.sHdrDynamic.rotation_angle[i - 1];
        dstInfo.data.semimajor_axis_internal_ellipse[i - 1] = srcInfo.sHdrDynamic.semimajor_axis_internal_ellipse[i - 1];
        dstInfo.data.semimajor_axis_external_ellipse[i - 1] = srcInfo.sHdrDynamic.semimajor_axis_external_ellipse[i - 1];
        dstInfo.data.semiminor_axis_external_ellipse[i - 1] = srcInfo.sHdrDynamic.semiminor_axis_external_ellipse[i - 1];
        dstInfo.data.overlap_process_option[i - 1]          = srcInfo.sHdrDynamic.overlap_process_option[i - 1];
    }
#endif
    dstInfo.data.targeted_system_display_maximum_luminance          = srcInfo.sHdrDynamic.targeted_system_display_maximum_luminance;
    dstInfo.data.targeted_system_display_actual_peak_luminance_flag = srcInfo.sHdrDynamic.targeted_system_display_actual_peak_luminance_flag;

    if (dstInfo.data.targeted_system_display_actual_peak_luminance_flag) {
        dstInfo.data.num_rows_targeted_system_display_actual_peak_luminance = srcInfo.sHdrDynamic.num_rows_targeted_system_display_actual_peak_luminance;
        dstInfo.data.num_cols_targeted_system_display_actual_peak_luminance = srcInfo.sHdrDynamic.num_cols_targeted_system_display_actual_peak_luminance;

#if 0       /* These infos are supported at blob type data, but not ExynosVideHdrDynamic type data.
             * So, if below data is supported by MFC Driver, annotation could be removed.
             */
        for (i = 0; i < dstInfo.data.num_rows_targeted_system_display_actual_peak_luminance; i++)
            for (j = 0; j < dstInfo.data.num_cols_targeted_system_display_actual_peak_luminance; j++)
                dstInfo.data.targeted_system_display_actual_peak_luminance[i][j] = srcInfo.sHdrDynamic.targeted_system_display_actual_peak_luminance[i][j];
#endif
    }

    for (i = 0; i < dstInfo.data.num_windows; i++) {
        ExynosVideoHdrWindowInfo *pWindowInfo = &(srcInfo.sHdrDynamic.window_info[i]);

        /* maxscl */
        for (j = 0; j < 3; j++)
            dstInfo.data.maxscl[i][j] = pWindowInfo->maxscl[j];

        /* average maxrgb */
        dstInfo.data.average_maxrgb[i] = pWindowInfo->average_maxrgb;

        /* distribution maxrgb */
        dstInfo.data.num_maxrgb_percentiles[i] = pWindowInfo->num_distribution_maxrgb_percentiles;
        for (j = 0; j < dstInfo.data.num_maxrgb_percentiles[i]; j++) {
            dstInfo.data.maxrgb_percentages[i][j] = pWindowInfo->distribution_maxrgb_percentages[j];
            dstInfo.data.maxrgb_percentiles[i][j] = pWindowInfo->distribution_maxrgb_percentiles[j];
        }

        /* fraction bright pixels */
        dstInfo.data.fraction_bright_pixels[i] = pWindowInfo->fraction_bright_pixels;

        /* tone mapping curve */
        dstInfo.data.tone_mapping.tone_mapping_flag[i] = pWindowInfo->tone_mapping_flag;

        if (dstInfo.data.tone_mapping.tone_mapping_flag[i]) {
            dstInfo.data.tone_mapping.knee_point_x[i] = pWindowInfo->knee_point_x;
            dstInfo.data.tone_mapping.knee_point_y[i] = pWindowInfo->knee_point_y;

            dstInfo.data.tone_mapping.num_bezier_curve_anchors[i] = pWindowInfo->num_bezier_curve_anchors;
            for (j = 0; j < dstInfo.data.tone_mapping.num_bezier_curve_anchors[i]; j++)
                dstInfo.data.tone_mapping.bezier_curve_anchors[i][j] = pWindowInfo->bezier_curve_anchors[j];
        }

        /* color saturation weight */
        dstInfo.data.color_saturation_mapping_flag[i] = pWindowInfo->color_saturation_mapping_flag;

        if (dstInfo.data.color_saturation_mapping_flag[i])
            dstInfo.data.color_saturation_weight[i] = pWindowInfo->color_saturation_weight;
    }

    dstInfo.data.mastering_display_actual_peak_luminance_flag = srcInfo.sHdrDynamic.mastering_display_actual_peak_luminance_flag;

    if (dstInfo.data.mastering_display_actual_peak_luminance_flag) {
        dstInfo.data.num_rows_mastering_display_actual_peak_luminance = srcInfo.sHdrDynamic.num_rows_mastering_display_actual_peak_luminance;
        dstInfo.data.num_cols_mastering_display_actual_peak_luminance = srcInfo.sHdrDynamic.num_cols_mastering_display_actual_peak_luminance;

#if 0       /* These infos are supported at blob type data, but not ExynosVideHdrDynamic type data.
             * So, if below data is supported by MFC Driver, annotation could be removed.
             */
        for (i = 0; i < dstInfo.data.num_rows_mastering_display_actual_peak_luminance; i++)
            for (j = 0; j < dstInfo.data.num_cols_mastering_display_actual_peak_luminance; j++)
                dstInfo.data.mastering_display_actual_peak_luminance[i][j] = srcInfo.sHdrDynamic.mastering_display_actual_peak_luminance[i][j];
#endif
    }
#else // USE_FULL_ST2094_40
    dstInfo.data.display_maximum_luminance = srcInfo.sHdrDynamic.targeted_system_display_maximum_luminance;

    if (srcInfo.sHdrDynamic.num_windows > 0) {
        /* save information on window-0 only */
        ExynosVideoHdrWindowInfo *pWindowInfo = &(srcInfo.sHdrDynamic.window_info[0]);
        int i;

        /* maxscl */
        for (i = 0; i < (int)(sizeof(dstInfo.data.maxscl)/sizeof(dstInfo.data.maxscl[0])); i++)
            dstInfo.data.maxscl[i] = pWindowInfo->maxscl[i];

        /* distribution maxrgb */
        dstInfo.data.num_maxrgb_percentiles = pWindowInfo->num_distribution_maxrgb_percentiles;
        for (i = 0; i < pWindowInfo->num_distribution_maxrgb_percentiles; i++) {
            dstInfo.data.maxrgb_percentages[i] = pWindowInfo->distribution_maxrgb_percentages[i];
            dstInfo.data.maxrgb_percentiles[i] = pWindowInfo->distribution_maxrgb_percentiles[i];
        }

        /* tone mapping curve */
        dstInfo.data.tone_mapping.tone_mapping_flag = pWindowInfo->tone_mapping_flag;
        if (pWindowInfo->tone_mapping_flag != 0) {
            dstInfo.data.tone_mapping.knee_point_x = pWindowInfo->knee_point_x;
            dstInfo.data.tone_mapping.knee_point_y = pWindowInfo->knee_point_y;

            dstInfo.data.tone_mapping.num_bezier_curve_anchors = pWindowInfo->num_bezier_curve_anchors;
            for (i = 0; i < pWindowInfo->num_bezier_curve_anchors; i++)
                dstInfo.data.tone_mapping.bezier_curve_anchors[i] = pWindowInfo->bezier_curve_anchors[i];
        }
    }
#endif // USE_FULL_ST2094_40

    return;
}

class ExynosVideoCodecDec::CodecDecImpl : public CodecImpl {
public:
    CodecDecImpl(std::string name) : CodecImpl(name) {
        mbLogOff = false;

        memset(&mVideoInstInfo, 0, sizeof(mVideoInstInfo));
        memset(&mOutGeometry, 0, sizeof(mOutGeometry));
        mOutGeometry.HdrInfo.sColorAspects.eRangeType       = RANGE_UNSPECIFIED;
        mOutGeometry.HdrInfo.sColorAspects.ePrimariesType   = PRIMARIES_UNSPECIFIED;
        mOutGeometry.HdrInfo.sColorAspects.eTransferType    = TRANSFER_UNSPECIFIED;
        mOutGeometry.HdrInfo.sColorAspects.eCoeffType       = MATRIX_COEFF_UNSPECIFIED;
        mNumDPB = 0;
        mNumDispDelay = 0;
        mRefreshHDRInfo = false;
        memset(&mHDRInfo, 0, sizeof(mHDRInfo));
        mDataSpace = HAL_DATASPACE_UNKNOWN;
        mIsThumbnail = false;
        mIsHeaderDecoded = false;
        mIsCompressedColor = false;

        mGeometry = &mOutGeometry;
    }

    ~CodecDecImpl() = default;

    ExynosVideoErrorType    enableDecodeOrder();
    ExynosVideoErrorType    setSearchBlackBar(bool enable);
    ExynosVideoErrorType    setDisplayDelay(uint32_t delay);
    ExynosVideoErrorType    enableThumbnailMode();
    int                     getActualFormat();
    ExynosVideoBoolType     checkSupportedFormat(ExynosVideoColorFormatType format);
    ExynosVideoErrorType    setOperatingRate(uint32_t framerate);
    ExynosVideoErrorType    setRealTimePriority(uint32_t realTimePriority);
    ExynosErrorType         checkRealTimeResource(int32_t width, int32_t height, int32_t operatingRate);

    ExynosVideoGeometry mOutGeometry;
    int                 mNumDPB;
    int                 mNumDispDelay;

    bool mRefreshHDRInfo;   /* forcefully re-inform HDRInfo */
    HDRInfo mHDRInfo;       /* given from media framework */
    int mDataSpace;         /* final value for vendor path */

    bool mIsThumbnail;
    bool mIsHeaderDecoded;
    bool mIsCompressedColor;

private:
    CodecDecImpl() = delete;
};

ExynosVideoErrorType ExynosVideoCodecDec::CodecDecImpl::enableDecodeOrder() {
    ExynosLogFunctionTrace();

    ExynosVideoErrorType ret = std::get<ExynosVideoDecOps>(mCommonOps).Enable_DecodeOrder(mHandle);

    return ret;
}

ExynosVideoErrorType ExynosVideoCodecDec::CodecDecImpl::setSearchBlackBar(bool enable) {
    ExynosLogFunctionTrace();

    ExynosVideoErrorType ret = std::get<ExynosVideoDecOps>(mCommonOps).Set_SearchBlackBar(mHandle, (enable)? VIDEO_TRUE:VIDEO_FALSE);

    return ret;
}

ExynosVideoErrorType ExynosVideoCodecDec::CodecDecImpl::setDisplayDelay(uint32_t delay) {
    ExynosLogFunctionTrace();

    ExynosVideoErrorType ret = std::get<ExynosVideoDecOps>(mCommonOps).Set_DisplayDelay(mHandle, delay);

    return ret;
}

ExynosVideoErrorType ExynosVideoCodecDec::CodecDecImpl::enableThumbnailMode() {
    ExynosLogFunctionTrace();

    ExynosVideoErrorType ret = std::get<ExynosVideoDecOps>(mCommonOps).Set_IFrameDecoding(mHandle);

    if (ret == VIDEO_ERROR_NONE) {
        mIsThumbnail = true;
    }

    return ret;
}

int ExynosVideoCodecDec::CodecDecImpl::getActualFormat() {
    ExynosLogFunctionTrace();

    return std::get<ExynosVideoDecOps>(mCommonOps).Get_ActualFormat(mHandle);
}

ExynosVideoBoolType ExynosVideoCodecDec::CodecDecImpl::checkSupportedFormat(ExynosVideoColorFormatType format) {
    for (int i = 0; i < VIDEO_COLORFORMAT_MAX; i++) {
        if (mVideoInstInfo.supportFormat[i] == format) {
            return VIDEO_TRUE;
        }
    }

    return VIDEO_FALSE;
}

ExynosVideoErrorType ExynosVideoCodecDec::CodecDecImpl::setOperatingRate(uint32_t framerate) {
    ExynosLogFunctionTrace();

    if (mHandle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    return std::get<ExynosVideoDecOps>(mCommonOps).Set_OperatingRate(mHandle, framerate);
}

ExynosVideoErrorType ExynosVideoCodecDec::CodecDecImpl::setRealTimePriority(uint32_t realTimePriority) {
    ExynosLogFunctionTrace();

    if (mHandle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    return std::get<ExynosVideoDecOps>(mCommonOps).Set_RealTimePriority(mHandle, realTimePriority);
}

ExynosErrorType ExynosVideoCodecDec::CodecDecImpl::checkRealTimeResource(
    int32_t width,
    int32_t height,
    int32_t operatingRate) {
    ExynosLogFunctionTrace();

    if (mHandle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return EXYNOS_ERROR_INVALID_PARAM;
    }

    ExynosVideoGeometry geometry;
    memset(&geometry, 0, sizeof(geometry));

    geometry.nWidth  = width;
    geometry.nHeight = height;
    geometry.eCompressionFormat = mVideoInstInfo.eCodecType;

    if (mInBufOps.Try_Geometry(mHandle, &geometry) != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] inbuf : Try_Geometry() for real time resource is failed", __FUNCTION__);
        return EXYNOS_ERROR_UNKNOWN;
    }

    if (std::get<ExynosVideoDecOps>(mCommonOps).Set_RealTimePriority(mHandle, 0 /* real time */) != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] Set_RealTimePriority for real time resource is failed", __FUNCTION__);
        return EXYNOS_ERROR_UNKNOWN;
    }

    if (std::get<ExynosVideoDecOps>(mCommonOps).Set_OperatingRate(mHandle, operatingRate) != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] Set_OperatingRate(%d) for real time resource is failed", __FUNCTION__, operatingRate);
        return EXYNOS_ERROR_UNKNOWN;
    }

    if (std::get<ExynosVideoDecOps>(mCommonOps).Get_RealTimePriority(mHandle) != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] Get_RealTimePriority for real time resource is failed", __FUNCTION__);
        return EXYNOS_ERROR_UNKNOWN;
    }

    return EXYNOS_ERROR_NONE;
}

#ifdef USE_DPB_MANAGER
class RefDPBManager : public SharedPtrManager, public ExynosLog {
public:
    RefDPBManager(const std::string name) : ExynosLog(name + "-RefDPBManager") {
        mbLogOff = false;
    }

    RefDPBManager(const std::string name, bool bFixedFDEnable) :
                    SharedPtrManager(bFixedFDEnable), ExynosLog(name + "-RefDPBManager") {
        mbLogOff = false;
    }

    ~RefDPBManager() {
        mMap.clear();
    }

    void *swapSharedPtrToPtr(std::shared_ptr<ExynosBuffer> shPtr, fds_t &fds) override {
        std::lock_guard<std::mutex> lock(mMutex);
        if (shPtr.get() != nullptr) {
            auto optKey = shPtr->getStIno();
            if (!optKey) {
                shPtr->setStIno(BufferFdManager::getStIno((shPtr->handle())->data[0]));
            }

            addRefDPB(shPtr);

            return SharedPtrManager::swapSharedPtrToPtr(shPtr, fds);
        }

        return nullptr;
    }

    std::shared_ptr<ExynosBuffer> swapPtrToSharedPtr(void *ptr, bool bDelayReturn) override {
        std::lock_guard<std::mutex> lock(mMutex);

        bDelayReturn = true;
        auto shPtr = SharedPtrManager::swapPtrToSharedPtr(ptr, bDelayReturn);

        if (shPtr.get() != nullptr) {
            uint64_t key;
            auto optKey = shPtr->getStIno();
            if (!optKey) {
                key = (uint64_t)BufferFdManager::getStIno((shPtr->handle())->data[0]);
                shPtr->setStIno(key);
            } else {
                key = *optKey;
            }

            if (!findRefDPB(key)) {
                ExynosLogW("[%s] ref count is already decreased: fd(%d)", __FUNCTION__, (shPtr->handle())->data[0]);
                // addRefDPB(shPtr);
            }
        }

        return shPtr;
    }

    void eraseSharedPtr(std::shared_ptr<ExynosBuffer> shPtr) override {
        std::lock_guard<std::mutex> lock(mMutex);
        if (shPtr != nullptr) {
            SharedPtrManager::eraseSharedPtr(shPtr);

            uint64_t key;
            auto optKey = shPtr->getStIno();
            if (!optKey) {
                key = (uint64_t)BufferFdManager::getStIno((shPtr->handle())->data[0]);
                shPtr->setStIno(key);
            } else {
                key = *optKey;
            }

            auto search = mMap.find(key);
            if (search != mMap.end()) {
                (search->second.second)--;

                ExynosLogT("[%s] delete ref DPB : key(%zu), orig(%p), cnt(%d)", __FUNCTION__,
                            key, (search->second.first).get(), search->second.second);

                if (search->second.second == 0) {
                    mMap.erase(search);
                }
            }
        }
    }

    void reset() override {
        std::lock_guard<std::mutex> lock(mMutex);
        SharedPtrManager::reset();

        for (auto it = mMap.begin(); it != mMap.end(); it++) {
            uint64_t key = it->first;

            (it->second.second) = 0;

            ExynosLogT("[%s] delete ref DPB : key(%zu), orig(%p), cnt(%d)", __FUNCTION__,
                        key, (it->second.first).get(), it->second.second);
        }

        mMap.clear();
    }

    int size() override {
        std::lock_guard<std::mutex> lock(mMutex);

        int qcnt = SharedPtrManager::size();

        ExynosLogV("[%s] QBUF(%d)", __FUNCTION__, qcnt);

        return qcnt;
    }

    void delRefDPB(RefDPBInfo &info) {
        std::lock_guard<std::mutex> lock(mMutex);

        for (int i = 0; i < VIDEO_BUFFER_MAX_NUM; i++) {
            if (info.dpbFD[i].fd <= 0) {
                break;
            }

            if (mMap.size() > 0) {
                auto search = mMap.find(BufferFdManager::getStIno(info.dpbFD[i].fd));
                if (search != mMap.end()) {
                    uint64_t key = search->first;
                    delayReturn(key);
                    ExynosLogT("[%s] ref key found", __FUNCTION__);

                    (search->second.second)--;

                    ExynosLogT("[%s] ref DPB : key(%zu), orig(%p), cnt(%d)", __FUNCTION__,
                            key, (search->second.first).get(), search->second.second);

                    if (search->second.second == 0) {
                        mMap.erase(search);
                    }
                }
            }
        }

        return;
    }

    int registeredDPBCount() {
        std::lock_guard<std::mutex> lock(mMutex);

        return mMap.size();
    }

    int referencedDPBCount() {
        std::lock_guard<std::mutex> lock(mMutex);

        int count = 0;

        for (const auto& [key, value] : mMap) {
            if (value.second > 1) {
                count++;
            }
        }

        return count;
    }

private:
    bool findRefDPB(std::shared_ptr<ExynosBuffer> buffer) {
        if ((mMap.size() > 0) && (buffer.get() != nullptr)) {
            uint64_t key;
            auto optKey = buffer->getStIno();
            if (!optKey) {
                key = (uint64_t)BufferFdManager::getStIno((buffer->handle())->data[0]);
                buffer->setStIno(key);
            } else {
                key = *optKey;
            }

            return (mMap.count(key) == 0)? false:true;
        }

        return false;
    }

    bool findRefDPB(uint64_t key) {
        return ((mMap.size() > 0) && (mMap.count(key) > 0))? true:false;
    }

    void addRefDPB(std::shared_ptr<ExynosBuffer> buffer) {
        if (buffer.get() != nullptr) {
            uint64_t key;
            auto optKey = buffer->getStIno();
            if (!optKey) {
                key = (uint64_t)BufferFdManager::getStIno((buffer->handle())->data[0]);
                buffer->setStIno(key);
            } else {
                key = *optKey;
            }

            auto search = mMap.find(key);
            if (search == mMap.end()) {
                search = mMap.insert(std::make_pair(key, std::make_pair(buffer->origin(), 1))).first;
            } else {
                (search->second.second)++;
            }

            ExynosLogT("[%s] ref DPB : key(%zu), orig(%p), cnt(%d)", __FUNCTION__,
                        key, (search->second.first).get(), search->second.second);
        }

        return;
    }

    void addRefDPB(uint64_t key, std::shared_ptr<ExynosBuffer> buffer) {
        if (buffer.get() != nullptr) {
            auto search = mMap.find(key);
            if (search == mMap.end()) {
                search = mMap.insert(std::make_pair(key, std::make_pair(buffer->origin(), 1))).first;
            } else {
                (search->second.second)++;
            }

            ExynosLogT("[%s] ref DPB : key(%zu), orig(%p), cnt(%d)", __FUNCTION__,
                        key, (search->second.first).get(), search->second.second);
        }

        return;
    }

    std::mutex mMutex;
    std::map<uint64_t, std::pair<std::shared_ptr<ExynosBuffer::ExynosBufferOrigin>, int>> mMap;
};
#endif

void DecCodecInfoRegistry::registCodecInfo() {
    if (mCodecInfo.get() != nullptr) {
        mCodecInfo->clear();
        mCodecInfo.reset();
    }

    std::map<ExynosVideoCodecBase::Type, CodecInfo> CodecInfoMap = {
        /* H.264 */
        {ExynosVideoCodecBase::Type::H264, {"ExynosVideoCodecDec-H264Dec", VIDEO_CODING_AVC, VIDEO_NORMAL, VIDEO_FALSE}},
        {ExynosVideoCodecBase::Type::H264_SECURE, {"ExynosVideoCodecDec-H264Dec.secure", VIDEO_CODING_AVC, VIDEO_SECURE, VIDEO_FALSE}},
        /* HEVC */
        {ExynosVideoCodecBase::Type::HEVC, {"ExynosVideoCodecDec-HevcDec", VIDEO_CODING_HEVC, VIDEO_NORMAL, VIDEO_FALSE}},
        {ExynosVideoCodecBase::Type::HEVC_SECURE, {"ExynosVideoCodecDec-HevcDec.secure", VIDEO_CODING_HEVC, VIDEO_SECURE, VIDEO_FALSE}},
        /* Mpeg4 */
        {ExynosVideoCodecBase::Type::MPEG4, {"ExynosVideoCodecDec-Mpeg4Dec", VIDEO_CODING_MPEG4, VIDEO_NORMAL, VIDEO_FALSE}},
        /* H.263 */
        {ExynosVideoCodecBase::Type::H263, {"ExynosVideoCodecDec-H263Dec", VIDEO_CODING_H263, VIDEO_NORMAL, VIDEO_FALSE}},
        /* VP8 */
        {ExynosVideoCodecBase::Type::VP8, {"ExynosVideoCodecDec-Vp8Dec", VIDEO_CODING_VP8, VIDEO_NORMAL, VIDEO_FALSE}},
        /* VP9 */
        {ExynosVideoCodecBase::Type::VP9, {"ExynosVideoCodecDec-Vp9Dec", VIDEO_CODING_VP9, VIDEO_NORMAL, VIDEO_FALSE}},
        {ExynosVideoCodecBase::Type::VP9_SECURE, {"ExynosVideoCodecDec-Vp9Dec.secure", VIDEO_CODING_VP9, VIDEO_SECURE, VIDEO_FALSE}},
        /* Mpeg2 */
        {ExynosVideoCodecBase::Type::MPEG2, {"ExynosVideoCodecDec-Mpeg2Dec", VIDEO_CODING_MPEG2, VIDEO_NORMAL, VIDEO_FALSE}},
        /* VC1 */
        {ExynosVideoCodecBase::Type::VC1, {"ExynosVideoCodecDec-Vc1Dec", VIDEO_CODING_VC1, VIDEO_NORMAL, VIDEO_FALSE}},
        /* WMV */
        {ExynosVideoCodecBase::Type::WMV, {"ExynosVideoCodecDec-WmvDec", VIDEO_CODING_VC1_RCV, VIDEO_NORMAL, VIDEO_FALSE}},
        /* AV1 */
        {ExynosVideoCodecBase::Type::AV1, {"ExynosVideoCodecDec-Av1Dec", VIDEO_CODING_AV1, VIDEO_NORMAL, VIDEO_FALSE}},
        {ExynosVideoCodecBase::Type::AV1_SECURE, {"ExynosVideoCodecDec-Av1Dec.secure", VIDEO_CODING_AV1, VIDEO_SECURE, VIDEO_FALSE}},
    };

    mCodecInfo = std::make_shared<std::map<ExynosVideoCodecBase::Type, CodecInfo>>(CodecInfoMap);
}

ExynosVideoCodecDec::ExynosVideoCodecDec(ExynosVideoCodecBase::Type type) {
    mbLogOff = false;

    auto codecInfo = getCodecInfo(type);

    mObjName    = codecInfo.objName;
    mCodecImpl  = std::make_shared<CodecDecImpl>(mObjName);
    mCodecImpl->mVideoInstInfo.eCodecType       = codecInfo.eCodecType;
    mCodecImpl->mVideoInstInfo.eSecurityType    = codecInfo.eSecurityType;

    setCodecImpl(mCodecImpl, false);

    mExynosPort[ExynosPort::Input].mBufManager  = std::make_shared<SharedPtrManager>(INPUT_FIXED_FD_ENABLE);
#ifdef USE_DPB_MANAGER
    mExynosPort[ExynosPort::Output].mBufManager = std::make_shared<RefDPBManager>(mObjName, OUTPUT_FIXED_FD_ENABLE);
#else
    mExynosPort[ExynosPort::Output].mBufManager = std::make_shared<SharedPtrManager>(OUTPUT_FIXED_FD_ENABLE);
#endif

    mCodecImpl->mVideoInstInfo.nMemoryType = VIDEO_MEMORY_DMABUF;

    mExynosPort[ExynosPort::Input].mIsConfigured = false;
    mExynosPort[ExynosPort::Output].mIsConfigured = false;

    {
        std::lock_guard<std::mutex> lock(mExynosPort[ExynosPort::Input].mStreamMutex);
        mExynosPort[ExynosPort::Input].mIsOn = false;
    }

    {
        std::lock_guard<std::mutex> lock(mExynosPort[ExynosPort::Output].mStreamMutex);
        mExynosPort[ExynosPort::Output].mIsOn = false;
    }

    init();
}

ExynosVideoCodecDec::~ExynosVideoCodecDec() {
    deinit();

    mCodecImpl.reset();
    mExynosPort[ExynosPort::Input].mBufManager->reset();
    mExynosPort[ExynosPort::Output].mBufManager->reset();
}

bool ExynosVideoCodecDec::isValidCodecHandle() {
    auto handle = mCodecImpl->mHandle;

    if (handle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return false;
    }

    return true;
}

ExynosVideoFrameStatusType ExynosVideoCodecDec::getDisplayStatus(ExynosVideoExtraInfo &extraInfo) {
    return extraInfo.specific.dec.displayStatus;
}

void ExynosVideoCodecDec::updateBufferInfo(ExynosVideoBuffer &buffer, ExynosBufferInfo &buf) {
    int frameType = buffer.extraInfo.specific.dec.frameType;

    ExynosLogD("[%s] frame type : %x", __FUNCTION__, frameType);

    if (frameType & VIDEO_FRAME_MULTI) {
        ExynosLogD("[%s] multi frame", __FUNCTION__);
        buf.eDataInfo = DataInfo::MultiData;
    }

    buf.stImageInfo.eFrameInfo = getFrameInfoType(frameType);

    buf.nID = getFrameTag(mCodecImpl);
    buf.stImageInfo.nPoc = buffer.extraInfo.specific.dec.nPoc;

    auto displayStatus = getDisplayStatus(buffer.extraInfo);

    ExynosLogD("[%s] output : dequeue / fd(%d), ptr(%p), id(%d), poc(%d)", __FUNCTION__,
                    buf.nFD[0], buf.obj.get(), buf.nID, buf.stImageInfo.nPoc);

    if (frameType & VIDEO_FRAME_CORRUPT) {
        buf.stImageInfo.eFrameInfo = buf.stImageInfo.eFrameInfo | FrameInfo::CorruptedFrame;
        ExynosLogD("[%s] frame type : Corrupted frame", __FUNCTION__);
    }

    if (mCodecImpl->mOutGeometry.bInterlaced == VIDEO_TRUE) {
        buf.stImageInfo.eFrameInfo = buf.stImageInfo.eFrameInfo | FrameInfo::InterlacedFrame;
        ExynosLogD("[%s] frame type : Interlaced frame", __FUNCTION__);
    }

    /* Inter Resolution Changed */
    if (displayStatus == VIDEO_FRAME_STATUS_DISPLAY_INTER_RESOL_CHANGE) {
        updateResolution(true);
    }

    /* resolution info */
    {
        auto geometry = mCodecImpl->mOutGeometry;

        buf.stImageInfo.nWidth  = geometry.nWidth;
        buf.stImageInfo.nHeight = geometry.nHeight;
        buf.stImageInfo.nStride = geometry.nStride;
        getColorFormat((int &)buf.stImageInfo.nFormat);
        buf.stImageInfo.stCropInfo.nTop     = geometry.cropRect.nTop;
        buf.stImageInfo.stCropInfo.nLeft    = geometry.cropRect.nLeft;
        buf.stImageInfo.stCropInfo.nWidth   = geometry.cropRect.nWidth;
        buf.stImageInfo.stCropInfo.nHeight  = geometry.cropRect.nHeight;
    }

    /* HDR */
    if (mCodecImpl->mHDRInfo.eValidInfo != VIDEO_INFO_TYPE_INVALID) {
        memcpy(&(buf.stImageInfo.stHDRInfo), &(mCodecImpl->mHDRInfo), sizeof(buf.stImageInfo.stHDRInfo));
    } else {
        buf.stImageInfo.stHDRInfo.eValidInfo = VIDEO_INFO_TYPE_INVALID;
    }

    buf.stImageInfo.stHDRInfo.eChangedInfo = VIDEO_INFO_TYPE_INVALID;
    if (frameType & VIDEO_FRAME_WITH_HDR_INFO) {
        updateHDRInfo(buf.stImageInfo.stHDRInfo);
    }

    if (frameType & VIDEO_FRAME_WITH_BLACK_BAR) {
        updateBlackBarInfoToParam(buf.params);
    }

    if (frameType & VIDEO_FRAME_WITH_FILM_GRAIN) {
        updateFilmGrainInfoToParam(buf.params);
    }

    updateExtraInfo(buf.obj->metadata(), buffer);

    /* set dataspace */
    if (mCodecImpl->mDataSpace != HAL_DATASPACE_UNKNOWN) {
        /* upadte */
        vendor::graphics::ExynosGraphicBufferMeta::set_dataspace(buf.obj->handle(), (android_dataspace_t)mCodecImpl->mDataSpace);
        buf.stImageInfo.nDataSpace = mCodecImpl->mDataSpace;

        ExynosLogD("[%s] dataspace(0x%x)", __FUNCTION__, mCodecImpl->mDataSpace);
    }

#ifdef USE_DPB_MANAGER
    auto refDPBManager = std::static_pointer_cast<RefDPBManager>(mExynosPort[ExynosPort::Output].mBufManager);
    refDPBManager->delRefDPB(buffer.extraInfo.specific.dec.refDPB);
#endif

    if (displayStatus == VIDEO_FRAME_STATUS_DECODING_ONLY) {  /* TODO */
        buf.eDataInfo = DataInfo::NoData;
        ExynosLogD("[%s] input data doesn't have output data for display. display status is decoding only", __FUNCTION__);
    }
}

ExynosErrorType ExynosVideoCodecDec::portStop(ExynosPort::Port port) {
    auto ret = ExynosVideoCodecCommon::portStop(port);
    if ((port == ExynosPort::Output) &&
        (ret == EXYNOS_ERROR_NONE)) {
        mCodecImpl->mRefreshHDRInfo = true;
    }

    return ret;
}

ExynosErrorType ExynosVideoCodecDec::init() {
    ExynosLogFunctionTrace();

    ExynosVideoDecOps decOps;
    auto err = Exynos_Video_Register_Decoder(&decOps, &(mCodecImpl->mInBufOps), &(mCodecImpl->mOutBufOps));
    if (err != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] Exynos_Video_Register_Decoder() is failed", __FUNCTION__);
        return EXYNOS_ERROR_UNKNOWN;
    }
    mCodecImpl->mCommonOps = decOps;

    mCodecImpl->mHandle = std::get<ExynosVideoDecOps>(mCodecImpl->mCommonOps).Init(&(mCodecImpl->mVideoInstInfo));
    if (mCodecImpl->mHandle == nullptr) {
        ExynosLogE("[%s] Init() is failed", __FUNCTION__);
        return EXYNOS_ERROR_RESOURCE;
    }

    return EXYNOS_ERROR_NONE;
}

void ExynosVideoCodecDec::deinit() {
    ExynosLogFunctionTrace();

    if (mCodecImpl->mVideoInstInfo.eCodecType == VIDEO_CODING_UNKNOWN) {
        return;
    }

    auto handle = mCodecImpl->mHandle;

    if (handle != nullptr) {
        streamOnOff(ExynosPort::Input, ExynosPort::Off);
        streamOnOff(ExynosPort::Output, ExynosPort::Off);

        if (std::get<ExynosVideoDecOps>(mCodecImpl->mCommonOps).Finalize(handle) != VIDEO_ERROR_NONE) {
            ExynosLogE("[%s] Finalize() is failed", __FUNCTION__);
        }

        mCodecImpl->mHandle = nullptr;
    }
}

ExynosErrorType ExynosVideoCodecDec::srcSetup(ExynosBufferInfo input) {
    ExynosLogFunctionTrace();

    ExynosVideoBufferOps &inBufOps  = mCodecImpl->mInBufOps;
    ExynosVideoBufferOps &outBufOps = mCodecImpl->mOutBufOps;

    auto handle = mCodecImpl->mHandle;

    if (handle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return EXYNOS_ERROR_BAD_STATE;
    }

    streamOnOff(ExynosPort::Input, ExynosPort::Off);

    if (mExynosPort[ExynosPort::Input].mIsConfigured) {
        inBufOps.Cleanup_Buffer(handle);
        mExynosPort[ExynosPort::Input].mIsConfigured = false;
    }

    if (inBufOps.Enable_Cacheable(handle) != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] inbuf : Enable_Cacheable() is failed", __FUNCTION__);
        return EXYNOS_ERROR_UNKNOWN;
    }

    /* input buffer geometry */
    {
        ExynosVideoGeometry geometry;
        memset(&geometry, 0, sizeof(geometry));

        geometry.nSizeImage         = 1;
        geometry.nPlaneCnt          = 1;  /* it must be 1 */

        if ((mCodecImpl->mVideoInstInfo.eCodecType == VIDEO_CODING_VC1) ||
            (mCodecImpl->mVideoInstInfo.eCodecType == VIDEO_CODING_VC1_RCV)) {
            auto buffer = input.obj;
            BufferAddressInfo addrInfo;
            if (!buffer->map(addrInfo)) {  /* need the address of buffer for header parsing */
                ExynosLogD("[%s] buffer->map() is failed", __FUNCTION__);
                return EXYNOS_ERROR_UNKNOWN;
            }

            mCodecImpl->mVideoInstInfo.eCodecType = getVc1FormatType(addrInfo.plane[0]);

            buffer->unmap();
        }

        geometry.eCompressionFormat = mCodecImpl->mVideoInstInfo.eCodecType;

        if (inBufOps.Set_Geometry(handle, &geometry) != VIDEO_ERROR_NONE) {
            ExynosLogE("[%s] inbuf : Set_Geometry() is failed", __FUNCTION__);
            return EXYNOS_ERROR_UNKNOWN;
        }
    }

    if (inBufOps.Setup(handle, 0/* DYNAMIC */) != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] inbuf : Setup() is failed", __FUNCTION__);
        return EXYNOS_ERROR_UNKNOWN;
    }

    mCodecImpl->mOutGeometry.eColorFormat = VIDEO_COLORFORMAT_NV12M;
    mCodecImpl->mOutGeometry.nPlaneCnt    = getPlaneCnt(mCodecImpl->mOutGeometry.eColorFormat);
    mCodecImpl->mOutGeometry.eFilledDataType = (ExynosFilledDataType)getDataType(mCodecImpl->mOutGeometry.eColorFormat);

    /* apply configurations */
    applyConfig(input.params);

    /* output buffer geometry */
    {
        ExynosVideoGeometry geometry;
        memset(&geometry, 0, sizeof(geometry));

        geometry.eColorFormat    = mCodecImpl->mOutGeometry.eColorFormat;
        geometry.nPlaneCnt       = mCodecImpl->mOutGeometry.nPlaneCnt;
        geometry.eFilledDataType = mCodecImpl->mOutGeometry.eFilledDataType;

        if (mCodecImpl->mHDRInfo.eValidInfo & VIDEO_INFO_TYPE_COLOR_ASPECTS) {
            ExynosColorAspects      &fwkCA = mCodecImpl->mHDRInfo.sColorAspects;
            ExynosVideoColorAspects &bitCA = mCodecImpl->mOutGeometry.HdrInfo.sColorAspects;

            bitCA.eRangeType       = (ExynosRangeType)fwkCA.mRange;
            bitCA.ePrimariesType   = (ExynosPrimariesType)fwkCA.mPrimaries;
            bitCA.eTransferType    = (ExynosTransferType)fwkCA.mTransfer;
            bitCA.eCoeffType       = (ExynosMatrixCoeffType)fwkCA.mMatrixCoeffs;
        }

        memcpy(&(geometry.HdrInfo), &(mCodecImpl->mOutGeometry.HdrInfo), sizeof(ExynosVideoHdrInfo));

        if (outBufOps.Set_Geometry(handle, &geometry) != VIDEO_ERROR_NONE) {
            ExynosLogE("[%s] outbuf : Set_Geometry() is failed", __FUNCTION__);
            return EXYNOS_ERROR_UNKNOWN;
        }
    }

    mExynosPort[ExynosPort::Input].mIsConfigured = true;

    return headerParsing(input);
}

ExynosErrorType ExynosVideoCodecDec::dstSetup() {
    ExynosLogFunctionTrace();

    ExynosVideoBufferOps &outBufOps = mCodecImpl->mOutBufOps;

    auto handle = mCodecImpl->mHandle;

    if (handle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return EXYNOS_ERROR_BAD_STATE;
    }

    streamOnOff(ExynosPort::Output, ExynosPort::Off);

    if (mExynosPort[ExynosPort::Output].mIsConfigured) {
        outBufOps.Cleanup_Buffer(handle);
        mExynosPort[ExynosPort::Output].mIsConfigured = false;
    }

    if (outBufOps.Setup(handle, 0/* DYNAMIC */) != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] outbuf : Setup() is failed", __FUNCTION__);
        return EXYNOS_ERROR_UNKNOWN;
    }

    if ((mCodecImpl->mOutGeometry.nWidth * mCodecImpl->mOutGeometry.nHeight) > UHD_IMAGE_SIZE) {
        ExynosVideoDecOps &ops = std::get<ExynosVideoDecOps>(mCodecImpl->mCommonOps);

        if (ops.Disable_LazyUnmap(handle) != VIDEO_ERROR_NONE) {
            ExynosLogE("[%s] Disable_LazyUnmap() is failed", __FUNCTION__);
            return EXYNOS_ERROR_UNKNOWN;
        }

        ExynosLogD("[%s] lazy unmap is disabled", __FUNCTION__);
    }

    mExynosPort[ExynosPort::Output].mIsConfigured = true;

    return EXYNOS_ERROR_NONE;
}

ExynosErrorType ExynosVideoCodecDec::updateResolution(bool bInterResolution) {
    ExynosLogFunctionTrace();

    ExynosVideoDecOps       &ops        = std::get<ExynosVideoDecOps>(mCodecImpl->mCommonOps);
    ExynosVideoBufferOps    &outBufOps  = mCodecImpl->mOutBufOps;

    auto handle = mCodecImpl->mHandle;

    if (handle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return EXYNOS_ERROR_BAD_STATE;
    }

    if (!mExynosPort[ExynosPort::Input].mIsConfigured) {
        ExynosLogE("[%s] src is not configured", __FUNCTION__);
        return EXYNOS_ERROR_BAD_STATE;
    }

    /* get output buffer geometry : bocking function : sync with header parsing finished */
    {
        ExynosVideoGeometry geometry;
        memset(&geometry, 0, sizeof(geometry));

        auto err = outBufOps.Get_Geometry(handle, &geometry);

        if (err == VIDEO_ERROR_HEADERINFO) {
            ExynosLogD("[%s] header parsing is not completed yet. re-try it with next buffer", __FUNCTION__);
            return EXYNOS_ERROR_TRY_AGAIN;
        }

        if (err == VIDEO_ERROR_NOSUPPORT) {
            ExynosLogE("[%s] feature is not supported", __FUNCTION__);
            return EXYNOS_ERROR_NOT_SUPPORT;
        }

        if (err != VIDEO_ERROR_NONE) {
            ExynosLogE("[%s] outbuf : Get_Geometry() is failed", __FUNCTION__);
            return EXYNOS_ERROR_UNKNOWN;
        }

        ExynosLogI("[%s] %s resolution info : %d x %d (%d, %d, %d, %d) %d", __FUNCTION__, (bInterResolution == true)?"inter":"nomal",
                    geometry.nWidth, geometry.nHeight,
                    geometry.cropRect.nTop, geometry.cropRect.nLeft, geometry.cropRect.nWidth, geometry.cropRect.nHeight,
                    geometry.nStride);

        if (bInterResolution == true) {
            /* just update crop information
             * in case of inter resolution changed,
             * video frame will be filled to DPB in keeping stride value of DPB.
             * the area about video frame is expressed as crop information.
             */
            mCodecImpl->mOutGeometry.cropRect.nTop     = geometry.cropRect.nTop;
            mCodecImpl->mOutGeometry.cropRect.nLeft    = geometry.cropRect.nLeft;
            mCodecImpl->mOutGeometry.cropRect.nWidth   = geometry.cropRect.nWidth;
            mCodecImpl->mOutGeometry.cropRect.nHeight  = geometry.cropRect.nHeight;

            return EXYNOS_ERROR_NONE;
        }

        /* color format and eFilledDataType could be changed by MFC due to various reason like as 10bit */
        if (mCodecImpl->mOutGeometry.eColorFormat != geometry.eColorFormat) {
            ExynosLogI("[%s] color format is changed to 0x%x from 0x%x", __FUNCTION__,
                        geometry.eColorFormat, mCodecImpl->mOutGeometry.eColorFormat);
        }

        if (mCodecImpl->mOutGeometry.eFilledDataType != geometry.eFilledDataType) {
            ExynosLogI("[%s] filled data type is changed to 0x%x from 0x%x", __FUNCTION__,
                        geometry.eFilledDataType, mCodecImpl->mOutGeometry.eFilledDataType);
        }

        memcpy(&(mCodecImpl->mOutGeometry), &geometry, sizeof(geometry));
    }

    /* get number of DPB required for decoding */
    {
    mCodecImpl->mNumDPB = ops.Get_ActualBufferCount(handle);

    mCodecImpl->mNumDispDelay = ops.Get_DisplayDelay(handle);
    if (mCodecImpl->mNumDispDelay < 0) {
        mCodecImpl->mNumDispDelay = mCodecImpl->mNumDPB;
        ExynosLogD("[%s] display delay info is available. it will be set as number of DPB(%d)", __FUNCTION__, mCodecImpl->mNumDispDelay);
    } else {
        ExynosLogD("[%s] display delay info : %d", __FUNCTION__, mCodecImpl->mNumDispDelay);
    }

#ifdef EXTRA_DPB_NUM
    if (!mCodecImpl->mIsThumbnail) {
        mCodecImpl->mNumDPB += EXTRA_DPB_NUM;
    }
#endif
    }
    /* TODO : interlace */

    return EXYNOS_ERROR_NONE;
}

ExynosErrorType ExynosVideoCodecDec::getResolution(
    uint32_t &width,
    uint32_t &height,
    CropInfo &crop) {
    ExynosLogFunctionTrace();

    if (!mCodecImpl->mIsHeaderDecoded) {
        ExynosLogE("[%s] header is not decoded yet", __FUNCTION__);
        return EXYNOS_ERROR_BAD_STATE;
    }

    width   = mCodecImpl->mOutGeometry.nWidth;
    height  = mCodecImpl->mOutGeometry.nHeight;

    crop.nTop       = mCodecImpl->mOutGeometry.cropRect.nTop;
    crop.nLeft      = mCodecImpl->mOutGeometry.cropRect.nLeft;
    crop.nWidth     = mCodecImpl->mOutGeometry.cropRect.nWidth;
    crop.nHeight    = mCodecImpl->mOutGeometry.cropRect.nHeight;

    return EXYNOS_ERROR_NONE;
}

ExynosErrorType ExynosVideoCodecDec::getDPBCount(int &count) {
    ExynosLogFunctionTrace();

    if (!mCodecImpl->mIsHeaderDecoded) {
        count = 0;
        ExynosLogE("[%s] header is not decoded yet", __FUNCTION__);
        return EXYNOS_ERROR_BAD_STATE;
    }

    count = mCodecImpl->mNumDPB;

    ExynosLogD("[%s] number of DPB : %d", __FUNCTION__, count);

    return EXYNOS_ERROR_NONE;
}

ExynosErrorType ExynosVideoCodecDec::getColorFormat(int &format) {
    ExynosLogFunctionTrace();

    if (!mCodecImpl->mIsHeaderDecoded) {
        format = VIDEO_COLORFORMAT_UNKNOWN;
        ExynosLogE("[%s] header is not decoded yet", __FUNCTION__);
        return EXYNOS_ERROR_BAD_STATE;
    }

    /* translate video format to pixel format */
    format = getPixelFormat(mCodecImpl->mOutGeometry.eColorFormat);
    if (format != 0/* HAL_PIXEL_FORMAT_UNDEFINED */) {
        ExynosLogV("[%s] color format : 0x%X", __FUNCTION__, format);
        return EXYNOS_ERROR_NONE;
    }

    ExynosLogD("[%s] can't find color format(0x%X)", __FUNCTION__, mCodecImpl->mOutGeometry.eColorFormat);

    return EXYNOS_ERROR_UNKNOWN;
}

ExynosErrorType ExynosVideoCodecDec::getDisplayDelay(int &count) {
    ExynosLogFunctionTrace();

    if (!mCodecImpl->mIsHeaderDecoded) {
        count = 0;
        ExynosLogE("[%s] header is not decoded yet", __FUNCTION__);
        return EXYNOS_ERROR_BAD_STATE;
    }

    count = mCodecImpl->mNumDispDelay;

    ExynosLogD("[%s] display delay : %d", __FUNCTION__, count);

    return EXYNOS_ERROR_NONE;
}

ExynosErrorType ExynosVideoCodecDec::getExtraBufNum(int &count) {
    ExynosLogFunctionTrace();

    count = 0;

#ifdef EXTRA_DPB_NUM
    if (!mCodecImpl->mIsThumbnail) {
        count = EXTRA_DPB_NUM;
    }
#endif

    return EXYNOS_ERROR_NONE;
}

void ExynosVideoCodecDec::applyConfig_PixelFormat(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* pixel format */
    auto baseParam = params.getParam(ExynosParamIndex::PixelFormatIndex);
    if (baseParam.get() == nullptr) {
        return;
    }

    auto param = std::static_pointer_cast<ExynosParam<ParamPixelFormat>>(baseParam);

    uint32_t format = (uint32_t)DEFAULT_PIXEL_FORMAT;

    switch (param->m.format) {
    case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_YCBCR_420_888:
        format = (uint32_t)DEFAULT_PIXEL_FORMAT;
        break;
    case HAL_PIXEL_FORMAT_YV12:
        format = HAL_PIXEL_FORMAT_EXYNOS_YV12_M;
        break;
#ifndef USE_FLEXIBLE_P010
    case HAL_PIXEL_FORMAT_YCBCR_P010:
        format = HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M;
        break;
#endif
    default:
        format = param->m.format;
        break;
    }

    ExynosVideoColorFormatType videoFormat = (ExynosVideoColorFormatType)getVideoFormat(format);

    if (mCodecImpl->checkSupportedFormat(videoFormat) != VIDEO_TRUE) {
        ExynosLogD("[%s] video format(%d) is not supported. uses default format(%d)", __FUNCTION__,
                    videoFormat, VIDEO_COLORFORMAT_NV12M);
        videoFormat = VIDEO_COLORFORMAT_NV12M;
    }

    mCodecImpl->mOutGeometry.eColorFormat    = videoFormat;
    mCodecImpl->mOutGeometry.nPlaneCnt       = getPlaneCnt(mCodecImpl->mOutGeometry.eColorFormat);
    mCodecImpl->mOutGeometry.eFilledDataType = (ExynosFilledDataType)getDataType(mCodecImpl->mOutGeometry.eColorFormat);
}

void ExynosVideoCodecDec::applyConfig_CompressedColor(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* compressed color */
    auto baseParam = params.getParam(ExynosParamIndex::CompressedColorIndex);
    if (baseParam.get() == nullptr) {
        return;
    }

    auto param = std::static_pointer_cast<ExynosParam<ParamCompressedColor>>(baseParam);
#ifdef USE_DEC_SW_CSC
    if (mCodecImpl->mOutGeometry.eColorFormat == VIDEO_COLORFORMAT_YV12M) {
        /* ignore request */
        ExynosLogD("[%s] does not use compressed color because only SW CSC is valid", __FUNCTION__);
        return;
    }
#endif

    if (param->m.format != 0) {
        auto videoFormat = (ExynosVideoColorFormatType)getVideoFormat(param->m.format);

        if (mCodecImpl->checkSupportedFormat(videoFormat) == VIDEO_TRUE) {
            mCodecImpl->mOutGeometry.eColorFormat    = videoFormat;
            mCodecImpl->mOutGeometry.nPlaneCnt       = getPlaneCnt(mCodecImpl->mOutGeometry.eColorFormat);
            mCodecImpl->mOutGeometry.eFilledDataType = (ExynosFilledDataType)getDataType(mCodecImpl->mOutGeometry.eColorFormat);
            mCodecImpl->mIsCompressedColor = true;
        }
    }
}

void ExynosVideoCodecDec::applyConfig_DecodeOrderDecoding(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* decode order decoding */
    auto baseParam = params.getParam(ExynosParamIndex::DecodeOrderIndex);
    if (baseParam.get() == nullptr) {
        return;
    }
    auto param = std::static_pointer_cast<ExynosParam<ParamDecodeOrder>>(baseParam);

    if (param->m.enable == On) {
        mCodecImpl->enableDecodeOrder();
    }
}

void ExynosVideoCodecDec::applyConfig_DisplayDelay(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* display delay */
    auto baseParam = params.getParam(ExynosParamIndex::DisplayDelayIndex);
    if (baseParam.get() == nullptr) {
        return;
    }

    auto param = std::static_pointer_cast<ExynosParam<ParamDisplayDelay>>(baseParam);

    mCodecImpl->setDisplayDelay(param->m.delay);
}

void ExynosVideoCodecDec::applyConfig_ThumbnailMode(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* thumbnail mode */
    auto baseParam = params.getParam(ExynosParamIndex::ThumbnailModeIndex);
    if (baseParam.get() == nullptr) {
        return;
    }

    auto param = std::static_pointer_cast<ExynosParam<ParamThumbnailMode>>(baseParam);

    if (param->m.enable != On) {
        return;
    }

    auto err = mCodecImpl->enableThumbnailMode();

    if (err == VIDEO_ERROR_NONE) {
        ExynosLogD("[%s] thumbnail mode is enabled", __FUNCTION__);
    } else {
        ExynosLogE("[%s] Set_IFrameDecoding() is failed", __FUNCTION__);
    }
}

void ExynosVideoCodecDec::applyConfig_Skype(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* skype */
    auto baseParam = params.getParam(ExynosParamIndex::ThumbnailModeIndex);
    if (baseParam.get() == nullptr) {
        return;
    }

    /* low latency */
    auto param = std::static_pointer_cast<ExynosParam<ParamSkypeLowLatency>>(baseParam);

    if (param->m.enable == On) {
        auto err = mCodecImpl->setDisplayDelay(0);

        if (err == VIDEO_ERROR_NONE) {
            ExynosLogD("[%s] low latency is enabled", __FUNCTION__);
        } else {
            ExynosLogE("[%s] Set_DisplayDelay(0) is failed", __FUNCTION__);
        }
    }
}

void ExynosVideoCodecDec::applyConfig_BlackBar(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* black bar search */
    auto baseParam = params.getParam(ExynosParamIndex::BlackBarIndex);
    if (baseParam.get() == nullptr) {
        return;
    }

    auto param = std::static_pointer_cast<ExynosParam<ParamBlackBar>>(baseParam);

    auto enable = (param->m.enable == On)? true:false;

    auto err = mCodecImpl->setSearchBlackBar(enable);

    if (err == VIDEO_ERROR_NONE) {
        ExynosLogD("[%s] black bar is %s", __FUNCTION__, (enable == true)? "enabled":"disabled");
    } else {
        ExynosLogE("[%s] Set_SearchBlackBar(%s) is failed", __FUNCTION__, (enable == true)? "enabled":"disabled");
    }
}

void ExynosVideoCodecDec::applyConfig_ColorAspects(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* color aspects */
    auto baseParam = params.getParam(ExynosParamIndex::ColorAspectsIndex);
    if (baseParam.get() == nullptr) {
        return;
    }

    auto param = std::static_pointer_cast<ExynosParam<ParamColorAspects>>(baseParam);

    ExynosColorAspects &CA = mCodecImpl->mHDRInfo.sColorAspects;

    CA.mRange           = param->m.range;
    CA.mPrimaries       = param->m.primaries;
    CA.mTransfer        = param->m.transfer;
    CA.mMatrixCoeffs    = param->m.matrix;

    mCodecImpl->mHDRInfo.eValidInfo |= VIDEO_INFO_TYPE_COLOR_ASPECTS;
    mCodecImpl->mDataSpace = ExynosUtils::GetDataspaceFromColorAspects(CA.mRange, CA.mPrimaries, CA.mTransfer, CA.mMatrixCoeffs);
}

void ExynosVideoCodecDec::applyConfig_HdrStatic(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* hdr static */
    auto baseParam = params.getParam(ExynosParamIndex::HDRStaticInfoIndex);
    if (baseParam.get() == nullptr) {
        return;
    }

    auto param = std::static_pointer_cast<ExynosParam<ParamHdrStaticInfo>>(baseParam);

    memcpy(&(mCodecImpl->mHDRInfo.sHdrStaticInfo), &(param->m.ST), sizeof(ExynosHdrStaticInfo));

    mCodecImpl->mHDRInfo.eValidInfo |= VIDEO_INFO_TYPE_HDR_STATIC;
}

void ExynosVideoCodecDec::applyConfig_OperatingRate(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* operating rate */
    auto baseParam = params.getParam(ExynosParamIndex::OperatingRateIndex);
    if (baseParam.get() == nullptr) {
        return;
    }

    auto param = std::static_pointer_cast<ExynosParam<ParamOperatingRate>>(baseParam);

    auto err = mCodecImpl->setOperatingRate(param->m.value);

    if (err == VIDEO_ERROR_NONE) {
        ExynosLogD("[%s] operating rate is %d", __FUNCTION__, param->m.value);
    } else {
        ExynosLogE("[%s] Set_OperatingRate(%d) is failed", __FUNCTION__, param->m.value);
    }
}

void ExynosVideoCodecDec::applyConfig_RealTimePriority(ExynosParams &params) {
    ExynosLogFunctionTrace();

    /* realtime priority */
    auto baseParam = params.getParam(ExynosParamIndex::RealTimePriorityIndex);
    if (baseParam.get() == nullptr) {
        return;
    }

    auto param = std::static_pointer_cast<ExynosParam<ParamRealTimePriority>>(baseParam);

    auto err = mCodecImpl->setRealTimePriority(param->m.value);

    if (err == VIDEO_ERROR_NONE) {
        ExynosLogD("[%s] RealTime priority is %d", __FUNCTION__, param->m.value);
    } else {
        ExynosLogE("[%s] Set_RealTimePriority(%d) is failed", __FUNCTION__, param->m.value);
    }
}

void ExynosVideoCodecDec::applyConfig(ExynosParams &params) {
    ExynosLogFunctionTrace();

    if (params.empty()) {
        /* there is nothing to change */
        return;
    }

    auto handle = mCodecImpl->mHandle;

    if (handle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return;
    }

    if (!mExynosPort[ExynosPort::Input].mIsConfigured) {  /* it is not allowed after header parsing */
        /* pixel format */
        applyConfig_PixelFormat(params);

        /* compressed color */
        applyConfig_CompressedColor(params);

        /* decode order decoding */
        applyConfig_DecodeOrderDecoding(params);

        /* display delay */
        applyConfig_DisplayDelay(params);

        /* thumbnail mode */
        applyConfig_ThumbnailMode(params);

        /* Skype */
        applyConfig_Skype(params);
    }

    /* black bar search */
    applyConfig_BlackBar(params);

    /* color aspects */
    applyConfig_ColorAspects(params);

    /* hdr static */
    applyConfig_HdrStatic(params);

    /* operating rate */
    applyConfig_OperatingRate(params);

    /* realtime priority */
    applyConfig_RealTimePriority(params);
}

ExynosErrorType ExynosVideoCodecDec::headerParsing(ExynosBufferInfo input) {
    ExynosLogFunctionTrace();

    auto handle = mCodecImpl->mHandle;

    if (handle == nullptr) {
        ExynosLogE("[%s] handle is null", __FUNCTION__);
        return EXYNOS_ERROR_BAD_STATE;
    }

    if (!mExynosPort[ExynosPort::Input].mIsConfigured) {
        ExynosLogE("[%s] src is not configured", __FUNCTION__);
        return EXYNOS_ERROR_BAD_STATE;
    }

    auto ret = srcEnqueue(input);
    if (ret != EXYNOS_ERROR_NONE) {
        ExynosLogE("[%s] srcEnqueue() is failed", __FUNCTION__);
        return ret;
    }

    ret = updateResolution();

    if (ret == EXYNOS_ERROR_NONE) {
        mCodecImpl->mIsHeaderDecoded = true;
    }

    mExynosPort[ExynosPort::Input].mBufManager->eraseSharedPtr(input.obj);

    /* input stream off
     * re-input scheme. input will be queued by inputEnqueue() */
    streamOnOff(ExynosPort::Input, ExynosPort::Off);

    return ret;
}

void ExynosVideoCodecDec::updateHDRInfo(HDRInfo &info) {
    ExynosLogFunctionTrace();

    ExynosVideoDecOps   &ops        = std::get<ExynosVideoDecOps>(mCodecImpl->mCommonOps);
    ExynosVideoHdrInfo  &hdrInfo    = mCodecImpl->mOutGeometry.HdrInfo;

    auto handle = mCodecImpl->mHandle;

    if (ops.Get_HDRInfo(handle, &hdrInfo) != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] Get_HDRInfo() is failed", __FUNCTION__);
        return;
    }

    /* color aspects */
    if (hdrInfo.eValidType & (HDR_INFO_COLOR_ASPECTS | HDR_INFO_RANGE)) {
        ExynosColorAspects &CA = info.sColorAspects;

        /* HDR_INFO_RANGE */
        CA.mRange           = hdrInfo.sColorAspects.eRangeType;

        /* HDR_INFO_COLOR_ASPECTS */
        CA.mPrimaries       = hdrInfo.sColorAspects.ePrimariesType;
        CA.mTransfer        = hdrInfo.sColorAspects.eTransferType;
        CA.mMatrixCoeffs    = hdrInfo.sColorAspects.eCoeffType;

        /* in case of VP9, trusts and uses info given from frameworks */
        if (mCodecImpl->mVideoInstInfo.eCodecType != VIDEO_CODING_VP9) {
            if ((hdrInfo.eChangedType & (HDR_INFO_COLOR_ASPECTS | HDR_INFO_RANGE)) ||
                (mCodecImpl->mRefreshHDRInfo)) {
                ExynosLogD("[%s] change ColorAspects(ISO:: r:%d, p:0x%x, t:0x%x, m:0x%x)", __FUNCTION__,
                            CA.mRange, CA.mPrimaries, CA.mTransfer, CA.mMatrixCoeffs);
                info.eChangedInfo |= VIDEO_INFO_TYPE_COLOR_ASPECTS;
            }

            mCodecImpl->mDataSpace = ExynosUtils::GetDataspaceFromColorAspects(
                                        mCodecImpl->mOutGeometry.nWidth, mCodecImpl->mOutGeometry.nHeight, CA);
        }

        ExynosLogV("[%s] has ColorAspects(ISO:: r:%d, p:0x%x, t:0x%x, m:0x%x)", __FUNCTION__,
                    CA.mRange, CA.mPrimaries, CA.mTransfer, CA.mMatrixCoeffs);
        info.eValidInfo |= VIDEO_INFO_TYPE_COLOR_ASPECTS;
    }

    /* hdr static info */
    if (hdrInfo.eValidType & (HDR_INFO_LIGHT | HDR_INFO_LUMINANCE)) {
        ExynosType1 &STATIC = info.sHdrStaticInfo.sType1;

        /* HDR_INFO_LIGHT */
        STATIC.mMaxFrameAverageLightLevel   = hdrInfo.sHdrStatic.max_pic_average_light;
        STATIC.mMaxContentLightLevel        = hdrInfo.sHdrStatic.max_content_light;

        /* HDR_INFO_LUMINANCE */
        if (mCodecImpl->mVideoInstInfo.eCodecType == VIDEO_CODING_AV1) {
            const float fp_24_8_for_SMPTE2086    = (1 / 256.f);
            const float fp_18_14_for_SMPTE2086   = (10000 / 16384.f);
            const float fp_0_16_for_SMPTE2086    = (50000 / 65536.f);

            STATIC.mMaxDisplayLuminance = (unsigned int)(hdrInfo.sHdrStatic.max_display_luminance * fp_24_8_for_SMPTE2086 + 0.5);
            STATIC.mMinDisplayLuminance = (unsigned int)(hdrInfo.sHdrStatic.min_display_luminance * fp_18_14_for_SMPTE2086 + 0.5);

            STATIC.mR.x = (unsigned int)(hdrInfo.sHdrStatic.red.x * fp_0_16_for_SMPTE2086 + 0.5);
            STATIC.mR.y = (unsigned int)(hdrInfo.sHdrStatic.red.y * fp_0_16_for_SMPTE2086 + 0.5);
            STATIC.mG.x = (unsigned int)(hdrInfo.sHdrStatic.green.x * fp_0_16_for_SMPTE2086 + 0.5);
            STATIC.mG.y = (unsigned int)(hdrInfo.sHdrStatic.green.y * fp_0_16_for_SMPTE2086 + 0.5);
            STATIC.mB.x = (unsigned int)(hdrInfo.sHdrStatic.blue.x * fp_0_16_for_SMPTE2086 + 0.5);
            STATIC.mB.y = (unsigned int)(hdrInfo.sHdrStatic.blue.y * fp_0_16_for_SMPTE2086 + 0.5);
            STATIC.mW.x = (unsigned int)(hdrInfo.sHdrStatic.white.x * fp_0_16_for_SMPTE2086 + 0.5);
            STATIC.mW.y = (unsigned int)(hdrInfo.sHdrStatic.white.y * fp_0_16_for_SMPTE2086 + 0.5);
        } else {
            STATIC.mMaxDisplayLuminance = hdrInfo.sHdrStatic.max_display_luminance * 0.0001f;  /* convert 0.0001cd/m^2 to cd/m^2 */
            STATIC.mMinDisplayLuminance = hdrInfo.sHdrStatic.min_display_luminance;

            STATIC.mR.x = hdrInfo.sHdrStatic.red.x;
            STATIC.mR.y = hdrInfo.sHdrStatic.red.y;
            STATIC.mG.x = hdrInfo.sHdrStatic.green.x;
            STATIC.mG.y = hdrInfo.sHdrStatic.green.y;
            STATIC.mB.x = hdrInfo.sHdrStatic.blue.x;
            STATIC.mB.y = hdrInfo.sHdrStatic.blue.y;
            STATIC.mW.x = hdrInfo.sHdrStatic.white.x;
            STATIC.mW.y = hdrInfo.sHdrStatic.white.y;
        }

        if ((hdrInfo.eChangedType & (HDR_INFO_LIGHT | HDR_INFO_LUMINANCE)) ||
            (mCodecImpl->mRefreshHDRInfo)) {
            ExynosLogD("[%s] change StaticInfo", __FUNCTION__);
            info.eChangedInfo |= VIDEO_INFO_TYPE_HDR_STATIC;
        }

        ExynosLogV("[%s] has StaticInfo", __FUNCTION__);
        info.eValidInfo |= VIDEO_INFO_TYPE_HDR_STATIC;
    }

    /* HDR dynamic info(struct) :
     * If this hdr dynamic struct info is updated,
     * HDR dynamic info(blob) will not be updated.
     */
    if (hdrInfo.sHdrDynamic.valid != 0) {
        ExynosHdrDynamicInfo &DYNAMIC = info.sHdrDynamicInfo;

        updateHdrDynamicInfo(DYNAMIC, hdrInfo);

        if (hdrInfo.eValidType & HDR_INFO_DYNAMIC_META) {
            ExynosLogV("[%s] has DynamicInfo", __FUNCTION__);
        }

        if (DYNAMIC.valid != 0) {
            ExynosLogI("[%s] change DynamicInfo", __FUNCTION__);
        }

        info.eChangedInfo |= VIDEO_INFO_TYPE_HDR_DYNAMIC;
        info.eValidInfo |= VIDEO_INFO_TYPE_HDR_DYNAMIC;
    }

    if (hdrInfo.sHdrDynamicBlob.nSize > 0) {
        info.sHdrDynamicInfo.valid = 0;  /* clear */

        ExynosLogV("[%s] has DynamicInfo", __FUNCTION__);

        ExynosHdrDynamicBlob *DYNAMIC_BLOB = &info.sHdrDynamicBlob;

        DYNAMIC_BLOB->nSize = (hdrInfo.eChangedType & HDR_INFO_DYNAMIC_META)? hdrInfo.sHdrDynamicBlob.nSize:0;

        if (DYNAMIC_BLOB->nSize > 0) {
            memcpy((char *)DYNAMIC_BLOB->pData, (char *)hdrInfo.sHdrDynamicBlob.pData, DYNAMIC_BLOB->nSize);

            ExynosLogI("[%s] change DynamicInfo", __FUNCTION__);

            info.eChangedInfo |= VIDEO_INFO_TYPE_HDR_DYNAMIC;
            info.eValidInfo |= VIDEO_INFO_TYPE_HDR_DYNAMIC;
        }
    }

    mCodecImpl->mRefreshHDRInfo = false;

    return;
}

void ExynosVideoCodecDec::updateBlackBarInfoToParam(ExynosParams &params) {
    ExynosLogFunctionTrace();

    ExynosVideoDecOps &ops = std::get<ExynosVideoDecOps>(mCodecImpl->mCommonOps);
    ExynosVideoRect    info;

    auto handle = mCodecImpl->mHandle;

    if (ops.Get_BlackBarCrop(handle, &info) != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] Get_BlackBarCrop() is failed", __FUNCTION__);
        return;
    }

    {
        auto param = std::make_shared<ExynosParam<ParamBlackBarInfo>>();

        param->m.rect.nLeft     = info.nLeft;
        param->m.rect.nTop      = info.nTop;
        param->m.rect.nWidth    = info.nWidth;
        param->m.rect.nHeight   = info.nHeight;

        params.addParam(param);
    }

    ExynosLogV("[%s] black bar info is (t:%d, l:%d, w:%d, h:%d)", __FUNCTION__,
                info.nTop, info.nLeft, info.nWidth, info.nHeight);

    return;
}

void ExynosVideoCodecDec::updateFilmGrainInfoToParam(ExynosParams &params) {
    ExynosLogFunctionTrace();

    ExynosVideoDecOps &ops = std::get<ExynosVideoDecOps>(mCodecImpl->mCommonOps);

    ExynosVideoFilmGrain info;
    memset(&info, 0, sizeof(info));

    auto handle = mCodecImpl->mHandle;

    if (ops.Get_FilmGrainInfo(handle, &info) != VIDEO_ERROR_NONE) {
        ExynosLogE("[%s] Get_FilmGrainInfo() is failed", __FUNCTION__);
        return;
    }

    {
        auto param = std::make_shared<ExynosParam<ParamFilmGrainInfo>>();

        memcpy(&(param->m.info), &info, sizeof(param->m.info));

        params.addParam(param);
    }

    ExynosLogD("[%s] film grain info : apply(%d), update(%d)", __FUNCTION__,
                info.apply_grain, info.update_grain);

    return;
}

void ExynosVideoCodecDec::updateExtraInfo(
    ExynosVideoMeta     *meta,
    ExynosVideoBuffer   &buffer) {
    ExynosLogFunctionTrace();

    if (meta != nullptr) {
        meta->eType = VIDEO_INFO_TYPE_INVALID;

        /* interlaced type for de-interlacing */
        if (mCodecImpl->mOutGeometry.bInterlaced == VIDEO_TRUE) {
            meta->eType = (ExynosVideoInfoType)(meta->eType | VIDEO_INFO_TYPE_INTERLACED);
            meta->data.dec.nInterlacedType = buffer.extraInfo.specific.dec.interlacedType;
        }

        /* actual format */
        if (mCodecImpl->mIsCompressedColor) {
            meta->nPixelFormat = 0;  /* HAL_PIXEL_FORMAT_UNUSED */

            if (buffer.extraInfo.specific.dec.frameType & VIDEO_FRAME_NEED_ACTUAL_FORMAT) {
                auto format = mCodecImpl->getActualFormat();

                if (format != VIDEO_COLORFORMAT_UNKNOWN) {
                    meta->eType = (ExynosVideoInfoType)(meta->eType | VIDEO_INFO_TYPE_CHECK_PIXEL_FORMAT);
                    meta->nPixelFormat = getPixelFormat(format);

                    ExynosLogD("[%s] filled with non-compressed format(0x%x)", __FUNCTION__, meta->nPixelFormat);
                }
            }
        }

        /* crop info : W/A for transcoding(dec's out is shared with enc's in) */
        {
            auto geometry = mCodecImpl->mOutGeometry;
            ExynosVideoCrop &crop = meta->crop;

            meta->eType = (ExynosVideoInfoType)(meta->eType | VIDEO_INFO_TYPE_CROP_INFO);
            crop.left   = geometry.cropRect.nLeft;
            crop.top    = geometry.cropRect.nTop;
            crop.width  = geometry.cropRect.nWidth;
            crop.height = geometry.cropRect.nHeight;
        }
    }

    return;
}

ExynosErrorType ExynosVideoCodecDec::checkRealTimeResource(
    int32_t width,
    int32_t height,
    int32_t operatingRate) {
    ExynosLogFunctionTrace();

    return mCodecImpl->checkRealTimeResource(width, height, operatingRate);
}

int ExynosVideoCodecDec::CodecSpecific_Set_FrameTag(std::shared_ptr<CodecImpl> codecImpl) {
    auto handle = codecImpl->mHandle;
    if (!CHECK_POINTER(handle)) {
        return -1;
    }

    std::get<ExynosVideoDecOps>(codecImpl->mCommonOps).Set_FrameTag(handle, codecImpl->mTagNum);
    return codecImpl->mTagNum;
}

int ExynosVideoCodecDec::CodecSpecific_Get_FrameTag(std::shared_ptr<CodecImpl> codecImpl) {
    auto handle = codecImpl->mHandle;
    if (!CHECK_POINTER(handle)) {
        return -1;
    }

    return std::get<ExynosVideoDecOps>(codecImpl->mCommonOps).Get_FrameTag(handle);
}

ExynosVideoErrorType ExynosVideoCodecDec::CodecSpecific_Stop_Wait_Buffer(std::shared_ptr<CodecImpl> codecImpl) {
    auto handle = codecImpl->mHandle;
    if (!CHECK_POINTER(handle)) {
        return VIDEO_ERROR_BADPARAM;
    }

    return std::get<ExynosVideoDecOps>(codecImpl->mCommonOps).Stop_Wait_Buffer(handle);
}

ExynosVideoErrorType ExynosVideoCodecDec::CodecSpecific_Reset_Wait_Buffer(std::shared_ptr<CodecImpl> codecImpl) {
    auto handle = codecImpl->mHandle;
    if (!CHECK_POINTER(handle)) {
        return VIDEO_ERROR_BADPARAM;
    }

    return std::get<ExynosVideoDecOps>(codecImpl->mCommonOps).Reset_Wait_Buffer(handle);
}

ExynosErrorType ExynosVideoCodecDec::getRefBufCount(int &cnt) {
    cnt = -1;

#ifdef USE_DPB_MANAGER
    if (mExynosPort[ExynosPort::Output].mBufManager.get() != nullptr) {
        auto refDPBManager = std::static_pointer_cast<RefDPBManager>(mExynosPort[ExynosPort::Output].mBufManager);

        cnt = refDPBManager->referencedDPBCount();
    }
#endif

    return EXYNOS_ERROR_NONE;
}

ExynosErrorType ExynosVideoCodecDec::getRegBufCount(int &cnt) {
    cnt = -1;

#ifdef USE_DPB_MANAGER
    if (mExynosPort[ExynosPort::Output].mBufManager.get() != nullptr) {
        auto refDPBManager = std::static_pointer_cast<RefDPBManager>(mExynosPort[ExynosPort::Output].mBufManager);

        cnt = refDPBManager->registeredDPBCount();
    }
#endif

    return EXYNOS_ERROR_NONE;
}

