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

#include <system/graphics.h>
#include "exynos_format.h"
#include "ExynosGraphicBuffer.h"
#include "hardware/exynos/acryl.h"

#include "ExynosCSC.h"

#define LOG_ON
#include "ExynosLog.h"
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "ExynosCSC"

typedef struct __coeff {
    int64_t CR;
    int64_t CG;
    int64_t CB;
} coeff;

typedef struct __coeff_matrix {
    coeff Y;
    coeff U;
    coeff V;
} coeff_matrix;

#if 1 /* refer to Codec2BufferUtils */
static const coeff_matrix kBT601[2] = {
    /* FULL RANGE */
    { {   77,  150,   29 },
      {  -43,  -85,  128 },
      {  128, -107,  -21 }
    },
    /* LIMITED RANGE */
    { {   66,  129,   25 },
      {  -38,  -74,  112 },
      {  112,  -94,  -18 }
    },
};

static const coeff_matrix kBT709[2] = {
    /* FULL_RANGE :: TRICKY: 18 is adjusted to 19 so that sum of row 1 is 256 */
    { {   54,  183,   19 },
      {  -29,  -99,  128 },
      {  128, -116,  -12 }
    },
    /* LIMITED RANGE :: TRICKY: -87 is adjusted to -86 so that sum of row 2 is 0 */
    { {   47,  157,   16 },
      {  -26,  -86,  112 },
      {  112, -102,  -10 }
    },
};
#endif

const coeff_matrix kBT2020[2] = {
    /* FULL RANGE */
    { {   67,  174,   15 },
      {  -36,  -92,  128 },
      {  128, -118,  -10 }
    },
    /* LIMITED RANGE */
    { {   58,  149,   13 },
      {  -31,  -79,  110 },
      {  110, -101,   -9 }
    },
};

typedef bool (*ConvFunc)(ExynosBufferInfo input, ExynosBufferInfo output);

static bool bufferMap(ExynosBufferInfo input, ExynosBufferInfo output,
                      BufferAddressInfo &inAddrInfo, BufferAddressInfo &outAddrInfo) {
    std::shared_ptr<ExynosBuffer> inBuf = input.obj;
    if (inBuf->map(inAddrInfo) == false) {
        StaticExynosLog(Level::Error, LOG_TAG, "[%s] mmap(input) is failed", __FUNCTION__);
        return false;
    }

    std::shared_ptr<ExynosBuffer> outBuf = output.obj;
    if (outBuf->map(outAddrInfo) == false) {
        StaticExynosLog(Level::Error, LOG_TAG, "[%s] mmap(output) is failed", __FUNCTION__);
        inBuf->unmap();
        return false;
    }

    if ((inAddrInfo.num != input.nPlane) ||
        (outAddrInfo.num != output.nPlane)) {
        StaticExynosLog(Level::Error, LOG_TAG, "[%s] information is weird", __FUNCTION__);
        inBuf->unmap();
        outBuf->unmap();
        return false;
    }

    return true;
}

