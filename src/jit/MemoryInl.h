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

struct MemAddress {
    enum Options : uint32_t {
        LoadToReg2 = 1 << 0,
        LoadToFReg0 = 1 << 1,
        Load32 = 1 << 2,
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        DontSetR1 = 1 << 3,
#endif /* SLJIT_32BIT_ARCHITECTURE */
#ifdef HAS_SIMD
        LoadToSimdReg0 = 1 << 4,
#endif /* HAS_SIMD */
    };

    void check(sljit_compiler* compiler, Operand* params, sljit_u32 offset, sljit_u32 size);
    void load(sljit_compiler* compiler);

    uint32_t options;
    JITArg memArg;
    JITArg loadArg;
};

void MemAddress::check(sljit_compiler* compiler, Operand* offsetOperand, sljit_u32 offset, sljit_u32 size)
{
    CompileContext* context = CompileContext::get(compiler);

    if (UNLIKELY(context->maximumMemorySize < size)) {
        // This memory load is never successful.
        sljit_set_label(sljit_emit_jump(compiler, SLJIT_JUMP), context->memoryTrapLabel);
        memArg.arg = 0;
        return;
    }

    JITArg offsetArg(offsetOperand);

    if (SLJIT_IS_IMM(offsetArg.arg)) {
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        sljit_uw totalOffset = offset + static_cast<sljit_u32>(offsetArg.argw);
#else /* !SLJIT_64BIT_ARCHITECTURE */
        sljit_u32 totalOffset = static_cast<sljit_u32>(offsetArg.argw);

        if (totalOffset + offset < (totalOffset | offset)) {
            // This memory load is never successful.
            sljit_set_label(sljit_emit_jump(compiler, SLJIT_JUMP), context->memoryTrapLabel);
            memArg.arg = 0;
            return;
        }

        totalOffset += offset;
#endif /* SLJIT_64BIT_ARCHITECTURE */

        if (UNLIKELY(totalOffset > context->maximumMemorySize - size)) {
            // This memory load is never successful.
            sljit_set_label(sljit_emit_jump(compiler, SLJIT_JUMP), context->memoryTrapLabel);
            memArg.arg = 0;
            return;
        }

        if (totalOffset + size <= context->initialMemorySize) {
            sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_MEM1(kContextReg),
                           OffsetOfContextField(memory0) + offsetof(Memory::TargetBuffer, buffer));
            memArg.arg = SLJIT_MEM1(SLJIT_R0);
            memArg.argw = totalOffset;
            load(compiler);
            return;
        }

#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_MEM1(kContextReg),
                       OffsetOfContextField(memory0) + offsetof(Memory::TargetBuffer, sizeInByte));
#else /* !SLJIT_64BIT_ARCHITECTURE */
        /* The sizeInByte is always a 32 bit number on 32 bit systems. */
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_MEM1(kContextReg),
                       OffsetOfContextField(memory0) + offsetof(Memory::TargetBuffer, sizeInByte) + WORD_LOW_OFFSET);
#endif /* SLJIT_64BIT_ARCHITECTURE */

        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_IMM, static_cast<sljit_sw>(totalOffset + size));
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_MEM1(kContextReg),
                       OffsetOfContextField(memory0) + offsetof(Memory::TargetBuffer, buffer));

        if (!(options & LoadToReg2)) {
            load(compiler);
        }

        sljit_set_label(sljit_emit_cmp(compiler, SLJIT_GREATER, SLJIT_R1, 0, SLJIT_R2, 0), context->memoryTrapLabel);

        if (options & LoadToReg2) {
            load(compiler);
        }

        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);

        memArg.arg = SLJIT_MEM1(SLJIT_R0);
        memArg.argw = -static_cast<sljit_sw>(size);
        return;
    }

    if (offset > context->maximumMemorySize - size) {
        // This memory load is never successful.
        sljit_set_label(sljit_emit_jump(compiler, SLJIT_JUMP), context->memoryTrapLabel);
        memArg.arg = 0;
        return;
    }

    sljit_s32 offsetReg = GET_SOURCE_REG(offsetArg.arg, SLJIT_R1);
    sljit_uw totalOffset = offset;

    MOVE_TO_REG(compiler, SLJIT_MOV_U32, offsetReg, offsetArg.arg, offsetArg.argw);

    if (context->initialMemorySize != context->maximumMemorySize) {
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_MEM1(kContextReg),
                       OffsetOfContextField(memory0) + offsetof(Memory::TargetBuffer, sizeInByte));
