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
        LoadInteger = 1 << 0,
        LoadFloat = 1 << 1,
        Load32 = 1 << 2,
#ifdef HAS_SIMD
        LoadSimd = 1 << 3,
#endif /* HAS_SIMD */
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        DontUseOffsetReg = 1 << 4,
#endif /* SLJIT_32BIT_ARCHITECTURE */
        CheckNaturalAlignment = 1 << 5,
        // Limits the resulting addressing mode to a base register with no offset.
        AbsoluteAddress = 1 << 6,
    };

    MemAddress(uint32_t options, uint8_t baseReg, uint8_t offsetReg, uint8_t sourceReg)
        : options(options)
        , baseReg(baseReg)
        , offsetReg(offsetReg)
        , sourceReg(sourceReg)
    {
    }

    void check(sljit_compiler* compiler, Operand* params, sljit_uw offset, sljit_u32 size, sljit_u16 memIndex);
    void load(sljit_compiler* compiler);

    uint32_t options;
    uint8_t baseReg;
    // Also used by 64 bit moves on 32 bit systems
    uint8_t offsetReg;
    uint8_t sourceReg;
    JITArg memArg;
    JITArg loadArg;
};

void MemAddress::check(sljit_compiler* compiler, Operand* offsetOperand, sljit_uw offset, sljit_u32 size, sljit_u16 memIndex)
{
    CompileContext* context = CompileContext::get(compiler);

    ASSERT(!(options & LoadInteger) || baseReg != sourceReg);
    ASSERT(!(options & LoadInteger) || offsetReg != sourceReg);
    ASSERT(!(options & CheckNaturalAlignment) || size != 1);

    MemoryType* memoryType = context->module->memoryType(memIndex);
    uint64_t initialMemorySize = memoryType->initialSize() * Memory::s_memoryPageSize;
    uint64_t maximumMemorySize = memoryType->maximumSize() * Memory::s_memoryPageSize;

    sljit_sw targetBufferOffset = context->targetBuffersStart;
    if (memIndex != 0) {
        targetBufferOffset += Memory::TargetBuffer::sizeInPointers(memIndex) * sizeof(void*);
    }

    if (UNLIKELY(maximumMemorySize < size)) {
        // This memory load is never successful.
        context->appendTrapJump(ExecutionContext::OutOfBoundsMemAccessError, sljit_emit_jump(compiler, SLJIT_JUMP));
        memArg.arg = 0;
        return;
    }

    JITArg offsetArg(offsetOperand);

    if (SLJIT_IS_IMM(offsetArg.arg)) {
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        offset += static_cast<sljit_u32>(offsetArg.argw);
#else /* !SLJIT_64BIT_ARCHITECTURE */
        sljit_uw offsetArgw = static_cast<sljit_uw>(offsetArg.argw);

        if (offset + offsetArgw < (offset | offsetArgw)) {
            // This memory load is never successful.
            context->appendTrapJump(ExecutionContext::OutOfBoundsMemAccessError, sljit_emit_jump(compiler, SLJIT_JUMP));
            memArg.arg = 0;
            return;
        }

        offset += offsetArgw;
#endif /* SLJIT_64BIT_ARCHITECTURE */

        if (UNLIKELY(offset > maximumMemorySize - size)) {
            // This memory load is never successful.
            context->appendTrapJump(ExecutionContext::OutOfBoundsMemAccessError, sljit_emit_jump(compiler, SLJIT_JUMP));
            memArg.arg = 0;
            return;
        }

        if ((options & CheckNaturalAlignment) && (offset & (size - 1)) != 0) {
            context->appendTrapJump(ExecutionContext::UnalignedAtomicError, sljit_emit_jump(compiler, SLJIT_NOT_ZERO));
            return;
        }

        if (offset + size <= initialMemorySize) {
            ASSERT(baseReg != 0);
            sljit_emit_op1(compiler, SLJIT_MOV_P, baseReg, 0, SLJIT_MEM1(kInstanceReg),
                           targetBufferOffset + offsetof(Memory::TargetBuffer, buffer));
            memArg.arg = SLJIT_MEM1(baseReg);
            memArg.argw = offset;
            load(compiler);

            if (options & AbsoluteAddress) {
                sljit_emit_op2(compiler, SLJIT_ADD, baseReg, 0, baseReg, 0, SLJIT_IMM, offset);
                memArg.argw = 0;
            }
            return;
        }

        ASSERT(baseReg != 0 && offsetReg != 0);
        /* The sizeInByte is always a 32 bit number on 32 bit systems. */
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(kInstanceReg),
                       targetBufferOffset + offsetof(Memory::TargetBuffer, sizeInByte) + WORD_LOW_OFFSET);

        sljit_emit_op1(compiler, SLJIT_MOV, offsetReg, 0, SLJIT_IMM, static_cast<sljit_sw>(offset + size));
        sljit_emit_op1(compiler, SLJIT_MOV_P, baseReg, 0, SLJIT_MEM1(kInstanceReg),
                       targetBufferOffset + offsetof(Memory::TargetBuffer, buffer));

        load(compiler);

        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_GREATER, offsetReg, 0, SLJIT_TMP_DEST_REG, 0);
        context->appendTrapJump(ExecutionContext::OutOfBoundsMemAccessError, cmp);

        sljit_emit_op2(compiler, SLJIT_ADD, baseReg, 0, baseReg, 0, offsetReg, 0);

        memArg.arg = SLJIT_MEM1(baseReg);
        memArg.argw = -static_cast<sljit_sw>(size);

        if (options & AbsoluteAddress) {
            sljit_emit_op2(compiler, SLJIT_SUB, baseReg, 0, baseReg, 0, SLJIT_IMM, static_cast<sljit_sw>(size));
            memArg.argw = 0;
        }
        return;
    }

    if (offset > maximumMemorySize - size) {
        // This memory load is never successful.
        context->appendTrapJump(ExecutionContext::OutOfBoundsMemAccessError, sljit_emit_jump(compiler, SLJIT_JUMP));
        memArg.arg = 0;
        return;
    }

    ASSERT(baseReg != 0 && offsetReg != 0);
    sljit_emit_op1(compiler, SLJIT_MOV_U32, offsetReg, 0, offsetArg.arg, offsetArg.argw);

    if (initialMemorySize != maximumMemorySize) {
        /* The sizeInByte is always a 32 bit number on 32 bit systems. */
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(kInstanceReg),
                       targetBufferOffset + offsetof(Memory::TargetBuffer, sizeInByte) + WORD_LOW_OFFSET);
        offset += size;
    }

    sljit_emit_op1(compiler, SLJIT_MOV_P, baseReg, 0, SLJIT_MEM1(kInstanceReg),
                   targetBufferOffset + offsetof(Memory::TargetBuffer, buffer));

    load(compiler);

    if (offset > 0) {
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        sljit_emit_op2(compiler, SLJIT_ADD, offsetReg, 0, offsetReg, 0, SLJIT_IMM, static_cast<sljit_sw>(offset));
#else /* !SLJIT_64BIT_ARCHITECTURE */
        sljit_emit_op2(compiler, SLJIT_ADD | SLJIT_SET_CARRY, offsetReg, 0, offsetReg, 0, SLJIT_IMM, static_cast<sljit_sw>(offset));
        context->appendTrapJump(ExecutionContext::OutOfBoundsMemAccessError, sljit_emit_jump(compiler, SLJIT_CARRY));
#endif /* SLJIT_64BIT_ARCHITECTURE */
    }

    if (initialMemorySize == maximumMemorySize) {
        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_GREATER, offsetReg, 0, SLJIT_IMM, static_cast<sljit_sw>(maximumMemorySize - size));
        context->appendTrapJump(ExecutionContext::OutOfBoundsMemAccessError, cmp);

        memArg.arg = SLJIT_MEM2(baseReg, offsetReg);
        memArg.argw = 0;

        if (options & CheckNaturalAlignment) {
            sljit_emit_op2u(compiler, SLJIT_AND | SLJIT_SET_Z, offsetReg, 0, SLJIT_IMM, size - 1);
            context->appendTrapJump(ExecutionContext::UnalignedAtomicError, sljit_emit_jump(compiler, SLJIT_NOT_ZERO));
        }

        uint32_t checkedOptions = 0;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        checkedOptions |= DontUseOffsetReg;
#endif /* SLJIT_32BIT_ARCHITECTURE */

        checkedOptions |= AbsoluteAddress;

        if (options & checkedOptions) {
            sljit_emit_op2(compiler, SLJIT_ADD, baseReg, 0, baseReg, 0, offsetReg, 0);
            memArg.arg = SLJIT_MEM1(baseReg);
        }
        return;
    }

    sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_GREATER, offsetReg, 0, SLJIT_TMP_DEST_REG, 0);
    context->appendTrapJump(ExecutionContext::OutOfBoundsMemAccessError, cmp);

    sljit_emit_op2(compiler, SLJIT_ADD, baseReg, 0, baseReg, 0, offsetReg, 0);

    if (options & CheckNaturalAlignment) {
        sljit_emit_op2u(compiler, SLJIT_AND | SLJIT_SET_Z, offsetReg, 0, SLJIT_IMM, size - 1);
        context->appendTrapJump(ExecutionContext::UnalignedAtomicError, sljit_emit_jump(compiler, SLJIT_NOT_ZERO));
    }

    memArg.arg = SLJIT_MEM1(baseReg);
    memArg.argw = -static_cast<sljit_sw>(size);

    if (options & AbsoluteAddress) {
        sljit_emit_op2(compiler, SLJIT_SUB, baseReg, 0, baseReg, 0, SLJIT_IMM, static_cast<sljit_sw>(size));
        memArg.argw = 0;
    }
}

void MemAddress::load(sljit_compiler* compiler)
{
    if (options & LoadInteger) {
        SLJIT_ASSERT(SLJIT_IS_MEM(loadArg.arg) && sourceReg != 0);

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        sljit_s32 opcode = SLJIT_MOV;
#else /* !SLJIT_32BIT_ARCHITECTURE */
        sljit_s32 opcode = (options & Load32) ? SLJIT_MOV32 : SLJIT_MOV;
#endif /* SLJIT_32BIT_ARCHITECTURE */

        sljit_emit_op1(compiler, opcode, sourceReg, 0, loadArg.arg, loadArg.argw);
        return;
    }

    if (options & LoadFloat) {
        SLJIT_ASSERT(SLJIT_IS_MEM(loadArg.arg) && sourceReg == 0);
        sljit_emit_fop1(compiler, (options & Load32) ? SLJIT_MOV_F32 : SLJIT_MOV_F64, SLJIT_TMP_DEST_FREG, 0, loadArg.arg, loadArg.argw);
        return;
    }

#ifdef HAS_SIMD
    if (options & LoadSimd) {
        SLJIT_ASSERT(SLJIT_IS_MEM(loadArg.arg) && sourceReg != 0);
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128, sourceReg, loadArg.arg, loadArg.argw);
        return;
    }
#endif /* HAS_SIMD */
}

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
static void atomicRmwLoad64(int64_t* shared_p, uint64_t* result)
{
    std::atomic<uint64_t>* shared = reinterpret_cast<std::atomic<uint64_t>*>(shared_p);
    *result = shared->load(std::memory_order_relaxed);
}

static void atomicRmwStore64(uint64_t* shared_p, int64_t* value)
{
    std::atomic<uint64_t>* shared = reinterpret_cast<std::atomic<uint64_t>*>(shared_p);
    shared->store(*value);
}

