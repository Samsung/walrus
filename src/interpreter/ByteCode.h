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

class FunctionType;

#if defined(NDEBUG)
#define F_NOP(F)
#else /* !NDEBUG */
#define F_NOP(F) F(Nop)
#endif /* NDEBUG */

#define FOR_EACH_BYTECODE_OP(F) \
    F_NOP(F)                    \
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
    F(MoveI32)                  \
    F(MoveF32)                  \
    F(MoveI64)                  \
    F(MoveF64)                  \
    F(MoveV128)                 \
    F(I32ReinterpretF32)        \
    F(I64ReinterpretF64)        \
    F(F32ReinterpretI32)        \
    F(F64ReinterpretI64)        \
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
    F(MemorySizeMulti)          \
    F(MemoryGrowMulti)          \
    F(MemoryInitMulti)          \
    F(MemoryCopyMulti)          \
    F(MemoryFillMulti)          \
    F(Load32Multi)              \
    F(Load64Multi)              \
    F(Store32Multi)             \
    F(Store64Multi)             \
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

#define FOR_EACH_BYTECODE_SIMD_BINARY_OP(F)                       \
    F(I8X16Add, add, uint8_t, uint8_t)                            \
    F(I8X16AddSatS, intAddSat, int8_t, int8_t)                    \
    F(I8X16AddSatU, intAddSat, uint8_t, uint8_t)                  \
    F(I8X16Sub, sub, uint8_t, uint8_t)                            \
    F(I8X16SubSatS, intSubSat, int8_t, int8_t)                    \
    F(I8X16SubSatU, intSubSat, uint8_t, uint8_t)                  \
    F(I16X8Add, add, uint16_t, uint16_t)                          \
    F(I16X8AddSatS, intAddSat, int16_t, int16_t)                  \
    F(I16X8AddSatU, intAddSat, uint16_t, uint16_t)                \
    F(I16X8Sub, sub, uint16_t, uint16_t)                          \
    F(I16X8SubSatS, intSubSat, int16_t, int16_t)                  \
    F(I16X8SubSatU, intSubSat, uint16_t, uint16_t)                \
    F(I16X8Mul, mul, uint16_t, uint16_t)                          \
    F(I32X4Add, add, uint32_t, uint32_t)                          \
    F(I32X4Sub, sub, uint32_t, uint32_t)                          \
    F(I32X4Mul, mul, uint32_t, uint32_t)                          \
    F(I64X2Add, add, uint64_t, uint64_t)                          \
    F(I64X2Sub, sub, uint64_t, uint64_t)                          \
    F(I64X2Mul, mul, uint64_t, uint64_t)                          \
    F(F32X4Add, add, float, float)                                \
    F(F32X4Sub, sub, float, float)                                \
    F(F32X4Mul, mul, float, float)                                \
    F(F32X4Div, floatDiv, float, float)                           \
    F(F64X2Add, add, double, double)                              \
    F(F64X2Sub, sub, double, double)                              \
    F(F64X2Mul, mul, double, double)                              \
    F(F64X2Div, floatDiv, double, double)                         \
    F(I8X16Eq, eqMask, uint8_t, uint8_t)                          \
    F(I8X16Ne, neMask, uint8_t, uint8_t)                          \
    F(I8X16LtS, ltMask, int8_t, int8_t)                           \
    F(I8X16LtU, ltMask, uint8_t, uint8_t)                         \
    F(I8X16GtS, gtMask, int8_t, int8_t)                           \
    F(I8X16GtU, gtMask, uint8_t, uint8_t)                         \
    F(I8X16LeS, leMask, int8_t, int8_t)                           \
    F(I8X16LeU, leMask, uint8_t, uint8_t)                         \
    F(I8X16GeS, geMask, int8_t, int8_t)                           \
    F(I8X16GeU, geMask, uint8_t, uint8_t)                         \
    F(I8X16MinS, intMin, int8_t, int8_t)                          \
    F(I8X16MinU, intMin, uint8_t, uint8_t)                        \
    F(I8X16MaxS, intMax, int8_t, int8_t)                          \
    F(I8X16MaxU, intMax, uint8_t, uint8_t)                        \
    F(I8X16AvgrU, intAvgr, uint8_t, uint8_t)                      \
    F(I16X8Eq, eqMask, uint16_t, uint16_t)                        \
    F(I16X8Ne, neMask, uint16_t, uint16_t)                        \
    F(I16X8LtS, ltMask, int16_t, int16_t)                         \
    F(I16X8LtU, ltMask, uint16_t, uint16_t)                       \
    F(I16X8GtS, gtMask, int16_t, int16_t)                         \
    F(I16X8GtU, gtMask, uint16_t, uint16_t)                       \
    F(I16X8LeS, leMask, int16_t, int16_t)                         \
    F(I16X8LeU, leMask, uint16_t, uint16_t)                       \
    F(I16X8GeS, geMask, int16_t, int16_t)                         \
    F(I16X8GeU, geMask, uint16_t, uint16_t)                       \
    F(I16X8MinS, intMin, int16_t, int16_t)                        \
    F(I16X8MinU, intMin, uint16_t, uint16_t)                      \
    F(I16X8MaxS, intMax, int16_t, int16_t)                        \
    F(I16X8MaxU, intMax, uint16_t, uint16_t)                      \
    F(I16X8AvgrU, intAvgr, uint16_t, uint16_t)                    \
    F(I32X4Eq, eqMask, uint32_t, uint32_t)                        \
    F(I32X4Ne, neMask, uint32_t, uint32_t)                        \
    F(I32X4LtS, ltMask, int32_t, int32_t)                         \
    F(I32X4LtU, ltMask, uint32_t, uint32_t)                       \
    F(I32X4GtS, gtMask, int32_t, int32_t)                         \
    F(I32X4GtU, gtMask, uint32_t, uint32_t)                       \
    F(I32X4LeS, leMask, int32_t, int32_t)                         \
    F(I32X4LeU, leMask, uint32_t, uint32_t)                       \
    F(I32X4GeS, geMask, int32_t, int32_t)                         \
    F(I32X4GeU, geMask, uint32_t, uint32_t)                       \
    F(I32X4MinS, intMin, int32_t, int32_t)                        \
    F(I32X4MinU, intMin, uint32_t, uint32_t)                      \
    F(I32X4MaxS, intMax, int32_t, int32_t)                        \
    F(I32X4MaxU, intMax, uint32_t, uint32_t)                      \
    F(I64X2Eq, eqMask, uint64_t, uint64_t)                        \
    F(I64X2Ne, neMask, uint64_t, uint64_t)                        \
    F(I64X2LtS, ltMask, int64_t, int64_t)                         \
    F(I64X2GtS, gtMask, int64_t, int64_t)                         \
    F(I64X2LeS, leMask, int64_t, int64_t)                         \
    F(I64X2GeS, geMask, int64_t, int64_t)                         \
    F(F32X4Eq, eqMask, float, uint32_t)                           \
    F(F32X4Ne, neMask, float, uint32_t)                           \
    F(F32X4Lt, ltMask, float, uint32_t)                           \
    F(F32X4Gt, gtMask, float, uint32_t)                           \
    F(F32X4Le, leMask, float, uint32_t)                           \
    F(F32X4Ge, geMask, float, uint32_t)                           \
    F(F32X4Min, floatMin, float, float)                           \
    F(F32X4Max, floatMax, float, float)                           \
    F(F32X4PMin, floatPMin, float, float)                         \
    F(F32X4PMax, floatPMax, float, float)                         \
    F(F64X2Eq, eqMask, double, uint64_t)                          \
    F(F64X2Ne, neMask, double, uint64_t)                          \
    F(F64X2Lt, ltMask, double, uint64_t)                          \
    F(F64X2Gt, gtMask, double, uint64_t)                          \
    F(F64X2Le, leMask, double, uint64_t)                          \
    F(F64X2Ge, geMask, double, uint64_t)                          \
    F(F64X2Min, floatMin, double, double)                         \
    F(F64X2Max, floatMax, double, double)                         \
    F(F64X2PMin, floatPMin, double, double)                       \
    F(F64X2PMax, floatPMax, double, double)                       \
    F(I16X8Q15mulrSatS, saturatingRoundingQMul, int16_t, int16_t) \
    F(V128And, intAnd, uint64_t, uint64_t)                        \
    F(V128Andnot, intAndNot, uint64_t, uint64_t)                  \
    F(V128Or, intOr, uint64_t, uint64_t)                          \
    F(V128Xor, intXor, uint64_t, uint64_t)

#define FOR_EACH_BYTECODE_SIMD_BINARY_SHIFT_OP(F) \
    F(I8X16Shl, intShl, uint8_t)                  \
    F(I8X16ShrS, intShr, int8_t)                  \
    F(I8X16ShrU, intShr, uint8_t)                 \
    F(I16X8Shl, intShl, uint16_t)                 \
    F(I16X8ShrS, intShr, int16_t)                 \
    F(I16X8ShrU, intShr, uint16_t)                \
    F(I32X4Shl, intShl, uint32_t)                 \
    F(I32X4ShrS, intShr, int32_t)                 \
    F(I32X4ShrU, intShr, uint32_t)                \
    F(I64X2Shl, intShl, uint64_t)                 \
    F(I64X2ShrS, intShr, int64_t)                 \
    F(I64X2ShrU, intShr, uint64_t)

#define FOR_EACH_BYTECODE_SIMD_BINARY_OTHER(F)                                 \
    F(I8X16Swizzle, (simdSwizzleOperation<uint8_t>))                           \
    F(I16X8ExtmulLowI8X16S, (simdExtmulOperation<int8_t, int16_t, true>))      \
    F(I16X8ExtmulHighI8X16S, (simdExtmulOperation<int8_t, int16_t, false>))    \
    F(I16X8ExtmulLowI8X16U, (simdExtmulOperation<uint8_t, uint16_t, true>))    \
    F(I16X8ExtmulHighI8X16U, (simdExtmulOperation<uint8_t, uint16_t, false>))  \
    F(I32X4ExtmulLowI16X8S, (simdExtmulOperation<int16_t, int32_t, true>))     \
    F(I32X4ExtmulHighI16X8S, (simdExtmulOperation<int16_t, int32_t, false>))   \
    F(I32X4ExtmulLowI16X8U, (simdExtmulOperation<uint16_t, uint32_t, true>))   \
    F(I32X4ExtmulHighI16X8U, (simdExtmulOperation<uint16_t, uint32_t, false>)) \
    F(I64X2ExtmulLowI32X4S, (simdExtmulOperation<int32_t, int64_t, true>))     \
    F(I64X2ExtmulHighI32X4S, (simdExtmulOperation<int32_t, int64_t, false>))   \
    F(I64X2ExtmulLowI32X4U, (simdExtmulOperation<uint32_t, uint64_t, true>))   \
    F(I64X2ExtmulHighI32X4U, (simdExtmulOperation<uint32_t, uint64_t, false>)) \
    F(I32X4DotI16X8S, (simdDotOperation<int16_t, uint32_t>))                   \
    F(I8X16NarrowI16X8S, (simdNarrowOperation<int16_t, int8_t>))               \
    F(I8X16NarrowI16X8U, (simdNarrowOperation<int16_t, uint8_t>))              \
    F(I16X8NarrowI32X4S, (simdNarrowOperation<int32_t, int16_t>))              \
    F(I16X8NarrowI32X4U, (simdNarrowOperation<int32_t, uint16_t>))

#define FOR_EACH_BYTECODE_SIMD_UNARY_OP(F) \
    F(I8X16Neg, intNeg, uint8_t)           \
    F(I8X16Abs, intAbs, uint8_t)           \
    F(I8X16Popcnt, intPopcnt, uint8_t)     \
    F(I16X8Neg, intNeg, uint16_t)          \
    F(I16X8Abs, intAbs, uint16_t)          \
    F(I32X4Neg, intNeg, uint32_t)          \
    F(I32X4Abs, intAbs, uint32_t)          \
    F(I64X2Neg, intNeg, uint64_t)          \
    F(I64X2Abs, intAbs, uint64_t)          \
    F(F32X4Neg, floatNeg, float)           \
    F(F32X4Abs, floatAbs, float)           \
    F(F32X4Ceil, floatCeil, float)         \
    F(F32X4Floor, floatFloor, float)       \
    F(F32X4Trunc, floatTrunc, float)       \
    F(F32X4Nearest, floatNearest, float)   \
    F(F32X4Sqrt, floatSqrt, float)         \
    F(F64X2Neg, floatNeg, double)          \
    F(F64X2Abs, floatAbs, double)          \
    F(F64X2Ceil, floatCeil, double)        \
    F(F64X2Floor, floatFloor, double)      \
    F(F64X2Trunc, floatTrunc, double)      \
    F(F64X2Nearest, floatNearest, double)  \
    F(F64X2Sqrt, floatSqrt, double)        \
    F(V128Not, intNot, uint64_t)

#define FOR_EACH_BYTECODE_SIMD_UNARY_CONVERT_OP(F)      \
    F(I16X8ExtendLowI8X16S, int8_t, int16_t, true)      \
    F(I16X8ExtendHighI8X16S, int8_t, int16_t, false)    \
    F(I16X8ExtendLowI8X16U, uint8_t, uint16_t, true)    \
    F(I16X8ExtendHighI8X16U, uint8_t, uint16_t, false)  \
    F(I32X4ExtendLowI16X8S, int16_t, int32_t, true)     \
    F(I32X4ExtendHighI16X8S, int16_t, int32_t, false)   \
    F(I32X4ExtendLowI16X8U, uint16_t, uint32_t, true)   \
    F(I32X4ExtendHighI16X8U, uint16_t, uint32_t, false) \
    F(I64X2ExtendLowI32X4S, int32_t, int64_t, true)     \
    F(I64X2ExtendHighI32X4S, int32_t, int64_t, false)   \
    F(I64X2ExtendLowI32X4U, uint32_t, uint64_t, true)   \
    F(I64X2ExtendHighI32X4U, uint32_t, uint64_t, false) \
    F(F64X2PromoteLowF32X4, float, double, true)        \
    F(F64X2ConvertLowI32X4S, int32_t, double, true)     \
    F(F64X2ConvertLowI32X4U, uint32_t, double, true)

