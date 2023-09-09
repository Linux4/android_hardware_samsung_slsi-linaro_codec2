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
 * @file    ExynosVideo_OSAL.c
 * @brief   ExynosVideo OSAL
 * @author  SeungBeom Kim (sbcrux.kim@samsung.com)
 *          Taehwan Kim   (t_h.kim@samsung.com)
 * @version    1.0.0
 * @history
 *   2016.01.07 : Create
 */
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "ExynosVideo_OSAL.h"
#include "ExynosVideo_OSAL_Dec.h"
#include "ExynosVideo_OSAL_Enc.h"

#include "ExynosVideoDec.h"
#include "ExynosVideoEnc.h"

#define LOG_NDEBUG 0
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "ExynosVideoOSAL"

#include "ExynosVideo_OSAL_Log.h"

#define GET_16BIT_HIGH(x)    ((x >> 16) & 0xFFFF)
#define GET_16BIT_LOW(x)      (x & 0xFFFF)
#define SET_16BIT_TO_HIGH(x) ((0xFFFF & x) << 16)

static struct {
    int primaries;
    int transfer;
    int coeff;
} ColorSpaceToColorAspects[] = {
    {PRIMARIES_UNSPECIFIED, TRANSFER_UNSPECIFIED, MATRIX_COEFF_UNSPECIFIED},   /* Unknown */
    {PRIMARIES_BT601_6_525, TRANSFER_SMPTE_170M,  MATRIX_COEFF_SMPTE170M},     /* Rec. ITU-R BT.601-7 */
    {PRIMARIES_BT709_5,     TRANSFER_SMPTE_170M,  MATRIX_COEFF_REC709},        /* Rec. ITU-R BT.709-6 */
    {PRIMARIES_BT601_6_525, TRANSFER_SMPTE_170M,  MATRIX_COEFF_SMPTE170M},     /* SMPTE-170 */
    {PRIMARIES_BT601_6_525, TRANSFER_SMPTE_240M,  MATRIX_COEFF_SMPTE240M},     /* SMPTE-240 */
    {PRIMARIES_BT2020,      TRANSFER_SMPTE_170M,  MATRIX_COEFF_BT2020},        /* Rec. ITU-R BT.2020-2 */
    {PRIMARIES_RESERVED,    TRANSFER_RESERVED,    MATRIX_COEFF_IDENTITY},      /* Reserved */
    {PRIMARIES_RESERVED,    TRANSFER_SMPTE_170M,  MATRIX_COEFF_REC709},        /* sRGB (IEC 61966-2-1) */
};

int Codec_OSAL_VideoMemoryToSystemMemory(
    ExynosVideoMemoryType eMemoryType) {
    if (eMemoryType == VIDEO_MEMORY_USERPTR)
        return V4L2_MEMORY_USERPTR;

    return V4L2_MEMORY_DMABUF;
}

unsigned int Codec_OSAL_CodingTypeToCompressdFormat(
    ExynosVideoCodingType eCodingType) {
    unsigned int nCompressdFormat = V4L2_PIX_FMT_H264;

    switch (eCodingType) {
    case VIDEO_CODING_AVC:
        nCompressdFormat = V4L2_PIX_FMT_H264;
        break;
    case VIDEO_CODING_MPEG4:
        nCompressdFormat = V4L2_PIX_FMT_MPEG4;
        break;
    case VIDEO_CODING_VP8:
        nCompressdFormat = V4L2_PIX_FMT_VP8;
        break;
    case VIDEO_CODING_H263:
        nCompressdFormat = V4L2_PIX_FMT_H263;
        break;
    case VIDEO_CODING_VC1:
        nCompressdFormat = V4L2_PIX_FMT_VC1_ANNEX_G;
        break;
    case VIDEO_CODING_VC1_RCV:
        nCompressdFormat = V4L2_PIX_FMT_VC1_ANNEX_L;
        break;
    case VIDEO_CODING_MPEG2:
        nCompressdFormat = V4L2_PIX_FMT_MPEG2;
        break;
    case VIDEO_CODING_HEVC:
        nCompressdFormat = V4L2_PIX_FMT_HEVC;
        break;
    case VIDEO_CODING_VP9:
        nCompressdFormat = V4L2_PIX_FMT_VP9;
        break;
    case VIDEO_CODING_AV1:
        nCompressdFormat = V4L2_PIX_FMT_AV1;
        break;
    default:
        nCompressdFormat = V4L2_PIX_FMT_H264;
        break;
    }

    return nCompressdFormat;
}

ExynosVideoColorFormatType Codec_OSAL_PixelFormatToColorFormat(
    unsigned int nPixelFormat) {
    ExynosVideoColorFormatType eColorFormat = VIDEO_COLORFORMAT_UNKNOWN;

    switch (nPixelFormat) {
    case V4L2_PIX_FMT_NV12M:
        eColorFormat = VIDEO_COLORFORMAT_NV12M;
        break;
    case V4L2_PIX_FMT_NV12M_S10B:
        eColorFormat = VIDEO_COLORFORMAT_NV12M_S10B;
        break;
    case V4L2_PIX_FMT_NV12M_P010:
        eColorFormat = VIDEO_COLORFORMAT_NV12M_P010;
        break;
    case V4L2_PIX_FMT_NV21M:
        eColorFormat = VIDEO_COLORFORMAT_NV21M;
        break;
    case V4L2_PIX_FMT_NV21M_S10B:
        eColorFormat = VIDEO_COLORFORMAT_NV21M_S10B;
        break;
    case V4L2_PIX_FMT_NV21M_P010:
        eColorFormat = VIDEO_COLORFORMAT_NV21M_P010;
        break;
    case V4L2_PIX_FMT_YUV420M:
        eColorFormat = VIDEO_COLORFORMAT_I420M;
        break;
    case V4L2_PIX_FMT_YVU420M:
        eColorFormat = VIDEO_COLORFORMAT_YV12M;
        break;
    case V4L2_PIX_FMT_NV12M_SBWC_8B:
        eColorFormat = VIDEO_COLORFORMAT_NV12M_SBWC;
        break;
    case V4L2_PIX_FMT_NV12M_SBWC_10B:
        eColorFormat = VIDEO_COLORFORMAT_NV12M_10B_SBWC;
        break;
    case V4L2_PIX_FMT_NV21M_SBWC_8B:
        eColorFormat = VIDEO_COLORFORMAT_NV21M_SBWC;
        break;
    case V4L2_PIX_FMT_NV21M_SBWC_10B:
        eColorFormat = VIDEO_COLORFORMAT_NV21M_10B_SBWC;
        break;
    case V4L2_PIX_FMT_NV12N:
        eColorFormat = VIDEO_COLORFORMAT_NV12;
        break;
    case V4L2_PIX_FMT_NV12N_P010:
        eColorFormat = VIDEO_COLORFORMAT_NV12_P010;
        break;
    case V4L2_PIX_FMT_NV12N_SBWC_8B:
        eColorFormat = VIDEO_COLORFORMAT_NV12_SBWC;
        break;
    case V4L2_PIX_FMT_NV12N_SBWC_10B:
        eColorFormat = VIDEO_COLORFORMAT_NV12_10B_SBWC;
        break;
    case V4L2_PIX_FMT_NV12N_SBWC_256_8B:
        eColorFormat = VIDEO_COLORFORMAT_NV12_256_SBWC;
        break;
    case V4L2_PIX_FMT_NV12N_SBWC_256_10B:
        eColorFormat = VIDEO_COLORFORMAT_NV12_10B_256_SBWC;
        break;
#ifdef USE_SINGLE_PALNE_SUPPORT
    case V4L2_PIX_FMT_NV12N_10B:
        eColorFormat = VIDEO_COLORFORMAT_NV12_S10B;
        break;
    case V4L2_PIX_FMT_YUV420N:
        eColorFormat = VIDEO_COLORFORMAT_I420;
        break;
    case V4L2_PIX_FMT_NV12NT:
        eColorFormat = VIDEO_COLORFORMAT_NV12_TILED;
        break;
#endif
    case V4L2_PIX_FMT_ARGB32:
        eColorFormat = VIDEO_COLORFORMAT_ARGB8888;
        break;
    case V4L2_PIX_FMT_BGR32:
        eColorFormat = VIDEO_COLORFORMAT_BGRA8888;
        break;
    case V4L2_PIX_FMT_RGB32X:
        eColorFormat = VIDEO_COLORFORMAT_RGBA8888;
        break;
    case V4L2_PIX_FMT_NV12MT:
    case V4L2_PIX_FMT_NV12MT_16X16:
        eColorFormat = VIDEO_COLORFORMAT_NV12M_TILED;
        break;
    default:
        break;
    }

    return eColorFormat;
}

unsigned int Codec_OSAL_ColorFormatToPixelFormat(
    ExynosVideoColorFormatType  eColorFormat,
    int                         nHwVersion) {
    unsigned int nPixelFormat = V4L2_PIX_FMT_NV12M;

    switch (eColorFormat) {
    case VIDEO_COLORFORMAT_NV12M:
        nPixelFormat = V4L2_PIX_FMT_NV12M;
        break;
    case VIDEO_COLORFORMAT_NV12M_S10B:
        nPixelFormat = V4L2_PIX_FMT_NV12M_S10B;
        break;
    case VIDEO_COLORFORMAT_NV12M_P010:
        nPixelFormat = V4L2_PIX_FMT_NV12M_P010;
        break;
    case VIDEO_COLORFORMAT_NV21M:
        nPixelFormat = V4L2_PIX_FMT_NV21M;
        break;
    case VIDEO_COLORFORMAT_NV21M_S10B:
        nPixelFormat = V4L2_PIX_FMT_NV21M_S10B;
        break;
    case VIDEO_COLORFORMAT_NV21M_P010:
        nPixelFormat = V4L2_PIX_FMT_NV21M_P010;
        break;
    case VIDEO_COLORFORMAT_I420M:
        nPixelFormat = V4L2_PIX_FMT_YUV420M;
        break;
    case VIDEO_COLORFORMAT_YV12M:
        nPixelFormat = V4L2_PIX_FMT_YVU420M;
        break;
    case VIDEO_COLORFORMAT_NV12M_SBWC:
        nPixelFormat = V4L2_PIX_FMT_NV12M_SBWC_8B;
        break;
    case VIDEO_COLORFORMAT_NV12M_10B_SBWC:
        nPixelFormat = V4L2_PIX_FMT_NV12M_SBWC_10B;
        break;
    case VIDEO_COLORFORMAT_NV21M_SBWC:
        nPixelFormat = V4L2_PIX_FMT_NV21M_SBWC_8B;
        break;
    case VIDEO_COLORFORMAT_NV21M_10B_SBWC:
        nPixelFormat = V4L2_PIX_FMT_NV21M_SBWC_10B;
        break;
    case VIDEO_COLORFORMAT_NV12M_SBWC_L50:
    case VIDEO_COLORFORMAT_NV12M_SBWC_L75:
        nPixelFormat = V4L2_PIX_FMT_NV12M_SBWCL_8B;
        break;
    case VIDEO_COLORFORMAT_NV12M_10B_SBWC_L40:
    case VIDEO_COLORFORMAT_NV12M_10B_SBWC_L60:
    case VIDEO_COLORFORMAT_NV12M_10B_SBWC_L80:
        nPixelFormat = V4L2_PIX_FMT_NV12M_SBWCL_10B;
        break;
    case VIDEO_COLORFORMAT_NV12M_32_SBWC_L:
        nPixelFormat = V4L2_PIX_FMT_NV12M_SBWCL_32_8B;
        break;
    case VIDEO_COLORFORMAT_NV12M_64_SBWC_L:
        nPixelFormat = V4L2_PIX_FMT_NV12M_SBWCL_64_8B;
        break;
    case VIDEO_COLORFORMAT_NV12M_10B_32_SBWC_L:
        nPixelFormat = V4L2_PIX_FMT_NV12M_SBWCL_32_10B;
        break;
    case VIDEO_COLORFORMAT_NV12M_10B_64_SBWC_L:
        nPixelFormat = V4L2_PIX_FMT_NV12M_SBWCL_64_10B;
        break;
    case VIDEO_COLORFORMAT_NV12:
        nPixelFormat = V4L2_PIX_FMT_NV12N;
        break;
    case VIDEO_COLORFORMAT_NV12_P010:
        nPixelFormat = V4L2_PIX_FMT_NV12N_P010;
        break;
    case VIDEO_COLORFORMAT_NV12_SBWC:
        nPixelFormat = V4L2_PIX_FMT_NV12N_SBWC_8B;
        break;
    case VIDEO_COLORFORMAT_NV12_10B_SBWC:
        nPixelFormat = V4L2_PIX_FMT_NV12N_SBWC_10B;
        break;
    case VIDEO_COLORFORMAT_NV12_256_SBWC:
        nPixelFormat = V4L2_PIX_FMT_NV12N_SBWC_256_8B;
        break;
    case VIDEO_COLORFORMAT_NV12_10B_256_SBWC:
        nPixelFormat = V4L2_PIX_FMT_NV12N_SBWC_256_10B;
        break;
    case VIDEO_COLORFORMAT_NV12_32_SBWC_L:
        nPixelFormat = V4L2_PIX_FMT_NV12M_SBWCL_32_8B;
        break;
    case VIDEO_COLORFORMAT_NV12_64_SBWC_L:
        nPixelFormat = V4L2_PIX_FMT_NV12M_SBWCL_64_8B;
        break;
    case VIDEO_COLORFORMAT_NV12_10B_32_SBWC_L:
        nPixelFormat = V4L2_PIX_FMT_NV12M_SBWCL_32_10B;
        break;
    case VIDEO_COLORFORMAT_NV12_10B_64_SBWC_L:
        nPixelFormat = V4L2_PIX_FMT_NV12M_SBWCL_64_10B;
        break;
#ifdef USE_SINGLE_PALNE_SUPPORT
    case VIDEO_COLORFORMAT_NV12_S10B:
        nPixelFormat = V4L2_PIX_FMT_NV12N_10B;
        break;
    case VIDEO_COLORFORMAT_I420:
        nPixelFormat = V4L2_PIX_FMT_YUV420N;
        break;
    case VIDEO_COLORFORMAT_NV12_TILED:
        nPixelFormat = V4L2_PIX_FMT_NV12NT;
        break;
    case VIDEO_COLORFORMAT_NV12_SBWC_L50:
    case VIDEO_COLORFORMAT_NV12_SBWC_L75:
        nPixelFormat = V4L2_PIX_FMT_NV12N_SBWCL_8B;
        break;
    case VIDEO_COLORFORMAT_NV12_10B_SBWC_L40:
    case VIDEO_COLORFORMAT_NV12_10B_SBWC_L60:
    case VIDEO_COLORFORMAT_NV12_10B_SBWC_L80:
        nPixelFormat = V4L2_PIX_FMT_NV12N_SBWCL_10B;
        break;
#endif
    case VIDEO_COLORFORMAT_ARGB8888:
        nPixelFormat = V4L2_PIX_FMT_ARGB32;
        break;
    case VIDEO_COLORFORMAT_BGRA8888:
        nPixelFormat = V4L2_PIX_FMT_BGR32;
        break;
    case VIDEO_COLORFORMAT_RGBA8888:
        nPixelFormat = V4L2_PIX_FMT_RGB32X;
        break;
    case VIDEO_COLORFORMAT_NV12M_TILED:
        if (nHwVersion == (int)MFC_51)
            nPixelFormat = V4L2_PIX_FMT_NV12MT;
        else
            nPixelFormat = V4L2_PIX_FMT_NV12MT_16X16;
        break;
    default:
        nPixelFormat = V4L2_PIX_FMT_NV12M;
        break;
    }

    return nPixelFormat;
}

int Codec_OSAL_DevOpen(
    const char              *sDevName,
    int                      nFlag,
    CodecOSALVideoContext   *pCtx) {
    if ((sDevName != NULL) &&
        (pCtx != NULL)) {
        pCtx->videoCtx.hDevice = exynos_v4l2_open_devname(sDevName, nFlag, 0);
        return pCtx->videoCtx.hDevice;
    }

    return -1;
}

void Codec_OSAL_DevClose(CodecOSALVideoContext *pCtx) {
    if ((pCtx != NULL) &&
        (pCtx->videoCtx.hDevice >= 0)) {
        exynos_v4l2_close(pCtx->videoCtx.hDevice);
    }

    return;
}

