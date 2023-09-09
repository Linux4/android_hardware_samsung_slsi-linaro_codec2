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
#ifndef EXYNOS_VIDEO_CODEC_DEC_H
#define EXYNOS_VIDEO_CODEC_DEC_H

#include <memory>

#include "ExynosVideoCodecCommon.h"
#include "ExynosVideoApi.h"

#define LOG_ON
#include "ExynosLog.h"

class DecCodecInfoRegistry : public CodecInfoRegistry {
public:
    DecCodecInfoRegistry() {
        registCodecInfo();
    }

    ~DecCodecInfoRegistry() = default;

    void registCodecInfo() override;
};

class ExynosVideoCodecDec : public ExynosVideoCodecCommon, public DecCodecInfoRegistry {
public:
    ExynosVideoCodecDec(ExynosVideoCodecBase::Type type);

    virtual ~ExynosVideoCodecDec();

    /* override function on ExynosVideoCodecBase */
    ExynosErrorType portStop(ExynosPort::Port port) override;
    ExynosErrorType init() override;
    void deinit() override;
    ExynosErrorType srcSetup(ExynosBufferInfo input) override;
    ExynosErrorType dstSetup() override;
    ExynosErrorType getRefBufCount(int &cnt) override;
    ExynosErrorType getRegBufCount(int &cnt) override;

    /* add function for ExynosVideoCodecDec */
    /* codec specific features */
    ExynosErrorType updateResolution(bool bInterResolution = false);
    ExynosErrorType getResolution(uint32_t &width, uint32_t &height, CropInfo &crop);
    ExynosErrorType getDPBCount(int &count);
    ExynosErrorType getColorFormat(int &format);
    ExynosErrorType getDisplayDelay(int &count);
    ExynosErrorType getExtraBufNum(int &count);

    ExynosErrorType checkRealTimeResource(int32_t width, int32_t height, int32_t operatingRate) override;

protected:
    bool isValidCodecHandle() override;
    void updateBufferInfo(ExynosVideoBuffer &buffer, ExynosBufferInfo &buf) override;

    ExynosVideoFrameStatusType getDisplayStatus(ExynosVideoExtraInfo &extraInfo) override;
    void applyConfig(ExynosParams &params) override;

    /* for common API */
    int CodecSpecific_Set_FrameTag(std::shared_ptr<CodecImpl> codecImpl) override;
    int CodecSpecific_Get_FrameTag(std::shared_ptr<CodecImpl> codecImpl) override;
    ExynosVideoErrorType CodecSpecific_Stop_Wait_Buffer(std::shared_ptr<CodecImpl> codecImpl) override;
    ExynosVideoErrorType CodecSpecific_Reset_Wait_Buffer(std::shared_ptr<CodecImpl> codecImpl) override;

private:
    class CodecDecImpl;

    /* override function on ExynosVideoCodecBase */

    /* add function for ExynosVideoCodecDec */
    ExynosErrorType headerParsing(ExynosBufferInfo input);
    void            updateHDRInfo(HDRInfo &info);
    void            updateBlackBarInfoToParam(ExynosParams &params);
    void            updateFilmGrainInfoToParam(ExynosParams &params);
    void            updateExtraInfo(ExynosVideoMeta *meta, ExynosVideoBuffer &buffer);
    void            applyConfig_PixelFormat(ExynosParams &params);
    void            applyConfig_CompressedColor(ExynosParams &params);
    void            applyConfig_DecodeOrderDecoding(ExynosParams &params);
    void            applyConfig_DisplayDelay(ExynosParams &params);
    void            applyConfig_ThumbnailMode(ExynosParams &params);
    void            applyConfig_Skype(ExynosParams &params);
    void            applyConfig_BlackBar(ExynosParams &params);
    void            applyConfig_ColorAspects(ExynosParams &params);
    void            applyConfig_HdrStatic(ExynosParams &params);
    void            applyConfig_OperatingRate(ExynosParams &params);
    void            applyConfig_RealTimePriority(ExynosParams &params);

    std::shared_ptr<CodecDecImpl> mCodecImpl;

    /* disable default constructor */
    ExynosVideoCodecDec() = delete;
};

#undef LOG_ON

#endif // EXYNOS_VIDEO_CODEC_DEC_H

