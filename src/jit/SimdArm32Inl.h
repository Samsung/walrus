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
constexpr uint32_t unsignedBit = 0b1 << 28;

enum Type : uint32_t {
    vadd = 0xef000840,
    vand = 0xef000150,
    vbic = 0xef100150,
    vbsl = 0xff100150,
    vceq = 0xff000850,
    vceqImm = 0xffb10140,
    vcge = 0xef000350,
    vcgt = 0xef000340,
    vcvtftf = 0xeeb70ac0,
    vcvtftiSIMD = 0xffb30640,
    vcvtfti = 0xeebc08c0,
    vcvtitfSIMD = 0xffb30640,
    vcvtitf = 0xeeb80840,
    veor = 0xff000150,
    vmlal = 0xef800800,
    vmovl = 0xef800a10,
    vmul = 0xef000950,
    vmull = 0xef800c00,
    vmvn = 0xffb005c0,
    vneg = 0xffb103c0,
    vorn = 0xef300150,
    vorr = 0xef200150,
    vpadd = 0xef000b10,
    vpaddl = 0xffb00240,
    vpmax = 0xef000a00,
    vpmin = 0xef000a10,
    vrev64 = 0xffB00040,
    vqadd = 0xef000050,
    vqrdmulh = 0xff000b40,
    vqmovn = 0xffb20200,
    vqsub = 0xef000250,
    vshlImm = 0xef800550,
    vshl = 0xef000440,
    vshrImm = 0xef800050,
    vsub = 0xff000840,
    vtbl = 0xffb00800,
    vtrn = 0xffB20080,
};
#else
constexpr uint32_t unsignedBit = 0b1 << 24;

enum Type : uint32_t {
    vadd = 0xf2000840,
    vand = 0xf2000150,
    vbic = 0xf2100150,
    vbsl = 0xf3100150,
    vceq = 0xf3000850,
    vceqImm = 0xf3b10140,
    vcge = 0xf2000350,
    vcgt = 0xf2000340,
    vcvtftf = 0xeb70ac0,
    vcvtftiSIMD = 0xf3b30640,
    vcvtfti = 0xebc08c0,
    vcvtitfSIMD = 0xf3b30640,
    vcvtitf = 0xeb80840,
    veor = 0xf3000150,
    vmlal = 0xf2800800,
    vmovl = 0xf2800a10,
    vmul = 0xf2000950,
    vmull = 0xf2800C00,
    vmvn = 0xf3b005c0,
    vneg = 0xf3b103c0,
    vorn = 0xf2300150,
    vorr = 0xf2200150,
    vpadd = 0xf2000b10,
    vpaddl = 0xf3b00240,
    vpmax = 0xf2000A00,
    vpmin = 0xf2000A10,
    vrev64 = 0xf3b00040,
    vqadd = 0xf2000050,
    vqrdmulh = 0xf3000b40,
    vqmovn = 0xf3b20200,
    vqsub = 0xf2000250,
    vshlImm = 0xf2800550,
    vshl = 0xf2000440,
    vshrImm = 0xf2800050,
    vsub = 0xf3000840,
    vtbl = 0xf3b00800,
    vtrn = 0xf3b20080,
};
#endif

constexpr uint8_t sizeOffset = 20;
constexpr uint8_t singleSizeOffset = 18;
constexpr uint32_t lBit = 0b1 << 7;

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
    SHL8 = 0b1 << 19,
    SHL16 = 0b1 << 20,
    SHL32 = 0b1 << 21,
};

enum ShiftMask : int32_t {
    shift8 = (1 << 3) - 1,
    shift16 = (1 << 4) - 1,
    shift32 = (1 << 5) - 1,
    shift64 = (1 << 6) - 1,
};

