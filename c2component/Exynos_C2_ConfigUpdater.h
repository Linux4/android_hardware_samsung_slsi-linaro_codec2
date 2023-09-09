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
#ifndef EXYNOS_C2_CONFIG_UPDATER_H
#define EXYNOS_C2_CONFIG_UPDATER_H

#include <memory>
#include <vector>

#include <C2Config.h>
#include <util/C2InterfaceHelper.h>
#include <system/graphics.h>

#include "exynos_format.h"
#include "Exynos_C2_Utils.h"

#include "Exynos_C2_DecComponent.h"

namespace StaticUpdateC2Config {
    bool updateC2Config_StreamSize(
        std::shared_ptr<ExynosC2Component::CommonParamIntf> paramIntf,
        std::vector<std::unique_ptr<C2Param>>  &outConfig,
        std::shared_ptr<ExynosBuffer>           buffer,
        bool                                    forceUpdate) {
        if (!CHECK_SHARED_PTR(buffer)) {
            return false;
        }

        if (!CHECK_SHARED_PTR(paramIntf)) {
            return false;
        }

        auto vdecIntf = std::static_pointer_cast<ExynosC2DecComponent::VdecCommonParamIntf>(paramIntf);

        std::shared_ptr<C2StreamPictureSizeInfo::output> streamSize = nullptr;
        {
            ExynosC2DecComponent::VdecCommonParamIntf::Lock lock = vdecIntf->lock();
            streamSize = vdecIntf->getStreamSize();
        }

        if (streamSize.get() == nullptr) {
            StaticExynosLog(Level::Trace, "updateC2Config", "[%s] obj is released", __FUNCTION__);
            return false;
        }

        if (forceUpdate ||
            ((streamSize->width  != buffer->mImageInfo.nWidth) ||
             (streamSize->height != buffer->mImageInfo.nHeight))) {
            StaticExynosLog(Level::Info, "updateC2Config", "[%s] picture(%d x %d) vs frame(%d x %d)", __FUNCTION__,
                            streamSize->width, streamSize->height,
                            buffer->mImageInfo.nWidth, buffer->mImageInfo.nHeight);

            C2StreamPictureSizeInfo::output size(0u, buffer->mImageInfo.nWidth, buffer->mImageInfo.nHeight);
            std::vector<std::unique_ptr<C2SettingResult>> failures;

            c2_status_t err = vdecIntf->config({&size}, C2_MAY_BLOCK, &failures);
            if (err == C2_OK) {
                outConfig.emplace_back(C2Param::Copy(size));
                return true;
            } else {
                StaticExynosLog(Level::Error, "updateC2Config", "[%s] config(C2StreamPictureSizeInfo) is failed", __FUNCTION__);
                return false;
            }
        }

        return false;
    }

    void updateC2Config_MaxStreamSize(
        std::shared_ptr<ExynosC2Component::CommonParamIntf> paramIntf,
        std::vector<std::unique_ptr<C2Param>>  &outConfig,
        std::shared_ptr<ExynosBuffer>           buffer) {
        if (!CHECK_SHARED_PTR(buffer)) {
            return;
        }

        if (!CHECK_SHARED_PTR(paramIntf)) {
            return;
        }

        auto vdecIntf = std::static_pointer_cast<ExynosC2DecComponent::VdecCommonParamIntf>(paramIntf);

        std::shared_ptr<C2StreamMaxPictureSizeTuning::output> maxStreamSize = nullptr;
        {
            ExynosC2DecComponent::VdecCommonParamIntf::Lock lock = vdecIntf->lock();
            maxStreamSize = vdecIntf->getMaxStreamSize();
        }

        if (maxStreamSize.get() == nullptr) {
            StaticExynosLog(Level::Trace, "updateC2Config", "[%s] obj is released", __FUNCTION__);
            return;
        }

        StaticExynosLog(Level::Info, "updateC2Config", "[%s] update to max picture(%d x %d)", __FUNCTION__,
                        maxStreamSize->width, maxStreamSize->height);

        outConfig.emplace_back(C2Param::Copy(*(maxStreamSize.get())));

        return;
    }

