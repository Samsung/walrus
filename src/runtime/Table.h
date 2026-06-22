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

#include "runtime/Type.h"
#include "runtime/Value.h"
#include "runtime/Object.h"

namespace Walrus {

class Store;
class ElementSegment;
class Instance;

class Table : public Extern {
    friend class JITFieldAccessor;

public:
    static Table* createTable(Store* store, Type type, uint64_t initialSize, uint64_t maximumSize, bool is64, void* init = nullptr);

    ~Table();

    Type type() const
    {
        return m_type;
    }

    bool is64() const
    {
        return m_is64;
    }

    uint64_t size() const
    {
        return m_size;
    }

    uint64_t maximumSize() const
    {
        return m_maximumSize;
    }

    void* getElement(ExecutionState& state, uint32_t elemIndex) const
    {
        ASSERT(!m_is64);
        if (UNLIKELY(elemIndex >= m_size)) {
            throwException(state);
        }
        return m_elements[elemIndex];
    }

    void* getElement64(ExecutionState& state, uint64_t elemIndex) const
    {
        ASSERT(m_is64);
        if (UNLIKELY(elemIndex >= m_size)) {
            throwException(state);
        }
        return m_elements[elemIndex];
    }

    void* uncheckedGetElement(uint32_t elemIndex) const
    {
        ASSERT(!m_is64 && elemIndex < m_size);
        return m_elements[elemIndex];
    }

    void* uncheckedGetElement64(uint64_t elemIndex) const
    {
        ASSERT(m_is64 && elemIndex < m_size);
        return m_elements[elemIndex];
    }

    void setElement(ExecutionState& state, uint32_t elemIndex, void* val)
    {
        ASSERT(!m_is64);
        if (UNLIKELY(elemIndex >= m_size)) {
            throwException(state);
        }
        m_elements[elemIndex] = val;
    }

    void setElement64(ExecutionState& state, uint64_t elemIndex, void* val)
    {
        ASSERT(m_is64);
        if (UNLIKELY(elemIndex >= m_size)) {
            throwException(state);
        }
        m_elements[elemIndex] = val;
    }

    void uncheckedSetElement(uint32_t elemIndex, void* val)
    {
        ASSERT(!m_is64 && elemIndex < m_size);
        m_elements[elemIndex] = val;
    }

    void uncheckedSetElement64(uint64_t elemIndex, void* val)
    {
        ASSERT(m_is64 && elemIndex < m_size);
        m_elements[elemIndex] = val;
    }

    void grow(uint64_t newSize, void* val);
    void copy(ExecutionState& state, const Table* srcTable, uint64_t n, uint64_t srcIndex, uint64_t dstIndex);
    void fill(ExecutionState& state, uint64_t n, void* value, uint64_t index);
    void init(ExecutionState& state, ElementSegment* source, uint64_t dstStart, uint64_t srcStart, uint64_t srcSize);

    void initTable(ElementSegment* source, uint64_t dstStart, uint64_t srcStart, uint64_t srcSize);
    void copyTable(const Table* srcTable, uint64_t n, uint64_t srcIndex, uint64_t dstIndex);
    void fillTable(uint64_t n, void* value, uint64_t index);

private:
    Table(Type type, uint64_t initialSize, uint64_t maximumSize, bool is64, void* init);

    bool isValidRange(uint64_t start, uint64_t size) const
    {
        return size <= m_size && start <= m_size - size;
    }

    void throwException(ExecutionState& state) const;

    // Table has elements of reference type (FuncRef | ExternRef)
    Type m_type;
    bool m_is64;
    uint64_t m_size;
    uint64_t m_maximumSize;

    // FIXME handle references of Function objects
    void** m_elements;
};

} // namespace Walrus

#endif // __WalrusTable__
