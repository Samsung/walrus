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
#include "runtime/Global.h"
#include "runtime/Module.h"
#include "runtime/Trap.h"
#include "runtime/Tag.h"
#include "util/MathOperation.h"

#if defined(WALRUS_ENABLE_COMPUTED_GOTO)
extern char FillByteCodeOpcodeTableAsmLbl[];
const void* FillByteCodeOpcodeAddress[] = { &FillByteCodeOpcodeTableAsmLbl[0] };
#endif

namespace Walrus {

OpcodeTable g_opcodeTable;

OpcodeTable::OpcodeTable()
{
#if defined(WALRUS_ENABLE_COMPUTED_GOTO)
    // Dummy bytecode execution to initialize the OpcodeTable.
    ExecutionState dummyState;
    ByteCode b;
    b.m_opcodeInAddress = const_cast<void*>(FillByteCodeOpcodeAddress[0]);
    size_t pc = reinterpret_cast<size_t>(&b);
    Interpreter::interpret(dummyState, pc, nullptr, nullptr, nullptr, nullptr, nullptr);
#endif
}

uint32_t* Interpreter::interpret(ExecutionState& state,
                                 uint8_t* bp)
{
    DefinedFunction* df = state.currentFunction()->asDefinedFunction();
    ModuleFunction* mf = df->moduleFunction();
    size_t programCounter = reinterpret_cast<size_t>(mf->byteCode());
    Instance* instance = df->instance();
    while (true) {
        try {
            return interpret(state, programCounter, bp, instance, instance->m_memory.data(), instance->m_table.data(), instance->m_global.data());
        } catch (std::unique_ptr<Exception>& e) {
            for (size_t i = e->m_programCounterInfo.size(); i > 0; i--) {
                if (e->m_programCounterInfo[i - 1].first == &state) {
                    programCounter = e->m_programCounterInfo[i - 1].second;
                    break;
                }
            }
            if (e->isUserException()) {
                bool isCatchSucessful = false;
                Tag* tag = e->tag().value();
                size_t offset = programCounter - reinterpret_cast<size_t>(mf->byteCode());
                for (const auto& item : mf->catchInfo()) {
                    if (item.m_tryStart <= offset && offset < item.m_tryEnd) {
                        if (item.m_tagIndex == std::numeric_limits<uint32_t>::max() || state.currentFunction()->asDefinedFunction()->instance()->tag(item.m_tagIndex) == tag) {
                            programCounter = item.m_catchStartPosition + reinterpret_cast<size_t>(mf->byteCode());
                            uint8_t* sp = bp + item.m_stackSizeToBe;
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

template <typename T>
ALWAYS_INLINE void writeValue(uint8_t* bp, uint32_t offset, const T& v)
{
    *reinterpret_cast<T*>(bp + offset) = v;
}

template <typename T>
ALWAYS_INLINE T readValue(uint8_t* bp, uint32_t offset)
{
    return *reinterpret_cast<T*>(bp + offset);
}

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
T intClz(ExecutionState& state, T val) { return clz(val); }
template <typename T>
T intCtz(ExecutionState& state, T val) { return ctz(val); }
template <typename T>
T intPopcnt(ExecutionState& state, T val) { return popCount(val); }
template <typename T>
T intNot(ExecutionState& state, T val) { return ~val; }
template <typename T>
T intNeg(ExecutionState& state, T val) { return ~val + 1; }
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

uint32_t* Interpreter::interpret(ExecutionState& state,
                                 size_t programCounter,
                                 uint8_t* bp,
                                 Instance* instance,
                                 Memory** memories,
                                 Table** tables,
                                 Global** globals)
{
    state.m_programCounterPointer = &programCounter;

#define ADD_PROGRAM_COUNTER(codeName) programCounter += sizeof(codeName);

#define BINARY_OPERATION(nativeParameterTypeName, nativeReturnTypeName, wasmTypeName, operationName, byteCodeOperationName) \
    DEFINE_OPCODE(wasmTypeName##byteCodeOperationName)                                                                      \
        :                                                                                                                   \
    {                                                                                                                       \
        BinaryOperation* code = (BinaryOperation*)programCounter;                                                           \
        auto lhs = readValue<nativeParameterTypeName>(bp, code->srcOffset()[0]);                                            \
        auto rhs = readValue<nativeParameterTypeName>(bp, code->srcOffset()[1]);                                            \
        writeValue<nativeReturnTypeName>(bp, code->dstOffset(), operationName(state, lhs, rhs));                            \
        ADD_PROGRAM_COUNTER(BinaryOperation);                                                                               \
        NEXT_INSTRUCTION();                                                                                                 \
    }

#define UNARY_OPERATION(nativeParameterTypeName, nativeReturnTypeName, wasmTypeName, operationName, byteCodeOperationName)                 \
    DEFINE_OPCODE(wasmTypeName##byteCodeOperationName)                                                                                     \
        :                                                                                                                                  \
    {                                                                                                                                      \
        UnaryOperation* code = (UnaryOperation*)programCounter;                                                                            \
        writeValue<nativeReturnTypeName>(bp, code->dstOffset(), operationName(readValue<nativeParameterTypeName>(bp, code->srcOffset()))); \
        ADD_PROGRAM_COUNTER(UnaryOperation);                                                                                               \
        NEXT_INSTRUCTION();                                                                                                                \
    }

#define UNARY_OPERATION_OPERATION_TEMPLATE_2(nativeParameterTypeName, nativeReturnTypeName, wasmTypeName, operationName, T1, T2, byteCodeOperationName)   \
    DEFINE_OPCODE(wasmTypeName##byteCodeOperationName)                                                                                                    \
        :                                                                                                                                                 \
    {                                                                                                                                                     \
        UnaryOperation* code = (UnaryOperation*)programCounter;                                                                                           \
        writeValue<nativeReturnTypeName>(bp, code->dstOffset(), operationName<T1, T2>(state, readValue<nativeParameterTypeName>(bp, code->srcOffset()))); \
        ADD_PROGRAM_COUNTER(UnaryOperation);                                                                                                              \
        NEXT_INSTRUCTION();                                                                                                                               \
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
        uint32_t offset = readValue<uint32_t>(bp, code->srcOffset());              \
        nativeReadTypeName value;                                                  \
        memories[0]->load(state, offset, code->offset(), &value);                  \
        writeValue<nativeWriteTypeName>(bp, code->dstOffset(), value);             \
        ADD_PROGRAM_COUNTER(MemoryLoad);                                           \
        NEXT_INSTRUCTION();                                                        \
    }

#define MEMORY_STORE_OPERATION(opcodeName, nativeReadTypeName, nativeWriteTypeName)        \
    DEFINE_OPCODE(opcodeName)                                                              \
        :                                                                                  \
    {                                                                                      \
        MemoryStore* code = (MemoryStore*)programCounter;                                  \
        nativeWriteTypeName value = readValue<nativeReadTypeName>(bp, code->src1Offset()); \
        uint32_t offset = readValue<uint32_t>(bp, code->src0Offset());                     \
        memories[0]->store(state, offset, code->offset(), value);                          \
        ADD_PROGRAM_COUNTER(MemoryStore);                                                  \
        NEXT_INSTRUCTION();                                                                \
    }


#if defined(WALRUS_ENABLE_COMPUTED_GOTO)
#define DEFINE_OPCODE(codeName) codeName##OpcodeLbl
#define DEFINE_DEFAULT
#define NEXT_INSTRUCTION() goto NextInstruction;

NextInstruction:
    /* Execute first instruction. */
    goto*(((ByteCode*)programCounter)->m_opcodeInAddress);
#else

#define DEFINE_OPCODE(codeName) case codeName##Opcode
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

    DEFINE_OPCODE(Move32)
        :
    {
        Move32* code = (Move32*)programCounter;
        *reinterpret_cast<uint32_t*>(bp + code->dstOffset()) = *reinterpret_cast<uint32_t*>(bp + code->srcOffset());
        ADD_PROGRAM_COUNTER(Move32);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Move64)
        :
    {
        Move64* code = (Move64*)programCounter;
        *reinterpret_cast<uint64_t*>(bp + code->dstOffset()) = *reinterpret_cast<uint64_t*>(bp + code->srcOffset());
        ADD_PROGRAM_COUNTER(Move64);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Load32)
        :
    {
        Load32* code = (Load32*)programCounter;
        uint32_t offset = readValue<uint32_t>(bp, code->srcOffset());
        memories[0]->load(state, offset, reinterpret_cast<uint32_t*>(bp + code->dstOffset()));
        ADD_PROGRAM_COUNTER(Load32);
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
        ASSERT(code->index() < instance->m_global.size());
        globals[code->index()]->value().writeNBytesToMemory<4>(bp + code->dstOffset());
        ADD_PROGRAM_COUNTER(GlobalGet32);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(GlobalGet64)
        :
    {
        GlobalGet64* code = (GlobalGet64*)programCounter;
        ASSERT(code->index() < instance->m_global.size());
        globals[code->index()]->value().writeNBytesToMemory<8>(bp + code->dstOffset());
        ADD_PROGRAM_COUNTER(GlobalGet64);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(GlobalSet32)
        :
    {
        GlobalSet32* code = (GlobalSet32*)programCounter;
        ASSERT(code->index() < instance->m_global.size());
        Value& val = globals[code->index()]->value();
        val.readFromStack<4>(bp + code->srcOffset());
        ADD_PROGRAM_COUNTER(GlobalSet32);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(GlobalSet64)
        :
    {
        GlobalSet64* code = (GlobalSet64*)programCounter;
        ASSERT(code->index() < instance->m_global.size());
        Value& val = globals[code->index()]->value();
        val.readFromStack<8>(bp + code->srcOffset());
        ADD_PROGRAM_COUNTER(GlobalSet64);
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
        MemorySize* code = (MemorySize*)programCounter;
        writeValue<int32_t>(bp, code->dstOffset(), memories[0]->sizeInPageSize());
        ADD_PROGRAM_COUNTER(MemorySize);
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
        ASSERT(code->tableIndex() < instance->m_table.size());
        Table* table = tables[code->tableIndex()];
        void* val = table->getElement(state, readValue<uint32_t>(bp, code->srcOffset()));
        writeValue(bp, code->dstOffset(), val);

        ADD_PROGRAM_COUNTER(TableGet);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(TableSet)
        :
    {
        TableSet* code = (TableSet*)programCounter;
        ASSERT(code->tableIndex() < instance->m_table.size());
        Table* table = tables[code->tableIndex()];
        void* ptr = readValue<void*>(bp, code->src1Offset());
        table->setElement(state, readValue<uint32_t>(bp, code->src0Offset()), ptr);

        ADD_PROGRAM_COUNTER(TableSet);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(TableGrow)
        :
    {
        TableGrow* code = (TableGrow*)programCounter;
        ASSERT(code->tableIndex() < instance->m_table.size());
        Table* table = tables[code->tableIndex()];
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
        ASSERT(code->tableIndex() < instance->m_table.size());
        Table* table = tables[code->tableIndex()];
        size_t size = table->size();
        writeValue<uint32_t>(bp, code->dstOffset(), size);

        ADD_PROGRAM_COUNTER(TableSize);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(TableCopy)
        :
    {
        TableCopy* code = (TableCopy*)programCounter;
        ASSERT(code->dstIndex() < instance->m_table.size());
        ASSERT(code->srcIndex() < instance->m_table.size());
        Table* dstTable = tables[code->dstIndex()];
        Table* srcTable = tables[code->srcIndex()];

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
        ASSERT(code->tableIndex() < instance->m_table.size());
        Table* table = tables[code->tableIndex()];

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

        ASSERT(code->tableIndex() < instance->m_table.size());
        Table* table = tables[code->tableIndex()];
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

    DEFINE_OPCODE(RefNull)
        :
    {
        RefNull* code = (RefNull*)programCounter;
        Value(code->type(), Value::Null).writeToMemory(code->dstOffset() + bp);

        ADD_PROGRAM_COUNTER(RefNull);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(RefIsNull)
        :
    {
        RefIsNull* code = (RefIsNull*)programCounter;

        Value val(reinterpret_cast<Function*>(readValue<void*>(bp, code->srcOffset())));
        writeValue(bp, code->dstOffset(), (int32_t)val.isNull());

        ADD_PROGRAM_COUNTER(RefIsNull);
        NEXT_INSTRUCTION();
    }

    DEFINE_OPCODE(Throw)
        :
    {
        Throw* code = (Throw*)programCounter;
        Tag* tag = instance->tag(code->tagIndex());
        Vector<uint8_t, GCUtil::gc_malloc_atomic_allocator<uint8_t>> userExceptionData;
        size_t sz = tag->functionType()->paramStackSize();
        userExceptionData.resizeWithUninitializedValues(sz);

        uint8_t* ptr = userExceptionData.data();
        auto& param = tag->functionType()->param();
        for (size_t i = 0; i < param.size(); i++) {
            auto sz = valueSizeInStack(param[i]);
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

    DEFINE_OPCODE(End)
        :
    {
        End* code = (End*)programCounter;
        return code->resultOffsets();
    }

    DEFINE_OPCODE(FillOpcodeTable)
        :
    {
#if defined(COMPILER_GCC) && __GNUC__ >= 9
        __attribute__((cold));
#endif
#if defined(WALRUS_ENABLE_COMPUTED_GOTO)
        asm volatile("FillByteCodeOpcodeTableAsmLbl:");
#define REGISTER_TABLE(rtype, type1, type2, type3, memSize, prefix, code, name, text, decomp) \
    g_opcodeTable.m_addressTable[OpcodeKind::name##Opcode] = &&name##OpcodeLbl;
        FOR_EACH_USED_OPCODE(REGISTER_TABLE)
#undef REGISTER_TABLE
#endif
        return nullptr;
    }

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
    const FunctionType* ft = target->functionType();
    const ValueTypeVector& param = ft->param();
    Value* paramVector = ALLOCA(sizeof(Value) * param.size(), Value);

    size_t c = 0;
    for (size_t i = 0; i < param.size(); i++) {
        paramVector[i] = Value(param[i], bp + code->stackOffsets()[c++]);
    }

    const ValueTypeVector& result = ft->result();
    Value* resultVector = ALLOCA(sizeof(Value) * result.size(), Value);
    size_t codeExtraOffsetsSize = sizeof(uint32_t) * ft->param().size() + sizeof(uint32_t) * ft->result().size();

    target->call(state, param.size(), paramVector, resultVector);

    for (size_t i = 0; i < result.size(); i++) {
        uint8_t* resultStackPointer = bp + code->stackOffsets()[c++];
        resultVector[i].writeToMemory(resultStackPointer);
    }

    programCounter += sizeof(Call) + codeExtraOffsetsSize;
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
    const ValueTypeVector& param = ft->param();
    Value* paramVector = ALLOCA(sizeof(Value) * param.size(), Value);

    size_t c = 0;
    for (size_t i = 0; i < param.size(); i++) {
        paramVector[i] = Value(param[i], bp + code->stackOffsets()[c++]);
    }

    const ValueTypeVector& result = ft->result();
    Value* resultVector = ALLOCA(sizeof(Value) * result.size(), Value);
    size_t codeExtraOffsetsSize = sizeof(uint32_t) * ft->param().size() + sizeof(uint32_t) * ft->result().size();

    target->call(state, param.size(), paramVector, resultVector);

    for (size_t i = 0; i < result.size(); i++) {
        uint8_t* resultStackPointer = bp + code->stackOffsets()[c++];
        resultVector[i].writeToMemory(resultStackPointer);
    }

    programCounter += sizeof(CallIndirect) + codeExtraOffsetsSize;
}

} // namespace Walrus
