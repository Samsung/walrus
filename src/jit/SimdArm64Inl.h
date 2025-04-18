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

/* Only included by jit-backend.cc */

namespace SimdOp {

constexpr uint8_t sizeOffset = 22;

enum Type : uint32_t {
    abs = 0x4e20b800,
    add = 0x4e208400,
    addp = 0x4e20bc00,
    andOp = 0x4e201c00,
    bic = 0x4e601c00,
    bsl = 0x6e601c00,
    cmeq = 0x6e208c00,
    cmeqz = 0x4e209800,
    cmge = 0x4e203c00,
    cmgt = 0x4e203400,
    cnt = 0x4e205800,
    eor = 0x6e201c00,
    fabs = 0x4ea0f800,
    fadd = 0x4e20d400,
    fcmeq = 0x4e20e400,
    fcmge = 0x6e20e400,
    fcmgt = 0x6ea0e400,
    fcvtzs = 0x4ea1b800,
    fcvtzu = 0x6ea1b800,
    fdiv = 0x6e20fc00,
    fmax = 0x4e20f400,
    fmin = 0x4ea0f400,
    fmla = 0x4e20cc00,
    fmls = 0x4ea0cc00,
    fmul = 0x6e20dc00,
    fneg = 0x6ea0f800,
    frintm = 0x4e219800, // floor
    frintn = 0x4e218800, // nearest
    frintp = 0x4ea18800, // ceil
    frintz = 0x4ea19800, // trunc
    fsub = 0x4ea0d400,
    fsqrt = 0x6ea1f800,
    fcvtn = 0xe216800,
    fcvtl = 0x4e217800,
    neg = 0x6e20b800,
    notOp = 0x6e205800,
    mul = 0x4e209c00,
    orr = 0x4ea01c00,
    rev64 = 0x4e200800,
    saddlp = 0x4e202800,
    scvtf = 0x4e21d800,
    sdot = 0x4e009400,
    shl = 0x4f005400,
    shll = 0x2e213800,
    smax = 0x4e206400,
    smin = 0x4e206c00,
    smull = 0xe20c000,
    sqadd = 0x4e200c00,
    sqrdmulh = 0x6e20b400,
    sqsub = 0x4e202c00,
    sqxtn = 0x0e214800,
    sqxtun = 0x2e212800,
    sshl = 0x4e204400,
    sshr = 0x4f000400,
    sub = 0x6e208400,
    sxtl = 0x0f00a400,
    tbl = 0x4e000000,
    uaddlp = 0x6e202800,
    ucmge = 0x6e203c00,
    ucmgt = 0x6e203400,
    ucvtf = 0x6e21d800,
    umax = 0x6e206400,
    umaxp = 0x6e20a400,
    umin = 0x6e206c00,
    uminv = 0x6e31a800,
    umlal = 0x2e208000,
    umull = 0x2e20c000,
    uqadd = 0x6e200c00,
    uqsub = 0x6e202c00,
    uqxtn = 0x2e214800,
    urhadd = 0x6e201400,
    ushl = 0x6e204400,
    ushr = 0x6f000400,
    uxtl = 0x2f00a400,
    uzp1 = 0x4e001800,
    xtn = 0xe212800,
    zip1 = 0x4e003800,
    zip2 = 0x4e007800,
};

enum IntSizeType : uint32_t {
    B16 = 0x0 << sizeOffset,
    H8 = 0x1 << sizeOffset,
    S4 = 0x2 << sizeOffset,
    D2 = 0x3 << sizeOffset,
};

enum FloatSizeType : uint32_t {
    FS4 = 0x0 << sizeOffset,
    FD2 = 0x1 << sizeOffset,
};

enum ShiftMask : int32_t {
    shiftB = (1 << 3) - 1,
    shiftH = (1 << 4) - 1,
    shiftS = (1 << 5) - 1,
    shiftD = (1 << 6) - 1,
};

enum ImmSizeType : uint32_t {
    ImmB16 = 0x1 << 19,
    ImmH8 = 0x1 << 20,
    ImmS4 = 0x1 << 21,
    ImmD2 = 0x1 << 22,
};

}; // namespace SimdOp

static void simdEmitOp(sljit_compiler* compiler, uint32_t opcode, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm)
{
    rd = sljit_get_register_index(SLJIT_SIMD_REG_128, rd);
    rn = sljit_get_register_index(SLJIT_SIMD_REG_128, rn);

    if (rm >= SLJIT_FR0) {
        rm = sljit_get_register_index(SLJIT_SIMD_REG_128, rm);
    }

    opcode |= (uint32_t)rd | ((uint32_t)rn << 5) | ((uint32_t)rm << 16);
    sljit_emit_op_custom(compiler, &opcode, sizeof(uint32_t));
}

