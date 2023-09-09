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

#include "Exynos_C2_Component.h"
#include "Exynos_C2_DecComponent.h"
#include "Exynos_C2_EncComponent.h"
#include "Exynos_C2_FilterParamCnv.h"

FilterParamSetterRet CommonCnv::cnvOperateRate(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2Component::CommonParamIntf>(intf);

    auto framerate = intfImpl->getOperateRate();

    if (framerate > 0) {
        auto id = intfImpl->findFilterID("dec");
        if (id == 0) {
            id = intfImpl->findFilterID("enc");
        }

        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamOperatingRate>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamOperatingRate>>(filterParam->getBaseParam());

            param->m.value = framerate;

            filterParam->registTargetFilter(id);

            StaticExynosLog(Level::Debug, "CommonParamIntf", "[%s] operating rate : %d", __FUNCTION__, param->m.value);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));

            auto cscId = intfImpl->findFilterID("csc");
            if (cscId != 0) {
                filterParam->registTargetFilter(cscId);
            }
        }
    }

    return ret;
}

FilterParamSetterRet CommonCnv::cnvRealTimePriority(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2Component::CommonParamIntf>(intf);

    auto realTimePriority = intfImpl->getRealTimePriority();

    auto id = intfImpl->findFilterID("dec");
    if (id == 0) {
        id = intfImpl->findFilterID("enc");
    }

    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamRealTimePriority>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamRealTimePriority>>(filterParam->getBaseParam());

        param->m.value = (uint32_t)(realTimePriority) * (-1);

        filterParam->registTargetFilter(id);

        StaticExynosLog(Level::Debug, "CommonParamIntf", "[%s] RealTime priority : %d", __FUNCTION__, param->m.value);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));

        auto cscId = intfImpl->findFilterID("csc");
        if (cscId != 0) {
            filterParam->registTargetFilter(cscId);
        }
    }

    return ret;
}

