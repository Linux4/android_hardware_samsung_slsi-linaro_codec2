/*
 *
 * Copyright 2016 Samsung Electronics S.LSI Co. LTD
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

/*
 * @file    ExynosVideo_OSAL.h
 * @brief   ExynosVideo OSAL define
 * @author  SeungBeom Kim (sbcrux.kim@samsung.com)
 *          Taehwan Kim   (t_h.kim@samsung.com)
 * @version    1.0.0
 * @history
 *   2016.01.07 : Create
 */

#ifndef _EXYNOS_VIDEO_OSAL_H_
#define _EXYNOS_VIDEO_OSAL_H_
#ifdef USE_EPOLL
#include <sys/epoll.h>
#else
#include <sys/poll.h>
#endif

#include "exynos_v4l2.h"
#include "videodev2_exynos_media.h"

#include "exynos_mfc_media.h"

#include <hardware/exynos/ion.h>

#include "ExynosVideoApi.h"

typedef enum _CodecOSAL_FlagType {
    CODEC_OSAL_FLAG_UNUSED                      = 0,

    /* get */
    CODEC_OSAL_FLAG_HDR_DYNAMIC_INFO            = MFC_FLAG_HDR_PLUS,
    CODEC_OSAL_FLAG_HDR_STATIC_INFO             = (MFC_FLAG_HDR_CONTENT_LIGHT | MFC_FLAG_HDR_DISPLAY_COLOUR),
    CODEC_OSAL_FLAG_COLOR_ASPECTS_INFO          = (MFC_FLAG_HDR_MAXTIX_COEFF | MFC_FLAG_HDR_COLOUR_DESC | MFC_FLAG_HDR_VIDEO_SIGNAL_TYPE),
    CODEC_OSAL_FLAG_HDR_INFO                    = (CODEC_OSAL_FLAG_COLOR_ASPECTS_INFO | CODEC_OSAL_FLAG_HDR_STATIC_INFO | CODEC_OSAL_FLAG_HDR_DYNAMIC_INFO),
    CODEC_OSAL_FLAG_BLACK_BAR_CROP_INFO         = MFC_FLAG_BLACKBAR_DETECT,
    CODEC_OSAL_FLAG_UNCOMP_DATA                 = MFC_FLAG_UNCOMP,
    CODEC_OSAL_FLAG_INTER_RESOLUTION_CHANGE     = MFC_FLAG_DISP_RES_CHANGE,
    CODEC_OSAL_FLAG_IDR                         = MFC_FLAG_IDR,
    CODEC_OSAL_FLAG_ACTUAL_FRAMERATE            = MFC_FLAG_FRAMERATE_CH,
    CODEC_OSAL_FLAG_FILM_GRAIN                  = MFC_FLAG_AV1_FILM_GRAIN,
    CODEC_OSAL_FLAG_MULTIFRAME                  = MFC_FLAG_MULTIFRAME,
    CODEC_OSAL_FLAG_CONSUMED_ONLY               = MFC_FLAG_CONSUMED_ONLY,

    /* set */
    CODEC_OSAL_FLAG_GDC_OTF                     = MFC_FLAG_ENC_SRC_VOTF,
    CODEC_OSAL_FLAG_REPLICA_DATA                = MFC_FLAG_ENC_SRC_FAKE,
    CODEC_OSAL_FLAG_UNCOMP_FORMAT               = MFC_FLAG_ENC_SRC_UNCOMP,
    CODEC_OSAL_FLAG_CSD                         = MFC_FLAG_CSD,
    CODEC_OSAL_FLAG_EMPTY_DATA                  = MFC_FLAG_EMPTY_DATA,
    CODEC_OSAL_FLAG_EOS                         = MFC_FLAG_LAST_FRAME,
} CodecOSAL_FlagType;

typedef enum _CodecOSAL_MemType {
    CODEC_OSAL_MEM_TYPE_MMAP    = V4L2_MEMORY_MMAP,
    CODEC_OSAL_MEM_TYPE_USERPTR = V4L2_MEMORY_USERPTR,
    CODEC_OSAL_MEM_TYPE_DMABUF  = V4L2_MEMORY_DMABUF,
} CodecOSAL_MemType;

