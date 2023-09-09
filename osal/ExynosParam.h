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
#ifndef EXYNOS_PARAM_H
#define EXYNOS_PARAM_H

#include <string>
#include <memory>
#include <map>

#include "ExynosDef.h"
#include "ExynosLog.h"

typedef int32_t ParamIndex;

class ExynosParamBase {
public:
    ExynosParamBase() = default;

    virtual ~ExynosParamBase() = default;

    virtual ParamIndex index() = 0;

    virtual std::string name() = 0;
};

template<typename T>
class ExynosParam : public ExynosParamBase {
public:
    ExynosParam() = default;
    ExynosParam(const T& obj) {
        m = obj.m;
    }

    ~ExynosParam() = default;

    ParamIndex index() override {
        return T::INDEX;
    }

    std::string name() override {
        return std::string(T::NAME);
    }

    T m;
};

class ExynosParams {
public:
    ExynosParams() {
        mParams.clear();
    }

    ExynosParams& operator=(const ExynosParams &rhs) {
        this->mParams = rhs.mParams;

        return *this;
    }

    virtual ~ExynosParams() {
        mParams.clear();
    }

    bool addParam(std::shared_ptr<ExynosParamBase> param) {
        if (param.get() == nullptr) {
            return false;
        }

        /* already has a key. change latest value */
        auto ret = mParams.insert_or_assign(param->index(), param);
        if ((ret.second == false) && (ret.first != mParams.end())) {
            StaticExynosLog(Level::Trace, "ExynosParams", "[%s] already has key(%d) replace value in ExynosParams", __FUNCTION__, param->index());
            return true;
        }

        return ret.second;
    }

    std::shared_ptr<ExynosParamBase> getParam(ParamIndex index) {
        auto it = mParams.find(index);

        if (it != mParams.end()) {
            return it->second;
        }

        return nullptr;
    }

    virtual bool empty() {
        return mParams.empty();
    }

    virtual int size() {
        return mParams.size();
    }

    virtual void clear() {
        mParams.clear();
    }

protected:
    std::map<ParamIndex, std::shared_ptr<ExynosParamBase>> mParams;
};

enum ExynosParamIndex : ParamIndex {
    UnknownIndex = 0,
    RequestParamUpdateIndex,
    InputFrameInfoIndex,
    OutputFrameInfoIndex,
    InputCropIndex,
    OutputCropIndex,
    PixelFormatIndex,
    ActualFormatIndex,
    OutputDelayIndex,
    BitrateIndex,
    BitrateModeIndex,
    FramerateIndex,
    IDRPeriodIndex,
    ProfileLevelIndex,
    GOPIndex,
    BFrameIndex,
    IntraRefreshIndex,
    ColorAspectsIndex,
    HDRStaticInfoIndex,
    HDRDynamicInfoIndex,
    DataSpaceIndex,
    LayeringIndex,
    MaxLayeringIndex,
    DecodeOrderIndex,
    BufferCopyIndex,
    BlackBarIndex,
    BlackBarInfoIndex,
    DisplayDelayIndex,
    ThumbnailModeIndex,
    PrependHeaderModeIndex,
    FrameQpIndex,
    QpRangeIndex,
    DropControlIndex,
    DynamicFramerateIndex,
    RefPFramesIndex,
    GeneralPBIndex,
    CompressedColorIndex,
    FilterListInfoIndex,
    FilmGrainInfoIndex,
    HDR2SDRIndex,
    HDR2SDRModeIndex,
    HDR2SDRPixelFormatIndex,
    HDR2SDRFrameCountIndex,
    EnabledFilterNumIndex,
    OperatingRateIndex,
    RealTimePriorityIndex,
    SliceSizeIndex,
    IFrameRatioIndex,
    ChromaQpOffsetIndex,
    AverageQpIndex,
    AverageQpInfoIndex,
    EntropyModeIndex,
    PMVIndex,
    IntraVOPRefreshIndex,
    MinQualityIndex,
    HDREncodingIndex,
    HDR10PlusStatIndex,
    HDR10PlusStatInfoIndex,
    MaxIFrameSizeIndex,

    /* functions */
    FuncOutputDelayUpdate,

    /* skype */
    SkypeLowLatencyIndex,
    SkypeInputControlIndex,
    SkypeFrameQpIndex,
    SkypeLTRCountIndex,
    SkypeLTRIndex,
    SkypeSARIndex,
    SkypeBaseLayerPidIndex,
};

