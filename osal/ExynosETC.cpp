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
#include <media/stagefright/foundation/ColorUtils.h>
#include <cutils/properties.h>
#include <system/graphics.h>
#include "exynos_format.h"

#include "ExynosBuffer.h"
#include "ExynosDef.h"
#include "C2ExynosParam.h"

#include "ExynosETC.h"

#undef LOG_TAG
#define LOG_ON
#include "ExynosLog.h"
#define LOG_TAG "ExynosUtils"

#define MIN_CPB_SIZE (1024 * 1024)
#define LOW_OUTPUT_SIZE(w, h) (w * h * 3 / 2)
#define LOW_OUTPUT_UPPTER_LIMIT (3840 * 2160 * 3 / 4)
#define MIDDLE_OUTPUT_BASELINE (3840 * 2160)
#define MIDDLE_OUTPUT_SIZE(w, h) (w * h * 3 / 4)
#define MIDDLE_OUTPUT_UPPER_LIMIT (7680 * 4320 * 3 / 8)
#define HIGH_OUTPUT_BASELINE (7680 * 4320)
#define MAX_OUTPUT_SIZE(w, h) (w * h * 3 / 8)
#define LIMITED_SECURE_OUTPUT_SIZE (1024 * 1024 * 3) /* 3MB : For projects which has small vstream heap */

using namespace android;

inline void initColorAspects(ColorAspects &aspects) {
    aspects.mRange          = ColorAspects::RangeUnspecified;
    aspects.mPrimaries      = ColorAspects::PrimariesUnspecified;
    aspects.mMatrixCoeffs   = ColorAspects::MatrixUnspecified;
    aspects.mTransfer       = ColorAspects::TransferUnspecified;
    return;
}

