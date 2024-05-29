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

namespace SimdOp {

#if (defined SLJIT_CONFIG_ARM_THUMB2 && SLJIT_CONFIG_ARM_THUMB2)
constexpr uint32_t unsignedBit = 0b1 << 28;

enum Type : uint32_t {
    it = 0xbf00,
    mov = 0xf04f0000,
    mvn = 0xf06f0000,
    vabs = 0xffb10340,
    vadd = 0xef000840,
    vaddf = 0xee300a00,
    vand = 0xef000150,
    vbic = 0xef100150,
    vbsl = 0xff100150,
    vceq = 0xff000850,
    vcge = 0xef000350,
    vcgt = 0xef000340,
    vcmp = 0xeeb40a40,
    vcvtftf = 0xeeb70ac0,
    vcvtftiSIMD = 0xffb30640,
    vcvtfti = 0xeebc08c0,
    vcvtitfSIMD = 0xffb30640,
    vcvtitf = 0xeeb80840,
    vdivf = 0xee800a00,
    veor = 0xff000150,
    vmlal = 0xef800800,
    vmov = 0xeeb00a40,
    vmovi = 0xef800010,
    vmovl = 0xef800a10,
    vmrs = 0xeef1fa10,
    vmul = 0xef000950,
    vmulf = 0xee200a00,
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
    vsqrt = 0xeeb108c0,
    vshrImm = 0xef800050,
    vsub = 0xff000840,
    vsubf = 0xee300a40,
    vtbl = 0xffb00800,
    vtrn = 0xffB20080,
};

enum CondCodes : uint32_t {
    EQ = 0x0 << 4,
    NE = 0x1 << 4,
    MI = 0x4 << 4,
    VS = 0x6 << 4,
    LS = 0x9 << 4,
    GE = 0xa << 4,
    GT = 0xc << 4,
    Invert = 0x1 << 4
};

#else
constexpr uint32_t unsignedBit = 0b1 << 24;

enum Type : uint32_t {
    mov = 0xe3a00000,
    // The condition flag is added later.
    mvn = 0x03e00000,
    vabs = 0xf3b10340,
    vadd = 0xf2000840,
    vaddf = 0xee300a00,
    vand = 0xf2000150,
    vbic = 0xf2100150,
    vbsl = 0xf3100150,
    vceq = 0xf3000850,
    vcge = 0xf2000350,
    vcgt = 0xf2000340,
    vcmp = 0xeeb40a40,
    vcvtftf = 0xeeb70ac0,
    vcvtftiSIMD = 0xf3b30640,
    vcvtfti = 0xeebc08c0,
    vcvtitfSIMD = 0xf3b30640,
    vcvtitf = 0xeeb80840,
    vdivf = 0xee800a00,
    veor = 0xf3000150,
    vmlal = 0xf2800800,
    vmov = 0xeeb00a40,
    vmovi = 0xf2800010,
    vmovl = 0xf2800a10,
    vmrs = 0xeef1fa10,
    vmul = 0xf2000950,
    vmulf = 0xee200a00,
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
    vsqrt = 0xeeb108c0,
    vshrImm = 0xf2800050,
    vsub = 0xf3000840,
    vsubf = 0xee300a40,
    vtbl = 0xf3b00800,
    vtrn = 0xf3b20080,
};

enum CondCodes : uint32_t {
    AL = static_cast<uint32_t>(0xe) << 28,
    EQ = static_cast<uint32_t>(0x0) << 28,
    NE = static_cast<uint32_t>(0x1) << 28,
    MI = static_cast<uint32_t>(0x4) << 28,
    VS = static_cast<uint32_t>(0x6) << 28,
    LS = static_cast<uint32_t>(0x9) << 28,
    GE = static_cast<uint32_t>(0xa) << 28,
    GT = static_cast<uint32_t>(0xc) << 28,
    Invert = static_cast<uint32_t>(0x1) << 28
};
#endif

constexpr uint8_t sizeOffset = 20;
constexpr uint8_t singleSizeOffset = 18;
constexpr uint32_t lBit = 0b1 << 7;
constexpr uint32_t floatBit = 0b1 << 10;
constexpr uint8_t singleFloatSizeOffset = 8;

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
    S64 = 0x3 << singleSizeOffset,
};

enum FloatSizeType : uint32_t {
    F32 = 0x0 << sizeOffset,
};

