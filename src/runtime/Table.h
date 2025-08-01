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
    static Table* createTable(Store* store, Type type, uint32_t initialSize, uint32_t maximumSize, void* init = nullptr);

    virtual Object::Kind kind() const override
    {
        return Object::TableKind;
    }

    virtual bool isTable() const override
    {
        return true;
    }

    Type type() const
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

    void grow(uint64_t newSize, void* val)
    {
        ASSERT(newSize <= m_maximumSize);
        m_elements.resize(newSize, val);
        m_size = newSize;
    }

    void* getElement(ExecutionState& state, uint32_t elemIndex) const
    {
        if (UNLIKELY(elemIndex >= m_size)) {
            throwException(state);
        }
        return m_elements[elemIndex];
    }

    void* uncheckedGetElement(uint32_t elemIndex) const
    {
        ASSERT(elemIndex < m_size);
        return m_elements[elemIndex];
    }

    void setElement(ExecutionState& state, uint32_t elemIndex, void* val)
    {
        if (UNLIKELY(elemIndex >= m_size)) {
            throwException(state);
        }
        m_elements[elemIndex] = val;
    }

    void uncheckedSetElement(uint32_t elemIndex, void* val)
    {
        ASSERT(elemIndex < m_size);
        m_elements[elemIndex] = val;
    }

    void copy(ExecutionState& state, const Table* srcTable, uint32_t n, uint32_t srcIndex, uint32_t dstIndex);
    void fill(ExecutionState& state, uint32_t n, void* value, uint32_t index);
    void init(ExecutionState& state, Instance* instance, ElementSegment* source, uint32_t dstStart, uint32_t srcStart, uint32_t srcSize);

    void initTable(Instance* instance, ElementSegment* source, uint32_t dstStart, uint32_t srcStart, uint32_t srcSize);
    void copyTable(const Table* srcTable, uint32_t n, uint32_t srcIndex, uint32_t dstIndex);
    void fillTable(uint32_t n, void* value, uint32_t index);

private:
    Table(Type type, uint32_t initialSize, uint32_t maximumSize, void* init);

    void throwException(ExecutionState& state) const;

    // Table has elements of reference type (FuncRef | ExternRef)
    Type m_type;
    uint32_t m_size;
    uint32_t m_maximumSize;

    // FIXME handle references of Function objects
    Vector<void*> m_elements;
};

} // namespace Walrus

#endif // __WalrusTable__