static void emitAtomicLoadStore64(sljit_compiler* compiler, Instruction* instr)
{
    uint32_t options = MemAddress::CheckNaturalAlignment | MemAddress::AbsoluteAddress;
    uint32_t size = 8;
    sljit_u32 offset;
    sljit_u16 memIndex = 0;

    Operand* operands = instr->operands();
    MemAddress addr(options, instr->requiredReg(0), instr->requiredReg(1), 0);

    if (instr->opcode() == ByteCode::I64AtomicLoadOpcode) {
        if (instr->info() & Instruction::kMultiMemory) {
            MemoryLoadMemIdx* loadMemIdxOperation = reinterpret_cast<MemoryLoadMemIdx*>(instr->byteCode());
            offset = loadMemIdxOperation->offset();
            memIndex = loadMemIdxOperation->memIndex();
        } else {
            MemoryLoad* loadOperation = reinterpret_cast<MemoryLoad*>(instr->byteCode());
            offset = loadOperation->offset();
        }
    } else {
        ASSERT(instr->opcode() == ByteCode::I64AtomicStoreOpcode || instr->opcode() == ByteCode::I64AtomicStoreMemIdxOpcode);
        if (instr->info() & Instruction::kMultiMemory) {
            MemoryStoreMemIdx* storeMemIdxOperation = reinterpret_cast<MemoryStoreMemIdx*>(instr->byteCode());
            offset = storeMemIdxOperation->offset();
            memIndex = storeMemIdxOperation->memIndex();
        } else {
            MemoryStore* storeOperation = reinterpret_cast<MemoryStore*>(instr->byteCode());
            offset = storeOperation->offset();
        }
    }

    addr.check(compiler, operands, offset, size, memIndex);

    if (addr.memArg.arg == 0) {
        return;
    }

    JITArgPair valueArgPair(operands + 1);
    sljit_sw stackTmpStart = CompileContext::get(compiler)->stackTmpStart;
    sljit_s32 type = SLJIT_ARGS2V(P, P);
    sljit_s32 faddr;

    if (instr->opcode() == ByteCode::I64AtomicLoadOpcode) {
        faddr = GET_FUNC_ADDR(sljit_sw, atomicRmwLoad64);
    } else {
        ASSERT(instr->opcode() == ByteCode::I64AtomicStoreOpcode);
        faddr = GET_FUNC_ADDR(sljit_sw, atomicRmwStore64);

        if (valueArgPair.arg1 != SLJIT_MEM1(kFrameReg)) {
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), stackTmpStart + WORD_LOW_OFFSET, valueArgPair.arg1, valueArgPair.arg1w);
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), stackTmpStart + WORD_HIGH_OFFSET, valueArgPair.arg2, valueArgPair.arg2w);
        }
    }

    if (valueArgPair.arg1 == SLJIT_MEM1(kFrameReg)) {
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R1, 0, kFrameReg, 0, SLJIT_IMM, valueArgPair.arg1w - WORD_LOW_OFFSET);
    } else {
        ASSERT(SLJIT_IS_REG(valueArgPair.arg1) || SLJIT_IS_IMM(valueArgPair.arg1));
        sljit_get_local_base(compiler, SLJIT_R1, 0, stackTmpStart);
    }

    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, GET_SOURCE_REG(addr.memArg.arg, instr->requiredReg(0)), 0);
    sljit_emit_icall(compiler, SLJIT_CALL, type, SLJIT_IMM, faddr);

    if ((instr->opcode() == ByteCode::I64AtomicLoadOpcode) && (valueArgPair.arg1 != SLJIT_MEM1(kFrameReg))) {
        sljit_emit_op1(compiler, SLJIT_MOV, valueArgPair.arg1, valueArgPair.arg1w, SLJIT_MEM1(SLJIT_SP), stackTmpStart + WORD_LOW_OFFSET);
        sljit_emit_op1(compiler, SLJIT_MOV, valueArgPair.arg2, valueArgPair.arg2w, SLJIT_MEM1(SLJIT_SP), stackTmpStart + WORD_HIGH_OFFSET);
    }
}
#endif /* SLJIT_32BIT_ARCHITECTURE  */

static void emitLoad(sljit_compiler* compiler, Instruction* instr)
{
    sljit_s32 opcode;
    sljit_u32 size;
    sljit_u32 offset = 0;
    uint32_t options = 0;
    sljit_u16 memIndex = 0;
#ifdef HAS_SIMD
    sljit_s32 simdType = 0;
#endif /* HAS_SIMD */

    switch (instr->opcode()) {
    case ByteCode::Load32Opcode:
        opcode = (instr->info() & Instruction::kHasFloatOperand) ? SLJIT_MOV_F32 : SLJIT_MOV32;
        size = 4;
        break;
    case ByteCode::Load64Opcode:
        opcode = (instr->info() & Instruction::kHasFloatOperand) ? SLJIT_MOV_F64 : SLJIT_MOV;
        size = 8;
        break;
    case ByteCode::I32AtomicLoadMemIdxOpcode:
    case ByteCode::I32AtomicLoadOpcode:
        options |= MemAddress::CheckNaturalAlignment;
        FALLTHROUGH;
    case ByteCode::I32LoadMemIdxOpcode:
    case ByteCode::I32LoadOpcode:
        opcode = SLJIT_MOV32;
        size = 4;
        break;
    case ByteCode::I32Load8SMemIdxOpcode:
    case ByteCode::I32Load8SOpcode:
        opcode = SLJIT_MOV32_S8;
        size = 1;
        break;
    case ByteCode::I32AtomicLoad8UMemIdxOpcode:
    case ByteCode::I32AtomicLoad8UOpcode:
        FALLTHROUGH;
    case ByteCode::I32Load8UMemIdxOpcode:
    case ByteCode::I32Load8UOpcode:
        opcode = SLJIT_MOV32_U8;
        size = 1;
        break;
    case ByteCode::I32Load16SMemIdxOpcode:
    case ByteCode::I32Load16SOpcode:
        opcode = SLJIT_MOV32_S16;
        size = 2;
        break;
    case ByteCode::I32AtomicLoad16UMemIdxOpcode:
    case ByteCode::I32AtomicLoad16UOpcode:
        options |= MemAddress::CheckNaturalAlignment;
        FALLTHROUGH;
    case ByteCode::I32Load16UMemIdxOpcode:
    case ByteCode::I32Load16UOpcode:
        opcode = SLJIT_MOV32_U16;
        size = 2;
        break;
    case ByteCode::I64AtomicLoadMemIdxOpcode:
    case ByteCode::I64AtomicLoadOpcode:
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        emitAtomicLoadStore64(compiler, instr);
        return;
#endif /* SLJIT_32BIT_ARCHITECTURE */
        options |= MemAddress::CheckNaturalAlignment;
        FALLTHROUGH;
    case ByteCode::I64LoadMemIdxOpcode:
    case ByteCode::I64LoadOpcode:
        opcode = SLJIT_MOV;
        size = 8;
        break;
    case ByteCode::I64Load8SMemIdxOpcode:
    case ByteCode::I64Load8SOpcode:
        opcode = SLJIT_MOV_S8;
        size = 1;
        break;
    case ByteCode::I64AtomicLoad8UMemIdxOpcode:
    case ByteCode::I64AtomicLoad8UOpcode:
        FALLTHROUGH;
    case ByteCode::I64Load8UMemIdxOpcode:
    case ByteCode::I64Load8UOpcode:
        opcode = SLJIT_MOV_U8;
        size = 1;
        break;
    case ByteCode::I64Load16SMemIdxOpcode:
    case ByteCode::I64Load16SOpcode:
        opcode = SLJIT_MOV_S16;
        size = 2;
        break;
    case ByteCode::I64AtomicLoad16UMemIdxOpcode:
    case ByteCode::I64AtomicLoad16UOpcode:
        options |= MemAddress::CheckNaturalAlignment;
        FALLTHROUGH;
    case ByteCode::I64Load16UMemIdxOpcode:
    case ByteCode::I64Load16UOpcode:
        opcode = SLJIT_MOV_U16;
        size = 2;
        break;
    case ByteCode::I64Load32SMemIdxOpcode:
    case ByteCode::I64Load32SOpcode:
        opcode = SLJIT_MOV_S32;
        size = 4;
        break;
    case ByteCode::I64AtomicLoad32UMemIdxOpcode:
    case ByteCode::I64AtomicLoad32UOpcode:
        options |= MemAddress::CheckNaturalAlignment;
        FALLTHROUGH;
    case ByteCode::I64Load32UMemIdxOpcode:
    case ByteCode::I64Load32UOpcode:
        opcode = SLJIT_MOV_U32;
        size = 4;
        break;
    case ByteCode::F32LoadMemIdxOpcode:
    case ByteCode::F32LoadOpcode:
        opcode = SLJIT_MOV_F32;
        size = 4;
        break;
#ifdef HAS_SIMD
    case ByteCode::V128LoadMemIdxOpcode:
    case ByteCode::V128LoadOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128;
        size = 16;
        break;
    case ByteCode::V128Load8SplatMemIdxOpcode:
    case ByteCode::V128Load8SplatOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8;
        size = 1;
        break;
    case ByteCode::V128Load16SplatMemIdxOpcode:
    case ByteCode::V128Load16SplatOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_16;
        size = 2;
        break;
    case ByteCode::V128Load32SplatMemIdxOpcode:
    case ByteCode::V128Load32SplatOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32;
        size = 4;
        break;
    case ByteCode::V128Load64SplatMemIdxOpcode:
    case ByteCode::V128Load64SplatOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        simdType |= SLJIT_SIMD_FLOAT;
#endif /* SLJIT_32BIT_ARCHITECTURE */
        size = 8;
        break;
    case ByteCode::V128Load8X8SMemIdxOpcode:
    case ByteCode::V128Load8X8SOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8 | SLJIT_SIMD_EXTEND_SIGNED | SLJIT_SIMD_EXTEND_16;
        size = 8;
        break;
    case ByteCode::V128Load8X8UMemIdxOpcode:
    case ByteCode::V128Load8X8UOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8 | SLJIT_SIMD_EXTEND_16;
        size = 8;
        break;
    case ByteCode::V128Load16X4SMemIdxOpcode:
    case ByteCode::V128Load16X4SOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_16 | SLJIT_SIMD_EXTEND_SIGNED | SLJIT_SIMD_EXTEND_32;
        size = 8;
        break;
    case ByteCode::V128Load16X4UMemIdxOpcode:
    case ByteCode::V128Load16X4UOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_16 | SLJIT_SIMD_EXTEND_32;
        size = 8;
        break;
    case ByteCode::V128Load32X2SMemIdxOpcode:
    case ByteCode::V128Load32X2SOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32 | SLJIT_SIMD_EXTEND_SIGNED | SLJIT_SIMD_EXTEND_64;
        size = 8;
        break;
    case ByteCode::V128Load32X2UMemIdxOpcode:
    case ByteCode::V128Load32X2UOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32 | SLJIT_SIMD_EXTEND_64;
        size = 8;
        break;
    case ByteCode::V128Load32ZeroMemIdxOpcode:
    case ByteCode::V128Load32ZeroOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32 | SLJIT_SIMD_LANE_ZERO;
        size = 4;
        break;
    case ByteCode::V128Load64ZeroMemIdxOpcode:
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
        ASSERT(instr->opcode() == ByteCode::F64LoadOpcode || instr->opcode() == ByteCode::F64LoadMemIdxOpcode);
        opcode = SLJIT_MOV_F64;
        size = 8;
        break;
    }

    if (instr->opcode() != ByteCode::Load32Opcode && instr->opcode() != ByteCode::Load64Opcode) {
        if (instr->info() & Instruction::kMultiMemory) {
            MemoryLoadMemIdx* loadMemIdxOperation = reinterpret_cast<MemoryLoadMemIdx*>(instr->byteCode());
            offset = loadMemIdxOperation->offset();
            memIndex = loadMemIdxOperation->memIndex();
        } else {
            MemoryLoad* loadOperation = reinterpret_cast<MemoryLoad*>(instr->byteCode());
            offset = loadOperation->offset();
        }
    }

    sljit_s32 start = 0;
#ifdef HAS_SIMD
    if (opcode == 0) {
        start = 1;
    }
#endif /* HAS_SIMD */

    Operand* operands = instr->operands();
    MemAddress addr(options, instr->requiredReg(start + 0), instr->requiredReg(start + 1), 0);

    addr.check(compiler, operands, offset, size, memIndex);

    if (addr.memArg.arg == 0) {
        return;
    }

    if (opcode == SLJIT_MOV_F64 || opcode == SLJIT_MOV_F32) {
        JITArg valueArg(operands + 1);

#if (defined SLJIT_FPU_UNALIGNED && SLJIT_FPU_UNALIGNED)
        sljit_emit_fop1(compiler, opcode, valueArg.arg, valueArg.argw, addr.memArg.arg, addr.memArg.argw);
#else /* SLJIT_FPU_UNALIGNED */
        sljit_s32 dstReg = GET_TARGET_REG(valueArg.arg, SLJIT_TMP_DEST_FREG);
        sljit_emit_fmem(compiler, opcode | SLJIT_MEM_UNALIGNED, dstReg, addr.memArg.arg, addr.memArg.argw);

        if (dstReg == SLJIT_TMP_DEST_FREG) {
            sljit_emit_fop1(compiler, opcode, valueArg.arg, valueArg.argw, SLJIT_TMP_DEST_FREG, 0);
        }
#endif /* SLJIT_FPU_UNALIGNED */
        return;
    }

#ifdef HAS_SIMD
    if (opcode == 0) {
        ASSERT((SLJIT_SIMD_EXTEND_16 >> 24) == 1);

        JITArg valueArg(operands + 1);
        sljit_s32 dstReg = GET_TARGET_REG(valueArg.arg, instr->requiredReg(0));

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
            sljit_emit_simd_mov(compiler, simdType, dstReg, valueArg.arg, valueArg.argw);
        }
        return;
    }
