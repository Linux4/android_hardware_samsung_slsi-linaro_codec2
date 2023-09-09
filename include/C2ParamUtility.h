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
#ifndef C2_PARAM_UTILITY_H
#define C2_PARAM_UTILITY_H

#include <memory>

#include "ExynosDef.h"
#include "C2ExynosParam.h"
#include "ExynosLog.h"

inline void updateHdrStaticInfo(std::shared_ptr<C2StreamHdrStaticInfo::output> codedStaticInfo, ExynosHdrStaticInfo ST) {
    codedStaticInfo->maxCll  = ST.sType1.mMaxContentLightLevel;       /* cd/m^2 */
    codedStaticInfo->maxFall = ST.sType1.mMaxFrameAverageLightLevel;  /* cd/m^2 */

    codedStaticInfo->mastering.red.x         = ST.sType1.mR.x * 0.00002f;
    codedStaticInfo->mastering.red.y         = ST.sType1.mR.y * 0.00002f;
    codedStaticInfo->mastering.green.x       = ST.sType1.mG.x * 0.00002f;
    codedStaticInfo->mastering.green.y       = ST.sType1.mG.y * 0.00002f;
    codedStaticInfo->mastering.blue.x        = ST.sType1.mB.x * 0.00002f;
    codedStaticInfo->mastering.blue.y        = ST.sType1.mB.y * 0.00002f;
    codedStaticInfo->mastering.white.x       = ST.sType1.mW.x * 0.00002f;
    codedStaticInfo->mastering.white.y       = ST.sType1.mW.y * 0.00002f;
    codedStaticInfo->mastering.maxLuminance  = ST.sType1.mMaxDisplayLuminance;
    codedStaticInfo->mastering.minLuminance  = ST.sType1.mMinDisplayLuminance * 0.0001f;  /* convert 0.0001cd/m^2 to cd/m^2 */

    StaticExynosLog(Level::Debug, "ExynosC2ParamUtility", "[%s] HDR :: static info[bitstream] r(%d, %d), g(%d, %d), b(%d, %d), w(%d, %d), max(%d), min(%d), cll(%d), fall(%d)"
        , __FUNCTION__,
        ST.sType1.mR.x, ST.sType1.mR.y,
        ST.sType1.mG.x, ST.sType1.mG.y,
        ST.sType1.mB.x, ST.sType1.mB.y,
        ST.sType1.mW.x, ST.sType1.mW.y,
        ST.sType1.mMaxDisplayLuminance, ST.sType1.mMinDisplayLuminance,
        ST.sType1.mMaxContentLightLevel, ST.sType1.mMaxFrameAverageLightLevel);
    StaticExynosLog(Level::Info, "ExynosC2ParamUtility", "[%s] HDR :: static info[framework] r(%f, %f), g(%f, %f), b(%f, %f), w(%f, %f), max(%f), min(%f), cll(%f), fall(%f)",
        __FUNCTION__,
        codedStaticInfo->mastering.red.x, codedStaticInfo->mastering.red.y,
        codedStaticInfo->mastering.green.x, codedStaticInfo->mastering.green.y,
        codedStaticInfo->mastering.blue.x, codedStaticInfo->mastering.blue.y,
        codedStaticInfo->mastering.white.x, codedStaticInfo->mastering.white.y,
        codedStaticInfo->mastering.maxLuminance, codedStaticInfo->mastering.minLuminance,
        codedStaticInfo->maxCll, codedStaticInfo->maxFall);
}

#endif // C2_PARAM_UTILITY_H