typedef enum _CodecOSAL_BufType {
    CODEC_OSAL_BUF_TYPE_SRC = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE,
    CODEC_OSAL_BUF_TYPE_DST = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
} CodecOSAL_BufType;

typedef enum _CodecOSAL_CID {
    CODEC_OSAL_CID_DEC_NUM_MIN_BUFFERS          = V4L2_CID_MIN_BUFFERS_FOR_CAPTURE,
    CODEC_OSAL_CID_DEC_DISPLAY_DELAY            = V4L2_CID_MPEG_MFC51_VIDEO_DECODER_H264_DISPLAY_DELAY,
    CODEC_OSAL_CID_DEC_IMMEDIATE_DISPLAY        = V4L2_CID_MPEG_VIDEO_DECODER_IMMEDIATE_DISPLAY,
    CODEC_OSAL_CID_DEC_I_FRAME_DECODING         = V4L2_CID_MPEG_MFC51_VIDEO_I_FRAME_DECODING,
    CODEC_OSAL_CID_DEC_PACKED_PB                = V4L2_CID_MPEG_MFC51_VIDEO_PACKED_PB,
    CODEC_OSAL_CID_DEC_DEBLOCK_FILTER           = V4L2_CID_MPEG_VIDEO_DECODER_MPEG4_DEBLOCK_FILTER,
    CODEC_OSAL_CID_DEC_SLICE_MODE               = V4L2_CID_MPEG_VIDEO_DECODER_SLICE_INTERFACE,
    CODEC_OSAL_CID_DEC_SEI_PARSING              = V4L2_CID_MPEG_VIDEO_H264_SEI_FRAME_PACKING,
    CODEC_OSAL_CID_DEC_DTS_MODE                 = V4L2_CID_MPEG_VIDEO_DECODER_DECODING_TIMESTAMP_MODE,
    CODEC_OSAL_CID_DEC_DUAL_DPB_MODE            = V4L2_CID_MPEG_MFC_SET_DUAL_DPB_MODE,
    CODEC_OSAL_CID_DEC_DYNAMIC_DPB_MODE         = V4L2_CID_MPEG_MFC_SET_DYNAMIC_DPB_MODE,
    CODEC_OSAL_CID_DEC_SET_USER_SHARED_HANDLE   = V4L2_CID_MPEG_MFC_SET_USER_SHARED_HANDLE,
    CODEC_OSAL_CID_DEC_GET_10BIT_INFO           = V4L2_CID_MPEG_MFC_GET_10BIT_INFO,
    CODEC_OSAL_CID_DEC_CHECK_STATE              = V4L2_CID_MPEG_MFC51_VIDEO_CHECK_STATE,
    CODEC_OSAL_CID_DEC_DISPLAY_STATUS           = V4L2_CID_MPEG_MFC51_VIDEO_DISPLAY_STATUS,
    CODEC_OSAL_CID_DEC_WAIT_DECODING_START      = V4L2_CID_MPEG_VIDEO_DECODER_WAIT_DECODING_START,
    CODEC_OSAL_CID_DEC_SEARCH_BLACK_BAR         = V4L2_CID_MPEG_VIDEO_BLACK_BAR_DETECT,
    CODEC_OSAL_CID_DEC_GET_DISPLAY_DELAY        = V4L2_CID_MPEG_VIDEO_GET_DISPLAY_DELAY,
    CODEC_OSAL_CID_DEC_ACTUAL_FORMAT            = V4L2_CID_MPEG_VIDEO_UNCOMP_FMT,
    CODEC_OSAL_CID_DEC_DECODE_ORDER             = V4L2_CID_MPEG_VIDEO_DECODING_ORDER,
    CODEC_OSAL_CID_DEC_FRAME_POC                = V4L2_CID_MPEG_VIDEO_FRAME_POC,

    CODEC_OSAL_CID_ENC_FRAME_TYPE                    = V4L2_CID_MPEG_MFC51_VIDEO_FORCE_FRAME_TYPE,
    CODEC_OSAL_CID_ENC_FRAME_RATE                    = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_RATE_CH,
    CODEC_OSAL_CID_ENC_BIT_RATE                      = V4L2_CID_MPEG_MFC51_VIDEO_BIT_RATE_CH,
    CODEC_OSAL_CID_ENC_FRAME_SKIP_MODE               = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE,
    CODEC_OSAL_CID_ENC_IDR_PERIOD                    = V4L2_CID_MPEG_MFC51_VIDEO_I_PERIOD_CH,
    CODEC_OSAL_CID_ENC_H264_PREPEND_SPSPPS_TO_IDR    = V4L2_CID_MPEG_VIDEO_H264_PREPEND_SPSPPS_TO_IDR,
    CODEC_OSAL_CID_ENC_HEVC_PREPEND_SPSPPS_TO_IDR    = V4L2_CID_MPEG_VIDEO_HEVC_PREPEND_SPSPPS_TO_IDR,
    CODEC_OSAL_CID_ENC_H264_TEMPORAL_SVC_LAYER_CH    = V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_CH,
    CODEC_OSAL_CID_ENC_HEVC_TEMPORAL_SVC_LAYER_CH    = V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_CH,
    CODEC_OSAL_CID_ENC_VP8_TEMPORAL_SVC_LAYER_CH     = V4L2_CID_MPEG_VIDEO_VP8_HIERARCHICAL_CODING_LAYER_CH,
    CODEC_OSAL_CID_ENC_VP9_TEMPORAL_SVC_LAYER_CH     = V4L2_CID_MPEG_VIDEO_VP9_HIERARCHICAL_CODING_LAYER_CH,
    CODEC_OSAL_CID_ENC_SET_CONFIG_QP                 = V4L2_CID_MPEG_MFC_CONFIG_QP,
    CODEC_OSAL_CID_ENC_H264_LTR_MARK_INDEX           = V4L2_CID_MPEG_MFC_H264_MARK_LTR,
    CODEC_OSAL_CID_ENC_H264_LTR_USE_INDEX            = V4L2_CID_MPEG_MFC_H264_USE_LTR,
    CODEC_OSAL_CID_ENC_H264_LTR_BASE_PID             = V4L2_CID_MPEG_MFC_H264_BASE_PRIORITY,
    CODEC_OSAL_CID_ENC_ROI_INFO                      = V4L2_CID_MPEG_VIDEO_ROI_CONTROL,
    CODEC_OSAL_CID_ENC_EXT_BUFFER_SIZE               = V4L2_CID_MPEG_MFC_GET_EXTRA_BUFFER_SIZE,
    CODEC_OSAL_CID_ENC_WP_ENABLE                     = V4L2_CID_MPEG_VIDEO_WEIGHTED_ENABLE,
    CODEC_OSAL_CID_ENC_YSUM_DATA                     = V4L2_CID_MPEG_VIDEO_YSUM,
    CODEC_OSAL_CID_ENC_I_FRAME_RATIO                 = V4L2_CID_MPEG_VIDEO_RATIO_OF_INTRA,
    CODEC_OSAL_CID_ENC_ENABLE_ADAPTIVE_LAYER_BITRATE = V4L2_CID_MPEG_VIDEO_HIERARCHICAL_BITRATE_CTRL,
    CODEC_OSAL_CID_ENC_HEADER_MODE                   = V4L2_CID_MPEG_VIDEO_HEADER_MODE,
    CODEC_OSAL_CID_ENC_ENABLE_DROP_CTRL              = V4L2_CID_MPEG_VIDEO_DROP_CONTROL,
    CODEC_OSAL_CID_ENC_AVERAGE_QP                    = V4L2_CID_MPEG_VIDEO_AVERAGE_QP,
    CODEC_OSAL_CID_ENC_GDC_VOTF                      = V4L2_CID_MPEG_VIDEO_GDC_VOTF,
    CODEC_OSAL_CID_ENC_GOP_CTRL                      = V4L2_CID_MPEG_VIDEO_GOP_CTRL,
    CODEC_OSAL_CID_ENC_MIN_QUALITY                   = V4L2_CID_MPEG_VIDEO_MIN_QUALITY,
    CODEC_OSAL_CID_ENC_HDR_STAT_USER_SHARED_HANDLE   = V4L2_CID_MPEG_MFC_HDR10_PLUS_STAT_USER_SHARED_HANDLE,
    CODEC_OSAL_CID_ENC_HDR_STAT_ENABLE               = V4L2_CID_MPEG_MFC_HDR10_PLUS_STAT_ENABLE,
    CODEC_OSAL_CID_ENC_MAX_IFRAME_SIZE               = V4L2_CID_MPEG_VIDEO_MAX_IFRAME_SIZE,

    CODEC_OSAL_CID_VIDEO_FRAME_TAG                          = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG,
    CODEC_OSAL_CID_VIDEO_QOS_RATIO                          = V4L2_CID_MPEG_VIDEO_QOS_RATIO,
    CODEC_OSAL_CID_VIDEO_CACHEABLE_BUFFER                   = V4L2_CID_CACHEABLE,
    CODEC_OSAL_CID_VIDEO_GET_VERSION_INFO                   = V4L2_CID_MPEG_MFC_GET_VERSION_INFO,
    CODEC_OSAL_CID_VIDEO_GET_EXT_INFO                       = V4L2_CID_MPEG_MFC_GET_EXT_INFO,
    CODEC_OSAL_CID_VIDEO_GET_DRIVER_VERSION                 = V4L2_CID_MPEG_MFC_GET_DRIVER_INFO,
    CODEC_OSAL_CID_VIDEO_SET_HDR_USER_SHARED_HANDLE         = V4L2_CID_MPEG_MFC_HDR_USER_SHARED_HANDLE,
    CODEC_OSAL_CID_VIDEO_SRC_BUF_FLAG                       = V4L2_CID_MPEG_VIDEO_SRC_BUF_FLAG,
    CODEC_OSAL_CID_VIDEO_DST_BUF_FLAG                       = V4L2_CID_MPEG_VIDEO_DST_BUF_FLAG,
    CODEC_OSAL_CID_VIDEO_FRAME_ERROR_TYPE                   = V4L2_CID_MPEG_VIDEO_FRAME_ERROR_TYPE,
    CODEC_OSAL_CID_VIDEO_SET_FILM_GRAIN_USER_SHARED_HANDLE  = V4L2_CID_MPEG_MFC_AV1_FILM_GRAIN_USER_SHARED_HANDLE,
    CODEC_OSAL_CID_VIDEO_SKIP_LAZY_UNMAP                    = V4L2_CID_MPEG_VIDEO_SKIP_LAZY_UNMAP,
    CODEC_OSAL_CID_ACTUAL_FRAMERATE                         = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_RATE,
    CODEC_OSAL_CID_ACTUAL_PRIORITY                          = V4L2_CID_MPEG_VIDEO_PRIORITY,

    CODEC_OSAL_CID_DEC_SEI_INFO             = 0xFFFF0000,
    CODEC_OSAL_CID_ENC_SET_PARAMS,
    CODEC_OSAL_CID_ENC_QP_RANGE,
    CODEC_OSAL_CID_DEC_DISCARD_RCVHEADER,  /* not implemented */
    CODEC_OSAL_CID_ENC_SLICE_MODE,         /* not implemented : V4L2_CID_MPEG_MFC_SET_SLICE_MODE */
    CODEC_OSAL_CID_DEC_HDR_INFO,
    CODEC_OSAL_CID_ENC_COLOR_ASPECTS,
    CODEC_OSAL_CID_ENC_HDR_STATIC_INFO,
    CODEC_OSAL_CID_ENC_MV_VER_POSITION,
} CodecOSAL_CID;