#endif /* HAS_SIMD */

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    if (!(opcode & SLJIT_32) && opcode != SLJIT_MOV32) {
        JITArgPair valueArgPair(operands + 1);
        sljit_s32 dstReg1 = GET_TARGET_REG(valueArgPair.arg1, instr->requiredReg(0));

        if (opcode == SLJIT_MOV) {
            sljit_s32 dstReg2 = GET_TARGET_REG(valueArgPair.arg2, instr->requiredReg(1));
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
            MOVE_FROM_REG(compiler, SLJIT_MOV, valueArgPair.arg1, valueArgPair.arg1w, dstReg1);
        } else {
            sljit_emit_op1(compiler, SLJIT_MOV, valueArgPair.arg2, valueArgPair.arg2w, SLJIT_IMM, 0);
            MOVE_FROM_REG(compiler, SLJIT_MOV, valueArgPair.arg1, valueArgPair.arg1w, dstReg1);
        }
        return;
    }
#endif /* SLJIT_32BIT_ARCHITECTURE */

    JITArg valueArg(operands + 1);
    sljit_s32 dstReg = GET_TARGET_REG(valueArg.arg, SLJIT_TMP_DEST_REG);

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
    sljit_u32 offset = 0;
    sljit_u32 options = 0;
    sljit_u16 memIndex = 0;
    sljit_s32 laneIndex = 0;

    switch (instr->opcode()) {
    case ByteCode::V128Load8LaneMemIdxOpcode:
    case ByteCode::V128Load8LaneOpcode:
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8;
        size = 1;
        break;
    case ByteCode::V128Load16LaneMemIdxOpcode:
    case ByteCode::V128Load16LaneOpcode:
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_16;
        size = 2;
        break;
    case ByteCode::V128Load32LaneMemIdxOpcode:
    case ByteCode::V128Load32LaneOpcode:
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32;
        size = 4;
        break;
    default:
        ASSERT(instr->opcode() == ByteCode::V128Load64LaneOpcode || instr->opcode() == ByteCode::V128Load64LaneMemIdxOpcode);
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE) \
    && !(defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64 | SLJIT_SIMD_FLOAT;
#else /* !SLJIT_32BIT_ARCHITECTURE || SLJIT_CONFIG_ARM_32 */
        simdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64;
#endif /* SLJIT_32BIT_ARCHITECTURE && !SLJIT_CONFIG_ARM_32 */
        size = 8;
        break;
    }

    if (instr->info() & Instruction::kMultiMemory) {
        SIMDMemoryLoadMemIdx* loadMemIdxOperation = reinterpret_cast<SIMDMemoryLoadMemIdx*>(instr->byteCode());
        offset = loadMemIdxOperation->offset();
        laneIndex = loadMemIdxOperation->index();
        memIndex = loadMemIdxOperation->memIndex();
    } else {
        SIMDMemoryLoad* loadOperation = reinterpret_cast<SIMDMemoryLoad*>(instr->byteCode());
        offset = loadOperation->offset();
        laneIndex = loadOperation->index();
    }

    Operand* operands = instr->operands();

    JITArg valueArg(operands + 2);
    sljit_s32 dstReg = GET_TARGET_REG(valueArg.arg, instr->requiredReg(0));

    JITArg initValue;
    simdOperandToArg(compiler, operands + 1, initValue, simdType, dstReg);

    MemAddress addr(options, instr->requiredReg(1), instr->requiredReg(2), 0);
    addr.check(compiler, operands, offset, size, memIndex);

    if (addr.memArg.arg == 0) {
        return;
    }

#if (defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
    if (simdType == (SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_64)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_64 | SLJIT_SIMD_ELEM_8, laneIndex == 0 ? dstReg : getHighRegister(dstReg), addr.memArg.arg, addr.memArg.argw);
    } else {
#endif /* SLJIT_CONFIG_ARM_32 */
        sljit_emit_simd_lane_mov(compiler, SLJIT_SIMD_LOAD | simdType, dstReg, laneIndex, addr.memArg.arg, addr.memArg.argw);
#if (defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
    }
#endif /* SLJIT_CONFIG_ARM_32 */

    if (SLJIT_IS_MEM(valueArg.arg)) {
        sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | simdType, dstReg, valueArg.arg, valueArg.argw);
    }
}

#endif /* HAS_SIMD */