int Codec_OSAL_QueryCap(CodecOSALVideoContext *pCtx) {
    int needCaps = (V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING);

    if ((pCtx != NULL) &&
        (pCtx->videoCtx.hDevice >= 0)) {
        if (exynos_v4l2_querycap(pCtx->videoCtx.hDevice, needCaps))
            return 0;
    }

    return -1;
}

int Codec_OSAL_EnqueueBuf(
    CodecOSALVideoContext   *pCtx,
    CodecOSAL_Buffer        *pBuf) {
    if ((pCtx != NULL) &&
        (pBuf != NULL) &&
        (pCtx->videoCtx.hDevice >= 0)) {
        struct v4l2_buffer  buf;
        struct v4l2_plane   planes[VIDEO_BUFFER_MAX_PLANES];
        int i;

        memset(&buf, 0, sizeof(buf));
        memset(&planes, 0, sizeof(planes));

        buf.type        = pBuf->type;
        buf.m.planes    = planes;
        buf.length      = pBuf->nPlane;
        buf.memory      = pBuf->memory;
        buf.index       = pBuf->index;

        for (i = 0; i < pBuf->nPlane; i++) {
            if ((unsigned int)buf.memory == (unsigned int)CODEC_OSAL_MEM_TYPE_USERPTR)
                buf.m.planes[i].m.userptr = (unsigned long)pBuf->planes[i].addr;
            else
                buf.m.planes[i].m.fd = (int)(unsigned long)pBuf->planes[i].addr;

            buf.m.planes[i].length      = pBuf->planes[i].bufferSize;
            buf.m.planes[i].bytesused   = pBuf->planes[i].dataLen;
            buf.m.planes[i].data_offset = 0;
        }

        if (pCtx->videoCtx.instInfo.bVideoBufFlagCtrl == VIDEO_TRUE) {
            int nCID = (buf.type == CODEC_OSAL_BUF_TYPE_SRC)? CODEC_OSAL_CID_VIDEO_SRC_BUF_FLAG:
                            CODEC_OSAL_CID_VIDEO_DST_BUF_FLAG;
            if (Codec_OSAL_SetControl(pCtx, nCID, pBuf->flags) < 0) {
                ALOGE("[%s] Codec_OSAL_SetControl(0x%x) is failed", __FUNCTION__, nCID);
            }
        } else {
            buf.reserved2 = pBuf->flags;
        }

        memcpy(&(buf.timestamp), &(pBuf->timestamp), sizeof(struct timeval));

        return exynos_v4l2_qbuf(pCtx->videoCtx.hDevice, &buf);
    }

EXIT:
    return -1;
}

int Codec_OSAL_DequeueBuf(
    CodecOSALVideoContext   *pCtx,
    CodecOSAL_Buffer        *pBuf) {
    if ((pCtx != NULL) &&
        (pBuf != NULL) &&
        (pCtx->videoCtx.hDevice >= 0)) {
        struct v4l2_buffer  buf;
        struct v4l2_plane   planes[VIDEO_BUFFER_MAX_PLANES];
        int i;

        memset(&buf, 0, sizeof(buf));
        memset(&planes, 0, sizeof(planes));

        buf.type        = pBuf->type;
        buf.m.planes    = planes;
        buf.length      = pBuf->nPlane;
        buf.memory      = pBuf->memory;

        if (exynos_v4l2_dqbuf(pCtx->videoCtx.hDevice, &buf) == 0) {
            pBuf->index = buf.index;

            if (pCtx->videoCtx.instInfo.bVideoBufFlagCtrl == VIDEO_TRUE) {
                int nCID = (buf.type == CODEC_OSAL_BUF_TYPE_SRC)? CODEC_OSAL_CID_VIDEO_SRC_BUF_FLAG:
                                CODEC_OSAL_CID_VIDEO_DST_BUF_FLAG;
                if (Codec_OSAL_GetControl(pCtx, nCID, (int *)&(pBuf->flags)) < 0) {
                    ALOGE("[%s] Codec_OSAL_GetControl(0x%x) is failed", __FUNCTION__, nCID);
                }
            } else {
                pBuf->flags = buf.reserved2;
            }

            pBuf->field     = buf.field;
            pBuf->timestamp = buf.timestamp;

            for (i = 0; i < pBuf->nPlane; i++)
                pBuf->planes[i].dataLen = buf.m.planes[i].bytesused;

            switch (buf.flags & (0x7 << 3)) {  /* V4L2_BUF_FLAG_KEYFRAME | V4L2_BUF_FLAG_PFRAME | V4L2_BUF_FLAG_BFRAME */
            case V4L2_BUF_FLAG_KEYFRAME:
                pBuf->frameType = (pBuf->flags & CODEC_OSAL_FLAG_IDR)? VIDEO_FRAME_IDR:VIDEO_FRAME_I;
                break;
            case V4L2_BUF_FLAG_PFRAME:
                pBuf->frameType = VIDEO_FRAME_P;
                break;
            case V4L2_BUF_FLAG_BFRAME:
                pBuf->frameType = VIDEO_FRAME_B;
                break;
            default:
                pBuf->frameType = VIDEO_FRAME_UNKNOWN;
                break;
            }

            if (buf.flags & V4L2_BUF_FLAG_ERROR)
                pBuf->frameType |= VIDEO_FRAME_CORRUPT;

            /* buf.reserved2 */
            if (pBuf->flags & CODEC_OSAL_FLAG_CONSUMED_ONLY) {
                pBuf->frameType |= VIDEO_FRAME_CONSUMED_ONLY;
            }

            return 0;
        }
    }

    return -1;
}

int Codec_OSAL_GetControls_SeiInfo(
    CodecOSALVideoContext   *pCtx,
    void                    *pInfo) {
    int ret = 0;

    ExynosVideoFramePacking *pFramePacking = (ExynosVideoFramePacking *)pInfo;

    struct v4l2_ext_control  ext_ctrl[FRAME_PACK_SEI_INFO_NUM];
    struct v4l2_ext_controls ext_ctrls;

    int seiAvailable, seiInfo, seiGridPos, i;
    unsigned int seiArgmtId;

    memset(pFramePacking, 0, sizeof(*pFramePacking));
    memset(ext_ctrl, 0, sizeof(ext_ctrl));

    ext_ctrls.ctrl_class = V4L2_CTRL_CLASS_MPEG;
    ext_ctrls.count = FRAME_PACK_SEI_INFO_NUM;
    ext_ctrls.controls = ext_ctrl;
    ext_ctrl[0].id =  V4L2_CID_MPEG_VIDEO_H264_SEI_FP_AVAIL;
    ext_ctrl[1].id =  V4L2_CID_MPEG_VIDEO_H264_SEI_FP_ARRGMENT_ID;
    ext_ctrl[2].id =  V4L2_CID_MPEG_VIDEO_H264_SEI_FP_INFO;
    ext_ctrl[3].id =  V4L2_CID_MPEG_VIDEO_H264_SEI_FP_GRID_POS;

    if (exynos_v4l2_g_ext_ctrl(pCtx->videoCtx.hDevice, &ext_ctrls) != 0) {
        ret = -1;
        goto EXIT;
    }

    seiAvailable = ext_ctrl[0].value;
    seiArgmtId   = ext_ctrl[1].value;
    seiInfo      = ext_ctrl[2].value;
    seiGridPos   = ext_ctrl[3].value;

    pFramePacking->available = seiAvailable;
    pFramePacking->arrangement_id = seiArgmtId;

    pFramePacking->arrangement_cancel_flag = OPERATE_BIT(seiInfo, 0x1, 0);
    pFramePacking->arrangement_type = OPERATE_BIT(seiInfo, 0x3f, 1);
    pFramePacking->quincunx_sampling_flag = OPERATE_BIT(seiInfo, 0x1, 8);
    pFramePacking->content_interpretation_type = OPERATE_BIT(seiInfo, 0x3f, 9);
    pFramePacking->spatial_flipping_flag = OPERATE_BIT(seiInfo, 0x1, 15);
    pFramePacking->frame0_flipped_flag = OPERATE_BIT(seiInfo, 0x1, 16);
    pFramePacking->field_views_flag = OPERATE_BIT(seiInfo, 0x1, 17);
    pFramePacking->current_frame_is_frame0_flag = OPERATE_BIT(seiInfo, 0x1, 18);

    pFramePacking->frame0_grid_pos_x = OPERATE_BIT(seiGridPos, 0xf, 0);
    pFramePacking->frame0_grid_pos_y = OPERATE_BIT(seiGridPos, 0xf, 4);
    pFramePacking->frame1_grid_pos_x = OPERATE_BIT(seiGridPos, 0xf, 8);
    pFramePacking->frame1_grid_pos_y = OPERATE_BIT(seiGridPos, 0xf, 12);

EXIT:
    return ret;
}

