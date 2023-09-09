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
#ifndef EXYNOS_VIDEO_ENC_CODEC_SET_API_H
#define EXYNOS_VIDEO_ENC_CODEC_SET_API_H

#include <string>
#include <map>

#include "ExynosVideoApi.h"

constexpr unsigned int GENERAL_TSVC_ENABLE = (1 << 16);

typedef struct _BitrateMode {
    int EnableFRMRateControl;
    int EnableMBRateControl;
    int CBRPeriodRf;
} BitrateMode;

std::map<std::string, std::map<ExynosBitrateMode, BitrateMode>> gBitRateModeTable = {
    { "H264",
      { { BITRATE_MODE_DISABLED,                 { 0, 0, 100 } },
        { BITRATE_MODE_CONST,                    { 1, 1, 10 } },
        { BITRATE_MODE_CONST_WITH_FRAME_SKIP,    { 1, 1, 5 } },
        { BITRATE_MODE_CONST_VT_CALL,            { 1, 1, 5 } },
        { BITRATE_MODE_CONST_WFD,                { 1, 1, 6 } },
        { BITRATE_MODE_VARIABLE,                 { 1, 1, 100 } },
        { BITRATE_MODE_VARIABLE_BIT_SAVE,        { 1, 1, 15 } },
        { BITRATE_MODE_DEFAULT,                  { 1, 1, 100 } }
      }
    },
    { "HEVC",
      { { BITRATE_MODE_DISABLED,                 { 0, 0, 100 } },
        { BITRATE_MODE_CONST,                    { 1, 1, 9 } },
        { BITRATE_MODE_CONST_WITH_FRAME_SKIP,    { 1, 1, 5 } },
        { BITRATE_MODE_CONST_VT_CALL,            { 1, 1, 5 } },
        { BITRATE_MODE_CONST_WFD,                { 1, 1, 6 } },
        { BITRATE_MODE_VARIABLE,                 { 1, 1, 100 } },
        { BITRATE_MODE_VARIABLE_BIT_SAVE,        { 1, 1, 15 } },
        { BITRATE_MODE_DEFAULT,                  { 1, 1, 100 } }
      }
    },
    { "Mpeg4",
      { { BITRATE_MODE_DISABLED,                 { 0, 0, 100 } },
        { BITRATE_MODE_CONST,                    { 1, 1, 9 } },
        { BITRATE_MODE_VARIABLE,                 { 1, 1, 100 } },
        { BITRATE_MODE_DEFAULT,                  { 1, 1, 100 } }
      }
    },
    { "H263",
      { { BITRATE_MODE_DISABLED,                 { 0, 0, 100 } },
        { BITRATE_MODE_CONST,                    { 1, 1, 9 } },
        { BITRATE_MODE_VARIABLE,                 { 1, 1, 100 } },
        { BITRATE_MODE_DEFAULT,                  { 1, 1, 100 } }
      }
    },
    { "Vp8",
      { { BITRATE_MODE_DISABLED,                 { 0, 0, 200 } },
        { BITRATE_MODE_CONST,                    { 1, 1, 9 } },
        { BITRATE_MODE_VARIABLE,                 { 1, 1, 200 } },
        { BITRATE_MODE_DEFAULT,                  { 1, 1, 200 } }
      }
    },
    { "Vp9",
      { { BITRATE_MODE_DISABLED,                 { 0, 0, 200 } },
        { BITRATE_MODE_CONST,                    { 1, 1, 9 } },
        { BITRATE_MODE_VARIABLE,                 { 1, 1, 200 } },
        { BITRATE_MODE_DEFAULT,                  { 1, 1, 200 } }
      }
    },
};

void setBitrateMode(std::string name, ExynosParams &params, ExynosVideoEncCommonParam &commonParam, ExynosBitrateMode &bitrateMode) {
    auto baseParam = params.getParam(ExynosParamIndex::BitrateModeIndex);
    if (baseParam.get() != nullptr) {
        auto param = std::static_pointer_cast<ExynosParam<ParamBitrateMode>>(baseParam);

        auto codecInfo = gBitRateModeTable.find(name);
        if (codecInfo == gBitRateModeTable.end()) {
            return;
        }

        bitrateMode = param->m.mode;

        auto bitrateModeTable = codecInfo->second;
        auto bitrateModeInfo = bitrateModeTable.find((param->m.mode));
        if (bitrateModeInfo == bitrateModeTable.end()) {
            bitrateMode = BITRATE_MODE_DEFAULT;
            bitrateModeInfo = bitrateModeTable.find(BITRATE_MODE_DEFAULT);
        }

        auto info = bitrateModeInfo->second;
        commonParam.EnableFRMRateControl = info.EnableFRMRateControl;   /* 0: Disable,  1: Frame level RC */
        commonParam.EnableMBRateControl  = info.EnableMBRateControl;    /* 0: Disable,  1: MB level RC */
        commonParam.CBRPeriodRf          = info.CBRPeriodRf;

        switch (param->m.mode) {
        case BITRATE_MODE_CONST_WITH_FRAME_SKIP:
            StaticExynosLog(Level::Debug, "", "[%s] %s CBR with frame skip", __FUNCTION__, name.c_str());
            commonParam.bFixedSlice          = VIDEO_TRUE;
            commonParam.FrameSkip            = VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT;
            break;
        case BITRATE_MODE_CONST_VT_CALL:
            StaticExynosLog(Level::Debug, "", "[%s] %s CBR for VT Call", __FUNCTION__, name.c_str());
            commonParam.bFixedSlice          = VIDEO_TRUE;
            break;
        default:
            break;
        }
    }

    return;
}

