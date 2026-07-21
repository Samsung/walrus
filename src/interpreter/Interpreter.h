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
#include "runtime/Instance.h"
#include "runtime/JITExec.h"
#include "runtime/Module.h"
#include "runtime/Store.h"
#include "runtime/Tag.h"
#include "interpreter/ByteCode.h"

#ifdef ENABLE_GC
#include "GCUtil.h"
#endif /* ENABLE_GC */

namespace Walrus {

class Instance;
class Memory;
class Table;
class Global;

class Interpreter {
private:
    friend class ByteCodeTable;
    friend class DefinedFunction;

    class StackFrame {
        MAKE_STACK_ALLOCATED();

    public:
        StackFrame(uint8_t* bp, size_t capacity)
            : m_bp(bp)
            , m_capacity(capacity)
            , m_owned(nullptr)
        {
        }

        ~StackFrame()
        {
            if (m_owned != nullptr) {
                deallocateBuffer(m_owned);
            }
        }

        uint8_t* bp() const { return m_bp; }
        size_t capacity() const { return m_capacity; }

        static uint8_t* allocateBuffer(size_t size)
        {
#ifdef ENABLE_GC
            return reinterpret_cast<uint8_t*>(GC_MALLOC_UNCOLLECTABLE(size));
#else
            return reinterpret_cast<uint8_t*>(malloc(size));
#endif
        }

        void replaceBuffer(uint8_t* buffer, size_t capacity)
        {
            if (m_owned != nullptr) {
                deallocateBuffer(m_owned);
            }
            m_owned = buffer;
            m_bp = buffer;
            m_capacity = capacity;
        }

    private:
        static void deallocateBuffer(uint8_t* buffer)
        {
#ifdef ENABLE_GC
            GC_FREE(buffer);
#else
            free(buffer);
#endif
        }

        uint8_t* m_bp;
        size_t m_capacity;
        uint8_t* m_owned;
    };

    ALWAYS_INLINE static void callInterpreter(ExecutionState& state, DefinedFunction* function, uint8_t* bp, ByteCodeStackOffset* offsets,
                                              uint16_t parameterOffsetCount, uint16_t resultOffsetCount)
    {
        ExecutionState newState(state, function);
        CHECK_STACK_LIMIT(newState);

        auto moduleFunction = function->moduleFunction();
        ALLOCA(uint8_t, functionStackBase, moduleFunction->requiredStackSize());

        for (size_t i = 0; i < parameterOffsetCount; i++) {
            ((size_t*)functionStackBase)[i] = *((size_t*)(bp + offsets[i]));
        }

        size_t programCounter = reinterpret_cast<size_t>(moduleFunction->byteCode());
        StackFrame frame(functionStackBase, moduleFunction->requiredStackSize());
        ByteCodeStackOffset* resultOffsets;

#if defined(WALRUS_ENABLE_JIT)
        if (moduleFunction->jitFunction() != nullptr) {
            resultOffsets = moduleFunction->jitFunction()->call(newState, function->instance(), functionStackBase);
        } else
#endif
        {
            while (true) {
                try {
                    resultOffsets = interpret(newState, programCounter, frame, function->instance());
                    break;
                } catch (std::unique_ptr<Exception>& e) {
                    if (UNLIKELY(!newState.m_currentFunction.hasValue())) {
                        throw std::unique_ptr<Exception>(std::move(e));
                    }
                    function = newState.m_currentFunction.value()->asDefinedFunction();
                    moduleFunction = function->moduleFunction();
                    for (size_t i = e->m_programCounterInfo.size(); i > 0; i--) {
                        if (e->m_programCounterInfo[i - 1].first == &newState) {
                            programCounter = e->m_programCounterInfo[i - 1].second;
                            break;
                        }
                    }
                    if (e->isUserException()) {
                        bool isCatchSucessful = false;
                        Tag* tag = e->tag().value();
                        size_t offset = programCounter - reinterpret_cast<size_t>(moduleFunction->byteCode());
                        for (const auto& item : moduleFunction->catchInfo()) {
                            if (item.m_tryStart <= offset && offset < item.m_tryEnd) {
                                if (item.m_tagIndex == std::numeric_limits<uint32_t>::max() || function->instance()->tag(item.m_tagIndex) == tag) {
                                    programCounter = item.m_catchStartPosition + reinterpret_cast<size_t>(moduleFunction->byteCode());
                                    uint8_t* sp = frame.bp() + item.m_stackSizeToBe;
                                    if (item.m_tagIndex != std::numeric_limits<uint32_t>::max() && tag->functionType()->paramStackSize()) {
                                        memcpy(sp, e->userExceptionData().data(), tag->functionType()->paramStackSize());
                                    }
                                    isCatchSucessful = true;
                                    break;
                                }
                            }
                        }
                        if (isCatchSucessful) {
                            continue;
                        }
                    }
                    throw std::unique_ptr<Exception>(std::move(e));
                }
            }
        }

        offsets += parameterOffsetCount;
        for (size_t i = 0; i < resultOffsetCount; i++) {
            *((size_t*)(bp + offsets[i])) = *((size_t*)(frame.bp() + resultOffsets[i]));
        }
    }

    static ByteCodeStackOffset* interpret(ExecutionState& state,
                                          size_t programCounter,
                                          StackFrame& frame,
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
                                  StackFrame& frame,
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
