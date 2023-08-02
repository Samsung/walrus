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
    neg = 0x6e20b800,
    mul = 0x4e209c00,
    rev64 = 0x4e200800,
    shll = 0x2e213800,
    sqadd = 0x4e200c00,
    sqsub = 0x4e202c00,
    sub = 0x6e208400,
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
    rd = sljit_get_register_index(SLJIT_SIMD_MEM_REG_128, rd);
    rn = sljit_get_register_index(SLJIT_SIMD_MEM_REG_128, rn);

    if (rm >= SLJIT_FR0) {
        rm = sljit_get_register_index(SLJIT_SIMD_MEM_REG_128, rm);
    }

    opcode |= (uint32_t)rd | ((uint32_t)rn << 5) | ((uint32_t)rm << 16);
    sljit_emit_op_custom(compiler, &opcode, sizeof(uint32_t));
}

static void simdEmitI64x2Mul(sljit_compiler* compiler, sljit_s32 rd, sljit_s32 rn, sljit_s32 rm)
{
    auto tmpReg1 = SLJIT_FR2;
    auto tmpReg2 = SLJIT_FR3;

    simdEmitOp(compiler,  SimdOp::rev64 | SimdOp::S4, tmpReg2, rm, 0);
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

    sljit_s32 type = SLJIT_SIMD_MEM_ELEM_128;

    switch (instr->opcode()) {
    case ByteCode::F64X2AbsOpcode:
        type = SLJIT_SIMD_MEM_FLOAT | SLJIT_SIMD_MEM_ELEM_64;
        break;
    case ByteCode::I8X16NegOpcode:
        type = SLJIT_SIMD_MEM_ELEM_8;
        break;
    case ByteCode::I16X8NegOpcode:
        type = SLJIT_SIMD_MEM_ELEM_16;
        break;
    case ByteCode::I32X4NegOpcode:
        type = SLJIT_SIMD_MEM_ELEM_32;
        break;
    case ByteCode::I64X2NegOpcode:
        type = SLJIT_SIMD_MEM_ELEM_64;
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
        sljit_emit_simd_mem(compiler, SLJIT_SIMD_MEM_STORE | SLJIT_SIMD_MEM_REG_128 | type, dst, args[1].arg, args[1].argw);
    }
}

static void emitBinarySIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[3];

    sljit_s32 type = SLJIT_SIMD_MEM_ELEM_128;

    switch (instr->opcode()) {
    case ByteCode::F64X2MinOpcode:
    case ByteCode::F64X2MaxOpcode:
        type = SLJIT_SIMD_MEM_FLOAT | SLJIT_SIMD_MEM_ELEM_64;
        break;
    case ByteCode::I8X16AddOpcode:
    case ByteCode::I8X16SubOpcode:
    case ByteCode::I8X16AddSatSOpcode:
    case ByteCode::I8X16AddSatUOpcode:
    case ByteCode::I8X16SubSatSOpcode:
    case ByteCode::I8X16SubSatUOpcode:
        type = SLJIT_SIMD_MEM_ELEM_8;
        break;
    case ByteCode::I16X8AddOpcode:
    case ByteCode::I16X8SubOpcode:
    case ByteCode::I16X8MulOpcode:
    case ByteCode::I16X8AddSatSOpcode:
    case ByteCode::I16X8AddSatUOpcode:
    case ByteCode::I16X8SubSatSOpcode:
    case ByteCode::I16X8SubSatUOpcode:
        type = SLJIT_SIMD_MEM_ELEM_16;
        break;
    case ByteCode::I32X4AddOpcode:
    case ByteCode::I32X4SubOpcode:
    case ByteCode::I32X4MulOpcode:
        type = SLJIT_SIMD_MEM_ELEM_32;
        break;
    case ByteCode::I64X2AddOpcode:
    case ByteCode::I64X2SubOpcode:
    case ByteCode::I64X2MulOpcode:
        type = SLJIT_SIMD_MEM_ELEM_64;
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
    case ByteCode::F64X2MinOpcode:
        simdEmitOp(compiler, SimdOp::fmin | SimdOp::FD2, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2MaxOpcode:
        simdEmitOp(compiler, SimdOp::fmax | SimdOp::FD2, dst, args[0].arg, args[1].arg);
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
    case ByteCode::I32X4AddOpcode:
        simdEmitOp(compiler, SimdOp::add | SimdOp::S4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4SubOpcode:
        simdEmitOp(compiler, SimdOp::sub | SimdOp::S4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I32X4MulOpcode:
        simdEmitOp(compiler, SimdOp::mul | SimdOp::S4, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2AddOpcode:
        simdEmitOp(compiler, SimdOp::add | SimdOp::D2, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2SubOpcode:
        simdEmitOp(compiler, SimdOp::sub | SimdOp::D2, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::I64X2MulOpcode: {
        simdEmitI64x2Mul(compiler, dst, args[0].arg, args[1].arg);
        break;
    }

    default:
        ASSERT_NOT_REACHED();
        break;
    }

    if (SLJIT_IS_MEM(args[2].arg)) {
        sljit_emit_simd_mem(compiler, SLJIT_SIMD_MEM_STORE | SLJIT_SIMD_MEM_REG_128 | type, dst, args[2].arg, args[2].argw);
    }
}