enum ExtendType : uint32_t {
    immS8 = 0b1 << 19,
    immS16 = 0b1 << 20,
    immS32 = 0b1 << 21,
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
    simdEmitOp(compiler, SimdOp::vshlImm | SimdOp::SHL32 | SimdOp::lBit, vd, 0, vd);
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

static void simdEmitV128AnyTrue(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rm)
{
    auto tmpReg = SLJIT_FR2;

    simdEmitOp(compiler, SimdOp::vpmax | SimdOp::I32 | SimdOp::unsignedBit, tmpReg, rm, getHighRegister(rm));
    simdEmitOp(compiler, SimdOp::vpmax | SimdOp::I32 | SimdOp::unsignedBit, tmpReg, tmpReg, tmpReg);
    sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32 | SLJIT_32, tmpReg, 0, rd, 0);
    sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_Z, rd, 0, SLJIT_IMM, 0);
    sljit_emit_op_flags(compiler, SLJIT_MOV, rd, 0, SLJIT_NOT_EQUAL);
}

static void simdEmitAllTrueEnd(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 tmpReg, sljit_s32 type)
{
    sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_LANE_SIGNED | SLJIT_32 | type, tmpReg, 0, rd, 0);
    sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_Z, rd, 0, SLJIT_IMM, 0);
    sljit_emit_op_flags(compiler, SLJIT_MOV, rd, 0, SLJIT_NOT_EQUAL);
}

static void simdEmitI8x16AllTrue(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rm)
{
    auto tmpReg = SLJIT_FR2;

    simdEmitOp(compiler, SimdOp::vpmin | SimdOp::I8 | SimdOp::unsignedBit, tmpReg, rm, getHighRegister(rm));

    for (int i = 0; i < 3; i++) {
        simdEmitOp(compiler, SimdOp::vpmin | SimdOp::I8 | SimdOp::unsignedBit, tmpReg, tmpReg, tmpReg);
    }

    simdEmitAllTrueEnd(compiler, rd, tmpReg, SLJIT_SIMD_ELEM_8);
}

static void simdEmitI16x8AllTrue(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rm)
{
    auto tmpReg = SLJIT_FR2;

    simdEmitOp(compiler, SimdOp::vpmin | SimdOp::I16 | SimdOp::unsignedBit, tmpReg, rm, getHighRegister(rm));

    for (int i = 0; i < 2; i++) {
        simdEmitOp(compiler, SimdOp::vpmin | SimdOp::I16 | SimdOp::unsignedBit, tmpReg, tmpReg, tmpReg);
    }

    simdEmitAllTrueEnd(compiler, rd, tmpReg, SLJIT_SIMD_ELEM_16);
}

static void simdEmitI32x4AllTrue(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rm)
{
    auto tmpReg = SLJIT_FR2;

    simdEmitOp(compiler, SimdOp::vpmin | SimdOp::I32 | SimdOp::unsignedBit, tmpReg, rm, getHighRegister(rm));
    simdEmitOp(compiler, SimdOp::vpmin | SimdOp::I32 | SimdOp::unsignedBit, tmpReg, tmpReg, tmpReg);
    simdEmitAllTrueEnd(compiler, rd, tmpReg, SLJIT_SIMD_ELEM_32);
}

static void simdEmitI64x2AllTrue(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rm)
{
    auto tmpReg = SLJIT_FR2;

    simdEmitOp(compiler, SimdOp::vpmax | SimdOp::I32 | SimdOp::unsignedBit, tmpReg, rm, getHighRegister(rm));
    simdEmitOp(compiler, SimdOp::vceqImm | SimdOp::S32, tmpReg, 0, tmpReg);
    simdEmitOp(compiler, SimdOp::vpmax | SimdOp::I32 | SimdOp::unsignedBit, tmpReg, tmpReg, tmpReg);
    sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_LANE_SIGNED | SLJIT_32 | SLJIT_SIMD_ELEM_32, tmpReg, 0, rd, 0);
    sljit_emit_op2(compiler, SLJIT_ADD, rd, 0, rd, 0, SLJIT_IMM, 1);
}

static void simdEmitF64x2PromoteLow(sljit_compiler* compiler, sljit_s32 vd, sljit_s32 vm, uint32_t operation)
{
    auto tmpReg = SLJIT_FR2;

    sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32 | SLJIT_SIMD_FLOAT, tmpReg, vm, 0);

    simdEmitOp(compiler, operation, vd, 0, tmpReg);
    simdEmitOp(compiler, operation | (0x1 << 5), getHighRegister(vd), 0, tmpReg);
}