static void emitUnarySIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[2];

    sljit_s32 srcType = SLJIT_SIMD_ELEM_128;
    sljit_s32 dstType = SLJIT_SIMD_ELEM_128;

    switch (instr->opcode()) {
    case ByteCode::I8X16NegOpcode:
    case ByteCode::I8X16AbsOpcode:
    case ByteCode::I8X16PopcntOpcode:
        srcType = SLJIT_SIMD_ELEM_8;
        dstType = SLJIT_SIMD_ELEM_8;
        break;
    case ByteCode::I16X8ExtendLowI8X16SOpcode:
    case ByteCode::I16X8ExtendHighI8X16SOpcode:
    case ByteCode::I16X8ExtendLowI8X16UOpcode:
    case ByteCode::I16X8ExtendHighI8X16UOpcode:
        srcType = SLJIT_SIMD_ELEM_8;
        dstType = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I16X8NegOpcode:
    case ByteCode::I16X8AbsOpcode:
    case ByteCode::I16X8ExtaddPairwiseI8X16SOpcode:
    case ByteCode::I16X8ExtaddPairwiseI8X16UOpcode:
        srcType = SLJIT_SIMD_ELEM_16;
        dstType = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I32X4NegOpcode:
    case ByteCode::I32X4AbsOpcode:
    case ByteCode::I32X4ExtaddPairwiseI16X8SOpcode:
    case ByteCode::I32X4ExtaddPairwiseI16X8UOpcode:
        srcType = SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I32X4ExtendLowI16X8SOpcode:
    case ByteCode::I32X4ExtendHighI16X8SOpcode:
    case ByteCode::I32X4ExtendLowI16X8UOpcode:
    case ByteCode::I32X4ExtendHighI16X8UOpcode:
        srcType = SLJIT_SIMD_ELEM_16;
        dstType = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I32X4TruncSatF32X4SOpcode:
    case ByteCode::I32X4TruncSatF32X4UOpcode:
    case ByteCode::I32X4RelaxedTruncF32X4SOpcode:
    case ByteCode::I32X4RelaxedTruncF32X4UOpcode:
        srcType = SLJIT_SIMD_ELEM_32 | SLJIT_SIMD_FLOAT;
        dstType = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I32X4TruncSatF64X2SZeroOpcode:
    case ByteCode::I32X4TruncSatF64X2UZeroOpcode:
    case ByteCode::I32X4RelaxedTruncF64X2SZeroOpcode:
    case ByteCode::I32X4RelaxedTruncF64X2UZeroOpcode:
        srcType = SLJIT_SIMD_ELEM_64 | SLJIT_SIMD_FLOAT;
        dstType = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I64X2NegOpcode:
    case ByteCode::I64X2AbsOpcode:
        srcType = SLJIT_SIMD_ELEM_64;
        dstType = SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::I64X2ExtendLowI32X4SOpcode:
    case ByteCode::I64X2ExtendHighI32X4SOpcode:
    case ByteCode::I64X2ExtendLowI32X4UOpcode:
    case ByteCode::I64X2ExtendHighI32X4UOpcode:
        srcType = SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::F32X4AbsOpcode:
    case ByteCode::F32X4NegOpcode:
    case ByteCode::F32X4SqrtOpcode:
    case ByteCode::F32X4CeilOpcode:
    case ByteCode::F32X4FloorOpcode:
    case ByteCode::F32X4TruncOpcode:
    case ByteCode::F32X4NearestOpcode:
        srcType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::F32X4ConvertI32X4SOpcode:
    case ByteCode::F32X4ConvertI32X4UOpcode:
        srcType = SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::F32X4DemoteF64X2ZeroOpcode:
        srcType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::F64X2AbsOpcode:
    case ByteCode::F64X2NegOpcode:
    case ByteCode::F64X2SqrtOpcode:
    case ByteCode::F64X2CeilOpcode:
    case ByteCode::F64X2FloorOpcode:
    case ByteCode::F64X2TruncOpcode:
    case ByteCode::F64X2NearestOpcode:
        srcType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64;
        dstType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::F64X2ConvertLowI32X4SOpcode:
    case ByteCode::F64X2ConvertLowI32X4UOpcode:
        srcType = SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::F64X2PromoteLowF32X4Opcode:
        srcType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::V128NotOpcode:
        srcType = SLJIT_SIMD_ELEM_128;
        dstType = SLJIT_SIMD_ELEM_128;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    simdOperandToArg(compiler, operands, args[0], srcType, instr->requiredReg(0));

    args[1].set(operands + 1);
    sljit_s32 dst = GET_TARGET_REG(args[1].arg, instr->requiredReg(0));

    switch (instr->opcode()) {
    case ByteCode::F32X4DemoteF64X2ZeroOpcode:
        simdEmitOp(compiler, SimdOp::fcvtn | SimdOp::FD2, dst, args[0].arg, 0);
        break;
    case ByteCode::F32X4ConvertI32X4SOpcode:
        simdEmitOp(compiler, SimdOp::scvtf | SimdOp::FS4, dst, args[0].arg, 0);
        break;
    case ByteCode::F32X4ConvertI32X4UOpcode:
        simdEmitOp(compiler, SimdOp::ucvtf | SimdOp::FS4, dst, args[0].arg, 0);
        break;
    case ByteCode::F64X2ConvertLowI32X4SOpcode:
        simdEmitOp(compiler, SimdOp::sxtl | (0x1 << 21), dst, args[0].arg, 0);
        simdEmitOp(compiler, SimdOp::scvtf | SimdOp::FD2, dst, dst, 0);
        break;
    case ByteCode::F64X2ConvertLowI32X4UOpcode:
        simdEmitOp(compiler, SimdOp::uxtl | (0x1 << 21), dst, args[0].arg, 0);
        simdEmitOp(compiler, SimdOp::ucvtf | SimdOp::FD2, dst, dst, 0);
        break;
    case ByteCode::V128NotOpcode:
        simdEmitOp(compiler, SimdOp::notOp, dst, args[0].arg, 0);
        break;
    case ByteCode::I8X16NegOpcode:
        simdEmitOp(compiler, SimdOp::neg | SimdOp::B16, dst, args[0].arg, 0);
        break;
    case ByteCode::I8X16AbsOpcode:
        simdEmitOp(compiler, SimdOp::abs | SimdOp::B16, dst, args[0].arg, 0);
        break;
    case ByteCode::I8X16PopcntOpcode:
        simdEmitOp(compiler, SimdOp::cnt | SimdOp::B16, dst, args[0].arg, 0);
        break;
    case ByteCode::I16X8NegOpcode:
        simdEmitOp(compiler, SimdOp::neg | SimdOp::H8, dst, args[0].arg, 0);
        break;
    case ByteCode::I16X8AbsOpcode:
        simdEmitOp(compiler, SimdOp::abs | SimdOp::H8, dst, args[0].arg, 0);
        break;
    case ByteCode::I16X8ExtaddPairwiseI8X16SOpcode:
        simdEmitOp(compiler, SimdOp::saddlp | SimdOp::B16, dst, args[0].arg, 0);
        break;
    case ByteCode::I16X8ExtaddPairwiseI8X16UOpcode:
        simdEmitOp(compiler, SimdOp::uaddlp | SimdOp::B16, dst, args[0].arg, 0);
        break;
    case ByteCode::I16X8ExtendLowI8X16SOpcode:
        simdEmitOp(compiler, SimdOp::sxtl | (0x1 << 19), dst, args[0].arg, 0);
        break;
    case ByteCode::I16X8ExtendHighI8X16SOpcode:
        simdEmitOp(compiler, SimdOp::sxtl | (0x1 << 19) | (0x1 << 30), dst, args[0].arg, 0);
        break;
    case ByteCode::I16X8ExtendLowI8X16UOpcode:
        simdEmitOp(compiler, SimdOp::uxtl | (0x1 << 19), dst, args[0].arg, 0);
        break;
    case ByteCode::I16X8ExtendHighI8X16UOpcode:
        simdEmitOp(compiler, SimdOp::uxtl | (0x1 << 19) | (0x1 << 30), dst, args[0].arg, 0);
        break;
    case ByteCode::I32X4NegOpcode:
        simdEmitOp(compiler, SimdOp::neg | SimdOp::S4, dst, args[0].arg, 0);
        break;
    case ByteCode::I32X4AbsOpcode:
        simdEmitOp(compiler, SimdOp::abs | SimdOp::S4, dst, args[0].arg, 0);
        break;
    case ByteCode::I32X4ExtaddPairwiseI16X8SOpcode:
        simdEmitOp(compiler, SimdOp::saddlp | SimdOp::H8, dst, args[0].arg, 0);
        break;
    case ByteCode::I32X4ExtaddPairwiseI16X8UOpcode:
        simdEmitOp(compiler, SimdOp::uaddlp | SimdOp::H8, dst, args[0].arg, 0);
        break;
    case ByteCode::I32X4ExtendLowI16X8SOpcode:
        simdEmitOp(compiler, SimdOp::sxtl | (0x1 << 20), dst, args[0].arg, 0);
        break;
    case ByteCode::I32X4ExtendHighI16X8SOpcode:
        simdEmitOp(compiler, SimdOp::sxtl | (0x1 << 20) | (0x1 << 30), dst, args[0].arg, 0);
        break;
    case ByteCode::I32X4ExtendLowI16X8UOpcode:
        simdEmitOp(compiler, SimdOp::uxtl | (0x1 << 20), dst, args[0].arg, 0);
        break;
    case ByteCode::I32X4ExtendHighI16X8UOpcode:
        simdEmitOp(compiler, SimdOp::uxtl | (0x1 << 20) | (0x1 << 30), dst, args[0].arg, 0);
        break;
    case ByteCode::I32X4TruncSatF32X4SOpcode:
    case ByteCode::I32X4RelaxedTruncF32X4SOpcode:
        simdEmitOp(compiler, SimdOp::fcvtzs | SimdOp::FS4, dst, args[0].arg, 0);
        break;
    case ByteCode::I32X4TruncSatF32X4UOpcode:
    case ByteCode::I32X4RelaxedTruncF32X4UOpcode:
        simdEmitOp(compiler, SimdOp::fcvtzu | SimdOp::FS4, dst, args[0].arg, 0);
        break;
    case ByteCode::I32X4TruncSatF64X2SZeroOpcode:
    case ByteCode::I32X4RelaxedTruncF64X2SZeroOpcode:
        simdEmitOp(compiler, SimdOp::fcvtzs | SimdOp::FD2, dst, args[0].arg, 0);
        simdEmitOp(compiler, SimdOp::sqxtn | SimdOp::S4, dst, dst, 0);
        break;
    case ByteCode::I32X4TruncSatF64X2UZeroOpcode:
    case ByteCode::I32X4RelaxedTruncF64X2UZeroOpcode:
        simdEmitOp(compiler, SimdOp::fcvtzu | SimdOp::FD2, dst, args[0].arg, 0);
        simdEmitOp(compiler, SimdOp::uqxtn | SimdOp::S4, dst, dst, 0);
        break;
    case ByteCode::I64X2NegOpcode:
        simdEmitOp(compiler, SimdOp::neg | SimdOp::D2, dst, args[0].arg, 0);
        break;
    case ByteCode::I64X2AbsOpcode:
        simdEmitOp(compiler, SimdOp::abs | SimdOp::D2, dst, args[0].arg, 0);
        break;
    case ByteCode::I64X2ExtendLowI32X4SOpcode:
        simdEmitOp(compiler, SimdOp::sxtl | (0x1 << 21), dst, args[0].arg, 0);
        break;
    case ByteCode::I64X2ExtendHighI32X4SOpcode:
        simdEmitOp(compiler, SimdOp::sxtl | (0x1 << 21) | (0x1 << 30), dst, args[0].arg, 0);
        break;
    case ByteCode::I64X2ExtendLowI32X4UOpcode:
        simdEmitOp(compiler, SimdOp::uxtl | (0x1 << 21), dst, args[0].arg, 0);
        break;
    case ByteCode::I64X2ExtendHighI32X4UOpcode:
        simdEmitOp(compiler, SimdOp::uxtl | (0x1 << 21) | (0x1 << 30), dst, args[0].arg, 0);
        break;
    case ByteCode::F32X4AbsOpcode:
        simdEmitOp(compiler, SimdOp::fabs | SimdOp::FS4, dst, args[0].arg, 0);
        break;
    case ByteCode::F32X4CeilOpcode:
        simdEmitOp(compiler, SimdOp::frintp | SimdOp::FS4, dst, args[0].arg, 0);
        break;
    case ByteCode::F32X4FloorOpcode:
        simdEmitOp(compiler, SimdOp::frintm | SimdOp::FS4, dst, args[0].arg, 0);
        break;
    case ByteCode::F32X4NearestOpcode:
        simdEmitOp(compiler, SimdOp::frintn | SimdOp::FS4, dst, args[0].arg, 0);
        break;
    case ByteCode::F32X4NegOpcode:
        simdEmitOp(compiler, SimdOp::fneg | SimdOp::FS4, dst, args[0].arg, 0);
        break;
    case ByteCode::F32X4SqrtOpcode:
        simdEmitOp(compiler, SimdOp::fsqrt | SimdOp::FS4, dst, args[0].arg, 0);
        break;
    case ByteCode::F32X4TruncOpcode:
        simdEmitOp(compiler, SimdOp::frintz | SimdOp::FS4, dst, args[0].arg, 0);
        break;
    case ByteCode::F64X2PromoteLowF32X4Opcode:
        simdEmitOp(compiler, SimdOp::fcvtl | SimdOp::FD2, dst, args[0].arg, 0);
        break;
    case ByteCode::F64X2AbsOpcode:
        simdEmitOp(compiler, SimdOp::fabs | SimdOp::FD2, dst, args[0].arg, 0);
        break;
    case ByteCode::F64X2CeilOpcode:
        simdEmitOp(compiler, SimdOp::frintp | SimdOp::FD2, dst, args[0].arg, 0);
        break;
    case ByteCode::F64X2FloorOpcode:
        simdEmitOp(compiler, SimdOp::frintm | SimdOp::FD2, dst, args[0].arg, 0);
        break;
    case ByteCode::F64X2NearestOpcode:
        simdEmitOp(compiler, SimdOp::frintn | SimdOp::FD2, dst, args[0].arg, 0);
        break;
    case ByteCode::F64X2NegOpcode:
        simdEmitOp(compiler, SimdOp::fneg | SimdOp::FD2, dst, args[0].arg, 0);
        break;
    case ByteCode::F64X2SqrtOpcode:
        simdEmitOp(compiler, SimdOp::fsqrt | SimdOp::FD2, dst, args[0].arg, 0);
        break;
    case ByteCode::F64X2TruncOpcode:
        simdEmitOp(compiler, SimdOp::frintz | SimdOp::FD2, dst, args[0].arg, 0);
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    if (SLJIT_IS_MEM(args[1].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | dstType, dst, args[1].arg, args[1].argw);
    }
}

static void simdEmitAllTrue(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn, SimdOp::IntSizeType size)
{
    sljit_s32 tmpReg = SLJIT_TMP_DEST_FREG;

    ASSERT(size != SimdOp::D2);
    simdEmitOp(compiler, SimdOp::uminv | size, tmpReg, rn, 0);

    auto type = SLJIT_SIMD_ELEM_8;

    if (size == SimdOp::H8) {
        type = SLJIT_SIMD_ELEM_16;
    } else if (size == SimdOp::S4) {
        type = SLJIT_SIMD_ELEM_32;
    }

    sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_STORE | type, tmpReg, 0, rd, 0);
    sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_Z, rd, 0, SLJIT_IMM, 0);
}