    void updateC2Config_CropInfo(
        std::shared_ptr<ExynosC2Component::CommonParamIntf> paramIntf,
        std::vector<std::unique_ptr<C2Param>>  &outConfig,
        std::shared_ptr<ExynosBuffer>           buffer) {
        if (!CHECK_SHARED_PTR(buffer)) {
            return;
        }

        if (!CHECK_SHARED_PTR(paramIntf)) {
            return;
        }

        auto vdecIntf = std::static_pointer_cast<ExynosC2DecComponent::VdecCommonParamIntf>(paramIntf);

        std::shared_ptr<C2StreamCropRectInfo::output> streamCropInfo = nullptr;
        {
            ExynosC2DecComponent::VdecCommonParamIntf::Lock lock = vdecIntf->lock();
            streamCropInfo = vdecIntf->getStreamCropInfo();
        }

        if (streamCropInfo.get() == nullptr) {
            StaticExynosLog(Level::Trace, "updateC2Config", "[%s] obj is released", __FUNCTION__);
            return;
        }

        if ((streamCropInfo->width != buffer->mImageInfo.stCropInfo.nWidth) ||
            (streamCropInfo->height != buffer->mImageInfo.stCropInfo.nHeight) ||
            (streamCropInfo->left != buffer->mImageInfo.stCropInfo.nLeft) ||
            (streamCropInfo->top != buffer->mImageInfo.stCropInfo.nTop)) {
            StaticExynosLog(Level::Info, "updateC2Config", "[%s] picture(l:%d, t:%d, w:%d, h:%d) vs frame(l:%d, t:%d, w:%d, h:%d)", __FUNCTION__,
                            streamCropInfo->left, streamCropInfo->top, streamCropInfo->width, streamCropInfo->height,
                            buffer->mImageInfo.stCropInfo.nLeft, buffer->mImageInfo.stCropInfo.nTop,
                            buffer->mImageInfo.stCropInfo.nWidth, buffer->mImageInfo.stCropInfo.nHeight);

            C2Rect crop = C2Rect(buffer->mImageInfo.stCropInfo.nWidth, buffer->mImageInfo.stCropInfo.nHeight).
                                 at(buffer->mImageInfo.stCropInfo.nLeft, buffer->mImageInfo.stCropInfo.nTop);

            C2StreamCropRectInfo::output rect(0, crop);
            std::vector<std::unique_ptr<C2SettingResult>> failures;

            c2_status_t err = vdecIntf->config({&rect}, C2_MAY_BLOCK, &failures);
            if (err == C2_OK) {
                outConfig.emplace_back(C2Param::Copy(rect));
            } else {
                StaticExynosLog(Level::Error, "updateC2Config", "[%s] config(C2StreamCropRectInfo) is failed", __FUNCTION__);
                return;
            }
        }

        return;
    }

