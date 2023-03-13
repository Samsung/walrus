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
#include "runtime/Trap.h"
#include "runtime/Value.h"

namespace Walrus {

ByteCodeStackOffset* JITFunction::call(ExecutionState& state, uint8_t* bp) const
{
    ASSERT(m_exportEntry);

    ExecutionContext context(m_module->instanceConstData());
    ByteCodeStackOffset* resultOffsets = m_module->exportCall()(&context, bp, m_exportEntry);

    if (context.error != ExecutionContext::NoError) {
        switch (context.error) {
        case ExecutionContext::OutOfStackError:
            Trap::throwException(state, "call stack exhausted");
            return resultOffsets;
        case ExecutionContext::DivideByZeroError:
            Trap::throwException(state, "integer divide by zero");
            return resultOffsets;
        case ExecutionContext::IntegerOverflowError:
            Trap::throwException(state, "integer overflow");
            return resultOffsets;
        case ExecutionContext::UnreachableError:
            Trap::throwException(state, "unreachable executed");
            return resultOffsets;
        default:
            Trap::throwException(state, "unknown exception");
            return resultOffsets;
        }
    }

    return resultOffsets;
}

} // namespace Walrus