static void simdEmitI64x2AllTrue(sljit_compiler* compiler, sljit_s32 rn)
{
    sljit_s32 tmpFReg = SLJIT_TMP_DEST_FREG;

    simdEmitOp(compiler, SimdOp::cmeqz | SimdOp::D2, tmpFReg, rn, rn);
    simdEmitOp(compiler, SimdOp::addp | SimdOp::D2, tmpFReg, tmpFReg, tmpFReg);
    sljit_emit_fop1(compiler, SLJIT_CMP_F64 | SLJIT_SET_ORDERED_EQUAL, tmpFReg, 0, tmpFReg, 0);
}

static void simdEmitV128AnyTrue(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn)
{
    sljit_s32 tmpFReg = SLJIT_TMP_DEST_FREG;

    simdEmitOp(compiler, SimdOp::umaxp | SimdOp::S4, tmpFReg, rn, rn);
    sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64 | SLJIT_SIMD_STORE, tmpFReg, 0, rd, 0);
    sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_Z, rd, 0, SLJIT_IMM, 0);
}

static bool emitUnaryCondSIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[2];

    sljit_s32 srcType = SLJIT_SIMD_ELEM_128;
    sljit_s32 type = SLJIT_NOT_EQUAL;

    switch (instr->opcode()) {
    case ByteCode::I8X16AllTrueOpcode:
        srcType = SLJIT_SIMD_ELEM_8;
        break;
    case ByteCode::I16X8AllTrueOpcode:
        srcType = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I32X4AllTrueOpcode:
        srcType = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I64X2AllTrueOpcode:
        srcType = SLJIT_SIMD_ELEM_64;
        type = SLJIT_ORDERED_EQUAL;
        break;
    default:
        ASSERT(instr->opcode() == ByteCode::V128AnyTrueOpcode);
        srcType = SLJIT_SIMD_ELEM_128;
        break;
    }

    simdOperandToArg(compiler, operands, args[0], srcType, instr->requiredReg(0));

    sljit_s32 dst = SLJIT_TMP_DEST_REG;

    if (!(instr->info() & Instruction::kIsMergeCompare)) {
        args[1].set(operands + 1);
        dst = GET_TARGET_REG(args[1].arg, SLJIT_TMP_DEST_REG);
    }

    switch (instr->opcode()) {
    case ByteCode::I8X16AllTrueOpcode:
        simdEmitAllTrue(compiler, dst, args[0].arg, SimdOp::B16);
        break;
    case ByteCode::I16X8AllTrueOpcode:
        simdEmitAllTrue(compiler, dst, args[0].arg, SimdOp::H8);
        break;
    case ByteCode::I32X4AllTrueOpcode:
        simdEmitAllTrue(compiler, dst, args[0].arg, SimdOp::S4);
        break;
    case ByteCode::I64X2AllTrueOpcode:
        simdEmitI64x2AllTrue(compiler, args[0].arg);
        break;
    default:
        ASSERT(instr->opcode() == ByteCode::V128AnyTrueOpcode);
        simdEmitV128AnyTrue(compiler, dst, args[0].arg);
        break;
    }

    ASSERT(instr->next() != nullptr);

    if (instr->info() & Instruction::kIsMergeCompare) {
        Instruction* nextInstr = instr->next()->asInstruction();

        if (nextInstr->opcode() == ByteCode::SelectOpcode) {
            emitSelect(compiler, nextInstr, type);
            return true;
        }

        ASSERT(nextInstr->opcode() == ByteCode::JumpIfTrueOpcode || nextInstr->opcode() == ByteCode::JumpIfFalseOpcode);

        if (nextInstr->opcode() == ByteCode::JumpIfFalseOpcode) {
            type ^= 0x1;
        }

        nextInstr->asExtended()->value().targetLabel->jumpFrom(sljit_emit_jump(compiler, type));
        return true;
    }

    sljit_emit_op_flags(compiler, SLJIT_MOV32, dst, 0, type);

    if (SLJIT_IS_MEM(args[1].arg)) {
        sljit_emit_op1(compiler, SLJIT_MOV32, args[1].arg, args[1].argw, dst, 0);
    }

    return false;
}

