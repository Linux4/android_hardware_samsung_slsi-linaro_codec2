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
 * @file        ExynosVideoEncoder.c
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
#include "ExynosVideoEnc.h"
#include "ExynosVideo_OSAL_Enc.h"

/* #define LOG_NDEBUG 0 */
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "ExynosVideoEncoder"

#include "ExynosVideo_OSAL_Log.h"

#define MAX_INPUTBUFFER_COUNT 32
#define MAX_OUTPUTBUFFER_COUNT 32

static struct {
    int eColorFormat;
    int nFormatFlag;
} SBWC_LOSSY_TABLE[] = {
    /* 8Bit multi plane format */
    {VIDEO_COLORFORMAT_NV12M_SBWC_L50    , 0},
    {VIDEO_COLORFORMAT_NV12M_SBWC_L75    , 2},
    /* 10Bit multi plane format */
    {VIDEO_COLORFORMAT_NV12M_10B_SBWC_L40, 4},
    {VIDEO_COLORFORMAT_NV12M_10B_SBWC_L60, 1},
    {VIDEO_COLORFORMAT_NV12M_10B_SBWC_L80, 3},
    /* 8Bit single plane format */
    {VIDEO_COLORFORMAT_NV12_SBWC_L50    , 0},
    {VIDEO_COLORFORMAT_NV12_SBWC_L75    , 2},
    /* 10Bit single plane format */
    {VIDEO_COLORFORMAT_NV12_10B_SBWC_L40, 4},
    {VIDEO_COLORFORMAT_NV12_10B_SBWC_L60, 1},
    {VIDEO_COLORFORMAT_NV12_10B_SBWC_L80, 3},
};

static struct {
    int eSBWCFormat;
    int eNormalFormat;
} SBWC_NONCOMP_TABLE[] = {
    {VIDEO_COLORFORMAT_NV12_SBWC,         VIDEO_COLORFORMAT_NV12},
    {VIDEO_COLORFORMAT_NV12_SBWC,         VIDEO_COLORFORMAT_NV12_DECOMP},
    {VIDEO_COLORFORMAT_NV12M_SBWC,        VIDEO_COLORFORMAT_NV12M},
    {VIDEO_COLORFORMAT_NV12_10B_SBWC,     VIDEO_COLORFORMAT_NV12_S10B},
    {VIDEO_COLORFORMAT_NV12_10B_SBWC,     VIDEO_COLORFORMAT_NV12_P010_DECOMP},
    {VIDEO_COLORFORMAT_NV12M_10B_SBWC,    VIDEO_COLORFORMAT_NV12M_P010},
    {VIDEO_COLORFORMAT_NV21M_SBWC,        VIDEO_COLORFORMAT_NV21M},
    {VIDEO_COLORFORMAT_NV21M_10B_SBWC,    VIDEO_COLORFORMAT_NV21M_P010},
    {VIDEO_COLORFORMAT_NV12_256_SBWC,     VIDEO_COLORFORMAT_NV12_DECOMP},
    {VIDEO_COLORFORMAT_NV12_10B_256_SBWC, VIDEO_COLORFORMAT_NV12_P010_DECOMP},
};

static void __Set_InstInfo(ExynosVideoInstInfo *pVideoInstInfo, int mode) {
    pVideoInstInfo->supportInfo.enc.bMaxIFrameSizeSupport        = (mode & (0x1 << 25))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->supportInfo.enc.bRealTimePrioritySupport     = (mode & (0x1 << 23))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->supportInfo.enc.bHDRDynamicStatInfoSupport   = (mode & (0x1 << 22))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->supportInfo.enc.bGopModeSupport              = (mode & (0x1 << 21))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->supportInfo.enc.bPMVSupport                  = (mode & (0x1 << 20))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->supportInfo.enc.bAverageQpSupport            = (mode & (0x1 << 19))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->supportInfo.enc.bOperatingRateSupport        = (mode & (0x1 << 18))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->supportInfo.enc.bGDCvOTFSupport              = (mode & (0x1 << 17))? VIDEO_TRUE:VIDEO_FALSE;

    pVideoInstInfo->bVideoBufFlagCtrl = (mode & (0x1 << 16))? VIDEO_TRUE:VIDEO_FALSE;

    pVideoInstInfo->supportInfo.enc.bChromaQpSupport             = (mode & (0x1 << 15))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->supportInfo.enc.bDropControlSupport          = (mode & (0x1 << 14))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->supportInfo.enc.bHDRDynamicInfoSupport       = (mode & (0x1 << 12))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->supportInfo.enc.bHDRStaticInfoSupport        = (mode & (0x1 << 11))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->supportInfo.enc.bAdaptiveLayerBitrateSupport = (mode & (0x1 << 10))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->supportInfo.enc.bColorAspectsSupport         = (mode & (0x1 << 9))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->supportInfo.enc.bIFrameRatioSupport          = (mode & (0x1 << 8))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->supportInfo.enc.bPVCSupport                  = (mode & (0x1 << 7))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->supportInfo.enc.bFixedSliceSupport           = (mode & (0x1 << 6))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->supportInfo.enc.bQpRangePBSupport            = (mode & (0x1 << 5))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->supportInfo.enc.bRoiInfoSupport              = (mode & (0x1 << 4))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->supportInfo.enc.bSkypeSupport                = (mode & (0x1 << 3))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->supportInfo.enc.bTemporalSvcSupport          = (mode & (0x1 << 2))? VIDEO_TRUE:VIDEO_FALSE;
}

/*
 * [Common] __Set_SupportFormat
 */