enum SingleFloatSizeType : uint32_t {
    SF32 = 0x2 << singleFloatSizeOffset,
    SF64 = 0x3 << singleFloatSizeOffset,
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

void setArgs(Operand* operand, JITArg& arg)
{
    if (VARIABLE_TYPE(operand->ref) == Operand::Immediate) {
        ASSERT(VARIABLE_GET_IMM(operand->ref)->opcode() == ByteCode::Const128Opcode);
        arg.arg = SLJIT_MEM0();
        arg.argw = (sljit_sw) reinterpret_cast<Const128*>(VARIABLE_GET_IMM(operand->ref)->asInstruction()->byteCode())->value();
    } else {
        arg.set(operand);
    }
}

static void F32x4Ceil(float* src, float* dst)
{
    for (int i = 0; i < 4; i++) {
        dst[i] = std::ceil(src[i]);
    }
}

static void F32x4Floor(float* src, float* dst)
{
    for (int i = 0; i < 4; i++) {
        dst[i] = std::floor(src[i]);
    }
}

static void F32x4Trunc(float* src, float* dst)
{
    for (int i = 0; i < 4; i++) {
        dst[i] = std::trunc(src[i]);
    }
}

static void F32x4NearestInt(float* src, float* dst)
{
    for (int i = 0; i < 4; i++) {
        dst[i] = std::nearbyint(src[i]);
    }
}

static void F64x2Ceil(double* src, double* dst)
{
    dst[0] = std::ceil(src[0]);
    dst[1] = std::ceil(src[1]);
}

static void F64x2Floor(double* src, double* dst)
{
    dst[0] = std::floor(src[0]);
    dst[1] = std::floor(src[1]);
}

static void F64x2Trunc(double* src, double* dst)
{
    dst[0] = std::trunc(src[0]);
    dst[1] = std::trunc(src[1]);
}

static void F64x2NearestInt(double* src, double* dst)
{
    dst[0] = std::nearbyint(src[0]);
    dst[1] = std::nearbyint(src[1]);
}

static void F64x2Min(double* src0, double* src1, double* dst)
{
    for (int i = 0; i < 2; i++) {
        if (UNLIKELY(std::isnan(src0[i]) || std::isnan(src1[i]))) {
            dst[i] = std::numeric_limits<double>::quiet_NaN();
        } else if (UNLIKELY(src0[i] == 0 && src1[i] == 0)) {
            dst[i] = std::signbit(src0[i]) ? src0[i] : src1[i];
        } else {
            dst[i] = std::min(src0[i], src1[i]);
        }
    }
}

static void F64x2Max(double* src0, double* src1, double* dst)
{
    for (int i = 0; i < 2; i++) {
        if (UNLIKELY(std::isnan(src0[i]) || std::isnan(src1[i]))) {
            dst[i] = std::numeric_limits<double>::quiet_NaN();
        } else if (UNLIKELY(src0[i] == 0 && src1[i] == 0)) {
            dst[i] = std::signbit(src0[i]) ? src1[i] : src0[i];
        } else {
            dst[i] = std::max(src0[i], src1[i]);
        }
    }
}

static void F64x2PMin(double* src0, double* src1, double* dst)
{
    dst[0] = src1[0] < src0[0] ? src1[0] : src0[0];
    dst[1] = src1[1] < src0[1] ? src1[1] : src0[1];
}

static void F64x2PMax(double* src0, double* src1, double* dst)
{
    dst[0] = src0[0] < src1[0] ? src1[0] : src0[0];
    dst[1] = src0[1] < src1[1] ? src1[1] : src0[1];
}

#define highRegister 0x100

static sljit_s32 getRegister(sljit_s32 reg)
{
    sljit_s32 flags = reg;
    reg = sljit_get_register_index(SLJIT_SIMD_REG_64, reg & 0xff);
    return (flags & highRegister) ? (reg | 0x01) : (reg & 0xfe);
}

static void simdEmitOpAbs(sljit_compiler* compiler, uint32_t opcode, sljit_s32 vd, sljit_s32 vn, sljit_s32 vm)
{
    opcode |= (uint32_t)vm | ((uint32_t)vd << 12) | ((uint32_t)vn << 16);
    sljit_emit_op_custom(compiler, &opcode, sizeof(uint32_t));
}

static void simdEmitOp(sljit_compiler* compiler, uint32_t opcode, sljit_s32 vd, sljit_s32 vn, sljit_s32 vm)
{
    vd = getRegister(vd);
    vm = getRegister(vm);

    if (vn >= SLJIT_FR0) {
        vn = getRegister(vn);
    }

    opcode |= (uint32_t)vm | ((uint32_t)vd << 12) | ((uint32_t)vn << 16);
    sljit_emit_op_custom(compiler, &opcode, sizeof(uint32_t));
}

static void simdEmitOpDst(sljit_compiler* compiler, uint32_t opcode, sljit_s32 vd)
{
    vd = getRegister(vd);

    opcode |= ((uint32_t)vd << 12);
    sljit_emit_op_custom(compiler, &opcode, sizeof(uint32_t));
}

static void simdEmitI64x2Neg(sljit_compiler* compiler, sljit_s32 vd, sljit_s32 vm)
{
    sljit_s32 tmpFReg = SLJIT_TMP_DEST_FREG;

    sljit_emit_simd_replicate(compiler, SLJIT_SIMD_REG_128, tmpFReg, SLJIT_IMM, 0);
    simdEmitOp(compiler, SimdOp::vsub | SimdOp::I64, vd, tmpFReg, vm);
}

static void simdEmitI64x2Eq(sljit_compiler* compiler, sljit_s32 vd, sljit_s32 vn, sljit_s32 vm)
{
    sljit_s32 tmpFReg = SLJIT_TMP_DEST_FREG;

    simdEmitOp(compiler, SimdOp::vceq | SimdOp::I32, vd, vn, vm);
    simdEmitOp(compiler, SimdOp::vrev64 | SimdOp::S32, tmpFReg, 0, vd);
    simdEmitOp(compiler, SimdOp::vand, vd, tmpFReg, vd);
}

static void simdEmitI64x2Ne(sljit_compiler* compiler, sljit_s32 vd, sljit_s32 vn, sljit_s32 vm)
{
    sljit_s32 tmpFReg = SLJIT_TMP_DEST_FREG;

    simdEmitOp(compiler, SimdOp::vceq | SimdOp::I32, vd, vn, vm);
    simdEmitOp(compiler, SimdOp::vrev64 | SimdOp::S32, tmpFReg, 0, vd);
    simdEmitOp(compiler, SimdOp::vmvn | SimdOp::I32, vd, 0, vd);
    simdEmitOp(compiler, SimdOp::vorn, vd, vd, tmpFReg);
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

static void simdEmitF64x2PromoteLow(sljit_compiler* compiler, sljit_s32 vd, sljit_s32 vm, uint32_t operation)
{
    simdEmitOp(compiler, operation | (0x1 << 5), vd | highRegister, 0, vm);
    simdEmitOp(compiler, operation, vd, 0, vm);
}

static void simdEmitTruncSatF64(sljit_compiler* compiler, sljit_s32 vd, sljit_s32 vm, bool isSigned = false)
{
    simdEmitOp(compiler, SimdOp::Type::vcvtfti | (0x3 << 8) | (isSigned << 16), vd, 0, vm);
    simdEmitOp(compiler, SimdOp::Type::vcvtfti | (0x3 << 8) | (0b1 << 22) | (isSigned << 16), vd, 0, vm | highRegister);
    simdEmitOpDst(compiler, SimdOp::Type::vmovi | 0, vd | highRegister);
}

static void simdEmitI32x4Dot(sljit_compiler* compiler, sljit_s32 vd, sljit_s32 vn, sljit_s32 vm)
{
    sljit_s32 tmpFReg = SLJIT_TMP_DEST_FREG;

    simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I16, tmpFReg, vn, vm);
    simdEmitOp(compiler, SimdOp::Type::vpadd | SimdOp::I32, vd, tmpFReg, tmpFReg | highRegister);
    simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I16, tmpFReg, vn | highRegister, vm | highRegister);
    simdEmitOp(compiler, SimdOp::Type::vpadd | SimdOp::I32, vd | highRegister, tmpFReg, tmpFReg | highRegister);
}

static void simdEmitF32x4Unary(sljit_compiler* compiler, uint32_t op, sljit_s32 vd, sljit_s32 vm)
{
    simdEmitOp(compiler, op, vd, 0, vm);
    simdEmitOp(compiler, op | (1 << 22) | (1 << 5), vd, 0, vm);
    simdEmitOp(compiler, op, vd | highRegister, 0, vm | highRegister);
    simdEmitOp(compiler, op | (1 << 22) | (1 << 5), vd | highRegister, 0, vm | highRegister);
}

static void simdEmitF64x2Unary(sljit_compiler* compiler, uint32_t op, sljit_s32 vd, sljit_s32 vm)
{
    simdEmitOp(compiler, op, vd, 0, vm);
    simdEmitOp(compiler, op, vd | highRegister, 0, vm | highRegister);
}

static void simdEmitF64x2Logical(sljit_compiler* compiler, uint32_t op, sljit_s32 vd, sljit_s32 vm)
{
    simdEmitOpDst(compiler, SimdOp::Type::vmovi | (1 << 6) | 1, SLJIT_TMP_DEST_FREG);
    simdEmitOp(compiler, SimdOp::Type::vshlImm | SimdOp::lBit | (63 << 16), SLJIT_TMP_DEST_FREG, 0, SLJIT_TMP_DEST_FREG);
    simdEmitOp(compiler, op, vd, vm, SLJIT_TMP_DEST_FREG);
}