typedef enum _CodecOSAL_InterlaceType {
    CODEC_OSAL_INTER_TYPE_NONE  = V4L2_FIELD_NONE,
    CODEC_OSAL_INTER_TYPE_INTER = V4L2_FIELD_INTERLACED,
    CODEC_OSAL_INTER_TYPE_TB    = V4L2_FIELD_INTERLACED_TB,
    CODEC_OSAL_INTER_TYPE_BT    = V4L2_FIELD_INTERLACED_BT,
} CodecOSAL_InterlaceType;

enum v4l2_mpeg_video_h264_sei_fp_type {
    V4L2_MPEG_VIDEO_H264_SEI_FP_TYPE_CHEKERBOARD    = 0,
    V4L2_MPEG_VIDEO_H264_SEI_FP_TYPE_COLUMN         = 1,
    V4L2_MPEG_VIDEO_H264_SEI_FP_TYPE_ROW            = 2,
    V4L2_MPEG_VIDEO_H264_SEI_FP_TYPE_SIDE_BY_SIDE   = 3,
    V4L2_MPEG_VIDEO_H264_SEI_FP_TYPE_TOP_BOTTOM     = 4,
    V4L2_MPEG_VIDEO_H264_SEI_FP_TYPE_TEMPORAL       = 5,
};

