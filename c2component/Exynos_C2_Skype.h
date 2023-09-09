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
#ifndef EXYNOS_C2_SKYPE_H
#define EXYNOS_C2_SKYPE_H

#include <memory>

#include "Exynos_C2_EncComponent.h"

#define MAX_LTR_FRAMES 4

constexpr int32_t SKYPE_DRIVER_VERSION = 200429;  /* TODO */

class SkypeParamIntf : public ExynosC2EncComponent::VencCommonParamIntf {
public:
    SkypeParamIntf(const std::shared_ptr<C2ReflectorHelper> &reflector,
    C2String name,
    C2Component::kind_t kind,
    C2Component::domain_t domain,
    C2String mediaType,
    ExynosC2FilterManager::FilterModuleInfoList listFilterInfo) :
    VencCommonParamIntf(reflector, name, kind, domain, mediaType, listFilterInfo) {
        /* skype */
        {
            addParameter(
                    DefineParam(mSkypeLowLatency, C2EXYNOS_PARAMKEY_SKYPE_ENC_LOW_LATENCY)
                    .withDefault(new C2ExynosPortSkypeLowLatencySetting::output(Off))
                    .withFields({ C2F(mSkypeLowLatency, enable).inRange(Off, On) })
                    .withSetter(SkypeLowLatencySetter)
                    .build());
            addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mSkypeLowLatency)), cnvSkypeLowLatency);

            addParameter(
                    DefineParam(mSkypeBitrateMode, C2EXYNOS_PARAMKEY_SKYPE_ENC_BITRATE_MODE)
                    .withDefault(new C2ExynosPortSkypeBitrateModeSetting::output(VendorC2Config::SKYPE_BITRATE_MODE_NONE, 0))
                    .withFields({
                        C2F(mSkypeBitrateMode, value).oneOf({
                            VendorC2Config::SKYPE_BITRATE_MODE_IGNORE,
                            VendorC2Config::SKYPE_BITRATE_MODE_VBR,
                            VendorC2Config::SKYPE_BITRATE_MODE_CBR,
                            /* VendorC2Config::SKYPE_BITRATE_MODE_VBR_SKIP_FRAME, */
                            VendorC2Config::SKYPE_BITRATE_MODE_CBR_SKIP_FRAME,
                            VendorC2Config::SKYPE_BITRATE_MODE_NONE}),
                        C2F(mSkypeBitrateMode, bitrate).any(),
                    })
                    .withSetter(SkypeBitrateModeSetter)
                    .build());
            addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mSkypeBitrateMode)), nullptr);
        }

        /* skype */
        {
            addParameter(
                    DefineParam(mSkypeCustomProfileLevel, C2EXYNOS_PARAMKEY_SKYPE_ENC_CUSTOM_PROFILE_LEVEL)
                    .withDefault(new C2ExynosPortCustomProfileLevelSetting::output(
                                    VendorC2Config::SKYPE_PROFILE_NONE,
                                    VendorC2Config::SKYPE_LEVEL_NONE))
                    .withFields({
                        C2F(mSkypeCustomProfileLevel, profile).oneOf({
                                VendorC2Config::SKYPE_PROFILE_NONE,
                                VendorC2Config::SKYPE_PROFILE_BASELINE,
                                VendorC2Config::SKYPE_PROFILE_MAIN,
                                VendorC2Config::SKYPE_PROFILE_HIGH,
                                VendorC2Config::SKYPE_PROFILE_CONSTRAINED_BASELINE,
                                VendorC2Config::SKYPE_PROFILE_CONSTRAINED_HIGH,
                        }),
                        C2F(mSkypeCustomProfileLevel, level).oneOf({
                                VendorC2Config::SKYPE_LEVEL_NONE,
                                VendorC2Config::SKYPE_LEVEL_1, VendorC2Config::SKYPE_LEVEL_1B, VendorC2Config::SKYPE_LEVEL_1_1,
                                VendorC2Config::SKYPE_LEVEL_1_2, VendorC2Config::SKYPE_LEVEL_1_3,
                                VendorC2Config::SKYPE_LEVEL_2, VendorC2Config::SKYPE_LEVEL_2_1, VendorC2Config::SKYPE_LEVEL_2_2,
                                VendorC2Config::SKYPE_LEVEL_3, VendorC2Config::SKYPE_LEVEL_3_1, VendorC2Config::SKYPE_LEVEL_3_2,
                                VendorC2Config::SKYPE_LEVEL_4, VendorC2Config::SKYPE_LEVEL_4_1, VendorC2Config::SKYPE_LEVEL_4_2,
                                VendorC2Config::SKYPE_LEVEL_5, VendorC2Config::SKYPE_LEVEL_5_1, VendorC2Config::SKYPE_LEVEL_5_2,
                                VendorC2Config::SKYPE_LEVEL_6, VendorC2Config::SKYPE_LEVEL_6_1, /* VendorC2Config::SKYPE_LEVEL_6_2, */
                        })
                    })
                    .withSetter(SkyepCustomProfileLevelSetter)
                    .build());
            addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mSkypeCustomProfileLevel)), nullptr);
        }

        /* skype */
        {
            /* information */
            addParameter(
                    DefineParam(mSkypeDriverVersionInfo, C2EXYNOS_PARAMKEY_SKYPE_ENC_DRIVER_VERSION)
                    .withConstValue(new C2ExynosPortDriverVersionInfo::output(SKYPE_DRIVER_VERSION))
                    .build());

            addParameter(
                    DefineParam(mSkypeMaxTemporalLayerInfo, C2EXYNOS_PARAMKEY_SKYPE_ENC_MAX_TEMPORAL_LAYER)
                    .withConstValue(new C2ExynosPortMaxTemporalLayerInfo::output(MAX_TEMPORAL_LAYERS, (MAX_TEMPORAL_LAYERS - 1)))
                    .build());

            addParameter(
                    DefineParam(mSkypePreprocessInfo, C2EXYNOS_PARAMKEY_SKYPE_ENC_PREPROCESS)
                    .withConstValue(new C2ExynosPortPreprocessInfo::output(0 /* 1:1 */, 0 /* not supported */))
                    .build());

            addParameter(
                    DefineParam(mSkypeMaxLTRInfo, C2EXYNOS_PARAMKEY_SKYPE_ENC_MAX_LTR)
                    .withConstValue(new C2ExynosPortMaxLTRInfo::output(MAX_LTR_FRAMES))
                    .build());

            addParameter(
                    DefineParam(mSkypeInputcontrol, C2EXYNOS_PARAMKEY_SKYPE_ENC_INPUT_CONTROL)
                    .withDefault(new C2ExynosPortInputControlSetting::output(Off))
                    .withFields({ C2F(mSkypeInputcontrol, enable).inRange(Off, On) })
                    .withSetter(SkypeInputControlSetter)
                    .build());
            addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mSkypeInputcontrol)), cnvSkypeInputControl);

            /* LTR */
            addParameter(
                    DefineParam(mSkypeLTRCount, C2EXYNOS_PARAMKEY_SKYPE_ENC_LTR_COUNT)
                    .withDefault(new C2ExynosPortLTRCountSetting::output(0))
                    .withFields({ C2F(mSkypeLTRCount, num_ltr_frames).inRange(0, MAX_LTR_FRAMES) })
                    .withSetter(SkypeLTRCountSetter)
                    .build());
            addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mSkypeLTRCount)), cnvSkypeLTRCount);

            addParameter(
                    DefineParam(mSkypeLTR, C2EXYNOS_PARAMKEY_SKYPE_ENC_LTR)
                    .withDefault(new C2ExynosPortLTRTuning::output(0, 0))
                    .withFields({
                        C2F(mSkypeLTR, mark_frame).any(),
                        C2F(mSkypeLTR, use_frame).any(),
                    })
                    .withSetter(SkypeLTRSetter)
                    .build());
            addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mSkypeLTR)), cnvSkypeLTR);

            /* SAR */
            addParameter(
                    DefineParam(mSkypeSar, C2EXYNOS_PARAMKEY_SKYPE_ENC_SAR)
                    .withDefault(new C2ExynosPortSarSetting::output(0, 0))
                    .withFields({
                        C2F(mSkypeSar, width).any(),
                        C2F(mSkypeSar, height).any(),
                    })
                    .withSetter(SkypeSarSetter)
                    .build());
            addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mSkypeSar)), cnvSkypeSar);

            /* slice header spacing */
            addParameter(
                    DefineParam(mSkypeSliceHeaderSpacing, C2EXYNOS_PARAMKEY_SKYPE_ENC_SLICE_HEADER_SPACING)
                    .withDefault(new C2ExynosPortSliceHeaderSpacingSetting::output(0))
                    .withFields({ C2F(mSkypeSliceHeaderSpacing, spacing).any() })
                    .withSetter(SkypeSliceHeaderSpacingSetter)
                    .build());
            addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mSkypeSliceHeaderSpacing)), cnvSkypeSliceHeaderSpacing);

            /* frame QP */
            addParameter(
                    DefineParam(mSkypeFrameQp, C2EXYNOS_PARAMKEY_SKYPE_ENC_FRAME_QP)
                    .withDefault(new C2ExynosPortFrameQpTuning::output(0))
                    .withFields({ C2F(mSkypeFrameQp, value).any() })
                    .withSetter(SkypeFrameQpSetter)
                    .build());
            addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mSkypeFrameQp)), cnvSkypeFrameQp);

            /* base layer pid */
            addParameter(
                    DefineParam(mSkypeBaseLayerPID, C2EXYNOS_PARAMKEY_SKYPE_ENC_BASE_LAYER_PID)
                    .withDefault(new C2ExynosPortBaseLayerPidTuning::output(0))
                    .withFields({ C2F(mSkypeBaseLayerPID, value).any() })
                    .withSetter(SkypeBaseLayerPidSetter)
                    .build());
            addFilterParamSetter(*(std::static_pointer_cast<C2Param>(mSkypeBaseLayerPID)), cnvSkypeBaseLayerPid);
        }
    }

    virtual ~SkypeParamIntf() = default;

    /* setter functions */
    static void SkypeProfileLevelSetter(
            bool mayBlock,
            C2P<C2StreamProfileLevelInfo::output> &me,
            const C2P<C2ExynosPortCustomProfileLevelSetting::output> &skype) {
        (void)mayBlock;

        struct MappingValue {
            int32_t skype;
            int32_t c2;
        };

        if (skype.v.profile != VendorC2Config::SKYPE_PROFILE_NONE) {
            constexpr MappingValue kProfiles[] = {
                { VendorC2Config::SKYPE_PROFILE_BASELINE,               PROFILE_AVC_BASELINE },
                { VendorC2Config::SKYPE_PROFILE_MAIN,                   PROFILE_AVC_MAIN },
                { VendorC2Config::SKYPE_PROFILE_HIGH,                   PROFILE_AVC_HIGH },
                { VendorC2Config::SKYPE_PROFILE_CONSTRAINED_BASELINE,   PROFILE_AVC_CONSTRAINED_BASELINE },
                { VendorC2Config::SKYPE_PROFILE_CONSTRAINED_HIGH,       PROFILE_AVC_CONSTRAINED_HIGH },
            };

            for (const MappingValue &profile : kProfiles) {
                if (skype.v.profile == profile.skype) {
                    me.set().profile = (C2Config::profile_t)profile.c2;
                }
            }
        }

        if (skype.v.level != VendorC2Config::SKYPE_LEVEL_NONE) {
            constexpr MappingValue kLevels[] = {
                { VendorC2Config::SKYPE_LEVEL_1,    LEVEL_AVC_1 },
                { VendorC2Config::SKYPE_LEVEL_1B,   LEVEL_AVC_1B },
                { VendorC2Config::SKYPE_LEVEL_1_1,  LEVEL_AVC_1_1 },
                { VendorC2Config::SKYPE_LEVEL_1_2,  LEVEL_AVC_1_2 },
                { VendorC2Config::SKYPE_LEVEL_1_3,  LEVEL_AVC_1_3 },
                { VendorC2Config::SKYPE_LEVEL_2,    LEVEL_AVC_2 },
                { VendorC2Config::SKYPE_LEVEL_2_1,  LEVEL_AVC_2_1 },
                { VendorC2Config::SKYPE_LEVEL_2_2,  LEVEL_AVC_2_2 },
                { VendorC2Config::SKYPE_LEVEL_3,    LEVEL_AVC_3 },
                { VendorC2Config::SKYPE_LEVEL_3_1,  LEVEL_AVC_3_1 },
                { VendorC2Config::SKYPE_LEVEL_3_2,  LEVEL_AVC_3_2 },
                { VendorC2Config::SKYPE_LEVEL_4,    LEVEL_AVC_4 },
                { VendorC2Config::SKYPE_LEVEL_4_1,  LEVEL_AVC_4_1 },
                { VendorC2Config::SKYPE_LEVEL_4_2,  LEVEL_AVC_4_2 },
                { VendorC2Config::SKYPE_LEVEL_5,    LEVEL_AVC_5 },
                { VendorC2Config::SKYPE_LEVEL_5_1,  LEVEL_AVC_5_1 },
                { VendorC2Config::SKYPE_LEVEL_5_2,  LEVEL_AVC_5_2 },
                { VendorC2Config::SKYPE_LEVEL_6,    LEVEL_AVC_6 },
                { VendorC2Config::SKYPE_LEVEL_6_1,  LEVEL_AVC_6_1 },
#if 0  /* not supported yet */
                { VendorC2Config::SKYPE_LEVEL_6_2,  LEVEL_AVC_6_2 },
#endif
            };

            for (const MappingValue &level : kLevels) {
                if (skype.v.level == level.skype) {
                    me.set().level = (C2Config::level_t)level.c2;
                }
            }
        }
    }

    static C2R SkypeLowLatencySetter(bool mayBlock, C2P<C2ExynosPortSkypeLowLatencySetting::output> &me) {
        (void)mayBlock;
        C2R res = C2R::Ok();

        StaticExynosLog(Level::Debug, "SkypeParamIntf", "[%s] skype :: low latency : %s",
                            __FUNCTION__, (me.v.enable == On)? "enabled":"disabled");
        return res;
    }

    static C2R SkypeInputControlSetter(bool mayBlock, C2P<C2ExynosPortInputControlSetting::output> &me) {
        (void)mayBlock;
        C2R res = C2R::Ok();

        StaticExynosLog(Level::Debug, "SkypeParamIntf", "[%s] skype :: input control : %s",
                            __FUNCTION__, (me.v.enable == On)? "enabled":"disabled");
        return res;
    }

    static C2R SkyepCustomProfileLevelSetter(bool mayBlock, C2P<C2ExynosPortCustomProfileLevelSetting::output> &me) {
        (void)mayBlock;
        C2R res = C2R::Ok();

        StaticExynosLog(Level::Debug, "SkypeParamIntf", "[%s] skype :: profile:0x%x, level:0x%x",
                            __FUNCTION__, me.v.profile, me.v.level);
        return res;
    }

    static C2R SkypeLTRCountSetter(bool mayBlock, C2P<C2ExynosPortLTRCountSetting::output> &me) {
        (void)mayBlock;
        C2R res = C2R::Ok();

        StaticExynosLog(Level::Debug, "SkypeParamIntf", "[%s] skype :: LTR :: count:%d", __FUNCTION__, me.v.num_ltr_frames);

        return res;
    }

    static C2R SkypeLTRSetter(bool mayBlock, C2P<C2ExynosPortLTRTuning::output> &me) {
        (void)mayBlock;
        C2R res = C2R::Ok();

        StaticExynosLog(Level::Debug, "SkypeParamIntf", "[%s] skype :: LTR :: mark:%d, use:%d",
                                __FUNCTION__, me.v.mark_frame, me.v.use_frame);

        return res;
    }

    static C2R SkypeSarSetter(bool mayBlock, C2P<C2ExynosPortSarSetting::output> &me) {
        (void)mayBlock;
        C2R res = C2R::Ok();

        StaticExynosLog(Level::Debug, "SkypeParamIntf", "[%s] skype :: SAR :: width:%d, height:%d",
                                            __FUNCTION__, me.v.width, me.v.height);

        return res;
    }

    static C2R SkypeSliceHeaderSpacingSetter(bool mayBlock, C2P<C2ExynosPortSliceHeaderSpacingSetting::output> &me) {
        (void)mayBlock;
        C2R res = C2R::Ok();

        StaticExynosLog(Level::Debug, "SkypeParamIntf", "[%s] skype :: Slice Header Spacing :: size:%d",
                                            __FUNCTION__, me.v.spacing);

        return res;
    }

    static C2R SkypeFrameQpSetter(bool mayBlock, C2P<C2ExynosPortFrameQpTuning::output> &me) {
        (void)mayBlock;
        C2R res = C2R::Ok();

        StaticExynosLog(Level::Debug, "SkypeParamIntf", "[%s] skype :: frame QP:%d",
                                            __FUNCTION__, me.v.value);

        return res;
    }

    static C2R SkypeBitrateModeSetter(bool mayBlock, C2P<C2ExynosPortSkypeBitrateModeSetting::output> &me) {
        (void)mayBlock;
        C2R res = C2R::Ok();

        StaticExynosLog(Level::Debug, "SkypeParamIntf", "[%s] skype :: bitrate mode:0x%x, bitrate:%d",
                                            __FUNCTION__, me.v.value, me.v.bitrate);

        return res;
    }

    static C2R SkypeBaseLayerPidSetter(bool mayBlock, C2P<C2ExynosPortBaseLayerPidTuning::output> &me) {
        (void)mayBlock;
        C2R res = C2R::Ok();

        StaticExynosLog(Level::Debug, "SkypeParamIntf", "[%s] skype :: base layer pid:%d", __FUNCTION__, me.v.value);

        return res;
    }

    static FilterParamSetterRet cnvSkypeLowLatency(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<SkypeParamIntf>(intf);

        auto id = intfImpl->findFilterID("enc");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamSkypeLowLatency>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamSkypeLowLatency>>(filterParam->getBaseParam());

            param->m.enable = intfImpl->getSkypeLowLatency();

            StaticExynosLog(Level::Essential, "SkypeParamIntf", "[%s] skype :: low latency : %s",
                            __FUNCTION__, (param->m.enable == On)? "enabled":"disabled");

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
        }

        return ret;
    }

    static FilterParamSetterRet cnvSkypeInputControl(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<SkypeParamIntf>(intf);

        auto id = intfImpl->findFilterID("enc");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamSkypeInputControl>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamSkypeInputControl>>(filterParam->getBaseParam());

            param->m.enable = intfImpl->getSkypeInputControl();

            StaticExynosLog(Level::Essential, "SkypeParamIntf", "[%s] skype :: input control : %s",
                            __FUNCTION__, (param->m.enable == On)? "enabled":"disabled");

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
        }

        return ret;
    }

    static FilterParamSetterRet cnvSkypeLTRCount(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<SkypeParamIntf>(intf);

        auto id = intfImpl->findFilterID("enc");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamSkypeLTRCount>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamSkypeLTRCount>>(filterParam->getBaseParam());

            param->m.value = intfImpl->getSkypeLTRCount();

            StaticExynosLog(Level::Essential, "SkypeParamIntf", "[%s] skype :: LTR :: count:%d", __FUNCTION__, param->m.value);

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
        }

        return ret;
    }

    static FilterParamSetterRet cnvSkypeLTR(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<SkypeParamIntf>(intf);

        auto id = intfImpl->findFilterID("enc");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamSkypeLTR>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamSkypeLTR>>(filterParam->getBaseParam());

            auto ltr = intfImpl->getSkypeLTR();

            param->m.mark = ltr->mark_frame;
            param->m.use  = ltr->use_frame;

            StaticExynosLog(Level::Essential, "SkypeParamIntf", "[%s] skype :: LTR :: mark:%d, use:%d",
                                __FUNCTION__, param->m.mark, param->m.use);

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
        }

        return ret;
    }

    static FilterParamSetterRet cnvSkypeSar(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<SkypeParamIntf>(intf);

        auto id = intfImpl->findFilterID("enc");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamSkypeSAR>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamSkypeSAR>>(filterParam->getBaseParam());

            auto sar = intfImpl->getSkypeSar();

            param->m.width  = sar->width;
            param->m.height = sar->height;

            StaticExynosLog(Level::Essential, "SkypeParamIntf", "[%s] skype :: SAR :: width:%d, height:%d",
                                __FUNCTION__, param->m.width, param->m.height);

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
        }

        return ret;
    }

    static FilterParamSetterRet cnvSkypeSliceHeaderSpacing(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<SkypeParamIntf>(intf);

        auto size = intfImpl->getSkypeSliceHeaderSpacing();
        if (size > 0) {
            auto id = intfImpl->findFilterID("enc");
            if (id != 0) {  /* target filter is valid */
                auto filterParam = std::make_shared<ExynosFilterParam<ParamSliceSize>>();
                auto param       = std::static_pointer_cast<ExynosParam<ParamSliceSize>>(filterParam->getBaseParam());

                param->m.mode = SLICE_MODE_MB;
                param->m.size = size;

                StaticExynosLog(Level::Essential, "SkypeParamIntf", "[%s] skype :: Slice Header Spacing :: size:%d",
                                    __FUNCTION__, param->m.size);

                filterParam->registTargetFilter(id);

                ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
            }
        }

        return ret;
    }

    static FilterParamSetterRet cnvSkypeFrameQp(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<SkypeParamIntf>(intf);

        auto id = intfImpl->findFilterID("enc");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamSkypeFrameQP>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamSkypeFrameQP>>(filterParam->getBaseParam());

            param->m.value = intfImpl->getSkypeFrameQp();

            StaticExynosLog(Level::Essential, "SkypeParamIntf", "[%s] skype :: frame QP:%d", __FUNCTION__, param->m.value);

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
        }

        return ret;
    }

    static FilterParamSetterRet cnvSkypeBaseLayerPid(std::shared_ptr<C2InterfaceHelper> intf) {
        FilterParamSetterRet ret;

        if (intf.get() == nullptr) {
            ret.clear();
            return ret;
        }

        auto intfImpl = std::static_pointer_cast<SkypeParamIntf>(intf);

        auto id = intfImpl->findFilterID("enc");
        if (id != 0) {  /* target filter is valid */
            auto filterParam = std::make_shared<ExynosFilterParam<ParamSkypeBaseLayerPid>>();
            auto param       = std::static_pointer_cast<ExynosParam<ParamSkypeBaseLayerPid>>(filterParam->getBaseParam());

            param->m.value = intfImpl->getSkypeBaseLayerPid();

            StaticExynosLog(Level::Essential, "SkypeParamIntf", "[%s] skype :: base layer pid:%d", __FUNCTION__, param->m.value);

            filterParam->registTargetFilter(id);

            ret.push_back(std::static_pointer_cast<ExynosFilterParamBase>(filterParam));
        }

        return ret;
    }

    /* getter functions */
    int32_t getSkypeLowLatency() const { return mSkypeLowLatency->enable; }

    int32_t getSkypeInputControl() const { return mSkypeInputcontrol->enable; }

    int32_t getSkypeLTRCount() const { return mSkypeLTRCount->num_ltr_frames; }

    std::shared_ptr<C2ExynosPortLTRTuning::output> getSkypeLTR() const { return mSkypeLTR; }

    std::shared_ptr<C2ExynosPortSarSetting::output> getSkypeSar() const { return mSkypeSar; }

    int32_t getSkypeSliceHeaderSpacing() const { return mSkypeSliceHeaderSpacing->spacing; }

    int32_t getSkypeFrameQp() const { return mSkypeFrameQp->value; }

    int32_t getSkypeBaseLayerPid() const { return mSkypeBaseLayerPID->value; }

