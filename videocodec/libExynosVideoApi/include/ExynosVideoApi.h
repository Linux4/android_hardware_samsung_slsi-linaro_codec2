/*
 *
 * Copyright 2012 Samsung Electronics S.LSI Co. LTD
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

#ifndef _EXYNOS_VIDEO_API_H_
#define _EXYNOS_VIDEO_API_H_

#include "ExynosDef.h"
#include "VendorVideoAPI.h"

#define VIDEO_MAX_PAD_NUM 2
#define VIDEO_INDEX_SRC_PAD 0
#define VIDEO_INDEX_DST_PAD 1

#define VIDEO_BUFFER_MAX_PLANES 3
#define VIDEO_BUFFER_MAX_NUM    32

/* Temporal SVC */
#define VIDEO_MIN_TEMPORAL_LAYERS 1
#define VIDEO_MAX_TEMPORAL_LAYERS 7
#define VIDEO_TEMPORAL_SHORT_TERM (0x1 << 16)

typedef enum _PortType {
    InputPort  = 0,
    OutputPort = 1,
} PortType;

typedef enum _ExynosVideoBoolType {
    VIDEO_FALSE = 0,
    VIDEO_TRUE  = 1,
} ExynosVideoBoolType;

typedef enum _ExynosVideoMemoryType {
    VIDEO_MEMORY_DMABUF     = 0,
    VIDEO_MEMORY_USERPTR    = 1,
    /* VIDEO_MEMORY_MMAP       = 2,*/  /* not used */
} ExynosVideoMemoryType;

typedef enum _ExynosVideoErrorType {
    VIDEO_ERROR_NONE            =  1,
    VIDEO_ERROR_BADPARAM        = -1,
    VIDEO_ERROR_OPENFAIL        = -2,
    VIDEO_ERROR_NOMEM           = -3,
    VIDEO_ERROR_APIFAIL         = -4,
    VIDEO_ERROR_MAPFAIL         = -5,
    VIDEO_ERROR_NOBUFFERS       = -6,
    VIDEO_ERROR_POLL            = -7,
    VIDEO_ERROR_DQBUF_EIO       = -8,
    VIDEO_ERROR_NOSUPPORT       = -9,
    VIDEO_ERROR_HEADERINFO      = -10,
    VIDEO_ERROR_WRONGBUFFERSIZE = -11,
    VIDEO_ERROR_TRY_AGAIN       = -12,
} ExynosVideoErrorType;

typedef enum _ExynosVideoCodingType {
    VIDEO_CODING_UNKNOWN = 0,
    VIDEO_CODING_MPEG2,
    VIDEO_CODING_H263,
    VIDEO_CODING_MPEG4,
    VIDEO_CODING_VC1,
    VIDEO_CODING_VC1_RCV,
    VIDEO_CODING_AVC,
    VIDEO_CODING_MVC,
    VIDEO_CODING_VP8,
    VIDEO_CODING_HEVC,
    VIDEO_CODING_VP9,
    VIDEO_CODING_AV1,
    VIDEO_CODING_XVID,
    VIDEO_CODING_FIMV1,
    VIDEO_CODING_FIMV2,
    VIDEO_CODING_FIMV3,
    VIDEO_CODING_FIMV4,
    VIDEO_CODING_RESERVED,
} ExynosVideoCodingType;

typedef enum _ExynosVideoColorFormatType {
    VIDEO_COLORFORMAT_UNKNOWN = 0,
    VIDEO_COLORFORMAT_NV12,
    VIDEO_COLORFORMAT_NV12_S10B,
    VIDEO_COLORFORMAT_NV12_P010,
    VIDEO_COLORFORMAT_NV12M,
    VIDEO_COLORFORMAT_NV12M_S10B,
    VIDEO_COLORFORMAT_NV12M_P010,
    VIDEO_COLORFORMAT_NV21M,
    VIDEO_COLORFORMAT_NV21M_S10B,
    VIDEO_COLORFORMAT_NV21M_P010,
    VIDEO_COLORFORMAT_NV12_TILED,
    VIDEO_COLORFORMAT_NV12M_TILED,
    VIDEO_COLORFORMAT_I420,
    VIDEO_COLORFORMAT_I420M,
    VIDEO_COLORFORMAT_YV12M,
    VIDEO_COLORFORMAT_ARGB8888,
    VIDEO_COLORFORMAT_BGRA8888,
    VIDEO_COLORFORMAT_RGBA8888,

    VIDEO_COLORFORMAT_NV12_DECOMP,
    VIDEO_COLORFORMAT_NV12_P010_DECOMP,

    VIDEO_COLORFORMAT_SBWC_START,
    VIDEO_COLORFORMAT_NV12_SBWC,
    VIDEO_COLORFORMAT_NV12M_SBWC,
    VIDEO_COLORFORMAT_NV12_10B_SBWC,
    VIDEO_COLORFORMAT_NV12M_10B_SBWC,
    VIDEO_COLORFORMAT_NV21M_SBWC,
    VIDEO_COLORFORMAT_NV21M_10B_SBWC,
    VIDEO_COLORFORMAT_NV12_SBWC_L50,
    VIDEO_COLORFORMAT_NV12_SBWC_L75,
    VIDEO_COLORFORMAT_NV12M_SBWC_L50,
    VIDEO_COLORFORMAT_NV12M_SBWC_L75,
    VIDEO_COLORFORMAT_NV12_10B_SBWC_L40,
    VIDEO_COLORFORMAT_NV12_10B_SBWC_L60,
    VIDEO_COLORFORMAT_NV12_10B_SBWC_L80,
    VIDEO_COLORFORMAT_NV12M_10B_SBWC_L40,
    VIDEO_COLORFORMAT_NV12M_10B_SBWC_L60,
    VIDEO_COLORFORMAT_NV12M_10B_SBWC_L80,
    VIDEO_COLORFORMAT_NV12_256_SBWC,
    VIDEO_COLORFORMAT_NV12_10B_256_SBWC,
    /* SBWC v2.7 */
    VIDEO_COLORFORMAT_NV12_32_SBWC_L,
    VIDEO_COLORFORMAT_NV12_64_SBWC_L,
    VIDEO_COLORFORMAT_NV12_10B_32_SBWC_L,
    VIDEO_COLORFORMAT_NV12_10B_64_SBWC_L,
    VIDEO_COLORFORMAT_NV12M_32_SBWC_L,
    VIDEO_COLORFORMAT_NV12M_64_SBWC_L,
    VIDEO_COLORFORMAT_NV12M_10B_32_SBWC_L,
    VIDEO_COLORFORMAT_NV12M_10B_64_SBWC_L,
    VIDEO_COLORFORMAT_MAX,
} ExynosVideoColorFormatType;