typedef uint64_t RequestedParamType;
enum ExynosRequestedParamFlag : RequestedParamType {
    REQUESTED_TYPE_NONE             = 0x0,
    REQUESTED_TYPE_PIC_SIZE         = 0x1 << 1,
    REQUESTED_TYPE_MAX_PIC_SIZE     = 0x1 << 2,
    REQUESTED_TYPE_MIN_DPB_CNT      = 0x1 << 3,

    REQUESTED_TYPE_ALL              = 0xFFFFFFFFFFFFFFFF,
};

struct ParamRequestParamUpdate {
    static constexpr char NAME[] = "request-param-update";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::RequestParamUpdateIndex,
    };

    ExynosRequestedParamFlag flags;
};

struct ParamInputFrameInfo {
    static constexpr char NAME[] = "input-frame-info";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::InputFrameInfoIndex,
    };

    size_t      width;
    size_t      height;
};

struct ParamOutputFrameInfo {
    static constexpr char NAME[] = "output-frame-finfo";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::OutputFrameInfoIndex,
    };

    size_t      width;
    size_t      height;
};

struct ParamInputCrop {
    static constexpr char NAME[] = "input-crop";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::InputCropIndex,
    };

    CropInfo crop;
};

struct ParamOutputCrop {
    static constexpr char NAME[] = "output-crop";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::OutputCropIndex,
    };

    CropInfo crop;
};

struct ParamPixelFormat {
    static constexpr char NAME[] = "pixel-format";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::PixelFormatIndex,
    };

    uint32_t format;
};

struct ParamActualFormat {
    static constexpr char NAME[] = "actual-format";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::ActualFormatIndex,
    };

    uint32_t format;
};

struct ParamOutputDelay {
    static constexpr char NAME[] = "output-delay";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::OutputDelayIndex,
    };

    int32_t delay;
};

struct ParamBitrate {
    static constexpr char NAME[] = "bitrate";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::BitrateIndex,
    };

    uint32_t bitrate;
};

typedef enum __ExynosBitrateMode {
    BITRATE_MODE_DISABLED = 0,
    BITRATE_MODE_DEFAULT,
    BITRATE_MODE_CONST,
    BITRATE_MODE_CONST_WITH_FRAME_SKIP,
    BITRATE_MODE_CONST_VT_CALL,
    BITRATE_MODE_VARIABLE,
    BITRATE_MODE_VARIABLE_WITH_FRAME_SKIP,
    BITRATE_MODE_CONST_QUALITY,
    BITRATE_MODE_CONST_WFD,
    BITRATE_MODE_VARIABLE_BIT_SAVE,
} ExynosBitrateMode;

struct ParamBitrateMode {
    static constexpr char NAME[] = "bitrate-mode";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::BitrateModeIndex,
    };

    ExynosBitrateMode mode;
};

struct ParamFramerate {
    static constexpr char NAME[] = "framerate";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::FramerateIndex,
    };

    uint32_t framerate;
};

struct ParamIDRPeriod {
    static constexpr char NAME[] = "IDR-period";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::IDRPeriodIndex,
    };

    uint32_t period;
};

struct ParamProfileLevel {
    static constexpr char NAME[] = "profile-level";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::ProfileLevelIndex,
    };

    uint32_t profile;
    uint32_t level;
};

struct ParamGOP {
    static constexpr char NAME[] = "gop";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::GOPIndex,
    };

    uint32_t size;
};

struct ParamBFrame {
    static constexpr char NAME[] = "b-frame";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::BFrameIndex,
    };

    uint32_t bframes;
};

struct ParamIntraRefresh {
    static constexpr char NAME[] = "intra-refresh";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::IntraRefreshIndex,
    };

    float period;
};

struct ParamColorAspects {
    static constexpr char NAME[] = "color-aspects";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::ColorAspectsIndex,
    };

    int32_t range;
    int32_t primaries;
    int32_t transfer;
    int32_t matrix;
};

struct ParamHdrStaticInfo {
    static constexpr char NAME[] = "color-aspects";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::HDRStaticInfoIndex,
    };

    ExynosHdrStaticInfo ST;
};

struct ParamHdrDynamicInfo {
    static constexpr char NAME[] = "color-aspects";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::HDRDynamicInfoIndex,
    };

    ExynosHdrDynamicInfo DY;
};

struct ParamDataSpace {
    static constexpr char NAME[] = "dataspace";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::DataSpaceIndex,
    };

    int dataspace;
};

struct ParamLayering {
    static constexpr char NAME[] = "layering";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::LayeringIndex,
    };

    uint32_t layerCount;
    uint32_t bLayerCount;
    uint32_t useAdaptiveBitrate;
    uint32_t bitrate[MAX_TEMPORAL_LAYERS];
};

