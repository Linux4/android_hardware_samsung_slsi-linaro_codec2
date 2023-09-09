/*
 *
 * Copyright 2019 Samsung Electronics S.LSI Co. LTD
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
#ifndef C2_EXYNOS_PARAM_H
#define C2_EXYNOS_PARAM_H

#include <C2Config.h>

#include "ExynosC2Constants.h"

// namespace android {

enum C2ExynosParamIndexKind : C2Param::type_index_t {
    kExynosParamIndexDecodeOrder = C2Param::TYPE_INDEX_VENDOR_START,
    kExynosParamIndexOutputStreamUsage,
    kExynosParamIndexReorderTimestamp,
    kExynosParamIndexCSCOutputPixelFormat,
    kExynosParamIndexCSCDataSpace,
    kExynosParamIndexCSCBufferCopy,
    kExynosParamIndexBlackBar,
    kExynosParamIndexBlackBarInfo,
    kExynosParamIndexDisplayDelay,
    kExynosParamIndexThumbnailMode,
    kExynosParamIndexCodecDPBNum,
    kExynosParamIndexQpRange,
    kExynosParamIndexDropControl,
    kExynosParamIndexDynamicFramerate,
    kExynosParamIndexRefPframes,
    kExynosParamIndexGeneralPB,
    kExynosParamIndexCompressedColor,
    kExynosParamIndexHdrStaticValidation,
    kExynosParamIndexImageConvert,
    kExynosParamIndexImageConvertMode,
    kExynosParamIndexImageConvertPixelFormat,
    kExynosParamIndexImageConvertControlFrame,
    kExynosParamIndexExtraBufferNum,
    kExynosParamIndexSliceSize,
    kExynosParamIndexVTCall,
    kExynosParamIndexIFrameRatio,
    kExynosParamIndexChromaQpOffset,
    kExynosParamIndexAverageQp,
    kExynosParamIndexAverageQpInfo,
    kExynosParamIndexEntropyMode,
    kExynosParamIndexPMV,
    kExynosParamIndexStartIDRFrame,
    kExynosParamIndexHDR10PlusStatEnc,
    kExynosParamIndexInternalUsage,
    kExynosParamIndex10BitSupport,
    kExynosParamIndexLLWFDMode,
    kExynosParamIndexMaxIFrameSize,

    /* skype */
    kExynosParamIndexSkypeDriverVersion,
    kExynosParamIndexSkypeEncMaxTemporalLayer,
    kExynosParamIndexSkypeEncPreprocess,
    kExynosParamIndexSkypeEncMaxLTR,
    kExynosParamIndexSkypeLowLatency,
    kExynosParamIndexSkypeEncInputControl,
    kExynosParamIndexSkypeEncLTRCount,
    kExynosParamIndexSkypeEncLTR,
    kExynosParamIndexSkypeEncCustomProfileLevel,
    kExynosParamIndexSkypeEncSar,
    kExynosParamIndexSkypeEncSliceHeaderSpacing,
    kExynosParamIndexSkypeEncFrameQP,
    kExynosParamIndexSkypeEncBaseLayerPID,
    kExynosParamIndexSkypeEncBitrateMode,
};

namespace VendorC2Config {

enum vendor_bitrate_mode_t : uint32_t {
#if 0  /* C2Config.h */
    BITRATE_CONST_SKIP_ALLOWED = 0,      ///< constant bitrate, frame skipping allowed
    BITRATE_CONST = 1,                   ///< constant bitrate, keep all frames
    BITRATE_VARIABLE_SKIP_ALLOWED = 2,   ///< bitrate can vary, frame skipping allowed
    BITRATE_VARIABLE = 3,                ///< bitrate can vary, keep all frames
    BITRATE_IGNORE = 7,                  ///< bitrate can be exceeded at will to achieve
                                         ///< quality or other settings

