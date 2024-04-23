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

static void emitMoveV128(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg dst(operands + 1);
    JITArg src;

    sljit_s32 dstReg = GET_TARGET_REG(dst.arg, SLJIT_TMP_DEST_FREG);

    simdOperandToArg(compiler, operands + 0, src, SLJIT_SIMD_REG_128, dstReg);
    ASSERT(SLJIT_IS_REG(src.arg));

    if (dst.arg != src.arg) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128, src.arg, dst.arg, dst.argw);
    }
}

static void emitExtractLaneSIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();

    sljit_s32 type = 0;
    uint32_t index = reinterpret_cast<SIMDExtractLane*>(instr->byteCode())->index();

    switch (instr->opcode()) {
    case ByteCode::I8X16ExtractLaneSOpcode:
        type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8 | SLJIT_SIMD_LANE_SIGNED | SLJIT_32;
        break;
    case ByteCode::I8X16ExtractLaneUOpcode:
        type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8 | SLJIT_32;
        break;
    case ByteCode::I16X8ExtractLaneSOpcode:
        type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_16 | SLJIT_SIMD_LANE_SIGNED | SLJIT_32;
        break;
    case ByteCode::I16X8ExtractLaneUOpcode:
        type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_16 | SLJIT_32;
        break;
    case ByteCode::I32X4ExtractLaneOpcode:
        type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32 | SLJIT_32;
        break;
    case ByteCode::I64X2ExtractLaneOpcode:
        type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::F32X4ExtractLaneOpcode:
        type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32 | SLJIT_SIMD_FLOAT;
        break;
    default:
        ASSERT(instr->opcode() == ByteCode::F64X2ExtractLaneOpcode);
        type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64 | SLJIT_SIMD_FLOAT;
        break;
    }

    JITArg args[2];
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    JITArgPair dstArgPair;

    if (type == (SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64)) {
        dstArgPair.set(operands + 1);

        if (SLJIT_IS_MEM(dstArgPair.arg1)) {
            type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64 | SLJIT_SIMD_FLOAT;
            args[1].arg = dstArgPair.arg1;
            args[1].argw = dstArgPair.arg1w - WORD_LOW_OFFSET;
        }
    } else {
        args[1].set(operands + 1);
    }
#else /* !SLJIT_32BIT_ARCHITECTURE */
    args[1].set(operands + 1);
#endif /* SLJIT_32BIT_ARCHITECTURE */

    simdOperandToArg(compiler, operands + 0, args[0], type & ~(SLJIT_SIMD_LANE_SIGNED | SLJIT_32), instr->requiredReg(0));

    if (type & SLJIT_SIMD_FLOAT) {
        sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_STORE | type, args[0].arg, index, args[1].arg, args[1].argw);
        return;
    }

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    if (type == (SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64)) {
        index <<= 1;
        type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32;

        sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_STORE | type, args[0].arg, index, dstArgPair.arg1, dstArgPair.arg1w);
        sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_STORE | type, args[0].arg, index + 1, dstArgPair.arg2, dstArgPair.arg2w);
        return;
    }
#endif /* SLJIT_32BIT_ARCHITECTURE */

    sljit_s32 dstReg = GET_TARGET_REG(args[1].arg, SLJIT_TMP_DEST_REG);
    sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_STORE | type, args[0].arg, index, dstReg, 0);

    if (SLJIT_IS_MEM(args[1].arg)) {
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        sljit_s32 op = (type & SLJIT_32) ? SLJIT_MOV32 : SLJIT_MOV;
        sljit_emit_op1(compiler, op, args[1].arg, args[1].argw, dstReg, 0);
#else /* !SLJIT_64BIT_ARCHITECTURE && !SLJIT_CONFIG_ARM_32 */
        sljit_s32 op = SLJIT_MOV;
        sljit_emit_op1(compiler, SLJIT_MOV, args[1].arg, args[1].argw, dstReg, 0);
#endif /* SLJIT_CONFIG_ARM_32 */
    }
}