static void simdEmitI64x2Mul(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm)
{
    auto tmpReg1 = SLJIT_TMP_FR0;
    auto tmpReg2 = SLJIT_TMP_FR1;

    // Swap arguments.
    if (rd == rm) {
        rm = rn;
        rn = rd;
    }

    simdEmitOp(compiler, SimdOp::rev64 | SimdOp::S4, tmpReg2, rm, 0);
    simdEmitOp(compiler, SimdOp::mul | SimdOp::S4, tmpReg2, tmpReg2, rn);
    simdEmitOp(compiler, SimdOp::xtn | SimdOp::S4, tmpReg1, rn, 0);
    simdEmitOp(compiler, SimdOp::addp | SimdOp::S4, rd, tmpReg2, tmpReg2);
    simdEmitOp(compiler, SimdOp::xtn | SimdOp::S4, tmpReg2, rm, 0);
    simdEmitOp(compiler, SimdOp::shll | SimdOp::S4, rd, rd, 0);
    simdEmitOp(compiler, SimdOp::umlal | SimdOp::S4, rd, tmpReg2, tmpReg1);
}

static void simdEmitFloatPMinMax(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm, SimdOp::FloatSizeType size, bool isMax)
{
    if (isMax) {
        simdEmitOp(compiler, SimdOp::fcmgt | size, rd, rm, rn);
    } else {
        simdEmitOp(compiler, SimdOp::fcmgt | size, rd, rn, rm);
    }

    simdEmitOp(compiler, SimdOp::bsl | SimdOp::B16, rd, rm, rn);
}

static void simdEmitNarrowSigned(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm, SimdOp::IntSizeType size)
{
    simdEmitOp(compiler, SimdOp::sqxtn | size, rd, rn, 0);
    simdEmitOp(compiler, SimdOp::sqxtn | size | (0x1 << 30), rd, rm, 0);
}

static void simdEmitNarrowUnsigned(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm, SimdOp::IntSizeType size)
{
    simdEmitOp(compiler, SimdOp::sqxtun | size, rd, rn, 0);
    simdEmitOp(compiler, SimdOp::sqxtun | size | (0x1 << 30), rd, rm, 0);
}

static void simdEmitDot(sljit_compiler* compiler, uint32_t type, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm)
{
    // The rd can be tmpReg1
#ifdef __ARM_FEATURE_DOTPROD
    simdEmitOp(compiler, SimdOp::sdot | type, rd, rn, rm);
#else
    sljit_s32 tmpReg1 = SLJIT_TMP_FR0;
    sljit_s32 tmpReg2 = SLJIT_TMP_FR1;
    uint32_t lowType = type - (0x1 << SimdOp::sizeOffset);

    // tmpReg2 = rn * rm upper
    simdEmitOp(compiler, SimdOp::smull | lowType | (0x1 << 30), tmpReg2, rn, rm);
    // tmpReg1 = rn * rm lower
    simdEmitOp(compiler, SimdOp::smull | lowType, tmpReg1, rn, rm);
    // Widening result
    simdEmitOp(compiler, SimdOp::saddlp | type, tmpReg2, tmpReg2, 0);
    simdEmitOp(compiler, SimdOp::saddlp | type, tmpReg1, tmpReg1, 0);
    // Combine + narrow
    simdEmitOp(compiler, SimdOp::xtn | type, rd, tmpReg1, 0);
    simdEmitOp(compiler, SimdOp::xtn | type | (0x1 << 30), rd, tmpReg2, 0);
#endif
}