    void updateC2Config_OutputDelay(
        std::shared_ptr<ExynosC2Component::CommonParamIntf> paramIntf,
        std::vector<std::unique_ptr<C2Param>>  &outConfig,
        std::shared_ptr<ExynosFilterParams>    filterParams,
        uint32_t                               &minOutputDelay,
        bool                                    forceUpdate) {
        if (!CHECK_SHARED_PTR(filterParams)) {
            return;
        }

        if (!CHECK_SHARED_PTR(paramIntf)) {
            return;
        }

        auto vdecIntf = std::static_pointer_cast<ExynosC2DecComponent::VdecCommonParamIntf>(paramIntf);

        auto filterParam = filterParams->getParam(ExynosParamIndex::OutputDelayIndex, TARGET_OWNER_COMPONENT);
        if (filterParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamOutputDelay>>(filterParam->getBaseParam());

            if (forceUpdate ||
                (minOutputDelay != param->m.delay)) {
                /* update Max DPB num */
                {
                    C2ExynosStreamMaxDPBNumInfo::output maxDPBNum(0u, param->m.delay);
                    std::vector<std::unique_ptr<C2SettingResult>> failures;

                    c2_status_t err = vdecIntf->config({&maxDPBNum}, C2_MAY_BLOCK, &failures);
                    if (err != C2_OK) {
                        StaticExynosLog(Level::Error, "updateC2Config", "[%s] config(C2ExynosStreamMaxDPBNumInfo) is failed", __FUNCTION__);
                        return;
                    }

                    minOutputDelay = param->m.delay;
                }

                C2PortActualDelayTuning::output outputDelay(minOutputDelay);
                std::vector<std::unique_ptr<C2SettingResult>> failures;

                c2_status_t err = vdecIntf->config({&outputDelay}, C2_MAY_BLOCK, &failures);
                if (err != C2_OK) {
                    StaticExynosLog(Level::Error, "updateC2Config", "[%s] config(C2PortActualDelayTuning) is failed", __FUNCTION__);
                    return;
                }

                {
                    std::vector<std::unique_ptr<C2Param>> params;
                    c2_status_t err = vdecIntf->query({}, { C2PortActualDelayTuning::output::PARAM_TYPE,
                                                             C2PortReorderKeySetting::output::PARAM_TYPE,
                                                           },
                                                      C2_MAY_BLOCK, &params);
                    if (err == C2_OK) {
                        for (int i = 0; i < params.size(); i++) {
                            switch (params[i]->coreIndex().coreIndex()) {
                            case C2PortActualDelayTuning::CORE_INDEX:
                            {
                                StaticExynosLog(Level::Info, "updateC2Config", "[%s] output delay(%d)", __FUNCTION__,
                                    C2PortActualDelayTuning::output::From(params[i].get())->value);
                            }
                                break;
                            case C2PortReorderKeySetting::CORE_INDEX:
                            {
                                StaticExynosLog(Level::Info, "updateC2Config", "[%s] reorder key(%d)", __FUNCTION__,
                                    C2PortReorderKeySetting::output::From(params[i].get())->value);
                            }
                                break;
                            default:
                                break;
                            }

                            outConfig.emplace_back(C2Param::Copy(*(params[i].get())));
                        }
                    }
                }
            }
        }

        return;
    }

    void updateC2Config_ExtraBufferNum(
        std::shared_ptr<ExynosC2Component::CommonParamIntf> paramIntf,
        std::vector<std::unique_ptr<C2Param>>  &outConfig,
        std::shared_ptr<ExynosBuffer>           buffer) {
        if (!CHECK_SHARED_PTR(buffer)) {
            return;
        }

        if (!CHECK_SHARED_PTR(paramIntf)) {
            return;
        }

        auto vdecIntf = std::static_pointer_cast<ExynosC2DecComponent::VdecCommonParamIntf>(paramIntf);

        auto filterParams = std::static_pointer_cast<ExynosFilterParams>(buffer->mParams);

        auto filterParam = filterParams->getParam(ExynosParamIndex::EnabledFilterNumIndex, TARGET_OWNER_COMPONENT);
        if (filterParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamEnabledFilterNum>>(filterParam->getBaseParam());

            {
                C2ExynosPortExtraBufferNumInfo::output extBufferNum((param->m.num * EXTRA_INTERNAL_BUFFER_NUM));
                std::vector<std::unique_ptr<C2SettingResult>> failures;

                c2_status_t err = vdecIntf->config({&extBufferNum}, C2_MAY_BLOCK, &failures);
                if (err != C2_OK) {
                    StaticExynosLog(Level::Error, "updateC2Config", "[%s] config(C2ExynosPortExtraBufferNumInfo) is failed", __FUNCTION__);
                    return;
                }
            }

            {
                std::vector<std::unique_ptr<C2Param>> params;
                c2_status_t err = vdecIntf->query({}, { C2ExynosPortExtraBufferNumInfo::output::PARAM_TYPE,
                                                         C2PortActualDelayTuning::output::PARAM_TYPE,
                                                       },
                                                  C2_MAY_BLOCK, &params);
                if (err == C2_OK) {
                    for (int i = 0; i < params.size(); i++) {
                        switch (params[i]->coreIndex().coreIndex()) {
                        case C2ExynosPortExtraBufferNumInfo::CORE_INDEX:
                        {
                            StaticExynosLog(Level::Info, "updateC2Config", "[%s] extra buffer num(%d)", __FUNCTION__,
                                C2ExynosPortExtraBufferNumInfo::output::From(params[i].get())->value);
                        }
                            break;
                        case C2PortActualDelayTuning::CORE_INDEX:
                        {
                            StaticExynosLog(Level::Info, "updateC2Config", "[%s] output delay(%d)", __FUNCTION__,
                                C2PortActualDelayTuning::output::From(params[i].get())->value);

                            /* if output delay is already updated, remove old value */
                            for (auto it = outConfig.begin(); it != outConfig.end(); it++) {
                                if ((*it)->type() == C2PortActualDelayTuning::output::PARAM_TYPE) {
                                    it = outConfig.erase(it);
                                }
                            }
                        }
                            break;
                        default:
                            break;
                        }

                        outConfig.emplace_back(C2Param::Copy(*(params[i].get())));
                    }
                }
            }
        }

        return;
    }