#else /* !SLJIT_64BIT_ARCHITECTURE */
        /* The sizeInByte is always a 32 bit number on 32 bit systems. */
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_MEM1(kContextReg),
                       OffsetOfContextField(memory0) + offsetof(Memory::TargetBuffer, sizeInByte) + WORD_LOW_OFFSET);
#endif /* SLJIT_64BIT_ARCHITECTURE */
        totalOffset += size;
    }

    sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_MEM1(kContextReg),
                   OffsetOfContextField(memory0) + offsetof(Memory::TargetBuffer, buffer));

    if (!(options & LoadToReg2) || context->initialMemorySize == context->maximumMemorySize) {
        load(compiler);
    }

    if (totalOffset > 0) {
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R1, 0, offsetReg, 0, SLJIT_IMM, static_cast<sljit_sw>(totalOffset));
#else /* !SLJIT_64BIT_ARCHITECTURE */
        sljit_emit_op2(compiler, SLJIT_ADD | SLJIT_SET_CARRY, SLJIT_R1, 0, offsetReg, 0, SLJIT_IMM, static_cast<sljit_sw>(totalOffset));
        sljit_set_label(sljit_emit_jump(compiler, SLJIT_CARRY), context->memoryTrapLabel);
#endif /* SLJIT_64BIT_ARCHITECTURE */
    }

    if (context->initialMemorySize == context->maximumMemorySize) {
        sljit_set_label(sljit_emit_cmp(compiler, SLJIT_GREATER, SLJIT_R1, 0, SLJIT_IMM, static_cast<sljit_sw>(context->maximumMemorySize - size)), context->memoryTrapLabel);

        memArg.arg = SLJIT_MEM2(SLJIT_R0, SLJIT_R1);
        memArg.argw = 0;

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        if (options & DontSetR1) {
            sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
            memArg.arg = SLJIT_MEM1(SLJIT_R0);
        }
#endif /* SLJIT_32BIT_ARCHITECTURE */
        return;
    }

    sljit_set_label(sljit_emit_cmp(compiler, SLJIT_GREATER, SLJIT_R1, 0, SLJIT_R2, 0), context->memoryTrapLabel);

    if (options & LoadToReg2) {
        load(compiler);
    }

    sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);

    memArg.arg = SLJIT_MEM1(SLJIT_R0);
    memArg.argw = -static_cast<sljit_sw>(size);
}

void MemAddress::load(sljit_compiler* compiler)
{
    if (options & LoadToReg2) {
        SLJIT_ASSERT(SLJIT_IS_MEM(loadArg.arg));

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        sljit_s32 opcode = SLJIT_MOV;
#else /* !SLJIT_32BIT_ARCHITECTURE */
        sljit_s32 opcode = (options & Load32) ? SLJIT_MOV32 : SLJIT_MOV;
#endif /* SLJIT_32BIT_ARCHITECTURE */

        sljit_emit_op1(compiler, opcode, SLJIT_R2, 0, loadArg.arg, loadArg.argw);
        return;
    }

    if (options & LoadToFReg0) {
        SLJIT_ASSERT(SLJIT_IS_MEM(loadArg.arg));
        sljit_emit_fop1(compiler, (options & Load32) ? SLJIT_MOV_F32 : SLJIT_MOV_F64, SLJIT_FR0, 0, loadArg.arg, loadArg.argw);
        return;
    }

#ifdef HAS_SIMD
    if (options & LoadToSimdReg0) {
        SLJIT_ASSERT(SLJIT_IS_MEM(loadArg.arg));
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, SLJIT_FR0, loadArg.arg, loadArg.argw);
        return;
    }
#endif /* HAS_SIMD */
}

