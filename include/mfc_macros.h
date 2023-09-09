/*
 *
 * Copyright 2020 Samsung Electronics S.LSI Co. LTD
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

/* w: width, h: height, b: block size, a: alignment, s: stride, e: extra */
#define SBWC_STRIDE(w, b, a)        ALIGN(((b) * (((w) + 31) / 32)), a)
#define SBWC_HEADER_STRIDE(w)       ((((((w) + 63) / 64) + 15) / HW_WIDTH_ALIGN) * HW_WIDTH_ALIGN)
#define SBWC_Y_SIZE(s, h, e)        (((s) * ((ALIGN((h), HW_HEIGHT_ALIGN) + 3) / 4)) + e)
#define SBWC_CBCR_SIZE(s, h, e)     (((s) * (((ALIGN((h), HW_HEIGHT_ALIGN) / 2) + 3) / 4)) + e)
#define SBWC_Y_HEADER_SIZE(w, h, a) ALIGN(((SBWC_HEADER_STRIDE(w) * ((ALIGN((h), HW_HEIGHT_ALIGN) + 3) / 4)) + 256), a)
#define SBWC_CBCR_HEADER_SIZE(w, h) ((SBWC_HEADER_STRIDE(w) * (((ALIGN((h), HW_HEIGHT_ALIGN) / 2) + 3) / 4)) + 128)

/* SBWC Lossless 32B align */
#define SBWC_8B_STRIDE(w)   SBWC_STRIDE(w, 128, 1)
#define SBWC_10B_STRIDE(w)  SBWC_STRIDE(w, 160, 1)

#define SBWC_8B_Y_SIZE(w, h)            SBWC_Y_SIZE(SBWC_8B_STRIDE(w), h, 64)
#define SBWC_8B_Y_HEADER_SIZE(w, h)     SBWC_Y_HEADER_SIZE(w, h, 32)
#define SBWC_8B_CBCR_SIZE(w, h)         SBWC_CBCR_SIZE(SBWC_8B_STRIDE(w), h, 64)
#define SBWC_8B_CBCR_HEADER_SIZE(w, h)  SBWC_CBCR_HEADER_SIZE(w, h)

#define SBWC_10B_Y_SIZE(w, h)           SBWC_Y_SIZE(SBWC_10B_STRIDE(w), h, 64)
#define SBWC_10B_Y_HEADER_SIZE(w, h)    ALIGN((((ALIGN((w), 32) * ALIGN((h), HW_HEIGHT_ALIGN) * 2) + 256) - SBWC_10B_Y_SIZE(w, h)), 32)
#define SBWC_10B_CBCR_SIZE(w, h)        SBWC_CBCR_SIZE(SBWC_10B_STRIDE(w), h, 64)
#define SBWC_10B_CBCR_HEADER_SIZE(w, h) (((ALIGN((w), 32) * ALIGN((h), HW_WIDTH_ALIGN)) + 256) - SBWC_10B_CBCR_SIZE(w, h))

/* SBWC Lossless 256B align */
#define SBWC_256_8B_STRIDE(w)   SBWC_STRIDE(w, 128, 256)
#define SBWC_256_10B_STRIDE(w)  SBWC_STRIDE(w, 256, 1)

#define SBWC_256_8B_Y_SIZE(w, h)            SBWC_Y_SIZE(SBWC_256_8B_STRIDE(w), h, 0)
#define SBWC_256_8B_Y_HEADER_SIZE(w, h)     SBWC_Y_HEADER_SIZE(w, h, 256)
#define SBWC_256_8B_CBCR_SIZE(w, h)         SBWC_CBCR_SIZE(SBWC_256_8B_STRIDE(w), h, 0)
#define SBWC_256_8B_CBCR_HEADER_SIZE(w, h)  SBWC_CBCR_HEADER_SIZE(w, h)

#define SBWC_256_10B_Y_SIZE(w, h)           SBWC_Y_SIZE(SBWC_256_10B_STRIDE(w), h, 0)
#define SBWC_256_10B_Y_HEADER_SIZE(w, h)    SBWC_Y_HEADER_SIZE(w, h, 256)
#define SBWC_256_10B_CBCR_SIZE(w, h)        SBWC_CBCR_SIZE(SBWC_256_10B_STRIDE(w), h, 0)
#define SBWC_256_10B_CBCR_HEADER_SIZE(w, h) SBWC_CBCR_HEADER_SIZE(w, h)

#define SBWCL_8B_STRIDE(w, r)           (((128 * (r)) / 100) * (((w) + 31) / 32))
#define SBWCL_10B_STRIDE(w, r)          (((160 * (r)) / 100) * (((w) + 31) / 32))
#define SBWCL_8B_Y_SIZE(w, h, r)        ((SBWCL_8B_STRIDE(w, r) * ((ALIGN((h), HW_HEIGHT_ALIGN) + 3) / 4)) + 64)
#define SBWCL_8B_CBCR_SIZE(w, h, r)     ((SBWCL_8B_STRIDE(w, r) * (((ALIGN((h), HW_HEIGHT_ALIGN) / 2) + 3) / 4)) + 64)
#define SBWCL_10B_Y_SIZE(w, h, r)       ((SBWCL_10B_STRIDE(w, r) * ((ALIGN((h), HW_HEIGHT_ALIGN) + 3) / 4)) + 64)
#define SBWCL_10B_CBCR_SIZE(w, h, r)    ((SBWCL_10B_STRIDE(w, r) * (((ALIGN((h), HW_HEIGHT_ALIGN) / 2) + 3) / 4)) + 64)

/* SBWC Lossy v2.7 32B/64B align */
#define SBWCL_32_STRIDE(w)             (96 * (((w) + 31) / 32))
#define SBWCL_64_STRIDE(w)             (128 * (((w) + 31) / 32))
#define SBWCL_HEADER_STRIDE(w)         ((((((w) + 63) / 64) + 15) / HW_WIDTH_ALIGN) * HW_WIDTH_ALIGN)

#define SBWCL_32_Y_SIZE(w, h)          (SBWCL_32_STRIDE(w) * ((ALIGN((h), HW_HEIGHT_ALIGN) + 3) / 4))
#define SBWCL_32_CBCR_SIZE(w, h)       (SBWCL_32_STRIDE(w) * (((ALIGN((h), HW_HEIGHT_ALIGN) / 2) + 3) / 4))

#define SBWCL_64_Y_SIZE(w, h)          (SBWCL_64_STRIDE(w) * ((ALIGN((h), HW_HEIGHT_ALIGN) + 3) / 4))
#define SBWCL_64_CBCR_SIZE(w, h)       (SBWCL_64_STRIDE(w) * (((ALIGN((h), HW_HEIGHT_ALIGN) / 2) + 3) / 4))

#define SBWCL_Y_HEADER_SIZE(w, h)      ((SBWCL_HEADER_STRIDE(w) * ((ALIGN((h), HW_HEIGHT_ALIGN) + 3) / 4)) + 64)
#define SBWCL_CBCR_HEADER_SIZE(w, h)   ((SBWCL_HEADER_STRIDE(w) * (((ALIGN((h), HW_HEIGHT_ALIGN) / 2) + 3) / 4)) + 64)