static void emitStore(sljit_compiler* compiler, Instruction* instr)
{
    sljit_s32 opcode;
    sljit_u32 size;
    sljit_u32 offset = 0;
    sljit_u32 options = 0;
    sljit_u16 memIndex = 0;
#ifdef HAS_SIMD
    sljit_s32 simdType = 0;
    sljit_s32 laneIndex = 0;
#endif /* HAS_SIMD */

    switch (instr->opcode()) {
    case ByteCode::Store32Opcode:
        opcode = (instr->info() & Instruction::kHasFloatOperand) ? SLJIT_MOV_F32 : SLJIT_MOV32;
        size = 4;
        break;
    case ByteCode::Store64Opcode:
        opcode = (instr->info() & Instruction::kHasFloatOperand) ? SLJIT_MOV_F64 : SLJIT_MOV;
        size = 8;
        break;
    case ByteCode::I32AtomicStoreMemIdxOpcode:
    case ByteCode::I32AtomicStoreOpcode:
        options |= MemAddress::CheckNaturalAlignment;
        FALLTHROUGH;
    case ByteCode::I32StoreMemIdxOpcode:
    case ByteCode::I32StoreOpcode:
        opcode = SLJIT_MOV32;
        size = 4;
        break;
    case ByteCode::I32AtomicStore8MemIdxOpcode:
    case ByteCode::I32AtomicStore8Opcode:
        FALLTHROUGH;
    case ByteCode::I32Store8MemIdxOpcode:
    case ByteCode::I32Store8Opcode:
        opcode = SLJIT_MOV32_U8;
        size = 1;
        break;
    case ByteCode::I32AtomicStore16MemIdxOpcode:
    case ByteCode::I32AtomicStore16Opcode:
        options |= MemAddress::CheckNaturalAlignment;
        FALLTHROUGH;
    case ByteCode::I32Store16MemIdxOpcode:
    case ByteCode::I32Store16Opcode:
        opcode = SLJIT_MOV32_U16;
        size = 2;
        break;
    case ByteCode::I64AtomicStoreMemIdxOpcode:
    case ByteCode::I64AtomicStoreOpcode:
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        emitAtomicLoadStore64(compiler, instr);
        return;
#endif /* SLJIT_32BIT_ARCHITECTURE */
        options |= MemAddress::CheckNaturalAlignment;
        FALLTHROUGH;
    case ByteCode::I64StoreMemIdxOpcode:
    case ByteCode::I64StoreOpcode:
        opcode = SLJIT_MOV;
        size = 8;
        break;
    case ByteCode::I64AtomicStore8MemIdxOpcode:
    case ByteCode::I64AtomicStore8Opcode:
        FALLTHROUGH;
    case ByteCode::I64Store8MemIdxOpcode:
    case ByteCode::I64Store8Opcode:
        opcode = SLJIT_MOV_U8;
        size = 1;
        break;
    case ByteCode::I64AtomicStore16MemIdxOpcode:
    case ByteCode::I64AtomicStore16Opcode:
        options |= MemAddress::CheckNaturalAlignment;
        FALLTHROUGH;
    case ByteCode::I64Store16MemIdxOpcode:
    case ByteCode::I64Store16Opcode:
        opcode = SLJIT_MOV_U16;
        size = 2;
        break;
    case ByteCode::I64AtomicStore32MemIdxOpcode:
    case ByteCode::I64AtomicStore32Opcode:
        options |= MemAddress::CheckNaturalAlignment;
        FALLTHROUGH;
    case ByteCode::I64Store32MemIdxOpcode:
    case ByteCode::I64Store32Opcode:
        opcode = SLJIT_MOV_U32;
        size = 4;
        break;
    case ByteCode::F32StoreMemIdxOpcode:
    case ByteCode::F32StoreOpcode:
        opcode = SLJIT_MOV_F32;
        size = 4;
        break;
#ifdef HAS_SIMD
    case ByteCode::V128StoreMemIdxOpcode:
    case ByteCode::V128StoreOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_128;
        size = 16;
        break;
    case ByteCode::V128Store8LaneMemIdxOpcode:
    case ByteCode::V128Store8LaneOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8;
        size = 1;
        break;
    case ByteCode::V128Store16LaneMemIdxOpcode:
    case ByteCode::V128Store16LaneOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_16;
        size = 2;
        break;
    case ByteCode::V128Store32LaneMemIdxOpcode:
    case ByteCode::V128Store32LaneOpcode:
        opcode = 0;
        simdType = SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_32;
        size = 4;
        break;
    case ByteCode::V128Store64LaneMemIdxOpcode:
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
        ASSERT(instr->opcode() == ByteCode::F64StoreOpcode || instr->opcode() == ByteCode::F64StoreMemIdxOpcode);
        opcode = SLJIT_MOV_F64;
        size = 8;
        break;
    }

    if (instr->opcode() != ByteCode::Store32Opcode && instr->opcode() != ByteCode::Store64Opcode) {
#ifdef HAS_SIMD
        if (opcode != 0 || size == 16) {
#endif /* HAS_SIMD */
            if (instr->info() & Instruction::kMultiMemory) {
                MemoryStoreMemIdx* storeMemIdxOperation = reinterpret_cast<MemoryStoreMemIdx*>(instr->byteCode());
                offset = storeMemIdxOperation->offset();
                memIndex = storeMemIdxOperation->memIndex();
            } else {
                MemoryStore* storeOperation = reinterpret_cast<MemoryStore*>(instr->byteCode());
                offset = storeOperation->offset();
            }
#ifdef HAS_SIMD
        } else {
            if (instr->info() & Instruction::kMultiMemory) {
                SIMDMemoryStoreMemIdx* storeMemIdxOperation = reinterpret_cast<SIMDMemoryStoreMemIdx*>(instr->byteCode());
                offset = storeMemIdxOperation->offset();
                laneIndex = storeMemIdxOperation->index();
                memIndex = storeMemIdxOperation->memIndex();
            } else {
                SIMDMemoryStore* storeOperation = reinterpret_cast<SIMDMemoryStore*>(instr->byteCode());
                offset = storeOperation->offset();
                laneIndex = storeOperation->index();
            }
        }
#endif /* HAS_SIMD */
    }

    sljit_s32 start = 0;
#ifdef HAS_SIMD
    if (opcode == 0) {
        start = 1;
    }
#endif /* HAS_SIMD */

    Operand* operands = instr->operands();
    MemAddress addr(options, instr->requiredReg(start), instr->requiredReg(start + 1), instr->requiredReg(start == 0 ? 2 : 0));
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    JITArgPair valueArgPair;
#endif /* SLJIT_32BIT_ARCHITECTURE */

    if (opcode == SLJIT_MOV_F64 || opcode == SLJIT_MOV_F32) {
        floatOperandToArg(compiler, operands + 1, addr.loadArg, SLJIT_TMP_DEST_FREG);

        if (SLJIT_IS_MEM(addr.loadArg.arg)) {
            addr.options |= MemAddress::LoadFloat;

            if (opcode == SLJIT_MOV_F32) {
                addr.options |= MemAddress::Load32;
            }
        }
#ifdef HAS_SIMD
    } else if (opcode == 0) {
        VariableRef ref = operands[1];

        if (VARIABLE_TYPE(ref) != Instruction::ConstPtr) {
            addr.loadArg.set(operands + 1);
        } else {
            ASSERT(VARIABLE_GET_IMM(ref)->opcode() == ByteCode::Const128Opcode);

            addr.loadArg.arg = SLJIT_MEM0();
            addr.loadArg.argw = reinterpret_cast<sljit_sw>(reinterpret_cast<Const128*>(VARIABLE_GET_IMM(ref)->byteCode())->value());
        }

        if (SLJIT_IS_MEM(addr.loadArg.arg)) {
            addr.options |= MemAddress::LoadSimd;
        }
#endif /* HAS_SIMD */
    } else {
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        if ((opcode & SLJIT_32) || opcode == SLJIT_MOV32) {
            addr.loadArg.set(operands + 1);

            if (SLJIT_IS_MEM(addr.loadArg.arg)) {
                addr.options |= MemAddress::LoadInteger;
            }
        } else {
            valueArgPair.set(operands + 1);

            addr.loadArg.arg = valueArgPair.arg1;
            addr.loadArg.argw = valueArgPair.arg1w;

            if (SLJIT_IS_MEM(valueArgPair.arg1)) {
                addr.options |= MemAddress::LoadInteger;
            }

            if (opcode == SLJIT_MOV && !SLJIT_IS_REG(valueArgPair.arg1)) {
                addr.options |= MemAddress::DontUseOffsetReg;
            }
        }
#else /* !SLJIT_32BIT_ARCHITECTURE */
        addr.loadArg.set(operands + 1);

        if (SLJIT_IS_MEM(addr.loadArg.arg)) {
            addr.options |= MemAddress::LoadInteger;

            if (opcode == SLJIT_MOV32 || (opcode & SLJIT_32)) {
                addr.options |= MemAddress::Load32;
            }
        }
#endif /* SLJIT_32BIT_ARCHITECTURE */
    }

    addr.check(compiler, operands, offset, size, memIndex);

    if (addr.memArg.arg == 0) {
        return;
    }

    if (opcode == SLJIT_MOV_F64 || opcode == SLJIT_MOV_F32) {
        if (addr.options & MemAddress::LoadFloat) {
            addr.loadArg.arg = SLJIT_TMP_DEST_FREG;
            addr.loadArg.argw = 0;
        }

#if (defined SLJIT_FPU_UNALIGNED && SLJIT_FPU_UNALIGNED)
        sljit_emit_fop1(compiler, opcode, addr.memArg.arg, addr.memArg.argw, addr.loadArg.arg, addr.loadArg.argw);
#else /* SLJIT_FPU_UNALIGNED */
        sljit_emit_fmem(compiler, opcode | SLJIT_MEM_STORE | SLJIT_MEM_UNALIGNED, addr.loadArg.arg, addr.memArg.arg, addr.memArg.argw);
#endif /* SLJIT_FPU_UNALIGNED */
        return;
    }

#ifdef HAS_SIMD
    if (opcode == 0) {
        if (addr.options & MemAddress::LoadSimd) {
            addr.loadArg.arg = instr->requiredReg(0);
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
        sljit_s32 dstReg1 = GET_SOURCE_REG(valueArgPair.arg1, instr->requiredReg(2));
        sljit_s32 dstReg2 = GET_SOURCE_REG(valueArgPair.arg2, instr->requiredReg(1));

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

    if (addr.options & MemAddress::LoadInteger) {
        addr.loadArg.arg = instr->requiredReg(2);
        addr.loadArg.argw = 0;
    }

    // TODO: sljit_emit_mem for unaligned access
    sljit_emit_op1(compiler, opcode, addr.memArg.arg, addr.memArg.argw, addr.loadArg.arg, addr.loadArg.argw);
}


#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
static void atomicRmwAdd64(uint64_t* shared_p, uint64_t* value, uint64_t* result)
{
    std::atomic<uint64_t>* shared = reinterpret_cast<std::atomic<uint64_t>*>(shared_p);
    *result = shared->fetch_add(*value);
}

static void atomicRmwSub64(uint64_t* shared_p, uint64_t* value, uint64_t* result)
{
    std::atomic<uint64_t>* shared = reinterpret_cast<std::atomic<uint64_t>*>(shared_p);
    *result = shared->fetch_sub(*value);
}

static void atomicRmwAnd64(uint64_t* shared_p, uint64_t* value, uint64_t* result)
{
    std::atomic<uint64_t>* shared = reinterpret_cast<std::atomic<uint64_t>*>(shared_p);
    *result = shared->fetch_and(*value);
}

static void atomicRmwOr64(uint64_t* shared_p, uint64_t* value, uint64_t* result)
{
    std::atomic<uint64_t>* shared = reinterpret_cast<std::atomic<uint64_t>*>(shared_p);
    *result = shared->fetch_or(*value);
}

static void atomicRmwXor64(uint64_t* shared_p, uint64_t* value, uint64_t* result)
{
    std::atomic<uint64_t>* shared = reinterpret_cast<std::atomic<uint64_t>*>(shared_p);
    *result = shared->fetch_xor(*value);
}

static void atomicRmwXchg64(uint64_t* shared_p, uint64_t* value, uint64_t* result)
{
    std::atomic<uint64_t>* shared = reinterpret_cast<std::atomic<uint64_t>*>(shared_p);
    *result = shared->exchange(*value);
}

static void atomicRmwCmpxchg64(uint64_t* shared_p, uint64_t* expectedValue, uint64_t* value)
{
    std::atomic<uint64_t>* shared = reinterpret_cast<std::atomic<uint64_t>*>(shared_p);
    shared->compare_exchange_weak(*expectedValue, *value);
}

static void emitAtomicRmw64(sljit_compiler* compiler, Instruction* instr)
{
    uint32_t options = MemAddress::CheckNaturalAlignment | MemAddress::AbsoluteAddress;
    uint32_t size = 8;
    sljit_u32 offset;
    sljit_u16 memIndex = 0;

    Operand* operands = instr->operands();
    MemAddress addr(options, instr->requiredReg(0), instr->requiredReg(1), 0);

    if (instr->info() & Instruction::kMultiMemory) {
        AtomicRmwMemIdx* rmwMemIdxOperation = reinterpret_cast<AtomicRmwMemIdx*>(instr->byteCode());
        offset = rmwMemIdxOperation->offset();
        memIndex = rmwMemIdxOperation->memIndex();
    } else {
        AtomicRmw* rmwOperation = reinterpret_cast<AtomicRmw*>(instr->byteCode());
        offset = rmwOperation->offset();
    }

    addr.check(compiler, operands, offset, size, memIndex);

    if (addr.memArg.arg == 0) {
        return;
    }

    JITArgPair srcArgPair(operands + 1);
    JITArgPair dstArgPair(operands + 2);
    sljit_sw stackTmpStart = CompileContext::get(compiler)->stackTmpStart;
    sljit_s32 functionAddr;

    switch (instr->opcode()) {
    case ByteCode::I64AtomicRmwAddMemIdxOpcode:
    case ByteCode::I64AtomicRmwAddOpcode: {
        functionAddr = GET_FUNC_ADDR(sljit_sw, atomicRmwAdd64);
        break;
    }
    case ByteCode::I64AtomicRmwSubMemIdxOpcode:
    case ByteCode::I64AtomicRmwSubOpcode: {
        functionAddr = GET_FUNC_ADDR(sljit_sw, atomicRmwSub64);
        break;
    }
    case ByteCode::I64AtomicRmwAndMemIdxOpcode:
    case ByteCode::I64AtomicRmwAndOpcode: {
        functionAddr = GET_FUNC_ADDR(sljit_sw, atomicRmwAnd64);
        break;
    }
    case ByteCode::I64AtomicRmwOrMemIdxOpcode:
    case ByteCode::I64AtomicRmwOrOpcode: {
        functionAddr = GET_FUNC_ADDR(sljit_sw, atomicRmwOr64);
        break;
    }
    case ByteCode::I64AtomicRmwXorMemIdxOpcode:
    case ByteCode::I64AtomicRmwXorOpcode: {
        functionAddr = GET_FUNC_ADDR(sljit_sw, atomicRmwXor64);
        break;
    }
    case ByteCode::I64AtomicRmwXchgMemIdxOpcode:
    case ByteCode::I64AtomicRmwXchgOpcode: {
        functionAddr = GET_FUNC_ADDR(sljit_sw, atomicRmwXchg64);
        break;
    }
    default: {
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
    }

    if (srcArgPair.arg1 != SLJIT_MEM1(kFrameReg)) {
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), stackTmpStart + WORD_LOW_OFFSET, srcArgPair.arg1, srcArgPair.arg1w);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), stackTmpStart + WORD_HIGH_OFFSET, srcArgPair.arg2, srcArgPair.arg2w);
    }

    if (addr.memArg.arg != SLJIT_R0) {
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_EXTRACT_REG(addr.memArg.arg), 0);
    }

    if (srcArgPair.arg1 != SLJIT_MEM1(kFrameReg)) {
        sljit_get_local_base(compiler, SLJIT_R1, 0, stackTmpStart);
    } else {
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R1, 0, kFrameReg, 0, SLJIT_IMM, srcArgPair.arg1w - WORD_LOW_OFFSET);
    }

    if (dstArgPair.arg1 != SLJIT_MEM1(kFrameReg)) {
        sljit_get_local_base(compiler, SLJIT_R2, 0, stackTmpStart + 8);
    } else {
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R2, 0, kFrameReg, 0, SLJIT_IMM, dstArgPair.arg1w - WORD_LOW_OFFSET);
    }

    sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS3V(P, P, P), SLJIT_IMM, functionAddr);

    if (dstArgPair.arg1 != SLJIT_MEM1(kFrameReg)) {
        sljit_emit_op1(compiler, SLJIT_MOV, dstArgPair.arg1, dstArgPair.arg1w, SLJIT_MEM1(SLJIT_SP), stackTmpStart + 8 + WORD_LOW_OFFSET);
        sljit_emit_op1(compiler, SLJIT_MOV, dstArgPair.arg2, dstArgPair.arg2w, SLJIT_MEM1(SLJIT_SP), stackTmpStart + 8 + WORD_HIGH_OFFSET);
    }
}

static void emitAtomicRmwCmpxchg64(sljit_compiler* compiler, Instruction* instr)
{
    uint32_t options = MemAddress::CheckNaturalAlignment | MemAddress::AbsoluteAddress;
    uint32_t size = 8;
    sljit_u32 offset;
    sljit_u16 memIndex = 0;

    Operand* operands = instr->operands();
    MemAddress addr(options, instr->requiredReg(0), instr->requiredReg(1), instr->requiredReg(2));

    if (instr->info() & Instruction::kMultiMemory) {
        AtomicRmwCmpxchgMemIdx* rmwCmpxchgMemIdxOperation = reinterpret_cast<AtomicRmwCmpxchgMemIdx*>(instr->byteCode());
        offset = rmwCmpxchgMemIdxOperation->offset();
        memIndex = rmwCmpxchgMemIdxOperation->memIndex();
    } else {
        AtomicRmwCmpxchg* rmwCmpxchgOperation = reinterpret_cast<AtomicRmwCmpxchg*>(instr->byteCode());
        offset = rmwCmpxchgOperation->offset();
    }

    addr.check(compiler, operands, offset, size, memIndex);

    if (addr.memArg.arg == 0) {
        return;
    }

    JITArgPair srcExpectedArgPair(operands + 1);
    JITArgPair srcValueArgPair(operands + 2);
    JITArgPair dstArgPair(operands + 3);
    sljit_sw stackTmpStart = CompileContext::get(compiler)->stackTmpStart;
    sljit_s32 type = SLJIT_ARGS3V(P, P, P);
    sljit_s32 functionAddr = GET_FUNC_ADDR(sljit_sw, atomicRmwCmpxchg64);

    if (dstArgPair.arg1 == SLJIT_MEM1(kFrameReg)) {
        if (dstArgPair.arg1 != srcExpectedArgPair.arg1 || dstArgPair.arg1w != srcExpectedArgPair.arg1w) {
            sljit_emit_op1(compiler, SLJIT_MOV, dstArgPair.arg1, dstArgPair.arg1w, srcExpectedArgPair.arg1, srcExpectedArgPair.arg1w);
            sljit_emit_op1(compiler, SLJIT_MOV, dstArgPair.arg2, dstArgPair.arg2w, srcExpectedArgPair.arg2, srcExpectedArgPair.arg2w);
        }
    } else {
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), stackTmpStart + WORD_LOW_OFFSET, srcExpectedArgPair.arg1, srcExpectedArgPair.arg1w);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), stackTmpStart + WORD_HIGH_OFFSET, srcExpectedArgPair.arg2, srcExpectedArgPair.arg2w);
    }

    if (srcValueArgPair.arg1 != SLJIT_MEM1(kFrameReg)) {
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), stackTmpStart + 8 + WORD_LOW_OFFSET, srcValueArgPair.arg1, srcValueArgPair.arg1w);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), stackTmpStart + 8 + WORD_HIGH_OFFSET, srcValueArgPair.arg2, srcValueArgPair.arg2w);
    }

    if (addr.memArg.arg != SLJIT_R0) {
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_EXTRACT_REG(addr.memArg.arg), 0);
    }

    if (dstArgPair.arg1 == SLJIT_MEM1(kFrameReg)) {
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R1, 0, kFrameReg, 0, SLJIT_IMM, dstArgPair.arg1w - WORD_LOW_OFFSET);
    } else {
        sljit_get_local_base(compiler, SLJIT_R1, 0, stackTmpStart);
    }

    if (srcValueArgPair.arg1 == SLJIT_MEM1(kFrameReg)) {
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R2, 0, kFrameReg, 0, SLJIT_IMM, srcValueArgPair.arg1w - WORD_LOW_OFFSET);
    } else {
        sljit_get_local_base(compiler, SLJIT_R2, 0, stackTmpStart + 8);
    }

    sljit_emit_icall(compiler, SLJIT_CALL, type, SLJIT_IMM, functionAddr);

    if (dstArgPair.arg1 != SLJIT_MEM1(kFrameReg)) {
        sljit_emit_op1(compiler, SLJIT_MOV, dstArgPair.arg1, dstArgPair.arg1w, SLJIT_MEM1(SLJIT_SP), stackTmpStart + WORD_LOW_OFFSET);
        sljit_emit_op1(compiler, SLJIT_MOV, dstArgPair.arg2, dstArgPair.arg2w, SLJIT_MEM1(SLJIT_SP), stackTmpStart + WORD_HIGH_OFFSET);
    }
}