static void emitLoad(sljit_compiler* compiler, Instruction* instr)
{
    sljit_s32 opcode;
    sljit_u32 size;
    sljit_u32 offset = 0;
#ifdef HAS_SIMD
    sljit_s32 simdType = 0;
#endif /* HAS_SIMD */

    switch (instr->opcode()) {
    case ByteCode::Load32Opcode:
        opcode = SLJIT_MOV32;
        size = 4;
        break;
    case ByteCode::Load64Opcode:
        opcode = SLJIT_MOV;
        size = 8;
        break;
    case ByteCode::I32LoadOpcode:
        opcode = SLJIT_MOV32;
        size = 4;
        break;
    case ByteCode::I32Load8SOpcode:
        opcode = SLJIT_MOV32_S8;
        size = 1;
        break;
    case ByteCode::I32Load8UOpcode:
        opcode = SLJIT_MOV32_U8;
        size = 1;
        break;
    case ByteCode::I32Load16SOpcode:
        opcode = SLJIT_MOV32_S16;
        size = 2;
        break;
    case ByteCode::I32Load16UOpcode:
        opcode = SLJIT_MOV32_U16;
        size = 2;
        break;
    case ByteCode::I64LoadOpcode:
        opcode = SLJIT_MOV;
        size = 8;
        break;
    case ByteCode::I64Load8SOpcode:
        opcode = SLJIT_MOV_S8;
        size = 1;
        break;
    case ByteCode::I64Load8UOpcode:
        opcode = SLJIT_MOV_U8;
        size = 1;
        break;
    case ByteCode::I64Load16SOpcode:
        opcode = SLJIT_MOV_S16;
        size = 2;
        break;
    case ByteCode::I64Load16UOpcode:
        opcode = SLJIT_MOV_U16;
        size = 2;
        break;
    case ByteCode::I64Load32SOpcode:
        opcode = SLJIT_MOV_S32;
        size = 4;
        break;
    case ByteCode::I64Load32UOpcode:
        opcode = SLJIT_MOV_U32;
        size = 4;
        break;
    case ByteCode::F32LoadOpcode:
        opcode = SLJIT_MOV_F32;
        size = 4;
        break;
#ifdef HAS_SIMD
    case ByteCode::V128LoadOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128;
        size = 16;
        break;
    case ByteCode::V128Load8SplatOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8;
        size = 1;
        break;
    case ByteCode::V128Load16SplatOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_16;
        size = 2;
        break;
    case ByteCode::V128Load32SplatOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32;
        size = 4;
        break;
    case ByteCode::V128Load64SplatOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        simdType |= SLJIT_SIMD_FLOAT;
#endif /* SLJIT_32BIT_ARCHITECTURE */
        size = 8;
        break;
    case ByteCode::V128Load8X8SOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8 | SLJIT_SIMD_EXTEND_SIGNED | SLJIT_SIMD_EXTEND_16;
        size = 8;
        break;
    case ByteCode::V128Load8X8UOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8 | SLJIT_SIMD_EXTEND_16;
        size = 8;
        break;
    case ByteCode::V128Load16X4SOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_16 | SLJIT_SIMD_EXTEND_SIGNED | SLJIT_SIMD_EXTEND_32;
        size = 8;
        break;
    case ByteCode::V128Load16X4UOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_16 | SLJIT_SIMD_EXTEND_32;
        size = 8;
        break;
    case ByteCode::V128Load32X2SOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32 | SLJIT_SIMD_EXTEND_SIGNED | SLJIT_SIMD_EXTEND_64;
        size = 8;
        break;
    case ByteCode::V128Load32X2UOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32 | SLJIT_SIMD_EXTEND_64;
        size = 8;
        break;
    case ByteCode::V128Load32ZeroOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32 | SLJIT_SIMD_LANE_ZERO;
        size = 4;
        break;
    case ByteCode::V128Load64ZeroOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64 | SLJIT_SIMD_LANE_ZERO;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        simdType |= SLJIT_SIMD_FLOAT;
#endif /* SLJIT_32BIT_ARCHITECTURE */
        size = 8;
        break;
#endif /* HAS_SIMD */
    default:
        ASSERT(instr->opcode() == ByteCode::F64LoadOpcode);
        opcode = SLJIT_MOV_F64;
        size = 8;
        break;
    }

    if (instr->opcode() != ByteCode::Load32Opcode && instr->opcode() != ByteCode::Load64Opcode) {
        MemoryLoad* loadOperation = reinterpret_cast<MemoryLoad*>(instr->byteCode());
        offset = loadOperation->offset();
    }

    Operand* operands = instr->operands();
    MemAddress addr;

    addr.options = 0;
    addr.check(compiler, operands, offset, size);

    if (addr.memArg.arg == 0) {
        return;
    }

    if (opcode == SLJIT_MOV_F64 || opcode == SLJIT_MOV_F32) {
        JITArg valueArg;
        floatOperandToArg(compiler, operands + 1, valueArg, SLJIT_FR0);

#if (defined SLJIT_FPU_UNALIGNED && SLJIT_FPU_UNALIGNED)
        sljit_emit_fop1(compiler, opcode, valueArg.arg, valueArg.argw, addr.memArg.arg, addr.memArg.argw);
#else /* SLJIT_FPU_UNALIGNED */
        sljit_s32 tmpReg = GET_TARGET_REG(valueArg.arg, SLJIT_FR0);
        sljit_emit_fmem(compiler, opcode | SLJIT_MEM_UNALIGNED, tmpReg, addr.memArg.arg, addr.memArg.argw);

        if (SLJIT_IS_MEM(valueArg.arg)) {
            sljit_emit_fop1(compiler, opcode, valueArg.arg, valueArg.argw, tmpReg, 0);
        }
#endif /* SLJIT_FPU_UNALIGNED */
        return;
    }

#ifdef HAS_SIMD
    if (opcode == 0) {
        ASSERT((SLJIT_SIMD_EXTEND_16 >> 24) == 1);

        JITArg valueArg(operands + 1);
        sljit_s32 dstReg = GET_TARGET_REG(valueArg.arg, SLJIT_FR0);

        // TODO: support aligned access
        if (size == 16) {
            sljit_emit_simd_mov(compiler, simdType, dstReg, addr.memArg.arg, addr.memArg.argw);
        } else if ((simdType >> 24) != 0) {
            sljit_emit_simd_extend(compiler, simdType, dstReg, addr.memArg.arg, addr.memArg.argw);
#if (defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
        } else if ((simdType & ~SLJIT_SIMD_LANE_ZERO) == (SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64 | SLJIT_SIMD_FLOAT)) {
            sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_64 | SLJIT_SIMD_ELEM_8, dstReg, addr.memArg.arg, addr.memArg.argw);

            if (instr->opcode() == ByteCode::V128Load64SplatOpcode) {
                sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_64 | SLJIT_SIMD_ELEM_8, getHighRegister(dstReg), dstReg, 0);
            } else {
                sljit_emit_simd_replicate(compiler, SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_64 | SLJIT_SIMD_ELEM_8, getHighRegister(dstReg), SLJIT_IMM, 0);
            }
#endif /* SLJIT_CONFIG_ARM_32 */
        } else if (simdType & SLJIT_SIMD_LANE_ZERO) {
            sljit_emit_simd_lane_mov(compiler, simdType, dstReg, 0, addr.memArg.arg, addr.memArg.argw);
        } else {
            sljit_emit_simd_replicate(compiler, simdType, dstReg, addr.memArg.arg, addr.memArg.argw);
        }

        if (SLJIT_IS_MEM(valueArg.arg)) {
            simdType = SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128;
            sljit_emit_simd_mov(compiler, simdType, SLJIT_FR0, valueArg.arg, valueArg.argw);
        }
        return;
    }