static bool conv420P(
    ExynosBufferInfo input,
    ExynosBufferInfo output) {
    BufferAddressInfo inAddrInfo, outAddrInfo;

    if (false == bufferMap(input, output, inAddrInfo, outAddrInfo)) {
        return false;
    }
    std::shared_ptr<ExynosBuffer> inBuf = input.obj;
    std::shared_ptr<ExynosBuffer> outBuf = output.obj;

    /* Y plane */
    char *pSrc = (char *)inAddrInfo.plane[0];
    char *pDst = (char *)outAddrInfo.plane[0];
    if ((input.stImageInfo.nStride == output.stImageInfo.nStride) &&
        (input.stImageInfo.stCropInfo.nWidth == output.stImageInfo.stCropInfo.nWidth)) {
        memcpy(pDst, pSrc, (input.stImageInfo.nStride * input.stImageInfo.stCropInfo.nHeight));
    } else {
        for (int i = 0; i < (int)input.stImageInfo.stCropInfo.nHeight; i++) {
            memcpy(pDst + (output.stImageInfo.nStride * i),
                   pSrc + (input.stImageInfo.nStride * i),
                   input.stImageInfo.stCropInfo.nWidth);
        }
    }

    int SRC_WIDTH_ALIGN = (inBuf->getFlags() & ExynosBuffer::GPU_TEXTURE)? HW_GPU_ALIGN:HW_WIDTH_ALIGN;
    if (input.stImageInfo.nFormat == HAL_PIXEL_FORMAT_YV12) {
        SRC_WIDTH_ALIGN = 16;
    }

    int DST_WIDTH_ALIGN = (outBuf->getFlags() & ExynosBuffer::GPU_TEXTURE)? HW_GPU_ALIGN:HW_WIDTH_ALIGN;
    if (output.stImageInfo.nFormat == HAL_PIXEL_FORMAT_YV12) {
        DST_WIDTH_ALIGN = 16;
    }

    char *pSrc_Plane2 = nullptr;
    char *pDst_Plane2 = nullptr;
    char *pSrc_Plane3 = nullptr;
    char *pDst_Plane3 = nullptr;
    int srcStride = ALIGN((input.stImageInfo.nStride >> 1), SRC_WIDTH_ALIGN);
    int dstStride = ALIGN((output.stImageInfo.nStride >> 1), DST_WIDTH_ALIGN);

    if (inAddrInfo.num < outAddrInfo.num) {
        /* user format to vendor format */
        pSrc_Plane2 = ((char *)inAddrInfo.plane[0] + (input.stImageInfo.nStride * input.stImageInfo.nHeight));
        pSrc_Plane3 = pSrc_Plane2 + (srcStride * (input.stImageInfo.nHeight / 2));

        pDst_Plane2 = (char *)outAddrInfo.plane[1];
        pDst_Plane3 = (char *)outAddrInfo.plane[2];
    } else if (inAddrInfo.num > outAddrInfo.num) {
        /* vendor format to user format */
        pSrc_Plane2 = ((char *)inAddrInfo.plane[1]);
        pSrc_Plane3 = ((char *)inAddrInfo.plane[2]);

        pDst_Plane2 = (char *)outAddrInfo.plane[0] + (output.stImageInfo.nStride * output.stImageInfo.nHeight);
        pDst_Plane3 = pDst_Plane2 + (dstStride * (output.stImageInfo.nHeight / 2));
    } else {
        StaticExynosLog(Level::Error, LOG_TAG, "[%s] wrong input & output buffer", __FUNCTION__);
        inBuf->unmap();
        outBuf->unmap();
        return false;
    }

    /* U/V plane */
    for (int i = 0; i < (int)(input.stImageInfo.stCropInfo.nHeight >> 1); i++) {
        memcpy(pDst_Plane2 + (dstStride * i),
               pSrc_Plane2 + (srcStride * i),
               (input.stImageInfo.stCropInfo.nWidth >> 1));
    }

    /* V/U plane */
    for (int i = 0; i < (int)(input.stImageInfo.stCropInfo.nHeight >> 1); i++) {
        memcpy(pDst_Plane3 + (dstStride * i),
               pSrc_Plane3 + (srcStride * i),
               (input.stImageInfo.stCropInfo.nWidth >> 1));
    }

    inBuf->unmap();
    outBuf->unmap();

    return true;
}

