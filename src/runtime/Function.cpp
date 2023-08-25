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
#include "runtime/Store.h"
#include "interpreter/Interpreter.h"
#include "runtime/Module.h"
#include "runtime/Value.h"

namespace Walrus {

DefinedFunction* DefinedFunction::createDefinedFunction(Store* store,
                                                        Instance* instance,
                                                        ModuleFunction* moduleFunction)
{
    DefinedFunction* func = new DefinedFunction(instance, moduleFunction);
    store->appendExtern(func);
    return func;
}

DefinedFunction::DefinedFunction(Instance* instance,
                                 ModuleFunction* moduleFunction)
    : Function(moduleFunction->functionType())
    , m_instance(instance)
    , m_moduleFunction(moduleFunction)
{
}

void DefinedFunction::call(ExecutionState& state, Value* argv, Value* result)
{
    const FunctionType* ft = functionType();
    size_t valueBufferSize = std::max(ft->paramStackSize(), ft->resultStackSize());
    ALLOCA(uint8_t, valueBuffer, valueBufferSize);
    uint16_t parameterOffsetSize = ft->paramStackSize() / sizeof(size_t);
    uint16_t resultOffsetSize = ft->resultStackSize() / sizeof(size_t);
    ALLOCA(uint16_t, offsetBuffer, (parameterOffsetSize + resultOffsetSize) * sizeof(uint16_t));
    const ValueTypeVector& paramTypeInfo = ft->param();
    const ValueTypeVector& resultTypeInfo = ft->result();

    size_t argc = paramTypeInfo.size();
    uint8_t* paramBuffer = valueBuffer;
    size_t offsetIndex = 0;
    for (size_t i = 0; i < argc; i++) {
        ASSERT(argv[i].type() == paramTypeInfo[i]);
        argv[i].writeToMemory(paramBuffer);
        size_t stackAllocatedSize = valueStackAllocatedSize(paramTypeInfo[i]);
        for (size_t j = 0; j < stackAllocatedSize; j += sizeof(size_t)) {
            offsetBuffer[offsetIndex++] = reinterpret_cast<size_t>(paramBuffer) - reinterpret_cast<size_t>(valueBuffer) + j;
        }
        paramBuffer += stackAllocatedSize;
    }
    ASSERT(offsetIndex == parameterOffsetSize);

    size_t resultOffset = 0;
    for (size_t i = 0; i < resultTypeInfo.size(); i++) {
        size_t stackAllocatedSize = valueStackAllocatedSize(resultTypeInfo[i]);
        for (size_t j = 0; j < stackAllocatedSize; j += sizeof(size_t)) {
            offsetBuffer[offsetIndex++] = resultOffset + j;
        }
        resultOffset += stackAllocatedSize;
    }
    ASSERT(offsetIndex == parameterOffsetSize + resultOffsetSize);
    interpreterCall(state, valueBuffer, offsetBuffer, parameterOffsetSize, resultOffsetSize);

    size_t resultOffsetIndex = 0;
    for (size_t i = 0; i < resultTypeInfo.size(); i++) {
        result[i] = Value(resultTypeInfo[i], valueBuffer + offsetBuffer[resultOffsetIndex + parameterOffsetSize]);
        size_t stackAllocatedSize = valueStackAllocatedSize(resultTypeInfo[i]);
        resultOffsetIndex += stackAllocatedSize / sizeof(size_t);
    }
}

void DefinedFunction::interpreterCall(ExecutionState& state, uint8_t* bp, ByteCodeStackOffset* offsets,
                                      uint16_t parameterOffsetCount, uint16_t resultOffsetCount)
{
    ExecutionState newState(state, this);
    CHECK_STACK_LIMIT(newState);

    ALLOCA(uint8_t, functionStackBase, m_moduleFunction->requiredStackSize());

    // init parameter space
    for (size_t i = 0; i < parameterOffsetCount; i++) {
        ((size_t*)functionStackBase)[i] = *((size_t*)(bp + offsets[i]));
    }

    auto resultOffsets = Interpreter::interpret(newState, functionStackBase);

    offsets += parameterOffsetCount;
    for (size_t i = 0; i < resultOffsetCount; i++) {
        *((size_t*)(bp + offsets[i])) = *((size_t*)(functionStackBase + resultOffsets[i]));
    }
}

ImportedFunction* ImportedFunction::createImportedFunction(Store* store,
                                                           FunctionType* functionType,
                                                           ImportedFunctionCallback callback,
                                                           void* data)
{
    ImportedFunction* func = new ImportedFunction(functionType,
                                                  callback,
                                                  data);
    store->appendExtern(func);
    return func;
}

void ImportedFunction::interpreterCall(ExecutionState& state, uint8_t* bp, ByteCodeStackOffset* offsets,
                                       uint16_t parameterOffsetCount, uint16_t resultOffsetCount)
{
    const FunctionType* ft = functionType();
    const ValueTypeVector& paramTypeInfo = ft->param();
    const ValueTypeVector& resultTypeInfo = ft->result();

    ALLOCA(Value, paramVector, sizeof(Value) * paramTypeInfo.size());
    ALLOCA(Value, resultVector, sizeof(Value) * resultTypeInfo.size());

    size_t offsetIndex = 0;
    size_t size = paramTypeInfo.size();
    Value* paramVectorStart = paramVector;
    for (size_t i = 0; i < size; i++) {
        paramVector[i] = Value(paramTypeInfo[i], bp + offsets[offsetIndex]);
        offsetIndex += valueFunctionCopyCount(paramTypeInfo[i]);
    }

    call(state, paramVectorStart, resultVector);

    for (size_t i = 0; i < resultTypeInfo.size(); i++) {
        resultVector[i].writeToMemory(bp + offsets[offsetIndex]);
        offsetIndex += valueFunctionCopyCount(resultTypeInfo[i]);
    }
}

void ImportedFunction::call(ExecutionState& state, Value* argv, Value* result)
{
    ExecutionState newState(state, this);
    CHECK_STACK_LIMIT(newState);
    m_callback(newState, argv, result, m_data);
}

} // namespace Walrus