#endif /* HAS_SIMD */

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    if (!(opcode & SLJIT_32) && opcode != SLJIT_MOV32) {
        JITArgPair valueArgPair(operands + 1);

        sljit_s32 dstReg1 = GET_TARGET_REG(valueArgPair.arg1, SLJIT_R0);

        if (opcode == SLJIT_MOV) {
            sljit_s32 dstReg2 = GET_TARGET_REG(valueArgPair.arg2, SLJIT_R1);
            sljit_emit_mem(compiler, SLJIT_MOV | SLJIT_MEM_LOAD | SLJIT_MEM_UNALIGNED, SLJIT_REG_PAIR(dstReg1, dstReg2), addr.memArg.arg, addr.memArg.argw);

            if (SLJIT_IS_MEM(valueArgPair.arg1)) {
#if (defined SLJIT_BIG_ENDIAN && SLJIT_BIG_ENDIAN)
                sljit_emit_mem(compiler, SLJIT_MOV | SLJIT_MEM_STORE, SLJIT_REG_PAIR(dstReg1, dstReg2), valueArgPair.arg2, valueArgPair.arg2w);
#else /* !SLJIT_BIG_ENDIAN */
                sljit_emit_mem(compiler, SLJIT_MOV | SLJIT_MEM_STORE, SLJIT_REG_PAIR(dstReg1, dstReg2), valueArgPair.arg1, valueArgPair.arg1w);
#endif /* SLJIT_BIG_ENDIAN */
            }
            return;
        }

        SLJIT_ASSERT(size <= 4);
        // TODO: sljit_emit_mem for unaligned access
        sljit_emit_op1(compiler, opcode, dstReg1, 0, addr.memArg.arg, addr.memArg.argw);

        if (opcode == SLJIT_MOV_S8 || opcode == SLJIT_MOV_S16 || opcode == SLJIT_MOV_S32) {
            sljit_emit_op2(compiler, SLJIT_ASHR, valueArgPair.arg2, valueArgPair.arg2w, dstReg1, 0, SLJIT_IMM, 31);
        } else {
            sljit_emit_op1(compiler, SLJIT_MOV, valueArgPair.arg2, valueArgPair.arg2w, SLJIT_IMM, 0);
        }

        MOVE_FROM_REG(compiler, SLJIT_MOV, valueArgPair.arg1, valueArgPair.arg1w, dstReg1);
        return;
    }