static bool conv420SPXto420SPX_Common(
    ExynosBufferInfo input,
    ExynosBufferInfo output,
    bool bIs10Bit = false) {
    BufferAddressInfo inAddrInfo, outAddrInfo;
    int byte = (bIs10Bit == false)? 1: 2;

    if (false == bufferMap(input, output, inAddrInfo, outAddrInfo)) {
        return false;
    }
    std::shared_ptr<ExynosBuffer> inBuf = input.obj;
    std::shared_ptr<ExynosBuffer> outBuf = output.obj;

    char *pSrc = (char *)inAddrInfo.plane[0];
    char *pDst = (char *)outAddrInfo.plane[0];
    char *pSrcCbCr = nullptr;
    char *pDstCbCr = nullptr;

    if ((input.stImageInfo.nFormat == HAL_PIXEL_FORMAT_YCrCb_420_SP) ||
        (input.stImageInfo.nFormat == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP) ||
        (input.stImageInfo.nFormat == HAL_PIXEL_FORMAT_YCBCR_P010)) {
        /* single-fd to multi-fd */
        int vstride = vendor::graphics::ExynosGraphicBufferMeta::get_vstride(input.obj->handle());

        pSrcCbCr = (char *)inAddrInfo.plane[0] + ((input.stImageInfo.nStride * byte) * vstride);
        pDstCbCr = (char *)outAddrInfo.plane[1];
    } else {
        /* multi-fd to single-fd */
        int vstride = vendor::graphics::ExynosGraphicBufferMeta::get_vstride(output.obj->handle());

        pSrcCbCr = (char *)inAddrInfo.plane[1];
        pDstCbCr = (char *)outAddrInfo.plane[0] + ((output.stImageInfo.nStride * byte) * vstride);
    }

    if ((input.stImageInfo.nStride == output.stImageInfo.nStride) &&
        (input.stImageInfo.stCropInfo.nWidth == output.stImageInfo.stCropInfo.nWidth)) {
        /* Y plane */
        memcpy(pDst, pSrc, ((input.stImageInfo.nStride * byte) * input.stImageInfo.stCropInfo.nHeight));

        /* CbCr plane */
        memcpy(pDstCbCr, pSrcCbCr, ((input.stImageInfo.nStride * byte) * (input.stImageInfo.stCropInfo.nHeight >> 1)));
    } else {
        /* Y plane */
        for (int i = 0; i < (int)input.stImageInfo.stCropInfo.nHeight; i++) {
            memcpy(pDst + ((output.stImageInfo.nStride * byte) * i),
                   pSrc + ((input.stImageInfo.nStride * byte) * i),
                   (input.stImageInfo.stCropInfo.nWidth * byte));
        }

        /* CbCr plane */
        for (int i = 0; i < (int)(input.stImageInfo.stCropInfo.nHeight >> 1); i++) {
            memcpy(pDstCbCr + ((output.stImageInfo.nStride * byte) * i),
                   pSrcCbCr + ((input.stImageInfo.nStride * byte) * i),
                   (input.stImageInfo.stCropInfo.nWidth * byte));
        }
    }

    inBuf->unmap();
    outBuf->unmap();

    return true;
}

static bool conv420Pto420PM(
    ExynosBufferInfo input,
    ExynosBufferInfo output) {
    return conv420P(input, output);
}

static bool conv420SPto420SPM(
    ExynosBufferInfo input,
    ExynosBufferInfo output) {
    return conv420SPXto420SPX_Common(input, output, false);
}

static bool convP010XtoP010X(
    ExynosBufferInfo input,
    ExynosBufferInfo output) {
    return conv420SPXto420SPX_Common(input, output, true);
}

static bool conv420PMto420P(
    ExynosBufferInfo input,
    ExynosBufferInfo output) {
    return conv420P(input, output);
}