static void __Set_SupportFormat(ExynosVideoInstInfo *pVideoInstInfo) {
    int nLastIndex = 0;

    if (CHECK_POINTER(pVideoInstInfo) == false) {
        goto EXIT;
    }

    memset(pVideoInstInfo->supportFormat, (int)VIDEO_COLORFORMAT_UNKNOWN, sizeof(pVideoInstInfo->supportFormat));

    pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12;
    pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M;
    pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M;

    switch ((int)pVideoInstInfo->HwVersion) {
    case MFC_170:
        /* NV12, NV21, I420, YV12, BGRA, RGBA, ARGB, SBWC(NV12, NV21, NV12_10B, NV21_10B), SBWC Lossy(v2.7) */
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_YV12M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_BGRA8888;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_RGBA8888;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_ARGB8888;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_SBWC;
#ifdef USE_SUPPORT_GPU_SBWC
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_256_SBWC;
#endif
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_SBWC;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_SBWC;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_32_SBWC_L;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_64_SBWC_L;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_32_SBWC_L;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_64_SBWC_L;

        if ((pVideoInstInfo->eCodecType == VIDEO_CODING_HEVC) ||
                (pVideoInstInfo->eCodecType == VIDEO_CODING_VP9)) {
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_P010;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_P010;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_P010;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_10B_SBWC;
#ifdef USE_SUPPORT_GPU_SBWC
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_10B_256_SBWC;
#endif
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_10B_SBWC;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_10B_SBWC;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_10B_32_SBWC_L;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_10B_64_SBWC_L;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_10B_32_SBWC_L;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_10B_64_SBWC_L;
        }
        break;
    case MFC_1601:
        /* NV12, NV21, I420, YV12, BGRA, RGBA, ARGB, SBWC(NV12, NV21, NV12_10B, NV21_10B), SBWC Lossy */
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_YV12M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_BGRA8888;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_RGBA8888;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_ARGB8888;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_SBWC;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_SBWC;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_SBWC;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_SBWC_L50;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_SBWC_L75;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_SBWC_L50;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_SBWC_L75;

        if ((pVideoInstInfo->eCodecType == VIDEO_CODING_HEVC) ||
                (pVideoInstInfo->eCodecType == VIDEO_CODING_VP9)) {
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_P010;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_P010;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_P010;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_10B_SBWC;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_10B_SBWC;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_10B_SBWC;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_10B_SBWC_L40;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_10B_SBWC_L60;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_10B_SBWC_L80;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_10B_SBWC_L40;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_10B_SBWC_L60;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_10B_SBWC_L80;
        }
        break;
    case MFC_160:
    case MFC_1506:
    case MFC_1501:
    case MFC_150:
        /* NV12, NV21, I420, YV12, BGRA, RGBA, ARGB, SBWC(NV12, NV21, NV12_10B, NV21_10B), SBWC Lossy */
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_YV12M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_BGRA8888;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_RGBA8888;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_ARGB8888;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_SBWC;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_SBWC;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_SBWC;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_SBWC_L50;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_SBWC_L75;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_SBWC_L50;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_SBWC_L75;

        if ((pVideoInstInfo->eCodecType == VIDEO_CODING_HEVC) ||
                (pVideoInstInfo->eCodecType == VIDEO_CODING_VP9)) {
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_P010;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_P010;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_P010;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_10B_SBWC;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_10B_SBWC;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_10B_SBWC;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_10B_SBWC_L60;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_10B_SBWC_L80;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_10B_SBWC_L60;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_10B_SBWC_L80;
        }
        break;
    case MFC_1410:
        /* NV12, NV21, I420, YV12, BGRA, RGBA, ARGB, NV12_S10B, NV21_S10B, SBWC(NV12, NV21, NV12_10B, NV21_10B), SBWC Lossy */
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_YV12M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_BGRA8888;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_RGBA8888;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_ARGB8888;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_SBWC;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_SBWC;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M_SBWC;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_SBWC_L50;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_SBWC_L75;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_SBWC_L50;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_SBWC_L75;

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
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_10B_SBWC_L60;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_10B_SBWC_L80;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_10B_SBWC_L60;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_10B_SBWC_L80;
        }
        break;
    case MFC_1421:
    case MFC_142:
    case MFC_140:
    case MFC_1400:
         /* NV12, NV21, I420, YV12, BGRA, RGBA, ARGB, NV12_S10B, NV21_S10B, SBWC(NV12, NV21, NV12_10B, NV21_10B) */
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_YV12M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_BGRA8888;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_RGBA8888;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_ARGB8888;
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
    case MFC_130: /* NV12, NV21, I420, YV12, NV12_S10B, NV21_S10B */
    case MFC_120:
    case MFC_1220:
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
    case MFC_1020:
    case MFC_1021:
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_YV12M;
        break;
    case MFC_90:  /* NV12, NV21, BGRA, RGBA, I420, YV12, ARGB */
    case MFC_80:
#ifdef USE_HEVC_HWIP
    case HEVC_10:
#endif
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_BGRA8888;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_RGBA8888;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_YV12M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_ARGB8888;
        break;
    case MFC_723:  /* NV12, NV21, BGRA, RGBA, I420, YV12, ARGB, NV12T */
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_BGRA8888;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_RGBA8888;
    case MFC_72:  /* NV12, NV21, I420, YV12, ARGB, NV12T */
    case MFC_77:
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_YV12M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_ARGB8888;
    case MFC_78:  /* NV12, NV21, NV12T */
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
        ALOGE("%s: invalid parameter", __FUNCTION__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (Codec_OSAL_GetControl(pCtx, CODEC_OSAL_CID_VIDEO_GET_VERSION_INFO, &version) != 0) {
        ALOGW("%s: MFC version information is not available", __FUNCTION__);
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
        memset(&(pVideoInstInfo->supportInfo.enc), 0, sizeof(pVideoInstInfo->supportInfo.enc));
        goto EXIT;
    }

    __Set_InstInfo(pVideoInstInfo, mode);

    if (mode & (0x1 << 1)) {
        if (Codec_OSAL_GetControl(pCtx, CODEC_OSAL_CID_ENC_EXT_BUFFER_SIZE, &(pVideoInstInfo->supportInfo.enc.nSpareSize)) != 0) {
            ALOGE("%s: Failed to GetControl(CODEC_OSAL_CID_ENC_EXT_BUFFER_SIZE)", __FUNCTION__);
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }
    }

    pVideoInstInfo->SwVersion = 0;
    if (pVideoInstInfo->supportInfo.enc.bSkypeSupport == VIDEO_TRUE) {
        int swVersion = 0;

        if (Codec_OSAL_GetControl(pCtx, CODEC_OSAL_CID_VIDEO_GET_DRIVER_VERSION, &swVersion) != 0) {
            ALOGE("%s: Failed to GetControl(CODEC_OSAL_CID_VIDEO_GET_DRIVER_VERSION)", __FUNCTION__);
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
                          "GOP(%d), "
                          "PMV(%d), "
                          "Average QP(%d), "
                          "GDC vOTF(%d), "
                          "Drop Ctrl(%d), "
                          "HDR dynamic(%d), "
                          "HDR static(%d), "
                          "Adaptive Layer(%d), "
                          "Color Aspects(%d), "
                          "ROI(%d), "
                          "HDR statistics(%d), ",
        pVideoInstInfo->bVideoBufFlagCtrl,
        pVideoInstInfo->supportInfo.enc.bRealTimePrioritySupport,
        pVideoInstInfo->supportInfo.enc.bOperatingRateSupport,
        pVideoInstInfo->supportInfo.enc.bGopModeSupport,
        pVideoInstInfo->supportInfo.enc.bPMVSupport,
        pVideoInstInfo->supportInfo.enc.bAverageQpSupport,
        pVideoInstInfo->supportInfo.enc.bGDCvOTFSupport,
        pVideoInstInfo->supportInfo.enc.bDropControlSupport,
        pVideoInstInfo->supportInfo.enc.bHDRDynamicInfoSupport,
        pVideoInstInfo->supportInfo.enc.bHDRStaticInfoSupport,
        pVideoInstInfo->supportInfo.enc.bAdaptiveLayerBitrateSupport,
        pVideoInstInfo->supportInfo.enc.bColorAspectsSupport,
        pVideoInstInfo->supportInfo.enc.bRoiInfoSupport,
        pVideoInstInfo->supportInfo.enc.bHDRDynamicStatInfoSupport);

    for (i = 0; i < VIDEO_COLORFORMAT_MAX; i++) {
        if (pVideoInstInfo->supportFormat[i] == VIDEO_COLORFORMAT_UNKNOWN) {
            break;
        }

        ALOGV("[VideoInstInfo] support format[%d] : 0x%x", i, pVideoInstInfo->supportFormat[i]);
    }

    return;
}

void EncSpecificClear(CodecOSALVideoContext *pCtx) {
    if (pCtx->videoCtx.instInfo.supportInfo.enc.bTemporalSvcSupport == VIDEO_TRUE) {
        if (pCtx->videoCtx.specificInfo.enc.pTemporalLayerAddr != NULL) {
            Codec_OSAL_MemoryUnmap(pCtx->videoCtx.specificInfo.enc.pTemporalLayerAddr, sizeof(TemporalLayerBuffer));
            pCtx->videoCtx.specificInfo.enc.pTemporalLayerAddr = NULL;
        }

        /* free a ion_buffer */
        if (pCtx->videoCtx.specificInfo.enc.nTemporalLayerFD >= 0) {
            close(pCtx->videoCtx.specificInfo.enc.nTemporalLayerFD);
            pCtx->videoCtx.specificInfo.enc.nTemporalLayerFD = -1;
        }
    }

    if (pCtx->videoCtx.instInfo.supportInfo.enc.bRoiInfoSupport == VIDEO_TRUE) {
        if (pCtx->videoCtx.specificInfo.enc.pROIAddr != NULL) {
            Codec_OSAL_MemoryUnmap(pCtx->videoCtx.specificInfo.enc.pROIAddr, sizeof(RoiInfoBuffer));
            pCtx->videoCtx.specificInfo.enc.pROIAddr = NULL;
        }

        /* free a ion_buffer */
        if (pCtx->videoCtx.specificInfo.enc.nROIFD >= 0) {
            close(pCtx->videoCtx.specificInfo.enc.nROIFD);
            pCtx->videoCtx.specificInfo.enc.nROIFD = -1;
        }
    }

    if (pCtx->videoCtx.instInfo.supportInfo.enc.bHDRDynamicInfoSupport == VIDEO_TRUE) {
        if (pCtx->videoCtx.specificInfo.enc.pHDRDynamicInfoAddr != NULL) {
            Codec_OSAL_MemoryUnmap(pCtx->videoCtx.specificInfo.enc.pHDRDynamicInfoAddr,
                    sizeof(ExynosVideoHdrDynamic) * VIDEO_BUFFER_MAX_NUM);
            pCtx->videoCtx.specificInfo.enc.pHDRDynamicInfoAddr = NULL;
        }

        /* free a ion_buffer */
        if (pCtx->videoCtx.specificInfo.enc.nHDRDynamicInfoFD >= 0) {
            close(pCtx->videoCtx.specificInfo.enc.nHDRDynamicInfoFD);
            pCtx->videoCtx.specificInfo.enc.nHDRDynamicInfoFD = -1;
        }
    }

    if (pCtx->videoCtx.instInfo.supportInfo.enc.bHDRDynamicStatInfoSupport == VIDEO_TRUE) {
        if (pCtx->videoCtx.specificInfo.enc.pHDRDynamicStatInfoAddr != NULL) {
            Codec_OSAL_MemoryUnmap(pCtx->videoCtx.specificInfo.enc.pHDRDynamicStatInfoAddr,
                    sizeof(ExynosVideoHdrDynamicStat) * VIDEO_BUFFER_MAX_NUM);
            pCtx->videoCtx.specificInfo.enc.pHDRDynamicStatInfoAddr = NULL;
        }

        /* free a ion_buffer */
        if (pCtx->videoCtx.specificInfo.enc.nHDRDynamicStatInfoFD >= 0) {
            close(pCtx->videoCtx.specificInfo.enc.nHDRDynamicStatInfoFD);
            pCtx->videoCtx.specificInfo.enc.nHDRDynamicStatInfoFD = -1;
        }
    }
}

void MFC_Encoder_ResourceClear(CodecOSALVideoContext *pCtx) {
    MFC_ResourceClear(pCtx, EncSpecificClear);
}

/*
 * [Encoder OPS] Init
 */
static void *MFC_Encoder_Init(ExynosVideoInstInfo *pVideoInfo) {
    CodecOSALVideoContext *pCtx     = NULL;

    int ret = 0;
    int fd = -1;
    int i;

    if (CHECK_POINTER(pVideoInfo) == false) {
        goto EXIT_ALLOC_FAIL;
    }

    pCtx = (CodecOSALVideoContext *)malloc(sizeof(*pCtx));
    if (CHECK_POINTER(pCtx) == false) {
        goto EXIT_ALLOC_FAIL;
    }

    memset(pCtx, 0, sizeof(*pCtx));

    pCtx->videoCtx.hDevice = -1;
    pCtx->videoCtx.hUser = -1;
    pCtx->videoCtx.hIONHandle = -1;
    pCtx->videoCtx.specificInfo.enc.nTemporalLayerFD = -1;
    pCtx->videoCtx.specificInfo.enc.nROIFD = -1;
    pCtx->videoCtx.specificInfo.enc.nHDRDynamicInfoFD = -1;
    pCtx->videoCtx.specificInfo.enc.nHDRDynamicStatInfoFD = -1;

    /* node open */
#ifdef USE_HEVC_HWIP
    if (pVideoInfo->eCodecType == VIDEO_CODING_HEVC) {
        if (pVideoInfo->eSecurityType == VIDEO_SECURE)
            ret = Codec_OSAL_DevOpen(VIDEO_HEVC_SECURE_ENCODER_NAME, O_RDWR, pCtx);
        else
            ret = Codec_OSAL_DevOpen(VIDEO_HEVC_ENCODER_NAME, O_RDWR, pCtx);
    } else
#endif
    {
        if (pVideoInfo->bOTFMode == VIDEO_TRUE) {
            if (pVideoInfo->eSecurityType == VIDEO_SECURE)
                ret = Codec_OSAL_DevOpen(VIDEO_MFC_OTF_SECURE_ENCODER_NAME, O_RDWR, pCtx);
            else
                ret = Codec_OSAL_DevOpen(VIDEO_MFC_OTF_ENCODER_NAME, O_RDWR, pCtx);
        } else {
            if (pVideoInfo->eSecurityType == VIDEO_SECURE)
                ret = Codec_OSAL_DevOpen(VIDEO_MFC_SECURE_ENCODER_NAME, O_RDWR, pCtx);
            else
                ret = Codec_OSAL_DevOpen(VIDEO_MFC_ENCODER_NAME, O_RDWR, pCtx);
        }
    }

    if (ret < 0) {
        ALOGE("%s: Failed to open encoder device", __FUNCTION__);
        goto EXIT_OPEN_FAIL;
    }

    if (pVideoInfo->bInitialized == VIDEO_FALSE) {
        __GetInstInfo(pCtx, pVideoInfo);
    }
    memcpy(&pCtx->videoCtx.instInfo, pVideoInfo, sizeof(pCtx->videoCtx.instInfo));

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

    if (pCtx->videoCtx.instInfo.supportInfo.enc.bTemporalSvcSupport == VIDEO_TRUE) {
        fd = exynos_ion_alloc(pCtx->videoCtx.hIONHandle, sizeof(TemporalLayerBuffer),
                              EXYNOS_ION_HEAP_SYSTEM_MASK, ION_FLAG_CACHED);
        if (fd < 0) {
            ALOGE("%s: Failed to exynos_ion_alloc() for nTemporalLayerFD", __FUNCTION__);
            pCtx->videoCtx.specificInfo.enc.nTemporalLayerFD = -1;
            goto EXIT_QUERYCAP_FAIL;
        }
        pCtx->videoCtx.specificInfo.enc.nTemporalLayerFD = fd;
        pCtx->videoCtx.specificInfo.enc.pTemporalLayerAddr = Codec_OSAL_MemoryMap(NULL, sizeof(TemporalLayerBuffer),
                                                   PROT_READ | PROT_WRITE, MAP_SHARED,  pCtx->videoCtx.specificInfo.enc.nTemporalLayerFD, 0);
        if (pCtx->videoCtx.specificInfo.enc.pTemporalLayerAddr == MAP_FAILED) {
            ALOGE("%s: Failed to mmap for pTemporalLayerAddr", __FUNCTION__);
            pCtx->videoCtx.specificInfo.enc.pTemporalLayerAddr = NULL;
            goto EXIT_QUERYCAP_FAIL;
        }

        memset(pCtx->videoCtx.specificInfo.enc.pTemporalLayerAddr, 0, sizeof(TemporalLayerBuffer));
    }

    if (pCtx->videoCtx.instInfo.supportInfo.enc.bRoiInfoSupport == VIDEO_TRUE) {
        fd = exynos_ion_alloc(pCtx->videoCtx.hIONHandle, sizeof(RoiInfoBuffer),
                              EXYNOS_ION_HEAP_SYSTEM_MASK, ION_FLAG_CACHED);
        if (fd < 0) {
            ALOGE("%s: Failed to exynos_ion_alloc() for nROIFD", __FUNCTION__);
            pCtx->videoCtx.specificInfo.enc.nROIFD = -1;
            goto EXIT_QUERYCAP_FAIL;
        }
        pCtx->videoCtx.specificInfo.enc.nROIFD = fd;
        pCtx->videoCtx.specificInfo.enc.pROIAddr = Codec_OSAL_MemoryMap(NULL, sizeof(RoiInfoBuffer),
                                                   PROT_READ | PROT_WRITE, MAP_SHARED,  pCtx->videoCtx.specificInfo.enc.nROIFD, 0);
        if (pCtx->videoCtx.specificInfo.enc.pROIAddr == MAP_FAILED) {
            ALOGE("%s: Failed to mmap for pROIAddr", __FUNCTION__);
            pCtx->videoCtx.specificInfo.enc.pROIAddr = NULL;
            goto EXIT_QUERYCAP_FAIL;
        }

        memset(pCtx->videoCtx.specificInfo.enc.pROIAddr, 0, sizeof(RoiInfoBuffer));
    }

    /* for HDR Dynamic Info */
    if (pCtx->videoCtx.instInfo.supportInfo.enc.bHDRDynamicInfoSupport == VIDEO_TRUE) {
        fd = exynos_ion_alloc(pCtx->videoCtx.hIONHandle, sizeof(ExynosVideoHdrDynamic) * VIDEO_BUFFER_MAX_NUM,
                              EXYNOS_ION_HEAP_SYSTEM_MASK, ION_FLAG_CACHED);
        if (fd < 0) {
            ALOGE("%s: Failed to exynos_ion_alloc() for nHDRDynamicInfoFD", __FUNCTION__);
            pCtx->videoCtx.specificInfo.enc.nHDRDynamicInfoFD = -1;
            goto EXIT_QUERYCAP_FAIL;
        }

        pCtx->videoCtx.specificInfo.enc.nHDRDynamicInfoFD = fd;

        pCtx->videoCtx.specificInfo.enc.pHDRDynamicInfoAddr =
                                        Codec_OSAL_MemoryMap(NULL, (sizeof(ExynosVideoHdrDynamic) * VIDEO_BUFFER_MAX_NUM),
                                              PROT_READ | PROT_WRITE, MAP_SHARED,
                                              pCtx->videoCtx.specificInfo.enc.nHDRDynamicInfoFD, 0);
        if (pCtx->videoCtx.specificInfo.enc.pHDRDynamicInfoAddr == MAP_FAILED) {
            ALOGE("%s: Failed to mmap for pHDRDynamicInfoAddr", __FUNCTION__);
            pCtx->videoCtx.specificInfo.enc.pHDRDynamicInfoAddr = NULL;
            goto EXIT_QUERYCAP_FAIL;
        }

        memset(pCtx->videoCtx.specificInfo.enc.pHDRDynamicInfoAddr,
                0, sizeof(ExynosVideoHdrDynamic) * VIDEO_BUFFER_MAX_NUM);

        if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_VIDEO_SET_HDR_USER_SHARED_HANDLE,
                                    pCtx->videoCtx.specificInfo.enc.nHDRDynamicInfoFD) != 0) {
            ALOGE("[%s] Failed to Codec_OSAL_SetControl(CODEC_OSAL_CID_VIDEO_SET_HDR_USER_SHARED_HANDLE)", __FUNCTION__);
            goto EXIT_QUERYCAP_FAIL;
        }
    }

    /* for HDR Dynamic Statistic Info */
    if (pCtx->videoCtx.instInfo.supportInfo.enc.bHDRDynamicStatInfoSupport == VIDEO_TRUE) {
        fd = exynos_ion_alloc(pCtx->videoCtx.hIONHandle, sizeof(ExynosVideoHdrDynamicStat) * VIDEO_BUFFER_MAX_NUM,
                              EXYNOS_ION_HEAP_SYSTEM_MASK, 0 /* non-cached */);
        if (fd < 0) {
            ALOGE("%s: Failed to exynos_ion_alloc() for nHDRDynamicStatInfoFD", __FUNCTION__);
            pCtx->videoCtx.specificInfo.enc.nHDRDynamicStatInfoFD = -1;
            goto EXIT_QUERYCAP_FAIL;
        }

        pCtx->videoCtx.specificInfo.enc.nHDRDynamicStatInfoFD = fd;

        pCtx->videoCtx.specificInfo.enc.pHDRDynamicStatInfoAddr =
                                        Codec_OSAL_MemoryMap(NULL, (sizeof(ExynosVideoHdrDynamicStat) * VIDEO_BUFFER_MAX_NUM),
                                              PROT_READ | PROT_WRITE, MAP_SHARED,
                                              pCtx->videoCtx.specificInfo.enc.nHDRDynamicStatInfoFD, 0);
        if (pCtx->videoCtx.specificInfo.enc.pHDRDynamicStatInfoAddr == MAP_FAILED) {
            ALOGE("%s: Failed to mmap for pHDRDynamicStatInfoAddr", __FUNCTION__);
            pCtx->videoCtx.specificInfo.enc.pHDRDynamicStatInfoAddr = NULL;
            goto EXIT_QUERYCAP_FAIL;
        }

        memset(pCtx->videoCtx.specificInfo.enc.pHDRDynamicStatInfoAddr,
                0, sizeof(ExynosVideoHdrDynamicStat) * VIDEO_BUFFER_MAX_NUM);

        if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ENC_HDR_STAT_USER_SHARED_HANDLE,
                                    pCtx->videoCtx.specificInfo.enc.nHDRDynamicStatInfoFD) != 0) {
            ALOGE("[%s] Failed to Codec_OSAL_SetControl(CODEC_OSAL_CID_ENC_HDR_STAT_USER_SHARED_HANDLE)", __FUNCTION__);
            goto EXIT_QUERYCAP_FAIL;
        }
    }


    return (void *)pCtx;

