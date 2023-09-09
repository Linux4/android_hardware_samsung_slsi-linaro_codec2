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

/*
 * @file        ExynosVideoDecoder.c
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 *              Taehwan Kim   (t_h.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.01.15: Initial Version
 *   2016.01.28: Update Version to support OSAL
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <pthread.h>

#include <sys/poll.h>

#ifdef USE_USER_POLL_EVENT
#include <sys/eventfd.h>
#endif

#include "ExynosVideoApi.h"
#include "ExynosVideoCommonApi.h"
#include "ExynosVideoDec.h"
#include "ExynosVideo_OSAL_Dec.h"

/* #define LOG_NDEBUG 0 */
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "ExynosVideoDecoder"

#include "ExynosVideo_OSAL_Log.h"

#define MAX_INPUTBUFFER_COUNT 32
#define MAX_OUTPUTBUFFER_COUNT 32

#define ALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))

static void __Set_InstInfo(ExynosVideoInstInfo *pVideoInstInfo, int mode) {
    pVideoInstInfo->supportInfo.dec.bRealTimePrioritySupport    = (mode & (0x1 << 23))? VIDEO_TRUE:VIDEO_FALSE;

    pVideoInstInfo->bVideoBufFlagCtrl = (mode & (0x1 << 16))? VIDEO_TRUE:VIDEO_FALSE;

    pVideoInstInfo->supportInfo.dec.bHDRDynamicBlobSupport      = (mode & (0x1 << 9))? VIDEO_TRUE:VIDEO_FALSE;

    pVideoInstInfo->supportInfo.dec.bOperatingRateSupport       = (mode & (0x1 << 8))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->supportInfo.dec.bFrameErrorTypeSupport      = (mode & (0x1 << 7))? VIDEO_TRUE:VIDEO_FALSE;

    /* bit(6) represents some features support required at c2 */
    pVideoInstInfo->supportInfo.dec.bPocSupport                 = (mode & (0x1 << 6))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->supportInfo.dec.bDisplayDelayInfoSupport    = (mode & (0x1 << 6))? VIDEO_TRUE:VIDEO_FALSE;

    pVideoInstInfo->supportInfo.dec.bDrvDPBManageSupport        = (mode & (0x1 << 5))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->supportInfo.dec.bHDRDynamicInfoSupport      = (mode & (0x1 << 4))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->supportInfo.dec.bSkypeSupport               = (mode & (0x1 << 3))? VIDEO_TRUE:VIDEO_FALSE;
}

/*
 * [Common] __Set_SupportFormat
 */
static void __Set_SupportFormat(ExynosVideoInstInfo *pVideoInstInfo) {
    int nLastIndex = 0;

    if (CHECK_POINTER(pVideoInstInfo) == false) {
        goto EXIT;
    }

    memset(pVideoInstInfo->supportFormat, (int)VIDEO_COLORFORMAT_UNKNOWN,
            sizeof(pVideoInstInfo->supportFormat));

#ifdef USE_HEVC_HWIP
    if (pVideoInstInfo->eCodecType == VIDEO_CODING_HEVC) {
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_YV12M;
        goto EXIT;
    }
#endif

    switch (pVideoInstInfo->HwVersion) {
    case MFC_170:
        /* NV12, NV21, I420, YV12, SBWC(NV12, NV21, NV12_10B, NV21_10B) */
        /* 2 plane */
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M;

        /* 3 plane */
        if (pVideoInstInfo->eCodecType != VIDEO_CODING_AV1) {
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420M;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_YV12M;
        }

        /* 10bit */
        if ((pVideoInstInfo->eCodecType == VIDEO_CODING_HEVC) ||
            (pVideoInstInfo->eCodecType == VIDEO_CODING_VP9) ||
            (pVideoInstInfo->eCodecType == VIDEO_CODING_AV1)) {
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_P010;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_P010;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_P010;
        }

        /* SBWC */
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_SBWC;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_SBWC;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_SBWC;
#ifdef USE_SUPPORT_GPU_SBWC
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_256_SBWC;
#endif

        /* SBWC 10bit */
        if ((pVideoInstInfo->eCodecType == VIDEO_CODING_HEVC) ||
            (pVideoInstInfo->eCodecType == VIDEO_CODING_VP9) ||
            (pVideoInstInfo->eCodecType == VIDEO_CODING_AV1)) {
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_10B_SBWC;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_10B_SBWC;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_10B_SBWC;
#ifdef USE_SUPPORT_GPU_SBWC
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_10B_256_SBWC;
#endif
        }
        break;
    case MFC_1601:
        /* NV12, NV21, I420, YV12, SBWC(NV12, NV21, NV12_10B, NV21_10B) */
        /* 2 plane */
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M;

        /* 3 plane */
        if (pVideoInstInfo->eCodecType != VIDEO_CODING_AV1) {
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420M;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_YV12M;
        }

        /* 10bit */
        if ((pVideoInstInfo->eCodecType == VIDEO_CODING_HEVC) ||
            (pVideoInstInfo->eCodecType == VIDEO_CODING_VP9) ||
            (pVideoInstInfo->eCodecType == VIDEO_CODING_AV1)) {
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_P010;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_P010;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_P010;
        }

        /* SBWC */
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_SBWC;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_SBWC;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_SBWC;

        /* SBWC 10bit */
        if ((pVideoInstInfo->eCodecType == VIDEO_CODING_HEVC) ||
            (pVideoInstInfo->eCodecType == VIDEO_CODING_VP9) ||
            (pVideoInstInfo->eCodecType == VIDEO_CODING_AV1)) {
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_10B_SBWC;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_10B_SBWC;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_10B_SBWC;
        }
        break;
    case MFC_160:
    case MFC_1506:
    case MFC_1501:
        /* NV12, NV21, I420, YV12, SBWC(NV12, NV21, NV12_10B, NV21_10B) */
        /* 2 plane */
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M;

        if ((pVideoInstInfo->eCodecType != VIDEO_CODING_AV1) ||
            (pVideoInstInfo->HwVersion == MFC_160)) {
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M;
        }

        /* 3 plane */
        if (pVideoInstInfo->eCodecType != VIDEO_CODING_AV1) {
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420M;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_YV12M;
        }

        /* 10bit */
        if ((pVideoInstInfo->eCodecType == VIDEO_CODING_HEVC) ||
            (pVideoInstInfo->eCodecType == VIDEO_CODING_VP9) ||
            (pVideoInstInfo->eCodecType == VIDEO_CODING_AV1)) {
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_P010;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_P010;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_P010;
        }

        /* SBWC */
        if (pVideoInstInfo->eCodecType != VIDEO_CODING_AV1) {
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_SBWC;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_SBWC;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_SBWC;
        }

        /* SBWC 10bit */
        if ((pVideoInstInfo->eCodecType == VIDEO_CODING_HEVC) ||
            (pVideoInstInfo->eCodecType == VIDEO_CODING_VP9)) {
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_10B_SBWC;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_10B_SBWC;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_10B_SBWC;
        }
        break;
    case MFC_150:
        /* NV12, NV21, I420, YV12, SBWC(NV12, NV21, NV12_10B, NV21_10B) */
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_YV12M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_SBWC;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_SBWC;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_SBWC;

        if ((pVideoInstInfo->eCodecType == VIDEO_CODING_HEVC) ||
            (pVideoInstInfo->eCodecType == VIDEO_CODING_VP9)) {
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_P010;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_P010;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_P010;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_10B_SBWC;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_10B_SBWC;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_10B_SBWC;
        }
        break;
    case MFC_1421:
    case MFC_142:
    case MFC_1410:
    case MFC_1400:
    case MFC_140:
        /* NV12, NV21, I420, YV12, NV12_S10B, NV21_S10B, SBWC(NV12, NV21, NV12_10B, NV21_10B) */
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_YV12M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_SBWC;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_SBWC;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_SBWC;

        if ((pVideoInstInfo->eCodecType == VIDEO_CODING_HEVC) ||
            (pVideoInstInfo->eCodecType == VIDEO_CODING_VP9)) {
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_S10B;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_S10B;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_P010;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_P010;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_S10B;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_P010;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_10B_SBWC;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_10B_SBWC;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_10B_SBWC;
        }
        break;
    case MFC_130:  /* NV12, NV21, I420, YV12, NV12_10B, NV21_10B */
    case MFC_120:
    case MFC_1220:
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_YV12M;

        if ((pVideoInstInfo->eCodecType == VIDEO_CODING_HEVC) ||
            (pVideoInstInfo->eCodecType == VIDEO_CODING_VP9)) {
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_S10B;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_S10B;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_P010;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_P010;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_S10B;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_P010;
        }
        break;
    case MFC_1420:  /* NV12, NV21, I420, YV12 */
    case MFC_110:
    case MFC_1120:
    case MFC_101:
    case MFC_100:
    case MFC_1010:
    case MFC_1011:
    case MFC_90:
    case MFC_80:
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_YV12M;
        break;
    case MFC_92:  /* NV12, NV21 */
    case MFC_78D:
    case MFC_1020:
    case MFC_1021:
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M;
        break;
    case MFC_723:  /* NV12T, [NV12, NV21, I420, YV12] */
    case MFC_72:
    case MFC_77:
#if 0
        if (pVideoInstInfo->supportInfo.dec.bDualDPBSupport == VIDEO_TRUE) {
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420M;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_YV12M;
        }
#endif
    case MFC_78:  /* NV12T */
    case MFC_65:
    case MFC_61:
    case MFC_51:
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_TILED;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_TILED;
        break;
    default:
        break;
    }

EXIT:
    return;
}

