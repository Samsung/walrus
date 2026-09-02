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

#include "GCUtil.h"
#include "runtime/JITExec.h"
#include "runtime/Instance.h"
#include "runtime/Module.h"
#include "runtime/Trap.h"
#include "runtime/Value.h"

namespace Walrus {

ByteCodeStackOffset* JITFunction::call(ExecutionContext& context, uint8_t* bp) const
{
    ASSERT(m_exportEntry);

    ExecutionState& state = context.state;
    ByteCodeStackOffset* resultOffsets = m_module->exportCall()(&context, bp, m_exportEntry);

    if (context.error != ExecutionContext::NoError) {
        if (UNLIKELY(context.ownedFrame != nullptr)) {
#ifdef ENABLE_GC
            GC_FREE(context.ownedFrame);
#else
            free(context.ownedFrame);
#endif
            context.ownedFrame = nullptr;
        }

        switch (context.error) {
        case ExecutionContext::CapturedException:
            throw std::unique_ptr<Exception>(context.capturedException);
        case ExecutionContext::OutOfStackError:
            Trap::throwException("call stack exhausted");
            return resultOffsets;
        case ExecutionContext::DivideByZeroError:
            Trap::throwException("integer divide by zero");
            return resultOffsets;
        case ExecutionContext::IntegerOverflowError:
            Trap::throwException("integer overflow");
            return resultOffsets;
        case ExecutionContext::TypeMismatchError:
            Trap::throwException("type mismatch");
            return resultOffsets;
        case ExecutionContext::AllocationError:
            Trap::throwException("memory allocation failed");
            return resultOffsets;
        case ExecutionContext::OutOfBoundsArrayAccessError:
            COMPILE_ASSERT(ExecutionContext::AllocationError + 1 == ExecutionContext::OutOfBoundsArrayAccessError,
                           "AllocationError and OutOfBoundsArrayAccessError errors must follow each other");
            Trap::throwException("out of bounds array access");
            return resultOffsets;
        case ExecutionContext::OutOfBoundsMemAccessError:
            COMPILE_ASSERT(ExecutionContext::AllocationError + 2 == ExecutionContext::OutOfBoundsMemAccessError,
                           "AllocationError and OutOfBoundsMemAccessError errors must follow each other");
            Trap::throwException("out of bounds memory access");
            return resultOffsets;
        case ExecutionContext::OutOfBoundsTableAccessError:
            COMPILE_ASSERT(ExecutionContext::AllocationError + 3 == ExecutionContext::OutOfBoundsTableAccessError,
                           "AllocationError and OutOfBoundsTableAccessError errors must follow each other");
            Trap::throwException("out of bounds table access");
            return resultOffsets;
        case ExecutionContext::NullReferenceError:
            Trap::throwException("null reference");
            return resultOffsets;
        case ExecutionContext::NullFunctionReferenceError:
            Trap::throwException("null function reference");
            return resultOffsets;
        case ExecutionContext::NullI31ReferenceError:
            Trap::throwException("null i31 reference");
            return resultOffsets;
        case ExecutionContext::NullArrayReferenceError:
            Trap::throwException("null array reference");
            return resultOffsets;
        case ExecutionContext::NullStructReferenceError:
            Trap::throwException("null structure reference");
            return resultOffsets;
        case ExecutionContext::UndefinedElementError:
            Trap::throwException("undefined element");
            return resultOffsets;
        case ExecutionContext::UninitializedElementError:
            Trap::throwException("uninitialized element");
            return resultOffsets;
        case ExecutionContext::IndirectCallTypeMismatchError:
            Trap::throwException("indirect call type mismatch");
            return resultOffsets;
        case ExecutionContext::CallRefTypeMismatchError:
            Trap::throwException("call by reference type mismatch");
            return resultOffsets;
        case ExecutionContext::CastFailureError:
            Trap::throwException("cast failure");
            return resultOffsets;
        case ExecutionContext::InvalidConversionToIntegerError:
            Trap::throwException("invalid conversion to integer");
            return resultOffsets;
        case ExecutionContext::UnreachableError:
            Trap::throwException("unreachable executed");
            return resultOffsets;
        case ExecutionContext::UnalignedAtomicError:
            Trap::throwException("unaligned atomic");
            return resultOffsets;
        case ExecutionContext::ExpectedSharedMemError:
            Trap::throwException("expected shared memory");
            return resultOffsets;
        default:
            Trap::throwException("unknown exception");
            return resultOffsets;
        }
    }

    return resultOffsets;
}

} // namespace Walrus