static bool convP010MtoYV12(
    ExynosBufferInfo input,
    ExynosBufferInfo output) {
    BufferAddressInfo inAddrInfo, outAddrInfo;

    if (false == bufferMap(input, output, inAddrInfo, outAddrInfo)) {
        return false;
    }
    std::shared_ptr<ExynosBuffer> inBuf = input.obj;
    std::shared_ptr<ExynosBuffer> outBuf = output.obj;

    /* Y plane */
    uint16_t *pSrc = (uint16_t *)inAddrInfo.plane[0];
    uint8_t  *pDst = (uint8_t *)outAddrInfo.plane[0];

    for (size_t y = 0; y < input.stImageInfo.stCropInfo.nHeight; ++y) {
        for (size_t x = 0; x < input.stImageInfo.stCropInfo.nWidth; ++x) {
            pDst[x] = (uint8_t)(pSrc[x] >> 8);  /* little endian : MSB(8bit + 2bit) */
        }

        pSrc += input.stImageInfo.nStride;
        pDst += output.stImageInfo.nStride;
    }

    int DST_WIDTH_ALIGN = (outBuf->getFlags() & ExynosBuffer::GPU_TEXTURE)? HW_GPU_ALIGN:HW_WIDTH_ALIGN;
    if (output.stImageInfo.nFormat == HAL_PIXEL_FORMAT_YV12) {
        DST_WIDTH_ALIGN = 16;
    }

    /* CbCr plane */
    uint16_t *pSrcCb = (uint16_t *)inAddrInfo.plane[1];

    uint8_t *pDstV = nullptr;
    uint8_t *pDstU = nullptr;

    if (output.stImageInfo.nFormat == HAL_PIXEL_FORMAT_EXYNOS_YV12_M) {
        pDstV = ((uint8_t *)outAddrInfo.plane[1]);
        pDstU = ((uint8_t *)outAddrInfo.plane[2]);
    } else {
        pDstV = ((uint8_t *)outAddrInfo.plane[0]) + (output.stImageInfo.nStride * output.stImageInfo.nHeight);
        pDstU = (uint8_t *)(pDstV + ALIGN((output.stImageInfo.nStride >> 1), DST_WIDTH_ALIGN) * (output.stImageInfo.nHeight >> 1));
    }

    for (size_t y = 0; y < (input.stImageInfo.stCropInfo.nHeight + 1) / 2; ++y) {
        for (size_t x = 0; x < (input.stImageInfo.stCropInfo.nWidth + 1) / 2; ++x) {
            pDstU[x] = (uint8_t)(pSrcCb[(x * 2)] >> 8);  /* little endian : MSB(8bit + 2bit) */
            pDstV[x] = (uint8_t)(pSrcCb[(x * 2) + 1] >> 8);
        }

        pSrcCb += input.stImageInfo.nStride;
        pDstV += ALIGN((output.stImageInfo.nStride >> 1), DST_WIDTH_ALIGN);
        pDstU += ALIGN((output.stImageInfo.nStride >> 1), DST_WIDTH_ALIGN);
    }

    inBuf->unmap();
    outBuf->unmap();

    return true;
}

static bool convP010toNV12X(
    ExynosBufferInfo input,
    ExynosBufferInfo output) {
    BufferAddressInfo inAddrInfo, outAddrInfo;

    if (false == bufferMap(input, output, inAddrInfo, outAddrInfo)) {
        return false;
    }
    std::shared_ptr<ExynosBuffer> inBuf = input.obj;
    std::shared_ptr<ExynosBuffer> outBuf = output.obj;

    /* Y plane */
    uint16_t *pSrc = (uint16_t *)inAddrInfo.plane[0];
    uint8_t  *pDst = (uint8_t *)outAddrInfo.plane[0];

    for (size_t y = 0; y < input.stImageInfo.stCropInfo.nHeight; ++y) {
        for (size_t x = 0; x < input.stImageInfo.stCropInfo.nWidth; ++x) {
            pDst[x] = (uint8_t)(pSrc[x] >> 8);  /* little endian : MSB(8bit + 2bit) */
        }

        pSrc += input.stImageInfo.nStride;
        pDst += output.stImageInfo.nStride;
    }

    /* CbCr plane */
    int vstride = vendor::graphics::ExynosGraphicBufferMeta::get_vstride(input.obj->handle());

    uint16_t *pSrcCb    = (uint16_t *)inAddrInfo.plane[0] + (input.stImageInfo.nStride * vstride);
    uint8_t  *pDstCb    = (uint8_t *)outAddrInfo.plane[1];

    if (output.stImageInfo.nFormat == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN) {
        int vstride = vendor::graphics::ExynosGraphicBufferMeta::get_vstride(output.obj->handle());

        pDstCb = (uint8_t *)outAddrInfo.plane[0] + (output.stImageInfo.nStride * vstride);
    }

    for (size_t y = 0; y < (input.stImageInfo.stCropInfo.nHeight + 1) / 2; ++y) {
        for (size_t x = 0; x < input.stImageInfo.stCropInfo.nWidth; x += 2) {
            pDstCb[x]       = (uint8_t)(pSrcCb[x] >> 8);  /* little endian : MSB(8bit + 2bit) */
            pDstCb[x + 1]   = (uint8_t)(pSrcCb[x + 1] >> 8);
        }

        pSrcCb += input.stImageInfo.nStride;
        pDstCb += output.stImageInfo.nStride;
    }

    inBuf->unmap();
    outBuf->unmap();

    return true;
}

