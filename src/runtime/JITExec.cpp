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

#include "runtime/JITExec.h"
#include "runtime/Instance.h"
#include "runtime/Module.h"
#include "runtime/Trap.h"
#include "runtime/Value.h"

namespace Walrus {

ByteCodeStackOffset* JITFunction::call(ExecutionState& state, Instance* instance, uint8_t* bp) const
{
    ASSERT(m_exportEntry);

    ExecutionContext context(m_module->instanceConstData(), state, instance);
    Memory* memory0 = nullptr;

    if (instance->module()->numberOfMemoryTypes() > 0) {
        memory0 = instance->memory(0);
        memory0->push(&context.memory0);
    }

    ByteCodeStackOffset* resultOffsets = m_module->exportCall()(&context, bp, m_exportEntry);

    if (memory0 != nullptr) {
        memory0->pop(&context.memory0);
    }

    if (context.error != ExecutionContext::NoError) {
        switch (context.error) {
        case ExecutionContext::CapturedException:
            throw std::unique_ptr<Exception>(context.capturedException);
        case ExecutionContext::OutOfStackError:
            Trap::throwException(state, "call stack exhausted");
            return resultOffsets;
        case ExecutionContext::DivideByZeroError:
            Trap::throwException(state, "integer divide by zero");
            return resultOffsets;
        case ExecutionContext::IntegerOverflowError:
            Trap::throwException(state, "integer overflow");
            return resultOffsets;
        case ExecutionContext::OutOfBoundsMemAccessError:
            Trap::throwException(state, "out of bounds memory access");
            return resultOffsets;
        case ExecutionContext::OutOfBoundsTableAccessError:
            Trap::throwException(state, "out of bounds table access");
            return resultOffsets;
        case ExecutionContext::TypeMismatchError:
            Trap::throwException(state, "type mismatch");
            return resultOffsets;
        case ExecutionContext::UndefinedElementError:
            Trap::throwException(state, "undefined element");
            return resultOffsets;
        case ExecutionContext::UninitializedElementError:
            Trap::throwException(state, "uninitialized element");
            return resultOffsets;
        case ExecutionContext::IndirectCallTypeMismatchError:
            Trap::throwException(state, "indirect call type mismatch");
            return resultOffsets;
        case ExecutionContext::InvalidConversionToIntegerError:
            Trap::throwException(state, "invalid conversion to integer");
            return resultOffsets;
        case ExecutionContext::UnreachableError:
            Trap::throwException(state, "unreachable executed");
            return resultOffsets;
#if defined(ENABLE_EXTENDED_FEATURES)
        case ExecutionContext::UnalignedAtomicError:
            Trap::throwException(state, "unaligned atomic");
            return resultOffsets;
#endif /* ENABLE_EXTENDED_FEATURES */
        default:
            Trap::throwException(state, "unknown exception");
            return resultOffsets;
        }
    }

    return resultOffsets;
}

} // namespace Walrus