    // bitrate modes are composed of the following flags
    BITRATE_FLAG_KEEP_ALL_FRAMES = 1,
    BITRATE_FLAG_CAN_VARY = 2,
    BITRATE_FLAG_CAN_EXCEED = 4,
#endif
    BITRATE_VENDOR_START                = (uint32_t)VENDOR_BITRATE_MODE_START,
    BITRATE_VENDOR_CONST_VT_CALL        = (uint32_t)VENDOR_BITRATE_MODE_CBR_VTCALL,
    BITRATE_VENDOR_CONST_WFD            = (uint32_t)VENDOR_BITRATE_MODE_CBR_WFD,
    BITRATE_VENDOR_VARIABLE_BIT_SAVE    = (uint32_t)VENDOR_BITRATE_MODE_VBR_BITSAVE,
};

enum vendor_profile_t : uint32_t {
    PROFILE_VC1_SIMPLE      = (uint32_t)VC1ProfileSimple,
    PROFILE_VC1_MAIN        = (uint32_t)VC1ProfileMain,
    PROFILE_VC1_ADVANCED    = (uint32_t)VC1ProfileAdvanced,
};

enum vendor_level_t : uint32_t {
    LEVEL_VC1_LOW       = (uint32_t)VC1LevelLow,
    LEVEL_VC1_MEDIUM    = (uint32_t)VC1LevelMedium,
    LEVEL_VC1_HIGH      = (uint32_t)VC1LevelHigh,
    LEVEL_VC1_0         = (uint32_t)VC1Level0,
    LEVEL_VC1_1         = (uint32_t)VC1Level1,
    LEVEL_VC1_2         = (uint32_t)VC1Level2,
    LEVEL_VC1_3         = (uint32_t)VC1Level3,
    LEVEL_VC1_4         = (uint32_t)VC1Level4,
};

enum vendor_compressed_color_t : uint32_t {
    COMPRESSED_COLOR_NONE       = (uint32_t)VENDOR_COMPRESSED_COLOR_NONE,
    COMPRESSED_COLOR_LOSSLESS   = (uint32_t)VENDOR_COMPRESSED_COLOR_LOSSLESS,
};

enum vendor_entropy_mode_t : uint32_t {
    ENTROPY_MODE_CAVLC = (uint32_t)VENDOR_ENTROPY_MODE_CAVLC,
    ENTROPY_MODE_CABAC = (uint32_t)VENDOR_ENTROPY_MODE_CABAC,
};

struct C2ExynosQpRangeStruct {
public:
    uint32_t I_minQP;
    uint32_t I_maxQP;
    uint32_t P_minQP;
    uint32_t P_maxQP;
    uint32_t B_minQP;
    uint32_t B_maxQP;

    inline C2ExynosQpRangeStruct()
        : I_minQP(0), I_maxQP(0), P_minQP(0), P_maxQP(0), B_minQP(0), B_maxQP(0) { }
    inline C2ExynosQpRangeStruct(uint32_t I_minQP_, uint32_t I_maxQP_,
                                 uint32_t P_minQP_, uint32_t P_maxQP_,
                                 uint32_t B_minQP_ = 0, uint32_t B_maxQP_ = 0)
        : I_minQP(I_minQP_), I_maxQP(I_maxQP_),
          P_minQP(P_minQP_), P_maxQP(P_maxQP_),
          B_minQP(B_minQP_), B_maxQP(B_maxQP_) { }

    enum : uint32_t { CORE_INDEX = kExynosParamIndexQpRange };

    DEFINE_AND_DESCRIBE_BASE_C2STRUCT(ExynosQpRange)
    C2FIELD(I_minQP, "I-minQP")
    C2FIELD(I_maxQP, "I-maxQP")
    C2FIELD(P_minQP, "P-minQP")
    C2FIELD(P_maxQP, "P-maxQP")
    C2FIELD(B_minQP, "B-minQP")
    C2FIELD(B_maxQP, "B-maxQP")
};

struct C2ExynosChromaQpStruct {
public:
    int32_t Cb;
    int32_t Cr;

    inline C2ExynosChromaQpStruct()
        : Cb(0), Cr(0) { }
    inline C2ExynosChromaQpStruct(int32_t Cb_, int32_t Cr_)
        : Cb(Cb_), Cr(Cr_) { }

    enum : uint32_t { CORE_INDEX = kExynosParamIndexChromaQpOffset };

    DEFINE_AND_DESCRIBE_BASE_C2STRUCT(ExynosChromaQp)
    C2FIELD(Cb, "cb")
    C2FIELD(Cr, "cr")
};

enum vendor_pmv_mode_t : uint32_t {
    PMV_DISABLED    = (uint32_t)VENDOR_PMV_DISABLED,
    PMV_LOCAL       = (uint32_t)VENDOR_PMV_LOCAL,
    PMV_GLOBAL      = (uint32_t)VENDOR_PMV_GLOBAL,
};

struct C2ExynosPMVStruct {
public:
    uint32_t Mode;
    int32_t  HorizonL0;
    int32_t  HorizonL1;
    int32_t  VerticalL0;
    int32_t  VerticalL1;

    inline C2ExynosPMVStruct()
        : Mode(0), HorizonL0(0), HorizonL1(0), VerticalL0(0), VerticalL1(0) { }
    inline C2ExynosPMVStruct(int32_t Mode_)
        : Mode(Mode_), HorizonL0(0), HorizonL1(0), VerticalL0(0), VerticalL1(0) { }
    inline C2ExynosPMVStruct(int32_t Mode_,
                             int32_t HorizonL0_, int32_t HorizonL1_, int32_t VerticalL0_, int32_t VerticalL1_)
        : Mode(Mode_), HorizonL0(HorizonL0_), HorizonL1(HorizonL1_), VerticalL0(VerticalL0_), VerticalL1(VerticalL1_) { }

    enum : uint32_t { CORE_INDEX = kExynosParamIndexPMV };

