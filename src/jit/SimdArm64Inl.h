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
    add = 0x4e208400,
    addp = 0x4e20bc00,
    andOp = 0x4e201c00,
    bic = 0x4e601c00,
    bsl = 0x6e601c00,
    cmeq = 0x6e208c00,
    cmeqz = 0x4e209800,
    cmge = 0x4e203c00,
    cmgt = 0x4e203400,
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
    umaxp = 0x6e20a400,
    uminv = 0x6e31a800,
    umlal = 0x2e208000,
    uqadd = 0x6e200c00,
    uqsub = 0x6e202c00,
    uqxtn = 0x2e214800,
    uxtl = 0x2f00a400,
    ushl = 0x6e204400,
    ushr = 0x6f000400,
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

static void simdEmitI64x2Mul(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm)
{
    auto tmpReg1 = SLJIT_FR2;
    auto tmpReg2 = SLJIT_FR3;

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

static void simdEmitV128AnyTrue(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn)
{
    simdEmitOp(compiler, SimdOp::umaxp | SimdOp::S4, rn, rn, rn);
    sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64 | SLJIT_SIMD_STORE, rd, 1, rn, 0);
    sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_Z, rd, 0, SLJIT_IMM, 0);
    sljit_emit_op_flags(compiler, SLJIT_MOV, rd, 0, SLJIT_NOT_EQUAL);
}

static void simdEmitAllTrue(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn, SimdOp::IntSizeType size)
{
    ASSERT(size != SimdOp::D2);
    simdEmitOp(compiler, SimdOp::uminv | size, rn, rn, 0);

    auto type = SLJIT_SIMD_ELEM_8;

    if (size == SimdOp::H8) {
        type = SLJIT_SIMD_ELEM_16;
    } else if (size == SimdOp::S4) {
        type = SLJIT_SIMD_ELEM_32;
    }

    sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_STORE | type, rd, 0, rn, 0);
    sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_Z, rd, 0, SLJIT_IMM, 0);
    sljit_emit_op_flags(compiler, SLJIT_MOV, rd, 0, SLJIT_NOT_EQUAL);
}

static void simdEmitI64x2AllTrue(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn)
{
    simdEmitOp(compiler, SimdOp::cmeqz | SimdOp::D2, rn, rn, rn);
    simdEmitOp(compiler, SimdOp::addp | SimdOp::D2, rn, rn, rn);
    sljit_emit_fop1(compiler, SLJIT_CMP_F64 | SLJIT_SET_ORDERED_EQUAL, rn, 0, rn, 0);
    sljit_emit_op_flags(compiler, SLJIT_MOV, rd, 0, SLJIT_ORDERED_EQUAL);
}

static void simdEmitDot(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm)
{
#ifdef __ARM_FEATURE_DOTPROD
    simdEmitOp(compiler, SimdOp::sdot | SimdOp::S4, rd, rn, rm);
#else
    auto tmpReg1 = SLJIT_FR2;
    auto tmpReg2 = SLJIT_FR3;

    // tmpReg1 = rn * rm lower
    simdEmitOp(compiler, SimdOp::smull | SimdOp::H8, tmpReg1, rn, rm);
    // tmpReg2 = rn * rm upper
    simdEmitOp(compiler, SimdOp::smull | SimdOp::H8 | (0x1 << 30), tmpReg2, rn, rm);
    // rd = tmpReg1[1], tmpReg2[1], tmpReg1[0], tmpReg2[0]
    simdEmitOp(compiler, SimdOp::zip1 | SimdOp::S4, rd, tmpReg1, tmpReg2);
    // tmpReg1 = tmpReg1[3], tmpReg2[3], tmpReg1[2], tmpReg2[2]
    simdEmitOp(compiler, SimdOp::zip2 | SimdOp::S4, tmpReg1, tmpReg1, tmpReg2);
    // rd = rd[3] + rd[2], rd[1] + rd[0]
    simdEmitOp(compiler, SimdOp::saddlp | SimdOp::S4, rd, rd, 0);
    // tmpReg1 = tmpReg1[3] + tmpReg1[2], tmpReg1[1] + tmpReg1[0]
    simdEmitOp(compiler, SimdOp::saddlp | SimdOp::S4, tmpReg1, tmpReg1, 0);
    // rd = rd[1], tmpReg1[1], rd[0], tmpReg1[0]
    simdEmitOp(compiler, SimdOp::uzp1 | SimdOp::S4, rd, rd, tmpReg1);
#endif
}

