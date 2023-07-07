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

#ifndef __WalrusByteCode__
#define __WalrusByteCode__

#if !defined(NDEBUG)
#include <cinttypes>
#include "runtime/Module.h"

#define DUMP_BYTECODE_OFFSET(name) \
    printf(#name ": %" PRIu32 " ", (uint32_t)m_##name);
#endif

namespace Walrus {

typedef uint16_t ByteCodeStackOffset;
class FunctionType;

#define FOR_EACH_BYTECODE_OP(F) \
    F(Unreachable)              \
    F(Throw)                    \
    F(End)                      \
    F(BrTable)                  \
    F(Call)                     \
    F(CallIndirect)             \
    F(Select)                   \
    F(MemorySize)               \
    F(MemoryGrow)               \
    F(MemoryInit)               \
    F(DataDrop)                 \
    F(MemoryCopy)               \
    F(MemoryFill)               \
    F(TableInit)                \
    F(ElemDrop)                 \
    F(TableCopy)                \
    F(TableGet)                 \
    F(TableSet)                 \
    F(TableGrow)                \
    F(TableSize)                \
    F(TableFill)                \
    F(RefFunc)                  \
    F(Move32)                   \
    F(Move64)                   \
    F(Move128)                  \
    F(Jump)                     \
    F(JumpIfTrue)               \
    F(JumpIfFalse)              \
    F(GlobalGet32)              \
    F(GlobalGet64)              \
    F(GlobalGet128)             \
    F(GlobalSet32)              \
    F(GlobalSet64)              \
    F(GlobalSet128)             \
    F(Const32)                  \
    F(Const64)                  \
    F(Const128)                 \
    F(Load32)                   \
    F(Load64)                   \
    F(Store32)                  \
    F(Store64)                  \
    F(FillOpcodeTable)

#define FOR_EACH_BYTECODE_BINARY_OP(F)            \
    F(I32Add, add, int32_t, int32_t)              \
    F(I32Sub, sub, int32_t, int32_t)              \
    F(I32Mul, mul, int32_t, int32_t)              \
    F(I32DivS, intDiv, int32_t, int32_t)          \
    F(I32DivU, intDiv, uint32_t, uint32_t)        \
    F(I32RemS, intRem, int32_t, int32_t)          \
    F(I32RemU, intRem, uint32_t, uint32_t)        \
    F(I32And, intAnd, int32_t, int32_t)           \
    F(I32Or, intOr, int32_t, int32_t)             \
    F(I32Xor, intXor, int32_t, int32_t)           \
    F(I32Shl, intShl, int32_t, int32_t)           \
    F(I32ShrS, intShr, int32_t, int32_t)          \
    F(I32ShrU, intShr, uint32_t, uint32_t)        \
    F(I32Rotl, intRotl, uint32_t, uint32_t)       \
    F(I32Rotr, intRotr, uint32_t, uint32_t)       \
    F(I32Eq, eq, int32_t, int32_t)                \
    F(I32Ne, ne, int32_t, int32_t)                \
    F(I32LtS, lt, int32_t, int32_t)               \
    F(I32LtU, lt, uint32_t, uint32_t)             \
    F(I32LeS, le, int32_t, int32_t)               \
    F(I32LeU, le, uint32_t, uint32_t)             \
    F(I32GtS, gt, int32_t, int32_t)               \
    F(I32GtU, gt, uint32_t, uint32_t)             \
    F(I32GeS, ge, int32_t, int32_t)               \
    F(I32GeU, ge, uint32_t, uint32_t)             \
    F(F32Add, add, float, float)                  \
    F(F32Sub, sub, float, float)                  \
    F(F32Mul, mul, float, float)                  \
    F(F32Div, floatDiv, float, float)             \
    F(F32Max, floatMax, float, float)             \
    F(F32Min, floatMin, float, float)             \
    F(F32Copysign, floatCopysign, float, float)   \
    F(F32Eq, eq, float, int32_t)                  \
    F(F32Ne, ne, float, int32_t)                  \
    F(F32Lt, lt, float, int32_t)                  \
    F(F32Le, le, float, int32_t)                  \
    F(F32Gt, gt, float, int32_t)                  \
    F(F32Ge, ge, float, int32_t)                  \
    F(I64Add, add, int64_t, int64_t)              \
    F(I64Sub, sub, int64_t, int64_t)              \
    F(I64Mul, mul, int64_t, int64_t)              \
    F(I64DivS, intDiv, int64_t, int64_t)          \
    F(I64DivU, intDiv, uint64_t, uint64_t)        \
    F(I64RemS, intRem, int64_t, int64_t)          \
    F(I64RemU, intRem, uint64_t, uint64_t)        \
    F(I64And, intAnd, int64_t, int64_t)           \
    F(I64Or, intOr, int64_t, int64_t)             \
    F(I64Xor, intXor, int64_t, int64_t)           \
    F(I64Shl, intShl, int64_t, int64_t)           \
    F(I64ShrS, intShr, int64_t, int64_t)          \
    F(I64ShrU, intShr, uint64_t, uint64_t)        \
    F(I64Rotl, intRotl, uint64_t, uint64_t)       \
    F(I64Rotr, intRotr, uint64_t, uint64_t)       \
    F(I64Eq, eq, int64_t, int32_t)                \
    F(I64Ne, ne, int64_t, int32_t)                \
    F(I64LtS, lt, int64_t, int32_t)               \
    F(I64LtU, lt, uint64_t, uint32_t)             \
    F(I64LeS, le, int64_t, int32_t)               \
    F(I64LeU, le, uint64_t, uint32_t)             \
    F(I64GtS, gt, int64_t, int32_t)               \
    F(I64GtU, gt, uint64_t, uint32_t)             \
    F(I64GeS, ge, int64_t, int32_t)               \
    F(I64GeU, ge, uint64_t, uint32_t)             \
    F(F64Add, add, double, double)                \
    F(F64Sub, sub, double, double)                \
    F(F64Mul, mul, double, double)                \
    F(F64Div, floatDiv, double, double)           \
    F(F64Max, floatMax, double, double)           \
    F(F64Min, floatMin, double, double)           \
    F(F64Copysign, floatCopysign, double, double) \
    F(F64Eq, eq, double, int32_t)                 \
    F(F64Ne, ne, double, int32_t)                 \
    F(F64Lt, lt, double, int32_t)                 \
    F(F64Le, le, double, int32_t)                 \
    F(F64Gt, gt, double, int32_t)                 \
    F(F64Ge, ge, double, int32_t)

#define FOR_EACH_BYTECODE_UNARY_OP(F)   \
    F(I32Clz, clz, uint32_t)            \
    F(I32Ctz, ctz, uint32_t)            \
    F(I32Popcnt, popCount, uint32_t)    \
    F(I32Eqz, intEqz, uint32_t)         \
    F(F32Sqrt, floatSqrt, float)        \
    F(F32Ceil, floatCeil, float)        \
    F(F32Floor, floatFloor, float)      \
    F(F32Trunc, floatTrunc, float)      \
    F(F32Nearest, floatNearest, float)  \
    F(F32Abs, floatAbs, float)          \
    F(F32Neg, floatNeg, float)          \
    F(I64Clz, clz, uint64_t)            \
    F(I64Ctz, ctz, uint64_t)            \
    F(I64Popcnt, popCount, uint64_t)    \
    F(I64Eqz, intEqz, uint64_t)         \
    F(F64Sqrt, floatSqrt, double)       \
    F(F64Ceil, floatCeil, double)       \
    F(F64Floor, floatFloor, double)     \
    F(F64Trunc, floatTrunc, double)     \
    F(F64Nearest, floatNearest, double) \
    F(F64Abs, floatAbs, double)         \
    F(F64Neg, floatNeg, double)

#define FOR_EACH_BYTECODE_UNARY_OP_2(F)                                 \
    F(I64Extend8S, intExtend, uint64_t, uint64_t, uint64_t, 7)          \
    F(I64Extend16S, intExtend, uint64_t, uint64_t, uint64_t, 15)        \
    F(I64Extend32S, intExtend, uint64_t, uint64_t, uint64_t, 31)        \
    F(I32Extend8S, intExtend, uint32_t, uint32_t, uint32_t, 7)          \
    F(I32Extend16S, intExtend, uint32_t, uint32_t, uint32_t, 15)        \
    F(I64ExtendI32S, doConvert, int32_t, int64_t, int64_t, int32_t)     \
    F(I64ExtendI32U, doConvert, uint32_t, uint64_t, uint64_t, uint32_t) \
    F(I32WrapI64, doConvert, uint64_t, uint32_t, uint32_t, uint64_t)    \
    F(I32TruncF32S, doConvert, float, int32_t, int32_t, float)          \
    F(I32TruncF32U, doConvert, float, uint32_t, uint32_t, float)        \
    F(I32TruncF64S, doConvert, double, int32_t, int32_t, double)        \
    F(I32TruncF64U, doConvert, double, uint32_t, uint32_t, double)      \
    F(I64TruncF32S, doConvert, float, int64_t, int64_t, float)          \
    F(I64TruncF32U, doConvert, float, uint64_t, uint64_t, float)        \
    F(I64TruncF64S, doConvert, double, int64_t, int64_t, double)        \
    F(I64TruncF64U, doConvert, double, uint64_t, uint64_t, double)      \
    F(F32ConvertI32S, doConvert, int32_t, float, float, int32_t)        \
    F(F32ConvertI32U, doConvert, uint32_t, float, float, uint32_t)      \
    F(F32ConvertI64S, doConvert, int64_t, float, float, int64_t)        \
    F(F32ConvertI64U, doConvert, uint64_t, float, float, uint64_t)      \
    F(F64ConvertI32S, doConvert, int32_t, double, double, int32_t)      \
    F(F64ConvertI32U, doConvert, uint32_t, double, double, uint32_t)    \
    F(F64ConvertI64S, doConvert, int64_t, double, double, int64_t)      \
    F(F64ConvertI64U, doConvert, uint64_t, double, double, uint64_t)    \
    F(I32TruncSatF32S, intTruncSat, float, int32_t, int32_t, float)     \
    F(I32TruncSatF32U, intTruncSat, float, uint32_t, uint32_t, float)   \
    F(I32TruncSatF64S, intTruncSat, double, int32_t, int32_t, double)   \
    F(I32TruncSatF64U, intTruncSat, double, uint32_t, uint32_t, double) \
    F(I64TruncSatF32S, intTruncSat, float, int64_t, int64_t, float)     \
    F(I64TruncSatF32U, intTruncSat, float, uint64_t, uint64_t, float)   \
    F(I64TruncSatF64S, intTruncSat, double, int64_t, int64_t, double)   \
    F(I64TruncSatF64U, intTruncSat, double, uint64_t, uint64_t, double) \
    F(F64PromoteF32, doConvert, float, double, double, float)           \
    F(F32DemoteF64, doConvert, double, float, float, double)

#define FOR_EACH_BYTECODE_LOAD_OP(F) \
    F(I32Load, int32_t, int32_t)     \
    F(I32Load8S, int8_t, int32_t)    \
    F(I32Load8U, uint8_t, int32_t)   \
    F(I32Load16S, int16_t, int32_t)  \
    F(I32Load16U, uint16_t, int32_t) \
    F(I64Load, int64_t, int64_t)     \
    F(I64Load8S, int8_t, int64_t)    \
    F(I64Load8U, uint8_t, int64_t)   \
    F(I64Load16S, int16_t, int64_t)  \
    F(I64Load16U, uint16_t, int64_t) \
    F(I64Load32S, int32_t, int64_t)  \
    F(I64Load32U, uint32_t, int64_t) \
    F(F32Load, float, float)         \
    F(F64Load, double, double)       \
    F(V128Load, Vec128, Vec128)

#define FOR_EACH_BYTECODE_STORE_OP(F) \
    F(I32Store, int32_t, int32_t)     \
    F(I32Store16, int32_t, int16_t)   \
    F(I32Store8, int32_t, int8_t)     \
    F(I64Store, int64_t, int64_t)     \
    F(I64Store32, int64_t, int32_t)   \
    F(I64Store16, int64_t, int16_t)   \
    F(I64Store8, int64_t, int8_t)     \
    F(F32Store, float, float)         \
    F(F64Store, double, double)       \
    F(V128Store, Vec128, Vec128)

#define FOR_EACH_BYTECODE_SIMD_BINARY_OP(F)        \
    F(I8X16Add, add, uint8_t, uint8_t)             \
    F(I8X16AddSatS, intAddSat, int8_t, int8_t)     \
    F(I8X16AddSatU, intAddSat, uint8_t, uint8_t)   \
    F(I8X16Sub, sub, uint8_t, uint8_t)             \
    F(I8X16SubSatS, intSubSat, int8_t, int8_t)     \
    F(I8X16SubSatU, intSubSat, uint8_t, uint8_t)   \
    F(I16X8Add, add, uint16_t, uint16_t)           \
    F(I16X8AddSatS, intAddSat, int16_t, int16_t)   \
    F(I16X8AddSatU, intAddSat, uint16_t, uint16_t) \
    F(I16X8Sub, sub, uint16_t, uint16_t)           \
    F(I16X8SubSatS, intSubSat, int16_t, int16_t)   \
    F(I16X8SubSatU, intSubSat, uint16_t, uint16_t) \
    F(I16X8Mul, mul, uint16_t, uint16_t)           \
    F(I32X4Add, add, uint32_t, uint32_t)           \
    F(I32X4Sub, sub, uint32_t, uint32_t)           \
    F(I32X4Mul, mul, uint32_t, uint32_t)           \
    F(I64X2Add, add, uint64_t, uint64_t)           \
    F(I64X2Sub, sub, uint64_t, uint64_t)           \
    F(I64X2Mul, mul, uint64_t, uint64_t)           \
    F(F32X4Add, add, float, float)                 \
    F(F32X4Sub, sub, float, float)                 \
    F(F32X4Mul, mul, float, float)                 \
    F(F32X4Div, floatDiv, float, float)            \
    F(F64X2Add, add, double, double)               \
    F(F64X2Sub, sub, double, double)               \
    F(F64X2Mul, mul, double, double)               \
    F(F64X2Div, floatDiv, double, double)          \
    F(I8X16Eq, eqMask, uint8_t, uint8_t)           \
    F(I8X16Ne, neMask, uint8_t, uint8_t)           \
    F(I8X16LtS, ltMask, int8_t, int8_t)            \
    F(I8X16LtU, ltMask, uint8_t, uint8_t)          \
    F(I8X16GtS, gtMask, int8_t, int8_t)            \
    F(I8X16GtU, gtMask, uint8_t, uint8_t)          \
    F(I8X16LeS, leMask, int8_t, int8_t)            \
    F(I8X16LeU, leMask, uint8_t, uint8_t)          \
    F(I8X16GeS, geMask, int8_t, int8_t)            \
    F(I8X16GeU, geMask, uint8_t, uint8_t)          \
    F(I16X8Eq, eqMask, uint16_t, uint16_t)         \
    F(I16X8Ne, neMask, uint16_t, uint16_t)         \
    F(I16X8LtS, ltMask, int16_t, int16_t)          \
    F(I16X8LtU, ltMask, uint16_t, uint16_t)        \
    F(I16X8GtS, gtMask, int16_t, int16_t)          \
    F(I16X8GtU, gtMask, uint16_t, uint16_t)        \
    F(I16X8LeS, leMask, int16_t, int16_t)          \
    F(I16X8LeU, leMask, uint16_t, uint16_t)        \
    F(I16X8GeS, geMask, int16_t, int16_t)          \
    F(I16X8GeU, geMask, uint16_t, uint16_t)        \
    F(I32X4Eq, eqMask, uint32_t, uint32_t)         \
    F(I32X4Ne, neMask, uint32_t, uint32_t)         \
    F(I32X4LtS, ltMask, int32_t, int32_t)          \
    F(I32X4LtU, ltMask, uint32_t, uint32_t)        \
    F(I32X4GtS, gtMask, int32_t, int32_t)          \
    F(I32X4GtU, gtMask, uint32_t, uint32_t)        \
    F(I32X4LeS, leMask, int32_t, int32_t)          \
    F(I32X4LeU, leMask, uint32_t, uint32_t)        \
    F(I32X4GeS, geMask, int32_t, int32_t)          \
    F(I32X4GeU, geMask, uint32_t, uint32_t)        \
    F(I64X2Eq, eqMask, uint64_t, uint64_t)         \
    F(I64X2Ne, neMask, uint64_t, uint64_t)         \
    F(I64X2LtS, ltMask, int64_t, int64_t)          \
    F(I64X2GtS, gtMask, int64_t, int64_t)          \
    F(I64X2LeS, leMask, int64_t, int64_t)          \
    F(I64X2GeS, geMask, int64_t, int64_t)          \
    F(F32X4Eq, eqMask, float, uint32_t)            \
    F(F32X4Ne, neMask, float, uint32_t)            \
    F(F32X4Lt, ltMask, float, uint32_t)            \
    F(F32X4Gt, gtMask, float, uint32_t)            \
    F(F32X4Le, leMask, float, uint32_t)            \
    F(F32X4Ge, geMask, float, uint32_t)            \
    F(F32X4PMin, floatPMin, float, float)          \
    F(F32X4PMax, floatPMax, float, float)          \
    F(F64X2Eq, eqMask, double, uint64_t)           \
    F(F64X2Ne, neMask, double, uint64_t)           \
    F(F64X2Lt, ltMask, double, uint64_t)           \
    F(F64X2Gt, gtMask, double, uint64_t)           \
    F(F64X2Le, leMask, double, uint64_t)           \
    F(F64X2Ge, geMask, double, uint64_t)           \
    F(F64X2PMin, floatPMin, double, double)        \
    F(F64X2PMax, floatPMax, double, double)

#define FOR_EACH_BYTECODE_SIMD_UNARY_OP(F) \
    F(I8X16Neg, intNeg, uint8_t)           \
    F(I16X8Neg, intNeg, uint16_t)          \
    F(I32X4Neg, intNeg, uint32_t)          \
    F(I64X2Neg, intNeg, uint64_t)          \
    F(F32X4Neg, floatNeg, float)           \
    F(F32X4Sqrt, floatSqrt, float)         \
    F(F64X2Neg, floatNeg, double)          \
    F(F64X2Sqrt, floatSqrt, double)

#define FOR_EACH_BYTECODE_SIMD_LOAD_OP(F) \
    F(V128Load8Lane, uint8_t)             \
    F(V128Load16Lane, uint16_t)           \
    F(V128Load32Lane, uint32_t)           \
    F(V128Load64Lane, uint64_t)

#define FOR_EACH_BYTECODE_SIMD_STORE_OP(F) \
    F(V128Store8Lane, uint8_t)             \
    F(V128Store16Lane, uint16_t)           \
    F(V128Store32Lane, uint32_t)           \
    F(V128Store64Lane, uint64_t)

#define FOR_EACH_BYTECODE(F)            \
    FOR_EACH_BYTECODE_OP(F)             \
    FOR_EACH_BYTECODE_BINARY_OP(F)      \
    FOR_EACH_BYTECODE_UNARY_OP(F)       \
    FOR_EACH_BYTECODE_UNARY_OP_2(F)     \
    FOR_EACH_BYTECODE_LOAD_OP(F)        \
    FOR_EACH_BYTECODE_STORE_OP(F)       \
    FOR_EACH_BYTECODE_SIMD_BINARY_OP(F) \
    FOR_EACH_BYTECODE_SIMD_UNARY_OP(F)  \
    FOR_EACH_BYTECODE_SIMD_LOAD_OP(F)   \
    FOR_EACH_BYTECODE_SIMD_STORE_OP(F)

class ByteCode {
public:
    // clang-format off
    enum Opcode : uint32_t {
#define DECLARE_BYTECODE(name, ...) name##Opcode,
        FOR_EACH_BYTECODE(DECLARE_BYTECODE)
#undef DECLARE_BYTECODE
        OpcodeKindEnd,
    };
    // clang-format on

    Opcode opcode() const;
    size_t getSize();

protected:
    friend class Interpreter;
    friend class ByteCodeTable;
    ByteCode(Opcode opcode);

    ByteCode()
#if defined(WALRUS_ENABLE_COMPUTED_GOTO)
        : m_opcodeInAddress(nullptr)
#else
        : m_opcode(Opcode::OpcodeKindEnd)
#endif
    {
    }

    union {
        Opcode m_opcode;
        void* m_opcodeInAddress;
    };
};

class ByteCodeTable {
public:
    ByteCodeTable();
#if defined(WALRUS_ENABLE_COMPUTED_GOTO)
    void* m_addressTable[ByteCode::OpcodeKindEnd];
    std::unordered_map<void*, int> m_addressToOpcodeTable;
#endif
};

extern ByteCodeTable g_byteCodeTable;

class Const32 : public ByteCode {
public:
    Const32(ByteCodeStackOffset dstOffset, uint32_t value)
        : ByteCode(Opcode::Const32Opcode)
        , m_dstOffset(dstOffset)
        , m_value(value)
    {
    }

    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }
    void setDstOffset(ByteCodeStackOffset o) { m_dstOffset = o; }
    uint32_t value() const { return m_value; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("const32 ");
        DUMP_BYTECODE_OFFSET(dstOffset);
        printf("value: %" PRId32, m_value);
    }
#endif

protected:
    ByteCodeStackOffset m_dstOffset;
    uint32_t m_value;
};


class Const64 : public ByteCode {
public:
    Const64(ByteCodeStackOffset dstOffset, uint64_t value)
        : ByteCode(Opcode::Const64Opcode)
        , m_dstOffset(dstOffset)
        , m_value(value)
    {
    }

    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }
    void setDstOffset(ByteCodeStackOffset o) { m_dstOffset = o; }
    uint64_t value() const { return m_value; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("const64 ");
        DUMP_BYTECODE_OFFSET(dstOffset);
        printf("value: %" PRIu64, m_value);
    }
#endif

protected:
    ByteCodeStackOffset m_dstOffset;
    uint64_t m_value;
};