    DEFINE_AND_DESCRIBE_BASE_C2STRUCT(ExynosPMV)
    C2FIELD(Mode,       "mode")
    C2FIELD(HorizonL0,  "horizonL0")
    C2FIELD(HorizonL1,  "horizonL1")
    C2FIELD(VerticalL0, "verticalL0")
    C2FIELD(VerticalL1, "verticalL1")
};

enum vendor_memoryusage_t : uint64_t {
    MAY_CPU_READ = (uint64_t)VENDOR_MEM_USAGE_CPU_READ,
};

struct C2ExynosSkypeDriverVersionStruct {
public:
    int32_t number;

    inline C2ExynosSkypeDriverVersionStruct()
        : number(0) { }
    inline C2ExynosSkypeDriverVersionStruct(int32_t number_)
        : number(number_) { }

    enum : uint32_t { CORE_INDEX = kExynosParamIndexSkypeDriverVersion };

    DEFINE_AND_DESCRIBE_BASE_C2STRUCT(ExynosSkypeDriverVersion)
    C2FIELD(number, "number")
};

struct C2ExynosSkypeMaxTemporalLayerStruct {
public:
    int32_t max_p_count;
    int32_t max_b_count;

    inline C2ExynosSkypeMaxTemporalLayerStruct()
        : max_p_count(0), max_b_count(0) { }
    inline C2ExynosSkypeMaxTemporalLayerStruct(int32_t max_p_count_)
        : max_p_count(max_p_count_), max_b_count(0) { }
    inline C2ExynosSkypeMaxTemporalLayerStruct(int32_t max_p_count_, int32_t max_b_count_)
        : max_p_count(max_p_count_), max_b_count(max_b_count_) { }

    enum : uint32_t { CORE_INDEX = kExynosParamIndexSkypeEncMaxTemporalLayer };

    DEFINE_AND_DESCRIBE_BASE_C2STRUCT(ExynosSkypeMaxTemporalLayer)
    C2FIELD(max_p_count, "max-p-count")
    C2FIELD(max_b_count, "max-b-count")
};

struct C2ExynosSkypePreprocessStruct {
public:
    int32_t max_downscale_factor;
    int32_t rotation;

    inline C2ExynosSkypePreprocessStruct()
        : max_downscale_factor(0), rotation(0) { }
    inline C2ExynosSkypePreprocessStruct(int32_t max_downscale_factor_, int32_t rotation_)
        : max_downscale_factor(max_downscale_factor_), rotation(rotation_) { }

    enum : uint32_t { CORE_INDEX = kExynosParamIndexSkypeEncPreprocess };

    DEFINE_AND_DESCRIBE_BASE_C2STRUCT(ExynosSkypePreprocess)
    C2FIELD(max_downscale_factor, "max-downscale-factor")
    C2FIELD(rotation, "rotation")
};

struct C2ExynosSkypeMaxLTRStruct {
public:
    int32_t max_count;

    inline C2ExynosSkypeMaxLTRStruct()
        : max_count(0) { }
    inline C2ExynosSkypeMaxLTRStruct(int32_t max_count_)
        : max_count(max_count_) { }

    enum : uint32_t { CORE_INDEX = kExynosParamIndexSkypeEncMaxLTR };

    DEFINE_AND_DESCRIBE_BASE_C2STRUCT(ExynosSkypeMaxLTR)
    C2FIELD(max_count, "max-count")
};

struct C2ExynosSkypeLowlatencyStruct {
public:
    int32_t enable;

    inline C2ExynosSkypeLowlatencyStruct()
        : enable(0) { }
    inline C2ExynosSkypeLowlatencyStruct(int32_t enable_)
        : enable(enable_) { }

    enum : uint32_t { CORE_INDEX = kExynosParamIndexSkypeLowLatency };

    DEFINE_AND_DESCRIBE_BASE_C2STRUCT(ExynosSkypeLowlatency)
    C2FIELD(enable, "enable")
};

struct C2ExynosSkypeInputControlStruct {
public:
    int32_t enable;

    inline C2ExynosSkypeInputControlStruct()
        : enable(0) { }
    inline C2ExynosSkypeInputControlStruct(int32_t enable_)
        : enable(enable_) { }

    enum : uint32_t { CORE_INDEX = kExynosParamIndexSkypeEncInputControl };