    void updateC2Config_BlackBarInfo(
        std::shared_ptr<ExynosC2Component::CommonParamIntf> paramIntf,
        std::vector<std::unique_ptr<C2Param>>  &outConfig,
        std::shared_ptr<ExynosBuffer>           buffer) {
        if (!CHECK_SHARED_PTR(buffer)) {
            return;
        }

        if (!CHECK_SHARED_PTR(paramIntf)) {
            return;
        }

        auto vdecIntf = std::static_pointer_cast<ExynosC2DecComponent::VdecCommonParamIntf>(paramIntf);

        auto filterParams = std::static_pointer_cast<ExynosFilterParams>(buffer->mParams);

        auto filterParam = filterParams->getParam(ExynosParamIndex::BlackBarInfoIndex, TARGET_OWNER_COMPONENT);
        if (filterParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamBlackBarInfo>>(filterParam->getBaseParam());

            C2ExynosStreamBlackBarInfo::output blackBarInfo(0u, C2Rect(0, 0));
            std::vector<std::unique_ptr<C2SettingResult>> failures;

            blackBarInfo.width  = param->m.rect.nWidth;
            blackBarInfo.height = param->m.rect.nHeight;
            blackBarInfo.left   = param->m.rect.nLeft;
            blackBarInfo.top    = param->m.rect.nTop;

            c2_status_t err = vdecIntf->config({&blackBarInfo}, C2_MAY_BLOCK, &failures);
            if (err != C2_OK) {
                StaticExynosLog(Level::Error, "updateC2Config", "[%s] config(C2ExynosStreamBlackBarInfo) is failed", __FUNCTION__);
                return;
            }

            outConfig.emplace_back(C2Param::Copy(blackBarInfo));

            StaticExynosLog(Level::Info, "updateC2Config", "[%s] black bar info is (t:%d, l:%d, w:%d, h:%d)", __FUNCTION__,
                        param->m.rect.nTop, param->m.rect.nLeft, param->m.rect.nWidth, param->m.rect.nHeight);
        }

        return;
    }

