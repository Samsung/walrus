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
    Instance* instance = args->instance;
    Memory* memory = instance->memory(0);
    DataSegment& sg = instance->dataSegment(args->segmentIndex);

    if (!memory->checkAccess(dstStart, srcSize)) {
        return ExecutionContext::OutOfBoundsMemAccessError;
    }

    if (srcStart >= sg.sizeInByte() || srcStart + srcSize > sg.sizeInByte()) {
        return ExecutionContext::OutOfBoundsMemAccessError;
    }

    memory->initMemory(&sg, dstStart, srcStart, srcSize);
    return ExecutionContext::NoError;
}

static sljit_sw copyMemory(uint32_t dstStart, uint32_t srcStart, uint32_t size, Instance* instance)
{
    Memory* memory = instance->memory(0);

    if (!memory->checkAccess(srcStart, size) || !memory->checkAccess(dstStart, size)) {
        return ExecutionContext::OutOfBoundsMemAccessError;
    }

    memory->copyMemory(dstStart, srcStart, size);
    return ExecutionContext::NoError;
}

static sljit_sw fillMemory(uint32_t start, uint32_t value, uint32_t size, Instance* instance)
{
    Memory* memory = instance->memory(0);

    if (!memory->checkAccess(start, size)) {
        return ExecutionContext::OutOfBoundsMemAccessError;
    }

    memory->fillMemory(start, value, size);
    return ExecutionContext::NoError;
}

static sljit_s32 growMemory(uint32_t newSize, Instance* instance)
{
    Memory* memory = instance->memory(0);
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
        ASSERT(!(instr->info() & Instruction::kIsCallback));

        JITArg dstArg(params);

        /* The sizeInByte is always a 32 bit number on 32 bit systems. */
        sljit_emit_op2(compiler, SLJIT_LSHR, dstArg.arg, dstArg.argw, SLJIT_MEM1(kInstanceReg),
                       context->targetBuffersStart + offsetof(Memory::TargetBuffer, sizeInByte) + WORD_LOW_OFFSET, SLJIT_IMM, 16);
        return;
    }
    case ByteCode::MemoryInitOpcode:
    case ByteCode::MemoryCopyOpcode:
    case ByteCode::MemoryFillOpcode: {
        ASSERT(instr->info() & Instruction::kIsCallback);

        emitInitR0R1R2(compiler, SLJIT_MOV32, params);

        sljit_sw addr;

        if (opcode == ByteCode::MemoryInitOpcode) {
            MemoryInit* memoryInit = reinterpret_cast<MemoryInit*>(instr->byteCode());

            sljit_sw stackTmpStart = CompileContext::get(compiler)->stackTmpStart;
            sljit_get_local_base(compiler, SLJIT_R3, 0, stackTmpStart);
            sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_MEM1(SLJIT_SP), OffsetOfStackTmp(MemoryInitArguments, instance), kInstanceReg, 0);
            sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_MEM1(SLJIT_SP), OffsetOfStackTmp(MemoryInitArguments, segmentIndex), SLJIT_IMM, memoryInit->segmentIndex());
            addr = GET_FUNC_ADDR(sljit_sw, initMemory);
        } else if (opcode == ByteCode::MemoryCopyOpcode) {
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R3, 0, kInstanceReg, 0);
            addr = GET_FUNC_ADDR(sljit_sw, copyMemory);
        } else {
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R3, 0, kInstanceReg, 0);
            addr = GET_FUNC_ADDR(sljit_sw, fillMemory);
        }

        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(W, 32, 32, 32, P), SLJIT_IMM, addr);

        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, ExecutionContext::NoError);
        context->appendTrapJump(ExecutionContext::GenericTrap, cmp);
        return;
    }
    default: {
        ASSERT(opcode == ByteCode::MemoryGrowOpcode && (instr->info() & Instruction::kIsCallback));
        JITArg arg(params);

        MOVE_TO_REG(compiler, SLJIT_MOV32, SLJIT_R0, arg.arg, arg.argw);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, kInstanceReg, 0);

        sljit_sw addr = GET_FUNC_ADDR(sljit_sw, growMemory);
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS2(32, 32, W), SLJIT_IMM, addr);

        arg.set(params + 1);
        MOVE_FROM_REG(compiler, SLJIT_MOV32, arg.arg, arg.argw, SLJIT_R0);
        return;
    }
    }
}

static void dropData(uint32_t segmentIndex, Instance* instance)
{
    DataSegment& sg = instance->dataSegment(segmentIndex);
    sg.drop();
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