class Const128 : public ByteCode {
public:
    Const128(ByteCodeStackOffset dstOffset, uint8_t* value)
        : ByteCode(Opcode::Const128Opcode)
        , m_dstOffset(dstOffset)
    {
        ASSERT(!!value);
        memcpy(m_value, value, 16);
    }

    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }
    void setDstOffset(ByteCodeStackOffset o) { m_dstOffset = o; }
    const uint8_t* value() const { return m_value; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("const128 ");
        DUMP_BYTECODE_OFFSET(dstOffset);
        //printf("value: %" PRIu64, m_value);
    }
#endif

protected:
    ByteCodeStackOffset m_dstOffset;
    uint8_t m_value[16];
};

// dummy ByteCode for binary operation
class BinaryOperation : public ByteCode {
public:
    BinaryOperation(Opcode code, ByteCodeStackOffset src0Offset, ByteCodeStackOffset src1Offset, ByteCodeStackOffset dstOffset)
        : ByteCode(code)
        , m_srcOffset{ src0Offset, src1Offset }
        , m_dstOffset(dstOffset)
    {
    }

    const ByteCodeStackOffset* srcOffset() const { return m_srcOffset; }
    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }
    void setDstOffset(ByteCodeStackOffset o) { m_dstOffset = o; }
#if !defined(NDEBUG)
    void dump(size_t pos)
    {
    }
