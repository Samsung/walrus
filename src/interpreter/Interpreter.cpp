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

#include "interpreter/Interpreter.h"
#include "interpreter/ByteCode.h"
#include "runtime/Function.h"
#include "runtime/Instance.h"
#include "runtime/Module.h"

namespace Walrus {

template <typename T>
ALWAYS_INLINE void writeValue(uint8_t*& sp, const T& v)
{
    *reinterpret_cast<T*>(sp) = v;
    sp += stackAllocatedSize<T>();
}

template <typename T>
ALWAYS_INLINE T readValue(uint8_t*& sp)
{
    sp -= stackAllocatedSize<T>();
    T v = *reinterpret_cast<T*>(sp);
    return v;
}

void Interpreter::interpret(ExecutionState& state,
                            size_t programCounter,
                            uint8_t* bp,
                            uint8_t*& sp)
{
#define ADD_PROGRAM_COUNTER(codeName) programCounter += sizeof(codeName);

#define DEFINE_OPCODE(codeName) case codeName##Opcode
#define DEFINE_DEFAULT                \
    default:                          \
        RELEASE_ASSERT_NOT_REACHED(); \
        }
#define NEXT_INSTRUCTION() goto NextInstruction;

NextInstruction:
    OpcodeKind currentOpcode = ((ByteCode*)programCounter)->opcode();

    switch (currentOpcode) {
        DEFINE_OPCODE(I32Const)
            :
        {
            I32Const* code = (I32Const*)programCounter;
            writeValue(sp, code->value());
            ADD_PROGRAM_COUNTER(I32Const);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(I64Const)
            :
        {
            I64Const* code = (I64Const*)programCounter;
            writeValue(sp, code->value());
            ADD_PROGRAM_COUNTER(I64Const);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(F32Const)
            :
        {
            F32Const* code = (F32Const*)programCounter;
            writeValue(sp, code->value());
            ADD_PROGRAM_COUNTER(F32Const);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(F64Const)
            :
        {
            F64Const* code = (F64Const*)programCounter;
            writeValue(sp, code->value());
            ADD_PROGRAM_COUNTER(F64Const);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(LocalGet)
            :
        {
            LocalGet* code = (LocalGet*)programCounter;
            memcpy(sp, &bp[code->offset()], code->size());
            sp += code->size();
            ADD_PROGRAM_COUNTER(LocalGet);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(LocalSet)
            :
        {
            LocalSet* code = (LocalSet*)programCounter;
            sp -= code->size();
            memcpy(&bp[code->offset()], sp, code->size());
            ADD_PROGRAM_COUNTER(LocalSet);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(I32Add)
            :
        {
            int32_t a = readValue<int32_t>(sp);
            int32_t b = readValue<int32_t>(sp);
            writeValue<int32_t>(sp, a + b);
            ADD_PROGRAM_COUNTER(BinaryOperation);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(Drop)
            :
        {
            Drop* code = (Drop*)programCounter;
            sp -= code->size();
            ADD_PROGRAM_COUNTER(Drop);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(Call)
            :
        {
            callOperation(state, programCounter, bp, sp);
            ADD_PROGRAM_COUNTER(Call);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(End)
            :
        {
            End* code = (End*)programCounter;
            return;
        }

    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
}

NEVER_INLINE void Interpreter::callOperation(
    ExecutionState& state,
    size_t programCounter,
    uint8_t* bp,
    uint8_t*& sp)
{
    Call* code = (Call*)programCounter;

    Function* target = state.currentFunction()->asDefinedFunction()->instance()->function(code->index());
    FunctionType* ft = target->functionType();
    const FunctionType::FunctionTypeVector& param = ft->param();
    Value* paramVector = ALLOCA(sizeof(Value) * param.size(), Value);

    sp = sp - ft->paramStackSize();
    uint8_t* paramStackPointer = sp;
    for (size_t i = 0; i < param.size(); i++) {
        paramVector[i] = Value(param[i], paramStackPointer);
        paramStackPointer += valueSizeInStack(param[i]);
    }

    const FunctionType::FunctionTypeVector& result = ft->result();
    Value* resultVector = ALLOCA(sizeof(Value) * result.size(), Value);
    target->call(state, param.size(), paramVector, resultVector);

    for (size_t i = 0; i < result.size(); i++) {
        resultVector[i].writeToStack(sp);
    }
}

} // namespace Walrus