FilterParamSetterRet DecCommonCnv::cnvPixelFormat(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2DecComponent::VdecCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("dec");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamPixelFormat>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamPixelFormat>>(filterParam->getBaseParam());

        param->m.format = intfImpl->getPixelFormat();

        filterParam->registTargetFilter(id);

        StaticExynosLog(Level::Essential, "VdecCommonParamIntf", "[%s] output format:0x%x", __FUNCTION__, param->m.format);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet DecCommonCnv::cnvActualFormat(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2DecComponent::VdecCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("csc");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamActualFormat>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamActualFormat>>(filterParam->getBaseParam());

        auto format = intfImpl->getActualFormat();

        param->m.format = format;

        filterParam->registTargetFilter(id);

        StaticExynosLog(Level::Essential, "VdecCommonParamIntf", "[%s] CSC output format:0x%x", __FUNCTION__, param->m.format);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet DecCommonCnv::cnvColorAspects(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2DecComponent::VdecCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("dec");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamColorAspects>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamColorAspects>>(filterParam->getBaseParam());

        auto FWCA = intfImpl->getColorAspects_l();

        ColorAspects sfAspects;

        /* mapping C2Color to CA and set it to variable */
        if (!C2Mapper::map(FWCA->range, &sfAspects.mRange)) {
            sfAspects.mRange = ColorAspects::RangeUnspecified;
        }
        if (!C2Mapper::map(FWCA->primaries, &sfAspects.mPrimaries)) {
            sfAspects.mPrimaries = ColorAspects::PrimariesUnspecified;
        }
        if (!C2Mapper::map(FWCA->transfer, &sfAspects.mTransfer)) {
            sfAspects.mTransfer = ColorAspects::TransferUnspecified;
        }
        if (!C2Mapper::map(FWCA->matrix, &sfAspects.mMatrixCoeffs)) {
            sfAspects.mMatrixCoeffs = ColorAspects::MatrixUnspecified;
        }

        if ((sfAspects.mPrimaries == ColorAspects::PrimariesUnspecified) &&
            (sfAspects.mTransfer == ColorAspects::TransferUnspecified) &&
            (sfAspects.mMatrixCoeffs == ColorAspects::MatrixUnspecified)) {
            /* invalid color aspects */
            return ret;
        }

        bool fullRange = false;

        /* convert CA on framework to ISO(bitstream) */
        ColorUtils::convertCodecColorAspectsToIsoAspects(
                sfAspects, &(param->m.primaries), &(param->m.transfer), &(param->m.matrix), &fullRange);

        param->m.range = (fullRange)? 1:2;  /* 0(UNSPECIFIED), 1(FULL), 2(LIMITED) */

        filterParam->registTargetFilter(id);

        StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] for bitstream : r(%d), p(0x%x), t(0x%x), m(0x%x)", __FUNCTION__,
                            param->m.range, param->m.primaries, param->m.transfer, param->m.matrix);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet DecCommonCnv::cnvHdrStaticInfo(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2DecComponent::VdecCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("dec");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamHdrStaticInfo>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamHdrStaticInfo>>(filterParam->getBaseParam());

        auto codedStaticInfo = intfImpl->getHdrStaticInfo(true);

        ExynosHdrStaticInfo &ST = param->m.ST;
        ST.sType1.mMaxContentLightLevel = codedStaticInfo->maxCll;  /* cd/m^2 */
        ST.sType1.mMaxFrameAverageLightLevel = codedStaticInfo->maxFall;  /* cd/m^2 */

        ST.sType1.mR.x = ((codedStaticInfo->mastering.red.x / 0.00002) + 0.5);
        ST.sType1.mR.y = ((codedStaticInfo->mastering.red.y / 0.00002) + 0.5);
        ST.sType1.mG.x = ((codedStaticInfo->mastering.green.x / 0.00002) + 0.5);
        ST.sType1.mG.y = ((codedStaticInfo->mastering.green.y / 0.00002) + 0.5);
        ST.sType1.mB.x = ((codedStaticInfo->mastering.blue.x / 0.00002) + 0.5);
        ST.sType1.mB.y = ((codedStaticInfo->mastering.blue.y / 0.00002) + 0.5);
        ST.sType1.mW.x = ((codedStaticInfo->mastering.white.x / 0.00002) + 0.5);
        ST.sType1.mW.y = ((codedStaticInfo->mastering.white.y / 0.00002) + 0.5);
        ST.sType1.mMaxDisplayLuminance = ((codedStaticInfo->mastering.maxLuminance / 0.0001) + 0.5);  /* convert 0.0001cd/m^2 to cd/m^2 */
        ST.sType1.mMinDisplayLuminance = ((codedStaticInfo->mastering.minLuminance / 0.0001) + 0.5);  /* convert 0.0001cd/m^2 to cd/m^2 */

        filterParam->registTargetFilter(id);

        StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] HDR :: static info[framework] r(%f, %f), g(%f, %f), b(%f, %f), w(%f, %f), max(%f), min(%f), cll(%f), fall(%f)", __FUNCTION__,
            codedStaticInfo->mastering.red.x, codedStaticInfo->mastering.red.y,
            codedStaticInfo->mastering.green.x, codedStaticInfo->mastering.green.y,
            codedStaticInfo->mastering.blue.x, codedStaticInfo->mastering.blue.y,
            codedStaticInfo->mastering.white.x, codedStaticInfo->mastering.white.y,
            codedStaticInfo->mastering.maxLuminance, codedStaticInfo->mastering.minLuminance,
            codedStaticInfo->maxCll, codedStaticInfo->maxFall);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet DecCommonCnv::cnvInputCrop(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2DecComponent::VdecCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("csc");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamInputCrop>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamInputCrop>>(filterParam->getBaseParam());

        auto crop = intfImpl->getInputCrop();

        param->m.crop = crop;

        StaticExynosLog(Level::Essential, "VdecCommonParamIntf", "[%s] input crop : left:%d, top:%d, crop_width:%d, crop_height:%d",
                        __FUNCTION__, crop.nLeft, crop.nTop, crop.nWidth, crop.nHeight);

        filterParam->registTargetFilter(id);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet DecCommonCnv::cnvOutputCrop(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2DecComponent::VdecCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("csc");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamOutputCrop>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamOutputCrop>>(filterParam->getBaseParam());

        auto crop = intfImpl->getOutputCrop();

        param->m.crop = crop;

        StaticExynosLog(Level::Essential, "VdecCommonParamIntf", "[%s] output crop : left:%d, top:%d, crop_width:%d, crop_height:%d",
                        __FUNCTION__, crop.nLeft, crop.nTop, crop.nWidth, crop.nHeight);

        filterParam->registTargetFilter(id);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet DecCommonCnv::cnvScaleSize(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2DecComponent::VdecCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("csc");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamOutputFrameInfo>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamOutputFrameInfo>>(filterParam->getBaseParam());

        auto size = intfImpl->getPictureSize();

        param->m.width  = size->width;
        param->m.height = size->height;

        StaticExynosLog(Level::Essential, "VdecCommonParamIntf", "[%s] scaled resolution : width:%d, height:%d",
                        __FUNCTION__, param->m.width, param->m.height);

        filterParam->registTargetFilter(id);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet DecCommonCnv::cnvDecodeOrder(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2DecComponent::VdecCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("dec");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamDecodeOrder>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamDecodeOrder>>(filterParam->getBaseParam());

        auto order = intfImpl->getDecodeOrder();

        param->m.enable = order->value;

        StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] Decode Order Decoding is %s", __FUNCTION__,
                            (param->m.enable == On)? "enabled":"disabled");

        filterParam->registTargetFilter(id);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet DecCommonCnv::cnvBufferCopy(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<ExynosC2DecComponent::VdecCommonParamIntf>(intf);

        auto id = intfImpl->findFilterID("csc");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamBufferCopy>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamBufferCopy>>(filterParam->getBaseParam());

            auto copy = intfImpl->getBufferCopy();

            param->m.enable = copy->value;

            StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] buffer copy is %s", __FUNCTION__,
                            (param->m.enable == On)? "enabled":"disabled");

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
        }

        return ret;
    }

FilterParamSetterRet DecCommonCnv::cnvBlackBar(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2DecComponent::VdecCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("dec");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamBlackBar>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamBlackBar>>(filterParam->getBaseParam());

        auto blackbar = intfImpl->getBlackBar();

        param->m.enable  = blackbar->value;

        StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] black bar is %s", __FUNCTION__,
                            (param->m.enable == On)? "enabled":"disabled");

        filterParam->registTargetFilter(id);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet DecCommonCnv::cnvThumbnailMode(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2DecComponent::VdecCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("dec");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamThumbnailMode>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamThumbnailMode>>(filterParam->getBaseParam());

        auto blackbar = intfImpl->getThumbnailMode();

        param->m.enable  = blackbar->value;

        StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] thumbnail mode is %s", __FUNCTION__,
                            (param->m.enable == On)? "enabled":"disabled");

        filterParam->registTargetFilter(id);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet DecCommonCnv::cnvCompressedColor(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2DecComponent::VdecCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("dec");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamCompressedColor>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamCompressedColor>>(filterParam->getBaseParam());

        auto pixelFormat = intfImpl->getPixelFormat();
        auto compColor = intfImpl->getCompressedColor();

        if (pixelFormat != HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED) {
            /* if required format is YV12, compressed color format could not be handled at client */
            StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] pixel format is (0x%x). compressed color will not be used", __FUNCTION__, pixelFormat);
            param->m.format = 0;  /* UNUSED */
        } else {
            switch (compColor->value) {
            case VendorC2Config::COMPRESSED_COLOR_LOSSLESS:
                param->m.format = HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC;

#ifdef USE_FLEXIBLE_P010
#ifdef USE_SUPPORT_GPU_SBWC
                param->m.format = HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_256_SBWC;
#else
                param->m.format = HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC;
#endif
#endif
                break;
            default:
                param->m.format = 0;  /* UNUSED */
                break;
            }

            StaticExynosLog(Level::Debug, "VdecCommonParamIntf", "[%s] compressed color (0x%x)", __FUNCTION__, param->m.format);
        }

        filterParam->registTargetFilter(id);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet DecCommonCnv::cnvDisplayDelay(std::shared_ptr<C2InterfaceHelper> intf, int32_t delay) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2DecComponent::VdecCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("dec");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamDisplayDelay>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamDisplayDelay>>(filterParam->getBaseParam());

        if (delay >= 0) {
            param->m.delay = delay;

            StaticExynosLog(Level::Essential, "VdecCommonParamIntf", "[%s] display delay : %d",
                            __FUNCTION__, param->m.delay);

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
        }
    }

    return ret;
}

