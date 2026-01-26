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

static sljit_sw initMemory(uint32_t dstStart, uint32_t srcStart, uint32_t srcSize, MemoryInitArguments* args)
{
    Memory* memory = args->memory;
    DataSegment* data = args->data;

    if (!memory->checkAccess(dstStart, srcSize)) {
        return ExecutionContext::OutOfBoundsMemAccessError;
    }

    if (srcStart >= data->sizeInByte() || srcStart + srcSize > data->sizeInByte()) {
        return ExecutionContext::OutOfBoundsMemAccessError;
    }

    memory->initMemory(data, dstStart, srcStart, srcSize);
    return ExecutionContext::NoError;
}

static sljit_sw initMemoryM64(sljit_uw dstStart, uint32_t srcStart, uint32_t srcSize, MemoryInitArguments* args)
{
    Memory* memory = args->memory;
    DataSegment* data = args->data;

    if (!memory->checkAccessM64(dstStart, srcSize)) {
        return ExecutionContext::OutOfBoundsMemAccessError;
    }

    if (srcStart >= data->sizeInByte() || srcStart + srcSize > data->sizeInByte()) {
        return ExecutionContext::OutOfBoundsMemAccessError;
    }

    memory->initMemory(data, dstStart, srcStart, srcSize);
    return ExecutionContext::NoError;
}

static sljit_sw copyMemory(uint32_t dstStart, uint32_t srcStart, uint32_t size, MemoryCopyArguments* args)
{
    Memory* srcMemory = args->srcMemory;
    Memory* dstMemory = args->dstMemory;

    if (!srcMemory->checkAccess(srcStart, size) || !dstMemory->checkAccess(dstStart, size)) {
        return ExecutionContext::OutOfBoundsMemAccessError;
    }

    srcMemory->copyMemory(dstMemory, dstStart, srcStart, size);
    return ExecutionContext::NoError;
}

static sljit_sw copyMemoryM64(sljit_uw dstStart, sljit_uw srcStart, sljit_uw size, MemoryCopyArguments* args)
{
    Memory* srcMemory = args->srcMemory;
    Memory* dstMemory = args->dstMemory;

    if (!srcMemory->checkAccessM64(srcStart, size) || !dstMemory->checkAccessM64(dstStart, size)) {
        return ExecutionContext::OutOfBoundsMemAccessError;
    }

    srcMemory->copyMemory(dstMemory, dstStart, srcStart, size);
    return ExecutionContext::NoError;
}

static sljit_sw copyMemoryM64M32(sljit_uw dstStart, uint32_t srcStart, uint32_t size, MemoryCopyArguments* args)
{
    Memory* srcMemory = args->srcMemory;
    Memory* dstMemory = args->dstMemory;

    if (!srcMemory->checkAccess(srcStart, size) || !dstMemory->checkAccessM64(dstStart, size)) {
        return ExecutionContext::OutOfBoundsMemAccessError;
    }

    srcMemory->copyMemory(dstMemory, dstStart, srcStart, size);
    return ExecutionContext::NoError;
}

static sljit_sw copyMemoryM32M64(uint32_t dstStart, sljit_uw srcStart, uint32_t size, MemoryCopyArguments* args)
{
    Memory* srcMemory = args->srcMemory;
    Memory* dstMemory = args->dstMemory;

    if (!srcMemory->checkAccessM64(srcStart, size) || !dstMemory->checkAccess(dstStart, size)) {
        return ExecutionContext::OutOfBoundsMemAccessError;
    }

    srcMemory->copyMemory(dstMemory, dstStart, srcStart, size);
    return ExecutionContext::NoError;
}

static sljit_sw fillMemory(uint32_t start, uint32_t value, uint32_t size, Memory* memory)
{
    if (!memory->checkAccess(start, size)) {
        return ExecutionContext::OutOfBoundsMemAccessError;
    }

    memory->fillMemory(start, value, size);
    return ExecutionContext::NoError;
}

static sljit_sw fillMemoryM64(sljit_uw start, uint32_t value, sljit_uw size, Memory* memory)
{
    if (!memory->checkAccessM64(start, size)) {
        return ExecutionContext::OutOfBoundsMemAccessError;
    }

    memory->fillMemory(start, value, size);
    return ExecutionContext::NoError;
}

static sljit_s32 growMemory(uint32_t newSize, Instance* instance, uint16_t memIndex)
{
    Memory* memory = instance->memory(memIndex);
    uint32_t oldSize = memory->sizeInPageSize();

    if (memory->grow(static_cast<uint64_t>(newSize) * Memory::s_memoryPageSize)) {
        return static_cast<sljit_s32>(oldSize);
    }

    return -1;
}

