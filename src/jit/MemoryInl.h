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
    void check(sljit_compiler* compiler, Operand* params, sljit_u32 offset, sljit_s32 size);

    JITArg memArg;
    sljit_s32 offsetReg;
    bool addRequired;
};

void MemAddress::check(sljit_compiler* compiler, Operand* params, sljit_u32 offset, sljit_s32 size)
{
    CompileContext* context = CompileContext::get(compiler);
    JITArg offsetArg;

    operandToArg(params, offsetArg);

    if (SLJIT_IS_IMM(offsetArg.arg)) {
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        sljit_sw totalOffset = static_cast<sljit_sw>(offset) + static_cast<sljit_sw>(static_cast<sljit_u32>(offsetArg.argw)) + size;
#else /* !SLJIT_64BIT_ARCHITECTURE */
        sljit_u32 totalOffset = static_cast<sljit_u32>(offsetArg.argw);

        if (totalOffset + offset < (totalOffset | offset)) {
            sljit_set_label(sljit_emit_jump(compiler, SLJIT_JUMP), context->memoryTrapLabel);
            memArg.arg = 0;
            return;
        }

        totalOffset += offset;

        if (totalOffset > UINT32_MAX - size) {
            sljit_set_label(sljit_emit_jump(compiler, SLJIT_JUMP), context->memoryTrapLabel);
            memArg.arg = 0;
            return;
        }

        totalOffset += size;
#endif /* SLJIT_64BIT_ARCHITECTURE */

        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_MEM1(kContextReg), OffsetOfContextField(memory0));
        sljit_emit_op1(compiler, SLJIT_MOV_U32, SLJIT_R1, 0, SLJIT_MEM1(SLJIT_R0), JITFieldAccessor::memorySizeInByteOffset());

        if (totalOffset > 255) {
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM, static_cast<sljit_sw>(totalOffset));
        }

        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_MEM1(SLJIT_R0), JITFieldAccessor::memoryBufferOffset());

        if (totalOffset <= 255) {
            sljit_set_label(sljit_emit_cmp(compiler, SLJIT_LESS, SLJIT_R1, 0, SLJIT_IMM, static_cast<sljit_sw>(totalOffset)), context->memoryTrapLabel);
        } else {
            sljit_set_label(sljit_emit_cmp(compiler, SLJIT_LESS, SLJIT_R1, 0, SLJIT_R2, 0), context->memoryTrapLabel);
        }

        if (totalOffset <= 255) {
            memArg.arg = SLJIT_MEM1(SLJIT_R0);
            memArg.argw = totalOffset - size;
            offsetReg = 0;
        } else {
            memArg.arg = SLJIT_MEM1(SLJIT_R0);
            memArg.argw = -size;
            offsetReg = SLJIT_R2;
        }
        return;
    }

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    if (offset >= UINT32_MAX - size) {
        // This memory load is never successful.
        sljit_set_label(sljit_emit_jump(compiler, SLJIT_JUMP), context->memoryTrapLabel);
        memArg.arg = 0;
        return;
    }
#endif /* SLJIT_32BIT_ARCHITECTURE */

    offsetReg = GET_SOURCE_REG(offsetArg.arg, SLJIT_R2);

    sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_MEM1(kContextReg), OffsetOfContextField(memory0));

    MOVE_TO_REG(compiler, SLJIT_MOV_U32, offsetReg, offsetArg.arg, offsetArg.argw);
    sljit_emit_op1(compiler, SLJIT_MOV_U32, SLJIT_R1, 0, SLJIT_MEM1(SLJIT_R0), JITFieldAccessor::memorySizeInByteOffset());
    sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_MEM1(SLJIT_R0), JITFieldAccessor::memoryBufferOffset());

#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
    sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R2, 0, offsetReg, 0, SLJIT_IMM, static_cast<sljit_sw>(offset) + size);
#else /* !SLJIT_64BIT_ARCHITECTURE */
    sljit_emit_op2(compiler, SLJIT_ADD | SLJIT_SET_CARRY, SLJIT_R2, 0, offsetReg, 0, SLJIT_IMM, static_cast<sljit_sw>(offset) + size);
    sljit_set_label(sljit_emit_jump(compiler, SLJIT_CARRY), context->memoryTrapLabel);