    DEFINE_AND_DESCRIBE_BASE_C2STRUCT(ExynosSkypeInputControl)
    C2FIELD(enable, "enable")
};

enum vendor_skype_profile_t : int32_t {
    SKYPE_PROFILE_NONE                  = SkypeProfileNone,
    SKYPE_PROFILE_BASELINE              = SkypeProfileBaseline,
    SKYPE_PROFILE_MAIN                  = SkypeProfileMain,
    SKYPE_PROFILE_HIGH                  = SkypeProfileHigh,
    SKYPE_PROFILE_CONSTRAINED_BASELINE  = SkypeProfileConstrainedBaseline,
    SKYPE_PROFILE_CONSTRAINED_HIGH      = SkypeProfileConstrainedHigh,
};

enum vendor_skype_level_t : int32_t {
    SKYPE_LEVEL_NONE     = SkypeLevelNone,
    SKYPE_LEVEL_1        = SkypeLevel1,
    SKYPE_LEVEL_1B       = SkypeLevel1B,
    SKYPE_LEVEL_1_1      = SkypeLevel1_1,
    SKYPE_LEVEL_1_2      = SkypeLevel1_2,
    SKYPE_LEVEL_1_3      = SkypeLevel1_3,
    SKYPE_LEVEL_2        = SkypeLevel2,
    SKYPE_LEVEL_2_1      = SkypeLevel2_1,
    SKYPE_LEVEL_2_2      = SkypeLevel2_2,
    SKYPE_LEVEL_3        = SkypeLevel3,
    SKYPE_LEVEL_3_1      = SkypeLevel3_1,
    SKYPE_LEVEL_3_2      = SkypeLevel3_2,
    SKYPE_LEVEL_4        = SkypeLevel4,
    SKYPE_LEVEL_4_1      = SkypeLevel4_1,
    SKYPE_LEVEL_4_2      = SkypeLevel4_2,
    SKYPE_LEVEL_5        = SkypeLevel5,
    SKYPE_LEVEL_5_1      = SkypeLevel5_1,
    SKYPE_LEVEL_5_2      = SkypeLevel5_2,
    SKYPE_LEVEL_6        = SkypeLevel6,
    SKYPE_LEVEL_6_1      = SkypeLevel6_1,
    SKYPE_LEVEL_6_2      = SkypeLevel6_2,
};

struct C2ExynosSkypeCustomProfileLevelStruct {
public:
    int32_t profile;
    int32_t level;

    inline C2ExynosSkypeCustomProfileLevelStruct()
        : profile(0), level(0) { }
    inline C2ExynosSkypeCustomProfileLevelStruct(int32_t profile_, int32_t level_)
        : profile(profile_), level(level_) { }

    enum : uint32_t { CORE_INDEX = kExynosParamIndexSkypeEncCustomProfileLevel };

    DEFINE_AND_DESCRIBE_BASE_C2STRUCT(ExynosSkypeCustomProfileLevel)
    C2FIELD(profile, "profile")
    C2FIELD(level, "level")
};

struct C2ExynosSkypeLTRCountStruct {
public:
    int32_t num_ltr_frames;

    inline C2ExynosSkypeLTRCountStruct()
        : num_ltr_frames(0) { }
    inline C2ExynosSkypeLTRCountStruct(int32_t num_ltr_frames_)
        : num_ltr_frames(num_ltr_frames_) { }

    enum : uint32_t { CORE_INDEX = kExynosParamIndexSkypeEncLTRCount };

    DEFINE_AND_DESCRIBE_BASE_C2STRUCT(ExynosSkypeLTRCount)
    C2FIELD(num_ltr_frames, "num-ltr-frames")
};

struct C2ExynosSkypeLTRStruct {
public:
    int32_t mark_frame;
    int32_t use_frame;

    inline C2ExynosSkypeLTRStruct()
        : mark_frame(0), use_frame(0) { }
    inline C2ExynosSkypeLTRStruct(int32_t mark_frame_, int32_t use_frame_)
        : mark_frame(mark_frame_), use_frame(use_frame_) { }

    enum : uint32_t { CORE_INDEX = kExynosParamIndexSkypeEncLTR };

    DEFINE_AND_DESCRIBE_BASE_C2STRUCT(ExynosSkypeLTR)
    C2FIELD(mark_frame, "mark-frame")
    C2FIELD(use_frame, "use-frame")
};

struct C2ExynosSkypeSliceHeaderSpacingStruct {
public:
    int32_t spacing;

    inline C2ExynosSkypeSliceHeaderSpacingStruct()
        : spacing(0) { }
    inline C2ExynosSkypeSliceHeaderSpacingStruct(int32_t spacing_)
        : spacing(spacing_) { }

    enum : uint32_t { CORE_INDEX = kExynosParamIndexSkypeEncSliceHeaderSpacing };

    DEFINE_AND_DESCRIBE_BASE_C2STRUCT(ExynosSkypeSliceHeaderSpacing)
    C2FIELD(spacing, "spacing")
};

enum vendor_skype_bitrate_mode_t : int32_t {
    SKYPE_BITRATE_MODE_IGNORE           = VENDOR_SKYPE_BITRATE_MODE_IGNORE,
    SKYPE_BITRATE_MODE_VBR              = VENDOR_SKYPE_BITRATE_MODE_VBR,
    SKYPE_BITRATE_MODE_CBR              = VENDOR_SKYPE_BITRATE_MODE_CBR,
    SKYPE_BITRATE_MODE_VBR_SKIP_FRAME   = VENDOR_SKYPE_BITRATE_MODE_VBR_SKIP_FRAME,
    SKYPE_BITRATE_MODE_CBR_SKIP_FRAME   = VENDOR_SKYPE_BITRATE_MODE_CBR_SKIP_FRAME,
    SKYPE_BITRATE_MODE_NONE             = VENDOR_SKYPE_BITRATE_MODE_NONE,
};

struct C2ExynosSkypeBitrateModeStruct {
public:
    int32_t value;
    int32_t bitrate;