typedef enum _ExynosVideoFlagType {
    VIDEO_FLAG_UNUSED           = 0,
    VIDEO_FLAG_CSD              = 0x1 << 0,
    VIDEO_FLAG_EMPTY_DATA       = 0x1 << 1,
    VIDEO_FLAG_EOS              = 0x1 << 2,
    VIDEO_FLAG_REPLICA_DATA     = 0x1 << 3,
} ExynosVideoFlagType;

typedef enum _ExynosVideoFrameType {
    VIDEO_FRAME_UNKNOWN               = 0,
    VIDEO_FRAME_I                     = 0x1 << 0,
    VIDEO_FRAME_P                     = 0x1 << 1,
    VIDEO_FRAME_B                     = 0x1 << 2,
    VIDEO_FRAME_IDR                   = 0x1 << 3,
    VIDEO_FRAME_TYPE_MASK             = 0xF,
    VIDEO_FRAME_SKIPPED               = 0x1 << 4,
    VIDEO_FRAME_CORRUPT               = 0x1 << 5,
    VIDEO_FRAME_WITH_HDR_INFO         = 0x1 << 6,
    VIDEO_FRAME_WITH_BLACK_BAR        = 0x1 << 7,
    VIDEO_FRAME_NEED_ACTUAL_FORMAT    = 0x1 << 8,
    VIDEO_FRAME_CONSUMED_ONLY         = 0x1 << 9,
    VIDEO_FRAME_NEED_ACTUAL_FRAMERATE = 0x1 << 10,
    VIDEO_FRAME_CONCEALMENT           = 0x1 << 11,
    VIDEO_FRAME_WITH_FILM_GRAIN       = 0x1 << 12,
    VIDEO_FRAME_MULTI                 = 0x1 << 13,
    VIDEO_FRAME_WITH_HDR_STAT_INFO    = 0x1 << 14,
} ExynosVideoFrameType;

typedef enum _ExynosVideoFrameStatusType {
    VIDEO_FRAME_STATUS_UNKNOWN = 0,
    VIDEO_FRAME_STATUS_DECODING_ONLY,
    VIDEO_FRAME_STATUS_DISPLAY_DECODING,
    VIDEO_FRAME_STATUS_DISPLAY_ONLY,
    VIDEO_FRAME_STATUS_DECODING_FINISHED,
    VIDEO_FRAME_STATUS_CHANGE_RESOL,
    VIDEO_FRAME_STATUS_ENABLED_S3D,
    VIDEO_FRAME_STATUS_LAST_FRAME,
    VIDEO_FRAME_STATUS_DISPLAY_INTER_RESOL_CHANGE,
} ExynosVideoFrameStatusType;

typedef enum _ExynosVideoFrameSkipMode {
    VIDEO_FRAME_SKIP_DISABLED = 0,
    VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT,
    VIDEO_FRAME_SKIP_MODE_BUF_LIMIT,
} ExynosVideoFrameSkipMode;

typedef enum _ExynosVideoMFCVersion {
    MFC_ERROR   = 0,
    MFC_51      = 0x51,
    MFC_61      = 0x61,
    MFC_65      = 0x65,
    MFC_72      = 0x72,
    MFC_723     = 0x723,
    MFC_77      = 0x77,
    MFC_78      = 0x78,
    MFC_78D     = 0x78D,
    MFC_80      = 0x80,
    MFC_90      = 0x90,
    MFC_92      = 0x92,
    MFC_100     = 0xA0,
    MFC_101     = 0xA01,
    MFC_1010    = 0xA0A0,
    MFC_1011    = 0xA0B0,
    MFC_1020    = 0xA140,
    MFC_1021    = 0x1021,
    MFC_110     = 0x1100,
    MFC_1120    = 0x1120,
    MFC_120     = 0x1200,
    MFC_1220    = 0x1202,
    MFC_130     = 0x1300,
    MFC_140     = 0x14000000,
    MFC_1400    = 0x14000100,
    MFC_1410    = 0x14100000,
    MFC_142     = 0x14020000,
    MFC_1420    = 0x1420,
    MFC_1421    = 0x14020100,
    MFC_150     = 0x1500000C,
    MFC_1501    = 0x1500010C,
    MFC_1506    = 0x15060000,
    MFC_160     = 0x1600000C,
    MFC_1601    = 0x1600010C,
    MFC_170     = 0x1700000C,
} ExynosVideoMFCVersion;

typedef enum _ExynosVideoHEVCVersion {
    HEVC_ERROR   = 0,
    HEVC_10      = 0x10,
} ExynosVideoHEVCVersion;

typedef enum _ExynosVideoSecurityType {
    VIDEO_NORMAL = 0,
    VIDEO_SECURE = 1,
} ExynosVideoSecurityType;

typedef struct _ExynosVideoRect {
    unsigned int nTop;
    unsigned int nLeft;
    unsigned int nWidth;
    unsigned int nHeight;
} ExynosVideoRect;

typedef enum _ExynosVideoPollType {
    POLL_TYPE_NONE  = 0,
    POLL_TYPE_SRC   = 0x1 << 0,
    POLL_TYPE_DST   = 0x1 << 1,
    POLL_TYPE_USER  = 0x1 << 2,
    POLL_TYPE_BOTH  = POLL_TYPE_SRC | POLL_TYPE_DST,
} ExynosVideoPollType;

typedef enum _ExynosFilledDataType {
    DATA_8BIT           = 0x00,
    DATA_10BIT          = 0x01,
    DATA_8BIT_WITH_2BIT = 0x11,
    DATA_8BIT_SBWC      = 0x100,
    DATA_10BIT_SBWC     = 0x101,
} ExynosFilledDataType;

typedef enum _ExynosHdrInfoType {
    HDR_INFO_NO_CHANGES             = 0,
    HDR_INFO_LIGHT                  = 0x1 << 0,  /* SEI(StaticMeta-CTA.861.3) */
    HDR_INFO_LUMINANCE              = 0x1 << 1,  /* SEI(StaticMeta-CTA.861.3), Luminance, Display primariy 0, 1, 2, White point */
    HDR_INFO_MATRIX_COEFF_ONLY      = 0x1 << 2,  /* SPS */
    HDR_INFO_COLOR_ASPECTS          = 0x1 << 3,  /* SPS, MaxtixCoeff + Primaries + Transfer */
    HDR_INFO_RANGE                  = 0x1 << 4,  /* SPS */
    HDR_INFO_DYNAMIC_META           = 0x1 << 6,  /* SEI(DynamicMeta-ST.2094-40) */
} ExynosHdrInfoType;

