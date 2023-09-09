/*
 *
 * Copyright 2021 Samsung Electronics S.LSI Co. LTD
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

#include <system/graphics.h>
#include "exynos_format.h"

#include "ExynosVideoCodecBase.h"

static struct Video2Pixel {
    int video;
    int pixel;
    int cnt;
    int type;
} Video2PixelMap[] = {
/* YUV format */
    /* normal */
    { VIDEO_COLORFORMAT_NV12M,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M,
      2,
      DATA_8BIT,
    },
    { VIDEO_COLORFORMAT_NV21M,
      HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M,
      2,
      DATA_8BIT,
    },
    { VIDEO_COLORFORMAT_I420M,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M,
      3,
      DATA_8BIT,
    },
    { VIDEO_COLORFORMAT_YV12M,
      HAL_PIXEL_FORMAT_EXYNOS_YV12_M,
      3,
      DATA_8BIT,
    },
    { VIDEO_COLORFORMAT_NV12M_SBWC,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC,
      2,
      DATA_8BIT_SBWC,
    },
    { VIDEO_COLORFORMAT_NV21M_SBWC,
      HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_SBWC,
      2,
      DATA_8BIT_SBWC,
    },
    { VIDEO_COLORFORMAT_NV12M_SBWC_L50,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L50,
      2,
      DATA_8BIT_SBWC,
    },
    { VIDEO_COLORFORMAT_NV12M_SBWC_L75,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L75,
      2,
      DATA_8BIT_SBWC,
    },
    { VIDEO_COLORFORMAT_NV12,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN,
      1,
      DATA_8BIT,
    },
    { VIDEO_COLORFORMAT_NV12M_32_SBWC_L,
      HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_32_SBWC_L,
      2,
      DATA_8BIT_SBWC,
    },
    { VIDEO_COLORFORMAT_NV12M_64_SBWC_L,
      HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_64_SBWC_L,
      2,
      DATA_8BIT_SBWC,
    },
    { VIDEO_COLORFORMAT_NV12_32_SBWC_L,
      HAL_PIXEL_FORMAT_EXYNOS_420_SPN_32_SBWC_L,
      1,
      DATA_8BIT_SBWC,
    },
    { VIDEO_COLORFORMAT_NV12_64_SBWC_L,
      HAL_PIXEL_FORMAT_EXYNOS_420_SPN_64_SBWC_L,
      1,
      DATA_8BIT_SBWC,
    },
    { VIDEO_COLORFORMAT_NV12_DECOMP,
      HAL_PIXEL_FORMAT_EXYNOS_420_SPN_SBWC_DECOMP,
      1,
      DATA_8BIT,
    },
    /* normal 10bit */
    { VIDEO_COLORFORMAT_NV12M_P010,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M,
      2,
      DATA_10BIT,
    },
    { VIDEO_COLORFORMAT_NV12M_S10B,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B,
      2,
      DATA_8BIT_WITH_2BIT,
    },
    { VIDEO_COLORFORMAT_NV12M_10B_SBWC,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC,
      2,
      DATA_10BIT_SBWC,
    },
    { VIDEO_COLORFORMAT_NV12M_10B_SBWC_L40,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L40,
      2,
      DATA_10BIT_SBWC,
    },
    { VIDEO_COLORFORMAT_NV12M_10B_SBWC_L60,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L60,
      2,
      DATA_10BIT_SBWC,
    },
    { VIDEO_COLORFORMAT_NV12M_10B_SBWC_L80,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L80,
      2,
      DATA_10BIT_SBWC,
    },
    { VIDEO_COLORFORMAT_NV12_P010,
      HAL_PIXEL_FORMAT_YCBCR_P010,
      1,
      DATA_10BIT,
    },
    { VIDEO_COLORFORMAT_NV12M_10B_32_SBWC_L,
      HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_10B_32_SBWC_L,
      2,
      DATA_10BIT_SBWC,
    },
    { VIDEO_COLORFORMAT_NV12M_10B_64_SBWC_L,
      HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_10B_64_SBWC_L,
      2,
      DATA_10BIT_SBWC,
    },
    { VIDEO_COLORFORMAT_NV12_10B_32_SBWC_L,
      HAL_PIXEL_FORMAT_EXYNOS_420_SPN_10B_32_SBWC_L,
      1,
      DATA_10BIT_SBWC,
    },
    { VIDEO_COLORFORMAT_NV12_10B_64_SBWC_L,
      HAL_PIXEL_FORMAT_EXYNOS_420_SPN_10B_64_SBWC_L,
      1,
      DATA_10BIT_SBWC,
    },
    { VIDEO_COLORFORMAT_NV12_P010_DECOMP,
      HAL_PIXEL_FORMAT_EXYNOS_P010_N_SBWC_DECOMP,
      1,
      DATA_10BIT,
    },
    /* secure */
    { VIDEO_COLORFORMAT_I420,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_PN,
      1,
      DATA_8BIT,
    },
    { VIDEO_COLORFORMAT_NV12_SBWC,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC,
      1,
      DATA_8BIT_SBWC,
    },
    { VIDEO_COLORFORMAT_NV12_256_SBWC,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_256_SBWC,
      1,
      DATA_8BIT_SBWC,
    },
    { VIDEO_COLORFORMAT_NV12_SBWC_L50,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC_L50,
      1,
      DATA_8BIT_SBWC,
    },
    { VIDEO_COLORFORMAT_NV12_SBWC_L75,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC_L75,
      1,
      DATA_8BIT_SBWC,
    },
    /* secure 10bit */
    { VIDEO_COLORFORMAT_NV12_S10B,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_S10B,
      1,
      DATA_8BIT_WITH_2BIT,
    },
    { VIDEO_COLORFORMAT_NV12_10B_SBWC,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC,
      1,
      DATA_10BIT_SBWC,
    },
    { VIDEO_COLORFORMAT_NV12_10B_256_SBWC,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_256_SBWC,
      1,
      DATA_10BIT_SBWC,
    },
    { VIDEO_COLORFORMAT_NV12_10B_SBWC_L40,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L40,
      1,
      DATA_10BIT_SBWC,
    },
    { VIDEO_COLORFORMAT_NV12_10B_SBWC_L60,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L60,
      1,
      DATA_10BIT_SBWC,
    },
    { VIDEO_COLORFORMAT_NV12_10B_SBWC_L80,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L80,
      1,
      DATA_10BIT_SBWC,
    },
    //VIDEO_COLORFORMAT_NV21M_S10B,  /* unused */
    //VIDEO_COLORFORMAT_NV21M_P010,  /* unused */
    //VIDEO_COLORFORMAT_NV12_TILED,  /* unused */
    //VIDEO_COLORFORMAT_NV12M_TILED, /* unused */