void setDefaultCommonConfig(ExynosVideoEncCommonParam &commonParam,
                                int SourceWidth, int SourceHeight, int IDRPeriod, int SliceMode,
                                int Bitrate, int FrameQp, int FrameQp_P, ExynosVideoQPRange QpRange,
                                int PadControlOn, int LumaPadVal, int CbPadVal, int CrPadVal,
                                int EnableFRMRateControl, int EnableMBRateControl, int CBRPeriodRf) {
    /* common parameters */
    commonParam.SourceWidth  = SourceWidth;
    commonParam.SourceHeight = SourceHeight;

    commonParam.IDRPeriod    = IDRPeriod;
    commonParam.SliceMode    = SliceMode; /* 0: none-slice */
    commonParam.Bitrate      = Bitrate;
    commonParam.FrameQp      = FrameQp;
    commonParam.FrameQp_P    = FrameQp_P;

    commonParam.QpRange.QpMin_I = QpRange.QpMin_I;
    commonParam.QpRange.QpMax_I = QpRange.QpMax_I;
    commonParam.QpRange.QpMin_P = QpRange.QpMin_P;
    commonParam.QpRange.QpMax_P = QpRange.QpMax_P;
    commonParam.QpRange.QpMin_B = QpRange.QpMin_B;
    commonParam.QpRange.QpMax_B = QpRange.QpMax_B;

    commonParam.PadControlOn = PadControlOn;    /* 0: disable, 1: enable */
    commonParam.LumaPadVal   = LumaPadVal;
    commonParam.CbPadVal     = CbPadVal;
    commonParam.CrPadVal     = CrPadVal;

    /* VBR */
    commonParam.EnableFRMRateControl = EnableFRMRateControl;   /* 0: Disable,  1: Frame level RC */
    commonParam.EnableMBRateControl  = EnableMBRateControl;    /* 0: Disable,  1: MB level RC */
    commonParam.CBRPeriodRf          = CBRPeriodRf;
}

void setH264DefaultConfig(ExynosVideoEncCommonParam &commonParam, ExynosVideoEncH264Param &h264Param) {
    /* common parameters */
    ExynosVideoQPRange QpRange = {5, 50, 5, 50, 5, 50};
    setDefaultCommonConfig(commonParam,
                           320, 240, 30, 0,
                           64000, 29, 30, QpRange,
                           0, 0, 0, 0, 1, 1, 100);

    /* H.264 specific parameters */
    h264Param.ProfileIDC   = 1;  /* constrained base line */
    h264Param.LevelIDC     = 15;  /* Level 5.2 */
    h264Param.FrameQp_B    = 32;
    h264Param.FrameRate    = 30;

    h264Param.NumberBFrames           = 0;
    h264Param.NumberRefForPframes     = 1;
    h264Param.NumberReferenceFrames   = 1;

    h264Param.LoopFilterDisable       = 0;    /* 1: Loop Filter Disable, 0: Filter Enable */
    h264Param.LoopFilterAlphaC0Offset = 0;
    h264Param.LoopFilterBetaOffset    = 0;
    h264Param.SymbolMode       = 0;  /* CAVLC */
    h264Param.PictureInterlace = 0;
    h264Param.Transform8x8Mode = 1;    /* 0: 4x4, 1: allow 8x8 */
    h264Param.DarkDisable      = 1;
    h264Param.SmoothDisable    = 1;
    h264Param.StaticDisable    = 1;
    h264Param.ActivityDisable  = 1;

    /* Temporal SVC */
    /* If MaxTemporalLayerCount value is 0, codec supported max value will be set */
    h264Param.MaxTemporalLayerCount = 0;
    h264Param.TemporalSVC.nCount    = 0;

    /* Hierarchal P & B */
    h264Param.HierarType = VIDEO_HIERARCHICAL_P;  /* Hierarchical P */


    h264Param.SarEnable = 0;
    h264Param.SarIndex  = 0;
    h264Param.SarWidth  = 0;
    h264Param.SarHeight = 0;

    h264Param.VuiRestrictionEnable = 1;

    h264Param.LTRFrames = 0;
    h264Param.ROIEnable = 0;
}

void setHevcDefaultConfig(ExynosVideoEncCommonParam &commonParam, ExynosVideoEncHevcParam &hevcParam) {
    /* common parameters */
    ExynosVideoQPRange QpRange = {5, 50, 5, 50, 5, 50};
    setDefaultCommonConfig(commonParam,
                           320, 240, 30, 0,
                           64000, 29, 30, QpRange,
                           0, 0, 0, 0, 1, 1, 100);

    /* HEVC specific parameters */
    hevcParam.ProfileIDC   = 0;   /* main */
    hevcParam.TierIDC      = 0;   /* main tier */
    hevcParam.LevelIDC     = 51;  /* 5.2 */
    hevcParam.FrameQp_B    = 32;
    hevcParam.FrameRate    = 30;

    hevcParam.NumberRefForPframes     = 1;
    hevcParam.NumberBFrames           = 0;

    hevcParam.MaxPartitionDepth       = 1;
    hevcParam.LoopFilterDisable       = 0;    /* 1: Loop Filter Disable, 0: Filter Enable */
    hevcParam.LoopFilterSliceFlag     = 1;
    hevcParam.LoopFilterTcOffset      = 0;
    hevcParam.LoopFilterBetaOffset    = 0;

    hevcParam.LongtermRefEnable       = 0;
    hevcParam.LongtermUserRef         = 0;
    hevcParam.LongtermStoreRef        = 0;

    hevcParam.DarkDisable             = 1;    /* disable adaptive rate control on dark region */
    hevcParam.SmoothDisable           = 1;    /* disable adaptive rate control on smooth region */
    hevcParam.StaticDisable           = 1;    /* disable adaptive rate control on static region */
    hevcParam.ActivityDisable         = 1;    /* disable adaptive rate control on high activity region */

    /* Temporal SVC */
    /* If MaxTemporalLayerCount value is 0, codec supported max value will be set */
    hevcParam.MaxTemporalLayerCount = 0;
    hevcParam.TemporalSVC.nCount    = (unsigned int)hevcParam.MaxTemporalLayerCount;

    /* Hierarchal P & B */
    hevcParam.HierarType = VIDEO_HIERARCHICAL_P;  /* Hierarchical P */

    hevcParam.GPBEnable = VIDEO_FALSE;
}

void setMpeg4DefaultConfig(ExynosVideoEncCommonParam &commonParam, ExynosVideoEncMpeg4Param &mpeg4Param) {
    /* common parameters */
    ExynosVideoQPRange QpRange = {3, 30, 3, 30, 3, 30};
    setDefaultCommonConfig(commonParam,
                           320, 240, 30, 0,
                           64000, 15, 16, QpRange,
                           0, 0, 0, 0, 1, 1, 100);

    /* Mpeg4 specific parameters */
    mpeg4Param.ProfileIDC           = 0;  /* simple */
    mpeg4Param.LevelIDC             = 6;  /* 4 */
    mpeg4Param.FrameQp_B            = 18;
    mpeg4Param.TimeIncreamentRes    = 30;
    mpeg4Param.VopTimeIncreament    = 1;
    mpeg4Param.SliceArgument        = 0;  /* number of MB or bytes */
    mpeg4Param.NumberBFrames        = 0;
    mpeg4Param.DisableQpelME        = 1;
}