static void simdEmitTruncSatF64(sljit_compiler* compiler, sljit_s32 vd, sljit_s32 vm, bool isSigned = false)
{
    simdEmitOp(compiler, SimdOp::Type::vcvtfti | (0x3 << 8) | (isSigned << 16), vd, 0, vm);
    simdEmitOp(compiler, SimdOp::Type::vcvtfti | (0x3 << 8) | (0b1 << 22) | (isSigned << 16), vd, 0, getHighRegister(vm));
    sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_REG_64 | SLJIT_SIMD_ELEM_32, vd + 1, 0, SLJIT_IMM, 0);
    sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_REG_64 | SLJIT_SIMD_ELEM_32, vd + 1, 1, SLJIT_IMM, 0);
}

static void simdEmitI32x4Dot(sljit_compiler* compiler, sljit_s32 vd, sljit_s32 vn, sljit_s32 vm)
{
    auto tmpReg = SLJIT_FR4;

    simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I16, tmpReg, vn, vm);
    simdEmitOp(compiler, SimdOp::Type::vpadd | SimdOp::I32, vd, tmpReg, getHighRegister(tmpReg));
    simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I16, tmpReg, getHighRegister(vn), getHighRegister(vm));
    simdEmitOp(compiler, SimdOp::Type::vpadd | SimdOp::I32, getHighRegister(vd), tmpReg, getHighRegister(tmpReg));
}