static sljit_uw growMemoryM64(sljit_uw newSize, Instance* instance, uint16_t memIndex)
{
    Memory* memory = instance->memory(memIndex);
    uint32_t oldSize = memory->sizeInPageSize();

    if (static_cast<uint64_t>(newSize) < Memory::s_maxMemory64Grow && memory->grow(static_cast<uint64_t>(newSize) * Memory::s_memoryPageSize)) {
        return static_cast<sljit_s32>(oldSize);
    }

    return -1;
}

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
static void memoryParamToArg(sljit_compiler* compiler, CompileContext* context, Operand* param, JITArg* arg)
{
    JITArgPair argPair(param);

    context->appendTrapJump(ExecutionContext::OutOfBoundsMemAccessError,
                            sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, argPair.arg2, argPair.arg2w, SLJIT_IMM, 0));

    arg->arg = argPair.arg1;
    arg->argw = argPair.arg1w;
}

#endif /* SLJIT_32BIT_ARCHITECTURE */

static void emitMemory(sljit_compiler* compiler, Instruction* instr)
{
    CompileContext* context = CompileContext::get(compiler);
    Operand* params = instr->operands();
    ByteCode::Opcode opcode = instr->opcode();

    switch (opcode) {
    case ByteCode::MemorySizeOpcode: {
        ASSERT(!(instr->info() & Instruction::kIsCallback));

        uint16_t memIndex = reinterpret_cast<ByteCodeOffsetMemIndex*>(instr->byteCode())->memIndex();
        JITArg dstArg(params);

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        /* The sizeInByte is always a 32 bit number on 32 bit systems. */
        sljit_emit_op2(compiler, SLJIT_LSHR, dstArg.arg, dstArg.argw, SLJIT_MEM1(kInstanceReg),
                       context->targetBuffersStart + offsetof(Memory::TargetBuffer, sizeInByte) + memIndex * sizeof(Memory::TargetBuffer) + WORD_LOW_OFFSET,
                       SLJIT_IMM, 16);
#else /* !SLJIT_32BIT_ARCHITECTURE */
        sljit_emit_op2(compiler, SLJIT_LSHR, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(kInstanceReg),
                       context->targetBuffersStart + offsetof(Memory::TargetBuffer, sizeInByte) + memIndex * sizeof(Memory::TargetBuffer),
                       SLJIT_IMM, 16);
        sljit_emit_op1(compiler, SLJIT_MOV_U32, dstArg.arg, dstArg.argw, SLJIT_TMP_DEST_REG, 0);
#endif /* SLJIT_32BIT_ARCHITECTURE */
        return;
    }
    case ByteCode::MemorySizeM64Opcode: {
        ASSERT(!(instr->info() & Instruction::kIsCallback));

        uint16_t memIndex = reinterpret_cast<ByteCodeOffsetMemIndex*>(instr->byteCode())->memIndex();

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        JITArgPair dstArgPair(params);
        /* The sizeInByte is always a 32 bit number on 32 bit systems. */
        sljit_emit_op2(compiler, SLJIT_LSHR, dstArgPair.arg1, dstArgPair.arg1w, SLJIT_MEM1(kInstanceReg),
                       context->targetBuffersStart + offsetof(Memory::TargetBuffer, sizeInByte) + memIndex * sizeof(Memory::TargetBuffer) + WORD_LOW_OFFSET,
                       SLJIT_IMM, 16);
        sljit_emit_op1(compiler, SLJIT_MOV, dstArgPair.arg2, dstArgPair.arg2w, SLJIT_IMM, 0);
#else /* !SLJIT_32BIT_ARCHITECTURE */
        JITArg dstArg(params);
        sljit_emit_op2(compiler, SLJIT_LSHR, dstArg.arg, dstArg.argw, SLJIT_MEM1(kInstanceReg),
                       context->targetBuffersStart + offsetof(Memory::TargetBuffer, sizeInByte) + memIndex * sizeof(Memory::TargetBuffer),
                       SLJIT_IMM, 16);
#endif /* SLJIT_32BIT_ARCHITECTURE */
        return;
    }
    case ByteCode::MemoryInitOpcode:
    case ByteCode::MemoryInitM64Opcode:
    case ByteCode::MemoryFillOpcode:
    case ByteCode::MemoryFillM64Opcode:
    case ByteCode::MemoryCopyOpcode:
    case ByteCode::MemoryCopyM64Opcode:
    case ByteCode::MemoryCopyM64M32Opcode:
    case ByteCode::MemoryCopyM32M64Opcode: {
        ASSERT(instr->info() & Instruction::kIsCallback);

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        JITArg src[3];

        switch (opcode) {
        case ByteCode::MemoryInitM64Opcode:
        case ByteCode::MemoryCopyM64M32Opcode:
            memoryParamToArg(compiler, context, params + 0, src + 0);
            src[1].set(params + 1);
            src[2].set(params + 2);
            break;
        case ByteCode::MemoryCopyM64Opcode:
            memoryParamToArg(compiler, context, params + 0, src + 0);
            memoryParamToArg(compiler, context, params + 1, src + 1);
            memoryParamToArg(compiler, context, params + 2, src + 2);
            break;
        case ByteCode::MemoryCopyM32M64Opcode:
            src[0].set(params + 0);
            memoryParamToArg(compiler, context, params + 1, src + 1);
            src[2].set(params + 2);
            break;
        case ByteCode::MemoryFillM64Opcode:
            memoryParamToArg(compiler, context, params + 0, src + 0);
            src[1].set(params + 1);
            memoryParamToArg(compiler, context, params + 2, src + 2);
            break;
        default:
            src[0].set(params + 0);
            src[1].set(params + 1);
            src[2].set(params + 2);
            break;
        }
        emitInitR0R1R2Args(compiler, SLJIT_MOV, SLJIT_MOV, SLJIT_MOV, src);
#else /* !SLJIT_32BIT_ARCHITECTURE */
        sljit_s32 movOp1, movOp2, movOp3;

        switch (opcode) {
        case ByteCode::MemoryInitM64Opcode:
        case ByteCode::MemoryCopyM64M32Opcode:
            movOp1 = SLJIT_MOV;
            movOp2 = SLJIT_MOV32;
            movOp3 = SLJIT_MOV32;
            break;
        case ByteCode::MemoryCopyM64Opcode:
            movOp1 = SLJIT_MOV;
            movOp2 = SLJIT_MOV;
            movOp3 = SLJIT_MOV;
            break;
        case ByteCode::MemoryCopyM32M64Opcode:
            movOp1 = SLJIT_MOV32;
            movOp2 = SLJIT_MOV;
            movOp3 = SLJIT_MOV32;
            break;
        case ByteCode::MemoryFillM64Opcode:
            movOp1 = SLJIT_MOV;
            movOp2 = SLJIT_MOV32;
            movOp3 = SLJIT_MOV;
            break;
        default:
            movOp1 = SLJIT_MOV32;
            movOp2 = SLJIT_MOV32;
            movOp3 = SLJIT_MOV32;
            break;
        }
        emitInitR0R1R2(compiler, movOp1, movOp2, movOp3, params);
#endif /* SLJIT_32BIT_ARCHITECTURE */

        sljit_sw addr;

        switch (opcode) {
        case ByteCode::MemoryInitOpcode:
        case ByteCode::MemoryInitM64Opcode: {
            ByteCodeOffset3MemIndexSegmentIndex* memoryInit = reinterpret_cast<ByteCodeOffset3MemIndexSegmentIndex*>(instr->byteCode());

            sljit_sw stackTmpStart = CompileContext::get(compiler)->stackTmpStart;
            sljit_get_local_base(compiler, SLJIT_R3, 0, stackTmpStart);
            sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_MEM1(SLJIT_SP), OffsetOfStackTmp(MemoryInitArguments, memory),
                           SLJIT_MEM1(kInstanceReg), Instance::alignedSize() + memoryInit->memIndex() * sizeof(void*));
            sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_MEM1(SLJIT_SP), OffsetOfStackTmp(MemoryInitArguments, data),
                           kInstanceReg, 0, SLJIT_IMM, context->dataSegmentsStart + (memoryInit->segmentIndex() * sizeof(ElementSegment)));
            addr = (opcode == ByteCode::MemoryInitOpcode) ? GET_FUNC_ADDR(sljit_sw, initMemory) : GET_FUNC_ADDR(sljit_sw, initMemoryM64);
            break;
        }
        case ByteCode::MemoryFillOpcode:
        case ByteCode::MemoryFillM64Opcode: {
            ByteCodeOffset3MemIndex* memoryFill = reinterpret_cast<ByteCodeOffset3MemIndex*>(instr->byteCode());

            sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R3, 0, SLJIT_MEM1(kInstanceReg), Instance::alignedSize() + memoryFill->memIndex() * sizeof(void*));
            addr = (opcode == ByteCode::MemoryFillOpcode) ? GET_FUNC_ADDR(sljit_sw, fillMemory) : GET_FUNC_ADDR(sljit_sw, fillMemoryM64);
            break;
        }
        default: {
            MemoryCopy* memoryCopy = reinterpret_cast<MemoryCopy*>(instr->byteCode());

            sljit_sw stackTmpStart = CompileContext::get(compiler)->stackTmpStart;
            sljit_get_local_base(compiler, SLJIT_R3, 0, stackTmpStart);
            sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_MEM1(SLJIT_SP), OffsetOfStackTmp(MemoryCopyArguments, srcMemory),
                           SLJIT_MEM1(kInstanceReg), Instance::alignedSize() + memoryCopy->srcMemIndex() * sizeof(void*));
            sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_MEM1(SLJIT_SP), OffsetOfStackTmp(MemoryCopyArguments, dstMemory),
                           SLJIT_MEM1(kInstanceReg), Instance::alignedSize() + memoryCopy->dstMemIndex() * sizeof(void*));

            switch (opcode) {
            case ByteCode::MemoryCopyM64Opcode:
                addr = GET_FUNC_ADDR(sljit_sw, copyMemoryM64);
                break;
            case ByteCode::MemoryCopyM64M32Opcode:
                addr = GET_FUNC_ADDR(sljit_sw, copyMemoryM64M32);
                break;
            case ByteCode::MemoryCopyM32M64Opcode:
                addr = GET_FUNC_ADDR(sljit_sw, copyMemoryM32M64);
                break;
            default:
                addr = GET_FUNC_ADDR(sljit_sw, copyMemory);
                break;
            }
            break;
        }
        }

        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(W, 32, 32, 32, P), SLJIT_IMM, addr);

        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, ExecutionContext::NoError);
        context->appendTrapJump(ExecutionContext::GenericTrap, cmp);
        return;
    }
    default: {
        ASSERT((opcode == ByteCode::MemoryGrowOpcode || opcode == ByteCode::MemoryGrowM64Opcode) && (instr->info() & Instruction::kIsCallback));

        uint16_t memIndex = reinterpret_cast<ByteCodeOffset2MemIndex*>(instr->byteCode())->memIndex();

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        JITArg arg;
        if (opcode == ByteCode::MemoryGrowOpcode) {
            arg.set(params);
        } else {
            memoryParamToArg(compiler, context, params + 0, &arg);
        }
        MOVE_TO_REG(compiler, SLJIT_MOV, SLJIT_R0, arg.arg, arg.argw);
#else /* !SLJIT_32BIT_ARCHITECTURE */
        JITArg arg(params);
        sljit_s32 movOpcode = (opcode == ByteCode::MemoryGrowOpcode) ? SLJIT_MOV32 : SLJIT_MOV;
        MOVE_TO_REG(compiler, movOpcode, SLJIT_R0, arg.arg, arg.argw);
#endif /* SLJIT_32BIT_ARCHITECTURE */

        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, kInstanceReg, 0);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM, memIndex);

        sljit_sw addr = (opcode == ByteCode::MemoryGrowOpcode) ? GET_FUNC_ADDR(sljit_sw, growMemory) : GET_FUNC_ADDR(sljit_sw, growMemoryM64);
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS3(32, 32, W, W), SLJIT_IMM, addr);

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        if (opcode == ByteCode::MemoryGrowOpcode) {
            arg.set(params + 1);
            MOVE_FROM_REG(compiler, SLJIT_MOV, arg.arg, arg.argw, SLJIT_R0);
        } else {
            JITArgPair argPair(params + 1);
            MOVE_FROM_REG(compiler, SLJIT_MOV, argPair.arg1, argPair.arg1w, SLJIT_R0);
            sljit_emit_op1(compiler, SLJIT_MOV, argPair.arg2, argPair.arg2w, SLJIT_IMM, 0);
        }
#else /* !SLJIT_32BIT_ARCHITECTURE */
        arg.set(params + 1);
        MOVE_FROM_REG(compiler, movOpcode, arg.arg, arg.argw, SLJIT_R0);
#endif /* SLJIT_32BIT_ARCHITECTURE */
        return;
    }
    }
}

static void dropData(uint32_t segmentIndex, Instance* instance)
{
    DataSegment* sg = instance->dataSegment(segmentIndex);
    sg->drop();
}

static void emitDataDrop(sljit_compiler* compiler, Instruction* instr)
{
    DataDrop* dataDrop = reinterpret_cast<DataDrop*>(instr->byteCode());

    ASSERT(instr->info() & Instruction::kIsCallback);

    sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, static_cast<sljit_sw>(dataDrop->segmentIndex()));
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, kInstanceReg, 0);

    sljit_sw addr = GET_FUNC_ADDR(sljit_sw, dropData);
    sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS2V(32, W), SLJIT_IMM, addr);
}

static void emitAtomicFence(sljit_compiler* compiler)
{
    sljit_emit_op0(compiler, SLJIT_MEMORY_BARRIER);
}