/* RGB format */
    { VIDEO_COLORFORMAT_ARGB8888,
      HAL_PIXEL_FORMAT_EXYNOS_ARGB_8888,
      1,
      DATA_8BIT,
    },
    { VIDEO_COLORFORMAT_BGRA8888,
      HAL_PIXEL_FORMAT_BGRA_8888,
      1,
      DATA_8BIT,
    },
    { VIDEO_COLORFORMAT_RGBA8888,
      HAL_PIXEL_FORMAT_RGBA_8888,
      1,
      DATA_8BIT,
    },
};

uint32_t ExynosVideoCodecBase::getVideoFormat(uint32_t pixel) {
    uint32_t video = VIDEO_COLORFORMAT_UNKNOWN;

    for (const struct Video2Pixel &element : Video2PixelMap) {
        if (element.pixel == pixel) {
            video = element.video;
            break;
        }
    }

    return video;
}

uint32_t ExynosVideoCodecBase::getPixelFormat(uint32_t video) {
    uint32_t pixel = 0;  /* HAL_PIXEL_FORMAT_UNDEFINED */

    for (const struct Video2Pixel &element : Video2PixelMap) {
        if (element.video == video) {
            pixel = element.pixel;
            break;
        }
    }

    return pixel;
}

uint32_t ExynosVideoCodecBase::getPlaneCnt(uint32_t video) {
    uint32_t cnt = 0;

    for (const struct Video2Pixel &element : Video2PixelMap) {
        if (element.video == video) {
            cnt = element.cnt;
            break;
        }
    }

    return cnt;
}

uint32_t ExynosVideoCodecBase::getDataType(uint32_t video) {
    uint32_t type = 0;

    for (const struct Video2Pixel &element : Video2PixelMap) {
        if (element.video == video) {
            type = element.type;
            break;
        }
    }

    return type;
}

