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

enum Type : uint32_t {
    fabs = 0x4ee0f800,
    fmin = 0x4ee0f400,
    fmax = 0x4e60f400,
};

};

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

static void emitUnarySIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[2];

    sljit_s32 type = SLJIT_SIMD_MEM_ELEM_128;

    switch (instr->opcode()) {
    case ByteCode::F64X2AbsOpcode:
        type = SLJIT_SIMD_MEM_FLOAT | SLJIT_SIMD_MEM_ELEM_64;
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
        simdEmitOp(compiler, SimdOp::fabs, dst, args[0].arg, 0);
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
        simdEmitOp(compiler, SimdOp::fmin, dst, args[0].arg, args[1].arg);
        break;
    case ByteCode::F64X2MaxOpcode:
        simdEmitOp(compiler, SimdOp::fmax, dst, args[0].arg, args[1].arg);
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    if (SLJIT_IS_MEM(args[2].arg)) {
        sljit_emit_simd_mem(compiler, SLJIT_SIMD_MEM_STORE | SLJIT_SIMD_MEM_REG_128 | type, dst, args[2].arg, args[2].argw);
    }
}