#endif

protected:
    ByteCodeStackOffset m_srcOffset[2];
    ByteCodeStackOffset m_dstOffset;
};

#if !defined(NDEBUG)
#define DEFINE_BINARY_BYTECODE_DUMP(name)                                                                                                              \
    void dump(size_t pos)                                                                                                                              \
    {                                                                                                                                                  \
        printf(#name " src1: %" PRIu32 " src2: %" PRIu32 " dst: %" PRIu32, (uint32_t)m_srcOffset[0], (uint32_t)m_srcOffset[1], (uint32_t)m_dstOffset); \
    }
#else
#define DEFINE_BINARY_BYTECODE_DUMP(name)
#endif

#define DEFINE_BINARY_BYTECODE(name, op, paramType, returnType)                                             \
    class name : public BinaryOperation {                                                                   \
    public:                                                                                                 \
        name(ByteCodeStackOffset src0Offset, ByteCodeStackOffset src1Offset, ByteCodeStackOffset dstOffset) \
            : BinaryOperation(Opcode::name##Opcode, src0Offset, src1Offset, dstOffset)                      \
        {                                                                                                   \
        }                                                                                                   \
        DEFINE_BINARY_BYTECODE_DUMP(name)                                                                   \
    };

// dummy ByteCode for unary operation
class UnaryOperation : public ByteCode {
public:
    UnaryOperation(Opcode code, ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset)
        : ByteCode(code)
        , m_srcOffset(srcOffset)
        , m_dstOffset(dstOffset)
    {
    }
    ByteCodeStackOffset srcOffset() const { return m_srcOffset; }
    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }
#if !defined(NDEBUG)
    void dump(size_t pos)
    {
    }
#endif

protected:
    ByteCodeStackOffset m_srcOffset;
    ByteCodeStackOffset m_dstOffset;
};

#if !defined(NDEBUG)
#define DEFINE_UNARY_BYTECODE_DUMP(name)                                                               \
    void dump(size_t pos)                                                                              \
    {                                                                                                  \
        printf(#name " src: %" PRIu32 " dst: %" PRIu32, (uint32_t)m_srcOffset, (uint32_t)m_dstOffset); \
    }
#else
#define DEFINE_UNARY_BYTECODE_DUMP(name)
#endif

#define DEFINE_UNARY_BYTECODE(name, ...)                                   \
    class name : public UnaryOperation {                                   \
    public:                                                                \
        name(ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset) \
            : UnaryOperation(Opcode::name##Opcode, srcOffset, dstOffset)   \
        {                                                                  \
        }                                                                  \
        DEFINE_UNARY_BYTECODE_DUMP(name)                                   \
    };

FOR_EACH_BYTECODE_BINARY_OP(DEFINE_BINARY_BYTECODE)
FOR_EACH_BYTECODE_UNARY_OP(DEFINE_UNARY_BYTECODE)
FOR_EACH_BYTECODE_UNARY_OP_2(DEFINE_UNARY_BYTECODE)
FOR_EACH_BYTECODE_SIMD_BINARY_OP(DEFINE_BINARY_BYTECODE)
FOR_EACH_BYTECODE_SIMD_UNARY_OP(DEFINE_UNARY_BYTECODE)
#undef DEFINE_BINARY_BYTECODE_DUMP
#undef DEFINE_BINARY_BYTECODE
#undef DEFINE_UNARY_BYTECODE_DUMP
#undef DEFINE_UNARY_BYTECODE

class Call : public ByteCode {
public:
    Call(uint32_t index, uint32_t offsetsSize
#if !defined(NDEBUG)
         ,
         FunctionType* functionType
#endif
         )
        : ByteCode(Opcode::CallOpcode)
        , m_index(index)
        , m_offsetsSize(offsetsSize)
#if !defined(NDEBUG)
        , m_functionType(functionType)
#endif
    {
    }

    uint32_t index() const { return m_index; }
    ByteCodeStackOffset* stackOffsets() const
    {
        return reinterpret_cast<ByteCodeStackOffset*>(reinterpret_cast<size_t>(this) + sizeof(Call));
    }

    uint32_t offsetsSize()
    {
        return m_offsetsSize;
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("call ");
        printf("index: %" PRId32 " ", m_index);
        size_t c = 0;
        auto arr = stackOffsets();
        printf("paramOffsets: ");
        for (size_t i = 0; i < m_functionType->param().size(); i++) {
            printf("%" PRIu32 " ", (uint32_t)arr[c++]);
        }
        printf(" ");

        printf("resultOffsets: ");
        for (size_t i = 0; i < m_functionType->result().size(); i++) {
            printf("%" PRIu32 " ", (uint32_t)arr[c++]);
        }
        printf(" ");
    }
#endif

protected:
    uint32_t m_index;
    uint32_t m_offsetsSize;
#if !defined(NDEBUG)
    FunctionType* m_functionType;
#endif
};

class CallIndirect : public ByteCode {
public:
    CallIndirect(ByteCodeStackOffset stackOffset, uint32_t tableIndex, FunctionType* functionType)
        : ByteCode(Opcode::CallIndirectOpcode)
        , m_calleeOffset(stackOffset)
        , m_tableIndex(tableIndex)
        , m_functionType(functionType)
    {
    }

    ByteCodeStackOffset calleeOffset() const { return m_calleeOffset; }
    uint32_t tableIndex() const { return m_tableIndex; }
    FunctionType* functionType() const { return m_functionType; }
    ByteCodeStackOffset* stackOffsets() const
    {
        return reinterpret_cast<ByteCodeStackOffset*>(reinterpret_cast<size_t>(this) + sizeof(CallIndirect));
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("call_indirect ");
        printf("tableIndex: %" PRId32 " ", m_tableIndex);
        DUMP_BYTECODE_OFFSET(calleeOffset);

        size_t c = 0;
        auto arr = stackOffsets();
        printf("paramOffsets: ");
        for (size_t i = 0; i < m_functionType->param().size(); i++) {
            printf("%" PRIu32 " ", (uint32_t)arr[c++]);
        }
        printf(" ");

        printf("resultOffsets: ");
        for (size_t i = 0; i < m_functionType->result().size(); i++) {
            printf("%" PRIu32 " ", (uint32_t)arr[c++]);
        }
        printf(" ");
    }
#endif

protected:
    ByteCodeStackOffset m_calleeOffset;
    uint32_t m_tableIndex;
    FunctionType* m_functionType;
};

class Move32 : public ByteCode {
public:
    Move32(ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset)
        : ByteCode(Opcode::Move32Opcode)
        , m_srcOffset(srcOffset)
        , m_dstOffset(dstOffset)
    {
    }

    ByteCodeStackOffset srcOffset() const { return m_srcOffset; }
    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("move32 ");
        DUMP_BYTECODE_OFFSET(srcOffset);
        DUMP_BYTECODE_OFFSET(dstOffset);
    }
#endif

protected:
    ByteCodeStackOffset m_srcOffset;
    ByteCodeStackOffset m_dstOffset;
};

class Move64 : public ByteCode {
public:
    Move64(ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset)
        : ByteCode(Opcode::Move64Opcode)
        , m_srcOffset(srcOffset)
        , m_dstOffset(dstOffset)
    {
    }

    ByteCodeStackOffset srcOffset() const { return m_srcOffset; }
    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("move64 ");
        DUMP_BYTECODE_OFFSET(srcOffset);
        DUMP_BYTECODE_OFFSET(dstOffset);
    }
#endif

protected:
    ByteCodeStackOffset m_srcOffset;
    ByteCodeStackOffset m_dstOffset;
};

class Move128 : public ByteCode {
public:
    Move128(ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset)
        : ByteCode(Opcode::Move128Opcode)
        , m_srcOffset(srcOffset)
        , m_dstOffset(dstOffset)
    {
    }

    ByteCodeStackOffset srcOffset() const { return m_srcOffset; }
    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("move128 ");
        DUMP_BYTECODE_OFFSET(srcOffset);
        DUMP_BYTECODE_OFFSET(dstOffset);
    }
#endif

protected:
    ByteCodeStackOffset m_srcOffset;
    ByteCodeStackOffset m_dstOffset;
};