static void emitUnarySIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[2];

    sljit_s32 type = SLJIT_SIMD_ELEM_128;
    bool isDSTNormalRegister = false;

    switch (instr->opcode()) {
    case ByteCode::F64X2PromoteLowF32X4Opcode:
    case ByteCode::I32X4TruncSatF32X4SOpcode:
    case ByteCode::I32X4TruncSatF32X4UOpcode:
    case ByteCode::I32X4TruncSatF64X2SZeroOpcode:
    case ByteCode::I32X4TruncSatF64X2UZeroOpcode:
        type = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::F32X4DemoteF64X2ZeroOpcode:
        type = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::I8X16AllTrueOpcode:
        isDSTNormalRegister = true;
        FALLTHROUGH;
    case ByteCode::I8X16NegOpcode:
    case ByteCode::I16X8ExtendLowI8X16SOpcode:
    case ByteCode::I16X8ExtendHighI8X16SOpcode:
    case ByteCode::I16X8ExtendLowI8X16UOpcode:
    case ByteCode::I16X8ExtendHighI8X16UOpcode:
        type = SLJIT_SIMD_ELEM_8;
        break;
    case ByteCode::I16X8AllTrueOpcode:
        isDSTNormalRegister = true;
        FALLTHROUGH;
    case ByteCode::I16X8NegOpcode:
    case ByteCode::I16X8ExtaddPairwiseI8X16SOpcode:
    case ByteCode::I16X8ExtaddPairwiseI8X16UOpcode:
    case ByteCode::I32X4ExtendLowI16X8SOpcode:
    case ByteCode::I32X4ExtendHighI16X8SOpcode:
    case ByteCode::I32X4ExtendLowI16X8UOpcode:
    case ByteCode::I32X4ExtendHighI16X8UOpcode:
        type = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I32X4AllTrueOpcode:
        isDSTNormalRegister = true;
        FALLTHROUGH;
    case ByteCode::I32X4ExtaddPairwiseI16X8SOpcode:
    case ByteCode::I32X4ExtaddPairwiseI16X8UOpcode:
    case ByteCode::I32X4NegOpcode:
    case ByteCode::F32X4ConvertI32X4SOpcode:
    case ByteCode::F32X4ConvertI32X4UOpcode:
    case ByteCode::F64X2ConvertLowI32X4SOpcode:
    case ByteCode::F64X2ConvertLowI32X4UOpcode:
        type = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I64X2AllTrueOpcode:
        isDSTNormalRegister = true;
        FALLTHROUGH;
    case ByteCode::I64X2NegOpcode:
        type = SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::V128AnyTrueOpcode:
        isDSTNormalRegister = true;
        FALLTHROUGH;
    case ByteCode::V128NotOpcode:
        type = SLJIT_SIMD_ELEM_128;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    simdOperandToArg(compiler, operands, args[0], type, SLJIT_FR0);

    args[1].set(operands + 1);
    sljit_s32 dst = GET_TARGET_REG(args[1].arg, isDSTNormalRegister ? SLJIT_R0 : SLJIT_FR0);

    switch (instr->opcode()) {
    case ByteCode::I8X16AllTrueOpcode:
        simdEmitI8x16AllTrue(compiler, dst, args[0].arg);
        break;
    case ByteCode::F64X2PromoteLowF32X4Opcode:
        simdEmitF64x2PromoteLow(compiler, dst, args[0].arg, SimdOp::Type::vcvtftf);
        break;
    case ByteCode::I32X4TruncSatF32X4SOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcvtftiSIMD | SimdOp::S32 | (0b1 << 8), dst, 0, args[0].arg);
        break;
    case ByteCode::I32X4TruncSatF32X4UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcvtftiSIMD | SimdOp::S32 | (0b11 << 7), dst, 0, args[0].arg);
        break;
    case ByteCode::I32X4TruncSatF64X2SZeroOpcode:
        simdEmitTruncSatF64(compiler, dst, args[0].arg, true);
        break;
    case ByteCode::I32X4TruncSatF64X2UZeroOpcode:
        simdEmitTruncSatF64(compiler, dst, args[0].arg);
        break;
    case ByteCode::F32X4DemoteF64X2ZeroOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcvtftf | (0x1 << 8), dst, 0, args[0].arg);
        simdEmitOp(compiler, SimdOp::Type::vcvtftf | (0x1 << 8) | (0x1 << 22), dst, 0, getHighRegister(args[0].arg));
        sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_REG_64 | SLJIT_SIMD_ELEM_32, dst + 1, 0, SLJIT_IMM, 0);
        sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_REG_64 | SLJIT_SIMD_ELEM_32, dst + 1, 1, SLJIT_IMM, 0);
        break;
    case ByteCode::I8X16NegOpcode:
        simdEmitOp(compiler, SimdOp::Type::vneg | SimdOp::S8, dst, 0, args[0].arg);
        break;
    case ByteCode::I16X8AllTrueOpcode:
        simdEmitI16x8AllTrue(compiler, dst, args[0].arg);
        break;
    case ByteCode::I16X8ExtendLowI8X16SOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmovl | SimdOp::immS8, dst, 0, args[0].arg);
        break;
    case ByteCode::I16X8ExtendHighI8X16SOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmovl | SimdOp::immS8, dst, 0, getHighRegister(args[0].arg));
        break;
    case ByteCode::I16X8ExtendLowI8X16UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmovl | SimdOp::immS8 | SimdOp::unsignedBit, dst, 0, args[0].arg);
        break;
    case ByteCode::I16X8ExtendHighI8X16UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmovl | SimdOp::immS8 | SimdOp::unsignedBit, dst, 0, getHighRegister(args[0].arg));
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
    case ByteCode::I32X4AllTrueOpcode:
        simdEmitI32x4AllTrue(compiler, dst, args[0].arg);
        break;
    case ByteCode::I32X4ExtendLowI16X8SOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmovl | SimdOp::immS16, dst, 0, args[0].arg);
        break;
    case ByteCode::I32X4ExtendHighI16X8SOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmovl | SimdOp::immS16, dst, 0, getHighRegister(args[0].arg));
        break;
    case ByteCode::I32X4ExtendLowI16X8UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmovl | SimdOp::immS16 | SimdOp::unsignedBit, dst, 0, args[0].arg);
        break;
    case ByteCode::I32X4ExtendHighI16X8UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmovl | SimdOp::immS16 | SimdOp::unsignedBit, dst, 0, getHighRegister(args[0].arg));
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
    case ByteCode::I64X2AllTrueOpcode:
        simdEmitI64x2AllTrue(compiler, dst, args[0].arg);
        break;
    case ByteCode::F32X4ConvertI32X4SOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcvtitfSIMD | SimdOp::S32, dst, 0, args[0].arg);
        break;
    case ByteCode::F32X4ConvertI32X4UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vcvtitfSIMD | SimdOp::S32 | (0x1 << 7), dst, 0, args[0].arg);
        break;
    case ByteCode::F64X2ConvertLowI32X4SOpcode:
        simdEmitF64x2PromoteLow(compiler, dst, args[0].arg, SimdOp::Type::vcvtitf | (0x1 << 7) | (0b11 << 8));
        break;
    case ByteCode::F64X2ConvertLowI32X4UOpcode:
        simdEmitF64x2PromoteLow(compiler, dst, args[0].arg, SimdOp::Type::vcvtitf | (0b11 << 8));
        break;
    case ByteCode::I64X2NegOpcode:
        simdEmitI64x2Neg(compiler, dst, args[0].arg);
        break;
    case ByteCode::V128AnyTrueOpcode:
        simdEmitV128AnyTrue(compiler, dst, args[0].arg);
        break;
    case ByteCode::V128NotOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmvn, dst, 0, args[0].arg);
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    if (SLJIT_IS_MEM(args[1].arg)) {
        if (!isDSTNormalRegister) {
            sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | type, dst, args[1].arg, args[1].argw);
        } else {
            sljit_emit_op1(compiler, SLJIT_MOV, args[1].arg, args[1].argw, dst, 0);
        }
    }
}