static ExynosVideoErrorType __GetInstInfo(
    CodecOSALVideoContext   *pCtx,
    ExynosVideoInstInfo     *pVideoInstInfo) {
    ExynosVideoErrorType ret = VIDEO_ERROR_NONE;

    int mode = 0, version = 0;

    if ((pVideoInstInfo == NULL) ||
        (pCtx == NULL)) {
        ALOGE("%s: bad parameter", __FUNCTION__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (Codec_OSAL_GetControl(pCtx, CODEC_OSAL_CID_VIDEO_GET_VERSION_INFO, &version) != 0) {
        ALOGW("%s: HW version information is not available", __FUNCTION__);
#ifdef USE_HEVC_HWIP
        if (pVideoInstInfo->eCodecType == VIDEO_CODING_HEVC)
            pVideoInstInfo->HwVersion = (int)HEVC_10;
        else
#endif
            pVideoInstInfo->HwVersion = (int)MFC_65;
    } else {
        pVideoInstInfo->HwVersion = version;
    }

    if (Codec_OSAL_GetControl(pCtx, CODEC_OSAL_CID_VIDEO_GET_EXT_INFO, &mode) != 0) {
        pVideoInstInfo->supportInfo.dec.bSkypeSupport = VIDEO_FALSE;
        goto EXIT;
    }

    __Set_InstInfo(pVideoInstInfo, mode);

    pVideoInstInfo->SwVersion = 0;
    if (pVideoInstInfo->supportInfo.dec.bSkypeSupport == VIDEO_TRUE) {
        int swVersion = 0;

        if (Codec_OSAL_GetControl(pCtx, CODEC_OSAL_CID_VIDEO_GET_DRIVER_VERSION, &swVersion) != 0) {
            ALOGE("%s: g_ctrl is failed(CODEC_OSAL_CID_VIDEO_GET_DRIVER_VERSION)", __FUNCTION__);
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }

        pVideoInstInfo->SwVersion = (unsigned int)swVersion;
    }

    __Set_SupportFormat(pVideoInstInfo);

    pVideoInstInfo->bInitialized = VIDEO_TRUE;

EXIT:
    return ret;
}

static void __PrintInstInfo(
    ExynosVideoInstInfo *pVideoInstInfo) {
    int i;

    if (pVideoInstInfo == NULL) {
        return;
    }

    ALOGV("[VideoInstInfo] Version : HW(0x%x), SW(0x%x)", pVideoInstInfo->HwVersion, pVideoInstInfo->SwVersion);
    ALOGV("[VideoInstInfo] Buf Flag(%d), "
                          "Priority(%d), "
                          "Operating Rate(%d), "
                          "Frame Error(%d), "
                          "POC(%d), "
                          "Display Delay(%d), "
                          "DPB Ref(%d), "
                          "HDR dynamic(%d), "
                          "HDR BLOB(%d), ",
        pVideoInstInfo->bVideoBufFlagCtrl,
        pVideoInstInfo->supportInfo.dec.bRealTimePrioritySupport,
        pVideoInstInfo->supportInfo.dec.bOperatingRateSupport,
        pVideoInstInfo->supportInfo.dec.bFrameErrorTypeSupport,
        pVideoInstInfo->supportInfo.dec.bPocSupport,
        pVideoInstInfo->supportInfo.dec.bDisplayDelayInfoSupport,
        pVideoInstInfo->supportInfo.dec.bDrvDPBManageSupport,
        pVideoInstInfo->supportInfo.dec.bHDRDynamicInfoSupport,
        pVideoInstInfo->supportInfo.dec.bHDRDynamicBlobSupport);

    for (i = 0; i < VIDEO_COLORFORMAT_MAX; i++) {
        if (pVideoInstInfo->supportFormat[i] == VIDEO_COLORFORMAT_UNKNOWN) {
            break;
        }

        ALOGV("[VideoInstInfo] support format[%d] : 0x%x", i, pVideoInstInfo->supportFormat[i]);
    }

    return;
}

void DecSpecificClear(CodecOSALVideoContext *pCtx) {
    if (pCtx->videoCtx.instInfo.supportInfo.dec.bDrvDPBManageSupport != VIDEO_TRUE) {
        if (pCtx->videoCtx.specificInfo.dec.pRefDPBAddr != NULL) {
            Codec_OSAL_MemoryUnmap(pCtx->videoCtx.specificInfo.dec.pRefDPBAddr,
                    sizeof(RefDPBInfo) * VIDEO_BUFFER_MAX_NUM);
            pCtx->videoCtx.specificInfo.dec.pRefDPBAddr = NULL;
        }

        /* free a ion_buffer */
        if (pCtx->videoCtx.specificInfo.dec.nRefDPBFD >= 0) {
            close(pCtx->videoCtx.specificInfo.dec.nRefDPBFD);
            pCtx->videoCtx.specificInfo.dec.nRefDPBFD = -1;
        }
    }

    if (pCtx->videoCtx.instInfo.supportInfo.dec.bHDRDynamicInfoSupport == VIDEO_TRUE) {
        bool hasBlob = false;
        if (pCtx->videoCtx.instInfo.supportInfo.dec.bHDRDynamicBlobSupport == VIDEO_TRUE) {
            hasBlob = true;
        }

        if (pCtx->videoCtx.specificInfo.dec.pHDRDynamicInfoAddr != NULL) {
            Codec_OSAL_MemoryUnmap(pCtx->videoCtx.specificInfo.dec.pHDRDynamicInfoAddr,
                    (hasBlob ? sizeof(ExynosVideoHdrDynamicBlob) : sizeof(ExynosVideoHdrDynamic)) * VIDEO_BUFFER_MAX_NUM);
            pCtx->videoCtx.specificInfo.dec.pHDRDynamicInfoAddr = NULL;
        }

        /* free a ion_buffer */
        if (pCtx->videoCtx.specificInfo.dec.nHDRDynamicInfoFD >= 0) {
            close(pCtx->videoCtx.specificInfo.dec.nHDRDynamicInfoFD);
            pCtx->videoCtx.specificInfo.dec.nHDRDynamicInfoFD = -1;
        }
    }

    if (pCtx->videoCtx.instInfo.eCodecType == VIDEO_CODING_AV1) {
        if (pCtx->videoCtx.specificInfo.dec.pFilmGrainAddr != NULL) {
            Codec_OSAL_MemoryUnmap(pCtx->videoCtx.specificInfo.dec.pFilmGrainAddr,
                    sizeof(ExynosVideoFilmGrain) * VIDEO_BUFFER_MAX_NUM);
            pCtx->videoCtx.specificInfo.dec.pFilmGrainAddr = NULL;
        }

        /* free a ion_buffer */
        if (pCtx->videoCtx.specificInfo.dec.nFilmGrainFD >= 0) {
            close(pCtx->videoCtx.specificInfo.dec.nFilmGrainFD);
            pCtx->videoCtx.specificInfo.dec.nFilmGrainFD = -1;
        }
    }
}

void MFC_Decoder_ResourceClear(CodecOSALVideoContext *pCtx) {
    MFC_ResourceClear(pCtx, DecSpecificClear);
}

/*
 * [Decoder OPS] Init
 */
static void *MFC_Decoder_Init(ExynosVideoInstInfo *pVideoInfo) {
    CodecOSALVideoContext *pCtx     = NULL;

    int ret = 0;
    int fd = -1;

    int i;

    if (CHECK_POINTER(pVideoInfo) == false) {
        goto EXIT_ALLOC_FAIL;
    }

    pCtx = (CodecOSALVideoContext *)malloc(sizeof(CodecOSALVideoContext));
    if (CHECK_POINTER(pCtx) == false) {
        goto EXIT_ALLOC_FAIL;
    }
    memset(pCtx, 0, sizeof(*pCtx));

    pCtx->videoCtx.hDevice = -1;
    pCtx->videoCtx.hUser = -1;
    pCtx->videoCtx.hIONHandle = -1;
    pCtx->videoCtx.specificInfo.dec.nRefDPBFD = -1;
    pCtx->videoCtx.specificInfo.dec.nHDRDynamicInfoFD = -1;
    pCtx->videoCtx.specificInfo.dec.nFilmGrainFD = -1;

    /* node open */
#ifdef USE_HEVC_HWIP
    if (pVideoInfo->eCodecType == VIDEO_CODING_HEVC) {
        if (pVideoInfo->eSecurityType == VIDEO_SECURE)
            ret = Codec_OSAL_DevOpen(VIDEO_HEVC_SECURE_DECODER_NAME, O_RDWR, pCtx);
        else
            ret = Codec_OSAL_DevOpen(VIDEO_HEVC_DECODER_NAME, O_RDWR, pCtx);
    } else
#endif
    {
        if (pVideoInfo->eSecurityType == VIDEO_SECURE)
            ret = Codec_OSAL_DevOpen(VIDEO_MFC_SECURE_DECODER_NAME, O_RDWR, pCtx);
        else
            ret = Codec_OSAL_DevOpen(VIDEO_MFC_DECODER_NAME, O_RDWR, pCtx);
    }

    if (ret < 0) {
        ALOGE("%s: Failed to open decoder device", __FUNCTION__);
        goto EXIT_OPEN_FAIL;
    }

    if (pVideoInfo->bInitialized == VIDEO_FALSE) {
        __GetInstInfo(pCtx, pVideoInfo);
    }

    memcpy(&pCtx->videoCtx.instInfo, pVideoInfo, sizeof(*pVideoInfo));

    __PrintInstInfo(&pCtx->videoCtx.instInfo);

#ifdef USE_USER_POLL_EVENT
    pCtx->videoCtx.hUser = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (pCtx->videoCtx.hUser == -1) {
        ALOGE("%s: Failed to eventfd() for user poll event", __FUNCTION__);
        goto EXIT_QUERYCAP_FAIL;
    }
#endif

    ret = MFC_Init_Common(pCtx);
    if (ret < 0) {
        goto EXIT_QUERYCAP_FAIL;
    }

    /* for DPB ref. count handling */
    if (pCtx->videoCtx.instInfo.supportInfo.dec.bDrvDPBManageSupport != VIDEO_TRUE) {
        fd = exynos_ion_alloc(pCtx->videoCtx.hIONHandle, sizeof(RefDPBInfo) * VIDEO_BUFFER_MAX_NUM,
                            EXYNOS_ION_HEAP_SYSTEM_MASK, ION_FLAG_CACHED);
        if (fd < 0) {
            ALOGE("%s: Failed to exynos_ion_alloc() for nPrivateDataShareFD", __FUNCTION__);
            pCtx->videoCtx.specificInfo.dec.nRefDPBFD = -1;
            goto EXIT_QUERYCAP_FAIL;
        }
        pCtx->videoCtx.specificInfo.dec.nRefDPBFD = fd;

        pCtx->videoCtx.specificInfo.dec.pRefDPBAddr =
                                        Codec_OSAL_MemoryMap(NULL, (sizeof(RefDPBInfo) * VIDEO_BUFFER_MAX_NUM),
                                            PROT_READ | PROT_WRITE, MAP_SHARED,
                                            pCtx->videoCtx.specificInfo.dec.nRefDPBFD, 0);
        if (pCtx->videoCtx.specificInfo.dec.pRefDPBAddr == MAP_FAILED) {
            ALOGE("%s: Failed to mmap for pRefDPBAddr", __FUNCTION__);
            pCtx->videoCtx.specificInfo.dec.pRefDPBAddr = NULL;
            goto EXIT_QUERYCAP_FAIL;
        }

        memset(pCtx->videoCtx.specificInfo.dec.pRefDPBAddr,
                0, sizeof(RefDPBInfo) * VIDEO_BUFFER_MAX_NUM);
    }

    /* for HDR Dynamic Info */
    if (pCtx->videoCtx.instInfo.supportInfo.dec.bHDRDynamicInfoSupport == VIDEO_TRUE) {
        bool hasBlob = false;
        if (pCtx->videoCtx.instInfo.supportInfo.dec.bHDRDynamicBlobSupport == VIDEO_TRUE) {
            hasBlob = true;
        }

        fd = exynos_ion_alloc(pCtx->videoCtx.hIONHandle,
                              ((hasBlob ? sizeof(ExynosVideoHdrDynamicBlob) : sizeof(ExynosVideoHdrDynamic)) * VIDEO_BUFFER_MAX_NUM),
                              EXYNOS_ION_HEAP_SYSTEM_MASK, ION_FLAG_CACHED);
        if (fd < 0) {
            ALOGE("[%s] Failed to exynos_ion_alloc() for pHDRInfoShareBufferFD", __FUNCTION__);
            pCtx->videoCtx.specificInfo.dec.nHDRDynamicInfoFD = -1;
            goto EXIT_QUERYCAP_FAIL;
        }
        pCtx->videoCtx.specificInfo.dec.nHDRDynamicInfoFD = fd;

        pCtx->videoCtx.specificInfo.dec.pHDRDynamicInfoAddr =
                                        Codec_OSAL_MemoryMap(NULL,
                                              ((hasBlob ? sizeof(ExynosVideoHdrDynamicBlob) : sizeof(ExynosVideoHdrDynamic)) * VIDEO_BUFFER_MAX_NUM),
                                              PROT_READ | PROT_WRITE, MAP_SHARED,
                                              pCtx->videoCtx.specificInfo.dec.nHDRDynamicInfoFD, 0);
        if (pCtx->videoCtx.specificInfo.dec.pHDRDynamicInfoAddr == MAP_FAILED) {
            ALOGE("[%s] Failed to Codec_OSAL_MemoryMap() for pHDRDynamicInfoAddr", __FUNCTION__);
            pCtx->videoCtx.specificInfo.dec.pHDRDynamicInfoAddr = NULL;
            goto EXIT_QUERYCAP_FAIL;
        }

        memset(pCtx->videoCtx.specificInfo.dec.pHDRDynamicInfoAddr, 0,
                ((hasBlob ? sizeof(ExynosVideoHdrDynamicBlob) : sizeof(ExynosVideoHdrDynamic)) * VIDEO_BUFFER_MAX_NUM));

        if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_VIDEO_SET_HDR_USER_SHARED_HANDLE,
                                    pCtx->videoCtx.specificInfo.dec.nHDRDynamicInfoFD) != 0) {
            ALOGE("[%s] Failed to Codec_OSAL_SetControl(CODEC_OSAL_CID_VIDEO_SET_HDR_USER_SHARED_HANDLE)", __FUNCTION__);
            goto EXIT_QUERYCAP_FAIL;
        }
    }

    /* AV1 only, for Film Grain */
    if (pCtx->videoCtx.instInfo.eCodecType == VIDEO_CODING_AV1) {
        fd = exynos_ion_alloc(pCtx->videoCtx.hIONHandle, sizeof(ExynosVideoFilmGrain) * VIDEO_BUFFER_MAX_NUM,
                              EXYNOS_ION_HEAP_SYSTEM_MASK, ION_FLAG_CACHED);
        if (fd < 0) {
            ALOGE("[%s] Failed to exynos_ion_alloc() for pFilmGrainFD", __FUNCTION__);
            pCtx->videoCtx.specificInfo.dec.nFilmGrainFD = -1;
            goto EXIT_QUERYCAP_FAIL;
        }
        pCtx->videoCtx.specificInfo.dec.nFilmGrainFD = fd;

        pCtx->videoCtx.specificInfo.dec.pFilmGrainAddr =
                                        Codec_OSAL_MemoryMap(NULL, (sizeof(ExynosVideoFilmGrain) * VIDEO_BUFFER_MAX_NUM),
                                              PROT_READ | PROT_WRITE, MAP_SHARED,
                                              pCtx->videoCtx.specificInfo.dec.nFilmGrainFD, 0);
        if (pCtx->videoCtx.specificInfo.dec.pFilmGrainAddr == MAP_FAILED) {
            ALOGE("[%s] Failed to Codec_OSAL_MemoryMap() for pFilmGrainAddr", __FUNCTION__);
            pCtx->videoCtx.specificInfo.dec.pFilmGrainAddr = NULL;
            goto EXIT_QUERYCAP_FAIL;
        }

        memset(pCtx->videoCtx.specificInfo.dec.pFilmGrainAddr,
                0, sizeof(ExynosVideoFilmGrain) * VIDEO_BUFFER_MAX_NUM);

        if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_VIDEO_SET_FILM_GRAIN_USER_SHARED_HANDLE,
                                    pCtx->videoCtx.specificInfo.dec.nFilmGrainFD) != 0) {
            ALOGE("[%s] Failed to Codec_OSAL_SetControl(CODEC_OSAL_CID_VIDEO_SET_FILM_GRAIN_USER_SHARED_HANDLE)", __FUNCTION__);
            goto EXIT_QUERYCAP_FAIL;
        }
    }

    return (void *)pCtx;