#endif /* SLJIT_32BIT_ARCHITECTURE */

    JITArg valueArg(operands + 1);
    sljit_s32 dstReg = GET_TARGET_REG(valueArg.arg, SLJIT_R0);

    // TODO: sljit_emit_mem for unaligned access
    sljit_emit_op1(compiler, opcode, dstReg, 0, addr.memArg.arg, addr.memArg.argw);

    if (!SLJIT_IS_MEM(valueArg.arg)) {
        return;
    }

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    opcode = SLJIT_MOV;
#else /* !SLJIT_32BIT_ARCHITECTURE */
    opcode = (opcode == SLJIT_MOV32 || (opcode & SLJIT_32)) ? SLJIT_MOV32 : SLJIT_MOV;
#endif /* SLJIT_32BIT_ARCHITECTURE */

    sljit_emit_op1(compiler, opcode, valueArg.arg, valueArg.argw, dstReg, 0);
}

#ifdef HAS_SIMD

static void emitLoadLaneSIMD(sljit_compiler* compiler, Instruction* instr)
{
    sljit_u32 size;
    sljit_s32 simdType = 0;

    switch (instr->opcode()) {
    case ByteCode::V128Load8LaneOpcode:
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8;
        size = 1;
        break;
    case ByteCode::V128Load16LaneOpcode:
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_16;
        size = 2;
        break;
    case ByteCode::V128Load32LaneOpcode:
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32;
        size = 4;
        break;
    default:
        ASSERT(instr->opcode() == ByteCode::V128Load64LaneOpcode);
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE) \
    && !(defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64 | SLJIT_SIMD_FLOAT;
#else /* !SLJIT_32BIT_ARCHITECTURE || SLJIT_CONFIG_ARM_32 */
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64;
#endif /* SLJIT_32BIT_ARCHITECTURE && !SLJIT_CONFIG_ARM_32 */
        size = 8;
        break;
    }

    SIMDMemoryLoad* loadOperation = reinterpret_cast<SIMDMemoryLoad*>(instr->byteCode());
    Operand* operands = instr->operands();
    JITArg valueArg(operands + 2);
    sljit_s32 dstReg = GET_TARGET_REG(valueArg.arg, SLJIT_FR0);

    JITArg initValue;
    simdOperandToArg(compiler, operands + 1, initValue, simdType, dstReg);

    MemAddress addr;

    addr.options = 0;
    addr.check(compiler, operands, loadOperation->offset(), size);

    if (addr.memArg.arg == 0) {
        return;
    }

#if (defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
    if (simdType == (SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_64 | SLJIT_SIMD_ELEM_8, loadOperation->index() == 0 ? dstReg : getHighRegister(dstReg), addr.memArg.arg, addr.memArg.argw);
    } else {
#endif /* SLJIT_CONFIG_ARM_32 */
        sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_LOAD | simdType, dstReg, loadOperation->index(), addr.memArg.arg, addr.memArg.argw);
#if (defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
    }
#endif /* SLJIT_CONFIG_ARM_32 */

    if (SLJIT_IS_MEM(valueArg.arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | simdType, SLJIT_FR0, valueArg.arg, valueArg.argw);
    }
}

#endif /* HAS_SIMD */