    inline C2ExynosSkypeBitrateModeStruct()
        : value(0), bitrate(0) { }
    inline C2ExynosSkypeBitrateModeStruct(int32_t value_)
        : value(value_), bitrate(0) { }
    inline C2ExynosSkypeBitrateModeStruct(int32_t value_, int32_t bitrate_)
        : value(value_), bitrate(bitrate_) { }

    enum : uint32_t { CORE_INDEX = kExynosParamIndexSkypeEncBitrateMode };

    DEFINE_AND_DESCRIBE_BASE_C2STRUCT(ExynosSkypeBitrateMode)
    C2FIELD(value, "value")
    C2FIELD(bitrate, "bitrate")
};

} // namespace VendorC2Config

typedef C2StreamParam<C2Setting, C2Uint64Value, kExynosParamIndexOutputStreamUsage> C2ExynosStreamOutputStreamSetting;
constexpr char C2EXYNOS_PARAMKEY_VENDOR_OUTPUT_STREAM_USAGE[] = VENDOR_KEY_NAME_OUTPUT_STREAM_USAGE;

typedef C2PortParam<C2Setting, C2Uint32Value, kExynosParamIndexDecodeOrder> C2ExynosPortDecodeOrderSetting;
constexpr char C2EXYNOS_PARAMKEY_DECODE_ORDER[] = VENDOR_KEY_NAME_DECODE_ORDER;

typedef C2PortParam<C2Setting, C2Uint32Value, kExynosParamIndexReorderTimestamp> C2ExynosPortReorderTimestampSetting;
constexpr char C2EXYNOS_PARAMKEY_REORDER_TIMESTAMP[] = VENDOR_KEY_NAME_REORDER_TIMESTAMP;

/* internal */
typedef C2GlobalParam<C2Info, C2Uint32Value, kExynosParamIndexCSCOutputPixelFormat> C2ExynosCSCOutputPixelFormatInfo;
constexpr char C2EXYNOS_PARAMKEY_CSC_OUTPUT_PIXEL_FORMAT[] = "sec-csc-output.pixel-format";

/* internal */
typedef C2StreamParam<C2Info, C2Uint32Value, kExynosParamIndexCSCDataSpace> C2ExynosStreamCSCDataSpaceInfo;
constexpr char C2EXYNOS_PARAMKEY_CSC_DATA_SPACE[] = "sec-csc-dataspace";

typedef C2PortParam<C2Setting, C2Uint32Value, kExynosParamIndexCSCBufferCopy> C2ExynosPortCSCBufferCopySetting;
constexpr char C2EXYNOS_PARAMKEY_CSC_BUFFER_COPY[] = VENDOR_KEY_NAME_BUFFER_COPY;

typedef C2PortParam<C2Setting, C2Uint32Value, kExynosParamIndexBlackBar> C2ExynosPortBlackBarSetting;
constexpr char C2EXYNOS_PARAMKEY_BLACK_BAR[] = VENDOR_KEY_NAME_BLACK_BAR;

typedef C2StreamParam<C2Info, C2RectStruct, kExynosParamIndexBlackBarInfo> C2ExynosStreamBlackBarInfo;
constexpr char C2EXYNOS_PARAMKEY_BLACK_BAR_INFO[] = VENDOR_KEY_NAME_BLACK_BAR_INFO;

typedef C2PortParam<C2Setting, C2Int32Value, kExynosParamIndexDisplayDelay> C2ExynosPortDisplayDelaySetting;
constexpr char C2EXYNOS_PARAMKEY_DISPLAY_DELAY[] = VENDOR_KEY_NAME_DISPLAY_DELAY;

typedef C2PortParam<C2Setting, C2Uint32Value, kExynosParamIndexThumbnailMode> C2ExynosPortThumbnailModeSetting;
constexpr char C2EXYNOS_PARAMKEY_THUMBNAIL_MODE[] = VENDOR_KEY_NAME_THUMBNAIL_MODE;

/* internal */
typedef C2StreamParam<C2Info, C2Uint32Value, kExynosParamIndexCodecDPBNum> C2ExynosStreamMaxDPBNumInfo;
constexpr char C2EXYNOS_PARAMKEY_MAX_DPB_NUM[] = "sec-dec-output.dpb-num";

