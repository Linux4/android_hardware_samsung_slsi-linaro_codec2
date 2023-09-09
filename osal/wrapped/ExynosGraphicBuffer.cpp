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
#include "ExynosGraphicBuffer.h"

#define LOG_ON
#include "ExynosLog.h"
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "ExynosGraphicBufferWrapper"

int vendor::graphics::ExynosGraphicBufferMeta::get_video_metadata_fd(buffer_handle_t hnd) {
    return VendorGraphicBufferMeta::get_video_metadata_fd(hnd);
}

int vendor::graphics::ExynosGraphicBufferMeta::set_dataspace(
    buffer_handle_t     hnd,
    android_dataspace_t dataspace) {
    return VendorGraphicBufferMeta::set_dataspace(hnd, dataspace);
}

uint64_t vendor::graphics::ExynosGraphicBufferMeta::get_internal_format(buffer_handle_t hnd) {
    return VendorGraphicBufferMeta::get_internal_format(hnd);
}