typedef struct _ExynosVideoPrimaries {
    unsigned short x;
    unsigned short y;
} ExynosVideoPrimaries;

typedef struct _ExynosVideoHdrStatic {
    int max_pic_average_light;
    int max_content_light;
    int max_display_luminance;
    int min_display_luminance;
    ExynosVideoPrimaries red;
    ExynosVideoPrimaries green;
    ExynosVideoPrimaries blue;
    ExynosVideoPrimaries white;
} ExynosVideoHdrStatic;

#define HDR_MAX_SCL 3
#define HDR_MAX_DISTRIBUTION 15
#define HDR_MAX_CURVEANCHORS 15

typedef struct _ExynosVideoHdrWindowInfo {
    unsigned int maxscl[HDR_MAX_SCL];
    unsigned int average_maxrgb;                                                    /* unused */

    unsigned char num_distribution_maxrgb_percentiles;
    unsigned char distribution_maxrgb_percentages[HDR_MAX_DISTRIBUTION];
    unsigned int  distribution_maxrgb_percentiles[HDR_MAX_DISTRIBUTION];

    unsigned int fraction_bright_pixels;                                            /* unused */

    unsigned short  tone_mapping_flag;
    unsigned short  knee_point_x;
    unsigned short  knee_point_y;
    unsigned short  num_bezier_curve_anchors;
    unsigned short  bezier_curve_anchors[HDR_MAX_CURVEANCHORS];

    unsigned char color_saturation_mapping_flag;                                    /* unused */
    unsigned char color_saturation_weight;                                          /* unused */

    /*
      * This field is reserved for ST2094-40 SEI below or the others
      * window_upper_left_corner_x
      * window_upper_left_corner_y
      * window_lower_right_corner_x
      * window_lower_right_corner_y
      * center_of_ellipse_x
      * center_of_ellipse_y
      * rotation_angle
      * semimajor_axis_internal_ellipse
      * semimajor_axis_external_ellipse
      * semiminor_axis_external_ellipse
      * overlap_process_option
      */
     unsigned int reserved[11];
} ExynosVideoHdrWindowInfo;

/* HDR dynamic meta(ST.2094-40) */
#define HDR_MAX_WINDOW 3
typedef struct _ExynosVideoHdrDynamic {
    unsigned int valid;

    unsigned char  itu_t_t35_country_code;
    unsigned short itu_t_t35_terminal_provider_code;
    unsigned short itu_t_t35_terminal_provider_oriented_code;

    unsigned char  application_identifier;
    unsigned short application_version;

    unsigned char num_windows;  /* HDR_MAX_WINDOW(3) */

    unsigned int  targeted_system_display_maximum_luminance;
    unsigned char targeted_system_display_actual_peak_luminance_flag;               /* unused */

    unsigned char num_rows_targeted_system_display_actual_peak_luminance;           /* unused */
    unsigned char num_cols_targeted_system_display_actual_peak_luminance;           /* unused */

    unsigned char mastering_display_actual_peak_luminance_flag;                     /* unused */
    unsigned char num_rows_mastering_display_actual_peak_luminance;                 /* unused */
    unsigned char num_cols_mastering_display_actual_peak_luminance;                 /* unused */

    ExynosVideoHdrWindowInfo window_info[HDR_MAX_WINDOW];

    /*
     * This field is reserved for ST2094-40 SEI below or the others
     * targeted_system_display_actual_peak_luminance[rows][cols]
     * mastering_display_actual_peak_luminance[rows][cols]
     */
    unsigned int reserved[11];
} ExynosVideoHdrDynamic;

typedef struct _ExynosVideoHdrDynamicBlob {
    unsigned int nSize;
    char         pData[1020];
} ExynosVideoHdrDynamicBlob;

typedef struct _ExynosVideoHdrDynamicStat {
    unsigned char hdr10_plus_stat_done;
    unsigned int  hdr10_plus_stat_sei_size;
    unsigned int  hdr10_plus_stat_offset;

    ExynosHdrDynamicStatInfo statistic_info;
} ExynosVideoHdrDynamicStat;

typedef struct _ExynosVideoColorAspects {
    ExynosRangeType         eRangeType;
    ExynosPrimariesType     ePrimariesType;
    ExynosTransferType      eTransferType;
    ExynosMatrixCoeffType   eCoeffType;
} ExynosVideoColorAspects;

typedef struct _ExynosVideoHdrInfo {
    ExynosHdrInfoType        eChangedType;
    ExynosHdrInfoType        eValidType;

    ExynosVideoColorAspects   sColorAspects;
    ExynosVideoHdrStatic      sHdrStatic;
    ExynosVideoHdrDynamic     sHdrDynamic;
    ExynosVideoHdrDynamicBlob sHdrDynamicBlob;
} ExynosVideoHdrInfo;

/* AV1 Film Grain */
#define AV1_FG_LUM_POS_SIZE 14
#define AV1_FG_CHR_POS_SIZE 10
#define AV1_FG_LUM_AR_COEF_SIZE 24
#define AV1_FG_CHR_AR_COEF_SIZE 25

typedef struct _ExynosVideoFilmGrain {
       unsigned char apply_grain;
       unsigned short grain_seed;

       unsigned char update_grain;
       unsigned char film_grain_params_ref_idx;

       unsigned char num_y_points;
       unsigned char point_y_value[AV1_FG_LUM_POS_SIZE];
       char point_y_scaling[AV1_FG_LUM_POS_SIZE];

       char chroma_scaling_from_luma;

       unsigned char num_cb_points;
       unsigned char point_cb_value[AV1_FG_CHR_POS_SIZE];
       char point_cb_scaling[AV1_FG_CHR_POS_SIZE];

       unsigned char num_cr_points;
       unsigned char point_cr_value[AV1_FG_CHR_POS_SIZE];
       char point_cr_scaling[AV1_FG_CHR_POS_SIZE];

       unsigned char grain_scaling_minus_8;

       char ar_coeff_lag;
       char ar_coeffs_y_plus_128[AV1_FG_LUM_AR_COEF_SIZE];
       char ar_coeffs_cb_plus_128[AV1_FG_CHR_AR_COEF_SIZE];
       char ar_coeffs_cr_plus_128[AV1_FG_CHR_AR_COEF_SIZE];
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
} ExynosVideoFilmGrain;

