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

#include "Walrus.h"

#include "runtime/ObjectType.h"
#include "runtime/Module.h"

namespace Walrus {

bool FunctionType::equals(const FunctionType* other) const
{
    if (this == other) {
        return true;
    }

    if (m_paramTypes->size() != other->param().size()) {
        return false;
    }

    if (memcmp(m_paramTypes->data(), other->param().data(), sizeof(Value::Type) * other->param().size())) {
        return false;
    }

    if (m_resultTypes->size() != other->result().size()) {
        return false;
    }

    if (memcmp(m_resultTypes->data(), other->result().data(), sizeof(Value::Type) * other->result().size())) {
        return false;
    }

    return true;
}

GlobalType::GlobalType(Value::Type type, bool mut)
    : ObjectType(ObjectType::GlobalKind)
    , m_type(type)
    , m_mutable(mut)
    , m_function(nullptr)
{
#ifndef NDEBUG
    switch (type) {
    case Value::I32:
    case Value::I64:
    case Value::F32:
    case Value::F64:
    case Value::V128:
    case Value::FuncRef:
    case Value::ExternRef:
        return;
    default:
        ASSERT_NOT_REACHED();
        return;
    }
#endif
}

GlobalType::~GlobalType()
{
    if (m_function) {
        delete m_function;
    }
}

} // namespace Walrus