static bool convRGBAtoNV21M(
    ExynosBufferInfo input,
    ExynosBufferInfo output) {
    BufferAddressInfo inAddrInfo, outAddrInfo;

    if (false == bufferMap(input, output, inAddrInfo, outAddrInfo)) {
        return false;
    }

    std::shared_ptr<ExynosBuffer> inBuf = input.obj;
    std::shared_ptr<ExynosBuffer> outBuf = output.obj;

    const coeff_matrix *matrix = nullptr;

    /* FULL RANGE */
    int range = 0;
    int zeroLvl = 0;
    int maxLvlLuma = 255;
    int maxLvlChroma = 255;

    if (!ExynosUtils::CheckFullRange(output.stImageInfo.nDataSpace)) {
        /* LIMITED RANGE */
        range = 1;
        zeroLvl = 16;
        maxLvlLuma = 235;
        maxLvlChroma = 240;
    }

    if (ExynosUtils::CheckBT601(output.stImageInfo.nDataSpace)) {
        matrix = &kBT601[range];
    } else if (ExynosUtils::CheckBT709(output.stImageInfo.nDataSpace)) {
        matrix = &kBT709[range];
    } else {
        matrix = &kBT2020[range];
    }

    {
        unsigned int tmp;
        unsigned int R, G, B;
        uint64_t Y, U, V;

        int src_stride = input.stImageInfo.nStride;
        int width  = input.stImageInfo.stCropInfo.nWidth;
        int height = input.stImageInfo.stCropInfo.nHeight;

        int dst_stride = output.stImageInfo.nStride;

        unsigned int *pSrc = (unsigned int *)inAddrInfo.plane[0];
        unsigned char *pDstY = (unsigned char *)outAddrInfo.plane[0];
        unsigned char *pDstVU = (unsigned char *)outAddrInfo.plane[1];

        int vu_offset = 0;

#define CLIP3(min,v,max) (((v) < (min)) ? (min) : (((max) > (v)) ? (v) : (max)))
        for (int j = 0; j < height; j++) {
            if ((j % 2) == 0) {
                vu_offset = ((j / 2) * dst_stride);
            }

            for (int i = 0; i < width; i++) {
                tmp = pSrc[(j * src_stride) + i];

                B = (tmp & 0x00FF0000) >> 16;
                G = (tmp & 0x0000FF00) >> 8;
                R = (tmp & 0x000000FF);

                Y = (((matrix->Y.CR * R) + (matrix->Y.CG * G) + (matrix->Y.CB * B)) >> 8) + zeroLvl;
                Y = CLIP3(zeroLvl, Y, maxLvlLuma);

                pDstY[(j * dst_stride) + i] = (unsigned char)Y;

                if ((j % 2) == 0 && (i % 2) == 0) {
                    U = (((matrix->U.CR * R) + (matrix->U.CG * G) + (matrix->U.CB * B)) >> 8) + 128;
                    U = CLIP3(zeroLvl, U, maxLvlChroma);

                    V = (((matrix->V.CR * R) + (matrix->V.CG * G) + (matrix->V.CB * B)) >> 8) + 128;
                    V = CLIP3(zeroLvl, V, maxLvlChroma);

                    pDstVU[vu_offset++] = (unsigned char)V;
                    pDstVU[vu_offset++] = (unsigned char)U;
                }
            }
        }
    }

    inBuf->unmap();
    outBuf->unmap();

    return true;
}

