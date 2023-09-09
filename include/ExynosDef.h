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
#ifndef EXYNOS_DEF_H
#define EXYNOS_DEF_H

#if __has_include(<video/mfc_macros.h>)
#  include <video/mfc_macros.h>
#else
#  include "mfc_macros.h"
#endif

#include "VendorVideoAPI.h"

#ifndef __OMX_EXPORTS
#define __OMX_EXPORTS
#define EXYNOS_EXPORT_REF __attribute__((visibility("default")))
#define EXYNOS_IMPORT_REF __attribute__((visibility("default")))
#endif

#define USE_C2A_VERSION

#define UNUSED(expr)  \
    do {              \
        (void)(expr); \
    } while (0)

#ifdef ALIGN
#undef ALIGN
#endif
#define ALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define MIN(x, y) (((x) < (y))? (x):(y))
#define MAX(x, y) (((x) > (y))? (x):(y))
#define HW_EXTRA_BYTES 256
#define HW_WIDTH_ALIGN 16
#define HW_HEIGHT_ALIGN 16
#ifndef HW_GPU_ALIGN
#define HW_GPU_ALIGN 16
#endif
#ifndef HW_CHROMA_VALIGN
#define HW_CHROMA_VALIGN 1
#endif
#define HW_S10B_FORMAT_8B_ALIGNMENT 16

#define GET_8B_Y_SIZE(w, h) (ALIGN(w, HW_S10B_FORMAT_8B_ALIGNMENT) * ALIGN(h, HW_WIDTH_ALIGN) + 256)
#define GET_8B_UV_SIZE(w, h) (ALIGN(((ALIGN(w, HW_S10B_FORMAT_8B_ALIGNMENT) * ALIGN(h, HW_WIDTH_ALIGN) / 2) + 256), HW_WIDTH_ALIGN))
#define GET_8B_CB_SIZE(w, h) (ALIGN(((ALIGN(w / 2, HW_S10B_FORMAT_8B_ALIGNMENT) * ALIGN(h, HW_WIDTH_ALIGN) / 2) + 256), HW_WIDTH_ALIGN))
#define GET_8B_CR_SIZE(w, h) (ALIGN(((ALIGN(w / 2, HW_S10B_FORMAT_8B_ALIGNMENT) * ALIGN(h, HW_WIDTH_ALIGN) / 2) + 256), HW_WIDTH_ALIGN))
#define GET_2B_SIZE(w, h) (ALIGN(w / 4, HW_WIDTH_ALIGN) * ALIGN(h, HW_WIDTH_ALIGN) + 64)
#define GET_10B_Y_SIZE(w, h) (GET_8B_Y_SIZE(w, h) + GET_2B_SIZE(w, h))
#define GET_10B_UV_SIZE(w, h) (GET_8B_UV_SIZE(w, h) + GET_2B_SIZE(w, h / 2))
#define GET_10B_CB_SIZE(w, h) (GET_8B_CB_SIZE(w, h) + GET_2B_SIZE(w, h / 2))
#define GET_10B_CR_SIZE(w, h) (GET_8B_CR_SIZE(w, h) + GET_2B_SIZE(w, h / 2))

#define GET_10B_UV_OFFSET(w, h) GET_10B_Y_SIZE(w, h)
#define GET_10B_CB_OFFSET(w, h) GET_10B_Y_SIZE(w, h)
#define GET_10B_CR_OFFSET(w, h) (GET_10B_Y_SIZE(w, h) + GET_10B_CB_SIZE(w, h))

#define USE_BUFFERQUEUE  /* for output buffer instead of pool */

#define DEFAULT_NUM_INPUT_BUFFER 4
#define DEFAULT_NUM_OUTPUT_BUFFER 4

#define EXTRA_INTERNAL_BUFFER_NUM 3

#define BASE_BUFFER_MAX_PLANES 3