static void emitUnarySIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[2];

    sljit_s32 type = SLJIT_SIMD_ELEM_128;
    bool isDSTNormalRegister = false;

    switch (instr->opcode()) {
    case ByteCode::F32X4AbsOpcode:
    case ByteCode::F32X4CeilOpcode:
    case ByteCode::F32X4FloorOpcode:
    case ByteCode::F32X4NearestOpcode:
    case ByteCode::F32X4NegOpcode:
    case ByteCode::F32X4SqrtOpcode:
    case ByteCode::F32X4TruncOpcode:
    case ByteCode::F64X2PromoteLowF32X4Opcode:
    case ByteCode::I32X4TruncSatF32X4SOpcode:
    case ByteCode::I32X4TruncSatF32X4UOpcode:
    case ByteCode::I32X4TruncSatF64X2SZeroOpcode:
    case ByteCode::I32X4TruncSatF64X2UZeroOpcode:
        type = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::F64X2AbsOpcode:
    case ByteCode::F64X2CeilOpcode:
    case ByteCode::F64X2FloorOpcode:
    case ByteCode::F64X2NearestOpcode:
    case ByteCode::F64X2NegOpcode:
    case ByteCode::F64X2SqrtOpcode:
    case ByteCode::F64X2TruncOpcode:
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
    case ByteCode::I16X8ExtaddPairwiseI8X16SOpcode:
    case ByteCode::I16X8ExtaddPairwiseI8X16UOpcode:
    case ByteCode::I16X8NegOpcode:
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
    case ByteCode::V128AnyTrueOpcode:
        simdEmitV128AnyTrue(compiler, dst, args[0].arg);
        break;
    case ByteCode::I8X16NegOpcode:
        simdEmitOp(compiler, SimdOp::neg | SimdOp::B16, dst, args[0].arg, 0);
        break;
    case ByteCode::I8X16AllTrueOpcode:
        simdEmitAllTrue(compiler, dst, args[0].arg, SimdOp::B16);
        break;
    case ByteCode::I16X8NegOpcode:
        simdEmitOp(compiler, SimdOp::neg | SimdOp::H8, dst, args[0].arg, 0);
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
    case ByteCode::I16X8AllTrueOpcode:
        simdEmitAllTrue(compiler, dst, args[0].arg, SimdOp::H8);
        break;
    case ByteCode::I32X4NegOpcode:
        simdEmitOp(compiler, SimdOp::neg | SimdOp::S4, dst, args[0].arg, 0);
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
    case ByteCode::I32X4AllTrueOpcode:
        simdEmitAllTrue(compiler, dst, args[0].arg, SimdOp::S4);
        break;
    case ByteCode::I32X4TruncSatF32X4SOpcode:
        simdEmitOp(compiler, SimdOp::fcvtzs | SimdOp::FS4, dst, args[0].arg, 0);
        break;
    case ByteCode::I32X4TruncSatF32X4UOpcode:
        simdEmitOp(compiler, SimdOp::fcvtzu | SimdOp::FS4, dst, args[0].arg, 0);
        break;
    case ByteCode::I32X4TruncSatF64X2SZeroOpcode:
        simdEmitOp(compiler, SimdOp::fcvtzs | SimdOp::FD2, dst, args[0].arg, 0);
        simdEmitOp(compiler, SimdOp::sqxtn | SimdOp::S4, dst, dst, 0);
        break;
    case ByteCode::I32X4TruncSatF64X2UZeroOpcode:
        simdEmitOp(compiler, SimdOp::fcvtzu | SimdOp::FD2, dst, args[0].arg, 0);
        simdEmitOp(compiler, SimdOp::uqxtn | SimdOp::S4, dst, dst, 0);
        break;
    case ByteCode::I64X2NegOpcode:
        simdEmitOp(compiler, SimdOp::neg | SimdOp::D2, dst, args[0].arg, 0);
        break;
    case ByteCode::I64X2AllTrueOpcode:
        simdEmitI64x2AllTrue(compiler, dst, args[0].arg);
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
        if (!isDSTNormalRegister) {
            sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | type, dst, args[1].arg, args[1].argw);
        } else {
            sljit_emit_op1(compiler, SLJIT_MOV32, args[1].arg, args[1].argw, dst, 0);
        }
    }
}