typedef enum _CodecOSAL_HeaderMode {
    CODEC_OSAL_HEADER_MODE_SEPARATE  = V4L2_MPEG_VIDEO_HEADER_MODE_SEPARATE,
    CODEC_OSAL_HEADER_WITH_1ST_IDR   = V4L2_MPEG_VIDEO_HEADER_MODE_JOINED_WITH_1ST_FRAME,
} CodecOSAL_HeaderMode;

typedef struct _CodecOSAL_Format {
    CodecOSAL_BufType       type;
    ExynosVideoCodingType   format;
    int                     field;
    int                     planeSize[VIDEO_BUFFER_MAX_PLANES];
    unsigned int            width;
    unsigned int            height;
    unsigned int            stride;
    unsigned int            cstride;
    int                     nPlane;
} CodecOSAL_Format;

typedef struct _CodecOSAL_Crop {
    CodecOSAL_BufType   type;
    int                 left;
    int                 top;
    int                 width;
    int                 height;
} CodecOSAL_Crop;

typedef struct _CodecOSAL_Plane {
    void               *addr;
    int                 bufferSize;
    int                 dataLen;
    unsigned            offset;
} CodecOSAL_Plane;

typedef struct _CodecOSAL_Buffer {
    CodecOSAL_BufType  type;
    int                memory;
    int                nPlane;
    CodecOSAL_Plane    planes[VIDEO_BUFFER_MAX_PLANES];
    int                index;
    unsigned int       flags;
    struct timeval     timestamp;

    unsigned int            field;
    ExynosVideoFrameType    frameType;
} CodecOSAL_Buffer;