static void simdEmitFloatUnaryOpWithCB(sljit_compiler* compiler, JITArg src, JITArg dst, sljit_sw funcAddr)
{
    if (src.arg == SLJIT_MEM1(kFrameReg)) {
        ASSERT((src.argw & (sizeof(void*) - 1)) == 0);
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R0, 0, kFrameReg, 0, SLJIT_IMM, src.argw);
    } else if (SLJIT_IS_REG(src.arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, src.arg, SLJIT_MEM1(kContextReg), OffsetOfContextField(tmp1));
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R0, 0, kContextReg, 0, SLJIT_IMM, OffsetOfContextField(tmp1));
    } else {
        ASSERT(src.arg == SLJIT_MEM0() && (src.argw & (sizeof(void*) - 1)) == 0);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, src.argw);
    }

    if (SLJIT_IS_REG(dst.arg)) {
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R1, 0, kContextReg, 0, SLJIT_IMM, OffsetOfContextField(tmp1));
    } else {
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R1, 0, kFrameReg, 0, SLJIT_IMM, dst.argw);
    }

    sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS2V(P, P), SLJIT_IMM, funcAddr);

    if (SLJIT_IS_REG(dst.arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, dst.arg, SLJIT_MEM1(kContextReg), OffsetOfContextField(tmp1));
    }
}

static void emitUnarySIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[2];

    sljit_s32 srcType = SLJIT_SIMD_ELEM_128;
    sljit_s32 dstType = SLJIT_SIMD_ELEM_128;
    bool isCallback = false;

    switch (instr->opcode()) {
    case ByteCode::I8X16NegOpcode:
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
    case ByteCode::I16X8ExtaddPairwiseI8X16SOpcode:
    case ByteCode::I16X8ExtaddPairwiseI8X16UOpcode:
        srcType = SLJIT_SIMD_ELEM_16;
        dstType = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I32X4NegOpcode:
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
        srcType = SLJIT_SIMD_ELEM_32 | SLJIT_SIMD_FLOAT;
        dstType = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I32X4TruncSatF64X2SZeroOpcode:
    case ByteCode::I32X4TruncSatF64X2UZeroOpcode:
        srcType = SLJIT_SIMD_ELEM_64 | SLJIT_SIMD_FLOAT;
        dstType = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I64X2NegOpcode:
        srcType = SLJIT_SIMD_ELEM_64;
        dstType = SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::F32X4CeilOpcode:
    case ByteCode::F32X4FloorOpcode:
    case ByteCode::F32X4TruncOpcode:
    case ByteCode::F32X4NearestOpcode:
        isCallback = true;
        FALLTHROUGH;
    case ByteCode::F32X4AbsOpcode:
    case ByteCode::F32X4NegOpcode:
    case ByteCode::F32X4SqrtOpcode:
    case ByteCode::F64X2PromoteLowF32X4Opcode:
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
    case ByteCode::F64X2CeilOpcode:
    case ByteCode::F64X2FloorOpcode:
    case ByteCode::F64X2TruncOpcode:
    case ByteCode::F64X2NearestOpcode:
        isCallback = true;
        FALLTHROUGH;
    case ByteCode::F64X2AbsOpcode:
    case ByteCode::F64X2NegOpcode:
    case ByteCode::F64X2SqrtOpcode:
        srcType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64;
        dstType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::F64X2ConvertLowI32X4SOpcode:
    case ByteCode::F64X2ConvertLowI32X4UOpcode:
        srcType = SLJIT_SIMD_ELEM_32;
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

    sljit_s32 dst;

    if (!isCallback) {
        simdOperandToArg(compiler, operands, args[0], srcType, instr->requiredReg(0));

        args[1].set(operands + 1);
        dst = GET_TARGET_REG(args[1].arg, instr->requiredReg(0));
    } else {
        for (uint8_t i = 0; i < 2; i++) {
            setArgs(operands + i, args[i]);
        }
    }

    switch (instr->opcode()) {
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
        simdEmitOp(compiler, SimdOp::Type::vcvtftf | (0x1 << 8) | (0x1 << 22), dst, 0, args[0].arg | highRegister);
        simdEmitOpDst(compiler, SimdOp::Type::vmovi | 0, dst | highRegister);
        break;
    case ByteCode::I8X16NegOpcode:
        simdEmitOp(compiler, SimdOp::Type::vneg | SimdOp::S8, dst, 0, args[0].arg);
        break;
    case ByteCode::I16X8ExtendLowI8X16SOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmovl | SimdOp::immS8, dst, 0, args[0].arg);
        break;
    case ByteCode::I16X8ExtendHighI8X16SOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmovl | SimdOp::immS8, dst, 0, args[0].arg | highRegister);
        break;
    case ByteCode::I16X8ExtendLowI8X16UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmovl | SimdOp::immS8 | SimdOp::unsignedBit, dst, 0, args[0].arg);
        break;
    case ByteCode::I16X8ExtendHighI8X16UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmovl | SimdOp::immS8 | SimdOp::unsignedBit, dst, 0, args[0].arg | highRegister);
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
    case ByteCode::I32X4ExtendLowI16X8SOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmovl | SimdOp::immS16, dst, 0, args[0].arg);
        break;
    case ByteCode::I32X4ExtendHighI16X8SOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmovl | SimdOp::immS16, dst, 0, args[0].arg | highRegister);
        break;
    case ByteCode::I32X4ExtendLowI16X8UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmovl | SimdOp::immS16 | SimdOp::unsignedBit, dst, 0, args[0].arg);
        break;
    case ByteCode::I32X4ExtendHighI16X8UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmovl | SimdOp::immS16 | SimdOp::unsignedBit, dst, 0, args[0].arg | highRegister);
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
    case ByteCode::V128NotOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmvn, dst, 0, args[0].arg);
        break;
    case ByteCode::F32X4AbsOpcode:
        simdEmitOp(compiler, SimdOp::Type::vabs | SimdOp::S32 | SimdOp::floatBit, dst, 0, args[0].arg);
        break;
    case ByteCode::F32X4SqrtOpcode:
        simdEmitF32x4Unary(compiler, SimdOp::Type::vsqrt | SimdOp::SF32, dst, args[0].arg);
        break;
    case ByteCode::F32X4NegOpcode:
        simdEmitOp(compiler, SimdOp::Type::vneg | SimdOp::S32 | SimdOp::floatBit, dst, 0, args[0].arg);
        break;
    case ByteCode::F32X4CeilOpcode:
        ASSERT(isCallback);
        simdEmitFloatUnaryOpWithCB(compiler, args[0], args[1], GET_FUNC_ADDR(sljit_sw, F32x4Ceil));
        break;
    case ByteCode::F32X4FloorOpcode:
        ASSERT(isCallback);
        simdEmitFloatUnaryOpWithCB(compiler, args[0], args[1], GET_FUNC_ADDR(sljit_sw, F32x4Floor));
        break;
    case ByteCode::F32X4TruncOpcode:
        ASSERT(isCallback);
        simdEmitFloatUnaryOpWithCB(compiler, args[0], args[1], GET_FUNC_ADDR(sljit_sw, F32x4Trunc));
        break;
    case ByteCode::F32X4NearestOpcode:
        ASSERT(isCallback);
        simdEmitFloatUnaryOpWithCB(compiler, args[0], args[1], GET_FUNC_ADDR(sljit_sw, F32x4NearestInt));
        break;
    case ByteCode::F64X2AbsOpcode:
        simdEmitF64x2Logical(compiler, SimdOp::Type::vbic, dst, args[0].arg);
        break;
    case ByteCode::F64X2SqrtOpcode:
        simdEmitF64x2Unary(compiler, SimdOp::Type::vsqrt | SimdOp::SF64, dst, args[0].arg);
        break;
    case ByteCode::F64X2NegOpcode:
        simdEmitF64x2Logical(compiler, SimdOp::Type::veor, dst, args[0].arg);
        break;
    case ByteCode::F64X2CeilOpcode:
        ASSERT(isCallback);
        simdEmitFloatUnaryOpWithCB(compiler, args[0], args[1], GET_FUNC_ADDR(sljit_sw, F64x2Ceil));
        break;
    case ByteCode::F64X2FloorOpcode:
        ASSERT(isCallback);
        simdEmitFloatUnaryOpWithCB(compiler, args[0], args[1], GET_FUNC_ADDR(sljit_sw, F64x2Floor));
        break;
    case ByteCode::F64X2TruncOpcode:
        ASSERT(isCallback);
        simdEmitFloatUnaryOpWithCB(compiler, args[0], args[1], GET_FUNC_ADDR(sljit_sw, F64x2Trunc));
        break;
    case ByteCode::F64X2NearestOpcode:
        ASSERT(isCallback);
        simdEmitFloatUnaryOpWithCB(compiler, args[0], args[1], GET_FUNC_ADDR(sljit_sw, F64x2NearestInt));
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    if (SLJIT_IS_MEM(args[1].arg) && !isCallback) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | dstType, dst, args[1].arg, args[1].argw);
    }
}