void setH263DefaultConfig(ExynosVideoEncCommonParam &commonParam, ExynosVideoEncH263Param &h263Param) {
    /* common parameters */
    ExynosVideoQPRange QpRange = {3, 30, 3, 30, 3, 30};
    setDefaultCommonConfig(commonParam,
                           320, 240, 30, 0,
                           64000, 15, 16, QpRange,
                           0, 0, 0, 0, 1, 1, 100);

    /* H263 specific parameters */
#if 0  /* not implemented yet */
    //h263Param.Profile           = 0;  /* baseline */
    //h263Param.Level             = 70;  /* 70 */
#endif
    h263Param.FrameRate = 30;
}

void setVp8DefaultConfig(ExynosVideoEncCommonParam &commonParam, ExynosVideoEncVp8Param &vp8Param) {
    /* common parameters */
    ExynosVideoQPRange QpRange = {2, 63, 2, 63, 2, 63};
    setDefaultCommonConfig(commonParam,
                           320, 240, 0, 0,
                           64000, 37, 40, QpRange,
                           0, 0, 0, 0, 1, 1, 200);

    /* VP8 specific parameters */
    vp8Param.Vp8Version   = 0;   /* Version 0 */
    vp8Param.VpX.FrameRate    = 30;

    vp8Param.Vp8NumberOfPartitions  = 0;  /* MAX(3) */
    vp8Param.Vp8FilterLevel         = 28;
    vp8Param.Vp8FilterSharpness     = 6;
    vp8Param.VpX.VpXGoldenFrameSel  = 0;
    vp8Param.VpX.VpXGFRefreshPeriod = 10;
    vp8Param.VpX.RefNumberForPFrame = 1;
    vp8Param.DisableIntraMd4x4      = 0;

    /* Temporal SVC */
    vp8Param.VpX.TemporalSVC.nCount = 0;
}

void setVp9DefaultConfig(ExynosVideoEncCommonParam &commonParam, ExynosVideoEncVp9Param &vp9Param) {
    /* common parameters */
    ExynosVideoQPRange QpRange = {2, 63, 2, 63, 2, 63};
    setDefaultCommonConfig(commonParam,
                           320, 240, 0, 0,
                           64000, 90, 100, QpRange,
                           0, 0, 0, 0, 1, 1, 200);

    /* VP9 specific parameters */
    vp9Param.Vp9Profile   = 1;   /* 0 */
    vp9Param.Vp9Level     = 41;  /* Level 4.1 */
    vp9Param.VpX.FrameRate         = 30;

    vp9Param.VpX.VpXGoldenFrameSel       = 0;
    vp9Param.VpX.VpXGFRefreshPeriod      = 30;
    vp9Param.VpX.RefNumberForPFrame      = 1;
    vp9Param.MaxPartitionDepth  = 0;

    /* Temporal SVC */
    vp9Param.VpX.TemporalSVC.nCount = 0;
}

bool setSkypeConfig(ExynosParams &params, ExynosVideoEncCommonParam &commonParam, ExynosVideoEncH264Param &h264Param) {
    bool ret = false;

    if (params.empty()) {
        /* there is nothing to change */
        return ret;
    }

    /* skype */
    {
        /* low latency */
        auto baseParam = params.getParam(ExynosParamIndex::SkypeLowLatencyIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamSkypeLowLatency>>(baseParam);

            if (param->m.enable == On) {
                h264Param.HeaderWithIFrame      = 0;  /* 1: header + first frame */
                h264Param.LoopFilterDisable     = 0;  /* 1: disable, 0: enable */
                commonParam.EnableFRMQpControl  = 1;  /* 0: disable, 1: per frame QP */
            }
        }

        /* input control */
        baseParam = params.getParam(ExynosParamIndex::SkypeInputControlIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamSkypeInputControl>>(baseParam);

            if (param->m.enable == On) {
                ret = true;
                h264Param.MaxTemporalLayerCount = h264Param.TemporalSVC.nCount; /* max layer count on short-term reference */
                h264Param.TemporalSVC.nCount |= GENERAL_TSVC_ENABLE;  /* always uses short-term reference */
            }
        }

        /* LTR */
        baseParam = params.getParam(ExynosParamIndex::SkypeLTRCountIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamSkypeLTRCount>>(baseParam);

            if (param->m.value >= 0) {
                h264Param.LTRFrames = param->m.value;
            }
        }

        /* SAR */
        baseParam = params.getParam(ExynosParamIndex::SkypeSARIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamSkypeSAR>>(baseParam);

            h264Param.SarEnable = 1;
            h264Param.SarIndex  = 0xFF;  /* depends on width/height info */
            h264Param.SarWidth  = param->m.width;
            h264Param.SarHeight = param->m.height;
        }

        /* max layer count */
        baseParam = params.getParam(ExynosParamIndex::MaxLayeringIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamMaxLayering>>(baseParam);

            h264Param.MaxTemporalLayerCount = param->m.layerCount;
        }
    }

    return ret;
}


void printCommonConfig(ExynosVideoEncCommonParam commonParam) {
    /* common parameters */
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] SourceWidth             : %d", __FUNCTION__, commonParam.SourceWidth);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] SourceHeight            : %d", __FUNCTION__, commonParam.SourceHeight);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] IDRPeriod               : %d", __FUNCTION__, commonParam.IDRPeriod);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] SliceMode               : %d", __FUNCTION__, commonParam.SliceMode);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] RandomIntraMBRefresh    : %d", __FUNCTION__, commonParam.RandomIntraMBRefresh);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] Bitrate                 : %d", __FUNCTION__, commonParam.Bitrate);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] FrameQp                 : %d", __FUNCTION__, commonParam.FrameQp);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] FrameQp_P               : %d", __FUNCTION__, commonParam.FrameQp_P);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] QP(I) ranege            : %d / %d", __FUNCTION__, commonParam.QpRange.QpMin_I, commonParam.QpRange.QpMax_I);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] QP(P) ranege            : %d / %d", __FUNCTION__, commonParam.QpRange.QpMin_P, commonParam.QpRange.QpMax_P);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] QP(B) ranege            : %d / %d", __FUNCTION__, commonParam.QpRange.QpMin_B, commonParam.QpRange.QpMax_B);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] PadControlOn            : %d", __FUNCTION__, commonParam.PadControlOn);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] LumaPadVal              : %d", __FUNCTION__, commonParam.LumaPadVal);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] CbPadVal                : %d", __FUNCTION__, commonParam.CbPadVal);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] CrPadVal                : %d", __FUNCTION__, commonParam.CrPadVal);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] FrameMap                : %d", __FUNCTION__, commonParam.FrameMap);

    /* rate control related parameters */
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] EnableFRMRateControl    : %d", __FUNCTION__, commonParam.EnableFRMRateControl);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] EnableMBRateControl     : %d", __FUNCTION__, commonParam.EnableMBRateControl);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] CBRPeriodRf             : %d", __FUNCTION__, commonParam.CBRPeriodRf);
}