static void emitStore(sljit_compiler* compiler, Instruction* instr)
{
    sljit_s32 opcode;
    sljit_u32 size;
    sljit_u32 offset = 0;
#ifdef HAS_SIMD
    sljit_s32 simdType = 0;
    sljit_s32 laneIndex = 0;
#endif /* HAS_SIMD */

    switch (instr->opcode()) {
    case ByteCode::Store32Opcode:
        opcode = SLJIT_MOV32;
        size = 4;
        break;
    case ByteCode::Store64Opcode:
        opcode = SLJIT_MOV;
        size = 8;
        break;
    case ByteCode::I32StoreOpcode:
        opcode = SLJIT_MOV32;
        size = 4;
        break;
    case ByteCode::I32Store8Opcode:
        opcode = SLJIT_MOV32_U8;
        size = 1;
        break;
    case ByteCode::I32Store16Opcode:
        opcode = SLJIT_MOV32_U16;
        size = 2;
        break;
    case ByteCode::I64StoreOpcode:
        opcode = SLJIT_MOV;
        size = 8;
        break;
    case ByteCode::I64Store8Opcode:
        opcode = SLJIT_MOV_U8;
        size = 1;
        break;
    case ByteCode::I64Store16Opcode:
        opcode = SLJIT_MOV_U16;
        size = 2;
        break;
    case ByteCode::I64Store32Opcode:
        opcode = SLJIT_MOV_U32;
        size = 4;
        break;
    case ByteCode::F32StoreOpcode:
        opcode = SLJIT_MOV_F32;
        size = 4;
        break;
#ifdef HAS_SIMD
    case ByteCode::V128StoreOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128;
        size = 16;
        break;
    case ByteCode::V128Store8LaneOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8;
        size = 1;
        break;
    case ByteCode::V128Store16LaneOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_16;
        size = 2;
        break;
    case ByteCode::V128Store32LaneOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32;
        size = 4;
        break;
    case ByteCode::V128Store64LaneOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        simdType |= SLJIT_SIMD_FLOAT;
#endif /* SLJIT_32BIT_ARCHITECTURE */
        size = 8;
        break;
#endif /* HAS_SIMD */
    default:
        ASSERT(instr->opcode() == ByteCode::F64StoreOpcode);
        opcode = SLJIT_MOV_F64;
        size = 8;
        break;
    }

    if (instr->opcode() != ByteCode::Store32Opcode && instr->opcode() != ByteCode::Store64Opcode) {
#ifdef HAS_SIMD
        if (opcode != 0) {
#endif /* HAS_SIMD */
            MemoryStore* storeOperation = reinterpret_cast<MemoryStore*>(instr->byteCode());
            offset = storeOperation->offset();
#ifdef HAS_SIMD
        } else {
            SIMDMemoryStore* storeOperation = reinterpret_cast<SIMDMemoryStore*>(instr->byteCode());
            offset = storeOperation->offset();
            laneIndex = storeOperation->index();
        }
#endif /* HAS_SIMD */
    }

    Operand* operands = instr->operands();
    MemAddress addr;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    JITArgPair valueArgPair;
#endif /* SLJIT_32BIT_ARCHITECTURE */

    addr.options = 0;

    if (opcode == SLJIT_MOV_F64 || opcode == SLJIT_MOV_F32) {
        floatOperandToArg(compiler, operands + 1, addr.loadArg, SLJIT_FR0);

        if (SLJIT_IS_MEM(addr.loadArg.arg)) {
            addr.options = MemAddress::LoadToFReg0;

            if (opcode == SLJIT_MOV_F32) {
                addr.options |= MemAddress::Load32;
            }
        }
#ifdef HAS_SIMD
    } else if (opcode == 0) {
        InstructionListItem* item = operands[1].item;

        if (item == nullptr || item->group() != Instruction::Immediate) {
            addr.loadArg.set(operands + 1);
        } else {
            ASSERT(item->asInstruction()->opcode() == ByteCode::Const128Opcode);

            addr.loadArg.arg = SLJIT_MEM0();
            addr.loadArg.argw = reinterpret_cast<sljit_sw>(reinterpret_cast<Const128*>(item->asInstruction()->byteCode())->value());
        }

        if (SLJIT_IS_MEM(addr.loadArg.arg)) {
            addr.options = MemAddress::LoadToSimdReg0;
        }
#endif /* HAS_SIMD */
    } else {
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        if ((opcode & SLJIT_32) || opcode == SLJIT_MOV32) {
            addr.loadArg.set(operands + 1);

            if (SLJIT_IS_MEM(addr.loadArg.arg)) {
                addr.options = MemAddress::LoadToReg2;
            }
        } else {
            valueArgPair.set(operands + 1);

            addr.loadArg.arg = valueArgPair.arg1;
            addr.loadArg.argw = valueArgPair.arg1w;

            if (SLJIT_IS_MEM(valueArgPair.arg1)) {
                addr.options = MemAddress::LoadToReg2;
            }

            if (opcode == SLJIT_MOV && !SLJIT_IS_REG(valueArgPair.arg1)) {
                addr.options |= MemAddress::DontSetR1;
            }
        }
#else /* !SLJIT_32BIT_ARCHITECTURE */
        addr.loadArg.set(operands + 1);

        if (SLJIT_IS_MEM(addr.loadArg.arg)) {
            addr.options = MemAddress::LoadToReg2;

            if (opcode == SLJIT_MOV32 || (opcode & SLJIT_32)) {
                addr.options |= MemAddress::Load32;
            }
        }
#endif /* SLJIT_32BIT_ARCHITECTURE */
    }

    addr.check(compiler, operands, offset, size);

    if (addr.memArg.arg == 0) {
        return;
    }

    if (opcode == SLJIT_MOV_F64 || opcode == SLJIT_MOV_F32) {
        if (addr.options & MemAddress::LoadToFReg0) {
            addr.loadArg.arg = SLJIT_FR0;
            addr.loadArg.argw = 0;
        }

#if (defined SLJIT_FPU_UNALIGNED && SLJIT_FPU_UNALIGNED)
        sljit_emit_fop1(compiler, opcode, addr.memArg.arg, addr.memArg.argw, addr.loadArg.arg, addr.loadArg.argw);
#else /* SLJIT_FPU_UNALIGNED */
        ASSERT(addr.memArg.argw == 0);
        sljit_emit_fmem(compiler, opcode | SLJIT_MEM_STORE | SLJIT_MEM_UNALIGNED, addr.loadArg.arg, addr.memArg.arg, addr.memArg.argw);
#endif /* SLJIT_FPU_UNALIGNED */
        return;
    }

#ifdef HAS_SIMD
    if (opcode == 0) {
        if (addr.options & MemAddress::LoadToSimdReg0) {
            addr.loadArg.arg = SLJIT_FR0;
            addr.loadArg.argw = 0;
        }

        if (size == 16) {
            sljit_emit_simd_mov(compiler, simdType, addr.loadArg.arg, addr.memArg.arg, addr.memArg.argw);
            return;
        }

#if (defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
        if (simdType == (SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64 | SLJIT_SIMD_FLOAT)) {
            sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_64 | SLJIT_SIMD_ELEM_8, laneIndex == 0 ? addr.loadArg.arg : getHighRegister(addr.loadArg.arg), addr.memArg.arg, addr.memArg.argw);
            return;
        }
#endif /* SLJIT_CONFIG_ARM_32 */
        sljit_emit_simd_lane_mov(compiler, simdType, addr.loadArg.arg, laneIndex, addr.memArg.arg, addr.memArg.argw);
        return;
    }
