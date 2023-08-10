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

static void emitExtractLaneSIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();

    sljit_s32 type = 0;
    uint32_t index = 0;

    switch (instr->opcode()) {
    case ByteCode::I8X16ExtractLaneSOpcode:
        index = reinterpret_cast<I8X16ExtractLaneS*>(instr->byteCode())->index();
        type = SLJIT_SIMD_ELEM_8 | SLJIT_SIMD_LANE_SIGNED | SLJIT_32;
        break;
    case ByteCode::I8X16ExtractLaneUOpcode:
        index = reinterpret_cast<I8X16ExtractLaneU*>(instr->byteCode())->index();
        type = SLJIT_SIMD_ELEM_8 | SLJIT_32;
        break;
    case ByteCode::I16X8ExtractLaneSOpcode:
        index = reinterpret_cast<I16X8ExtractLaneS*>(instr->byteCode())->index();
        type = SLJIT_SIMD_ELEM_16 | SLJIT_SIMD_LANE_SIGNED | SLJIT_32;
        break;
    case ByteCode::I16X8ExtractLaneUOpcode:
        index = reinterpret_cast<I16X8ExtractLaneU*>(instr->byteCode())->index();
        type = SLJIT_SIMD_ELEM_16 | SLJIT_32;
        break;
    case ByteCode::I32X4ExtractLaneOpcode:
        index = reinterpret_cast<I32X4ExtractLane*>(instr->byteCode())->index();
        type = SLJIT_SIMD_ELEM_32 | SLJIT_32;
        break;
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
    case ByteCode::I64X2ExtractLaneOpcode:
        index = reinterpret_cast<I64X2ExtractLane*>(instr->byteCode())->index();
        type = SLJIT_SIMD_ELEM_64;
        break;
#endif /* SLJIT_64BIT_ARCHITECTURE */
    case ByteCode::F32X4ExtractLaneOpcode:
        index = reinterpret_cast<F32X4ExtractLane*>(instr->byteCode())->index();
        type = SLJIT_SIMD_ELEM_32 | SLJIT_SIMD_FLOAT;
        break;
    default:
        ASSERT(instr->opcode() == ByteCode::F64X2ExtractLaneOpcode);
        index = reinterpret_cast<F64X2ExtractLane*>(instr->byteCode())->index();
        type = SLJIT_SIMD_ELEM_64 | SLJIT_SIMD_FLOAT;
        break;
    }

    JITArg args[2];
    simdOperandToArg(compiler, operands + 0, args[0], type & ~(SLJIT_SIMD_LANE_SIGNED | SLJIT_32), SLJIT_FR0);
    args[1].set(operands + 1);

    if (type & SLJIT_SIMD_FLOAT) {
        sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | type, args[0].arg, index, args[1].arg, args[1].argw);
        return;
    }

    sljit_s32 dstReg = GET_TARGET_REG(args[1].arg, SLJIT_R0);
    sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | type, args[0].arg, index, dstReg, 0);

    if (SLJIT_IS_MEM(args[1].arg)) {
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        sljit_s32 op = (type & SLJIT_32) ? SLJIT_MOV32 : SLJIT_MOV;
#else /* !SLJIT_64BIT_ARCHITECTURE */
        sljit_s32 op = SLJIT_MOV;
#endif /* SLJIT_64BIT_ARCHITECTURE */

        sljit_emit_op1(compiler, op, args[1].arg, args[1].argw, dstReg, 0);
    }
}

