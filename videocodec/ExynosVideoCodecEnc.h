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
#ifndef EXYNOS_VIDEO_CODEC_ENC_H
#define EXYNOS_VIDEO_CODEC_ENC_H

#include <memory>

#include "ExynosVideoCodecCommon.h"

#define LOG_ON
#include "ExynosLog.h"

class EncCodecInfoRegistry : public CodecInfoRegistry {
public:
    EncCodecInfoRegistry() {
        registCodecInfo();
    }

    ~EncCodecInfoRegistry() = default;

    void registCodecInfo() override;
};

class ExynosVideoCodecEnc : public ExynosVideoCodecCommon, public EncCodecInfoRegistry {
public:
    ExynosVideoCodecEnc(ExynosVideoCodecBase::Type type);

    virtual ~ExynosVideoCodecEnc();

    /* override function on ExynosVideoCodecBase */
    ExynosErrorType init() override;
    void deinit() override;
    ExynosErrorType srcSetup(ExynosBufferInfo input) override;
    ExynosErrorType dstSetup() override;

    bool availableToEnqueue(ExynosBufferInfo &buf);

    ExynosErrorType checkRealTimeResource(int32_t width, int32_t height, int32_t operatingRate) override;

protected:
    bool isValidCodecHandle() override;
    void updateBufferInfo(ExynosVideoBuffer &buffer, ExynosBufferInfo &buf) override;
    void applyConfig(ExynosParams &params) override;

    /* add function for ExynosVideoCodecEnc */
    bool needToChangeResolution(ExynosBufferInfo &buf);
    ExynosErrorType changeResolution(ExynosBufferInfo &buf);
    void setDRCRequired(bool bDRCRequired) override;
    bool getDRCRequired() override;
    void applyExtraInfo(ExynosBufferInfo &buf) override;
    void setDropControl() override;
    void runPerformance() override;

    /* for common API */
    int CodecSpecific_Set_FrameTag(std::shared_ptr<CodecImpl> codecImpl) override;
    int CodecSpecific_Get_FrameTag(std::shared_ptr<CodecImpl> codecImpl) override;
    ExynosVideoErrorType CodecSpecific_Stop_Wait_Buffer(std::shared_ptr<CodecImpl> codecImpl) override;
    ExynosVideoErrorType CodecSpecific_Reset_Wait_Buffer(std::shared_ptr<CodecImpl> codecImpl) override;

private:
    void applyConfig_Bitrate(ExynosParams &params);
    void applyConfig_Framerate(ExynosParams &params);
    void applyConfig_IdrPeriod(ExynosParams &params);
    void applyConfig_Layering(ExynosParams &params);
    void applyConfig_QpRange(ExynosParams &params);
    void applyConfig_OperatingRate(ExynosParams &params);
    void applyConfig_RealTimePriority(ExynosParams &params);
    void applyConfig_AverageQp(ExynosParams &params);
    void applyConfig_IFrameRatio(ExynosParams &params);
    void applyConfig_PMV(ExynosParams &params);
    void applyConfig_IntraVopRefresh(ExynosParams &params);
    void applyConfig_PrependHeaderMode(ExynosParams &params);
    void applyConfig_ColorAspects(ExynosParams &params);
    void applyConfig_DropControl(ExynosParams &params);
    void applyConfig_Skype(ExynosParams &params);
    void applyConfig_HDREncoding(ExynosParams &params);
    void applyConfig_HDRStaticInfo(ExynosParams &params);
    void applyConfig_HDR10PlusInfo(ExynosParams &params);
    void applyConfig_HDR10PlusStat(ExynosParams &params);
    void applyConfig_SetMaxIFrameSize(ExynosParams &params);

    void applyExtraInfo_Config(ExynosBufferInfo &buf);
    void applyExtraInfo_Param(ExynosBufferInfo &buf);

    class CodecEncImpl;

    /* override function on ExynosVideoCodecBase */

    /* add function for ExynosVideoCodecEnc */
    void getColorAspectsForRGB(int &range, int &primaries, int &transfer, int &matrix);
    void updateAverageQpInfoToParam(ExynosParams &params);
    void updateHDR10PlusStatInfoToParam(ExynosParams &params);
    void updateHDRDynamicInfoToParam(ExynosParams &params, ExynosHdrDynamicInfo &DY);

    std::shared_ptr<CodecEncImpl> mCodecImpl;

    /* disable default constructor */
    ExynosVideoCodecEnc() = delete;
};

#undef LOG_ON

#endif // EXYNOS_VIDEO_CODEC_ENC_H