struct SW_CONV_INFO {
    int srcFormat;
    int dstFormat;
    ConvFunc func;
} SW_CONV_INFO_TABLE[] = {
    /* h/w format to user format */
    { HAL_PIXEL_FORMAT_EXYNOS_YV12_M,
      HAL_PIXEL_FORMAT_YV12,
      conv420PMto420P },
    { HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M,
      HAL_PIXEL_FORMAT_YCBCR_P010,
      convP010XtoP010X },
    { HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M,
      HAL_PIXEL_FORMAT_YV12,
      convP010MtoYV12 },
    { HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M,
      HAL_PIXEL_FORMAT_EXYNOS_YV12_M,
      convP010MtoYV12 },

    /* user format to h/w format */
    { HAL_PIXEL_FORMAT_YV12,
      HAL_PIXEL_FORMAT_EXYNOS_YV12_M,
      conv420Pto420PM },
    { HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M,
      conv420Pto420PM },
    { HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M,
      conv420SPto420SPM },
    { HAL_PIXEL_FORMAT_YCrCb_420_SP,
      HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M,
      conv420SPto420SPM },
    { HAL_PIXEL_FORMAT_YCBCR_P010,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M,
      convP010XtoP010X },
    { HAL_PIXEL_FORMAT_YCBCR_P010,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M,
      convP010toNV12X },
    { HAL_PIXEL_FORMAT_YCBCR_P010,
      HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN,
      convP010toNV12X },
    { HAL_PIXEL_FORMAT_RGBA_8888,
      HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M,
      convRGBAtoNV21M },
};

class SWCSCImpl : public CSCImpl {
public:
    SWCSCImpl(std::string name) {
        mObjName = name;
    }

    ~SWCSCImpl() = default;

    bool run(ExynosBufferInfo &input, ExynosBufferInfo &output) override {
        ExynosLogFunctionTrace();

        if ((input.obj.get() == nullptr) ||
            (output.obj.get() == nullptr)) {
            /* buffer is invalid */
            ExynosLogE("[%s] buffer is invalid(input:%p, output:%p)", __FUNCTION__,
                        input.obj.get(), output.obj.get());
            return false;
        }

        for (SW_CONV_INFO &info : SW_CONV_INFO_TABLE) {
            if ((info.srcFormat == input.stImageInfo.nFormat) &&
                (info.dstFormat == output.stImageInfo.nFormat)) {
                if (ExynosUtils::CheckRGB(input.stImageInfo.nFormat) &&
                    !ExynosUtils::CheckRGB(output.stImageInfo.nFormat)) {
                    updateActualDataSpace(output.stImageInfo.nDataSpace);
                }

                return info.func(input, output);
            }
        }

        ExynosLogE("[%s] conversion is not supported", __FUNCTION__);

        return false;
    }

private:
    void updateActualDataSpace(unsigned int &dataspace) override {
        ExynosLogD("[%s] dataspace(0x%x)", __FUNCTION__, dataspace);
    }

    SWCSCImpl() = delete;
};


class HWCSCImpl : public CSCImpl {
public:
    HWCSCImpl(std::string name) {
        mObjName = name;

        auto handle = Acrylic::createScaler();

        auto delfunc = [](Acrylic *p) {
                           if (p != nullptr) {
                               delete p;
                           }
                       };

        mCompositor = std::shared_ptr<Acrylic>(handle, std::move(delfunc));

        mSrcImage = mCompositor->createLayer();
        mFPS = ACRYLIC_MAX_FRAMERATE;
    }

    ~HWCSCImpl() {
        if (mCompositor.get() != nullptr) {
            if (mSrcImage != nullptr) {
                mCompositor->removeLayer(mSrcImage);
                mSrcImage = nullptr;
            }

            mCompositor.reset();
        }
    }