typedef C2PortParam<C2Tuning, VendorC2Config::C2ExynosQpRangeStruct, kExynosParamIndexQpRange> C2ExynosPortQpRangeTuning;
constexpr char C2EXYNOS_PARAMKEY_QP_RANGE[] = VENDOR_KEY_NAME_QP_RANGE;

typedef C2PortParam<C2Setting, C2Uint32Value, kExynosParamIndexDropControl> C2ExynosPortDropControlSetting;
constexpr char C2EXYNOS_PARAMKEY_DROP_CONTROL[] = VENDOR_KEY_NAME_DROP_CONTROL;

typedef C2PortParam<C2Setting, C2Uint32Value, kExynosParamIndexDynamicFramerate> C2ExynosPortDynamicFramerateSetting;
constexpr char C2EXYNOS_PARAMKEY_DYNAMIC_FRAMERATE[] = VENDOR_KEY_NAME_DYNAMIC_FRAMERATE;

typedef C2PortParam<C2Setting, C2Uint32Value, kExynosParamIndexRefPframes> C2ExynosPortRefPframeSetting;
constexpr char C2EXYNOS_PARAMKEY_REF_P_FRAMES[] = VENDOR_KEY_NAME_REF_P_FRAMES;

typedef C2PortParam<C2Setting, C2Uint32Value, kExynosParamIndexGeneralPB> C2ExynosPortGeneralPBSetting;
constexpr char C2EXYNOS_PARAMKEY_GENERAL_PB[] = VENDOR_KEY_NAME_GENERAL_PB;

typedef C2PortParam<C2Setting, C2Uint32Value, kExynosParamIndexCompressedColor> C2ExynosPortCompressedColorSetting;
constexpr char C2EXYNOS_PARAMKEY_COMPRESSED_COLOR[] = VENDOR_KEY_NAME_COMPRESSED_COLOR;

/* internal */
typedef C2StreamParam<C2Info, C2BoolValue, kExynosParamIndexHdrStaticValidation> C2ExynosStreamHdrStaticValidationInfo;
constexpr char C2EXYNOS_PARAMKEY_HDR_STATIC_VALIDATION_INFO[] = "sec-dec-output.hdr-static-validation-info";

typedef C2PortParam<C2Setting, C2Uint32Value, kExynosParamIndexImageConvert> C2ExynosPortImageConvertSetting;
constexpr char C2EXYNOS_PARAMKEY_IMAGE_CONVERT[] = VENDOR_KEY_NAME_IMAGE_CONVERT;

typedef C2PortParam<C2Setting, C2Uint32Value, kExynosParamIndexImageConvertMode> C2ExynosPortImageConvertModeSetting;
constexpr char C2EXYNOS_PARAMKEY_IMAGE_CONVERT_MODE[] = VENDOR_KEY_NAME_IMAGE_CONVERT_MODE;

typedef C2PortParam<C2Setting, C2Uint32Value, kExynosParamIndexImageConvertPixelFormat> C2ExynosPortImageConvertPixelFormatSetting;
constexpr char C2EXYNOS_PARAMKEY_IMAGE_CONVERT_PIXEL_FORMAT[] = VENDOR_KEY_NAME_IMAGE_CONVERT_PIXEL_FORMAT;

typedef C2PortParam<C2Setting, C2Uint32Value, kExynosParamIndexImageConvertControlFrame> C2ExynosPortImageConvertControlFrameSetting;
constexpr char C2EXYNOS_PARAMKEY_IMAGE_CONVERT_CONTROL_FRAME[] = VENDOR_KEY_NAME_IMAGE_CONVERT_CONTROL_FRAME;

/* internal */
typedef C2PortParam<C2Info, C2Uint32Value, kExynosParamIndexExtraBufferNum> C2ExynosPortExtraBufferNumInfo;
constexpr char C2EXYNOS_PARAMKEY_EXTRA_BUFFER_NUM[] = "sec-dec-output.extra-buffer-num";

typedef C2PortParam<C2Setting, C2Uint32Value, kExynosParamIndexAverageQp> C2ExynosPortAverageQpSetting;
typedef C2StreamParam<C2Info, C2Uint32Value, kExynosParamIndexAverageQpInfo> C2ExynosStreamAverageQpInfo;
constexpr char C2EXYNOS_PARAMKEY_AVERAGE_QP[] = VENDOR_KEY_NAME_AVERAGE_QP;

typedef C2StreamParam<C2Setting, C2Uint32Value, kExynosParamIndexSliceSize> C2ExynosStreamSliceSizeSetting;
constexpr char C2EXYNOS_PARAMKEY_SLICE_SIZE[] = VENDOR_KEY_NAME_SLICE_SIZE;

typedef C2PortParam<C2Setting, C2Uint32Value, kExynosParamIndexVTCall> C2ExynosPortVTCallSetting;
constexpr char C2EXYNOS_PARAMKEY_VT_CALL[] = VENDOR_KEY_NAME_VT_CALL;

