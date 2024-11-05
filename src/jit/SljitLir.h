/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#ifndef __WalrusSljitLir__
#define __WalrusSljitLir__

#if defined(WALRUS_ENABLE_JIT)

// Setup the configuration
#define SLJIT_CONFIG_AUTO 1
#define SLJIT_CONFIG_STATIC 1
#define SLJIT_VERBOSE 0

#if defined(NDEBUG)
#define SLJIT_DEBUG 0
#else
#define SLJIT_DEBUG 1
#endif

extern "C" {
#include "../../third_party/sljit/sljit_src/sljitLir.h"
}

#endif // WALRUS_ENABLE_JIT
#endif // __WalrusSljitLir__
