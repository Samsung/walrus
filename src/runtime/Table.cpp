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
#include "Store.h"
#include "runtime/Trap.h"
#include "runtime/Instance.h"
#include "runtime/Module.h"
#include "runtime/Function.h"

#ifdef ENABLE_GC
#include "GCUtil.h"
#endif /* ENABLE_GC */

namespace Walrus {

DEFINE_GLOBAL_TYPE_INFO(tableTypeInfo, TableKind);

Table* Table::createTable(Store* store, Type type, uint32_t initialSize, uint32_t maximumSize, void* init)
{
    Table* tbl = new Table(type, initialSize, maximumSize, init ? init : reinterpret_cast<void*>(Value::NullBits));
    store->appendExtern(tbl);

    return tbl;
}

Table::Table(Type type, uint32_t initialSize, uint32_t maximumSize, void* init)
    : Extern(GET_GLOBAL_TYPE_INFO(tableTypeInfo))
    , m_type(type)
    , m_size(initialSize)
    , m_maximumSize(maximumSize)
{
    if (initialSize == 0) {
        m_elements = nullptr;
        return;
    }
#ifdef ENABLE_GC
    m_elements = reinterpret_cast<void**>(GC_MALLOC_UNCOLLECTABLE(static_cast<size_t>(initialSize) * sizeof(void*)));
#else
    m_elements = reinterpret_cast<void**>(malloc(static_cast<size_t>(initialSize) * sizeof(void*)));
#endif
    std::fill(m_elements, m_elements + initialSize, init);
}

Table::~Table()
{
#ifdef ENABLE_GC
    GC_FREE(m_elements);
#else
    free(m_elements);
#endif
}

void Table::grow(uint64_t newSize, void* val)
{
    ASSERT(newSize <= m_maximumSize);
    if (newSize == m_size) {
        return;
    }
#ifdef ENABLE_GC
    if (LIKELY(m_elements != nullptr)) {
        m_elements = reinterpret_cast<void**>(GC_REALLOC(m_elements, static_cast<size_t>(newSize) * sizeof(void*)));
    } else {
        m_elements = reinterpret_cast<void**>(GC_MALLOC_UNCOLLECTABLE(static_cast<size_t>(newSize) * sizeof(void*)));
    }
#else
    m_elements = reinterpret_cast<void**>(realloc(m_elements, static_cast<size_t>(newSize) * sizeof(void*)));
#endif
    std::fill(m_elements + m_size, m_elements + newSize, val);
    m_size = newSize;
}

void Table::init(ExecutionState& state, ElementSegment* source, uint32_t dstStart, uint32_t srcStart, uint32_t srcSize)
{
    if (UNLIKELY((uint64_t)dstStart + (uint64_t)srcSize > (uint64_t)m_size)) {
        throwException(state);
    }
    if (UNLIKELY((srcStart + srcSize) > source->size())) {
        throwException(state);
    }
    if (UNLIKELY(!m_type.isRef())) {
        Trap::throwException(state, "type mismatch");
    }

    this->initTable(source, dstStart, srcStart, srcSize);
}

void Table::copy(ExecutionState& state, const Table* srcTable, uint32_t n, uint32_t srcIndex, uint32_t dstIndex)
{
    if (UNLIKELY(((uint64_t)srcIndex + (uint64_t)n > (uint64_t)srcTable->size()) || ((uint64_t)dstIndex + (uint64_t)n > (uint64_t)m_size))) {
        throwException(state);
    }

    this->copyTable(srcTable, n, srcIndex, dstIndex);
}

void Table::fill(ExecutionState& state, uint32_t n, void* value, uint32_t index)
{
    if ((uint64_t)index + (uint64_t)n > (uint64_t)m_size) {
        throwException(state);
    }

    this->fillTable(n, value, index);
}

void Table::throwException(ExecutionState& state) const
{
    Trap::throwException(state, "out of bounds table access");
}

void Table::initTable(ElementSegment* source, uint32_t dstStart, uint32_t srcStart, uint32_t srcSize)
{
    memcpy(m_elements + dstStart, source->elements().data() + srcStart, srcSize * sizeof(void*));
}

void Table::copyTable(const Table* srcTable, uint32_t n, uint32_t srcIndex, uint32_t dstIndex)
{
    while (n > 0) {
        if (dstIndex <= srcIndex) {
            m_elements[dstIndex] = srcTable->uncheckedGetElement(srcIndex);
            dstIndex++;
            srcIndex++;
        } else {
            m_elements[dstIndex + n - 1] = srcTable->uncheckedGetElement(srcIndex + n - 1);
        }
        n--;
    }
}

void Table::fillTable(uint32_t n, void* value, uint32_t index)
{
    while (n > 0) {
        m_elements[index] = value;
        n--;
        index++;
    }
}

} // namespace Walrus