static void emitBinarySIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[3];

    sljit_s32 type = SLJIT_SIMD_ELEM_128;
    bool isSwizzle = false;

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
    case ByteCode::I8X16NarrowI16X8SOpcode:
    case ByteCode::I8X16NarrowI16X8UOpcode:
    case ByteCode::I32X4DotI16X8SOpcode:
    case ByteCode::I16X8Q15mulrSatSOpcode:
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
    case ByteCode::I16X8NarrowI32X4SOpcode:
    case ByteCode::I16X8NarrowI32X4UOpcode:
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
    case ByteCode::I8X16SwizzleOpcode:
        isSwizzle = true;
        FALLTHROUGH;
    case ByteCode::V128AndOpcode:
    case ByteCode::V128OrOpcode:
    case ByteCode::V128XorOpcode:
    case ByteCode::V128AndnotOpcode:
        type = SLJIT_SIMD_ELEM_128;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    simdOperandToArg(compiler, operands, args[0], type, SLJIT_FR0);
    simdOperandToArg(compiler, operands + 1, args[1], type, SLJIT_FR2);

    args[2].set(operands + 2);
    sljit_s32 dst = GET_TARGET_REG(args[2].arg, isSwizzle ? SLJIT_FR4 : SLJIT_FR0);

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
    case ByteCode::I8X16NarrowI16X8SOpcode:
        simdEmitOp(compiler, SimdOp::Type::vqmovn | SimdOp::S8 | (0x1 << 7), dst, 0, args[0].arg);
        simdEmitOp(compiler, SimdOp::Type::vqmovn | SimdOp::S8 | (0x1 << 7), getHighRegister(dst), 0, args[1].arg);
        break;
    case ByteCode::I8X16NarrowI16X8UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vqmovn | SimdOp::S8 | (0x1 << 6), dst, 0, args[0].arg);
        simdEmitOp(compiler, SimdOp::Type::vqmovn | SimdOp::S8 | (0x1 << 6), getHighRegister(dst), 0, args[1].arg);
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
    case ByteCode::I32X4DotI16X8SOpcode:
        simdEmitI32x4Dot(compiler, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8Q15mulrSatSOpcode:
        simdEmitOp(compiler, SimdOp::Type::vqrdmulh | SimdOp::I16, dst, args[0].arg, args[1].arg);
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
    case ByteCode::I16X8NarrowI32X4SOpcode:
        simdEmitOp(compiler, SimdOp::Type::vqmovn | SimdOp::S16 | (0x1 << 7), dst, 0, args[0].arg);
        simdEmitOp(compiler, SimdOp::Type::vqmovn | SimdOp::S16 | (0x1 << 7), getHighRegister(dst), 0, args[1].arg);
        break;
    case ByteCode::I16X8NarrowI32X4UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vqmovn | SimdOp::S16 | (0x1 << 6), dst, 0, args[0].arg);
        simdEmitOp(compiler, SimdOp::Type::vqmovn | SimdOp::S16 | (0x1 << 6), getHighRegister(dst), 0, args[1].arg);
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
    case ByteCode::V128AndOpcode:
        simdEmitOp(compiler, SimdOp::Type::vand, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::V128OrOpcode:
        simdEmitOp(compiler, SimdOp::Type::vorr, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::V128XorOpcode:
        simdEmitOp(compiler, SimdOp::Type::veor, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::V128AndnotOpcode:
        simdEmitOp(compiler, SimdOp::Type::vbic, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16SwizzleOpcode:
        simdEmitOp(compiler, SimdOp::Type::vtbl | (0b1 << 8), dst, args[0].arg, args[1].arg);
        simdEmitOp(compiler, SimdOp::Type::vtbl | (0b1 << 8), getHighRegister(dst), args[0].arg, getHighRegister(args[1].arg));
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
    Operand* operands = instr->operands();
    JITArg args[3];

    simdOperandToArg(compiler, operands, args[0], SLJIT_SIMD_ELEM_128, SLJIT_FR4);
    simdOperandToArg(compiler, operands + 1, args[1], SLJIT_SIMD_ELEM_128, SLJIT_FR2);
    simdOperandToArg(compiler, operands + 2, args[2], SLJIT_SIMD_ELEM_128, SLJIT_FR0);

    simdEmitOp(compiler, SimdOp::Type::vbsl, args[2].arg, args[0].arg, args[1].arg);

    args[1].set(operands + 3);
    sljit_s32 dst = GET_TARGET_REG(args[1].arg, SLJIT_FR0);

    if (SLJIT_IS_MEM(args[1].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, dst, args[1].arg, args[1].argw);
    }
}

static void emitShiftSIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[3];

    sljit_s32 type = SLJIT_SIMD_ELEM_128;

    uint32_t op = 0;
    uint32_t immOp = 0;
    int mask = 0;
    bool isShr = true;

    switch (instr->opcode()) {
    case ByteCode::I8X16ShlOpcode:
        isShr = false;
        immOp |= SimdOp::Type::vshlImm | SimdOp::SHL8;
        op |= SimdOp::Type::vshl | SimdOp::I8;
        mask = SimdOp::shift8;
        type = SLJIT_SIMD_ELEM_8;
        break;
    case ByteCode::I8X16ShrUOpcode:
        immOp |= (0b1 << 24);
        op |= SimdOp::unsignedBit;
        FALLTHROUGH;
    case ByteCode::I8X16ShrSOpcode:
        mask = SimdOp::shift8;
        type = SLJIT_SIMD_ELEM_8;
        immOp |= SimdOp::Type::vshrImm | SimdOp::SHL8;
        op |= SimdOp::Type::vshl | SimdOp::I8;
        break;
    case ByteCode::I16X8ShlOpcode:
        isShr = false;
        immOp |= SimdOp::Type::vshlImm | SimdOp::SHL16;
        op |= SimdOp::Type::vshl | SimdOp::I16;
        mask = SimdOp::shift16;
        type = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I16X8ShrUOpcode:
        immOp |= (0b1 << 24);
        op |= SimdOp::unsignedBit;
        FALLTHROUGH;
    case ByteCode::I16X8ShrSOpcode:
        mask = SimdOp::shift16;
        type = SLJIT_SIMD_ELEM_16;
        immOp |= SimdOp::Type::vshrImm | SimdOp::SHL16;
        op |= SimdOp::Type::vshl | SimdOp::I16;
        break;
    case ByteCode::I32X4ShlOpcode:
        isShr = false;
        immOp |= SimdOp::Type::vshlImm | SimdOp::SHL32;
        op |= SimdOp::Type::vshl | SimdOp::I32;
        mask = SimdOp::shift32;
        type = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I32X4ShrUOpcode:
        immOp |= (0b1 << 24);
        op |= SimdOp::unsignedBit;
        FALLTHROUGH;
    case ByteCode::I32X4ShrSOpcode:
        mask = SimdOp::shift32;
        type = SLJIT_SIMD_ELEM_32;
        immOp |= SimdOp::Type::vshrImm | SimdOp::SHL32;
        op |= SimdOp::Type::vshl | SimdOp::I32;
        break;
    case ByteCode::I64X2ShlOpcode:
        isShr = false;
        immOp |= SimdOp::Type::vshlImm | SimdOp::lBit;
        op |= SimdOp::Type::vshl | SimdOp::I64;
        mask = SimdOp::shift64;
        type = SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::I64X2ShrUOpcode:
        immOp |= (0b1 << 24);
        op |= SimdOp::unsignedBit;
        FALLTHROUGH;
    case ByteCode::I64X2ShrSOpcode:
        mask = SimdOp::shift64;
        type = SLJIT_SIMD_ELEM_64;
        immOp |= SimdOp::Type::vshrImm | SimdOp::lBit;
        op |= SimdOp::Type::vshl | SimdOp::I64;
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    simdOperandToArg(compiler, operands, args[0], type, SLJIT_FR0);
    args[1].set(operands + 1);

    args[2].set(operands + 2);
    sljit_s32 dst = GET_TARGET_REG(args[2].arg, SLJIT_FR2);

    if (SLJIT_IS_IMM(args[1].arg)) {
        if (isShr) {
            args[1].argw = (args[1].argw ^ mask) + 1;
        }

        args[1].argw &= mask;
        if (args[1].argw == 0) {
            if (args[2].arg != args[0].arg) {
                sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | type, args[0].arg, args[2].arg, args[2].argw);
            }
            return;
        }

        simdEmitOp(compiler, immOp | (args[1].argw << 16), dst, 0, args[0].arg);
    } else {
        sljit_s32 srcReg = GET_SOURCE_REG(args[1].arg, SLJIT_R0);
        sljit_emit_op2(compiler, SLJIT_AND32, srcReg, 0, args[1].arg, args[1].argw, SLJIT_IMM, mask);

        if (isShr) {
            sljit_emit_op2(compiler, SLJIT_SUB32, srcReg, 0, SLJIT_IMM, 0, srcReg, 0);
        }

        sljit_emit_simd_replicate(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8, dst, srcReg, 0);
        simdEmitOp(compiler, op, dst, dst, args[0].arg);
    }

    if (SLJIT_IS_MEM(args[2].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | type, dst, args[2].arg, args[2].argw);
    }
}

static void emitShuffleSIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[3];

    simdOperandToArg(compiler, operands, args[0], SLJIT_SIMD_ELEM_128, SLJIT_FR2);
    simdOperandToArg(compiler, operands + 1, args[1], SLJIT_SIMD_ELEM_128, SLJIT_FR4);

    args[2].set(operands + 2);
    sljit_s32 dst = GET_TARGET_REG(args[2].arg, SLJIT_FR0);

    I8X16Shuffle* shuffle = reinterpret_cast<I8X16Shuffle*>(instr->byteCode());
    const sljit_s32 type = SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8;
    sljit_emit_simd_mov(compiler, type, dst, SLJIT_MEM0(), reinterpret_cast<sljit_sw>(shuffle->value()));

    simdEmitOp(compiler, SimdOp::vtbl | (0b11 << 8), dst, args[0].arg, dst);
    simdEmitOp(compiler, SimdOp::vtbl | (0b11 << 8), getHighRegister(dst), args[0].arg, getHighRegister(dst));

    if (SLJIT_IS_MEM(args[2].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, dst, args[2].arg, args[2].argw);
    }
}