typedef struct v4l2_requestbuffers CodecOSAL_ReqBuf;

#define CODEC_OSAL_POLL_TIMEOUT -1
#define CODEC_OSAL_MAX_POLLFD 2
#define CODE_OSAL_POLL_SRC 0
#define CODE_OSAL_POLL_DST 1

#ifdef USE_EPOLL
#define CODEC_OSAL_POLL_SRC_EVENT (EPOLLOUT | EPOLLWRNORM)
#define CODEC_OSAL_POLL_DST_EVENT (EPOLLIN | EPOLLRDNORM)
#define CODEC_OSAL_POLL_USR_EVENT (EPOLLIN)
#define CODEC_OSAL_POLL_ERR_EVENT (EPOLLERR)
#else
#define CODEC_OSAL_POLL_SRC_EVENT (POLLOUT | POLLWRNORM)
#define CODEC_OSAL_POLL_DST_EVENT (POLLIN | POLLRDNORM)
#define CODEC_OSAL_POLL_USR_EVENT (POLLIN)
#define CODEC_OSAL_POLL_ERR_EVENT (POLLERR)
#endif

typedef struct _CodecOSAL_Pollfd {
    int     fd;
    short   events;
    short   revents;
} CodecOSAL_Pollfd;

typedef struct _CodecOSALInfo {
    int reserved;
} CodecOSALInfo;