    bool run(ExynosBufferInfo &input, ExynosBufferInfo &output) override {
        ExynosLogFunctionTrace();

        if (mSrcImage == nullptr) {
            /* obj is not created */
            return false;
        }

        int srcBuf[MAX_HW2D_PLANES] = { 0, };
        size_t srcSize[MAX_HW2D_PLANES] = { 0, };
        uint32_t srcStride[MAX_HW2D_PLANES] = { 0, };
        uint32_t srcCStride = vendor::graphics::ExynosGraphicBufferMeta::get_cstride(input.obj->handle());

        for (int i = 0; i < input.nPlane; i++) {
            srcBuf[i]  = input.nFD[i];
            srcSize[i] = input.nAllocLen[i];
            ExynosLogD("[%s] input : buffer[%d] fd(%d), size(%d)", __FUNCTION__, i, srcBuf[i], srcSize[i]);
        }

        for (int i = 0, cnt = ExynosUtils::GetImagePlaneCnt(input.stImageInfo.nFormat); i < cnt; i++) {
            srcStride[i] = (i == 0)? input.stImageInfo.nStride:srcCStride;
            ExynosLogD("[%s] input : buffer[%d] stride(%d)", __FUNCTION__, i, srcStride[i]);
        }

        int srcWidth     = input.stImageInfo.nStride;
        int srcHeight    = vendor::graphics::ExynosGraphicBufferMeta::get_vstride(input.obj->handle());
        int srcFormat    = input.stImageInfo.nFormat;
        int srcDataspace = input.stImageInfo.nDataSpace;

        hwc_rect_t srcRect = { (int)input.stImageInfo.stCropInfo.nLeft,
                               (int)input.stImageInfo.stCropInfo.nTop,
                               (int)(input.stImageInfo.stCropInfo.nWidth + input.stImageInfo.stCropInfo.nLeft),
                               (int)(input.stImageInfo.stCropInfo.nHeight + input.stImageInfo.stCropInfo.nTop) };

        int dstWidth     = output.stImageInfo.nStride;
        int dstHeight    = vendor::graphics::ExynosGraphicBufferMeta::get_vstride(output.obj->handle());
        int dstFormat    = output.stImageInfo.nFormat;

        if (ExynosUtils::CheckRGB(input.stImageInfo.nFormat) &&
            !ExynosUtils::CheckRGB(output.stImageInfo.nFormat)) {
            updateActualDataSpace(output.stImageInfo.nDataSpace);
        }

        int dstDataspace = output.stImageInfo.nDataSpace;

        hwc_rect_t dstRect = { (int)output.stImageInfo.stCropInfo.nLeft,
                               (int)output.stImageInfo.stCropInfo.nTop,
                               (int)(output.stImageInfo.stCropInfo.nWidth + output.stImageInfo.stCropInfo.nLeft),
                               (int)(output.stImageInfo.stCropInfo.nHeight + output.stImageInfo.stCropInfo.nTop) };

        int dstBuf[MAX_HW2D_PLANES] = { 0, };
        size_t dstSize[MAX_HW2D_PLANES] = { 0, };
        uint32_t dstStride[MAX_HW2D_PLANES] = { 0, };
        uint32_t dstCStride = vendor::graphics::ExynosGraphicBufferMeta::get_cstride(output.obj->handle());

        for (int i = 0; i < output.nPlane; i++) {
            dstBuf[i]  = output.nFD[i];
            dstSize[i] = output.nAllocLen[i];
            ExynosLogD("[%s] output : buffer[%d] fd(%d), size(%d)", __FUNCTION__, i, dstBuf[i], dstSize[i]);
        }

        for (int i = 0, cnt = ExynosUtils::GetImagePlaneCnt(output.stImageInfo.nFormat); i < cnt; i++) {
            dstStride[i] = (i == 0)? output.stImageInfo.nStride:dstCStride;
            ExynosLogD("[%s] output : buffer[%d] stride(%d)", __FUNCTION__, i, dstStride[i]);
        }

        /* src */
        mSrcImage->setImageDimension(srcWidth, srcHeight);
        mSrcImage->setStride(srcStride);
        mSrcImage->setImageType(srcFormat, srcDataspace);
        mSrcImage->setCompositArea(srcRect, dstRect);  /* TODO : rotation, blur -> transform, attr */
        mSrcImage->setCompositMode(HWC_BLENDING_NONE);
        mSrcImage->setImageBuffer(srcBuf, srcSize, input.nPlane, -1/*fence*/, AcrylicCanvas::ATTR_NONE);  /* SECURE : AcrylicCanvas::ATTR_PROTECTED */

        /* dst */
        mCompositor->setCanvasDimension(dstWidth, dstHeight);
        mCompositor->setCanvasStride(dstStride);
        mCompositor->setCanvasImageType(dstFormat, dstDataspace);
        mCompositor->setCanvasBuffer(dstBuf, dstSize, output.nPlane, -1/*fence*/, AcrylicCanvas::ATTR_NONE);  /* SECURE : AcrylicCanvas::ATTR_PROTECTED */

        auto ret = mCompositor->execute();

        /* performance hint */
        AcrylicPerformanceRequest request;
        request.reset(1);
        request.getFrame(0)->setFrameRate(mFPS);
        mCompositor->requestPerformanceQoS(&request);

        return ret;
    }

