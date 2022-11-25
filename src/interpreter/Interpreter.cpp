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
#include "runtime/Instance.h"
#include "runtime/Function.h"
#include "runtime/Memory.h"
#include "runtime/Table.h"
#include "runtime/Module.h"
#include "runtime/Trap.h"
#include "runtime/Tag.h"
#include "util/MathOperation.h"

namespace Walrus {

void Interpreter::interpret(ExecutionState& state,
                            uint8_t* bp,
                            uint8_t*& sp)
{
    ModuleFunction* mf = state.currentFunction()->asDefinedFunction()->moduleFunction();
    size_t programCounter = reinterpret_cast<size_t>(mf->byteCode());
    while (true) {
        try {
            interpret(state, programCounter, bp, sp);
            break;
        } catch (std::unique_ptr<Exception>& e) {
            if (e->isUserException()) {
                bool isCatchSucessful = false;
                Tag* tag = e->tag().value();
                size_t offset = programCounter - reinterpret_cast<size_t>(mf->byteCode());
                for (const auto& item : mf->catchInfo()) {
                    if (item.m_tryStart <= offset && offset < item.m_tryEnd) {
                        if (item.m_tagIndex == std::numeric_limits<uint32_t>::max() || state.currentFunction()->asDefinedFunction()->instance()->tag(item.m_tagIndex) == tag) {
                            programCounter = item.m_catchStartPosition + reinterpret_cast<size_t>(mf->byteCode());
                            sp = bp + item.m_stackSizeToBe;
                            if (item.m_tagIndex != std::numeric_limits<uint32_t>::max() && tag->functionType()->paramStackSize()) {
                                memcpy(sp, e->userExceptionData().data(), tag->functionType()->paramStackSize());
                                sp += tag->functionType()->paramStackSize();
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
        Trap::throwException("integer divide by zero");
    }
    if (UNLIKELY(!isNormalDivRem(lhs, rhs))) {
        Trap::throwException("integer overflow");
    }
    return lhs / rhs;
}

template <typename T>
T intRem(T lhs, T rhs)
{
    if (UNLIKELY(rhs == 0)) {
        Trap::throwException("integer divide by zero");
    }
    if (LIKELY(isNormalDivRem(lhs, rhs))) {
        return lhs % rhs;
    } else {
        return 0;
    }
}

template <typename R, typename T>
R doConvert(T val)
{
    if (std::is_integral<R>::value && std::is_floating_point<T>::value) {
        // Don't use std::isnan here because T may be a non-floating-point type.
        if (UNLIKELY(isNaN(val))) {
            Trap::throwException("invalid conversion to integer");
        }
    }
    if (UNLIKELY(!canConvert<R>(val))) {
        Trap::throwException("integer overflow");
    }
    return convert<R>(val);
}

void Interpreter::interpret(ExecutionState& state,
                            size_t& programCounter,
                            uint8_t* bp,
                            uint8_t*& sp)
{
    Instance* instance = state.currentFunction()->asDefinedFunction()->instance();
#define ADD_PROGRAM_COUNTER(codeName) programCounter += sizeof(codeName);

#define DEFINE_OPCODE(codeName) case codeName##Opcode
#define DEFINE_DEFAULT                \
    default:                          \
        RELEASE_ASSERT_NOT_REACHED(); \
        }
#define NEXT_INSTRUCTION() goto NextInstruction;

#define BINARY_OPERATION(nativeParameterTypeName, nativeReturnTypeName, wasmTypeName, operationName, byteCodeOperationName) \
    DEFINE_OPCODE(wasmTypeName##byteCodeOperationName)                                                                      \
        :                                                                                                                   \
    {                                                                                                                       \
        auto rhs = readValue<nativeParameterTypeName>(sp);                                                                  \
        auto lhs = readValue<nativeParameterTypeName>(sp);                                                                  \
        writeValue<nativeReturnTypeName>(sp, operationName(lhs, rhs));                                                      \
        ADD_PROGRAM_COUNTER(BinaryOperation);                                                                               \
        NEXT_INSTRUCTION();                                                                                                 \
    }

#define UNARY_OPERATION(nativeParameterTypeName, nativeReturnTypeName, wasmTypeName, operationName, byteCodeOperationName) \
    DEFINE_OPCODE(wasmTypeName##byteCodeOperationName)                                                                     \
        :                                                                                                                  \
    {                                                                                                                      \
        writeValue<nativeReturnTypeName>(sp, operationName(readValue<nativeParameterTypeName>(sp)));                       \
        ADD_PROGRAM_COUNTER(UnaryOperation);                                                                               \
        NEXT_INSTRUCTION();                                                                                                \
    }

#define UNARY_OPERATION_OPERATION_TEMPLATE_2(nativeParameterTypeName, nativeReturnTypeName, wasmTypeName, operationName, T1, T2, byteCodeOperationName) \
    DEFINE_OPCODE(wasmTypeName##byteCodeOperationName)                                                                                                  \
        :                                                                                                                                               \
    {                                                                                                                                                   \
        writeValue<nativeReturnTypeName>(sp, operationName<T1, T2>(readValue<nativeParameterTypeName>(sp)));                                            \
        ADD_PROGRAM_COUNTER(UnaryOperation);                                                                                                            \
        NEXT_INSTRUCTION();                                                                                                                             \
    }

#define UNARY_OPERATION_NOOP(wasmTypeName, byteCodeOperationName) \
    DEFINE_OPCODE(wasmTypeName##byteCodeOperationName)            \
        :                                                         \
    {                                                             \
        ADD_PROGRAM_COUNTER(UnaryOperation);                      \
        NEXT_INSTRUCTION();                                       \
    }

#define MEMORY_LOAD_OPERATION(opcodeName, nativeReadTypeName, nativeWriteTypeName) \
    DEFINE_OPCODE(opcodeName)                                                      \
        :                                                                          \
    {                                                                              \
        MemoryLoad* code = (MemoryLoad*)programCounter;                            \
        uint32_t offset = readValue<uint32_t>(sp);                                 \
        nativeReadTypeName value;                                                  \
        instance->memory(0)->load(offset, code->offset(), &value);                 \
        writeValue<nativeWriteTypeName>(sp, value);                                \
        ADD_PROGRAM_COUNTER(MemoryLoad);                                           \
        NEXT_INSTRUCTION();                                                        \
    }

#define MEMORY_STORE_OPERATION(opcodeName, nativeReadTypeName, nativeWriteTypeName) \
    DEFINE_OPCODE(opcodeName)                                                       \
        :                                                                           \
    {                                                                               \
        MemoryStore* code = (MemoryStore*)programCounter;                           \
        nativeWriteTypeName value = readValue<nativeReadTypeName>(sp);              \
        uint32_t offset = readValue<uint32_t>(sp);                                  \
        instance->memory(0)->store(offset, code->offset(), value);                  \
        ADD_PROGRAM_COUNTER(MemoryStore);                                           \
        NEXT_INSTRUCTION();                                                         \
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

        DEFINE_OPCODE(LocalGet4)
            :
        {
            LocalGet4* code = (LocalGet4*)programCounter;
            *reinterpret_cast<uint32_t*>(sp) = *reinterpret_cast<uint32_t*>(&bp[code->offset()]);
            sp += 4;
            ADD_PROGRAM_COUNTER(LocalGet4);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(LocalGet8)
            :
        {
            LocalGet8* code = (LocalGet8*)programCounter;
            *reinterpret_cast<uint64_t*>(sp) = *reinterpret_cast<uint64_t*>(&bp[code->offset()]);
            sp += 8;
            ADD_PROGRAM_COUNTER(LocalGet8);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(LocalSet4)
            :
        {
            LocalSet4* code = (LocalSet4*)programCounter;
            sp -= 4;
            *reinterpret_cast<uint32_t*>(&bp[code->offset()]) = *reinterpret_cast<uint32_t*>(sp);
            ADD_PROGRAM_COUNTER(LocalSet4);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(LocalSet8)
            :
        {
            LocalSet4* code = (LocalSet4*)programCounter;
            sp -= 8;
            *reinterpret_cast<uint64_t*>(&bp[code->offset()]) = *reinterpret_cast<uint64_t*>(sp);
            ADD_PROGRAM_COUNTER(LocalSet8);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(LocalTee4)
            :
        {
            LocalSet4* code = (LocalSet4*)programCounter;
            *reinterpret_cast<uint32_t*>(&bp[code->offset()]) = *reinterpret_cast<uint32_t*>(sp - 4);
            ADD_PROGRAM_COUNTER(LocalTee4);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(LocalTee8)
            :
        {
            LocalSet4* code = (LocalSet4*)programCounter;
            *reinterpret_cast<uint64_t*>(&bp[code->offset()]) = *reinterpret_cast<uint64_t*>(sp - 8);
            ADD_PROGRAM_COUNTER(LocalTee8);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(Select)
            :
        {
            Select* code = (Select*)programCounter;
            auto cond = readValue<int32_t>(sp);
            if (code->size() == 4) {
                uint32_t falseValue = readValue<int32_t>(sp);
                uint32_t trueValue = readValue<int32_t>(sp);
                if (cond) {
                    writeValue(sp, trueValue);
                } else {
                    writeValue(sp, falseValue);
                }
            } else {
                ASSERT(code->size() == 8);
                uint64_t falseValue = readValue<int64_t>(sp);
                uint64_t trueValue = readValue<int64_t>(sp);
                if (cond) {
                    writeValue(sp, trueValue);
                } else {
                    writeValue(sp, falseValue);
                }
            }
            ADD_PROGRAM_COUNTER(Select);
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

        UNARY_OPERATION(uint32_t, uint32_t, I32, clz, Clz)
        UNARY_OPERATION(uint32_t, uint32_t, I32, ctz, Ctz)
        UNARY_OPERATION(uint32_t, uint32_t, I32, popCount, Popcnt)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(uint32_t, uint32_t, I32, intExtend, uint32_t, 7, Extend8S)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(uint32_t, uint32_t, I32, intExtend, uint32_t, 15, Extend16S)
        UNARY_OPERATION(uint32_t, uint32_t, I32, intEqz, Eqz)

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

        UNARY_OPERATION(float, float, F32, floatSqrt, Sqrt)
        UNARY_OPERATION(float, float, F32, floatCeil, Ceil)
        UNARY_OPERATION(float, float, F32, floatFloor, Floor)
        UNARY_OPERATION(float, float, F32, floatTrunc, Trunc)
        UNARY_OPERATION(float, float, F32, floatNearest, Nearest)
        UNARY_OPERATION(float, float, F32, floatAbs, Abs)
        UNARY_OPERATION(float, float, F32, floatNeg, Neg)

        BINARY_OPERATION(int64_t, int64_t, I64, add, Add)
        BINARY_OPERATION(int64_t, int64_t, I64, sub, Sub)
        BINARY_OPERATION(int64_t, int64_t, I64, mul, Mul)
        BINARY_OPERATION(int64_t, int64_t, I64, intDiv, DivS)
        BINARY_OPERATION(uint64_t, uint64_t, I64, intDiv, DivU)
        BINARY_OPERATION(int64_t, int64_t, I64, intRem, RemS)
        BINARY_OPERATION(uint64_t, uint64_t, I64, intRem, RemU)
        BINARY_OPERATION(int64_t, int64_t, I64, intAnd, And)
        BINARY_OPERATION(int64_t, int64_t, I64, intOr, Or)
        BINARY_OPERATION(int64_t, int64_t, I64, intXor, Xor)
        BINARY_OPERATION(int64_t, int64_t, I64, intShl, Shl)
        BINARY_OPERATION(int64_t, int64_t, I64, intShr, ShrS)
        BINARY_OPERATION(uint64_t, uint64_t, I64, intShr, ShrU)
        BINARY_OPERATION(uint64_t, uint64_t, I64, intRotl, Rotl)
        BINARY_OPERATION(uint64_t, uint64_t, I64, intRotr, Rotr)
        BINARY_OPERATION(int64_t, int32_t, I64, eq, Eq)
        BINARY_OPERATION(int64_t, int32_t, I64, ne, Ne)
        BINARY_OPERATION(int64_t, int32_t, I64, lt, LtS)
        BINARY_OPERATION(uint64_t, uint32_t, I64, lt, LtU)
        BINARY_OPERATION(int64_t, int32_t, I64, le, LeS)
        BINARY_OPERATION(uint64_t, uint32_t, I64, le, LeU)
        BINARY_OPERATION(int64_t, int32_t, I64, gt, GtS)
        BINARY_OPERATION(uint64_t, uint32_t, I64, gt, GtU)
        BINARY_OPERATION(int64_t, int32_t, I64, ge, GeS)
        BINARY_OPERATION(uint64_t, uint32_t, I64, ge, GeU)

        UNARY_OPERATION(uint64_t, uint64_t, I64, clz, Clz)
        UNARY_OPERATION(uint64_t, uint64_t, I64, ctz, Ctz)
        UNARY_OPERATION(uint64_t, uint64_t, I64, popCount, Popcnt)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(uint64_t, uint64_t, I64, intExtend, uint64_t, 7, Extend8S)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(uint64_t, uint64_t, I64, intExtend, uint64_t, 15, Extend16S)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(uint64_t, uint64_t, I64, intExtend, uint64_t, 31, Extend32S)
        UNARY_OPERATION(uint64_t, uint32_t, I64, intEqz, Eqz)

        BINARY_OPERATION(double, double, F64, add, Add)
        BINARY_OPERATION(double, double, F64, sub, Sub)
        BINARY_OPERATION(double, double, F64, mul, Mul)
        BINARY_OPERATION(double, double, F64, floatDiv, Div)
        BINARY_OPERATION(double, double, F64, floatMax, Max)
        BINARY_OPERATION(double, double, F64, floatMin, Min)
        BINARY_OPERATION(double, double, F64, floatCopysign, Copysign)
        BINARY_OPERATION(double, int32_t, F64, eq, Eq)
        BINARY_OPERATION(double, int32_t, F64, ne, Ne)
        BINARY_OPERATION(double, int32_t, F64, lt, Lt)
        BINARY_OPERATION(double, int32_t, F64, le, Le)
        BINARY_OPERATION(double, int32_t, F64, gt, Gt)
        BINARY_OPERATION(double, int32_t, F64, ge, Ge)

        UNARY_OPERATION(double, double, F64, floatSqrt, Sqrt)
        UNARY_OPERATION(double, double, F64, floatCeil, Ceil)
        UNARY_OPERATION(double, double, F64, floatFloor, Floor)
        UNARY_OPERATION(double, double, F64, floatTrunc, Trunc)
        UNARY_OPERATION(double, double, F64, floatNearest, Nearest)
        UNARY_OPERATION(double, double, F64, floatAbs, Abs)
        UNARY_OPERATION(double, double, F64, floatNeg, Neg)

        UNARY_OPERATION_OPERATION_TEMPLATE_2(int32_t, int64_t, I64, doConvert, int64_t, int32_t, ExtendI32S)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(uint32_t, uint64_t, I64, doConvert, uint64_t, uint32_t, ExtendI32U)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(uint64_t, uint32_t, I32, doConvert, uint32_t, uint64_t, WrapI64)

        UNARY_OPERATION_OPERATION_TEMPLATE_2(float, int32_t, I32, doConvert, int32_t, float, TruncF32S)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(float, uint32_t, I32, doConvert, uint32_t, float, TruncF32U)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(double, int32_t, I32, doConvert, int32_t, double, TruncF64S)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(double, uint32_t, I32, doConvert, uint32_t, double, TruncF64U)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(float, int64_t, I64, doConvert, int64_t, float, TruncF32S)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(float, uint64_t, I64, doConvert, uint64_t, float, TruncF32U)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(double, int64_t, I64, doConvert, int64_t, double, TruncF64S)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(double, uint64_t, I64, doConvert, uint64_t, double, TruncF64U)

        UNARY_OPERATION_OPERATION_TEMPLATE_2(int32_t, float, F32, doConvert, float, int32_t, ConvertI32S)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(uint32_t, float, F32, doConvert, float, uint32_t, ConvertI32U)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(int64_t, float, F32, doConvert, float, int64_t, ConvertI64S)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(uint64_t, float, F32, doConvert, float, uint64_t, ConvertI64U)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(int32_t, double, F64, doConvert, double, int32_t, ConvertI32S)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(uint32_t, double, F64, doConvert, double, uint32_t, ConvertI32U)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(int64_t, double, F64, doConvert, double, int64_t, ConvertI64S)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(uint64_t, double, F64, doConvert, double, uint64_t, ConvertI64U)

        UNARY_OPERATION_OPERATION_TEMPLATE_2(float, int32_t, I32, intTruncSat, int32_t, float, TruncSatF32S)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(float, uint32_t, I32, intTruncSat, uint32_t, float, TruncSatF32U)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(double, int32_t, I32, intTruncSat, int32_t, double, TruncSatF64S)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(double, uint32_t, I32, intTruncSat, uint32_t, double, TruncSatF64U)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(float, int64_t, I64, intTruncSat, int64_t, float, TruncSatF32S)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(float, uint64_t, I64, intTruncSat, uint64_t, float, TruncSatF32U)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(double, int64_t, I64, intTruncSat, int64_t, double, TruncSatF64S)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(double, uint64_t, I64, intTruncSat, uint64_t, double, TruncSatF64U)

        UNARY_OPERATION_OPERATION_TEMPLATE_2(float, double, F64, doConvert, double, float, PromoteF32)
        UNARY_OPERATION_OPERATION_TEMPLATE_2(double, float, F32, doConvert, float, double, DemoteF64)

        UNARY_OPERATION_NOOP(I32, ReinterpretF32)
        UNARY_OPERATION_NOOP(F32, ReinterpretI32)
        UNARY_OPERATION_NOOP(I64, ReinterpretF64)
        UNARY_OPERATION_NOOP(F64, ReinterpretI64)

        DEFINE_OPCODE(Drop)
            :
        {
            Drop* code = (Drop*)programCounter;
            if (UNLIKELY(code->parameterSize())) {
                auto dest = sp - code->dropSize();
                auto src = sp - code->parameterSize();
                sp -= (code->dropSize() - code->parameterSize());
                memmove(dest, src, code->parameterSize());
            } else {
                sp -= code->dropSize();
            }
            ADD_PROGRAM_COUNTER(Drop);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(Jump)
            :
        {
            Jump* code = (Jump*)programCounter;
            programCounter += code->offset();
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(JumpIfTrue)
            :
        {
            JumpIfTrue* code = (JumpIfTrue*)programCounter;
            if (readValue<int32_t>(sp)) {
                programCounter += code->offset();
            } else {
                ADD_PROGRAM_COUNTER(JumpIfTrue);
            }
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(JumpIfFalse)
            :
        {
            JumpIfFalse* code = (JumpIfFalse*)programCounter;
            if (readValue<int32_t>(sp)) {
                ADD_PROGRAM_COUNTER(JumpIfFalse);
            } else {
                programCounter += code->offset();
            }
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(Call)
            :
        {
            callOperation(state, programCounter, bp, sp);
            ADD_PROGRAM_COUNTER(Call);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(CallIndirect)
            :
        {
            callIndirectOperation(state, programCounter, bp, sp);
            ADD_PROGRAM_COUNTER(CallIndirect);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(BrTable)
            :
        {
            BrTable* code = (BrTable*)programCounter;
            uint32_t value = readValue<uint32_t>(sp);

            if (value >= code->tableSize()) {
                // default case
                programCounter += code->defaultOffset();
            } else {
                programCounter += code->jumpOffsets()[value];
            }
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(GlobalGet4)
            :
        {
            GlobalGet4* code = (GlobalGet4*)programCounter;
            instance->global(code->index()).writeToStack<4>(sp);
            ADD_PROGRAM_COUNTER(GlobalGet4);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(GlobalGet8)
            :
        {
            GlobalGet8* code = (GlobalGet8*)programCounter;
            instance->global(code->index()).writeToStack<8>(sp);
            ADD_PROGRAM_COUNTER(GlobalGet8);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(GlobalSet4)
            :
        {
            GlobalSet4* code = (GlobalSet4*)programCounter;
            instance->global(code->index()).readFromStack<4>(sp);
            ADD_PROGRAM_COUNTER(GlobalSet4);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(GlobalSet8)
            :
        {
            GlobalSet8* code = (GlobalSet8*)programCounter;
            instance->global(code->index()).readFromStack<8>(sp);
            ADD_PROGRAM_COUNTER(GlobalSet8);
            NEXT_INSTRUCTION();
        }

        MEMORY_LOAD_OPERATION(I32Load, int32_t, int32_t)
        MEMORY_LOAD_OPERATION(I32Load8S, int8_t, int32_t)
        MEMORY_LOAD_OPERATION(I32Load8U, uint8_t, int32_t)
        MEMORY_LOAD_OPERATION(I32Load16S, int16_t, int32_t)
        MEMORY_LOAD_OPERATION(I32Load16U, uint16_t, int32_t)
        MEMORY_LOAD_OPERATION(I64Load, int64_t, int64_t)
        MEMORY_LOAD_OPERATION(I64Load8S, int8_t, int64_t)
        MEMORY_LOAD_OPERATION(I64Load8U, uint8_t, int64_t)
        MEMORY_LOAD_OPERATION(I64Load16S, int16_t, int64_t)
        MEMORY_LOAD_OPERATION(I64Load16U, uint16_t, int64_t)
        MEMORY_LOAD_OPERATION(I64Load32S, int32_t, int64_t)
        MEMORY_LOAD_OPERATION(I64Load32U, uint32_t, int64_t)
        MEMORY_LOAD_OPERATION(F32Load, float, float)
        MEMORY_LOAD_OPERATION(F64Load, double, double)

        MEMORY_STORE_OPERATION(I32Store, int32_t, int32_t)
        MEMORY_STORE_OPERATION(I32Store16, int32_t, int16_t)
        MEMORY_STORE_OPERATION(I32Store8, int32_t, int8_t)
        MEMORY_STORE_OPERATION(I64Store, int64_t, int64_t)
        MEMORY_STORE_OPERATION(I64Store32, int64_t, int32_t)
        MEMORY_STORE_OPERATION(I64Store16, int64_t, int16_t)
        MEMORY_STORE_OPERATION(I64Store8, int64_t, int8_t)
        MEMORY_STORE_OPERATION(F32Store, float, float)
        MEMORY_STORE_OPERATION(F64Store, double, double)

        DEFINE_OPCODE(MemorySize)
            :
        {
            writeValue<int32_t>(sp, instance->memory(0)->sizeInPageSize());
            ADD_PROGRAM_COUNTER(MemorySize);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(MemoryGrow)
            :
        {
            Memory* m = instance->memory(0);
            auto oldSize = m->sizeInPageSize();
            if (m->grow(readValue<int32_t>(sp) * (uint64_t)Memory::s_memoryPageSize)) {
                writeValue<int32_t>(sp, oldSize);
            } else {
                writeValue<int32_t>(sp, -1);
            }
            ADD_PROGRAM_COUNTER(MemoryGrow);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(MemoryInit)
            :
        {
            MemoryInit* code = (MemoryInit*)programCounter;
            Memory* m = instance->memory(0);
            DataSegment& sg = instance->dataSegment(code->segmentIndex());
            auto size = readValue<int32_t>(sp);
            auto srcStart = readValue<int32_t>(sp);
            auto dstStart = readValue<int32_t>(sp);
            m->init(&sg, dstStart, srcStart, size);
            ADD_PROGRAM_COUNTER(MemoryInit);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(MemoryCopy)
            :
        {
            Memory* m = instance->memory(0);
            auto size = readValue<int32_t>(sp);
            auto srcStart = readValue<int32_t>(sp);
            auto dstStart = readValue<int32_t>(sp);
            m->copy(dstStart, srcStart, size);
            ADD_PROGRAM_COUNTER(MemoryCopy);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(MemoryFill)
            :
        {
            Memory* m = instance->memory(0);
            auto size = readValue<int32_t>(sp);
            auto value = readValue<int32_t>(sp);
            auto dstStart = readValue<int32_t>(sp);
            m->fill(dstStart, value, size);
            ADD_PROGRAM_COUNTER(MemoryCopy);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(DataDrop)
            :
        {
            DataDrop* code = (DataDrop*)programCounter;
            DataSegment& sg = instance->dataSegment(code->segmentIndex());
            sg.drop();
            ADD_PROGRAM_COUNTER(DataDrop);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(TableGet)
            :
        {
            TableGet* code = (TableGet*)programCounter;
            Table* table = instance->table(code->tableIndex());
            Value val = table->getElement(readValue<uint32_t>(sp));
            val.writeToStack(sp);

            ADD_PROGRAM_COUNTER(TableGet);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(TableSet)
            :
        {
            TableSet* code = (TableSet*)programCounter;
            Table* table = instance->table(code->tableIndex());
            Value val(table->type(), reinterpret_cast<intptr_t>(readValue<void*>(sp)), Value::Force);
            table->setElement(readValue<uint32_t>(sp), val);

            ADD_PROGRAM_COUNTER(TableSet);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(TableGrow)
            :
        {
            TableGrow* code = (TableGrow*)programCounter;
            Table* table = instance->table(code->tableIndex());
            size_t size = table->size();
            size_t newSize = readValue<int32_t>(sp) + size;
            // FIXME read reference
            Value val(reinterpret_cast<Function*>(readValue<void*>(sp)));

            if (newSize <= table->maximumSize()) {
                table->grow(newSize, val);
                writeValue<int32_t>(sp, size);
            } else {
                writeValue<int32_t>(sp, -1);
            }

            ADD_PROGRAM_COUNTER(TableGrow);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(TableSize)
            :
        {
            TableSize* code = (TableSize*)programCounter;
            Table* table = instance->table(code->tableIndex());
            size_t size = table->size();
            writeValue<int32_t>(sp, size);

            ADD_PROGRAM_COUNTER(TableSize);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(TableCopy)
            :
        {
            TableCopy* code = (TableCopy*)programCounter;
            Table* dstTable = instance->table(code->dstIndex());
            Table* srcTable = instance->table(code->srcIndex());
            int n = readValue<int32_t>(sp);
            int srcIndex = readValue<int32_t>(sp);
            int dstIndex = readValue<int32_t>(sp);
            dstTable->copy(srcTable, n, srcIndex, dstIndex);

            ADD_PROGRAM_COUNTER(TableCopy);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(TableFill)
            :
        {
            TableFill* code = (TableFill*)programCounter;
            Table* table = instance->table(code->tableIndex());
            int32_t n = readValue<int32_t>(sp);
            Value val(reinterpret_cast<Function*>(readValue<void*>(sp)));
            int32_t index = readValue<int32_t>(sp);
            table->fill(n, val, index);

            ADD_PROGRAM_COUNTER(TableFill);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(TableInit)
            :
        {
            TableInit* code = (TableInit*)programCounter;
            ElementSegment& sg = instance->elementSegment(code->segmentIndex());
            auto size = readValue<int32_t>(sp);
            auto srcStart = readValue<int32_t>(sp);
            auto dstStart = readValue<int32_t>(sp);
            instance->table(code->tableIndex())->init(instance, &sg, dstStart, srcStart, size);
            ADD_PROGRAM_COUNTER(TableInit);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(ElemDrop)
            :
        {
            ElemDrop* code = (ElemDrop*)programCounter;
            instance->elementSegment(code->segmentIndex()).drop();
            ADD_PROGRAM_COUNTER(ElemDrop);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(RefNull)
            :
        {
            Value(Value::Null).writeToStack(sp);

            ADD_PROGRAM_COUNTER(RefNull);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(RefIsNull)
            :
        {
            Value val(reinterpret_cast<Function*>(readValue<void*>(sp)));
            writeValue(sp, (int32_t)val.isNull());

            ADD_PROGRAM_COUNTER(RefNull);
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(Throw)
            :
        {
            Throw* code = (Throw*)programCounter;
            Tag* tag = instance->tag(code->tagIndex());
            Vector<uint8_t, GCUtil::gc_malloc_allocator<uint8_t>> userExceptionData;
            size_t sz = tag->functionType()->paramStackSize();
            userExceptionData.resizeWithUninitializedValues(sz);
            memcpy(userExceptionData.data(), sp - sz, sz);
            Trap::throwException(tag, std::move(userExceptionData));
            ASSERT_NOT_REACHED();
            NEXT_INSTRUCTION();
        }

        DEFINE_OPCODE(Unreachable)
            :
        {
            Trap::throwException("unreachable executed");
            ASSERT_NOT_REACHED();
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

NEVER_INLINE void Interpreter::callIndirectOperation(
    ExecutionState& state,
    size_t programCounter,
    uint8_t* bp,
    uint8_t*& sp)
{
    CallIndirect* code = (CallIndirect*)programCounter;
    Table* table = state.currentFunction()->asDefinedFunction()->instance()->table(code->tableIndex());

    uint32_t idx = readValue<uint32_t>(sp);
    if (idx >= table->size()) {
        Trap::throwException("undefined element");
    }
    auto val = table->uncheckedGetElement(idx);
    if (val.type() != Value::FuncRef) {
        Trap::throwException("uninitialized element");
    }
    Function* target = val.asFunction();
    FunctionType* ft = target->functionType();
    if (!ft->equals(state.currentFunction()->asDefinedFunction()->instance()->module()->functionType(code->functionTypeIndex()))) {
        Trap::throwException("indirect call type mismatch");
    }
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