static bool emitUnaryCondSIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[2];

    sljit_s32 srcType = SLJIT_SIMD_ELEM_128;
    uint32_t opcode;
    int repeat = 1;

    switch (instr->opcode()) {
    case ByteCode::I8X16AllTrueOpcode:
        srcType = SLJIT_SIMD_ELEM_8;
        opcode = SimdOp::vpmin | SimdOp::I8 | SimdOp::unsignedBit;
        repeat = 3;
        break;
    case ByteCode::I16X8AllTrueOpcode:
        srcType = SLJIT_SIMD_ELEM_16;
        opcode = SimdOp::vpmin | SimdOp::I16 | SimdOp::unsignedBit;
        repeat = 2;
        break;
    case ByteCode::I32X4AllTrueOpcode:
        srcType = SLJIT_SIMD_ELEM_32;
        opcode = SimdOp::vpmin | SimdOp::I32 | SimdOp::unsignedBit;
        break;
    case ByteCode::I64X2AllTrueOpcode:
        srcType = SLJIT_SIMD_ELEM_64;
        opcode = SimdOp::vpmax | SimdOp::I32 | SimdOp::unsignedBit;
        break;
    default:
        ASSERT(instr->opcode() == ByteCode::V128AnyTrueOpcode);
        srcType = SLJIT_SIMD_ELEM_32;
        opcode = SimdOp::vpmax | SimdOp::I32 | SimdOp::unsignedBit;
        break;
    }

    simdOperandToArg(compiler, operands, args[0], srcType, SLJIT_TMP_DEST_FREG);

    sljit_s32 dst = SLJIT_TMP_DEST_REG;
    sljit_s32 tmpFReg = SLJIT_TMP_DEST_FREG;

    if (!(instr->info() & Instruction::kIsMergeCompare)) {
        args[1].set(operands + 1);
        dst = GET_TARGET_REG(args[1].arg, SLJIT_TMP_DEST_REG);
    }

    simdEmitOp(compiler, opcode, tmpFReg, args[0].arg, args[0].arg | highRegister);

    if (srcType == SLJIT_SIMD_ELEM_64) {
        opcode = SimdOp::vpmin | SimdOp::I32 | SimdOp::unsignedBit;
        srcType = SLJIT_SIMD_ELEM_32;
    }

    do {
        simdEmitOp(compiler, opcode, tmpFReg, tmpFReg, tmpFReg);
    } while (--repeat != 0);

    sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_LANE_SIGNED | SLJIT_32 | srcType, tmpFReg, 0, dst, 0);
    sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_Z, dst, 0, SLJIT_IMM, 0);

    ASSERT(instr->next() != nullptr);

    if (instr->info() & Instruction::kIsMergeCompare) {
        Instruction* nextInstr = instr->next()->asInstruction();

        if (nextInstr->opcode() == ByteCode::SelectOpcode) {
            emitSelect(compiler, nextInstr, SLJIT_NOT_EQUAL);
            return true;
        }

        ASSERT(nextInstr->opcode() == ByteCode::JumpIfTrueOpcode || nextInstr->opcode() == ByteCode::JumpIfFalseOpcode);

        sljit_s32 type = SLJIT_NOT_EQUAL;

        if (nextInstr->opcode() == ByteCode::JumpIfFalseOpcode) {
            type ^= 0x1;
        }

        nextInstr->asExtended()->value().targetLabel->jumpFrom(sljit_emit_jump(compiler, type));
        return true;
    }

    sljit_emit_op_flags(compiler, SLJIT_MOV, dst, 0, SLJIT_NOT_EQUAL);

    if (SLJIT_IS_MEM(args[1].arg)) {
        sljit_emit_op1(compiler, SLJIT_MOV32, args[1].arg, args[1].argw, dst, 0);
    }

    return false;
}

static void simdEmitI64x2Mul(sljit_compiler* compiler, sljit_s32 vd, sljit_s32 vn, sljit_s32 vm)
{
    sljit_s32 tmpFReg1 = SLJIT_TMP_DEST_FREG;

    // Swap arguments.
    if (vd == vm) {
        vm = vn;
        vn = vd;
    }

    sljit_emit_simd_mov(compiler, SLJIT_SIMD_REG_128, tmpFReg1, vn, 0);

    // Swaps two 32 bit values: [0][1] [2][3] -> [0][2] [1][3]
    simdEmitOp(compiler, SimdOp::vtrn | SimdOp::S32, vm, 0, vm | highRegister);
    simdEmitOp(compiler, SimdOp::vtrn | SimdOp::S32, tmpFReg1, 0, tmpFReg1 | highRegister);
    simdEmitOp(compiler, SimdOp::vmull | SimdOp::I32 | SimdOp::unsignedBit, vd, tmpFReg1, vm | highRegister);
    simdEmitOp(compiler, SimdOp::vmlal | SimdOp::I32 | SimdOp::unsignedBit, vd, tmpFReg1 | highRegister, vm);
    simdEmitOp(compiler, SimdOp::vshlImm | SimdOp::SHL32 | SimdOp::lBit, vd, 0, vd);
    simdEmitOp(compiler, SimdOp::vmlal | SimdOp::I32 | SimdOp::unsignedBit, vd, tmpFReg1, vm);

    // Revert vm. TODO: This might be unnecessary.
    simdEmitOp(compiler, SimdOp::vtrn | SimdOp::S32, vm, 0, vm | highRegister);
}