#define MAX_TEMPORAL_LAYERS 7
#define MAX_TEMPORAL_B_LAYERS 5
#define MAX_VPX_TEMPORAL_LAYERS 3

typedef uint32_t ONOFF;
enum : ONOFF {
    Off = 0,
    On,
};

enum ExynosErrorType : int32_t {
    EXYNOS_ERROR_NONE = 0,
    EXYNOS_ERROR_TRY_AGAIN,
    EXYNOS_ERROR_TIMED_OUT,
    EXYNOS_ERROR_INVALID_PARAM,
    EXYNOS_ERROR_BAD_STATE,
    EXYNOS_ERROR_CORRUPTED,
    EXYNOS_ERROR_RESOURCE,
    EXYNOS_ERROR_CHANGE_RESOLUTION,
    EXYNOS_ERROR_NO_BUFFER_PROCESS, /* low latency WFD does not need buffer process */
    EXYNOS_ERROR_UNKNOWN,
    EXYNOS_ERROR_NOT_SUPPORT,
};

enum ExynosDebugType : uint32_t {
    EXYNOS_DEBUG_NONE           = 0,
    EXYNOS_DEBUG_INPUT          = 0x1 << 0,
    EXYNOS_DEBUG_OUTPUT         = 0x1 << 1,
    EXYNOS_DEBUG_ALL            = (EXYNOS_DEBUG_INPUT | EXYNOS_DEBUG_OUTPUT),

    EXYNOS_DEBUG_FILE_MASK      = 0x1 << 4,
    EXYNOS_DEBUG_FILE_WHOLE     = 0x0 << 4,
    EXYNOS_DEBUG_FILE_SPLIT     = 0x1 << 4,
};

enum DataInfo : int32_t {
    UnusedData,
    UsedData,

    NoData,
    ReplicaData,

    SingleData,
    MultiData,

    CorruptedData,
};

typedef uint32_t FrameInfoType;

enum FrameInfo : FrameInfoType {
    UnknownFrame        = 0x0,
    CorruptedFrame      = 0x1,

    CodecSpecificData   = 0x1 << 1,
    IDRframe            = 0x1 << 2,
    Iframe              = 0x1 << 3,
    Pframe              = 0x1 << 4,
    Bframe              = 0x1 << 5,
    FRAME_INFO_MASK     = 0b111110,

    InterlacedFrame     = 0x1 << 6,

    EndOfStream         = 0x1 << 7,
};

typedef struct CropInfo {
    uint32_t nLeft;
    uint32_t nTop;
    uint32_t nWidth;
    uint32_t nHeight;
} CropInfo;

typedef enum _ExynosRangeType {
    RANGE_UNSPECIFIED   = 0,
    RANGE_FULL          = 1,
    RANGE_LIMITED       = 2,
} ExynosRangeType;

typedef enum _ExynosPrimariesType {
    PRIMARIES_RESERVED      = 0,
    PRIMARIES_BT709_5       = 1,
    PRIMARIES_UNSPECIFIED   = 2,
    PRIMARIES_BT470_6M      = 4,
    PRIMARIES_BT601_6_625   = 5,
    PRIMARIES_BT601_6_525   = 6,
    PRIMARIES_SMPTE_240M    = 7,
    PRIMARIES_GENERIC_FILM  = 8,
    PRIMARIES_BT2020        = 9,
} ExynosPrimariesType;

typedef enum _ExynosTransferType {
    TRANSFER_RESERVED       = 0,
    TRANSFER_BT709          = 1,
    TRANSFER_UNSPECIFIED    = 2,
    TRANSFER_GAMMA_22       = 4, /* Assumed display gamma 2.2 */
    TRANSFER_GAMMA_28       = 5, /* Assumed display gamma 2.8 */
    TRANSFER_SMPTE_170M     = 6,
    TRANSFER_SMPTE_240M     = 7,
    TRANSFER_LINEAR         = 8,
    TRANSFER_XvYCC          = 11,
    TRANSFER_BT1361         = 12,
    TRANSFER_SRGB           = 13,
    TRANSFER_BT2020_1       = 14, /* functionally the same as the 1, 6, 15 */
    TRANSFER_BT2020_2       = 15, /* functionally the same as the 1, 6, 14 */
    TRANSFER_ST2084         = 16,
    TRANSFER_ST428          = 17,
    TRANSFER_HLG            = 18,
} ExynosTransferType;