static void emitBinarySIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[3];

    sljit_s32 srcType = SLJIT_SIMD_ELEM_128;
    sljit_s32 dstType = SLJIT_SIMD_ELEM_128;

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
    case ByteCode::I8X16MinSOpcode:
    case ByteCode::I8X16MinUOpcode:
    case ByteCode::I8X16MaxSOpcode:
    case ByteCode::I8X16MaxUOpcode:
    case ByteCode::I8X16AvgrUOpcode:
    case ByteCode::I8X16SwizzleOpcode:
    case ByteCode::I8X16RelaxedSwizzleOpcode:
        srcType = SLJIT_SIMD_ELEM_8;
        dstType = SLJIT_SIMD_ELEM_8;
        break;
    case ByteCode::I8X16NarrowI16X8SOpcode:
    case ByteCode::I8X16NarrowI16X8UOpcode:
        srcType = SLJIT_SIMD_ELEM_16;
        dstType = SLJIT_SIMD_ELEM_8;
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
    case ByteCode::I16X8MinSOpcode:
    case ByteCode::I16X8MinUOpcode:
    case ByteCode::I16X8MaxSOpcode:
    case ByteCode::I16X8MaxUOpcode:
    case ByteCode::I16X8AvgrUOpcode:
    case ByteCode::I16X8Q15mulrSatSOpcode:
    case ByteCode::I16X8RelaxedQ15mulrSOpcode:
        srcType = SLJIT_SIMD_ELEM_16;
        dstType = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I16X8DotI8X16I7X16SOpcode:
    case ByteCode::I16X8ExtmulLowI8X16SOpcode:
    case ByteCode::I16X8ExtmulHighI8X16SOpcode:
    case ByteCode::I16X8ExtmulLowI8X16UOpcode:
    case ByteCode::I16X8ExtmulHighI8X16UOpcode:
        srcType = SLJIT_SIMD_ELEM_8;
        dstType = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I16X8NarrowI32X4SOpcode:
    case ByteCode::I16X8NarrowI32X4UOpcode:
        srcType = SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_ELEM_16;
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
    case ByteCode::I32X4MinSOpcode:
    case ByteCode::I32X4MinUOpcode:
    case ByteCode::I32X4MaxSOpcode:
    case ByteCode::I32X4MaxUOpcode:
        srcType = SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I32X4ExtmulLowI16X8SOpcode:
    case ByteCode::I32X4ExtmulHighI16X8SOpcode:
    case ByteCode::I32X4ExtmulLowI16X8UOpcode:
    case ByteCode::I32X4ExtmulHighI16X8UOpcode:
    case ByteCode::I32X4DotI16X8SOpcode:
        srcType = SLJIT_SIMD_ELEM_16;
        dstType = SLJIT_SIMD_ELEM_32;
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
        srcType = SLJIT_SIMD_ELEM_64;
        dstType = SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::I64X2ExtmulLowI32X4SOpcode:
    case ByteCode::I64X2ExtmulHighI32X4SOpcode:
    case ByteCode::I64X2ExtmulLowI32X4UOpcode:
    case ByteCode::I64X2ExtmulHighI32X4UOpcode:
        srcType = SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::F32X4AddOpcode:
    case ByteCode::F32X4SubOpcode:
    case ByteCode::F32X4MulOpcode:
    case ByteCode::F32X4DivOpcode:
    case ByteCode::F32X4EqOpcode:
    case ByteCode::F32X4NeOpcode:
    case ByteCode::F32X4LtOpcode:
    case ByteCode::F32X4LeOpcode:
    case ByteCode::F32X4GtOpcode:
    case ByteCode::F32X4GeOpcode:
    case ByteCode::F32X4PMinOpcode:
    case ByteCode::F32X4PMaxOpcode:
    case ByteCode::F32X4MaxOpcode:
    case ByteCode::F32X4MinOpcode:
    case ByteCode::F32X4RelaxedMaxOpcode:
    case ByteCode::F32X4RelaxedMinOpcode:
        srcType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::F64X2AddOpcode:
    case ByteCode::F64X2SubOpcode:
    case ByteCode::F64X2MulOpcode:
    case ByteCode::F64X2DivOpcode:
    case ByteCode::F64X2SqrtOpcode:
    case ByteCode::F64X2EqOpcode:
    case ByteCode::F64X2NeOpcode:
    case ByteCode::F64X2LtOpcode:
    case ByteCode::F64X2LeOpcode:
    case ByteCode::F64X2GtOpcode:
    case ByteCode::F64X2GeOpcode:
    case ByteCode::F64X2PMinOpcode:
    case ByteCode::F64X2PMaxOpcode:
    case ByteCode::F64X2MaxOpcode:
    case ByteCode::F64X2MinOpcode:
    case ByteCode::F64X2RelaxedMaxOpcode:
    case ByteCode::F64X2RelaxedMinOpcode:
        srcType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64;
        dstType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::V128AndOpcode:
    case ByteCode::V128OrOpcode:
    case ByteCode::V128XorOpcode:
    case ByteCode::V128AndnotOpcode:
        srcType = SLJIT_SIMD_ELEM_128;
        dstType = SLJIT_SIMD_ELEM_128;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    simdOperandToArg(compiler, operands, args[0], srcType, instr->requiredReg(0));
    simdOperandToArg(compiler, operands + 1, args[1], srcType, instr->requiredReg(1));

    args[2].set(operands + 2);
    sljit_s32 dst = GET_TARGET_REG(args[2].arg, instr->requiredReg(2));

    switch (instr->opcode()) {
    case ByteCode::F32X4EqOpcode:
        simdEmitOp(compiler, SimdOp::fcmeq, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4NeOpcode:
        simdEmitOp(compiler, SimdOp::fcmeq, dst, args[0].arg, args[1].arg);
        simdEmitOp(compiler, SimdOp::notOp, dst, dst, 0);
        break;
    case ByteCode::F32X4LtOpcode:
        simdEmitOp(compiler, SimdOp::fcmgt, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::F32X4LeOpcode:
        simdEmitOp(compiler, SimdOp::fcmge, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::F32X4GtOpcode:
        simdEmitOp(compiler, SimdOp::fcmgt, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4GeOpcode:
        simdEmitOp(compiler, SimdOp::fcmge, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16AddOpcode:
        simdEmitOp(compiler, SimdOp::add | SimdOp::B16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16SubOpcode:
        simdEmitOp(compiler, SimdOp::sub | SimdOp::B16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16AddSatSOpcode:
        simdEmitOp(compiler, SimdOp::sqadd | SimdOp::B16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16AddSatUOpcode:
        simdEmitOp(compiler, SimdOp::uqadd | SimdOp::B16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16SubSatSOpcode:
        simdEmitOp(compiler, SimdOp::sqsub | SimdOp::B16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16SubSatUOpcode:
        simdEmitOp(compiler, SimdOp::uqsub | SimdOp::B16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16EqOpcode:
        simdEmitOp(compiler, SimdOp::cmeq | SimdOp::B16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16NeOpcode:
        simdEmitOp(compiler, SimdOp::cmeq | SimdOp::B16, dst, args[0].arg, args[1].arg);
        simdEmitOp(compiler, SimdOp::notOp, dst, dst, 0);
        break;
    case ByteCode::I8X16LtSOpcode:
        simdEmitOp(compiler, SimdOp::cmgt | SimdOp::B16, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I8X16LtUOpcode:
        simdEmitOp(compiler, SimdOp::ucmgt | SimdOp::B16, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I8X16LeSOpcode:
        simdEmitOp(compiler, SimdOp::cmge | SimdOp::B16, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I8X16LeUOpcode:
        simdEmitOp(compiler, SimdOp::ucmge | SimdOp::B16, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I8X16GtSOpcode:
        simdEmitOp(compiler, SimdOp::cmgt | SimdOp::B16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16GtUOpcode:
        simdEmitOp(compiler, SimdOp::ucmgt | SimdOp::B16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16GeSOpcode:
        simdEmitOp(compiler, SimdOp::cmge | SimdOp::B16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16GeUOpcode:
        simdEmitOp(compiler, SimdOp::ucmge | SimdOp::B16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16MinSOpcode:
        simdEmitOp(compiler, SimdOp::smin | SimdOp::B16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16MinUOpcode:
        simdEmitOp(compiler, SimdOp::umin | SimdOp::B16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16MaxSOpcode:
        simdEmitOp(compiler, SimdOp::smax | SimdOp::B16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16MaxUOpcode:
        simdEmitOp(compiler, SimdOp::umax | SimdOp::B16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16AvgrUOpcode:
        simdEmitOp(compiler, SimdOp::urhadd | SimdOp::B16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16NarrowI16X8SOpcode:
        simdEmitNarrowSigned(compiler, dst, args[0].arg, args[1].arg, SimdOp::B16);
        break;
    case ByteCode::I8X16NarrowI16X8UOpcode:
        simdEmitNarrowUnsigned(compiler, dst, args[0].arg, args[1].arg, SimdOp::B16);
        break;
    case ByteCode::I16X8AddOpcode:
        simdEmitOp(compiler, SimdOp::add | SimdOp::H8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8SubOpcode:
        simdEmitOp(compiler, SimdOp::sub | SimdOp::H8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8MulOpcode:
        simdEmitOp(compiler, SimdOp::mul | SimdOp::H8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8AddSatSOpcode:
        simdEmitOp(compiler, SimdOp::sqadd | SimdOp::H8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8AddSatUOpcode:
        simdEmitOp(compiler, SimdOp::uqadd | SimdOp::H8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8SubSatSOpcode:
        simdEmitOp(compiler, SimdOp::sqsub | SimdOp::H8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8SubSatUOpcode:
        simdEmitOp(compiler, SimdOp::uqsub | SimdOp::H8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8EqOpcode:
        simdEmitOp(compiler, SimdOp::cmeq | SimdOp::H8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8NeOpcode:
        simdEmitOp(compiler, SimdOp::cmeq | SimdOp::H8, dst, args[0].arg, args[1].arg);
        simdEmitOp(compiler, SimdOp::notOp, dst, dst, 0);
        break;
    case ByteCode::I16X8LtSOpcode:
        simdEmitOp(compiler, SimdOp::cmgt | SimdOp::H8, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I16X8LtUOpcode:
        simdEmitOp(compiler, SimdOp::ucmgt | SimdOp::H8, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I16X8LeSOpcode:
        simdEmitOp(compiler, SimdOp::cmge | SimdOp::H8, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I16X8LeUOpcode:
        simdEmitOp(compiler, SimdOp::ucmge | SimdOp::H8, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I16X8GtSOpcode:
        simdEmitOp(compiler, SimdOp::cmgt | SimdOp::H8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8GtUOpcode:
        simdEmitOp(compiler, SimdOp::ucmgt | SimdOp::H8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8GeSOpcode:
        simdEmitOp(compiler, SimdOp::cmge | SimdOp::H8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8GeUOpcode:
        simdEmitOp(compiler, SimdOp::ucmge | SimdOp::H8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8MinSOpcode:
        simdEmitOp(compiler, SimdOp::smin | SimdOp::H8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8MinUOpcode:
        simdEmitOp(compiler, SimdOp::umin | SimdOp::H8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8MaxSOpcode:
        simdEmitOp(compiler, SimdOp::smax | SimdOp::H8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8MaxUOpcode:
        simdEmitOp(compiler, SimdOp::umax | SimdOp::H8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8AvgrUOpcode:
        simdEmitOp(compiler, SimdOp::urhadd | SimdOp::H8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8ExtmulLowI8X16SOpcode:
        simdEmitOp(compiler, SimdOp::smull | SimdOp::B16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8ExtmulHighI8X16SOpcode:
        simdEmitOp(compiler, SimdOp::smull | SimdOp::B16 | (0x1 << 30), dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8ExtmulLowI8X16UOpcode:
        simdEmitOp(compiler, SimdOp::umull | SimdOp::B16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8ExtmulHighI8X16UOpcode:
        simdEmitOp(compiler, SimdOp::umull | SimdOp::B16 | (0x1 << 30), dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8NarrowI32X4SOpcode:
        simdEmitNarrowSigned(compiler, dst, args[0].arg, args[1].arg, SimdOp::H8);
        break;
    case ByteCode::I16X8NarrowI32X4UOpcode:
        simdEmitNarrowUnsigned(compiler, dst, args[0].arg, args[1].arg, SimdOp::H8);
        break;
    case ByteCode::I16X8Q15mulrSatSOpcode:
    case ByteCode::I16X8RelaxedQ15mulrSOpcode:
        simdEmitOp(compiler, SimdOp::sqrdmulh | SimdOp::H8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8DotI8X16I7X16SOpcode:
        simdEmitDot(compiler, SimdOp::H8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4AddOpcode:
        simdEmitOp(compiler, SimdOp::add | SimdOp::S4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4SubOpcode:
        simdEmitOp(compiler, SimdOp::sub | SimdOp::S4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4MulOpcode:
        simdEmitOp(compiler, SimdOp::mul | SimdOp::S4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4EqOpcode:
        simdEmitOp(compiler, SimdOp::cmeq | SimdOp::S4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4NeOpcode:
        simdEmitOp(compiler, SimdOp::cmeq | SimdOp::S4, dst, args[0].arg, args[1].arg);
        simdEmitOp(compiler, SimdOp::notOp, dst, dst, 0);
        break;
    case ByteCode::I32X4LtSOpcode:
        simdEmitOp(compiler, SimdOp::cmgt | SimdOp::S4, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I32X4LtUOpcode:
        simdEmitOp(compiler, SimdOp::ucmgt | SimdOp::S4, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I32X4LeSOpcode:
        simdEmitOp(compiler, SimdOp::cmge | SimdOp::S4, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I32X4LeUOpcode:
        simdEmitOp(compiler, SimdOp::ucmge | SimdOp::S4, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I32X4GtSOpcode:
        simdEmitOp(compiler, SimdOp::cmgt | SimdOp::S4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4GtUOpcode:
        simdEmitOp(compiler, SimdOp::ucmgt | SimdOp::S4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4GeSOpcode:
        simdEmitOp(compiler, SimdOp::cmge | SimdOp::S4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4GeUOpcode:
        simdEmitOp(compiler, SimdOp::ucmge | SimdOp::S4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4MinSOpcode:
        simdEmitOp(compiler, SimdOp::smin | SimdOp::S4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4MinUOpcode:
        simdEmitOp(compiler, SimdOp::umin | SimdOp::S4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4MaxSOpcode:
        simdEmitOp(compiler, SimdOp::smax | SimdOp::S4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4MaxUOpcode:
        simdEmitOp(compiler, SimdOp::umax | SimdOp::S4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4ExtmulLowI16X8SOpcode:
        simdEmitOp(compiler, SimdOp::smull | SimdOp::H8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4ExtmulHighI16X8SOpcode:
        simdEmitOp(compiler, SimdOp::smull | SimdOp::H8 | (0x1 << 30), dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4ExtmulLowI16X8UOpcode:
        simdEmitOp(compiler, SimdOp::umull | SimdOp::H8, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4ExtmulHighI16X8UOpcode:
        simdEmitOp(compiler, SimdOp::umull | SimdOp::H8 | (0x1 << 30), dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4DotI16X8SOpcode:
        simdEmitDot(compiler, SimdOp::S4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2AddOpcode:
        simdEmitOp(compiler, SimdOp::add | SimdOp::D2, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2SubOpcode:
        simdEmitOp(compiler, SimdOp::sub | SimdOp::D2, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2MulOpcode:
        simdEmitI64x2Mul(compiler, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2EqOpcode:
        simdEmitOp(compiler, SimdOp::cmeq | SimdOp::D2, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2NeOpcode:
        simdEmitOp(compiler, SimdOp::cmeq | SimdOp::D2, dst, args[0].arg, args[1].arg);
        simdEmitOp(compiler, SimdOp::notOp, dst, dst, 0);
        break;
    case ByteCode::I64X2LtSOpcode:
        simdEmitOp(compiler, SimdOp::cmgt | SimdOp::D2, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I64X2LeSOpcode:
        simdEmitOp(compiler, SimdOp::cmge | SimdOp::D2, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::I64X2GtSOpcode:
        simdEmitOp(compiler, SimdOp::cmgt | SimdOp::D2, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2GeSOpcode:
        simdEmitOp(compiler, SimdOp::cmge | SimdOp::D2, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4AddOpcode:
        simdEmitOp(compiler, SimdOp::fadd | SimdOp::FS4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4DivOpcode:
        simdEmitOp(compiler, SimdOp::fdiv | SimdOp::FS4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4MaxOpcode:
    case ByteCode::F32X4RelaxedMaxOpcode:
        simdEmitOp(compiler, SimdOp::fmax | SimdOp::FS4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4MinOpcode:
    case ByteCode::F32X4RelaxedMinOpcode:
        simdEmitOp(compiler, SimdOp::fmin | SimdOp::FS4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4MulOpcode:
        simdEmitOp(compiler, SimdOp::fmul | SimdOp::FS4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4PMinOpcode:
        simdEmitFloatPMinMax(compiler, dst, args[0].arg, args[1].arg, SimdOp::FS4, false);
        break;
    case ByteCode::F32X4PMaxOpcode:
        simdEmitFloatPMinMax(compiler, dst, args[0].arg, args[1].arg, SimdOp::FS4, true);
        break;
    case ByteCode::F32X4SubOpcode:
        simdEmitOp(compiler, SimdOp::fsub | SimdOp::FS4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2AddOpcode:
        simdEmitOp(compiler, SimdOp::fadd | SimdOp::FD2, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2DivOpcode:
        simdEmitOp(compiler, SimdOp::fdiv | SimdOp::FD2, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2MaxOpcode:
    case ByteCode::F64X2RelaxedMaxOpcode:
        simdEmitOp(compiler, SimdOp::fmax | SimdOp::FD2, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2MinOpcode:
    case ByteCode::F64X2RelaxedMinOpcode:
        simdEmitOp(compiler, SimdOp::fmin | SimdOp::FD2, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2MulOpcode:
        simdEmitOp(compiler, SimdOp::fmul | SimdOp::FD2, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2PMinOpcode:
        simdEmitFloatPMinMax(compiler, dst, args[0].arg, args[1].arg, SimdOp::FD2, false);
        break;
    case ByteCode::F64X2PMaxOpcode:
        simdEmitFloatPMinMax(compiler, dst, args[0].arg, args[1].arg, SimdOp::FD2, true);
        break;
    case ByteCode::F64X2SubOpcode:
        simdEmitOp(compiler, SimdOp::fsub | SimdOp::FD2, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2EqOpcode:
        simdEmitOp(compiler, SimdOp::fcmeq | SimdOp::FD2, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2NeOpcode:
        simdEmitOp(compiler, SimdOp::fcmeq | SimdOp::FD2, dst, args[0].arg, args[1].arg);
        simdEmitOp(compiler, SimdOp::notOp, dst, dst, 0);
        break;
    case ByteCode::F64X2LtOpcode:
        simdEmitOp(compiler, SimdOp::fcmgt | SimdOp::FD2, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::F64X2LeOpcode:
        simdEmitOp(compiler, SimdOp::fcmge | SimdOp::FD2, dst, args[1].arg, args[0].arg);
        break;
    case ByteCode::F64X2GtOpcode:
        simdEmitOp(compiler, SimdOp::fcmgt | SimdOp::FD2, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2GeOpcode:
        simdEmitOp(compiler, SimdOp::fcmge | SimdOp::FD2, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2ExtmulLowI32X4SOpcode:
        simdEmitOp(compiler, SimdOp::smull | SimdOp::S4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2ExtmulHighI32X4SOpcode:
        simdEmitOp(compiler, SimdOp::smull | SimdOp::S4 | (0x1 << 30), dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2ExtmulLowI32X4UOpcode:
        simdEmitOp(compiler, SimdOp::umull | SimdOp::S4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2ExtmulHighI32X4UOpcode:
        simdEmitOp(compiler, SimdOp::umull | SimdOp::S4 | (0x1 << 30), dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::V128AndOpcode:
        simdEmitOp(compiler, SimdOp::andOp, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::V128OrOpcode:
        simdEmitOp(compiler, SimdOp::orr, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::V128XorOpcode:
        simdEmitOp(compiler, SimdOp::eor, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::V128AndnotOpcode:
        simdEmitOp(compiler, SimdOp::bic, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I8X16SwizzleOpcode:
    case ByteCode::I8X16RelaxedSwizzleOpcode:
        simdEmitOp(compiler, SimdOp::tbl, dst, args[0].arg, args[1].arg);
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    if (SLJIT_IS_MEM(args[2].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | dstType, dst, args[2].arg, args[2].argw);
    }
}

static void simdEmitDotAdd(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm, sljit_s32 ro)
{
    sljit_s32 tmpReg = SLJIT_TMP_DEST_FREG;

    simdEmitDot(compiler, SimdOp::H8, tmpReg, rn, rm);
    simdEmitOp(compiler, SimdOp::saddlp | SimdOp::H8, tmpReg, tmpReg, 0);
    simdEmitOp(compiler, SimdOp::add | SimdOp::S4, rd, ro, tmpReg);
}

static void emitTernarySIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[4];

    sljit_s32 srcType = SLJIT_SIMD_ELEM_128;
    sljit_s32 dstType = SLJIT_SIMD_ELEM_128;
    bool moveToDst = true;

    switch (instr->opcode()) {
    case ByteCode::V128BitSelectOpcode:
        srcType = SLJIT_SIMD_ELEM_128;
        dstType = SLJIT_SIMD_ELEM_128;
        break;
    case ByteCode::I8X16RelaxedLaneSelectOpcode:
        srcType = SLJIT_SIMD_ELEM_8;
        dstType = SLJIT_SIMD_ELEM_8;
        break;
    case ByteCode::I16X8RelaxedLaneSelectOpcode:
        srcType = SLJIT_SIMD_ELEM_16;
        dstType = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I32X4RelaxedLaneSelectOpcode:
        srcType = SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I64X2RelaxedLaneSelectOpcode:
        srcType = SLJIT_SIMD_ELEM_64;
        dstType = SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::I32X4DotI8X16I7X16AddSOpcode:
        srcType = SLJIT_SIMD_ELEM_8;
        dstType = SLJIT_SIMD_ELEM_32;
        moveToDst = false;
        break;
    case ByteCode::F32X4RelaxedMaddOpcode:
    case ByteCode::F32X4RelaxedNmaddOpcode:
        srcType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::F64X2RelaxedMaddOpcode:
    case ByteCode::F64X2RelaxedNmaddOpcode:
        srcType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64;
        dstType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    simdOperandToArg(compiler, operands, args[0], srcType, instr->requiredReg(0));
    simdOperandToArg(compiler, operands + 1, args[1], srcType, instr->requiredReg(1));
    simdOperandToArg(compiler, operands + 2, args[2], dstType, instr->requiredReg(2));

    args[3].set(operands + 3);
    sljit_s32 dst = GET_TARGET_REG(args[3].arg, instr->requiredReg(2));

    if (moveToDst && dst != args[2].arg) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128 | srcType, dst, args[2].arg, 0);
    }

    switch (instr->opcode()) {
    case ByteCode::V128BitSelectOpcode:
    case ByteCode::I8X16RelaxedLaneSelectOpcode:
    case ByteCode::I16X8RelaxedLaneSelectOpcode:
    case ByteCode::I32X4RelaxedLaneSelectOpcode:
    case ByteCode::I64X2RelaxedLaneSelectOpcode:
        simdEmitOp(compiler, SimdOp::bsl, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4DotI8X16I7X16AddSOpcode:
        simdEmitDotAdd(compiler, dst, args[0].arg, args[1].arg, args[2].arg);
        break;
    case ByteCode::F32X4RelaxedMaddOpcode:
        simdEmitOp(compiler, SimdOp::fmla | SimdOp::FS4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4RelaxedNmaddOpcode:
        simdEmitOp(compiler, SimdOp::fmls | SimdOp::FS4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2RelaxedMaddOpcode:
        simdEmitOp(compiler, SimdOp::fmla | SimdOp::FD2, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2RelaxedNmaddOpcode:
        simdEmitOp(compiler, SimdOp::fmls | SimdOp::FD2, dst, args[0].arg, args[1].arg);
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    if (SLJIT_IS_MEM(args[3].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | dstType, dst, args[3].arg, args[3].argw);
    }
}

static void emitShuffleSIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[3];

    /* Must be two consecutive registers! */
    ASSERT(sljit_get_register_index(SLJIT_SIMD_REG_128, SLJIT_TMP_FR0) + 1 == sljit_get_register_index(SLJIT_SIMD_REG_128, SLJIT_TMP_FR1));

    simdOperandToArg(compiler, operands, args[0], SLJIT_SIMD_ELEM_128, SLJIT_TMP_FR0);
    simdOperandToArg(compiler, operands + 1, args[1], SLJIT_SIMD_ELEM_128, SLJIT_TMP_FR1);
    args[2].set(operands + 2);
    sljit_s32 dst = GET_TARGET_REG(args[2].arg, instr->requiredReg(0));

    if (sljit_get_register_index(SLJIT_SIMD_REG_128, args[0].arg) + 1 != sljit_get_register_index(SLJIT_SIMD_REG_128, args[1].arg)) {
        if (args[0].arg != SLJIT_TMP_FR0) {
            sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, SLJIT_TMP_FR0, args[0].arg, args[0].argw);
            args[0].arg = SLJIT_TMP_FR0;
        }

        if (args[1].arg != SLJIT_TMP_FR1) {
            sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, SLJIT_TMP_FR1, args[1].arg, args[1].argw);
            args[1].arg = SLJIT_TMP_FR1;
        }
    } else if (dst == args[0].arg || dst == args[1].arg) {
        dst = SLJIT_TMP_FR0;
    }

    I8X16Shuffle* shuffle = reinterpret_cast<I8X16Shuffle*>(instr->byteCode());
    const sljit_s32 type = SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8;
    sljit_emit_simd_mov(compiler, type, dst, SLJIT_MEM0(), reinterpret_cast<sljit_sw>(shuffle->value()));

    simdEmitOp(compiler, SimdOp::tbl | (1 << 13), dst, args[0].arg, dst);

    if (args[2].arg != dst) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, dst, args[2].arg, args[2].argw);
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
        op = SimdOp::sshl | SimdOp::B16;
        immOp = SimdOp::shl | SimdOp::ImmB16;
        mask = SimdOp::shiftB;
        type = SLJIT_SIMD_ELEM_8;
        break;
    case ByteCode::I8X16ShrSOpcode:
        op = SimdOp::sshl | SimdOp::B16;
        immOp = SimdOp::sshr | SimdOp::ImmB16;
        mask = SimdOp::shiftB;
        type = SLJIT_SIMD_ELEM_8;
        break;
    case ByteCode::I8X16ShrUOpcode:
        op = SimdOp::ushl | SimdOp::B16;
        immOp = SimdOp::ushr | SimdOp::ImmB16;
        mask = SimdOp::shiftB;
        type = SLJIT_SIMD_ELEM_8;
        break;
    case ByteCode::I16X8ShlOpcode:
        isShr = false;
        op = SimdOp::sshl | SimdOp::H8;
        immOp = SimdOp::shl | SimdOp::ImmH8;
        mask = SimdOp::shiftH;
        type = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I16X8ShrSOpcode:
        op = SimdOp::sshl | SimdOp::H8;
        immOp = SimdOp::sshr | SimdOp::ImmH8;
        mask = SimdOp::shiftH;
        type = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I16X8ShrUOpcode:
        op = SimdOp::ushl | SimdOp::H8;
        immOp = SimdOp::ushr | SimdOp::ImmH8;
        mask = SimdOp::shiftH;
        type = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I32X4ShlOpcode:
        isShr = false;
        op = SimdOp::sshl | SimdOp::S4;
        immOp = SimdOp::shl | SimdOp::ImmS4;
        mask = SimdOp::shiftS;
        type = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I32X4ShrSOpcode:
        op = SimdOp::sshl | SimdOp::S4;
        immOp = SimdOp::sshr | SimdOp::ImmS4;
        mask = SimdOp::shiftS;
        type = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I32X4ShrUOpcode:
        op = SimdOp::ushl | SimdOp::S4;
        immOp = SimdOp::ushr | SimdOp::ImmS4;
        mask = SimdOp::shiftS;
        type = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I64X2ShlOpcode:
        isShr = false;
        op = SimdOp::sshl | SimdOp::D2;
        immOp = SimdOp::shl | SimdOp::ImmD2;
        mask = SimdOp::shiftD;
        type = SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::I64X2ShrSOpcode:
        op = SimdOp::sshl | SimdOp::D2;
        immOp = SimdOp::sshr | SimdOp::ImmD2;
        mask = SimdOp::shiftD;
        type = SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::I64X2ShrUOpcode:
        op = SimdOp::ushl | SimdOp::D2;
        immOp = SimdOp::ushr | SimdOp::ImmD2;
        mask = SimdOp::shiftD;
        type = SLJIT_SIMD_ELEM_64;
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    simdOperandToArg(compiler, operands, args[0], type, SLJIT_TMP_DEST_FREG);
    args[1].set(operands + 1);

    args[2].set(operands + 2);
    sljit_s32 dst = GET_TARGET_REG(args[2].arg, instr->requiredReg(0));
    sljit_s32 shiftReg = (dst == args[0].arg) ? SLJIT_TMP_DEST_FREG : dst;

    if (SLJIT_IS_IMM(args[1].arg)) {
        if (isShr)
            args[1].argw = (args[1].argw ^ mask) + 1;

        args[1].argw &= mask;
        if (args[1].argw == 0) {
            // Move operation.
            if (args[2].arg != args[0].arg) {
                sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | type, args[0].arg, args[2].arg, args[2].argw);
            }
            return;
        }

        simdEmitOp(compiler, immOp | (args[1].argw << 16), dst, args[0].arg, 0);
    } else {
        sljit_emit_op2(compiler, SLJIT_AND32, SLJIT_TMP_DEST_REG, 0, args[1].arg, args[1].argw, SLJIT_IMM, mask);

        if (isShr) {
            sljit_emit_op2(compiler, SLJIT_SUB, SLJIT_TMP_DEST_REG, 0, SLJIT_IMM, 0, SLJIT_TMP_DEST_REG, 0);
        }

        sljit_emit_simd_replicate(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8, shiftReg, SLJIT_TMP_DEST_REG, 0);
        simdEmitOp(compiler, op, dst, args[0].arg, shiftReg);
    }

    if (SLJIT_IS_MEM(args[2].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | type, dst, args[2].arg, args[2].argw);
    }
}
