/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

/* Only included by jit-backend.cc */

constexpr uint8_t getHighRegister(sljit_s32 reg)
{
    return reg + 1;
}

namespace SimdOp {

#if (defined SLJIT_CONFIG_ARM_THUMB2 && SLJIT_CONFIG_ARM_THUMB2)
constexpr uint8_t sizeOffset = 20;
constexpr uint8_t singleSizeOffset = 18;
constexpr uint32_t unsignedBit = 0b1 << 28;
constexpr uint32_t lBit = 0b1 << 7;

enum Type : uint32_t {
    vadd = 0xef000840,
    vand = 0xef000150,
    vceq = 0xff000850,
    vcge = 0xef000350,
    vcgt = 0xef000340,
    vmlal = 0xef800800,
    vmul = 0xef000950,
    vmull = 0xef800c00,
    vmvn = 0xffb005c0,
    vneg = 0xffb103c0,
    vorn = 0xef300150,
    vpaddl = 0xffb00240,
    vrev64 = 0xffB00040,
    vqadd = 0xef000050,
    vqsub = 0xef000250,
    vshlImm = 0xef8005D0,
    vshrImm = 0xef800050,
    vsub = 0xff000840,
    vtrn = 0xffB20080,
};
#else
constexpr uint8_t sizeOffset = 0;

enum Type : uint32_t {
    // TODO
};
#endif

enum IntSizeType : uint32_t {
    I8 = 0x0 << sizeOffset,
    I16 = 0x1 << sizeOffset,
    I32 = 0x2 << sizeOffset,
    I64 = 0x3 << sizeOffset,
};

enum SingleSizeType : uint32_t {
    S8 = 0x0 << singleSizeOffset,
    S16 = 0x1 << singleSizeOffset,
    S32 = 0x2 << singleSizeOffset,
};

enum ShiftType : uint32_t {
    SHL32 = 0b1 << 21,
};

}; // namespace SimdOp

static void simdEmitOp(sljit_compiler* compiler, uint32_t opcode, sljit_s32 vd, sljit_s32 vn, sljit_s32 vm)
{
    vd = sljit_get_register_index(SLJIT_SIMD_REG_64, vd);
    vm = sljit_get_register_index(SLJIT_SIMD_REG_64, vm);

    if (vn >= SLJIT_FR0) {
        vn = sljit_get_register_index(SLJIT_SIMD_REG_64, vn);
    }

    opcode |= (uint32_t)vm | ((uint32_t)vd << 12) | ((uint32_t)vn << 16);
    sljit_emit_op_custom(compiler, &opcode, sizeof(uint32_t));
}

static void simdEmitI64x2Mul(sljit_compiler* compiler, sljit_s32 vd, sljit_s32 vn, sljit_s32 vm)
{
    auto tmpReg1 = SLJIT_FR4;

    sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128, tmpReg1, vn, 0);
    simdEmitOp(compiler, SimdOp::vtrn | SimdOp::S32, tmpReg1, 0, getHighRegister(tmpReg1));
    simdEmitOp(compiler, SimdOp::vtrn | SimdOp::S32, vm, 0, getHighRegister(vm));
    simdEmitOp(compiler, SimdOp::vmull | SimdOp::I32 | SimdOp::unsignedBit, vd, tmpReg1, getHighRegister(vm));
    simdEmitOp(compiler, SimdOp::vmlal | SimdOp::I32 | SimdOp::unsignedBit, vd, getHighRegister(tmpReg1), vm);
    simdEmitOp(compiler, SimdOp::vshlImm | SimdOp::SHL32, vd, 0, vd);
    simdEmitOp(compiler, SimdOp::vmlal | SimdOp::I32 | SimdOp::unsignedBit, vd, tmpReg1, vm);
}

static void simdEmitI64x2Neg(sljit_compiler* compiler, sljit_s32 vd, sljit_s32 vm)
{
    auto tmpReg1 = SLJIT_FR4;

    sljit_emit_simd_replicate(compiler, SLJIT_SIMD_REG_128, tmpReg1, SLJIT_IMM, 0);
    simdEmitOp(compiler, SimdOp::vsub | SimdOp::I64, vd, tmpReg1, vm);
}

