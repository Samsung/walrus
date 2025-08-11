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

#include "Global.h"

namespace Walrus {

DEFINE_GLOBAL_TYPE_INFO(globalTypeInfo, GlobalKind);

Global::Global(const Value& value, const MutableType& type)
    : Extern(GET_GLOBAL_TYPE_INFO(globalTypeInfo))
    , m_value(value)
    , m_type(type)
{
}

} // namespace Walrus
