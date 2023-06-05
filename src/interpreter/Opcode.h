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

#ifndef __WalrusOpcode__
#define __WalrusOpcode__

#include "runtime/Value.h"

namespace Walrus {

enum OpcodeKind : size_t {
#define WABT_OPCODE(rtype, type1, type2, type3, memSize, prefix, code, name, \
                    text, decomp, size)                                      \
    name##Opcode,
#include "interpreter/opcode.def"
#undef WABT_OPCODE
    InvalidOpcode,
};

#define FOR_EACH_USED_OPCODE(WABT_OPCODE)                                                                    \
    WABT_OPCODE(___, ___, ___, ___, 0, 0, 0x00, Unreachable, "unreachable", "")                              \
    WABT_OPCODE(___, ___, ___, ___, 0, 0, 0x08, Throw, "throw", "")                                          \
    WABT_OPCODE(___, ___, ___, ___, 0, 0, 0x0b, End, "end", "")                                              \
    WABT_OPCODE(___, I32, ___, ___, 0, 0, 0x0e, BrTable, "br_table", "")                                     \
    WABT_OPCODE(___, ___, ___, ___, 0, 0, 0x10, Call, "call", "")                                            \
    WABT_OPCODE(___, ___, ___, ___, 0, 0, 0x11, CallIndirect, "call_indirect", "")                           \
    WABT_OPCODE(___, ___, ___, I32, 0, 0, 0x1b, Select, "select", "")                                        \
    WABT_OPCODE(I32, I32, ___, ___, 4, 0, 0x28, I32Load, "i32.load", "")                                     \
    WABT_OPCODE(I64, I32, ___, ___, 8, 0, 0x29, I64Load, "i64.load", "")                                     \
    WABT_OPCODE(F32, I32, ___, ___, 4, 0, 0x2a, F32Load, "f32.load", "")                                     \
    WABT_OPCODE(F64, I32, ___, ___, 8, 0, 0x2b, F64Load, "f64.load", "")                                     \
    WABT_OPCODE(I32, I32, ___, ___, 1, 0, 0x2c, I32Load8S, "i32.load8_s", "")                                \
    WABT_OPCODE(I32, I32, ___, ___, 1, 0, 0x2d, I32Load8U, "i32.load8_u", "")                                \
    WABT_OPCODE(I32, I32, ___, ___, 2, 0, 0x2e, I32Load16S, "i32.load16_s", "")                              \
    WABT_OPCODE(I32, I32, ___, ___, 2, 0, 0x2f, I32Load16U, "i32.load16_u", "")                              \
    WABT_OPCODE(I64, I32, ___, ___, 1, 0, 0x30, I64Load8S, "i64.load8_s", "")                                \
    WABT_OPCODE(I64, I32, ___, ___, 1, 0, 0x31, I64Load8U, "i64.load8_u", "")                                \
    WABT_OPCODE(I64, I32, ___, ___, 2, 0, 0x32, I64Load16S, "i64.load16_s", "")                              \
    WABT_OPCODE(I64, I32, ___, ___, 2, 0, 0x33, I64Load16U, "i64.load16_u", "")                              \
    WABT_OPCODE(I64, I32, ___, ___, 4, 0, 0x34, I64Load32S, "i64.load32_s", "")                              \
    WABT_OPCODE(I64, I32, ___, ___, 4, 0, 0x35, I64Load32U, "i64.load32_u", "")                              \
    WABT_OPCODE(___, I32, I32, ___, 4, 0, 0x36, I32Store, "i32.store", "")                                   \
    WABT_OPCODE(___, I32, I64, ___, 8, 0, 0x37, I64Store, "i64.store", "")                                   \
    WABT_OPCODE(___, I32, F32, ___, 4, 0, 0x38, F32Store, "f32.store", "")                                   \
    WABT_OPCODE(___, I32, F64, ___, 8, 0, 0x39, F64Store, "f64.store", "")                                   \
    WABT_OPCODE(___, I32, I32, ___, 1, 0, 0x3a, I32Store8, "i32.store8", "")                                 \
    WABT_OPCODE(___, I32, I32, ___, 2, 0, 0x3b, I32Store16, "i32.store16", "")                               \
    WABT_OPCODE(___, I32, I64, ___, 1, 0, 0x3c, I64Store8, "i64.store8", "")                                 \
    WABT_OPCODE(___, I32, I64, ___, 2, 0, 0x3d, I64Store16, "i64.store16", "")                               \
    WABT_OPCODE(___, I32, I64, ___, 4, 0, 0x3e, I64Store32, "i64.store32", "")                               \
    WABT_OPCODE(I32, ___, ___, ___, 0, 0, 0x3f, MemorySize, "memory.size", "")                               \
    WABT_OPCODE(I32, I32, ___, ___, 0, 0, 0x40, MemoryGrow, "memory.grow", "")                               \
    WABT_OPCODE(I32, I32, ___, ___, 0, 0, 0x45, I32Eqz, "i32.eqz", "eqz")                                    \
    WABT_OPCODE(I32, I32, I32, ___, 0, 0, 0x46, I32Eq, "i32.eq", "==")                                       \
    WABT_OPCODE(I32, I32, I32, ___, 0, 0, 0x47, I32Ne, "i32.ne", "!=")                                       \
    WABT_OPCODE(I32, I32, I32, ___, 0, 0, 0x48, I32LtS, "i32.lt_s", "<")                                     \
    WABT_OPCODE(I32, I32, I32, ___, 0, 0, 0x49, I32LtU, "i32.lt_u", "<")                                     \
    WABT_OPCODE(I32, I32, I32, ___, 0, 0, 0x4a, I32GtS, "i32.gt_s", ">")                                     \
    WABT_OPCODE(I32, I32, I32, ___, 0, 0, 0x4b, I32GtU, "i32.gt_u", ">")                                     \
    WABT_OPCODE(I32, I32, I32, ___, 0, 0, 0x4c, I32LeS, "i32.le_s", "<=")                                    \
    WABT_OPCODE(I32, I32, I32, ___, 0, 0, 0x4d, I32LeU, "i32.le_u", "<=")                                    \
    WABT_OPCODE(I32, I32, I32, ___, 0, 0, 0x4e, I32GeS, "i32.ge_s", ">=")                                    \
    WABT_OPCODE(I32, I32, I32, ___, 0, 0, 0x4f, I32GeU, "i32.ge_u", ">=")                                    \
    WABT_OPCODE(I32, I64, ___, ___, 0, 0, 0x50, I64Eqz, "i64.eqz", "eqz")                                    \
    WABT_OPCODE(I32, I64, I64, ___, 0, 0, 0x51, I64Eq, "i64.eq", "==")                                       \
    WABT_OPCODE(I32, I64, I64, ___, 0, 0, 0x52, I64Ne, "i64.ne", "!=")                                       \
    WABT_OPCODE(I32, I64, I64, ___, 0, 0, 0x53, I64LtS, "i64.lt_s", "<")                                     \
    WABT_OPCODE(I32, I64, I64, ___, 0, 0, 0x54, I64LtU, "i64.lt_u", "<")                                     \
    WABT_OPCODE(I32, I64, I64, ___, 0, 0, 0x55, I64GtS, "i64.gt_s", ">")                                     \
    WABT_OPCODE(I32, I64, I64, ___, 0, 0, 0x56, I64GtU, "i64.gt_u", ">")                                     \
    WABT_OPCODE(I32, I64, I64, ___, 0, 0, 0x57, I64LeS, "i64.le_s", "<=")                                    \
    WABT_OPCODE(I32, I64, I64, ___, 0, 0, 0x58, I64LeU, "i64.le_u", "<=")                                    \
    WABT_OPCODE(I32, I64, I64, ___, 0, 0, 0x59, I64GeS, "i64.ge_s", ">=")                                    \
    WABT_OPCODE(I32, I64, I64, ___, 0, 0, 0x5a, I64GeU, "i64.ge_u", ">=")                                    \
    WABT_OPCODE(I32, F32, F32, ___, 0, 0, 0x5b, F32Eq, "f32.eq", "==")                                       \
    WABT_OPCODE(I32, F32, F32, ___, 0, 0, 0x5c, F32Ne, "f32.ne", "!=")                                       \
    WABT_OPCODE(I32, F32, F32, ___, 0, 0, 0x5d, F32Lt, "f32.lt", "<")                                        \
    WABT_OPCODE(I32, F32, F32, ___, 0, 0, 0x5e, F32Gt, "f32.gt", ">")                                        \
    WABT_OPCODE(I32, F32, F32, ___, 0, 0, 0x5f, F32Le, "f32.le", "<=")                                       \
    WABT_OPCODE(I32, F32, F32, ___, 0, 0, 0x60, F32Ge, "f32.ge", ">=")                                       \
    WABT_OPCODE(I32, F64, F64, ___, 0, 0, 0x61, F64Eq, "f64.eq", "==")                                       \
    WABT_OPCODE(I32, F64, F64, ___, 0, 0, 0x62, F64Ne, "f64.ne", "!=")                                       \
    WABT_OPCODE(I32, F64, F64, ___, 0, 0, 0x63, F64Lt, "f64.lt", "<")                                        \
    WABT_OPCODE(I32, F64, F64, ___, 0, 0, 0x64, F64Gt, "f64.gt", ">")                                        \
    WABT_OPCODE(I32, F64, F64, ___, 0, 0, 0x65, F64Le, "f64.le", "<=")                                       \
    WABT_OPCODE(I32, F64, F64, ___, 0, 0, 0x66, F64Ge, "f64.ge", ">=")                                       \
    WABT_OPCODE(I32, I32, ___, ___, 0, 0, 0x67, I32Clz, "i32.clz", "clz")                                    \
    WABT_OPCODE(I32, I32, ___, ___, 0, 0, 0x68, I32Ctz, "i32.ctz", "ctz")                                    \
    WABT_OPCODE(I32, I32, ___, ___, 0, 0, 0x69, I32Popcnt, "i32.popcnt", "popcnt")                           \
    WABT_OPCODE(I32, I32, I32, ___, 0, 0, 0x6a, I32Add, "i32.add", "+")                                      \
    WABT_OPCODE(I32, I32, I32, ___, 0, 0, 0x6b, I32Sub, "i32.sub", "-")                                      \
    WABT_OPCODE(I32, I32, I32, ___, 0, 0, 0x6c, I32Mul, "i32.mul", "*")                                      \
    WABT_OPCODE(I32, I32, I32, ___, 0, 0, 0x6d, I32DivS, "i32.div_s", "/")                                   \
    WABT_OPCODE(I32, I32, I32, ___, 0, 0, 0x6e, I32DivU, "i32.div_u", "/")                                   \
    WABT_OPCODE(I32, I32, I32, ___, 0, 0, 0x6f, I32RemS, "i32.rem_s", "%")                                   \
    WABT_OPCODE(I32, I32, I32, ___, 0, 0, 0x70, I32RemU, "i32.rem_u", "%")                                   \
    WABT_OPCODE(I32, I32, I32, ___, 0, 0, 0x71, I32And, "i32.and", "&")                                      \
    WABT_OPCODE(I32, I32, I32, ___, 0, 0, 0x72, I32Or, "i32.or", "|")                                        \
    WABT_OPCODE(I32, I32, I32, ___, 0, 0, 0x73, I32Xor, "i32.xor", "^")                                      \
    WABT_OPCODE(I32, I32, I32, ___, 0, 0, 0x74, I32Shl, "i32.shl", "<<")                                     \
    WABT_OPCODE(I32, I32, I32, ___, 0, 0, 0x75, I32ShrS, "i32.shr_s", ">>")                                  \
    WABT_OPCODE(I32, I32, I32, ___, 0, 0, 0x76, I32ShrU, "i32.shr_u", ">>")                                  \
    WABT_OPCODE(I32, I32, I32, ___, 0, 0, 0x77, I32Rotl, "i32.rotl", "<<")                                   \
    WABT_OPCODE(I32, I32, I32, ___, 0, 0, 0x78, I32Rotr, "i32.rotr", ">>")                                   \
    WABT_OPCODE(I64, I64, ___, ___, 0, 0, 0x79, I64Clz, "i64.clz", "clz")                                    \
    WABT_OPCODE(I64, I64, ___, ___, 0, 0, 0x7a, I64Ctz, "i64.ctz", "ctz")                                    \
    WABT_OPCODE(I64, I64, ___, ___, 0, 0, 0x7b, I64Popcnt, "i64.popcnt", "popcnt")                           \
    WABT_OPCODE(I64, I64, I64, ___, 0, 0, 0x7c, I64Add, "i64.add", "+")                                      \
    WABT_OPCODE(I64, I64, I64, ___, 0, 0, 0x7d, I64Sub, "i64.sub", "-")                                      \
    WABT_OPCODE(I64, I64, I64, ___, 0, 0, 0x7e, I64Mul, "i64.mul", "*")                                      \
    WABT_OPCODE(I64, I64, I64, ___, 0, 0, 0x7f, I64DivS, "i64.div_s", "/")                                   \
    WABT_OPCODE(I64, I64, I64, ___, 0, 0, 0x80, I64DivU, "i64.div_u", "/")                                   \
    WABT_OPCODE(I64, I64, I64, ___, 0, 0, 0x81, I64RemS, "i64.rem_s", "%")                                   \
    WABT_OPCODE(I64, I64, I64, ___, 0, 0, 0x82, I64RemU, "i64.rem_u", "%")                                   \
    WABT_OPCODE(I64, I64, I64, ___, 0, 0, 0x83, I64And, "i64.and", "&")                                      \
    WABT_OPCODE(I64, I64, I64, ___, 0, 0, 0x84, I64Or, "i64.or", "|")                                        \
    WABT_OPCODE(I64, I64, I64, ___, 0, 0, 0x85, I64Xor, "i64.xor", "^")                                      \
    WABT_OPCODE(I64, I64, I64, ___, 0, 0, 0x86, I64Shl, "i64.shl", "<<")                                     \
    WABT_OPCODE(I64, I64, I64, ___, 0, 0, 0x87, I64ShrS, "i64.shr_s", ">>")                                  \
    WABT_OPCODE(I64, I64, I64, ___, 0, 0, 0x88, I64ShrU, "i64.shr_u", ">>")                                  \
    WABT_OPCODE(I64, I64, I64, ___, 0, 0, 0x89, I64Rotl, "i64.rotl", "<<")                                   \
    WABT_OPCODE(I64, I64, I64, ___, 0, 0, 0x8a, I64Rotr, "i64.rotr", ">>")                                   \
    WABT_OPCODE(F32, F32, F32, ___, 0, 0, 0x8b, F32Abs, "f32.abs", "abs")                                    \
    WABT_OPCODE(F32, F32, F32, ___, 0, 0, 0x8c, F32Neg, "f32.neg", "-")                                      \
    WABT_OPCODE(F32, F32, F32, ___, 0, 0, 0x8d, F32Ceil, "f32.ceil", "ceil")                                 \
    WABT_OPCODE(F32, F32, F32, ___, 0, 0, 0x8e, F32Floor, "f32.floor", "floor")                              \
    WABT_OPCODE(F32, F32, F32, ___, 0, 0, 0x8f, F32Trunc, "f32.trunc", "trunc")                              \
    WABT_OPCODE(F32, F32, F32, ___, 0, 0, 0x90, F32Nearest, "f32.nearest", "nearest")                        \
    WABT_OPCODE(F32, F32, F32, ___, 0, 0, 0x91, F32Sqrt, "f32.sqrt", "sqrt")                                 \
    WABT_OPCODE(F32, F32, F32, ___, 0, 0, 0x92, F32Add, "f32.add", "+")                                      \
    WABT_OPCODE(F32, F32, F32, ___, 0, 0, 0x93, F32Sub, "f32.sub", "-")                                      \
    WABT_OPCODE(F32, F32, F32, ___, 0, 0, 0x94, F32Mul, "f32.mul", "*")                                      \
    WABT_OPCODE(F32, F32, F32, ___, 0, 0, 0x95, F32Div, "f32.div", "/")                                      \
    WABT_OPCODE(F32, F32, F32, ___, 0, 0, 0x96, F32Min, "f32.min", "min")                                    \
    WABT_OPCODE(F32, F32, F32, ___, 0, 0, 0x97, F32Max, "f32.max", "max")                                    \
    WABT_OPCODE(F32, F32, F32, ___, 0, 0, 0x98, F32Copysign, "f32.copysign", "copysign")                     \
    WABT_OPCODE(F64, F64, F64, ___, 0, 0, 0x99, F64Abs, "f64.abs", "abs")                                    \
    WABT_OPCODE(F64, F64, F64, ___, 0, 0, 0x9a, F64Neg, "f64.neg", "-")                                      \
    WABT_OPCODE(F64, F64, F64, ___, 0, 0, 0x9b, F64Ceil, "f64.ceil", "ceil")                                 \
    WABT_OPCODE(F64, F64, F64, ___, 0, 0, 0x9c, F64Floor, "f64.floor", "floor")                              \
    WABT_OPCODE(F64, F64, F64, ___, 0, 0, 0x9d, F64Trunc, "f64.trunc", "trunc")                              \
    WABT_OPCODE(F64, F64, F64, ___, 0, 0, 0x9e, F64Nearest, "f64.nearest", "nearest")                        \
    WABT_OPCODE(F64, F64, F64, ___, 0, 0, 0x9f, F64Sqrt, "f64.sqrt", "sqrt")                                 \
    WABT_OPCODE(F64, F64, F64, ___, 0, 0, 0xa0, F64Add, "f64.add", "+")                                      \
    WABT_OPCODE(F64, F64, F64, ___, 0, 0, 0xa1, F64Sub, "f64.sub", "-")                                      \
    WABT_OPCODE(F64, F64, F64, ___, 0, 0, 0xa2, F64Mul, "f64.mul", "*")                                      \
    WABT_OPCODE(F64, F64, F64, ___, 0, 0, 0xa3, F64Div, "f64.div", "/")                                      \
    WABT_OPCODE(F64, F64, F64, ___, 0, 0, 0xa4, F64Min, "f64.min", "min")                                    \
    WABT_OPCODE(F64, F64, F64, ___, 0, 0, 0xa5, F64Max, "f64.max", "max")                                    \
    WABT_OPCODE(F64, F64, F64, ___, 0, 0, 0xa6, F64Copysign, "f64.copysign", "copysign")                     \
    WABT_OPCODE(I32, I64, ___, ___, 0, 0, 0xa7, I32WrapI64, "i32.wrap_i64", "")                              \
    WABT_OPCODE(I32, F32, ___, ___, 0, 0, 0xa8, I32TruncF32S, "i32.trunc_f32_s", "")                         \
    WABT_OPCODE(I32, F32, ___, ___, 0, 0, 0xa9, I32TruncF32U, "i32.trunc_f32_u", "")                         \
    WABT_OPCODE(I32, F64, ___, ___, 0, 0, 0xaa, I32TruncF64S, "i32.trunc_f64_s", "")                         \
    WABT_OPCODE(I32, F64, ___, ___, 0, 0, 0xab, I32TruncF64U, "i32.trunc_f64_u", "")                         \
    WABT_OPCODE(I64, I32, ___, ___, 0, 0, 0xac, I64ExtendI32S, "i64.extend_i32_s", "")                       \
    WABT_OPCODE(I64, I32, ___, ___, 0, 0, 0xad, I64ExtendI32U, "i64.extend_i32_u", "")                       \
    WABT_OPCODE(I64, F32, ___, ___, 0, 0, 0xae, I64TruncF32S, "i64.trunc_f32_s", "")                         \
    WABT_OPCODE(I64, F32, ___, ___, 0, 0, 0xaf, I64TruncF32U, "i64.trunc_f32_u", "")                         \
    WABT_OPCODE(I64, F64, ___, ___, 0, 0, 0xb0, I64TruncF64S, "i64.trunc_f64_s", "")                         \
    WABT_OPCODE(I64, F64, ___, ___, 0, 0, 0xb1, I64TruncF64U, "i64.trunc_f64_u", "")                         \
    WABT_OPCODE(F32, I32, ___, ___, 0, 0, 0xb2, F32ConvertI32S, "f32.convert_i32_s", "")                     \
    WABT_OPCODE(F32, I32, ___, ___, 0, 0, 0xb3, F32ConvertI32U, "f32.convert_i32_u", "")                     \
    WABT_OPCODE(F32, I64, ___, ___, 0, 0, 0xb4, F32ConvertI64S, "f32.convert_i64_s", "")                     \
    WABT_OPCODE(F32, I64, ___, ___, 0, 0, 0xb5, F32ConvertI64U, "f32.convert_i64_u", "")                     \
    WABT_OPCODE(F32, F64, ___, ___, 0, 0, 0xb6, F32DemoteF64, "f32.demote_f64", "")                          \
    WABT_OPCODE(F64, I32, ___, ___, 0, 0, 0xb7, F64ConvertI32S, "f64.convert_i32_s", "")                     \
    WABT_OPCODE(F64, I32, ___, ___, 0, 0, 0xb8, F64ConvertI32U, "f64.convert_i32_u", "")                     \
    WABT_OPCODE(F64, I64, ___, ___, 0, 0, 0xb9, F64ConvertI64S, "f64.convert_i64_s", "")                     \
    WABT_OPCODE(F64, I64, ___, ___, 0, 0, 0xba, F64ConvertI64U, "f64.convert_i64_u", "")                     \
    WABT_OPCODE(F64, F32, ___, ___, 0, 0, 0xbb, F64PromoteF32, "f64.promote_f32", "")                        \
    WABT_OPCODE(I32, I32, ___, ___, 0, 0, 0xC0, I32Extend8S, "i32.extend8_s", "")                            \
    WABT_OPCODE(I32, I32, ___, ___, 0, 0, 0xC1, I32Extend16S, "i32.extend16_s", "")                          \
    WABT_OPCODE(I64, I64, ___, ___, 0, 0, 0xC2, I64Extend8S, "i64.extend8_s", "")                            \
    WABT_OPCODE(I64, I64, ___, ___, 0, 0, 0xC3, I64Extend16S, "i64.extend16_s", "")                          \
    WABT_OPCODE(I64, I64, ___, ___, 0, 0, 0xC4, I64Extend32S, "i64.extend32_s", "")                          \
    WABT_OPCODE(I32, F32, ___, ___, 0, 0xfc, 0x00, I32TruncSatF32S, "i32.trunc_sat_f32_s", "")               \
    WABT_OPCODE(I32, F32, ___, ___, 0, 0xfc, 0x01, I32TruncSatF32U, "i32.trunc_sat_f32_u", "")               \
    WABT_OPCODE(I32, F64, ___, ___, 0, 0xfc, 0x02, I32TruncSatF64S, "i32.trunc_sat_f64_s", "")               \
    WABT_OPCODE(I32, F64, ___, ___, 0, 0xfc, 0x03, I32TruncSatF64U, "i32.trunc_sat_f64_u", "")               \
    WABT_OPCODE(I64, F32, ___, ___, 0, 0xfc, 0x04, I64TruncSatF32S, "i64.trunc_sat_f32_s", "")               \
    WABT_OPCODE(I64, F32, ___, ___, 0, 0xfc, 0x05, I64TruncSatF32U, "i64.trunc_sat_f32_u", "")               \
    WABT_OPCODE(I64, F64, ___, ___, 0, 0xfc, 0x06, I64TruncSatF64S, "i64.trunc_sat_f64_s", "")               \
    WABT_OPCODE(I64, F64, ___, ___, 0, 0xfc, 0x07, I64TruncSatF64U, "i64.trunc_sat_f64_u", "")               \
    WABT_OPCODE(___, I32, I32, I32, 0, 0xfc, 0x08, MemoryInit, "memory.init", "")                            \
    WABT_OPCODE(___, ___, ___, ___, 0, 0xfc, 0x09, DataDrop, "data.drop", "")                                \
    WABT_OPCODE(___, I32, I32, I32, 0, 0xfc, 0x0a, MemoryCopy, "memory.copy", "")                            \
    WABT_OPCODE(___, I32, I32, I32, 0, 0xfc, 0x0b, MemoryFill, "memory.fill", "")                            \
    WABT_OPCODE(___, I32, I32, I32, 0, 0xfc, 0x0c, TableInit, "table.init", "")                              \
    WABT_OPCODE(___, ___, ___, ___, 0, 0xfc, 0x0d, ElemDrop, "elem.drop", "")                                \
    WABT_OPCODE(___, I32, I32, I32, 0, 0xfc, 0x0e, TableCopy, "table.copy", "")                              \
    WABT_OPCODE(___, I32, ___, ___, 0, 0, 0x25, TableGet, "table.get", "")                                   \
    WABT_OPCODE(___, I32, ___, ___, 0, 0, 0x26, TableSet, "table.set", "")                                   \
    WABT_OPCODE(___, ___, I32, ___, 0, 0xfc, 0x0f, TableGrow, "table.grow", "")                              \
    WABT_OPCODE(___, ___, ___, ___, 0, 0xfc, 0x10, TableSize, "table.size", "")                              \
    WABT_OPCODE(___, I32, ___, I32, 0, 0xfc, 0x11, TableFill, "table.fill", "")                              \
    WABT_OPCODE(___, ___, ___, ___, 0, 0, 0xd2, RefFunc, "ref.func", "")                                     \
    WABT_OPCODE(___, ___, ___, ___, 0, 0, 0xe7, Move32, "move_32", "")                                       \
    WABT_OPCODE(___, ___, ___, ___, 0, 0, 0xe8, Move64, "move_64", "")                                       \
    WABT_OPCODE(___, ___, ___, ___, 0, 0, 0xe9, Jump, "jump", "")                                            \
    WABT_OPCODE(___, I32, ___, ___, 0, 0, 0xe0, JumpIfTrue, "jump_if_true", "")                              \
    WABT_OPCODE(___, I32, ___, ___, 0, 0, 0xea, JumpIfFalse, "jump_if_false", "")                            \
    WABT_OPCODE(___, ___, ___, ___, 0, 0, 0xeb, GlobalGet32, "global_get_32", "")                            \
    WABT_OPCODE(___, ___, ___, ___, 0, 0, 0xec, GlobalGet64, "global_get_64", "")                            \
    WABT_OPCODE(___, ___, ___, ___, 0, 0, 0xed, GlobalSet32, "global_set_32", "")                            \
    WABT_OPCODE(___, ___, ___, ___, 0, 0, 0xee, GlobalSet64, "global_set_64", "")                            \
    WABT_OPCODE(___, ___, ___, ___, 0, 0, 0xef, Const32, "const_32", "")                                     \
    WABT_OPCODE(___, ___, ___, ___, 0, 0, 0xe0, Const64, "const_64", "")                                     \
    WABT_OPCODE(___, ___, ___, ___, 0, 0, 0xe1, Load32, "load_32", "")                                       \
    WABT_OPCODE(___, ___, ___, ___, 0, 0, 0xe2, Load64, "load_64", "")                                       \
    WABT_OPCODE(___, ___, ___, ___, 0, 0, 0xe3, Store32, "store_32", "")                                     \
    WABT_OPCODE(___, ___, ___, ___, 0, 0, 0xe4, Store64, "store_64", "")                                     \
    WABT_OPCODE(___, ___, ___, ___, 0, 0, 0xe5, FillOpcodeTable, "fill_opcode_table", "")                    \
    WABT_OPCODE(I32, I32, ___, ___, 4, 0xfe, 0x10, I32AtomicLoad, "i32.atomic.load", "")                     \
    WABT_OPCODE(I64, I32, ___, ___, 8, 0xfe, 0x11, I64AtomicLoad, "i64.atomic.load", "")                     \
    WABT_OPCODE(I32, I32, ___, ___, 1, 0xfe, 0x12, I32AtomicLoad8U, "i32.atomic.load8_u", "")                \
    WABT_OPCODE(I32, I32, ___, ___, 2, 0xfe, 0x13, I32AtomicLoad16U, "i32.atomic.load16_u", "")              \
    WABT_OPCODE(I64, I32, ___, ___, 1, 0xfe, 0x14, I64AtomicLoad8U, "i64.atomic.load8_u", "")                \
    WABT_OPCODE(I64, I32, ___, ___, 2, 0xfe, 0x15, I64AtomicLoad16U, "i64.atomic.load16_u", "")              \
    WABT_OPCODE(I64, I32, ___, ___, 4, 0xfe, 0x16, I64AtomicLoad32U, "i64.atomic.load32_u", "")              \
    WABT_OPCODE(___, I32, I32, ___, 4, 0xfe, 0x17, I32AtomicStore, "i32.atomic.store", "")                   \
    WABT_OPCODE(___, I32, I64, ___, 8, 0xfe, 0x18, I64AtomicStore, "i64.atomic.store", "")                   \
    WABT_OPCODE(___, I32, I32, ___, 1, 0xfe, 0x19, I32AtomicStore8, "i32.atomic.store8", "")                 \
    WABT_OPCODE(___, I32, I32, ___, 2, 0xfe, 0x1a, I32AtomicStore16, "i32.atomic.store16", "")               \
    WABT_OPCODE(___, I32, I64, ___, 1, 0xfe, 0x1b, I64AtomicStore8, "i64.atomic.store8", "")                 \
    WABT_OPCODE(___, I32, I64, ___, 2, 0xfe, 0x1c, I64AtomicStore16, "i64.atomic.store16", "")               \
    WABT_OPCODE(___, I32, I64, ___, 4, 0xfe, 0x1d, I64AtomicStore32, "i64.atomic.store32", "")               \
    WABT_OPCODE(I32, I32, I32, ___, 4, 0xfe, 0x1e, I32AtomicRmwAdd, "i32.atomic.rmw.add", "")                \
    WABT_OPCODE(I64, I32, I64, ___, 8, 0xfe, 0x1f, I64AtomicRmwAdd, "i64.atomic.rmw.add", "")                \
    WABT_OPCODE(I32, I32, I32, ___, 1, 0xfe, 0x20, I32AtomicRmw8AddU, "i32.atomic.rmw8.add_u", "")           \
    WABT_OPCODE(I32, I32, I32, ___, 2, 0xfe, 0x21, I32AtomicRmw16AddU, "i32.atomic.rmw16.add_u", "")         \
    WABT_OPCODE(I64, I32, I64, ___, 1, 0xfe, 0x22, I64AtomicRmw8AddU, "i64.atomic.rmw8.add_u", "")           \
    WABT_OPCODE(I64, I32, I64, ___, 2, 0xfe, 0x23, I64AtomicRmw16AddU, "i64.atomic.rmw16.add_u", "")         \
    WABT_OPCODE(I64, I32, I64, ___, 4, 0xfe, 0x24, I64AtomicRmw32AddU, "i64.atomic.rmw32.add_u", "")         \
    WABT_OPCODE(I32, I32, I32, ___, 4, 0xfe, 0x25, I32AtomicRmwSub, "i32.atomic.rmw.sub", "")                \
    WABT_OPCODE(I64, I32, I64, ___, 8, 0xfe, 0x26, I64AtomicRmwSub, "i64.atomic.rmw.sub", "")                \
    WABT_OPCODE(I32, I32, I32, ___, 1, 0xfe, 0x27, I32AtomicRmw8SubU, "i32.atomic.rmw8.sub_u", "")           \
    WABT_OPCODE(I32, I32, I32, ___, 2, 0xfe, 0x28, I32AtomicRmw16SubU, "i32.atomic.rmw16.sub_u", "")         \
    WABT_OPCODE(I64, I32, I64, ___, 1, 0xfe, 0x29, I64AtomicRmw8SubU, "i64.atomic.rmw8.sub_u", "")           \
    WABT_OPCODE(I64, I32, I64, ___, 2, 0xfe, 0x2a, I64AtomicRmw16SubU, "i64.atomic.rmw16.sub_u", "")         \
    WABT_OPCODE(I64, I32, I64, ___, 4, 0xfe, 0x2b, I64AtomicRmw32SubU, "i64.atomic.rmw32.sub_u", "")         \
    WABT_OPCODE(I32, I32, I32, ___, 4, 0xfe, 0x2c, I32AtomicRmwAnd, "i32.atomic.rmw.and", "")                \
    WABT_OPCODE(I64, I32, I64, ___, 8, 0xfe, 0x2d, I64AtomicRmwAnd, "i64.atomic.rmw.and", "")                \
    WABT_OPCODE(I32, I32, I32, ___, 1, 0xfe, 0x2e, I32AtomicRmw8AndU, "i32.atomic.rmw8.and_u", "")           \
    WABT_OPCODE(I32, I32, I32, ___, 2, 0xfe, 0x2f, I32AtomicRmw16AndU, "i32.atomic.rmw16.and_u", "")         \
    WABT_OPCODE(I64, I32, I64, ___, 1, 0xfe, 0x30, I64AtomicRmw8AndU, "i64.atomic.rmw8.and_u", "")           \
    WABT_OPCODE(I64, I32, I64, ___, 2, 0xfe, 0x31, I64AtomicRmw16AndU, "i64.atomic.rmw16.and_u", "")         \
    WABT_OPCODE(I64, I32, I64, ___, 4, 0xfe, 0x32, I64AtomicRmw32AndU, "i64.atomic.rmw32.and_u", "")         \
    WABT_OPCODE(I32, I32, I32, ___, 4, 0xfe, 0x33, I32AtomicRmwOr, "i32.atomic.rmw.or", "")                  \
    WABT_OPCODE(I64, I32, I64, ___, 8, 0xfe, 0x34, I64AtomicRmwOr, "i64.atomic.rmw.or", "")                  \
    WABT_OPCODE(I32, I32, I32, ___, 1, 0xfe, 0x35, I32AtomicRmw8OrU, "i32.atomic.rmw8.or_u", "")             \
    WABT_OPCODE(I32, I32, I32, ___, 2, 0xfe, 0x36, I32AtomicRmw16OrU, "i32.atomic.rmw16.or_u", "")           \
    WABT_OPCODE(I64, I32, I64, ___, 1, 0xfe, 0x37, I64AtomicRmw8OrU, "i64.atomic.rmw8.or_u", "")             \
    WABT_OPCODE(I64, I32, I64, ___, 2, 0xfe, 0x38, I64AtomicRmw16OrU, "i64.atomic.rmw16.or_u", "")           \
    WABT_OPCODE(I64, I32, I64, ___, 4, 0xfe, 0x39, I64AtomicRmw32OrU, "i64.atomic.rmw32.or_u", "")           \
    WABT_OPCODE(I32, I32, I32, ___, 4, 0xfe, 0x3a, I32AtomicRmwXor, "i32.atomic.rmw.xor", "")                \
    WABT_OPCODE(I64, I32, I64, ___, 8, 0xfe, 0x3b, I64AtomicRmwXor, "i64.atomic.rmw.xor", "")                \
    WABT_OPCODE(I32, I32, I32, ___, 1, 0xfe, 0x3c, I32AtomicRmw8XorU, "i32.atomic.rmw8.xor_u", "")           \
    WABT_OPCODE(I32, I32, I32, ___, 2, 0xfe, 0x3d, I32AtomicRmw16XorU, "i32.atomic.rmw16.xor_u", "")         \
    WABT_OPCODE(I64, I32, I64, ___, 1, 0xfe, 0x3e, I64AtomicRmw8XorU, "i64.atomic.rmw8.xor_u", "")           \
    WABT_OPCODE(I64, I32, I64, ___, 2, 0xfe, 0x3f, I64AtomicRmw16XorU, "i64.atomic.rmw16.xor_u", "")         \
    WABT_OPCODE(I64, I32, I64, ___, 4, 0xfe, 0x40, I64AtomicRmw32XorU, "i64.atomic.rmw32.xor_u", "")         \
    WABT_OPCODE(I32, I32, I32, ___, 4, 0xfe, 0x41, I32AtomicRmwXchg, "i32.atomic.rmw.xchg", "")              \
    WABT_OPCODE(I64, I32, I64, ___, 8, 0xfe, 0x42, I64AtomicRmwXchg, "i64.atomic.rmw.xchg", "")              \
    WABT_OPCODE(I32, I32, I32, ___, 1, 0xfe, 0x43, I32AtomicRmw8XchgU, "i32.atomic.rmw8.xchg_u", "")         \
    WABT_OPCODE(I32, I32, I32, ___, 2, 0xfe, 0x44, I32AtomicRmw16XchgU, "i32.atomic.rmw16.xchg_u", "")       \
    WABT_OPCODE(I64, I32, I64, ___, 1, 0xfe, 0x45, I64AtomicRmw8XchgU, "i64.atomic.rmw8.xchg_u", "")         \
    WABT_OPCODE(I64, I32, I64, ___, 2, 0xfe, 0x46, I64AtomicRmw16XchgU, "i64.atomic.rmw16.xchg_u", "")       \
    WABT_OPCODE(I64, I32, I64, ___, 4, 0xfe, 0x47, I64AtomicRmw32XchgU, "i64.atomic.rmw32.xchg_u", "")       \
    WABT_OPCODE(I32, I32, I32, I32, 4, 0xfe, 0x48, I32AtomicRmwCmpxchg, "i32.atomic.rmw.cmpxchg", "")        \
    WABT_OPCODE(I64, I32, I64, I64, 8, 0xfe, 0x49, I64AtomicRmwCmpxchg, "i64.atomic.rmw.cmpxchg", "")        \
    WABT_OPCODE(I32, I32, I32, I32, 1, 0xfe, 0x4a, I32AtomicRmw8CmpxchgU, "i32.atomic.rmw8.cmpxchg_u", "")   \
    WABT_OPCODE(I32, I32, I32, I32, 2, 0xfe, 0x4b, I32AtomicRmw16CmpxchgU, "i32.atomic.rmw16.cmpxchg_u", "") \
    WABT_OPCODE(I64, I32, I64, I64, 1, 0xfe, 0x4c, I64AtomicRmw8CmpxchgU, "i64.atomic.rmw8.cmpxchg_u", "")   \
    WABT_OPCODE(I64, I32, I64, I64, 2, 0xfe, 0x4d, I64AtomicRmw16CmpxchgU, "i64.atomic.rmw16.cmpxchg_u", "") \
    WABT_OPCODE(I64, I32, I64, I64, 4, 0xfe, 0x4e, I64AtomicRmw32CmpxchgU, "i64.atomic.rmw32.cmpxchg_u", "")