class Load32 : public ByteCode {
public:
    Load32(ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset)
        : ByteCode(Load32Opcode)
        , m_srcOffset(srcOffset)
        , m_dstOffset(dstOffset)
    {
    }

    ByteCodeStackOffset srcOffset() const { return m_srcOffset; }
    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("load32 ");
        DUMP_BYTECODE_OFFSET(srcOffset);
        DUMP_BYTECODE_OFFSET(dstOffset);
    }
#endif

protected:
    ByteCodeStackOffset m_srcOffset;
    ByteCodeStackOffset m_dstOffset;
};

class Load64 : public ByteCode {
public:
    Load64(ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset)
        : ByteCode(Load64Opcode)
        , m_srcOffset(srcOffset)
        , m_dstOffset(dstOffset)
    {
    }

    ByteCodeStackOffset srcOffset() const { return m_srcOffset; }
    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("load64 ");
        DUMP_BYTECODE_OFFSET(srcOffset);
        DUMP_BYTECODE_OFFSET(dstOffset);
    }
#endif

protected:
    ByteCodeStackOffset m_srcOffset;
    ByteCodeStackOffset m_dstOffset;
};

class Store32 : public ByteCode {
public:
    Store32(ByteCodeStackOffset src0, ByteCodeStackOffset src1)
        : ByteCode(Store32Opcode)
        , m_src0Offset(src0)
        , m_src1Offset(src1)
    {
    }