typedef C2StreamParam<C2Tuning, C2Uint32Value, kExynosParamIndexIFrameRatio> C2ExynosStreamIFrameRatioTunning;
constexpr char C2EXYNOS_PARAMKEY_I_FRAME_RATIO[] = VENDOR_KEY_NAME_I_FRAME_RATIO;

typedef C2StreamParam<C2Setting, VendorC2Config::C2ExynosChromaQpStruct, kExynosParamIndexChromaQpOffset> C2ExynosStreamChromaQpOffsetSetting;
constexpr char C2EXYNOS_PARAMKEY_CHROMA_QP_OFFSET[] = VENDOR_KEY_NAME_CHROMA_QP_OFFSET;

typedef C2StreamParam<C2Setting, C2Uint32Value, kExynosParamIndexEntropyMode> C2ExynosStreamEntropyModeSetting;
constexpr char C2EXYNOS_PARAMKEY_ENTROPY_MODE[] = VENDOR_KEY_NAME_ENTROPY_MODE;

typedef C2StreamParam<C2Tuning, VendorC2Config::C2ExynosPMVStruct, kExynosParamIndexPMV> C2ExynosStreamPMVTuning;
constexpr char C2EXYNOS_PARAMKEY_PMV[] = VENDOR_KEY_NAME_PMV;

typedef C2PortParam<C2Setting, C2Uint32Value, kExynosParamIndexLLWFDMode> C2ExynosPortLLWFDModeSetting;
constexpr char C2EXYNOS_PARAMKEY_LLWFD_MODE[] = VENDOR_KEY_NAME_LLWFD_MODE;

/* internal */
typedef C2StreamParam<C2Tuning, C2EasyBoolValue, kExynosParamIndexStartIDRFrame> C2ExynosStreamStartIDRFrameTuning;
constexpr char C2EXYNOS_PARAMKEY_START_IDR_FRAME[] = "sec-enc-output.start-idr-frame";

/* internal */
typedef C2StreamParam<C2Tuning, C2Uint64Value, kExynosParamIndexInternalUsage> C2ExynosStreamInternalUsageTuning;
constexpr char C2EXYNOS_PARAMKEY_INTERNAL_STREAM_USAGE[] = "sec-enc-internal-input.usage";

typedef C2PortParam<C2Setting, C2Uint32Value, kExynosParamIndexHDR10PlusStatEnc> C2ExynosPortHDR10PlusStatEncSetting;
constexpr char C2EXYNOS_PARAMKEY_HDR10PLUS_STAT_ENC[] = VENDOR_KEY_NAME_HDR10PLUS_STAT_ENC;

/* internal */
typedef C2GlobalParam<C2Info, C2EasyBoolValue, kExynosParamIndex10BitSupport> C2Exynos10BitSupportInfo;
constexpr char C2EXYNOS_PARAMKEY_ENC_INPUT_10BIT_SUPPORT[] = "sec-enc-input.10bit-support";

typedef C2StreamParam<C2Tuning, C2Uint32Value, kExynosParamIndexMaxIFrameSize> C2ExynosStreamMaxIFrameSizeTuning;
constexpr char C2EXYNOS_PARAMKEY_MAX_I_FRAME_SIZE[] = VENDOR_KEY_NAME_MAX_I_FRAME_SIZE;

/* [deprecated]
 * coded.bitrate-mode
 */
typedef C2StreamBitrateModeTuning C2ExynosStreamBitrateModeTuning;
constexpr char C2EXYNOS_PARAMKEY_BITRATE_MODE[] = "coded.bitrate-mode";


/////////////////////////////////////////////////////
//// SKYPE
/////////////////////////////////////////////////////

typedef C2PortParam<C2Info, VendorC2Config::C2ExynosSkypeDriverVersionStruct, kExynosParamIndexSkypeDriverVersion> C2ExynosPortDriverVersionInfo;
constexpr char C2EXYNOS_PARAMKEY_SKYPE_DEC_DRIVER_VERSION[] = VENDOR_KEY_NAME_SKYPE_DEC_DRIVER_VERSION;
constexpr char C2EXYNOS_PARAMKEY_SKYPE_ENC_DRIVER_VERSION[] = VENDOR_KEY_NAME_SKYPE_ENC_DRIVER_VERSION;

typedef C2PortParam<C2Info, VendorC2Config::C2ExynosSkypeMaxTemporalLayerStruct, kExynosParamIndexSkypeEncMaxTemporalLayer> C2ExynosPortMaxTemporalLayerInfo;
constexpr char C2EXYNOS_PARAMKEY_SKYPE_ENC_MAX_TEMPORAL_LAYER[] = VENDOR_KEY_NAME_SKYPE_ENC_MAX_TEMPORAL_LAYER;

typedef C2PortParam<C2Info, VendorC2Config::C2ExynosSkypePreprocessStruct, kExynosParamIndexSkypeEncPreprocess> C2ExynosPortPreprocessInfo;
constexpr char C2EXYNOS_PARAMKEY_SKYPE_ENC_PREPROCESS[] = VENDOR_KEY_NAME_SKYPE_ENC_PREPROCESS;

