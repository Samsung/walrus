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
#include "util/Util.h"
#include <cstddef>
#include <vector>

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
        , tco_paramSize(0)
        , tco_functionTarget(nullptr)
    {
    }

    ExecutionState(ExecutionState& parent, Function* currentFunction)
        : m_parent(&parent)
        , m_currentFunction(currentFunction)
        , m_stackLimit(parent.m_stackLimit)
        , tco_paramSize(0)
        ,tco_functionTarget(nullptr)
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
    
    void destroyTCO() {
        tco_paramStore.clear();
        tco_paramSize = 0;
        tco_functionTarget = nullptr;
        
    }
    
    bool hasTCO() {
        return tco_functionTarget != nullptr;
    }
    
    std::vector<uint32_t> tco_paramStore;    
    size_t tco_paramSize;
    Function* tco_functionTarget;
private:
    friend class ByteCodeTable;
    ExecutionState()
    {
        m_stackLimit = (size_t)currentStackPointer();

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