typedef struct _ExynosVideoGeometry {
    unsigned int                 nWidth;
    unsigned int                 nHeight;
    unsigned int                 nStride;
    unsigned int                 nCStride;
    unsigned int                 nSizeImage;
    unsigned int                 nAlignPlaneSize[VIDEO_BUFFER_MAX_PLANES];
    unsigned int                 nPlaneCnt;
    ExynosVideoRect              cropRect;
    ExynosVideoCodingType        eCompressionFormat;
    ExynosVideoColorFormatType   eColorFormat;
    ExynosVideoBoolType          bInterlaced;
    ExynosFilledDataType         eFilledDataType;
    ExynosVideoHdrInfo           HdrInfo;
    ExynosVideoFilmGrain         FilmGrainInfo;
    ExynosVideoHdrDynamicStat    HdrDynamicStatInfo;
} ExynosVideoGeometry;

typedef struct _ReleaseDPB {
    int fd;
    int fd1;
    int fd2;
} ReleaseDPB;

typedef struct _RefDPBInfo {
    int index;
    ReleaseDPB dpbFD[VIDEO_BUFFER_MAX_NUM];
} RefDPBInfo;

typedef struct _ExynosVideoExtraInfo {
    unsigned long long  nTimeStamp;
    unsigned int        nFlags;
    void               *pBuffer;
    void               *pPrivate;

    union {
        struct {
            ExynosVideoFrameStatusType  displayStatus;
            ExynosVideoFrameType        frameType;
            int                         interlacedType;
            RefDPBInfo                  refDPB;
            int                         nPoc;
        } dec;

        struct {
            ExynosVideoFrameType        frameType;
        } enc;
    } specific;
} ExynosVideoExtraInfo;

typedef struct _ExynosVideoPlane {
    void          *addr;
    int            fd;
    unsigned int   allocSize;
    unsigned int   dataSize;
    unsigned long  offset;
} ExynosVideoPlane;

typedef struct _ExynosVideoBuffer {
    int                     nPlaneCnt;
    ExynosVideoPlane        planes[VIDEO_BUFFER_MAX_PLANES];
    ExynosVideoExtraInfo    extraInfo;
} ExynosVideoBuffer;

typedef struct _ExynosVideoFramePacking {
    int           available;
    unsigned int  arrangement_id;
    int           arrangement_cancel_flag;
    unsigned char arrangement_type;
    int           quincunx_sampling_flag;
    unsigned char content_interpretation_type;
    int           spatial_flipping_flag;
    int           frame0_flipped_flag;
    int           field_views_flag;
    int           current_frame_is_frame0_flag;
    unsigned char frame0_grid_pos_x;
    unsigned char frame0_grid_pos_y;
    unsigned char frame1_grid_pos_x;
    unsigned char frame1_grid_pos_y;
} ExynosVideoFramePacking;

typedef struct _ExynosVideoQPRange {
    int QpMin_I;
    int QpMax_I;
    int QpMin_P;
    int QpMax_P;
    int QpMin_B;
    int QpMax_B;
} ExynosVideoQPRange;

typedef enum _ExynosVideoHierarchicalType {
    VIDEO_HIERARCHICAL_P = 0,
    VIDEO_HIERARCHICAL_B,
} ExynosVideoHierarchicalType;

/* for Temporal SVC */
typedef struct _TemporalLayerBuffer {
    /* In case of H.264 codec,
     * nTemporalLayerCount contains the following data format.
     * --------------------------------------------------------------------
     * | Temporal SVC Coding Style (16bit) | Temporal Layer Count (16bit) |
     * --------------------------------------------------------------------
     * - Temporal SVC Coding Style Value
     *   0 : default
     *   1 : custom Temporal SVC mode(for MCD)
     *
     * - Temporal Layer Count Value
     *   0       : Temporal SVC is disabled
     *   1 ~ MAX : Temporal SVC is enabled
     */
    unsigned int nCount;
    unsigned int nBitrateRatio[VIDEO_MAX_TEMPORAL_LAYERS];
} TemporalLayerBuffer;

/* for Roi Info */
typedef struct _RoiInfoBuffer {
    unsigned long long     pRoiMBInfo; /* For 32/64 bit compatibility */
    int                    nRoiMBInfoSize;
    int                    nUpperQpOffset;
    int                    nLowerQpOffset;
    ExynosVideoBoolType    bUseRoiInfo;
} RoiInfoBuffer;

/* for Chroma QP Offset */
typedef struct _ChromaQPOffset {
    int Cr;
    int Cb;
} ChromaQPOffset;

/* for PMV */
typedef struct _ExynosVideoPMV {
    int mode;  /* 0:disable, 1:local, 2:global */
    /* in case of global, must set followings */
    int hor_L0;
    int hor_L1;
    int ver_L0;
    int ver_L1;
} ExynosVideoPMV;

typedef struct _ExynosVideoEncCommonParam {
    int SourceWidth;                    /* [IN] width of video to be encoded */
    int SourceHeight;                   /* [IN] height of video to be encoded */
    ExynosVideoFrameSkipMode FrameSkip; /* [IN] frame skip mode */
    int IDRPeriod;                      /* [IN] GOP number(interval of I-frame) */
    int SliceMode;                      /* [IN] Multi slice mode */
    int RandomIntraMBRefresh;           /* [IN] cyclic intra refresh */
    int EnableFRMRateControl;           /* [IN] frame based rate control enable */
    int EnableMBRateControl;            /* [IN] Enable macroblock-level rate control */
    int Bitrate;                        /* [IN] rate control parameter(bit rate) */
    int FrameQp;                        /* [IN] The quantization parameter of the frame */
    int FrameQp_P;                      /* [IN] The quantization parameter of the P frame */
    int EnableFRMQpControl;             /* [IN] Enable quantization parameter per frame */
    ExynosVideoQPRange QpRange;         /* [IN] Quantization range */
    int CBRPeriodRf;                    /* [IN] Reaction coefficient parameter for rate control */
    int PadControlOn;                   /* [IN] Enable padding control */
    int LumaPadVal;                     /* [IN] Luma pel value used to fill padding area */
    int CbPadVal;                       /* [IN] CB pel value used to fill padding area */
    int CrPadVal;                       /* [IN] CR pel value used to fill padding area */
    int FrameMap;                       /* [IN] Encoding input mode(tile mode or linear mode) */
    ExynosVideoBoolType PerceptualMode; /* [IN] Enable Perceptual video coding */
    ExynosVideoBoolType bFixedSlice;    /* [IN] Whether Slice(MBs or bytes) size is fixed or not */
    ExynosVideoBoolType bDropControl;
    ExynosVideoPMV PMV;                 /* [IN] PMV */
}ExynosVideoEncCommonParam;