    ByteCodeStackOffset src0Offset() const { return m_src0Offset; }
    ByteCodeStackOffset src1Offset() const { return m_src1Offset; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("store32 ");
        DUMP_BYTECODE_OFFSET(src0Offset);
        DUMP_BYTECODE_OFFSET(src1Offset);
    }
#endif

protected:
    ByteCodeStackOffset m_src0Offset;
    ByteCodeStackOffset m_src1Offset;
};

class Store64 : public ByteCode {
public:
    Store64(ByteCodeStackOffset src0, ByteCodeStackOffset src1)
        : ByteCode(Store64Opcode)
        , m_src0Offset(src0)
        , m_src1Offset(src1)
    {
    }

    ByteCodeStackOffset src0Offset() const { return m_src0Offset; }
    ByteCodeStackOffset src1Offset() const { return m_src1Offset; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("store64 ");
        DUMP_BYTECODE_OFFSET(src0Offset);
        DUMP_BYTECODE_OFFSET(src1Offset);
    }
#endif

protected:
    ByteCodeStackOffset m_src0Offset;
    ByteCodeStackOffset m_src1Offset;
};

class Jump : public ByteCode {
public:
    Jump(int32_t offset = 0)
        : ByteCode(Opcode::JumpOpcode)
        , m_offset(offset)
    {
    }

    int32_t offset() const { return m_offset; }
    void setOffset(int32_t offset)
    {
        m_offset = offset;
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("jump dst: %" PRId32, (int32_t)pos + m_offset);
    }
#endif

protected:
    uint32_t m_offset;
};

class JumpIfTrue : public ByteCode {
public:
    JumpIfTrue(ByteCodeStackOffset srcOffset, int32_t offset = 0)
        : ByteCode(Opcode::JumpIfTrueOpcode)
        , m_srcOffset(srcOffset)
        , m_offset(offset)
    {
    }

    ByteCodeStackOffset srcOffset() const { return m_srcOffset; }
    int32_t offset() const { return m_offset; }
    void setOffset(int32_t offset)
    {
        m_offset = offset;
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("jump_if_true ");
        DUMP_BYTECODE_OFFSET(srcOffset);
        printf("dst: %" PRId32, (int32_t)pos + m_offset);
    }
#endif

protected:
    ByteCodeStackOffset m_srcOffset;
    int32_t m_offset;
};

class JumpIfFalse : public ByteCode {
public:
    JumpIfFalse(ByteCodeStackOffset srcOffset, int32_t offset = 0)
        : ByteCode(Opcode::JumpIfFalseOpcode)
        , m_srcOffset(srcOffset)
        , m_offset(offset)
    {
    }

    ByteCodeStackOffset srcOffset() const { return m_srcOffset; }
    int32_t offset() const { return m_offset; }
    void setOffset(int32_t offset)
    {
        m_offset = offset;
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("jump_if_false ");
        DUMP_BYTECODE_OFFSET(srcOffset);
        printf("dst: %" PRId32, (int32_t)pos + m_offset);
    }
#endif

protected:
    ByteCodeStackOffset m_srcOffset;
    int32_t m_offset;
};

class Select : public ByteCode {
public:
    Select(ByteCodeStackOffset condOffset, uint16_t size, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset dst)
        : ByteCode(Opcode::SelectOpcode)
        , m_condOffset(condOffset)
        , m_valueSize(size)
        , m_src0Offset(src0)
        , m_src1Offset(src1)
        , m_dstOffset(dst)
    {
    }

    ByteCodeStackOffset condOffset() const { return m_condOffset; }
    uint16_t valueSize() const
    {
        return m_valueSize;
    }
    ByteCodeStackOffset src0Offset() const { return m_src0Offset; }
    ByteCodeStackOffset src1Offset() const { return m_src1Offset; }
    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("select ");
        DUMP_BYTECODE_OFFSET(condOffset);
        DUMP_BYTECODE_OFFSET(src0Offset);
        DUMP_BYTECODE_OFFSET(src1Offset);
        DUMP_BYTECODE_OFFSET(dstOffset);
    }
#endif

protected:
    ByteCodeStackOffset m_condOffset;
    uint16_t m_valueSize;
    ByteCodeStackOffset m_src0Offset;
    ByteCodeStackOffset m_src1Offset;
    ByteCodeStackOffset m_dstOffset;
};

class BrTable : public ByteCode {
public:
    BrTable(ByteCodeStackOffset condOffset, uint32_t tableSize)
        : ByteCode(Opcode::BrTableOpcode)
        , m_condOffset(condOffset)
        , m_defaultOffset(0)
        , m_tableSize(tableSize)
    {
    }

    ByteCodeStackOffset condOffset() const { return m_condOffset; }
    int32_t defaultOffset() const { return m_defaultOffset; }
    static inline size_t offsetOfDefault() { return offsetof(BrTable, m_defaultOffset); }

    uint32_t tableSize() const { return m_tableSize; }
    int32_t* jumpOffsets() const
    {
        return reinterpret_cast<int32_t*>(reinterpret_cast<size_t>(this) + sizeof(BrTable));
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("br_table ");
        DUMP_BYTECODE_OFFSET(condOffset);
        printf("tableSize: %" PRIu32 ", defaultOffset: %" PRId32, m_tableSize, m_defaultOffset);
        printf(" table contents: ");
        for (size_t i = 0; i < m_tableSize; i++) {
            printf("%zu->%" PRId32 " ", i, jumpOffsets()[i]);
        }
    }
#endif

protected:
    ByteCodeStackOffset m_condOffset;
    int32_t m_defaultOffset;
    uint32_t m_tableSize;
};

class MemorySize : public ByteCode {
public:
    MemorySize(uint32_t index, ByteCodeStackOffset dstOffset)
        : ByteCode(Opcode::MemorySizeOpcode)
        , m_dstOffset(dstOffset)
    {
        ASSERT(index == 0);
    }

    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("memory.size ");
        DUMP_BYTECODE_OFFSET(dstOffset);
    }
#endif

protected:
    ByteCodeStackOffset m_dstOffset;
};

class MemoryInit : public ByteCode {
public:
    MemoryInit(uint32_t index, uint32_t segmentIndex, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset src2)
        : ByteCode(Opcode::MemoryInitOpcode)
        , m_segmentIndex(segmentIndex)
        , m_srcOffsets{ src0, src1, src2 }
    {
        ASSERT(index == 0);
    }

    uint32_t segmentIndex() const
    {
        return m_segmentIndex;
    }

