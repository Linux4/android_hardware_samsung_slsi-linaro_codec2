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
#ifndef EXYNOS_C2_FILTER_CNV_H
#define EXYNOS_C2_FILTER_CNV_H

#include <list>
#include <memory>

#include <C2Config.h>
#include <util/C2InterfaceHelper.h>
#include "Exynos_C2_Utils.h"


typedef std::list<std::shared_ptr<ExynosFilterParamBase>> FilterParamSetterRet;
typedef std::function<FilterParamSetterRet (std::shared_ptr<C2InterfaceHelper> intf)> FilterParamSetterFunc;

typedef struct {
    FilterParamSetterFunc   func;
    std::list<uint32_t>     deps;
} FilterParamSetterInfo;

/* generate filter param */
/* Common Cnv */
namespace CommonCnv {
    FilterParamSetterRet cnvOperateRate(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvRealTimePriority(std::shared_ptr<C2InterfaceHelper> intf);
}; // namespace CommonCnv

/* Decode Common Cnv */
namespace DecCommonCnv {
    FilterParamSetterRet cnvPixelFormat(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvActualFormat(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvColorAspects(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvHdrStaticInfo(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvInputCrop(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvOutputCrop(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvScaleSize(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvDecodeOrder(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvBufferCopy(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvBlackBar(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvThumbnailMode(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvCompressedColor(std::shared_ptr<C2InterfaceHelper> intf);

    FilterParamSetterRet cnvDisplayDelay(std::shared_ptr<C2InterfaceHelper> intf, int32_t delay);
    FilterParamSetterRet cnvImageConvertControlFrameCommon(std::shared_ptr<C2InterfaceHelper> intf,
                                                                  uint32_t imageConvertControlFrame);
}; // namespace DecCommonCnv

/* Encode Common Cnv */
namespace EncCommonCnv {
    FilterParamSetterRet cnvSubscribedParam(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvFrameRate(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvActualFormat(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvStartIDRFrame(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvRequestSyncFrame(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvSyncFramePeriod(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvIntraRefresh(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvInputCrop(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvScaleSize(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvColorAspects(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvCSCDataSpace(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvQpRange(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvDropControl(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvDynamicFramerate(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvBitrate(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvAverageQp(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvMinQuality(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvPMV(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvStreamSize(std::shared_ptr<C2InterfaceHelper> intf);

    FilterParamSetterRet cnvBitrateMode(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvVendorBitrateMode(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvLayeringCommon(std::shared_ptr<C2InterfaceHelper> intf,
                                                  std::shared_ptr<C2StreamTemporalLayeringTuning::output> layering);
    FilterParamSetterRet cnvPrependHeaderMode(std::shared_ptr<C2InterfaceHelper> intf,
                                            std::shared_ptr<C2PrependHeaderModeSetting> mode);
    FilterParamSetterRet cnvRefPframes(std::shared_ptr<C2InterfaceHelper> intf, uint32_t refPframes);
    FilterParamSetterRet cnvIFrameRatio(std::shared_ptr<C2InterfaceHelper> intf, uint32_t ratio);
    FilterParamSetterRet cnvChromaQpOffset(std::shared_ptr<C2InterfaceHelper> intf,
                                                  std::shared_ptr<C2ExynosStreamChromaQpOffsetSetting::output> offset);
    FilterParamSetterRet cnvGop(std::shared_ptr<C2InterfaceHelper> intf,
                                       std::shared_ptr<C2StreamGopTuning::output> gop);
    FilterParamSetterRet cnvHdrFormat(std::shared_ptr<C2InterfaceHelper> intf);
    FilterParamSetterRet cnvHdrStaticInfo(std::shared_ptr<C2InterfaceHelper> intf,
                                            std::shared_ptr<C2StreamHdrStaticInfo::input> hdrStaticInfo);
    FilterParamSetterRet cnvHdrDynamicInfo(std::shared_ptr<C2InterfaceHelper> intf,
                                           std::shared_ptr<C2StreamHdrDynamicMetadataInfo::input> hdrDynamicInfo);
    FilterParamSetterRet cnvMaxIFrameSize(std::shared_ptr<C2InterfaceHelper> intf, int size);
}; // namespace EncCommonCnv

#endif // EXYNOS_C2_FILTER_CNV_H