bool GetYUVInfo(
    int width, int height, int format,
    int &cnt, unsigned int size[BASE_BUFFER_MAX_PLANES],
    uint64_t flags = 0) {
    int WIDTH_ALIGN = (flags & ExynosBuffer::GPU_TEXTURE)? HW_GPU_ALIGN:HW_WIDTH_ALIGN;

    switch (format) {
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_SBWC:
    {
        cnt     = 2;
        size[0] = (SBWC_8B_Y_SIZE(width, height) +           /* SBWC Y */
                   SBWC_8B_Y_HEADER_SIZE(width, height));    /* SBWC Y HEADER */
        size[1] = (SBWC_8B_CBCR_SIZE(width, height) +        /* SBWC CBCR */
                   SBWC_8B_CBCR_HEADER_SIZE(width, height)); /* SBWC CBCR HEADER */
    }
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L50:
    {
        cnt     = 2;
        size[0] = SBWCL_8B_Y_SIZE(width, height, 50);    /* SBWC_L50 Y */
        size[1] = SBWCL_8B_CBCR_SIZE(width, height, 50); /* SBWC_L50 CBCR */
    }
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L75:
    {
        cnt     = 2;
        size[0] = SBWCL_8B_Y_SIZE(width, height, 75);    /* SBWC_L50 Y */
        size[1] = SBWCL_8B_CBCR_SIZE(width, height, 75); /* SBWC_L50 CBCR */
    }
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_32_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_10B_32_SBWC_L:
    {
        cnt     = 2;
        size[0] = SBWCL_32_Y_SIZE(width, height) + SBWCL_Y_HEADER_SIZE(width, height);    /* SBWC Y : stride(32B) */
        size[1] = SBWCL_32_CBCR_SIZE(width, height) + SBWCL_CBCR_HEADER_SIZE(width, height);  /* SBWC CBCR : stride(32B) */
    }
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_64_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_10B_64_SBWC_L:
    {
        cnt     = 2;
        size[0] = SBWCL_64_Y_SIZE(width, height) + SBWCL_Y_HEADER_SIZE(width, height);    /* SBWC Y : stride(64B) */
        size[1] = SBWCL_64_CBCR_SIZE(width, height) + SBWCL_CBCR_HEADER_SIZE(width, height);  /* SBWC CBCR : stride(64B) */
    }
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC:
    {
        cnt     = 2;
        size[0] = (SBWC_10B_Y_SIZE(width, height) +           /* SBWC Y */
                   SBWC_10B_Y_HEADER_SIZE(width, height));    /* SBWC Y HEADER */
        size[1] = (SBWC_10B_CBCR_SIZE(width, height) +        /* SBWC CBCR */
                   SBWC_10B_CBCR_HEADER_SIZE(width, height)); /* SBWC CBCR HEADER */
    }
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L40:
    {
        cnt     = 2;
        size[0] = SBWCL_10B_Y_SIZE(width, height, 40);    /* SBWC_L40 Y */
        size[1] = SBWCL_10B_CBCR_SIZE(width, height, 40); /* SBWC_L40 CBCR */
    }
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L60:
    {
        cnt     = 2;
        size[0] = SBWCL_10B_Y_SIZE(width, height, 60);    /* SBWC_L40 Y */
        size[1] = SBWCL_10B_CBCR_SIZE(width, height, 60); /* SBWC_L40 CBCR */
    }
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L80:
    {
        cnt     = 2;
        size[0] = SBWCL_10B_Y_SIZE(width, height, 80);    /* SBWC_L40 Y */
        size[1] = SBWCL_10B_CBCR_SIZE(width, height, 80); /* SBWC_L40 CBCR */
    }
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_PN:
    {
        cnt     = 1;
        size[0] = (ALIGN(width, WIDTH_ALIGN) * ALIGN(height, HW_HEIGHT_ALIGN) + 256) +
                    (ALIGN((ALIGN(width / 2, WIDTH_ALIGN) * ALIGN((ALIGN(height, HW_HEIGHT_ALIGN) / 2),
                      HW_CHROMA_VALIGN) + 256), HW_HEIGHT_ALIGN) * 2);
    }
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_420_SPN_SBWC_DECOMP:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC:
    {
        cnt     = 1;
        size[0] = (SBWC_8B_Y_SIZE(width, height) +           /* SBWC Y */
                   SBWC_8B_Y_HEADER_SIZE(width, height)) +   /* SBWC Y HEADER */
                  (SBWC_8B_CBCR_SIZE(width, height) +        /* SBWC CBCR */
                   SBWC_8B_CBCR_HEADER_SIZE(width, height)); /* SBWC CBCR HEADER */
    }
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_256_SBWC:
    {
        cnt     = 1;
        size[0] = (SBWC_256_8B_Y_SIZE(width, height) +           /* SBWC Y */
                   SBWC_256_8B_Y_HEADER_SIZE(width, height)) +   /* SBWC Y HEADER */
                  (SBWC_256_8B_CBCR_SIZE(width, height) +        /* SBWC CBCR */
                   SBWC_256_8B_CBCR_HEADER_SIZE(width, height)); /* SBWC CBCR HEADER */
    }
        break;
#if 0  /* TBD */
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC_L50:
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC_L75:
        break;
#endif
    case HAL_PIXEL_FORMAT_EXYNOS_420_SPN_32_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SPN_10B_32_SBWC_L:
    {
        cnt     = 1;
        size[0] = SBWCL_32_Y_SIZE(width, height) + SBWCL_Y_HEADER_SIZE(width, height) +  /* SBWC Y : stride(32B) */
                  SBWCL_32_CBCR_SIZE(width, height) + SBWCL_CBCR_HEADER_SIZE(width, height);  /* SBWC CBCR : stride(32B) */
    }
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_420_SPN_64_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SPN_10B_64_SBWC_L:
    {
        cnt     = 1;
        size[0] = SBWCL_64_Y_SIZE(width, height) + SBWCL_Y_HEADER_SIZE(width, height) +  /* SBWC Y : stride(64B) */
                  SBWCL_64_CBCR_SIZE(width, height) + SBWCL_CBCR_HEADER_SIZE(width, height);  /* SBWC CBCR : stride(64B) */
    }
        break;
/* h/w format - secure 10bit */
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_S10B:
    {
        size[0] = ((ALIGN(width, HW_S10B_FORMAT_8B_ALIGNMENT) * ALIGN(height, HW_HEIGHT_ALIGN) + 256) +  /* Y 8bit */
                   (ALIGN(width / 4, WIDTH_ALIGN) * ALIGN(height, HW_HEIGHT_ALIGN) + 64)) +           /* Y 2bit */
                   ((ALIGN((ALIGN(width, HW_S10B_FORMAT_8B_ALIGNMENT) * ALIGN((ALIGN(height, HW_HEIGHT_ALIGN) / 2),
                      HW_CHROMA_VALIGN) + 256), HW_HEIGHT_ALIGN)) +                                                           /* CbCr 8bit */
                   (ALIGN(width / 4, WIDTH_ALIGN) * ALIGN((ALIGN(height, HW_HEIGHT_ALIGN) / 2), HW_CHROMA_VALIGN) + 64));  /* CbCr 2bit */
    }
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_P010_N_SBWC_DECOMP:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC:
    {
        cnt     = 1;
        size[0] = (SBWC_10B_Y_SIZE(width, height) +           /* SBWC Y */
                   SBWC_10B_Y_HEADER_SIZE(width, height)) +   /* SBWC Y HEADER */
                  (SBWC_10B_CBCR_SIZE(width, height) +        /* SBWC CBCR */
                   SBWC_10B_CBCR_HEADER_SIZE(width, height)); /* SBWC CBCR HEADER */
    }
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_256_SBWC:
    {
        cnt     = 1;
        size[0] = (SBWC_256_10B_Y_SIZE(width, height) +           /* SBWC Y */
                   SBWC_256_10B_Y_HEADER_SIZE(width, height)) +   /* SBWC Y HEADER */
                  (SBWC_256_10B_CBCR_SIZE(width, height) +        /* SBWC CBCR */
                   SBWC_256_10B_CBCR_HEADER_SIZE(width, height)); /* SBWC CBCR HEADER */
    }
        break;
#if 0  /* TBD */
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L40:
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L60:
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L80:
        break;
#endif
    default:
        return false;
        break;
    }
    return true;
}