    const ByteCodeStackOffset* srcOffsets() const
    {
        return m_srcOffsets;
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("memory.init ");
        DUMP_BYTECODE_OFFSET(srcOffsets[0]);
        DUMP_BYTECODE_OFFSET(srcOffsets[1]);
        DUMP_BYTECODE_OFFSET(srcOffsets[2]);
        printf("segmentIndex: %" PRIu32, m_segmentIndex);
    }
#endif

protected:
    uint32_t m_segmentIndex;
    ByteCodeStackOffset m_srcOffsets[3];
};

class MemoryCopy : public ByteCode {
public:
    MemoryCopy(uint32_t srcIndex, uint32_t dstIndex, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset src2)
        : ByteCode(Opcode::MemoryCopyOpcode)
        , m_srcOffsets{ src0, src1, src2 }
    {
        ASSERT(srcIndex == 0);
        ASSERT(dstIndex == 0);
    }

    const ByteCodeStackOffset* srcOffsets() const
    {
        return m_srcOffsets;
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("memory.copy ");
        DUMP_BYTECODE_OFFSET(srcOffsets[0]);
        DUMP_BYTECODE_OFFSET(srcOffsets[1]);
        DUMP_BYTECODE_OFFSET(srcOffsets[2]);
    }
#endif
protected:
    ByteCodeStackOffset m_srcOffsets[3];
};

class MemoryFill : public ByteCode {
public:
    MemoryFill(uint32_t memIdx, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset src2)
        : ByteCode(Opcode::MemoryFillOpcode)
        , m_srcOffsets{ src0, src1, src2 }
    {
        ASSERT(memIdx == 0);
    }

    const ByteCodeStackOffset* srcOffsets() const
    {
        return m_srcOffsets;
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("memory.fill ");
        DUMP_BYTECODE_OFFSET(srcOffsets[0]);
        DUMP_BYTECODE_OFFSET(srcOffsets[1]);
        DUMP_BYTECODE_OFFSET(srcOffsets[2]);
    }
#endif
protected:
    ByteCodeStackOffset m_srcOffsets[3];
};

class DataDrop : public ByteCode {
public:
    DataDrop(uint32_t segmentIndex)
        : ByteCode(Opcode::DataDropOpcode)
        , m_segmentIndex(segmentIndex)
    {
    }

    uint32_t segmentIndex() const
    {
        return m_segmentIndex;
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("data.drop ");
        printf("segmentIndex: %" PRIu32, m_segmentIndex);
    }
#endif

protected:
    uint32_t m_segmentIndex;
};

class MemoryGrow : public ByteCode {
public:
    MemoryGrow(uint32_t index, ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset)
        : ByteCode(Opcode::MemoryGrowOpcode)
        , m_srcOffset(srcOffset)
        , m_dstOffset(dstOffset)
    {
        ASSERT(index == 0);
    }

    ByteCodeStackOffset srcOffset() const { return m_srcOffset; }
    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("memory.grow ");
        DUMP_BYTECODE_OFFSET(srcOffset);
        DUMP_BYTECODE_OFFSET(dstOffset);
    }
#endif

protected:
    ByteCodeStackOffset m_srcOffset;
    ByteCodeStackOffset m_dstOffset;
};

// dummy ByteCode for memory load operation
class MemoryLoad : public ByteCode {
public:
    MemoryLoad(Opcode code, uint32_t offset, ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset)
        : ByteCode(code)
        , m_offset(offset)
        , m_srcOffset(srcOffset)
        , m_dstOffset(dstOffset)
    {
    }

    uint32_t offset() const { return m_offset; }
    ByteCodeStackOffset srcOffset() const { return m_srcOffset; }
    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
    }
#endif
protected:
    uint32_t m_offset;
    ByteCodeStackOffset m_srcOffset;
    ByteCodeStackOffset m_dstOffset;
};

// dummy ByteCode for simd memory load operation
class SIMDMemoryLoad : public ByteCode {
public:
    SIMDMemoryLoad(Opcode code, uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset index, ByteCodeStackOffset dst)
        : ByteCode(code)
        , m_offset(offset)
        , m_src0Offset(src0)
        , m_src1Offset(src1)
        , m_index(index)
        , m_dstOffset(dst)
    {
    }

    uint32_t offset() const { return m_offset; }
    ByteCodeStackOffset index() const { return m_index; }
    ByteCodeStackOffset src0Offset() const { return m_src0Offset; }
    ByteCodeStackOffset src1Offset() const { return m_src1Offset; }
    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
    }
#endif
protected:
    uint32_t m_offset;
    ByteCodeStackOffset m_src0Offset;
    ByteCodeStackOffset m_src1Offset;
    ByteCodeStackOffset m_index;
    ByteCodeStackOffset m_dstOffset;
};