#define FOR_EACH_BYTECODE_SIMD_UNARY_OTHER(F)                                       \
    F(V128AnyTrue, (simdAnyTrueOperation))                                          \
    F(I8X16Bitmask, (simdBitmaskOperation<int8_t>))                                 \
    F(I8X16AllTrue, (simdAllTrueOperation<uint8_t, 16>))                            \
    F(I8X16Splat, (simdSplatOperation<uint32_t, uint8_t>))                          \
    F(I16X8Bitmask, (simdBitmaskOperation<int16_t>))                                \
    F(I16X8AllTrue, (simdAllTrueOperation<uint16_t, 8>))                            \
    F(I16X8Splat, (simdSplatOperation<uint32_t, uint16_t>))                         \
    F(I32X4Bitmask, (simdBitmaskOperation<int32_t>))                                \
    F(I32X4AllTrue, (simdAllTrueOperation<uint32_t, 4>))                            \
    F(I32X4Splat, (simdSplatOperation<uint32_t, uint32_t>))                         \
    F(I64X2Bitmask, (simdBitmaskOperation<int64_t>))                                \
    F(I64X2AllTrue, (simdAllTrueOperation<uint64_t, 2>))                            \
    F(I64X2Splat, (simdSplatOperation<uint64_t, uint64_t>))                         \
    F(I16X8ExtaddPairwiseI8X16S, (simdExtaddPairwiseOperation<int8_t, int16_t>))    \
    F(I16X8ExtaddPairwiseI8X16U, (simdExtaddPairwiseOperation<uint8_t, uint16_t>))  \
    F(I32X4ExtaddPairwiseI16X8S, (simdExtaddPairwiseOperation<int16_t, int32_t>))   \
    F(I32X4ExtaddPairwiseI16X8U, (simdExtaddPairwiseOperation<uint16_t, uint32_t>)) \
    F(I32X4TruncSatF32X4S, (simdTruncSatOperation<float, int32_t>))                 \
    F(I32X4TruncSatF32X4U, (simdTruncSatOperation<float, uint32_t>))                \
    F(I32X4TruncSatF64X2SZero, (simdTruncSatZeroOperation<double, int32_t>))        \
    F(I32X4TruncSatF64X2UZero, (simdTruncSatZeroOperation<double, uint32_t>))       \
    F(F32X4ConvertI32X4S, (simdConvertOperation<int32_t, float>))                   \
    F(F32X4ConvertI32X4U, (simdConvertOperation<uint32_t, float>))                  \
    F(F32X4DemoteF64X2Zero, (simdDemoteZeroOperation))                              \
    F(F32X4Splat, (simdSplatOperation<float, float>))                               \
    F(F64X2Splat, (simdSplatOperation<double, double>))

#define FOR_EACH_BYTECODE_SIMD_LOAD_SPLAT_OP(F) \
    F(V128Load8Splat, uint8_t)                  \
    F(V128Load16Splat, uint16_t)                \
    F(V128Load32Splat, uint32_t)                \
    F(V128Load64Splat, uint64_t)

#define FOR_EACH_BYTECODE_SIMD_LOAD_EXTEND_OP(F) \
    F(V128Load8X8S, S8x8, int16_t)               \
    F(V128Load8X8U, U8x8, uint16_t)              \
    F(V128Load16X4S, S16x4, int32_t)             \
    F(V128Load16X4U, U16x4, uint32_t)            \
    F(V128Load32X2S, S32x2, int64_t)             \
    F(V128Load32X2U, U32x2, uint64_t)

#define FOR_EACH_BYTECODE_SIMD_LOAD_LANE_OP(F) \
    F(V128Load8Lane, uint8_t)                  \
    F(V128Load16Lane, uint16_t)                \
    F(V128Load32Lane, uint32_t)                \
    F(V128Load64Lane, uint64_t)

#define FOR_EACH_BYTECODE_SIMD_STORE_LANE_OP(F) \
    F(V128Store8Lane, uint8_t)                  \
    F(V128Store16Lane, uint16_t)                \
    F(V128Store32Lane, uint32_t)                \
    F(V128Store64Lane, uint64_t)

#define FOR_EACH_BYTECODE_SIMD_EXTRACT_LANE_OP(F) \
    F(I8X16ExtractLaneS, int8_t, int32_t)         \
    F(I8X16ExtractLaneU, uint8_t, uint32_t)       \
    F(I16X8ExtractLaneS, int16_t, int32_t)        \
    F(I16X8ExtractLaneU, uint16_t, uint32_t)      \
    F(I32X4ExtractLane, int32_t, uint32_t)        \
    F(I64X2ExtractLane, uint64_t, uint64_t)       \
    F(F32X4ExtractLane, float, float)             \
    F(F64X2ExtractLane, double, double)

#define FOR_EACH_BYTECODE_SIMD_REPLACE_LANE_OP(F) \
    F(I8X16ReplaceLane, uint32_t, uint8_t)        \
    F(I16X8ReplaceLane, uint32_t, uint16_t)       \
    F(I32X4ReplaceLane, uint32_t, uint32_t)       \
    F(I64X2ReplaceLane, uint64_t, uint64_t)       \
    F(F32X4ReplaceLane, float, float)             \
    F(F64X2ReplaceLane, double, double)

#define FOR_EACH_BYTECODE_SIMD_ETC_OP(F) \
    F(V128BitSelect)                     \
    F(V128Load32Zero)                    \
    F(V128Load64Zero)                    \
    F(I8X16Shuffle)

// Extended Features
#define FOR_EACH_BYTECODE_ATOMIC_LOAD_OP(F) \
    F(I32AtomicLoad, uint32_t, uint32_t)    \
    F(I64AtomicLoad, uint64_t, uint64_t)    \
    F(I32AtomicLoad8U, uint8_t, uint32_t)   \
    F(I32AtomicLoad16U, uint16_t, uint32_t) \
    F(I64AtomicLoad8U, uint8_t, uint64_t)   \
    F(I64AtomicLoad16U, uint16_t, uint64_t) \
    F(I64AtomicLoad32U, uint32_t, uint64_t)

#define FOR_EACH_BYTECODE_ATOMIC_STORE_OP(F) \
    F(I32AtomicStore, uint32_t, uint32_t)    \
    F(I64AtomicStore, uint64_t, uint64_t)    \
    F(I32AtomicStore8, uint32_t, uint8_t)    \
    F(I32AtomicStore16, uint32_t, uint16_t)  \
    F(I64AtomicStore8, uint64_t, uint8_t)    \
    F(I64AtomicStore16, uint64_t, uint16_t)  \
    F(I64AtomicStore32, uint64_t, uint32_t)

#define FOR_EACH_BYTECODE_ATOMIC_RMW_OP(F)                                \
    F(I64AtomicRmwAdd, uint64_t, uint64_t, Memory::AtomicRmwOp::Add)      \
    F(I64AtomicRmw8AddU, uint64_t, uint8_t, Memory::AtomicRmwOp::Add)     \
    F(I64AtomicRmw16AddU, uint64_t, uint16_t, Memory::AtomicRmwOp::Add)   \
    F(I64AtomicRmw32AddU, uint64_t, uint32_t, Memory::AtomicRmwOp::Add)   \
    F(I32AtomicRmwAdd, uint32_t, uint32_t, Memory::AtomicRmwOp::Add)      \
    F(I32AtomicRmw8AddU, uint32_t, uint8_t, Memory::AtomicRmwOp::Add)     \
    F(I32AtomicRmw16AddU, uint32_t, uint16_t, Memory::AtomicRmwOp::Add)   \
                                                                          \
    F(I64AtomicRmwSub, uint64_t, uint64_t, Memory::AtomicRmwOp::Sub)      \
    F(I64AtomicRmw8SubU, uint64_t, uint8_t, Memory::AtomicRmwOp::Sub)     \
    F(I64AtomicRmw16SubU, uint64_t, uint16_t, Memory::AtomicRmwOp::Sub)   \
    F(I64AtomicRmw32SubU, uint64_t, uint32_t, Memory::AtomicRmwOp::Sub)   \
    F(I32AtomicRmwSub, uint32_t, uint32_t, Memory::AtomicRmwOp::Sub)      \
    F(I32AtomicRmw8SubU, uint32_t, uint8_t, Memory::AtomicRmwOp::Sub)     \
    F(I32AtomicRmw16SubU, uint32_t, uint16_t, Memory::AtomicRmwOp::Sub)   \
                                                                          \
    F(I64AtomicRmwAnd, uint64_t, uint64_t, Memory::AtomicRmwOp::And)      \
    F(I64AtomicRmw8AndU, uint64_t, uint8_t, Memory::AtomicRmwOp::And)     \
    F(I64AtomicRmw16AndU, uint64_t, uint16_t, Memory::AtomicRmwOp::And)   \
    F(I64AtomicRmw32AndU, uint64_t, uint32_t, Memory::AtomicRmwOp::And)   \
    F(I32AtomicRmwAnd, uint32_t, uint32_t, Memory::AtomicRmwOp::And)      \
    F(I32AtomicRmw8AndU, uint32_t, uint8_t, Memory::AtomicRmwOp::And)     \
    F(I32AtomicRmw16AndU, uint32_t, uint16_t, Memory::AtomicRmwOp::And)   \
                                                                          \
    F(I64AtomicRmwOr, uint64_t, uint64_t, Memory::AtomicRmwOp::Or)        \
    F(I64AtomicRmw8OrU, uint64_t, uint8_t, Memory::AtomicRmwOp::Or)       \
    F(I64AtomicRmw16OrU, uint64_t, uint16_t, Memory::AtomicRmwOp::Or)     \
    F(I64AtomicRmw32OrU, uint64_t, uint32_t, Memory::AtomicRmwOp::Or)     \
    F(I32AtomicRmwOr, uint32_t, uint32_t, Memory::AtomicRmwOp::Or)        \
    F(I32AtomicRmw8OrU, uint32_t, uint8_t, Memory::AtomicRmwOp::Or)       \
    F(I32AtomicRmw16OrU, uint32_t, uint16_t, Memory::AtomicRmwOp::Or)     \
                                                                          \
    F(I64AtomicRmwXor, uint64_t, uint64_t, Memory::AtomicRmwOp::Xor)      \
    F(I64AtomicRmw8XorU, uint64_t, uint8_t, Memory::AtomicRmwOp::Xor)     \
    F(I64AtomicRmw16XorU, uint64_t, uint16_t, Memory::AtomicRmwOp::Xor)   \
    F(I64AtomicRmw32XorU, uint64_t, uint32_t, Memory::AtomicRmwOp::Xor)   \
    F(I32AtomicRmwXor, uint32_t, uint32_t, Memory::AtomicRmwOp::Xor)      \
    F(I32AtomicRmw8XorU, uint32_t, uint8_t, Memory::AtomicRmwOp::Xor)     \
    F(I32AtomicRmw16XorU, uint32_t, uint16_t, Memory::AtomicRmwOp::Xor)   \
                                                                          \
    F(I64AtomicRmwXchg, uint64_t, uint64_t, Memory::AtomicRmwOp::Xchg)    \
    F(I64AtomicRmw8XchgU, uint64_t, uint8_t, Memory::AtomicRmwOp::Xchg)   \
    F(I64AtomicRmw16XchgU, uint64_t, uint16_t, Memory::AtomicRmwOp::Xchg) \
    F(I64AtomicRmw32XchgU, uint64_t, uint32_t, Memory::AtomicRmwOp::Xchg) \
    F(I32AtomicRmwXchg, uint32_t, uint32_t, Memory::AtomicRmwOp::Xchg)    \
    F(I32AtomicRmw8XchgU, uint32_t, uint8_t, Memory::AtomicRmwOp::Xchg)   \
    F(I32AtomicRmw16XchgU, uint32_t, uint16_t, Memory::AtomicRmwOp::Xchg)

#define FOR_EACH_BYTECODE_ATOMIC_RMW_CMPXCHG_OP(F) \
    F(I32AtomicRmwCmpxchg, uint32_t, uint32_t)     \
    F(I64AtomicRmwCmpxchg, uint64_t, uint64_t)     \
    F(I32AtomicRmw8CmpxchgU, uint32_t, uint8_t)    \
    F(I32AtomicRmw16CmpxchgU, uint32_t, uint16_t)  \
    F(I64AtomicRmw8CmpxchgU, uint64_t, uint8_t)    \
    F(I64AtomicRmw16CmpxchgU, uint64_t, uint16_t)  \
    F(I64AtomicRmw32CmpxchgU, uint64_t, uint32_t)

#define FOR_EACH_BYTECODE_ATOMIC_OTHER(F) \
    F(MemoryAtomicNotify)                 \
    F(MemoryAtomicWait32)                 \
    F(MemoryAtomicWait64)                 \
    F(AtomicFence)

#define FOR_EACH_BYTECODE_RELAXED_SIMD_UNARY_OTHER(F)                            \
    F(I32X4RelaxedTruncF32X4S, (simdTruncSatOperation<float, int32_t>))          \
    F(I32X4RelaxedTruncF32X4U, (simdTruncSatOperation<float, uint32_t>))         \
    F(I32X4RelaxedTruncF64X2SZero, (simdTruncSatZeroOperation<double, int32_t>)) \
    F(I32X4RelaxedTruncF64X2UZero, (simdTruncSatZeroOperation<double, uint32_t>))

#define FOR_EACH_BYTECODE_RELAXED_SIMD_BINARY_OP(F) \
    F(F32X4RelaxedMin, floatMin, float, float)      \
    F(F32X4RelaxedMax, floatMax, float, float)      \
    F(F64X2RelaxedMin, floatMin, double, double)    \
    F(F64X2RelaxedMax, floatMax, double, double)    \
    F(I16X8RelaxedQ15mulrS, saturatingRoundingQMul, int16_t, int16_t)

