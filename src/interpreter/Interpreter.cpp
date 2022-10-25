/*
 * Copyright 2020 WebAssembly Community Group participants
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
#include "runtime/Trap.h"
#include "util/MathOperation.h"

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

template <typename T>
bool intEqz(T val) { return val == 0; }
template <typename T>
bool eq(T lhs, T rhs) { return lhs == rhs; }
template <typename T>
bool ne(T lhs, T rhs) { return lhs != rhs; }
template <typename T>
bool lt(T lhs, T rhs) { return lhs < rhs; }
template <typename T>
bool le(T lhs, T rhs) { return lhs <= rhs; }
template <typename T>
bool gt(T lhs, T rhs) { return lhs > rhs; }
template <typename T>
bool ge(T lhs, T rhs) { return lhs >= rhs; }
template <typename T>
T add(T lhs, T rhs) { return canonNaN(lhs + rhs); }
template <typename T>
T sub(T lhs, T rhs) { return canonNaN(lhs - rhs); }
template <typename T>
T xchg(T lhs, T rhs) { return rhs; }
template <typename T>
T intAnd(T lhs, T rhs) { return lhs & rhs; }
template <typename T>
T intOr(T lhs, T rhs) { return lhs | rhs; }
template <typename T>
T intXor(T lhs, T rhs) { return lhs ^ rhs; }
template <typename T>
T intShl(T lhs, T rhs) { return lhs << shiftMask(rhs); }
template <typename T>
T intShr(T lhs, T rhs) { return lhs >> shiftMask(rhs); }
template <typename T>
T intMin(T lhs, T rhs) { return std::min(lhs, rhs); }
template <typename T>
T intMax(T lhs, T rhs) { return std::max(lhs, rhs); }
template <typename T>
T intAndNot(T lhs, T rhs) { return lhs & ~rhs; }
template <typename T>
T intClz(T val) { return clz(val); }
template <typename T>
T intCtz(T val) { return ctz(val); }
template <typename T>
T intPopcnt(T val) { return popCount(val); }
template <typename T>
T intNot(T val) { return ~val; }
template <typename T>
T intNeg(T val) { return ~val + 1; }
template <typename T>
T intAvgr(T lhs, T rhs) { return (lhs + rhs + 1) / 2; }

template <typename T>
T intDiv(T lhs, T rhs)
{
    if (UNLIKELY(rhs == 0)) {
        Trap::throwException(new String("integer divide by zero"));
    }
    if (UNLIKELY(!isNormalDivRem(lhs, rhs))) {
        Trap::throwException(new String("integer overflow"));
    }
    return lhs / rhs;
}

template <typename T>
T intRem(T lhs, T rhs)
{
    if (UNLIKELY(rhs == 0)) {
        Trap::throwException(new String("integer divide by zero"));
    }
    if (LIKELY(isNormalDivRem(lhs, rhs))) {
        return lhs % rhs;
    } else {
        return 0;
    }
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

#define BINARY_OPERATION(nativeParameterTypeName, nativeReturnTypeName, wasmTypeName, operationName, byteCodeOperationName)                  \
    DEFINE_OPCODE(wasmTypeName##byteCodeOperationName)                                                                                       \
        :                                                                                                                                    \
    {                                                                                                                                        \
        writeValue<nativeReturnTypeName>(sp, operationName(readValue<nativeParameterTypeName>(sp), readValue<nativeParameterTypeName>(sp))); \
        ADD_PROGRAM_COUNTER(BinaryOperation);                                                                                                \
        NEXT_INSTRUCTION();                                                                                                                  \
    }

#define UNARY_OPERATION(nativeTypeName, wasmTypeName, operationName, byteCodeOperationName) \
    DEFINE_OPCODE(wasmTypeName##byteCodeOperationName)                                      \
        :                                                                                   \
    {                                                                                       \
        writeValue<nativeTypeName>(sp, operationName(readValue<nativeTypeName>(sp)));       \
        ADD_PROGRAM_COUNTER(UnaryOperation);                                                \
        NEXT_INSTRUCTION();                                                                 \
    }

#define UNARY_OPERATION_OPERATION_TEMPLATE_2(nativeTypeName, wasmTypeName, operationName, T1, T2, byteCodeOperationName) \
    DEFINE_OPCODE(wasmTypeName##byteCodeOperationName)                                                                   \
        :                                                                                                                \
    {                                                                                                                    \
        writeValue<nativeTypeName>(sp, operationName<T1, T2>(readValue<nativeTypeName>(sp)));                            \
        ADD_PROGRAM_COUNTER(UnaryOperation);                                                                             \
        NEXT_INSTRUCTION();                                                                                              \
    }

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

        BINARY_OPERATION(int32_t, int32_t, I32, add, Add)
        BINARY_OPERATION(int32_t, int32_t, I32, sub, Sub)
        BINARY_OPERATION(int32_t, int32_t, I32, mul, Mul)
        BINARY_OPERATION(int32_t, int32_t, I32, intDiv, DivS)
        BINARY_OPERATION(uint32_t, uint32_t, I32, intDiv, DivU)
        BINARY_OPERATION(int32_t, int32_t, I32, intRem, RemS)
        BINARY_OPERATION(uint32_t, uint32_t, I32, intRem, RemU)
        BINARY_OPERATION(int32_t, int32_t, I32, intAnd, And)
        BINARY_OPERATION(int32_t, int32_t, I32, intOr, Or)
        BINARY_OPERATION(int32_t, int32_t, I32, intXor, Xor)
        BINARY_OPERATION(int32_t, int32_t, I32, intShl, Shl)
        BINARY_OPERATION(int32_t, int32_t, I32, intShr, ShrS)
        BINARY_OPERATION(uint32_t, uint32_t, I32, intShr, ShrU)
        BINARY_OPERATION(uint32_t, uint32_t, I32, intRotl, Rotl)
        BINARY_OPERATION(uint32_t, uint32_t, I32, intRotr, Rotr)
        BINARY_OPERATION(int32_t, int32_t, I32, eq, Eq)
        BINARY_OPERATION(int32_t, int32_t, I32, ne, Ne)
        BINARY_OPERATION(int32_t, int32_t, I32, lt, LtS)
        BINARY_OPERATION(uint32_t, uint32_t, I32, lt, LtU)
        BINARY_OPERATION(int32_t, int32_t, I32, le, LeS)
        BINARY_OPERATION(uint32_t, uint32_t, I32, le, LeU)
        BINARY_OPERATION(int32_t, int32_t, I32, gt, GtS)
        BINARY_OPERATION(uint32_t, uint32_t, I32, gt, GtU)
        BINARY_OPERATION(int32_t, int32_t, I32, ge, GeS)
        BINARY_OPERATION(uint32_t, uint32_t, I32, ge, GeU)

        UNARY_OPERATION(uint32_t, I32, clz, Clz)
        UNARY_OPERATION(uint32_t, I32, ctz, Ctz)
        UNARY_OPERATION(uint32_t, I32, popCount, Popcnt)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(uint32_t, I32, intExtend, uint32_t, 7, Extend8S)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(uint32_t, I32, intExtend, uint32_t, 15, Extend16S)
        UNARY_OPERATION(uint32_t, I32, intEqz, Eqz)

        BINARY_OPERATION(float, float, F32, add, Add)
        BINARY_OPERATION(float, float, F32, sub, Sub)
        BINARY_OPERATION(float, float, F32, mul, Mul)
        BINARY_OPERATION(float, float, F32, floatDiv, Div)
        BINARY_OPERATION(float, float, F32, floatMax, Max)
        BINARY_OPERATION(float, float, F32, floatMin, Min)
        BINARY_OPERATION(float, float, F32, floatCopysign, Copysign)
        BINARY_OPERATION(float, int32_t, F32, eq, Eq)
        BINARY_OPERATION(float, int32_t, F32, ne, Ne)
        BINARY_OPERATION(float, int32_t, F32, lt, Lt)
        BINARY_OPERATION(float, int32_t, F32, le, Le)
        BINARY_OPERATION(float, int32_t, F32, gt, Gt)
        BINARY_OPERATION(float, int32_t, F32, ge, Ge)

        UNARY_OPERATION(float, F32, floatSqrt, Sqrt)
        UNARY_OPERATION(float, F32, floatCeil, Ceil)
        UNARY_OPERATION(float, F32, floatFloor, Floor)
        UNARY_OPERATION(float, F32, floatTrunc, Trunc)
        UNARY_OPERATION(float, F32, floatNearest, Nearest)
        UNARY_OPERATION(float, F32, floatAbs, Abs)
        UNARY_OPERATION(float, F32, floatNeg, Neg)

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