static void simdEmitF32x4Binary(sljit_compiler* compiler, uint32_t op, sljit_s32 vd, sljit_s32 vn, sljit_s32 vm)
{
    simdEmitOp(compiler, op, vd, vn, vm);
    simdEmitOp(compiler, op | (1 << 22) | (1 << 7) | (1 << 5), vd, vn, vm);
    simdEmitOp(compiler, op, vd | highRegister, vn | highRegister, vm | highRegister);
    simdEmitOp(compiler, op | (1 << 22) | (1 << 7) | (1 << 5), vd | highRegister, vn | highRegister, vm | highRegister);
}

static void simdEmitF64x2Binary(sljit_compiler* compiler, uint32_t op, sljit_s32 vd, sljit_s32 vn, sljit_s32 vm)
{
    simdEmitOp(compiler, op, vd, vn, vm);
    simdEmitOp(compiler, op, vd | highRegister, vn | highRegister, vm | highRegister);
}

static void simdEmitF32x4PMinMax(sljit_compiler* compiler, uint32_t type, sljit_s32 vd, sljit_s32 vn, sljit_s32 vm)
{
    sljit_u32 opcode;

    // Swap arguments.
    if (vd == vm) {
        vm = vn;
        vn = vd;
        type ^= SimdOp::CondCodes::Invert;
    }

    for (int i = 0; i < 4; i++) {
        if (i == 2) {
            vd |= highRegister;
            vn |= highRegister;
            vm |= highRegister;
        }

        uint32_t flags = (i & 0x1) != 0 ? ((1 << 22) | (1 << 5)) : 0;
        simdEmitOp(compiler, SimdOp::Type::vcmp | flags, vn, 0, vm);

        opcode = SimdOp::Type::vmrs;
        sljit_emit_op_custom(compiler, &opcode, sizeof(uint32_t));

        if (vd != vn) {
            simdEmitOp(compiler, SimdOp::Type::vmov | flags, vd, 0, vn);
        }

#if (defined SLJIT_CONFIG_ARM_THUMB2 && SLJIT_CONFIG_ARM_THUMB2)
        opcode = SimdOp::Type::it | type | 0x8;
        sljit_emit_op_custom(compiler, &opcode, sizeof(uint16_t));
        simdEmitOp(compiler, SimdOp::Type::vmov | flags, vd, 0, vm);
#else /* SLJIT_CONFIG_ARM_THUMB2 */
        simdEmitOp(compiler, (SimdOp::Type::vmov - SimdOp::CondCodes::AL) | type | flags, vd, 0, vm);
#endif /* !SLJIT_CONFIG_ARM_THUMB2 */
    }
}

static void simdEmitF64x2PMinMax(sljit_compiler* compiler, uint32_t type, sljit_s32 vd, sljit_s32 vn, sljit_s32 vm)
{
    sljit_u32 opcode;

    // Swap arguments.
    if (vd == vm) {
        vm = vn;
        vn = vd;
        type ^= SimdOp::CondCodes::Invert;
    }

    for (int i = 0; i < 2; i++) {
        if (i == 1) {
            vd |= highRegister;
            vn |= highRegister;
            vm |= highRegister;
        }

        simdEmitOp(compiler, SimdOp::Type::vcmp | SimdOp::SF64, vn, 0, vm);

        opcode = SimdOp::Type::vmrs;
        sljit_emit_op_custom(compiler, &opcode, sizeof(uint32_t));

        if (vd != vn) {
            simdEmitOp(compiler, SimdOp::Type::vmov | SimdOp::SF64, vd, 0, vn);
        }

#if (defined SLJIT_CONFIG_ARM_THUMB2 && SLJIT_CONFIG_ARM_THUMB2)
        opcode = SimdOp::Type::it | type | 0x8;
        sljit_emit_op_custom(compiler, &opcode, sizeof(uint16_t));
        simdEmitOp(compiler, SimdOp::Type::vmov | SimdOp::SF64, vd, 0, vm);
#else /* SLJIT_CONFIG_ARM_THUMB2 */
        simdEmitOp(compiler, (SimdOp::Type::vmov - SimdOp::CondCodes::AL) | type | SimdOp::SF64, vd, 0, vm);
#endif /* !SLJIT_CONFIG_ARM_THUMB2 */
    }
}

static void F32x4Min(float* src0, float* src1, float* dst)
{
    for (int i = 0; i < 4; i++) {
        if (UNLIKELY(std::isnan(src0[i]) || std::isnan(src1[i]))) {
            dst[i] = std::numeric_limits<float>::quiet_NaN();
        } else if (UNLIKELY(src0[i] == 0 && src1[i] == 0)) {
            dst[i] = std::signbit(src0[i]) ? src0[i] : src1[i];
        } else {
            dst[i] = std::min(src0[i], src1[i]);
        }
    }
}

static void F32x4Max(float* src0, float* src1, float* dst)
{
    for (int i = 0; i < 4; i++) {
        if (UNLIKELY(std::isnan(src0[i]) || std::isnan(src1[i]))) {
            dst[i] = std::numeric_limits<float>::quiet_NaN();
        } else if (UNLIKELY(src0[i] == 0 && src1[i] == 0)) {
            dst[i] = std::signbit(src0[i]) ? src1[i] : src0[i];
        } else {
            dst[i] = std::max(src0[i], src1[i]);
        }
    }
}

static void simdEmitFloatBinaryOpWithCB(sljit_compiler* compiler, JITArg src[2], JITArg dst, sljit_sw funcAddr)
{
    if (src[0].arg == SLJIT_MEM1(kFrameReg)) {
        ASSERT((src[0].argw & (sizeof(void*) - 1)) == 0);
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R0, 0, kFrameReg, 0, SLJIT_IMM, src[0].argw);
    } else if (SLJIT_IS_REG(src[0].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, src[0].arg, SLJIT_MEM1(kContextReg), OffsetOfContextField(tmp1));
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R0, 0, kContextReg, 0, SLJIT_IMM, OffsetOfContextField(tmp1));
    } else {
        ASSERT(src[0].arg == SLJIT_MEM0() && (src[0].argw & (sizeof(void*) - 1)) == 0);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, src[0].argw);
    }

    if (src[1].arg == SLJIT_MEM1(kFrameReg)) {
        ASSERT((src[1].argw & (sizeof(void*) - 1)) == 0);
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R1, 0, kFrameReg, 0, SLJIT_IMM, src[1].argw);
    } else if (SLJIT_IS_REG(src[1].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, src[1].arg, SLJIT_MEM1(kContextReg), OffsetOfContextField(tmp2));
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R1, 0, kContextReg, 0, SLJIT_IMM, OffsetOfContextField(tmp2));
    } else {
        ASSERT(src[1].arg == SLJIT_MEM0() && (src[1].argw & (sizeof(void*) - 1)) == 0);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_IMM, src[1].argw);
    }

    if (SLJIT_IS_REG(dst.arg)) {
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R2, 0, kContextReg, 0, SLJIT_IMM, OffsetOfContextField(tmp1));
    } else {
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R2, 0, kFrameReg, 0, SLJIT_IMM, dst.argw);
    }

    sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS3V(P, P, P), SLJIT_IMM, funcAddr);

    if (SLJIT_IS_REG(dst.arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, dst.arg, SLJIT_MEM1(kContextReg), OffsetOfContextField(tmp1));
    }
}