#define FOR_EACH_BYTECODE_RELAXED_SIMD_BINARY_OTHER(F)      \
    F(I8X16RelaxedSwizzle, (simdSwizzleOperation<uint8_t>)) \
    F(I16X8DotI8X16I7X16S, (simdDotOperation<int8_t, uint16_t>))

#define FOR_EACH_BYTECODE_RELAXED_SIMD_TERNARY_OP(F)   \
    F(F32X4RelaxedMadd, floatMulAdd, float, float)     \
    F(F32X4RelaxedNmadd, floatNegMulAdd, float, float) \
    F(F64X2RelaxedMadd, floatMulAdd, double, double)   \
    F(F64X2RelaxedNmadd, floatNegMulAdd, double, double)

#define FOR_EACH_BYTECODE_RELAXED_SIMD_TERNARY_OTHER(F) \
    F(I32X4DotI8X16I7X16AddS, (simdDotAddOperation))    \
    F(I8X16RelaxedLaneSelect, (simdBitSelectOperation)) \
    F(I16X8RelaxedLaneSelect, (simdBitSelectOperation)) \
    F(I32X4RelaxedLaneSelect, (simdBitSelectOperation)) \
    F(I64X2RelaxedLaneSelect, (simdBitSelectOperation))

#define FOR_EACH_BYTECODE(F)                        \
    FOR_EACH_BYTECODE_OP(F)                         \
    FOR_EACH_BYTECODE_BINARY_OP(F)                  \
    FOR_EACH_BYTECODE_UNARY_OP(F)                   \
    FOR_EACH_BYTECODE_UNARY_OP_2(F)                 \
    FOR_EACH_BYTECODE_LOAD_OP(F)                    \
    FOR_EACH_BYTECODE_STORE_OP(F)                   \
    FOR_EACH_BYTECODE_SIMD_BINARY_OP(F)             \
    FOR_EACH_BYTECODE_SIMD_BINARY_SHIFT_OP(F)       \
    FOR_EACH_BYTECODE_SIMD_BINARY_OTHER(F)          \
    FOR_EACH_BYTECODE_RELAXED_SIMD_BINARY_OP(F)     \
    FOR_EACH_BYTECODE_RELAXED_SIMD_BINARY_OTHER(F)  \
    FOR_EACH_BYTECODE_SIMD_UNARY_OP(F)              \
    FOR_EACH_BYTECODE_SIMD_UNARY_CONVERT_OP(F)      \
    FOR_EACH_BYTECODE_RELAXED_SIMD_UNARY_OTHER(F)   \
    FOR_EACH_BYTECODE_RELAXED_SIMD_TERNARY_OP(F)    \
    FOR_EACH_BYTECODE_RELAXED_SIMD_TERNARY_OTHER(F) \
    FOR_EACH_BYTECODE_SIMD_UNARY_OTHER(F)           \
    FOR_EACH_BYTECODE_SIMD_LOAD_SPLAT_OP(F)         \
    FOR_EACH_BYTECODE_SIMD_LOAD_EXTEND_OP(F)        \
    FOR_EACH_BYTECODE_SIMD_LOAD_LANE_OP(F)          \
    FOR_EACH_BYTECODE_SIMD_STORE_LANE_OP(F)         \
    FOR_EACH_BYTECODE_SIMD_EXTRACT_LANE_OP(F)       \
    FOR_EACH_BYTECODE_SIMD_REPLACE_LANE_OP(F)       \
    FOR_EACH_BYTECODE_SIMD_ETC_OP(F)                \
    FOR_EACH_BYTECODE_ATOMIC_LOAD_OP(F)             \
    FOR_EACH_BYTECODE_ATOMIC_STORE_OP(F)            \
    FOR_EACH_BYTECODE_ATOMIC_RMW_OP(F)              \
    FOR_EACH_BYTECODE_ATOMIC_RMW_CMPXCHG_OP(F)      \
    FOR_EACH_BYTECODE_ATOMIC_OTHER(F)

class ByteCode {
public:
    // clang-format off
    enum Opcode : uint32_t {
#define DECLARE_BYTECODE(name, ...) name## Opcode,
        FOR_EACH_BYTECODE(DECLARE_BYTECODE)
#undef DECLARE_BYTECODE
        OpcodeKindEnd,
    };
    // clang-format on

    static size_t pointerAlignedSize(const size_t originalSize)
    {
        return (originalSize + (sizeof(void*) - 1)) & ~(sizeof(void*) - 1);
    }

    Opcode opcode() const;
    size_t getSize() const;

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

class ByteCodeOffset2 : public ByteCode {
public:
    ByteCodeOffset2(Opcode opcode, ByteCodeStackOffset stackOffset1, ByteCodeStackOffset stackOffset2)
        : ByteCode(opcode)
        , m_stackOffset1(stackOffset1)
        , m_stackOffset2(stackOffset2)
    {
    }

    ByteCodeStackOffset stackOffset1() const { return m_stackOffset1; }
    ByteCodeStackOffset stackOffset2() const { return m_stackOffset2; }

protected:
    ByteCodeStackOffset m_stackOffset1;
    ByteCodeStackOffset m_stackOffset2;
};

class ByteCodeOffset3 : public ByteCode {
public:
    ByteCodeOffset3(Opcode opcode, ByteCodeStackOffset stackOffset1, ByteCodeStackOffset stackOffset2, ByteCodeStackOffset stackOffset3)
        : ByteCode(opcode)
        , m_stackOffsets{ stackOffset1, stackOffset2, stackOffset3 }
    {
    }

    const ByteCodeStackOffset* stackOffsets() const { return m_stackOffsets; }
    ByteCodeStackOffset stackOffset1() const { return m_stackOffsets[0]; }
    ByteCodeStackOffset stackOffset2() const { return m_stackOffsets[1]; }
    ByteCodeStackOffset stackOffset3() const { return m_stackOffsets[2]; }

protected:
    ByteCodeStackOffset m_stackOffsets[3];
};

class ByteCodeOffsetValue : public ByteCode {
public:
    ByteCodeOffsetValue(Opcode opcode, ByteCodeStackOffset stackOffset, uint32_t value)
        : ByteCode(opcode)
        , m_stackOffset(stackOffset)
        , m_value(value)
    {
    }

    ByteCodeStackOffset stackOffset() const { return m_stackOffset; }
    uint32_t uint32Value() const { return m_value; }
    int32_t int32Value() const { return static_cast<int32_t>(m_value); }

protected:
    ByteCodeStackOffset m_stackOffset;
    uint32_t m_value;
};

class ByteCodeOffset2Value : public ByteCode {
public:
    ByteCodeOffset2Value(Opcode opcode, ByteCodeStackOffset stackOffset1, ByteCodeStackOffset stackOffset2, uint32_t value)
        : ByteCode(opcode)
        , m_stackOffset1(stackOffset1)
        , m_stackOffset2(stackOffset2)
        , m_value(value)
    {
    }

    ByteCodeStackOffset stackOffset1() const { return m_stackOffset1; }
    ByteCodeStackOffset stackOffset2() const { return m_stackOffset2; }
    uint32_t uint32Value() const { return m_value; }
    int32_t int32Value() const { return static_cast<int32_t>(m_value); }

protected:
    ByteCodeStackOffset m_stackOffset1;
    ByteCodeStackOffset m_stackOffset2;
    uint32_t m_value;
};

class ByteCodeOffset4 : public ByteCode {
public:
    ByteCodeOffset4(Opcode opcode, ByteCodeStackOffset src0Offset, ByteCodeStackOffset src1Offset, ByteCodeStackOffset src2Offset, ByteCodeStackOffset dstOffset)
        : ByteCode(opcode)
        , m_stackOffsets{ src0Offset, src1Offset, src2Offset, dstOffset }
    {
    }

    const ByteCodeStackOffset* srcOffsets() const { return m_stackOffsets; }
    ByteCodeStackOffset src0Offset() const { return m_stackOffsets[0]; }
    ByteCodeStackOffset src1Offset() const { return m_stackOffsets[1]; }
    ByteCodeStackOffset src2Offset() const { return m_stackOffsets[2]; }
    ByteCodeStackOffset dstOffset() const { return m_stackOffsets[3]; }

protected:
    ByteCodeStackOffset m_stackOffsets[4];
};

class ByteCodeOffset4Value : public ByteCode {
public:
    ByteCodeOffset4Value(Opcode opcode, ByteCodeStackOffset src0Offset, ByteCodeStackOffset src1Offset, ByteCodeStackOffset src2Offset, ByteCodeStackOffset dstOffset, uint32_t value)
        : ByteCode(opcode)
        , m_stackOffset1(src0Offset)
        , m_stackOffset2(src1Offset)
        , m_stackOffset3(src2Offset)
        , m_stackOffset4(dstOffset)
        , m_value(value)
    {
    }

    ByteCodeStackOffset src0Offset() const { return m_stackOffset1; }
    ByteCodeStackOffset src1Offset() const { return m_stackOffset2; }
    ByteCodeStackOffset src2Offset() const { return m_stackOffset3; }
    ByteCodeStackOffset dstOffset() const { return m_stackOffset4; }
    uint32_t offset() const { return m_value; }

protected:
    ByteCodeStackOffset m_stackOffset1;
    ByteCodeStackOffset m_stackOffset2;
    ByteCodeStackOffset m_stackOffset3;
    ByteCodeStackOffset m_stackOffset4;
    uint32_t m_value;
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
    const uint8_t* value() const { return reinterpret_cast<const uint8_t*>(m_value); }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("const128 ");
        DUMP_BYTECODE_OFFSET(dstOffset);
    }
#endif

protected:
    ByteCodeStackOffset m_dstOffset;
    // Maintains 32 bit alignment.
    uint32_t m_value[4];
};

// dummy ByteCode for binary operation
class BinaryOperation : public ByteCodeOffset3 {
public:
    BinaryOperation(Opcode code, ByteCodeStackOffset src0Offset, ByteCodeStackOffset src1Offset, ByteCodeStackOffset dstOffset)
        : ByteCodeOffset3(code, src0Offset, src1Offset, dstOffset)
    {
    }

    const ByteCodeStackOffset* srcOffset() const { return stackOffsets(); }
    ByteCodeStackOffset dstOffset() const { return stackOffset3(); }
    void setDstOffset(ByteCodeStackOffset o) { m_stackOffsets[2] = o; }
#if !defined(NDEBUG)
    void dump(size_t pos)
    {
    }
#endif
};