typedef struct _ExynosVideoEncH264Param {
    /* H.264 specific parameters */
    int ProfileIDC;                     /* [IN] profile */
    int LevelIDC;                       /* [IN] level */
    int FrameQp_B;                      /* [IN] The quantization parameter of the B frame */
    int FrameRate;                      /* [IN] rate control parameter(frame rate) */
    int SliceArgument;                  /* [IN] MB number or byte number */
    int NumberBFrames;                  /* [IN] The number of consecutive B frame inserted */
    int NumberReferenceFrames;          /* [IN] The number of reference pictures used */
    int NumberRefForPframes;            /* [IN] The number of reference pictures used for encoding P pictures */
    int LoopFilterDisable;              /* [IN] disable the loop filter */
    int LoopFilterAlphaC0Offset;        /* [IN] Alpha & C0 offset for H.264 loop filter */
    int LoopFilterBetaOffset;           /* [IN] Beta offset for H.264 loop filter */
    int SymbolMode;                     /* [IN] The mode of entropy coding(CABAC, CAVLC) */
    int PictureInterlace;               /* [IN] Enables the interlace mode */
    int Transform8x8Mode;               /* [IN] Allow 8x8 transform(This is allowed only for high profile) */
    int DarkDisable;                    /* [IN] Disable adaptive rate control on dark region */
    int SmoothDisable;                  /* [IN] Disable adaptive rate control on smooth region */
    int StaticDisable;                  /* [IN] Disable adaptive rate control on static region */
    int ActivityDisable;                /* [IN] Disable adaptive rate control on high activity region */
    TemporalLayerBuffer TemporalSVC;    /* [IN] Temporal SVC */
    int MaxTemporalLayerCount;          /* [IN] Max Temporal Layer count */
    ExynosVideoHierarchicalType HierarType;  /* [IN] Hierarchal P & B */
    int VuiRestrictionEnable;           /* [IN] Num Reorder Frames 0 enable */
    int HeaderWithIFrame;               /* [IN] Header With I-Frame 0:disable, 1:enable*/
    int SarEnable;                      /* [IN] SarEnable */
    int SarIndex;                       /* [IN] SarIndex */
    int SarWidth;                       /* [IN] SarWidth */
    int SarHeight;                      /* [IN] SarHeight */
    int LTRFrames;                      /* [IN] LTR frames */
    int ROIEnable;                      /* [IN] ROIEnable */
    ChromaQPOffset chromaQPOffset;      /* [IN] Chroma QP Offset */
} ExynosVideoEncH264Param;

typedef struct _ExynosVideoEncMpeg4Param {
    /* MPEG4 specific parameters */
    int ProfileIDC;                     /* [IN] profile */
    int LevelIDC;                       /* [IN] level */
    int FrameQp_B;                      /* [IN] The quantization parameter of the B frame */
    int TimeIncreamentRes;              /* [IN] frame rate */
    int VopTimeIncreament;              /* [IN] frame rate */
    int SliceArgument;                  /* [IN] MB number or byte number */
    int NumberBFrames;                  /* [IN] The number of consecutive B frame inserted */
    int DisableQpelME;                  /* [IN] disable quarter-pixel motion estimation */
} ExynosVideoEncMpeg4Param;

typedef struct _ExynosVideoEncH263Param {
    /* H.263 specific parameters */
    int FrameRate;                      /* [IN] rate control parameter(frame rate) */
} ExynosVideoEncH263Param;

typedef struct _ExynosVideoEncVpXParam {
    /* VPX specific parameters */
    int FrameRate;                    /* [IN] rate control parameter(frame rate) */
    int VpXGoldenFrameSel;            /* [IN] indication of golden frame */
    int VpXGFRefreshPeriod;           /* [IN] refresh period of golden frame */
    int RefNumberForPFrame;           /* [IN] number of refernce picture for p frame */
    TemporalLayerBuffer TemporalSVC;  /* [IN] Temporal SVC */
} ExynosVideoEncVpXParam;

typedef struct _ExynosVideoEncVp8Param {
    /* Common Param */
    ExynosVideoEncVpXParam VpX;

    /* VP8 specific parameters */
    int Vp8Version;                   /* [IN] vp8 version */
    int Vp8NumberOfPartitions;        /* [IN] number of partitions */
    int Vp8FilterLevel;               /* [IN] filter level */
    int Vp8FilterSharpness;           /* [IN] filter sharpness */
    int DisableIntraMd4x4;            /* [IN] prevent intra 4x4 mode */
} ExynosVideoEncVp8Param;

typedef struct _ExynosVideoEncHevcParam {
    /* HEVC specific parameters */
    int ProfileIDC;                   /* [IN] profile */
    int TierIDC;                      /* [IN] tier flag(MAIN, HIGH) */
    int LevelIDC;                     /* [IN] level */
    int FrameQp_B;                    /* [IN] The quantization parameter of the B frame */
    int FrameRate;                    /* [IN] rate control parameter(frame rate) */
    int MaxPartitionDepth;            /* [IN] Max partition depth */
    int NumberBFrames;                /* [IN] The number of consecutive B frame inserted */
    int NumberReferenceFrames;        /* [IN] The number of reference pictures used */
    int NumberRefForPframes;          /* [IN] The number of reference pictures used for encoding P pictures */
    int LoopFilterDisable;            /* [IN] disable the loop filter */
    int LoopFilterSliceFlag;          /* [IN] in loop filter, select across or not slice boundary */
    int LoopFilterTcOffset;           /* [IN] TC offset for HEVC loop filter */
    int LoopFilterBetaOffset;         /* [IN] Beta offset for HEVC loop filter */
    int LongtermRefEnable;            /* [IN] long term reference enable */
    int LongtermUserRef;              /* [IN] use long term reference index (0 or 1) */
    int LongtermStoreRef;             /* [IN] use long term frame index (0 or 1) */
    int DarkDisable;                  /* [IN] Disable adaptive rate control on dark region */
    int SmoothDisable;                /* [IN] Disable adaptive rate control on smooth region */
    int StaticDisable;                /* [IN] Disable adaptive rate control on static region */
    int ActivityDisable;              /* [IN] Disable adaptive rate control on high activity region */
    TemporalLayerBuffer TemporalSVC;  /* [IN] Temporal SVC */
    int MaxTemporalLayerCount;        /* [IN] Max Temporal Layer count */
    int ROIEnable;                    /* [IN] ROIEnable */
    ExynosVideoBoolType GPBEnable;    /* [IN] GPBEnable */
    ExynosVideoHierarchicalType HierarType;  /* [IN] Hierarchal P & B */
    ChromaQPOffset chromaQPOffset;    /* [IN] Chroma QP Offset */
} ExynosVideoEncHevcParam;