typedef C2PortParam<C2Info, VendorC2Config::C2ExynosSkypeMaxLTRStruct, kExynosParamIndexSkypeEncMaxLTR> C2ExynosPortMaxLTRInfo;
constexpr char C2EXYNOS_PARAMKEY_SKYPE_ENC_MAX_LTR[] = VENDOR_KEY_NAME_SKYPE_ENC_MAX_LTR;

typedef C2PortParam<C2Setting, VendorC2Config::C2ExynosSkypeLowlatencyStruct, kExynosParamIndexSkypeLowLatency> C2ExynosPortSkypeLowLatencySetting;
constexpr char C2EXYNOS_PARAMKEY_SKYPE_DEC_LOW_LATENCY[] = VENDOR_KEY_NAME_SKYPE_DEC_LOW_LATENCY;
constexpr char C2EXYNOS_PARAMKEY_SKYPE_ENC_LOW_LATENCY[] = VENDOR_KEY_NAME_SKYPE_ENC_LOW_LATENCY;

typedef C2PortParam<C2Setting, VendorC2Config::C2ExynosSkypeInputControlStruct, kExynosParamIndexSkypeEncInputControl> C2ExynosPortInputControlSetting;
constexpr char C2EXYNOS_PARAMKEY_SKYPE_ENC_INPUT_CONTROL[] = VENDOR_KEY_NAME_SKYPE_ENC_INPUT_CONTROL;

typedef C2PortParam<C2Setting, VendorC2Config::C2ExynosSkypeCustomProfileLevelStruct,
                                kExynosParamIndexSkypeEncCustomProfileLevel> C2ExynosPortCustomProfileLevelSetting;
constexpr char C2EXYNOS_PARAMKEY_SKYPE_ENC_CUSTOM_PROFILE_LEVEL[] = VENDOR_KEY_NAME_SKYPE_ENC_CUSTOM_PROFILE_LEVEL;

typedef C2PortParam<C2Setting, VendorC2Config::C2ExynosSkypeLTRCountStruct, kExynosParamIndexSkypeEncLTRCount> C2ExynosPortLTRCountSetting;
constexpr char C2EXYNOS_PARAMKEY_SKYPE_ENC_LTR_COUNT[] = VENDOR_KEY_NAME_SKYPE_ENC_LTR_COUNT;

typedef C2PortParam<C2Tuning, VendorC2Config::C2ExynosSkypeLTRStruct, kExynosParamIndexSkypeEncLTR> C2ExynosPortLTRTuning;
constexpr char C2EXYNOS_PARAMKEY_SKYPE_ENC_LTR[] = VENDOR_KEY_NAME_SKYPE_ENC_LTR;

typedef C2PortParam<C2Setting, C2PictureSizeStruct, kExynosParamIndexSkypeEncSar> C2ExynosPortSarSetting;
constexpr char C2EXYNOS_PARAMKEY_SKYPE_ENC_SAR[] = VENDOR_KEY_NAME_SKYPE_ENC_SAR;

typedef C2PortParam<C2Setting, VendorC2Config::C2ExynosSkypeSliceHeaderSpacingStruct, kExynosParamIndexSkypeEncSliceHeaderSpacing> C2ExynosPortSliceHeaderSpacingSetting;
constexpr char C2EXYNOS_PARAMKEY_SKYPE_ENC_SLICE_HEADER_SPACING[] = VENDOR_KEY_NAME_SKYPE_ENC_SLICE_HEADER_SPACING;

typedef C2PortParam<C2Tuning, C2Int32Value, kExynosParamIndexSkypeEncFrameQP> C2ExynosPortFrameQpTuning;
constexpr char C2EXYNOS_PARAMKEY_SKYPE_ENC_FRAME_QP[] = VENDOR_KEY_NAME_SKYPE_ENC_FRAME_QP;

typedef C2PortParam<C2Tuning, C2Int32Value, kExynosParamIndexSkypeEncBaseLayerPID> C2ExynosPortBaseLayerPidTuning;
constexpr char C2EXYNOS_PARAMKEY_SKYPE_ENC_BASE_LAYER_PID[] = VENDOR_KEY_NAME_SKYPE_ENC_BASE_LAYER_PID;

typedef C2PortParam<C2Setting, VendorC2Config::C2ExynosSkypeBitrateModeStruct,
                                    kExynosParamIndexSkypeEncBitrateMode> C2ExynosPortSkypeBitrateModeSetting;
constexpr char C2EXYNOS_PARAMKEY_SKYPE_ENC_BITRATE_MODE[] = VENDOR_KEY_NAME_SKYPE_ENC_BITRATE_MODE;
//}  // namespace android

#endif // C2_EXYNOS_PARAM_H