static void emitBinarySIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[3];

    sljit_s32 type = SLJIT_SIMD_ELEM_128;
    bool isDSTModified = false;

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
    case ByteCode::I8X16NarrowI16X8SOpcode:
    case ByteCode::I8X16NarrowI16X8UOpcode:
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
    case ByteCode::I16X8NarrowI32X4SOpcode:
    case ByteCode::I16X8NarrowI32X4UOpcode:
    case ByteCode::I16X8Q15mulrSatSOpcode:
    case ByteCode::I32X4DotI16X8SOpcode:
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
    case ByteCode::F32X4PMinOpcode:
    case ByteCode::F32X4PMaxOpcode:
        isDSTModified = true;
        FALLTHROUGH;
    case ByteCode::F32X4AddOpcode:
    case ByteCode::F32X4DivOpcode:
    case ByteCode::F32X4MaxOpcode:
    case ByteCode::F32X4MinOpcode:
    case ByteCode::F32X4MulOpcode:
    case ByteCode::F32X4SubOpcode:
    case ByteCode::F32X4EqOpcode:
    case ByteCode::F32X4NeOpcode:
    case ByteCode::F32X4LtOpcode:
    case ByteCode::F32X4LeOpcode:
    case ByteCode::F32X4GtOpcode:
    case ByteCode::F32X4GeOpcode:
        type = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::F64X2PMinOpcode:
    case ByteCode::F64X2PMaxOpcode:
        isDSTModified = true;
        FALLTHROUGH;
    case ByteCode::F64X2AddOpcode:
    case ByteCode::F64X2DivOpcode:
    case ByteCode::F64X2MaxOpcode:
    case ByteCode::F64X2MinOpcode:
    case ByteCode::F64X2MulOpcode:
    case ByteCode::F64X2SubOpcode:
    case ByteCode::F64X2EqOpcode:
    case ByteCode::F64X2NeOpcode:
    case ByteCode::F64X2LtOpcode:
    case ByteCode::F64X2LeOpcode:
    case ByteCode::F64X2GtOpcode:
    case ByteCode::F64X2GeOpcode:
        type = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::V128AndOpcode:
    case ByteCode::V128OrOpcode:
    case ByteCode::V128XorOpcode:
    case ByteCode::V128AndnotOpcode:
    case ByteCode::I8X16SwizzleOpcode:
        type = SLJIT_SIMD_ELEM_128;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    simdOperandToArg(compiler, operands, args[0], type, SLJIT_FR0);
    simdOperandToArg(compiler, operands + 1, args[1], type, SLJIT_FR1);

    args[2].set(operands + 2);
    sljit_s32 dst = GET_TARGET_REG(args[2].arg, isDSTModified ? SLJIT_FR2 : SLJIT_FR0);

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
    case ByteCode::I16X8ExtmulLowI8X16SOpcode: {
        auto tmpReg = SLJIT_FR2;
        simdEmitOp(compiler, SimdOp::sxtl | (0x1 << 19), dst, args[0].arg, 0);
        simdEmitOp(compiler, SimdOp::sxtl | (0x1 << 19), tmpReg, args[1].arg, 0);
        simdEmitOp(compiler, SimdOp::mul | SimdOp::H8, dst, dst, tmpReg);
        break;
    }
    case ByteCode::I16X8ExtmulHighI8X16SOpcode: {
        auto tmpReg = SLJIT_FR2;
        simdEmitOp(compiler, SimdOp::sxtl | (0x1 << 19) | (0x1 << 30), dst, args[0].arg, 0);
        simdEmitOp(compiler, SimdOp::sxtl | (0x1 << 19) | (0x1 << 30), tmpReg, args[1].arg, 0);
        simdEmitOp(compiler, SimdOp::mul | SimdOp::H8, dst, dst, tmpReg);
        break;
    }
    case ByteCode::I16X8ExtmulLowI8X16UOpcode: {
        auto tmpReg = SLJIT_FR2;
        simdEmitOp(compiler, SimdOp::uxtl | (0x1 << 19), dst, args[0].arg, 0);
        simdEmitOp(compiler, SimdOp::uxtl | (0x1 << 19), tmpReg, args[1].arg, 0);
        simdEmitOp(compiler, SimdOp::mul | SimdOp::H8, dst, dst, tmpReg);
        break;
    }
    case ByteCode::I16X8ExtmulHighI8X16UOpcode: {
        auto tmpReg = SLJIT_FR2;
        simdEmitOp(compiler, SimdOp::uxtl | (0x1 << 19) | (0x1 << 30), dst, args[0].arg, 0);
        simdEmitOp(compiler, SimdOp::uxtl | (0x1 << 19) | (0x1 << 30), tmpReg, args[1].arg, 0);
        simdEmitOp(compiler, SimdOp::mul | SimdOp::H8, dst, dst, tmpReg);
        break;
    }
    case ByteCode::I16X8NarrowI32X4SOpcode:
        simdEmitNarrowSigned(compiler, dst, args[0].arg, args[1].arg, SimdOp::H8);
        break;
    case ByteCode::I16X8NarrowI32X4UOpcode:
        simdEmitNarrowUnsigned(compiler, dst, args[0].arg, args[1].arg, SimdOp::H8);
        break;
    case ByteCode::I16X8Q15mulrSatSOpcode:
        simdEmitOp(compiler, SimdOp::sqrdmulh | SimdOp::H8, dst, args[0].arg, args[1].arg);
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
    case ByteCode::I32X4ExtmulLowI16X8SOpcode: {
        auto tmpReg = SLJIT_FR2;
        simdEmitOp(compiler, SimdOp::sxtl | (0x1 << 20), dst, args[0].arg, 0);
        simdEmitOp(compiler, SimdOp::sxtl | (0x1 << 20), tmpReg, args[1].arg, 0);
        simdEmitOp(compiler, SimdOp::mul | SimdOp::S4, dst, dst, tmpReg);
        break;
    }
    case ByteCode::I32X4ExtmulHighI16X8SOpcode: {
        auto tmpReg = SLJIT_FR2;
        simdEmitOp(compiler, SimdOp::sxtl | (0x1 << 20) | (0x1 << 30), dst, args[0].arg, 0);
        simdEmitOp(compiler, SimdOp::sxtl | (0x1 << 20) | (0x1 << 30), tmpReg, args[1].arg, 0);
        simdEmitOp(compiler, SimdOp::mul | SimdOp::S4, dst, dst, tmpReg);
        break;
    }
    case ByteCode::I32X4ExtmulLowI16X8UOpcode: {
        auto tmpReg = SLJIT_FR2;
        simdEmitOp(compiler, SimdOp::uxtl | (0x1 << 20), dst, args[0].arg, 0);
        simdEmitOp(compiler, SimdOp::uxtl | (0x1 << 20), tmpReg, args[1].arg, 0);
        simdEmitOp(compiler, SimdOp::mul | SimdOp::S4, dst, dst, tmpReg);
        break;
    }
    case ByteCode::I32X4ExtmulHighI16X8UOpcode: {
        auto tmpReg = SLJIT_FR2;
        simdEmitOp(compiler, SimdOp::uxtl | (0x1 << 20) | (0x1 << 30), dst, args[0].arg, 0);
        simdEmitOp(compiler, SimdOp::uxtl | (0x1 << 20) | (0x1 << 30), tmpReg, args[1].arg, 0);
        simdEmitOp(compiler, SimdOp::mul | SimdOp::S4, dst, dst, tmpReg);
        break;
    }
    case ByteCode::I32X4DotI16X8SOpcode:
        simdEmitDot(compiler, dst, args[0].arg, args[1].arg);
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
        simdEmitOp(compiler, SimdOp::fmax | SimdOp::FS4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4MinOpcode:
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
        simdEmitOp(compiler, SimdOp::fmax | SimdOp::FD2, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2MinOpcode:
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
    case ByteCode::I64X2ExtmulLowI32X4SOpcode: {
        auto tmpReg = SLJIT_FR4;
        simdEmitOp(compiler, SimdOp::sxtl | (0x1 << 21), dst, args[0].arg, 0);
        simdEmitOp(compiler, SimdOp::sxtl | (0x1 << 21), tmpReg, args[1].arg, 0);
        simdEmitI64x2Mul(compiler, dst, dst, tmpReg);
        break;
    }
    case ByteCode::I64X2ExtmulHighI32X4SOpcode: {
        auto tmpReg = SLJIT_FR4;
        simdEmitOp(compiler, SimdOp::sxtl | (0x1 << 21) | (0x1 << 30), dst, args[0].arg, 0);
        simdEmitOp(compiler, SimdOp::sxtl | (0x1 << 21) | (0x1 << 30), tmpReg, args[1].arg, 0);
        simdEmitI64x2Mul(compiler, dst, dst, tmpReg);
        break;
    }
    case ByteCode::I64X2ExtmulLowI32X4UOpcode: {
        auto tmpReg = SLJIT_FR4;
        simdEmitOp(compiler, SimdOp::uxtl | (0x1 << 21), dst, args[0].arg, 0);
        simdEmitOp(compiler, SimdOp::uxtl | (0x1 << 21), tmpReg, args[1].arg, 0);
        simdEmitI64x2Mul(compiler, dst, dst, tmpReg);
        break;
    }
    case ByteCode::I64X2ExtmulHighI32X4UOpcode: {
        auto tmpReg = SLJIT_FR4;
        simdEmitOp(compiler, SimdOp::uxtl | (0x1 << 21) | (0x1 << 30), dst, args[0].arg, 0);
        simdEmitOp(compiler, SimdOp::uxtl | (0x1 << 21) | (0x1 << 30), tmpReg, args[1].arg, 0);
        simdEmitI64x2Mul(compiler, dst, dst, tmpReg);
        break;
    }
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
        simdEmitOp(compiler, SimdOp::tbl, dst, args[0].arg, args[1].arg);
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

    simdOperandToArg(compiler, operands, args[0], SLJIT_SIMD_ELEM_128, SLJIT_FR2);
    simdOperandToArg(compiler, operands + 1, args[1], SLJIT_SIMD_ELEM_128, SLJIT_FR1);
    simdOperandToArg(compiler, operands + 2, args[2], SLJIT_SIMD_ELEM_128, SLJIT_FR0);

    simdEmitOp(compiler, SimdOp::bsl, args[2].arg, args[0].arg, args[1].arg);

    args[1].set(operands + 3);
    sljit_s32 dst = GET_TARGET_REG(args[1].arg, SLJIT_FR0);

    if (SLJIT_IS_MEM(args[1].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, dst, args[1].arg, args[1].argw);
    }
}

static void emitShuffleSIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[3];

    /* Must be two consecutive registers! */
    simdOperandToArg(compiler, operands, args[0], SLJIT_SIMD_ELEM_128, SLJIT_FR1);
    simdOperandToArg(compiler, operands + 1, args[1], SLJIT_SIMD_ELEM_128, SLJIT_FR2);

    args[2].set(operands + 2);
    sljit_s32 dst = GET_TARGET_REG(args[2].arg, SLJIT_FR0);

    I8X16Shuffle* shuffle = reinterpret_cast<I8X16Shuffle*>(instr->byteCode());
    const sljit_s32 type = SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8;
    sljit_emit_simd_mov(compiler, type, dst, SLJIT_MEM0(), reinterpret_cast<sljit_sw>(shuffle->value()));

    simdEmitOp(compiler, SimdOp::tbl | (1 << 13), dst, args[0].arg, dst);

    if (SLJIT_IS_MEM(args[2].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, dst, args[2].arg, args[2].argw);
    }
}

