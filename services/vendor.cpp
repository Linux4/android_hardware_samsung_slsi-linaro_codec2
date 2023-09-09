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
// #define LOG_NDEBUG 0
#ifdef USE_CODEC2_HIDL_1_2
#define LOG_TAG "samsung.hardware.media.c2@1.2-service"

#include <codec2/hidl/1.2/ComponentStore.h>
using namespace ::android::hardware::media::c2::V1_2;
#else
#define LOG_TAG "samsung.hardware.media.c2@1.0-service"

#include <codec2/hidl/1.0/ComponentStore.h>
using namespace ::android::hardware::media::c2::V1_0;
#endif

#include <hidl/HidlTransportSupport.h>
#include <binder/ProcessState.h>
#include <minijail.h>

#include <C2Component.h>
#include <C2PlatformSupport.h>

#include "C2ExynosSupport.h"

// OmxStore is added for visibility by dumpstate.
#include <media/stagefright/omx/1.0/OmxStore.h>

// This is created by module "codec2.vendor.base.policy". This can be modified.
static constexpr char kBaseSeccompPolicyPath[] =
        "/vendor/etc/seccomp_policy/codec2.vendor.base.policy";

// Additional device-specific seccomp permissions can be added in this file.
static constexpr char kExtSeccompPolicyPath[] =
        "/vendor/etc/seccomp_policy/codec2.vendor.ext.policy";

int main(int /* argc */, char** /* argv */) {
    ALOGI("media hwcodec service starting");
    signal(SIGPIPE, SIG_IGN);
    android::SetUpMinijail(kBaseSeccompPolicyPath, kExtSeccompPolicyPath);

    // vndbinder is needed by BufferQueue.
    android::ProcessState::initWithDriver("/dev/vndbinder");
    android::ProcessState::self()->startThreadPool();

    // Extra threads may be needed to handle a stacked IPC sequence that
    // contains alternating binder and hwbinder calls. (See b/35283480.)
    android::hardware::configureRpcThreadpool(8, true /* callerWillJoin */);

    /* RegisterCodecServices() */
    {
        android::sp<IComponentStore> store;

        ALOGI("Creating vendor Codec2 service...");

        store = new utils::ComponentStore(std::static_pointer_cast<C2ComponentStore>(android::GetCodec2ExynosComponentStore()));
        if (store == nullptr) {
            ALOGE("Cannot create vendor Codec2 service.");
        } else {
            if (store->registerAsService("default") != android::OK) {
                ALOGE("Cannot register vendor Codec2 service.");
            } else {
                ALOGI("Vendor Codec2 service created.");
            }
        }
    }

    android::hardware::joinRpcThreadpool();

    return 0;
}