typedef struct _CodecOSALVideoContext {
    ExynosVideoContext videoCtx;
    CodecOSALInfo      osalCtx;
} CodecOSALVideoContext;

int Codec_OSAL_VideoMemoryToSystemMemory(ExynosVideoMemoryType eMemoryType);

unsigned int Codec_OSAL_CodingTypeToCompressdFormat(ExynosVideoCodingType eCodingType);
ExynosVideoColorFormatType Codec_OSAL_PixelFormatToColorFormat(unsigned int nPixelFormat);
unsigned int Codec_OSAL_ColorFormatToPixelFormat(ExynosVideoColorFormatType eColorFormat, int nHwVersion);

int Codec_OSAL_DevOpen(const char *sDevName, int nFlag, CodecOSALVideoContext *pCtx);
void Codec_OSAL_DevClose(CodecOSALVideoContext *pCtx);

int Codec_OSAL_QueryCap(CodecOSALVideoContext *pCtx);

int Codec_OSAL_EnqueueBuf(CodecOSALVideoContext *pCtx, CodecOSAL_Buffer *pBuf);
int Codec_OSAL_DequeueBuf(CodecOSALVideoContext *pCtx, CodecOSAL_Buffer *pBuf);

int Codec_OSAL_GetControls(CodecOSALVideoContext *pCtx, unsigned int nCID, void *pInfo);
int Codec_OSAL_SetControls(CodecOSALVideoContext *pCtx, unsigned int nCID, void *pInfo);

int Codec_OSAL_GetControl(CodecOSALVideoContext *pCtx, unsigned int nCID, int *pValue);
int Codec_OSAL_SetControl(CodecOSALVideoContext *pCtx, unsigned int nCID, unsigned long nValue);

int Codec_OSAL_GetCrop(CodecOSALVideoContext *pCtx, CodecOSAL_Crop *pCrop);
int Codec_OSAL_SetCrop(CodecOSALVideoContext *pCtx, CodecOSAL_Crop *pCrop);

int Codec_OSAL_GetFormat(CodecOSALVideoContext *pCtx, CodecOSAL_Format *pFmt);
int Codec_OSAL_SetFormat(CodecOSALVideoContext *pCtx, CodecOSAL_Format *pFmt);
int Codec_OSAL_TryFormat(CodecOSALVideoContext *pCtx, CodecOSAL_Format *pFmt);

int Codec_OSAL_RequestBuf(CodecOSALVideoContext *pCtx, CodecOSAL_ReqBuf *pReqBuf);
int Codec_OSAL_QueryBuf(CodecOSALVideoContext *pCtx, CodecOSAL_Buffer *pBuf);

int Codec_OSAL_SetStreamOn(CodecOSALVideoContext *pCtx, int nPort);
int Codec_OSAL_SetStreamOff(CodecOSALVideoContext *pCtx, int nPort);

void *Codec_OSAL_MemoryMap(void *addr, size_t len, int prot, int flags, unsigned long fd, off_t offset);
int Codec_OSAL_MemoryUnmap(void *addr, size_t len);

#ifndef USE_EPOLL
int Codec_OSAL_Poll(CodecOSALVideoContext *pCtx, CodecOSAL_Pollfd pollfd[CODEC_OSAL_MAX_POLLFD], int cnt, int time);
#else
void *Codec_OSAL_Epoll_Create();
int   Codec_OSAL_Epoll_Regist(void *pHandle, CodecOSAL_Pollfd pollfd[CODEC_OSAL_MAX_POLLFD], int cnt);
int   Codec_OSAL_Epoll(void *pHandle, CodecOSAL_Pollfd pollfd[CODEC_OSAL_MAX_POLLFD], int time);
void  Codec_OSAL_Epoll_Destroy(void *pHandle);
#endif

#endif