    void setOperatingRate(uint32_t operatingRate) override {
        ExynosLogD("[%s]  frame rate: %d", __FUNCTION__, operatingRate);

        mFPS = MIN(((operatingRate == 0)? ACRYLIC_MAX_FRAMERATE:operatingRate), ACRYLIC_MAX_FRAMERATE);
    }

private:
    void updateActualDataSpace(unsigned int &dataspace) override {
        ExynosLogD("[%s] dataspace(0x%x)", __FUNCTION__, dataspace);
    }

    std::shared_ptr<Acrylic> mCompositor;
    AcrylicLayer *mSrcImage;
    uint32_t mFPS;

    HWCSCImpl() = delete;
};

ExynosCSC::ExynosCSC(std::string name, ExynosCSC::Type type) {
    mbLogOff = false;

    switch (type) {
    case ExynosCSC::Type::HW:
        mObjName = name + "-HWCSC";
        mImpl = std::static_pointer_cast<CSCImpl>(std::make_shared<HWCSCImpl>(mObjName));
        break;
    case ExynosCSC::Type::HW_SECURE:
        mObjName = name + "-HWCSC.secure";
        mImpl = std::static_pointer_cast<CSCImpl>(std::make_shared<HWCSCImpl>(mObjName));
        break;
    default:
        mObjName = name + "-SWCSC";
        mImpl = std::static_pointer_cast<CSCImpl>(std::make_shared<SWCSCImpl>(mObjName));
        break;
    }
}

bool ExynosCSC::process(ExynosBufferInfo input, ExynosBufferInfo output) {
    ExynosLogFunctionTrace();

    if (mImpl.get() == nullptr) {
        return false;
    }

    auto ret = mImpl->run(input, output);
    if (ret) {
        /* converting RGB to YUV domain
         * update color aspects got from datapsace to metadata
         */
        if (ExynosUtils::CheckRGB(input.stImageInfo.nFormat) &&
            !ExynosUtils::CheckRGB(output.stImageInfo.nFormat)) {
            auto meta = output.obj->metadata();

            if (meta != nullptr) {
                meta->eType = (ExynosVideoInfoType)(meta->eType | VIDEO_INFO_TYPE_COLOR_ASPECTS);

                ExynosColorAspects *pCA = &(meta->sColorAspects);

                ExynosUtils::GetColorAspectsFromDataspace(pCA->mRange, pCA->mPrimaries, pCA->mTransfer, pCA->mMatrixCoeffs,
                                                            output.stImageInfo.nDataSpace);

                ExynosLogD("[%s] set ColorAspects(ISO:: r:%d, p:0x%x, t:0x%x, m:0x%x)", __FUNCTION__,
                            pCA->mRange, pCA->mPrimaries, pCA->mTransfer, pCA->mMatrixCoeffs);
            }
        }
    }

    return ret;
}

void ExynosCSC::setOperatingRate(uint32_t operatingRate) {
    ExynosLogFunctionTrace();

    if (mImpl.get() == nullptr) {
        return;
    }

    mImpl->setOperatingRate(operatingRate);

    return;
}