typedef enum _ExynosMatrixCoeffType {
    MATRIX_COEFF_IDENTITY        = 0,
    MATRIX_COEFF_REC709          = 1,
    MATRIX_COEFF_UNSPECIFIED     = 2,
    MATRIX_COEFF_RESERVED        = 3,
    MATRIX_COEFF_470_SYSTEM_M    = 4,
    MATRIX_COEFF_470_SYSTEM_BG   = 5,
    MATRIX_COEFF_SMPTE170M       = 6,
    MATRIX_COEFF_SMPTE240M       = 7,
    MATRIX_COEFF_BT2020          = 9,
    MATRIX_COEFF_BT2020_CONSTANT = 10,
} ExynosMatrixCoeffType;

typedef struct HDRInfo {
    int                     eChangedInfo;    /* ExynosVideoInfoType */
    int                     eValidInfo;      /* ExynosVideoInfoType */
    ExynosColorAspects      sColorAspects;
    ExynosHdrStaticInfo     sHdrStaticInfo;
    ExynosHdrDynamicInfo    sHdrDynamicInfo;
    ExynosHdrDynamicBlob    sHdrDynamicBlob;
} HDRInfo;

typedef struct ImageInfo {
    FrameInfoType   eFrameInfo;
    uint32_t        nWidth;
    uint32_t        nHeight;
    uint32_t        nStride;
    uint32_t        nFormat;
    CropInfo        stCropInfo;
    HDRInfo         stHDRInfo;
    uint32_t        nDataSpace;
    uint32_t        nPoc;
    uint64_t        nTimeStamp;
} ImageInfo;

/* it depends on videocodec */
#define FG_LUM_POS_SIZE 14
#define FG_CHR_POS_SIZE 10
#define FG_LUM_AR_COEF_SIZE 24
#define FG_CHR_AR_COEF_SIZE 25

typedef struct FilmGrainInfo {
    unsigned char apply_grain;
    unsigned short grain_seed;

    unsigned char update_grain;
    unsigned char film_grain_params_ref_idx;

    unsigned char num_y_points;
    unsigned char point_y_value[FG_LUM_POS_SIZE];
    char point_y_scaling[FG_LUM_POS_SIZE];

    char chroma_scaling_from_luma;

    unsigned char num_cb_points;
    unsigned char point_cb_value[FG_CHR_POS_SIZE];
    char point_cb_scaling[FG_CHR_POS_SIZE];

    unsigned char num_cr_points;
    unsigned char point_cr_value[FG_CHR_POS_SIZE];
    char point_cr_scaling[FG_CHR_POS_SIZE];

    unsigned char grain_scaling_minus_8;

    char ar_coeff_lag;
    char ar_coeffs_y_plus_128[FG_LUM_AR_COEF_SIZE];
    char ar_coeffs_cb_plus_128[FG_CHR_AR_COEF_SIZE];
    char ar_coeffs_cr_plus_128[FG_CHR_AR_COEF_SIZE];
    unsigned char ar_coeff_shift_minus_6;

    char grain_scale_shift;

    char cb_mult;
    char cb_luma_mult;
    short cb_offset;

    char cr_mult;
    char cr_luma_mult;
    short cr_offset;

    unsigned char overlap_flag;
    unsigned char clip_to_restricted_range;
    unsigned char mc_identity;
} FilmGrainInfo;

#endif // EXYNOS_DEF_H