EXIT_QUERYCAP_FAIL:
    MFC_Encoder_ResourceClear(pCtx);

EXIT_OPEN_FAIL:
    free(pCtx);

EXIT_ALLOC_FAIL:
    return NULL;
}

/*
 * [Encoder OPS] Finalize
 */
static ExynosVideoErrorType MFC_Encoder_Finalize(void *pHandle) {
    CodecOSALVideoContext *pCtx         = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPlane      *pVideoPlane  = NULL;
    pthread_mutex_t       *pMutex       = NULL;
    ExynosVideoErrorType   ret          = VIDEO_ERROR_NONE;
    int i, j;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    MFC_Encoder_ResourceClear(pCtx);

    free(pCtx);

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Frame Tag
 */
static ExynosVideoErrorType MFC_Encoder_Set_FrameTag(
    void   *pHandle,
    int     nFrameTag) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_VIDEO_FRAME_TAG, nFrameTag) != 0) {
        ALOGE("%s: Failed to SetControl(val : 0x%x)", __FUNCTION__, nFrameTag);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Get Frame Tag
 */
static int MFC_Encoder_Get_FrameTag(void *pHandle) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;

    int nFrameTag = -1;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_GetControl(pCtx, CODEC_OSAL_CID_VIDEO_FRAME_TAG, &nFrameTag) != 0) {
        ALOGE("%s: Failed to GetControl(val : 0x%x)", __FUNCTION__, nFrameTag);
        goto EXIT;
    }

EXIT:
    return nFrameTag;
}

/*
 * [Encoder OPS] Set Extended Control
 */
static ExynosVideoErrorType MFC_Encoder_Set_EncParam(
    void                *pHandle,
    ExynosVideoEncParam *pEncParam) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if ((CHECK_POINTER(pCtx) == false) ||
        (CHECK_POINTER(pEncParam) == false)) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (Codec_OSAL_SetControls(pCtx, CODEC_OSAL_CID_ENC_SET_PARAMS, (void *)pEncParam) != 0) {
        ALOGE("%s: Failed to SetControls", __FUNCTION__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Slice Mode
 */
static ExynosVideoErrorType MFC_Encoder_Set_SliceMode(
    void *pHandle,
    int   nSliceMode,
    int   nSliceArgument) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    int value;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    value = (((nSliceMode << 16) & 0xffff0000) | (nSliceArgument & 0xffff));

#if 0  // TODO
    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ENC_SLICE_MODE, value) != 0) {
        ALOGE("%s: Failed to SetControl(val : 0x%x)", __FUNCTION__, value);
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
 * [Encoder OPS] Enable Prepend SPS and PPS to every IDR Frames
 */
static ExynosVideoErrorType MFC_Encoder_Enable_PrependSpsPpsToIdr(void *pHandle) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    switch ((int)pCtx->videoCtx.instInfo.eCodecType) {
    case VIDEO_CODING_AVC:
        if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ENC_H264_PREPEND_SPSPPS_TO_IDR, 1) != 0) {
            ALOGE("%s: Failed to SetControl", __FUNCTION__);
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }
        break;
    case VIDEO_CODING_HEVC:
        if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ENC_HEVC_PREPEND_SPSPPS_TO_IDR, 1) != 0) {
            ALOGE("%s: Failed to SetControl", __FUNCTION__);
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }
        break;
    default:
        ALOGE("%s: codec(%x) can not support PrependSpsPpsToIdr", __FUNCTION__, pCtx->videoCtx.instInfo.eCodecType);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Frame Type
 */