int Codec_OSAL_GetControls_HdrInfo(
    CodecOSALVideoContext   *pCtx,
    void                    *pInfo) {
    int ret = 0;

    ExynosVideoHdrInfo *pHdrInfo = (ExynosVideoHdrInfo *)pInfo;

    struct v4l2_ext_control     ext_ctrl[HDR_INFO_NUM];
    struct v4l2_ext_controls    ext_ctrls;

    int infoType = pHdrInfo->eValidType;
    int i = 0;

    memset(ext_ctrl, 0, (sizeof(struct v4l2_ext_control) * HDR_INFO_NUM));
    ext_ctrls.ctrl_class    = V4L2_CTRL_CLASS_MPEG;
    ext_ctrls.controls      = ext_ctrl;

    if (infoType & HDR_INFO_LIGHT) {
        ext_ctrl[i].id      = V4L2_CID_MPEG_VIDEO_SEI_MAX_PIC_AVERAGE_LIGHT;
        ext_ctrl[i + 1].id  = V4L2_CID_MPEG_VIDEO_SEI_MAX_CONTENT_LIGHT;
        i += 2;
    }

    if (infoType & HDR_INFO_LUMINANCE) {
        ext_ctrl[i].id       = V4L2_CID_MPEG_VIDEO_SEI_MAX_DISPLAY_LUMINANCE;
        ext_ctrl[i + 1].id   = V4L2_CID_MPEG_VIDEO_SEI_MIN_DISPLAY_LUMINANCE;
        ext_ctrl[i + 2].id   = V4L2_CID_MPEG_VIDEO_SEI_WHITE_POINT;
        ext_ctrl[i + 3].id   = V4L2_CID_MPEG_VIDEO_SEI_DISPLAY_PRIMARIES_0;
        ext_ctrl[i + 4].id   = V4L2_CID_MPEG_VIDEO_SEI_DISPLAY_PRIMARIES_1;
        ext_ctrl[i + 5].id   = V4L2_CID_MPEG_VIDEO_SEI_DISPLAY_PRIMARIES_2;
        i += 6;
    }

    if (infoType & HDR_INFO_COLOR_ASPECTS) {
        if (pCtx->videoCtx.instInfo.eCodecType == VIDEO_CODING_VP9) {
            ext_ctrl[i].id          = V4L2_CID_MPEG_VIDEO_COLOUR_PRIMARIES;
            i++;
        } else {
            ext_ctrl[i].id          = V4L2_CID_MPEG_VIDEO_MATRIX_COEFFICIENTS;
            ext_ctrl[i + 1].id      = V4L2_CID_MPEG_VIDEO_COLOUR_PRIMARIES;
            ext_ctrl[i + 2].id      = V4L2_CID_MPEG_VIDEO_TRANSFER_CHARACTERISTICS;
            i += 3;
        }
    } else if (infoType & HDR_INFO_MATRIX_COEFF_ONLY) {
        ext_ctrl[i].id      = V4L2_CID_MPEG_VIDEO_MATRIX_COEFFICIENTS;
        i++;
    }

    if (infoType & HDR_INFO_RANGE) {
        ext_ctrl[i].id      = V4L2_CID_MPEG_VIDEO_FULL_RANGE_FLAG;
        i++;
    }

    ext_ctrls.count = i;

    if (exynos_v4l2_g_ext_ctrl(pCtx->videoCtx.hDevice, &ext_ctrls) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    i = 0;
    pHdrInfo->eChangedType = HDR_INFO_NO_CHANGES | (pHdrInfo->eChangedType & HDR_INFO_DYNAMIC_META);
    /* eType will be set when value is changed */
    if (infoType & HDR_INFO_LIGHT) {
        if ((pHdrInfo->sHdrStatic.max_pic_average_light != ext_ctrl[i].value) ||
            (pHdrInfo->sHdrStatic.max_content_light != ext_ctrl[i + 1].value)) {
            pHdrInfo->eChangedType |= HDR_INFO_LIGHT;
        }

        pHdrInfo->sHdrStatic.max_pic_average_light = ext_ctrl[i].value;
        pHdrInfo->sHdrStatic.max_content_light     = ext_ctrl[i + 1].value;
        i += 2;
    }

    if (infoType & HDR_INFO_LUMINANCE) {
        if ((pHdrInfo->sHdrStatic.max_display_luminance != ext_ctrl[i].value) ||
            (pHdrInfo->sHdrStatic.min_display_luminance != ext_ctrl[i + 1].value)   ||
            (pHdrInfo->sHdrStatic.white.x != GET_16BIT_HIGH(ext_ctrl[i + 2].value)) ||
            (pHdrInfo->sHdrStatic.white.y != GET_16BIT_LOW(ext_ctrl[i + 2].value))  ||
            (pHdrInfo->sHdrStatic.green.x   != GET_16BIT_HIGH(ext_ctrl[i + 3].value)) ||
            (pHdrInfo->sHdrStatic.green.y   != GET_16BIT_LOW(ext_ctrl[i + 3].value))  ||
            (pHdrInfo->sHdrStatic.blue.x != GET_16BIT_HIGH(ext_ctrl[i + 4].value)) ||
            (pHdrInfo->sHdrStatic.blue.y != GET_16BIT_LOW(ext_ctrl[i + 4].value))  ||
            (pHdrInfo->sHdrStatic.red.x  != GET_16BIT_HIGH(ext_ctrl[i + 5].value)) ||
            (pHdrInfo->sHdrStatic.red.y  != GET_16BIT_LOW(ext_ctrl[i + 5].value))) {
            pHdrInfo->eChangedType |= HDR_INFO_LUMINANCE;
        }

        pHdrInfo->sHdrStatic.max_display_luminance  = ext_ctrl[i].value;
        pHdrInfo->sHdrStatic.min_display_luminance  = ext_ctrl[i + 1].value;
        pHdrInfo->sHdrStatic.white.x                = GET_16BIT_HIGH(ext_ctrl[i + 2].value); /* [31:16] */
        pHdrInfo->sHdrStatic.white.y                = GET_16BIT_LOW(ext_ctrl[i + 2].value);  /* [15:0]  */
        pHdrInfo->sHdrStatic.green.x                = GET_16BIT_HIGH(ext_ctrl[i + 3].value);
        pHdrInfo->sHdrStatic.green.y                = GET_16BIT_LOW(ext_ctrl[i + 3].value);
        pHdrInfo->sHdrStatic.blue.x                 = GET_16BIT_HIGH(ext_ctrl[i + 4].value);
        pHdrInfo->sHdrStatic.blue.y                 = GET_16BIT_LOW(ext_ctrl[i + 4].value);
        pHdrInfo->sHdrStatic.red.x                  = GET_16BIT_HIGH(ext_ctrl[i + 5].value);
        pHdrInfo->sHdrStatic.red.y                  = GET_16BIT_LOW(ext_ctrl[i + 5].value);
        i += 6;
    }

    if (infoType & HDR_INFO_COLOR_ASPECTS) {
        if (pCtx->videoCtx.instInfo.eCodecType == VIDEO_CODING_VP9) {
            if (((int)pHdrInfo->sColorAspects.eCoeffType != ColorSpaceToColorAspects[ext_ctrl[i].value].coeff) ||
                ((int)pHdrInfo->sColorAspects.ePrimariesType != ColorSpaceToColorAspects[ext_ctrl[i].value].primaries) ||
                ((int)pHdrInfo->sColorAspects.eTransferType != ColorSpaceToColorAspects[ext_ctrl[i].value].transfer)) {
                pHdrInfo->eChangedType |= HDR_INFO_COLOR_ASPECTS;
            }

            pHdrInfo->sColorAspects.eCoeffType     = (ExynosMatrixCoeffType)ColorSpaceToColorAspects[ext_ctrl[i].value].coeff;
            pHdrInfo->sColorAspects.ePrimariesType = (ExynosPrimariesType)ColorSpaceToColorAspects[ext_ctrl[i].value].primaries;
            pHdrInfo->sColorAspects.eTransferType  = (ExynosTransferType)ColorSpaceToColorAspects[ext_ctrl[i].value].transfer;
            i++;
        } else {
            ExynosMatrixCoeffType   coeff       = (ExynosMatrixCoeffType)ext_ctrl[i].value;
            ExynosPrimariesType     primaries   = (ExynosPrimariesType)ext_ctrl[i + 1].value;
            ExynosTransferType      transfer    = (ExynosTransferType)ext_ctrl[i + 2].value;

            if ((coeff == MATRIX_COEFF_IDENTITY) ||
                (coeff == MATRIX_COEFF_UNSPECIFIED)) {
                coeff = pHdrInfo->sColorAspects.eCoeffType;
            }
            if ((primaries == PRIMARIES_RESERVED) ||
                (primaries == PRIMARIES_UNSPECIFIED)) {
                primaries = pHdrInfo->sColorAspects.ePrimariesType;
            }
            if ((transfer == TRANSFER_RESERVED) ||
                (transfer == TRANSFER_UNSPECIFIED)) {
                transfer = pHdrInfo->sColorAspects.eTransferType;
            }

            if ((pHdrInfo->sColorAspects.eCoeffType != coeff) ||
                (pHdrInfo->sColorAspects.ePrimariesType != primaries) ||
                (pHdrInfo->sColorAspects.eTransferType  != transfer)) {
                pHdrInfo->eChangedType |= HDR_INFO_COLOR_ASPECTS;
            }

            pHdrInfo->sColorAspects.eCoeffType     = coeff;
            pHdrInfo->sColorAspects.ePrimariesType = primaries;
            pHdrInfo->sColorAspects.eTransferType  = transfer;
            i += 3;
        }
    } else if (infoType & HDR_INFO_MATRIX_COEFF_ONLY) {
        if ((int)pHdrInfo->sColorAspects.eCoeffType != ext_ctrl[i].value) {
            pHdrInfo->eChangedType |= HDR_INFO_MATRIX_COEFF_ONLY;
        }

        pHdrInfo->sColorAspects.eCoeffType  = (ExynosMatrixCoeffType)ext_ctrl[i].value;
        i++;
    }

    if (infoType & HDR_INFO_RANGE) {
        ExynosRangeType eRangeType = RANGE_UNSPECIFIED;

        eRangeType = (ExynosRangeType)((ext_ctrl[i].value == 0)? RANGE_LIMITED:RANGE_FULL);

        if (pHdrInfo->sColorAspects.eRangeType != eRangeType) {
            pHdrInfo->eChangedType |= HDR_INFO_RANGE;
        }

        pHdrInfo->sColorAspects.eRangeType = eRangeType;
        i++;
    }

EXIT:
    return ret;
}

int Codec_OSAL_GetControls(
    CodecOSALVideoContext   *pCtx,
    unsigned int             nCID,
    void                    *pInfo) {
    int ret = 0;

    if ((pCtx == NULL) ||
        (pInfo == NULL) ||
        (pCtx->videoCtx.hDevice < 0)) {
        ret = -1;
        goto EXIT;
    }

    switch (nCID) {
    case CODEC_OSAL_CID_DEC_SEI_INFO:
        ret = Codec_OSAL_GetControls_SeiInfo(pCtx, pInfo);
        break;
    case CODEC_OSAL_CID_DEC_HDR_INFO:
        ret = Codec_OSAL_GetControls_HdrInfo(pCtx, pInfo);
        break;
    default:
    {
        ALOGE("%s: unsupported type(%x)", __FUNCTION__, nCID);
        ret = -1;
    }
        break;
    }

EXIT:
    return ret;
}

int Codec_OSAL_SetControls_EncParams_Avc(
    CodecOSALVideoContext   *pCtx,
    void                    *pInfo,
    struct v4l2_ext_control ext_ctrl[]) {
    int count = 0;

    ExynosVideoEncParam         *pEncParam      = (ExynosVideoEncParam *)pInfo;
    ExynosVideoEncCommonParam   *pCommonParam   = &pEncParam->common;

    int i;

    /* avc codec specific parameters */
    {
        ExynosVideoEncH264Param *pH264Param = &pEncParam->codec.h264;

        /* common parameters but id is depends on codec */
        ext_ctrl[8].id = V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP;
        ext_ctrl[8].value = pCommonParam->FrameQp;
        ext_ctrl[9].id =  V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP;
        ext_ctrl[9].value = pCommonParam->FrameQp_P;
        ext_ctrl[10].id =  V4L2_CID_MPEG_VIDEO_H264_MAX_QP;  /* QP range : I frame */
        ext_ctrl[10].value = GET_H264_QP_VALUE(pCommonParam->QpRange.QpMax_I);
        ext_ctrl[11].id = V4L2_CID_MPEG_VIDEO_H264_MIN_QP;
        ext_ctrl[11].value = GET_H264_QP_VALUE(pCommonParam->QpRange.QpMin_I);
        ext_ctrl[12].id = V4L2_CID_MPEG_MFC51_VIDEO_RC_REACTION_COEFF;
        ext_ctrl[12].value = pCommonParam->CBRPeriodRf;

        /* H.264 specific parameters */
        switch (pCommonParam->SliceMode) {
        case 0:
            ext_ctrl[13].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB;
            ext_ctrl[13].value = 1;  /* default */
            ext_ctrl[14].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_BYTES;
            ext_ctrl[14].value = 2800; /* based on MFC6.x */
            break;
        case 1:
            ext_ctrl[13].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB;
            ext_ctrl[13].value = pH264Param->SliceArgument;
            ext_ctrl[14].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_BYTES;
            ext_ctrl[14].value = 2800; /* based on MFC6.x */
            break;
        case 2:
            ext_ctrl[13].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB;
            ext_ctrl[13].value = 1; /* default */
            ext_ctrl[14].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_BYTES;
            ext_ctrl[14].value = pH264Param->SliceArgument;
            break;
        default:
            break;
        }

        ext_ctrl[15].id = V4L2_CID_MPEG_VIDEO_H264_PROFILE;
        ext_ctrl[15].value = pH264Param->ProfileIDC;
        ext_ctrl[16].id = V4L2_CID_MPEG_VIDEO_H264_LEVEL;
        ext_ctrl[16].value = pH264Param->LevelIDC;
        ext_ctrl[17].id = V4L2_CID_MPEG_MFC51_VIDEO_H264_NUM_REF_PIC_FOR_P;
        ext_ctrl[17].value = pH264Param->NumberRefForPframes;
        /*
         * It should be set using h264Param->NumberBFrames after being handled by appl.
         */
        ext_ctrl[18].id =  V4L2_CID_MPEG_VIDEO_B_FRAMES;
        ext_ctrl[18].value = pH264Param->NumberBFrames;
        ext_ctrl[19].id = V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_MODE;
        ext_ctrl[19].value = pH264Param->LoopFilterDisable;
        ext_ctrl[20].id = V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_ALPHA;
        ext_ctrl[20].value = pH264Param->LoopFilterAlphaC0Offset;
        ext_ctrl[21].id = V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_BETA;
        ext_ctrl[21].value = pH264Param->LoopFilterBetaOffset;
        ext_ctrl[22].id = V4L2_CID_MPEG_VIDEO_H264_ENTROPY_MODE;
        ext_ctrl[22].value = pH264Param->SymbolMode;
        ext_ctrl[23].id = V4L2_CID_MPEG_MFC51_VIDEO_H264_INTERLACE;
        ext_ctrl[23].value = pH264Param->PictureInterlace;
        ext_ctrl[24].id = V4L2_CID_MPEG_VIDEO_H264_8X8_TRANSFORM;
        ext_ctrl[24].value = pH264Param->Transform8x8Mode;
        ext_ctrl[25].id = V4L2_CID_MPEG_MFC51_VIDEO_H264_RC_FRAME_RATE;
        ext_ctrl[25].value = pH264Param->FrameRate;
        ext_ctrl[26].id =  V4L2_CID_MPEG_VIDEO_H264_B_FRAME_QP;
        ext_ctrl[26].value = pH264Param->FrameQp_B;
        ext_ctrl[27].id =  V4L2_CID_MPEG_MFC51_VIDEO_H264_ADAPTIVE_RC_DARK;
        ext_ctrl[27].value = pH264Param->DarkDisable;
        ext_ctrl[28].id =  V4L2_CID_MPEG_MFC51_VIDEO_H264_ADAPTIVE_RC_SMOOTH;
        ext_ctrl[28].value = pH264Param->SmoothDisable;
        ext_ctrl[29].id =  V4L2_CID_MPEG_MFC51_VIDEO_H264_ADAPTIVE_RC_STATIC;
        ext_ctrl[29].value = pH264Param->StaticDisable;
        ext_ctrl[30].id =  V4L2_CID_MPEG_MFC51_VIDEO_H264_ADAPTIVE_RC_ACTIVITY;
        ext_ctrl[30].value = pH264Param->ActivityDisable;

        /* doesn't have to be set */
        ext_ctrl[31].id = V4L2_CID_MPEG_VIDEO_GOP_CLOSURE;
        ext_ctrl[31].value = 1;
        ext_ctrl[32].id = V4L2_CID_MPEG_VIDEO_H264_I_PERIOD;
        ext_ctrl[32].value = 0;
        ext_ctrl[33].id = V4L2_CID_MPEG_VIDEO_VBV_SIZE;
        ext_ctrl[33].value = 0;
        ext_ctrl[34].id = V4L2_CID_MPEG_VIDEO_HEADER_MODE;

        if (pH264Param->HeaderWithIFrame == 0) {
            /* default */
            ext_ctrl[34].value = V4L2_MPEG_VIDEO_HEADER_MODE_SEPARATE;              /* 0: seperated header */
        } else {
            ext_ctrl[34].value = V4L2_MPEG_VIDEO_HEADER_MODE_JOINED_WITH_1ST_FRAME; /* 1: header + first frame */
        }
        ext_ctrl[35].id =  V4L2_CID_MPEG_VIDEO_H264_VUI_SAR_ENABLE;
        ext_ctrl[35].value = pH264Param->SarEnable;
        ext_ctrl[36].id = V4L2_CID_MPEG_VIDEO_H264_VUI_SAR_IDC;
        ext_ctrl[36].value = pH264Param->SarIndex;
        ext_ctrl[37].id = V4L2_CID_MPEG_VIDEO_H264_VUI_EXT_SAR_WIDTH;
        ext_ctrl[37].value = pH264Param->SarWidth;
        ext_ctrl[38].id =  V4L2_CID_MPEG_VIDEO_H264_VUI_EXT_SAR_HEIGHT;
        ext_ctrl[38].value = pH264Param->SarHeight;

        switch (pCommonParam->FrameSkip) {
        case VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT:
            ext_ctrl[39].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[39].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT;
            break;
        case VIDEO_FRAME_SKIP_MODE_BUF_LIMIT:
            ext_ctrl[39].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[39].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_BUF_LIMIT;
            break;
        default:
            /* VIDEO_FRAME_SKIP_MODE_DISABLE (default) */
            ext_ctrl[39].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[39].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_DISABLED;
            break;
        }

        /* SVC is not supported yet */
        ext_ctrl[40].id =  V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING;
        ext_ctrl[40].value = 0;
        switch (pH264Param->HierarType) {
        case VIDEO_HIERARCHICAL_B:
            ext_ctrl[41].id =  V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_TYPE;
            ext_ctrl[41].value = V4L2_MPEG_VIDEO_H264_HIERARCHICAL_CODING_B;
            break;
        case VIDEO_HIERARCHICAL_P:
        default:
            ext_ctrl[41].id =  V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_TYPE;
            ext_ctrl[41].value = V4L2_MPEG_VIDEO_H264_HIERARCHICAL_CODING_P;
            break;
        }
        ext_ctrl[42].id =  V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER;
        ext_ctrl[42].value = pH264Param->TemporalSVC.nCount;
        ext_ctrl[43].id =  V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_QP;
        ext_ctrl[43].value = ((unsigned int)0 << 16 | 29);
        ext_ctrl[44].id =  V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_QP;
        ext_ctrl[44].value = ((unsigned int)1 << 16 | 29);
        ext_ctrl[45].id =  V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_QP;
        ext_ctrl[45].value = ((unsigned int)2 << 16 | 29);

        ext_ctrl[46].id =  V4L2_CID_MPEG_VIDEO_H264_SEI_FRAME_PACKING;
        ext_ctrl[46].value = 0;
        ext_ctrl[47].id =  V4L2_CID_MPEG_VIDEO_H264_SEI_FP_CURRENT_FRAME_0;
        ext_ctrl[47].value = 0;
        ext_ctrl[48].id =  V4L2_CID_MPEG_VIDEO_H264_SEI_FP_ARRANGEMENT_TYPE;
        ext_ctrl[48].value = V4L2_MPEG_VIDEO_H264_SEI_FP_TYPE_SIDE_BY_SIDE;

        /* FMO is not supported yet */
        ext_ctrl[49].id =  V4L2_CID_MPEG_VIDEO_H264_FMO;
        ext_ctrl[49].value = 0;
        ext_ctrl[50].id =  V4L2_CID_MPEG_VIDEO_H264_FMO_MAP_TYPE;
        ext_ctrl[50].value = V4L2_MPEG_VIDEO_H264_FMO_MAP_TYPE_INTERLEAVED_SLICES;
        ext_ctrl[51].id = V4L2_CID_MPEG_VIDEO_H264_FMO_SLICE_GROUP;
        ext_ctrl[51].value = 4;
        ext_ctrl[52].id =  V4L2_CID_MPEG_VIDEO_H264_FMO_RUN_LENGTH;
        ext_ctrl[52].value = ((unsigned int)0 << 30 | 0);
        ext_ctrl[53].id =  V4L2_CID_MPEG_VIDEO_H264_FMO_RUN_LENGTH;
        ext_ctrl[53].value = ((unsigned int)1 << 30 | 0);
        ext_ctrl[54].id =  V4L2_CID_MPEG_VIDEO_H264_FMO_RUN_LENGTH;
        ext_ctrl[54].value = ((unsigned int)2 << 30 | 0);
        ext_ctrl[55].id =  V4L2_CID_MPEG_VIDEO_H264_FMO_RUN_LENGTH;
        ext_ctrl[55].value = ((unsigned int)3 << 30 | 0);
        ext_ctrl[56].id =  V4L2_CID_MPEG_VIDEO_H264_FMO_CHANGE_DIRECTION;
        ext_ctrl[56].value = V4L2_MPEG_VIDEO_H264_FMO_CHANGE_DIR_RIGHT;
        ext_ctrl[57].id = V4L2_CID_MPEG_VIDEO_H264_FMO_CHANGE_RATE;
        ext_ctrl[57].value = 0;

        /* ASO is not supported yet */
        ext_ctrl[58].id =  V4L2_CID_MPEG_VIDEO_H264_ASO;
        ext_ctrl[58].value = 0;
        for (i = 0; i < 32; i++) {
            ext_ctrl[59 + i].id =  V4L2_CID_MPEG_VIDEO_H264_ASO_SLICE_ORDER;
            ext_ctrl[59 + i].value = ((unsigned int)i << 16 | 0);
        }

        /* VUI RESRICTION ENABLE */
        ext_ctrl[91].id    = V4L2_CID_MPEG_MFC_H264_VUI_RESTRICTION_ENABLE;
        ext_ctrl[91].value = pH264Param->VuiRestrictionEnable;

        count = H264_CTRL_NUM;

        if (pCtx->videoCtx.instInfo.supportInfo.enc.bTemporalSvcSupport == VIDEO_TRUE) {
            i = count;
            ext_ctrl[i].id          = V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_BIT0;
            ext_ctrl[i].value       = pH264Param->TemporalSVC.nBitrateRatio[0];
            ext_ctrl[i + 1].id      = V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_BIT1;
            ext_ctrl[i + 1].value   = pH264Param->TemporalSVC.nBitrateRatio[1];
            ext_ctrl[i + 2].id      = V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_BIT2;
            ext_ctrl[i + 2].value   = pH264Param->TemporalSVC.nBitrateRatio[2];
            ext_ctrl[i + 3].id      = V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_BIT3;
            ext_ctrl[i + 3].value   = pH264Param->TemporalSVC.nBitrateRatio[3];
            ext_ctrl[i + 4].id      = V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_BIT4;
            ext_ctrl[i + 4].value   = pH264Param->TemporalSVC.nBitrateRatio[4];
            ext_ctrl[i + 5].id      = V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_BIT5;
            ext_ctrl[i + 5].value   = pH264Param->TemporalSVC.nBitrateRatio[5];
            ext_ctrl[i + 6].id      = V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_BIT6;
            ext_ctrl[i + 6].value   = pH264Param->TemporalSVC.nBitrateRatio[6];
            count += 7;
        }

        if (pCtx->videoCtx.instInfo.supportInfo.enc.bSkypeSupport == VIDEO_TRUE) {
            i = count;

            /* H264 LTR FRAMES (0: disable, LTRFrames > 0: enable) */
            ext_ctrl[i].id      = V4L2_CID_MPEG_MFC_H264_NUM_OF_LTR;
            ext_ctrl[i].value   = pH264Param->LTRFrames;

            /* FRAME LEVEL QP ENABLE */
            ext_ctrl[i + 1].id      = V4L2_CID_MPEG_MFC_CONFIG_QP_ENABLE;
            ext_ctrl[i + 1].value   = pCommonParam->EnableFRMQpControl;

            /* CONFIG QP VALUE */
            ext_ctrl[i + 2].id      = V4L2_CID_MPEG_MFC_CONFIG_QP;
            ext_ctrl[i + 2].value   = pCommonParam->FrameQp;

            /* MAX LAYER COUNT for SHORT TERM */
            ext_ctrl[i + 3].id      = V4L2_CID_MPEG_VIDEO_TEMPORAL_SHORTTERM_MAX_LAYER;
            ext_ctrl[i + 3].value   = pH264Param->MaxTemporalLayerCount;

            count += 4;
        }

        if (pCtx->videoCtx.instInfo.supportInfo.enc.bRoiInfoSupport == VIDEO_TRUE) {
            i = count;
            ext_ctrl[i].id     = V4L2_CID_MPEG_VIDEO_ROI_ENABLE;
            ext_ctrl[i].value  = pH264Param->ROIEnable;

            count += 1;
        }

        /* optional : if these are not set, set value are same as I frame */
        if (pCtx->videoCtx.instInfo.supportInfo.enc.bQpRangePBSupport == VIDEO_TRUE) {
            i = count;
            ext_ctrl[i].id          = V4L2_CID_MPEG_VIDEO_H264_MIN_QP_P;  /* P frame */
            ext_ctrl[i].value       = GET_H264_QP_VALUE(pCommonParam->QpRange.QpMin_P);
            ext_ctrl[i + 1].id      =  V4L2_CID_MPEG_VIDEO_H264_MAX_QP_P;
            ext_ctrl[i + 1].value   = GET_H264_QP_VALUE(pCommonParam->QpRange.QpMax_P);
            ext_ctrl[i + 2].id      = V4L2_CID_MPEG_VIDEO_H264_MIN_QP_B;  /* B frame */
            ext_ctrl[i + 2].value   = GET_H264_QP_VALUE(pCommonParam->QpRange.QpMin_B);
            ext_ctrl[i + 3].id      =  V4L2_CID_MPEG_VIDEO_H264_MAX_QP_B;
            ext_ctrl[i + 3].value   = GET_H264_QP_VALUE(pCommonParam->QpRange.QpMax_B);

            count += 4;
        }

        if (pCtx->videoCtx.instInfo.supportInfo.enc.bPVCSupport == VIDEO_TRUE) {
            i = count;
            ext_ctrl[i].id = V4L2_CID_MPEG_VIDEO_RC_PVC_ENABLE;
            ext_ctrl[i].value = pCommonParam->PerceptualMode;

            count += 1;
        }

        if (pCtx->videoCtx.instInfo.supportInfo.enc.bChromaQpSupport == VIDEO_TRUE) {
            i = count;
            ext_ctrl[i].id          = V4L2_CID_MPEG_VIDEO_CHROMA_QP_OFFSET_CB;
            ext_ctrl[i].value       = pH264Param->chromaQPOffset.Cb;
            ext_ctrl[i + 1].id      = V4L2_CID_MPEG_VIDEO_CHROMA_QP_OFFSET_CR;
            ext_ctrl[i + 1].value   = pH264Param->chromaQPOffset.Cr;

            count +=2;
        }
    }

    return count;
}

int Codec_OSAL_SetControls_EncParams_Mpeg4(
    CodecOSALVideoContext   *pCtx,
    void                    *pInfo,
    struct v4l2_ext_control ext_ctrl[]) {
    int count = 0;

    ExynosVideoEncParam         *pEncParam      = (ExynosVideoEncParam *)pInfo;
    ExynosVideoEncCommonParam   *pCommonParam   = &pEncParam->common;

    int i;

    /* mpeg4 codec specific parameters */
    {
        ExynosVideoEncMpeg4Param *pMpeg4Param = &pEncParam->codec.mpeg4;

        /* common parameters but id is depends on codec */
        ext_ctrl[8].id = V4L2_CID_MPEG_VIDEO_MPEG4_I_FRAME_QP;
        ext_ctrl[8].value = pCommonParam->FrameQp;
        ext_ctrl[9].id =  V4L2_CID_MPEG_VIDEO_MPEG4_P_FRAME_QP;
        ext_ctrl[9].value = pCommonParam->FrameQp_P;
        ext_ctrl[10].id =  V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP;  /* I frame */
        ext_ctrl[10].value = GET_MPEG4_QP_VALUE(pCommonParam->QpRange.QpMax_I);
        ext_ctrl[11].id = V4L2_CID_MPEG_VIDEO_MPEG4_MIN_QP;
        ext_ctrl[11].value = GET_MPEG4_QP_VALUE(pCommonParam->QpRange.QpMin_I);
        ext_ctrl[12].id = V4L2_CID_MPEG_MFC51_VIDEO_RC_REACTION_COEFF;
        ext_ctrl[12].value = pCommonParam->CBRPeriodRf;

        /* MPEG4 specific parameters */
        switch (pCommonParam->SliceMode) {
        case 0:
            ext_ctrl[13].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB;
            ext_ctrl[13].value = 1;  /* default */
            ext_ctrl[14].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_BYTES;
            ext_ctrl[14].value = 2800; /* based on MFC6.x */
            break;
        case 1:
            ext_ctrl[13].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB;
            ext_ctrl[13].value = pMpeg4Param->SliceArgument;
            ext_ctrl[14].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_BYTES;
            ext_ctrl[14].value = 2800; /* based on MFC6.x */
            break;
        case 2:
            ext_ctrl[13].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB;
            ext_ctrl[13].value = 1; /* default */
            ext_ctrl[14].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_BYTES;
            ext_ctrl[14].value = pMpeg4Param->SliceArgument;
            break;
        default:
            break;
        }

        ext_ctrl[15].id = V4L2_CID_MPEG_VIDEO_MPEG4_PROFILE;
        ext_ctrl[15].value = pMpeg4Param->ProfileIDC;
        ext_ctrl[16].id = V4L2_CID_MPEG_VIDEO_MPEG4_LEVEL;
        ext_ctrl[16].value = pMpeg4Param->LevelIDC;
        ext_ctrl[17].id = V4L2_CID_MPEG_VIDEO_MPEG4_QPEL;
        ext_ctrl[17].value = pMpeg4Param->DisableQpelME;

        /*
         * It should be set using mpeg4Param->NumberBFrames after being handled by appl.
         */
        ext_ctrl[18].id =  V4L2_CID_MPEG_VIDEO_B_FRAMES;
        ext_ctrl[18].value = pMpeg4Param->NumberBFrames;

        ext_ctrl[19].id = V4L2_CID_MPEG_MFC51_VIDEO_MPEG4_VOP_TIME_RES;
        ext_ctrl[19].value = pMpeg4Param->TimeIncreamentRes;
        ext_ctrl[20].id = V4L2_CID_MPEG_MFC51_VIDEO_MPEG4_VOP_FRM_DELTA;
        ext_ctrl[20].value = pMpeg4Param->VopTimeIncreament;
        ext_ctrl[21].id =  V4L2_CID_MPEG_VIDEO_MPEG4_B_FRAME_QP;
        ext_ctrl[21].value = pMpeg4Param->FrameQp_B;
        ext_ctrl[22].id = V4L2_CID_MPEG_VIDEO_VBV_SIZE;
        ext_ctrl[22].value = 0;
        ext_ctrl[23].id = V4L2_CID_MPEG_VIDEO_HEADER_MODE;
        ext_ctrl[23].value = V4L2_MPEG_VIDEO_HEADER_MODE_SEPARATE;
        ext_ctrl[24].id = V4L2_CID_MPEG_MFC51_VIDEO_RC_FIXED_TARGET_BIT;
        ext_ctrl[24].value = 1;

        switch (pCommonParam->FrameSkip) {
        case VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT:
            ext_ctrl[25].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[25].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT;
            break;
        case VIDEO_FRAME_SKIP_MODE_BUF_LIMIT:
            ext_ctrl[25].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[25].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_BUF_LIMIT;
            break;
        default:
            /* VIDEO_FRAME_SKIP_MODE_DISABLE (default) */
            ext_ctrl[25].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[25].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_DISABLED;
            break;
        }
        count = MPEG4_CTRL_NUM;

        /* optional : if these are not set, set value are same as I frame */
        if (pCtx->videoCtx.instInfo.supportInfo.enc.bQpRangePBSupport == VIDEO_TRUE) {
            i = count;
            ext_ctrl[i].id = V4L2_CID_MPEG_VIDEO_MPEG4_MIN_QP_P;  /* P frame */
            ext_ctrl[i].value = GET_MPEG4_QP_VALUE(pCommonParam->QpRange.QpMin_P);
            ext_ctrl[i + 1].id =  V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP_P;
            ext_ctrl[i + 1].value = GET_MPEG4_QP_VALUE(pCommonParam->QpRange.QpMax_P);
            ext_ctrl[i + 2].id = V4L2_CID_MPEG_VIDEO_MPEG4_MIN_QP_B;  /* B frame */
            ext_ctrl[i + 2].value = GET_MPEG4_QP_VALUE(pCommonParam->QpRange.QpMin_B);
            ext_ctrl[i + 3].id =  V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP_B;
            ext_ctrl[i + 3].value = GET_MPEG4_QP_VALUE(pCommonParam->QpRange.QpMax_B);

            count += 4;
        }
    }

    return count;
}

int Codec_OSAL_SetControls_EncParams_H263(
    CodecOSALVideoContext   *pCtx,
    void                    *pInfo,
    struct v4l2_ext_control ext_ctrl[]) {
    int count = 0;

    ExynosVideoEncParam         *pEncParam      = (ExynosVideoEncParam *)pInfo;
    ExynosVideoEncCommonParam   *pCommonParam   = &pEncParam->common;

    int i;

    /* h.263 codec specific parameters */
    {
        ExynosVideoEncH263Param *pH263Param = &pEncParam->codec.h263;

        /* common parameters but id is depends on codec */
        ext_ctrl[8].id = V4L2_CID_MPEG_VIDEO_H263_I_FRAME_QP;
        ext_ctrl[8].value = pCommonParam->FrameQp;
        ext_ctrl[9].id =  V4L2_CID_MPEG_VIDEO_H263_P_FRAME_QP;
        ext_ctrl[9].value = pCommonParam->FrameQp_P;
        ext_ctrl[10].id =  V4L2_CID_MPEG_VIDEO_H263_MAX_QP;  /* I frame */
        ext_ctrl[10].value = GET_H263_QP_VALUE(pCommonParam->QpRange.QpMax_I);
        ext_ctrl[11].id = V4L2_CID_MPEG_VIDEO_H263_MIN_QP;
        ext_ctrl[11].value = GET_H263_QP_VALUE(pCommonParam->QpRange.QpMin_I);
        ext_ctrl[12].id = V4L2_CID_MPEG_MFC51_VIDEO_RC_REACTION_COEFF;
        ext_ctrl[12].value = pCommonParam->CBRPeriodRf;

        /* H263 specific parameters */
        ext_ctrl[13].id = V4L2_CID_MPEG_MFC51_VIDEO_H263_RC_FRAME_RATE;
        ext_ctrl[13].value = pH263Param->FrameRate;
        ext_ctrl[14].id = V4L2_CID_MPEG_VIDEO_VBV_SIZE;
        ext_ctrl[14].value = 0;
        ext_ctrl[15].id = V4L2_CID_MPEG_VIDEO_HEADER_MODE;
        ext_ctrl[15].value = V4L2_MPEG_VIDEO_HEADER_MODE_SEPARATE;
        ext_ctrl[16].id = V4L2_CID_MPEG_MFC51_VIDEO_RC_FIXED_TARGET_BIT;
        ext_ctrl[16].value = 1;

        switch (pCommonParam->FrameSkip) {
        case VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT:
            ext_ctrl[17].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[17].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT;
            break;
        case VIDEO_FRAME_SKIP_MODE_BUF_LIMIT:
            ext_ctrl[17].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[17].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_BUF_LIMIT;
            break;
        default:
            /* VIDEO_FRAME_SKIP_MODE_DISABLE (default) */
            ext_ctrl[17].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[17].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_DISABLED;
            break;
        }
        count = H263_CTRL_NUM;

        /* optional : if these are not set, set value are same as I frame */
        if (pCtx->videoCtx.instInfo.supportInfo.enc.bQpRangePBSupport == VIDEO_TRUE) {
            i = count;
            ext_ctrl[i].id = V4L2_CID_MPEG_VIDEO_H263_MIN_QP_P;  /* P frame */
            ext_ctrl[i].value = GET_H263_QP_VALUE(pCommonParam->QpRange.QpMin_P);
            ext_ctrl[i + 1].id =  V4L2_CID_MPEG_VIDEO_H263_MAX_QP_P;
            ext_ctrl[i + 1].value = GET_H263_QP_VALUE(pCommonParam->QpRange.QpMax_P);

            count += 2;
        }
    }

    return count;
}

int Codec_OSAL_SetControls_EncParams_Vp8(
    CodecOSALVideoContext   *pCtx,
    void                    *pInfo,
    struct v4l2_ext_control ext_ctrl[]) {
    int count = 0;

    ExynosVideoEncParam         *pEncParam      = (ExynosVideoEncParam *)pInfo;
    ExynosVideoEncCommonParam   *pCommonParam   = &pEncParam->common;

    int i;

    /* vp8 codec specific parameters */
    {
        ExynosVideoEncVp8Param *pVp8Param = &pEncParam->codec.vp8;

        /* common parameters but id is depends on codec */
        ext_ctrl[8].id = V4L2_CID_MPEG_VIDEO_VP8_I_FRAME_QP;
        ext_ctrl[8].value = pCommonParam->FrameQp;
        ext_ctrl[9].id =  V4L2_CID_MPEG_VIDEO_VP8_P_FRAME_QP;
        ext_ctrl[9].value = pCommonParam->FrameQp_P;
        ext_ctrl[10].id =  V4L2_CID_MPEG_VIDEO_VP8_MAX_QP;  /* I frame */
        ext_ctrl[10].value = GET_VP8_QP_VALUE(pCommonParam->QpRange.QpMax_I);
        ext_ctrl[11].id = V4L2_CID_MPEG_VIDEO_VP8_MIN_QP;
        ext_ctrl[11].value = GET_VP8_QP_VALUE(pCommonParam->QpRange.QpMin_I);
        ext_ctrl[12].id = V4L2_CID_MPEG_MFC51_VIDEO_RC_REACTION_COEFF;
        ext_ctrl[12].value = pCommonParam->CBRPeriodRf;

        /* VP8 specific parameters */
        ext_ctrl[13].id = V4L2_CID_MPEG_MFC70_VIDEO_VP8_RC_FRAME_RATE;
        ext_ctrl[13].value = pVp8Param->VpX.FrameRate;
        ext_ctrl[14].id = V4L2_CID_MPEG_VIDEO_VBV_SIZE;
        ext_ctrl[14].value = 0;
        ext_ctrl[15].id = V4L2_CID_MPEG_VIDEO_HEADER_MODE;
        ext_ctrl[15].value = V4L2_MPEG_VIDEO_HEADER_MODE_SEPARATE;
        ext_ctrl[16].id = V4L2_CID_MPEG_MFC51_VIDEO_RC_FIXED_TARGET_BIT;
        ext_ctrl[16].value = 1;

        switch (pCommonParam->FrameSkip) {
        case VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT:
            ext_ctrl[17].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[17].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT;
            break;
        case VIDEO_FRAME_SKIP_MODE_BUF_LIMIT:
            ext_ctrl[17].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[17].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_BUF_LIMIT;
            break;
        default:
            /* VIDEO_FRAME_SKIP_MODE_DISABLE (default) */
            ext_ctrl[17].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[17].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_DISABLED;
            break;
        }

        ext_ctrl[18].id = V4L2_CID_MPEG_MFC70_VIDEO_VP8_VERSION;
        ext_ctrl[18].value = pVp8Param->Vp8Version;

        ext_ctrl[19].id = V4L2_CID_MPEG_MFC70_VIDEO_VP8_NUM_OF_PARTITIONS;
        ext_ctrl[19].value = pVp8Param->Vp8NumberOfPartitions;

        ext_ctrl[20].id = V4L2_CID_MPEG_MFC70_VIDEO_VP8_FILTER_LEVEL;
        ext_ctrl[20].value = pVp8Param->Vp8FilterLevel;

        ext_ctrl[21].id = V4L2_CID_MPEG_MFC70_VIDEO_VP8_FILTER_SHARPNESS;
        ext_ctrl[21].value = pVp8Param->Vp8FilterSharpness;

        ext_ctrl[22].id = V4L2_CID_MPEG_MFC70_VIDEO_VP8_GOLDEN_FRAMESEL;
        ext_ctrl[22].value = pVp8Param->VpX.VpXGoldenFrameSel;

        ext_ctrl[23].id = V4L2_CID_MPEG_MFC70_VIDEO_VP8_HIERARCHY_QP_ENABLE;
        ext_ctrl[23].value = 0;

        ext_ctrl[24].id = V4L2_CID_MPEG_MFC70_VIDEO_VP8_HIERARCHY_QP_LAYER0;
        ext_ctrl[24].value = ((unsigned int)0 << 16 | 37);

        ext_ctrl[25].id = V4L2_CID_MPEG_MFC70_VIDEO_VP8_HIERARCHY_QP_LAYER1;
        ext_ctrl[25].value = ((unsigned int)1 << 16 | 37);

        ext_ctrl[26].id = V4L2_CID_MPEG_MFC70_VIDEO_VP8_HIERARCHY_QP_LAYER2;
        ext_ctrl[26].value = ((unsigned int)2 << 16 | 37);

        ext_ctrl[27].id = V4L2_CID_MPEG_MFC70_VIDEO_VP8_REF_NUMBER_FOR_PFRAMES;
        ext_ctrl[27].value = pVp8Param->VpX.RefNumberForPFrame;

        ext_ctrl[28].id = V4L2_CID_MPEG_MFC70_VIDEO_VP8_DISABLE_INTRA_MD4X4;
        ext_ctrl[28].value = pVp8Param->DisableIntraMd4x4;

        ext_ctrl[29].id = V4L2_CID_MPEG_MFC70_VIDEO_VP8_NUM_TEMPORAL_LAYER;
        ext_ctrl[29].value = pVp8Param->VpX.TemporalSVC.nCount;
        count = VP8_CTRL_NUM;

        if (pCtx->videoCtx.instInfo.supportInfo.enc.bTemporalSvcSupport == VIDEO_TRUE) {
            i = count;
            ext_ctrl[i].id         = V4L2_CID_MPEG_VIDEO_VP8_HIERARCHICAL_CODING_LAYER_BIT0;
            ext_ctrl[i].value      = pVp8Param->VpX.TemporalSVC.nBitrateRatio[0];
            ext_ctrl[i + 1].id     = V4L2_CID_MPEG_VIDEO_VP8_HIERARCHICAL_CODING_LAYER_BIT1;
            ext_ctrl[i + 1].value  = pVp8Param->VpX.TemporalSVC.nBitrateRatio[1];
            ext_ctrl[i + 2].id     = V4L2_CID_MPEG_VIDEO_VP8_HIERARCHICAL_CODING_LAYER_BIT2;
            ext_ctrl[i + 2].value  = pVp8Param->VpX.TemporalSVC.nBitrateRatio[2];

            count += 3;
        }

        /* optional : if these are not set, set value are same as I frame */
        if (pCtx->videoCtx.instInfo.supportInfo.enc.bQpRangePBSupport == VIDEO_TRUE) {
            i = count;
            ext_ctrl[i].id = V4L2_CID_MPEG_VIDEO_VP8_MIN_QP_P;  /* P frame */
            ext_ctrl[i].value = GET_VP8_QP_VALUE(pCommonParam->QpRange.QpMin_P);
            ext_ctrl[i + 1].id =  V4L2_CID_MPEG_VIDEO_VP8_MAX_QP_P;
            ext_ctrl[i + 1].value = GET_VP8_QP_VALUE(pCommonParam->QpRange.QpMax_P);

            count += 2;
        }

        if (pCtx->videoCtx.instInfo.supportInfo.enc.bPVCSupport == VIDEO_TRUE) {
            i = count;
            ext_ctrl[i].id = V4L2_CID_MPEG_VIDEO_RC_PVC_ENABLE;
            ext_ctrl[i].value = pCommonParam->PerceptualMode;

            count += 1;
        }
    }

    return count;
}

int Codec_OSAL_SetControls_EncParams_Hevc(
    CodecOSALVideoContext   *pCtx,
    void                    *pInfo,
    struct v4l2_ext_control ext_ctrl[]) {
    int count = 0;

    ExynosVideoEncParam         *pEncParam      = (ExynosVideoEncParam *)pInfo;
    ExynosVideoEncCommonParam   *pCommonParam   = &pEncParam->common;

    int i;

    /* hevc codec specific parameters */
    {
        ExynosVideoEncHevcParam *pHevcParam = &pEncParam->codec.hevc;

        /* common parameters but id is depends on codec */
        ext_ctrl[8].id = V4L2_CID_MPEG_VIDEO_HEVC_I_FRAME_QP;
        ext_ctrl[8].value = pCommonParam->FrameQp;
        ext_ctrl[9].id =  V4L2_CID_MPEG_VIDEO_HEVC_P_FRAME_QP;
        ext_ctrl[9].value = pCommonParam->FrameQp_P;
        ext_ctrl[10].id =  V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP;  /* I frame */
        ext_ctrl[10].value = GET_HEVC_QP_VALUE(pCommonParam->QpRange.QpMax_I,
                                            pCtx->videoCtx.instInfo.HwVersion);
        ext_ctrl[11].id = V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP;
        ext_ctrl[11].value = GET_HEVC_QP_VALUE(pCommonParam->QpRange.QpMin_I,
                                            pCtx->videoCtx.instInfo.HwVersion);
        ext_ctrl[12].id = V4L2_CID_MPEG_MFC51_VIDEO_RC_REACTION_COEFF;
        ext_ctrl[12].value = pCommonParam->CBRPeriodRf;

        /* HEVC specific parameters */
        ext_ctrl[13].id =  V4L2_CID_MPEG_VIDEO_HEVC_B_FRAME_QP;
        ext_ctrl[13].value = pHevcParam->FrameQp_B;
        ext_ctrl[14].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_RC_FRAME_RATE;
        ext_ctrl[14].value = pHevcParam->FrameRate;

        switch (pCommonParam->FrameSkip) {
        case VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT:
            ext_ctrl[15].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[15].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT;
            break;
        case VIDEO_FRAME_SKIP_MODE_BUF_LIMIT:
            ext_ctrl[15].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[15].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_BUF_LIMIT;
            break;
        default:
            /* VIDEO_FRAME_SKIP_MODE_DISABLE (default) */
            ext_ctrl[15].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[15].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_DISABLED;
            break;
        }

        ext_ctrl[16].id = V4L2_CID_MPEG_VIDEO_HEVC_PROFILE;
        ext_ctrl[16].value = pHevcParam->ProfileIDC;

        ext_ctrl[17].id = V4L2_CID_MPEG_VIDEO_HEVC_LEVEL;
        ext_ctrl[17].value = pHevcParam->LevelIDC;

        ext_ctrl[18].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_TIER_FLAG;
        ext_ctrl[18].value = pHevcParam->TierIDC;

        ext_ctrl[19].id =  V4L2_CID_MPEG_VIDEO_B_FRAMES;
        ext_ctrl[19].value = pHevcParam->NumberBFrames;

        ext_ctrl[20].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_MAX_PARTITION_DEPTH;
        ext_ctrl[20].value = pHevcParam->MaxPartitionDepth;

        ext_ctrl[21].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_REF_NUMBER_FOR_PFRAMES;
        ext_ctrl[21].value = pHevcParam->NumberRefForPframes;

        ext_ctrl[22].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_LF_DISABLE;
        ext_ctrl[22].value = pHevcParam->LoopFilterDisable;

        ext_ctrl[23].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_LF_SLICE_BOUNDARY;
        ext_ctrl[23].value = pHevcParam->LoopFilterSliceFlag;

        ext_ctrl[24].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_LF_TC_OFFSET_DIV2;
        ext_ctrl[24].value = pHevcParam->LoopFilterTcOffset;

        ext_ctrl[25].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_LF_BETA_OFFSET_DIV2;
        ext_ctrl[25].value = pHevcParam->LoopFilterBetaOffset;

        ext_ctrl[26].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_LTR_ENABLE;
        ext_ctrl[26].value = pHevcParam->LongtermRefEnable;

        ext_ctrl[27].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_USER_REF;
        ext_ctrl[27].value = pHevcParam->LongtermUserRef;

        ext_ctrl[28].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_STORE_REF;
        ext_ctrl[28].value = pHevcParam->LongtermStoreRef;

        /* should be set V4L2_CID_MPEG_VIDEO_MB_RC_ENABLE first */
        ext_ctrl[29].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_ADAPTIVE_RC_DARK;
        ext_ctrl[29].value = pHevcParam->DarkDisable;

        ext_ctrl[30].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_ADAPTIVE_RC_SMOOTH;
        ext_ctrl[30].value = pHevcParam->SmoothDisable;

        ext_ctrl[31].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_ADAPTIVE_RC_STATIC;
        ext_ctrl[31].value = pHevcParam->StaticDisable;

        ext_ctrl[32].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_ADAPTIVE_RC_ACTIVITY;
        ext_ctrl[32].value = pHevcParam->ActivityDisable;

        /* doesn't have to be set */
        ext_ctrl[33].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_REFRESH_TYPE;
        ext_ctrl[33].value = 2;
        ext_ctrl[34].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_REFRESH_PERIOD;
        ext_ctrl[34].value = 0;
        ext_ctrl[35].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_LOSSLESS_CU_ENABLE;
        ext_ctrl[35].value = 0;
        ext_ctrl[36].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_CONST_INTRA_PRED_ENABLE;
        ext_ctrl[36].value = 0;
        ext_ctrl[37].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_WAVEFRONT_ENABLE;
        ext_ctrl[37].value = 0;
        ext_ctrl[38].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_SIGN_DATA_HIDING;
        ext_ctrl[38].value = 1;
        ext_ctrl[39].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_GENERAL_PB_ENABLE;
        ext_ctrl[39].value = pHevcParam->GPBEnable;
        ext_ctrl[40].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_TEMPORAL_ID_ENABLE;
        ext_ctrl[40].value = 1;
        ext_ctrl[41].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_STRONG_SMOTHING_FLAG;
        ext_ctrl[41].value = 1;
        ext_ctrl[42].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_MAX_NUM_MERGE_MV_MINUS1;
        ext_ctrl[42].value = 4;
        ext_ctrl[43].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_DISABLE_INTRA_PU_SPLIT;
        ext_ctrl[43].value = 0;
        ext_ctrl[44].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_DISABLE_TMV_PREDICTION;
        ext_ctrl[44].value = 0;
        ext_ctrl[45].id = V4L2_CID_MPEG_VIDEO_VBV_SIZE;
        ext_ctrl[45].value = 0;
        ext_ctrl[46].id = V4L2_CID_MPEG_VIDEO_HEADER_MODE;
        ext_ctrl[46].value = V4L2_MPEG_VIDEO_HEADER_MODE_SEPARATE;

        /* SVC is not supported yet */
        ext_ctrl[47].id =  V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_QP_ENABLE;
        ext_ctrl[47].value = 0;
        switch (pHevcParam->HierarType) {
        case VIDEO_HIERARCHICAL_B:
            ext_ctrl[48].id =  V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_TYPE;
            ext_ctrl[48].value = V4L2_MPEG_VIDEO_H264_HIERARCHICAL_CODING_B;
            break;
        case VIDEO_HIERARCHICAL_P:
        default:
            ext_ctrl[48].id =  V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_TYPE;
            ext_ctrl[48].value = V4L2_MPEG_VIDEO_H264_HIERARCHICAL_CODING_P;
            break;
        }
        ext_ctrl[49].id =  V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER;
        ext_ctrl[49].value = pHevcParam->TemporalSVC.nCount;
        ext_ctrl[50].id =  V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_QP;
        ext_ctrl[50].value = ((unsigned int)0 << 16 | 29);
        ext_ctrl[51].id =  V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_QP;
        ext_ctrl[51].value = ((unsigned int)1 << 16 | 29);
        ext_ctrl[52].id =  V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_QP;
        ext_ctrl[52].value = ((unsigned int)2 << 16 | 29);
        count = HEVC_CTRL_NUM;

        if (pCtx->videoCtx.instInfo.supportInfo.enc.bTemporalSvcSupport == VIDEO_TRUE) {
            i = count;
            ext_ctrl[i].id         = V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_BIT0;
            ext_ctrl[i].value      = pHevcParam->TemporalSVC.nBitrateRatio[0];
            ext_ctrl[i + 1].id     = V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_BIT1;
            ext_ctrl[i + 1].value  = pHevcParam->TemporalSVC.nBitrateRatio[1];
            ext_ctrl[i + 2].id     = V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_BIT2;
            ext_ctrl[i + 2].value  = pHevcParam->TemporalSVC.nBitrateRatio[2];
            ext_ctrl[i + 3].id     = V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_BIT3;
            ext_ctrl[i + 3].value  = pHevcParam->TemporalSVC.nBitrateRatio[3];
            ext_ctrl[i + 4].id     = V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_BIT4;
            ext_ctrl[i + 4].value  = pHevcParam->TemporalSVC.nBitrateRatio[4];
            ext_ctrl[i + 5].id     = V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_BIT5;
            ext_ctrl[i + 5].value  = pHevcParam->TemporalSVC.nBitrateRatio[5];
            ext_ctrl[i + 6].id     = V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_BIT6;
            ext_ctrl[i + 6].value  = pHevcParam->TemporalSVC.nBitrateRatio[6];
            count += 7;
        }

        if (pCtx->videoCtx.instInfo.supportInfo.enc.bSkypeSupport == VIDEO_TRUE) {
            i = count;
            /* MAX LAYER COUNT */
            ext_ctrl[i].id      = V4L2_CID_MPEG_VIDEO_TEMPORAL_SHORTTERM_MAX_LAYER;
            ext_ctrl[i].value   = pHevcParam->MaxTemporalLayerCount;

            count += 1;
        }

        if (pCtx->videoCtx.instInfo.supportInfo.enc.bRoiInfoSupport == VIDEO_TRUE) {
            i = count;
            ext_ctrl[i].id     = V4L2_CID_MPEG_VIDEO_ROI_ENABLE;
            ext_ctrl[i].value  = pHevcParam->ROIEnable;

            count += 1;
        }

        /* optional : if these are not set, set value are same as I frame */
        if (pCtx->videoCtx.instInfo.supportInfo.enc.bQpRangePBSupport == VIDEO_TRUE) {
            i = count;
            ext_ctrl[i].id = V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP_P;  /* P frame */
            ext_ctrl[i].value = GET_HEVC_QP_VALUE(pCommonParam->QpRange.QpMin_P,
                                                    pCtx->videoCtx.instInfo.HwVersion);
            ext_ctrl[i + 1].id =  V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP_P;
            ext_ctrl[i + 1].value = GET_HEVC_QP_VALUE(pCommonParam->QpRange.QpMax_P,
                                                    pCtx->videoCtx.instInfo.HwVersion);
            ext_ctrl[i + 2].id = V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP_B;  /* B frame */
            ext_ctrl[i + 2].value = GET_HEVC_QP_VALUE(pCommonParam->QpRange.QpMin_B,
                                                    pCtx->videoCtx.instInfo.HwVersion);
            ext_ctrl[i + 3].id =  V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP_B;
            ext_ctrl[i + 3].value = GET_HEVC_QP_VALUE(pCommonParam->QpRange.QpMax_B,
                                                    pCtx->videoCtx.instInfo.HwVersion);

            count += 4;
        }

        if (pCtx->videoCtx.instInfo.supportInfo.enc.bPVCSupport == VIDEO_TRUE) {
            i = count;
            ext_ctrl[i].id = V4L2_CID_MPEG_VIDEO_RC_PVC_ENABLE;
            ext_ctrl[i].value = pCommonParam->PerceptualMode;

            count += 1;
        }

        if (pCtx->videoCtx.instInfo.supportInfo.enc.bChromaQpSupport == VIDEO_TRUE) {
            i = count;
            ext_ctrl[i].id          = V4L2_CID_MPEG_VIDEO_CHROMA_QP_OFFSET_CB;
            ext_ctrl[i].value       = pHevcParam->chromaQPOffset.Cb;
            ext_ctrl[i + 1].id      = V4L2_CID_MPEG_VIDEO_CHROMA_QP_OFFSET_CR;
            ext_ctrl[i + 1].value   = pHevcParam->chromaQPOffset.Cr;

            count +=2;
        }
    }

    return count;
}

int Codec_OSAL_SetControls_EncParams_Vp9(
    CodecOSALVideoContext   *pCtx,
    void                    *pInfo,
    struct v4l2_ext_control ext_ctrl[]) {
    int count = 0;

    ExynosVideoEncParam         *pEncParam      = (ExynosVideoEncParam *)pInfo;
    ExynosVideoEncCommonParam   *pCommonParam   = &pEncParam->common;

    int i;

    /* vp9 codec specific parameters */
    {
        ExynosVideoEncVp9Param *pVp9Param = &pEncParam->codec.vp9;

        /* VP9 specific parameters */
        ext_ctrl[8].id      = V4L2_CID_MPEG_VIDEO_VP9_I_FRAME_QP;
        ext_ctrl[8].value   = pCommonParam->FrameQp;

        ext_ctrl[9].id      = V4L2_CID_MPEG_VIDEO_VP9_P_FRAME_QP;
        ext_ctrl[9].value   = pCommonParam->FrameQp_P;

        ext_ctrl[10].id     = V4L2_CID_MPEG_VIDEO_VP9_MAX_QP;  /* I frame */
        ext_ctrl[10].value  = GET_VP9_QP_VALUE(pCommonParam->QpRange.QpMax_I);

        ext_ctrl[11].id     = V4L2_CID_MPEG_VIDEO_VP9_MIN_QP;
        ext_ctrl[11].value  = GET_VP9_QP_VALUE(pCommonParam->QpRange.QpMin_I);

        ext_ctrl[12].id     = V4L2_CID_MPEG_MFC51_VIDEO_RC_REACTION_COEFF;
        ext_ctrl[12].value  = pCommonParam->CBRPeriodRf;

        ext_ctrl[13].id     = V4L2_CID_MPEG_VIDEO_VP9_RC_FRAME_RATE;
        ext_ctrl[13].value  = pVp9Param->VpX.FrameRate;

        ext_ctrl[14].id     = V4L2_CID_MPEG_VIDEO_VBV_SIZE;
        ext_ctrl[14].value  = 0;

        ext_ctrl[15].id     = V4L2_CID_MPEG_VIDEO_HEADER_MODE;
        ext_ctrl[15].value  = V4L2_MPEG_VIDEO_HEADER_MODE_SEPARATE;

        ext_ctrl[16].id     = V4L2_CID_MPEG_VIDEO_VP9_DISABLE_INTRA_PU_SPLIT;
        ext_ctrl[16].value  = 0;

        switch (pCommonParam->FrameSkip) {
        case VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT:
            ext_ctrl[17].id     = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[17].value  = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT;
            break;
        case VIDEO_FRAME_SKIP_MODE_BUF_LIMIT:
            ext_ctrl[17].id     = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[17].value  = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_BUF_LIMIT;
            break;
        default:
            /* VIDEO_FRAME_SKIP_MODE_DISABLE (default) */
            ext_ctrl[17].id     = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[17].value  = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_DISABLED;
            break;
        }

        ext_ctrl[18].id     = V4L2_CID_MPEG_VIDEO_VP9_VERSION;
        ext_ctrl[18].value  = pVp9Param->Vp9Profile;

        /* TODO */
        // ext_ctrl[18].id     = V4L2_CID_MPEG_VIDEO_VP9_LEVEL;
        // ext_ctrl[18].value  = pVp9Param->Vp9Level;

        ext_ctrl[19].id     = V4L2_CID_MPEG_VIDEO_VP9_MAX_PARTITION_DEPTH;
        ext_ctrl[19].value  = pVp9Param->MaxPartitionDepth;

        ext_ctrl[20].id     = V4L2_CID_MPEG_VIDEO_VP9_GOLDEN_FRAMESEL;
        ext_ctrl[20].value  = pVp9Param->VpX.VpXGoldenFrameSel;

        ext_ctrl[21].id     = V4L2_CID_MPEG_VIDEO_VP9_GF_REFRESH_PERIOD;
        ext_ctrl[21].value  = pVp9Param->VpX.VpXGFRefreshPeriod;

        ext_ctrl[22].id     = V4L2_CID_MPEG_VIDEO_VP9_HIERARCHY_QP_ENABLE;
        ext_ctrl[22].value  = 0;

        ext_ctrl[23].id     = V4L2_CID_MPEG_VIDEO_VP9_HIERARCHICAL_CODING_LAYER_QP;
        ext_ctrl[23].value  = ((unsigned int)0 << 16 | 90);

        ext_ctrl[24].id     = V4L2_CID_MPEG_VIDEO_VP9_HIERARCHICAL_CODING_LAYER_QP;
        ext_ctrl[24].value  = ((unsigned int)1 << 16 | 90);

        ext_ctrl[25].id     = V4L2_CID_MPEG_VIDEO_VP9_HIERARCHICAL_CODING_LAYER_QP;
        ext_ctrl[25].value  = ((unsigned int)2 << 16 | 90);

        ext_ctrl[26].id     = V4L2_CID_MPEG_VIDEO_VP9_REF_NUMBER_FOR_PFRAMES;
        ext_ctrl[26].value  = pVp9Param->VpX.RefNumberForPFrame;

        ext_ctrl[27].id     = V4L2_CID_MPEG_VIDEO_VP9_HIERARCHICAL_CODING_LAYER;
        ext_ctrl[27].value  = pVp9Param->VpX.TemporalSVC.nCount;

        ext_ctrl[28].id     = V4L2_CID_MPEG_VIDEO_DISABLE_IVF_HEADER;
        ext_ctrl[28].value  = 1;
        count = VP9_CTRL_NUM;

        if (pCtx->videoCtx.instInfo.supportInfo.enc.bTemporalSvcSupport == VIDEO_TRUE) {
            i = count;
            ext_ctrl[i].id          = V4L2_CID_MPEG_VIDEO_VP9_HIERARCHICAL_CODING_LAYER_BIT0;
            ext_ctrl[i].value       = pVp9Param->VpX.TemporalSVC.nBitrateRatio[0];
            ext_ctrl[i + 1].id      = V4L2_CID_MPEG_VIDEO_VP9_HIERARCHICAL_CODING_LAYER_BIT1;
            ext_ctrl[i + 1].value   = pVp9Param->VpX.TemporalSVC.nBitrateRatio[1];
            ext_ctrl[i + 2].id      = V4L2_CID_MPEG_VIDEO_VP9_HIERARCHICAL_CODING_LAYER_BIT2;
            ext_ctrl[i + 2].value   = pVp9Param->VpX.TemporalSVC.nBitrateRatio[2];

            count += 3;
        }

        /* optional : if these are not set, set value are same as I frame */
        if (pCtx->videoCtx.instInfo.supportInfo.enc.bQpRangePBSupport == VIDEO_TRUE) {
            i = count;
            ext_ctrl[i].id = V4L2_CID_MPEG_VIDEO_VP9_MIN_QP_P;  /* P frame */
            ext_ctrl[i].value = GET_VP9_QP_VALUE(pCommonParam->QpRange.QpMin_P);
            ext_ctrl[i + 1].id =  V4L2_CID_MPEG_VIDEO_VP9_MAX_QP_P;
            ext_ctrl[i + 1].value = GET_VP9_QP_VALUE(pCommonParam->QpRange.QpMax_P);

            count += 2;
        }

        if (pCtx->videoCtx.instInfo.supportInfo.enc.bPVCSupport == VIDEO_TRUE) {
            i = count;
            ext_ctrl[i].id = V4L2_CID_MPEG_VIDEO_RC_PVC_ENABLE;
            ext_ctrl[i].value = pCommonParam->PerceptualMode;

            count += 1;
        }
    }

    return count;
}

int Codec_OSAL_SetControls_EncParams(
    CodecOSALVideoContext   *pCtx,
    void                    *pInfo) {
    int ret = 0;

    ExynosVideoEncParam         *pEncParam      = (ExynosVideoEncParam *)pInfo;
    ExynosVideoEncCommonParam   *pCommonParam   = &pEncParam->common;

    int i;
    struct v4l2_ext_control  ext_ctrl[MAX_CTRL_NUM];
    struct v4l2_ext_controls ext_ctrls;

    /* common parameters */
    ext_ctrl[0].id = V4L2_CID_MPEG_VIDEO_GOP_SIZE;
    ext_ctrl[0].value = (pCommonParam->IDRPeriod < 0)? 0:pCommonParam->IDRPeriod;
    ext_ctrl[1].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MODE;

    if ((pCtx->videoCtx.instInfo.supportInfo.enc.bFixedSliceSupport == VIDEO_TRUE) &&
        (pCommonParam->bFixedSlice == VIDEO_TRUE) &&
        (pCommonParam->SliceMode != 0)) {
        ext_ctrl[1].value = 4;  /* fixed bytes */
    } else {
        ext_ctrl[1].value = pCommonParam->SliceMode;  /* 0: one, 1: fixed #mb, 2: fixed #bytes */
    }
    ext_ctrl[2].id = V4L2_CID_MPEG_VIDEO_CYCLIC_INTRA_REFRESH_MB;
    ext_ctrl[2].value = pCommonParam->RandomIntraMBRefresh;
    ext_ctrl[3].id = V4L2_CID_MPEG_MFC51_VIDEO_PADDING;
    ext_ctrl[3].value = pCommonParam->PadControlOn;
    ext_ctrl[4].id = V4L2_CID_MPEG_MFC51_VIDEO_PADDING_YUV;
    ext_ctrl[4].value = pCommonParam->CrPadVal;
    ext_ctrl[4].value |= (pCommonParam->CbPadVal << 8);
    ext_ctrl[4].value |= (pCommonParam->LumaPadVal << 16);
    ext_ctrl[5].id = V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE;
    ext_ctrl[5].value = pCommonParam->EnableFRMRateControl;
    ext_ctrl[6].id = V4L2_CID_MPEG_VIDEO_MB_RC_ENABLE;
    ext_ctrl[6].value = pCommonParam->EnableMBRateControl;
    ext_ctrl[7].id = V4L2_CID_MPEG_VIDEO_BITRATE;

    /* FIXME temporary fix */
    if (pCommonParam->Bitrate)
        ext_ctrl[7].value = pCommonParam->Bitrate;
    else
        ext_ctrl[7].value = 1; /* just for testing Movie studio */

    /* codec specific parameters */
    switch (pCtx->videoCtx.instInfo.eCodecType) {
    case VIDEO_CODING_AVC:
    {
        ext_ctrls.count = Codec_OSAL_SetControls_EncParams_Avc(pCtx, pInfo, ext_ctrl);
    }
        break;
    case VIDEO_CODING_MPEG4:
    {
        ext_ctrls.count = Codec_OSAL_SetControls_EncParams_Mpeg4(pCtx, pInfo, ext_ctrl);
    }
        break;
    case VIDEO_CODING_H263:
    {
        ext_ctrls.count = Codec_OSAL_SetControls_EncParams_H263(pCtx, pInfo, ext_ctrl);
    }
        break;
    case VIDEO_CODING_VP8:
    {
        ext_ctrls.count = Codec_OSAL_SetControls_EncParams_Vp8(pCtx, pInfo, ext_ctrl);
    }
        break;
    case VIDEO_CODING_HEVC:
    {
        ext_ctrls.count = Codec_OSAL_SetControls_EncParams_Hevc(pCtx, pInfo, ext_ctrl);
    }
        break;
    case VIDEO_CODING_VP9:
    {
        ext_ctrls.count = Codec_OSAL_SetControls_EncParams_Vp9(pCtx, pInfo, ext_ctrl);
    }
        break;
    default:
    {
        ALOGE("[%s] Undefined codec type", __FUNCTION__);
        ret = -1;
        goto EXIT;
    }
        break;
    }

    /* extra common parameters */
    if (pCtx->videoCtx.instInfo.supportInfo.enc.bDropControlSupport == VIDEO_TRUE) {
        i = ext_ctrls.count;
        ext_ctrl[i].id = V4L2_CID_MPEG_VIDEO_DROP_CONTROL;
        ext_ctrl[i].value = pCommonParam->bDropControl;

        ext_ctrls.count += 1;
    }

    ext_ctrls.ctrl_class = V4L2_CTRL_CLASS_MPEG;
    ext_ctrls.controls = ext_ctrl;

    if (exynos_v4l2_s_ext_ctrl(pCtx->videoCtx.hDevice, &ext_ctrls) != 0) {
        ret = -1;
        goto EXIT;
    }

EXIT:
    return ret;
}

int Codec_OSAL_SetControls_QPRange(
    CodecOSALVideoContext   *pCtx,
    void                    *pInfo) {
    int ret = 0;

    ExynosVideoQPRange *pQpRange = (ExynosVideoQPRange *)pInfo;

    int cids[3][2], values[3][2];  /* I, P, B : min & max */
    int i;

    memset(cids, 0, sizeof(cids));
    memset(values, 0, sizeof(values));

    /* codec specific parameters */
    /* common parameters but id is depends on codec */
    switch (pCtx->videoCtx.instInfo.eCodecType) {
    case VIDEO_CODING_AVC:
    {
        cids[0][0] = V4L2_CID_MPEG_VIDEO_H264_MIN_QP;
        cids[0][1] = V4L2_CID_MPEG_VIDEO_H264_MAX_QP;

        values[0][0] = GET_H264_QP_VALUE(pQpRange->QpMin_I);
        values[0][1] = GET_H264_QP_VALUE(pQpRange->QpMax_I);

        if (pCtx->videoCtx.instInfo.supportInfo.enc.bQpRangePBSupport == VIDEO_TRUE) {
            if (pQpRange->QpMin_P > pQpRange->QpMax_P) {
                ALOGE("%s: QP(P) range(%d, %d) is wrong", __FUNCTION__, pQpRange->QpMin_P, pQpRange->QpMax_P);
                ret = -1;
                goto EXIT;
            }

            if (pQpRange->QpMin_B > pQpRange->QpMax_B) {
                ALOGE("%s: QP(B) range(%d, %d) is wrong", __FUNCTION__, pQpRange->QpMin_B, pQpRange->QpMax_B);
                ret = -1;
                goto EXIT;
            }

            cids[1][0] = V4L2_CID_MPEG_VIDEO_H264_MIN_QP_P;
            cids[1][1] = V4L2_CID_MPEG_VIDEO_H264_MAX_QP_P;
            cids[2][0] = V4L2_CID_MPEG_VIDEO_H264_MIN_QP_B;
            cids[2][1] = V4L2_CID_MPEG_VIDEO_H264_MAX_QP_B;

            values[1][0] = GET_H264_QP_VALUE(pQpRange->QpMin_P);
            values[1][1] = GET_H264_QP_VALUE(pQpRange->QpMax_P);
            values[2][0] = GET_H264_QP_VALUE(pQpRange->QpMin_B);
            values[2][1] = GET_H264_QP_VALUE(pQpRange->QpMax_B);
        }
    }
        break;
    case VIDEO_CODING_MPEG4:
    {
        cids[0][0] = V4L2_CID_MPEG_VIDEO_MPEG4_MIN_QP;
        cids[0][1] = V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP;

        values[0][0] = GET_MPEG4_QP_VALUE(pQpRange->QpMin_I);
        values[0][1] = GET_MPEG4_QP_VALUE(pQpRange->QpMax_I);

        if (pCtx->videoCtx.instInfo.supportInfo.enc.bQpRangePBSupport == VIDEO_TRUE) {
            if (pQpRange->QpMin_P > pQpRange->QpMax_P) {
                ALOGE("%s: QP(P) range(%d, %d) is wrong", __FUNCTION__, pQpRange->QpMin_P, pQpRange->QpMax_P);
                ret = -1;
                goto EXIT;
            }

            if (pQpRange->QpMin_B > pQpRange->QpMax_B) {
                ALOGE("%s: QP(B) range(%d, %d) is wrong", __FUNCTION__, pQpRange->QpMin_B, pQpRange->QpMax_B);
                ret = -1;
                goto EXIT;
            }

            cids[1][0] = V4L2_CID_MPEG_VIDEO_MPEG4_MIN_QP_P;
            cids[1][1] = V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP_P;
            cids[2][0] = V4L2_CID_MPEG_VIDEO_MPEG4_MIN_QP_B;
            cids[2][1] = V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP_B;

            values[1][0] = GET_MPEG4_QP_VALUE(pQpRange->QpMin_P);
            values[1][1] = GET_MPEG4_QP_VALUE(pQpRange->QpMax_P);
            values[2][0] = GET_MPEG4_QP_VALUE(pQpRange->QpMin_B);
            values[2][1] = GET_MPEG4_QP_VALUE(pQpRange->QpMax_B);
        }
    }
        break;
    case VIDEO_CODING_H263:
    {
        cids[0][0] = V4L2_CID_MPEG_VIDEO_H263_MIN_QP;
        cids[0][1] = V4L2_CID_MPEG_VIDEO_H263_MAX_QP;

        values[0][0] = GET_H263_QP_VALUE(pQpRange->QpMin_I);
        values[0][1] = GET_H263_QP_VALUE(pQpRange->QpMax_I);

        if (pCtx->videoCtx.instInfo.supportInfo.enc.bQpRangePBSupport == VIDEO_TRUE) {
            if (pQpRange->QpMin_P > pQpRange->QpMax_P) {
                ALOGE("%s: QP(P) range(%d, %d) is wrong", __FUNCTION__, pQpRange->QpMin_P, pQpRange->QpMax_P);
                ret = -1;
                goto EXIT;
            }

            cids[1][0] = V4L2_CID_MPEG_VIDEO_H263_MIN_QP_P;
            cids[1][1] = V4L2_CID_MPEG_VIDEO_H263_MAX_QP_P;

            values[1][0] = GET_H263_QP_VALUE(pQpRange->QpMin_P);
            values[1][1] = GET_H263_QP_VALUE(pQpRange->QpMax_P);
        }
    }
        break;
    case VIDEO_CODING_VP8:
    {
        cids[0][0] = V4L2_CID_MPEG_VIDEO_VP8_MIN_QP;
        cids[0][1] = V4L2_CID_MPEG_VIDEO_VP8_MAX_QP;

        values[0][0] = GET_VP8_QP_VALUE(pQpRange->QpMin_I);
        values[0][1] = GET_VP8_QP_VALUE(pQpRange->QpMax_I);

        if (pCtx->videoCtx.instInfo.supportInfo.enc.bQpRangePBSupport == VIDEO_TRUE) {
            if (pQpRange->QpMin_P > pQpRange->QpMax_P) {
                ALOGE("%s: QP(P) range(%d, %d) is wrong", __FUNCTION__, pQpRange->QpMin_P, pQpRange->QpMax_P);
                ret = -1;
                goto EXIT;
            }

            cids[1][0] = V4L2_CID_MPEG_VIDEO_VP8_MIN_QP_P;
            cids[1][1] = V4L2_CID_MPEG_VIDEO_VP8_MAX_QP_P;

            values[1][0] = GET_VP8_QP_VALUE(pQpRange->QpMin_P);
            values[1][1] = GET_VP8_QP_VALUE(pQpRange->QpMax_P);
        }
    }
        break;
    case VIDEO_CODING_HEVC:
    {
        cids[0][0] = V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP;
        cids[0][1] = V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP;

        values[0][0] = GET_HEVC_QP_VALUE(pQpRange->QpMin_I,
                                        pCtx->videoCtx.instInfo.HwVersion);
        values[0][1] = GET_HEVC_QP_VALUE(pQpRange->QpMax_I,
                                        pCtx->videoCtx.instInfo.HwVersion);

        if (pCtx->videoCtx.instInfo.supportInfo.enc.bQpRangePBSupport == VIDEO_TRUE) {
            if (pQpRange->QpMin_P > pQpRange->QpMax_P) {
                ALOGE("%s: QP(P) range(%d, %d) is wrong", __FUNCTION__, pQpRange->QpMin_P, pQpRange->QpMax_P);
                ret = -1;
                goto EXIT;
            }

            if (pQpRange->QpMin_B > pQpRange->QpMax_B) {
                ALOGE("%s: QP(B) range(%d, %d) is wrong", __FUNCTION__, pQpRange->QpMin_B, pQpRange->QpMax_B);
                ret = -1;
                goto EXIT;
            }

            cids[1][0] = V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP_P;
            cids[1][1] = V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP_P;
            cids[2][0] = V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP_B;
            cids[2][1] = V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP_B;

            values[1][0] = GET_HEVC_QP_VALUE(pQpRange->QpMin_P,
                                            pCtx->videoCtx.instInfo.HwVersion);
            values[1][1] = GET_HEVC_QP_VALUE(pQpRange->QpMax_P,
                                            pCtx->videoCtx.instInfo.HwVersion);
            values[2][0] = GET_HEVC_QP_VALUE(pQpRange->QpMin_B,
                                            pCtx->videoCtx.instInfo.HwVersion);
            values[2][1] = GET_HEVC_QP_VALUE(pQpRange->QpMax_B,
                                            pCtx->videoCtx.instInfo.HwVersion);
        }
    }
        break;
    case VIDEO_CODING_VP9:
    {
        cids[0][0] = V4L2_CID_MPEG_VIDEO_VP9_MIN_QP;
        cids[0][1] = V4L2_CID_MPEG_VIDEO_VP9_MAX_QP;

        values[0][0] = GET_VP9_QP_VALUE(pQpRange->QpMin_I);
        values[0][1] = GET_VP9_QP_VALUE(pQpRange->QpMax_I);

        if (pCtx->videoCtx.instInfo.supportInfo.enc.bQpRangePBSupport == VIDEO_TRUE) {
            cids[1][0] = V4L2_CID_MPEG_VIDEO_VP9_MIN_QP_P;
            cids[1][1] = V4L2_CID_MPEG_VIDEO_VP9_MAX_QP_P;

            values[1][0] = GET_VP9_QP_VALUE(pQpRange->QpMin_P);
            values[1][1] = GET_VP9_QP_VALUE(pQpRange->QpMax_P);
        }
    }
        break;
    default:
    {
        ALOGE("[%s] Undefined codec type", __FUNCTION__);
        ret = -1;
        goto EXIT;
    }
        break;
    }

    for (i = 0; i < (int)(sizeof(cids)/sizeof(cids[0])); i++) {
        if (cids[i][0] == 0)
            break;

        ALOGV("%s: QP[%d] range (%d / %d)", __FUNCTION__, i, values[i][0], values[i][1]);

        /* keep a calling sequence as Max->Min because dirver has a restriction */
        if (exynos_v4l2_s_ctrl(pCtx->videoCtx.hDevice, cids[i][1], values[i][1]) != 0) {
            ALOGE("%s: Failed to s_ctrl for max value", __FUNCTION__);
            ret = -1;
            goto EXIT;
        }

        if (exynos_v4l2_s_ctrl(pCtx->videoCtx.hDevice, cids[i][0], values[i][0]) != 0) {
            ALOGE("%s: Failed to s_ctrl for min value", __FUNCTION__);
            ret = -1;
            goto EXIT;
        }
    }

EXIT:
    return ret;
}

int Codec_OSAL_SetControls_ColorAspects(
    CodecOSALVideoContext   *pCtx,
    void                    *pInfo) {
    int ret = 0;

    ExynosVideoColorAspects *pColorAspects = (ExynosVideoColorAspects *)pInfo;
    struct v4l2_ext_control  ext_ctrl[4];  /* range, primaries, transfer, matrix coeff */
    struct v4l2_ext_controls ext_ctrls;

    ext_ctrl[0].id    = V4L2_CID_MPEG_VIDEO_FULL_RANGE_FLAG;
    ext_ctrl[0].value = (pColorAspects->eRangeType == RANGE_FULL)? 1:0;

    if (pCtx->videoCtx.instInfo.eCodecType == VIDEO_CODING_VP9) {
        int i;

        ext_ctrl[1].id    = V4L2_CID_MPEG_VIDEO_COLOUR_PRIMARIES;
        ext_ctrl[1].value = 0;

        /* PRIMARIES_BT601_6_625 is supposed to be used as same as PRIMARIES_BT601_6_525 based on MFC
         * And matrix doesn't have PRIMARIES_BT601_6_625 case
         */
        if (pColorAspects->ePrimariesType == PRIMARIES_BT601_6_625)
            pColorAspects->ePrimariesType = PRIMARIES_BT601_6_525;

        /* TRANSFER_SMPTE_170M is supposed to be used as same as TRANSFER_BT709 based on spec.
         * And ColorSpace doesn't support TRANSFER_BT709
         */
        if (pColorAspects->eTransferType == TRANSFER_BT709)
            pColorAspects->eTransferType = TRANSFER_SMPTE_170M;

        for (i = 0; i < (int)(sizeof(ColorSpaceToColorAspects)/sizeof(ColorSpaceToColorAspects[0])); i++) {
            if ((ColorSpaceToColorAspects[i].primaries == pColorAspects->ePrimariesType) &&
                (ColorSpaceToColorAspects[i].transfer == pColorAspects->eTransferType) &&
                (ColorSpaceToColorAspects[i].coeff == pColorAspects->eCoeffType)) {
                ext_ctrl[1].value = i;
                break;
            }
        }

        ext_ctrls.count = 2;
    } else {
        ext_ctrl[1].id    = V4L2_CID_MPEG_VIDEO_COLOUR_PRIMARIES;
        ext_ctrl[1].value = pColorAspects->ePrimariesType;

        ext_ctrl[2].id    = V4L2_CID_MPEG_VIDEO_TRANSFER_CHARACTERISTICS;
        ext_ctrl[2].value = pColorAspects->eTransferType;

        ext_ctrl[3].id    = V4L2_CID_MPEG_VIDEO_MATRIX_COEFFICIENTS;
        ext_ctrl[3].value = pColorAspects->eCoeffType;

        ext_ctrls.count = 4;
    }

    ext_ctrls.ctrl_class = V4L2_CTRL_CLASS_MPEG;
    ext_ctrls.controls   = ext_ctrl;

    if (exynos_v4l2_s_ext_ctrl(pCtx->videoCtx.hDevice, &ext_ctrls) != 0) {
        ret = -1;
        goto EXIT;
    }

EXIT:
    return ret;
}

int Codec_OSAL_SetControls_MvVer_Position(
    CodecOSALVideoContext   *pCtx,
    void                    *pInfo) {
    int ret = 0;

    ExynosVideoPMV *pPMV = (ExynosVideoPMV *)pInfo;
    struct v4l2_ext_control  ext_ctrl[5];
    struct v4l2_ext_controls ext_ctrls;
    int i;

    ext_ctrl[0].id    = V4L2_CID_MPEG_VIDEO_MV_SEARCH_MODE;
    ext_ctrl[0].value = pPMV->mode;
    ext_ctrls.count = 1;

    if (pPMV->mode == 2) {  /* Globl PMV */
        int i = ext_ctrls.count;
        ext_ctrl[i].id    = V4L2_CID_MPEG_VIDEO_MV_HOR_POSITION_L0;
        ext_ctrl[i].value = pPMV->hor_L0;

        ext_ctrl[i + 1].id    = V4L2_CID_MPEG_VIDEO_MV_HOR_POSITION_L1;
        ext_ctrl[i + 1].value = pPMV->hor_L1;

        ext_ctrl[i + 2].id    = V4L2_CID_MPEG_VIDEO_MV_VER_POSITION_L0;
        ext_ctrl[i + 2].value = pPMV->ver_L0;

        ext_ctrl[i + 3].id    = V4L2_CID_MPEG_VIDEO_MV_VER_POSITION_L1;
        ext_ctrl[i + 3].value = pPMV->ver_L1;

        ext_ctrls.count += 4;
    }

    ext_ctrls.ctrl_class = V4L2_CTRL_CLASS_MPEG;
    ext_ctrls.controls   = ext_ctrl;

    for (i = 0; i < ext_ctrls.count; i++) {
        ret = exynos_v4l2_s_ctrl(pCtx->videoCtx.hDevice, ext_ctrl[i].id, ext_ctrl[i].value);
        if (ret != 0) {
            ALOGE("%s: Failed to s_ctrl for CID(0x%x)/value(%d)", __FUNCTION__, ext_ctrl[i].id, ext_ctrl[i].value);
            goto EXIT;
        }
    }

EXIT:
    return ret;
}

int Codec_OSAL_SetControls_HDRStaticInfo(
    CodecOSALVideoContext   *pCtx,
    void                    *pInfo) {
    int ret = 0;

    ExynosVideoHdrStatic *pHDRStaticInfo = (ExynosVideoHdrStatic *)pInfo;
    struct v4l2_ext_control  ext_ctrl[9];
    struct v4l2_ext_controls ext_ctrls;

    ExynosVideoHdrStatic invalidHDR;

    /* check a validation */
    memset(&invalidHDR, 0, sizeof(ExynosVideoHdrStatic));
    if (memcmp(&invalidHDR, pHDRStaticInfo, sizeof(ExynosVideoHdrStatic)) == 0) {
        /* all values are invalid */
        ALOGD("%s: HDRStaticInfo is unspecified. HDRStaticInfo won't be set", __FUNCTION__);
        goto EXIT;
    }

    ext_ctrl[0].id    = V4L2_CID_MPEG_VIDEO_STATIC_INFO_ENABLE;
    ext_ctrl[0].value = 1;

    ext_ctrl[1].id    = V4L2_CID_MPEG_VIDEO_SEI_MAX_PIC_AVERAGE_LIGHT;
    ext_ctrl[1].value = pHDRStaticInfo->max_pic_average_light;

    ext_ctrl[2].id    = V4L2_CID_MPEG_VIDEO_SEI_MAX_CONTENT_LIGHT;
    ext_ctrl[2].value = pHDRStaticInfo->max_content_light;

    ext_ctrl[3].id    = V4L2_CID_MPEG_VIDEO_SEI_MAX_DISPLAY_LUMINANCE;
    ext_ctrl[3].value = pHDRStaticInfo->max_display_luminance;

    ext_ctrl[4].id    = V4L2_CID_MPEG_VIDEO_SEI_MIN_DISPLAY_LUMINANCE;
    ext_ctrl[4].value = pHDRStaticInfo->min_display_luminance;

    ext_ctrl[5].id    = V4L2_CID_MPEG_VIDEO_SEI_WHITE_POINT;
    ext_ctrl[5].value = SET_16BIT_TO_HIGH(pHDRStaticInfo->white.x) | GET_16BIT_LOW(pHDRStaticInfo->white.y);

    ext_ctrl[6].id    = V4L2_CID_MPEG_VIDEO_SEI_DISPLAY_PRIMARIES_0;
    ext_ctrl[6].value = SET_16BIT_TO_HIGH(pHDRStaticInfo->green.x) | GET_16BIT_LOW(pHDRStaticInfo->green.y);

    ext_ctrl[7].id    = V4L2_CID_MPEG_VIDEO_SEI_DISPLAY_PRIMARIES_1;
    ext_ctrl[7].value = SET_16BIT_TO_HIGH(pHDRStaticInfo->blue.x) | GET_16BIT_LOW(pHDRStaticInfo->blue.y);

    ext_ctrl[8].id    = V4L2_CID_MPEG_VIDEO_SEI_DISPLAY_PRIMARIES_2;
    ext_ctrl[8].value = SET_16BIT_TO_HIGH(pHDRStaticInfo->red.x) | GET_16BIT_LOW(pHDRStaticInfo->red.y);

    ext_ctrls.count = 9;

    ext_ctrls.ctrl_class = V4L2_CTRL_CLASS_MPEG;
    ext_ctrls.controls   = ext_ctrl;

    if (exynos_v4l2_s_ext_ctrl(pCtx->videoCtx.hDevice, &ext_ctrls) != 0) {
        ret = -1;
        goto EXIT;
    }

EXIT:
    return ret;
}

int Codec_OSAL_SetControls(
    CodecOSALVideoContext   *pCtx,
    unsigned int             nCID,
    void                    *pInfo) {
    int ret = 0;

    if ((pCtx == NULL) ||
        (pInfo == NULL) ||
        (pCtx->videoCtx.hDevice < 0)) {
        ret = -1;
        goto EXIT;
    }

    switch (nCID) {
    case CODEC_OSAL_CID_ENC_SET_PARAMS:
        ret = Codec_OSAL_SetControls_EncParams(pCtx, pInfo);
        break;
    case CODEC_OSAL_CID_ENC_QP_RANGE:
        ret = Codec_OSAL_SetControls_QPRange(pCtx, pInfo);
        break;
    case CODEC_OSAL_CID_ENC_COLOR_ASPECTS:
        ret = Codec_OSAL_SetControls_ColorAspects(pCtx, pInfo);
        break;
    case CODEC_OSAL_CID_ENC_MV_VER_POSITION:
        Codec_OSAL_SetControls_MvVer_Position(pCtx, pInfo);
        break;
    case CODEC_OSAL_CID_ENC_HDR_STATIC_INFO:
        ret = Codec_OSAL_SetControls_HDRStaticInfo(pCtx, pInfo);
        break;
    default:
    {
        ALOGE("%s: unsupported CID(%x)", __FUNCTION__, nCID);
        ret = -1;
    }
        break;
    }

EXIT:
    return ret;
}

int Codec_OSAL_GetControl(
    CodecOSALVideoContext   *pCtx,
    unsigned int             uCID,
    int                     *pValue) {
    if ((pCtx != NULL) &&
        (pValue != NULL) &&
        (pCtx->videoCtx.hDevice >= 0)) {
        return exynos_v4l2_g_ctrl(pCtx->videoCtx.hDevice, uCID, pValue);
    }

    return -1;
}

int Codec_OSAL_SetControl(
    CodecOSALVideoContext  *pCtx,
    unsigned int            uCID,
    unsigned long           nValue) {
    if ((pCtx != NULL) &&
        (pCtx->videoCtx.hDevice >= 0)) {
        return exynos_v4l2_s_ctrl(pCtx->videoCtx.hDevice, uCID, nValue);
    }

    return -1;
}

int Codec_OSAL_GetCrop(
    CodecOSALVideoContext   *pCtx,
    CodecOSAL_Crop          *pCrop) {
    if ((pCtx != NULL) &&
        (pCrop != NULL) &&
        (pCtx->videoCtx.hDevice >= 0)) {
        struct v4l2_crop crop;

        memset(&crop, 0, sizeof(crop));
        crop.type = pCrop->type;

        if (exynos_v4l2_g_crop(pCtx->videoCtx.hDevice, &crop) == 0) {
            pCrop->top      = crop.c.top;
            pCrop->left     = crop.c.left;
            pCrop->width    = crop.c.width;
            pCrop->height   = crop.c.height;

            return 0;
        }
    }

    return -1;
}

int Codec_OSAL_SetCrop(
    CodecOSALVideoContext   *pCtx,
    CodecOSAL_Crop          *pCrop) {
    if ((pCtx != NULL) &&
        (pCrop != NULL) &&
        (pCtx->videoCtx.hDevice >= 0)) {
        struct v4l2_crop crop;

        memset(&crop, 0, sizeof(crop));
        crop.type = pCrop->type;
        crop.c.top = pCrop->top;
        crop.c.left = pCrop->left;
        crop.c.width = pCrop->width;
        crop.c.height = pCrop->height;

        return exynos_v4l2_s_crop(pCtx->videoCtx.hDevice, &crop);
    }

    return -1;
}

int Codec_OSAL_GetFormat(
    CodecOSALVideoContext   *pCtx,
    CodecOSAL_Format        *pFmt) {
    if ((pCtx != NULL) &&
        (pFmt != NULL) &&
        (pCtx->videoCtx.hDevice >= 0)) {
        struct v4l2_format fmt;
        int i;

        memset(&fmt, 0, sizeof(fmt));
        fmt.type = pFmt->type;

        if (exynos_v4l2_g_fmt(pCtx->videoCtx.hDevice, &fmt) == 0) {
            pFmt->format = fmt.fmt.pix_mp.pixelformat;
            pFmt->width  = fmt.fmt.pix_mp.width;
            pFmt->height = fmt.fmt.pix_mp.height;
            pFmt->stride = fmt.fmt.pix_mp.plane_fmt[0].bytesperline;
            pFmt->nPlane = fmt.fmt.pix_mp.num_planes;
            pFmt->field  = fmt.fmt.pix_mp.field;

            for (i = 0; i < fmt.fmt.pix_mp.num_planes; i++)
                pFmt->planeSize[i] = fmt.fmt.pix_mp.plane_fmt[i].sizeimage;

            return 0;
        }
    }

    return -1;
}

int Codec_OSAL_SetFormat(
    CodecOSALVideoContext   *pCtx,
    CodecOSAL_Format        *pFmt) {
    if ((pCtx != NULL) &&
        (pFmt != NULL) &&
        (pCtx->videoCtx.hDevice >= 0)) {
        struct v4l2_format fmt;
        int i;

        memset(&fmt, 0, sizeof(fmt));
        fmt.type                                    = pFmt->type;
        fmt.fmt.pix_mp.pixelformat                  = pFmt->format;
        fmt.fmt.pix_mp.width                        = pFmt->width;
        fmt.fmt.pix_mp.height                       = pFmt->height;
        for (i = 0; i < pFmt->nPlane; i++) {
            fmt.fmt.pix_mp.plane_fmt[i].bytesperline = (i == 0)? pFmt->stride:pFmt->cstride;
        }
        fmt.fmt.pix_mp.num_planes                   = pFmt->nPlane;
        fmt.fmt.pix_mp.flags                        = pFmt->field;

        for (i = 0; i < pFmt->nPlane; i++)
            fmt.fmt.pix_mp.plane_fmt[i].sizeimage = pFmt->planeSize[i];

        return exynos_v4l2_s_fmt(pCtx->videoCtx.hDevice, &fmt);
    }

    return -1;
}

int Codec_OSAL_TryFormat(
    CodecOSALVideoContext   *pCtx,
    CodecOSAL_Format        *pFmt) {
    if ((pCtx != NULL) &&
        (pFmt != NULL) &&
        (pCtx->videoCtx.hDevice >= 0)) {
        struct v4l2_format fmt;
        int i;

        memset(&fmt, 0, sizeof(fmt));
        fmt.type                                    = pFmt->type;
        fmt.fmt.pix_mp.pixelformat                  = pFmt->format;
        fmt.fmt.pix_mp.width                        = pFmt->width;
        fmt.fmt.pix_mp.height                       = pFmt->height;
        for (i = 0; i < pFmt->nPlane; i++) {
            fmt.fmt.pix_mp.plane_fmt[i].bytesperline = (i == 0)? pFmt->stride:pFmt->cstride;
        }
        fmt.fmt.pix_mp.num_planes                   = pFmt->nPlane;
        fmt.fmt.pix_mp.flags                        = pFmt->field;

        for (i = 0; i < pFmt->nPlane; i++)
            fmt.fmt.pix_mp.plane_fmt[i].sizeimage = pFmt->planeSize[i];

        return exynos_v4l2_try_fmt(pCtx->videoCtx.hDevice, &fmt);
    }

    return -1;
}

int Codec_OSAL_RequestBuf(
    CodecOSALVideoContext   *pCtx,
    CodecOSAL_ReqBuf        *pReqBuf) {
    if ((pCtx != NULL) &&
        (pReqBuf != NULL) &&
        (pCtx->videoCtx.hDevice >= 0)) {
        return exynos_v4l2_reqbufs(pCtx->videoCtx.hDevice, pReqBuf);
    }

    return -1;
}

int Codec_OSAL_QueryBuf(
    CodecOSALVideoContext   *pCtx,
    CodecOSAL_Buffer        *pBuf) {
    if ((pCtx != NULL) &&
        (pBuf != NULL) &&
        (pCtx->videoCtx.hDevice >= 0)) {
        struct v4l2_buffer  buf;
        struct v4l2_plane   planes[VIDEO_BUFFER_MAX_PLANES];
        int i;

        memset(&buf, 0, sizeof(buf));
        memset(&planes, 0, sizeof(planes));

        buf.type        = pBuf->type;
        buf.m.planes    = planes;
        buf.length      = pBuf->nPlane;
        buf.memory      = pBuf->memory;

        if (exynos_v4l2_querybuf(pCtx->videoCtx.hDevice, &buf) == 0) {
            for (i = 0; i < (int)buf.length; i++) {
                pBuf->planes[i].bufferSize  = buf.m.planes[i].length;
                pBuf->planes[i].offset      = buf.m.planes[i].m.mem_offset;
            }

            return 0;
        }
    }

    return -1;
}

int Codec_OSAL_SetStreamOn(
    CodecOSALVideoContext  *pCtx,
    int                     nPort) {
    if ((pCtx != NULL) &&
        (pCtx->videoCtx.hDevice >= 0)) {
        return exynos_v4l2_streamon(pCtx->videoCtx.hDevice, nPort);
    }

    return -1;
}

int Codec_OSAL_SetStreamOff(
    CodecOSALVideoContext  *pCtx,
    int                     nPort) {
    if ((pCtx != NULL) &&
        (pCtx->videoCtx.hDevice >= 0)) {
        return exynos_v4l2_streamoff(pCtx->videoCtx.hDevice, nPort);
    }

    return -1;
}

void *Codec_OSAL_MemoryMap(void *addr, size_t len, int prot, int flags, unsigned long fd, off_t offset) {
    return mmap(addr, len, prot, flags, fd, offset);
}

int Codec_OSAL_MemoryUnmap(void *addr, size_t len) {
    return munmap(addr, len);
}

#ifndef USE_EPOLL
int Codec_OSAL_Poll(
    CodecOSALVideoContext  *pCtx,
    CodecOSAL_Pollfd        pollfd[CODEC_OSAL_MAX_POLLFD],
    int                     cnt,
    int                     time) {
    struct pollfd pollfds[CODEC_OSAL_MAX_POLLFD];

    int ret, i;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __FUNCTION__);
        return -1;
    }

    memset(&pollfds, 0, sizeof(pollfds));

    for (i = 0; i < cnt; i++) {
        pollfds[i].fd       = pollfd[i].fd;
        pollfds[i].events   = pollfd[i].events;
        pollfds[i].revents  = 0;
    }

    ret = poll((struct pollfd *)pollfds, cnt, time);

    for (i = 0; i < cnt; i++) {
        pollfd[i].revents = pollfds[i].revents;
    }

    return ret;
}
#else
typedef struct __CodecOSALEpoll {
    int epfd;
    CodecOSAL_Pollfd pollfds[CODEC_OSAL_MAX_POLLFD];
    int cnt;
} CodecOSALEpoll;

