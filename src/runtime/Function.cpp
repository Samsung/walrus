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
#include "runtime/Module.h"
#include "runtime/Store.h"
#include "interpreter/Interpreter.h"
#include "runtime/Tag.h"
#include "runtime/Instance.h"
#include "runtime/Value.h"

namespace Walrus {

DEFINE_GLOBAL_TYPE_INFO(functionTypeInfo, FunctionKind);

Function::Function(const FunctionType* functionType)
    : Extern(functionType->subTypeList() != nullptr ? functionType->subTypeList() : GET_GLOBAL_TYPE_INFO(functionTypeInfo))
    , m_functionType(functionType)
{
}

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
    Interpreter::callInterpreter<Interpreter::ParamsAndResults>(state, this, argv, result, 0, 0);
}

void DefinedFunction::interpreterCall(ExecutionState& state, uint8_t* bp, ByteCodeStackOffset* offsets,
                                      uint16_t parameterOffsetCount, uint16_t resultOffsetCount)
{
    Interpreter::callInterpreter<Interpreter::BpAndOffsets>(state, this, bp, offsets, parameterOffsetCount, resultOffsetCount);
}

void NativeFunction::interpreterCall(ExecutionState& state, uint8_t* bp, ByteCodeStackOffset* offsets,
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

void WasiFunction::call(ExecutionState& state, Value* argv, Value* result)
{
    ExecutionState newState(state, this);
    CHECK_STACK_LIMIT(newState);
    m_callback(newState, argv, result, this->m_runningInstance);
}

} // namespace Walrus