void printH264Config(ExynosVideoEncH264Param h264Param) {
    /* H.264 specific parameters */
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] ProfileIDC              : %d", __FUNCTION__, h264Param.ProfileIDC);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] LevelIDC                : %d", __FUNCTION__, h264Param.LevelIDC);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] FrameQp_B               : %d", __FUNCTION__, h264Param.FrameQp_B);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] FrameRate               : %d", __FUNCTION__, h264Param.FrameRate);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] SliceArgument           : %d", __FUNCTION__, h264Param.SliceArgument);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] NumberBFrames           : %d", __FUNCTION__, h264Param.NumberBFrames);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] NumberReferenceFrames   : %d", __FUNCTION__, h264Param.NumberReferenceFrames);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] NumberRefForPframes     : %d", __FUNCTION__, h264Param.NumberRefForPframes);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] LoopFilterDisable       : %d", __FUNCTION__, h264Param.LoopFilterDisable);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] LoopFilterAlphaC0Offset : %d", __FUNCTION__, h264Param.LoopFilterAlphaC0Offset);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] LoopFilterBetaOffset    : %d", __FUNCTION__, h264Param.LoopFilterBetaOffset);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] SymbolMode              : %d", __FUNCTION__, h264Param.SymbolMode);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] PictureInterlace        : %d", __FUNCTION__, h264Param.PictureInterlace);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] Transform8x8Mode        : %d", __FUNCTION__, h264Param.Transform8x8Mode);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] DarkDisable             : %d", __FUNCTION__, h264Param.DarkDisable);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] SmoothDisable           : %d", __FUNCTION__, h264Param.SmoothDisable);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] StaticDisable           : %d", __FUNCTION__, h264Param.StaticDisable);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] ActivityDisable         : %d", __FUNCTION__, h264Param.ActivityDisable);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] VuiRestrictionEnable:   : %d", __FUNCTION__, h264Param.VuiRestrictionEnable);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] HeaderWithIFrame:       : %d", __FUNCTION__, h264Param.HeaderWithIFrame);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] SarEnable:              : %d", __FUNCTION__, h264Param.SarEnable);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] SarIndex:               : %d", __FUNCTION__, h264Param.SarIndex);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] SarWidth:               : %d", __FUNCTION__, h264Param.SarWidth);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] SarHeight:              : %d", __FUNCTION__, h264Param.SarHeight);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] LTRFrames:              : %d", __FUNCTION__, h264Param.LTRFrames);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] ROIEnable:              : %d", __FUNCTION__, h264Param.ROIEnable);

    /* layering (Temporal SVC) */
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] MaxTemporalLayerCount   : %d", __FUNCTION__, h264Param.MaxTemporalLayerCount);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] TemporalSVC.nCount      : %d", __FUNCTION__, h264Param.TemporalSVC.nCount);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] HierarType:             : %d", __FUNCTION__, h264Param.HierarType);

    /* chroma qp offset */
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] chromaQPOffset.cb       : %d", __FUNCTION__, h264Param.chromaQPOffset.Cb);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] chromaQPOffset.cr       : %d", __FUNCTION__, h264Param.chromaQPOffset.Cr);
}

void printHevcConfig(ExynosVideoEncHevcParam hevcParam) {
    /* HEVC specific parameters */
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] ProfileIDC              : %d", __FUNCTION__, hevcParam.ProfileIDC);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] TierIDC                 : %d", __FUNCTION__, hevcParam.TierIDC);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] LevelIDC                : %d", __FUNCTION__, hevcParam.LevelIDC);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] FrameQp_B               : %d", __FUNCTION__, hevcParam.FrameQp_B);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] FrameRate               : %d", __FUNCTION__, hevcParam.FrameRate);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] MaxPartitionDepth       : %d", __FUNCTION__, hevcParam.MaxPartitionDepth);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] NumberBFrames           : %d", __FUNCTION__, hevcParam.NumberBFrames);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] NumberReferenceFrames   : %d", __FUNCTION__, hevcParam.NumberRefForPframes);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] LoopFilterDisable       : %d", __FUNCTION__, hevcParam.LoopFilterDisable);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] LoopFilterSliceFlag     : %d", __FUNCTION__, hevcParam.LoopFilterSliceFlag);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] LoopFilterTcOffset      : %d", __FUNCTION__, hevcParam.LoopFilterTcOffset);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] LoopFilterBetaOffset    : %d", __FUNCTION__, hevcParam.LoopFilterBetaOffset);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] LongtermRefEnable       : %d", __FUNCTION__, hevcParam.LongtermRefEnable);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] LongtermUserRef         : %d", __FUNCTION__, hevcParam.LongtermUserRef);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] LongtermStoreRef        : %d", __FUNCTION__, hevcParam.LongtermStoreRef);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] DarkDisable             : %d", __FUNCTION__, hevcParam.DarkDisable);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] SmoothDisable           : %d", __FUNCTION__, hevcParam.SmoothDisable);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] StaticDisable           : %d", __FUNCTION__, hevcParam.StaticDisable);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] ActivityDisable         : %d", __FUNCTION__, hevcParam.ActivityDisable);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] ROIEnable:              : %d", __FUNCTION__, hevcParam.ROIEnable);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] GPBEnable:              : %d", __FUNCTION__, hevcParam.GPBEnable);

    /* layering (Temporal SVC) */
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] MaxTemporalLayerCount   : %d", __FUNCTION__, hevcParam.MaxTemporalLayerCount);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] TemporalSVC.nCount      : %d", __FUNCTION__, hevcParam.TemporalSVC.nCount);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] HierarType:             : %d", __FUNCTION__, hevcParam.HierarType);

    /* chroma qp offset */
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] chromaQPOffset.cb       : %d", __FUNCTION__, hevcParam.chromaQPOffset.Cb);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] chromaQPOffset.cr       : %d", __FUNCTION__, hevcParam.chromaQPOffset.Cr);
}

