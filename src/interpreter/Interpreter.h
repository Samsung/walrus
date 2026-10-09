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

#ifndef __WalrusInterpreter__
#define __WalrusInterpreter__

#include "runtime/ExecutionState.h"
#include "runtime/Function.h"
#include "runtime/GCException.h"
#include "runtime/Instance.h"
#include "runtime/JITExec.h"
#include "runtime/Module.h"
#include "runtime/Store.h"
#include "runtime/Tag.h"
#include "interpreter/ByteCode.h"

namespace Walrus {

class Instance;
class Memory;
class Table;
class Global;

class Interpreter {
private:
    friend class ByteCodeTable;
    friend class DefinedFunction;

    enum CallMode {
        BpAndOffsets,
        ParamsAndResults,
    };

    template <CallMode mode>
    ALWAYS_INLINE static void callInterpreter(ExecutionState& state, DefinedFunction* function, void* arg1, void* arg2,
                                              uint16_t count1, uint16_t count2)
    {
        CHECK_STACK_LIMIT(state);

        auto moduleFunction = function->moduleFunction();
        size_t requiredStackSize = moduleFunction->requiredStackSize();
        uint8_t* functionStackBase;
        uint8_t* owned;

        if (requiredStackSize < 2048) {
            functionStackBase = reinterpret_cast<uint8_t*>(alloca(requiredStackSize));
            owned = nullptr;
        } else {
            functionStackBase = ExecutionState::allocateBuffer(requiredStackSize);
            owned = functionStackBase;
        }

        if (mode == BpAndOffsets) {
            uint8_t* bp = reinterpret_cast<uint8_t*>(arg1);
            ByteCodeStackOffset* offsets = reinterpret_cast<ByteCodeStackOffset*>(arg2);
            for (size_t i = 0; i < count1; i++) {
                ((size_t*)functionStackBase)[i] = *((size_t*)(bp + offsets[i]));
            }
        } else {
            uint8_t* dst = functionStackBase;
            Value* params = reinterpret_cast<Value*>(arg1);
            const TypeVector::Types& paramTypeInfo = function->functionType()->param().types();
            size_t size = paramTypeInfo.size();

            for (size_t i = 0; i < size; i++) {
                params[i].writeToMemory(dst);
                dst += valueStackAllocatedSize(paramTypeInfo[i]);
            }
        }

        ExecutionState newState(state, function, functionStackBase, requiredStackSize, owned);
        ByteCodeStackOffset* resultOffsets;

#if defined(WALRUS_ENABLE_JIT)
        if (moduleFunction->jitFunction() != nullptr) {
            const JITFunction* jitFunc = moduleFunction->jitFunction();
            ExecutionContext context(jitFunc->instanceConstData(), newState, function->instance());
            resultOffsets = jitFunc->call(context, newState.bp());
        } else
#endif
        {
            size_t programCounter = reinterpret_cast<size_t>(moduleFunction->byteCode());
            resultOffsets = interpret(newState, programCounter, function->instance());
        }

        uint8_t* src = newState.bp();
        if (mode == BpAndOffsets) {
            uint8_t* bp = reinterpret_cast<uint8_t*>(arg1);
            ByteCodeStackOffset* offsets = reinterpret_cast<ByteCodeStackOffset*>(arg2) + count1;
            for (size_t i = 0; i < count2; i++) {
                *((size_t*)(bp + offsets[i])) = *((size_t*)(src + resultOffsets[i]));
            }
        } else {
            Value* results = reinterpret_cast<Value*>(arg2);
            const TypeVector::Types& resultTypeInfo = function->functionType()->result().types();
            size_t size = resultTypeInfo.size();
            size_t resultOffsetIndex = 0;

            for (size_t i = 0; i < size; i++) {
                Value::Type type = resultTypeInfo[i];
                results[i] = Value(type, src + resultOffsets[resultOffsetIndex]);
                resultOffsetIndex += valueStackAllocatedSize(type) / sizeof(size_t);
            }
        }
    }

    static ByteCodeStackOffset* interpret(ExecutionState& state,
                                          size_t programCounter,
                                          Instance* instance);

    static void callOperation(ExecutionState& state,
                              size_t& programCounter,
                              uint8_t* bp,
                              Instance* instance);

    static void callIndirectOperation(ExecutionState& state,
                                      size_t& programCounter,
                                      uint8_t* bp,
                                      Instance* instance,
                                      bool is64);

    static void callRefOperation(ExecutionState& state,
                                 size_t& programCounter,
                                 uint8_t* bp,
                                 Instance* instance);

    static bool tailCallOperation(ExecutionState& state,
                                  size_t& programCounter,
                                  Instance*& instance,
                                  Function* target,
                                  ByteCodeStackOffset* offsets,
                                  uint16_t parameterOffsetCount,
                                  uint16_t resultOffsetCount);

    static bool testRefGeneric(void* refPtr, Value::Type type);
    static bool testRefDefined(void* refPtr, const CompositeType** typeInfo);
};

} // namespace Walrus

#endif // __WalrusOpcode__