typedef struct _ExynosVideoEncVp9Param {
    /* Common Param */
    ExynosVideoEncVpXParam VpX;

    /* VP9 specific parameters */
    int Vp9Profile;                   /* [IN] vp9 profile */
    int Vp9Level;                     /* [IN] vp9 level */
    int MaxPartitionDepth;            /* [IN] Max partition depth */
} ExynosVideoEncVp9Param;

typedef union _ExynosVideoEncCodecParam {
    ExynosVideoEncH264Param     h264;
    ExynosVideoEncMpeg4Param    mpeg4;
    ExynosVideoEncH263Param     h263;
    ExynosVideoEncVp8Param      vp8;
    ExynosVideoEncHevcParam     hevc;
    ExynosVideoEncVp9Param      vp9;
} ExynosVideoEncCodecParam;

typedef struct _ExynosVideoEncParam {
    ExynosVideoEncCommonParam   common;
    ExynosVideoEncCodecParam    codec;
} ExynosVideoEncParam;

typedef struct _ExynosVideoDecSupportInfo {
    ExynosVideoBoolType bSkypeSupport;          /* H.264 only */
    ExynosVideoBoolType bHDRDynamicInfoSupport; /* HEVC */
    ExynosVideoBoolType bHDRDynamicBlobSupport; /* HEVC AV1*/
    ExynosVideoBoolType bDrvDPBManageSupport;
    ExynosVideoBoolType bDisplayDelayInfoSupport;
    ExynosVideoBoolType bPocSupport;
    ExynosVideoBoolType bFrameErrorTypeSupport;
    ExynosVideoBoolType bOperatingRateSupport;
    ExynosVideoBoolType bRealTimePrioritySupport;
} ExynosVideoDecSupportInfo;

typedef struct _ExynosVideoEncSupportInfo {
    int                 nSpareSize;                     /* required extra size */
    ExynosVideoBoolType bTemporalSvcSupport;            /* H.264, HEVC, VP8, VP9 */
    ExynosVideoBoolType bSkypeSupport;                  /* H.264 only */
    ExynosVideoBoolType bRoiInfoSupport;                /* H.264, HEVC */
    ExynosVideoBoolType bQpRangePBSupport;
    ExynosVideoBoolType bFixedSliceSupport;             /* H.264 only */
    ExynosVideoBoolType bPVCSupport;                    /* H.264, HEVC, VP8, VP9 */
    ExynosVideoBoolType bIFrameRatioSupport;            /* H.264, HEVC */
    ExynosVideoBoolType bColorAspectsSupport;           /* H.264, HEVC, VP9 */
    ExynosVideoBoolType bAdaptiveLayerBitrateSupport;   /* H.264, HEVC, VP8, VP9 */
    ExynosVideoBoolType bHDRStaticInfoSupport;          /* HEVC */
    ExynosVideoBoolType bHDRDynamicInfoSupport;         /* HEVC */
    ExynosVideoBoolType bDropControlSupport;
    ExynosVideoBoolType bChromaQpSupport;               /* H.264, HEVC */
    ExynosVideoBoolType bGDCvOTFSupport;
    ExynosVideoBoolType bOperatingRateSupport;
    ExynosVideoBoolType bRealTimePrioritySupport;
    ExynosVideoBoolType bAverageQpSupport;
    ExynosVideoBoolType bPMVSupport;
    ExynosVideoBoolType bGopModeSupport;
    ExynosVideoBoolType bHDRDynamicStatInfoSupport;     /* HEVC */
    ExynosVideoBoolType bMaxIFrameSizeSupport;
} ExynosVideoEncSupportInfo;

typedef struct _ExynosVideoInstInfo {
    ExynosVideoBoolType        bInitialized;
    ExynosVideoCodingType      eCodecType;
    ExynosVideoSecurityType    eSecurityType;
    ExynosVideoBoolType        bOTFMode;
    ExynosVideoBoolType        bVideoBufFlagCtrl;

    unsigned int               nWidth;
    unsigned int               nHeight;
    unsigned int               nBitrate;
    unsigned int               nFramerate;

    ExynosVideoMemoryType      nMemoryType;

    int                        HwVersion;
    unsigned int               SwVersion;

    ExynosVideoColorFormatType supportFormat[VIDEO_COLORFORMAT_MAX];

    union {
        ExynosVideoDecSupportInfo dec;
        ExynosVideoEncSupportInfo enc;
    }                          supportInfo;
} ExynosVideoInstInfo;

typedef struct _ExynosVideoDecInfo {
    /* DPB ref. handling on user */
    int   nRefDPBFD;
    void *pRefDPBAddr;

    /* HDR dynamic info */
    int   nHDRDynamicInfoFD;
    void *pHDRDynamicInfoAddr;

    /* Film Grain */
    int   nFilmGrainFD;
    void *pFilmGrainAddr;
} ExynosVideoDecInfo;

typedef struct _ExynosVideoEncInfo {
    /* Temporal SVC */
    int   nTemporalLayerFD;
    void *pTemporalLayerAddr;

    /* ROI */
    int   nROIFD;
    void *pROIAddr;

    /* HDR dynamic info */
    int   nHDRDynamicInfoFD;
    void *pHDRDynamicInfoAddr;

    /* HDR dynamic statistic info */
    int   nHDRDynamicStatInfoFD;
    void *pHDRDynamicStatInfoAddr;
    ExynosVideoBoolType bHDR10PlusStatMode;

    /* adpative frame rate */
    int                     oldFrameRate;
    signed long long        oldTimeStamp;
    signed long long        oldDuration;

    /* disable dynamic framerate change */
    ExynosVideoBoolType bDisableDFR;

    /* format changed encoding */
    ExynosVideoColorFormatType actualFormat;

    ExynosVideoBoolType bEnableGDCvOTF;
    ExynosVideoBoolType bUseGDCvOTF;
} ExynosVideoEncInfo;