void ExynosUtils::GetYUVPlaneInfo(
    int width, int height, int format,
    int &cnt, unsigned int size[BASE_BUFFER_MAX_PLANES],
    uint64_t flags) {
    if (true == GetYUVInfo(width, height, format, cnt, size, flags))
        return;

    int WIDTH_ALIGN = (flags & ExynosBuffer::GPU_TEXTURE)? HW_GPU_ALIGN:HW_WIDTH_ALIGN;

    switch (format) {
/* h/w format - normal 8bit */
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:
    {
        cnt     = 2;
        size[0] = (ALIGN(width, WIDTH_ALIGN) * ALIGN(height, HW_HEIGHT_ALIGN))
                    + HW_EXTRA_BYTES;
        size[1] = (ALIGN(width, WIDTH_ALIGN) * ALIGN(ALIGN(height, HW_HEIGHT_ALIGN) / 2, HW_CHROMA_VALIGN))
                    + HW_EXTRA_BYTES;
    }
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
    {
        int stride  = ALIGN(width, WIDTH_ALIGN);
        cnt         = 3;
        size[0]     = (stride * ALIGN(height, HW_HEIGHT_ALIGN))
                        + HW_EXTRA_BYTES;
        size[1]     = (ALIGN(stride / 2, WIDTH_ALIGN) * ALIGN(ALIGN(height, HW_HEIGHT_ALIGN) / 2, HW_CHROMA_VALIGN))
                        + HW_EXTRA_BYTES;
        size[2]     = size[1];
    }
        break;
/* h/w format - normal 10bit */
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M:
    {
        cnt     = 2;
        size[0] = (ALIGN(ALIGN(width * 2, WIDTH_ALIGN) * ALIGN(height, HW_HEIGHT_ALIGN), 256))
                    + HW_EXTRA_BYTES;
        size[1] = (ALIGN(ALIGN(width * 2, WIDTH_ALIGN) * ALIGN((ALIGN(height, HW_HEIGHT_ALIGN) / 2), HW_CHROMA_VALIGN), 256))
                    + HW_EXTRA_BYTES;
    }
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B:
    {
        cnt     = 2;
        size[0] = ALIGN((ALIGN(width, HW_S10B_FORMAT_8B_ALIGNMENT) * ALIGN(height, HW_HEIGHT_ALIGN) + 256) +  /* Y 8bit */
                        (ALIGN(width / 4, WIDTH_ALIGN) * ALIGN(height, HW_HEIGHT_ALIGN) + 256), 256);      /* Y 2bit */
        size[1] = ALIGN((ALIGN(width, HW_S10B_FORMAT_8B_ALIGNMENT) * ALIGN((ALIGN(height, HW_HEIGHT_ALIGN) / 2), HW_CHROMA_VALIGN) + 256) +  /* CbCr 8bit */
                        (ALIGN(width / 4, WIDTH_ALIGN) * ALIGN((ALIGN(height, HW_HEIGHT_ALIGN) / 2), HW_CHROMA_VALIGN) + 256), 256);      /* CbCr 2bit */
    }
        break;
/* h/w format - 8bit */
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN:
    {
        cnt     = 1;
        size[0] = (ALIGN(width, WIDTH_ALIGN) * ALIGN(height, HW_HEIGHT_ALIGN)) +
                  (ALIGN((ALIGN(width, WIDTH_ALIGN) * ALIGN((ALIGN(height, HW_HEIGHT_ALIGN) / 2),
                    HW_CHROMA_VALIGN)), HW_HEIGHT_ALIGN));
    }
        break;
/* user format */
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    {
        int stride  = ALIGN(width, 2);
        cnt         = 1;  /* it doesn't have private data buffer */
        size[0]     = ((stride * height) + (stride * height / 2))
                        + HW_EXTRA_BYTES;
    }
        break;
    case HAL_PIXEL_FORMAT_YV12:
    {
#ifdef YV12_ALIGN
        if (flags & ExynosBuffer::GPU_TEXTURE) {
            WIDTH_ALIGN = YV12_ALIGN;
        }
#endif
        int stride  = ALIGN(width, WIDTH_ALIGN);
        cnt         = 1;  /* it doesn't have private data buffer */
        size[0]     = ((stride * height) + (ALIGN(stride / 2, 16) * height))
                        + HW_EXTRA_BYTES;
    }
        break;
    case HAL_PIXEL_FORMAT_YCBCR_P010:
    {
        int stride  = ALIGN(width * 2, WIDTH_ALIGN);
        cnt     = 1;
        size[0] = (stride * ALIGN(height, HW_HEIGHT_ALIGN)) +
                  (ALIGN(ALIGN((stride * (ALIGN(height, HW_HEIGHT_ALIGN) / 2)),
                    HW_CHROMA_VALIGN), HW_HEIGHT_ALIGN));
    }
        break;
    case HAL_PIXEL_FORMAT_RGBA_1010102:
      [[fallthrough]];
    case HAL_PIXEL_FORMAT_RGBA_8888:
      [[fallthrough]];
    case HAL_PIXEL_FORMAT_BGRA_8888:
      [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_ARGB_8888:
    {
        cnt     = 1;  /* it doesn't have private data buffer */
        size[0] = (width * height * 4)
                    + HW_EXTRA_BYTES;
        break;
    }
    default:
    {
        cnt     = 2;
        size[0] = (ALIGN(width, WIDTH_ALIGN) * ALIGN(height, HW_HEIGHT_ALIGN))
                    + HW_EXTRA_BYTES;
        size[1] = (ALIGN(width, WIDTH_ALIGN) * ALIGN(height / 2, HW_HEIGHT_ALIGN))
                    + HW_EXTRA_BYTES;
    }
        break;
    }
}

void ExynosUtils::GetYUVImageInfo(
    int width, int height, int format,
    int &cnt, unsigned int size[BASE_BUFFER_MAX_PLANES],
    uint64_t flags) {
    if (true == GetYUVInfo(width, height, format, cnt, size, flags))
        return;

    int WIDTH_ALIGN = (flags & ExynosBuffer::GPU_TEXTURE)? HW_GPU_ALIGN:HW_WIDTH_ALIGN;

    switch (format) {
/* h/w format - normal 8bit */
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:
    {
        cnt     = 2;
        size[0] = ALIGN(width, WIDTH_ALIGN) * height;
        size[1] = ALIGN(width, WIDTH_ALIGN) * height / 2;
    }
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
    {
        int stride  = ALIGN(width, WIDTH_ALIGN);
        cnt         = 3;
        size[0]     = stride * height;
        size[1]     = ALIGN(stride / 2, WIDTH_ALIGN) * height / 2;
        size[2]     = size[1];
    }
        break;
/* h/w format - normal 10bit */
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M:
    {
        cnt     = 2;
        size[0] = ALIGN(width * 2, WIDTH_ALIGN) * height;
        size[1] = ALIGN(width * 2, WIDTH_ALIGN) * height / 2;
    }
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B:
    {
        cnt     = 2;
        /* discard 2bit data */
        size[0] = ALIGN(width, WIDTH_ALIGN) * height;
        size[1] = ALIGN(width, WIDTH_ALIGN) * height / 2;
    }
        break;
/* h/w format - 8bit */
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN:
    {
        cnt     = 1;
        size[0] = (ALIGN(width, WIDTH_ALIGN) * ALIGN(height, HW_HEIGHT_ALIGN)) +
                  (ALIGN(width, WIDTH_ALIGN) * ALIGN(height, HW_HEIGHT_ALIGN) / 2);
    }
        break;
/* user format */
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    {
        int stride  = ALIGN(width, 2);
        cnt         = 1;  /* it doesn't have private data buffer */
        size[0]     = ((stride * height) + (stride * height / 2));
    }
        break;
    case HAL_PIXEL_FORMAT_YV12:
    {
#ifdef YV12_ALIGN
        if (flags & ExynosBuffer::GPU_TEXTURE) {
            WIDTH_ALIGN = YV12_ALIGN;
        }
#endif
        int stride  = ALIGN(width, WIDTH_ALIGN);
        cnt         = 1;  /* it doesn't have private data buffer */
        size[0]     = (stride * height) + (ALIGN(stride / 2, 16) * height);
    }
        break;
    case HAL_PIXEL_FORMAT_YCBCR_P010:
    {
        int stride  = ALIGN(width * 2, WIDTH_ALIGN);
        cnt     = 1;
        size[0] = (stride * ALIGN(height, HW_HEIGHT_ALIGN)) + (stride * ALIGN(height, HW_HEIGHT_ALIGN) / 2);
    }
        break;
    case HAL_PIXEL_FORMAT_RGBA_1010102:
      [[fallthrough]];
    case HAL_PIXEL_FORMAT_RGBA_8888:
      [[fallthrough]];
    case HAL_PIXEL_FORMAT_BGRA_8888:
      [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_ARGB_8888:
    {
        cnt     = 1;  /* it doesn't have private data buffer */
        size[0] = width * height * 4;
    }
      break;
    default:
    {
        cnt     = 2;
        size[0] = ALIGN(width, WIDTH_ALIGN) * height;
        size[1] = ALIGN(width, WIDTH_ALIGN) * height / 2;
    }
        break;
    }
}

int ExynosUtils::GetPlaneCnt(int format) {
    int cnt = 0;

    switch (format) {
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_SBWC:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L50:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L75:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_32_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_10B_32_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_64_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_10B_64_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L40:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L60:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L80:
    {
        cnt = 2;
    }
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
    {
        cnt = 3;
    }
        break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_PN:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_S10B:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SPN_SBWC_DECOMP:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_256_SBWC:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_P010_N_SBWC_DECOMP:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_256_SBWC:
        [[fallthrough]];
#if 0  /* TBD */
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L40:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L60:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L80:
        [[fallthrough]];
#endif
    case HAL_PIXEL_FORMAT_EXYNOS_420_SPN_32_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SPN_10B_32_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SPN_64_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SPN_10B_64_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_YV12:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_YCBCR_P010:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_RGBA_1010102:
      [[fallthrough]];
    case HAL_PIXEL_FORMAT_RGBA_8888:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_BGRA_8888:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_ARGB_8888:
    {
        cnt = 1;
    }
        break;
    default:
        break;
    }

    return cnt;
}

int ExynosUtils::GetImagePlaneCnt(int format) {
    int cnt = 0;

    switch (format) {
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_PN:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_YV12:
    {
        cnt = 3;
    }
        break;
    case HAL_PIXEL_FORMAT_RGBA_1010102:
      [[fallthrough]];
    case HAL_PIXEL_FORMAT_RGBA_8888:
      [[fallthrough]];
    case HAL_PIXEL_FORMAT_BGRA_8888:
      [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_ARGB_8888:
    {
        cnt = 1;
    }
        break;
    default:
        cnt = 2;
        break;
    }

    return cnt;
}

bool ExynosUtils::CheckRGB(const int format) {
    bool ret = false;

    switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_1010102:
      [[fallthrough]];
    case HAL_PIXEL_FORMAT_RGBA_8888:
      [[fallthrough]];
    case HAL_PIXEL_FORMAT_BGRA_8888:
      [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_ARGB_8888:
        ret = true;
        break;
    default:
        break;
    }

    return ret;
}

bool ExynosUtils::CheckFullRange(const int dataspace)
{
    bool ret = false;

    if ((dataspace & HAL_DATASPACE_RANGE_MASK) != 0) {
        ret = ((dataspace & HAL_DATASPACE_RANGE_MASK) == HAL_DATASPACE_RANGE_FULL);
    } else {
        switch (dataspace) {
        case HAL_DATASPACE_SRGB_LINEAR:  /* BT.709 | LINEAR | FULL */
            [[fallthrough]];
        case HAL_DATASPACE_SRGB:  /* BT.709 | SRGB | FULL */
            [[fallthrough]];
        case HAL_DATASPACE_JFIF:  /* BT.601_625 | SMPTE170M | FULL */
            ret = true;
            break;
        default:
            ret = false;
            break;
        }
    }

    return ret;
}

bool ExynosUtils::CheckBT601(const int dataspace) {
    bool ret = false;

    switch (dataspace & HAL_DATASPACE_STANDARD_MASK) {
    case HAL_DATASPACE_STANDARD_BT601_625:
        [[fallthrough]];
    case HAL_DATASPACE_STANDARD_BT601_625_UNADJUSTED:
        [[fallthrough]];
    case HAL_DATASPACE_STANDARD_BT601_525:
        [[fallthrough]];
    case HAL_DATASPACE_STANDARD_BT601_525_UNADJUSTED:
        ret = true;
        break;
    default:
        /* it could be legacy dataspace */
        if ((dataspace == HAL_DATASPACE_BT601_625) ||
            (dataspace == HAL_DATASPACE_BT601_525)) {
            ret = true;
        }
        break;
    }

    return ret;
}

bool ExynosUtils::CheckBT709(const int dataspace) {
    bool ret = false;

    if (((dataspace & HAL_DATASPACE_STANDARD_MASK) == HAL_DATASPACE_STANDARD_BT709) ||
        (dataspace == HAL_DATASPACE_BT709)) {
        ret = true;
    }

    return ret;
}

bool ExynosUtils::CheckBT2020(const int dataspace) {
    bool ret = false;

    int standard = dataspace & HAL_DATASPACE_STANDARD_MASK;

    if ((standard == HAL_DATASPACE_STANDARD_BT2020) ||
        (standard == HAL_DATASPACE_STANDARD_BT2020_CONSTANT_LUMINANCE)) {
        ret = true;
    }

    return ret;
}

bool ExynosUtils::CheckCompressedFormat(const int format) {
    bool ret = false;

    switch (format) {
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC:
      [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_SBWC:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L50:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L75:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_32_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_10B_32_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_64_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_10B_64_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L40:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L60:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L80:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SPN_32_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SPN_10B_32_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SPN_64_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_420_SPN_10B_64_SBWC_L:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_256_SBWC:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_256_SBWC:
        ret = true;
        break;
    default:
        break;
    }

    return ret;
}

bool ExynosUtils::Check10BitFormat(const int format) {
    bool ret = false;

    switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_1010102:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_YCBCR_P010:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L40:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L60:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L80:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_S10B:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_256_SBWC:
        [[fallthrough]];
    case HAL_PIXEL_FORMAT_EXYNOS_P010_N_SBWC_DECOMP:
        ret = true;
        break;
    default:
        break;
    }

    return ret;
}

int ExynosUtils::GetSupportedDataspaceOnGPU() {
    static int dataspace = HAL_DATASPACE_UNKNOWN;

    if (dataspace == HAL_DATASPACE_UNKNOWN) {
        int val = property_get_int32("ro.vendor.gpu.dataspace", 0);

        switch (val) {
        case 1:  /* BT.709 */
            dataspace = HAL_DATASPACE_BT709;
            break;
        case 2:  /* BT.2020 */
            dataspace = HAL_DATASPACE_BT2020;
            break;
        case 0:  /* BT.601 */
            [[fallthrough]];
        default:
            dataspace = HAL_DATASPACE_BT601_625;
        }
    }

    return dataspace;
}

void ExynosUtils::GetColorAspectsFromDataspace(
    int         &range,
    int         &primaries,
    int         &transfer,
    int         &matrix,
    const int    dataspace) {
    auto legacy = ((dataspace & ~(HAL_DATASPACE_TRANSFER_MASK | HAL_DATASPACE_STANDARD_MASK |
                                  HAL_DATASPACE_RANGE_MASK)) != 0);
    if (legacy) {
        switch (dataspace) {
        case HAL_DATASPACE_SRGB_LINEAR:
            range       = ExynosRangeType::RANGE_FULL;
            primaries   = ExynosPrimariesType::PRIMARIES_BT709_5;
            transfer    = ExynosTransferType::TRANSFER_LINEAR;
            matrix      = ExynosMatrixCoeffType::MATRIX_COEFF_REC709;
            break;
        case HAL_DATASPACE_SRGB:
            range       = ExynosRangeType::RANGE_FULL;
            primaries   = ExynosPrimariesType::PRIMARIES_BT709_5;
            transfer    = ExynosTransferType::TRANSFER_SRGB;
            matrix      = ExynosMatrixCoeffType::MATRIX_COEFF_REC709;
            break;
        case HAL_DATASPACE_JFIF:
            range       = ExynosRangeType::RANGE_FULL;
            primaries   = ExynosPrimariesType::PRIMARIES_BT601_6_625;
            transfer    = ExynosTransferType::TRANSFER_SMPTE_170M;
            matrix      = ExynosMatrixCoeffType::MATRIX_COEFF_SMPTE170M;
            break;
        case HAL_DATASPACE_BT601_625:
            range       = ExynosRangeType::RANGE_LIMITED;
            primaries   = ExynosPrimariesType::PRIMARIES_BT601_6_625;
            transfer    = ExynosTransferType::TRANSFER_SMPTE_170M;
            matrix      = ExynosMatrixCoeffType::MATRIX_COEFF_SMPTE170M;
            break;
        case HAL_DATASPACE_BT601_525:
            range       = ExynosRangeType::RANGE_LIMITED;
            primaries   = ExynosPrimariesType::PRIMARIES_BT601_6_525;
            transfer    = ExynosTransferType::TRANSFER_SMPTE_170M;
            matrix      = ExynosMatrixCoeffType::MATRIX_COEFF_SMPTE170M;
            break;
        case HAL_DATASPACE_BT709:
            range       = ExynosRangeType::RANGE_LIMITED;
            primaries   = ExynosPrimariesType::PRIMARIES_BT709_5;
            transfer    = ExynosTransferType::TRANSFER_SMPTE_170M;
            matrix      = ExynosMatrixCoeffType::MATRIX_COEFF_REC709;
            break;
        default:
            break;
        }
    } else {
        switch (dataspace & HAL_DATASPACE_RANGE_MASK) {
        case HAL_DATASPACE_RANGE_FULL:
            range = ExynosRangeType::RANGE_FULL;
            break;
        case HAL_DATASPACE_RANGE_LIMITED:
            range = ExynosRangeType::RANGE_LIMITED;
            break;
        default:
            range = ExynosRangeType::RANGE_UNSPECIFIED;
            break;
        }

        switch (dataspace & HAL_DATASPACE_STANDARD_MASK) {
        case HAL_DATASPACE_STANDARD_BT709:
            primaries = ExynosPrimariesType::PRIMARIES_BT709_5;
            matrix    = ExynosMatrixCoeffType::MATRIX_COEFF_REC709;
            break;
        case HAL_DATASPACE_STANDARD_BT601_625:
            primaries = ExynosPrimariesType::PRIMARIES_BT601_6_625;
            matrix    = ExynosMatrixCoeffType::MATRIX_COEFF_SMPTE170M;
            break;
        case HAL_DATASPACE_STANDARD_BT601_625_UNADJUSTED:
            primaries = ExynosPrimariesType::PRIMARIES_BT601_6_625;
            matrix    = ExynosMatrixCoeffType::MATRIX_COEFF_REC709;
            break;
        case HAL_DATASPACE_STANDARD_BT601_525:
            primaries = ExynosPrimariesType::PRIMARIES_BT601_6_525;
            matrix    = ExynosMatrixCoeffType::MATRIX_COEFF_SMPTE170M;
            break;
        case HAL_DATASPACE_STANDARD_BT601_525_UNADJUSTED:
            primaries = ExynosPrimariesType::PRIMARIES_BT601_6_525;
            matrix    = ExynosMatrixCoeffType::MATRIX_COEFF_SMPTE240M;
            break;
        case HAL_DATASPACE_STANDARD_BT2020:
            primaries = ExynosPrimariesType::PRIMARIES_BT2020;
            matrix    = ExynosMatrixCoeffType::MATRIX_COEFF_BT2020;
            break;
        case HAL_DATASPACE_STANDARD_BT2020_CONSTANT_LUMINANCE:
            primaries = ExynosPrimariesType::PRIMARIES_BT2020;
            matrix    = ExynosMatrixCoeffType::MATRIX_COEFF_BT2020_CONSTANT;
            break;
        case HAL_DATASPACE_STANDARD_BT470M:
            primaries = ExynosPrimariesType::PRIMARIES_BT470_6M;
            matrix    = ExynosMatrixCoeffType::MATRIX_COEFF_470_SYSTEM_M;
            break;
        default:
            primaries = ExynosPrimariesType::PRIMARIES_UNSPECIFIED;
            matrix    = ExynosMatrixCoeffType::MATRIX_COEFF_UNSPECIFIED;
            break;
        }

        switch (dataspace & HAL_DATASPACE_TRANSFER_MASK) {
        case HAL_DATASPACE_TRANSFER_LINEAR:
            transfer = ExynosTransferType::TRANSFER_LINEAR;
            break;
        case HAL_DATASPACE_TRANSFER_SRGB:
            transfer = ExynosTransferType::TRANSFER_SRGB;
            break;
        case HAL_DATASPACE_TRANSFER_SMPTE_170M:
            transfer = ExynosTransferType::TRANSFER_SMPTE_170M;
            break;
        case HAL_DATASPACE_TRANSFER_GAMMA2_2:
            transfer = ExynosTransferType::TRANSFER_GAMMA_22;
            break;
#if 0
        case HAL_DATASPACE_TRANSFER_GAMMA2_6:
            transfer = ExynosTransferType::TRANSFER_GAMMA_26;
            break;
#endif
        case HAL_DATASPACE_TRANSFER_GAMMA2_8:
            transfer = ExynosTransferType::TRANSFER_GAMMA_28;
            break;
        case HAL_DATASPACE_TRANSFER_ST2084:
            transfer = ExynosTransferType::TRANSFER_ST2084;
            break;
        case HAL_DATASPACE_TRANSFER_HLG:
            transfer = ExynosTransferType::TRANSFER_HLG;
            break;
        default:
            transfer = ExynosTransferType::TRANSFER_UNSPECIFIED;
            break;
        }
    }

    return;
}

int ExynosUtils::GetDefaultDataSpaceIfNeeded(
    int width,
    int height) {
    int dataspace = HAL_DATASPACE_UNKNOWN;

    ColorAspects aspects;
    initColorAspects(aspects);

    ColorUtils::setDefaultCodecColorAspectsIfNeeded(aspects, width, height);

    dataspace = ColorUtils::getDataSpaceForColorAspects(aspects, false);

    return dataspace;
}

int ExynosUtils::GetDataspaceFromColorAspects(
    int range,
    int primaries,
    int transfer,
    int matrix) {
    ColorAspects aspects;
    initColorAspects(aspects);

    ColorUtils::convertIsoColorAspectsToCodecAspects(primaries, transfer, matrix,
                                                     ((range == ExynosRangeType::RANGE_FULL)? true:false), aspects);

    return ColorUtils::getDataSpaceForColorAspects(aspects, true);
}

int ExynosUtils::GetDataspaceFromColorAspects(
    int width,
    int height,
    const ExynosColorAspects &CA) {
    int dataspace = HAL_DATASPACE_UNKNOWN;

    ColorAspects aspects;
    initColorAspects(aspects);

    ColorUtils::convertIsoColorAspectsToCodecAspects(CA.mPrimaries, CA.mTransfer, CA.mMatrixCoeffs,
                                                     ((CA.mRange == ExynosRangeType::RANGE_FULL)? true:false), aspects);

    ColorUtils::setDefaultCodecColorAspectsIfNeeded(aspects, width, height);

    dataspace = ColorUtils::getDataSpaceForColorAspects(aspects, true);

    return dataspace;
}

uint32_t ExynosUtils::GetOutputSizeForEnc(uint32_t width, uint32_t height) {
    if ((width * height) >= HIGH_OUTPUT_BASELINE) {
        return MAX_OUTPUT_SIZE(width, height);
    }

    if ((width * height) >= MIDDLE_OUTPUT_BASELINE) {
        return MIN(MIDDLE_OUTPUT_SIZE(width, height), MIDDLE_OUTPUT_UPPER_LIMIT);
    }

    return MAX(MIN(LOW_OUTPUT_SIZE(width, height), LOW_OUTPUT_UPPTER_LIMIT), MIN_CPB_SIZE);
}

uint32_t ExynosUtils::GetOutputSizeForEncSecure(uint32_t width, uint32_t height) {
    auto size = GetOutputSizeForEnc(width, height);

#ifdef USE_SMALL_SECURE_MEMORY
    size = MIN(size, LIMITED_SECURE_OUTPUT_SIZE);
#endif

    return size;
}

ExynosDebugType ExynosUtils::GetDebugType(const std::string& name) {
    ExynosDebugType type = EXYNOS_DEBUG_NONE;

    int val = property_get_int32("vendor.debug.c2.dump", EXYNOS_DEBUG_NONE);
    if (val > EXYNOS_DEBUG_NONE) {
        char prop[PROPERTY_VALUE_MAX] = { 0, };
        property_get("vendor.debug.c2.dump.opt", prop, "default");

        std::string opt(prop);

        if (opt == std::string("default")) {
            type = (ExynosDebugType)val;
        } else {
            /* value example) Dec,Enc,CSC,... */
            const std::string delimiter = ",";

            std::string::size_type pos = 0;
            std::string::size_type start = 0;

            do {
                std::string token = "";

                pos = opt.find(delimiter, start);

                if (pos == std::string::npos) {
                    token = opt.substr(start, opt.size() - start);
                } else {
                    token = opt.substr(start, pos - start);
                    start = pos + 1;
                }

                if (name.find(token) != std::string::npos) {
                    type = (ExynosDebugType)val;
                    break;
                }
            } while (pos != std::string::npos);
        }
    }

    StaticExynosLog(Level::Trace, "ExynosUtils", "[%s] %s : debug type(0x%x)",
                        __FUNCTION__, name.c_str(), type);

    return type;
}

uint32_t ExynosUtils::GetCompressedColorType() {
    uint32_t compressedColor = VendorC2Config::COMPRESSED_COLOR_NONE;
    bool val = property_get_bool("vendor.debug.c2.sbwc.enable", false);
    if (val) {
        compressedColor = VendorC2Config::COMPRESSED_COLOR_LOSSLESS;
    }
    return compressedColor;
}

uint32_t ExynosUtils::GetMinQuality() {
    int val = property_get_int32("vendor.debug.c2.minquality", 0);

    return val;
}

bool ExynosUtils::GetFilmgrainType() {
    bool val = property_get_bool("vendor.debug.c2.filmgrain.disable", false);

    return !val;
}

uint64_t ExynosUtils::GetUsageType() {
    uint64_t val = property_get_int64("vendor.debug.c2.usage", 0);

    return val;
}