static ExynosVideoErrorType MFC_Encoder_Set_FrameType(
    void                    *pHandle,
    ExynosVideoFrameType     eFrameType) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ENC_FRAME_TYPE, eFrameType) != 0) {
        ALOGE("%s: Failed to SetControl(val : 0x%x)", __FUNCTION__, eFrameType);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Frame Skip
 */
static ExynosVideoErrorType MFC_Encoder_Set_FrameSkip(
    void   *pHandle,
    int     nFrameSkip) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ENC_FRAME_SKIP_MODE, nFrameSkip) != 0) {
        ALOGE("%s: Failed to SetControl(val : 0x%x)", __FUNCTION__, nFrameSkip);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Frame Rate
 */
static ExynosVideoErrorType MFC_Encoder_Set_FrameRate(
    void   *pHandle,
    int     nFramerate) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ENC_FRAME_RATE, nFramerate) != 0) {
        ALOGE("%s: Failed to SetControl(val : 0x%x)", __FUNCTION__, nFramerate);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Bit Rate
 */
static ExynosVideoErrorType MFC_Encoder_Set_BitRate(
    void   *pHandle,
    int     nBitrate) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ENC_BIT_RATE, nBitrate) != 0) {
        ALOGE("%s: Failed to SetControl(val : 0x%x)", __FUNCTION__, nBitrate);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Quantization Min/Max
 */
static ExynosVideoErrorType MFC_Encoder_Set_QuantizationRange(
    void                *pHandle,
    ExynosVideoQPRange   qpRange) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (qpRange.QpMin_I > qpRange.QpMax_I) {
        ALOGE("%s: QP(I) range(%d, %d) is wrong", __FUNCTION__, qpRange.QpMin_I, qpRange.QpMax_I);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (Codec_OSAL_SetControls(pCtx, CODEC_OSAL_CID_ENC_QP_RANGE, &qpRange) != 0) {
        ALOGE("%s: Failed to SetControl", __FUNCTION__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set IDR Period
 */
static ExynosVideoErrorType MFC_Encoder_Set_IDRPeriod(
    void   *pHandle,
    int     nIDRPeriod) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ENC_IDR_PERIOD, (nIDRPeriod < 0)? 0:nIDRPeriod) != 0) {
        ALOGE("%s: Failed to SetControl(val : 0x%x)", __FUNCTION__, nIDRPeriod);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Qos Ratio
 */
static ExynosVideoErrorType MFC_Encoder_Set_QosRatio(
    void *pHandle,
    int   nRatio) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_VIDEO_QOS_RATIO, nRatio) != 0) {
        ALOGE("%s: Failed to SetControl(val : 0x%x)", __FUNCTION__, nRatio);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Dynamic QP Control
 */
static ExynosVideoErrorType MFC_Encoder_Set_DynamicQpControl(
    void    *pHandle,
    int      nQp) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ENC_SET_CONFIG_QP, nQp) != 0) {
        ALOGE("%s: Failed to SetControl(val : 0x%x)", __FUNCTION__, nQp);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Mark LTR-frame
 */
static ExynosVideoErrorType MFC_Encoder_Set_MarkLTRFrame(
    void    *pHandle,
    int      nLongTermFrmIdx) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ENC_H264_LTR_MARK_INDEX, nLongTermFrmIdx) != 0) {
        ALOGE("%s: Failed to SetControl(val : 0x%x)", __FUNCTION__, nLongTermFrmIdx);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Used LTR-frame
 */
static ExynosVideoErrorType MFC_Encoder_Set_UsedLTRFrame(
    void    *pHandle,
    int      nUsedLTRFrameNum) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ENC_H264_LTR_USE_INDEX, nUsedLTRFrameNum) != 0) {
        ALOGE("%s: Failed to SetControl(val : 0x%x)", __FUNCTION__, nUsedLTRFrameNum);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Base PID
 */
static ExynosVideoErrorType MFC_Encoder_Set_BasePID(
    void    *pHandle,
    int      nPID) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ENC_H264_LTR_BASE_PID, nPID) != 0) {
        ALOGE("%s: Failed to SetControl(val : 0x%x)", __FUNCTION__, nPID);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Layer Change
 */
static ExynosVideoErrorType MFC_Encoder_Set_LayerChange(
    void                *pHandle,
    TemporalLayerBuffer  TemporalSVC) {
    CodecOSALVideoContext   *pCtx           = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType     ret            = VIDEO_ERROR_NONE;
    TemporalLayerBuffer     *pTemporalLayer = NULL;

    unsigned int CID = 0;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    pTemporalLayer = (TemporalLayerBuffer *)pCtx->videoCtx.specificInfo.enc.pTemporalLayerAddr;

    switch ((int)pCtx->videoCtx.instInfo.eCodecType) {
    case VIDEO_CODING_AVC:
        CID = CODEC_OSAL_CID_ENC_H264_TEMPORAL_SVC_LAYER_CH;
        break;
    case VIDEO_CODING_HEVC:
        CID = CODEC_OSAL_CID_ENC_HEVC_TEMPORAL_SVC_LAYER_CH;
        break;
    case VIDEO_CODING_VP8:
        CID = CODEC_OSAL_CID_ENC_VP8_TEMPORAL_SVC_LAYER_CH;
        break;
    case VIDEO_CODING_VP9:
        CID = CODEC_OSAL_CID_ENC_VP9_TEMPORAL_SVC_LAYER_CH;
        break;
    default:
        ALOGE("%s: this codec type is not supported(%x), F/W ver(%x)",
                __FUNCTION__, pCtx->videoCtx.instInfo.eCodecType, pCtx->videoCtx.instInfo.HwVersion);
        ret = VIDEO_ERROR_NOSUPPORT;
        goto EXIT;
        break;
    }

    memcpy(pTemporalLayer, &TemporalSVC, sizeof(TemporalLayerBuffer));
    if (Codec_OSAL_SetControl(pCtx, CID,
                                pCtx->videoCtx.specificInfo.enc.nTemporalLayerFD) != 0) {
        ALOGE("%s: Failed to SetControl", __FUNCTION__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Enable Adaptive Layer Bitrate mode
 */
static ExynosVideoErrorType MFC_Encoder_Enable_AdaptiveLayerBitrate(
    void    *pHandle,
    int      bEnable) {
    CodecOSALVideoContext   *pCtx  = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType     ret   = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    switch ((int)pCtx->videoCtx.instInfo.eCodecType) {
    case VIDEO_CODING_AVC:
    case VIDEO_CODING_HEVC:
    case VIDEO_CODING_VP8:
    case VIDEO_CODING_VP9:
        if (pCtx->videoCtx.instInfo.supportInfo.enc.bAdaptiveLayerBitrateSupport == VIDEO_TRUE) {
            if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ENC_ENABLE_ADAPTIVE_LAYER_BITRATE, bEnable) != 0) {
                ret = VIDEO_ERROR_APIFAIL;
                goto EXIT;
            }
        }

        ret = VIDEO_ERROR_NONE;
        break;
    default:
        ALOGE("%s: codec(%x) can not support Enable_AdaptiveLayerBitrate", __FUNCTION__, pCtx->videoCtx.instInfo.eCodecType);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Color Aspects
 */
ExynosVideoErrorType MFC_Encoder_Set_ColorAspects(
    void                    *pHandle,
    ExynosVideoColorAspects *pColorAspects) {
    CodecOSALVideoContext   *pCtx  = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType     ret   = VIDEO_ERROR_NONE;

    if ((CHECK_POINTER(pCtx) == false) ||
        (CHECK_POINTER(pColorAspects) == false)) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->videoCtx.instInfo.supportInfo.enc.bColorAspectsSupport == VIDEO_FALSE) {
        ALOGE("%s: can not support Set_ColorAspects. // codec type(%x), F/W ver(%x)",
                    __FUNCTION__, pCtx->videoCtx.instInfo.eCodecType, pCtx->videoCtx.instInfo.HwVersion);
        ret = VIDEO_ERROR_NOSUPPORT;
        goto EXIT;
    }

    if (Codec_OSAL_SetControls(pCtx, CODEC_OSAL_CID_ENC_COLOR_ASPECTS, (void *)pColorAspects) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    memcpy(&(pCtx->videoCtx.pad[VIDEO_INDEX_SRC_PAD].geometry.HdrInfo.sColorAspects), pColorAspects, sizeof(ExynosVideoColorAspects));
    pCtx->videoCtx.pad[VIDEO_INDEX_SRC_PAD].geometry.HdrInfo.eValidType |= (HDR_INFO_RANGE | HDR_INFO_COLOR_ASPECTS);

    ret = VIDEO_ERROR_NONE;

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set HDR Static Info
 */
ExynosVideoErrorType MFC_Encoder_Set_HDRStaticInfo(
    void                  *pHandle,
    ExynosVideoHdrStatic  *pHDRStaticInfo) {
    CodecOSALVideoContext   *pCtx  = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType     ret   = VIDEO_ERROR_NONE;

    if ((CHECK_POINTER(pCtx) == false) ||
        (CHECK_POINTER(pHDRStaticInfo) == false)) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->videoCtx.instInfo.supportInfo.enc.bHDRStaticInfoSupport == VIDEO_FALSE) {
        ALOGE("%s: can not support Set_HDRStaticInfo. // codec type(%x), F/W ver(%x)",
                    __FUNCTION__, pCtx->videoCtx.instInfo.eCodecType, pCtx->videoCtx.instInfo.HwVersion);
        ret = VIDEO_ERROR_NOSUPPORT;
        goto EXIT;
    }

    switch ((int)pCtx->videoCtx.instInfo.eCodecType) {
    case VIDEO_CODING_HEVC:
        if (Codec_OSAL_SetControls(pCtx, CODEC_OSAL_CID_ENC_HDR_STATIC_INFO, (void *)pHDRStaticInfo) != 0) {
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }

        memcpy(&(pCtx->videoCtx.pad[VIDEO_INDEX_SRC_PAD].geometry.HdrInfo.sHdrStatic), pHDRStaticInfo, sizeof(ExynosVideoHdrStatic));
        pCtx->videoCtx.pad[VIDEO_INDEX_SRC_PAD].geometry.HdrInfo.eValidType |= (HDR_INFO_LIGHT | HDR_INFO_LUMINANCE);
        ret = VIDEO_ERROR_NONE;
        break;
    default:
        ALOGE("%s: codec(%x) can not support Set_HDRStaticInfo", __FUNCTION__, pCtx->videoCtx.instInfo.eCodecType);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set HDR Dynamic Info
 */
ExynosVideoErrorType MFC_Encoder_Set_HDRDynamicInfo(
    void                  *pHandle,
    ExynosVideoHdrDynamic *pHDRDynamicInfo) {
    CodecOSALVideoContext   *pCtx  = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType     ret   = VIDEO_ERROR_NONE;

    if ((CHECK_POINTER(pCtx) == false) ||
        (CHECK_POINTER(pHDRDynamicInfo) == false)) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->videoCtx.instInfo.supportInfo.enc.bHDRDynamicInfoSupport == VIDEO_FALSE) {
        ALOGE("%s: can not support Set_HDRStaticInfo. // codec type(%x), F/W ver(%x)",
                    __FUNCTION__, pCtx->videoCtx.instInfo.eCodecType, pCtx->videoCtx.instInfo.HwVersion);
        ret = VIDEO_ERROR_NOSUPPORT;
        goto EXIT;
    }

    memcpy(&(pCtx->videoCtx.pad[VIDEO_INDEX_SRC_PAD].geometry.HdrInfo.sHdrDynamic), pHDRDynamicInfo, sizeof(ExynosVideoHdrDynamic));
    pCtx->videoCtx.pad[VIDEO_INDEX_SRC_PAD].geometry.HdrInfo.eValidType |= HDR_INFO_DYNAMIC_META;

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Enable HDR Dynamic Statistic
 */
ExynosVideoErrorType MFC_Encoder_Enable_HDRDynamicStatInfo(void *pHandle) {
    CodecOSALVideoContext   *pCtx  = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType     ret   = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if ((pCtx->videoCtx.instInfo.supportInfo.enc.bHDRDynamicStatInfoSupport == VIDEO_FALSE) ||
        (pCtx->videoCtx.instInfo.eCodecType != VIDEO_CODING_HEVC)) {
        ALOGE("%s: can not support Set_HDRDynamicStatInfo. // codec type(%x), F/W ver(%x)",
                    __FUNCTION__, pCtx->videoCtx.instInfo.eCodecType, pCtx->videoCtx.instInfo.HwVersion);
        ret = VIDEO_ERROR_NOSUPPORT;
        goto EXIT;
    }


    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ENC_HDR_STAT_ENABLE, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pCtx->videoCtx.specificInfo.enc.bHDR10PlusStatMode = VIDEO_TRUE;

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Get HDR Dynamic Statistic Info
 */
ExynosVideoErrorType MFC_Encoder_Get_HDRDynamicStatInfo(
    void                      *pHandle,
    ExynosVideoHdrDynamicStat *pHDRDynamicStatInfo) {
    CodecOSALVideoContext       *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo          *pDstPad  = NULL;
    ExynosVideoErrorType         ret      = VIDEO_ERROR_NONE;

    if ((CHECK_POINTER(pCtx) == false) ||
        (CHECK_POINTER(pHDRDynamicStatInfo) == false)) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->videoCtx.instInfo.supportInfo.enc.bHDRDynamicStatInfoSupport == VIDEO_FALSE) {
        ALOGE("%s: can not support Get_HDRDynamicStatInfo. // codec type(%x), F/W ver(%x)",
                    __FUNCTION__, pCtx->videoCtx.instInfo.eCodecType, pCtx->videoCtx.instInfo.HwVersion);
        ret = VIDEO_ERROR_NOSUPPORT;
        goto EXIT;
    }

    pDstPad = &(pCtx->videoCtx.pad[VIDEO_INDEX_DST_PAD]);
    memcpy(pHDRDynamicStatInfo, &(pDstPad->geometry.HdrDynamicStatInfo), sizeof(ExynosVideoHdrDynamicStat));

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Roi Information
 */
static ExynosVideoErrorType MFC_Encoder_Set_RoiInfo(
    void            *pHandle,
    RoiInfoBuffer   *pRoiInfo) {
    CodecOSALVideoContext   *pCtx  = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType     ret   = VIDEO_ERROR_NONE;
    RoiInfoBuffer           *pROI  = NULL;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    pROI = (RoiInfoBuffer *)pCtx->videoCtx.specificInfo.enc.pROIAddr;
    if (pCtx->videoCtx.instInfo.supportInfo.enc.bRoiInfoSupport == VIDEO_FALSE) {
        ALOGE("%s: ROI Info setting is not supported :: codec type(%x), F/W ver(%x)",
                    __FUNCTION__, pCtx->videoCtx.instInfo.eCodecType, pCtx->videoCtx.instInfo.HwVersion);
        ret = VIDEO_ERROR_NOSUPPORT;
        goto EXIT;
    }

    memcpy(pROI, pRoiInfo, sizeof(RoiInfoBuffer));
    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ENC_ROI_INFO,
                                pCtx->videoCtx.specificInfo.enc.nROIFD) != 0) {
        ALOGE("%s: Failed to SetControl", __FUNCTION__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Enable Weighted Prediction Encoding
 */
static ExynosVideoErrorType MFC_Encoder_Enable_WeightedPrediction(void *pHandle) {
    CodecOSALVideoContext   *pCtx  = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType     ret   = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ENC_WP_ENABLE, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Y-SUM Data
 */
static ExynosVideoErrorType MFC_Encoder_Set_YSumData(
    void           *pHandle,
    unsigned int    nHighData,
    unsigned int    nLowData) {
    CodecOSALVideoContext   *pCtx  = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType     ret   = VIDEO_ERROR_NONE;

    unsigned long long nYSumData = 0;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    nYSumData = (((unsigned long long)nHighData) << 32) | nLowData;  /* 64bit data */

    /* MFC D/D just want a LowData */
    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ENC_YSUM_DATA, (int)nLowData) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set I-frame ratio (It is applied only CBR mode)
 */
static ExynosVideoErrorType MFC_Encoder_Set_IFrameRatio(
    void    *pHandle,
    int      nRatio) {
    CodecOSALVideoContext   *pCtx  = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType     ret   = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    switch ((int)pCtx->videoCtx.instInfo.eCodecType) {
    case VIDEO_CODING_AVC:
    case VIDEO_CODING_HEVC:
        if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ENC_I_FRAME_RATIO, nRatio) != 0) {
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }

        ret = VIDEO_ERROR_NONE;
        break;
    default:
        ALOGE("%s: codec(%x) can not support Set_IFrameRatio", __FUNCTION__, pCtx->videoCtx.instInfo.eCodecType);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Header Mode
 */
ExynosVideoErrorType MFC_Encoder_Set_HeaderMode(
    void                  *pHandle,
    ExynosVideoBoolType    bSeparated) {
    CodecOSALVideoContext   *pCtx  = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType     ret   = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ENC_HEADER_MODE,
            ((bSeparated == VIDEO_TRUE)? CODEC_OSAL_HEADER_MODE_SEPARATE:CODEC_OSAL_HEADER_WITH_1ST_IDR)) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Drop Control
 */
ExynosVideoErrorType MFC_Encoder_Set_DropControl(
    void                  *pHandle,
    ExynosVideoBoolType    bEnable) {
    CodecOSALVideoContext   *pCtx  = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType     ret   = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (pCtx->videoCtx.instInfo.supportInfo.enc.bDropControlSupport == VIDEO_FALSE) {
        ALOGE("%s: can't support Set_DropControl. // codec type(%x), F/W ver(%x)",
                    __FUNCTION__, pCtx->videoCtx.instInfo.eCodecType, pCtx->videoCtx.instInfo.HwVersion);
        ret = VIDEO_ERROR_NOSUPPORT;
        goto EXIT;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ENC_ENABLE_DROP_CTRL, (bEnable == VIDEO_TRUE)? 1:0) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Disable DFR
 */
ExynosVideoErrorType MFC_Encoder_Disable_DynamicFrameRate(void *pHandle) {
    CodecOSALVideoContext   *pCtx  = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType     ret   = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    pCtx->videoCtx.specificInfo.enc.bDisableDFR = VIDEO_TRUE;

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set ActualFormat
 */
ExynosVideoErrorType MFC_Encoder_Set_ActualFormat(
    void   *pHandle,
    int     nFormat) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo    *pSrcPad  = NULL;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;

    int i;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    pSrcPad = &(pCtx->videoCtx.pad[VIDEO_INDEX_SRC_PAD]);

    for (i = 0; i < (int)(sizeof(SBWC_NONCOMP_TABLE)/sizeof(SBWC_NONCOMP_TABLE[0])); i++) {
        if (SBWC_NONCOMP_TABLE[i].eSBWCFormat == pSrcPad->geometry.eColorFormat) {
            if (SBWC_NONCOMP_TABLE[i].eNormalFormat == nFormat) {
                pCtx->videoCtx.specificInfo.enc.actualFormat = (ExynosVideoColorFormatType)nFormat;
                ALOGV("%s: actual format is 0x%x", __FUNCTION__, pCtx->videoCtx.specificInfo.enc.actualFormat);
                goto EXIT;
            }
        }
    }

    /* start with non SBWC, discard nFormat */
    // ret = VIDEO_ERROR_BADPARAM;

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Enable GDCvOTF
 */
ExynosVideoErrorType MFC_Encoder_Enable_GDCvOTF(void *pHandle, int nType) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NOSUPPORT;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (pCtx->videoCtx.instInfo.supportInfo.enc.bGDCvOTFSupport == VIDEO_TRUE) {
        if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ENC_GDC_VOTF, nType) != 0) {
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }

        pCtx->videoCtx.specificInfo.enc.bEnableGDCvOTF = VIDEO_TRUE;
        ret = VIDEO_ERROR_NONE;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Enable GDCvOTF
 */
ExynosVideoErrorType MFC_Encoder_Set_GDCvOTF(void *pHandle, ExynosVideoBoolType bEnable) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->videoCtx.instInfo.supportInfo.enc.bGDCvOTFSupport == VIDEO_TRUE) {
        pCtx->videoCtx.specificInfo.enc.bUseGDCvOTF = bEnable;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Operating Rate
 */
static ExynosVideoErrorType MFC_Encoder_Set_OperatingRate(
    void        *pHandle,
    unsigned int framerate) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->videoCtx.instInfo.supportInfo.enc.bOperatingRateSupport != VIDEO_TRUE) {
        ret = VIDEO_ERROR_NOSUPPORT;
        goto EXIT;
    }

    ret = MFC_Set_OperatingRate(pHandle, framerate);

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set RealTimePriority
 */
static ExynosVideoErrorType MFC_Encoder_Set_RealTimePriority(
    void        *pHandle,
    unsigned int RealTimePriority) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->videoCtx.instInfo.supportInfo.enc.bRealTimePrioritySupport != VIDEO_TRUE) {
        ret = VIDEO_ERROR_NOSUPPORT;
        goto EXIT;
    }

    ret = MFC_Set_RealTimePriority(pHandle, RealTimePriority);

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Get Real Time Priority
 */
ExynosVideoErrorType MFC_Encoder_Get_RealTimePriority(void *pHandle) {
    return MFC_Get_RealTimePriority(pHandle);
}

/*
 * [Encoder OPS] Get Average QP
 */
static ExynosVideoErrorType MFC_Encoder_Get_AverageQp(void *pHandle, int *pQpValue) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    int i = 0;

    if ((CHECK_POINTER(pCtx) == false) ||
        (CHECK_POINTER(pQpValue) == false)) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->videoCtx.instInfo.supportInfo.enc.bAverageQpSupport != VIDEO_TRUE) {
        ret = VIDEO_ERROR_NOSUPPORT;
        goto EXIT;
    }

    if (Codec_OSAL_GetControl(pCtx, CODEC_OSAL_CID_ENC_AVERAGE_QP, pQpValue) != 0) {
        ALOGE("%s: Failed to get average qp", __FUNCTION__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    switch (pCtx->videoCtx.instInfo.eCodecType) {
    case VIDEO_CODING_VP8:
    {
        for (i = 0; i < sizeof(vp8_qp_trans)/sizeof(vp8_qp_trans[0]); i++) {
            if ((*pQpValue) == vp8_qp_trans[i]) {
                (*pQpValue) = i;
                break;
            }
        }

        if (i == sizeof(vp8_qp_trans)/sizeof(vp8_qp_trans[0])) {
            /* can not find qp value on the table */
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }
    }
        break;
    case VIDEO_CODING_VP9:
    {
        for (i = 0; i < sizeof(vp9_qp_trans)/sizeof(vp9_qp_trans[0]); i++) {
            if ((*pQpValue) == vp9_qp_trans[i]) {
                (*pQpValue) = i;
                break;
            }
        }

        if (i == sizeof(vp9_qp_trans)/sizeof(vp9_qp_trans[0])) {
            /* can not find qp value on the table */
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }
    }
        break;
    default:
        /* nothing to do */
        break;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set PMV
 */
static ExynosVideoErrorType MFC_Encoder_Set_PMV(void *pHandle, ExynosVideoPMV *pPMV) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if ((CHECK_POINTER(pCtx) == false) ||
        (CHECK_POINTER(pPMV) == false)) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->videoCtx.instInfo.supportInfo.enc.bPMVSupport != VIDEO_TRUE) {
        ret = VIDEO_ERROR_NOSUPPORT;
        goto EXIT;
    }

    if (Codec_OSAL_SetControls(pCtx, CODEC_OSAL_CID_ENC_MV_VER_POSITION, pPMV) != 0) {
        ALOGE("%s: Failed to set PMV", __FUNCTION__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set GOP Mode
 */
static ExynosVideoErrorType MFC_Encoder_Set_GopMode(void *pHandle, ExynosVideoFrameType mode) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (CHECK_POINTER(pCtx) == false) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->videoCtx.instInfo.supportInfo.enc.bGopModeSupport != VIDEO_TRUE) {
        ret = VIDEO_ERROR_NOSUPPORT;
        goto EXIT;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ENC_GOP_CTRL, (mode == VIDEO_FRAME_I)? 1:0) != 0) {
        ALOGE("%s: Failed to set gop ctrl(%x)", __FUNCTION__, mode);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Min Quality
 */
static ExynosVideoErrorType MFC_Encoder_Set_MinQuality(void *pHandle, int level) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: invalid parameter", __FUNCTION__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ENC_MIN_QUALITY, level) != 0) {
        ALOGV("%s: Failed to set min quality ctrl(%d)", __FUNCTION__, level);
        // ret = VIDEO_ERROR_APIFAIL;  // discard an error
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Crop
 */
static ExynosVideoErrorType MFC_Encoder_Set_Crop(void *pHandle, ExynosVideoRect *pBufferCrop) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    CodecOSAL_Crop  crop;

    if (pCtx == NULL) {
        ALOGE("%s: invalid parameter", __FUNCTION__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&crop, 0, sizeof(crop));

    crop.type   = CODEC_OSAL_BUF_TYPE_SRC;
    crop.top    = pBufferCrop->nTop;
    crop.left   = pBufferCrop->nLeft;
    crop.width  = pBufferCrop->nWidth;
    crop.height = pBufferCrop->nHeight;

    if (Codec_OSAL_SetCrop(pCtx, &crop) != 0) {
        ALOGV("%s: Failed to set crop", __FUNCTION__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Max I Frame Size
 */
static ExynosVideoErrorType MFC_Encoder_Set_MaxIFrameSize(void *pHandle, int size) {
    CodecOSALVideoContext *pCtx = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: invalid parameter", __FUNCTION__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->videoCtx.instInfo.supportInfo.enc.bMaxIFrameSizeSupport != VIDEO_TRUE) {
        ret = VIDEO_ERROR_NOSUPPORT;
        goto EXIT;
    }

    if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ENC_MAX_IFRAME_SIZE, size) != 0) {
        ALOGE("%s: Failed to set max I frame size(%d)", __FUNCTION__, size);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Skip Lazy Unmap
 */
static ExynosVideoErrorType MFC_Encoder_Disable_LazyUnmap(void *pHandle) {
    return MFC_Disable_LazyUnmap(pHandle);
}

/*
 * [Encoder OPS] Stop Wait Buffer
 */
ExynosVideoErrorType MFC_Encoder_Stop_Wait_Buffer(void *pHandle) {
    return MFC_Stop_Wait_Buffer(pHandle);
}

/*
 * [Encoder OPS] Reset Wait Buffer
 */
ExynosVideoErrorType MFC_Encoder_Reset_Wait_Buffer(void *pHandle) {
    return MFC_Reset_Wait_Buffer(pHandle);
}

/*
 * [Encoder Buffer OPS] Enable Cacheable (Input)
 */
static ExynosVideoErrorType MFC_Encoder_Enable_Cacheable_Inbuf(void *pHandle) {
    return MFC_Enable_Cacheable_Inbuf(pHandle);
}

/*
 * [Encoder Buffer OPS] Enable Cacheable (Output)
 */
static ExynosVideoErrorType MFC_Encoder_Enable_Cacheable_Outbuf(void *pHandle) {
    return MFC_Enable_Cacheable_Outbuf(pHandle);
}

/*
 * [Encoder Buffer OPS] Set Geometry (Src)
 */
static ExynosVideoErrorType MFC_Encoder_Set_Geometry_Inbuf(
    void                *pHandle,
    ExynosVideoGeometry *pBufferConf) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo    *pSrcPad  = NULL;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;

    CodecOSAL_Format fmt;

    int i;

    if ((CHECK_POINTER(pCtx) == false) ||
        (CHECK_POINTER(pBufferConf) == false)) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    pSrcPad = &(pCtx->videoCtx.pad[VIDEO_INDEX_SRC_PAD]);

    memset(&fmt, 0, sizeof(fmt));

    fmt.type   = CODEC_OSAL_BUF_TYPE_SRC;
    fmt.format = Codec_OSAL_ColorFormatToPixelFormat(pBufferConf->eColorFormat, pCtx->videoCtx.instInfo.HwVersion);
    fmt.width  = pBufferConf->nWidth;
    fmt.height = pBufferConf->nHeight;
    fmt.stride = pBufferConf->nStride;
    fmt.cstride = pBufferConf->nCStride;
    fmt.nPlane = pBufferConf->nPlaneCnt;

    for (i = 0; i < (int)(sizeof(SBWC_LOSSY_TABLE)/sizeof(SBWC_LOSSY_TABLE[0])); i++) {
        if (pBufferConf->eColorFormat == SBWC_LOSSY_TABLE[i].eColorFormat)
            fmt.field |= (1 << SBWC_LOSSY_TABLE[i].nFormatFlag);
    }

    if (Codec_OSAL_SetFormat(pCtx, &fmt) != 0) {
        ALOGE("%s: Failed to SetFormat", __FUNCTION__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    memcpy(&(pSrcPad->geometry), pBufferConf, sizeof(ExynosVideoGeometry));
    pSrcPad->nPlane = pBufferConf->nPlaneCnt;

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Set Geometry (Dst)
 */
static ExynosVideoErrorType MFC_Encoder_Set_Geometry_Outbuf(
    void                *pHandle,
    ExynosVideoGeometry *pBufferConf) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo    *pDstPad  = NULL;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;

    CodecOSAL_Format fmt;

    if ((CHECK_POINTER(pCtx) == false) ||
        (CHECK_POINTER(pBufferConf) == false)) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    pDstPad = &(pCtx->videoCtx.pad[VIDEO_INDEX_DST_PAD]);

    memset(&fmt, 0, sizeof(fmt));
    fmt.type            = CODEC_OSAL_BUF_TYPE_DST;
    fmt.format          = Codec_OSAL_CodingTypeToCompressdFormat(pBufferConf->eCompressionFormat);
    fmt.planeSize[0]    = pBufferConf->nSizeImage;
    fmt.nPlane          = pBufferConf->nPlaneCnt;
    fmt.width           = pBufferConf->nWidth;
    fmt.height          = pBufferConf->nHeight;

    if (Codec_OSAL_SetFormat(pCtx, &fmt) != 0) {
        ALOGE("%s: Failed to SetFormat", __FUNCTION__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    memcpy(&(pDstPad->geometry), pBufferConf, sizeof(ExynosVideoGeometry));
    pDstPad->nPlane = pBufferConf->nPlaneCnt;

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Get Geometry (Src)
 */
static ExynosVideoErrorType MFC_Encoder_Get_Geometry_Inbuf(
    void                *pHandle,
    ExynosVideoGeometry *pBufferConf) {
    CodecOSALVideoContext *pCtx         = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret          = VIDEO_ERROR_NONE;
    ExynosVideoPadInfo    *pSrcPad      = NULL;
    ExynosVideoGeometry   *pInGeometry  = NULL;

    CodecOSAL_Format fmt;
    int i;

    if ((CHECK_POINTER(pCtx) == false) ||
        (CHECK_POINTER(pBufferConf) == false)) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    pSrcPad = &(pCtx->videoCtx.pad[VIDEO_INDEX_SRC_PAD]);

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = CODEC_OSAL_BUF_TYPE_SRC;

    if (Codec_OSAL_GetFormat(pCtx, &fmt) != 0) {
        ALOGE("%s: Failed to GetFormat", __FUNCTION__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pInGeometry = &(pSrcPad->geometry);

    pInGeometry->nWidth     = fmt.width;
    pInGeometry->nHeight    = fmt.height;
    pInGeometry->nSizeImage = fmt.planeSize[0];

    memcpy(pBufferConf, pInGeometry, sizeof(ExynosVideoGeometry));

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Get Geometry (Dst)
 */
static ExynosVideoErrorType MFC_Encoder_Get_Geometry_Outbuf(
    void                *pHandle,
    ExynosVideoGeometry *pBufferConf) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo    *pDstPad  = NULL;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;

    CodecOSAL_Format fmt;

    if ((CHECK_POINTER(pCtx) == false) ||
        (CHECK_POINTER(pBufferConf) == false)) {
        ALOGE("%s: invalid parameter", __FUNCTION__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    pDstPad = &(pCtx->videoCtx.pad[VIDEO_INDEX_DST_PAD]);

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = CODEC_OSAL_BUF_TYPE_DST;

    if (Codec_OSAL_GetFormat(pCtx, &fmt) != 0) {
        ALOGE("%s: Failed to GetFormat", __FUNCTION__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pDstPad->geometry.nSizeImage = fmt.planeSize[0];
    pBufferConf->nSizeImage = fmt.planeSize[0];

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Try Geometry (Src)
 */
static ExynosVideoErrorType MFC_Encoder_Try_Geometry_Inbuf(
    void                *pHandle,
    ExynosVideoGeometry *pBufferConf) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;

    CodecOSAL_Format fmt;

    int i;

    if ((CHECK_POINTER(pCtx) == false) ||
        (CHECK_POINTER(pBufferConf) == false)) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&fmt, 0, sizeof(fmt));

    fmt.type    = CODEC_OSAL_BUF_TYPE_SRC;
    fmt.format  = Codec_OSAL_ColorFormatToPixelFormat(pBufferConf->eColorFormat, pCtx->videoCtx.instInfo.HwVersion);
    fmt.width   = pBufferConf->nWidth;
    fmt.height  = pBufferConf->nHeight;
    fmt.stride  = pBufferConf->nStride;
    fmt.cstride = pBufferConf->nCStride;
    fmt.nPlane  = pBufferConf->nPlaneCnt;

    for (i = 0; i < (int)(sizeof(SBWC_LOSSY_TABLE)/sizeof(SBWC_LOSSY_TABLE[0])); i++) {
        if (pBufferConf->eColorFormat == SBWC_LOSSY_TABLE[i].eColorFormat)
            fmt.field |= (1 << SBWC_LOSSY_TABLE[i].nFormatFlag);
    }

    if (Codec_OSAL_TryFormat(pCtx, &fmt) != 0) {
        ALOGE("%s: Failed to TryFormat", __FUNCTION__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Try Geometry (Dst)
 */
static ExynosVideoErrorType MFC_Encoder_Try_Geometry_Outbuf(
    void                *pHandle,
    ExynosVideoGeometry *pBufferConf) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;

    CodecOSAL_Format fmt;

    if ((CHECK_POINTER(pCtx) == false) ||
        (CHECK_POINTER(pBufferConf) == false)) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&fmt, 0, sizeof(fmt));
    fmt.type            = CODEC_OSAL_BUF_TYPE_DST;
    fmt.format          = Codec_OSAL_CodingTypeToCompressdFormat(pBufferConf->eCompressionFormat);
    fmt.planeSize[0]    = pBufferConf->nSizeImage;
    fmt.nPlane          = pBufferConf->nPlaneCnt;
    fmt.width           = pBufferConf->nWidth;
    fmt.height          = pBufferConf->nHeight;

    if (Codec_OSAL_TryFormat(pCtx, &fmt) != 0) {
        ALOGE("%s: Failed to TryFormat", __FUNCTION__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Setup (Src)
 */
static ExynosVideoErrorType MFC_Encoder_Setup_Inbuf(
    void           *pHandle,
    unsigned int    nBufferCount) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo    *pSrcPad  = NULL;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;

    CodecOSAL_ReqBuf reqBuf;

    int i, j;

    if (CHECK_POINTER(pCtx) == false) {
        return VIDEO_ERROR_BADPARAM;
    }

    if (nBufferCount == 0) {
        nBufferCount = MAX_INPUTBUFFER_COUNT;
        ALOGV("%s: Change buffer count %d", __FUNCTION__, nBufferCount);
    }

    pSrcPad = &(pCtx->videoCtx.pad[VIDEO_INDEX_SRC_PAD]);

    memset(&reqBuf, 0, sizeof(reqBuf));

    reqBuf.type = CODEC_OSAL_BUF_TYPE_SRC;
    reqBuf.count = nBufferCount;
    reqBuf.memory = Codec_OSAL_VideoMemoryToSystemMemory(pCtx->videoCtx.instInfo.nMemoryType);

    if (Codec_OSAL_RequestBuf(pCtx, &reqBuf) != 0) {
        ALOGE("Failed to RequestBuf");
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pSrcPad->nBufNum = (int)reqBuf.count;

    memset(&(pSrcPad->slot), 0, sizeof(pSrcPad->slot));

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Setup (Dst)
 */
static ExynosVideoErrorType MFC_Encoder_Setup_Outbuf(
    void           *pHandle,
    unsigned int    nBufferCount) {
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

    reqBuf.type = CODEC_OSAL_BUF_TYPE_DST;
    reqBuf.count = nBufferCount;
    reqBuf.memory = Codec_OSAL_VideoMemoryToSystemMemory(pCtx->videoCtx.instInfo.nMemoryType);

    if (Codec_OSAL_RequestBuf(pCtx, &reqBuf) != 0) {
        ALOGE("%s: Failed to RequestBuf", __FUNCTION__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pDstPad->nBufNum = reqBuf.count;

    memset(&(pDstPad->slot), 0, sizeof(pDstPad->slot));

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Run (src)
 */
static ExynosVideoErrorType MFC_Encoder_Run_Inbuf(void *pHandle) {
    return MFC_Run_Inbuf(pHandle);
}

/*
 * [Encoder Buffer OPS] Run (Dst)
 */
static ExynosVideoErrorType MFC_Encoder_Run_Outbuf(void *pHandle) {
    return MFC_Run_Outbuf(pHandle);
}

/*
 * [Encoder Buffer OPS] Stop (Src)
 */
static ExynosVideoErrorType MFC_Encoder_Stop_Inbuf(void *pHandle) {
    return MFC_Stop_Inbuf(pHandle);
}

/*
 * [Encoder Buffer OPS] Stop (Dst)
 */
static ExynosVideoErrorType MFC_Encoder_Stop_Outbuf(void *pHandle) {
    return MFC_Stop_Outbuf(pHandle);
}

/*
 * [Encoder Buffer OPS] Find (Input)
 */
static int MFC_Encoder_Find_Inbuf(
    void *pHandle,
    void *pBuffer) {
    return MFC_Find_Inbuf(pHandle, pBuffer);
}

/*
 * [Encoder Buffer OPS] FindIndex (Input)
 */
static int MFC_Encoder_FindEmptySlot_Inbuf(void *pHandle) {
    return MFC_FindEmptySlot_Inbuf(pHandle);
}

/*
 * [Encoder Buffer OPS] FindIndex (Output)
 */
static int MFC_Encoder_FindEmptySlot_Outbuf(void *pHandle) {
    return MFC_FindEmptySlot_Outbuf(pHandle, true);
}

/*
 * [Encoder Buffer OPS] Enqueue (Input)
 */
static ExynosVideoErrorType MFC_Encoder_Enqueue_Inbuf(
    void                *pHandle,
    ExynosVideoBuffer   *pVideoBuffer) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo    *pSrcPad  = NULL;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;
    pthread_mutex_t       *pMutex   = NULL;

    CodecOSAL_Buffer buf;

    int index, i;

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

    ret = MFC_setupCodecOSALBuffer(pCtx, pVideoBuffer, pSrcPad, &buf, true);

    if (ret != VIDEO_ERROR_NONE) {
        pthread_mutex_unlock(pMutex);
        goto EXIT;
    }

    if ((pCtx->videoCtx.specificInfo.enc.bDisableDFR == VIDEO_FALSE) &&
        ((pCtx->videoCtx.instInfo.eCodecType == VIDEO_CODING_VP8) ||
         (pCtx->videoCtx.instInfo.eCodecType == VIDEO_CODING_VP9) ||
         (pCtx->videoCtx.instInfo.eCodecType == VIDEO_CODING_AVC) ||
         (pCtx->videoCtx.instInfo.eCodecType == VIDEO_CODING_HEVC))) {
        int     oldFrameRate   = 0;
        int     curFrameRate   = 0;
        int64_t curDuration    = 0;

        curDuration = (int64_t)(pVideoBuffer->extraInfo.nTimeStamp - pCtx->videoCtx.specificInfo.enc.oldTimeStamp);
        if ((curDuration > 0) && (pCtx->videoCtx.specificInfo.enc.oldDuration > 0)) {
            oldFrameRate = ROUND_UP(1E6 / pCtx->videoCtx.specificInfo.enc.oldDuration);
            curFrameRate = ROUND_UP(1E6 / curDuration);

            if (CHECK_FRAMERATE_VALIDITY(curFrameRate, oldFrameRate)) {
                if (Codec_OSAL_SetControl(pCtx, CODEC_OSAL_CID_ENC_FRAME_RATE, curFrameRate) != 0) {
                    ALOGE("%s: Failed to SetControl(oldts: %lld, curts: %lld, oldfps: %d, curfps: %d)", __FUNCTION__,
                            (long long)pCtx->videoCtx.specificInfo.enc.oldTimeStamp,
                            (long long)pVideoBuffer->extraInfo.nTimeStamp,
                            oldFrameRate,
                            curFrameRate);

                    pthread_mutex_unlock(pMutex);
                    ret = VIDEO_ERROR_APIFAIL;
                    goto EXIT;
                }
                pCtx->videoCtx.specificInfo.enc.oldFrameRate = curFrameRate;
            } else {
                ALOGV("%s: current ts: %lld, old ts: %lld, curFrameRate: %d, oldFrameRate: %d", __FUNCTION__,
                      (long long)pVideoBuffer->extraInfo.nTimeStamp,
                      (long long)pCtx->videoCtx.specificInfo.enc.oldTimeStamp,
                      curFrameRate, oldFrameRate);
            }
        }

        if (curDuration > 0)
            pCtx->videoCtx.specificInfo.enc.oldDuration = curDuration;
        pCtx->videoCtx.specificInfo.enc.oldTimeStamp = (int64_t)pVideoBuffer->extraInfo.nTimeStamp;
    }

    /* save HDR Dynamic Info to shared buffer instead of ioctl call */
    if (pCtx->videoCtx.instInfo.supportInfo.enc.bHDRDynamicInfoSupport == VIDEO_TRUE) {
        ExynosVideoHdrDynamic *pHdrDynamic = ((ExynosVideoHdrDynamic *)pCtx->videoCtx.specificInfo.enc.pHDRDynamicInfoAddr) + buf.index;
        memcpy(pHdrDynamic, &(pSrcPad->geometry.HdrInfo.sHdrDynamic), sizeof(ExynosVideoHdrDynamic));
    }

    pthread_mutex_unlock(pMutex);

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
 * [Encoder Buffer OPS] Enqueue (Output)
 */
static ExynosVideoErrorType MFC_Encoder_Enqueue_Outbuf(
    void                *pHandle,
    ExynosVideoBuffer   *pVideoBuffer) {
    CodecOSALVideoContext *pCtx     = (CodecOSALVideoContext *)pHandle;
    ExynosVideoPadInfo    *pDstPad  = NULL;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;
    pthread_mutex_t       *pMutex   = NULL;

    CodecOSAL_Buffer buf;

    int index, i;

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

    index = MFC_Encoder_FindEmptySlot_Outbuf(pCtx);
    if (index == -1) {
        pthread_mutex_unlock(pMutex);
        ALOGE("%s: Failed to get index", __FUNCTION__);
        ret = VIDEO_ERROR_NOBUFFERS;
        goto EXIT;
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
    pthread_mutex_unlock(pMutex);

    if (Codec_OSAL_EnqueueBuf(pCtx, &buf) != 0) {
        ALOGE("%s: Failed to enqueue output buffer", __FUNCTION__);
        pthread_mutex_lock(pMutex);
        memset(&(pDstPad->slot[buf.index].buffer), 0, sizeof(ExynosVideoBuffer));
        pDstPad->slot[buf.index].bQueued = VIDEO_FALSE;
        pthread_mutex_unlock(pMutex);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Dequeue (Input)
 */
static ExynosVideoErrorType MFC_Encoder_Dequeue_Inbuf(
    void              *pHandle,
    ExynosVideoBuffer *pVideoBuffer) {
    return MFC_Dequeue_Inbuf(pHandle, pVideoBuffer, true);
}

static ExynosVideoErrorType MFC_Encoder_StateProcessing(
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

    pOutbuf->extraInfo.specific.enc.frameType = buf->frameType;

    {
        int64_t sec  = (int64_t)(buf->timestamp.tv_sec * 1E6);
        int64_t usec = (int64_t)buf->timestamp.tv_usec;
        pOutbuf->extraInfo.nTimeStamp = sec + usec;
    }

    /* HDR Dynamic Stat Info is obtained from shared buffer instead of ioctl call */
    if (pCtx->videoCtx.specificInfo.enc.bHDR10PlusStatMode == VIDEO_TRUE) {
        ExynosVideoHdrDynamicStat *pHdrDynamicStat = ((ExynosVideoHdrDynamicStat *)pCtx->videoCtx.specificInfo.enc.pHDRDynamicStatInfoAddr)
                                                     + buf->index;

        memcpy((char *)(&(pDstPad->geometry.HdrDynamicStatInfo)),
               (char *)pHdrDynamicStat,
               sizeof(ExynosVideoHdrDynamicStat));

        pOutbuf->extraInfo.specific.enc.frameType |= VIDEO_FRAME_WITH_HDR_STAT_INFO;
    }

    memcpy(pVideoBuffer, pOutbuf, sizeof(ExynosVideoBuffer));
    pDstPad->slot[buf->index].bQueued = VIDEO_FALSE;

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Dequeue (Output)
 */
static ExynosVideoErrorType MFC_Encoder_Dequeue_Outbuf(
    void              *pHandle,
    ExynosVideoBuffer *pVideoBuffer) {
    return MFC_Common_Dequeue_Outbuf(pHandle, pVideoBuffer, &MFC_Encoder_StateProcessing);
}

/*
 * [Encoder Buffer OPS] Cleanup Buffer (Input)
 */
static ExynosVideoErrorType MFC_Encoder_Cleanup_Buffer_Inbuf(void *pHandle) {
    return MFC_Cleanup_Buffer_Inbuf(pHandle);
}

/*
 * [Encoder Buffer OPS] Cleanup Buffer (Output)
 */
static ExynosVideoErrorType MFC_Encoder_Cleanup_Buffer_Outbuf(void *pHandle) {
    return MFC_Cleanup_Buffer_Outbuf(pHandle);
}

#ifndef USE_EPOLL
/*
 * [Encoder OPS] Wait Buffer (Input)
 */
ExynosVideoErrorType MFC_Encoder_Wait_Buffer_Inbuf(
    void                *pHandle,
    ExynosVideoPollType *avail) {
    return MFC_Wait_Buffer_Common(pHandle, avail, true, InputPort);
}

/*
 * [Encoder OPS] Wait Buffer (Output)
 */
ExynosVideoErrorType MFC_Encoder_Wait_Buffer_Outbuf(
    void                *pHandle,
    ExynosVideoPollType *avail) {
    return MFC_Wait_Buffer_Common(pHandle, avail, true, OutputPort);
}
#else
/*
 * [Decoder OPS] Wait Buffer use epoll (Input)
 */
ExynosVideoErrorType MFC_Encoder_Wait_Buffer_Inbuf_Epoll(
    void                *pHandle,
    ExynosVideoPollType *avail) {
    return MFC_Wait_Buffer_Common_Epoll(pHandle, avail, InputPort);
}

/*
 * [Decoder OPS] Wait Buffer use epoll (Output)
 */
ExynosVideoErrorType MFC_Encoder_Wait_Buffer_Outbuf_Epoll(
    void                *pHandle,
    ExynosVideoPollType *avail) {
    return MFC_Wait_Buffer_Common_Epoll(pHandle, avail, OutputPort);
}
#endif

/*
 * [Encoder OPS] Common
 */
static ExynosVideoEncOps defEncOps = {
    .Init                        = MFC_Encoder_Init,
    .Finalize                    = MFC_Encoder_Finalize,

    .Set_FrameTag                = MFC_Encoder_Set_FrameTag,
    .Get_FrameTag                = MFC_Encoder_Get_FrameTag,

    .Set_EncParam                = MFC_Encoder_Set_EncParam,

    .Set_SliceMode               = MFC_Encoder_Set_SliceMode,
    .Enable_PrependSpsPpsToIdr   = MFC_Encoder_Enable_PrependSpsPpsToIdr,

    .Set_FrameType               = MFC_Encoder_Set_FrameType,
    .Set_FrameSkip               = MFC_Encoder_Set_FrameSkip,

    .Set_FrameRate               = MFC_Encoder_Set_FrameRate,
    .Set_BitRate                 = MFC_Encoder_Set_BitRate,
    .Set_QpRange                 = MFC_Encoder_Set_QuantizationRange,
    .Set_IDRPeriod               = MFC_Encoder_Set_IDRPeriod,

    .Set_QosRatio                = MFC_Encoder_Set_QosRatio,

    .Set_DynamicQpControl        = MFC_Encoder_Set_DynamicQpControl,
    .Set_MarkLTRFrame            = MFC_Encoder_Set_MarkLTRFrame,
    .Set_UsedLTRFrame            = MFC_Encoder_Set_UsedLTRFrame,
    .Set_BasePID                 = MFC_Encoder_Set_BasePID,

    .Set_LayerChange             = MFC_Encoder_Set_LayerChange,
    .Enable_AdaptiveLayerBitrate = MFC_Encoder_Enable_AdaptiveLayerBitrate,

    .Set_ColorAspects            = MFC_Encoder_Set_ColorAspects,
    .Set_HDRStaticInfo           = MFC_Encoder_Set_HDRStaticInfo,
    .Set_HDRDynamicInfo          = MFC_Encoder_Set_HDRDynamicInfo,

    .Set_RoiInfo                 = MFC_Encoder_Set_RoiInfo,

    .Enable_WeightedPrediction   = MFC_Encoder_Enable_WeightedPrediction,
    .Set_YSumData                = MFC_Encoder_Set_YSumData,

    .Set_IFrameRatio             = MFC_Encoder_Set_IFrameRatio,

    .Set_HeaderMode              = MFC_Encoder_Set_HeaderMode,

    .Set_DropControl             = MFC_Encoder_Set_DropControl,

    .Disable_DynamicFrameRate    = MFC_Encoder_Disable_DynamicFrameRate,

    .Set_ActualFormat            = MFC_Encoder_Set_ActualFormat,

    .Enable_GDCvOTF              = MFC_Encoder_Enable_GDCvOTF,
    .Set_GDCvOTF                 = MFC_Encoder_Set_GDCvOTF,

    .Set_OperatingRate           = MFC_Encoder_Set_OperatingRate,

    .Set_RealTimePriority        = MFC_Encoder_Set_RealTimePriority,
    .Get_RealTimePriority        = MFC_Encoder_Get_RealTimePriority,

    .Get_AverageQp               = MFC_Encoder_Get_AverageQp,

    .Set_PMV                     = MFC_Encoder_Set_PMV,

    .Set_GopMode                 = MFC_Encoder_Set_GopMode,

    .Set_MinQuality              = MFC_Encoder_Set_MinQuality,

    .Enable_HDRDynamicStatInfo   = MFC_Encoder_Enable_HDRDynamicStatInfo,
    .Get_HDRDynamicStatInfo      = MFC_Encoder_Get_HDRDynamicStatInfo,

    .Set_Crop                    = MFC_Encoder_Set_Crop,

    .Set_MaxIFrameSize           = MFC_Encoder_Set_MaxIFrameSize,

    .Disable_LazyUnmap           = MFC_Encoder_Disable_LazyUnmap,

    .Stop_Wait_Buffer            = MFC_Encoder_Stop_Wait_Buffer,
    .Reset_Wait_Buffer           = MFC_Encoder_Reset_Wait_Buffer,
};

/*
 * [Encoder Buffer OPS] Input
 */
static ExynosVideoBufferOps defInbufOps = {
    .Enable_Cacheable       = MFC_Encoder_Enable_Cacheable_Inbuf,
    .Set_Geometry           = MFC_Encoder_Set_Geometry_Inbuf,
    .Get_Geometry           = MFC_Encoder_Get_Geometry_Inbuf,
    .Try_Geometry           = MFC_Encoder_Try_Geometry_Inbuf,
    .Setup                  = MFC_Encoder_Setup_Inbuf,
    .Run                    = MFC_Encoder_Run_Inbuf,
    .Stop                   = MFC_Encoder_Stop_Inbuf,
    .Enqueue                = MFC_Encoder_Enqueue_Inbuf,
    .Dequeue                = MFC_Encoder_Dequeue_Inbuf,
    .Cleanup_Buffer         = MFC_Encoder_Cleanup_Buffer_Inbuf,
#ifdef USE_EPOLL
    .Wait_Buffer            = MFC_Encoder_Wait_Buffer_Inbuf_Epoll,
#else
    .Wait_Buffer            = MFC_Encoder_Wait_Buffer_Inbuf,
#endif
};

/*
 * [Encoder Buffer OPS] Output
 */
static ExynosVideoBufferOps defOutbufOps = {
    .Enable_Cacheable       = MFC_Encoder_Enable_Cacheable_Outbuf,
    .Set_Geometry           = MFC_Encoder_Set_Geometry_Outbuf,
    .Get_Geometry           = MFC_Encoder_Get_Geometry_Outbuf,
    .Try_Geometry           = MFC_Encoder_Try_Geometry_Outbuf,
    .Setup                  = MFC_Encoder_Setup_Outbuf,
    .Run                    = MFC_Encoder_Run_Outbuf,
    .Stop                   = MFC_Encoder_Stop_Outbuf,
    .Enqueue                = MFC_Encoder_Enqueue_Outbuf,
    .Dequeue                = MFC_Encoder_Dequeue_Outbuf,
    .Cleanup_Buffer         = MFC_Encoder_Cleanup_Buffer_Outbuf,
#ifdef USE_EPOLL
    .Wait_Buffer            = MFC_Encoder_Wait_Buffer_Outbuf_Epoll,
#else
    .Wait_Buffer            = MFC_Encoder_Wait_Buffer_Outbuf,
#endif
};

ExynosVideoErrorType MFC_Exynos_Video_GetInstInfo_Encoder(
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
        codecRet = Codec_OSAL_DevOpen(VIDEO_HEVC_ENCODER_NAME, O_RDWR, &videoCtx);
    else
#endif
        if (pVideoInstInfo->bOTFMode == VIDEO_TRUE)
            codecRet = Codec_OSAL_DevOpen(VIDEO_MFC_OTF_ENCODER_NAME, O_RDWR, &videoCtx);
        else
            codecRet = Codec_OSAL_DevOpen(VIDEO_MFC_ENCODER_NAME, O_RDWR, &videoCtx);

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

ExynosVideoErrorType MFC_Exynos_Video_Register_Encoder(
    ExynosVideoEncOps    *pEncOps,
    ExynosVideoBufferOps *pInbufOps,
    ExynosVideoBufferOps *pOutbufOps) {
    ExynosVideoErrorType ret = VIDEO_ERROR_NONE;

    if ((pEncOps == NULL) || (pInbufOps == NULL) || (pOutbufOps == NULL)) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memcpy((char *)pEncOps, (char *)&defEncOps, sizeof(defEncOps));
    memcpy((char *)pInbufOps, (char *)&defInbufOps, sizeof(defInbufOps));
    memcpy((char *)pOutbufOps, (char *)&defOutbufOps, sizeof(defOutbufOps));

EXIT:
    return ret;
}