struct ParamMaxLayering {
    static constexpr char NAME[] = "max-layering";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::MaxLayeringIndex,
    };

    uint32_t layerCount;
};

struct ParamDecodeOrder {
    static constexpr char NAME[] = "decode-order";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::DecodeOrderIndex,
    };

    uint32_t enable;
};

struct ParamBufferCopy {
    static constexpr char NAME[] = "buffer-copy";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::BufferCopyIndex,
    };

    uint32_t enable;
};

struct ParamBlackBar {
    static constexpr char NAME[] = "black-bar";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::BlackBarIndex,
    };

    uint32_t enable;
};

struct ParamBlackBarInfo {
    static constexpr char NAME[] = "black-bar-info";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::BlackBarInfoIndex,
    };

    CropInfo rect;
};

struct ParamDisplayDelay {
    static constexpr char NAME[] = "display-delay";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::DisplayDelayIndex,
    };

    uint32_t delay;
};

struct ParamThumbnailMode {
    static constexpr char NAME[] = "thumbnail-mode";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::ThumbnailModeIndex,
    };

    uint32_t enable;
};

typedef enum __ExynosPrependHeaderMode {
    PREPEND_HEADER_MODE_NONE = 0,
    PREPEND_HEADER_MODE_SPS_PPS_TO_IDR,
} ExynosPrependHeaderMode;

struct ParamPrependHeaderMode {
    static constexpr char NAME[] = "prepend-header-mode";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::PrependHeaderModeIndex,
    };

    ExynosPrependHeaderMode mode;
};

struct ParamFrameQP {
    static constexpr char NAME[] = "frame-qp";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::FrameQpIndex,
    };

    uint32_t I;
    uint32_t P;
    uint32_t B;
};

struct ParamQpRange {
    static constexpr char NAME[] = "qp-range";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::QpRangeIndex,
    };

    uint32_t minI;
    uint32_t maxI;
    uint32_t minP;
    uint32_t maxP;
    uint32_t minB;
    uint32_t maxB;
};

struct ParamDropControl {
    static constexpr char NAME[] = "drop-control";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::DropControlIndex,
    };

    uint32_t enable;
};

struct ParamDynamicFramerate {
    static constexpr char NAME[] = "dynamic-framerate";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::DynamicFramerateIndex,
    };

    uint32_t enable;
};

struct ParamRefPframes {
    static constexpr char NAME[] = "reference-p-frames";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::RefPFramesIndex,
    };

    uint32_t pframes;
};

struct ParamGeneralPB {
    static constexpr char NAME[] = "general-pb";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::GeneralPBIndex,
    };

    uint32_t enable;
};

struct ParamCompressedColor {
    static constexpr char NAME[] = "compressed-color";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::CompressedColorIndex,
    };

    uint32_t format;
};

struct ParamFilterListInfo {
    static constexpr char NAME[] = "filter-list-info";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::FilterListInfoIndex,
    };

    std::weak_ptr<std::map<std::string, int>> wkInfo;
};

struct ParamFilmGrainInfo {
    static constexpr char NAME[] = "film-grain-info";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::FilmGrainInfoIndex,
    };

    FilmGrainInfo info;
};

struct ParamHDR2SDR {
    static constexpr char NAME[] = "hdr-to-sdr";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::HDR2SDRIndex,
    };

    uint32_t enable;
};

struct ParamHDR2SDRMode {
    static constexpr char NAME[] = "hdr-to-sdr-mode";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::HDR2SDRModeIndex,
    };

    uint32_t type;
};

struct ParamHDR2SDRPixelFormat {
    static constexpr char NAME[] = "hdr-to-sdr-pixel-format";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::HDR2SDRPixelFormatIndex,
    };

    uint32_t format;
};

struct ParamHDR2SDRFrameCount {
    static constexpr char NAME[] = "hdr-to-sdr-frame-count";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::HDR2SDRFrameCountIndex,
    };

    uint32_t value;
};

struct ParamEnabledFilterNum {
    static constexpr char NAME[] = "enabled-filter-num";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::EnabledFilterNumIndex,
    };

    uint32_t num;
};

struct ParamOperatingRate {
    static constexpr char NAME[] = "operating-rate";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::OperatingRateIndex,
    };

    uint32_t value;
};

struct ParamRealTimePriority {
    static constexpr char NAME[] = "realtime-priority";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::RealTimePriorityIndex,
   };

    uint32_t value;
};

struct ParamAverageQp {
    static constexpr char NAME[] = "average-qp";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::AverageQpIndex,
    };

    uint32_t enable;
};