static void emitReplaceLaneSIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();

    sljit_s32 type = 0;
    uint32_t index = reinterpret_cast<SIMDReplaceLane*>(instr->byteCode())->index();

    switch (instr->opcode()) {
    case ByteCode::I8X16ReplaceLaneOpcode:
        type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8 | SLJIT_32;
        break;
    case ByteCode::I16X8ReplaceLaneOpcode:
        type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_16 | SLJIT_32;
        break;
    case ByteCode::I32X4ReplaceLaneOpcode:
        type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32 | SLJIT_32;
        break;
    case ByteCode::I64X2ReplaceLaneOpcode:
        type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::F32X4ReplaceLaneOpcode:
        type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32 | SLJIT_SIMD_FLOAT;
        break;
    default:
        ASSERT(instr->opcode() == ByteCode::F64X2ReplaceLaneOpcode);
        type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64 | SLJIT_SIMD_FLOAT;
        break;
    }

    JITArg args[3];
    args[2].set(operands + 2);

    sljit_s32 dstReg = GET_TARGET_REG(args[2].arg, instr->requiredReg(0));
    simdOperandToArg(compiler, operands + 0, args[0], type & ~SLJIT_32, dstReg);

    if (args[0].arg != dstReg) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | (type & ~SLJIT_32), dstReg, args[0].arg, 0);
    }

    if (type & SLJIT_SIMD_FLOAT) {
        floatOperandToArg(compiler, operands + 1, args[1], SLJIT_TMP_DEST_FREG);
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    } else if (type == (SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64)) {
        JITArgPair srcArgPair(operands + 1);

        if (SLJIT_IS_MEM(srcArgPair.arg1)) {
            type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64 | SLJIT_SIMD_FLOAT;

            args[1].arg = srcArgPair.arg1;
            args[1].argw = srcArgPair.arg1w - WORD_LOW_OFFSET;
        } else {
            index <<= 1;
            type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32 | SLJIT_32;

            sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_LOAD | type, dstReg, index, srcArgPair.arg1, srcArgPair.arg1w);
            index++;

            args[1].arg = srcArgPair.arg2;
            args[1].argw = srcArgPair.arg2w;
        }
#endif /* SLJIT_32BIT_ARCHITECTURE */
    } else {
        args[1].set(operands + 1);
    }

    sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_LOAD | type, dstReg, index, args[1].arg, args[1].argw);

    if (SLJIT_IS_MEM(args[2].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | (type & ~SLJIT_32), dstReg, args[2].arg, args[2].argw);
    }
}

static void emitSplatSIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();

    sljit_s32 type = 0;

    switch (instr->opcode()) {
    case ByteCode::I8X16SplatOpcode:
        type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8;
        break;
    case ByteCode::I16X8SplatOpcode:
        type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I32X4SplatOpcode:
        type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32;
        break;
    case ByteCode::I64X2SplatOpcode:
        type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64;
        break;
    case ByteCode::F32X4SplatOpcode:
        type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32 | SLJIT_SIMD_FLOAT;
        break;
    default:
        ASSERT(instr->opcode() == ByteCode::F64X2SplatOpcode);
        type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64 | SLJIT_SIMD_FLOAT;
        break;
    }

    JITArg args[2];
    args[1].set(operands + 1);

    sljit_s32 dstReg = GET_TARGET_REG(args[1].arg, instr->requiredReg(0));

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    if (type == (SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64)) {
        JITArgPair srcArgPair(operands);

        if (SLJIT_IS_MEM(srcArgPair.arg1)) {
            type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64 | SLJIT_SIMD_FLOAT;
            sljit_emit_simd_replicate(compiler, type, dstReg, srcArgPair.arg1, srcArgPair.arg1w - WORD_LOW_OFFSET);
        } else {
            type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32;
            sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_LOAD | type, dstReg, 0, srcArgPair.arg1, srcArgPair.arg1w);
            sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_LOAD | type, dstReg, 1, srcArgPair.arg2, srcArgPair.arg2w);

            type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64 | SLJIT_SIMD_FLOAT;
            sljit_emit_simd_lane_replicate(compiler, type, dstReg, dstReg, 0);
        }
    } else {
#endif /* SLJIT_32BIT_ARCHITECTURE */
        args[0].set(operands);
        sljit_emit_simd_replicate(compiler, type, dstReg, args[0].arg, args[0].argw);
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    }
#endif /* SLJIT_32BIT_ARCHITECTURE */

    if (SLJIT_IS_MEM(args[1].arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | type, dstReg, args[1].arg, args[1].argw);
    }
}

