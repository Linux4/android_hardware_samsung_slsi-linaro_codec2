/*
 *
 * Copyright 2020 Samsung Electronics S.LSI Co. LTD
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
#ifndef EXYNOS_PERFORMANCE_H
#define EXYNOS_PERFORMANCE_H

#include <memory>

class ExynosPerformance {
public:
    class ExynosPerformanceImpl;

    ExynosPerformance(bool encoder = false);

    ~ExynosPerformance();

    void notify(uint32_t num, uint32_t fps);

private:
    std::shared_ptr<ExynosPerformanceImpl> mImpl;
};

#endif // EXYNOS_PERFORMANCE_H
