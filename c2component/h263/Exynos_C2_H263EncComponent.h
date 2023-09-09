/*
 *
 * Copyright 2019 Samsung Electronics S.LSI Co. LTD
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
#ifndef EXYNOS_C2_H263ENC_COMPONENT_H
#define EXYNOS_C2_H263ENC_COMPONENT_H

#include <memory>

#include "Exynos_C2_EncComponent.h"
#include "Exynos_H263Enc_Filter.h"
#include "Exynos_CSC_Filter.h"

#define LOG_ON
#include "ExynosLog.h"

class ExynosC2H263EncComponent : public ExynosC2EncComponent/*, public std::enable_shared_from_this<ExynosC2H263EncComponent>*/ {
public:
    class H263EncParamIntf;

    ExynosC2H263EncComponent(c2_node_id_t id, const std::shared_ptr<H263EncParamIntf> &intfImpl);

    ~ExynosC2H263EncComponent() {
        mParamIntf.reset();
    }

    /* add function for ExynosC2H263EncComponent */
    bool init(std::weak_ptr<C2Component> wkComponent);

private:
    /* override function on ExynosC2EncComponent */
    c2_status_t onSetup() override;

    /* disable default constructor */
    ExynosC2H263EncComponent() = delete;
};

#ifdef __cplusplus
extern "C" {
#endif

EXYNOS_EXPORT_REF ::C2ComponentFactory* CreateCodec2Factory();
EXYNOS_EXPORT_REF void DestroyCodec2Factory(::C2ComponentFactory *factory);

#ifdef __cplusplus
}
#endif

#endif // EXYNOS_C2_H263ENC_COMPONENT_H
