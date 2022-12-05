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

class ElementSegment;
class Instance;

class Table : public gc {
public:
    Table(Value::Type type, uint32_t initialSize, uint32_t maximumSize);

    Value::Type type() const
    {
        return m_type;
    }

    uint32_t size() const
    {
        return m_size;
    }

    uint32_t maximumSize() const
    {
        return m_maximumSize;
    }

    void grow(uint64_t newSize, const Value& val)
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

    Value uncheckedGetElement(uint32_t elemIndex) const
    {
        return m_elements[elemIndex];
    }

    void setElement(uint32_t elemIndex, const Value& val)
    {
        if (UNLIKELY(elemIndex >= m_size)) {
            throwException();
        }
        m_elements[elemIndex] = val;
    }

    void copy(const Table* srcTable, uint32_t n, uint32_t srcIndex, uint32_t dstIndex);
    void fill(uint32_t n, const Value& value, uint32_t index);
    void init(Instance* instance, ElementSegment* source, uint32_t dstStart, uint32_t srcStart, uint32_t srcSize);

private:
    void throwException() const;

    // Table has elements of reference type (FuncRef | ExternRef)
    Value::Type m_type;
    uint32_t m_size;
    uint32_t m_maximumSize;

    ValueVector m_elements;
};

} // namespace Walrus

#endif // __WalrusInstance__