static void simdEmitI64x2Eq(sljit_compiler* compiler, sljit_s32 vd, sljit_s32 vn, sljit_s32 vm)
{
    auto tmpReg = SLJIT_FR4;

    simdEmitOp(compiler, SimdOp::vceq | SimdOp::I32, vd, vn, vm);
    simdEmitOp(compiler, SimdOp::vrev64 | SimdOp::S32, tmpReg, 0, vd);
    simdEmitOp(compiler, SimdOp::vand, vd, tmpReg, vd);
}

static void simdEmitI64x2Ne(sljit_compiler* compiler, sljit_s32 vd, sljit_s32 vn, sljit_s32 vm)
{
    auto tmpReg = SLJIT_FR4;

    simdEmitOp(compiler, SimdOp::vceq | SimdOp::I32, vd, vn, vm);
    simdEmitOp(compiler, SimdOp::vrev64 | SimdOp::S32, tmpReg, 0, vd);
    simdEmitOp(compiler, SimdOp::vmvn | SimdOp::I32, vd, 0, vd);
    simdEmitOp(compiler, SimdOp::vorn, vd, vd, tmpReg);
}

static void simdEmitI64x2GtS(sljit_compiler* compiler, sljit_s32 vd, sljit_s32 vn, sljit_s32 vm)
{
    simdEmitOp(compiler, SimdOp::vqsub | SimdOp::I64, vd, vm, vn);
    simdEmitOp(compiler, SimdOp::vshrImm | (0b1 << 16) | SimdOp::lBit, vd, 0, vd);
}

static void simdEmitI64x2GeS(sljit_compiler* compiler, sljit_s32 vd, sljit_s32 vn, sljit_s32 vm)
{
    simdEmitOp(compiler, SimdOp::vqsub | SimdOp::I64, vd, vn, vm);
    simdEmitOp(compiler, SimdOp::vshrImm | (0b1 << 16) | SimdOp::lBit, vd, 0, vd);
    simdEmitOp(compiler, SimdOp::vmvn | SimdOp::I32, vd, 0, vd);
}

static void emitUnarySIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[2];

    sljit_s32 type = SLJIT_SIMD_ELEM_128;

    switch (instr->opcode()) {
    case ByteCode::I8X16NegOpcode:
        type = SLJIT_SIMD_ELEM_8;
        break;
    case ByteCode::I16X8ExtaddPairwiseI8X16SOpcode:
    case ByteCode::I16X8ExtaddPairwiseI8X16UOpcode:
    case ByteCode::I16X8NegOpcode:
        type = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I32X4ExtaddPairwiseI16X8SOpcode:
    case ByteCode::I32X4ExtaddPairwiseI16X8UOpcode:
    case ByteCode::I32X4NegOpcode:
        type = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I64X2NegOpcode:
        type = SLJIT_SIMD_ELEM_64;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    simdOperandToArg(compiler, operands, args[0], type, SLJIT_FR0);

    args[1].set(operands + 1);
    sljit_s32 dst = GET_TARGET_REG(args[1].arg, SLJIT_FR0);

    switch (instr->opcode()) {
    case ByteCode::I8X16NegOpcode:
        simdEmitOp(compiler, SimdOp::Type::vneg | SimdOp::S8, dst, 0, args[0].arg);
        break;
    case ByteCode::I16X8ExtaddPairwiseI8X16SOpcode:
        simdEmitOp(compiler, SimdOp::Type::vpaddl | SimdOp::S8, dst, 0, args[0].arg);
        break;
    case ByteCode::I16X8ExtaddPairwiseI8X16UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vpaddl | SimdOp::S8 | (0x1 << 7), dst, 0, args[0].arg);
        break;
    case ByteCode::I16X8NegOpcode:
        simdEmitOp(compiler, SimdOp::Type::vneg | SimdOp::S16, dst, 0, args[0].arg);
        break;
    case ByteCode::I32X4ExtaddPairwiseI16X8SOpcode:
        simdEmitOp(compiler, SimdOp::Type::vpaddl | SimdOp::S16, dst, 0, args[0].arg);
        break;
    case ByteCode::I32X4ExtaddPairwiseI16X8UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vpaddl | SimdOp::S16 | (0x1 << 7), dst, 0, args[0].arg);
        break;
    case ByteCode::I32X4NegOpcode:
        simdEmitOp(compiler, SimdOp::Type::vneg | SimdOp::S32, dst, 0, args[0].arg);
        break;
    case ByteCode::I64X2NegOpcode:
        simdEmitI64x2Neg(compiler, dst, args[0].arg);
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    if (SLJIT_IS_MEM(args[1].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | type, dst, args[1].arg, args[1].argw);
    }
}

