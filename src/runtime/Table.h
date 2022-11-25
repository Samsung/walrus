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

#ifndef __WalrusTable__
#define __WalrusTable__

#include "runtime/Value.h"

namespace Walrus {

class Table : public gc {
public:
    Table(Value::Type type, size_t initialSize, size_t maximumSize);

    Value::Type type() const
    {
        return m_type;
    }

    size_t size() const
    {
        return m_size;
    }

    size_t maximumSize() const
    {
        return m_maximumSize;
    }

    void grow(size_t newSize, const Value& val)
    {
        ASSERT(newSize <= m_maximumSize);
        m_elements.resize(newSize, val);
        m_size = newSize;
    }

    Value getElement(uint32_t elemIndex) const
    {
        if (UNLIKELY(elemIndex >= m_size)) {
            throwException();
        }
        return m_elements[elemIndex];
    }

    void setElement(uint32_t elemIndex, const Value& val)
    {
        ASSERT(val.type() == m_type);
        if (UNLIKELY(elemIndex >= m_size)) {
            throwException();
        }
        m_elements[elemIndex] = val;
    }

    void copy(const Table* srcTable, int32_t n, int32_t srcIndex, int32_t dstIndex);
    void fill(int32_t n, const Value& value, int32_t index);

private:
    void throwException() const;

    // Table has elements of reference type (FuncRef | ExternRef)
    Value::Type m_type;
    size_t m_size;
    size_t m_maximumSize;

    ValueVector m_elements;
};

} // namespace Walrus

#endif // __WalrusInstance__
