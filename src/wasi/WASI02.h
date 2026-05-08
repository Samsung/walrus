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

#ifndef __WalrusWASI02__
#define __WalrusWASI02__

#ifdef ENABLE_WASI

#include "Walrus.h"
#include "runtime/Component.h"

namespace Walrus {

class ComponentInstance;
class DefinedFunctionTypes;

ComponentInstance* wasi02LoadInstance(ComponentType::External& external, DefinedFunctionTypes& functionTypes);

} // namespace Walrus

#endif

#endif // __WalrusWASI02__