static void simdEmitSetCompareResult(sljit_compiler* compiler, uint32_t type, sljit_u32 reg)
{
    sljit_u32 opcode;

    opcode = SimdOp::Type::vmrs;
    sljit_emit_op_custom(compiler, &opcode, sizeof(uint32_t));
#if (defined SLJIT_CONFIG_ARM_THUMB2 && SLJIT_CONFIG_ARM_THUMB2)
    opcode = SimdOp::Type::mov | (reg << 8);
    sljit_emit_op_custom(compiler, &opcode, sizeof(uint32_t));
    opcode = SimdOp::Type::it | type | 0x8;
    sljit_emit_op_custom(compiler, &opcode, sizeof(uint16_t));
    opcode = SimdOp::Type::mvn | (reg << 8);
    sljit_emit_op_custom(compiler, &opcode, sizeof(uint32_t));
#else /* SLJIT_CONFIG_ARM_THUMB2 */
    opcode = SimdOp::Type::mov | (reg << 12);
    sljit_emit_op_custom(compiler, &opcode, sizeof(uint32_t));
    opcode = SimdOp::Type::mvn | (reg << 12) | type;
    sljit_emit_op_custom(compiler, &opcode, sizeof(uint32_t));
#endif /* !SLJIT_CONFIG_ARM_THUMB2 */
}

static void simdEmitF32x4Compare(sljit_compiler* compiler, uint32_t type, sljit_s32 vd, sljit_s32 vn, sljit_s32 vm)
{
    sljit_u32 reg = sljit_get_register_index(SLJIT_GP_REGISTER, SLJIT_TMP_DEST_REG);

    for (int i = 0; i < 4; i++) {
        if (i == 2) {
            vd |= highRegister;
            vn |= highRegister;
            vm |= highRegister;
        }

        uint32_t flags = (i & 0x1) != 0 ? ((1 << 22) | (1 << 5)) : 0;
        simdEmitOp(compiler, SimdOp::Type::vcmp | flags, vn, 0, vm);
        simdEmitSetCompareResult(compiler, type, reg);

        // VMOV between an arm core register.
        sljit_u32 opcode = 0xee000a10 | ((i & 0x1) != 0 ? (1 << 7) : 0) | (reg << 12) | (getRegister(vd) << 16);
        sljit_emit_op_custom(compiler, &opcode, sizeof(uint32_t));
    }
}

static void simdEmitF64x2Compare(sljit_compiler* compiler, uint32_t type, sljit_s32 vd, sljit_s32 vn, sljit_s32 vm)
{
    sljit_u32 reg = sljit_get_register_index(SLJIT_GP_REGISTER, SLJIT_TMP_DEST_REG);

    for (int i = 0; i < 2; i++) {
        if (i == 1) {
            vd |= highRegister;
            vn |= highRegister;
            vm |= highRegister;
        }

        simdEmitOp(compiler, SimdOp::Type::vcmp | SimdOp::SF64, vn, 0, vm);
        simdEmitSetCompareResult(compiler, type, reg);

        // VMOV between two arm core registers.
        sljit_u32 opcode = 0xec400b10 | (reg << 16) | (reg << 12) | getRegister(vd);
        sljit_emit_op_custom(compiler, &opcode, sizeof(uint32_t));
    }
}

