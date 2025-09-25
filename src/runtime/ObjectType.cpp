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
#include "runtime/GCStruct.h"
#include "runtime/TypeStore.h"

namespace Walrus {

bool FunctionType::equals(const FunctionType* other, bool isSubType) const
{
    if (this == other) {
        return true;
    }

    // TODO: This should work for all types.
    // However, functions defined by API has no type at
    // the moment represented by a null recursive type.
    if (getRecursiveType() != nullptr && other->getRecursiveType() != nullptr) {
        if (isSubType) {
            return other->isSubTypeOf(this);
        }
        return getRecursiveType() == other->getRecursiveType();
    }

    if (getRecursiveType() != nullptr && !getRecursiveType()->isSingleType()) {
        return false;
    }

    if (other->getRecursiveType() != nullptr && !other->getRecursiveType()->isSingleType()) {
        return false;
    }

    if (m_paramTypes->size() != other->param().size()) {
        return false;
    }

    if (memcmp(m_paramTypes->data(), other->param().data(), sizeof(Type) * other->param().size())) {
        return false;
    }

    if (m_resultTypes->size() != other->result().size()) {
        return false;
    }

    if (memcmp(m_resultTypes->data(), other->result().data(), sizeof(Type) * other->result().size())) {
        return false;
    }

    return true;
}

bool StructType::initialize()
{
    size_t fieldCount = fields().size();
    m_fieldOffsets.reserve(fieldCount);

    uint32_t offset = sizeof(GCStruct);
    uint32_t align = sizeof(void*);

    for (size_t i = 0; i < fieldCount; i++) {
        Value::Type type = fields()[i].type();
        uint32_t size;

        if (type == Value::I8) {
            size = 1;
        } else if (type == Value::I16) {
            size = 2;
        } else {
            size = static_cast<uint32_t>(valueSize(fields()[i]));
        }

        if (align < size) {
            align = size;
        }

        uint32_t newOffset = (offset + (size - 1)) & ~(size - 1);
        if (newOffset < offset) {
            // Overflow check
            return false;
        }

        m_fieldOffsets[i] = newOffset;
        offset = newOffset + size;
    }

    m_structSize = (offset + (align - 1)) & ~(align - 1);
    // Overflow check
    return m_structSize >= offset;
}

GlobalType::GlobalType(const MutableType& type)
    : ObjectType(ObjectType::GlobalKind)
    , m_type(type)
    , m_function(nullptr)
{
#ifndef NDEBUG
    switch (type.type()) {
    case Value::I32:
    case Value::I64:
    case Value::F32:
    case Value::F64:
    case Value::V128:
        return;
    default:
        ASSERT(type.isRef());
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
