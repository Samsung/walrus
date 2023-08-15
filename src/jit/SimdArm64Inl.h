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
    fabs = 0x4ea0f800,
    fmin = 0x4ea0f400,
    fmax = 0x4e20f400,
    add = 0x4e208400,
    addp = 0x4e20bc00,
    andOp = 0x4e201c00,
    bic = 0x4e601c00,
    bsl = 0x6e601c00,
    cmeq = 0x6e208c00,
    cmge = 0x4e203c00,
    cmgt = 0x4e203400,
    dup = 0x4e000c00,
    eor = 0x6e201c00,
    fcmeq = 0x4e20e400,
    fcmge = 0x6e20e400,
    fcmgt = 0x6ea0e400,
    neg = 0x6e20b800,
    notOp = 0x6e205800,
    mul = 0x4e209c00,
    orr = 0x4ea01c00,
    rev64 = 0x4e200800,
    shl = 0x4f005400,
    shll = 0x2e213800,
    sqadd = 0x4e200c00,
    sqsub = 0x4e202c00,
    sub = 0x6e208400,
    ucmge = 0x6e203c00,
    ucmgt = 0x6e203400,
    umlal = 0x2e208000,
    uqadd = 0x6e200c00,
    uqsub = 0x6e202c00,
    xtn = 0xe212800,
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

static void emitUnarySIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[2];

    sljit_s32 type = SLJIT_SIMD_ELEM_128;

    switch (instr->opcode()) {
    case ByteCode::F64X2AbsOpcode:
        type = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::I8X16NegOpcode:
        type = SLJIT_SIMD_ELEM_8;
        break;
    case ByteCode::I16X8NegOpcode:
        type = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I32X4NegOpcode:
        type = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I64X2NegOpcode:
        type = SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::V128NotOpcode:
        type = SLJIT_SIMD_ELEM_128;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    simdOperandToArg(compiler, operands, args[0], type, SLJIT_FR0);

    args[1].set(operands + 1);
    sljit_s32 dst = GET_TARGET_REG(args[1].arg, SLJIT_FR0);

    switch (instr->opcode()) {
    case ByteCode::F64X2AbsOpcode:
        simdEmitOp(compiler, SimdOp::fabs | SimdOp::FD2, dst, args[0].arg, 0);
        break;
    case ByteCode::V128NotOpcode:
        simdEmitOp(compiler, SimdOp::notOp, dst, args[0].arg, 0);
        break;
    case ByteCode::I8X16NegOpcode:
        simdEmitOp(compiler, SimdOp::neg | SimdOp::B16, dst, args[0].arg, 0);
        break;
    case ByteCode::I16X8NegOpcode:
        simdEmitOp(compiler, SimdOp::neg | SimdOp::H8, dst, args[0].arg, 0);
        break;
    case ByteCode::I32X4NegOpcode:
        simdEmitOp(compiler, SimdOp::neg | SimdOp::S4, dst, args[0].arg, 0);
        break;
    case ByteCode::I64X2NegOpcode:
        simdEmitOp(compiler, SimdOp::neg | SimdOp::D2, dst, args[0].arg, 0);
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
    case ByteCode::F32X4EqOpcode:
    case ByteCode::F32X4NeOpcode:
    case ByteCode::F32X4LtOpcode:
    case ByteCode::F32X4LeOpcode:
    case ByteCode::F32X4GtOpcode:
    case ByteCode::F32X4GeOpcode:
        type = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::F64X2MinOpcode:
    case ByteCode::F64X2MaxOpcode:
    case ByteCode::F64X2EqOpcode:
    case ByteCode::F64X2NeOpcode:
    case ByteCode::F64X2LtOpcode:
    case ByteCode::F64X2LeOpcode:
    case ByteCode::F64X2GtOpcode:
    case ByteCode::F64X2GeOpcode:
        type = SLJIT_SIMD_FLOAT | SLJIT_SIMD_ELEM_64;
        break;
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
    simdOperandToArg(compiler, operands + 1, args[1], type, SLJIT_FR1);

    args[2].set(operands + 2);
    sljit_s32 dst = GET_TARGET_REG(args[2].arg, SLJIT_FR0);

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
    case ByteCode::F64X2MinOpcode:
        simdEmitOp(compiler, SimdOp::fmin | SimdOp::FD2, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2MaxOpcode:
        simdEmitOp(compiler, SimdOp::fmax | SimdOp::FD2, dst, args[0].arg, args[1].arg);
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
    ASSERT(instr != nullptr);

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