    void updateC2Config_ReorderDepth(
        std::shared_ptr<ExynosC2Component::CommonParamIntf> paramIntf,
        std::vector<std::unique_ptr<C2Param>>  &outConfig,
        std::shared_ptr<ExynosParams>           params) {
        if (!CHECK_SHARED_PTR(params)) {
            return;
        }

        if (!CHECK_SHARED_PTR(paramIntf)) {
            return;
        }

        auto vdecIntf = std::static_pointer_cast<ExynosC2DecComponent::VdecCommonParamIntf>(paramIntf);

        auto filterParams = std::static_pointer_cast<ExynosFilterParams>(params);

        auto filterParam = filterParams->getParam(ExynosParamIndex::OutputDelayIndex, TARGET_OWNER_COMPONENT);
        if (filterParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamOutputDelay>>(filterParam->getBaseParam());

            std::shared_ptr<C2ExynosPortDecodeOrderSetting::output> decodeOrder = nullptr;
            {
                ExynosC2DecComponent::VdecCommonParamIntf::Lock lock = vdecIntf->lock();

                decodeOrder = vdecIntf->getDecodeOrder();
            }

            std::vector<C2Param::Index> indices;

            /* in decode order decoding, update reorder depth */
            if (decodeOrder == nullptr) {
                return;
            }

            if (decodeOrder->value == On) {
                C2PortReorderBufferDepthTuning::output reorderDepth(param->m.delay);

                std::vector<std::unique_ptr<C2SettingResult>> failures;

                c2_status_t err = vdecIntf->config({&reorderDepth}, C2_MAY_BLOCK, &failures);
                if (err != C2_OK) {
                    StaticExynosLog(Level::Error, "updateC2Config", "[%s] config(C2PortReorderBufferDepthTuning) is failed", __FUNCTION__);
                    return;
                }

                indices.push_back(C2PortReorderBufferDepthTuning::output::PARAM_TYPE);
            }

            {
                std::vector<std::unique_ptr<C2Param>> params;
                c2_status_t err = vdecIntf->query({}, indices, C2_MAY_BLOCK, &params);
                if (err == C2_OK) {
                    for (int i = 0; i < params.size(); i++) {
                        switch (params[i]->coreIndex().coreIndex()) {
                        case C2PortReorderBufferDepthTuning::CORE_INDEX:
                        {
                            StaticExynosLog(Level::Info, "updateC2Config", "[%s] reorder depth(%d)", __FUNCTION__,
                                C2PortReorderBufferDepthTuning::output::From(params[i].get())->value);
                        }
                            break;
                        default:
                            break;
                        }

                        outConfig.emplace_back(C2Param::Copy(*(params[i].get())));
                    }
                }
            }
        }
    }