struct ParamAverageQpInfo {
    static constexpr char NAME[] = "average-qp-info";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::AverageQpInfoIndex,
    };

    int32_t value;
};

typedef enum __ExynosSliceMode {
    SLICE_MODE_NONE     = 0,
    SLICE_MODE_MB       = 1,
    SLICE_MODE_BYTES    = 2,
} ExynosSliceMode;

struct ParamSliceSize {
    static constexpr char NAME[] = "slice-size";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::SliceSizeIndex,
    };

    ExynosSliceMode mode;
    uint32_t size;
};

struct ParamIFrameRatio {
    static constexpr char NAME[] = "i-frame-ratio";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::IFrameRatioIndex,
    };

    uint32_t value;
};

struct ParamChromaQpOffset {
    static constexpr char NAME[] = "chroma-qp-offset";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::ChromaQpOffsetIndex,
    };

    int32_t cb;
    int32_t cr;
};

struct ParamEntropyMode {
    static constexpr char NAME[] = "entropy-mode";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::EntropyModeIndex,
    };

    uint32_t value;
};

struct ParamPMV {
    static constexpr char NAME[] = "pmv";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::PMVIndex,
    };

    uint32_t mode;
    int32_t hor_L0;
    int32_t hor_L1;
    int32_t ver_L0;
    int32_t ver_L1;
};

struct ParamIntraVOPRefresh {
    static constexpr char NAME[] = "intra-vop-refresh";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::IntraVOPRefreshIndex,
    };

    int32_t request;
};

struct ParamMinQuality {
    static constexpr char NAME[] = "min-quality";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::MinQualityIndex,
    };

    int32_t level;
};

typedef enum __ExynosHdrEncodingType {
    HDR_ENCODING_UNKNOWN = 0,
    HDR_ENCODING_HDR10,         /* HDR static */
    HDR_ENCODING_HDR10_PLUS,    /* HDR static + HDR dynamic */
} ExynosHdrEncodingType;

struct ParamHdrEncoding {
    static constexpr char NAME[] = "hdr-encoding";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::HDREncodingIndex,
    };

    ExynosHdrEncodingType type;
};

struct FuncParamOutputDelayUpdate {
    static constexpr char NAME[] = "func-output-delay-update";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::FuncOutputDelayUpdate,
    };

    std::shared_ptr<std::function<void(std::shared_ptr<ExynosParams> params)>> func;
};

typedef struct HDRStatInfo {
    unsigned int  hdr10_plus_stat_sei_size;
    unsigned int  hdr10_plus_stat_offset;

    ExynosHdrDynamicStatInfo    sHdrDynamicStatInfo;
} HDRStatInfo;

struct ParamHDR10PlusStat {
    static constexpr char NAME[] = "hdr-10-plus-stat";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::HDR10PlusStatIndex,
    };

    int32_t enable;
};

struct ParamHDR10PlusStatInfo {
    static constexpr char NAME[] = "hdr-10-plus-stat-info";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::HDR10PlusStatInfoIndex,
    };

    HDRStatInfo info;
};

struct ParamMaxIFrameSize {
    static constexpr char NAME[] = "max-I-frame-size";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::MaxIFrameSizeIndex,
    };

    int size;
};

/* skype */
struct ParamSkypeLowLatency {
    static constexpr char NAME[] = "skype-low-latency";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::SkypeLowLatencyIndex,
    };

    int32_t enable;
};

struct ParamSkypeInputControl {
    static constexpr char NAME[] = "skype-input-control";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::SkypeInputControlIndex,
    };

    int32_t enable;
};

struct ParamSkypeFrameQP {
    static constexpr char NAME[] = "skype-frame-qp";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::SkypeFrameQpIndex,
    };

    int32_t value;
};

struct ParamSkypeLTRCount {
    static constexpr char NAME[] = "skype-ltr-count";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::SkypeLTRCountIndex,
    };

    int32_t value;
};

struct ParamSkypeLTR {
    static constexpr char NAME[] = "skype-ltr";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::SkypeLTRIndex,
    };

    int32_t mark;
    int32_t use;
};

struct ParamSkypeSAR {
    static constexpr char NAME[] = "skype-sar";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::SkypeSARIndex,
    };

    int32_t width;
    int32_t height;
};

struct ParamSkypeBaseLayerPid {
    static constexpr char NAME[] = "skype-base-layer-pid";

    enum : ParamIndex {
        INDEX = ExynosParamIndex::SkypeBaseLayerPidIndex,
    };

    int32_t value;
};

#endif // EXYNOS_PARAM_H