#endif /* HAS_SIMD */

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    if (opcode == SLJIT_MOV) {
        sljit_s32 dstReg1 = GET_SOURCE_REG(valueArgPair.arg1, SLJIT_R2);
        sljit_s32 dstReg2 = GET_SOURCE_REG(valueArgPair.arg2, SLJIT_R1);

        if (SLJIT_IS_MEM(valueArgPair.arg1)) {
            sljit_emit_op1(compiler, SLJIT_MOV, dstReg2, 0, valueArgPair.arg2, valueArgPair.arg2w);
        } else if (SLJIT_IS_IMM(valueArgPair.arg1)) {
#if (defined SLJIT_BIG_ENDIAN && SLJIT_BIG_ENDIAN)
            sljit_emit_op1(compiler, SLJIT_MOV, dstReg1, 0, SLJIT_IMM, valueArgPair.arg1w);
            sljit_emit_op1(compiler, SLJIT_MOV, dstReg2, 0, SLJIT_IMM, valueArgPair.arg2w);
#else /* !SLJIT_BIG_ENDIAN */
            sljit_emit_op1(compiler, SLJIT_MOV, dstReg2, 0, SLJIT_IMM, valueArgPair.arg2w);
            sljit_emit_op1(compiler, SLJIT_MOV, dstReg1, 0, SLJIT_IMM, valueArgPair.arg1w);
#endif /* SLJIT_BIG_ENDIAN */
        }

        sljit_emit_mem(compiler, SLJIT_MOV | SLJIT_MEM_STORE | SLJIT_MEM_UNALIGNED, SLJIT_REG_PAIR(dstReg1, dstReg2), addr.memArg.arg, addr.memArg.argw);
        return;
    }
#endif /* SLJIT_32BIT_ARCHITECTURE */

    if (addr.options & MemAddress::LoadToReg2) {
        addr.loadArg.arg = SLJIT_R2;
        addr.loadArg.argw = 0;
    }

    // TODO: sljit_emit_mem for unaligned access
    sljit_emit_op1(compiler, opcode, addr.memArg.arg, addr.memArg.argw, addr.loadArg.arg, addr.loadArg.argw);
}

static sljit_sw initMemory(uint32_t dstStart, uint32_t srcStart, uint32_t srcSize, ExecutionContext* context)
{
    Memory* memory = context->instance->memory(0);
    DataSegment& sg = context->instance->dataSegment(*(sljit_u32*)&context->tmp1);

    if (!memory->checkAccess(dstStart, srcSize)) {
        return ExecutionContext::OutOfBoundsMemAccessError;
    }

    if (srcStart >= sg.sizeInByte() || srcStart + srcSize > sg.sizeInByte()) {
        return ExecutionContext::OutOfBoundsMemAccessError;
    }

    memory->initMemory(&sg, dstStart, srcStart, srcSize);
    return ExecutionContext::NoError;
}