void printMpeg4Config(ExynosVideoEncMpeg4Param mpeg4Param) {
    /* Mpeg4 specific parameters */
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] ProfileIDC              : %d", __FUNCTION__, mpeg4Param.ProfileIDC);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] LevelIDC                : %d", __FUNCTION__, mpeg4Param.LevelIDC);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] FrameQp_B               : %d", __FUNCTION__, mpeg4Param.FrameQp_B);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] TimeIncreamentRes       : %d", __FUNCTION__, mpeg4Param.TimeIncreamentRes);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] VopTimeIncreament       : %d", __FUNCTION__, mpeg4Param.VopTimeIncreament);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] SliceArgument           : %d", __FUNCTION__, mpeg4Param.SliceArgument);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] NumberBFrames           : %d", __FUNCTION__, mpeg4Param.NumberBFrames);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] DisableQpelME           : %d", __FUNCTION__, mpeg4Param.DisableQpelME);
}

void printH263Config(ExynosVideoEncH263Param h263Param) {
    /* H263 specific parameters */
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] FrameRate               : %d", __FUNCTION__, h263Param.FrameRate);
}

void printVp8Config(ExynosVideoEncVp8Param vp8Param) {
    ExynosVideoEncVpXParam    &VpX         = vp8Param.VpX;

    /* VP8 specific parameters */
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] Vp8Version              : %d", __FUNCTION__, vp8Param.Vp8Version);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] FrameRate               : %d", __FUNCTION__, VpX.FrameRate);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] Vp8NumberOfPartition    : %d", __FUNCTION__, vp8Param.Vp8NumberOfPartitions);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] Vp8FilterLevel          : %d", __FUNCTION__, vp8Param.Vp8FilterLevel);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] Vp8FilterSharpness      : %d", __FUNCTION__, vp8Param.Vp8FilterSharpness);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] Vp8GoldenFrameSel       : %d", __FUNCTION__, VpX.VpXGoldenFrameSel);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] Vp8GFRefreshPeriod      : %d", __FUNCTION__, VpX.VpXGFRefreshPeriod);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] RefNumberForPFrame      : %d", __FUNCTION__, VpX.RefNumberForPFrame);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] DisableIntraMd4x4       : %d", __FUNCTION__, vp8Param.DisableIntraMd4x4);

    /* layering (Temporal SVC) */
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] TemporalSVC.nCount      : %d", __FUNCTION__, VpX.TemporalSVC.nCount);
}

void printVp9Config(ExynosVideoEncVp9Param vp9Param) {
    ExynosVideoEncVpXParam    &VpX         = vp9Param.VpX;

    /* VP9 specific parameters */
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] Vp9Profile              : %d", __FUNCTION__, vp9Param.Vp9Profile);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] Vp9Level                : %d", __FUNCTION__, vp9Param.Vp9Level);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] FrameRate               : %d", __FUNCTION__, VpX.FrameRate);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] Vp9GoldenFrameSel       : %d", __FUNCTION__, VpX.VpXGoldenFrameSel);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] Vp9GFRefreshPeriod      : %d", __FUNCTION__, VpX.VpXGFRefreshPeriod);
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] RefNumberForPFrame      : %d", __FUNCTION__, VpX.RefNumberForPFrame);
    StaticExynosLog(Level::Debug, "printCommonConfig", "[%s] MaxPartitionDepth       : %d", __FUNCTION__, vp9Param.MaxPartitionDepth);

    /* layering (Temporal SVC) */
    StaticExynosLog(Level::Essential, "printCommonConfig", "[%s] TemporalSVC.nCount      : %d", __FUNCTION__, VpX.TemporalSVC.nCount);
}

