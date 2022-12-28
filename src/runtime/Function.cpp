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

DefinedFunction::DefinedFunction(Store* store,
                                 Instance* instance,
                                 ModuleFunction* moduleFunction)
    : Function(store, moduleFunction->functionType())
    , m_instance(instance)
    , m_moduleFunction(moduleFunction)
{
}

void DefinedFunction::call(ExecutionState& state, const uint32_t argc, Value* argv, Value* result)
{
    ExecutionState newState(state, this);
    checkStackLimit(newState);
    uint8_t* functionStackBase = ALLOCA(m_moduleFunction->requiredStackSize(), uint8_t);
    uint8_t* functionStackPointer = functionStackBase;

    // init parameter space
    for (size_t i = 0; i < argc; i++) {
        argv[i].writeToMemory(functionStackPointer);
        switch (argv[i].type()) {
        case Value::I32: {
            functionStackPointer += stackAllocatedSize<int32_t>();
            break;
        }
        case Value::F32: {
            functionStackPointer += stackAllocatedSize<float>();
            break;
        }
        case Value::F64: {
            functionStackPointer += stackAllocatedSize<double>();
            break;
        }
        case Value::I64: {
            functionStackPointer += stackAllocatedSize<int64_t>();
            break;
        }
        case Value::FuncRef:
        case Value::ExternRef: {
            functionStackPointer += stackAllocatedSize<void*>();
            break;
        }
        default: {
            ASSERT_NOT_REACHED();
            break;
        }
        }
    }
    // init local space
    auto localSize = m_moduleFunction->requiredStackSizeDueToLocal();
    memset(functionStackPointer, 0, localSize);

    auto resultOffsets = Interpreter::interpret(newState, functionStackBase);

    const FunctionType* ft = functionType();
    const ValueTypeVector& resultTypeInfo = ft->result();
    for (size_t i = 0; i < resultTypeInfo.size(); i++) {
        result[i] = Value(resultTypeInfo[i], functionStackBase + resultOffsets[i]);
    }
}

void ImportedFunction::call(ExecutionState& state, const uint32_t argc, Value* argv, Value* result)
{
    ExecutionState newState(state, this);
    checkStackLimit(newState);
    m_callback(newState, argc, argv, result, m_data);
}

} // namespace Walrus