EXIT_QUERYCAP_FAIL:
    MFC_Decoder_ResourceClear(pCtx);

EXIT_OPEN_FAIL:
    free(pCtx);

EXIT_ALLOC_FAIL:
    return NULL;
}

/*
 * [Decoder OPS] Finalize
 */
static ExynosVideoErrorType MFC_Decoder_Finalize(void *pHandle) {
    CodecOSALVideoContext *pCtx         = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPlane      *pVideoPlane  = NULL;
    pthread_mutex_t       *pMutex       = NULL;
    ExynosVideoErrorType   ret          = VIDEO_ERROR_NONE;
    int i, j;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    MFC_Decoder_ResourceClear(pCtx);

    free(pCtx);

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Set Frame Tag
 */
static ExynosVideoErrorType MFC_Decoder_Set_FrameTag(
    void *pHandle,
    int   nFrameTag) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_VIDEO_FRAME_TAG, nFrameTag) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Get Frame Tag
 */
static int MFC_Decoder_Get_FrameTag(void *pHandle) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    int nFrameTag = -1;

    if (CHECK_POINTER(pCtx) == false) {
        goto EXIT;
    }

    Codec_OSAL_GetControl(pCtx, CODEC_OSAL_CID_VIDEO_FRAME_TAG, &nFrameTag);

EXIT:
    return nFrameTag;
}

/*
 * [Decoder OPS] Get Buffer Count
 */
static int MFC_Decoder_Get_ActualBufferCount(void *pHandle) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    int bufferCount = 0;

    if (CHECK_POINTER(pCtx) == false) {
        goto EXIT;
    }

    Codec_OSAL_GetControl(pCtx, CODEC_OSAL_CID_DEC_NUM_MIN_BUFFERS, &bufferCount);

EXIT:
    return bufferCount;
}

/*
 * [Decoder OPS] Get Display Delay
 */
static int MFC_Decoder_Get_DisplayDelay(void *pHandle) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    int delay = -1;

    if (CHECK_POINTER(pCtx) == false) {
        goto EXIT;
    }

    if (pCtx->videoCtx.instInfo.supportInfo.dec.bDisplayDelayInfoSupport == VIDEO_FALSE) {
        ALOGE("%s: can not support Get_DisplayDelay. // codec type(%x), F/W ver(%x)",
                    __FUNCTION__, pCtx->videoCtx.instInfo.eCodecType, pCtx->videoCtx.instInfo.HwVersion);
        goto EXIT;
    }

    if (Codec_OSAL_GetControl(pCtx, CODEC_OSAL_CID_DEC_GET_DISPLAY_DELAY, &delay) != 0) {
        goto EXIT;
    }

EXIT:
    return delay;
}

/*
 * [Decoder OPS] Set Display Delay
 */
static ExynosVideoErrorType MFC_Decoder_Set_DisplayDelay(
    void *pHandle,
    int   nDelay) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_DEC_DISPLAY_DELAY, nDelay) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Set I-Frame Decoding
 */
static ExynosVideoErrorType MFC_Decoder_Set_IFrameDecoding(void *pHandle) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

#ifdef USE_HEVC_HWIP
    if ((pCtx->videoCtx.instInfo.eCodecType != VIDEO_CODING_HEVC) &&
        (pCtx->videoCtx.instInfo.HwVersion == (int)MFC_51))
#else
    if (pCtx->videoCtx.instInfo.HwVersion == (int)MFC_51)
#endif
        return MFC_Decoder_Set_DisplayDelay(pHandle, 0);

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_DEC_I_FRAME_DECODING, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Enable Packed PB
 */
static ExynosVideoErrorType MFC_Decoder_Enable_PackedPB(void *pHandle) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_DEC_PACKED_PB, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Enable Loop Filter
 */
static ExynosVideoErrorType MFC_Decoder_Enable_LoopFilter(void *pHandle) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_DEC_DEBLOCK_FILTER, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Enable Slice Mode
 */
static ExynosVideoErrorType MFC_Decoder_Enable_SliceMode(void *pHandle) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_DEC_SLICE_MODE, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Enable Discard RCV header
 */
static ExynosVideoErrorType MFC_Decoder_Enable_DiscardRcvHeader(void *pHandle) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

#if 0  // TODO
    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_DEC_DISCARD_RCVHEADER, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }
#else
    ALOGW("%s: not implemented yet", __FUNCTION__);
#endif

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Enable Decoding Timestamp Mode
 */
static ExynosVideoErrorType MFC_Decoder_Enable_DTSMode(void *pHandle) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_DEC_DTS_MODE, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Set Qos Ratio
 */
static ExynosVideoErrorType MFC_Decoder_Set_QosRatio(
    void *pHandle,
    int   nRatio) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_VIDEO_QOS_RATIO, nRatio) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Enable SEI Parsing
 */
static ExynosVideoErrorType MFC_Decoder_Enable_SEIParsing(void *pHandle) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_DEC_SEI_PARSING, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Get Frame Packing information
 */
