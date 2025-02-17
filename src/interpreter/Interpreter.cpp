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
#include "runtime/Instance.h"
#include "runtime/Function.h"
#include "runtime/Memory.h"
#include "runtime/Table.h"
#include "runtime/Global.h"
#include "runtime/Module.h"
#include "runtime/Trap.h"
#include "runtime/Tag.h"
#include "util/MathOperation.h"

#if defined(WALRUS_ENABLE_COMPUTED_GOTO) && !defined(WALRUS_COMPUTED_GOTO_INTERPRETER_INIT_WITH_NULL)
extern char FillByteCodeOpcodeTableAsmLbl[];
const void* FillByteCodeOpcodeAddress[] = { &FillByteCodeOpcodeTableAsmLbl[0] };
#endif

namespace Walrus {

ByteCodeTable g_byteCodeTable;


// SIMD Structures
template <typename T, uint8_t L>
struct SIMDValue {
    using LaneType = T;
    static constexpr uint8_t Lanes = L;

    T v[L];

    inline T& operator[](uint8_t idx)
    {
#if defined(WALRUS_BIG_ENDIAN)
        idx = (~idx) & (L - 1);
#endif
        ASSERT(idx < L);
        return v[idx];
    }
};

typedef SIMDValue<int8_t, 16> S8x16;
typedef SIMDValue<uint8_t, 16> U8x16;
typedef SIMDValue<int16_t, 8> S16x8;
typedef SIMDValue<uint16_t, 8> U16x8;
typedef SIMDValue<int32_t, 4> S32x4;
typedef SIMDValue<uint32_t, 4> U32x4;
typedef SIMDValue<int64_t, 2> S64x2;
typedef SIMDValue<uint64_t, 2> U64x2;
typedef SIMDValue<float, 4> F32x4;
typedef SIMDValue<double, 2> F64x2;

// for load extend instructions
typedef SIMDValue<int8_t, 8> S8x8;
typedef SIMDValue<uint8_t, 8> U8x8;
typedef SIMDValue<int16_t, 4> S16x4;
typedef SIMDValue<uint16_t, 4> U16x4;
typedef SIMDValue<int32_t, 2> S32x2;
typedef SIMDValue<uint32_t, 2> U32x2;

COMPILE_ASSERT(sizeof(uint32_t) == sizeof(float), "");
COMPILE_ASSERT(sizeof(uint64_t) == sizeof(double), "");
COMPILE_ASSERT(sizeof(S8x16) == sizeof(Vec128), "");
COMPILE_ASSERT(sizeof(U8x16) == sizeof(Vec128), "");
COMPILE_ASSERT(sizeof(S16x8) == sizeof(Vec128), "");
COMPILE_ASSERT(sizeof(U16x8) == sizeof(Vec128), "");
COMPILE_ASSERT(sizeof(S32x4) == sizeof(Vec128), "");
COMPILE_ASSERT(sizeof(U32x4) == sizeof(Vec128), "");
COMPILE_ASSERT(sizeof(S64x2) == sizeof(Vec128), "");
COMPILE_ASSERT(sizeof(U64x2) == sizeof(Vec128), "");
COMPILE_ASSERT(sizeof(F32x4) == sizeof(Vec128), "");
COMPILE_ASSERT(sizeof(F64x2) == sizeof(Vec128), "");

COMPILE_ASSERT(sizeof(S8x8) == 8, "");
COMPILE_ASSERT(sizeof(U8x8) == 8, "");
COMPILE_ASSERT(sizeof(S16x4) == 8, "");
COMPILE_ASSERT(sizeof(U16x4) == 8, "");
COMPILE_ASSERT(sizeof(S32x2) == 8, "");
COMPILE_ASSERT(sizeof(U32x2) == 8, "");

template <typename T>
struct SIMDType;
template <>
struct SIMDType<int8_t> {
    using Type = S8x16;
};
template <>
struct SIMDType<uint8_t> {
    using Type = U8x16;
};
template <>
struct SIMDType<int16_t> {
    using Type = S16x8;
};
template <>
struct SIMDType<uint16_t> {
    using Type = U16x8;
};
template <>
struct SIMDType<int32_t> {
    using Type = S32x4;
};
template <>
struct SIMDType<uint32_t> {
    using Type = U32x4;
};
template <>
struct SIMDType<int64_t> {
    using Type = S64x2;
};
template <>
struct SIMDType<uint64_t> {
    using Type = U64x2;
};
template <>
struct SIMDType<float> {
    using Type = F32x4;
};
template <>
struct SIMDType<double> {
    using Type = F64x2;
};

ByteCodeTable::ByteCodeTable()
{
#if defined(WALRUS_ENABLE_COMPUTED_GOTO)
    // Dummy bytecode execution to initialize the ByteCodeTable.
    ExecutionState dummyState;
    ByteCode b;
#if defined(WALRUS_COMPUTED_GOTO_INTERPRETER_INIT_WITH_NULL)
    b.m_opcodeInAddress = nullptr;
#else
    b.m_opcodeInAddress = const_cast<void*>(FillByteCodeOpcodeAddress[0]);
#endif
    size_t pc = reinterpret_cast<size_t>(&b);
    Interpreter::interpret(dummyState, pc, nullptr, nullptr);
#endif
}

template <typename T>
ALWAYS_INLINE void writeValue(uint8_t* bp, ByteCodeStackOffset offset, const T& v)
{
    *reinterpret_cast<T*>(bp + offset) = v;
}

template <typename T>
ALWAYS_INLINE T readValue(uint8_t* bp, ByteCodeStackOffset offset)
{
    return *reinterpret_cast<T*>(bp + offset);
}

// SIMD helper function

template <typename T>
bool intEqz(T val) { return val == 0; }
template <typename T>
bool eq(ExecutionState& state, T lhs, T rhs) { return lhs == rhs; }
template <typename T>
bool ne(ExecutionState& state, T lhs, T rhs) { return lhs != rhs; }
template <typename T>
bool lt(ExecutionState& state, T lhs, T rhs) { return lhs < rhs; }
template <typename T>
bool le(ExecutionState& state, T lhs, T rhs) { return lhs <= rhs; }
template <typename T>
bool gt(ExecutionState& state, T lhs, T rhs) { return lhs > rhs; }
template <typename T>
bool ge(ExecutionState& state, T lhs, T rhs) { return lhs >= rhs; }
template <typename T>
T add(ExecutionState& state, T lhs, T rhs) { return canonNaN(lhs + rhs); }
template <typename T>
T sub(ExecutionState& state, T lhs, T rhs) { return canonNaN(lhs - rhs); }
template <typename T>
T xchg(ExecutionState& state, T lhs, T rhs) { return rhs; }
template <typename T>
T intAnd(ExecutionState& state, T lhs, T rhs) { return lhs & rhs; }
template <typename T>
T intOr(ExecutionState& state, T lhs, T rhs) { return lhs | rhs; }
template <typename T>
T intXor(ExecutionState& state, T lhs, T rhs) { return lhs ^ rhs; }
template <typename T>
T intShl(ExecutionState& state, T lhs, T rhs) { return lhs << shiftMask(rhs); }
template <typename T>
T intShr(ExecutionState& state, T lhs, T rhs) { return lhs >> shiftMask(rhs); }
template <typename T>
T intMin(ExecutionState& state, T lhs, T rhs) { return std::min(lhs, rhs); }
template <typename T>
T intMax(ExecutionState& state, T lhs, T rhs) { return std::max(lhs, rhs); }
template <typename T>
T intAndNot(ExecutionState& state, T lhs, T rhs) { return lhs & ~rhs; }
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
T intAvgr(ExecutionState& state, T lhs, T rhs) { return (lhs + rhs + 1) / 2; }

template <typename T>
T intDiv(ExecutionState& state, T lhs, T rhs)
{
    if (UNLIKELY(rhs == 0)) {
        Trap::throwException(state, "integer divide by zero");
    }
    if (UNLIKELY(!isNormalDivRem(lhs, rhs))) {
        Trap::throwException(state, "integer overflow");
    }
    return lhs / rhs;
}

template <typename T>
T intRem(ExecutionState& state, T lhs, T rhs)
{
    if (UNLIKELY(rhs == 0)) {
        Trap::throwException(state, "integer divide by zero");
    }
    if (LIKELY(isNormalDivRem(lhs, rhs))) {
        return lhs % rhs;
    } else {
        return 0;
    }
}

template <typename R, typename T>
R doConvert(ExecutionState& state, T val)
{
    if (std::is_integral<R>::value && std::is_floating_point<T>::value) {
        // Don't use std::isnan here because T may be a non-floating-point type.
        if (UNLIKELY(isNaN(val))) {
            Trap::throwException(state, "invalid conversion to integer");
        }
    }
    if (UNLIKELY(!canConvert<R>(val))) {
        Trap::throwException(state, "integer overflow");
    }
    return convert<R>(val);
}

template <typename T>
inline static void simdSwizzleOperation(ExecutionState& state, BinaryOperation* code, uint8_t* bp)
{
    using Type = typename SIMDType<T>::Type;
    auto lhs = readValue<Type>(bp, code->srcOffset()[0]);
    auto rhs = readValue<Type>(bp, code->srcOffset()[1]);
    Type result;
    for (uint8_t i = 0; i < Type::Lanes; i++) {
        result[i] = rhs[i] < Type::Lanes ? lhs[rhs[i]] : 0;
    }
    writeValue<Type>(bp, code->dstOffset(), result);
}

inline static void simdBitSelectOperation(ExecutionState& state, ByteCodeOffset4* code, uint8_t* bp)
{
    using Type = typename SIMDType<uint64_t>::Type;
    auto src0 = readValue<Type>(bp, code->src0Offset());
    auto src1 = readValue<Type>(bp, code->src1Offset());
    auto src2 = readValue<Type>(bp, code->src2Offset());
    Type result;
    for (uint8_t i = 0; i < Type::Lanes; i++) {
        result[i] = (src0[i] & src2[i]) | (src1[i] & ~src2[i]);
    }
    writeValue<Type>(bp, code->dstOffset(), result);
}

// FIXME optimize this function
template <typename P, typename R, bool Low>
inline static void simdExtmulOperation(ExecutionState& state, BinaryOperation* code, uint8_t* bp)
{
    using ParamType = typename SIMDType<P>::Type;
    using ResultType = typename SIMDType<R>::Type;
    auto lhs = readValue<ParamType>(bp, code->srcOffset()[0]);
    auto rhs = readValue<ParamType>(bp, code->srcOffset()[1]);
    ResultType result;
    for (uint8_t i = 0; i < ResultType::Lanes; i++) {
        uint8_t laneIdx = (Low ? 0 : ResultType::Lanes) + i;
        result[i] = static_cast<R>(lhs[laneIdx]) * static_cast<R>(rhs[laneIdx]);
    }
    writeValue<ResultType>(bp, code->dstOffset(), result);
}

template <typename P, typename R>
inline static void simdDotOperation(ExecutionState& state, BinaryOperation* code, uint8_t* bp)
{
    using ParamType = typename SIMDType<P>::Type;
    using ResultType = typename SIMDType<R>::Type;
    auto lhs = readValue<ParamType>(bp, code->srcOffset()[0]);
    auto rhs = readValue<ParamType>(bp, code->srcOffset()[1]);
    ResultType result;
    for (uint8_t i = 0; i < ResultType::Lanes; i++) {
        uint8_t laneIdx = i * 2;
        uint32_t lo = static_cast<uint32_t>(lhs[laneIdx]) * static_cast<uint32_t>(rhs[laneIdx]);
        uint32_t hi = static_cast<uint32_t>(lhs[laneIdx + 1]) * static_cast<uint32_t>(rhs[laneIdx + 1]);
        result[i] = add(state, lo, hi);
    }
    writeValue<ResultType>(bp, code->dstOffset(), result);
}

inline static void simdDotAddOperation(ExecutionState& state, TernaryOperation* code, uint8_t* bp)
{
    using ParamType = typename SIMDType<int8_t>::Type;
    using ResultType = typename SIMDType<int32_t>::Type;
    auto src0 = readValue<ParamType>(bp, code->src0Offset());
    auto src1 = readValue<ParamType>(bp, code->src1Offset());
    auto src2 = readValue<ResultType>(bp, code->src2Offset());
    ResultType result;
    for (uint8_t i = 0; i < ResultType::Lanes; i++) {
        uint8_t laneIdx = i * 4;
        int16_t lo0 = static_cast<int16_t>(src0[laneIdx]) * static_cast<int16_t>(src1[laneIdx]);
        int16_t hi0 = static_cast<int16_t>(src0[laneIdx + 1]) * static_cast<int16_t>(src1[laneIdx + 1]);
        int16_t lo1 = static_cast<int16_t>(src0[laneIdx + 2]) * static_cast<int16_t>(src1[laneIdx + 2]);
        int16_t hi1 = static_cast<int16_t>(src0[laneIdx + 3]) * static_cast<int16_t>(src1[laneIdx + 3]);
        int32_t tmp = static_cast<int16_t>(lo0 + hi0) + static_cast<int16_t>(lo1 + hi1);
        result[i] = add(state, tmp, src2[i]);
    }
    writeValue<ResultType>(bp, code->dstOffset(), result);
}

template <typename P, typename R>
inline static void simdNarrowOperation(ExecutionState& state, BinaryOperation* code, uint8_t* bp)
{
    using ParamType = typename SIMDType<P>::Type;
    using ResultType = typename SIMDType<R>::Type;
    auto lhs = readValue<ParamType>(bp, code->srcOffset()[0]);
    auto rhs = readValue<ParamType>(bp, code->srcOffset()[1]);
    ResultType result;
    for (uint8_t i = 0; i < ParamType::Lanes; i++) {
        result[i] = saturate<R, P>(lhs[i]);
    }
    for (uint8_t i = 0; i < ParamType::Lanes; i++) {
        result[ParamType::Lanes + i] = saturate<R, P>(rhs[i]);
    }
    writeValue<ResultType>(bp, code->dstOffset(), result);
}

inline static void simdAnyTrueOperation(ExecutionState& state, UnaryOperation* code, uint8_t* bp)
{
    using Type = typename SIMDType<uint8_t>::Type;
    auto val = readValue<Type>(bp, code->srcOffset());
    uint32_t result = (std::count_if(std::begin(val.v), std::end(val.v), [](uint8_t x) { return x != 0; }) >= 1);
    writeValue<uint32_t>(bp, code->dstOffset(), result);
}

template <typename T>
inline static void simdBitmaskOperation(ExecutionState& state, UnaryOperation* code, uint8_t* bp)
{
    using Type = typename SIMDType<T>::Type;
    auto val = readValue<Type>(bp, code->srcOffset());
    uint32_t result = 0;
    for (uint8_t i = 0; i < Type::Lanes; i++) {
        if (val[i] < 0) {
            result |= 1 << i;
        }
    }
    writeValue<uint32_t>(bp, code->dstOffset(), result);
}

template <typename T, uint8_t Count>
inline static void simdAllTrueOperation(ExecutionState& state, UnaryOperation* code, uint8_t* bp)
{
    using Type = typename SIMDType<T>::Type;
    auto val = readValue<Type>(bp, code->srcOffset());
    uint32_t result = (std::count_if(std::begin(val.v), std::end(val.v), [](uint8_t x) { return x != 0; }) >= Count);
    writeValue<uint32_t>(bp, code->dstOffset(), result);
}

template <typename P, typename R>
inline static void simdExtaddPairwiseOperation(ExecutionState& state, UnaryOperation* code, uint8_t* bp)
{
    using ParamType = typename SIMDType<P>::Type;
    using ResultType = typename SIMDType<R>::Type;
    auto val = readValue<ParamType>(bp, code->srcOffset());
    ResultType result;
    for (uint8_t i = 0; i < ResultType::Lanes; i++) {
        uint8_t laneIdx = i * 2;
        result[i] = static_cast<R>(val[laneIdx]) + static_cast<R>(val[laneIdx + 1]);
    }
    writeValue<ResultType>(bp, code->dstOffset(), result);
}

template <typename P, typename R>
inline static void simdTruncSatOperation(ExecutionState& state, UnaryOperation* code, uint8_t* bp)
{
    using ParamType = typename SIMDType<P>::Type;
    using ResultType = typename SIMDType<R>::Type;
    auto val = readValue<ParamType>(bp, code->srcOffset());
    ResultType result;
    for (uint8_t i = 0; i < ParamType::Lanes; i++) {
        result[i] = intTruncSat<R, P>(state, val[i]);
    }
    writeValue<ResultType>(bp, code->dstOffset(), result);
}

template <typename P, typename R>
inline static void simdTruncSatZeroOperation(ExecutionState& state, UnaryOperation* code, uint8_t* bp)
{
    // FIXME init result vector with zeros
    using ParamType = typename SIMDType<P>::Type;
    using ResultType = typename SIMDType<R>::Type;
    auto val = readValue<ParamType>(bp, code->srcOffset());
    ResultType result;
    for (uint8_t i = 0; i < ParamType::Lanes; i++) {
        result[i] = intTruncSat<R, P>(state, val[i]);
    }
    for (uint8_t i = ParamType::Lanes; i < ResultType::Lanes; i++) {
        result[i] = 0;
    }
    writeValue<ResultType>(bp, code->dstOffset(), result);
}

template <typename P, typename R>
inline static void simdConvertOperation(ExecutionState& state, UnaryOperation* code, uint8_t* bp)
{
    using ParamType = typename SIMDType<P>::Type;
    using ResultType = typename SIMDType<R>::Type;
    auto val = readValue<ParamType>(bp, code->srcOffset());
    ResultType result;
    for (uint8_t i = 0; i < ResultType::Lanes; i++) {
        result[i] = convert<R, P>(val[i]);
    }
    writeValue<ResultType>(bp, code->dstOffset(), result);
}

inline static void simdDemoteZeroOperation(ExecutionState& state, UnaryOperation* code, uint8_t* bp)
{
    using ParamType = typename SIMDType<double>::Type;
    using ResultType = typename SIMDType<float>::Type;
    auto val = readValue<ParamType>(bp, code->srcOffset());
    ResultType result;
    for (uint8_t i = 0; i < ParamType::Lanes; i++) {
        result[i] = convert<float, double>(val[i]);
    }
    for (uint8_t i = ParamType::Lanes; i < ResultType::Lanes; i++) {
        result[i] = 0;
    }
    writeValue<ResultType>(bp, code->dstOffset(), result);
}

template <typename P, typename R>
inline static void simdSplatOperation(ExecutionState& state, UnaryOperation* code, uint8_t* bp)
{
    using ResultType = typename SIMDType<R>::Type;
    auto val = readValue<P>(bp, code->srcOffset());
    ResultType result;
    std::fill(std::begin(result.v), std::end(result.v), val);
    writeValue<ResultType>(bp, code->dstOffset(), result);
}

#if defined(WALRUS_ENABLE_COMPUTED_GOTO)
static void initAddressToOpcodeTable()
{
#define REGISTER_TABLE(name, ...) \
    g_byteCodeTable.m_addressToOpcodeTable[g_byteCodeTable.m_addressTable[ByteCode::name##Opcode]] = ByteCode::name##Opcode;
    FOR_EACH_BYTECODE(REGISTER_TABLE)
#undef REGISTER_TABLE
}
#endif

ByteCodeStackOffset* Interpreter::interpret(ExecutionState& state,
                                            size_t programCounter,
                                            uint8_t* bp,
                                            Instance* instance)
{
    Memory** memories = reinterpret_cast<Memory**>(reinterpret_cast<uintptr_t>(instance) + Instance::alignedSize());

    state.m_programCounterPointer = &programCounter;

#define ADD_PROGRAM_COUNTER(codeName) programCounter += sizeof(codeName);

#define BINARY_OPERATION(name, op, paramType, returnType)                   \
    DEFINE_OPCODE(name)                                                     \
        :                                                                   \
    {                                                                       \
        name* code = (name*)programCounter;                                 \
        auto lhs = readValue<paramType>(bp, code->srcOffset()[0]);          \
        auto rhs = readValue<paramType>(bp, code->srcOffset()[1]);          \
        writeValue<returnType>(bp, code->dstOffset(), op(state, lhs, rhs)); \
        ADD_PROGRAM_COUNTER(name);                                          \
        NEXT_INSTRUCTION();                                                 \
    }

#define UNARY_OPERATION(name, op, type)                                                      \
    DEFINE_OPCODE(name)                                                                      \
        :                                                                                    \
    {                                                                                        \
        name* code = (name*)programCounter;                                                  \
        writeValue<type>(bp, code->dstOffset(), op(readValue<type>(bp, code->srcOffset()))); \
        ADD_PROGRAM_COUNTER(name);                                                           \
        NEXT_INSTRUCTION();                                                                  \
    }

#define UNARY_OPERATION_2(name, op, paramType, returnType, T1, T2)                                                     \
    DEFINE_OPCODE(name)                                                                                                \
        :                                                                                                              \
    {                                                                                                                  \
        name* code = (name*)programCounter;                                                                            \
        writeValue<returnType>(bp, code->dstOffset(), op<T1, T2>(state, readValue<paramType>(bp, code->srcOffset()))); \
        ADD_PROGRAM_COUNTER(name);                                                                                     \
        NEXT_INSTRUCTION();                                                                                            \
    }

#define MOVE_OPERATION(name, type)                                                                           \
    DEFINE_OPCODE(name)                                                                                      \
        :                                                                                                    \
    {                                                                                                        \
        name* code = (name*)programCounter;                                                                  \
        *reinterpret_cast<type*>(bp + code->dstOffset()) = *reinterpret_cast<type*>(bp + code->srcOffset()); \
        ADD_PROGRAM_COUNTER(name);                                                                           \
        NEXT_INSTRUCTION();                                                                                  \
    }

#define SIMD_BINARY_OPERATION(name, op, paramType, resultType)     \
    DEFINE_OPCODE(name)                                            \
        :                                                          \
    {                                                              \
        using ParamType = typename SIMDType<paramType>::Type;      \
        using ResultType = typename SIMDType<resultType>::Type;    \
        COMPILE_ASSERT(ParamType::Lanes == ResultType::Lanes, ""); \
        name* code = (name*)programCounter;                        \
        auto lhs = readValue<ParamType>(bp, code->srcOffset()[0]); \
        auto rhs = readValue<ParamType>(bp, code->srcOffset()[1]); \
        ResultType result;                                         \
        for (uint8_t i = 0; i < ParamType::Lanes; i++) {           \
            result[i] = op(state, lhs[i], rhs[i]);                 \
        }                                                          \
        writeValue<ResultType>(bp, code->dstOffset(), result);     \
        ADD_PROGRAM_COUNTER(name);                                 \
        NEXT_INSTRUCTION();                                        \
    }

#define SIMD_BINARY_SHIFT_OPERATION(name, op, opType)                   \
    DEFINE_OPCODE(name)                                                 \
        :                                                               \
    {                                                                   \
        using Type = typename SIMDType<opType>::Type;                   \
        name* code = (name*)programCounter;                             \
        auto lhs = readValue<Type>(bp, code->srcOffset()[0]);           \
        auto amount = readValue<uint32_t>(bp, code->srcOffset()[1]);    \
        Type result;                                                    \
        for (uint8_t i = 0; i < Type::Lanes; i++) {                     \
            result[i] = op(state, lhs[i], static_cast<opType>(amount)); \
        }                                                               \
        writeValue<Type>(bp, code->dstOffset(), result);                \
        ADD_PROGRAM_COUNTER(name);                                      \
        NEXT_INSTRUCTION();                                             \
    }

#define SIMD_BINARY_OTHER_OPERATION(name, op)            \
    DEFINE_OPCODE(name)                                  \
        :                                                \
    {                                                    \
        op(state, (BinaryOperation*)programCounter, bp); \
        ADD_PROGRAM_COUNTER(BinaryOperation);            \
        NEXT_INSTRUCTION();                              \
    }

#define SIMD_UNARY_OPERATION(name, op, type)               \
    DEFINE_OPCODE(name)                                    \
        :                                                  \
    {                                                      \
        using Type = typename SIMDType<type>::Type;        \
        name* code = (name*)programCounter;                \
        auto val = readValue<Type>(bp, code->srcOffset()); \
        Type result;                                       \
        for (uint8_t i = 0; i < Type::Lanes; i++) {        \
            result[i] = op(val[i]);                        \
        }                                                  \
        writeValue<Type>(bp, code->dstOffset(), result);   \
        ADD_PROGRAM_COUNTER(name);                         \
        NEXT_INSTRUCTION();                                \
    }

#define SIMD_UNARY_CONVERT_OPERATION(name, P, R, Low)                       \
    DEFINE_OPCODE(name)                                                     \
        :                                                                   \
    {                                                                       \
        using ParamType = typename SIMDType<P>::Type;                       \
        using ResultType = typename SIMDType<R>::Type;                      \
        name* code = (name*)programCounter;                                 \
        auto val = readValue<ParamType>(bp, code->srcOffset());             \
        ResultType result;                                                  \
        for (uint8_t i = 0; i < ResultType::Lanes; i++) {                   \
            result[i] = convert<R>(val[(Low ? 0 : ResultType::Lanes) + i]); \
        }                                                                   \
        writeValue<ResultType>(bp, code->dstOffset(), result);              \
        ADD_PROGRAM_COUNTER(name);                                          \
        NEXT_INSTRUCTION();                                                 \
    }

#define SIMD_UNARY_OTHER_OPERATION(name, op)            \
    DEFINE_OPCODE(name)                                 \
        :                                               \
    {                                                   \
        op(state, (UnaryOperation*)programCounter, bp); \
        ADD_PROGRAM_COUNTER(UnaryOperation);            \
        NEXT_INSTRUCTION();                             \
    }

#define SIMD_TERNARY_OPERATION(name, op, paramType, resultType)    \
    DEFINE_OPCODE(name)                                            \
        :                                                          \
    {                                                              \
        using ParamType = typename SIMDType<paramType>::Type;      \
        using ResultType = typename SIMDType<resultType>::Type;    \
        COMPILE_ASSERT(ParamType::Lanes == ResultType::Lanes, ""); \
        name* code = (name*)programCounter;                        \
        auto src0 = readValue<ParamType>(bp, code->src0Offset());  \
        auto src1 = readValue<ParamType>(bp, code->src1Offset());  \
        auto src2 = readValue<ParamType>(bp, code->src2Offset());  \
        ResultType result;                                         \
        for (uint8_t i = 0; i < ParamType::Lanes; i++) {           \
            result[i] = op(state, src0[i], src1[i], src2[i]);      \
        }                                                          \
        writeValue<ResultType>(bp, code->dstOffset(), result);     \
        ADD_PROGRAM_COUNTER(name);                                 \
        NEXT_INSTRUCTION();                                        \
    }

#define SIMD_TERNARY_OTHER_OPERATION(name, op)            \
    DEFINE_OPCODE(name)                                   \
        :                                                 \
    {                                                     \
        op(state, (TernaryOperation*)programCounter, bp); \
        ADD_PROGRAM_COUNTER(BinaryOperation);             \
        NEXT_INSTRUCTION();                               \
    }

#define MEMORY_LOAD_OPERATION(opcodeName, readType, writeType)        \
    DEFINE_OPCODE(opcodeName)                                         \
        :                                                             \
    {                                                                 \
        MemoryLoad* code = (MemoryLoad*)programCounter;               \
        uint32_t offset = readValue<uint32_t>(bp, code->srcOffset()); \
        readType value;                                               \
        memories[0]->load(state, offset, code->offset(), &value);     \
        writeValue<writeType>(bp, code->dstOffset(), value);          \
        ADD_PROGRAM_COUNTER(MemoryLoad);                              \
        NEXT_INSTRUCTION();                                           \
    }

#define MULTI_MEMORY_LOAD_OPERATION(opcodeName, readType, writeType)             \
    DEFINE_OPCODE(opcodeName##Multi)                                             \
        :                                                                        \
    {                                                                            \
        MemoryLoadMulti* code = (MemoryLoadMulti*)programCounter;                \
        uint32_t offset = readValue<uint32_t>(bp, code->srcOffset());            \
        readType value;                                                          \
        memories[code->memIndex()]->load(state, offset, code->offset(), &value); \
        writeValue<writeType>(bp, code->dstOffset(), value);                     \
        ADD_PROGRAM_COUNTER(MemoryLoadMulti);                                    \
        NEXT_INSTRUCTION();                                                      \
    }

#define MEMORY_STORE_OPERATION(opcodeName, readType, writeType)        \
    DEFINE_OPCODE(opcodeName)                                          \
        :                                                              \
    {                                                                  \
        MemoryStore* code = (MemoryStore*)programCounter;              \
        writeType value = readValue<readType>(bp, code->src1Offset()); \
        uint32_t offset = readValue<uint32_t>(bp, code->src0Offset()); \
        memories[0]->store(state, offset, code->offset(), value);      \
        ADD_PROGRAM_COUNTER(MemoryStore);                              \
        NEXT_INSTRUCTION();                                            \
    }

#define MULTI_MEMORY_STORE_OPERATION(opcodeName, readType, writeType)            \
    DEFINE_OPCODE(opcodeName##Multi)                                             \
        :                                                                        \
    {                                                                            \
        MemoryStoreMulti* code = (MemoryStoreMulti*)programCounter;              \
        writeType value = readValue<readType>(bp, code->src1Offset());           \
        uint32_t offset = readValue<uint32_t>(bp, code->src0Offset());           \
        memories[code->memIndex()]->store(state, offset, code->offset(), value); \
        ADD_PROGRAM_COUNTER(MemoryStoreMulti);                                   \
        NEXT_INSTRUCTION();                                                      \
    }

#define SIMD_MEMORY_LOAD_SPLAT_OPERATION(opcodeName, opType)          \
    DEFINE_OPCODE(opcodeName)                                         \
        :                                                             \
    {                                                                 \
        using Type = typename SIMDType<opType>::Type;                 \
        MemoryLoad* code = (MemoryLoad*)programCounter;               \
        uint32_t offset = readValue<uint32_t>(bp, code->srcOffset()); \
        opType value;                                                 \
        memories[0]->load(state, offset, code->offset(), &value);     \
        Type result;                                                  \
        std::fill(std::begin(result.v), std::end(result.v), value);   \
        writeValue<Type>(bp, code->dstOffset(), result);              \
        ADD_PROGRAM_COUNTER(MemoryLoad);                              \
        NEXT_INSTRUCTION();                                           \
    }

#define SIMD_MULTI_MEMORY_LOAD_SPLAT_OPERATION(opcodeName, opType)              \
    DEFINE_OPCODE(opcodeName##Multi)                                            \
        :                                                                       \
    {                                                                           \
        using Type = typename SIMDType<opType>::Type;                           \
        MemoryLoadMulti* code = (MemoryLoadMulti*)programCounter;               \
        uint32_t offset = readValue<uint32_t>(bp, code->srcOffset());           \
        opType value;                                                           \
        memories[code->memIndex()]->load(state, offset, code->offset(), &value); \
        Type result;                                                            \
        std::fill(std::begin(result.v), std::end(result.v), value);             \
        writeValue<Type>(bp, code->dstOffset(), result);                        \
        ADD_PROGRAM_COUNTER(MemoryLoadMulti);                                   \
        NEXT_INSTRUCTION();                                                     \
    }

#define SIMD_MEMORY_LOAD_EXTEND_OPERATION(opcodeName, readType, writeType) \
    DEFINE_OPCODE(opcodeName)                                              \
        :                                                                  \
    {                                                                      \
        using WriteType = typename SIMDType<writeType>::Type;              \
        MemoryLoad* code = (MemoryLoad*)programCounter;                    \
        uint32_t offset = readValue<uint32_t>(bp, code->srcOffset());      \
        readType value;                                                    \
        memories[0]->load(state, offset, code->offset(), &value);          \
        WriteType result;                                                  \
        for (uint8_t i = 0; i < WriteType::Lanes; i++) {                   \
            result[i] = value[i];                                          \
        }                                                                  \
        writeValue<WriteType>(bp, code->dstOffset(), result);              \
        ADD_PROGRAM_COUNTER(MemoryLoad);                                   \
        NEXT_INSTRUCTION();                                                \
    }

#define SIMD_MULTI_MEMORY_LOAD_EXTEND_OPERATION(opcodeName, readType, writeType) \
    DEFINE_OPCODE(opcodeName##Multi)                                             \
        :                                                                        \
    {                                                                            \
        using WriteType = typename SIMDType<writeType>::Type;                    \
        MemoryLoadMulti* code = (MemoryLoadMulti*)programCounter;                \
        uint32_t offset = readValue<uint32_t>(bp, code->srcOffset());            \
        readType value;                                                          \
        memories[code->memIndex()]->load(state, offset, code->offset(), &value); \
        WriteType result;                                                        \
        for (uint8_t i = 0; i < WriteType::Lanes; i++) {                         \
            result[i] = value[i];                                                \
        }                                                                        \
        writeValue<WriteType>(bp, code->dstOffset(), result);                    \
        ADD_PROGRAM_COUNTER(MemoryLoadMulti);                                    \
        NEXT_INSTRUCTION();                                                      \
    }

#define SIMD_MEMORY_LOAD_LANE_OPERATION(opcodeName, opType)            \
    DEFINE_OPCODE(opcodeName)                                          \
        :                                                              \
    {                                                                  \
        using Type = typename SIMDType<opType>::Type;                  \
        SIMDMemoryLoad* code = (SIMDMemoryLoad*)programCounter;        \
        uint32_t offset = readValue<uint32_t>(bp, code->src0Offset()); \
        Type result = readValue<Type>(bp, code->src1Offset());         \
        opType value;                                                  \
        memories[0]->load(state, offset, code->offset(), &value);      \
        result[code->index()] = value;                                 \
        writeValue<Type>(bp, code->dstOffset(), result);               \
        ADD_PROGRAM_COUNTER(SIMDMemoryLoad);                           \
        NEXT_INSTRUCTION();                                            \
    }

#define SIMD_MULTI_MEMORY_LOAD_LANE_OPERATION(opcodeName, opType)                \
    DEFINE_OPCODE(opcodeName##Multi)                                             \
        :                                                                        \
    {                                                                            \
        using Type = typename SIMDType<opType>::Type;                            \
        SIMDMemoryLoadMulti* code = (SIMDMemoryLoadMulti*)programCounter;        \
        uint32_t offset = readValue<uint32_t>(bp, code->src0Offset());           \
        Type result = readValue<Type>(bp, code->src1Offset());                   \
        opType value;                                                            \
        memories[code->memIndex()]->load(state, offset, code->offset(), &value); \
        result[code->index()] = value;                                           \
        writeValue<Type>(bp, code->dstOffset(), result);                         \
        ADD_PROGRAM_COUNTER(SIMDMemoryLoadMulti);                                \
        NEXT_INSTRUCTION();                                                      \
    }

#define SIMD_MEMORY_STORE_LANE_OPERATION(opcodeName, opType)           \
    DEFINE_OPCODE(opcodeName)                                          \
        :                                                              \
    {                                                                  \
        using Type = typename SIMDType<opType>::Type;                  \
        SIMDMemoryStore* code = (SIMDMemoryStore*)programCounter;      \
        Type result = readValue<Type>(bp, code->src1Offset());         \
        opType value = result[code->index()];                          \
        uint32_t offset = readValue<uint32_t>(bp, code->src0Offset()); \
        memories[0]->store(state, offset, code->offset(), value);      \
        ADD_PROGRAM_COUNTER(SIMDMemoryStore);                          \
        NEXT_INSTRUCTION();                                            \
    }

#define SIMD_MULTI_MEMORY_STORE_LANE_OPERATION(opcodeName, opType)               \
    DEFINE_OPCODE(opcodeName##Multi)                                             \
        :                                                                        \
    {                                                                            \
        using Type = typename SIMDType<opType>::Type;                            \
        SIMDMemoryStoreMulti* code = (SIMDMemoryStoreMulti*)programCounter;      \
        Type result = readValue<Type>(bp, code->src1Offset());                   \
        opType value = result[code->index()];                                    \
        uint32_t offset = readValue<uint32_t>(bp, code->src0Offset());           \
        memories[code->memIndex()]->store(state, offset, code->offset(), value); \
        ADD_PROGRAM_COUNTER(SIMDMemoryStoreMulti);                               \
        NEXT_INSTRUCTION();                                                      \
    }

#define SIMD_EXTRACT_LANE_OPERATION(opcodeName, readType, writeType)         \
    DEFINE_OPCODE(opcodeName)                                                \
        :                                                                    \
    {                                                                        \
        using Type = typename SIMDType<readType>::Type;                      \
        opcodeName* code = (opcodeName*)programCounter;                      \
        Type result = readValue<Type>(bp, code->srcOffset());                \
        writeValue<writeType>(bp, code->dstOffset(), result[code->index()]); \
        ADD_PROGRAM_COUNTER(opcodeName);                                     \
        NEXT_INSTRUCTION();                                                  \
    }

#define SIMD_REPLACE_LANE_OPERATION(opcodeName, readType, writeType)          \
    DEFINE_OPCODE(opcodeName)                                                 \
        :                                                                     \
    {                                                                         \
        using ResultType = typename SIMDType<writeType>::Type;                \
        opcodeName* code = (opcodeName*)programCounter;                       \
        auto val = readValue<readType>(bp, code->srcOffsets()[1]);            \
        ResultType result = readValue<ResultType>(bp, code->srcOffsets()[0]); \
        result[code->index()] = val;                                          \
        writeValue<ResultType>(bp, code->dstOffset(), result);                \
        ADD_PROGRAM_COUNTER(opcodeName);                                      \
        NEXT_INSTRUCTION();                                                   \
    }

#define ATOMIC_MEMORY_LOAD_OPERATION(opcodeName, readType, writeType)   \
    DEFINE_OPCODE(opcodeName)                                           \
        :                                                               \
    {                                                                   \
        MemoryLoad* code = (MemoryLoad*)programCounter;                 \
        uint32_t offset = readValue<uint32_t>(bp, code->srcOffset());   \
        readType value;                                                 \
        memories[0]->atomicLoad(state, offset, code->offset(), &value); \
        writeValue<writeType>(bp, code->dstOffset(), value);            \
        ADD_PROGRAM_COUNTER(MemoryLoad);                                \
        NEXT_INSTRUCTION();                                             \
    }

#define ATOMIC_MULTI_MEMORY_LOAD_OPERATION(opcodeName, readType, writeType)            \
    DEFINE_OPCODE(opcodeName##Multi)                                                   \
        :                                                                              \
    {                                                                                  \
        MemoryLoadMulti* code = (MemoryLoadMulti*)programCounter;                      \
        uint32_t offset = readValue<uint32_t>(bp, code->srcOffset());                  \
        readType value;                                                                \
        memories[code->memIndex()]->atomicLoad(state, offset, code->offset(), &value); \
        writeValue<writeType>(bp, code->dstOffset(), value);                           \
        ADD_PROGRAM_COUNTER(MemoryLoadMulti);                                          \
        NEXT_INSTRUCTION();                                                            \
    }

#define ATOMIC_MEMORY_STORE_OPERATION(opcodeName, readType, writeType)  \
    DEFINE_OPCODE(opcodeName)                                           \
        :                                                               \
    {                                                                   \
        MemoryStore* code = (MemoryStore*)programCounter;               \
        writeType value = readValue<readType>(bp, code->src1Offset());  \
        uint32_t offset = readValue<uint32_t>(bp, code->src0Offset());  \
        memories[0]->atomicStore(state, offset, code->offset(), value); \
        ADD_PROGRAM_COUNTER(MemoryStore);                               \
        NEXT_INSTRUCTION();                                             \
    }

#define ATOMIC_MULTI_MEMORY_STORE_OPERATION(opcodeName, readType, writeType)           \
    DEFINE_OPCODE(opcodeName##Multi)                                                   \
        :                                                                              \
    {                                                                                  \
        MemoryStoreMulti* code = (MemoryStoreMulti*)programCounter;                    \
        writeType value = readValue<readType>(bp, code->src1Offset());                 \
        uint32_t offset = readValue<uint32_t>(bp, code->src0Offset());                 \
        memories[code->memIndex()]->atomicStore(state, offset, code->offset(), value); \
        ADD_PROGRAM_COUNTER(MemoryStoreMulti);                                         \
        NEXT_INSTRUCTION();                                                            \
    }

#define ATOMIC_MEMORY_RMW_OPERATION(opcodeName, R, T, operationName)                       \
    DEFINE_OPCODE(opcodeName)                                                              \
        :                                                                                  \
    {                                                                                      \
        AtomicRmw* code = (AtomicRmw*)programCounter;                                      \
        T value = static_cast<T>(readValue<R>(bp, code->src1Offset()));                    \
        uint32_t offset = readValue<uint32_t>(bp, code->src0Offset());                     \
        T old;                                                                             \
        memories[0]->atomicRmw(state, offset, code->offset(), value, &old, operationName); \
        writeValue<R>(bp, code->dstOffset(), static_cast<R>(old));                         \
        ADD_PROGRAM_COUNTER(AtomicRmw);                                                    \
        NEXT_INSTRUCTION();                                                                \
    }

#define ATOMIC_MULTI_MEMORY_RMW_OPERATION(opcodeName, R, T, operationName)                                \
    DEFINE_OPCODE(opcodeName##Multi)                                                                      \
        :                                                                                                 \
    {                                                                                                     \
        AtomicRmwMulti* code = (AtomicRmwMulti*)programCounter;                                           \
        T value = static_cast<T>(readValue<R>(bp, code->src1Offset()));                                   \
        uint32_t offset = readValue<uint32_t>(bp, code->src0Offset());                                    \
        T old;                                                                                            \
        memories[code->memIndex()]->atomicRmw(state, offset, code->offset(), value, &old, operationName); \
        writeValue<R>(bp, code->dstOffset(), static_cast<R>(old));                                        \
        ADD_PROGRAM_COUNTER(AtomicRmwMulti);                                                              \
        NEXT_INSTRUCTION();                                                                               \
    }

#define ATOMIC_MEMORY_RMW_CMPXCHG_OPERATION(opcodeName, T, V)                                    \
    DEFINE_OPCODE(opcodeName)                                                                    \
        :                                                                                        \
    {                                                                                            \
        AtomicRmwCmpxchg* code = (AtomicRmwCmpxchg*)programCounter;                              \
        V replace = static_cast<V>(readValue<T>(bp, code->src2Offset()));                        \
        T expectValue = readValue<T>(bp, code->src1Offset());                                    \
        uint32_t offset = readValue<uint32_t>(bp, code->src0Offset());                           \
        V old;                                                                                   \
        if (expectValue > std::numeric_limits<V>::max()) {                                       \
            memories[0]->atomicLoad(state, offset, code->offset(), &old);                        \
        } else {                                                                                 \
            V expect = static_cast<V>(expectValue);                                              \
            memories[0]->atomicRmwCmpxchg(state, offset, code->offset(), expect, replace, &old); \
        }                                                                                        \
        writeValue<T>(bp, code->dstOffset(), static_cast<T>(old));                               \
        ADD_PROGRAM_COUNTER(AtomicRmwCmpxchg);                                                   \
        NEXT_INSTRUCTION();                                                                      \
    }

#define ATOMIC_MULTI_MEMORY_RMW_CMPXCHG_OPERATION(opcodeName, T, V)                                             \
    DEFINE_OPCODE(opcodeName##Multi)                                                                            \
        :                                                                                                       \
    {                                                                                                           \
        AtomicRmwCmpxchgMulti* code = (AtomicRmwCmpxchgMulti*)programCounter;                                   \
        V replace = static_cast<V>(readValue<T>(bp, code->src2Offset()));                                       \
        T expectValue = readValue<T>(bp, code->src1Offset());                                                   \
        uint32_t offset = readValue<uint32_t>(bp, code->src0Offset());                                          \
        V old;                                                                                                  \
        if (expectValue > std::numeric_limits<V>::max()) {                                                      \
            memories[code->memIndex()]->atomicLoad(state, offset, code->offset(), &old);                        \
        } else {                                                                                                \
            V expect = static_cast<V>(expectValue);                                                             \
            memories[code->memIndex()]->atomicRmwCmpxchg(state, offset, code->offset(), expect, replace, &old); \
        }                                                                                                       \
        writeValue<T>(bp, code->dstOffset(), static_cast<T>(old));                                              \
        ADD_PROGRAM_COUNTER(AtomicRmwCmpxchgMulti);                                                             \
        NEXT_INSTRUCTION();                                                                                     \
    }


#if defined(WALRUS_ENABLE_COMPUTED_GOTO)
#if defined(WALRUS_COMPUTED_GOTO_INTERPRETER_INIT_WITH_NULL)
    if (UNLIKELY((((ByteCode*)programCounter)->m_opcodeInAddress) == NULL)) {
        goto FillOpcodeTableOpcodeLbl;
    }
#endif

#define DEFINE_OPCODE(codeName) codeName##OpcodeLbl
#define DEFINE_DEFAULT
#define NEXT_INSTRUCTION() goto NextInstruction;

NextInstruction:
    /* Execute first instruction. */
    goto*(((ByteCode*)programCounter)->m_opcodeInAddress);
#else

#define DEFINE_OPCODE(codeName) case ByteCode::Opcode::codeName##Opcode
#define DEFINE_DEFAULT                \
    default:                          \
        RELEASE_ASSERT_NOT_REACHED(); \
        }
#define NEXT_INSTRUCTION() \
    goto NextInstruction;
NextInstruction:
    auto currentOpcode = ((ByteCode*)programCounter)->m_opcode;

    switch (currentOpcode) {
#endif

    DEFINE_OPCODE(Const32)
        :
    {
        Const32* code = (Const32*)programCounter;
        *reinterpret_cast<uint32_t*>(bp + code->dstOffset()) = code->value();
        ADD_PROGRAM_COUNTER(Const32);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Const64)
        :
    {
        Const64* code = (Const64*)programCounter;
        *reinterpret_cast<uint64_t*>(bp + code->dstOffset()) = code->value();
        ADD_PROGRAM_COUNTER(Const64);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Const128)
        :
    {
        Const128* code = (Const128*)programCounter;
        memcpy(bp + code->dstOffset(), code->value(), 16);
        ADD_PROGRAM_COUNTER(Const128);
        NEXT_INSTRUCTION();
    }

    MOVE_OPERATION(MoveI32, uint32_t)
    MOVE_OPERATION(MoveF32, float)
    MOVE_OPERATION(MoveI64, uint64_t)
    MOVE_OPERATION(MoveF64, double)

    DEFINE_OPCODE(MoveV128)
        :
    {
        MoveV128* code = (MoveV128*)programCounter;
        memcpy(bp + code->dstOffset(), bp + code->srcOffset(), 16);
        ADD_PROGRAM_COUNTER(MoveV128);
        NEXT_INSTRUCTION();
    }

    MOVE_OPERATION(I32ReinterpretF32, uint32_t)
    MOVE_OPERATION(I64ReinterpretF64, uint64_t)
    MOVE_OPERATION(F32ReinterpretI32, uint32_t)
    MOVE_OPERATION(F64ReinterpretI64, uint64_t)

    DEFINE_OPCODE(Load32)
        :
    {
        Load32* code = (Load32*)programCounter;
        uint32_t offset = readValue<uint32_t>(bp, code->srcOffset());
        memories[0]->load(state, offset, reinterpret_cast<uint32_t*>(bp + code->dstOffset()));
        ADD_PROGRAM_COUNTER(Load32);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Load32Multi)
        :
    {
        Load32Multi* code = (Load32Multi*)programCounter;
        uint32_t offset = readValue<uint32_t>(bp, code->srcOffset());
        memories[code->memIndex()]->load(state, offset, reinterpret_cast<uint32_t*>(bp + code->dstOffset()));
        ADD_PROGRAM_COUNTER(Load32Multi);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Load64)
        :
    {
        Load64* code = (Load64*)programCounter;
        uint32_t offset = readValue<uint32_t>(bp, code->srcOffset());
        memories[0]->load(state, offset, reinterpret_cast<uint64_t*>(bp + code->dstOffset()));
        ADD_PROGRAM_COUNTER(Load64);
        NEXT_INSTRUCTION();
    }
    DEFINE_OPCODE(Load64Multi)
        :
    {
        Load64Multi* code = (Load64Multi*)programCounter;
        uint32_t offset = readValue<uint32_t>(bp, code->srcOffset());
        memories[code->memIndex()]->load(state, offset, reinterpret_cast<uint64_t*>(bp + code->dstOffset()));
        ADD_PROGRAM_COUNTER(Load64Multi);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Store32)
        :
    {
        Store32* code = (Store32*)programCounter;
        uint32_t value = readValue<uint32_t>(bp, code->src1Offset());
        uint32_t offset = readValue<uint32_t>(bp, code->src0Offset());
        memories[0]->store(state, offset, value);
        ADD_PROGRAM_COUNTER(Store32);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Store32Multi)
        :
    {
        Store32Multi* code = (Store32Multi*)programCounter;
        uint32_t value = readValue<uint32_t>(bp, code->src1Offset());
        uint32_t offset = readValue<uint32_t>(bp, code->src0Offset());
        memories[code->memIndex()]->store(state, offset, value);
        ADD_PROGRAM_COUNTER(Store32Multi);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Store64)
        :
    {
        Store64* code = (Store64*)programCounter;
        uint64_t value = readValue<uint64_t>(bp, code->src1Offset());
        uint32_t offset = readValue<uint32_t>(bp, code->src0Offset());
        memories[0]->store(state, offset, value);
        ADD_PROGRAM_COUNTER(Store64);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Store64Multi)
        :
    {
        Store64Multi* code = (Store64Multi*)programCounter;
        uint64_t value = readValue<uint64_t>(bp, code->src1Offset());
        uint32_t offset = readValue<uint32_t>(bp, code->src0Offset());
        memories[code->memIndex()]->store(state, offset, value);
        ADD_PROGRAM_COUNTER(Store64Multi);
        NEXT_INSTRUCTION();
    }

    FOR_EACH_BYTECODE_BINARY_OP(BINARY_OPERATION)
    FOR_EACH_BYTECODE_UNARY_OP(UNARY_OPERATION)
    FOR_EACH_BYTECODE_UNARY_OP_2(UNARY_OPERATION_2)
    FOR_EACH_BYTECODE_SIMD_BINARY_OP(SIMD_BINARY_OPERATION)
    FOR_EACH_BYTECODE_SIMD_BINARY_SHIFT_OP(SIMD_BINARY_SHIFT_OPERATION)
    FOR_EACH_BYTECODE_SIMD_BINARY_OTHER(SIMD_BINARY_OTHER_OPERATION)
    FOR_EACH_BYTECODE_RELAXED_SIMD_BINARY_OP(SIMD_BINARY_OPERATION)
    FOR_EACH_BYTECODE_RELAXED_SIMD_BINARY_OTHER(SIMD_BINARY_OTHER_OPERATION)
    FOR_EACH_BYTECODE_SIMD_UNARY_OP(SIMD_UNARY_OPERATION)
    FOR_EACH_BYTECODE_SIMD_UNARY_CONVERT_OP(SIMD_UNARY_CONVERT_OPERATION)
    FOR_EACH_BYTECODE_SIMD_UNARY_OTHER(SIMD_UNARY_OTHER_OPERATION)
    FOR_EACH_BYTECODE_RELAXED_SIMD_UNARY_OTHER(SIMD_UNARY_OTHER_OPERATION)
    FOR_EACH_BYTECODE_RELAXED_SIMD_TERNARY_OP(SIMD_TERNARY_OPERATION)
    FOR_EACH_BYTECODE_RELAXED_SIMD_TERNARY_OTHER(SIMD_TERNARY_OTHER_OPERATION)

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
        if (readValue<int32_t>(bp, code->srcOffset())) {
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
        if (readValue<int32_t>(bp, code->srcOffset())) {
            ADD_PROGRAM_COUNTER(JumpIfFalse);
        } else {
            programCounter += code->offset();
        }
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Call)
        :
    {
        callOperation(state, programCounter, bp, instance);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(CallIndirect)
        :
    {
        callIndirectOperation(state, programCounter, bp, instance);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Select)
        :
    {
        Select* code = (Select*)programCounter;
        auto cond = readValue<int32_t>(bp, code->condOffset());
        if (cond) {
            memmove(bp + code->dstOffset(), bp + code->src0Offset(), code->valueSize());
        } else {
            memmove(bp + code->dstOffset(), bp + code->src1Offset(), code->valueSize());
        }

        ADD_PROGRAM_COUNTER(Select);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(BrTable)
        :
    {
        BrTable* code = (BrTable*)programCounter;
        uint32_t value = readValue<uint32_t>(bp, code->condOffset());

        if (value >= code->tableSize()) {
            // default case
            programCounter += code->defaultOffset();
        } else {
            programCounter += code->jumpOffsets()[value];
        }
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(GlobalGet32)
        :
    {
        GlobalGet32* code = (GlobalGet32*)programCounter;
        ASSERT(code->index() < instance->module()->numberOfGlobalTypes());
        instance->m_globals[code->index()]->value().writeNBytesToMemory<4>(bp + code->dstOffset());
        ADD_PROGRAM_COUNTER(GlobalGet32);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(GlobalGet64)
        :
    {
        GlobalGet64* code = (GlobalGet64*)programCounter;
        ASSERT(code->index() < instance->module()->numberOfGlobalTypes());
        instance->m_globals[code->index()]->value().writeNBytesToMemory<8>(bp + code->dstOffset());
        ADD_PROGRAM_COUNTER(GlobalGet64);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(GlobalGet128)
        :
    {
        GlobalGet128* code = (GlobalGet128*)programCounter;
        ASSERT(code->index() < instance->module()->numberOfGlobalTypes());
        instance->m_globals[code->index()]->value().writeNBytesToMemory<16>(bp + code->dstOffset());
        ADD_PROGRAM_COUNTER(GlobalGet128);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(GlobalSet32)
        :
    {
        GlobalSet32* code = (GlobalSet32*)programCounter;
        ASSERT(code->index() < instance->module()->numberOfGlobalTypes());
        Value& val = instance->m_globals[code->index()]->value();
        val.readFromStack<4>(bp + code->srcOffset());
        ADD_PROGRAM_COUNTER(GlobalSet32);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(GlobalSet64)
        :
    {
        GlobalSet64* code = (GlobalSet64*)programCounter;
        ASSERT(code->index() < instance->module()->numberOfGlobalTypes());
        Value& val = instance->m_globals[code->index()]->value();
        val.readFromStack<8>(bp + code->srcOffset());
        ADD_PROGRAM_COUNTER(GlobalSet64);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(GlobalSet128)
        :
    {
        GlobalSet128* code = (GlobalSet128*)programCounter;
        ASSERT(code->index() < instance->module()->numberOfGlobalTypes());
        Value& val = instance->m_globals[code->index()]->value();
        val.readFromStack<16>(bp + code->srcOffset());
        ADD_PROGRAM_COUNTER(GlobalSet128);
        NEXT_INSTRUCTION();
    }

    FOR_EACH_BYTECODE_LOAD_OP(MEMORY_LOAD_OPERATION)
    FOR_EACH_BYTECODE_LOAD_OP(MULTI_MEMORY_LOAD_OPERATION)
    FOR_EACH_BYTECODE_STORE_OP(MEMORY_STORE_OPERATION)
    FOR_EACH_BYTECODE_STORE_OP(MULTI_MEMORY_STORE_OPERATION)
    FOR_EACH_BYTECODE_SIMD_LOAD_SPLAT_OP(SIMD_MEMORY_LOAD_SPLAT_OPERATION)
    FOR_EACH_BYTECODE_SIMD_LOAD_SPLAT_OP(SIMD_MULTI_MEMORY_LOAD_SPLAT_OPERATION)
    FOR_EACH_BYTECODE_SIMD_LOAD_EXTEND_OP(SIMD_MEMORY_LOAD_EXTEND_OPERATION)
    FOR_EACH_BYTECODE_SIMD_LOAD_EXTEND_OP(SIMD_MULTI_MEMORY_LOAD_EXTEND_OPERATION)
    FOR_EACH_BYTECODE_SIMD_LOAD_LANE_OP(SIMD_MEMORY_LOAD_LANE_OPERATION)
    FOR_EACH_BYTECODE_SIMD_LOAD_LANE_OP(SIMD_MULTI_MEMORY_LOAD_LANE_OPERATION)
    FOR_EACH_BYTECODE_SIMD_STORE_LANE_OP(SIMD_MEMORY_STORE_LANE_OPERATION)
    FOR_EACH_BYTECODE_SIMD_STORE_LANE_OP(SIMD_MULTI_MEMORY_STORE_LANE_OPERATION)
    FOR_EACH_BYTECODE_SIMD_EXTRACT_LANE_OP(SIMD_EXTRACT_LANE_OPERATION)
    FOR_EACH_BYTECODE_SIMD_REPLACE_LANE_OP(SIMD_REPLACE_LANE_OPERATION)
    FOR_EACH_BYTECODE_ATOMIC_LOAD_OP(ATOMIC_MEMORY_LOAD_OPERATION)
    FOR_EACH_BYTECODE_ATOMIC_LOAD_OP(ATOMIC_MULTI_MEMORY_LOAD_OPERATION)
    FOR_EACH_BYTECODE_ATOMIC_STORE_OP(ATOMIC_MEMORY_STORE_OPERATION)
    FOR_EACH_BYTECODE_ATOMIC_STORE_OP(ATOMIC_MULTI_MEMORY_STORE_OPERATION)
    FOR_EACH_BYTECODE_ATOMIC_RMW_OP(ATOMIC_MEMORY_RMW_OPERATION)
    FOR_EACH_BYTECODE_ATOMIC_RMW_OP(ATOMIC_MULTI_MEMORY_RMW_OPERATION)
    FOR_EACH_BYTECODE_ATOMIC_RMW_CMPXCHG_OP(ATOMIC_MEMORY_RMW_CMPXCHG_OPERATION)
    FOR_EACH_BYTECODE_ATOMIC_RMW_CMPXCHG_OP(ATOMIC_MULTI_MEMORY_RMW_CMPXCHG_OPERATION)

    DEFINE_OPCODE(MemoryAtomicWait32)
        :
    {
        MemoryAtomicWait32* code = (MemoryAtomicWait32*)programCounter;
        int64_t timeOut = readValue<int64_t>(bp, code->src2Offset());
        uint32_t expect = readValue<uint32_t>(bp, code->src1Offset());
        uint32_t offset = readValue<uint32_t>(bp, code->src0Offset());
        uint32_t result;
        memories[0]->atomicWait(state, instance->module()->store(), offset, code->offset(), expect, timeOut, &result);
        writeValue<uint32_t>(bp, code->dstOffset(), result);
        ADD_PROGRAM_COUNTER(MemoryAtomicWait32);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(MemoryAtomicWait32Multi)
        :
    {
        MemoryAtomicWait32Multi* code = (MemoryAtomicWait32Multi*)programCounter;
        int64_t timeOut = readValue<int64_t>(bp, code->src2Offset());
        uint32_t expect = readValue<uint32_t>(bp, code->src1Offset());
        uint32_t offset = readValue<uint32_t>(bp, code->src0Offset());
        uint32_t result;
        memories[code->memIndex()]->atomicWait(state, instance->module()->store(), offset, code->offset(), expect, timeOut, &result);
        writeValue<uint32_t>(bp, code->dstOffset(), result);
        ADD_PROGRAM_COUNTER(MemoryAtomicWait32Multi);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(MemoryAtomicWait64)
        :
    {
        MemoryAtomicWait64* code = (MemoryAtomicWait64*)programCounter;
        int64_t timeOut = readValue<int64_t>(bp, code->src2Offset());
        uint64_t expect = readValue<uint64_t>(bp, code->src1Offset());
        uint32_t offset = readValue<uint32_t>(bp, code->src0Offset());
        uint32_t result;
        memories[0]->atomicWait(state, instance->module()->store(), offset, code->offset(), expect, timeOut, &result);
        writeValue<uint32_t>(bp, code->dstOffset(), result);
        ADD_PROGRAM_COUNTER(MemoryAtomicWait64);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(MemoryAtomicWait64Multi)
        :
    {
        MemoryAtomicWait64Multi* code = (MemoryAtomicWait64Multi*)programCounter;
        int64_t timeOut = readValue<int64_t>(bp, code->src2Offset());
        uint64_t expect = readValue<uint64_t>(bp, code->src1Offset());
        uint32_t offset = readValue<uint32_t>(bp, code->src0Offset());
        uint32_t result;
        memories[code->memIndex()]->atomicWait(state, instance->module()->store(), offset, code->offset(), expect, timeOut, &result);
        writeValue<uint32_t>(bp, code->dstOffset(), result);
        ADD_PROGRAM_COUNTER(MemoryAtomicWait64Multi);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(MemoryAtomicNotify)
        :
    {
        MemoryAtomicNotify* code = (MemoryAtomicNotify*)programCounter;
        uint32_t count = readValue<uint32_t>(bp, code->src1Offset());
        uint32_t offset = readValue<uint32_t>(bp, code->src0Offset());
        uint32_t result;
        memories[0]->atomicNotify(state, instance->module()->store(), offset, code->offset(), count, &result);
        writeValue<uint32_t>(bp, code->dstOffset(), result);
        ADD_PROGRAM_COUNTER(MemoryAtomicNotify);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(MemoryAtomicNotifyMulti)
        :
    {
        MemoryAtomicNotifyMulti* code = (MemoryAtomicNotifyMulti*)programCounter;
        uint32_t count = readValue<uint32_t>(bp, code->src1Offset());
        uint32_t offset = readValue<uint32_t>(bp, code->src0Offset());
        uint32_t result;
        memories[code->memIndex()]->atomicNotify(state, instance->module()->store(), offset, code->offset(), count, &result);
        writeValue<uint32_t>(bp, code->dstOffset(), result);
        ADD_PROGRAM_COUNTER(MemoryAtomicNotifyMulti);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(AtomicFence)
        :
    {
        // FIXME do nothing
        ADD_PROGRAM_COUNTER(AtomicFence);
        NEXT_INSTRUCTION();
    }

    // FOR_EACH_BYTECODE_SIMD_ETC_OP
    DEFINE_OPCODE(V128BitSelect)
        :
    {
        simdBitSelectOperation(state, (ByteCodeOffset4*)programCounter, bp);
        ADD_PROGRAM_COUNTER(V128BitSelect);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(V128Load32Zero)
        :
    {
        using Type = typename SIMDType<uint32_t>::Type;
        V128Load32Zero* code = (V128Load32Zero*)programCounter;
        uint32_t offset = readValue<uint32_t>(bp, code->srcOffset());
        uint32_t value;
        memories[0]->load(state, offset, code->offset(), &value);
        Type result;
        std::fill(std::begin(result.v), std::end(result.v), 0);
        result[0] = value;
        writeValue<Type>(bp, code->dstOffset(), result);
        ADD_PROGRAM_COUNTER(V128Load32Zero);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(V128Load32ZeroMulti)
        :
    {
        using Type = typename SIMDType<uint32_t>::Type;
        V128Load32ZeroMulti* code = (V128Load32ZeroMulti*)programCounter;
        uint32_t offset = readValue<uint32_t>(bp, code->srcOffset());
        uint32_t value;
        memories[code->memIndex()]->load(state, offset, code->offset(), &value);
        Type result;
        std::fill(std::begin(result.v), std::end(result.v), 0);
        result[0] = value;
        writeValue<Type>(bp, code->dstOffset(), result);
        ADD_PROGRAM_COUNTER(V128Load32ZeroMulti);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(V128Load64Zero)
        :
    {
        using Type = typename SIMDType<uint64_t>::Type;
        V128Load64Zero* code = (V128Load64Zero*)programCounter;
        uint32_t offset = readValue<uint32_t>(bp, code->srcOffset());
        uint64_t value;
        memories[0]->load(state, offset, code->offset(), &value);
        Type result;
        std::fill(std::begin(result.v), std::end(result.v), 0);
        result[0] = value;
        writeValue<Type>(bp, code->dstOffset(), result);
        ADD_PROGRAM_COUNTER(V128Load64Zero);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(V128Load64ZeroMulti)
        :
    {
        using Type = typename SIMDType<uint64_t>::Type;
        V128Load64ZeroMulti* code = (V128Load64ZeroMulti*)programCounter;
        uint32_t offset = readValue<uint32_t>(bp, code->srcOffset());
        uint64_t value;
        memories[code->memIndex()]->load(state, offset, code->offset(), &value);
        Type result;
        std::fill(std::begin(result.v), std::end(result.v), 0);
        result[0] = value;
        writeValue<Type>(bp, code->dstOffset(), result);
        ADD_PROGRAM_COUNTER(V128Load64ZeroMulti);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(I8X16Shuffle)
        :
    {
        using Type = typename SIMDType<uint8_t>::Type;
        I8X16Shuffle* code = (I8X16Shuffle*)programCounter;
        Type sel;
        memcpy(sel.v, code->value(), 16);
        auto lhs = readValue<Type>(bp, code->srcOffsets()[0]);
        auto rhs = readValue<Type>(bp, code->srcOffsets()[1]);
        Type result;
        for (uint8_t i = 0; i < Type::Lanes; i++) {
            result[i] = sel[i] < Type::Lanes ? lhs[sel[i]] : rhs[sel[i] - Type::Lanes];
        }
        writeValue<Type>(bp, code->dstOffset(), result);
        ADD_PROGRAM_COUNTER(I8X16Shuffle);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(MemorySize)
        :
    {
        MemorySize* code = (MemorySize*)programCounter;
        writeValue<int32_t>(bp, code->dstOffset(), memories[0]->sizeInPageSize());
        ADD_PROGRAM_COUNTER(MemorySize);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(MemorySizeMulti)
        :
    {
        MemorySizeMulti* code = (MemorySizeMulti*)programCounter;
        writeValue<int32_t>(bp, code->dstOffset(), memories[code->memIndex()]->sizeInPageSize());
        ADD_PROGRAM_COUNTER(MemorySizeMulti);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(MemoryGrow)
        :
    {
        MemoryGrow* code = (MemoryGrow*)programCounter;
        Memory* m = memories[0];
        auto oldSize = m->sizeInPageSize();
        if (m->grow(readValue<int32_t>(bp, code->srcOffset()) * (uint64_t)Memory::s_memoryPageSize)) {
            writeValue<int32_t>(bp, code->dstOffset(), oldSize);
        } else {
            writeValue<int32_t>(bp, code->dstOffset(), -1);
        }
        ADD_PROGRAM_COUNTER(MemoryGrow);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(MemoryGrowMulti)
        :
    {
        MemoryGrowMulti* code = (MemoryGrowMulti*)programCounter;
        Memory* m = memories[code->memIndex()];
        auto oldSize = m->sizeInPageSize();
        if (m->grow(readValue<int32_t>(bp, code->srcOffset()) * (uint64_t)Memory::s_memoryPageSize)) {
            writeValue<int32_t>(bp, code->dstOffset(), oldSize);
        } else {
            writeValue<int32_t>(bp, code->dstOffset(), -1);
        }
        ADD_PROGRAM_COUNTER(MemoryGrowMulti);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(MemoryInit)
        :
    {
        MemoryInit* code = (MemoryInit*)programCounter;
        Memory* m = memories[0];
        DataSegment& sg = instance->dataSegment(code->segmentIndex());
        auto dstStart = readValue<int32_t>(bp, code->srcOffsets()[0]);
        auto srcStart = readValue<int32_t>(bp, code->srcOffsets()[1]);
        auto size = readValue<int32_t>(bp, code->srcOffsets()[2]);
        m->init(state, &sg, dstStart, srcStart, size);
        ADD_PROGRAM_COUNTER(MemoryInit);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(MemoryInitMulti)
        :
    {
        MemoryInitMulti* code = (MemoryInitMulti*)programCounter;
        Memory* m = memories[code->memIndex()];
        DataSegment& sg = instance->dataSegment(code->segmentIndex());
        auto dstStart = readValue<int32_t>(bp, code->srcOffsets()[0]);
        auto srcStart = readValue<int32_t>(bp, code->srcOffsets()[1]);
        auto size = readValue<int32_t>(bp, code->srcOffsets()[2]);
        m->init(state, &sg, dstStart, srcStart, size);
        ADD_PROGRAM_COUNTER(MemoryInitMulti);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(MemoryCopy)
        :
    {
        MemoryCopy* code = (MemoryCopy*)programCounter;
        Memory* m = memories[0];
        auto dstStart = readValue<int32_t>(bp, code->srcOffsets()[0]);
        auto srcStart = readValue<int32_t>(bp, code->srcOffsets()[1]);
        auto size = readValue<int32_t>(bp, code->srcOffsets()[2]);
        m->copy(state, dstStart, srcStart, size);
        ADD_PROGRAM_COUNTER(MemoryCopy);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(MemoryCopyMulti)
        :
    {
        MemoryCopyMulti* code = (MemoryCopyMulti*)programCounter;
        Memory* srcMem = memories[code->srcMemIndex()];
        Memory* dstMem = memories[code->dstMemIndex()];
        auto dstStart = readValue<int32_t>(bp, code->srcOffsets()[0]);
        auto srcStart = readValue<int32_t>(bp, code->srcOffsets()[1]);
        auto size = readValue<int32_t>(bp, code->srcOffsets()[2]);
        srcMem->copy(state, dstStart, srcStart, size, dstMem);
        ADD_PROGRAM_COUNTER(MemoryCopyMulti);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(MemoryFill)
        :
    {
        MemoryFill* code = (MemoryFill*)programCounter;
        Memory* m = memories[0];
        auto dstStart = readValue<int32_t>(bp, code->srcOffsets()[0]);
        auto value = readValue<int32_t>(bp, code->srcOffsets()[1]);
        auto size = readValue<int32_t>(bp, code->srcOffsets()[2]);
        m->fill(state, dstStart, value, size);
        ADD_PROGRAM_COUNTER(MemoryFill);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(MemoryFillMulti)
        :
    {
        MemoryFillMulti* code = (MemoryFillMulti*)programCounter;
        Memory* m = memories[code->memIndex()];
        auto dstStart = readValue<int32_t>(bp, code->srcOffsets()[0]);
        auto value = readValue<int32_t>(bp, code->srcOffsets()[1]);
        auto size = readValue<int32_t>(bp, code->srcOffsets()[2]);
        m->fill(state, dstStart, value, size);
        ADD_PROGRAM_COUNTER(MemoryFillMulti);
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
        ASSERT(code->tableIndex() < instance->module()->numberOfTableTypes());
        Table* table = instance->m_tables[code->tableIndex()];
        void* val = table->getElement(state, readValue<uint32_t>(bp, code->srcOffset()));
        writeValue(bp, code->dstOffset(), val);

        ADD_PROGRAM_COUNTER(TableGet);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(TableSet)
        :
    {
        TableSet* code = (TableSet*)programCounter;
        ASSERT(code->tableIndex() < instance->module()->numberOfTableTypes());
        Table* table = instance->m_tables[code->tableIndex()];
        void* ptr = readValue<void*>(bp, code->src1Offset());
        table->setElement(state, readValue<uint32_t>(bp, code->src0Offset()), ptr);

        ADD_PROGRAM_COUNTER(TableSet);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(TableGrow)
        :
    {
        TableGrow* code = (TableGrow*)programCounter;
        ASSERT(code->tableIndex() < instance->module()->numberOfTableTypes());
        Table* table = instance->m_tables[code->tableIndex()];
        size_t size = table->size();

        uint64_t newSize = (uint64_t)readValue<uint32_t>(bp, code->src1Offset()) + size;
        // FIXME read reference
        void* ptr = readValue<void*>(bp, code->src0Offset());

        if (newSize <= table->maximumSize()) {
            table->grow(newSize, ptr);
            writeValue<uint32_t>(bp, code->dstOffset(), size);
        } else {
            writeValue<uint32_t>(bp, code->dstOffset(), -1);
        }

        ADD_PROGRAM_COUNTER(TableGrow);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(TableSize)
        :
    {
        TableSize* code = (TableSize*)programCounter;
        ASSERT(code->tableIndex() < instance->module()->numberOfTableTypes());
        Table* table = instance->m_tables[code->tableIndex()];
        size_t size = table->size();
        writeValue<uint32_t>(bp, code->dstOffset(), size);

        ADD_PROGRAM_COUNTER(TableSize);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(TableCopy)
        :
    {
        TableCopy* code = (TableCopy*)programCounter;
        ASSERT(code->dstIndex() < instance->module()->numberOfTableTypes());
        ASSERT(code->srcIndex() < instance->module()->numberOfTableTypes());
        Table* dstTable = instance->m_tables[code->dstIndex()];
        Table* srcTable = instance->m_tables[code->srcIndex()];

        uint32_t dstIndex = readValue<uint32_t>(bp, code->srcOffsets()[0]);
        uint32_t srcIndex = readValue<uint32_t>(bp, code->srcOffsets()[1]);
        uint32_t n = readValue<uint32_t>(bp, code->srcOffsets()[2]);

        dstTable->copy(state, srcTable, n, srcIndex, dstIndex);

        ADD_PROGRAM_COUNTER(TableCopy);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(TableFill)
        :
    {
        TableFill* code = (TableFill*)programCounter;
        ASSERT(code->tableIndex() < instance->module()->numberOfTableTypes());
        Table* table = instance->m_tables[code->tableIndex()];

        int32_t index = readValue<int32_t>(bp, code->srcOffsets()[0]);
        void* ptr = readValue<void*>(bp, code->srcOffsets()[1]);
        int32_t n = readValue<int32_t>(bp, code->srcOffsets()[2]);
        table->fill(state, n, ptr, index);

        ADD_PROGRAM_COUNTER(TableFill);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(TableInit)
        :
    {
        TableInit* code = (TableInit*)programCounter;
        ElementSegment& sg = instance->elementSegment(code->segmentIndex());

        int32_t dstStart = readValue<int32_t>(bp, code->srcOffsets()[0]);
        int32_t srcStart = readValue<int32_t>(bp, code->srcOffsets()[1]);
        int32_t size = readValue<int32_t>(bp, code->srcOffsets()[2]);

        ASSERT(code->tableIndex() < instance->module()->numberOfTableTypes());
        Table* table = instance->m_tables[code->tableIndex()];
        table->init(state, instance, &sg, dstStart, srcStart, size);
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

    DEFINE_OPCODE(RefFunc)
        :
    {
        RefFunc* code = (RefFunc*)programCounter;
        Value(instance->function(code->funcIndex())).writeToMemory(code->dstOffset() + bp);

        ADD_PROGRAM_COUNTER(RefFunc);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Throw)
        :
    {
        Throw* code = (Throw*)programCounter;
        Tag* tag = instance->tag(code->tagIndex());
        Vector<uint8_t> userExceptionData;
        size_t sz = tag->functionType()->paramStackSize();
        userExceptionData.resizeWithUninitializedValues(sz);

        uint8_t* ptr = userExceptionData.data();
        auto& param = tag->functionType()->param();
        for (size_t i = 0; i < param.size(); i++) {
            auto sz = valueStackAllocatedSize(param[i]);
            memcpy(ptr, bp + code->dataOffsets()[i], sz);
            ptr += sz;
        }
        Trap::throwException(state, tag, std::move(userExceptionData));
        ASSERT_NOT_REACHED();
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Unreachable)
        :
    {
        Trap::throwException(state, "unreachable executed");
        ASSERT_NOT_REACHED();
        NEXT_INSTRUCTION();
    }

#if !defined(NDEBUG)
    DEFINE_OPCODE(Nop)
        :
    {
        ADD_PROGRAM_COUNTER(Nop);
        NEXT_INSTRUCTION();
    }
#endif /* !NDEBUG */

    DEFINE_OPCODE(End)
        :
    {
        End* code = (End*)programCounter;
        return code->resultOffsets();
    }
#if defined(WALRUS_ENABLE_COMPUTED_GOTO)
    DEFINE_OPCODE(FillOpcodeTable)
        :
    {
#if defined(COMPILER_GCC) && __GNUC__ >= 9
        __attribute__((cold));
#endif
#if !defined(WALRUS_COMPUTED_GOTO_INTERPRETER_INIT_WITH_NULL)
        asm volatile("FillByteCodeOpcodeTableAsmLbl:");
#endif

#define REGISTER_TABLE(name, ...) \
    g_byteCodeTable.m_addressTable[ByteCode::name##Opcode] = &&name##OpcodeLbl;
        FOR_EACH_BYTECODE(REGISTER_TABLE)
#undef REGISTER_TABLE
        initAddressToOpcodeTable();
        return nullptr;
    }
#endif
    DEFINE_DEFAULT

    return nullptr;
}

NEVER_INLINE void Interpreter::callOperation(
    ExecutionState& state,
    size_t& programCounter,
    uint8_t* bp,
    Instance* instance)
{
    Call* code = (Call*)programCounter;
    Function* target = instance->function(code->index());
    target->interpreterCall(state, bp, code->stackOffsets(), code->parameterOffsetsSize(), code->resultOffsetsSize());
    programCounter += ByteCode::pointerAlignedSize(sizeof(Call) + sizeof(ByteCodeStackOffset) * code->parameterOffsetsSize()
                                                   + sizeof(ByteCodeStackOffset) * code->resultOffsetsSize());
}

NEVER_INLINE void Interpreter::callIndirectOperation(
    ExecutionState& state,
    size_t& programCounter,
    uint8_t* bp,
    Instance* instance)
{
    CallIndirect* code = (CallIndirect*)programCounter;
    Table* table = instance->table(code->tableIndex());

    uint32_t idx = readValue<uint32_t>(bp, code->calleeOffset());
    if (idx >= table->size()) {
        Trap::throwException(state, "undefined element");
    }
    auto target = reinterpret_cast<Function*>(table->uncheckedGetElement(idx));
    if (UNLIKELY(Value::isNull(target))) {
        Trap::throwException(state, "uninitialized element " + std::to_string(idx));
    }
    const FunctionType* ft = target->functionType();
    if (!ft->equals(code->functionType())) {
        Trap::throwException(state, "indirect call type mismatch");
    }

    target->interpreterCall(state, bp, code->stackOffsets(), code->parameterOffsetsSize(), code->resultOffsetsSize());
    programCounter += ByteCode::pointerAlignedSize(sizeof(CallIndirect) + sizeof(ByteCodeStackOffset) * code->parameterOffsetsSize()
                                                   + sizeof(ByteCodeStackOffset) * code->resultOffsetsSize());
}
} // namespace Walrus
