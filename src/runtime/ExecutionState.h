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
    ExecutionState()
    {
    }

    ExecutionState(ExecutionState& parent)
        : m_parent(&parent)
    {
    }

    ExecutionState(ExecutionState& parent, Function* currentFunction)
        : m_parent(&parent)
        , m_currentFunction(currentFunction)
    {
    }

    Optional<Function*> currentFunction() const
    {
        return m_currentFunction;
    }

private:
    Optional<ExecutionState*> m_parent;
    Optional<Function*> m_currentFunction;
};

} // namespace Walrus

#endif // __WalrusFunction__
