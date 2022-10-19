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

#include "runtime/Function.h"
#include "interpreter/Interpreter.h"
#include "runtime/Module.h"
#include "runtime/Value.h"

namespace Walrus {

void DefinedFunction::call(ExecutionState& state, const uint32_t argc, Value* argv, Value* result)
{
    ExecutionState newState(state, this);
    uint8_t* sp = ALLOCA(m_moduleFunction->requiredStackSize(), uint8_t);
    Interpreter::interpret(newState, reinterpret_cast<size_t>(m_moduleFunction->byteCode()), sp);

    FunctionType* ft = functionType();
    const FunctionType::FunctionTypeVector& resultTypeInfo = ft->result();

    sp = sp - ft->resultStackSize();
    uint8_t* resultStackPointer = sp;
    for (size_t i = 0; i < resultTypeInfo.size(); i++) {
        result[i] = Value(resultTypeInfo[i], resultStackPointer);
        resultStackPointer += valueSizeInStack(resultTypeInfo[i]);
    }
}

void ImportedFunction::call(ExecutionState& state, const uint32_t argc, Value* argv, Value* result)
{
    ExecutionState newState(state, this);
    m_callback(newState, argc, argv, result, m_data);
}

} // namespace Walrus