class OpcodeTable {
public:
    OpcodeTable();
#if defined(WALRUS_ENABLE_COMPUTED_GOTO)
    void* m_addressTable[InvalidOpcode];
    std::unordered_map<void*, int> m_addressToOpcodeTable;
#endif
};

extern OpcodeTable g_opcodeTable;

struct ByteCodeInfo {
    enum ByteCodeType { ___,
                        I32,
                        I64,
                        F32,
                        F64,
                        V128 };
    OpcodeKind m_code;
    ByteCodeType m_resultType;
    ByteCodeType m_paramTypes[3];
    const char* m_name;

    size_t stackShrinkSize() const
    {
        ASSERT(m_code != OpcodeKind::InvalidOpcode);
        return byteCodeTypeToMemorySize(m_paramTypes[0]) + byteCodeTypeToMemorySize(m_paramTypes[1]) + byteCodeTypeToMemorySize(m_paramTypes[2]);
    }

    size_t stackGrowSize() const
    {
        ASSERT(m_code != OpcodeKind::InvalidOpcode);
        return byteCodeTypeToMemorySize(m_resultType);
    }

    static size_t byteCodeTypeToMemorySize(ByteCodeType tp)
    {
        switch (tp) {
        case I32:
            return stackAllocatedSize<int32_t>();
        case F32:
            return stackAllocatedSize<float>();
        case I64:
            return stackAllocatedSize<int64_t>();
        case F64:
            return stackAllocatedSize<double>();
        case V128:
            return 16;
        default:
            return 0;
        }
    }
};

extern ByteCodeInfo g_byteCodeInfo[OpcodeKind::InvalidOpcode];

} // namespace Walrus

#endif // __WalrusOpcode__