static void emitBinarySIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[3];

    sljit_s32 srcType = SLJIT_SIMD_ELEM_128;
    sljit_s32 dstType = SLJIT_SIMD_ELEM_128;
    bool isCallback = false;

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
    case ByteCode::I8X16SwizzleOpcode:
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
    case ByteCode::I16X8Q15mulrSatSOpcode:
        srcType = SLJIT_SIMD_ELEM_16;
        dstType = SLJIT_SIMD_ELEM_16;
        break;
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
    case ByteCode::F32X4MinOpcode:
    case ByteCode::F32X4MaxOpcode:
        isCallback = true;
        FALLTHROUGH;
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
        srcType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32;
        dstType = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::F64X2MinOpcode:
    case ByteCode::F64X2MaxOpcode:
        isCallback = true;
        FALLTHROUGH;
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

    sljit_s32 dst;

    ASSERT((srcType & SLJIT_SIMD_FLOAT) == (dstType & SLJIT_SIMD_FLOAT));

    if (!isCallback) {
        simdOperandToArg(compiler, operands, args[0], srcType, instr->requiredReg(0));
        simdOperandToArg(compiler, operands + 1, args[1], srcType, instr->requiredReg(1));

        args[2].set(operands + 2);
        dst = GET_TARGET_REG(args[2].arg, instr->requiredReg(instr->opcode() == ByteCode::I8X16SwizzleOpcode ? 1 : 2));
    } else {
        for (uint8_t i = 0; i < 3; i++) {
            setArgs(operands + i, args[i]);
        }
    }

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
        simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I8, dst, args[0].arg | highRegister, args[1].arg | highRegister);
        break;
    case ByteCode::I16X8ExtmulLowI8X16UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I8 | SimdOp::unsignedBit, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I16X8ExtmulHighI8X16UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I8 | SimdOp::unsignedBit, dst, args[0].arg | highRegister, args[1].arg | highRegister);
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
        simdEmitOp(compiler, SimdOp::Type::vqmovn | SimdOp::S8 | (0x1 << 7), dst | highRegister, 0, args[1].arg);
        break;
    case ByteCode::I8X16NarrowI16X8UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vqmovn | SimdOp::S8 | (0x1 << 6), dst, 0, args[0].arg);
        simdEmitOp(compiler, SimdOp::Type::vqmovn | SimdOp::S8 | (0x1 << 6), dst | highRegister, 0, args[1].arg);
        break;
    case ByteCode::I32X4ExtmulLowI16X8SOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I16, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4ExtmulHighI16X8SOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I16, dst, args[0].arg | highRegister, args[1].arg | highRegister);
        break;
    case ByteCode::I32X4ExtmulLowI16X8UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I16 | SimdOp::unsignedBit, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4ExtmulHighI16X8UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I16 | SimdOp::unsignedBit, dst, args[0].arg | highRegister, args[1].arg | highRegister);
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
        simdEmitOp(compiler, SimdOp::Type::vqmovn | SimdOp::S16 | (0x1 << 7), dst | highRegister, 0, args[1].arg);
        break;
    case ByteCode::I16X8NarrowI32X4UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vqmovn | SimdOp::S16 | (0x1 << 6), dst, 0, args[0].arg);
        simdEmitOp(compiler, SimdOp::Type::vqmovn | SimdOp::S16 | (0x1 << 6), dst | highRegister, 0, args[1].arg);
        break;
    case ByteCode::I64X2ExtmulLowI32X4SOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I32, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2ExtmulHighI32X4SOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I32, dst, args[0].arg | highRegister, args[1].arg | highRegister);
        break;
    case ByteCode::I64X2ExtmulLowI32X4UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I32 | SimdOp::unsignedBit, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2ExtmulHighI32X4UOpcode:
        simdEmitOp(compiler, SimdOp::Type::vmull | SimdOp::I32 | SimdOp::unsignedBit, dst, args[0].arg | highRegister, args[1].arg | highRegister);
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
        ASSERT(dst != args[0].arg);
        simdEmitOp(compiler, SimdOp::Type::vtbl | (0b1 << 8), dst, args[0].arg, args[1].arg);
        simdEmitOp(compiler, SimdOp::Type::vtbl | (0b1 << 8), dst | highRegister, args[0].arg, args[1].arg | highRegister);
        break;
    case ByteCode::F32X4EqOpcode:
        simdEmitF32x4Compare(compiler, SimdOp::CondCodes::EQ, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4NeOpcode:
        simdEmitF32x4Compare(compiler, SimdOp::CondCodes::NE, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4LtOpcode:
        simdEmitF32x4Compare(compiler, SimdOp::CondCodes::MI, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4LeOpcode:
        simdEmitF32x4Compare(compiler, SimdOp::CondCodes::LS, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4GtOpcode:
        simdEmitF32x4Compare(compiler, SimdOp::CondCodes::GT, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4GeOpcode:
        simdEmitF32x4Compare(compiler, SimdOp::CondCodes::GE, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4AddOpcode:
        simdEmitF32x4Binary(compiler, SimdOp::Type::vaddf, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4SubOpcode:
        simdEmitF32x4Binary(compiler, SimdOp::Type::vsubf, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4MulOpcode:
        simdEmitF32x4Binary(compiler, SimdOp::Type::vmulf, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4DivOpcode:
        simdEmitF32x4Binary(compiler, SimdOp::Type::vdivf, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4MinOpcode:
        ASSERT(isCallback);
        simdEmitFloatBinaryOpWithCB(compiler, args, args[2], GET_FUNC_ADDR(sljit_sw, F32x4Min));
        break;
    case ByteCode::F32X4MaxOpcode:
        ASSERT(isCallback);
        simdEmitFloatBinaryOpWithCB(compiler, args, args[2], GET_FUNC_ADDR(sljit_sw, F32x4Max));
        break;
    case ByteCode::F32X4PMinOpcode:
        simdEmitF32x4PMinMax(compiler, SimdOp::CondCodes::GT, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F32X4PMaxOpcode:
        simdEmitF32x4PMinMax(compiler, SimdOp::CondCodes::MI, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2EqOpcode:
        simdEmitF64x2Compare(compiler, SimdOp::CondCodes::EQ, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2NeOpcode:
        simdEmitF64x2Compare(compiler, SimdOp::CondCodes::NE, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2LtOpcode:
        simdEmitF64x2Compare(compiler, SimdOp::CondCodes::MI, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2LeOpcode:
        simdEmitF64x2Compare(compiler, SimdOp::CondCodes::LS, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2GtOpcode:
        simdEmitF64x2Compare(compiler, SimdOp::CondCodes::GT, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2GeOpcode:
        simdEmitF64x2Compare(compiler, SimdOp::CondCodes::GE, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2AddOpcode:
        simdEmitF64x2Binary(compiler, SimdOp::Type::vaddf | SimdOp::SF64, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2SubOpcode:
        simdEmitF64x2Binary(compiler, SimdOp::Type::vsubf | SimdOp::SF64, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2MulOpcode:
        simdEmitF64x2Binary(compiler, SimdOp::Type::vmulf | SimdOp::SF64, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2DivOpcode:
        simdEmitF64x2Binary(compiler, SimdOp::Type::vdivf | SimdOp::SF64, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2MinOpcode:
        ASSERT(isCallback);
        simdEmitFloatBinaryOpWithCB(compiler, args, args[2], GET_FUNC_ADDR(sljit_sw, F64x2Min));
        break;
    case ByteCode::F64X2MaxOpcode:
        ASSERT(isCallback);
        simdEmitFloatBinaryOpWithCB(compiler, args, args[2], GET_FUNC_ADDR(sljit_sw, F64x2Max));
        break;
    case ByteCode::F64X2PMinOpcode:
        simdEmitF64x2PMinMax(compiler, SimdOp::CondCodes::GT, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2PMaxOpcode:
        simdEmitF64x2PMinMax(compiler, SimdOp::CondCodes::MI, dst, args[0].arg, args[1].arg);
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    if (SLJIT_IS_MEM(args[2].arg) && !isCallback) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | dstType, dst, args[2].arg, args[2].argw);
    }
}

static void emitSelectSIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[3];

    simdOperandToArg(compiler, operands, args[0], SLJIT_SIMD_ELEM_128, instr->requiredReg(0));
    simdOperandToArg(compiler, operands + 1, args[1], SLJIT_SIMD_ELEM_128, instr->requiredReg(1));
    simdOperandToArg(compiler, operands + 2, args[2], SLJIT_SIMD_ELEM_128, instr->requiredReg(2));

    sljit_s32 dst = instr->requiredReg(2);

    if (dst != args[2].arg) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, dst, args[2].arg, args[2].argw);
    }

    simdEmitOp(compiler, SimdOp::Type::vbsl, dst, args[0].arg, args[1].arg);

    args[2].set(operands + 3);
    if (SLJIT_IS_MEM(args[2].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, dst, args[2].arg, args[2].argw);
    }
}

static void emitShuffleSIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[2];

    sljit_s32 dstReg = instr->requiredReg(2);

    simdOperandToArg(compiler, operands, args[0], SLJIT_SIMD_ELEM_128, instr->requiredReg(0));
    simdOperandToArg(compiler, operands + 1, args[1], SLJIT_SIMD_ELEM_128, instr->requiredReg(1));

    // Directly generate the code sequence to avoid saved register and register remapping related issues.
    sljit_u32 src1 = sljit_get_register_index(SLJIT_SIMD_REG_64, instr->requiredReg(0)) & ~static_cast<sljit_u32>(0x1);
    sljit_u32 src2 = sljit_get_register_index(SLJIT_SIMD_REG_64, instr->requiredReg(1)) & ~static_cast<sljit_u32>(0x1);
    sljit_u32 dst = sljit_get_register_index(SLJIT_SIMD_REG_64, dstReg) & ~static_cast<sljit_u32>(0x1);
    sljit_u32 tmp = sljit_get_register_index(SLJIT_SIMD_REG_64, SLJIT_TMP_DEST_FREG) & ~static_cast<sljit_u32>(0x1);

    sljit_u32 restoreReg = VariableList::kUnusedReg;
    const sljit_u32 highestReg = 14;
    bool restoreFR0 = false;

    if (src1 + 2 != src2) {
        if (src1 + 2 == dst) {
            simdEmitOpAbs(compiler, SimdOp::Type::vorr, dst, src2, src2);
            dst = tmp;
            dstReg = SLJIT_TMP_DEST_FREG;
        } else if (src1 + 2 == tmp) {
            simdEmitOpAbs(compiler, SimdOp::Type::vorr, tmp, src2, src2);
        } else if (src2 - 2 == dst) {
            simdEmitOpAbs(compiler, SimdOp::Type::vorr, dst, src1, src1);
            src1 = dst;
            dst = tmp;
            dstReg = SLJIT_TMP_DEST_FREG;
        } else if (src2 - 2 == tmp) {
            simdEmitOpAbs(compiler, SimdOp::Type::vorr, tmp, src1, src1);
            src1 = tmp;
        } else if (src1 < highestReg) {
            restoreReg = src1 + 2;
            simdEmitOpAbs(compiler, SimdOp::Type::vorr, tmp, restoreReg, restoreReg);
            simdEmitOpAbs(compiler, SimdOp::Type::vorr, restoreReg, src2, src2);
        } else if (src2 > 0) {
            restoreReg = src2 - 2;
            simdEmitOpAbs(compiler, SimdOp::Type::vorr, tmp, restoreReg, restoreReg);
            simdEmitOpAbs(compiler, SimdOp::Type::vorr, restoreReg, highestReg, highestReg);
            src1 = restoreReg;
        } else {
            restoreReg = 2;
            simdEmitOpAbs(compiler, SimdOp::Type::vorr, tmp, restoreReg, restoreReg);
            simdEmitOpAbs(compiler, SimdOp::Type::vorr, restoreReg, 0, 0);
            simdEmitOpAbs(compiler, SimdOp::Type::vorr, 0, highestReg, highestReg);
            src1 = 0;
            restoreFR0 = true;
        }
    }

    I8X16Shuffle* shuffle = reinterpret_cast<I8X16Shuffle*>(instr->byteCode());
    const sljit_s32 type = SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8;
    sljit_emit_simd_mov(compiler, type, dstReg, SLJIT_MEM0(), reinterpret_cast<sljit_sw>(shuffle->value()));

    simdEmitOpAbs(compiler, SimdOp::vtbl | (0b11 << 8), dst, src1, dst);
    dst++;
    simdEmitOpAbs(compiler, SimdOp::vtbl | (0b11 << 8), dst, src1, dst);

    if (restoreFR0) {
        simdEmitOpAbs(compiler, SimdOp::Type::vorr, 0, 2, 2);
    }

    if (restoreReg != VariableList::kUnusedReg) {
        simdEmitOpAbs(compiler, SimdOp::Type::vorr, restoreReg, tmp, tmp);
    }

    args[0].set(operands + 2);

    if (args[0].arg != dstReg && args[0].arg != dstReg + 1) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, dstReg, args[0].arg, args[0].argw);
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
        immOp = SimdOp::Type::vshlImm | SimdOp::SHL8;
        op = SimdOp::Type::vshl | SimdOp::I8;
        mask = SimdOp::shift8;
        type = SLJIT_SIMD_ELEM_8;
        break;
    case ByteCode::I8X16ShrUOpcode:
        immOp = (0b1 << 24);
        op = SimdOp::unsignedBit;
        FALLTHROUGH;
    case ByteCode::I8X16ShrSOpcode:
        mask = SimdOp::shift8;
        type = SLJIT_SIMD_ELEM_8;
        immOp |= SimdOp::Type::vshrImm | SimdOp::SHL8;
        op |= SimdOp::Type::vshl | SimdOp::I8;
        break;
    case ByteCode::I16X8ShlOpcode:
        isShr = false;
        immOp = SimdOp::Type::vshlImm | SimdOp::SHL16;
        op = SimdOp::Type::vshl | SimdOp::I16;
        mask = SimdOp::shift16;
        type = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I16X8ShrUOpcode:
        immOp = (0b1 << 24);
        op = SimdOp::unsignedBit;
        FALLTHROUGH;
    case ByteCode::I16X8ShrSOpcode:
        mask = SimdOp::shift16;
        type = SLJIT_SIMD_ELEM_16;
        immOp |= SimdOp::Type::vshrImm | SimdOp::SHL16;
        op |= SimdOp::Type::vshl | SimdOp::I16;
        break;
    case ByteCode::I32X4ShlOpcode:
        isShr = false;
        immOp = SimdOp::Type::vshlImm | SimdOp::SHL32;
        op = SimdOp::Type::vshl | SimdOp::I32;
        mask = SimdOp::shift32;
        type = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I32X4ShrUOpcode:
        immOp = (0b1 << 24);
        op = SimdOp::unsignedBit;
        FALLTHROUGH;
    case ByteCode::I32X4ShrSOpcode:
        mask = SimdOp::shift32;
        type = SLJIT_SIMD_ELEM_32;
        immOp |= SimdOp::Type::vshrImm | SimdOp::SHL32;
        op |= SimdOp::Type::vshl | SimdOp::I32;
        break;
    case ByteCode::I64X2ShlOpcode:
        isShr = false;
        immOp = SimdOp::Type::vshlImm | SimdOp::lBit;
        op = SimdOp::Type::vshl | SimdOp::I64;
        mask = SimdOp::shift64;
        type = SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::I64X2ShrUOpcode:
        immOp = (0b1 << 24);
        op = SimdOp::unsignedBit;
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

    simdOperandToArg(compiler, operands, args[0], type, instr->requiredReg(0));
    args[1].set(operands + 1);

    args[2].set(operands + 2);
    sljit_s32 dst = GET_TARGET_REG(args[2].arg, instr->requiredReg(0));

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
        sljit_s32 srcReg = SLJIT_TMP_DEST_REG;
        sljit_emit_op2(compiler, SLJIT_AND32, srcReg, 0, args[1].arg, args[1].argw, SLJIT_IMM, mask);

        if (isShr) {
            sljit_emit_op2(compiler, SLJIT_SUB32, srcReg, 0, SLJIT_IMM, 0, srcReg, 0);
        }

        sljit_emit_simd_replicate(compiler, SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8, SLJIT_TMP_DEST_FREG, srcReg, 0);
        simdEmitOp(compiler, op, dst, SLJIT_TMP_DEST_FREG, args[0].arg);
    }

    if (SLJIT_IS_MEM(args[2].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | type, dst, args[2].arg, args[2].argw);
    }
}
