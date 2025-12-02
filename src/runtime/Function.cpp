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
#include "runtime/Tag.h"
#include "runtime/Instance.h"
#include "runtime/Value.h"

namespace Walrus {

DEFINE_GLOBAL_TYPE_INFO(functionTypeInfo, FunctionKind);

Function::Function(FunctionType* functionType)
    : Extern(functionType->subTypeList() != nullptr ? functionType->subTypeList() : GET_GLOBAL_TYPE_INFO(functionTypeInfo))
    , m_functionType(functionType)
{
}

DefinedFunction* DefinedFunction::createDefinedFunction(Store* store,
                                                        Instance* instance,
                                                        ModuleFunction* moduleFunction)
{
    DefinedFunction* func;
    if (moduleFunction->hasTryCatch()) {
        func = new DefinedFunctionWithTryCatch(instance, moduleFunction);
    } else {
        func = new DefinedFunction(instance, moduleFunction);
    }
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
    const TypeVector::Types& paramTypeInfo = ft->param().types();
    const TypeVector::Types& resultTypeInfo = ft->result().types();

    size_t argc = paramTypeInfo.size();
    uint8_t* paramBuffer = valueBuffer;
    size_t offsetIndex = 0;
    for (size_t i = 0; i < argc; i++) {
        ASSERT(Value::isRefType(paramTypeInfo[i]) ? argv[i].isRef() : argv[i].type() == paramTypeInfo[i]);
        argv[i].writeToMemory(paramBuffer);
        size_t stackAllocatedSize = valueStackAllocatedSize(paramTypeInfo[i]);
        // size_t stackAllocatedSize = valueSize(paramTypeInfo[i]);
        for (size_t j = 0; j < stackAllocatedSize; j += sizeof(size_t)) {
            // for (size_t j = 0; j < stackAllocatedSize; j += stackAllocatedSize) {
            offsetBuffer[offsetIndex++] = reinterpret_cast<size_t>(paramBuffer) - reinterpret_cast<size_t>(valueBuffer) + j;
        }
        paramBuffer += stackAllocatedSize;
    }
    ASSERT(offsetIndex == parameterOffsetSize);

    size_t resultOffset = 0;
    for (size_t i = 0; i < resultTypeInfo.size(); i++) {
        size_t stackAllocatedSize = valueStackAllocatedSize(resultTypeInfo[i]);
        // size_t stackAllocatedSize = valueSize(resultTypeInfo[i]);
        for (size_t j = 0; j < stackAllocatedSize; j += sizeof(size_t)) {
            // for (size_t j = 0; j < stackAllocatedSize; j += stackAllocatedSize) {
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
        // size_t stackAllocatedSize = valueSize(resultTypeInfo[i]);
        resultOffsetIndex += stackAllocatedSize / sizeof(size_t);
        // if (stackAllocatedSize > sizeof(uint64_t)) {
        //     resultOffsetIndex++;
        // }
        // resultOffsetIndex++;
    }
}

void DefinedFunction::interpreterCall(ExecutionState& state, uint8_t* bp, ByteCodeStackOffset* offsets,
                                      uint16_t parameterOffsetCount, uint16_t resultOffsetCount)
{
    Interpreter::callInterpreter<false>(state, this, bp, offsets, parameterOffsetCount, resultOffsetCount);
}

void DefinedFunctionWithTryCatch::interpreterCall(ExecutionState& state, uint8_t* bp, ByteCodeStackOffset* offsets,
                                                  uint16_t parameterOffsetCount, uint16_t resultOffsetCount)
{
    Interpreter::callInterpreter<true>(state, this, bp, offsets, parameterOffsetCount, resultOffsetCount);
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
    const TypeVector::Types& paramTypeInfo = ft->param().types();
    const TypeVector::Types& resultTypeInfo = ft->result().types();

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

WasiFunction* WasiFunction::createWasiFunction(Store* store,
                                               FunctionType* functionType,
                                               WasiFunctionCallback callback)
{
    WasiFunction* func = new WasiFunction(functionType,
                                          callback);
    store->appendExtern(func);
    return func;
}

void WasiFunction::interpreterCall(ExecutionState& state, uint8_t* bp, ByteCodeStackOffset* offsets,
                                   uint16_t parameterOffsetCount, uint16_t resultOffsetCount)
{
    const FunctionType* ft = functionType();
    const TypeVector::Types& paramTypeInfo = ft->param().types();
    const TypeVector::Types& resultTypeInfo = ft->result().types();

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

void WasiFunction::call(ExecutionState& state, Value* argv, Value* result)
{
    ExecutionState newState(state, this);
    CHECK_STACK_LIMIT(newState);
    m_callback(newState, argv, result, this->m_runningInstance);
}

} // namespace Walrus