protected:
    /* skype */
    std::shared_ptr<C2ExynosPortDriverVersionInfo::output>            mSkypeDriverVersionInfo;
    std::shared_ptr<C2ExynosPortMaxTemporalLayerInfo::output>         mSkypeMaxTemporalLayerInfo;
    std::shared_ptr<C2ExynosPortPreprocessInfo::output>               mSkypePreprocessInfo;
    std::shared_ptr<C2ExynosPortMaxLTRInfo::output>                   mSkypeMaxLTRInfo;
    std::shared_ptr<C2ExynosPortSkypeLowLatencySetting::output>       mSkypeLowLatency;
    std::shared_ptr<C2ExynosPortInputControlSetting::output>          mSkypeInputcontrol;
    std::shared_ptr<C2ExynosPortCustomProfileLevelSetting::output>    mSkypeCustomProfileLevel;
    std::shared_ptr<C2ExynosPortLTRCountSetting::output>              mSkypeLTRCount;
    std::shared_ptr<C2ExynosPortLTRTuning::output>                    mSkypeLTR;
    std::shared_ptr<C2ExynosPortSarSetting::output>                   mSkypeSar;
    std::shared_ptr<C2ExynosPortSliceHeaderSpacingSetting::output>    mSkypeSliceHeaderSpacing;
    std::shared_ptr<C2ExynosPortFrameQpTuning::output>                mSkypeFrameQp;
    std::shared_ptr<C2ExynosPortBaseLayerPidTuning::output>           mSkypeBaseLayerPID;
    std::shared_ptr<C2ExynosPortSkypeBitrateModeSetting::output>      mSkypeBitrateMode;

