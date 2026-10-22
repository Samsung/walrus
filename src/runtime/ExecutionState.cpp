/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Walrus.h"
#include "runtime/ExecutionState.h"

#include <cstdint>
#if defined(__linux__)
#include <pthread.h>
#endif

namespace Walrus {
thread_local ExecutionState* ExecutionState::s_ownedFrames;

MAY_THREAD_LOCAL ExecutionState::ThreadStackBound ExecutionState::s_threadStackBound;

void ExecutionState::resolveThreadStackBound()
{
    // 0 (stack growing down) or SIZE_MAX (growing up) never clamps.
#ifdef STACK_GROWS_DOWN
    size_t bound = 0;
#else
    size_t bound = SIZE_MAX;
#endif
#if defined(__linux__)
    // A thread's stack range never changes, and pthread_getattr_np() parses
    // /proc/self/maps for the initial thread, so resolve it once per thread.
    pthread_attr_t attr;
    if (pthread_getattr_np(pthread_self(), &attr) == 0) {
        void* stackAddr = nullptr;
        size_t stackSize = 0;
        if (pthread_attr_getstack(&attr, &stackAddr, &stackSize) == 0 && stackAddr && stackSize) {
            size_t margin = STACK_LIMIT_FROM_END;
            if (margin > stackSize / 2) {
                margin = stackSize / 2;
            }
#ifdef STACK_GROWS_DOWN
            bound = reinterpret_cast<size_t>(stackAddr) + margin;
#else
            bound = reinterpret_cast<size_t>(stackAddr) + stackSize - margin;
#endif
        }
        pthread_attr_destroy(&attr);
    }
#endif
    s_threadStackBound.value = bound;
    s_threadStackBound.resolved = true;
}
} // namespace Walrus