static void emitBinarySIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[3];

    sljit_s32 type = SLJIT_SIMD_ELEM_128;

    switch (instr->opcode()) {
    case ByteCode::I8X16AddOpcode:
    case ByteCode::I8X16SubOpcode:
    case ByteCode::I8X16AddSatSOpcode:
    case ByteCode::I8X16AddSatUOpcode:
    case ByteCode::I8X16SubSatSOpcode:
    case ByteCode::I8X16SubSatUOpcode:
    case ByteCode::I8X16EqOpcode:
    case ByteCode::I8X16NeOpcode:
    case ByteCode::I8X16LtSOpcode:
    case ByteCode::I8X16LtUOpcode:
    case ByteCode::I8X16LeSOpcode:
    case ByteCode::I8X16LeUOpcode:
    case ByteCode::I8X16GtSOpcode:
    case ByteCode::I8X16GtUOpcode:
    case ByteCode::I8X16GeSOpcode:
    case ByteCode::I8X16GeUOpcode:
    case ByteCode::I16X8ExtmulLowI8X16SOpcode:
    case ByteCode::I16X8ExtmulHighI8X16SOpcode:
    case ByteCode::I16X8ExtmulLowI8X16UOpcode:
    case ByteCode::I16X8ExtmulHighI8X16UOpcode:
        type = SLJIT_SIMD_ELEM_8;
        break;
    case ByteCode::I16X8AddOpcode:
    case ByteCode::I16X8SubOpcode:
    case ByteCode::I16X8MulOpcode:
    case ByteCode::I16X8AddSatSOpcode:
    case ByteCode::I16X8AddSatUOpcode:
    case ByteCode::I16X8SubSatSOpcode:
    case ByteCode::I16X8SubSatUOpcode:
    case ByteCode::I16X8EqOpcode:
    case ByteCode::I16X8NeOpcode:
    case ByteCode::I16X8LtSOpcode:
    case ByteCode::I16X8LtUOpcode:
    case ByteCode::I16X8LeSOpcode:
    case ByteCode::I16X8LeUOpcode:
    case ByteCode::I16X8GtSOpcode:
    case ByteCode::I16X8GtUOpcode:
    case ByteCode::I16X8GeSOpcode:
    case ByteCode::I16X8GeUOpcode:
    case ByteCode::I32X4ExtmulLowI16X8SOpcode:
    case ByteCode::I32X4ExtmulHighI16X8SOpcode:
    case ByteCode::I32X4ExtmulLowI16X8UOpcode:
    case ByteCode::I32X4ExtmulHighI16X8UOpcode:
        type = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I32X4AddOpcode:
    case ByteCode::I32X4SubOpcode:
    case ByteCode::I32X4MulOpcode:
    case ByteCode::I32X4EqOpcode:
    case ByteCode::I32X4NeOpcode:
    case ByteCode::I32X4LtSOpcode:
    case ByteCode::I32X4LtUOpcode:
    case ByteCode::I32X4LeSOpcode:
    case ByteCode::I32X4LeUOpcode:
    case ByteCode::I32X4GtSOpcode:
    case ByteCode::I32X4GtUOpcode:
    case ByteCode::I32X4GeSOpcode:
    case ByteCode::I32X4GeUOpcode:
    case ByteCode::I64X2ExtmulLowI32X4SOpcode:
    case ByteCode::I64X2ExtmulHighI32X4SOpcode:
    case ByteCode::I64X2ExtmulLowI32X4UOpcode:
    case ByteCode::I64X2ExtmulHighI32X4UOpcode:
        type = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I64X2AddOpcode:
    case ByteCode::I64X2SubOpcode:
    case ByteCode::I64X2MulOpcode:
    case ByteCode::I64X2EqOpcode:
    case ByteCode::I64X2NeOpcode:
    case ByteCode::I64X2LtSOpcode:
    case ByteCode::I64X2LeSOpcode:
    case ByteCode::I64X2GtSOpcode:
    case ByteCode::I64X2GeSOpcode:
        type = SLJIT_SIMD_ELEM_64;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    simdOperandToArg(compiler, operands, args[0], type, SLJIT_FR0);
    simdOperandToArg(compiler, operands + 1, args[1], type, SLJIT_FR2);

    args[2].set(operands + 2);
    sljit_s32 dst = GET_TARGET_REG(args[2].arg, SLJIT_FR0);

    switch (instr->opcode()) {
    case ByteCode::I8X16AddOpcode:
        simdEmitOp(compiler, SimdOp::Type::vadd | SimdOp::I8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16SubOpcode:
        simdEmitOp(compiler, SimdOp::Type::vsub | SimdOp::I8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16AddSatSOpcode:
        simdEmitOp(compiler, SimdOp::Type::vqadd | SimdOp::I8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16AddSatUOpcode:
        simdEmitOp(compiler, SimdOp::Type::vqadd | SimdOp::I8 | SimdOp::unsignedBit, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16SubSatSOpcode:
        simdEmitOp(compiler, SimdOp::Type::vqsub | SimdOp::I8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16SubSatUOpcode:
        simdEmitOp(compiler, SimdOp::Type::vqsub | SimdOp::I8 | SimdOp::unsignedBit, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16EqOpcode:
        simdEmitOp(compiler, SimdOp::Type::vceq | SimdOp::I8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16NeOpcode:
        simdEmitOp(compiler, SimdOp::Type::vceq | SimdOp::I8, dst, args[0].arg, args[1].arg);
        simdEmitOp(compiler, SimdOp::Type::vmvn | SimdOp::I8, dst, 0, dst);
        break;
    case ByteCode::I8X16LtSOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcgt | SimdOp::I8, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I8X16LtUOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcgt | SimdOp::I8 | SimdOp::unsignedBit, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I8X16LeSOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcge | SimdOp::I8, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I8X16LeUOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcge | SimdOp::I8 | SimdOp::unsignedBit, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I8X16GtSOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcgt | SimdOp::I8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16GtUOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcgt | SimdOp::I8 | SimdOp::unsignedBit, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16GeSOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcge | SimdOp::I8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16GeUOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcge | SimdOp::I8 | SimdOp::unsignedBit, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8ExtmulLowI8X16SOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8ExtmulHighI8X16SOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I8, dst, getHighRegister(args[0].arg), getHighRegister(args[1].arg));
        break;
    case ByteCode::I16X8ExtmulLowI8X16UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I8 | SimdOp::unsignedBit, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8ExtmulHighI8X16UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I8 | SimdOp::unsignedBit, dst, getHighRegister(args[0].arg), getHighRegister(args[1].arg));
        break;
    case ByteCode::I16X8AddOpcode:
        simdEmitOp(compiler, SimdOp::Type::vadd | SimdOp::I16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8SubOpcode:
        simdEmitOp(compiler, SimdOp::Type::vsub | SimdOp::I16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8MulOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmul | SimdOp::I16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8AddSatSOpcode:
        simdEmitOp(compiler, SimdOp::Type::vqadd | SimdOp::I16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8AddSatUOpcode:
        simdEmitOp(compiler, SimdOp::Type::vqadd | SimdOp::I16 | SimdOp::unsignedBit, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8SubSatSOpcode:
        simdEmitOp(compiler, SimdOp::Type::vqsub | SimdOp::I16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8SubSatUOpcode:
        simdEmitOp(compiler, SimdOp::Type::vqsub | SimdOp::I16 | SimdOp::unsignedBit, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8EqOpcode:
        simdEmitOp(compiler, SimdOp::Type::vceq | SimdOp::I16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8NeOpcode:
        simdEmitOp(compiler, SimdOp::Type::vceq | SimdOp::I16, dst, args[0].arg, args[1].arg);
        simdEmitOp(compiler, SimdOp::Type::vmvn | SimdOp::I16, dst, 0, dst);
        break;
    case ByteCode::I16X8LtSOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcgt | SimdOp::I16, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I16X8LtUOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcgt | SimdOp::I16 | SimdOp::unsignedBit, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I16X8LeSOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcge | SimdOp::I16, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I16X8LeUOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcge | SimdOp::I16 | SimdOp::unsignedBit, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I16X8GtSOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcgt | SimdOp::I16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8GtUOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcgt | SimdOp::I16 | SimdOp::unsignedBit, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8GeSOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcge | SimdOp::I16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8GeUOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcge | SimdOp::I16 | SimdOp::unsignedBit, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4ExtmulLowI16X8SOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4ExtmulHighI16X8SOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I16, dst, getHighRegister(args[0].arg), getHighRegister(args[1].arg));
        break;
    case ByteCode::I32X4ExtmulLowI16X8UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I16 | SimdOp::unsignedBit, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4ExtmulHighI16X8UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I16 | SimdOp::unsignedBit, dst, getHighRegister(args[0].arg), getHighRegister(args[1].arg));
        break;
    case ByteCode::I32X4AddOpcode:
        simdEmitOp(compiler, SimdOp::Type::vadd | SimdOp::I32, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4SubOpcode:
        simdEmitOp(compiler, SimdOp::Type::vsub | SimdOp::I32, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4MulOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmul | SimdOp::I32, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4EqOpcode:
        simdEmitOp(compiler, SimdOp::Type::vceq | SimdOp::I32, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4NeOpcode:
        simdEmitOp(compiler, SimdOp::Type::vceq | SimdOp::I32, dst, args[0].arg, args[1].arg);
        simdEmitOp(compiler, SimdOp::Type::vmvn | SimdOp::I32, dst, 0, dst);
        break;
    case ByteCode::I32X4LtSOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcgt | SimdOp::I32, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I32X4LtUOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcgt | SimdOp::I32 | SimdOp::unsignedBit, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I32X4LeSOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcge | SimdOp::I32, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I32X4LeUOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcge | SimdOp::I32 | SimdOp::unsignedBit, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I32X4GtSOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcgt | SimdOp::I32, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4GtUOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcgt | SimdOp::I32 | SimdOp::unsignedBit, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4GeSOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcge | SimdOp::I32, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4GeUOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcge | SimdOp::I32 | SimdOp::unsignedBit, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2ExtmulLowI32X4SOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I32, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2ExtmulHighI32X4SOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I32, dst, getHighRegister(args[0].arg), getHighRegister(args[1].arg));
        break;
    case ByteCode::I64X2ExtmulLowI32X4UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I32 | SimdOp::unsignedBit, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2ExtmulHighI32X4UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I32 | SimdOp::unsignedBit, dst, getHighRegister(args[0].arg), getHighRegister(args[1].arg));
        break;
    case ByteCode::I64X2AddOpcode:
        simdEmitOp(compiler, SimdOp::Type::vadd | SimdOp::I64, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2SubOpcode:
        simdEmitOp(compiler, SimdOp::Type::vsub | SimdOp::I64, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2MulOpcode:
        simdEmitI64x2Mul(compiler, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2EqOpcode:
        simdEmitI64x2Eq(compiler, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2NeOpcode:
        simdEmitI64x2Ne(compiler, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2LtSOpcode:
        simdEmitI64x2GtS(compiler, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I64X2LeSOpcode:
        simdEmitI64x2GeS(compiler, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I64X2GtSOpcode:
        simdEmitI64x2GtS(compiler, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2GeSOpcode:
        simdEmitI64x2GeS(compiler, dst, args[0].arg, args[1].arg);
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    if (SLJIT_IS_MEM(args[2].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | type, dst, args[2].arg, args[2].argw);
    }
}

static void emitSelectSIMD(sljit_compiler* compiler, Instruction* instr)
{
    // TODO
    ASSERT_NOT_REACHED();
}

static void emitShiftSIMD(sljit_compiler* compiler, Instruction* instr)
{
    // TODO
    ASSERT_NOT_REACHED();
}

static void emitShuffleSIMD(sljit_compiler* compiler, Instruction* instr)
{
    // TODO
    ASSERT_NOT_REACHED();
}
