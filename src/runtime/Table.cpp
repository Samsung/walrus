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

} // namespace Walrus
