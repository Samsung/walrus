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

#include "Table.h"
#include "runtime/Trap.h"
#include "runtime/Instance.h"
#include "runtime/Module.h"

namespace Walrus {

Table::Table(Value::Type type, size_t initialSize, size_t maximumSize)
    : m_type(type)
    , m_size(initialSize)
    , m_maximumSize(maximumSize)
{
    m_elements.resize(initialSize, Value(Value::Null));
}

void Table::copy(const Table* srcTable, int32_t n, int32_t srcIndex, int32_t dstIndex)
{
    if (UNLIKELY(((size_t)srcIndex + n > srcTable->size()) || ((size_t)dstIndex + n > m_size))) {
        throwException();
    }

    while (n > 0) {
        if (dstIndex <= srcIndex) {
            m_elements[dstIndex] = srcTable->getElement(srcIndex);
            dstIndex++;
            srcIndex++;
        } else {
            m_elements[dstIndex + n - 1] = srcTable->getElement(srcIndex + n - 1);
        }
        n--;
    }
}

void Table::fill(int32_t n, const Value& value, int32_t index)
{
    if ((size_t)index + n > m_size) {
        throwException();
    }

    while (n > 0) {
        m_elements[index] = value;
        n--;
        index++;
    }
}

void Table::throwException() const
{
    Trap::throwException("out of bounds table access");
}

void Table::init(Instance* instance, ElementSegment* source, uint32_t dstStart, uint32_t srcStart, uint32_t srcSize)
{
    if (UNLIKELY(dstStart + srcSize > m_size)) {
        throwException();
    }
    if (UNLIKELY(!source->element() || (srcStart + srcSize) > source->element()->functionIndex().size())) {
        throwException();
    }
    if (m_type != Value::Type::FuncRef) {
        Trap::throwException("type mismatch");
    }

    const auto& f = source->element()->functionIndex();
    size_t end = dstStart + srcSize;
    for (size_t i = dstStart; i < end; i++) {
        auto idx = f[srcStart++];
        if (idx != std::numeric_limits<uint32_t>::max()) {
            m_elements[i] = Value(instance->function(idx));
        } else {
            m_elements[i] = Value(Value::Null);
        }
    }
}

} // namespace Walrus