static void emitBitMaskSIMD(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();

    sljit_s32 type = 0;

    switch (instr->opcode()) {
    case ByteCode::I8X16BitmaskOpcode:
        type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8;
        break;
    case ByteCode::I16X8BitmaskOpcode:
        type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_16;
        break;
    case ByteCode::I32X4BitmaskOpcode:
        type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32;
        break;
    default:
        ASSERT(instr->opcode() == ByteCode::I64X2BitmaskOpcode);
        type = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64;
        break;
    }

    JITArg src;
    simdOperandToArg(compiler, operands, src, type, instr->requiredReg(0));

    JITArg dst(operands + 1);
    sljit_emit_simd_sign(compiler, SLJIT_SIMD_STORE | type | SLJIT_32, src.arg, dst.arg, dst.argw);
}

static void emitGlobalGet128(sljit_compiler* compiler, Instruction* instr)
{
    CompileContext* context = CompileContext::get(compiler);
    JITArg dst(instr->operands());

    GlobalGet128* globalGet = reinterpret_cast<GlobalGet128*>(instr->byteCode());

    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_TMP_MEM_REG, 0, SLJIT_MEM1(kContextReg), OffsetOfContextField(instance));
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_TMP_MEM_REG, 0, SLJIT_MEM1(SLJIT_TMP_MEM_REG), context->globalsStart + globalGet->index() * sizeof(void*));
    sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, SLJIT_TMP_DEST_FREG, SLJIT_MEM1(SLJIT_TMP_MEM_REG), JITFieldAccessor::globalValueOffset());
    sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, SLJIT_TMP_DEST_FREG, dst.arg, dst.argw);
}

static void emitGlobalSet128(sljit_compiler* compiler, Instruction* instr)
{
    CompileContext* context = CompileContext::get(compiler);
    JITArg src;

    simdOperandToArg(compiler, instr->operands(), src, SLJIT_SIMD_ELEM_128, SLJIT_TMP_DEST_FREG);

    GlobalSet128* globalSet = reinterpret_cast<GlobalSet128*>(instr->byteCode());

    if (SLJIT_IS_MEM(src.arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, SLJIT_TMP_DEST_FREG, src.arg, src.argw);
        src.arg = SLJIT_TMP_DEST_FREG;
        src.argw = 0;
    }

    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_TMP_MEM_REG, 0, SLJIT_MEM1(kContextReg), OffsetOfContextField(instance));
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_TMP_MEM_REG, 0, SLJIT_MEM1(SLJIT_TMP_MEM_REG), context->globalsStart + globalSet->index() * sizeof(void*));
    sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, src.arg, SLJIT_MEM1(SLJIT_TMP_MEM_REG), JITFieldAccessor::globalValueOffset());
}

static void emitSelect128(sljit_compiler* compiler, Instruction* instr, sljit_s32 type)
{
    Operand* operands = instr->operands();
    assert(instr->opcode() == ByteCode::SelectOpcode && instr->paramCount() == 3);

    JITArg args[2];
    JITArg target(operands + 3);

    simdOperandToArg(compiler, operands, args[0], SLJIT_SIMD_ELEM_128, instr->requiredReg(0));
    simdOperandToArg(compiler, operands + 1, args[1], SLJIT_SIMD_ELEM_128, instr->requiredReg(1));


    if (type == -1) {
        JITArg cond(operands + 2);
        sljit_emit_op2u(compiler, SLJIT_SUB32 | SLJIT_SET_Z, cond.arg, cond.argw, SLJIT_IMM, 0);
        type = SLJIT_NOT_ZERO;
    }

    const sljit_s32 mov_type = SLJIT_SIMD_ELEM_128 | SLJIT_SIMD_REG_128;
    sljit_s32 targetReg = GET_TARGET_REG(target.arg, SLJIT_TMP_DEST_FREG);
    sljit_s32 baseReg = 0;

    if (args[1].arg == targetReg) {
        baseReg = 1;
        type ^= 1;
    }

    if (args[0].arg != targetReg) {
        sljit_emit_simd_mov(compiler, mov_type, targetReg, args[0].arg, args[0].argw);
    }

    sljit_jump* jump = sljit_emit_jump(compiler, type);
    sljit_s32 otherReg = baseReg ^ 0x1;

    sljit_emit_simd_mov(compiler, mov_type, targetReg, args[otherReg].arg, args[otherReg].argw);
    sljit_set_label(jump, sljit_emit_label(compiler));

    if (SLJIT_IS_MEM(target.arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | mov_type, targetReg, target.arg, target.argw);
    }
}