#if !defined(NDEBUG)
#define DEFINE_BINARY_BYTECODE_DUMP(name)                                                                                                                          \
    void dump(size_t pos)                                                                                                                                          \
    {                                                                                                                                                              \
        printf(#name " src1: %" PRIu32 " src2: %" PRIu32 " dst: %" PRIu32, (uint32_t)m_stackOffsets[0], (uint32_t)m_stackOffsets[1], (uint32_t)m_stackOffsets[2]); \
    }
#else
#define DEFINE_BINARY_BYTECODE_DUMP(name)
#endif

#define DEFINE_BINARY_BYTECODE(name, ...)                                                                   \
    class name : public BinaryOperation {                                                                   \
    public:                                                                                                 \
        name(ByteCodeStackOffset src0Offset, ByteCodeStackOffset src1Offset, ByteCodeStackOffset dstOffset) \
            : BinaryOperation(Opcode::name##Opcode, src0Offset, src1Offset, dstOffset)                      \
        {                                                                                                   \
        }                                                                                                   \
        DEFINE_BINARY_BYTECODE_DUMP(name)                                                                   \
    };

// dummy ByteCode for unary operation
class UnaryOperation : public ByteCodeOffset2 {
public:
    UnaryOperation(Opcode code, ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset)
        : ByteCodeOffset2(code, srcOffset, dstOffset)
    {
    }

    ByteCodeStackOffset srcOffset() const { return stackOffset1(); }
    ByteCodeStackOffset dstOffset() const { return stackOffset2(); }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
    }
#endif
};

#if !defined(NDEBUG)
#define DEFINE_UNARY_BYTECODE_DUMP(name)                                                                     \
    void dump(size_t pos)                                                                                    \
    {                                                                                                        \
        printf(#name " src: %" PRIu32 " dst: %" PRIu32, (uint32_t)m_stackOffset1, (uint32_t)m_stackOffset2); \
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

// dummy ByteCode for ternary operation
class TernaryOperation : public ByteCodeOffset4 {
public:
    TernaryOperation(Opcode code, ByteCodeStackOffset src0Offset, ByteCodeStackOffset src1Offset, ByteCodeStackOffset src2Offset, ByteCodeStackOffset dstOffset)
        : ByteCodeOffset4(code, src0Offset, src1Offset, src2Offset, dstOffset)
    {
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
    }
#endif
};

#if !defined(NDEBUG)
#define DEFINE_TERNARY_BYTECODE_DUMP(name)                                                                                                                                                                        \
    void dump(size_t pos)                                                                                                                                                                                         \
    {                                                                                                                                                                                                             \
        printf(#name " src1: %" PRIu32 " src2: %" PRIu32 " src3: %" PRIu32 " dst: %" PRIu32, (uint32_t)m_stackOffsets[0], (uint32_t)m_stackOffsets[1], (uint32_t)m_stackOffsets[2], (uint32_t)m_stackOffsets[3]); \
    }
#else
#define DEFINE_TERNARY_BYTECODE_DUMP(name)
#endif

#define DEFINE_TERNARY_BYTECODE(name, ...)                                                                                                  \
    class name : public TernaryOperation {                                                                                                  \
    public:                                                                                                                                 \
        name(ByteCodeStackOffset src0Offset, ByteCodeStackOffset src1Offset, ByteCodeStackOffset src2Offset, ByteCodeStackOffset dstOffset) \
            : TernaryOperation(Opcode::name##Opcode, src0Offset, src1Offset, src2Offset, dstOffset)                                         \
        {                                                                                                                                   \
        }                                                                                                                                   \
        DEFINE_TERNARY_BYTECODE_DUMP(name)                                                                                                  \
    };


FOR_EACH_BYTECODE_BINARY_OP(DEFINE_BINARY_BYTECODE)
FOR_EACH_BYTECODE_UNARY_OP(DEFINE_UNARY_BYTECODE)
FOR_EACH_BYTECODE_UNARY_OP_2(DEFINE_UNARY_BYTECODE)
FOR_EACH_BYTECODE_SIMD_BINARY_OP(DEFINE_BINARY_BYTECODE)
FOR_EACH_BYTECODE_SIMD_BINARY_SHIFT_OP(DEFINE_BINARY_BYTECODE)
FOR_EACH_BYTECODE_SIMD_BINARY_OTHER(DEFINE_BINARY_BYTECODE)
FOR_EACH_BYTECODE_RELAXED_SIMD_BINARY_OP(DEFINE_BINARY_BYTECODE)
FOR_EACH_BYTECODE_RELAXED_SIMD_BINARY_OTHER(DEFINE_BINARY_BYTECODE)
FOR_EACH_BYTECODE_SIMD_UNARY_OP(DEFINE_UNARY_BYTECODE)
FOR_EACH_BYTECODE_SIMD_UNARY_CONVERT_OP(DEFINE_UNARY_BYTECODE)
FOR_EACH_BYTECODE_SIMD_UNARY_OTHER(DEFINE_UNARY_BYTECODE)
FOR_EACH_BYTECODE_RELAXED_SIMD_UNARY_OTHER(DEFINE_UNARY_BYTECODE)
FOR_EACH_BYTECODE_RELAXED_SIMD_TERNARY_OP(DEFINE_TERNARY_BYTECODE)
FOR_EACH_BYTECODE_RELAXED_SIMD_TERNARY_OTHER(DEFINE_TERNARY_BYTECODE)

#define DEFINE_MOVE_BYTECODE(name)                                         \
    class name : public ByteCodeOffset2 {                                  \
    public:                                                                \
        name(ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset) \
            : ByteCodeOffset2(Opcode::name##Opcode, srcOffset, dstOffset)  \
        {                                                                  \
        }                                                                  \
        ByteCodeStackOffset srcOffset() const { return stackOffset1(); }   \
        ByteCodeStackOffset dstOffset() const { return stackOffset2(); }   \
        DEFINE_UNARY_BYTECODE_DUMP(name)                                   \
    };

DEFINE_MOVE_BYTECODE(MoveI32)
DEFINE_MOVE_BYTECODE(MoveF32)
DEFINE_MOVE_BYTECODE(MoveI64)
DEFINE_MOVE_BYTECODE(MoveF64)
DEFINE_MOVE_BYTECODE(MoveV128)
DEFINE_MOVE_BYTECODE(I32ReinterpretF32)
DEFINE_MOVE_BYTECODE(I64ReinterpretF64)
DEFINE_MOVE_BYTECODE(F32ReinterpretI32)
DEFINE_MOVE_BYTECODE(F64ReinterpretI64)

#undef DEFINE_BINARY_BYTECODE_DUMP
#undef DEFINE_BINARY_BYTECODE
#undef DEFINE_UNARY_BYTECODE_DUMP
#undef DEFINE_UNARY_BYTECODE

class Call : public ByteCode {
public:
    Call(uint32_t index, uint16_t parameterOffsetsSize, uint16_t resultOffsetsSize)
        : ByteCode(Opcode::CallOpcode)
        , m_index(index)
        , m_parameterOffsetsSize(parameterOffsetsSize)
        , m_resultOffsetsSize(resultOffsetsSize)
    {
    }

    uint32_t index() const { return m_index; }
    ByteCodeStackOffset* stackOffsets() const
    {
        return reinterpret_cast<ByteCodeStackOffset*>(reinterpret_cast<size_t>(this) + sizeof(Call));
    }

    uint16_t parameterOffsetsSize() const
    {
        return m_parameterOffsetsSize;
    }

    uint16_t resultOffsetsSize() const
    {
        return m_resultOffsetsSize;
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("call ");
        printf("index: %" PRId32 " ", m_index);
        size_t c = 0;
        auto arr = stackOffsets();
        printf("paramOffsets: ");
        for (size_t i = 0; i < m_parameterOffsetsSize; i++) {
            printf("%" PRIu32 " ", (uint32_t)arr[c++]);
        }
        printf(" ");

        printf("resultOffsets: ");
        for (size_t i = 0; i < m_resultOffsetsSize; i++) {
            printf("%" PRIu32 " ", (uint32_t)arr[c++]);
        }
    }
#endif

protected:
    uint32_t m_index;
    uint16_t m_parameterOffsetsSize;
    uint16_t m_resultOffsetsSize;
};

class CallIndirect : public ByteCode {
public:
    CallIndirect(ByteCodeStackOffset stackOffset, uint32_t tableIndex, FunctionType* functionType,
                 uint16_t parameterOffsetsSize, uint16_t resultOffsetsSize)
        : ByteCode(Opcode::CallIndirectOpcode)
        , m_calleeOffset(stackOffset)
        , m_tableIndex(tableIndex)
        , m_functionType(functionType)
        , m_parameterOffsetsSize(parameterOffsetsSize)
        , m_resultOffsetsSize(resultOffsetsSize)
    {
    }

    ByteCodeStackOffset calleeOffset() const { return m_calleeOffset; }
    uint32_t tableIndex() const { return m_tableIndex; }
    FunctionType* functionType() const { return m_functionType; }
    ByteCodeStackOffset* stackOffsets() const
    {
        return reinterpret_cast<ByteCodeStackOffset*>(reinterpret_cast<size_t>(this) + sizeof(CallIndirect));
    }

    uint16_t parameterOffsetsSize() const
    {
        return m_parameterOffsetsSize;
    }

    uint16_t resultOffsetsSize() const
    {
        return m_resultOffsetsSize;
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
        for (size_t i = 0; i < m_parameterOffsetsSize; i++) {
            printf("%" PRIu32 " ", (uint32_t)arr[c++]);
        }
        printf(" ");

        printf("resultOffsets: ");
        for (size_t i = 0; i < m_resultOffsetsSize; i++) {
            printf("%" PRIu32 " ", (uint32_t)arr[c++]);
        }
    }
#endif

protected:
    ByteCodeStackOffset m_calleeOffset;
    uint32_t m_tableIndex;
    FunctionType* m_functionType;
    uint16_t m_parameterOffsetsSize;
    uint16_t m_resultOffsetsSize;
};

class Load32 : public ByteCodeOffset2 {
public:
    Load32(ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset)
        : ByteCodeOffset2(Load32Opcode, srcOffset, dstOffset)
    {
    }

    ByteCodeStackOffset srcOffset() const { return stackOffset1(); }
    ByteCodeStackOffset dstOffset() const { return stackOffset2(); }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("load32 ");
        DUMP_BYTECODE_OFFSET(stackOffset1);
        DUMP_BYTECODE_OFFSET(stackOffset2);
    }
#endif
};

class Load32Multi : public ByteCodeOffset2 {
public:
    Load32Multi(uint32_t index, uint32_t alignment, ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset)
        : ByteCodeOffset2(Load32Opcode, srcOffset, dstOffset)
        , m_memIndex(index)
        , m_alignment(alignment)
    {
    }

    ByteCodeStackOffset srcOffset() const { return stackOffset1(); }
    ByteCodeStackOffset dstOffset() const { return stackOffset2(); }
    uint16_t memIndex() const { return m_memIndex; }
    uint16_t alignment() const { return m_alignment; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("load32 ");
        DUMP_BYTECODE_OFFSET(stackOffset1);
        DUMP_BYTECODE_OFFSET(stackOffset2);
        printf("memIndex: %" PRIu16, m_memIndex);
        printf("alignment: %" PRIu16, m_alignment);
    }
#endif

protected:
    uint16_t m_memIndex;
    uint16_t m_alignment;
};

class Load64 : public ByteCodeOffset2 {
public:
    Load64(ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset)
        : ByteCodeOffset2(Load64Opcode, srcOffset, dstOffset)
    {
    }

    ByteCodeStackOffset srcOffset() const { return stackOffset1(); }
    ByteCodeStackOffset dstOffset() const { return stackOffset2(); }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("load64 ");
        DUMP_BYTECODE_OFFSET(stackOffset1);
        DUMP_BYTECODE_OFFSET(stackOffset2);
    }
#endif
};

class Load64Multi : public ByteCodeOffset2 {
public:
    Load64Multi(uint32_t index, uint32_t alignment, ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset)
        : ByteCodeOffset2(Load64Opcode, srcOffset, dstOffset)
        , m_memIndex(index)
        , m_alignment(alignment)
    {
    }

    ByteCodeStackOffset srcOffset() const { return stackOffset1(); }
    ByteCodeStackOffset dstOffset() const { return stackOffset2(); }
    uint16_t memIndex() const { return m_memIndex; }
    uint16_t alignment() const { return m_alignment; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("load64 ");
        DUMP_BYTECODE_OFFSET(stackOffset1);
        DUMP_BYTECODE_OFFSET(stackOffset2);
        printf("memIndex: %" PRIu16, m_memIndex);
        printf("alignment: %" PRIu16, m_alignment);
    }
#endif

protected:
    uint16_t m_memIndex;
    uint16_t m_alignment;
};

class Store32 : public ByteCodeOffset2 {
public:
    Store32(ByteCodeStackOffset src0Offset, ByteCodeStackOffset src1Offset)
        : ByteCodeOffset2(Store32Opcode, src0Offset, src1Offset)
    {
    }

    ByteCodeStackOffset src0Offset() const { return stackOffset1(); }
    ByteCodeStackOffset src1Offset() const { return stackOffset2(); }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("store32 ");
        DUMP_BYTECODE_OFFSET(stackOffset1);
        DUMP_BYTECODE_OFFSET(stackOffset2);
    }
#endif
};

class Store32Multi : public ByteCodeOffset2 {
public:
    Store32Multi(uint32_t index, uint32_t alignment, ByteCodeStackOffset src0Offset, ByteCodeStackOffset src1Offset)
        : ByteCodeOffset2(Store32Opcode, src0Offset, src1Offset)
        , m_memIndex(index)
        , m_alignment(alignment)
    {
    }

    ByteCodeStackOffset src0Offset() const { return stackOffset1(); }
    ByteCodeStackOffset src1Offset() const { return stackOffset2(); }
    uint16_t memIndex() const { return m_memIndex; }
    uint16_t alignment() const { return m_alignment; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("store32 ");
        DUMP_BYTECODE_OFFSET(stackOffset1);
        DUMP_BYTECODE_OFFSET(stackOffset2);
        printf("memIndex: %" PRIu16, m_memIndex);
        printf("alignment: %" PRIu16, m_alignment);
    }
#endif

protected:
    uint16_t m_memIndex;
    uint16_t m_alignment;
};

class Store64 : public ByteCodeOffset2 {
public:
    Store64(ByteCodeStackOffset src0Offset, ByteCodeStackOffset src1Offset)
        : ByteCodeOffset2(Store64Opcode, src0Offset, src1Offset)
    {
    }

    ByteCodeStackOffset src0Offset() const { return stackOffset1(); }
    ByteCodeStackOffset src1Offset() const { return stackOffset2(); }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("store64 ");
        DUMP_BYTECODE_OFFSET(stackOffset1);
        DUMP_BYTECODE_OFFSET(stackOffset2);
    }
#endif
};

class Store64Multi : public ByteCodeOffset2 {
public:
    Store64Multi(uint32_t index, uint32_t alignment, ByteCodeStackOffset src0Offset, ByteCodeStackOffset src1Offset)
        : ByteCodeOffset2(Store64Opcode, src0Offset, src1Offset)
        , m_memIndex(index)
        , m_alignment(alignment)
    {
    }

    ByteCodeStackOffset src0Offset() const { return stackOffset1(); }
    ByteCodeStackOffset src1Offset() const { return stackOffset2(); }
    uint16_t memIndex() const { return m_memIndex; }
    uint16_t alignment() const { return m_alignment; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("store64 ");
        DUMP_BYTECODE_OFFSET(stackOffset1);
        DUMP_BYTECODE_OFFSET(stackOffset2);
        printf("memIndex: %" PRIu16, m_memIndex);
        printf("alignment: %" PRIu16, m_alignment);
    }
#endif

protected:
    uint16_t m_memIndex;
    uint16_t m_alignment;
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

class JumpIfTrue : public ByteCodeOffsetValue {
public:
    JumpIfTrue(ByteCodeStackOffset srcOffset, int32_t offset = 0)
        : ByteCodeOffsetValue(Opcode::JumpIfTrueOpcode, srcOffset, static_cast<uint32_t>(offset))
    {
    }

    ByteCodeStackOffset srcOffset() const { return stackOffset(); }
    int32_t offset() const { return int32Value(); }
    void setOffset(int32_t offset)
    {
        m_value = static_cast<uint32_t>(offset);
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("jump_if_true ");
        DUMP_BYTECODE_OFFSET(stackOffset);
        printf("dst: %" PRId32, (int32_t)pos + offset());
    }
#endif
};

class JumpIfFalse : public ByteCodeOffsetValue {
public:
    JumpIfFalse(ByteCodeStackOffset srcOffset, int32_t offset = 0)
        : ByteCodeOffsetValue(Opcode::JumpIfFalseOpcode, srcOffset, static_cast<uint32_t>(offset))
    {
    }

    ByteCodeStackOffset srcOffset() const { return stackOffset(); }
    int32_t offset() const { return int32Value(); }
    void setOffset(int32_t offset)
    {
        m_value = static_cast<uint32_t>(offset);
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("jump_if_false ");
        DUMP_BYTECODE_OFFSET(stackOffset);
        printf("dst: %" PRId32, (int32_t)pos + offset());
    }
#endif
};

class Select : public ByteCode {
public:
    Select(ByteCodeStackOffset condOffset, uint16_t size, bool isFloat, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset dst)
        : ByteCode(Opcode::SelectOpcode)
        , m_condOffset(condOffset)
        , m_valueSize(size)
        , m_isFloat(isFloat)
        , m_src0Offset(src0)
        , m_src1Offset(src1)
        , m_dstOffset(dst)
    {
    }

    ByteCodeStackOffset condOffset() const { return m_condOffset; }
    uint16_t valueSize() const { return m_valueSize; }
    bool isFloat() const { return m_isFloat != 0; }
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
    uint8_t m_valueSize;
    uint8_t m_isFloat;
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

class MemorySizeMulti : public ByteCode {
public:
    MemorySizeMulti(uint32_t index, ByteCodeStackOffset dstOffset)
        : ByteCode(Opcode::MemorySizeOpcode)
        , m_dstOffset(dstOffset)
    {
        ASSERT(index != 0);
    }

    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }
    uint16_t memIndex() const { return m_memIndex; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("memory.size ");
        DUMP_BYTECODE_OFFSET(dstOffset);
        printf("memIndex: %" PRIu16, m_memIndex);
    }
#endif

protected:
    ByteCodeStackOffset m_dstOffset;
    uint16_t m_memIndex;
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

class MemoryInitMulti : public ByteCode {
public:
    MemoryInitMulti(uint32_t index, uint32_t segmentIndex, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset src2)
        : ByteCode(Opcode::MemoryInitOpcode)
        , m_segmentIndex(segmentIndex)
        , m_memIndex(index)
        , m_srcOffsets{ src0, src1, src2 }
    {
        ASSERT(index != 0);
    }

    uint32_t segmentIndex() const
    {
        return m_segmentIndex;
    }

    const ByteCodeStackOffset* srcOffsets() const
    {
        return m_srcOffsets;
    }

    uint16_t memIndex() const { return m_memIndex; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("memory.init ");
        DUMP_BYTECODE_OFFSET(srcOffsets[0]);
        DUMP_BYTECODE_OFFSET(srcOffsets[1]);
        DUMP_BYTECODE_OFFSET(srcOffsets[2]);
        printf("segmentIndex: %" PRIu32, m_segmentIndex);
        printf("memIndex: %" PRIu16, m_memIndex);
    }
#endif

protected:
    uint32_t m_segmentIndex;
    uint16_t m_memIndex;
    ByteCodeStackOffset m_srcOffsets[3];
};

class MemoryCopy : public ByteCodeOffset3 {
public:
    MemoryCopy(uint32_t srcIndex, uint32_t dstIndex, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset src2)
        : ByteCodeOffset3(Opcode::MemoryCopyOpcode, src0, src1, src2)
    {
        ASSERT(srcIndex == 0);
        ASSERT(dstIndex == 0);
    }

    const ByteCodeStackOffset* srcOffsets() const
    {
        return stackOffsets();
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("memory.copy ");
        DUMP_BYTECODE_OFFSET(stackOffsets[0]);
        DUMP_BYTECODE_OFFSET(stackOffsets[1]);
        DUMP_BYTECODE_OFFSET(stackOffsets[2]);
    }
#endif
};

class MemoryCopyMulti : public ByteCodeOffset3 {
public:
    MemoryCopyMulti(uint32_t srcIndex, uint32_t dstIndex, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset src2)
        : ByteCodeOffset3(Opcode::MemoryCopyOpcode, src0, src1, src2)
        , m_srcMemIndex(srcIndex)
        , m_dstMemIndex(dstIndex)
    {
    }

    const ByteCodeStackOffset* srcOffsets() const
    {
        return stackOffsets();
    }

    uint16_t srcMemIndex() const { return m_srcMemIndex; }
    uint16_t dstMemIndex() const { return m_dstMemIndex; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("memory.copy ");
        DUMP_BYTECODE_OFFSET(stackOffsets[0]);
        DUMP_BYTECODE_OFFSET(stackOffsets[1]);
        DUMP_BYTECODE_OFFSET(stackOffsets[2]);
        printf("srcMemIndex: %" PRIu16, m_srcMemIndex);
        printf("dstMemIndex: %" PRIu16, m_dstMemIndex);
    }
#endif

protected:
    uint16_t m_srcMemIndex;
    uint16_t m_dstMemIndex;
};

class MemoryFill : public ByteCodeOffset3 {
public:
    MemoryFill(uint32_t memIdx, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset src2)
        : ByteCodeOffset3(Opcode::MemoryFillOpcode, src0, src1, src2)
    {
        ASSERT(memIdx == 0);
    }

    const ByteCodeStackOffset* srcOffsets() const
    {
        return stackOffsets();
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("memory.fill ");
        DUMP_BYTECODE_OFFSET(stackOffsets[0]);
        DUMP_BYTECODE_OFFSET(stackOffsets[1]);
        DUMP_BYTECODE_OFFSET(stackOffsets[2]);
    }
#endif
};

class MemoryFillMulti : public ByteCodeOffset3 {
public:
    MemoryFillMulti(uint32_t memIdx, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset src2)
        : ByteCodeOffset3(Opcode::MemoryFillOpcode, src0, src1, src2)
        , m_memIndex(memIdx)
    {
        ASSERT(memIdx != 0);
    }

    const ByteCodeStackOffset* srcOffsets() const
    {
        return stackOffsets();
    }

    uint16_t memIndex() const { return m_memIndex; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("memory.fill ");
        DUMP_BYTECODE_OFFSET(stackOffsets[0]);
        DUMP_BYTECODE_OFFSET(stackOffsets[1]);
        DUMP_BYTECODE_OFFSET(stackOffsets[2]);
        printf("memIndex: %" PRIu16, m_memIndex);
    }
#endif

protected:
    uint16_t m_memIndex;
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

class MemoryGrow : public ByteCodeOffset2 {
public:
    MemoryGrow(uint32_t index, ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset)
        : ByteCodeOffset2(Opcode::MemoryGrowOpcode, srcOffset, dstOffset)
    {
        ASSERT(index == 0);
    }

    ByteCodeStackOffset srcOffset() const { return stackOffset1(); }
    ByteCodeStackOffset dstOffset() const { return stackOffset2(); }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("memory.grow ");
        DUMP_BYTECODE_OFFSET(stackOffset1);
        DUMP_BYTECODE_OFFSET(stackOffset2);
    }
#endif
};

class MemoryGrowMulti : public ByteCodeOffset2 {
public:
    MemoryGrowMulti(uint32_t index, ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset)
        : ByteCodeOffset2(Opcode::MemoryGrowOpcode, srcOffset, dstOffset)
        , m_memIndex(index)
    {
        ASSERT(index != 0);
    }

    ByteCodeStackOffset srcOffset() const { return stackOffset1(); }
    ByteCodeStackOffset dstOffset() const { return stackOffset2(); }
    uint16_t memIndex() const { return m_memIndex; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("memory.grow ");
        DUMP_BYTECODE_OFFSET(stackOffset1);
        DUMP_BYTECODE_OFFSET(stackOffset2);
        printf("memIndex: %" PRIu16, m_memIndex);
    }
#endif

protected:
    uint16_t m_memIndex;
};

// dummy ByteCode for memory load operation
class MemoryLoad : public ByteCodeOffset2Value {
public:
    MemoryLoad(Opcode code, uint32_t offset, ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset)
        : ByteCodeOffset2Value(code, srcOffset, dstOffset, offset)
    {
    }

    uint32_t offset() const { return uint32Value(); }
    ByteCodeStackOffset srcOffset() const { return stackOffset1(); }
    ByteCodeStackOffset dstOffset() const { return stackOffset2(); }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
    }
#endif
};

// dummy ByteCode for multi memory load operation
class MemoryLoadMulti : public ByteCodeOffset2Value {
public:
    MemoryLoadMulti(uint32_t index, uint32_t alignment, Opcode code, uint32_t offset, ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset)
        : ByteCodeOffset2Value(code, srcOffset, dstOffset, offset)
        , m_memIndex(index)
        , m_alignment(alignment)
    {
    }

    uint32_t offset() const { return uint32Value(); }
    ByteCodeStackOffset srcOffset() const { return stackOffset1(); }
    ByteCodeStackOffset dstOffset() const { return stackOffset2(); }
    uint16_t memIndex() const { return m_memIndex; }
    uint16_t alignment() const { return m_alignment; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
    }
#endif

protected:
    uint16_t m_memIndex;
    uint16_t m_alignment;
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

// dummy ByteCode for simd multi memory load operation
class SIMDMemoryLoadMulti : public ByteCode {
public:
    SIMDMemoryLoadMulti(uint32_t memIndex, uint32_t alignment, Opcode code, uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset index, ByteCodeStackOffset dst)
        : ByteCode(code)
        , m_offset(offset)
        , m_src0Offset(src0)
        , m_src1Offset(src1)
        , m_index(index)
        , m_dstOffset(dst)
        , m_memIndex(memIndex)
        , m_alignment(alignment)
    {
    }

    uint32_t offset() const { return m_offset; }
    ByteCodeStackOffset index() const { return m_index; }
    ByteCodeStackOffset src0Offset() const { return m_src0Offset; }
    ByteCodeStackOffset src1Offset() const { return m_src1Offset; }
    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }
    uint16_t memIndex() const { return m_memIndex; }
    uint16_t alignment() const { return m_alignment; }

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
    uint16_t m_memIndex;
    uint16_t m_alignment;
};

#if !defined(NDEBUG)
#define DEFINE_LOAD_BYTECODE_DUMP(name)                                                                                              \
    void dump(size_t pos)                                                                                                            \
    {                                                                                                                                \
        printf(#name " src: %" PRIu32 " dst: %" PRIu32 " offset: %" PRIu32, (uint32_t)srcOffset(), (uint32_t)dstOffset(), offset()); \
    }
#else
#define DEFINE_LOAD_BYTECODE_DUMP(name)
#endif

#define DEFINE_LOAD_BYTECODE(name, ...)                                                     \
    class name : public MemoryLoad {                                                        \
    public:                                                                                 \
        name(uint32_t offset, ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset) \
            : MemoryLoad(Opcode::name##Opcode, offset, srcOffset, dstOffset)                \
        {                                                                                   \
        }                                                                                   \
        DEFINE_LOAD_BYTECODE_DUMP(name)                                                     \
    };

#if !defined(NDEBUG)
#define DEFINE_LOAD_MULTI_BYTECODE_DUMP(name)                                                                            \
    void dump(size_t pos)                                                                                                \
    {                                                                                                                    \
        printf(#name " src: %" PRIu32 " dst: %" PRIu32 " offset: %" PRIu32 " memIndex: %" PRIu16 " alignment: %" PRIu16, \
               (uint32_t)srcOffset(), (uint32_t)dstOffset(), offset(), m_memIndex, m_alignment);                         \
    }
#else
#define DEFINE_LOAD_MULTI_BYTECODE_DUMP(name)
#endif

#define DEFINE_LOAD_MULTI_BYTECODE(name, ...)                                                                                          \
    class name##Multi : public MemoryLoad {                                                                                            \
    public:                                                                                                                            \
        name##Multi(uint32_t index, uint32_t alignment, uint32_t offset, ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset) \
            : MemoryLoad(Opcode::name##Opcode, offset, srcOffset, dstOffset)                                                           \
            , m_memIndex(index)                                                                                                        \
            , m_alignment(alignment)                                                                                                   \
        {                                                                                                                              \
        }                                                                                                                              \
        DEFINE_LOAD_MULTI_BYTECODE_DUMP(name)                                                                                          \
        uint16_t memIndex() const { return m_memIndex; }                                                                               \
        uint16_t alignment() const { return m_alignment; }                                                                             \
                                                                                                                                       \
    protected:                                                                                                                         \
        uint16_t m_memIndex;                                                                                                           \
        uint16_t m_alignment;                                                                                                          \
    };

#if !defined(NDEBUG)
#define DEFINE_SIMD_LOAD_LANE_BYTECODE_DUMP(name)                                                                                                                                                                              \
    void dump(size_t pos)                                                                                                                                                                                                      \
    {                                                                                                                                                                                                                          \
        printf(#name " idx: %" PRIu32 " src0: %" PRIu32 " src1: %" PRIu32 " dst: %" PRIu32 " offset: %" PRIu32, (uint32_t)m_index, (uint32_t)m_src0Offset, (uint32_t)m_src1Offset, (uint32_t)m_dstOffset, (uint32_t)m_offset); \
    }
#else
#define DEFINE_SIMD_LOAD_LANE_BYTECODE_DUMP(name)
#endif

#define DEFINE_SIMD_LOAD_LANE_BYTECODE(name, opType)                                                                                  \
    class name : public SIMDMemoryLoad {                                                                                              \
    public:                                                                                                                           \
        name(uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset index, ByteCodeStackOffset dst) \
            : SIMDMemoryLoad(Opcode::name##Opcode, offset, src0, src1, index, dst)                                                    \
        {                                                                                                                             \
        }                                                                                                                             \
        DEFINE_SIMD_LOAD_LANE_BYTECODE_DUMP(name)                                                                                     \
    };

#if !defined(NDEBUG)
#define DEFINE_SIMD_LOAD_LANE_MULTI_BYTECODE_DUMP(name)                                                                                                      \
    void dump(size_t pos)                                                                                                                                    \
    {                                                                                                                                                        \
        printf(#name " idx: %" PRIu32 " src0: %" PRIu32 " src1: %" PRIu32 " dst: %" PRIu32 " offset: %" PRIu32 " memIndex: %" PRIu16 " alignment: %" PRIu16, \
               (uint32_t)m_index, (uint32_t)m_src0Offset, (uint32_t)m_src1Offset, (uint32_t)m_dstOffset, (uint32_t)m_offset, m_memIndex, m_alignment);       \
    }
#else
#define DEFINE_SIMD_LOAD_LANE_MULTI_BYTECODE_DUMP(name)
#endif

#define DEFINE_SIMD_LOAD_LANE_MULTI_BYTECODE(name, opType)                                                                                                                          \
    class name##Multi : public SIMDMemoryLoad {                                                                                                                                     \
    public:                                                                                                                                                                         \
        name##Multi(uint32_t memIndex, uint32_t alignment, uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset index, ByteCodeStackOffset dst) \
            : SIMDMemoryLoad(Opcode::name##Opcode, offset, src0, src1, index, dst)                                                                                                  \
            , m_memIndex(memIndex)                                                                                                                                                  \
            , m_alignment(alignment)                                                                                                                                                \
        {                                                                                                                                                                           \
        }                                                                                                                                                                           \
        DEFINE_SIMD_LOAD_LANE_BYTECODE_DUMP(name)                                                                                                                                   \
        uint16_t memIndex() const { return m_memIndex; }                                                                                                                            \
        uint16_t alignment() const { return m_alignment; }                                                                                                                          \
                                                                                                                                                                                    \
    protected:                                                                                                                                                                      \
        uint16_t m_memIndex;                                                                                                                                                        \
        uint16_t m_alignment;                                                                                                                                                       \
    };

// dummy ByteCode for memory store operation
class MemoryStore : public ByteCodeOffset2Value {
public:
    MemoryStore(Opcode opcode, uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1)
        : ByteCodeOffset2Value(opcode, src0, src1, offset)
    {
    }

    uint32_t offset() const { return uint32Value(); }
    ByteCodeStackOffset src0Offset() const { return stackOffset1(); }
    ByteCodeStackOffset src1Offset() const { return stackOffset2(); }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
    }
#endif
};

// dummy ByteCode for multi memory store operation
class MemoryStoreMulti : public ByteCodeOffset2Value {
public:
    MemoryStoreMulti(uint32_t memIndex, uint32_t alignment, Opcode opcode, uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1)
        : ByteCodeOffset2Value(opcode, src0, src1, offset)
        , m_memIndex(memIndex)
        , m_alignment(alignment)
    {
    }

    uint32_t offset() const { return uint32Value(); }
    ByteCodeStackOffset src0Offset() const { return stackOffset1(); }
    ByteCodeStackOffset src1Offset() const { return stackOffset2(); }
    uint16_t memIndex() const { return m_memIndex; }
    uint16_t alignment() const { return m_alignment; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
    }
#endif

protected:
    uint16_t m_memIndex;
    uint16_t m_alignment;
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

// dummy ByteCode for simd multi memory store operation
class SIMDMemoryStoreMulti : public ByteCode {
public:
    SIMDMemoryStoreMulti(uint32_t memIndex, uint32_t alignment, Opcode opcode, uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset index)
        : ByteCode(opcode)
        , m_offset(offset)
        , m_src0Offset(src0)
        , m_src1Offset(src1)
        , m_index(index)
        , m_memIndex(memIndex)
        , m_alignment(alignment)
    {
    }

    uint32_t offset() const { return m_offset; }
    ByteCodeStackOffset index() const { return m_index; }
    ByteCodeStackOffset src0Offset() const { return m_src0Offset; }
    ByteCodeStackOffset src1Offset() const { return m_src1Offset; }
    uint16_t memIndex() const { return m_memIndex; }
    uint16_t alignment() const { return m_alignment; }

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
    uint16_t m_memIndex;
    uint16_t m_alignment;
};

// dummy ByteCode for simd extract lane operation
class SIMDExtractLane : public ByteCode {
public:
    SIMDExtractLane(Opcode opcode, ByteCodeStackOffset index, ByteCodeStackOffset src, ByteCodeStackOffset dst)
        : ByteCode(opcode)
        , m_srcOffset(src)
        , m_dstOffset(dst)
        , m_index(index)
    {
    }

    ByteCodeStackOffset index() const { return m_index; }
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
    ByteCodeStackOffset m_index;
};

// dummy ByteCode for simd extract lane operation
class SIMDReplaceLane : public ByteCode {
public:
    SIMDReplaceLane(Opcode opcode, ByteCodeStackOffset index, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset dst)
        : ByteCode(opcode)
        , m_srcOffsets{ src0, src1 }
        , m_dstOffset(dst)
        , m_index(index)
    {
    }

    uint32_t index() const { return m_index; }
    const ByteCodeStackOffset* srcOffsets() const { return m_srcOffsets; }
    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
    }
#endif
protected:
    ByteCodeStackOffset m_srcOffsets[2];
    ByteCodeStackOffset m_dstOffset;
    ByteCodeStackOffset m_index;
};

#if !defined(NDEBUG)
#define DEFINE_STORE_BYTECODE_DUMP(name)                                                                                                 \
    void dump(size_t pos)                                                                                                                \
    {                                                                                                                                    \
        printf(#name " src0: %" PRIu32 " src1: %" PRIu32 " offset: %" PRIu32, (uint32_t)src0Offset(), (uint32_t)src1Offset(), offset()); \
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
#define DEFINE_STORE_MULTI_BYTECODE_DUMP(name)                                                                             \
    void dump(size_t pos)                                                                                                  \
    {                                                                                                                      \
        printf(#name " src0: %" PRIu32 " src1: %" PRIu32 " offset: %" PRIu32 " memIndex: %" PRIu16 " alignment: %" PRIu16, \
               (uint32_t)src0Offset(), (uint32_t)src1Offset(), offset(), m_memIndex, m_alignment);                         \
    }
#else
#define DEFINE_STORE_MULTI_BYTECODE_DUMP(name)
#endif

#define DEFINE_STORE_MULTI_BYTECODE(name, readType, writeType)                                                               \
    class name##Multi : public MemoryStore {                                                                                 \
    public:                                                                                                                  \
        name##Multi(uint32_t index, uint32_t alignment, uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1) \
            : MemoryStore(Opcode::name##Opcode, offset, src0, src1)                                                          \
            , m_memIndex(index)                                                                                              \
            , m_alignment(alignment)                                                                                         \
        {                                                                                                                    \
        }                                                                                                                    \
        DEFINE_STORE_MULTI_BYTECODE_DUMP(name)                                                                               \
        uint16_t memIndex() const { return m_memIndex; }                                                                     \
        uint16_t alignment() const { return m_alignment; }                                                                   \
                                                                                                                             \
    protected:                                                                                                               \
        uint16_t m_memIndex;                                                                                                 \
        uint16_t m_alignment;                                                                                                \
    };

#if !defined(NDEBUG)
#define DEFINE_SIMD_STORE_LANE_BYTECODE_DUMP(name)                                                                                                                                     \
    void dump(size_t pos)                                                                                                                                                              \
    {                                                                                                                                                                                  \
        printf(#name " idx: %" PRIu32 " src0: %" PRIu32 " src1: %" PRIu32 " offset: %" PRIu32, (uint32_t)m_index, (uint32_t)m_src0Offset, (uint32_t)m_src1Offset, (uint32_t)m_offset); \
    }
#else
#define DEFINE_SIMD_STORE_LANE_BYTECODE_DUMP(name)
#endif

#define DEFINE_SIMD_STORE_LANE_BYTECODE(name, opType)                                                        \
    class name : public SIMDMemoryStore {                                                                    \
    public:                                                                                                  \
        name(uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset index) \
            : SIMDMemoryStore(Opcode::name##Opcode, offset, src0, src1, index)                               \
        {                                                                                                    \
        }                                                                                                    \
        DEFINE_SIMD_STORE_LANE_BYTECODE_DUMP(name)                                                           \
    };

#if !defined(NDEBUG)
#define DEFINE_SIMD_STORE_LANE_MULTI_BYTECODE_DUMP(name)                                                                                    \
    void dump(size_t pos)                                                                                                                   \
    {                                                                                                                                       \
        printf(#name " idx: %" PRIu32 " src0: %" PRIu32 " src1: %" PRIu32 " offset: %" PRIu32 " memIndex: %" PRIu16 " alignment: %" PRIu16, \
               (uint32_t)m_index,                                                                                                           \
               (uint32_t)m_src0Offset, (uint32_t)m_src1Offset, (uint32_t)m_offset, m_memIndex, m_alignment);                                \
    }
#else
#define DEFINE_SIMD_STORE_LANE_MULTI_BYTECODE_DUMP(name)
#endif

#define DEFINE_SIMD_STORE_LANE_MULTI_BYTECODE(name, opType)                                                                                                \
    class name##Multi : public SIMDMemoryStore {                                                                                                           \
    public:                                                                                                                                                \
        name##Multi(uint32_t memIndex, uint32_t alignment, uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset index) \
            : SIMDMemoryStore(Opcode::name##Opcode, offset, src0, src1, index)                                                                             \
            , m_memIndex(memIndex)                                                                                                                         \
            , m_alignment(alignment)                                                                                                                       \
        {                                                                                                                                                  \
        }                                                                                                                                                  \
        DEFINE_SIMD_STORE_LANE_BYTECODE_DUMP(name)                                                                                                         \
        uint16_t memIndex() const { return m_memIndex; }                                                                                                   \
        uint16_t alignment() const { return m_alignment; }                                                                                                 \
                                                                                                                                                           \
    protected:                                                                                                                                             \
        uint16_t m_memIndex;                                                                                                                               \
        uint16_t m_alignment;                                                                                                                              \
    };


#if !defined(NDEBUG)
#define DEFINE_SIMD_EXTRACT_LANE_BYTECODE_DUMP(name)                                                                                       \
    void dump(size_t pos)                                                                                                                  \
    {                                                                                                                                      \
        printf(#name " idx: %" PRIu32 " src: %" PRIu32 " dst: %" PRIu32, (uint32_t)m_index, (uint32_t)m_srcOffset, (uint32_t)m_dstOffset); \
    }
#else
#define DEFINE_SIMD_EXTRACT_LANE_BYTECODE_DUMP(name)
#endif

#define DEFINE_SIMD_EXTRACT_LANE_BYTECODE(name, ...)                          \
    class name : public SIMDExtractLane {                                     \
    public:                                                                   \
        name(uint8_t index, ByteCodeStackOffset src, ByteCodeStackOffset dst) \
            : SIMDExtractLane(Opcode::name##Opcode, index, src, dst)          \
        {                                                                     \
        }                                                                     \
        DEFINE_SIMD_EXTRACT_LANE_BYTECODE_DUMP(name)                          \
    };

#if !defined(NDEBUG)
#define DEFINE_SIMD_REPLACE_LANE_BYTECODE_DUMP(name)                                                                                                                                         \
    void dump(size_t pos)                                                                                                                                                                    \
    {                                                                                                                                                                                        \
        printf(#name " idx: %" PRIu32 " src0: %" PRIu32 " src1: %" PRIu32 " dst: %" PRIu32, (uint32_t)m_index, (uint32_t)m_srcOffsets[0], (uint32_t)m_srcOffsets[1], (uint32_t)m_dstOffset); \
    }
#else
#define DEFINE_SIMD_REPLACE_LANE_BYTECODE_DUMP(name)
#endif

#define DEFINE_SIMD_REPLACE_LANE_BYTECODE(name, ...)                                                     \
    class name : public SIMDReplaceLane {                                                                \
    public:                                                                                              \
        name(uint8_t index, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset dst) \
            : SIMDReplaceLane(Opcode::name##Opcode, index, src0, src1, dst)                              \
        {                                                                                                \
        }                                                                                                \
        DEFINE_SIMD_REPLACE_LANE_BYTECODE_DUMP(name)                                                     \
    };


class AtomicRmw : public ByteCode {
public:
    AtomicRmw(Opcode opcode, uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset dst)
        : ByteCode(opcode)
        , m_offset(offset)
        , m_src0Offset(src0)
        , m_src1Offset(src1)
        , m_dstOffset(dst)
    {
    }

    uint32_t offset() const { return m_offset; }
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
    ByteCodeStackOffset m_dstOffset;
};

class AtomicRmwMulti : public ByteCode {
public:
    AtomicRmwMulti(uint32_t memIndex, uint32_t alignment, Opcode opcode, uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset dst)
        : ByteCode(opcode)
        , m_offset(offset)
        , m_src0Offset(src0)
        , m_src1Offset(src1)
        , m_dstOffset(dst)
        , m_memIndex(memIndex)
        , m_alignment(alignment)
    {
    }

    uint32_t offset() const { return m_offset; }
    ByteCodeStackOffset src0Offset() const { return m_src0Offset; }
    ByteCodeStackOffset src1Offset() const { return m_src1Offset; }
    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }
    uint16_t memIndex() const { return m_memIndex; }
    uint16_t alignment() const { return m_alignment; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
    }
#endif

protected:
    uint32_t m_offset;
    ByteCodeStackOffset m_src0Offset;
    ByteCodeStackOffset m_src1Offset;
    ByteCodeStackOffset m_dstOffset;
    uint16_t m_memIndex;
    uint16_t m_alignment;
};

class AtomicRmwCmpxchg : public ByteCodeOffset4Value {
public:
    AtomicRmwCmpxchg(Opcode opcode, uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset src2, ByteCodeStackOffset dst)
        : ByteCodeOffset4Value(opcode, src0, src1, src2, dst, offset)

    {
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
    }
#endif
};

class AtomicRmwCmpxchgMulti : public ByteCodeOffset4Value {
public:
    AtomicRmwCmpxchgMulti(uint32_t memIndex, uint32_t alignment, Opcode opcode, uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset src2, ByteCodeStackOffset dst)
        : ByteCodeOffset4Value(opcode, src0, src1, src2, dst, offset)
        , m_memIndex(memIndex)
        , m_alignment(alignment)

    {
    }

    uint16_t memIndex() const { return m_memIndex; }
    uint16_t alignment() const { return m_alignment; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
    }
#endif

protected:
    uint16_t m_memIndex;
    uint16_t m_alignment;
};

class MemoryAtomicWait32 : public ByteCodeOffset4Value {
public:
    MemoryAtomicWait32(uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset src2, ByteCodeStackOffset dst)
        : ByteCodeOffset4Value(Opcode::MemoryAtomicWait32Opcode, src0, src1, src2, dst, offset)
    {
    }
#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("MemoryAtomicWait32 src0: %" PRIu32 " src1: %" PRIu32 " src2: %" PRIu32 " dst: %" PRIu32 " offset: %" PRIu32, (uint32_t)m_stackOffset1, (uint32_t)m_stackOffset2, (uint32_t)m_stackOffset3, (uint32_t)m_stackOffset4, (uint32_t)m_value);
    }
#endif
};

class MemoryAtomicWait32Multi : public ByteCodeOffset4Value {
public:
    MemoryAtomicWait32Multi(uint32_t index, uint32_t alignment, uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset src2, ByteCodeStackOffset dst)
        : ByteCodeOffset4Value(Opcode::MemoryAtomicWait32Opcode, src0, src1, src2, dst, offset)
        , m_memIndex(index)
        , m_alignment(alignment)
    {
    }
#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("MemoryAtomicWait32 src0: %" PRIu32 " src1: %" PRIu32 " src2: %" PRIu32 " dst: %" PRIu32 " offset: %" PRIu32 " memIndex: %" PRIu16 " alignment: %" PRIu16,
               (uint32_t)m_stackOffset1, (uint32_t)m_stackOffset2, (uint32_t)m_stackOffset3, (uint32_t)m_stackOffset4, (uint32_t)m_value, m_memIndex, m_alignment);
    }
#endif
    uint16_t memIndex() const { return m_memIndex; }
    uint16_t alignment() const { return m_alignment; }

protected:
    uint16_t m_memIndex;
    uint16_t m_alignment;
};

class MemoryAtomicWait64 : public ByteCodeOffset4Value {
public:
    MemoryAtomicWait64(uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset src2, ByteCodeStackOffset dst)
        : ByteCodeOffset4Value(Opcode::MemoryAtomicWait64Opcode, src0, src1, src2, dst, offset)
    {
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("MemoryAtomicWait64 src0: %" PRIu32 " src1: %" PRIu32 " src2: %" PRIu32 " dst: %" PRIu32 " offset: %" PRIu32, (uint32_t)m_stackOffset1, (uint32_t)m_stackOffset2, (uint32_t)m_stackOffset3, (uint32_t)m_stackOffset4, (uint32_t)m_value);
    }
#endif
};

class MemoryAtomicWait64Multi : public ByteCodeOffset4Value {
public:
    MemoryAtomicWait64Multi(uint32_t index, uint32_t alignment, uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset src2, ByteCodeStackOffset dst)
        : ByteCodeOffset4Value(Opcode::MemoryAtomicWait64Opcode, src0, src1, src2, dst, offset)
        , m_memIndex(index)
        , m_alignment(alignment)
    {
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("MemoryAtomicWait64 src0: %" PRIu32 " src1: %" PRIu32 " src2: %" PRIu32 " dst: %" PRIu32 " offset: %" PRIu32 " memIndex: %" PRIu16 " alignment: %" PRIu16,
               (uint32_t)m_stackOffset1, (uint32_t)m_stackOffset2, (uint32_t)m_stackOffset3, (uint32_t)m_stackOffset4, (uint32_t)m_value, m_memIndex, m_alignment);
    }
#endif
    uint16_t memIndex() const { return m_memIndex; }
    uint16_t alignment() const { return m_alignment; }

protected:
    uint16_t m_memIndex;
    uint16_t m_alignment;
};

class MemoryAtomicNotify : public ByteCode {
public:
    MemoryAtomicNotify(uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset dst)
        : ByteCode(Opcode::MemoryAtomicNotifyOpcode)
        , m_offset(offset)
        , m_src0Offset(src0)
        , m_src1Offset(src1)
        , m_dstOffset(dst)
    {
    }

    uint32_t offset() const { return m_offset; }
    ByteCodeStackOffset src0Offset() const { return m_src0Offset; }
    ByteCodeStackOffset src1Offset() const { return m_src1Offset; }
    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("MemoryAtomicNotify src0: %" PRIu32 " src1: %" PRIu32 " dst: %" PRIu32 " offset: %" PRIu32, (uint32_t)m_src0Offset, (uint32_t)m_src1Offset, (uint32_t)m_dstOffset, (uint32_t)m_offset);
    }
#endif
protected:
    uint32_t m_offset;
    ByteCodeStackOffset m_src0Offset;
    ByteCodeStackOffset m_src1Offset;
    ByteCodeStackOffset m_dstOffset;
};

class MemoryAtomicNotifyMulti : public ByteCode {
public:
    MemoryAtomicNotifyMulti(uint32_t index, uint32_t alignment, uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset dst)
        : ByteCode(Opcode::MemoryAtomicNotifyOpcode)
        , m_offset(offset)
        , m_src0Offset(src0)
        , m_src1Offset(src1)
        , m_dstOffset(dst)
        , m_memIndex(index)
        , m_alignment(alignment)
    {
    }

    uint32_t offset() const { return m_offset; }
    ByteCodeStackOffset src0Offset() const { return m_src0Offset; }
    ByteCodeStackOffset src1Offset() const { return m_src1Offset; }
    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }
    uint16_t memIndex() const { return m_memIndex; }
    uint16_t alignment() const { return m_alignment; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("MemoryAtomicNotify src0: %" PRIu32 " src1: %" PRIu32 " dst: %" PRIu32 " offset: %" PRIu32 " memIndex: %" PRIu16 " alignment: %" PRIu16,
               (uint32_t)m_src0Offset, (uint32_t)m_src1Offset, (uint32_t)m_dstOffset, (uint32_t)m_offset, m_memIndex, m_alignment);
    }
#endif
protected:
    uint32_t m_offset;
    ByteCodeStackOffset m_src0Offset;
    ByteCodeStackOffset m_src1Offset;
    ByteCodeStackOffset m_dstOffset;
    uint16_t m_memIndex;
    uint16_t m_alignment;
};

class AtomicFence : public ByteCode {
public:
    AtomicFence()
        : ByteCode(Opcode::AtomicFenceOpcode)
    {
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
    }
#endif
protected:
    uint32_t m_offset;
};

#if !defined(NDEBUG)
#define DEFINE_RMW_BYTECODE_DUMP(name)                                                                                                                                                     \
    void dump(size_t pos)                                                                                                                                                                  \
    {                                                                                                                                                                                      \
        printf(#name " src0: %" PRIu32 " src1: %" PRIu32 " dst: %" PRIu32 " offset: %" PRIu32, (uint32_t)m_src0Offset, (uint32_t)m_src1Offset, (uint32_t)m_dstOffset, (uint32_t)m_offset); \
    }
#else
#define DEFINE_RMW_BYTECODE_DUMP(name)
#endif

#define DEFINE_RMW_BYTECODE(name, paramType, writeType, operationName)                                     \
    class name : public AtomicRmw {                                                                        \
    public:                                                                                                \
        name(uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset dst) \
            : AtomicRmw(Opcode::name##Opcode, offset, src0, src1, dst)                                     \
        {                                                                                                  \
        }                                                                                                  \
        DEFINE_RMW_BYTECODE_DUMP(name)                                                                     \
    };

#if !defined(NDEBUG)
#define DEFINE_RMW_MULTI_BYTECODE_DUMP(name)                                                                                                \
    void dump(size_t pos)                                                                                                                   \
    {                                                                                                                                       \
        printf(#name " src0: %" PRIu32 " src1: %" PRIu32 " dst: %" PRIu32 " offset: %" PRIu32 " memIndex: %" PRIu16 " alignment: %" PRIu16, \
               (uint32_t)m_src0Offset, (uint32_t)m_src1Offset, (uint32_t)m_dstOffset, (uint32_t)m_offset, m_memIndex, m_alignment);         \
    }
#else
#define DEFINE_RMW_MULTI_BYTECODE_DUMP(name)
#endif

#define DEFINE_RMW_MULTI_BYTECODE(name, paramType, writeType, operationName)                                                                          \
    class name##Multi : public AtomicRmw {                                                                                                            \
    public:                                                                                                                                           \
        name##Multi(uint32_t index, uint32_t alignment, uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset dst) \
            : AtomicRmw(Opcode::name##Opcode, offset, src0, src1, dst)                                                                                \
            , m_memIndex(index)                                                                                                                       \
            , m_alignment(alignment)                                                                                                                  \
        {                                                                                                                                             \
        }                                                                                                                                             \
        DEFINE_RMW_MULTI_BYTECODE_DUMP(name)                                                                                                          \
        uint16_t memIndex() const { return m_memIndex; }                                                                                              \
        uint16_t alignment() const { return m_alignment; }                                                                                            \
                                                                                                                                                      \
    protected:                                                                                                                                        \
        uint16_t m_memIndex;                                                                                                                          \
        uint16_t m_alignment;                                                                                                                         \
    };

#if !defined(NDEBUG)
#define DEFINE_RMW_CMPXCHG_BYTECODE_DUMP(name)                                                                                                                                                                                               \
    void dump(size_t pos)                                                                                                                                                                                                                    \
    {                                                                                                                                                                                                                                        \
        printf(#name " src0: %" PRIu32 " src1: %" PRIu32 " src2: %" PRIu32 " dst: %" PRIu32 " offset: %" PRIu32, (uint32_t)m_stackOffset1, (uint32_t)m_stackOffset2, (uint32_t)m_stackOffset3, (uint32_t)m_stackOffset4, (uint32_t)m_value); \
    }
#else
#define DEFINE_RMW_CMPXCHG_BYTECODE_DUMP(name)
#endif

#define DEFINE_RMW_CMPXCHG_BYTECODE(name, paramType, writeType)                                                                      \
    class name : public AtomicRmwCmpxchg {                                                                                           \
    public:                                                                                                                          \
        name(uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset src2, ByteCodeStackOffset dst) \
            : AtomicRmwCmpxchg(Opcode::name##Opcode, offset, src0, src1, src2, dst)                                                  \
        {                                                                                                                            \
        }                                                                                                                            \
        DEFINE_RMW_CMPXCHG_BYTECODE_DUMP(name)                                                                                       \
    };

#if !defined(NDEBUG)
#define DEFINE_RMW_CMPXCHG_MULTI_BYTECODE_DUMP(name)                                                                                                                \
    void dump(size_t pos)                                                                                                                                           \
    {                                                                                                                                                               \
        printf(#name " src0: %" PRIu32 " src1: %" PRIu32 " src2: %" PRIu32 " dst: %" PRIu32 " offset: %" PRIu32 " memIndex: %" PRIu16 " alignment: %" PRIu16,       \
               (uint32_t)m_stackOffset1, (uint32_t)m_stackOffset2, (uint32_t)m_stackOffset3, (uint32_t)m_stackOffset4, (uint32_t)m_value, m_memIndex, m_alignment); \
    }
#else
#define DEFINE_RMW_CMPXCHG_MULTI_BYTECODE_DUMP(name)
#endif

#define DEFINE_RMW_CMPXCHG_MULTI_BYTECODE(name, paramType, writeType)                                                                                                           \
    class name##Multi : public AtomicRmwCmpxchg {                                                                                                                               \
    public:                                                                                                                                                                     \
        name##Multi(uint32_t index, uint32_t alignment, uint32_t offset, ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset src2, ByteCodeStackOffset dst) \
            : AtomicRmwCmpxchg(Opcode::name##Opcode, offset, src0, src1, src2, dst)                                                                                             \
            , m_memIndex(index)                                                                                                                                                 \
            , m_alignment(alignment)                                                                                                                                            \
        {                                                                                                                                                                       \
        }                                                                                                                                                                       \
        DEFINE_RMW_CMPXCHG_BYTECODE_DUMP(name)                                                                                                                                  \
        uint16_t memIndex() const { return m_memIndex; }                                                                                                                        \
        uint16_t alignment() const { return m_alignment; }                                                                                                                      \
                                                                                                                                                                                \
    protected:                                                                                                                                                                  \
        uint16_t m_memIndex;                                                                                                                                                    \
        uint16_t m_alignment;                                                                                                                                                   \
    };


FOR_EACH_BYTECODE_LOAD_OP(DEFINE_LOAD_BYTECODE)
FOR_EACH_BYTECODE_LOAD_OP(DEFINE_LOAD_MULTI_BYTECODE)
FOR_EACH_BYTECODE_STORE_OP(DEFINE_STORE_BYTECODE)
FOR_EACH_BYTECODE_STORE_OP(DEFINE_STORE_MULTI_BYTECODE)
FOR_EACH_BYTECODE_SIMD_LOAD_SPLAT_OP(DEFINE_LOAD_BYTECODE)
FOR_EACH_BYTECODE_SIMD_LOAD_SPLAT_OP(DEFINE_LOAD_MULTI_BYTECODE)
FOR_EACH_BYTECODE_SIMD_LOAD_EXTEND_OP(DEFINE_LOAD_BYTECODE)
FOR_EACH_BYTECODE_SIMD_LOAD_EXTEND_OP(DEFINE_LOAD_MULTI_BYTECODE)
FOR_EACH_BYTECODE_SIMD_LOAD_LANE_OP(DEFINE_SIMD_LOAD_LANE_BYTECODE)
FOR_EACH_BYTECODE_SIMD_LOAD_LANE_OP(DEFINE_SIMD_LOAD_LANE_MULTI_BYTECODE)
FOR_EACH_BYTECODE_SIMD_STORE_LANE_OP(DEFINE_SIMD_STORE_LANE_BYTECODE)
FOR_EACH_BYTECODE_SIMD_STORE_LANE_OP(DEFINE_SIMD_STORE_LANE_MULTI_BYTECODE)
FOR_EACH_BYTECODE_SIMD_EXTRACT_LANE_OP(DEFINE_SIMD_EXTRACT_LANE_BYTECODE)
FOR_EACH_BYTECODE_SIMD_REPLACE_LANE_OP(DEFINE_SIMD_REPLACE_LANE_BYTECODE)
FOR_EACH_BYTECODE_ATOMIC_LOAD_OP(DEFINE_LOAD_BYTECODE)
FOR_EACH_BYTECODE_ATOMIC_LOAD_OP(DEFINE_LOAD_MULTI_BYTECODE)
FOR_EACH_BYTECODE_ATOMIC_STORE_OP(DEFINE_STORE_BYTECODE)
FOR_EACH_BYTECODE_ATOMIC_STORE_OP(DEFINE_STORE_MULTI_BYTECODE)
FOR_EACH_BYTECODE_ATOMIC_RMW_OP(DEFINE_RMW_BYTECODE)
FOR_EACH_BYTECODE_ATOMIC_RMW_OP(DEFINE_RMW_MULTI_BYTECODE)
FOR_EACH_BYTECODE_ATOMIC_RMW_CMPXCHG_OP(DEFINE_RMW_CMPXCHG_BYTECODE)
FOR_EACH_BYTECODE_ATOMIC_RMW_CMPXCHG_OP(DEFINE_RMW_CMPXCHG_MULTI_BYTECODE)
#undef DEFINE_LOAD_BYTECODE_DUMP
#undef DEFINE_LOAD_MULTI_BYTECODE_DUMP
#undef DEFINE_LOAD_BYTECODE
#undef DEFINE_LOAD_MULTI_BYTECODE
#undef DEFINE_STORE_BYTECODE_DUMP
#undef DEFINE_STORE_MULTI_BYTECODE_DUMP
#undef DEFINE_STORE_BYTECODE
#undef DEFINE_STORE_MULTI_BYTECODE
#undef DEFINE_SIMD_LOAD_LANE_BYTECODE
#undef DEFINE_SIMD_LOAD_LANE_MULTI_BYTECODE
#undef DEFINE_SIMD_STORE_LANE_BYTECODE
#undef DEFINE_SIMD_STORE_LANE_MULTI_BYTECODE
#undef DEFINE_RMW_BYTECODE_DUMP
#undef DEFINE_RMW_MULTI_BYTECODE_DUMP
#undef DEFINE_RMW_BYTECODE
#undef DEFINE_RMW_MULTI_BYTECODE
#undef DEFINE_RMW_CMPXCHG_BYTECODE_DUMP
#undef DEFINE_RMW_CMPXCHG_MULTI_BYTECODE_DUMP
#undef DEFINE_RMW_CMPXCHG_BYTECODE
#undef DEFINE_RMW_CMPXCHG_MULTI_BYTECODE

// FOR_EACH_BYTECODE_SIMD_ETC_OP
class V128BitSelect : public ByteCodeOffset4 {
public:
    V128BitSelect(ByteCodeStackOffset lhs, ByteCodeStackOffset rhs, ByteCodeStackOffset c, ByteCodeStackOffset dst)
        : ByteCodeOffset4(Opcode::V128BitSelectOpcode, lhs, rhs, c, dst)
    {
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("v128.bitselect lhs: %" PRIu32 " rhs: %" PRIu32 " c: %" PRIu32 " dst: %" PRIu32, (uint32_t)m_stackOffsets[0], (uint32_t)m_stackOffsets[1], (uint32_t)m_stackOffsets[2], (uint32_t)m_stackOffsets[3]);
    }
#endif
};

class V128Load32Zero : public MemoryLoad {
public:
    V128Load32Zero(uint32_t offset, ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset)
        : MemoryLoad(Opcode::V128Load32ZeroOpcode, offset, srcOffset, dstOffset)
    {
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("V128Load32Zero src: %" PRIu32 " dst: %" PRIu32 " offset: %" PRIu32, (uint32_t)srcOffset(), (uint32_t)dstOffset(), offset());
    }
#endif
};

class V128Load32ZeroMulti : public MemoryLoad {
public:
    V128Load32ZeroMulti(uint32_t index, uint32_t alignment, uint32_t offset, ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset)
        : MemoryLoad(Opcode::V128Load32ZeroOpcode, offset, srcOffset, dstOffset)
        , m_memIndex(index)
        , m_alignment(alignment)
    {
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("V128Load32Zero src: %" PRIu32 " dst: %" PRIu32 " offset: %" PRIu32 " memIndex: %" PRIu16 " alignment: %" PRIu16, (uint32_t)srcOffset(), (uint32_t)dstOffset(), offset(), m_memIndex, m_alignment);
    }
#endif
    uint16_t memIndex() const { return m_memIndex; }
    uint16_t alignment() const { return m_alignment; }

protected:
    uint16_t m_memIndex;
    uint16_t m_alignment;
};

class V128Load64Zero : public MemoryLoad {
public:
    V128Load64Zero(uint32_t offset, ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset)
        : MemoryLoad(Opcode::V128Load64ZeroOpcode, offset, srcOffset, dstOffset)
    {
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("V128Load64Zero src: %" PRIu32 " dst: %" PRIu32 " offset: %" PRIu32, (uint32_t)srcOffset(), (uint32_t)dstOffset(), offset());
    }
#endif
};

class V128Load64ZeroMulti : public MemoryLoad {
public:
    V128Load64ZeroMulti(uint32_t index, uint32_t alignment, uint32_t offset, ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset)
        : MemoryLoad(Opcode::V128Load64ZeroOpcode, offset, srcOffset, dstOffset)
        , m_memIndex(index)
        , m_alignment(alignment)
    {
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("V128Load64Zero src: %" PRIu32 " dst: %" PRIu32 " offset: %" PRIu32 " memIndex: %" PRIu16 " alignment: %" PRIu16, (uint32_t)srcOffset(), (uint32_t)dstOffset(), offset(), m_memIndex, m_alignment);
    }
    uint16_t memIndex() const { return m_memIndex; }
    uint16_t alignment() const { return m_alignment; }
#endif
protected:
    uint16_t m_memIndex;
    uint16_t m_alignment;
};

class I8X16Shuffle : public ByteCode {
public:
    I8X16Shuffle(ByteCodeStackOffset src0, ByteCodeStackOffset src1, ByteCodeStackOffset dst, uint8_t* value)
        : ByteCode(Opcode::I8X16ShuffleOpcode)
        , m_srcOffsets{ src0, src1 }
        , m_dstOffset(dst)
    {
        ASSERT(!!value);
        memcpy(m_value, value, 16);
    }

    const ByteCodeStackOffset* srcOffsets() const { return m_srcOffsets; }
    ByteCodeStackOffset dstOffset() const { return m_dstOffset; }
    const uint8_t* value() const { return m_value; }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("I8X16Shuffle src0: %" PRIu32 " src1: %" PRIu32 " dst: %" PRIu32, (uint32_t)m_srcOffsets[0], (uint32_t)m_srcOffsets[1], (uint32_t)m_dstOffset);
    }
#endif
protected:
    ByteCodeStackOffset m_srcOffsets[2];
    ByteCodeStackOffset m_dstOffset;
    uint8_t m_value[16];
};

class TableGet : public ByteCodeOffset2Value {
public:
    TableGet(uint32_t index, ByteCodeStackOffset srcOffset, ByteCodeStackOffset dstOffset)
        : ByteCodeOffset2Value(Opcode::TableGetOpcode, srcOffset, dstOffset, index)
    {
    }

    ByteCodeStackOffset srcOffset() const { return stackOffset1(); }
    ByteCodeStackOffset dstOffset() const { return stackOffset2(); }
    uint32_t tableIndex() const { return uint32Value(); }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("table.get ");
        DUMP_BYTECODE_OFFSET(stackOffset1);
        DUMP_BYTECODE_OFFSET(stackOffset2);
        printf("tableIndex: %" PRIu32, tableIndex());
    }
#endif
};

class TableSet : public ByteCodeOffset2Value {
public:
    TableSet(uint32_t index, ByteCodeStackOffset src0, ByteCodeStackOffset src1)
        : ByteCodeOffset2Value(Opcode::TableSetOpcode, src0, src1, index)
    {
    }

    ByteCodeStackOffset src0Offset() const { return stackOffset1(); }
    ByteCodeStackOffset src1Offset() const { return stackOffset2(); }
    uint32_t tableIndex() const { return uint32Value(); }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("table.set ");
        DUMP_BYTECODE_OFFSET(stackOffset1);
        DUMP_BYTECODE_OFFSET(stackOffset2);
        printf("tableIndex: %" PRIu32, tableIndex());
    }
#endif
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

class TableSize : public ByteCodeOffsetValue {
public:
    TableSize(uint32_t index, ByteCodeStackOffset dst)
        : ByteCodeOffsetValue(Opcode::TableSizeOpcode, dst, index)
    {
    }

    uint32_t tableIndex() const { return uint32Value(); }
    ByteCodeStackOffset dstOffset() const { return stackOffset(); }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("table.size ");
        DUMP_BYTECODE_OFFSET(stackOffset);
        printf("tableIndex: %" PRIu32, tableIndex());
    }
#endif
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

class RefFunc : public ByteCodeOffsetValue {
public:
    RefFunc(uint32_t funcIndex, ByteCodeStackOffset dstOffset)
        : ByteCodeOffsetValue(Opcode::RefFuncOpcode, dstOffset, funcIndex)
    {
    }

    ByteCodeStackOffset dstOffset() const { return stackOffset(); }
    uint32_t funcIndex() const { return uint32Value(); }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("ref.func ");
        DUMP_BYTECODE_OFFSET(stackOffset);
        printf("funcIndex: %" PRIu32, funcIndex());
    }
#endif
};

class GlobalGet32 : public ByteCodeOffsetValue {
public:
    GlobalGet32(ByteCodeStackOffset dstOffset, uint32_t index)
        : ByteCodeOffsetValue(Opcode::GlobalGet32Opcode, dstOffset, index)
    {
    }

    ByteCodeStackOffset dstOffset() const { return stackOffset(); }
    uint32_t index() const { return uint32Value(); }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("global.get32 ");
        DUMP_BYTECODE_OFFSET(stackOffset);
        printf("index: %" PRId32, uint32Value());
    }
#endif
};

class GlobalGet64 : public ByteCodeOffsetValue {
public:
    GlobalGet64(ByteCodeStackOffset dstOffset, uint32_t index)
        : ByteCodeOffsetValue(Opcode::GlobalGet64Opcode, dstOffset, index)
    {
    }

    ByteCodeStackOffset dstOffset() const { return stackOffset(); }
    uint32_t index() const { return uint32Value(); }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("global.get64 ");
        DUMP_BYTECODE_OFFSET(stackOffset);
        printf("index: %" PRId32, uint32Value());
    }
#endif
};

class GlobalGet128 : public ByteCodeOffsetValue {
public:
    GlobalGet128(ByteCodeStackOffset dstOffset, uint32_t index)
        : ByteCodeOffsetValue(Opcode::GlobalGet128Opcode, dstOffset, index)
    {
    }

    ByteCodeStackOffset dstOffset() const { return stackOffset(); }
    uint32_t index() const { return uint32Value(); }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("global.get128 ");
        DUMP_BYTECODE_OFFSET(stackOffset);
        printf("index: %" PRId32, uint32Value());
    }
#endif
};

class GlobalSet32 : public ByteCodeOffsetValue {
public:
    GlobalSet32(ByteCodeStackOffset srcOffset, uint32_t index)
        : ByteCodeOffsetValue(Opcode::GlobalSet32Opcode, srcOffset, index)
    {
    }

    ByteCodeStackOffset srcOffset() const { return stackOffset(); }
    uint32_t index() const { return uint32Value(); }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("global.set32 ");
        DUMP_BYTECODE_OFFSET(stackOffset);
        printf("index: %" PRId32, uint32Value());
    }
#endif
};

class GlobalSet64 : public ByteCodeOffsetValue {
public:
    GlobalSet64(ByteCodeStackOffset srcOffset, uint32_t index)
        : ByteCodeOffsetValue(Opcode::GlobalSet64Opcode, srcOffset, index)
    {
    }

    ByteCodeStackOffset srcOffset() const { return stackOffset(); }
    uint32_t index() const { return uint32Value(); }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("global.set64 ");
        DUMP_BYTECODE_OFFSET(stackOffset);
        printf("index: %" PRId32, uint32Value());
    }
#endif
};

class GlobalSet128 : public ByteCodeOffsetValue {
public:
    GlobalSet128(ByteCodeStackOffset srcOffset, uint32_t index)
        : ByteCodeOffsetValue(Opcode::GlobalSet128Opcode, srcOffset, index)
    {
    }

    ByteCodeStackOffset srcOffset() const { return stackOffset(); }
    uint32_t index() const { return uint32Value(); }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("global.set128 ");
        DUMP_BYTECODE_OFFSET(stackOffset);
        printf("index: %" PRId32, uint32Value());
    }
#endif
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

    uint32_t offsetsSize() const
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
        }
    }
#endif

protected:
    uint32_t m_tagIndex;
    uint32_t m_offsetsSize;
};

#if !defined(NDEBUG)
class Nop : public ByteCode {
public:
    Nop()
        : ByteCode(Opcode::NopOpcode)
    {
    }


    void dump(size_t pos)
    {
        printf("nop");
    }

protected:
};
#endif /* !NDEBUG*/

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

    uint32_t offsetsSize() const
    {
        return m_offsetsSize;
    }

#if !defined(NDEBUG)
    void dump(size_t pos)
    {
        printf("end");
        if (offsetsSize()) {
            printf(" resultOffsets: ");
            auto arr = resultOffsets();
            for (size_t i = 0; i < offsetsSize(); i++) {
                printf("%" PRIu32 " ", arr[i]);
            }
        }
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