#if !defined(NDEBUG)
#define DEFINE_LOAD_BYTECODE_DUMP(name)                                                                                                        \
    void dump(size_t pos)                                                                                                                      \
    {                                                                                                                                          \
        printf(#name " src: %" PRIu32 " dst: %" PRIu32 " offset: %" PRIu32, (uint32_t)m_srcOffset, (uint32_t)m_dstOffset, (uint32_t)m_offset); \
    }
#else
#define DEFINE_LOAD_BYTECODE_DUMP(name)
#endif

#define DEFINE_LOAD_BYTECODE(name, readType, writeType)                                     \
    class name : public MemoryLoad {                                                        \
    public:                                                                                 \
        name(uint32_t offset, ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset) \
            : MemoryLoad(Opcode::name##Opcode, offset, srcOffset, dstOffset)                \
        {                                                                                   \
        }                                                                                   \
        DEFINE_LOAD_BYTECODE_DUMP(name)                                                     \
    };

#if !defined(NDEBUG)
#define DEFINE_SIMD_LOAD_BYTECODE_DUMP(name)                                                                                                                                                                                   \
    void dump(size_t pos)                                                                                                                                                                                                      \
    {                                                                                                                                                                                                                          \
        printf(#name " idx: %" PRIu32 " src0: %" PRIu32 " src1: %" PRIu32 " dst: %" PRIu32 " offset: %" PRIu32, (uint32_t)m_index, (uint32_t)m_src0Offset, (uint32_t)m_src1Offset, (uint32_t)m_dstOffset, (uint32_t)m_offset); \
    }
#else
#define DEFINE_SIMD_LOAD_BYTECODE_DUMP(name)
#endif

#define DEFINE_SIMD_LOAD_BYTECODE(name, opType)                                                                                       \
    class name : public SIMDMemoryLoad {                                                                                              \
    public:                                                                                                                           \
        name(uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset index, ByteCodeStackOffset dst) \
            : SIMDMemoryLoad(Opcode::name##Opcode, offset, src0, src1, index, dst)                                                    \
        {                                                                                                                             \
        }                                                                                                                             \
        DEFINE_SIMD_LOAD_BYTECODE_DUMP(name)                                                                                          \
    };

// dummy ByteCode for memory store operation
class MemoryStore : public ByteCode {
public:
    MemoryStore(Opcode opcode, uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1)
        : ByteCode(opcode)
        , m_offset(offset)
        , m_src0Offset(src0)
        , m_src1Offset(src1)
    {
    }

    uint32_t offset() const { return m_offset; }
    ByteCodeStackOffset src0Offset() const { return m_src0Offset; }
    ByteCodeStackOffset src1Offset() const { return m_src1Offset; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
    }
#endif
protected:
    uint32_t m_offset;
    ByteCodeStackOffset m_src0Offset;
    ByteCodeStackOffset m_src1Offset;
};

// dummy ByteCode for simd memory store operation
class SIMDMemoryStore : public ByteCode {
public:
    SIMDMemoryStore(Opcode opcode, uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset index)
        : ByteCode(opcode)
        , m_offset(offset)
        , m_src0Offset(src0)
        , m_src1Offset(src1)
        , m_index(index)
    {
    }

    uint32_t offset() const { return m_offset; }
    ByteCodeStackOffset index() const { return m_index; }
    ByteCodeStackOffset src0Offset() const { return m_src0Offset; }
    ByteCodeStackOffset src1Offset() const { return m_src1Offset; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
    }
#endif
protected:
    uint32_t m_offset;
    ByteCodeStackOffset m_src0Offset;
    ByteCodeStackOffset m_src1Offset;
    ByteCodeStackOffset m_index;
};

#if !defined(NDEBUG)
#define DEFINE_STORE_BYTECODE_DUMP(name)                                                                                                          \
    void dump(size_t pos)                                                                                                                         \
    {                                                                                                                                             \
        printf(#name " src0: %" PRIu32 "src1: %" PRIu32 " offset: %" PRIu32, (uint32_t)m_src0Offset, (uint32_t)m_src1Offset, (uint32_t)m_offset); \
    }
#else
#define DEFINE_STORE_BYTECODE_DUMP(name)
#endif

#define DEFINE_STORE_BYTECODE(name, readType, writeType)                          \
    class name : public MemoryStore {                                             \
    public:                                                                       \
        name(uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1) \
            : MemoryStore(Opcode::name##Opcode, offset, src0, src1)               \
        {                                                                         \
        }                                                                         \
        DEFINE_STORE_BYTECODE_DUMP(name)                                          \
    };

#if !defined(NDEBUG)
#define DEFINE_SIMD_STORE_BYTECODE_DUMP(name)                                                                                                                                         \
    void dump(size_t pos)                                                                                                                                                             \
    {                                                                                                                                                                                 \
        printf(#name " idx: %" PRIu32 " src0: %" PRIu32 "src1: %" PRIu32 " offset: %" PRIu32, (uint32_t)m_index, (uint32_t)m_src0Offset, (uint32_t)m_src1Offset, (uint32_t)m_offset); \
    }
#else
#define DEFINE_SIMD_STORE_BYTECODE_DUMP(name)
#endif

#define DEFINE_SIMD_STORE_BYTECODE(name, opType)                                                             \
    class name : public SIMDMemoryStore {                                                                    \
    public:                                                                                                  \
        name(uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset index) \
            : SIMDMemoryStore(Opcode::name##Opcode, offset, src0, src1, index)                               \
        {                                                                                                    \
        }                                                                                                    \
        DEFINE_SIMD_STORE_BYTECODE_DUMP(name)                                                                \
    };

FOR_EACH_BYTECODE_LOAD_OP(DEFINE_LOAD_BYTECODE)
FOR_EACH_BYTECODE_STORE_OP(DEFINE_STORE_BYTECODE)
FOR_EACH_BYTECODE_SIMD_LOAD_OP(DEFINE_SIMD_LOAD_BYTECODE)
FOR_EACH_BYTECODE_SIMD_STORE_OP(DEFINE_SIMD_STORE_BYTECODE)
#undef DEFINE_LOAD_BYTECODE_DUMP
#undef DEFINE_LOAD_BYTECODE
#undef DEFINE_STORE_BYTECODE_DUMP
#undef DEFINE_STORE_BYTECODE

class TableGet : public ByteCode {
public:
    TableGet(uint32_t index, ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset)
        : ByteCode(Opcode::TableGetOpcode)
        , m_tableIndex(index)
        , m_srcOffset(srcOffset)
        , m_dstOffset(dstOffset)
    {
    }

    uint32_t tableIndex() const { return m_tableIndex; }
    ByteCodeStackOffset srcOffset() const { return m_srcOffset; }
    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("table.get ");
        DUMP_BYTECODE_OFFSET(srcOffset);
        DUMP_BYTECODE_OFFSET(dstOffset);
        printf("tableIndex: %" PRIu32, m_tableIndex);
    }
#endif

protected:
    uint32_t m_tableIndex;
    ByteCodeStackOffset m_srcOffset;
    ByteCodeStackOffset m_dstOffset;
};

class TableSet : public ByteCode {
public:
    TableSet(uint32_t index, ByteCodeStackOffset src0, ByteCodeStackOffset src1)
        : ByteCode(Opcode::TableSetOpcode)
        , m_tableIndex(index)
        , m_src0Offset(src0)
        , m_src1Offset(src1)
    {
    }

    ByteCodeStackOffset src0Offset() const { return m_src0Offset; }
    ByteCodeStackOffset src1Offset() const { return m_src1Offset; }
    uint32_t tableIndex() const { return m_tableIndex; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("table.set ");
        DUMP_BYTECODE_OFFSET(src0Offset);
        DUMP_BYTECODE_OFFSET(src1Offset);
        printf("tableIndex: %" PRIu32, m_tableIndex);
    }
#endif

protected:
    uint32_t m_tableIndex;
    ByteCodeStackOffset m_src0Offset;
    ByteCodeStackOffset m_src1Offset;
};

class TableGrow : public ByteCode {
public:
    TableGrow(uint32_t index, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset dst)
        : ByteCode(Opcode::TableGrowOpcode)
        , m_tableIndex(index)
        , m_src0Offset(src0)
        , m_src1Offset(src1)
        , m_dstOffset(dst)
    {
    }

    uint32_t tableIndex() const { return m_tableIndex; }
    ByteCodeStackOffset src0Offset() const { return m_src0Offset; }
    ByteCodeStackOffset src1Offset() const { return m_src1Offset; }
    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("table.grow ");
        DUMP_BYTECODE_OFFSET(src0Offset);
        DUMP_BYTECODE_OFFSET(src1Offset);
        DUMP_BYTECODE_OFFSET(dstOffset);
        printf("tableIndex: %" PRIu32, m_tableIndex);
    }
#endif

protected:
    uint32_t m_tableIndex;
    ByteCodeStackOffset m_src0Offset;
    ByteCodeStackOffset m_src1Offset;
    ByteCodeStackOffset m_dstOffset;
};

class TableSize : public ByteCode {
public:
    TableSize(uint32_t index, ByteCodeStackOffset dst)
        : ByteCode(Opcode::TableSizeOpcode)
        , m_tableIndex(index)
        , m_dstOffset(dst)
    {
    }

    uint32_t tableIndex() const { return m_tableIndex; }
    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("table.size ");
        DUMP_BYTECODE_OFFSET(dstOffset);
        printf("tableIndex: %" PRIu32, m_tableIndex);
    }
#endif

protected:
    uint32_t m_tableIndex;
    ByteCodeStackOffset m_dstOffset;
};

class TableCopy : public ByteCode {
public:
    TableCopy(uint32_t dstIndex, uint32_t srcIndex, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset src2)
        : ByteCode(Opcode::TableCopyOpcode)
        , m_dstIndex(dstIndex)
        , m_srcIndex(srcIndex)
        , m_srcOffsets{ src0, src1, src2 }
    {
    }

    uint32_t dstIndex() const { return m_dstIndex; }
    uint32_t srcIndex() const { return m_srcIndex; }
    const ByteCodeStackOffset* srcOffsets() const
    {
        return m_srcOffsets;
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("table.copy ");
        DUMP_BYTECODE_OFFSET(srcOffsets[0]);
        DUMP_BYTECODE_OFFSET(srcOffsets[1]);
        DUMP_BYTECODE_OFFSET(srcOffsets[2]);
        printf("dstIndex: %" PRIu32 " srcIndex: %" PRIu32, m_dstIndex, m_srcIndex);
    }
#endif

protected:
    uint32_t m_dstIndex;
    uint32_t m_srcIndex;
    ByteCodeStackOffset m_srcOffsets[3];
};

class TableFill : public ByteCode {
public:
    TableFill(uint32_t index, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset src2)
        : ByteCode(Opcode::TableFillOpcode)
        , m_tableIndex(index)
        , m_srcOffsets{ src0, src1, src2 }
    {
    }

    uint32_t tableIndex() const { return m_tableIndex; }
    const ByteCodeStackOffset* srcOffsets() const
    {
        return m_srcOffsets;
    }
#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("table.fill ");
        DUMP_BYTECODE_OFFSET(srcOffsets[0]);
        DUMP_BYTECODE_OFFSET(srcOffsets[1]);
        DUMP_BYTECODE_OFFSET(srcOffsets[2]);
        printf("tableIndex: %" PRIu32, m_tableIndex);
    }
#endif

protected:
    uint32_t m_tableIndex;
    ByteCodeStackOffset m_srcOffsets[3];
};

class TableInit : public ByteCode {
public:
    TableInit(uint32_t tableIndex, uint32_t segmentIndex, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset src2)
        : ByteCode(Opcode::TableInitOpcode)
        , m_tableIndex(tableIndex)
        , m_segmentIndex(segmentIndex)
        , m_srcOffsets{ src0, src1, src2 }
    {
    }

    uint32_t tableIndex() const { return m_tableIndex; }
    uint32_t segmentIndex() const { return m_segmentIndex; }
    const ByteCodeStackOffset* srcOffsets() const
    {
        return m_srcOffsets;
    }
#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("table.init ");
        DUMP_BYTECODE_OFFSET(srcOffsets[0]);
        DUMP_BYTECODE_OFFSET(srcOffsets[1]);
        DUMP_BYTECODE_OFFSET(srcOffsets[2]);
        printf("tableIndex: %" PRIu32 " segmentIndex: %" PRIu32, m_tableIndex, m_segmentIndex);
    }
#endif

protected:
    uint32_t m_tableIndex;
    uint32_t m_segmentIndex;
    ByteCodeStackOffset m_srcOffsets[3];
};

class ElemDrop : public ByteCode {
public:
    ElemDrop(uint32_t segmentIndex)
        : ByteCode(Opcode::ElemDropOpcode)
        , m_segmentIndex(segmentIndex)
    {
    }

    uint32_t segmentIndex() const
    {
        return m_segmentIndex;
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("elem.drop segmentIndex: %" PRIu32, m_segmentIndex);
    }
#endif

protected:
    uint32_t m_segmentIndex;
};

class RefFunc : public ByteCode {
public:
    RefFunc(uint32_t funcIndex, ByteCodeStackOffset dstOffset)
        : ByteCode(Opcode::RefFuncOpcode)
        , m_funcIndex(funcIndex)
        , m_dstOffset(dstOffset)
    {
    }

    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }
    uint32_t funcIndex() const { return m_funcIndex; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("ref.func ");
        DUMP_BYTECODE_OFFSET(dstOffset);
        printf("funcIndex: %" PRIu32, m_funcIndex);
    }
#endif

private:
    uint32_t m_funcIndex;
    ByteCodeStackOffset m_dstOffset;
};

class GlobalGet32 : public ByteCode {
public:
    GlobalGet32(ByteCodeStackOffset dstOffset, uint32_t index)
        : ByteCode(Opcode::GlobalGet32Opcode)
        , m_dstOffset(dstOffset)
        , m_index(index)
    {
    }

    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }
    uint32_t index() const { return m_index; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("global.get32 ");
        printf("index: %" PRId32,
               m_index);
    }
#endif

protected:
    ByteCodeStackOffset m_dstOffset;
    uint32_t m_index;
};

class GlobalGet64 : public ByteCode {
public:
    GlobalGet64(ByteCodeStackOffset dstOffset, uint32_t index)
        : ByteCode(Opcode::GlobalGet64Opcode)
        , m_dstOffset(dstOffset)
        , m_index(index)
    {
    }

    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }
    uint32_t index() const { return m_index; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("global.get64 ");
        printf("index: %" PRId32,
               m_index);
    }
#endif

protected:
    ByteCodeStackOffset m_dstOffset;
    uint32_t m_index;
};