#endif /* SLJIT_32BIT_ARCHITECTURE */

#define OP_XCHG (SLJIT_OP2_BASE + 16)
#define OP_CMPXCHG (SLJIT_OP2_BASE + 17)

static void emitAtomic(sljit_compiler* compiler, Instruction* instr)
{
    CompileContext* context = CompileContext::get(compiler);
    bool noShortAtomic = !(context->compiler->options() & JITCompiler::kHasShortAtomic);
    sljit_s32 operationSize = SLJIT_MOV;
    sljit_s32 size = 0;
    sljit_s32 offset = 0;
    sljit_u16 memIndex = 0;
    sljit_s32 operation;
    uint32_t options = MemAddress::CheckNaturalAlignment | MemAddress::AbsoluteAddress;

    switch (instr->opcode()) {
    case ByteCode::I64AtomicRmwCmpxchgMemIdxOpcode:
    case ByteCode::I64AtomicRmwCmpxchgOpcode: {
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        emitAtomicRmwCmpxchg64(compiler, instr);
        return;
#else /* !SLJIT_32BIT_ARCHITECTURE */
        operationSize = SLJIT_MOV;
        size = 8;
        break;
#endif /* SLJIT_32BIT_ARCHITECTURE */
    }
    case ByteCode::I64AtomicRmwAddMemIdxOpcode:
    case ByteCode::I64AtomicRmwSubMemIdxOpcode:
    case ByteCode::I64AtomicRmwAndMemIdxOpcode:
    case ByteCode::I64AtomicRmwOrMemIdxOpcode:
    case ByteCode::I64AtomicRmwXorMemIdxOpcode:
    case ByteCode::I64AtomicRmwXchgMemIdxOpcode:
    case ByteCode::I64AtomicRmwAddOpcode:
    case ByteCode::I64AtomicRmwSubOpcode:
    case ByteCode::I64AtomicRmwAndOpcode:
    case ByteCode::I64AtomicRmwOrOpcode:
    case ByteCode::I64AtomicRmwXorOpcode:
    case ByteCode::I64AtomicRmwXchgOpcode: {
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        emitAtomicRmw64(compiler, instr);
        return;
#else /* !SLJIT_32BIT_ARCHITECTURE */
        operationSize = SLJIT_MOV;
        size = 8;
        break;
#endif /* SLJIT_32BIT_ARCHITECTURE */
    }
    case ByteCode::I32AtomicRmwAddMemIdxOpcode:
    case ByteCode::I32AtomicRmwSubMemIdxOpcode:
    case ByteCode::I32AtomicRmwAndMemIdxOpcode:
    case ByteCode::I32AtomicRmwOrMemIdxOpcode:
    case ByteCode::I32AtomicRmwXorMemIdxOpcode:
    case ByteCode::I32AtomicRmwXchgMemIdxOpcode:
    case ByteCode::I32AtomicRmwCmpxchgMemIdxOpcode:
    case ByteCode::I32AtomicRmwAddOpcode:
    case ByteCode::I32AtomicRmwSubOpcode:
    case ByteCode::I32AtomicRmwAndOpcode:
    case ByteCode::I32AtomicRmwOrOpcode:
    case ByteCode::I32AtomicRmwXorOpcode:
    case ByteCode::I32AtomicRmwXchgOpcode:
    case ByteCode::I32AtomicRmwCmpxchgOpcode: {
        operationSize = SLJIT_MOV32;
        size = 4;
        break;
    }
    case ByteCode::I64AtomicRmw32AddUMemIdxOpcode:
    case ByteCode::I64AtomicRmw32SubUMemIdxOpcode:
    case ByteCode::I64AtomicRmw32AndUMemIdxOpcode:
    case ByteCode::I64AtomicRmw32OrUMemIdxOpcode:
    case ByteCode::I64AtomicRmw32XorUMemIdxOpcode:
    case ByteCode::I64AtomicRmw32XchgUMemIdxOpcode:
    case ByteCode::I64AtomicRmw32CmpxchgUMemIdxOpcode:
    case ByteCode::I64AtomicRmw32AddUOpcode:
    case ByteCode::I64AtomicRmw32SubUOpcode:
    case ByteCode::I64AtomicRmw32AndUOpcode:
    case ByteCode::I64AtomicRmw32OrUOpcode:
    case ByteCode::I64AtomicRmw32XorUOpcode:
    case ByteCode::I64AtomicRmw32XchgUOpcode:
    case ByteCode::I64AtomicRmw32CmpxchgUOpcode: {
#if defined(SLJIT_CONFIG_RISCV) && SLJIT_CONFIG_RISCV
        operationSize = SLJIT_MOV_S32;
#else /* !SLJIT_CONFIG_RISCV */
        operationSize = SLJIT_MOV_U32;
#endif /* SLJIT_CONFIG_RISCV */
        size = 4;
        break;
    }
    case ByteCode::I32AtomicRmw8AddUMemIdxOpcode:
    case ByteCode::I32AtomicRmw8SubUMemIdxOpcode:
    case ByteCode::I32AtomicRmw8AndUMemIdxOpcode:
    case ByteCode::I32AtomicRmw8OrUMemIdxOpcode:
    case ByteCode::I32AtomicRmw8XorUMemIdxOpcode:
    case ByteCode::I32AtomicRmw8XchgUMemIdxOpcode:
    case ByteCode::I32AtomicRmw8CmpxchgUMemIdxOpcode:
    case ByteCode::I32AtomicRmw8AddUOpcode:
    case ByteCode::I32AtomicRmw8SubUOpcode:
    case ByteCode::I32AtomicRmw8AndUOpcode:
    case ByteCode::I32AtomicRmw8OrUOpcode:
    case ByteCode::I32AtomicRmw8XorUOpcode:
    case ByteCode::I32AtomicRmw8XchgUOpcode:
    case ByteCode::I32AtomicRmw8CmpxchgUOpcode: {
        operationSize = SLJIT_MOV32_U8;
        size = 1;
        options &= ~MemAddress::CheckNaturalAlignment;
        break;
    }
    case ByteCode::I64AtomicRmw8OrUMemIdxOpcode:
    case ByteCode::I64AtomicRmw8AddUMemIdxOpcode:
    case ByteCode::I64AtomicRmw8SubUMemIdxOpcode:
    case ByteCode::I64AtomicRmw8AndUMemIdxOpcode:
    case ByteCode::I64AtomicRmw8XorUMemIdxOpcode:
    case ByteCode::I64AtomicRmw8XchgUMemIdxOpcode:
    case ByteCode::I64AtomicRmw8CmpxchgUMemIdxOpcode:
    case ByteCode::I64AtomicRmw8OrUOpcode:
    case ByteCode::I64AtomicRmw8AddUOpcode:
    case ByteCode::I64AtomicRmw8SubUOpcode:
    case ByteCode::I64AtomicRmw8AndUOpcode:
    case ByteCode::I64AtomicRmw8XorUOpcode:
    case ByteCode::I64AtomicRmw8XchgUOpcode:
    case ByteCode::I64AtomicRmw8CmpxchgUOpcode: {
        operationSize = SLJIT_MOV_U8;
        size = 1;
        options &= ~MemAddress::CheckNaturalAlignment;
        break;
    }
    case ByteCode::I32AtomicRmw16AddUMemIdxOpcode:
    case ByteCode::I32AtomicRmw16SubUMemIdxOpcode:
    case ByteCode::I32AtomicRmw16AndUMemIdxOpcode:
    case ByteCode::I32AtomicRmw16OrUMemIdxOpcode:
    case ByteCode::I32AtomicRmw16XorUMemIdxOpcode:
    case ByteCode::I32AtomicRmw16XchgUMemIdxOpcode:
    case ByteCode::I32AtomicRmw16CmpxchgUMemIdxOpcode:
    case ByteCode::I32AtomicRmw16AddUOpcode:
    case ByteCode::I32AtomicRmw16SubUOpcode:
    case ByteCode::I32AtomicRmw16AndUOpcode:
    case ByteCode::I32AtomicRmw16OrUOpcode:
    case ByteCode::I32AtomicRmw16XorUOpcode:
    case ByteCode::I32AtomicRmw16XchgUOpcode:
    case ByteCode::I32AtomicRmw16CmpxchgUOpcode: {
        operationSize = SLJIT_MOV32_U16;
        size = 2;
        break;
    }
    case ByteCode::I64AtomicRmw16AddUMemIdxOpcode:
    case ByteCode::I64AtomicRmw16SubUMemIdxOpcode:
    case ByteCode::I64AtomicRmw16AndUMemIdxOpcode:
    case ByteCode::I64AtomicRmw16OrUMemIdxOpcode:
    case ByteCode::I64AtomicRmw16XorUMemIdxOpcode:
    case ByteCode::I64AtomicRmw16XchgUMemIdxOpcode:
    case ByteCode::I64AtomicRmw16CmpxchgUMemIdxOpcode:
    case ByteCode::I64AtomicRmw16AddUOpcode:
    case ByteCode::I64AtomicRmw16SubUOpcode:
    case ByteCode::I64AtomicRmw16AndUOpcode:
    case ByteCode::I64AtomicRmw16OrUOpcode:
    case ByteCode::I64AtomicRmw16XorUOpcode:
    case ByteCode::I64AtomicRmw16XchgUOpcode:
    case ByteCode::I64AtomicRmw16CmpxchgUOpcode: {
        operationSize = SLJIT_MOV_U16;
        size = 2;
        break;
    }
    default: {
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
    }

    switch (instr->opcode()) {
    case ByteCode::I32AtomicRmwAddMemIdxOpcode:
    case ByteCode::I32AtomicRmw8AddUMemIdxOpcode:
    case ByteCode::I32AtomicRmw16AddUMemIdxOpcode:
    case ByteCode::I64AtomicRmwAddMemIdxOpcode:
    case ByteCode::I64AtomicRmw8AddUMemIdxOpcode:
    case ByteCode::I64AtomicRmw16AddUMemIdxOpcode:
    case ByteCode::I64AtomicRmw32AddUMemIdxOpcode:
    case ByteCode::I32AtomicRmwAddOpcode:
    case ByteCode::I32AtomicRmw8AddUOpcode:
    case ByteCode::I32AtomicRmw16AddUOpcode:
    case ByteCode::I64AtomicRmwAddOpcode:
    case ByteCode::I64AtomicRmw8AddUOpcode:
    case ByteCode::I64AtomicRmw16AddUOpcode:
    case ByteCode::I64AtomicRmw32AddUOpcode: {
        operation = SLJIT_ADD;
        break;
    }
    case ByteCode::I32AtomicRmwSubMemIdxOpcode:
    case ByteCode::I32AtomicRmw8SubUMemIdxOpcode:
    case ByteCode::I32AtomicRmw16SubUMemIdxOpcode:
    case ByteCode::I64AtomicRmwSubMemIdxOpcode:
    case ByteCode::I64AtomicRmw8SubUMemIdxOpcode:
    case ByteCode::I64AtomicRmw16SubUMemIdxOpcode:
    case ByteCode::I64AtomicRmw32SubUMemIdxOpcode:
    case ByteCode::I32AtomicRmwSubOpcode:
    case ByteCode::I32AtomicRmw8SubUOpcode:
    case ByteCode::I32AtomicRmw16SubUOpcode:
    case ByteCode::I64AtomicRmwSubOpcode:
    case ByteCode::I64AtomicRmw8SubUOpcode:
    case ByteCode::I64AtomicRmw16SubUOpcode:
    case ByteCode::I64AtomicRmw32SubUOpcode: {
        operation = SLJIT_SUB;
        break;
    }
    case ByteCode::I32AtomicRmwAndMemIdxOpcode:
    case ByteCode::I32AtomicRmw8AndUMemIdxOpcode:
    case ByteCode::I32AtomicRmw16AndUMemIdxOpcode:
    case ByteCode::I64AtomicRmwAndMemIdxOpcode:
    case ByteCode::I64AtomicRmw8AndUMemIdxOpcode:
    case ByteCode::I64AtomicRmw16AndUMemIdxOpcode:
    case ByteCode::I64AtomicRmw32AndUMemIdxOpcode:
    case ByteCode::I32AtomicRmwAndOpcode:
    case ByteCode::I32AtomicRmw8AndUOpcode:
    case ByteCode::I32AtomicRmw16AndUOpcode:
    case ByteCode::I64AtomicRmwAndOpcode:
    case ByteCode::I64AtomicRmw8AndUOpcode:
    case ByteCode::I64AtomicRmw16AndUOpcode:
    case ByteCode::I64AtomicRmw32AndUOpcode: {
        operation = SLJIT_AND;
        break;
    }
    case ByteCode::I32AtomicRmwOrMemIdxOpcode:
    case ByteCode::I32AtomicRmw8OrUMemIdxOpcode:
    case ByteCode::I32AtomicRmw16OrUMemIdxOpcode:
    case ByteCode::I64AtomicRmwOrMemIdxOpcode:
    case ByteCode::I64AtomicRmw8OrUMemIdxOpcode:
    case ByteCode::I64AtomicRmw16OrUMemIdxOpcode:
    case ByteCode::I64AtomicRmw32OrUMemIdxOpcode:
    case ByteCode::I32AtomicRmwOrOpcode:
    case ByteCode::I32AtomicRmw8OrUOpcode:
    case ByteCode::I32AtomicRmw16OrUOpcode:
    case ByteCode::I64AtomicRmwOrOpcode:
    case ByteCode::I64AtomicRmw8OrUOpcode:
    case ByteCode::I64AtomicRmw16OrUOpcode:
    case ByteCode::I64AtomicRmw32OrUOpcode: {
        operation = SLJIT_OR;
        break;
    }
    case ByteCode::I32AtomicRmwXorMemIdxOpcode:
    case ByteCode::I32AtomicRmw8XorUMemIdxOpcode:
    case ByteCode::I32AtomicRmw16XorUMemIdxOpcode:
    case ByteCode::I64AtomicRmwXorMemIdxOpcode:
    case ByteCode::I64AtomicRmw8XorUMemIdxOpcode:
    case ByteCode::I64AtomicRmw16XorUMemIdxOpcode:
    case ByteCode::I64AtomicRmw32XorUMemIdxOpcode:
    case ByteCode::I32AtomicRmwXorOpcode:
    case ByteCode::I32AtomicRmw8XorUOpcode:
    case ByteCode::I32AtomicRmw16XorUOpcode:
    case ByteCode::I64AtomicRmwXorOpcode:
    case ByteCode::I64AtomicRmw8XorUOpcode:
    case ByteCode::I64AtomicRmw16XorUOpcode:
    case ByteCode::I64AtomicRmw32XorUOpcode: {
        operation = SLJIT_XOR;
        break;
    }
    case ByteCode::I32AtomicRmwXchgMemIdxOpcode:
    case ByteCode::I32AtomicRmw8XchgUMemIdxOpcode:
    case ByteCode::I32AtomicRmw16XchgUMemIdxOpcode:
    case ByteCode::I64AtomicRmwXchgMemIdxOpcode:
    case ByteCode::I64AtomicRmw8XchgUMemIdxOpcode:
    case ByteCode::I64AtomicRmw16XchgUMemIdxOpcode:
    case ByteCode::I64AtomicRmw32XchgUMemIdxOpcode:
    case ByteCode::I32AtomicRmwXchgOpcode:
    case ByteCode::I32AtomicRmw8XchgUOpcode:
    case ByteCode::I32AtomicRmw16XchgUOpcode:
    case ByteCode::I64AtomicRmwXchgOpcode:
    case ByteCode::I64AtomicRmw8XchgUOpcode:
    case ByteCode::I64AtomicRmw16XchgUOpcode:
    case ByteCode::I64AtomicRmw32XchgUOpcode: {
        operation = OP_XCHG;
        break;
    }
    case ByteCode::I32AtomicRmwCmpxchgMemIdxOpcode:
    case ByteCode::I32AtomicRmw8CmpxchgUMemIdxOpcode:
    case ByteCode::I32AtomicRmw16CmpxchgUMemIdxOpcode:
    case ByteCode::I64AtomicRmwCmpxchgMemIdxOpcode:
    case ByteCode::I64AtomicRmw8CmpxchgUMemIdxOpcode:
    case ByteCode::I64AtomicRmw16CmpxchgUMemIdxOpcode:
    case ByteCode::I64AtomicRmw32CmpxchgUMemIdxOpcode:
    case ByteCode::I32AtomicRmwCmpxchgOpcode:
    case ByteCode::I32AtomicRmw8CmpxchgUOpcode:
    case ByteCode::I32AtomicRmw16CmpxchgUOpcode:
    case ByteCode::I64AtomicRmwCmpxchgOpcode:
    case ByteCode::I64AtomicRmw8CmpxchgUOpcode:
    case ByteCode::I64AtomicRmw16CmpxchgUOpcode:
    case ByteCode::I64AtomicRmw32CmpxchgUOpcode: {
        operation = OP_CMPXCHG;
        break;
    }
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }

    Operand* operands = instr->operands();
    MemAddress addr(options, instr->requiredReg(0), instr->requiredReg(1), instr->requiredReg(2));

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    JITArgPair valueArgPair;
    if ((operationSize & SLJIT_32) || operationSize == SLJIT_MOV32) {
        addr.loadArg.set(operands + 1);
    } else {
        valueArgPair.set(operands + 1);

        addr.loadArg.arg = valueArgPair.arg1;
        addr.loadArg.argw = valueArgPair.arg1w;
    }

    if (SLJIT_IS_MEM(addr.loadArg.arg)) {
        addr.options |= MemAddress::LoadInteger;
    }
#else /* !SLJIT_32BIT_ARCHITECTURE */
    addr.loadArg.set(operands + 1);

    if (SLJIT_IS_MEM(addr.loadArg.arg)) {
        addr.options |= MemAddress::LoadInteger;

        if (operationSize == SLJIT_MOV32 || (operationSize & SLJIT_32)) {
            addr.options |= MemAddress::Load32;
        }
    }
#endif /* SLJIT_32BIT_ARCHITECTURE */

    if (operation != OP_CMPXCHG) {
        if (instr->info() & Instruction::kMultiMemory) {
            AtomicRmwMemIdx* atomicRmwMemIdxOperation = reinterpret_cast<AtomicRmwMemIdx*>(instr->byteCode());
            memIndex = atomicRmwMemIdxOperation->memIndex();
            offset = atomicRmwMemIdxOperation->offset();
        } else {
            offset = reinterpret_cast<AtomicRmw*>(instr->byteCode())->offset();
        }
        addr.check(compiler, operands, offset, size, memIndex);

        JITArg dst;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        JITArgPair dstPair;
        dstPair.arg2 = 0;

        if ((operationSize & SLJIT_32) || operationSize == SLJIT_MOV32) {
#endif /* SLJIT_32BIT_ARCHITECTURE */
            dst = JITArg(operands + 2);
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        } else {
            dstPair.set(operands + 2);

            dst.arg = dstPair.arg1;
            dst.argw = dstPair.arg1w;
        }
#endif /* SLJIT_32BIT_ARCHITECTURE */

        sljit_s32 srcReg = GET_SOURCE_REG(addr.loadArg.arg, instr->requiredReg(2));

        if (SLJIT_IS_IMM(addr.loadArg.arg)) {
            sljit_emit_op1(compiler, SLJIT_MOV, srcReg, 0, addr.loadArg.arg, addr.loadArg.argw);
        }

        sljit_s32 baseReg = SLJIT_EXTRACT_REG(addr.memArg.arg);
        JITArg memValue(operands + 0);
        sljit_s32 maskReg = 0;
        sljit_s32 tmpReg = (operation == OP_XCHG) ? srcReg : instr->requiredReg(1);

        if (noShortAtomic && size <= 2) {
            sljit_s32 oldSrcReg = srcReg;
            operationSize = SLJIT_MOV32;
            tmpReg = instr->requiredReg(1);
            srcReg = instr->requiredReg(2);

            sljit_emit_op2(compiler, SLJIT_AND32, tmpReg, 0, baseReg, 0, SLJIT_IMM, 0x3);
            // Multiply by 8.
            sljit_emit_op2(compiler, SLJIT_SHL32, tmpReg, 0, tmpReg, 0, SLJIT_IMM, 3);

            if (operation == SLJIT_AND) {
                // Uses double invert to set all non used bits to 1.
                sljit_emit_op2(compiler, SLJIT_XOR, srcReg, 0, oldSrcReg, 0, SLJIT_IMM, -1);
                oldSrcReg = srcReg;
            }

            if (operation != SLJIT_ADD && operation != SLJIT_SUB) {
                sljit_emit_op1(compiler, size == 1 ? SLJIT_MOV32_U8 : SLJIT_MOV32_U16, srcReg, 0, oldSrcReg, 0);
                oldSrcReg = srcReg;
            }

            sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_MEM1(SLJIT_SP), context->stackTmpStart, tmpReg, 0);
            sljit_emit_op2(compiler, SLJIT_SHL32, srcReg, 0, oldSrcReg, 0, tmpReg, 0);

            if (operation == SLJIT_ADD || operation == SLJIT_SUB || operation == OP_XCHG) {
                maskReg = instr->requiredReg(3);
                sljit_emit_op2(compiler, SLJIT_SHL32, maskReg, 0, SLJIT_IMM, size == 1 ? 0xff : 0xffff, tmpReg, 0);

                if (operation == OP_XCHG) {
                    sljit_emit_op2(compiler, SLJIT_XOR32, maskReg, 0, maskReg, 0, SLJIT_IMM, -1);
                }
            } else {
                ASSERT(operation == SLJIT_AND || operation == SLJIT_OR || operation == SLJIT_XOR);

                if (operation == SLJIT_AND) {
                    sljit_emit_op2(compiler, SLJIT_XOR32, srcReg, 0, srcReg, 0, SLJIT_IMM, -1);
                }
            }

            sljit_emit_op2(compiler, SLJIT_AND, baseReg, 0, baseReg, 0, SLJIT_IMM, ~0x3);

            if (operation != OP_XCHG) {
                operation |= SLJIT_32;
            }
        }

        struct sljit_label* restartOnFailure = sljit_emit_label(compiler);

        sljit_emit_atomic_load(compiler, operationSize, SLJIT_TMP_DEST_REG, baseReg);

        if (operation != OP_XCHG) {
            sljit_emit_op2(compiler, operation, tmpReg, 0, SLJIT_TMP_DEST_REG, 0, srcReg, 0);
        }

        if (noShortAtomic && size <= 2) {
            if (operation == SLJIT_ADD32 || operation == SLJIT_SUB32) {
                sljit_emit_op2(compiler, SLJIT_XOR32, tmpReg, 0, tmpReg, 0, SLJIT_TMP_DEST_REG, 0);
                sljit_emit_op2(compiler, SLJIT_AND32, tmpReg, 0, tmpReg, 0, maskReg, 0);
                sljit_emit_op2(compiler, SLJIT_XOR32, tmpReg, 0, tmpReg, 0, SLJIT_TMP_DEST_REG, 0);
            } else if (operation == OP_XCHG) {
                sljit_emit_op2(compiler, SLJIT_AND32, tmpReg, 0, SLJIT_TMP_DEST_REG, 0, maskReg, 0);
                sljit_emit_op2(compiler, SLJIT_OR32, tmpReg, 0, tmpReg, 0, srcReg, 0);
            }
        }

        sljit_emit_atomic_store(compiler, operationSize | SLJIT_SET_ATOMIC_STORED, tmpReg, baseReg, SLJIT_TMP_DEST_REG);

        sljit_set_label(sljit_emit_jump(compiler, SLJIT_ATOMIC_NOT_STORED), restartOnFailure);

        if (noShortAtomic && size <= 2) {
            sljit_emit_op1(compiler, SLJIT_MOV32, tmpReg, 0, SLJIT_MEM1(SLJIT_SP), context->stackTmpStart);
            sljit_emit_op2(compiler, SLJIT_LSHR32, SLJIT_TMP_DEST_REG, 0, SLJIT_TMP_DEST_REG, 0, tmpReg, 0);
            sljit_emit_op1(compiler, size == 1 ? SLJIT_MOV32_U8 : SLJIT_MOV32_U16, SLJIT_TMP_DEST_REG, 0, SLJIT_TMP_DEST_REG, 0);
        }

#if (defined(SLJIT_CONFIG_RISCV) && SLJIT_CONFIG_RISCV) && (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        if (operationSize == SLJIT_MOV_S32) {
            sljit_emit_op1(compiler, SLJIT_MOV_U32, SLJIT_TMP_DEST_REG, 0, SLJIT_TMP_DEST_REG, 0);
        }
#endif /* SLJIT_CONFIG_RISCV && SLJIT_64BIT_ARCHITECTURE */

        sljit_emit_op1(compiler, SLJIT_MOV, dst.arg, dst.argw, SLJIT_TMP_DEST_REG, 0);
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        if (dstPair.arg2 != 0) {
            sljit_emit_op1(compiler, SLJIT_MOV, dstPair.arg2, dstPair.arg2w, SLJIT_IMM, 0);
        }
#endif /* SLJIT_32BIT_ARCHITECTURE */
        return;
    }

    if (instr->info() & Instruction::kMultiMemory) {
        AtomicRmwCmpxchgMemIdx* atomicRmwCmpxchgMemIdxOperation = reinterpret_cast<AtomicRmwCmpxchgMemIdx*>(instr->byteCode());
        memIndex = atomicRmwCmpxchgMemIdxOperation->memIndex();
        offset = atomicRmwCmpxchgMemIdxOperation->offset();
    } else {
        offset = reinterpret_cast<AtomicRmwCmpxchg*>(instr->byteCode())->offset();
    }
    addr.check(compiler, operands, offset, size, memIndex);

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    JITArg srcExpected, srcValue, dst;
    JITArgPair dstPair, srcExpectedPair, srcValuePair;
    sljit_s32 srcExpectedReg, srcValueReg, maskReg, oldSrcExpectedReg;

    struct sljit_jump* compareUpperHalfFalse;
    struct sljit_jump* compareTopFalse;
    struct sljit_jump* storeSuccess;
    struct sljit_jump* earlyReturn;

    bool is32BitOp = (operationSize & SLJIT_32) || operationSize == SLJIT_MOV32;

    if (is32BitOp) {
        srcExpected = JITArg(operands + 1);
        srcValue = JITArg(operands + 2);
        dst = JITArg(operands + 3);

        srcExpectedReg = GET_SOURCE_REG(srcExpected.arg, instr->requiredReg(2));
        srcValueReg = GET_SOURCE_REG(srcValue.arg, instr->requiredReg(1));
    } else {
        srcExpectedPair = JITArgPair(operands + 1);
        srcExpectedReg = GET_SOURCE_REG(srcExpectedPair.arg1, instr->requiredReg(2));

        srcValuePair = JITArgPair(operands + 2);
        srcValue.arg = srcValuePair.arg1;
        srcValue.argw = srcValuePair.arg1w;
        srcValueReg = GET_SOURCE_REG(srcValue.arg, instr->requiredReg(1));

        dstPair = JITArgPair(operands + 3);
        dst.arg = dstPair.arg1;
        dst.argw = dstPair.arg1w;
    }

    maskReg = SLJIT_TMP_DEST_REG;
    oldSrcExpectedReg = srcExpectedReg;

    MOVE_TO_REG(compiler, is32BitOp ? SLJIT_MOV32 : SLJIT_MOV, srcValueReg, srcValue.arg, srcValue.argw);

    if (SLJIT_IS_IMM(addr.loadArg.arg)) {
        sljit_emit_op1(compiler, SLJIT_MOV, srcExpectedReg, 0, addr.loadArg.arg, addr.loadArg.argw);
    }

    sljit_s32 baseReg = SLJIT_EXTRACT_REG(addr.memArg.arg);

    if (noShortAtomic && size <= 2) {
        sljit_s32 oldSrcValueReg = srcValueReg;
        operationSize = SLJIT_MOV32;

        srcValueReg = instr->requiredReg(1);
        srcExpectedReg = instr->requiredReg(2);
        maskReg = instr->requiredReg(3);

        sljit_emit_op1(compiler, SLJIT_MOV32, srcExpectedReg, 0, oldSrcExpectedReg, 0);

        sljit_emit_op2(compiler, SLJIT_AND, maskReg, 0, baseReg, 0, SLJIT_IMM, 0x3);
        sljit_emit_op2(compiler, SLJIT_SHL32, maskReg, 0, maskReg, 0, SLJIT_IMM, 3);

        sljit_emit_op1(compiler, size == 1 ? SLJIT_MOV32_U8 : SLJIT_MOV32_U16, srcValueReg, 0, oldSrcValueReg, 0);

        sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_MEM1(SLJIT_SP), context->stackTmpStart, maskReg, 0);
        sljit_emit_op2(compiler, SLJIT_SHL32, srcValueReg, 0, srcValueReg, 0, maskReg, 0);
    }

    if (noShortAtomic && size <= 2) {
        // if this is true, abort after the load, and return the loaded return value
        compareTopFalse = sljit_emit_cmp(compiler, SLJIT_LESS_EQUAL, srcExpectedReg, 0, SLJIT_IMM, size == 1 ? 0xff : 0xffff);
        sljit_emit_op1(compiler, SLJIT_MOV32, srcValueReg, 0, SLJIT_IMM, 0xffffffff); // 0xffffffff here is outside the possible range of the normal operation, used as a flag to return early
        sljit_set_label(compareTopFalse, sljit_emit_label(compiler));

        sljit_emit_op2(compiler, SLJIT_SHL32, srcExpectedReg, 0, srcExpectedReg, 0, maskReg, 0);

        sljit_emit_op2(compiler, SLJIT_AND, baseReg, 0, baseReg, 0, SLJIT_IMM, ~0x3);
    }

    struct sljit_label* restartOnFailure = sljit_emit_label(compiler);

    if (noShortAtomic && size <= 2) {
        sljit_emit_op1(compiler, SLJIT_MOV32, maskReg, 0, SLJIT_MEM1(SLJIT_SP), context->stackTmpStart);
        sljit_emit_op2(compiler, SLJIT_SHL32, maskReg, 0, SLJIT_IMM, size == 1 ? 0xff : 0xffff, maskReg, 0);
    }

    sljit_emit_atomic_load(compiler, operationSize, SLJIT_TMP_DEST_REG, baseReg);

    if (!is32BitOp && srcExpectedPair.arg2 != 0) {
        compareUpperHalfFalse = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, srcExpectedPair.arg2, srcExpectedPair.arg2w, SLJIT_IMM, 0);
    }

    if (noShortAtomic && size <= 2) {
        earlyReturn = sljit_emit_cmp(compiler, SLJIT_EQUAL, srcValueReg, 0, SLJIT_IMM, 0xffffffff);
    }

    if (noShortAtomic && size <= 2) {
        sljit_emit_op2(compiler, SLJIT_AND32, maskReg, 0, maskReg, 0, SLJIT_TMP_DEST_REG, 0);
    }

    sljit_s32 compare = SLJIT_NOT_EQUAL | (operationSize & SLJIT_32) | (operationSize == SLJIT_MOV32 ? SLJIT_32 : 0);
    struct sljit_jump* compareFalse = sljit_emit_cmp(compiler, compare, maskReg, 0, srcExpectedReg, 0);

    if (noShortAtomic && size <= 2) {
        sljit_emit_op2(compiler, SLJIT_XOR32, maskReg, 0, maskReg, 0, srcValueReg, 0);
        sljit_emit_op2(compiler, SLJIT_XOR32, maskReg, 0, maskReg, 0, SLJIT_TMP_DEST_REG, 0);
        srcValueReg = maskReg;
    }

    sljit_emit_atomic_store(compiler, operationSize | SLJIT_SET_ATOMIC_STORED, srcValueReg, baseReg, SLJIT_TMP_DEST_REG);
    sljit_set_label(sljit_emit_jump(compiler, SLJIT_ATOMIC_NOT_STORED), restartOnFailure);

    if ((noShortAtomic && size <= 2) || (!is32BitOp && srcExpectedPair.arg2 != 0)) {
        storeSuccess = sljit_emit_jump(compiler, SLJIT_ATOMIC_STORED);
    }

    if (noShortAtomic && size <= 2) {
        sljit_set_label(earlyReturn, sljit_emit_label(compiler));
    }

    if (!is32BitOp && srcExpectedPair.arg2 != 0) {
        sljit_set_label(compareUpperHalfFalse, sljit_emit_label(compiler));
    }

    sljit_set_label(compareFalse, sljit_emit_label(compiler));
    if ((noShortAtomic && size <= 2) || (!is32BitOp && srcExpectedPair.arg2 != 0)) {
        sljit_set_label(storeSuccess, sljit_emit_label(compiler));
    }

    if (size <= 2) {
        if (noShortAtomic) {
            sljit_emit_op1(compiler, SLJIT_MOV32, maskReg, 0, SLJIT_MEM1(SLJIT_SP), context->stackTmpStart);
            sljit_emit_op2(compiler, SLJIT_LSHR32, SLJIT_TMP_DEST_REG, 0, SLJIT_TMP_DEST_REG, 0, maskReg, 0);
        }
#if (defined(SLJIT_CONFIG_RISCV) && SLJIT_CONFIG_RISCV)
        sljit_emit_op1(compiler, size == 1 ? SLJIT_MOV32_U8 : SLJIT_MOV32_U16, SLJIT_TMP_DEST_REG, 0, SLJIT_TMP_DEST_REG, 0);
#endif /* SLJIT_CONFIG_RISCV */
    }
    sljit_emit_op1(compiler, SLJIT_MOV, dst.arg, dst.argw, SLJIT_TMP_DEST_REG, 0);
    if (!is32BitOp && dstPair.arg2 != 0) {
        sljit_emit_op1(compiler, SLJIT_MOV, dstPair.arg2, dstPair.arg2w, SLJIT_IMM, 0);
    }