static void emitShiftSIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[3];

    sljit_s32 type = SLJIT_SIMD_ELEM_128;

    int mask = 0;
    bool isShr = true;

    switch (instr->opcode()) {
    case ByteCode::I8X16ShlOpcode:
        isShr = false;
        FALLTHROUGH;
    case ByteCode::I8X16ShrSOpcode:
    case ByteCode::I8X16ShrUOpcode:
        mask = SimdOp::shiftB;
        type = SLJIT_SIMD_ELEM_8;
        break;
    case ByteCode::I16X8ShlOpcode:
        isShr = false;
        FALLTHROUGH;
    case ByteCode::I16X8ShrSOpcode:
    case ByteCode::I16X8ShrUOpcode:
        mask = SimdOp::shiftH;
        type = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I32X4ShlOpcode:
        isShr = false;
        FALLTHROUGH;
    case ByteCode::I32X4ShrSOpcode:
    case ByteCode::I32X4ShrUOpcode:
        mask = SimdOp::shiftS;
        type = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I64X2ShlOpcode:
        isShr = false;
        FALLTHROUGH;
    case ByteCode::I64X2ShrSOpcode:
    case ByteCode::I64X2ShrUOpcode:
        mask = SimdOp::shiftD;
        type = SLJIT_SIMD_ELEM_64;
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    simdOperandToArg(compiler, operands, args[0], type, SLJIT_FR0);
    args[1].set(operands + 1);

    args[2].set(operands + 2);
    sljit_s32 dst = GET_TARGET_REG(args[2].arg, SLJIT_FR1);

    if (SLJIT_IS_IMM(args[1].arg)) {
        if (isShr)
            args[1].argw = (args[1].argw ^ mask) + 1;

        args[1].argw &= mask;
        if (args[1].argw == 0) {
            if (args[2].arg != args[0].arg) {
                sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | type, args[0].arg, args[2].arg, args[2].argw);
            }
            return;
        }
    } else {
        sljit_s32 srcReg = GET_SOURCE_REG(args[1].arg, SLJIT_R0);
        MOVE_TO_REG(compiler, SLJIT_MOV32, srcReg, args[1].arg, args[1].argw);

        sljit_emit_op2(compiler, SLJIT_AND32, srcReg, 0, srcReg, 0, SLJIT_IMM, mask);
        sljit_emit_simd_replicate(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8, dst, srcReg, 0);
    }

    switch (instr->opcode()) {
    case ByteCode::I8X16ShlOpcode:
        if (SLJIT_IS_IMM(args[1].arg)) {
            simdEmitOp(compiler, SimdOp::shl | SimdOp::ImmB16 | (args[1].argw << 16), dst, args[0].arg, 0);
        } else {
            simdEmitOp(compiler, SimdOp::sshl | SimdOp::B16, dst, args[0].arg, dst);
        }
        break;
    case ByteCode::I8X16ShrSOpcode:
        if (SLJIT_IS_IMM(args[1].arg)) {
            simdEmitOp(compiler, SimdOp::sshr | SimdOp::ImmB16 | (args[1].argw << 16), dst, args[0].arg, 0);
        } else {
            simdEmitOp(compiler, SimdOp::neg | SimdOp::B16, dst, dst, 0);
            simdEmitOp(compiler, SimdOp::sshl | SimdOp::B16, dst, args[0].arg, dst);
        }
        break;
    case ByteCode::I8X16ShrUOpcode:
        if (SLJIT_IS_IMM(args[1].arg)) {
            simdEmitOp(compiler, SimdOp::ushr | SimdOp::ImmB16 | (args[1].argw << 16), dst, args[0].arg, 0);
        } else {
            simdEmitOp(compiler, SimdOp::neg | SimdOp::B16, dst, dst, 0);
            simdEmitOp(compiler, SimdOp::ushl | SimdOp::B16, dst, args[0].arg, dst);
        }
        break;
    case ByteCode::I16X8ShlOpcode:
        if (SLJIT_IS_IMM(args[1].arg)) {
            simdEmitOp(compiler, SimdOp::shl | SimdOp::ImmH8 | (args[1].argw << 16), dst, args[0].arg, 0);
        } else {
            simdEmitOp(compiler, SimdOp::sshl | SimdOp::H8, dst, args[0].arg, dst);
        }
        break;
    case ByteCode::I16X8ShrSOpcode:
        if (SLJIT_IS_IMM(args[1].arg)) {
            simdEmitOp(compiler, SimdOp::sshr | SimdOp::ImmH8 | (args[1].argw << 16), dst, args[0].arg, 0);
        } else {
            simdEmitOp(compiler, SimdOp::neg | SimdOp::H8, dst, dst, 0);
            simdEmitOp(compiler, SimdOp::sshl | SimdOp::H8, dst, args[0].arg, dst);
        }
        break;
    case ByteCode::I16X8ShrUOpcode:
        if (SLJIT_IS_IMM(args[1].arg)) {
            simdEmitOp(compiler, SimdOp::ushr | SimdOp::ImmH8 | (args[1].argw << 16), dst, args[0].arg, 0);
        } else {
            simdEmitOp(compiler, SimdOp::neg | SimdOp::H8, dst, dst, 0);
            simdEmitOp(compiler, SimdOp::ushl | SimdOp::H8, dst, args[0].arg, dst);
        }
        break;
    case ByteCode::I32X4ShlOpcode:
        if (SLJIT_IS_IMM(args[1].arg)) {
            simdEmitOp(compiler, SimdOp::shl | SimdOp::ImmS4 | (args[1].argw << 16), dst, args[0].arg, 0);
        } else {
            simdEmitOp(compiler, SimdOp::sshl | SimdOp::S4, dst, args[0].arg, dst);
        }
        break;
    case ByteCode::I32X4ShrSOpcode:
        if (SLJIT_IS_IMM(args[1].arg)) {
            simdEmitOp(compiler, SimdOp::sshr | SimdOp::ImmS4 | (args[1].argw << 16), dst, args[0].arg, 0);
        } else {
            simdEmitOp(compiler, SimdOp::neg | SimdOp::S4, dst, dst, 0);
            simdEmitOp(compiler, SimdOp::sshl | SimdOp::S4, dst, args[0].arg, dst);
        }
        break;
    case ByteCode::I32X4ShrUOpcode:
        if (SLJIT_IS_IMM(args[1].arg)) {
            simdEmitOp(compiler, SimdOp::ushr | SimdOp::ImmS4 | (args[1].argw << 16), dst, args[0].arg, 0);
        } else {
            simdEmitOp(compiler, SimdOp::neg | SimdOp::S4, dst, dst, 0);
            simdEmitOp(compiler, SimdOp::ushl | SimdOp::S4, dst, args[0].arg, dst);
        }
        break;
    case ByteCode::I64X2ShlOpcode:
        if (SLJIT_IS_IMM(args[1].arg)) {
            simdEmitOp(compiler, SimdOp::shl | SimdOp::ImmD2 | (args[1].argw << 16), dst, args[0].arg, 0);
        } else {
            simdEmitOp(compiler, SimdOp::sshl | SimdOp::D2, dst, args[0].arg, dst);
        }
        break;
    case ByteCode::I64X2ShrSOpcode:
        if (SLJIT_IS_IMM(args[1].arg)) {
            simdEmitOp(compiler, SimdOp::sshr | SimdOp::ImmD2 | (args[1].argw << 16), dst, args[0].arg, 0);
        } else {
            simdEmitOp(compiler, SimdOp::neg | SimdOp::D2, dst, dst, 0);
            simdEmitOp(compiler, SimdOp::sshl | SimdOp::D2, dst, args[0].arg, dst);
        }
        break;
    default:
        ASSERT(instr->opcode() == ByteCode::I64X2ShrUOpcode);

        if (SLJIT_IS_IMM(args[1].arg)) {
            simdEmitOp(compiler, SimdOp::ushr | SimdOp::ImmD2 | (args[1].argw << 16), dst, args[0].arg, 0);
        } else {
            simdEmitOp(compiler, SimdOp::neg | SimdOp::D2, dst, dst, 0);
            simdEmitOp(compiler, SimdOp::ushl | SimdOp::D2, dst, args[0].arg, dst);
        }
    }

    if (SLJIT_IS_MEM(args[2].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | type, dst, args[2].arg, args[2].argw);
    }
}