#endif /* SLJIT_64BIT_ARCHITECTURE */

    sljit_set_label(sljit_emit_cmp(compiler, SLJIT_LESS, SLJIT_R1, 0, SLJIT_R2, 0), context->memoryTrapLabel);

    memArg.arg = SLJIT_MEM1(SLJIT_R0);
    memArg.argw = -size;
    offsetReg = SLJIT_R2;
}

static void emitLoad(sljit_compiler* compiler, Instruction* instr)
{
    sljit_s32 opcode;
    sljit_s32 size;
    sljit_u32 offset = 0;

    switch (instr->opcode()) {
    case Load32Opcode:
        opcode = SLJIT_MOV32;
        size = 4;
        break;
    case Load64Opcode:
        opcode = SLJIT_MOV;
        size = 8;
        break;
    case I32LoadOpcode:
        opcode = SLJIT_MOV32;
        size = 4;
        break;
    case I32Load8SOpcode:
        opcode = SLJIT_MOV32_S8;
        size = 1;
        break;
    case I32Load8UOpcode:
        opcode = SLJIT_MOV32_U8;
        size = 1;
        break;
    case I32Load16SOpcode:
        opcode = SLJIT_MOV32_S16;
        size = 2;
        break;
    case I32Load16UOpcode:
        opcode = SLJIT_MOV32_U16;
        size = 2;
        break;
    case I64LoadOpcode:
        opcode = SLJIT_MOV;
        size = 8;
        break;
    case I64Load8SOpcode:
        opcode = SLJIT_MOV_S8;
        size = 1;
        break;
    case I64Load8UOpcode:
        opcode = SLJIT_MOV_U8;
        size = 1;
        break;
    case I64Load16SOpcode:
        opcode = SLJIT_MOV_S16;
        size = 2;
        break;
    case I64Load16UOpcode:
        opcode = SLJIT_MOV_U16;
        size = 2;
        break;
    case I64Load32SOpcode:
        opcode = SLJIT_MOV_S32;
        size = 4;
        break;
    case I64Load32UOpcode:
        opcode = SLJIT_MOV_U32;
        size = 4;
        break;
    case F32LoadOpcode:
        opcode = SLJIT_MOV_F32;
        size = 4;
        break;
    default:
        ASSERT(instr->opcode() == F64LoadOpcode);
        opcode = SLJIT_MOV_F64;
        size = 8;
        break;
    }

    if (instr->opcode() != Load32Opcode && instr->opcode() != Load64Opcode) {
        MemoryLoad* loadOperation = reinterpret_cast<MemoryLoad*>(instr->byteCode());
        offset = loadOperation->offset();
    }

    Operand* operands = instr->operands();
    MemAddress addr;

    addr.check(compiler, operands, offset, size);

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    if (addr.memArg.arg == 0) {
        return;
    }
#endif /* SLJIT_32BIT_ARCHITECTURE */

    if (addr.offsetReg != 0) {
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R0, 0, SLJIT_R0, 0, addr.offsetReg, 0);
    }

    if (opcode == SLJIT_MOV_F64 || opcode == SLJIT_MOV_F32) {
        JITArg valueArg;
        floatOperandToArg(compiler, operands + 1, valueArg, SLJIT_FR0);

        // TODO: sljit_emit_fmem for unaligned access
        sljit_emit_fop1(compiler, opcode, valueArg.arg, valueArg.argw, addr.memArg.arg, addr.memArg.argw);
        return;
    }

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    if (!(opcode & SLJIT_32)) {
        JITArgPair valueArgPair;
        operandToArgPair(operands + 1, valueArgPair);

        sljit_s32 dstReg1 = GET_TARGET_REG(valueArgPair.arg1, SLJIT_R0);

        if (opcode == SLJIT_MOV) {
            sljit_s32 dstReg2 = GET_TARGET_REG(valueArgPair.arg2, SLJIT_R1);

            sljit_emit_mem(compiler, opcode | SLJIT_MEM_LOAD, SLJIT_REG_PAIR(dstReg1, dstReg2), addr.memArg.arg, addr.memArg.argw);
            if (SLJIT_IS_MEM(valueArgPair.arg1)) {
#if (defined SLJIT_BIG_ENDIAN && SLJIT_BIG_ENDIAN)
                sljit_emit_mem(compiler, opcode | SLJIT_MEM_STORE, SLJIT_REG_PAIR(dstReg1, dstReg2), valueArgPair.arg2, valueArgPair.arg2w);
#else /* !SLJIT_BIG_ENDIAN */
                sljit_emit_mem(compiler, opcode | SLJIT_MEM_STORE, SLJIT_REG_PAIR(dstReg1, dstReg2), valueArgPair.arg1, valueArgPair.arg1w);
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

    JITArg valueArg;
    operandToArg(operands + 1, valueArg);

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

static void emitStore(sljit_compiler* compiler, Instruction* instr)
{
    sljit_s32 opcode;
    sljit_s32 size;
    sljit_u32 offset = 0;

    switch (instr->opcode()) {
    case Store32Opcode:
        opcode = SLJIT_MOV32;
        size = 4;
        break;
    case Store64Opcode:
        opcode = SLJIT_MOV;
        size = 8;
        break;
    case I32StoreOpcode:
        opcode = SLJIT_MOV32;
        size = 4;
        break;
    case I32Store8Opcode:
        opcode = SLJIT_MOV32_U8;
        size = 1;
        break;
    case I32Store16Opcode:
        opcode = SLJIT_MOV32_U16;
        size = 2;
        break;
    case I64StoreOpcode:
        opcode = SLJIT_MOV;
        size = 8;
        break;
    case I64Store8Opcode:
        opcode = SLJIT_MOV_U8;
        size = 1;
        break;
    case I64Store16Opcode:
        opcode = SLJIT_MOV_U16;
        size = 2;
        break;
    case I64Store32Opcode:
        opcode = SLJIT_MOV_U32;
        size = 4;
        break;
    case F32StoreOpcode:
        opcode = SLJIT_MOV_F32;
        size = 4;
        break;
    default:
        ASSERT(instr->opcode() == F64StoreOpcode);
        opcode = SLJIT_MOV_F64;
        size = 8;
        break;
    }

    if (instr->opcode() != Store32Opcode && instr->opcode() != Store64Opcode) {
        MemoryStore* storeOperation = reinterpret_cast<MemoryStore*>(instr->byteCode());
        offset = storeOperation->offset();
    }

    Operand* operands = instr->operands();
    MemAddress addr;

    addr.check(compiler, operands, offset, size);

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    if (addr.memArg.arg == 0) {
        return;
    }
#endif /* SLJIT_32BIT_ARCHITECTURE */

    if (opcode == SLJIT_MOV_F64 || opcode == SLJIT_MOV_F32) {
        JITArg valueArg;
        floatOperandToArg(compiler, operands + 1, valueArg, SLJIT_FR0);

        if (addr.offsetReg != 0) {
            if (SLJIT_IS_MEM(valueArg.arg)) {
                sljit_emit_fop1(compiler, opcode, SLJIT_FR0, 0, valueArg.arg, valueArg.argw);
                valueArg.arg = SLJIT_FR0;
                valueArg.argw = 0;
            }

            sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R0, 0, SLJIT_R0, 0, addr.offsetReg, 0);
        }

        // TODO: sljit_emit_fmem for unaligned access
        sljit_emit_fop1(compiler, opcode, addr.memArg.arg, addr.memArg.argw, valueArg.arg, valueArg.argw);
        return;
    }

    JITArg valueArg;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    if (!(opcode & SLJIT_32)) {
        JITArgPair valueArgPair;
        operandToArgPair(operands + 1, valueArgPair);

        if (opcode == SLJIT_MOV) {
            if (addr.offsetReg != 0) {
                sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R0, 0, SLJIT_R0, 0, addr.offsetReg, 0);
            }

            sljit_s32 dstReg1 = GET_SOURCE_REG(valueArgPair.arg1, SLJIT_R1);
            sljit_s32 dstReg2 = GET_SOURCE_REG(valueArgPair.arg2, SLJIT_R2);

            if (SLJIT_IS_MEM(valueArgPair.arg1)) {
#if (defined SLJIT_BIG_ENDIAN && SLJIT_BIG_ENDIAN)
                sljit_emit_mem(compiler, opcode | SLJIT_MEM_LOAD, SLJIT_REG_PAIR(dstReg1, dstReg2), valueArgPair.arg2, valueArgPair.arg2w);
#else /* !SLJIT_BIG_ENDIAN */
                sljit_emit_mem(compiler, opcode | SLJIT_MEM_LOAD, SLJIT_REG_PAIR(dstReg1, dstReg2), valueArgPair.arg1, valueArgPair.arg1w);
#endif /* SLJIT_BIG_ENDIAN */
            } else if (SLJIT_IS_IMM(valueArgPair.arg1)) {
#if (defined SLJIT_BIG_ENDIAN && SLJIT_BIG_ENDIAN)
                sljit_emit_op1(compiler, SLJIT_MOV, dstReg1, 0, SLJIT_IMM, valueArgPair.arg1w);
                sljit_emit_op1(compiler, SLJIT_MOV, dstReg2, 0, SLJIT_IMM, valueArgPair.arg2w);
#else /* !SLJIT_BIG_ENDIAN */
                sljit_emit_op1(compiler, SLJIT_MOV, dstReg2, 0, SLJIT_IMM, valueArgPair.arg2w);
                sljit_emit_op1(compiler, SLJIT_MOV, dstReg1, 0, SLJIT_IMM, valueArgPair.arg1w);
#endif /* SLJIT_BIG_ENDIAN */
            }

            sljit_emit_mem(compiler, opcode | SLJIT_MEM_STORE, SLJIT_REG_PAIR(dstReg1, dstReg2), addr.memArg.arg, addr.memArg.argw);
            return;
        }

        // Ignore the high word.
        SLJIT_ASSERT(size <= 4);
        valueArg.arg = valueArgPair.arg1;
        valueArg.argw = valueArgPair.arg1w;
    } else {
        operandToArg(operands + 1, valueArg);
    }
#else /* !SLJIT_32BIT_ARCHITECTURE */
    operandToArg(operands + 1, valueArg);
#endif /* SLJIT_32BIT_ARCHITECTURE */

    if (SLJIT_IS_MEM(valueArg.arg)) {
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        sljit_s32 mov_opcode = SLJIT_MOV;
#else /* !SLJIT_32BIT_ARCHITECTURE */
        sljit_s32 mov_opcode = (opcode == SLJIT_MOV32 || (opcode & SLJIT_32)) ? SLJIT_MOV32 : SLJIT_MOV;
#endif /* SLJIT_32BIT_ARCHITECTURE */

        sljit_emit_op1(compiler, mov_opcode, SLJIT_R1, 0, valueArg.arg, valueArg.argw);
        valueArg.arg = SLJIT_R1;
        valueArg.argw = 0;
    }

    if (addr.offsetReg != 0) {
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R0, 0, SLJIT_R0, 0, addr.offsetReg, 0);
    }

    // TODO: sljit_emit_mem for unaligned access
    sljit_emit_op1(compiler, opcode, addr.memArg.arg, addr.memArg.argw, valueArg.arg, valueArg.argw);
}