typedef struct _ExynosVideoSlotInfo {
    ExynosVideoBuffer    buffer;
    ExynosVideoBoolType  bQueued;

    /* in case of decoder, slot could be used two times due to referenced DPB */
    ExynosVideoBoolType  bSlotUsed;
    int                  nUsedCnt;
} ExynosVideoSlotInfo;

typedef struct _ExynosVideoPadInfo {
    ExynosVideoSlotInfo     slot[VIDEO_BUFFER_MAX_NUM];
    ExynosVideoGeometry     geometry;
    int                     nBufNum;
    int                     nPlane;
    ExynosVideoBoolType     bStreamOn;
    void                   *pMutex;
    void                   *hPoll;  /* poll event */
} ExynosVideoPadInfo;

typedef struct _ExynosVideoContext {
    int hDevice;    /* H/W device */
    int hUser;      /* user poll event */
    int hIONHandle; /* shared memory */

    ExynosVideoPadInfo      pad[VIDEO_MAX_PAD_NUM];  /* SRC & DST */

    ExynosVideoInstInfo     instInfo;  /* common information on instance */

    union ExynosVideoSpecificInfo {
        ExynosVideoDecInfo dec;
        ExynosVideoEncInfo enc;
    }                       specificInfo;
} ExynosVideoContext;

typedef struct _ExynosVideoDecOps {
    void *                (*Init)(ExynosVideoInstInfo *pVideoInfo);
    ExynosVideoErrorType  (*Finalize)(void *pHandle);

    /* identifying a frame */
    ExynosVideoErrorType  (*Set_FrameTag)(void *pHandle, int nFrameTag);
    int                   (*Get_FrameTag)(void *pHandle);

    int                   (*Get_ActualBufferCount)(void *pHandle);
    int                   (*Get_DisplayDelay)(void *pHandle);
    ExynosVideoErrorType  (*Set_DisplayDelay)(void *pHandle, int nDelay);
    ExynosVideoErrorType  (*Set_IFrameDecoding)(void *pHandle);  /* it is only valid on I frame. it is similar as display delay(0) */

    ExynosVideoErrorType  (*Enable_PackedPB)(void *pHandle);
    ExynosVideoErrorType  (*Enable_LoopFilter)(void *pHandle);
    ExynosVideoErrorType  (*Enable_SliceMode)(void *pHandle);
    ExynosVideoErrorType  (*Enable_DiscardRcvHeader)(void *pHandle);

    /* informing that timestamp is consist of DTS(Decoding Time Stamp). normally, it is PTS(Presentation Time Stamp) */
    ExynosVideoErrorType  (*Enable_DTSMode)(void *pHandle);

    /* Quailty Of Service */
    ExynosVideoErrorType  (*Set_QosRatio)(void *pHandle, int nRatio);

    /* Stereoscopic 3D */
    ExynosVideoErrorType  (*Enable_SEIParsing)(void *pHandle);
    ExynosVideoErrorType  (*Get_FramePackingInfo)(void *pHandle, ExynosVideoFramePacking *pFramepacking);

    /* High Dynamic Range */
    ExynosVideoErrorType  (*Get_HDRInfo)(void *pHandle, ExynosVideoHdrInfo *pHdrInfo);

    /* black bar */
    ExynosVideoErrorType  (*Set_SearchBlackBar)(void *pHandle, ExynosVideoBoolType bUse);
    ExynosVideoErrorType  (*Get_BlackBarCrop)(void *pHandle, ExynosVideoRect *pBufferCrop);

    int                   (*Get_ActualFormat)(void *pHandle);

    int                   (*Get_ActualFramerate)(void *pHandle);

    ExynosVideoErrorType  (*Enable_DecodeOrder)(void *pHandle);

    /* Film Grain */
    ExynosVideoErrorType  (*Get_FilmGrainInfo)(void *pHandle, ExynosVideoFilmGrain *pFilmGrainInfo);

    ExynosVideoErrorType  (*Set_OperatingRate)(void *pHandle, unsigned int framerate);
    ExynosVideoErrorType  (*Set_RealTimePriority)(void *pHandle, unsigned int RealTimePriority);
    ExynosVideoErrorType  (*Get_RealTimePriority)(void *pHandle);

    ExynosVideoErrorType  (*Disable_LazyUnmap)(void *pHandle);

    ExynosVideoErrorType  (*Stop_Wait_Buffer)(void *pHandle);
    ExynosVideoErrorType  (*Reset_Wait_Buffer)(void *pHandle);
} ExynosVideoDecOps;

