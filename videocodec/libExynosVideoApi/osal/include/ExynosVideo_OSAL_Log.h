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

/*
 * @file    ExynosVideo_OSAL_Log.h
 * @brief   ExynosVideo OSAL Log define
 * @author  SeungBeom Kim (sbcrux.kim@samsung.com)
 *          Taehwan Kim   (t_h.kim@samsung.com)
 * @version    1.0.0
 * @history
 *   2021.11.24 : Create
 */

#ifndef _EXYNOS_VIDEO_OSAL_LOG_H_
#define _EXYNOS_VIDEO_OSAL_LOG_H_

#include <log/log.h>

#define STR_INDIR(x) #x
#define TO_STRING(x) STR_INDIR(x)

#define CHECK_POINTER(x) ({                 \
    if (x == NULL) {                        \
        ALOGE("[%s] : [%s] Line %s, Name: %s", __FILE__, __FUNCTION__, TO_STRING(__LINE__), TO_STRING(x)); \
    }                                       \
    (x != NULL);                            \
})

#define CHECK_EQUAL(x, y) ({                \
    if (x != y) {                           \
        ALOGE("[%s] : [%s] Line %s, %s vs %s, not equal", __FILE__, __FUNCTION__, TO_STRING(__LINE__), TO_STRING(x), TO_STRING(y)); \
    }                                       \
    (x == y);                               \
})

#endif