static sljit_sw initMemory(uint32_t dstStart, uint32_t srcStart, uint32_t srcSize, ExecutionContext* context)
{
    DataSegment& sg = context->instance->dataSegment(*(sljit_u32*)&context->tmp1);

    if (!context->memory0->checkAccess(dstStart, srcSize)) {
        return ExecutionContext::OutOfBoundsMemAccessError;
    }

    if (srcStart >= sg.sizeInByte() || srcStart + srcSize > sg.sizeInByte()) {
        return ExecutionContext::OutOfBoundsMemAccessError;
    }

    context->memory0->initMemory(&sg, dstStart, srcStart, srcSize);
    return ExecutionContext::NoError;
}

static sljit_sw copyMemory(uint32_t dstStart, uint32_t srcStart, uint32_t size, ExecutionContext* context)
{
    if (!context->memory0->checkAccess(srcStart, size) || !context->memory0->checkAccess(dstStart, size)) {
        return ExecutionContext::OutOfBoundsMemAccessError;
    }

    context->memory0->copyMemory(dstStart, srcStart, size);
    return ExecutionContext::NoError;
}

static sljit_sw fillMemory(uint32_t start, uint32_t value, uint32_t size, ExecutionContext* context)
{
    if (!context->memory0->checkAccess(start, size)) {
        return ExecutionContext::OutOfBoundsMemAccessError;
    }

    context->memory0->fillMemory(start, value, size);
    return ExecutionContext::NoError;
}