    void updateC2Buffer_ColorAspects(
        std::shared_ptr<ExynosC2Component::CommonParamIntf> paramIntf,
        std::shared_ptr<C2Buffer>               c2buffer,
        std::shared_ptr<ExynosBuffer>           buffer) {
        if ((!CHECK_SHARED_PTR(c2buffer)) ||
            (!CHECK_SHARED_PTR(buffer))) {
            return;
        }

        if (!CHECK_SHARED_PTR(paramIntf)) {
            return;
        }

        auto vdecIntf = std::static_pointer_cast<ExynosC2DecComponent::VdecCommonParamIntf>(paramIntf);

        if (buffer->mImageInfo.stHDRInfo.eChangedInfo & VIDEO_INFO_TYPE_COLOR_ASPECTS) {
            ExynosColorAspects &CA = buffer->mImageInfo.stHDRInfo.sColorAspects;

            C2StreamColorAspectsInfo::input codedAspects = { 0u };
            ColorAspects sfAspects;

            /* convert ISO(bitstream) to CA on framework */
            ColorUtils::convertIsoColorAspectsToCodecAspects(
                    CA.mPrimaries, CA.mTransfer, CA.mMatrixCoeffs,
                    (CA.mRange == 1)? true:false, sfAspects);

            /* mapping CA to C2Color and set it to variable */
            if (!C2Mapper::map(sfAspects.mRange, &codedAspects.range)) {
                codedAspects.range = C2Color::RANGE_UNSPECIFIED;
            }
            if (!C2Mapper::map(sfAspects.mPrimaries, &codedAspects.primaries)) {
                codedAspects.primaries = C2Color::PRIMARIES_UNSPECIFIED;
            }
            if (!C2Mapper::map(sfAspects.mTransfer, &codedAspects.transfer)) {
                codedAspects.transfer = C2Color::TRANSFER_UNSPECIFIED;
            }
            if (!C2Mapper::map(sfAspects.mMatrixCoeffs, &codedAspects.matrix)) {
                codedAspects.matrix = C2Color::MATRIX_UNSPECIFIED;
            }

            std::vector<std::unique_ptr<C2SettingResult>> failures;

            /* update coded color aspects(C2_PARAMKEY_VUI_COLOR_ASPECTS),
             * color aspects(C2_PARAMKEY_COLOR_ASPECTS) will be decided based on default and coded value
             */
            auto err = paramIntf->config({ &codedAspects }, C2_MAY_BLOCK, &failures);
            if (err == C2_OK) {
                /* set color aspects(C2_PARAMKEY_COLOR_ASPECTS) to c2buffer as C2Info */
                ExynosC2DecComponent::VdecCommonParamIntf::Lock lock = vdecIntf->lock();

                auto aspects = vdecIntf->getColorAspects_l();

                StaticExynosLog(Level::Info, "updateC2Config", "[%s] HDR :: color aspects(r:%d, p:0x%x, t:0x%x, m:0x%x)", __FUNCTION__,
                            aspects->range, aspects->primaries, aspects->transfer, aspects->matrix);

                c2buffer->setInfo(aspects);
            } else {
                StaticExynosLog(Level::Error, "updateC2Config", "[%s] config(C2StreamColorAspectsInfo) is failed", __FUNCTION__);
                return;
            }
        }
    }