typedef struct _ExynosVideoEncOps {
    void *               (*Init)(ExynosVideoInstInfo *pVideoInfo);
    ExynosVideoErrorType (*Finalize)(void *pHandle);

    /* identifying a frame */
    ExynosVideoErrorType (*Set_FrameTag)(void *pHandle, int nFrameTag);
    int                  (*Get_FrameTag)(void *pHandle);

    ExynosVideoErrorType (*Set_EncParam)(void *pHandle, ExynosVideoEncParam *pEncParam);

    ExynosVideoErrorType (*Set_SliceMode)(void *pHandle, int nSliceMode, int nSliceArgument);
    ExynosVideoErrorType (*Enable_PrependSpsPpsToIdr)(void *pHandle);

    ExynosVideoErrorType (*Set_FrameType)(void *pHandle, ExynosVideoFrameType eFrameType);
    ExynosVideoErrorType (*Set_FrameSkip)(void *pHandle, int nFrameSkip);

    ExynosVideoErrorType (*Set_FrameRate)(void *pHandle, int nFramerate);
    ExynosVideoErrorType (*Set_BitRate)(void *pHandle, int nBitrate);
    ExynosVideoErrorType (*Set_QpRange)(void *pHandle, ExynosVideoQPRange qpRange);
    ExynosVideoErrorType (*Set_IDRPeriod)(void *pHandle, int nPeriod);

    /* Quailty Of Service */
    ExynosVideoErrorType (*Set_QosRatio)(void *pHandle, int nRatio);

    /* skype */
    ExynosVideoErrorType (*Set_DynamicQpControl)(void *pHandle, int nQp);
    ExynosVideoErrorType (*Set_MarkLTRFrame)(void *pHandle, int nLongTermFrmIdx);
    ExynosVideoErrorType (*Set_UsedLTRFrame)(void *pHandle, int nUsedLTRFrameNum);
    ExynosVideoErrorType (*Set_BasePID)(void *pHandle, int nPID);

    /* Temporal Layer Scalable Video Coding */
    ExynosVideoErrorType (*Set_LayerChange)(void *pHandle, TemporalLayerBuffer TemporalSVC);
    ExynosVideoErrorType (*Enable_AdaptiveLayerBitrate)(void *pHandle, int bEnable);

    /* High Dynamic Range */
    ExynosVideoErrorType (*Set_ColorAspects)(void *pHandle, ExynosVideoColorAspects *pColorAspects);
    ExynosVideoErrorType (*Set_HDRStaticInfo)(void *pHandle, ExynosVideoHdrStatic *pHDRStaticInfo);
    ExynosVideoErrorType (*Set_HDRDynamicInfo)(void *pHandle, ExynosVideoHdrDynamic *pHDRDynamicInfo);

    /* Region Of Interest Coding */
    ExynosVideoErrorType (*Set_RoiInfo)(void *pHandle, RoiInfoBuffer *pRoiInfo);

    /* Weighted Prediction Coding */
    ExynosVideoErrorType (*Enable_WeightedPrediction)(void *pHandle);
    ExynosVideoErrorType (*Set_YSumData)(void *pHandle, unsigned int nHighData, unsigned int nLowData);

    /* ratio of I-frame to P-frame */
    ExynosVideoErrorType (*Set_IFrameRatio)(void *pHandle, int nRatio);

    ExynosVideoErrorType (*Set_HeaderMode)(void *pHandle, ExynosVideoBoolType bSeparated);

    /* generates bitstream on considering variable FPS */
    ExynosVideoErrorType (*Set_DropControl)(void *pHandle, ExynosVideoBoolType bEnable);

    ExynosVideoErrorType (*Disable_DynamicFrameRate)(void *pHandle);

    ExynosVideoErrorType (*Set_ActualFormat)(void *pHandle, int nFormat);

    ExynosVideoErrorType (*Enable_GDCvOTF)(void *pHandle, int nType);
    ExynosVideoErrorType (*Set_GDCvOTF)(void *pHandle, ExynosVideoBoolType bEnable);

    ExynosVideoErrorType (*Set_OperatingRate)(void *pHandle, unsigned int framerate);
    ExynosVideoErrorType (*Set_RealTimePriority)(void *pHandle, unsigned int RealTimePriority);
    ExynosVideoErrorType (*Get_RealTimePriority)(void *pHandle);

    ExynosVideoErrorType (*Get_AverageQp)(void *pHandle, int *pQpValue);

    ExynosVideoErrorType (*Set_PMV)(void *pHAndle, ExynosVideoPMV *pPMV);

    ExynosVideoErrorType (*Set_GopMode)(void *pHandle, ExynosVideoFrameType mode);

    ExynosVideoErrorType (*Set_MinQuality)(void *pHandle, int level);

    ExynosVideoErrorType (*Enable_HDRDynamicStatInfo)(void *pHandle);
    ExynosVideoErrorType (*Get_HDRDynamicStatInfo)(void *pHandle, ExynosVideoHdrDynamicStat *pHDRDynamicInfo);

    ExynosVideoErrorType (*Set_Crop)(void *pHandle, ExynosVideoRect *pBufferCrop);

    ExynosVideoErrorType (*Set_MaxIFrameSize)(void *pHandle, int size);

    ExynosVideoErrorType (*Disable_LazyUnmap)(void *pHandle);

    ExynosVideoErrorType (*Stop_Wait_Buffer)(void *pHandle);
    ExynosVideoErrorType (*Reset_Wait_Buffer)(void *pHandle);
} ExynosVideoEncOps;

typedef struct _ExynosVideoBufferOps {
    ExynosVideoErrorType  (*Enable_Cacheable)(void *pHandle);

    ExynosVideoErrorType  (*Set_Geometry)(void *pHandle, ExynosVideoGeometry *pBufferConf);
    ExynosVideoErrorType  (*Get_Geometry)(void *pHandle, ExynosVideoGeometry *pBufferConf);
    ExynosVideoErrorType  (*Try_Geometry)(void *pHandle, ExynosVideoGeometry *pBufferConf);

    ExynosVideoErrorType  (*Setup)(void *pHandle, unsigned int nBufferCount);

    ExynosVideoErrorType  (*Run)(void *pHandle);
    ExynosVideoErrorType  (*Stop)(void *pHandle);

    ExynosVideoErrorType  (*Enqueue)(void *pHandle, ExynosVideoBuffer *pVideoBuffer);
    ExynosVideoErrorType  (*Dequeue)(void *pHandle, ExynosVideoBuffer *pVideoBuffer);
    ExynosVideoErrorType  (*Cleanup_Buffer)(void *pHandle);

    ExynosVideoErrorType  (*Wait_Buffer)(void *pHandle, ExynosVideoPollType *pAvail);
} ExynosVideoBufferOps;

#ifdef __cplusplus
extern "C" {
#endif

ExynosVideoErrorType Exynos_Video_GetInstInfo(
    ExynosVideoInstInfo *pVideoInstInfo, ExynosVideoBoolType bIsDec);

ExynosVideoErrorType Exynos_Video_Register_Decoder(
    ExynosVideoDecOps    *pDecOps,
    ExynosVideoBufferOps *pInbufOps,
    ExynosVideoBufferOps *pOutbufOps);

ExynosVideoErrorType Exynos_Video_Register_Encoder(
    ExynosVideoEncOps    *pEncOps,
    ExynosVideoBufferOps *pInbufOps,
    ExynosVideoBufferOps *pOutbufOps);

void Exynos_Video_Unregister_Decoder(
    ExynosVideoDecOps    *pDecOps,
    ExynosVideoBufferOps *pInbufOps,
    ExynosVideoBufferOps *pOutbufOps);

void Exynos_Video_Unregister_Encoder(
    ExynosVideoEncOps    *pEncOps,
    ExynosVideoBufferOps *pInbufOps,
    ExynosVideoBufferOps *pOutbufOps);

#ifdef __cplusplus
}
#endif

#endif /* _EXYNOS_VIDEO_API_H_ */