class GlobalGet128 : public ByteCode {
public:
    GlobalGet128(ByteCodeStackOffset dstOffset, uint32_t index)
        : ByteCode(Opcode::GlobalGet128Opcode)
        , m_dstOffset(dstOffset)
        , m_index(index)
    {
    }

    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }
    uint32_t index() const { return m_index; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("global.get128 ");
        printf("index: %" PRId32,
               m_index);
    }
#endif

protected:
    ByteCodeStackOffset m_dstOffset;
    uint32_t m_index;
};

class GlobalSet32 : public ByteCode {
public:
    GlobalSet32(ByteCodeStackOffset srcOffset, uint32_t index)
        : ByteCode(Opcode::GlobalSet32Opcode)
        , m_srcOffset(srcOffset)
        , m_index(index)
    {
    }

    ByteCodeStackOffset srcOffset() const { return m_srcOffset; }
    uint32_t index() const { return m_index; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("global.set32 ");
        printf("index: %" PRId32,
               m_index);
    }
#endif

protected:
    ByteCodeStackOffset m_srcOffset;
    uint32_t m_index;
};

class GlobalSet64 : public ByteCode {
public:
    GlobalSet64(ByteCodeStackOffset srcOffset, uint32_t index)
        : ByteCode(Opcode::GlobalSet64Opcode)
        , m_srcOffset(srcOffset)
        , m_index(index)
    {
    }

    ByteCodeStackOffset srcOffset() const { return m_srcOffset; }
    uint32_t index() const { return m_index; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("global.set64 ");
        printf("index: %" PRId32,
               m_index);
    }
#endif

protected:
    ByteCodeStackOffset m_srcOffset;
    uint32_t m_index;
};

class GlobalSet128 : public ByteCode {
public:
    GlobalSet128(ByteCodeStackOffset srcOffset, uint32_t index)
        : ByteCode(Opcode::GlobalSet128Opcode)
        , m_srcOffset(srcOffset)
        , m_index(index)
    {
    }

    ByteCodeStackOffset srcOffset() const { return m_srcOffset; }
    uint32_t index() const { return m_index; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("global.set128 ");
        printf("index: %" PRId32,
               m_index);
    }
#endif

protected:
    ByteCodeStackOffset m_srcOffset;
    uint32_t m_index;
};

class Throw : public ByteCode {
public:
    Throw(uint32_t index, uint32_t offsetsSize)
        : ByteCode(Opcode::ThrowOpcode)
        , m_tagIndex(index)
        , m_offsetsSize(offsetsSize)
    {
    }

    uint32_t tagIndex() const { return m_tagIndex; }
    ByteCodeStackOffset* dataOffsets() const
    {
        return reinterpret_cast<ByteCodeStackOffset*>(reinterpret_cast<size_t>(this) + sizeof(Throw));
    }

    uint32_t offsetsSize()
    {
        return m_offsetsSize;
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("throw tagIndex: %" PRId32 " ",
               m_tagIndex);

        if (m_tagIndex != std::numeric_limits<uint32_t>::max()) {
            auto arr = dataOffsets();
            printf("resultOffsets: ");
            for (size_t i = 0; i < offsetsSize(); i++) {
                printf("%" PRIu32 " ", (uint32_t)arr[i]);
            }
            printf(" ");
        }
    }
#endif

protected:
    uint32_t m_tagIndex;
    uint32_t m_offsetsSize;
};

class Unreachable : public ByteCode {
public:
    Unreachable()
        : ByteCode(Opcode::UnreachableOpcode)
    {
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("unreachable");
    }
#endif

protected:
};

class End : public ByteCode {
public:
    End(uint32_t offsetsSize)
        : ByteCode(Opcode::EndOpcode)
        , m_offsetsSize(offsetsSize)
    {
    }

    ByteCodeStackOffset* resultOffsets() const
    {
        return reinterpret_cast<ByteCodeStackOffset*>(reinterpret_cast<size_t>(this) + sizeof(End));
    }

    uint32_t offsetsSize()
    {
        return m_offsetsSize;
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        auto arr = resultOffsets();
        printf("end resultOffsets: ");
        for (size_t i = 0; i < offsetsSize(); i++) {
            printf("%" PRIu32 " ", arr[i]);
        }
        printf(" ");
    }
#endif

protected:
    uint32_t m_offsetsSize;
};

class FillOpcodeTable : public ByteCode {
public:
    FillOpcodeTable()
        : ByteCode(Opcode::FillOpcodeTableOpcode)
    {
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("fill opcode table");
    }
#endif
};

} // namespace Walrus

#endif // __WalrusByteCode__
