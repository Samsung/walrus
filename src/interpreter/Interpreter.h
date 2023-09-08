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
    friend class DefinedFunctionWithTryCatch;

    template <const bool considerException>
    ALWAYS_INLINE static void callInterpreter(ExecutionState& state, DefinedFunction* function, uint8_t* bp, ByteCodeStackOffset* offsets,
                                              uint16_t parameterOffsetCount, uint16_t resultOffsetCount)
    {
        ExecutionState newState(state, function);
        CHECK_STACK_LIMIT(newState);

        auto moduleFunction = function->moduleFunction();
        ALLOCA(uint8_t, functionStackBase, moduleFunction->requiredStackSize());

        // init parameter space
        for (size_t i = 0; i < parameterOffsetCount; i++) {
            ((size_t*)functionStackBase)[i] = *((size_t*)(bp + offsets[i]));
        }

        size_t programCounter = reinterpret_cast<size_t>(moduleFunction->byteCode());
        ByteCodeStackOffset* resultOffsets;

        if (moduleFunction->jitFunction() != nullptr) {
            resultOffsets = moduleFunction->jitFunction()->call(newState, function->instance(), functionStackBase);
        } else if (considerException) {
            while (true) {
                try {
                    resultOffsets = interpret(newState, programCounter, functionStackBase, function->instance());
                    break;
                } catch (std::unique_ptr<Exception>& e) {
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
                                    uint8_t* sp = functionStackBase + item.m_stackSizeToBe;
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
        } else {
            resultOffsets = interpret(newState, programCounter, functionStackBase, function->instance());
        }

        offsets += parameterOffsetCount;
        for (size_t i = 0; i < resultOffsetCount; i++) {
            *((size_t*)(bp + offsets[i])) = *((size_t*)(functionStackBase + resultOffsets[i]));
        }
    }

    static ByteCodeStackOffset* interpret(ExecutionState& state,
                                          size_t programCounter,
                                          uint8_t* bp,
                                          Instance* instance);

    static void callOperation(ExecutionState& state,
                              size_t& programCounter,
                              uint8_t* bp,
                              Instance* instance);

    static void callIndirectOperation(ExecutionState& state,
                                      size_t& programCounter,
                                      uint8_t* bp,
                                      Instance* instance);
};

} // namespace Walrus

#endif // __WalrusOpcode__