void *Codec_OSAL_Epoll_Create() {
    void            *handle = NULL;
    CodecOSALEpoll  *pEpoll = NULL;

    handle = (void *)malloc(sizeof(CodecOSALEpoll));
    if (handle == NULL) {
        ALOGE("[%s] malloc() is failed", __FUNCTION__);
        return (void*)handle;
    }
    memset(handle, 0, sizeof(CodecOSALEpoll));

    pEpoll = (CodecOSALEpoll *)handle;

    pEpoll->epfd = epoll_create(1);
    if (pEpoll->epfd <= 0) {
        free(handle);
        handle = NULL;
        ALOGE("[%s] epoll_create() is failed", __FUNCTION__);
    }

    return handle;
}

int Codec_OSAL_Epoll_Regist(
    void               *pHandle,
    CodecOSAL_Pollfd    pollfd[CODEC_OSAL_MAX_POLLFD],
    int                 cnt) {
    CodecOSALEpoll *pEpoll = (CodecOSALEpoll *)pHandle;
    int err = 0;
    int i = 0;

    if (pEpoll == NULL) {
        ALOGE("[%s] bad parameter", __FUNCTION__);
        return -1;
    }

    if (pEpoll->epfd < 0) {
        ALOGE("[%s] bad parameter", __FUNCTION__);
        return -1;
    }

    for (i = 0; i < cnt; i++) {
        struct epoll_event event;

        memset(&event, 0, sizeof(event));
        event.data.fd = pollfd[i].fd;
        event.events  = pollfd[i].events;

        err = epoll_ctl(pEpoll->epfd, EPOLL_CTL_ADD, pollfd[i].fd, &event);
        if (err < 0) {
            ALOGE("[%s] epoll_ctl(EPOLL_CTL_ADD) is failed: err(0x%x)", __FUNCTION__, err);
            break;
        }
    }

    memcpy(pEpoll->pollfds, pollfd, (sizeof(CodecOSAL_Pollfd) * CODEC_OSAL_MAX_POLLFD));
    pEpoll->cnt = cnt;

    return err;
}

