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

namespace Walrus {

Table* Table::createTable(Store* store, Value::Type type, uint32_t initialSize, uint32_t maximumSize)
{
    Table* tbl = new Table(type, initialSize, maximumSize);
    store->appendExtern(tbl);

    return tbl;
}

Table::Table(Value::Type type, uint32_t initialSize, uint32_t maximumSize)
    : m_type(type)
    , m_size(initialSize)
    , m_maximumSize(maximumSize)
{
    m_elements.resize(initialSize, reinterpret_cast<void*>(Value::NullBits));
}

void Table::init(ExecutionState& state, Instance* instance, ElementSegment* source, uint32_t dstStart, uint32_t srcStart, uint32_t srcSize)
{
    if (UNLIKELY((uint64_t)dstStart + (uint64_t)srcSize > (uint64_t)m_size)) {
        throwException(state);
    }
    if (UNLIKELY(!source->element() || (srcStart + srcSize) > source->element()->exprFunctions().size())) {
        throwException(state);
    }
    if (UNLIKELY(m_type != Value::Type::FuncRef)) {
        Trap::throwException(state, "type mismatch");
    }

    this->initTable(instance, source, dstStart, srcStart, srcSize);
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

void Table::initTable(Instance* instance, ElementSegment* source, uint32_t dstStart, uint32_t srcStart, uint32_t srcSize)
{
    const auto& exprs = source->element()->exprFunctions();
    uint32_t end = dstStart + srcSize;

    for (uint32_t i = dstStart; i < end; i++) {
        ModuleFunction* exprFunc = exprs[srcStart++];

        struct RunData {
            Instance* instance;
            ModuleFunction* exprFunc;
            Function* func;
        } data = { instance, exprFunc, nullptr };
        Walrus::Trap trap;
        trap.run([](Walrus::ExecutionState& state, void* d) {
            RunData* data = reinterpret_cast<RunData*>(d);
            DefinedFunctionWithTryCatch fakeFunction(data->instance, data->exprFunc);
            Value func;
            fakeFunction.call(state, nullptr, &func);
            data->func = func.asFunction();
        },
                 &data);

        m_elements[i] = data.func;
    }
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