static sljit_s32 growMemory(uint32_t newSize, ExecutionContext* context)
{
    uint32_t oldSize = context->memory0->sizeInPageSize();

    if (context->memory0->grow(static_cast<uint64_t>(newSize) * Memory::s_memoryPageSize)) {
        return static_cast<sljit_s32>(oldSize);
    }

    return -1;
}

static void emitMemory(sljit_compiler* compiler, Instruction* instr)
{
    CompileContext* context = CompileContext::get(compiler);
    Operand* params = instr->operands();
    OpcodeKind opcode = instr->opcode();

    switch (opcode) {
    case MemorySizeOpcode: {
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_MEM1(kContextReg), OffsetOfContextField(memory0));

        JITArg dstArg;
        operandToArg(params, dstArg);

        sljit_s32 dstReg = GET_TARGET_REG(dstArg.arg, SLJIT_R0);

        sljit_emit_op1(compiler, SLJIT_MOV_U32, dstReg, 0, SLJIT_MEM1(SLJIT_R0), JITFieldAccessor::memorySizeInByteOffset());
        sljit_emit_op2(compiler, SLJIT_LSHR, dstReg, 0, dstReg, 0, SLJIT_IMM, 16);
        MOVE_FROM_REG(compiler, SLJIT_MOV_U32, dstArg.arg, dstArg.argw, dstReg);
        return;
    }
    case MemoryInitOpcode:
    case MemoryCopyOpcode:
    case MemoryFillOpcode: {
        JITArg srcArg;

        for (int i = 0; i < 3; i++) {
            operandToArg(params + i, srcArg);
            sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_R(i), 0, srcArg.arg, srcArg.argw);
        }

        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R3, 0, kContextReg, 0);

        sljit_sw addr;

        if (opcode == MemoryInitOpcode) {
            MemoryInit* memoryInit = reinterpret_cast<MemoryInit*>(instr->byteCode());

            sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_MEM1(kContextReg), OffsetOfContextField(tmp1), SLJIT_IMM, memoryInit->segmentIndex());
            addr = GET_FUNC_ADDR(sljit_sw, initMemory);
        } else if (opcode == MemoryCopyOpcode) {
            addr = GET_FUNC_ADDR(sljit_sw, copyMemory);
        } else {
            addr = GET_FUNC_ADDR(sljit_sw, fillMemory);
        }

        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(W, 32, 32, 32, W), SLJIT_IMM, addr);

        // Currently all traps are OutOfBoundsMemAccessError.
        sljit_set_label(sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, ExecutionContext::NoError), context->memoryTrapLabel);
        return;
    }
    case MemoryGrowOpcode: {
        JITArg arg;

        operandToArg(params, arg);
        MOVE_TO_REG(compiler, SLJIT_MOV32, SLJIT_R0, arg.arg, arg.argw);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, kContextReg, 0);

        sljit_sw addr = GET_FUNC_ADDR(sljit_sw, growMemory);
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS2(32, 32, W), SLJIT_IMM, addr);

        operandToArg(params + 1, arg);
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
    sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS2(VOID, 32, W), SLJIT_IMM, addr);
}