static void emitReplaceLaneSIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();

    sljit_s32 type = 0;
    uint32_t index = 0;

    switch (instr->opcode()) {
    case ByteCode::I8X16ReplaceLaneOpcode:
        index = reinterpret_cast<I8X16ReplaceLane*>(instr->byteCode())->index();
        type = SLJIT_SIMD_ELEM_8 | SLJIT_32;
        break;
    case ByteCode::I16X8ReplaceLaneOpcode:
        index = reinterpret_cast<I16X8ReplaceLane*>(instr->byteCode())->index();
        type = SLJIT_SIMD_ELEM_16 | SLJIT_32;
        break;
    case ByteCode::I32X4ReplaceLaneOpcode:
        index = reinterpret_cast<I32X4ReplaceLane*>(instr->byteCode())->index();
        type = SLJIT_SIMD_ELEM_32 | SLJIT_32;
        break;
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
    case ByteCode::I64X2ReplaceLaneOpcode:
        index = reinterpret_cast<I64X2ReplaceLane*>(instr->byteCode())->index();
        type = SLJIT_SIMD_ELEM_64;
        break;
#endif /* SLJIT_64BIT_ARCHITECTURE */
    case ByteCode::F32X4ReplaceLaneOpcode:
        index = reinterpret_cast<F32X4ReplaceLane*>(instr->byteCode())->index();
        type = SLJIT_SIMD_ELEM_32 | SLJIT_SIMD_FLOAT;
        break;
    default:
        ASSERT(instr->opcode() == ByteCode::F64X2ReplaceLaneOpcode);
        index = reinterpret_cast<F64X2ReplaceLane*>(instr->byteCode())->index();
        type = SLJIT_SIMD_ELEM_64 | SLJIT_SIMD_FLOAT;
        break;
    }

    JITArg args[3];
    args[2].set(operands + 2);

    sljit_s32 dstReg = GET_TARGET_REG(args[2].arg, SLJIT_FR0);
    simdOperandToArg(compiler, operands + 0, args[0], type & ~SLJIT_32, dstReg);

    if (type & SLJIT_SIMD_FLOAT) {
        floatOperandToArg(compiler, operands + 1, args[1], SLJIT_FR1);
    } else {
        args[1].set(operands + 1);
    }

    if (args[0].arg != dstReg) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128 | (type & ~SLJIT_32), dstReg, args[0].arg, 0);
    }

    sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128 | type, dstReg, index, args[1].arg, args[1].argw);

    if (SLJIT_IS_MEM(args[2].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | (type & ~SLJIT_32), dstReg, args[2].arg, args[2].argw);
    }
}

static void emitSplatSIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();

    sljit_s32 type = 0;
    uint32_t index = 0;

    switch (instr->opcode()) {
    case ByteCode::I8X16SplatOpcode:
        type = SLJIT_SIMD_ELEM_8;
        break;
    case ByteCode::I16X8SplatOpcode:
        type = SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I32X4SplatOpcode:
        type = SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I64X2SplatOpcode:
        type = SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::F32X4SplatOpcode:
        type = SLJIT_SIMD_ELEM_32 | SLJIT_SIMD_FLOAT;
        break;
    default:
        ASSERT(instr->opcode() == ByteCode::F64X2SplatOpcode);
        type = SLJIT_SIMD_ELEM_64 | SLJIT_SIMD_FLOAT;
        break;
    }

    JITArg args[2] = { operands + 0, operands + 1 };
    sljit_s32 dstReg = GET_TARGET_REG(args[1].arg, SLJIT_FR0);

    sljit_emit_simd_replicate(compiler, SLJIT_SIMD_REG_128 | type, dstReg, args[0].arg, args[0].argw);

    if (SLJIT_IS_MEM(args[1].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | type, dstReg, args[1].arg, args[1].argw);
    }
}

static void emitGlobalGet128(sljit_compiler* compiler, Instruction* instr)
{
    CompileContext* context = CompileContext::get(compiler);
    JITArg dst(instr->operands());

    GlobalGet128* globalGet = reinterpret_cast<GlobalGet128*>(instr->byteCode());

    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_MEM1(kContextReg), OffsetOfContextField(instance));
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_MEM1(SLJIT_R0), context->globalsStart + globalGet->index() * sizeof(void*));
    sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, SLJIT_FR0, SLJIT_MEM1(SLJIT_R0), JITFieldAccessor::globalValueOffset());
    sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, SLJIT_FR0, dst.arg, dst.argw);
}

static void emitGlobalSet128(sljit_compiler* compiler, Instruction* instr)
{
    CompileContext* context = CompileContext::get(compiler);
    JITArg src;

    simdOperandToArg(compiler, instr->operands(), src, SLJIT_SIMD_ELEM_128, SLJIT_FR0);

    GlobalSet128* globalSet = reinterpret_cast<GlobalSet128*>(instr->byteCode());

    if (SLJIT_IS_MEM(src.arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, SLJIT_FR0, src.arg, src.argw);
        src.arg = SLJIT_FR0;
        src.argw = 0;
    }

    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_MEM1(kContextReg), OffsetOfContextField(instance));
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_MEM1(SLJIT_R0), context->globalsStart + globalSet->index() * sizeof(void*));
    sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, src.arg, SLJIT_MEM1(SLJIT_R0), JITFieldAccessor::globalValueOffset());
}
