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

#ifndef __WalrusExecutionState__
#define __WalrusExecutionState__

#include "util/Optional.h"

namespace Walrus {

class Function;

class ExecutionState {
public:
    friend class Exception;
    friend class Trap;
    friend class Interpreter;

    ExecutionState(ExecutionState& parent)
        : m_parent(&parent)
        , m_stackLimit(parent.m_stackLimit)
    {
    }

    ExecutionState(ExecutionState& parent, Function* currentFunction)
        : m_parent(&parent)
        , m_currentFunction(currentFunction)
        , m_stackLimit(parent.m_stackLimit)
    {
    }

    Optional<Function*> currentFunction() const
    {
        return m_currentFunction;
    }

    size_t stackLimit() const
    {
        return m_stackLimit;
    }

private:
    friend class ByteCodeTable;
    ExecutionState()
    {
        volatile int sp;
        m_stackLimit = (size_t)&sp;

#ifdef STACK_GROWS_DOWN
        m_stackLimit = m_stackLimit - STACK_LIMIT_FROM_BASE;
#else
        m_stackLimit = m_stackLimit + STACK_LIMIT_FROM_BASE;
#endif
    }

    Optional<ExecutionState*> m_parent;
    Optional<Function*> m_currentFunction;
    size_t m_stackLimit;
    Optional<size_t*> m_programCounterPointer;
};

} // namespace Walrus

#endif // __WalrusFunction__