FilterParamSetterRet DecCommonCnv::cnvImageConvertControlFrameCommon(std::shared_ptr<C2InterfaceHelper> intf, uint32_t imageConvertControlFrame) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2DecComponent::VdecCommonParamIntf>(intf);

    auto count = imageConvertControlFrame;

    if (count > 0) {
        auto id = intfImpl->findFilterID("hdr2sdr");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamHDR2SDRFrameCount>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamHDR2SDRFrameCount>>(filterParam->getBaseParam());

            param->m.value = count;

            StaticExynosLog(Level::Essential, "DecParamIntf", "[%s] image convert count(%d)", __FUNCTION__, param->m.value);

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
        }
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvSubscribedParam(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;
    ret.clear();

    if (intf.get() == nullptr) {
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    auto subscribes = intfImpl->getSubscribes();
    if (subscribes.get() == nullptr) {
        return ret;
    }

    for (size_t i = 0; i < subscribes->flexCount(); ++i) {
        auto &indices = subscribes->m.values[i];

        switch (indices) {
        case C2AndroidStreamAverageBlockQuantizationInfo::output::PARAM_TYPE:
        {
            auto id = intfImpl->findFilterID("enc");
            if (id != 0) {  /* target filter is valid */
                auto filterParam = std::make_shared<ExynosFilterParam<ParamAverageQp>>();
                auto param       = std::static_pointer_cast<ExynosParam<ParamAverageQp>>(filterParam->getBaseParam());

                param->m.enable = 1;

                StaticExynosLog(Level::Essential, "VencCommonParamIntf", "[%s] average qp is enabled", __FUNCTION__);

                filterParam->registTargetFilter(id);

                ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
           }
       }
            break;
       default:
            break;
        }
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvFrameRate(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    /* framerate */
    {
        auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

        auto id = intfImpl->findFilterID("enc");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamFramerate>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamFramerate>>(filterParam->getBaseParam());

            param->m.framerate = intfImpl->getFrameRate();

            StaticExynosLog(Level::Essential, "VencCommonParamIntf", "[%s] framerate(%d)", __FUNCTION__, param->m.framerate);

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));

            auto cscId = intfImpl->findFilterID("csc");
            if (cscId != 0) {
                filterParam->registTargetFilter(cscId);
            }
        }
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvActualFormat(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;
    ret.clear();

    if (intf.get() == nullptr) {
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("csc");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamActualFormat>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamActualFormat>>(filterParam->getBaseParam());

        auto format = intfImpl->getActualFormat();
        auto dataspace = intfImpl->getDataSpace();

        auto isCSCRequired =
            [format, dataspace]()->bool {
                if (!ExynosUtils::CheckRGB(format)) {
                    /* there is no domain conversion */
                    return false;
                }

                auto ret = true;
                auto legacy = ((dataspace & ~(HAL_DATASPACE_TRANSFER_MASK | HAL_DATASPACE_STANDARD_MASK |
                                              HAL_DATASPACE_RANGE_MASK)) != 0);
                if (legacy) {
                    switch (dataspace) {
                    case HAL_DATASPACE_BT601_625:
                        [[fallthrough]];
                    case HAL_DATASPACE_BT709:
                        ret = false;
                        break;
                    default:
                        break;
                    }
                } else {
                    auto standard = false, transfer = false;

                    switch (dataspace & HAL_DATASPACE_STANDARD_MASK) {
                    case HAL_DATASPACE_STANDARD_BT709:
                        [[fallthrough]];
                    case HAL_DATASPACE_STANDARD_BT601_625:
                        [[fallthrough]];
                    case HAL_DATASPACE_STANDARD_UNSPECIFIED:
                        standard = false;
                        break;
                    default:
                        standard = true;
                        break;
                    }

                    switch (dataspace & HAL_DATASPACE_TRANSFER_MASK) {
                    case HAL_DATASPACE_TRANSFER_SMPTE_170M:
                        [[fallthrough]];
                    case HAL_DATASPACE_TRANSFER_UNSPECIFIED:
                        transfer = false;
                        break;
                    default:
                        transfer = true;
                        break;
                    }

                    ret = (standard || transfer);
                }

                if (ret) {
                    StaticExynosLog(Level::Debug, "VencCommonParamIntf",
                                        "[%s] dataspace(0x%x) should be handled by CSC", __FUNCTION__, dataspace);
                }

                return ret;
            }();

        if (isCSCRequired) {
            format = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M;  /* need to color conversion */
        }

        param->m.format = format;

        StaticExynosLog(Level::Essential, "VencCommonParamIntf", "[%s] actual format(0x%x)", __FUNCTION__, param->m.format);

        filterParam->registTargetFilter(id);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvStartIDRFrame(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;
    ret.clear();

    if (intf.get() == nullptr) {
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    if (intfImpl->getStartIDRFrame()) {
        auto framerate  = intfImpl->getFrameRate();
        auto period     = intfImpl->getIDRPeriod();

        auto id = intfImpl->findFilterID("enc");
        if (id != 0) {  /* target filter is valid */
            /* IDR period */
            {
                auto filterParam = std::make_shared<ExynosFilterParam<ParamIDRPeriod>>();
                auto param       = std::static_pointer_cast<ExynosParam<ParamIDRPeriod>>(filterParam->getBaseParam());

                param->m.period = period;

                StaticExynosLog(Level::Essential, "VencCommonParamIntf", "[%s] IDR period(%d)", __FUNCTION__,
                                    param->m.period);

                filterParam->registTargetFilter(id);

                ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
            }

            /* framerate */
            {
                auto filterParam = std::make_shared<ExynosFilterParam<ParamFramerate>>();
                auto param       = std::static_pointer_cast<ExynosParam<ParamFramerate>>(filterParam->getBaseParam());

                param->m.framerate = framerate;

                StaticExynosLog(Level::Essential, "VencCommonParamIntf", "[%s] framerate(%.2f)", __FUNCTION__, framerate);

                filterParam->registTargetFilter(id);

                ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
            }
        }
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvRequestSyncFrame(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;
    ret.clear();

    if (intf.get() == nullptr) {
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    if (intfImpl->getRequestSyncFrame()) {
        auto id = intfImpl->findFilterID("enc");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamIntraVOPRefresh>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamIntraVOPRefresh>>(filterParam->getBaseParam());

            param->m.request = On;

            StaticExynosLog(Level::Essential, "VencCommonParamIntf", "[%s] request sync frame", __FUNCTION__);

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
        }
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvSyncFramePeriod(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;
    ret.clear();

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("enc");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamIDRPeriod>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamIDRPeriod>>(filterParam->getBaseParam());

        auto period     = intfImpl->getIDRPeriod();

        param->m.period = period;

        StaticExynosLog(Level::Essential, "VencCommonParamIntf", "[%s] IDR period(%d)", __FUNCTION__,
                            param->m.period);

        filterParam->registTargetFilter(id);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvIntraRefresh(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;
    ret.clear();

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("enc");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamIntraRefresh>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamIntraRefresh>>(filterParam->getBaseParam());

        auto intraRefresh = intfImpl->getIntraRefresh();

        if (intraRefresh->mode != C2Config::INTRA_REFRESH_DISABLED) {
            param->m.period = (uint32_t)intraRefresh->period;
        } else {
            param->m.period = 0;
        }

        StaticExynosLog(Level::Essential, "VencCommonParamIntf", "[%s] intra refresh(%f)", __FUNCTION__,
                        param->m.period);

        filterParam->registTargetFilter(id);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvInputCrop(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("csc");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamInputCrop>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamInputCrop>>(filterParam->getBaseParam());

        auto crop = intfImpl->getInputCrop();

        param->m.crop = crop;

        StaticExynosLog(Level::Essential, "VencCommonParamIntf", "[%s] input crop : left:%d, top:%d, crop_width:%d, crop_height:%d",
                        __FUNCTION__, crop.nLeft, crop.nTop, crop.nWidth, crop.nHeight);

        filterParam->registTargetFilter(id);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvScaleSize(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("csc");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamOutputFrameInfo>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamOutputFrameInfo>>(filterParam->getBaseParam());

        auto size = intfImpl->getPictureSize();

        param->m.width  = size->width;
        param->m.height = size->height;

        StaticExynosLog(Level::Essential, "VencCommonParamIntf", "[%s] scaled resolution : width:%d, height:%d",
                        __FUNCTION__, param->m.width, param->m.height);

        filterParam->registTargetFilter(id);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvColorAspects(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    struct Range {
        C2Color::range_t range;
        int value;
    };

    constexpr Range RangeMap[] = {
        { C2Color::RANGE_UNSPECIFIED, 0},
        { C2Color::RANGE_FULL, 1},
        { C2Color::RANGE_LIMITED, 2},
    };

    struct Primaries {
        C2Color::primaries_t primaries;
        int value;
    };

    constexpr Primaries PrimariesMap[] = {
        { C2Color::PRIMARIES_UNSPECIFIED, 2},
        { C2Color::PRIMARIES_BT709, 1},
        { C2Color::PRIMARIES_BT470_M, 4},
        { C2Color::PRIMARIES_BT601_625, 5},
        { C2Color::PRIMARIES_BT601_525, 6},
        { C2Color::PRIMARIES_GENERIC_FILM, 8},
        { C2Color::PRIMARIES_BT2020, 9},
        /* TBD : not supported
        { C2Color::PRIMARIES_RP431, },
        { C2Color::PRIMARIES_EG432, },
        { C2Color::PRIMARIES_EBU3213, },
        */
    };

    struct Transfer {
        C2Color::transfer_t transfer;
        int value;
    };

    constexpr Transfer TransferMap[] = {
        { C2Color::TRANSFER_UNSPECIFIED, 2},
        { C2Color::TRANSFER_LINEAR, 8},
        { C2Color::TRANSFER_SRGB, 13},
        { C2Color::TRANSFER_170M, 6},  /* BT.601, BT.709, BT.2020 */
        { C2Color::TRANSFER_GAMMA22, 4},
        { C2Color::TRANSFER_GAMMA28, 5},
        { C2Color::TRANSFER_ST2084, 16},
        { C2Color::TRANSFER_HLG, 18},
        { C2Color::TRANSFER_240M, 7},
        { C2Color::TRANSFER_XVYCC, 11},
        { C2Color::TRANSFER_BT1361, 12},
        { C2Color::TRANSFER_ST428, 17},
    };

    struct Matrix {
        C2Color::matrix_t matrix;
        int value;
    };

    constexpr Matrix MatrixMap[] = {
        { C2Color::MATRIX_UNSPECIFIED, 2},
        { C2Color::MATRIX_BT709, 1},
        /* TBD : not supported
        { C2Color::MATRIX_FCC47_73_682, 4},
        */
        { C2Color::MATRIX_BT601, 6},
        { C2Color::MATRIX_240M, 7},
        { C2Color::MATRIX_BT2020, 9},
        { C2Color::MATRIX_BT2020_CONSTANT, 10},
    };

    auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("enc");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamColorAspects>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamColorAspects>>(filterParam->getBaseParam());

        auto CA = intfImpl->getColorAspects_l();

        for (const Range &element : RangeMap) {
            if (CA->range == element.range) {
                param->m.range = element.value;
            }
        }

        for (const Primaries &element : PrimariesMap) {
            if (CA->primaries == element.primaries) {
                param->m.primaries = element.value;
            }
        }

        for (const Transfer &element : TransferMap) {
            if (CA->transfer == element.transfer) {
                param->m.transfer = element.value;
            }
        }

        for (const Matrix &element : MatrixMap) {
            if (CA->matrix == element.matrix) {
                param->m.matrix = element.value;
            }
        }

        StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] r(%d), p(0x%x), t(0x%x), m(0x%x)", __FUNCTION__,
                            param->m.range, param->m.primaries, param->m.transfer, param->m.matrix);

        filterParam->registTargetFilter(id);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvCSCDataSpace(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("csc");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamDataSpace>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamDataSpace>>(filterParam->getBaseParam());

        auto dataspace = intfImpl->getDataSpace();

        if (dataspace == HAL_DATASPACE_UNKNOWN) {
            StaticExynosLog(Level::Essential, "VencCommonParamIntf", "[%s] get default dataspace", __FUNCTION__);

            auto streamSize = intfImpl->getStreamSize();

            dataspace = ExynosUtils::GetDefaultDataSpaceIfNeeded(streamSize->width, streamSize->height);
        } else {
            auto legacy = ((dataspace & ~(HAL_DATASPACE_TRANSFER_MASK | HAL_DATASPACE_STANDARD_MASK |
                                          HAL_DATASPACE_RANGE_MASK)) != 0);
            if (!legacy) {
                /* set default values, if it is not decided */
                if ((dataspace & HAL_DATASPACE_RANGE_MASK) == HAL_DATASPACE_RANGE_UNSPECIFIED) {
                    dataspace |= HAL_DATASPACE_RANGE_LIMITED;
                    StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] set default RANGE to LIMITED", __FUNCTION__);
                }

                if ((dataspace & HAL_DATASPACE_STANDARD_MASK) == HAL_DATASPACE_STANDARD_UNSPECIFIED) {
                    switch (dataspace & HAL_DATASPACE_TRANSFER_MASK) {
                    case HAL_DATASPACE_TRANSFER_LINEAR:
                        [[fallthrough]];
                    case HAL_DATASPACE_TRANSFER_SRGB:
                        [[fallthrough]];
                    case HAL_DATASPACE_TRANSFER_GAMMA2_2:
                        [[fallthrough]];
                    case HAL_DATASPACE_TRANSFER_GAMMA2_6:
                        [[fallthrough]];
                    case HAL_DATASPACE_TRANSFER_GAMMA2_8:
                        dataspace |= HAL_DATASPACE_STANDARD_BT709;
                        StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] set default STANDARD to BT.709", __FUNCTION__);
                        break;
                    case HAL_DATASPACE_TRANSFER_ST2084:
                        [[fallthrough]];
                    case HAL_DATASPACE_TRANSFER_HLG:
                        dataspace |= HAL_DATASPACE_STANDARD_BT2020;
                        StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] set default STANDARD to BT.2020", __FUNCTION__);
                        break;
                    case HAL_DATASPACE_TRANSFER_SMPTE_170M:
                        [[fallthrough]];
                    default:
                        dataspace |= HAL_DATASPACE_STANDARD_BT601_625;
                        StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] set default STANDARD to BT.601_625", __FUNCTION__);
                        break;
                    }
                }

                if ((dataspace & HAL_DATASPACE_TRANSFER_MASK) == HAL_DATASPACE_TRANSFER_UNSPECIFIED) {
                    dataspace |= HAL_DATASPACE_TRANSFER_SMPTE_170M;
                    StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] set default TRANSFER to SMPTE170M", __FUNCTION__);
                }
            }
        }

        param->m.dataspace = dataspace;

        StaticExynosLog(Level::Essential, "VencCommonParamIntf", "[%s] dataspace : 0x%x",
                        __FUNCTION__, param->m.dataspace);

        filterParam->registTargetFilter(id);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvQpRange(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("enc");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamQpRange>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamQpRange>>(filterParam->getBaseParam());

        auto range = intfImpl->getQpRange();

        param->m.minI = range->I_minQP;
        param->m.maxI = range->I_maxQP;
        param->m.minP = range->P_minQP;
        param->m.maxP = range->P_maxQP;
        param->m.minB = range->B_minQP;
        param->m.maxB = range->B_maxQP;

        StaticExynosLog(Level::Essential, "VencCommonParamIntf", "[%s] qp range : I(%d, %d), P(%d, %d), B(%d, %d)",
                        __FUNCTION__, param->m.minI, param->m.maxI, param->m.minP, param->m.maxP, param->m.minB, param->m.maxB);

        filterParam->registTargetFilter(id);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvDropControl(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("enc");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamDropControl>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamDropControl>>(filterParam->getBaseParam());

        auto enable = intfImpl->getDropControl();

        param->m.enable = enable? 1:0;

        StaticExynosLog(Level::Essential, "VencCommonParamIntf", "[%s] drop control is %s",
                        __FUNCTION__, enable? "enabled":"disabled");

        filterParam->registTargetFilter(id);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvDynamicFramerate(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("enc");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamDynamicFramerate>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamDynamicFramerate>>(filterParam->getBaseParam());

        auto enable = intfImpl->getDynamicFramerate();

        param->m.enable = enable? 1:0;

        StaticExynosLog(Level::Essential, "VencCommonParamIntf", "[%s] dynamic framerate is %s",
                        __FUNCTION__, (param->m.enable != 0)? "enabled":"disabled");

        filterParam->registTargetFilter(id);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvBitrate(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    /* bitrate */
    {
        auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

        auto id = intfImpl->findFilterID("enc");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamBitrate>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamBitrate>>(filterParam->getBaseParam());

            param->m.bitrate = intfImpl->getBitrate();

            StaticExynosLog(Level::Essential, "EncParamIntf", "[%s] bitrate(%d)", __FUNCTION__, param->m.bitrate);

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
        }
    }

    return ret;
}

static std::shared_ptr<ExynosFilterParam<ParamBitrateMode>> makeBitrateModeParam(uint32_t mode, uint32_t id) {
    auto filterParam = std::make_shared<ExynosFilterParam<ParamBitrateMode>>();
    auto param       = std::static_pointer_cast<ExynosParam<ParamBitrateMode>>(filterParam->getBaseParam());

    switch (mode) {
    case C2Config::BITRATE_CONST:
        param->m.mode = ExynosBitrateMode::BITRATE_MODE_CONST;
        break;
    case C2Config::BITRATE_CONST_SKIP_ALLOWED:
        param->m.mode = ExynosBitrateMode::BITRATE_MODE_CONST_WITH_FRAME_SKIP;
        break;
    case C2Config::BITRATE_VARIABLE:
        param->m.mode = ExynosBitrateMode::BITRATE_MODE_VARIABLE;
        break;
    case VendorC2Config::BITRATE_VENDOR_CONST_VT_CALL:
        param->m.mode = ExynosBitrateMode::BITRATE_MODE_CONST_VT_CALL;
        break;
    case VendorC2Config::BITRATE_VENDOR_CONST_WFD:
        param->m.mode = ExynosBitrateMode::BITRATE_MODE_CONST_WFD;
        break;
    case VendorC2Config::BITRATE_VENDOR_VARIABLE_BIT_SAVE:
        param->m.mode = ExynosBitrateMode::BITRATE_MODE_VARIABLE_BIT_SAVE;
        break;
    default:
        param->m.mode = ExynosBitrateMode::BITRATE_MODE_DISABLED;
        break;
    }

    filterParam->registTargetFilter(id);

    return filterParam;
}

FilterParamSetterRet EncCommonCnv::cnvBitrateMode(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl   = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    auto mode = intfImpl->getBitrateMode();

    auto id = intfImpl->findFilterID("enc");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = makeBitrateModeParam(mode, id);
        auto param = std::static_pointer_cast<ExynosParam<ParamBitrateMode>>(filterParam->getBaseParam());

        StaticExynosLog(Level::Essential, "EncParamIntf", "[%s] bitrate mode(%d)", __FUNCTION__, param->m.mode);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvVendorBitrateMode(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl   = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    auto mode = intfImpl->getVendorBitrateMode();
    if (mode < (uint32_t)VendorC2Config::BITRATE_VENDOR_START) {
        ret.clear();
        return ret;
    }

    auto id = intfImpl->findFilterID("enc");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = makeBitrateModeParam(mode, id);
        auto param = std::static_pointer_cast<ExynosParam<ParamBitrateMode>>(filterParam->getBaseParam());

        StaticExynosLog(Level::Essential, "EncParamIntf", "[%s] final bitrate mode(%d)", __FUNCTION__, param->m.mode);
        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvLayeringCommon(std::shared_ptr<C2InterfaceHelper> intf,
                                              std::shared_ptr<C2StreamTemporalLayeringTuning::output> layering) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    if (layering->m.layerCount > 0) {
        auto id = intfImpl->findFilterID("enc");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamLayering>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamLayering>>(filterParam->getBaseParam());

            param->m.layerCount     = layering->m.layerCount;
            param->m.bLayerCount    = layering->m.bLayerCount;

            StaticExynosLog(Level::Essential, "EncParamIntf", "[%s] layering - count(total(%d), b(%d))",
                                __FUNCTION__, param->m.layerCount, param->m.bLayerCount);

            auto bitrate = intfImpl->getBitrate();

            memset(&param->m.bitrate, 0, sizeof(param->m.bitrate));

            if (layering->flexCount() > 0) {
                param->m.useAdaptiveBitrate = 0;

                param->m.bitrate[0] = bitrate * layering->m.bitrateRatios[0];

                StaticExynosLog(Level::Essential, "EncParamIntf", "[%s] layering - layer[0] : bitrate(%d)",
                                    __FUNCTION__, param->m.bitrate[0]);

                for (size_t ix = 1; ix < layering->flexCount(); ++ix) {
                    param->m.bitrate[ix] = bitrate * (layering->m.bitrateRatios[ix] - layering->m.bitrateRatios[ix - 1]);

                    StaticExynosLog(Level::Essential, "EncParamIntf", "[%s] layering - layer[%d] : bitrate(%d)",
                                        __FUNCTION__, ix, param->m.bitrate[ix]);
                }
            } else {
                param->m.useAdaptiveBitrate = 1;

                for (size_t ix = 0; ix < param->m.layerCount; ++ix) {
                    param->m.bitrate[ix] = bitrate;
                }

                StaticExynosLog(Level::Essential, "EncParamIntf", "[%s] layering - adaptive bitrate(%d)",
                                    __FUNCTION__, bitrate);
            }

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
        }
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvAverageQp(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    bool enable = intfImpl->getAverageQp();

    if (enable) {
        auto id = intfImpl->findFilterID("enc");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamAverageQp>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamAverageQp>>(filterParam->getBaseParam());

            param->m.enable = 1;

            StaticExynosLog(Level::Essential, "VencCommonParamIntf", "[%s] average qp is enabled", __FUNCTION__);

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
        }
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvMinQuality(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;
    ret.clear();

    if (intf.get() == nullptr) {
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    uint32_t level = intfImpl->getMinQuality();

    StaticExynosLog(Level::Essential, "VencCommonParamIntf", "[%s] min quality is %d", __FUNCTION__, level);
    if (level > 0) {
        auto id = intfImpl->findFilterID("enc");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamMinQuality>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamMinQuality>>(filterParam->getBaseParam());

            param->m.level = level;

            StaticExynosLog(Level::Essential, "VencCommonParamIntf", "[%s] min quality is %d", __FUNCTION__, param->m.level);

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
        }
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvPMV(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;
    ret.clear();

    if (intf.get() == nullptr) {
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    auto pmv = intfImpl->getPMV();

    if ((pmv.get() != nullptr) &&
        (pmv->Mode != VendorC2Config::PMV_DISABLED)) {
        auto id = intfImpl->findFilterID("enc");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamPMV>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamPMV>>(filterParam->getBaseParam());

            param->m.mode   = pmv->Mode;
            param->m.hor_L0 = pmv->HorizonL0;
            param->m.hor_L1 = pmv->HorizonL1;
            param->m.ver_L0 = pmv->VerticalL0;
            param->m.ver_L1 = pmv->VerticalL1;

            StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] PMV mode is %d", __FUNCTION__, param->m.mode);

            if (param->m.mode == VendorC2Config::PMV_GLOBAL) {
                StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] Global PMV horizon(L0:%d, L1:%d)", __FUNCTION__,
                                    param->m.hor_L0, param->m.hor_L1);
                StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] Global PMV vertical(L0:%d, L1:%d)", __FUNCTION__,
                                    param->m.ver_L0, param->m.ver_L1);
            }

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
        }
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvStreamSize(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl   = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);
    auto size       = intfImpl->getStreamSize();

    StaticExynosLog(Level::Essential, "VencCommonParamIntf", "[%s] stream size(%d x %d)", __FUNCTION__, size->width, size->height);

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvPrependHeaderMode(std::shared_ptr<C2InterfaceHelper> intf,
                                        std::shared_ptr<C2PrependHeaderModeSetting> mode) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("enc");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamPrependHeaderMode>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamPrependHeaderMode>>(filterParam->getBaseParam());

        if (mode->value == C2Config::PREPEND_HEADER_TO_ALL_SYNC) {
            param->m.mode = PREPEND_HEADER_MODE_SPS_PPS_TO_IDR;
        } else {
            param->m.mode = PREPEND_HEADER_MODE_NONE;
        }

        StaticExynosLog(Level::Essential, "VencCommonParamIntf", "[%s] prepend header mode(0x%x)", __FUNCTION__, param->m.mode);

        filterParam->registTargetFilter(id);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvRefPframes(std::shared_ptr<C2InterfaceHelper> intf, uint32_t refPframes) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("enc");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamRefPframes>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamRefPframes>>(filterParam->getBaseParam());

        param->m.pframes = refPframes;

        StaticExynosLog(Level::Essential, "VencCommonParamIntf", "[%s] Ref. P frame(%d)", __FUNCTION__, param->m.pframes);

        filterParam->registTargetFilter(id);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvIFrameRatio(std::shared_ptr<C2InterfaceHelper> intf, uint32_t ratio) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("enc");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamIFrameRatio>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamIFrameRatio>>(filterParam->getBaseParam());

        param->m.value = ratio;

        StaticExynosLog(Level::Essential, "VencCommonParamIntf", "[%s] I frame ratio(%d)", __FUNCTION__, param->m.value);

        filterParam->registTargetFilter(id);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvChromaQpOffset(std::shared_ptr<C2InterfaceHelper> intf,
                                std::shared_ptr<C2ExynosStreamChromaQpOffsetSetting::output> offset) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    if (offset.get() != nullptr) {
        auto id = intfImpl->findFilterID("enc");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamChromaQpOffset>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamChromaQpOffset>>(filterParam->getBaseParam());

            param->m.cb = offset->Cb;
            param->m.cr = offset->Cr;

            StaticExynosLog(Level::Essential, "VencCommonParamIntf", "[%s] chroma qp offset(cb:%d, cr:%d)",
                                __FUNCTION__, param->m.cb, param->m.cr);

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
        }
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvGop(std::shared_ptr<C2InterfaceHelper> intf, std::shared_ptr<C2StreamGopTuning::output> gop) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("enc");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamBFrame>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamBFrame>>(filterParam->getBaseParam());

        ParseGop(*gop, nullptr, nullptr, &(param->m.bframes));

        StaticExynosLog(Level::Essential, "VencCommonParamIntf", "[%s] B frame(%d)", __FUNCTION__, param->m.bframes);

        filterParam->registTargetFilter(id);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvHdrFormat(std::shared_ptr<C2InterfaceHelper> intf) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    uint32_t type = intfImpl->getHdrFormat();

    if ((type == C2Config::hdr_format_t::HDR10) ||
        (type == C2Config::hdr_format_t::HDR10_PLUS)) {
        auto id = intfImpl->findFilterID("enc");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamHdrEncoding>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamHdrEncoding>>(filterParam->getBaseParam());

            if ((C2Config::hdr_format_t)type == C2Config::hdr_format_t::HDR10) {
                param->m.type = HDR_ENCODING_HDR10;
            } else {
                param->m.type = HDR_ENCODING_HDR10_PLUS;
            }

            StaticExynosLog(Level::Essential, "VencCommonParamIntf", "[%s] hdr type : 0x%x", __FUNCTION__, param->m.type);

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
        }
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvHdrStaticInfo(std::shared_ptr<C2InterfaceHelper> intf,
                                                    std::shared_ptr<C2StreamHdrStaticInfo::input> hdrStaticInfo) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    if (hdrStaticInfo->maxFall == 65536) {
        return ret;
    }

    auto id = intfImpl->findFilterID("enc");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamHdrStaticInfo>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamHdrStaticInfo>>(filterParam->getBaseParam());

        ExynosHdrStaticInfo &ST = param->m.ST;
        ST.sType1.mMaxContentLightLevel = hdrStaticInfo->maxCll;  /* cd/m^2 */
        ST.sType1.mMaxFrameAverageLightLevel = hdrStaticInfo->maxFall;  /* cd/m^2 */

        ST.sType1.mR.x = ((hdrStaticInfo->mastering.red.x / 0.00002) + 0.5);
        ST.sType1.mR.y = ((hdrStaticInfo->mastering.red.y / 0.00002) + 0.5);
        ST.sType1.mG.x = ((hdrStaticInfo->mastering.green.x / 0.00002) + 0.5);
        ST.sType1.mG.y = ((hdrStaticInfo->mastering.green.y / 0.00002) + 0.5);
        ST.sType1.mB.x = ((hdrStaticInfo->mastering.blue.x / 0.00002) + 0.5);
        ST.sType1.mB.y = ((hdrStaticInfo->mastering.blue.y / 0.00002) + 0.5);
        ST.sType1.mW.x = ((hdrStaticInfo->mastering.white.x / 0.00002) + 0.5);
        ST.sType1.mW.y = ((hdrStaticInfo->mastering.white.y / 0.00002) + 0.5);
        ST.sType1.mMaxDisplayLuminance = ((hdrStaticInfo->mastering.maxLuminance / 0.0001) + 0.5);  /* convert 0.0001cd/m^2 to cd/m^2 */
        ST.sType1.mMinDisplayLuminance = ((hdrStaticInfo->mastering.minLuminance / 0.0001) + 0.5);  /* convert 0.0001cd/m^2 to cd/m^2 */

        filterParam->registTargetFilter(id);

        StaticExynosLog(Level::Debug, "VencCommonParamIntf",
            "[%s] HDR :: static info[framework] r(%f, %f), g(%f, %f), b(%f, %f), w(%f, %f), max(%f), min(%f), cll(%f), fall(%f)",
            __FUNCTION__,
            hdrStaticInfo->mastering.red.x, hdrStaticInfo->mastering.red.y,
            hdrStaticInfo->mastering.green.x, hdrStaticInfo->mastering.green.y,
            hdrStaticInfo->mastering.blue.x, hdrStaticInfo->mastering.blue.y,
            hdrStaticInfo->mastering.white.x, hdrStaticInfo->mastering.white.y,
            hdrStaticInfo->mastering.maxLuminance, hdrStaticInfo->mastering.minLuminance,
            hdrStaticInfo->maxCll, hdrStaticInfo->maxFall);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvHdrDynamicInfo(std::shared_ptr<C2InterfaceHelper> intf,
                                                     std::shared_ptr<C2StreamHdrDynamicMetadataInfo::input> hdrDynamicInfo) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("enc");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamHdrDynamicInfo>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamHdrDynamicInfo>>(filterParam->getBaseParam());

        ExynosHdrDynamicInfo &DY = param->m.DY;

        DY.valid = 1;
        auto err = Exynos_parsing_user_data_registered_itu_t_t35(&DY, (char *)hdrDynamicInfo->m.data);
        if (err < 0) {
            StaticExynosLog(Level::Error, "VencCommonParamIntf",
                            "[%s] converting HDR dynamic info(size:%zu) is failed", __FUNCTION__, hdrDynamicInfo->flexCount());
            return ret;
        }

        filterParam->registTargetFilter(id);

        StaticExynosLog(Level::Debug, "VencCommonParamIntf",
                        "[%s] HDR :: dynamic info (size:%zu)", __FUNCTION__, hdrDynamicInfo->flexCount());

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}

FilterParamSetterRet EncCommonCnv::cnvMaxIFrameSize(std::shared_ptr<C2InterfaceHelper> intf, int size) {
    FilterParamSetterRet ret;

    if (intf.get() == nullptr) {
        ret.clear();
        return ret;
    }

    auto intfImpl = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(intf);

    auto id = intfImpl->findFilterID("enc");
    if (id != 0) {  /* target filter is valid */
        auto filterParam = std::make_shared<ExynosFilterParam<ParamMaxIFrameSize>>();
        auto param       = std::static_pointer_cast<ExynosParam<ParamMaxIFrameSize>>(filterParam->getBaseParam());

        param->m.size = size;
        filterParam->registTargetFilter(id);

        StaticExynosLog(Level::Debug, "VencCommonParamIntf", "[%s] Max I Frame Size : %d", __FUNCTION__, size);

        ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
    }

    return ret;
}