ExynosVideoErrorType setFramerate(void *handle, std::variant<ExynosVideoDecOps, ExynosVideoEncOps> commonOps,
                            ExynosVideoInstInfo videoInstInfo, ExynosVideoEncParam &encParam, uint32_t framerate) {
    if (handle == nullptr) {
        StaticExynosLog(Level::Error, "setFramerate", "[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    auto ret = std::get<ExynosVideoEncOps>(commonOps).Set_FrameRate(handle, framerate);
    if (ret == VIDEO_ERROR_NONE) {
        switch (videoInstInfo.eCodecType) {
        case VIDEO_CODING_AVC:
        {
            encParam.codec.h264.FrameRate = framerate;
        }
            break;
        case VIDEO_CODING_HEVC:
        {
            encParam.codec.hevc.FrameRate = framerate;
        }
            break;
        case VIDEO_CODING_MPEG4:
        {
            encParam.codec.mpeg4.TimeIncreamentRes = framerate;
        }
            break;
        case VIDEO_CODING_H263:
        {
            encParam.codec.h263.FrameRate = framerate;
        }
            break;
        case VIDEO_CODING_VP8:
        {
            encParam.codec.vp8.VpX.FrameRate = framerate;
        }
            break;
        case VIDEO_CODING_VP9:
        {
            encParam.codec.vp9.VpX.FrameRate = framerate;
        }
            break;
        default:
            StaticExynosLog(Level::Error, "setFramerate", "[%s] invalid type(%x)", __FUNCTION__, videoInstInfo.eCodecType);
            break;
        }
    }

    return ret;
}

ExynosVideoErrorType setLayering(void *handle, std::variant<ExynosVideoDecOps, ExynosVideoEncOps> commonOps,
                     ExynosVideoInstInfo videoInstInfo, ExynosVideoEncParam &encParam, struct ParamLayering &layering) {
    ExynosVideoErrorType ret = VIDEO_ERROR_NONE;

    TemporalLayerBuffer temporalSVC;

    memset(&temporalSVC, 0, sizeof(TemporalLayerBuffer));

    if (handle == nullptr) {
        StaticExynosLog(Level::Error, "setLayering", "[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    temporalSVC.nCount = layering.layerCount;

    for (uint32_t i = 0; i < temporalSVC.nCount; i++) {
     temporalSVC.nBitrateRatio[i] = layering.bitrate[i];
    }

    ret = std::get<ExynosVideoEncOps>(commonOps).Set_LayerChange(handle, temporalSVC);
    if (ret == VIDEO_ERROR_NONE) {
        switch (videoInstInfo.eCodecType) {
        case VIDEO_CODING_AVC:
        {
            ExynosVideoEncH264Param &param = encParam.codec.h264;

            memcpy((&param.TemporalSVC), &temporalSVC, sizeof(TemporalLayerBuffer));
        }
            break;
        case VIDEO_CODING_HEVC:
        {
            ExynosVideoEncHevcParam &param = encParam.codec.hevc;

            memcpy((&param.TemporalSVC), &temporalSVC, sizeof(TemporalLayerBuffer));
        }
            break;
        case VIDEO_CODING_VP8:
        {
            ExynosVideoEncVp8Param &vp8Param = encParam.codec.vp8;

            memcpy((&vp8Param.VpX.TemporalSVC), &temporalSVC, sizeof(TemporalLayerBuffer));
        }
            break;
        case VIDEO_CODING_VP9:
        {
            ExynosVideoEncVp9Param &vp9Param = encParam.codec.vp9;

            memcpy((&vp9Param.VpX.TemporalSVC), &temporalSVC, sizeof(TemporalLayerBuffer));
        }
            break;
        default:
            ret = VIDEO_ERROR_NOSUPPORT;
            break;
        }
    }

    return ret;
}

ExynosVideoErrorType setPMV(void *handle, std::variant<ExynosVideoDecOps, ExynosVideoEncOps> commonOps,
                            ExynosVideoEncParam &encParam, struct ParamPMV &pmv) {
    if (handle == nullptr) {
        StaticExynosLog(Level::Error, "printSetPMV", "[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    ExynosVideoPMV videoPMV;
    memset(&videoPMV, 0, sizeof(videoPMV));
    videoPMV.mode = pmv.mode;
    videoPMV.hor_L0 = pmv.hor_L0;
    videoPMV.hor_L1 = pmv.hor_L1;
    videoPMV.ver_L0 = pmv.ver_L0;
    videoPMV.ver_L1 = pmv.ver_L1;

    auto ret = std::get<ExynosVideoEncOps>(commonOps).Set_PMV(handle, &videoPMV);
    if (ret == VIDEO_ERROR_NONE) {
        ExynosVideoEncCommonParam &commonParam = encParam.common;

        memcpy(&(commonParam.PMV), &videoPMV, sizeof(videoPMV));

        StaticExynosLog(Level::Essential, "printSetPMV", "[%s] PMV mode:%d", __FUNCTION__, commonParam.PMV.mode);
        if (commonParam.PMV.mode == 2) {  /* GLOBAL */
            StaticExynosLog(Level::Debug, "printSetPMV", "[%s] PMV horizon(L0:%d, L1:%d)", __FUNCTION__, commonParam.PMV.hor_L0, commonParam.PMV.hor_L1);
            StaticExynosLog(Level::Debug, "printSetPMV", "[%s] PMV vertical(L0:%d, L1:%d)", __FUNCTION__, commonParam.PMV.ver_L0, commonParam.PMV.ver_L1);
        }
    }

    return ret;
}

void setH264StaticConfig(void *handle, std::variant<ExynosVideoDecOps, ExynosVideoEncOps> commonOps,
                         ExynosParams &params, ExynosVideoEncParam &encParam, ExynosVideoInstInfo videoInstInfo,
                         bool &bIsSkype, bool &bIsROI, ExynosBitrateMode &bitrateMode, ExynosVideoMeta *meta) {
    if (params.empty()) {
        /* there is nothing to change */
        return;
    }

    ExynosVideoEncCommonParam &commonParam = encParam.common;
    ExynosVideoEncH264Param   &h264Param   = encParam.codec.h264;

    /* Profile & Level */
    {
        auto baseParam = params.getParam(ExynosParamIndex::ProfileLevelIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamProfileLevel>>(baseParam);

            h264Param.ProfileIDC    = param->m.profile;
            h264Param.LevelIDC      = param->m.level;

            /* W/A : the behavior of ACodec */
            if (h264Param.ProfileIDC > 1) {
                h264Param.SymbolMode = 1;  /* CABAC */
            }
        }
    }

    /* bitrate mode */
    {
        setBitrateMode("H264", params, commonParam, bitrateMode);
    }

    /* frame QP */
    {
        auto baseParam = params.getParam(ExynosParamIndex::FrameQpIndex);

        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamFrameQP>>(baseParam);

            commonParam.FrameQp    = param->m.I;
            commonParam.FrameQp_P  = param->m.P;
            h264Param.FrameQp_B    = param->m.B;
        }
    }

    /* B frame */
    {
        auto baseParam = params.getParam(ExynosParamIndex::BFrameIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamBFrame>>(baseParam);

            h264Param.NumberBFrames = param->m.bframes;
        }
    }

    /* layering (Temporal SVC) */
    {
        auto baseParam = params.getParam(ExynosParamIndex::LayeringIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamLayering>>(baseParam);

            h264Param.MaxTemporalLayerCount = 0;  /* set max count supported by H/W
                                                   * because layer count could not be bigger than max count
                                                   * at runtime.
                                                   */
            h264Param.TemporalSVC.nCount = param->m.layerCount;

            h264Param.HierarType = (param->m.bLayerCount > 0)? VIDEO_HIERARCHICAL_B:VIDEO_HIERARCHICAL_P;

            for (uint32_t i = 0; i < h264Param.TemporalSVC.nCount; i++) {
                h264Param.TemporalSVC.nBitrateRatio[i] = param->m.bitrate[i];
            }

            std::get<ExynosVideoEncOps>(commonOps).Enable_AdaptiveLayerBitrate(handle, param->m.useAdaptiveBitrate);
        }
    }

    /* Ref P frames */
    {
        auto baseParam = params.getParam(ExynosParamIndex::RefPFramesIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamRefPframes>>(baseParam);

            h264Param.NumberRefForPframes   = param->m.pframes;
            h264Param.NumberReferenceFrames = param->m.pframes;
        }
    }

    /* slice size */
    {
        auto baseParam = params.getParam(ExynosParamIndex::SliceSizeIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamSliceSize>>(baseParam);

            commonParam.SliceMode   = param->m.mode;  /* bytes mode */
            h264Param.SliceArgument = param->m.size;  /* slice size */
        }
    }

    /* chroma qp offset */
    {
        auto baseParam = params.getParam(ExynosParamIndex::ChromaQpOffsetIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamChromaQpOffset>>(baseParam);

            h264Param.chromaQPOffset.Cb = param->m.cb;
            h264Param.chromaQPOffset.Cr = param->m.cr;
        }
    }

    /* entropy mode */
    {
        auto baseParam = params.getParam(ExynosParamIndex::EntropyModeIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamEntropyMode>>(baseParam);

            h264Param.SymbolMode = param->m.value;
        }
    }

    /* skype */
    bIsSkype = setSkypeConfig(params, commonParam, h264Param);

    if (meta != nullptr) {
        if ((videoInstInfo.supportInfo.enc.bRoiInfoSupport == VIDEO_TRUE) &&
            (meta->eType & VIDEO_INFO_TYPE_ROI_INFO)) {
            h264Param.ROIEnable = 1;
            bIsROI = true;
        }
    }
}

void setHevcStaticConfig(void *handle, std::variant<ExynosVideoDecOps, ExynosVideoEncOps> commonOps,
                         ExynosParams &params, ExynosVideoEncParam &encParam, ExynosVideoInstInfo videoInstInfo,
                         bool &bIsROI, ExynosBitrateMode &bitrateMode, ExynosVideoMeta *meta) {
    if (params.empty()) {
        /* there is nothing to change */
        return;
    }

    ExynosVideoEncCommonParam &commonParam = encParam.common;
    ExynosVideoEncHevcParam   &hevcParam   = encParam.codec.hevc;

    /* Profile & Level */
    {
        auto baseParam = params.getParam(ExynosParamIndex::ProfileLevelIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamProfileLevel>>(baseParam);

            hevcParam.ProfileIDC    = param->m.profile;
            hevcParam.TierIDC       = 0;
            hevcParam.LevelIDC      = param->m.level;

            if (param->m.level > 0xFF) {
               hevcParam.TierIDC    = 1;
               hevcParam.LevelIDC   = (param->m.level - 0xFF);
            }
        }
    }

    /* bitrate mode */
    {
        setBitrateMode("HEVC", params, commonParam, bitrateMode);
    }

    /* frame QP */
    {
        auto baseParam = params.getParam(ExynosParamIndex::FrameQpIndex);

        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamFrameQP>>(baseParam);

            commonParam.FrameQp    = param->m.I;
            commonParam.FrameQp_P  = param->m.P;
            hevcParam.FrameQp_B    = param->m.B;
        }
    }

    /* B frame */
    {
        auto baseParam = params.getParam(ExynosParamIndex::BFrameIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamBFrame>>(baseParam);

            hevcParam.NumberBFrames = param->m.bframes;
        }
    }

    /* layering (Temporal SVC) */
    {
        auto baseParam = params.getParam(ExynosParamIndex::LayeringIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamLayering>>(baseParam);

            hevcParam.MaxTemporalLayerCount = 0;  /* set max count supported by H/W
                                                   * because layer count could not be bigger than max count
                                                   * at runtime.
                                                   */
            hevcParam.TemporalSVC.nCount = param->m.layerCount;

            if (param->m.bLayerCount > 0) {
                hevcParam.HierarType = VIDEO_HIERARCHICAL_B;
                hevcParam.NumberRefForPframes = 1;
            } else {
                hevcParam.HierarType = VIDEO_HIERARCHICAL_P;
            }

            for (uint32_t i = 0; i < hevcParam.TemporalSVC.nCount; i++) {
                hevcParam.TemporalSVC.nBitrateRatio[i] = param->m.bitrate[i];
            }

            std::get<ExynosVideoEncOps>(commonOps).Enable_AdaptiveLayerBitrate(handle, param->m.useAdaptiveBitrate);
        }
    }

    /* Ref P frames */
    {
        auto baseParam = params.getParam(ExynosParamIndex::RefPFramesIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamRefPframes>>(baseParam);

            hevcParam.NumberRefForPframes = param->m.pframes;
        }
    }

    /* general PB */
    {
        auto baseParam = params.getParam(ExynosParamIndex::GeneralPBIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamGeneralPB>>(baseParam);

            hevcParam.GPBEnable = (param->m.enable != 0)? VIDEO_TRUE:VIDEO_FALSE;
        }
    }

    /* chroma qp offset */
    {
        auto baseParam = params.getParam(ExynosParamIndex::ChromaQpOffsetIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamChromaQpOffset>>(baseParam);

            hevcParam.chromaQPOffset.Cb = param->m.cb;
            hevcParam.chromaQPOffset.Cr = param->m.cr;
        }
    }

    if (meta != nullptr) {
        if ((videoInstInfo.supportInfo.enc.bRoiInfoSupport == VIDEO_TRUE) &&
            (meta->eType & VIDEO_INFO_TYPE_ROI_INFO)) {
            hevcParam.ROIEnable = 1;
            bIsROI = true;
        }
    }
}

void setMpeg4StaticConfig(ExynosParams &params, ExynosVideoEncParam &encParam,
                          ExynosBitrateMode &bitrateMode, ExynosVideoMeta *meta) {
    UNUSED(meta);

    // ExynosVideoEncOps &ops = mOps;

    if (params.empty()) {
        /* there is nothing to change */
        return;
    }

    ExynosVideoEncCommonParam &commonParam = encParam.common;
    ExynosVideoEncMpeg4Param  &mpeg4Param  = encParam.codec.mpeg4;

    /* Profile & Level */
    {
        auto baseParam = params.getParam(ExynosParamIndex::ProfileLevelIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamProfileLevel>>(baseParam);

            mpeg4Param.ProfileIDC    = param->m.profile;
            mpeg4Param.LevelIDC      = param->m.level;
        }
    }

    /* bitrate mode */
    {
        setBitrateMode("Mpeg4", params, commonParam, bitrateMode);
    }

    /* frame QP */
    {
        auto baseParam = params.getParam(ExynosParamIndex::FrameQpIndex);

        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamFrameQP>>(baseParam);

            commonParam.FrameQp    = param->m.I;
            commonParam.FrameQp_P  = param->m.P;
            mpeg4Param.FrameQp_B   = param->m.B;
        }
    }

    /* B frame */
    {
        auto baseParam = params.getParam(ExynosParamIndex::BFrameIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamBFrame>>(baseParam);

            mpeg4Param.NumberBFrames = param->m.bframes;
        }
    }
}

void setH263StaticConfig(ExynosParams &params, ExynosVideoEncParam &encParam,
                         ExynosBitrateMode &bitrateMode, ExynosVideoMeta *meta) {
    UNUSED(meta);

    // ExynosVideoEncOps &ops = mOps;

    if (params.empty()) {
        /* there is nothing to change */
        return;
    }

    ExynosVideoEncCommonParam &commonParam = encParam.common;
    // ExynosVideoEncH263Param   &h263Param   = mEncParam.codec.h263;

    /* bitrate mode */
    {
        setBitrateMode("H263", params, commonParam, bitrateMode);
    }

    /* frame QP */
    {
        auto baseParam = params.getParam(ExynosParamIndex::FrameQpIndex);

        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamFrameQP>>(baseParam);

            commonParam.FrameQp    = param->m.I;
            commonParam.FrameQp_P  = param->m.P;
        }
    }
}

void setVpXStaticConfig(void *handle, std::variant<ExynosVideoDecOps, ExynosVideoEncOps> commonOps,
                        std::string name, ExynosParams &params, ExynosVideoEncParam &encParam,
                        ExynosVideoEncVpXParam &vpXParam, ExynosBitrateMode &bitrateMode) {
    ExynosVideoEncCommonParam &commonParam = encParam.common;

    /* bitrate mode */
    {
        setBitrateMode(name, params, commonParam, bitrateMode);
    }

    /* frame QP */
    {
        auto baseParam = params.getParam(ExynosParamIndex::FrameQpIndex);

        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamFrameQP>>(baseParam);

            commonParam.FrameQp    = param->m.I;
            commonParam.FrameQp_P  = param->m.P;
        }
    }

    /* layering (Temporal SVC) */
    {
        auto baseParam = params.getParam(ExynosParamIndex::LayeringIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamLayering>>(baseParam);

            vpXParam.TemporalSVC.nCount = param->m.layerCount;

            for (uint32_t i = 0; i < vpXParam.TemporalSVC.nCount; i++) {
                vpXParam.TemporalSVC.nBitrateRatio[i] = param->m.bitrate[i];
            }

            std::get<ExynosVideoEncOps>(commonOps).Enable_AdaptiveLayerBitrate(handle, param->m.useAdaptiveBitrate);
        }
    }

    /* Ref P frames */
    {
        auto baseParam = params.getParam(ExynosParamIndex::RefPFramesIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamRefPframes>>(baseParam);

            vpXParam.RefNumberForPFrame = param->m.pframes;
        }
    }
}

void setVp8StaticConfig(void *handle, std::variant<ExynosVideoDecOps, ExynosVideoEncOps> commonOps,
                        ExynosParams &params, ExynosVideoEncParam &encParam,
                        ExynosBitrateMode &bitrateMode, ExynosVideoMeta *meta) {
    UNUSED(meta);

    // ExynosVideoEncOps &ops = std::get<ExynosVideoEncOps>(mCommonOps);

    if (params.empty()) {
        /* there is nothing to change */
        return;
    }

    ExynosVideoEncVp8Param    &vp8Param    = encParam.codec.vp8;
    ExynosVideoEncVpXParam    &VpX         = vp8Param.VpX;

    setVpXStaticConfig(handle, commonOps, "Vp8", params, encParam, VpX, bitrateMode);
}

void setVp9StaticConfig(void *handle, std::variant<ExynosVideoDecOps, ExynosVideoEncOps> commonOps,
                        ExynosParams &params, ExynosVideoEncParam &encParam,
                        ExynosBitrateMode &bitrateMode, ExynosVideoMeta *meta) {
    UNUSED(meta);

    if (params.empty()) {
        /* there is nothing to change */
        return;
    }

    ExynosVideoEncVp9Param    &vp9Param    = encParam.codec.vp9;
    ExynosVideoEncVpXParam    &VpX         = vp9Param.VpX;

    /* Profile & Level */
    {
        auto baseParam = params.getParam(ExynosParamIndex::ProfileLevelIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamProfileLevel>>(baseParam);

            vp9Param.Vp9Profile    = param->m.profile;
            vp9Param.Vp9Level      = param->m.level;
        }
    }

    setVpXStaticConfig(handle, commonOps, "Vp9", params, encParam, VpX, bitrateMode);
}

ExynosVideoErrorType applySkypeConfig(void *handle, std::variant<ExynosVideoDecOps, ExynosVideoEncOps> commonOps, ExynosParams &params) {
    if (handle == nullptr) {
        StaticExynosLog(Level::Error, "printSkypeConfig", "[%s] handle is null", __FUNCTION__);
        return VIDEO_ERROR_BADPARAM;
    }

    ExynosVideoEncOps &ops = std::get<ExynosVideoEncOps>(commonOps);

    /* frame QP */
    {
        auto baseParam = params.getParam(ExynosParamIndex::SkypeFrameQpIndex);

        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamSkypeFrameQP>>(baseParam);

            auto ret = ops.Set_DynamicQpControl(handle, param->m.value);
            if (ret != VIDEO_ERROR_NONE) {
                StaticExynosLog(Level::Error, "printSkypeConfig", "[%s] Set_DynamicQpControl(%d) is failed", __FUNCTION__, param->m.value);
                return ret;
            }

            StaticExynosLog(Level::Essential, "printSkypeConfig", "[%s] skype :: frame QP:%d", __FUNCTION__, param->m.value);
        }
    }

    /* LTR */
    {
        auto baseParam = params.getParam(ExynosParamIndex::SkypeLTRIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamSkypeLTR>>(baseParam);

            auto ret = ops.Set_MarkLTRFrame(handle, (param->m.mark + 1));
            if (ret != VIDEO_ERROR_NONE) {
                StaticExynosLog(Level::Error, "printSkypeConfig", "[%s] Set_MarkLTRFrame(%d) is failed", __FUNCTION__, (param->m.mark + 1));
                return ret;
            }

            ret = ops.Set_UsedLTRFrame(handle, param->m.use);
            if (ret != VIDEO_ERROR_NONE) {
                StaticExynosLog(Level::Error, "printSkypeConfig", "[%s] Set_UsedLTRFrame(0x%x) is failed", __FUNCTION__, param->m.use);
                return ret;
            }

            StaticExynosLog(Level::Essential, "printSkypeConfig", "[%s] skype :: LTR :: mark:index(%d), use:BM(0x%x)",
                                __FUNCTION__, (param->m.mark + 1), param->m.use);
        }
    }

    /* base layer pid */
    {
        auto baseParam = params.getParam(ExynosParamIndex::SkypeBaseLayerPidIndex);
        if (baseParam.get() != nullptr) {
            auto param = std::static_pointer_cast<ExynosParam<ParamSkypeBaseLayerPid>>(baseParam);

            auto ret = ops.Set_BasePID(handle, param->m.value);
            if (ret != VIDEO_ERROR_NONE) {
                StaticExynosLog(Level::Error, "printSkypeConfig", "[%s] Set_UsedLTRFrame(0x%x) is failed", __FUNCTION__, param->m.value);
                return ret;
            }

            StaticExynosLog(Level::Essential, "printSkypeConfig", "[%s] skype :: BasePID:%d", __FUNCTION__, param->m.value);
        }
    }

    return VIDEO_ERROR_NONE;
}

#endif // EXYNOS_VIDEO_ENC_CODEC_SET_API_H