int Codec_OSAL_Epoll(
    void               *pHandle,
    CodecOSAL_Pollfd    pollfd[CODEC_OSAL_MAX_POLLFD],
    int                 time) {
    CodecOSALEpoll *pEpoll = (CodecOSALEpoll *)pHandle;
    int cnt = 0;

    struct epoll_event events[CODEC_OSAL_MAX_POLLFD];

    int i = 0;

    if (pEpoll == NULL) {
        ALOGE("[%s] bad parameter", __FUNCTION__);
        return -1;
    }

    if (pEpoll->epfd < 0) {
        ALOGE("[%s] bad parameter", __FUNCTION__);
        return -1;
    }

    memset(pollfd, 0, (sizeof(CodecOSAL_Pollfd) * CODEC_OSAL_MAX_POLLFD));
    memset(events, 0, sizeof(events));

    cnt = epoll_wait(pEpoll->epfd, events, CODEC_OSAL_MAX_POLLFD, time);
    if (cnt > 0) {
        for (i = 0; i < cnt; i++) {
            pollfd[i].fd        = events[i].data.fd;
            pollfd[i].revents   = events[i].events;
        }
    }

    return cnt;
}

void Codec_OSAL_Epoll_Destroy(void *pHandle) {
    CodecOSALEpoll *pEpoll = (CodecOSALEpoll *)pHandle;
    int err = 0;
    int i = 0;

    if (pEpoll == NULL) {
        ALOGE("[%s] bad parameter", __FUNCTION__);
        return;
    }

    if (pEpoll->epfd < 0) {
        ALOGE("[%s] bad parameter", __FUNCTION__);
        return;
    }

    for (i = 0; i < pEpoll->cnt; i++) {
        struct epoll_event event;

        memset(&event, 0, sizeof(event));
        event.data.fd = pEpoll->pollfds[i].fd;
        event.events  = pEpoll->pollfds[i].events;

        err = epoll_ctl(pEpoll->epfd, EPOLL_CTL_DEL, pEpoll->pollfds[i].fd, &event);
        if (err < 0) {
            ALOGE("[%s] epoll_ctl(EPOLL_CTL_DEL) is failed: err(0x%x)", __FUNCTION__, err);
            continue;
        }
    }

    pEpoll->cnt = 0;

    close(pEpoll->epfd);
    pEpoll->epfd = -1;

    free(pEpoll);
    pEpoll = NULL;

    return;
}
#endif