private:
};

void skypeSetup(std::shared_ptr<ExynosC2Component::CommonParamIntf> paramIntf, int32_t enable,
                   std::shared_ptr<C2StreamTemporalLayeringTuning::output> layering) {
    /* for skype */
    {
        auto encIntf = std::static_pointer_cast<ExynosC2EncComponent::VencCommonParamIntf>(paramIntf);

        if (enable == On) {
            /* max temporal layer */
            if ((layering.get() != nullptr) &&
                (layering->m.layerCount > 0)) {
                auto max = layering->m.layerCount + layering->m.bLayerCount;

                auto id = encIntf->findFilterID("enc");
                if (id != 0) {  /* target filter is valid */
                    auto filterParam = std::make_shared<ExynosFilterParam<ParamMaxLayering>>();
                    auto param       = std::static_pointer_cast<ExynosParam<ParamMaxLayering>>(filterParam->getBaseParam());

                    param->m.layerCount = max;

                    StaticExynosLog(Level::Debug, "SkypeParamIntf", "[%s] skype :: max temporal layer count: %d", __FUNCTION__, max);

                    filterParam->registTargetFilter(id);

                    encIntf->keepFilterParam({ filterParam });
                }
            }
        }
    }
}

#endif // EXYNOS_C2_SKYPE_H