    void updateC2Buffer_HDRInfo(
        std::shared_ptr<ExynosC2Component::CommonParamIntf> paramIntf,
        std::shared_ptr<C2Buffer> c2buffer,
        std::shared_ptr<ExynosBuffer> buffer) {
        if ((!CHECK_SHARED_PTR(paramIntf)) ||
            (!CHECK_SHARED_PTR_NOLOG(c2buffer)) ||
            (!CHECK_SHARED_PTR(buffer))) {
            return;
        }

        auto queryFunc = [paramIntf](std::vector<C2Param::Index> indices,
                                     std::vector<std::unique_ptr<C2Param>> &params)->bool {
                                return paramIntf->query({}, indices, C2_MAY_BLOCK, &params);
                            };

        {
            std::vector<C2Param::Index> indices;
            indices.push_back(C2StreamHdrStaticInfo::output::PARAM_TYPE);
            indices.push_back(C2StreamHdrDynamicMetadataInfo::output::PARAM_TYPE);

            std::vector<std::unique_ptr<C2Param>> params;

            if (queryFunc(indices, params) != C2_OK) {
                /* HDR10+ is not supported */
                return;
            }
        }

        auto vdecIntf = std::static_pointer_cast<ExynosC2DecComponent::VdecCommonParamIntf>(paramIntf);

        if (c2buffer.get() != nullptr) {
            /* HDR Static */
            if (buffer->mImageInfo.stHDRInfo.eChangedInfo & VIDEO_INFO_TYPE_HDR_STATIC) {
                ExynosHdrStaticInfo &ST = buffer->mImageInfo.stHDRInfo.sHdrStaticInfo;

                std::shared_ptr<C2StreamHdrStaticInfo::output> codedStaticInfo = std::make_shared<C2StreamHdrStaticInfo::output>();
                updateHdrStaticInfo(codedStaticInfo, ST);

                std::vector<std::unique_ptr<C2SettingResult>> failures;

                auto err = paramIntf->config({ codedStaticInfo.get() }, C2_MAY_BLOCK, &failures);
                if (err != C2_OK) {
                    StaticExynosLog(Level::Error, "updateC2Config", "[%s] config(C2StreamHdrStaticInfo) is failed", __FUNCTION__);
                }
            }
            {
                /* it could be valid if it was updated by bitstream or framework */
                std::shared_ptr<C2StreamHdrStaticInfo::output> codedStaticInfo = vdecIntf->getHdrStaticInfo();
                if (codedStaticInfo.get() != nullptr) {
                    StaticExynosLog(Level::Info, "VdecCommonParamIntf", "[%s] HDR :: static info[final] r(%f, %f), g(%f, %f), b(%f, %f), w(%f, %f), max(%f), min(%f), cll(%f), fall(%f)", __FUNCTION__,
                        codedStaticInfo->mastering.red.x, codedStaticInfo->mastering.red.y,
                        codedStaticInfo->mastering.green.x, codedStaticInfo->mastering.green.y,
                        codedStaticInfo->mastering.blue.x, codedStaticInfo->mastering.blue.y,
                        codedStaticInfo->mastering.white.x, codedStaticInfo->mastering.white.y,
                        codedStaticInfo->mastering.maxLuminance, codedStaticInfo->mastering.minLuminance,
                        codedStaticInfo->maxCll, codedStaticInfo->maxFall);

                    c2buffer->setInfo(codedStaticInfo);
                }
            }

            /* HDR Dynamic */
            if (buffer->mImageInfo.stHDRInfo.eChangedInfo & VIDEO_INFO_TYPE_HDR_DYNAMIC) {
                int size     = 0;
                bool hasBlob = false;
                char data[MAX_HDR10PLUS_SIZE] = { 0, };

                if (buffer->mImageInfo.stHDRInfo.sHdrDynamicInfo.valid > 0) {
                    /* convert bitstream data to blob */
                    size = Exynos_dynamic_meta_to_itu_t_t35(&buffer->mImageInfo.stHDRInfo.sHdrDynamicInfo, (char *)data);
                    if (size <= 0) {
                        StaticExynosLog(Level::Error, "updateC2Config", "[%s] Exynos_dynamic_meta_to_itu_t_t35 is failed", __FUNCTION__);
                    }

                    hasBlob = false;
                } else if (buffer->mImageInfo.stHDRInfo.sHdrDynamicBlob.nSize > 0) {
                    size = buffer->mImageInfo.stHDRInfo.sHdrDynamicBlob.nSize;
                    hasBlob = true;
                }

                if (size > 0) {
                    auto hdrDynamicInfo = C2StreamHdrDynamicMetadataInfo::output::AllocShared(size, 0u,
                                                    C2Config::HDR_DYNAMIC_METADATA_TYPE_SMPTE_2094_40);
                    memcpy(hdrDynamicInfo->m.data, (hasBlob? (char *)buffer->mImageInfo.stHDRInfo.sHdrDynamicBlob.pData: data), size);

                    std::vector<std::unique_ptr<C2SettingResult>> failures;

                    auto err = paramIntf->config({ hdrDynamicInfo.get() }, C2_MAY_BLOCK, &failures);
                    if (err == C2_OK) {
                        std::vector<std::unique_ptr<C2Param>> params;

                        if (queryFunc({ C2StreamHdrDynamicMetadataInfo::output::PARAM_TYPE }, params) == C2_OK) {
                            hdrDynamicInfo.reset(C2StreamHdrDynamicMetadataInfo::output::From(params[0].release()));
                            c2buffer->setInfo(hdrDynamicInfo);
                            StaticExynosLog(Level::Debug, "updateC2Config", "[%s] HDR :: dynamic info (size:%zu)", __FUNCTION__, hdrDynamicInfo->flexCount());
                        }
                    } else {
                        StaticExynosLog(Level::Error, "updateC2Config", "[%s] config(C2StreamHdrDynamicMetadataInfo) is failed", __FUNCTION__);
                    }
                }
            }
        }

        return;
    }
} // namespace StaticUpdateC2Config

#endif // EXYNOS_C2_CONFIG_UPDATER_H