static ExynosVideoErrorType MFC_Decoder_Get_FramePackingInfo(
    void                    *pHandle,
    ExynosVideoFramePacking *pFramePacking) {
    CodecOSALVideoContext   *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType     ret  = VIDEO_ERROR_NONE;

    if ((CHECK_POINTER(pCtx) == false) || (CHECK_POINTER(pFramePacking) == false)) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (Codec_OSAL_GetControls(pCtx, CODEC_OSAL_CID_DEC_SEI_INFO, (void *)pFramePacking) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Get HDR Info
 */
static ExynosVideoErrorType MFC_Decoder_Get_HDRInfo(
    void                *pHandle,
    ExynosVideoHdrInfo  *pHdrInfo) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo    *pDstPad  = NULL;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;

    if ((CHECK_POINTER(pCtx) == false) ||
        (CHECK_POINTER(pHdrInfo) == false)) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    pDstPad = &(pCtx->videoCtx.pad[VIDEO_INDEX_DST_PAD]);

    /* get Color Aspects and HDR Static Info
     * in case of HDR Dynamic Info is already updated at dqbuf */
    if (pDstPad->geometry.HdrInfo.eValidType & ~HDR_INFO_DYNAMIC_META) {
        if (Codec_OSAL_GetControls(pCtx, CODEC_OSAL_CID_DEC_HDR_INFO,
                                (void *)&(pDstPad->geometry.HdrInfo)) != 0) {
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }
    }

    memcpy(pHdrInfo, &(pDstPad->geometry.HdrInfo), sizeof(ExynosVideoHdrInfo));

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Set Search Black Bar
 */
static ExynosVideoErrorType MFC_Decoder_Set_SearchBlackBar(
    void                *pHandle,
    ExynosVideoBoolType  bUse) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_DEC_SEARCH_BLACK_BAR, bUse) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Get BlackBarCrop
 */
static ExynosVideoErrorType MFC_Decoder_Get_BlackBarCrop(
    void            *pHandle,
    ExynosVideoRect *pBufferCrop) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    CodecOSAL_Crop  crop;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (CHECK_POINTER(pBufferCrop) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    memset(&crop, 0, sizeof(crop));

    crop.type = CODEC_OSAL_BUF_TYPE_DST;
    if (Codec_OSAL_GetCrop(pCtx, &crop) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pBufferCrop->nTop      = crop.top;
    pBufferCrop->nLeft     = crop.left;
    pBufferCrop->nWidth    = crop.width;
    pBufferCrop->nHeight   = crop.height;

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Get Actual Format
 */
static int MFC_Decoder_Get_ActualFormat(void *pHandle) {
    CodecOSALVideoContext     *pCtx         = (CodecOSALVideoContext *)pHandle;
    ExynosVideoGeometry       *pOutGeometry = NULL;
    ExynosVideoColorFormatType nVideoFormat = VIDEO_COLORFORMAT_UNKNOWN;
    int                        nV4l2Format  = 0;

    if (CHECK_POINTER(pCtx) == false) {
        goto EXIT;
    }

    if (Codec_OSAL_GetControl(pCtx, CODEC_OSAL_CID_DEC_ACTUAL_FORMAT, &nV4l2Format) != 0) {
        ALOGE("%s: Get extra format is failed", __FUNCTION__);
        goto EXIT;
    }

    nVideoFormat = Codec_OSAL_PixelFormatToColorFormat((unsigned int)nV4l2Format);

    pOutGeometry = &(pCtx->videoCtx.pad[VIDEO_INDEX_DST_PAD].geometry);
    switch ((int)pOutGeometry->eColorFormat) {
    case VIDEO_COLORFORMAT_NV12_SBWC:
    case VIDEO_COLORFORMAT_NV12_256_SBWC:
        if (nVideoFormat == VIDEO_COLORFORMAT_NV12) {
            nVideoFormat = VIDEO_COLORFORMAT_NV12_DECOMP;
        }
        break;
    case VIDEO_COLORFORMAT_NV12_10B_SBWC:
    case VIDEO_COLORFORMAT_NV12_10B_256_SBWC:
        if (nVideoFormat == VIDEO_COLORFORMAT_NV12_P010) {
            nVideoFormat = VIDEO_COLORFORMAT_NV12_P010_DECOMP;
        }
        break;
    default:
        break;
    }

EXIT:
    return nVideoFormat;
}

/*
 * [Decoder OPS] Get Actual Framerate
 */
static int MFC_Decoder_Get_ActualFramerate(void *pHandle) {
    CodecOSALVideoContext     *pCtx         = (CodecOSALVideoContext *)pHandle;
    int                        nFrameRate   = -1;

    if (CHECK_POINTER(pCtx) == false) {
        goto EXIT;
    }

    if (Codec_OSAL_GetControl(pCtx, CODEC_OSAL_CID_ACTUAL_FRAMERATE, &nFrameRate) != 0) {
        ALOGW("%s: Get Actual frame rate is failed", __FUNCTION__);
        goto EXIT;
    }

EXIT:
    return nFrameRate;
}

/*
 * [Decoder OPS] Enable Decode Order
 */
static ExynosVideoErrorType MFC_Decoder_Enable_DecodeOrder(void *pHandle) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_DEC_DECODE_ORDER, 1) != 0) {
        ALOGE("%s: Failed to enable decoding order", __FUNCTION__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Get Film Grain Info
 */
static ExynosVideoErrorType MFC_Decoder_Get_FilmGrainInfo(
    void                 *pHandle,
    ExynosVideoFilmGrain *pFilmGrainInfo) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo    *pDstPad  = NULL;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;

    if ((CHECK_POINTER(pCtx) == false) ||
        (CHECK_POINTER(pFilmGrainInfo) == false)) {
        return VIDEO_ERROR_BADPARAM;
    }

    pDstPad = &(pCtx->videoCtx.pad[VIDEO_INDEX_DST_PAD]);
    memcpy(pFilmGrainInfo, &(pDstPad->geometry.FilmGrainInfo), sizeof(ExynosVideoFilmGrain));

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Set Operating Rate
 */
static ExynosVideoErrorType MFC_Decoder_Set_OperatingRate(
    void        *pHandle,
    unsigned int framerate) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __FUNCTION__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->videoCtx.instInfo.supportInfo.dec.bOperatingRateSupport != VIDEO_TRUE) {
        ret = VIDEO_ERROR_NOSUPPORT;
        goto EXIT;
    }

    ret = MFC_Set_OperatingRate(pHandle, framerate);

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Set RealTimePriority
 */
static ExynosVideoErrorType MFC_Decoder_Set_RealTimePriority(
    void        *pHandle,
    unsigned int RealTimePriority) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->videoCtx.instInfo.supportInfo.dec.bRealTimePrioritySupport != VIDEO_TRUE) {
        ret = VIDEO_ERROR_NOSUPPORT;
        goto EXIT;
    }

    ret = MFC_Set_RealTimePriority(pHandle, RealTimePriority);

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Get Real Time Priority
 */
ExynosVideoErrorType MFC_Decoder_Get_RealTimePriority(void *pHandle) {
    return MFC_Get_RealTimePriority(pHandle);
}

/*
 * [Decoder OPS] Skip Lazy Unmap
 */
static ExynosVideoErrorType MFC_Decoder_Disable_LazyUnmap(void *pHandle) {
    return MFC_Disable_LazyUnmap(pHandle);
}

/*
 * [Decoder OPS] Stop Wait Buffer
 */
ExynosVideoErrorType MFC_Decoder_Stop_Wait_Buffer(void *pHandle) {
    return MFC_Stop_Wait_Buffer(pHandle);
}

/*
 * [Decoder OPS] Reset Wait Buffer
 */
ExynosVideoErrorType MFC_Decoder_Reset_Wait_Buffer(void *pHandle) {
    return MFC_Reset_Wait_Buffer(pHandle);
}

/*
 * [Decoder Buffer OPS] Enable Cacheable (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Enable_Cacheable_Inbuf(void *pHandle) {
    return MFC_Enable_Cacheable_Inbuf(pHandle);
}

/*
 * [Decoder Buffer OPS] Enable Cacheable (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Enable_Cacheable_Outbuf(void *pHandle) {
    return MFC_Enable_Cacheable_Outbuf(pHandle);
}

/*
 * [Decoder Buffer OPS] Set Geometry (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Set_Geometry_Inbuf(
    void                *pHandle,
    ExynosVideoGeometry *pBufferConf) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo    *pSrcPad  = NULL;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;

    CodecOSAL_Format fmt;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (CHECK_POINTER(pBufferConf) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    pSrcPad = &(pCtx->videoCtx.pad[VIDEO_INDEX_SRC_PAD]);

    memset(&fmt, 0, sizeof(fmt));
    fmt.type            = CODEC_OSAL_BUF_TYPE_SRC;
    fmt.format          = Codec_OSAL_CodingTypeToCompressdFormat(pBufferConf->eCompressionFormat);
    fmt.planeSize[0]    = pBufferConf->nSizeImage;
    fmt.nPlane          = pBufferConf->nPlaneCnt;

    if (Codec_OSAL_SetFormat(pCtx, &fmt) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pCtx->videoCtx.instInfo.eCodecType = pBufferConf->eCompressionFormat;

    memcpy(&(pSrcPad->geometry), pBufferConf, sizeof(ExynosVideoGeometry));
    pSrcPad->nPlane = pBufferConf->nPlaneCnt;

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Set Geometry (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Set_Geometry_Outbuf(
    void                *pHandle,
    ExynosVideoGeometry *pBufferConf) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo    *pDstPad  = NULL;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;

    CodecOSAL_Format fmt;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (CHECK_POINTER(pBufferConf) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    pDstPad = &(pCtx->videoCtx.pad[VIDEO_INDEX_DST_PAD]);

    memset(&fmt, 0, sizeof(fmt));
    fmt.type    = CODEC_OSAL_BUF_TYPE_DST;
    fmt.format  = Codec_OSAL_ColorFormatToPixelFormat(
                        pBufferConf->eColorFormat, pCtx->videoCtx.instInfo.HwVersion);
    fmt.nPlane  = pBufferConf->nPlaneCnt;
    fmt.width   = pBufferConf->nWidth;
    fmt.height  = pBufferConf->nHeight;

    if (Codec_OSAL_SetFormat(pCtx, &fmt) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    memcpy(&(pDstPad->geometry), pBufferConf, sizeof(ExynosVideoGeometry));
    pDstPad->nPlane = pBufferConf->nPlaneCnt;

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Get Geometry (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Get_Geometry_Outbuf(
    void                *pHandle,
    ExynosVideoGeometry *pBufferConf) {
    CodecOSALVideoContext *pCtx         = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret          = VIDEO_ERROR_NONE;
    ExynosVideoPadInfo    *pDstPad      = NULL;
    ExynosVideoGeometry   *pOutGeometry = NULL;

    CodecOSAL_Format fmt;
    CodecOSAL_Crop   crop;
    int i, value;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (CHECK_POINTER(pBufferConf) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    pDstPad = &(pCtx->videoCtx.pad[VIDEO_INDEX_DST_PAD]);

    memset(&fmt, 0, sizeof(fmt));
    memset(&crop, 0, sizeof(crop));

    fmt.type = CODEC_OSAL_BUF_TYPE_DST;
    if (Codec_OSAL_GetFormat(pCtx, &fmt) != 0) {
        switch (errno) {
        case EAGAIN:
            ret = VIDEO_ERROR_HEADERINFO;
            break;
        case EINVAL:
            ret = VIDEO_ERROR_NOSUPPORT;
            break;
        default:
            ret = VIDEO_ERROR_APIFAIL;
            break;
        }

        goto EXIT;
    }

    crop.type = CODEC_OSAL_BUF_TYPE_DST;
    if (Codec_OSAL_GetCrop(pCtx, &crop) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pOutGeometry = &(pDstPad->geometry);

    pOutGeometry->nWidth        = fmt.width;
    pOutGeometry->nHeight       = fmt.height;
    pOutGeometry->eColorFormat  = Codec_OSAL_PixelFormatToColorFormat(fmt.format);
    pOutGeometry->nStride       = fmt.stride;

    if ((fmt.field == CODEC_OSAL_INTER_TYPE_INTER) ||
        (fmt.field == CODEC_OSAL_INTER_TYPE_TB) ||
        (fmt.field == CODEC_OSAL_INTER_TYPE_BT))
        pOutGeometry->bInterlaced = VIDEO_TRUE;
    else
        pOutGeometry->bInterlaced = VIDEO_FALSE;

    switch (pOutGeometry->eColorFormat) {
        /* if it is 10bit contents, the format obtained from MFC D/D will be one of 10bit formats */
    case VIDEO_COLORFORMAT_NV12_S10B:
        pOutGeometry->eFilledDataType = DATA_8BIT_WITH_2BIT;
        pDstPad->nPlane = 1;
        break;
    case VIDEO_COLORFORMAT_NV12M_S10B:
    case VIDEO_COLORFORMAT_NV21M_S10B:
        pOutGeometry->eFilledDataType = DATA_8BIT_WITH_2BIT;
        pDstPad->nPlane = 2;
        break;
    case VIDEO_COLORFORMAT_NV12_P010:
        pOutGeometry->eFilledDataType = DATA_10BIT;
        pDstPad->nPlane = 1;
        pOutGeometry->nStride = (fmt.stride / 2);  /* MFC D/D returns bytes multiplied bpp instead of length on pixel */
        break;
    case VIDEO_COLORFORMAT_NV12M_P010:
    case VIDEO_COLORFORMAT_NV21M_P010:
        pOutGeometry->eFilledDataType = DATA_10BIT;
        pDstPad->nPlane = 2;
        pOutGeometry->nStride = (fmt.stride / 2);  /* MFC D/D returns bytes multiplied bpp instead of length on pixel */
        break;
    case VIDEO_COLORFORMAT_NV12M_SBWC:
    case VIDEO_COLORFORMAT_NV21M_SBWC:
        pOutGeometry->eFilledDataType = DATA_8BIT_SBWC;
        pOutGeometry->nStride = ALIGN(fmt.width, 16);
        pDstPad->nPlane = 2;
        break;
    case VIDEO_COLORFORMAT_NV12_SBWC:
    case VIDEO_COLORFORMAT_NV12_256_SBWC:
        pOutGeometry->eFilledDataType = DATA_8BIT_SBWC;
        pOutGeometry->nStride = ALIGN(fmt.width, 16);
        pDstPad->nPlane = 1;
        break;
    case VIDEO_COLORFORMAT_NV12M_10B_SBWC:
    case VIDEO_COLORFORMAT_NV21M_10B_SBWC:
        pOutGeometry->eFilledDataType = DATA_10BIT_SBWC;
        pOutGeometry->nStride = ALIGN(fmt.width, 16);
        pDstPad->nPlane = 2;
        break;
    case VIDEO_COLORFORMAT_NV12_10B_SBWC:
    case VIDEO_COLORFORMAT_NV12_10B_256_SBWC:
        pOutGeometry->eFilledDataType = DATA_10BIT_SBWC;
        pOutGeometry->nStride = ALIGN(fmt.width, 16);
        pDstPad->nPlane = 1;
        break;
    default:
        /* for backward compatibility : MFC D/D only supports g_ctrl */
        Codec_OSAL_GetControl(pCtx, CODEC_OSAL_CID_DEC_GET_10BIT_INFO, &value);
        if (value == 1) {
            /* supports only S10B format */
            pOutGeometry->eFilledDataType = DATA_8BIT_WITH_2BIT;
            pOutGeometry->eColorFormat = VIDEO_COLORFORMAT_NV12M_S10B;
            pDstPad->nPlane = 2;

#ifdef USE_SINGLE_PALNE_SUPPORT
            if (pCtx->videoCtx.instInfo.eSecurityType == VIDEO_SECURE) {
                pOutGeometry->eColorFormat = VIDEO_COLORFORMAT_NV12_S10B;
                pDstPad->nPlane = 1;
            }
#endif
        } else {
            pOutGeometry->eFilledDataType = DATA_8BIT;
        }
        break;
    }

    /* Get planes aligned buffer size */
    for (i = 0; i < pDstPad->nPlane; i++)
        pOutGeometry->nAlignPlaneSize[i] = fmt.planeSize[i];

    pOutGeometry->cropRect.nTop       = crop.top;
    pOutGeometry->cropRect.nLeft      = crop.left;
    pOutGeometry->cropRect.nWidth     = crop.width;
    pOutGeometry->cropRect.nHeight    = crop.height;

    ALOGV("%s: %s contents", __FUNCTION__, (pOutGeometry->eFilledDataType & (DATA_10BIT | DATA_10BIT_SBWC)? "10bit":"8bit"));

    memcpy(pBufferConf, pOutGeometry, sizeof(ExynosVideoGeometry));

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Try Geometry (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Try_Geometry_Inbuf(
    void                *pHandle,
    ExynosVideoGeometry *pBufferConf) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;

    CodecOSAL_Format fmt;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (CHECK_POINTER(pBufferConf) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    memset(&fmt, 0, sizeof(fmt));
    fmt.type            = CODEC_OSAL_BUF_TYPE_SRC;
    fmt.format          = Codec_OSAL_CodingTypeToCompressdFormat(pBufferConf->eCompressionFormat);
    fmt.planeSize[0]    = pBufferConf->nSizeImage;
    fmt.nPlane          = pBufferConf->nPlaneCnt;
    fmt.width           = pBufferConf->nWidth;
    fmt.height          = pBufferConf->nHeight;

    if (Codec_OSAL_TryFormat(pCtx, &fmt) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Try Geometry (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Try_Geometry_Outbuf(
    void                *pHandle,
    ExynosVideoGeometry *pBufferConf) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;

    CodecOSAL_Format fmt;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (CHECK_POINTER(pBufferConf) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    memset(&fmt, 0, sizeof(fmt));
    fmt.type    = CODEC_OSAL_BUF_TYPE_DST;
    fmt.format  = Codec_OSAL_ColorFormatToPixelFormat(
                        pBufferConf->eColorFormat, pCtx->videoCtx.instInfo.HwVersion);
    fmt.nPlane  = pBufferConf->nPlaneCnt;
    fmt.width   = pBufferConf->nWidth;
    fmt.height  = pBufferConf->nHeight;

    if (Codec_OSAL_TryFormat(pCtx, &fmt) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Setup (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Setup_Inbuf(
    void         *pHandle,
    unsigned int  nBufferCount) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo    *pSrcPad  = NULL;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;

    CodecOSAL_ReqBuf reqBuf;

    int i;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (nBufferCount == 0) {
        nBufferCount = MAX_INPUTBUFFER_COUNT;
        ALOGV("%s: Change buffer count %d", __FUNCTION__, nBufferCount);
    }

    pSrcPad = &(pCtx->videoCtx.pad[VIDEO_INDEX_SRC_PAD]);

    memset(&reqBuf, 0, sizeof(reqBuf));

    reqBuf.type     = CODEC_OSAL_BUF_TYPE_SRC;
    reqBuf.count    = nBufferCount;
    reqBuf.memory   = Codec_OSAL_VideoMemoryToSystemMemory(pCtx->videoCtx.instInfo.nMemoryType);

    if (Codec_OSAL_RequestBuf(pCtx, &reqBuf) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    if (reqBuf.count != nBufferCount) {
        ALOGE("%s: asked for %d, got %d\n", __FUNCTION__, nBufferCount, reqBuf.count);
        ret = VIDEO_ERROR_NOMEM;
        goto EXIT;
    }

    pSrcPad->nBufNum = reqBuf.count;

    memset(&(pSrcPad->slot), 0, sizeof(pSrcPad->slot));

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Setup (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Setup_Outbuf(
    void         *pHandle,
    unsigned int  nBufferCount) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo    *pDstPad  = NULL;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;

    CodecOSAL_ReqBuf reqBuf;

    int i, j;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (nBufferCount == 0) {
        nBufferCount = MAX_OUTPUTBUFFER_COUNT;
        ALOGV("%s: Change buffer count %d", __FUNCTION__, nBufferCount);
    }

    pDstPad = &(pCtx->videoCtx.pad[VIDEO_INDEX_DST_PAD]);

    memset(&reqBuf, 0, sizeof(reqBuf));

    /* dynamic regist scheme */
    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_DEC_DYNAMIC_DPB_MODE, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    if (pCtx->videoCtx.instInfo.supportInfo.dec.bDrvDPBManageSupport != VIDEO_TRUE) {
        if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_DEC_SET_USER_SHARED_HANDLE,
                                    pCtx->videoCtx.specificInfo.dec.nRefDPBFD) != 0) {
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }
    }

    reqBuf.type     = CODEC_OSAL_BUF_TYPE_DST;
    reqBuf.count    = nBufferCount;
    reqBuf.memory   = Codec_OSAL_VideoMemoryToSystemMemory(pCtx->videoCtx.instInfo.nMemoryType);

    if (Codec_OSAL_RequestBuf(pCtx, &reqBuf) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    if (reqBuf.count != nBufferCount) {
        ALOGE("%s: asked for %d, got %d\n", __FUNCTION__, nBufferCount, reqBuf.count);
        ret = VIDEO_ERROR_NOMEM;
        goto EXIT;
    }

    pDstPad->nBufNum = reqBuf.count;

    memset(&(pDstPad->slot), 0, sizeof(pDstPad->slot));

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Run (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Run_Inbuf(void *pHandle) {
    return MFC_Run_Inbuf(pHandle);
}

/*
 * [Decoder Buffer OPS] Run (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Run_Outbuf(void *pHandle) {
    return MFC_Run_Outbuf(pHandle);
}

/*
 * [Decoder Buffer OPS] Stop (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Stop_Inbuf(void *pHandle) {
    return MFC_Stop_Inbuf(pHandle);
}

/*
 * [Decoder Buffer OPS] Stop (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Stop_Outbuf(void *pHandle) {
    return MFC_Stop_Outbuf(pHandle);
}

/*
 * [Decoder Buffer OPS] Find (Input)
 */
static int MFC_Decoder_Find_Inbuf(
    void *pHandle,
    void *pBuffer) {
    return MFC_Find_Inbuf(pHandle, pBuffer);
}

/*
 * [Decoder Buffer OPS] Find index on non-used slot (Input)
 */
static int MFC_Decoder_FindEmptySlot_Inbuf(void *pHandle) {
    return MFC_FindEmptySlot_Inbuf(pHandle);
}

/*
 * [Decoder Buffer OPS] Find (Outnput)
 */
static int MFC_Decoder_Find_Outbuf(
    void *pHandle,
    void *pBuffer) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo    *pDstPad  = NULL;

    int nIndex = -1, i;

    if (CHECK_POINTER(pCtx) == false) {
        goto EXIT;
    }

    pDstPad = &(pCtx->videoCtx.pad[VIDEO_INDEX_DST_PAD]);

    for (i = 0; i < pDstPad->nBufNum; i++) {
        if (pDstPad->slot[i].bQueued == VIDEO_FALSE) {
            if ((pBuffer == NULL) ||
                (pDstPad->slot[i].buffer.planes[0].addr == pBuffer)) {
                nIndex = i;
                break;
            }
        }
    }

EXIT:
    return nIndex;
}

/*
 * [Decoder Buffer OPS] Find index on non-used slot (Output)
 */
static int MFC_Decoder_FindEmptySlot_Outbuf(void *pHandle) {
    return MFC_FindEmptySlot_Outbuf(pHandle, false);
}

/*
 * [Decoder Buffer OPS] Enqueue (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Enqueue_Inbuf(
    void                *pHandle,
    ExynosVideoBuffer   *pVideoBuffer) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo    *pSrcPad  = NULL;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;
    pthread_mutex_t       *pMutex   = NULL;

    CodecOSAL_Buffer buf;

    if ((CHECK_POINTER(pCtx) == false) ||
        (CHECK_POINTER(pVideoBuffer) == false)) {
        return VIDEO_ERROR_BADPARAM;
    }

    pSrcPad = &(pCtx->videoCtx.pad[VIDEO_INDEX_SRC_PAD]);

    if (pVideoBuffer->nPlaneCnt != pSrcPad->nPlane) {
        ALOGE("%s: geometry(%d) vs nPlaneCnt(%d)", __FUNCTION__,
                                    pSrcPad->nPlane, pVideoBuffer->nPlaneCnt);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    pMutex = (pthread_mutex_t*)pSrcPad->pMutex;
    pthread_mutex_lock(pMutex);

    ret = MFC_setupCodecOSALBuffer(pCtx, pVideoBuffer, pSrcPad, &buf, false);

    pthread_mutex_unlock(pMutex);

    if (ret != VIDEO_ERROR_NONE) {
        goto EXIT;
    }

    if (Codec_OSAL_EnqueueBuf(pCtx, &buf) != 0) {
        ALOGE("%s: Failed to enqueue input buffer", __FUNCTION__);
        pthread_mutex_lock(pMutex);
        memset(&(pSrcPad->slot[buf.index].buffer), 0, sizeof(ExynosVideoBuffer));
        pSrcPad->slot[buf.index].bQueued = VIDEO_FALSE;
        pthread_mutex_unlock(pMutex);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Enqueue (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Enqueue_Outbuf(
    void                *pHandle,
    ExynosVideoBuffer   *pVideoBuffer) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo    *pDstPad  = NULL;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;
    pthread_mutex_t       *pMutex   = NULL;

    CodecOSAL_Buffer buf;

    int i, index;

    if ((CHECK_POINTER(pCtx) == false) ||
        (CHECK_POINTER(pVideoBuffer) == false)) {
        return VIDEO_ERROR_BADPARAM;
    }

    pDstPad = &(pCtx->videoCtx.pad[VIDEO_INDEX_DST_PAD]);

    if (pVideoBuffer->nPlaneCnt != pDstPad->nPlane) {
        ALOGE("%s: geometry(%d) vs nPlaneCnt(%d)", __FUNCTION__,
                                    pDstPad->nPlane, pVideoBuffer->nPlaneCnt);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));
    buf.type    = CODEC_OSAL_BUF_TYPE_DST;
    buf.nPlane  = pVideoBuffer->nPlaneCnt;

    pMutex = (pthread_mutex_t*)pDstPad->pMutex;
    pthread_mutex_lock(pMutex);

    if (pCtx->videoCtx.instInfo.supportInfo.dec.bDrvDPBManageSupport != VIDEO_TRUE) {
        /* index should be same as last used it in case of same fd what is referenced by internally */
        index = MFC_Decoder_Find_Outbuf(pCtx, pVideoBuffer->planes[0].addr);
        if (index == -1) {
            ALOGV("%s: Failed to find index", __FUNCTION__);
            index = MFC_Decoder_FindEmptySlot_Outbuf(pCtx);
            if (index == -1) {
                pthread_mutex_unlock(pMutex);
                ALOGE("%s: Failed to get index", __FUNCTION__);
                ret = VIDEO_ERROR_NOBUFFERS;
                goto EXIT;
            }
        }
    } else {
        index = MFC_Decoder_FindEmptySlot_Outbuf(pCtx);
        if (index == -1) {
            pthread_mutex_unlock(pMutex);
            ALOGE("%s: Failed to get index", __FUNCTION__);
            ret = VIDEO_ERROR_NOBUFFERS;
            goto EXIT;
        }
    }

    buf.index = index;
    ALOGV("%s: pDstPad->slot[%d].bQueued:%d, pFd[0]:%d",
           __FUNCTION__, index, pDstPad->slot[buf.index].bQueued, pVideoBuffer->planes[0].fd);

    buf.memory = Codec_OSAL_VideoMemoryToSystemMemory(pCtx->videoCtx.instInfo.nMemoryType);
    for (i = 0; i < buf.nPlane; i++) {
        if (pCtx->videoCtx.instInfo.nMemoryType == VIDEO_MEMORY_DMABUF)
            buf.planes[i].addr = (void *)(unsigned long)pVideoBuffer->planes[i].fd;
        else
            buf.planes[i].addr = pVideoBuffer->planes[i].addr;

        buf.planes[i].bufferSize    = pVideoBuffer->planes[i].allocSize;
        buf.planes[i].dataLen       = pVideoBuffer->planes[i].dataSize;

        ALOGV("%s: pDstPad->slot[%d].buffer[%d]/ fd=%d, alloc=%d, len=%d", __FUNCTION__, index, i,
                    pVideoBuffer->planes[i].fd, pVideoBuffer->planes[i].allocSize, pVideoBuffer->planes[i].dataSize);
    }

    memcpy(&(pDstPad->slot[buf.index].buffer), pVideoBuffer, sizeof(ExynosVideoBuffer));

    pDstPad->slot[buf.index].bQueued = VIDEO_TRUE;

    if (pCtx->videoCtx.instInfo.supportInfo.dec.bDrvDPBManageSupport != VIDEO_TRUE) {
        pDstPad->slot[buf.index].bSlotUsed = VIDEO_TRUE;
        pDstPad->slot[buf.index].nUsedCnt++;
    }

    pthread_mutex_unlock(pMutex);

    if (Codec_OSAL_EnqueueBuf(pCtx, &buf) != 0) {
        int state = 0;

        pthread_mutex_lock(pMutex);
        memset(&(pDstPad->slot[buf.index].buffer), 0, sizeof(ExynosVideoBuffer));
        pDstPad->slot[buf.index].bQueued = VIDEO_FALSE;

        if (pCtx->videoCtx.instInfo.supportInfo.dec.bDrvDPBManageSupport != VIDEO_TRUE) {
            pDstPad->slot[buf.index].nUsedCnt--;

            if (pDstPad->slot[buf.index].nUsedCnt == 0)
                pDstPad->slot[buf.index].bSlotUsed = VIDEO_FALSE;
        }

        Codec_OSAL_GetControl(pCtx, CODEC_OSAL_CID_DEC_CHECK_STATE, &state);
        if (state == 1) {
            /* The case of Resolution is changed */
            ret = VIDEO_ERROR_WRONGBUFFERSIZE;
        } else {
            ALOGE("%s: Failed to enqueue output buffer", __FUNCTION__);
            ret = VIDEO_ERROR_APIFAIL;
        }

        pthread_mutex_unlock(pMutex);
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Dequeue (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Dequeue_Inbuf(
    void              *pHandle,
    ExynosVideoBuffer *pVideoBuffer) {
    return MFC_Dequeue_Inbuf(pHandle, pVideoBuffer, false);
}

/*
 * [Decoder Buffer OPS] BufferIndexFree (Output)
 */
void MFC_Decoder_BufferIndexFree_Outbuf(
    void   *pHandle,
    int     nIndex) {
    CodecOSALVideoContext   *pCtx       = (CodecOSALVideoContext *)pHandle;
    RefDPBInfo              *pRefDPB    = ((RefDPBInfo *)pCtx->videoCtx.specificInfo.dec.pRefDPBAddr) + nIndex;
    ExynosVideoPadInfo      *pDstPad    = (ExynosVideoPadInfo *)&(pCtx->videoCtx.pad[VIDEO_INDEX_DST_PAD]);
    ExynosVideoSlotInfo     *pDstSlot   = (ExynosVideoSlotInfo *)&(pDstPad->slot[0]);
    int i, j;

    ALOGV("De-queue buf.index:%d, fd:%d", nIndex, pDstSlot[nIndex].buffer.planes[0].fd);

    if (pDstSlot[nIndex].nUsedCnt == 0)
        pDstSlot[nIndex].bSlotUsed = VIDEO_FALSE;

    for (i = 0; i < VIDEO_BUFFER_MAX_NUM; i++) {
        if (pRefDPB->dpbFD[i].fd <= 0)
            break;

        for (j = 0; j < pDstPad->nBufNum; j++) {
            if (pRefDPB->dpbFD[i].fd == pDstSlot[j].buffer.planes[0].fd) {
                if (pDstSlot[j].bQueued == VIDEO_FALSE) {
                    /* it was already returned for display */
                    if (pDstSlot[j].nUsedCnt > 0)
                        pDstSlot[j].nUsedCnt--;
                } else {
                    if (pDstSlot[j].nUsedCnt > 1) {
                        /* The buffer being used as the reference buffer came again. */
                        pDstSlot[j].nUsedCnt--;
                    } else {
                        /* Reference DPB buffer is internally reused. */
                    }
                }

                ALOGV("[%s]dec Cnt : FD:%d, pCtx->videoCtx.pOutbuf[%d].nUsedCnt:%d", __FUNCTION__,
                        pRefDPB->dpbFD[i].fd, j, pDstSlot[j].nUsedCnt);

                if ((pDstSlot[j].nUsedCnt == 0) &&
                    (pDstSlot[j].bQueued == VIDEO_FALSE)) {
                    pDstSlot[j].bSlotUsed = VIDEO_FALSE;
                    memset(&(pDstSlot[j].buffer), 0, sizeof(ExynosVideoBuffer));
                }
            }
        }
    }

    memset((char *)pRefDPB, 0, sizeof(RefDPBInfo));

    return;
}

static ExynosVideoErrorType MFC_Decoder_StateProcessing(
    void               *pHandle,
    ExynosVideoBuffer  *pVideoBuffer,
    ExynosVideoPadInfo *pDstPad,
    ExynosVideoBuffer  *pOutbuf,
    CodecOSAL_Buffer   *buf) {
    CodecOSALVideoContext  *pCtx    = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType    ret     = VIDEO_ERROR_NONE;

    int value = 0, state = 0;
    int i;

    if ((CHECK_POINTER(pCtx) == false) ||
        (CHECK_POINTER(pVideoBuffer) == false)) {
        return VIDEO_ERROR_BADPARAM;
    }

    Codec_OSAL_GetControl(pCtx, CODEC_OSAL_CID_DEC_DISPLAY_STATUS, &value);

    switch (value) {
    case 0:
        pOutbuf->extraInfo.specific.dec.displayStatus = VIDEO_FRAME_STATUS_DECODING_ONLY;
#ifdef USE_HEVC_HWIP
        if ((pCtx->videoCtx.instInfo.eCodecType == VIDEO_CODING_HEVC) ||
            (pCtx->videoCtx.instInfo.HwVersion != (int)MFC_51)) {
#else
        if (pCtx->videoCtx.instInfo.HwVersion != (int)MFC_51) {
#endif
            Codec_OSAL_GetControl(pCtx, CODEC_OSAL_CID_DEC_CHECK_STATE, &state);
            if (state == 4) /* DPB realloc for S3D SEI */
                pOutbuf->extraInfo.specific.dec.displayStatus = VIDEO_FRAME_STATUS_ENABLED_S3D;
        }
        break;
    case 1:
        pOutbuf->extraInfo.specific.dec.displayStatus = VIDEO_FRAME_STATUS_DISPLAY_DECODING;
        if (buf->flags & CODEC_OSAL_FLAG_INTER_RESOLUTION_CHANGE)
            pOutbuf->extraInfo.specific.dec.displayStatus = VIDEO_FRAME_STATUS_DISPLAY_INTER_RESOL_CHANGE;
        break;
    case 2:
        pOutbuf->extraInfo.specific.dec.displayStatus = VIDEO_FRAME_STATUS_DISPLAY_ONLY;
        if (buf->flags & CODEC_OSAL_FLAG_INTER_RESOLUTION_CHANGE)
            pOutbuf->extraInfo.specific.dec.displayStatus = VIDEO_FRAME_STATUS_DISPLAY_INTER_RESOL_CHANGE;
        break;
    case 3:
        Codec_OSAL_GetControl(pCtx, CODEC_OSAL_CID_DEC_CHECK_STATE, &state);
        if (state == 1) /* Resolution is changed */
            pOutbuf->extraInfo.specific.dec.displayStatus = VIDEO_FRAME_STATUS_CHANGE_RESOL;
        else            /* Decoding is finished */
            pOutbuf->extraInfo.specific.dec.displayStatus = VIDEO_FRAME_STATUS_DECODING_FINISHED;
        break;
    case 4:
        pOutbuf->extraInfo.specific.dec.displayStatus = VIDEO_FRAME_STATUS_LAST_FRAME;
        break;
    default:
        pOutbuf->extraInfo.specific.dec.displayStatus = VIDEO_FRAME_STATUS_UNKNOWN;
        break;
    }

    if ((buf->frameType & VIDEO_FRAME_CORRUPT) &&
        (pCtx->videoCtx.instInfo.supportInfo.dec.bFrameErrorTypeSupport == VIDEO_TRUE)) {
        int corruptType = 0;

        Codec_OSAL_GetControl(pCtx, CODEC_OSAL_CID_VIDEO_FRAME_ERROR_TYPE, &corruptType);
        ALOGD("[%s] error type : %d", __FUNCTION__, corruptType);
        switch (corruptType) {
        case 3:  /* BROKEN */
            /* nothing to do */
            break;
        case 2:  /* SYNC POINT */
        case 1:  /* CONCEALMENT */
            /* replace to concealment */
            buf->frameType = (buf->frameType & ~(VIDEO_FRAME_CORRUPT)) | VIDEO_FRAME_CONCEALMENT;
            break;
        default:
            /* replace to concealment */
            buf->frameType = (buf->frameType & ~(VIDEO_FRAME_CORRUPT));
            break;
        }
    }

    pOutbuf->extraInfo.specific.dec.frameType = buf->frameType;

    if (pDstPad->geometry.bInterlaced == VIDEO_TRUE) {
        if ((buf->field == CODEC_OSAL_INTER_TYPE_TB) ||
            (buf->field == CODEC_OSAL_INTER_TYPE_BT)) {
            pOutbuf->extraInfo.specific.dec.interlacedType = buf->field;
        } else {
            ALOGV("%s: buf.field's value is invald(%d)", __FUNCTION__, buf->field);
            pOutbuf->extraInfo.specific.dec.interlacedType = CODEC_OSAL_INTER_TYPE_NONE;
        }
    }

    if (pCtx->videoCtx.instInfo.supportInfo.dec.bPocSupport == VIDEO_TRUE) {
        int poc = -1;

        Codec_OSAL_GetControl(pCtx, CODEC_OSAL_CID_DEC_FRAME_POC, &poc);

        pOutbuf->extraInfo.specific.dec.nPoc = poc;
    }

    if (buf->flags & CODEC_OSAL_FLAG_HDR_INFO) {
        ExynosVideoHdrInfo  *pHdrInfo = &(pDstPad->geometry.HdrInfo);

        if (!(buf->frameType & VIDEO_FRAME_CORRUPT)) {
            pHdrInfo->eValidType   = (ExynosHdrInfoType)(buf->flags & CODEC_OSAL_FLAG_HDR_INFO);
            pHdrInfo->eChangedType = HDR_INFO_NO_CHANGES;

            /* HDR Dynamic Info is obtained from shared buffer instead of ioctl call */
            if ((pCtx->videoCtx.instInfo.supportInfo.dec.bHDRDynamicInfoSupport == VIDEO_TRUE) &&
                (buf->flags & CODEC_OSAL_FLAG_HDR_DYNAMIC_INFO)) {
                /* Shared buffer related with HDR dynamic info can contain 2 types of data.
                 *  1. ExynosVideoHdrDynamicBlob : blob data type
                 *  2. ExynosVideoHdrDynamic struct type
                 */
                if (pCtx->videoCtx.instInfo.supportInfo.dec.bHDRDynamicBlobSupport == VIDEO_TRUE) {
                    ExynosVideoHdrDynamicBlob *pHdrDynamicBlob =
                        ((ExynosVideoHdrDynamicBlob *)pCtx->videoCtx.specificInfo.dec.pHDRDynamicInfoAddr) + buf->index;

                    if ((pHdrDynamicBlob->nSize > 0) &&
                        (pHdrDynamicBlob->nSize <= sizeof(pHdrDynamicBlob->pData))) {
                        if (memcmp((char *)pHdrDynamicBlob, (char *)(&(pDstPad->geometry.HdrInfo.sHdrDynamicBlob)), sizeof(ExynosVideoHdrDynamicBlob))) {
                            pHdrInfo->eChangedType |= HDR_INFO_DYNAMIC_META;

                            memcpy((char *)(&(pDstPad->geometry.HdrInfo.sHdrDynamicBlob)),
                                    (char *)pHdrDynamicBlob,
                                    sizeof(ExynosVideoHdrDynamicBlob));
                        }
                    }
                } else {
                    ExynosVideoHdrDynamic *pHdrDynamic =
                        ((ExynosVideoHdrDynamic *)pCtx->videoCtx.specificInfo.dec.pHDRDynamicInfoAddr) + buf->index;
                    if (pHdrDynamic->valid != 0) {
                        if (memcmp((char *)pHdrDynamic, (char *)(&(pDstPad->geometry.HdrInfo.sHdrDynamic)), sizeof(ExynosVideoHdrDynamic))) {
                            pHdrInfo->eChangedType |= HDR_INFO_DYNAMIC_META;
                        }

                        memcpy((char *)(&(pDstPad->geometry.HdrInfo.sHdrDynamic)),
                                (char *)pHdrDynamic,
                                sizeof(ExynosVideoHdrDynamic));
                    }
                }
            }

            pOutbuf->extraInfo.specific.dec.frameType |= VIDEO_FRAME_WITH_HDR_INFO;
        }
    }

    /* Film Grain Info is obtained from shared buffer instead of ioctl call */
    if ((pCtx->videoCtx.instInfo.eCodecType == VIDEO_CODING_AV1) &&
        (buf->flags & CODEC_OSAL_FLAG_FILM_GRAIN)) {
        ExynosVideoFilmGrain *pFilmGrain = ((ExynosVideoFilmGrain *)pCtx->videoCtx.specificInfo.dec.pFilmGrainAddr)
                                                + buf->index;

        memcpy((char *)(&(pDstPad->geometry.FilmGrainInfo)),
               (char *)pFilmGrain,
               sizeof(ExynosVideoFilmGrain));

        pOutbuf->extraInfo.specific.dec.frameType |= VIDEO_FRAME_WITH_FILM_GRAIN;
    }

    if (buf->flags & CODEC_OSAL_FLAG_BLACK_BAR_CROP_INFO)
        pOutbuf->extraInfo.specific.dec.frameType |= VIDEO_FRAME_WITH_BLACK_BAR;

    if (pDstPad->geometry.eFilledDataType & (DATA_8BIT_SBWC | DATA_10BIT_SBWC)) {
        if (buf->flags & CODEC_OSAL_FLAG_UNCOMP_DATA) {
            /* SBWC scenario, but data is not compressed by black bar */
            pOutbuf->extraInfo.specific.dec.frameType |= VIDEO_FRAME_NEED_ACTUAL_FORMAT;
        }
    }

    if (buf->flags & CODEC_OSAL_FLAG_ACTUAL_FRAMERATE) {
        /* for PowerHint: framerate from app can be invalid */
        pOutbuf->extraInfo.specific.dec.frameType |= VIDEO_FRAME_NEED_ACTUAL_FRAMERATE;
    }

    if (buf->flags & CODEC_OSAL_FLAG_MULTIFRAME) {
        pOutbuf->extraInfo.specific.dec.frameType |= VIDEO_FRAME_MULTI;
    }

    memcpy(pVideoBuffer, pOutbuf, sizeof(ExynosVideoBuffer));
    pDstPad->slot[buf->index].bQueued = VIDEO_FALSE;

    if (pCtx->videoCtx.instInfo.supportInfo.dec.bDrvDPBManageSupport != VIDEO_TRUE) {
        memcpy((char *)(&(pVideoBuffer->extraInfo.specific.dec.refDPB)),
               (char *)(((RefDPBInfo *)pCtx->videoCtx.specificInfo.dec.pRefDPBAddr) + buf->index),
               sizeof(RefDPBInfo));

        MFC_Decoder_BufferIndexFree_Outbuf(pHandle, buf->index);
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Dequeue (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Dequeue_Outbuf(
    void              *pHandle,
    ExynosVideoBuffer *pVideoBuffer) {
    return MFC_Common_Dequeue_Outbuf(pHandle, pVideoBuffer, &MFC_Decoder_StateProcessing);
}

/*
 * [Decoder Buffer OPS] Cleanup Buffer (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Cleanup_Buffer_Inbuf(void *pHandle) {
    return MFC_Cleanup_Buffer_Inbuf(pHandle);
}

/*
 * [Decoder Buffer OPS] Cleanup Buffer (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Cleanup_Buffer_Outbuf(void *pHandle) {
    return MFC_Cleanup_Buffer_Outbuf(pHandle);
}

#ifndef USE_EPOLL
/*
 * [Decoder OPS] Wait Buffer (Input)
 */
ExynosVideoErrorType MFC_Decoder_Wait_Buffer_Inbuf(
    void                *pHandle,
    ExynosVideoPollType *avail) {
    return MFC_Wait_Buffer_Common(pHandle, avail, false, InputPort);
}

/*
 * [Decoder OPS] Wait Buffer (Output)
 */
ExynosVideoErrorType MFC_Decoder_Wait_Buffer_Outbuf(
    void                *pHandle,
    ExynosVideoPollType *avail) {
    return MFC_Wait_Buffer_Common(pHandle, avail, false, OutputPort);
}

#else
/*
 * [Decoder OPS] Wait Buffer use epoll (Input)
 */
ExynosVideoErrorType MFC_Decoder_Wait_Buffer_Inbuf_Epoll(
    void                *pHandle,
    ExynosVideoPollType *avail) {
    return MFC_Wait_Buffer_Common_Epoll(pHandle, avail, InputPort);
}

/*
 * [Decoder OPS] Wait Buffer use epoll (Output)
 */
ExynosVideoErrorType MFC_Decoder_Wait_Buffer_Outbuf_Epoll(
    void                *pHandle,
    ExynosVideoPollType *avail) {
    return MFC_Wait_Buffer_Common_Epoll(pHandle, avail, OutputPort);
}
#endif

/*
 * [Decoder OPS] Common
 */
static ExynosVideoDecOps defDecOps = {
    .Init                       = MFC_Decoder_Init,
    .Finalize                   = MFC_Decoder_Finalize,
    .Set_FrameTag               = MFC_Decoder_Set_FrameTag,
    .Get_FrameTag               = MFC_Decoder_Get_FrameTag,
    .Get_ActualBufferCount      = MFC_Decoder_Get_ActualBufferCount,
    .Get_DisplayDelay           = MFC_Decoder_Get_DisplayDelay,
    .Set_DisplayDelay           = MFC_Decoder_Set_DisplayDelay,
    .Set_IFrameDecoding         = MFC_Decoder_Set_IFrameDecoding,

    .Enable_PackedPB            = MFC_Decoder_Enable_PackedPB,
    .Enable_LoopFilter          = MFC_Decoder_Enable_LoopFilter,
    .Enable_SliceMode           = MFC_Decoder_Enable_SliceMode,
    .Enable_DiscardRcvHeader    = MFC_Decoder_Enable_DiscardRcvHeader,
    .Enable_DTSMode             = MFC_Decoder_Enable_DTSMode,

    .Set_QosRatio               = MFC_Decoder_Set_QosRatio,

    .Enable_SEIParsing          = MFC_Decoder_Enable_SEIParsing,
    .Get_FramePackingInfo       = MFC_Decoder_Get_FramePackingInfo,

    .Get_HDRInfo                = MFC_Decoder_Get_HDRInfo,

    .Get_FilmGrainInfo          = MFC_Decoder_Get_FilmGrainInfo,

    .Set_SearchBlackBar         = MFC_Decoder_Set_SearchBlackBar,
    .Get_BlackBarCrop           = MFC_Decoder_Get_BlackBarCrop,

    .Get_ActualFormat           = MFC_Decoder_Get_ActualFormat,

    .Get_ActualFramerate        = MFC_Decoder_Get_ActualFramerate,

    .Enable_DecodeOrder         = MFC_Decoder_Enable_DecodeOrder,

    .Set_OperatingRate          = MFC_Decoder_Set_OperatingRate,

    .Set_RealTimePriority       = MFC_Decoder_Set_RealTimePriority,
    .Get_RealTimePriority       = MFC_Decoder_Get_RealTimePriority,

    .Disable_LazyUnmap          = MFC_Decoder_Disable_LazyUnmap,

    .Stop_Wait_Buffer           = MFC_Decoder_Stop_Wait_Buffer,
    .Reset_Wait_Buffer          = MFC_Decoder_Reset_Wait_Buffer,
};

/*
 * [Decoder Buffer OPS] Input
 */
static ExynosVideoBufferOps defInbufOps = {
    .Enable_Cacheable       = MFC_Decoder_Enable_Cacheable_Inbuf,
    .Set_Geometry           = MFC_Decoder_Set_Geometry_Inbuf,
    .Get_Geometry           = NULL,
    .Try_Geometry           = MFC_Decoder_Try_Geometry_Inbuf,
    .Setup                  = MFC_Decoder_Setup_Inbuf,
    .Run                    = MFC_Decoder_Run_Inbuf,
    .Stop                   = MFC_Decoder_Stop_Inbuf,
    .Enqueue                = MFC_Decoder_Enqueue_Inbuf,
    .Dequeue                = MFC_Decoder_Dequeue_Inbuf,
    .Cleanup_Buffer         = MFC_Decoder_Cleanup_Buffer_Inbuf,
#ifdef USE_EPOLL
    .Wait_Buffer            = MFC_Decoder_Wait_Buffer_Inbuf_Epoll,
#else
    .Wait_Buffer            = MFC_Decoder_Wait_Buffer_Inbuf,
#endif
};

/*
 * [Decoder Buffer OPS] Output
 */
static ExynosVideoBufferOps defOutbufOps = {
    .Enable_Cacheable       = MFC_Decoder_Enable_Cacheable_Outbuf,
    .Set_Geometry           = MFC_Decoder_Set_Geometry_Outbuf,
    .Get_Geometry           = MFC_Decoder_Get_Geometry_Outbuf,
    .Try_Geometry           = MFC_Decoder_Try_Geometry_Outbuf,
    .Setup                  = MFC_Decoder_Setup_Outbuf,
    .Run                    = MFC_Decoder_Run_Outbuf,
    .Stop                   = MFC_Decoder_Stop_Outbuf,
    .Enqueue                = MFC_Decoder_Enqueue_Outbuf,
    .Dequeue                = MFC_Decoder_Dequeue_Outbuf,
    .Cleanup_Buffer         = MFC_Decoder_Cleanup_Buffer_Outbuf,
#ifdef USE_EPOLL
    .Wait_Buffer            = MFC_Decoder_Wait_Buffer_Outbuf_Epoll,
#else
    .Wait_Buffer            = MFC_Decoder_Wait_Buffer_Outbuf,
#endif
};

ExynosVideoErrorType MFC_Exynos_Video_GetInstInfo_Decoder(
    ExynosVideoInstInfo *pVideoInstInfo) {
    CodecOSALVideoContext   videoCtx;
    ExynosVideoErrorType    ret = VIDEO_ERROR_NONE;

    int codecRet = 0;

    memset(&videoCtx, 0, sizeof(videoCtx));

    if (pVideoInstInfo == NULL) {
        return VIDEO_ERROR_BADPARAM;
    }

#ifdef USE_HEVC_HWIP
    if (pVideoInstInfo->eCodecType == VIDEO_CODING_HEVC)
        codecRet = Codec_OSAL_DevOpen(VIDEO_HEVC_DECODER_NAME, O_RDWR, &videoCtx);
    else
#endif
        codecRet = Codec_OSAL_DevOpen(VIDEO_MFC_DECODER_NAME, O_RDWR, &videoCtx);

    if (codecRet < 0) {
        ALOGE("%s: Failed to open decoder device", __FUNCTION__);
        ret = VIDEO_ERROR_OPENFAIL;
        goto EXIT;
    }

    ret = __GetInstInfo(&videoCtx, pVideoInstInfo);

EXIT:
    Codec_OSAL_DevClose(&videoCtx);

    return ret;
}

ExynosVideoErrorType MFC_Exynos_Video_Register_Decoder(
    ExynosVideoDecOps    *pDecOps,
    ExynosVideoBufferOps *pInbufOps,
    ExynosVideoBufferOps *pOutbufOps) {
    ExynosVideoErrorType ret = VIDEO_ERROR_NONE;

    if ((pDecOps == NULL) || (pInbufOps == NULL) || (pOutbufOps == NULL)) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memcpy((char *)pDecOps, (char *)&defDecOps, sizeof(defDecOps));
    memcpy((char *)pInbufOps, (char *)&defInbufOps, sizeof(defInbufOps));
    memcpy((char *)pOutbufOps, (char *)&defOutbufOps, sizeof(defOutbufOps));

EXIT:
    return ret;
}