static sljit_sw copyMemory(uint32_t dstStart, uint32_t srcStart, uint32_t size, ExecutionContext* context)
{
    Memory* memory = context->instance->memory(0);

    if (!memory->checkAccess(srcStart, size) || !memory->checkAccess(dstStart, size)) {
        return ExecutionContext::OutOfBoundsMemAccessError;
    }

    memory->copyMemory(dstStart, srcStart, size);
    return ExecutionContext::NoError;
}

static sljit_sw fillMemory(uint32_t start, uint32_t value, uint32_t size, ExecutionContext* context)
{
    Memory* memory = context->instance->memory(0);

    if (!memory->checkAccess(start, size)) {
        return ExecutionContext::OutOfBoundsMemAccessError;
    }

    memory->fillMemory(start, value, size);
    return ExecutionContext::NoError;
}

static sljit_s32 growMemory(uint32_t newSize, ExecutionContext* context)
{
    Memory* memory = context->instance->memory(0);
    uint32_t oldSize = memory->sizeInPageSize();

    if (memory->grow(static_cast<uint64_t>(newSize) * Memory::s_memoryPageSize)) {
        return static_cast<sljit_s32>(oldSize);
    }

    return -1;
}

static void emitMemory(sljit_compiler* compiler, Instruction* instr)
{
    CompileContext* context = CompileContext::get(compiler);
    Operand* params = instr->operands();
    ByteCode::Opcode opcode = instr->opcode();

    switch (opcode) {
    case ByteCode::MemorySizeOpcode: {
        JITArg dstArg(params);

        sljit_emit_op2(compiler, SLJIT_LSHR32, dstArg.arg, dstArg.argw,
                       SLJIT_MEM1(kContextReg), OffsetOfContextField(memory0) + offsetof(Memory::TargetBuffer, sizeInByte), SLJIT_IMM, 16);
        return;
    }
    case ByteCode::MemoryInitOpcode:
    case ByteCode::MemoryCopyOpcode:
    case ByteCode::MemoryFillOpcode: {
        JITArg srcArg;

        for (int i = 0; i < 3; i++) {
            srcArg.set(params + i);
            sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_R(i), 0, srcArg.arg, srcArg.argw);
        }

        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R3, 0, kContextReg, 0);

        sljit_sw addr;

        if (opcode == ByteCode::MemoryInitOpcode) {
            MemoryInit* memoryInit = reinterpret_cast<MemoryInit*>(instr->byteCode());

            sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_MEM1(kContextReg), OffsetOfContextField(tmp1), SLJIT_IMM, memoryInit->segmentIndex());
            addr = GET_FUNC_ADDR(sljit_sw, initMemory);
        } else if (opcode == ByteCode::MemoryCopyOpcode) {
            addr = GET_FUNC_ADDR(sljit_sw, copyMemory);
        } else {
            addr = GET_FUNC_ADDR(sljit_sw, fillMemory);
        }

        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(W, 32, 32, 32, W), SLJIT_IMM, addr);

        // Currently all traps are OutOfBoundsMemAccessError.
        sljit_set_label(sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, ExecutionContext::NoError), context->memoryTrapLabel);
        return;
    }
    case ByteCode::MemoryGrowOpcode: {
        JITArg arg(params);

        MOVE_TO_REG(compiler, SLJIT_MOV32, SLJIT_R0, arg.arg, arg.argw);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, kContextReg, 0);

        sljit_sw addr = GET_FUNC_ADDR(sljit_sw, growMemory);
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS2(32, 32, W), SLJIT_IMM, addr);

        arg.set(params + 1);
        MOVE_FROM_REG(compiler, SLJIT_MOV32, arg.arg, arg.argw, SLJIT_R0);
        return;
    }
    default: {
        return;
    }
    }
}

static void dropData(uint32_t segmentIndex, ExecutionContext* context)
{
    DataSegment& sg = context->instance->dataSegment(segmentIndex);
    sg.drop();
}

static void emitDataDrop(sljit_compiler* compiler, Instruction* instr)
{
    DataDrop* dataDrop = reinterpret_cast<DataDrop*>(instr->byteCode());

    sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, static_cast<sljit_sw>(dataDrop->segmentIndex()));
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, kContextReg, 0);

    sljit_sw addr = GET_FUNC_ADDR(sljit_sw, dropData);
    sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS2V(32, W), SLJIT_IMM, addr);
}