#else /* !SLJIT_32BIT_ARCHITECTURE */
    JITArg srcExpected(operands + 1);
    JITArg srcValue(operands + 2);
    JITArg dst(operands + 3);

    sljit_s32 srcExpectedReg = GET_SOURCE_REG(srcExpected.arg, instr->requiredReg(2));
    sljit_s32 srcValueReg = GET_SOURCE_REG(srcValue.arg, instr->requiredReg(1));
    sljit_s32 maskReg = SLJIT_TMP_DEST_REG;
    sljit_s32 oldSrcExpectedReg = srcExpectedReg;
    struct sljit_jump* compareTopFalse;
    struct sljit_jump* storeSuccess;

    MOVE_TO_REG(compiler, (operationSize & SLJIT_32) || operationSize == SLJIT_MOV32 ? SLJIT_MOV32 : SLJIT_MOV, srcValueReg, srcValue.arg, srcValue.argw);

    if (SLJIT_IS_IMM(addr.loadArg.arg)) {
        sljit_emit_op1(compiler, SLJIT_MOV, srcExpectedReg, 0, addr.loadArg.arg, addr.loadArg.argw);
    }

    sljit_s32 baseReg = SLJIT_EXTRACT_REG(addr.memArg.arg);

    if (noShortAtomic && size <= 2) {
        sljit_s32 oldSrcValueReg = srcValueReg;
        operationSize = SLJIT_MOV32;

        srcValueReg = instr->requiredReg(1);
        srcExpectedReg = instr->requiredReg(2);
        maskReg = instr->requiredReg(3);

        sljit_emit_op2(compiler, SLJIT_AND, maskReg, 0, baseReg, 0, SLJIT_IMM, 0x3);
        sljit_emit_op2(compiler, SLJIT_SHL32, maskReg, 0, maskReg, 0, SLJIT_IMM, 3);

        sljit_emit_op1(compiler, size == 1 ? SLJIT_MOV32_U8 : SLJIT_MOV32_U16, srcValueReg, 0, oldSrcValueReg, 0);

        sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_MEM1(SLJIT_SP), context->stackTmpStart, maskReg, 0);
        sljit_emit_op2(compiler, SLJIT_SHL32, srcValueReg, 0, srcValueReg, 0, maskReg, 0);

        sljit_emit_op1(compiler, size == 1 ? SLJIT_MOV32_U8 : SLJIT_MOV32_U16, SLJIT_TMP_DEST_REG, 0, SLJIT_IMM, size == 1 ? 0xff : 0xffff);
        compareTopFalse = sljit_emit_cmp(compiler, SLJIT_GREATER, oldSrcExpectedReg, 0, SLJIT_TMP_DEST_REG, 0);

        sljit_emit_op2(compiler, SLJIT_SHL32, srcExpectedReg, 0, oldSrcExpectedReg, 0, maskReg, 0);

        sljit_emit_op2(compiler, SLJIT_AND, baseReg, 0, baseReg, 0, SLJIT_IMM, ~0x3);
    }

    struct sljit_label* restartOnFailure = sljit_emit_label(compiler);

    if (noShortAtomic && size <= 2) {
        sljit_emit_op1(compiler, SLJIT_MOV32, maskReg, 0, SLJIT_MEM1(SLJIT_SP), context->stackTmpStart);
        sljit_emit_op2(compiler, SLJIT_SHL32, maskReg, 0, SLJIT_IMM, size == 1 ? 0xff : 0xffff, maskReg, 0);
    }

    sljit_emit_atomic_load(compiler, operationSize, SLJIT_TMP_DEST_REG, baseReg);

