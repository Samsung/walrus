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

#include "jit/Allocator.h"
#include "runtime/JITExec.h"
#include "runtime/Trap.h"
#include "runtime/Value.h"

namespace Walrus {

void JITFunction::call(ExecutionState& state, const uint32_t argc, Value* argv, Value* result, const ValueTypeVector& resultTypeInfo) const
{
    ASSERT(m_exportEntry);

    size_t size = 4096;

    const size_t alignment = LocalsAllocator::alignment;
    void* data = malloc(size + alignment + sizeof(ExecutionContext));
    uintptr_t start = reinterpret_cast<uintptr_t>(data);

    start = (start + (alignment - 1)) & ~(alignment - 1);

    ExecutionContext* context = reinterpret_cast<ExecutionContext*>(start);

    context->lastFrame = nullptr;
    context->error = ExecutionContext::NoError;
    context->currentInstanceData = nullptr;
    context->currentInstanceConstData = m_module->instanceConstData();

    void* args = reinterpret_cast<void*>(start + size - argsSize());
    StackAllocator stackAllocator;

    for (Value::Type resultType : resultTypeInfo) {
        stackAllocator.push(LocationInfo::typeToValueInfo(resultType));
    }

    if (argc > 0) {
        LocalsAllocator localsAllocator(stackAllocator.size());

        Value* argvEnd = argv + argc;

        while (argv < argvEnd) {
            localsAllocator.allocate(LocationInfo::typeToValueInfo(argv->type()));
            argv->writeToMemory(reinterpret_cast<uint8_t*>(args) + localsAllocator.last().value);
            ++argv;
        }
    }

    m_module->exportCall()(context + 1, args, m_exportEntry);

    if (context->error != ExecutionContext::NoError) {
        switch (context->error) {
        case ExecutionContext::OutOfStackError:
            Trap::throwException(state, "call stack exhausted");
            return;
        case ExecutionContext::DivideByZeroError:
            Trap::throwException(state, "integer divide by zero");
            return;
        case ExecutionContext::IntegerOverflowError:
            Trap::throwException(state, "integer overflow");
            return;
        case ExecutionContext::UnreachableError:
            Trap::throwException(state, "unreachable executed");
            return;
        case ExecutionContext::MemoryOutOfBoundsError:
            Trap::throwException(state, "out of bounds memory access");
            return;
        default:
            Trap::throwException(state, "unknown exception");
            return;
        }
        return;
    }

    int resultIndex = 0;
    std::vector<LocationInfo>& offsets = stackAllocator.values();

    for (Value::Type resultType : resultTypeInfo) {
        result[resultIndex] = Value(resultType, reinterpret_cast<uint8_t*>(args) + offsets[resultIndex].value);
        resultIndex++;
    }

    free(data);
}

} // namespace Walrus