#if (defined(SLJIT_CONFIG_RISCV) && SLJIT_CONFIG_RISCV) && (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
    if (operationSize == SLJIT_MOV_S32) {
        sljit_emit_op1(compiler, SLJIT_MOV_U32, SLJIT_TMP_DEST_REG, 0, SLJIT_TMP_DEST_REG, 0);
    }
#endif /* SLJIT_CONFIG_RISCV && SLJIT_64BIT_ARCHITECTURE */

    if (noShortAtomic && size <= 2) {
        sljit_emit_op2(compiler, SLJIT_AND32, maskReg, 0, maskReg, 0, SLJIT_TMP_DEST_REG, 0);
    }

    sljit_s32 compare = SLJIT_NOT_EQUAL | (operationSize & SLJIT_32) | (operationSize == SLJIT_MOV32 ? SLJIT_32 : 0);
    struct sljit_jump* compareFalse = sljit_emit_cmp(compiler, compare, maskReg, 0, srcExpectedReg, 0);

    if (noShortAtomic && size <= 2) {
        sljit_emit_op2(compiler, SLJIT_XOR32, maskReg, 0, maskReg, 0, srcValueReg, 0);
        sljit_emit_op2(compiler, SLJIT_XOR32, maskReg, 0, maskReg, 0, SLJIT_TMP_DEST_REG, 0);
        srcValueReg = maskReg;
    }

    sljit_emit_atomic_store(compiler, operationSize | SLJIT_SET_ATOMIC_STORED, srcValueReg, baseReg, SLJIT_TMP_DEST_REG);
    sljit_set_label(sljit_emit_jump(compiler, SLJIT_ATOMIC_NOT_STORED), restartOnFailure);

    if (noShortAtomic && size <= 2) {
        storeSuccess = sljit_emit_jump(compiler, SLJIT_ATOMIC_STORED);

        sljit_set_label(compareTopFalse, sljit_emit_label(compiler));
        sljit_emit_op2(compiler, SLJIT_AND, baseReg, 0, baseReg, 0, SLJIT_IMM, ~0x3);
        sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(baseReg), 0);
    }

    sljit_set_label(compareFalse, sljit_emit_label(compiler));

    if (noShortAtomic && size <= 2) {
        sljit_set_label(storeSuccess, sljit_emit_label(compiler));
        sljit_emit_op1(compiler, SLJIT_MOV32, maskReg, 0, SLJIT_MEM1(SLJIT_SP), context->stackTmpStart);
        sljit_emit_op2(compiler, SLJIT_LSHR32, SLJIT_TMP_DEST_REG, 0, SLJIT_TMP_DEST_REG, 0, maskReg, 0);
        sljit_emit_op1(compiler, size == 1 ? SLJIT_MOV32_U8 : SLJIT_MOV32_U16, SLJIT_TMP_DEST_REG, 0, SLJIT_TMP_DEST_REG, 0);
    }

    sljit_emit_op1(compiler, SLJIT_MOV, dst.arg, dst.argw, SLJIT_TMP_DEST_REG, 0);
#endif /* SLJIT_32BIT_ARCHITECTURE */
}

#undef OP_XCHG
#undef OP_CMPXCHG

static sljit_s32 atomicWaitCallback(ExecutionContext* context, uint8_t* address, int64_t* args, sljit_s32 size)
{
    Instance* instance = context->instance;

    if (!instance->memory(0)->isShared()) {
        return ExecutionContext::ExpectedSharedMemError;
    }

    uint32_t result = 0;
    int64_t expect = args[0];
    int64_t timeout = args[1];

    if (size == 8) {
        instance->memory(0)->atomicWait(context->state, instance->module()->store(), address, expect, timeout, &result);
    } else {
        instance->memory(0)->atomicWait(context->state, instance->module()->store(), address, (int32_t)expect, timeout, &result);
    }

    args[0] = result;
    return ExecutionContext::NoError;
}

static void emitAtomicWait(sljit_compiler* compiler, Instruction* instr)
{
    uint32_t options = MemAddress::CheckNaturalAlignment | MemAddress::AbsoluteAddress;
    sljit_s32 size = 0;
    sljit_s32 offset = 0;
    sljit_u16 memIndex = 0;

    CompileContext* context = CompileContext::get(compiler);
    sljit_sw stackTmpStart = context->stackTmpStart;

    switch (instr->opcode()) {
    case ByteCode::MemoryAtomicWait64MemIdxOpcode:
    case ByteCode::MemoryAtomicWait64Opcode: {
        size = 8;
        break;
    }
    case ByteCode::MemoryAtomicWait32MemIdxOpcode:
    case ByteCode::MemoryAtomicWait32Opcode: {
        size = 4;
        break;
    }
    default: {
        ASSERT_NOT_REACHED();
        break;
    }
    }

    if (instr->info() & Instruction::kMultiMemory) {
        ByteCodeOffset4ValueMemIdx* atomicWaitMemIdxOperation = reinterpret_cast<ByteCodeOffset4ValueMemIdx*>(instr->byteCode());
        offset = atomicWaitMemIdxOperation->offset();
        memIndex = atomicWaitMemIdxOperation->memIndex();
    } else {
        ByteCodeOffset4Value* atomicWaitOperation = reinterpret_cast<ByteCodeOffset4Value*>(instr->byteCode());
        offset = atomicWaitOperation->offset();
    }

    Operand* operands = instr->operands();
    MemAddress addr(options, instr->requiredReg(0), instr->requiredReg(1), 0);
    addr.check(compiler, operands, offset, size, memIndex);

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    JITArgPair expectedPair;
#endif /* SLJIT_32BIT_ARCHITECTURE */
    JITArg expected;

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    if (instr->opcode() == ByteCode::MemoryAtomicWait64Opcode || instr->opcode() == ByteCode::MemoryAtomicWait64MemIdxOpcode) {
        expectedPair = JITArgPair(operands + 1);
    } else {
        expected = JITArg(operands + 1);
    }
    JITArgPair timeout(operands + 2);
#else /* !SLJIT_32BIT_ARCHITECTURE */
    expected = JITArg(operands + 1);
    JITArg timeout(operands + 2);
#endif /* SLJIT_32BIT_ARCHITECTURE */
    JITArg dst(operands + 3);

    struct sljit_jump* memoryShared;

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    if (instr->opcode() == ByteCode::MemoryAtomicWait64Opcode || instr->opcode() == ByteCode::MemoryAtomicWait64MemIdxOpcode) {
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), stackTmpStart + WORD_LOW_OFFSET, expectedPair.arg1, expectedPair.arg1w);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), stackTmpStart + WORD_HIGH_OFFSET, expectedPair.arg2, expectedPair.arg2w);
    } else {
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), stackTmpStart, expected.arg, expected.argw);
    }
#else /* !SLJIT_32BIT_ARCHITECTURE */
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), stackTmpStart, expected.arg, expected.argw);
#endif /* SLJIT_32BIT_ARCHITECTURE */

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), stackTmpStart + 8 + WORD_LOW_OFFSET, timeout.arg1, timeout.arg1w);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), stackTmpStart + 8 + WORD_HIGH_OFFSET, timeout.arg2, timeout.arg2w);
#else /* !SLJIT_32BIT_ARCHITECTURE */
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), stackTmpStart + 8, timeout.arg, timeout.argw);
#endif /* SLJIT_32BIT_ARCHITECTURE */

    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_EXTRACT_REG(addr.memArg.arg), 0);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_MEM1(SLJIT_SP), kContextOffset);
    sljit_get_local_base(compiler, SLJIT_R2, 0, stackTmpStart);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R3, 0, SLJIT_IMM, size);

    sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(W, P, W, W, W), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, atomicWaitCallback));

    memoryShared = sljit_emit_cmp(compiler, SLJIT_EQUAL, SLJIT_IMM, ExecutionContext::NoError, SLJIT_R0, 0);
    context->appendTrapJump(ExecutionContext::ExpectedSharedMemError, sljit_emit_jump(compiler, SLJIT_JUMP));
    sljit_set_label(memoryShared, sljit_emit_label(compiler));

    sljit_emit_op1(compiler, SLJIT_MOV, dst.arg, dst.argw, SLJIT_MEM1(SLJIT_SP), stackTmpStart + WORD_LOW_OFFSET);
}

static sljit_s32 atomicNotifyCallback(Instance* instance, uint8_t* address, int32_t count)
{
    uint32_t result = 0;
    instance->memory(0)->atomicNotify(instance->module()->store(), address, count, &result);
    return result;
}

static void emitAtomicNotify(sljit_compiler* compiler, Instruction* instr)
{
    uint32_t options = MemAddress::CheckNaturalAlignment | MemAddress::AbsoluteAddress;
    sljit_s32 offset = 0;
    sljit_u16 memIndex = 0;

    if (instr->opcode() == ByteCode::MemoryAtomicNotifyMemIdxOpcode) {
        MemoryAtomicNotifyMemIdx* atomicNotifyMemIdxOperation = reinterpret_cast<MemoryAtomicNotifyMemIdx*>(instr->byteCode());
        offset = atomicNotifyMemIdxOperation->offset();
        memIndex = atomicNotifyMemIdxOperation->memIndex();
    } else {
        MemoryAtomicNotify* atomicNotifyOperation = reinterpret_cast<MemoryAtomicNotify*>(instr->byteCode());
        offset = atomicNotifyOperation->offset();
    }

    Operand* operands = instr->operands();
    MemAddress addr(options, instr->requiredReg(0), instr->requiredReg(1), 0);
    addr.check(compiler, operands, offset, 4, memIndex);

    JITArg count(operands + 1);
    JITArg dst(operands + 2);

    if (count.arg == SLJIT_R1) {
        if (SLJIT_EXTRACT_REG(addr.memArg.arg) != SLJIT_R2) {
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, count.arg, count.argw);
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_EXTRACT_REG(addr.memArg.arg), 0);
        } else {
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_TMP_DEST_REG, 0, SLJIT_R1, 0);
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_R2, 0);
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_TMP_DEST_REG, 0);
        }
    } else {
        MOVE_TO_REG(compiler, SLJIT_MOV, SLJIT_R1, SLJIT_EXTRACT_REG(addr.memArg.arg), 0);
        MOVE_TO_REG(compiler, SLJIT_MOV, SLJIT_R2, count.arg, count.argw);
    }

    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, kInstanceReg, 0);
    sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS3(W, P, W, 32), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, atomicNotifyCallback));

    MOVE_FROM_REG(compiler, SLJIT_MOV, dst.arg, dst.argw, SLJIT_R0);
}
