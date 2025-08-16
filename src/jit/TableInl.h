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

/* Only included by Backend.cpp */

static sljit_sw initTable(uint32_t dstStart, uint32_t srcStart, uint32_t srcSize, InitTableArguments* args)
{
    Instance* instance = args->instance;
    auto table = instance->table(args->tableIndex);
    auto source = instance->elementSegment(args->segmentIndex);

    if (UNLIKELY((uint64_t)dstStart + (uint64_t)srcSize > (uint64_t)table->size())
        || UNLIKELY(!source.element() || (srcStart + srcSize) > source.element()->exprFunctions().size())) {
        return ExecutionContext::OutOfBoundsTableAccessError;
    }

    if (UNLIKELY(!table->type().isRef())) {
        return ExecutionContext::TypeMismatchError;
    }

    table->initTable(instance, &source, dstStart, srcStart, srcSize);
    return ExecutionContext::NoError;
}

static sljit_sw copyTable(uint32_t dstIndex, uint32_t srcIndex, uint32_t n, TableCopyArguments* args)
{
    Instance* instance = args->instance;
    auto srcTable = instance->table(args->srcIndex);
    auto dstTable = instance->table(args->dstIndex);

    if (UNLIKELY(((uint64_t)srcIndex + (uint64_t)n > (uint64_t)srcTable->size()) || ((uint64_t)dstIndex + (uint64_t)n > (uint64_t)dstTable->size()))) {
        return ExecutionContext::OutOfBoundsTableAccessError;
    }

    dstTable->copyTable(srcTable, n, srcIndex, dstIndex);
    return ExecutionContext::NoError;
}

static sljit_sw fillTable(uint32_t index, void* ptr, uint32_t n, TableFillArguments* args)
{
    Instance* instance = args->instance;
    auto srcTable = instance->table(args->tableIndex);

    if ((uint64_t)index + (uint64_t)n > (uint64_t)srcTable->size()) {
        return ExecutionContext::OutOfBoundsTableAccessError;
    }

    srcTable->fillTable(n, ptr, index);
    return ExecutionContext::NoError;
}

static sljit_s32 growTable(void* ptr, uint32_t newSize, uint32_t tableIndex, Instance* instance)
{
    auto srcTable = instance->table(tableIndex);
    auto oldSize = srcTable->size();

    uint64_t totalSize = static_cast<uint64_t>(newSize) + oldSize;

    if (totalSize <= srcTable->maximumSize()) {
        srcTable->grow(totalSize, ptr);
        return oldSize;
    }

    return -1;
}

static void dropElement(uint32_t segmentIndex, Instance* instance)
{
    instance->elementSegment(segmentIndex).drop();
}

static void emitElemDrop(sljit_compiler* compiler, Instruction* instr)
{
    ASSERT(instr->info() & Instruction::kIsCallback);
    ElemDrop* elemDrop = reinterpret_cast<ElemDrop*>(instr->byteCode());

    sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, static_cast<sljit_sw>(elemDrop->segmentIndex()));
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, kInstanceReg, 0);
    sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS2V(32, W), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, dropElement));
}

static void emitTable(sljit_compiler* compiler, Instruction* instr)
{
    sljit_sw addr;
    CompileContext* context = CompileContext::get(compiler);
    sljit_sw stackTmpStart = context->stackTmpStart;

    switch (instr->opcode()) {
    case ByteCode::TableInitOpcode: {
        ASSERT(instr->info() & Instruction::kIsCallback);
        emitInitR0R1R2(compiler, SLJIT_MOV32, SLJIT_MOV32, SLJIT_MOV32, instr->operands());

        TableInit* tableInit = reinterpret_cast<TableInit*>(instr->byteCode());
        sljit_get_local_base(compiler, SLJIT_R3, 0, stackTmpStart);
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_MEM1(SLJIT_SP), OffsetOfStackTmp(InitTableArguments, instance), kInstanceReg, 0);
        sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_MEM1(SLJIT_SP), OffsetOfStackTmp(InitTableArguments, tableIndex), SLJIT_IMM, tableInit->tableIndex());
        sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_MEM1(SLJIT_SP), OffsetOfStackTmp(InitTableArguments, segmentIndex), SLJIT_IMM, tableInit->segmentIndex());
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4V(32, 32, 32, P), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, initTable));

        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, ExecutionContext::NoError);
        context->appendTrapJump(ExecutionContext::GenericTrap, cmp);
        break;
    }
    case ByteCode::TableSizeOpcode: {
        JITArg dstArg(instr->operands());

        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_TMP_MEM_REG, 0, SLJIT_MEM1(kInstanceReg), context->tableStart + ((reinterpret_cast<TableSize*>(instr->byteCode()))->tableIndex() * sizeof(void*)));
        moveIntToDest(compiler, SLJIT_MOV32, dstArg, JITFieldAccessor::tableSizeOffset());
        break;
    }
    case ByteCode::TableCopyOpcode: {
        ASSERT(instr->info() & Instruction::kIsCallback);
        emitInitR0R1R2(compiler, SLJIT_MOV32, SLJIT_MOV32, SLJIT_MOV32, instr->operands());

        TableCopy* tableCopy = reinterpret_cast<TableCopy*>(instr->byteCode());
        sljit_get_local_base(compiler, SLJIT_R3, 0, stackTmpStart);
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_MEM1(SLJIT_SP), OffsetOfStackTmp(TableCopyArguments, instance), kInstanceReg, 0);
        sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_MEM1(SLJIT_SP), OffsetOfStackTmp(TableCopyArguments, srcIndex), SLJIT_IMM, tableCopy->srcIndex());
        sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_MEM1(SLJIT_SP), OffsetOfStackTmp(TableCopyArguments, dstIndex), SLJIT_IMM, tableCopy->dstIndex());
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4V(32, 32, 32, P), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, copyTable));

        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, ExecutionContext::NoError);
        context->appendTrapJump(ExecutionContext::GenericTrap, cmp);
        break;
    }
    case ByteCode::TableFillOpcode: {
        ASSERT(instr->info() & Instruction::kIsCallback);
        emitInitR0R1R2(compiler, SLJIT_MOV32, SLJIT_MOV_P, SLJIT_MOV32, instr->operands());

        sljit_get_local_base(compiler, SLJIT_R3, 0, stackTmpStart);
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_MEM1(SLJIT_SP), OffsetOfStackTmp(TableFillArguments, instance), kInstanceReg, 0);
        sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_MEM1(SLJIT_SP), OffsetOfStackTmp(TableFillArguments, tableIndex), SLJIT_IMM, (reinterpret_cast<TableFill*>(instr->byteCode()))->tableIndex());
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4V(32, P, 32, P), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, fillTable));

        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, ExecutionContext::NoError);
        context->appendTrapJump(ExecutionContext::GenericTrap, cmp);
        break;
    }
    case ByteCode::TableGrowOpcode: {
        ASSERT(instr->info() & Instruction::kIsCallback);

        Operand* operands = instr->operands();
        JITArg args[3] = { operands, operands + 1, operands + 2 };

        emitInitR0R1(compiler, SLJIT_MOV_P, SLJIT_MOV32, args);

        sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_R2, 0, SLJIT_IMM, ((reinterpret_cast<TableGrow*>(instr->byteCode()))->tableIndex()));
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R3, 0, kInstanceReg, 0);
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(W, P, 32, 32, P), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, growTable));
        MOVE_FROM_REG(compiler, SLJIT_MOV32, args[2].arg, args[2].argw, SLJIT_R0);
        break;
    }
    case ByteCode::TableSetOpcode: {
        ASSERT(!(instr->info() & Instruction::kIsCallback));
        JITArg src[2] = { instr->operands(), instr->operands() + 1 };

        sljit_s32 destinationReg = instr->requiredReg(0);
        sljit_s32 tmpReg = instr->requiredReg(1);

        if (src[0].arg != destinationReg) {
            MOVE_TO_REG(compiler, SLJIT_MOV32, destinationReg, src[0].arg, src[0].argw);
            src[0].arg = destinationReg;
            src[0].argw = 0;
        }

        sljit_emit_op1(compiler, SLJIT_MOV_P, tmpReg, 0, SLJIT_MEM1(kInstanceReg), context->tableStart + ((reinterpret_cast<TableSet*>(instr->byteCode()))->tableIndex() * sizeof(void*)));

        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_GREATER_EQUAL | SLJIT_32, destinationReg, 0, SLJIT_MEM1(tmpReg), JITFieldAccessor::tableSizeOffset());
        context->appendTrapJump(ExecutionContext::OutOfBoundsTableAccessError, cmp);
        sljit_emit_op1(compiler, SLJIT_MOV_P, tmpReg, 0, SLJIT_MEM1(tmpReg), JITFieldAccessor::tableElements());
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM2(tmpReg, destinationReg), SLJIT_WORD_SHIFT, src[1].arg, src[1].argw);
        break;
    }
    case ByteCode::TableGetOpcode: {
        ASSERT(!(instr->info() & Instruction::kIsCallback));
        JITArg srcArg(instr->operands());

        sljit_s32 sourceReg = instr->requiredReg(0);
        sljit_s32 tmpReg = instr->requiredReg(1);

        if (srcArg.arg != sourceReg) {
            MOVE_TO_REG(compiler, SLJIT_MOV32, sourceReg, srcArg.arg, srcArg.argw);
            srcArg.arg = sourceReg;
            srcArg.argw = 0;
        }

        sljit_emit_op1(compiler, SLJIT_MOV_P, tmpReg, 0, SLJIT_MEM1(kInstanceReg), context->tableStart + ((reinterpret_cast<TableGet*>(instr->byteCode()))->tableIndex() * sizeof(void*)));

        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_GREATER_EQUAL | SLJIT_32, sourceReg, 0, SLJIT_MEM1(tmpReg), JITFieldAccessor::tableSizeOffset());
        context->appendTrapJump(ExecutionContext::OutOfBoundsTableAccessError, cmp);

        JITArg dstArg(instr->operands() + 1);

        sljit_emit_op1(compiler, SLJIT_MOV_P, tmpReg, 0, SLJIT_MEM1(tmpReg), JITFieldAccessor::tableElements());
        sljit_emit_op1(compiler, SLJIT_MOV_P, dstArg.arg, dstArg.argw, SLJIT_MEM2(tmpReg, sourceReg), SLJIT_WORD_SHIFT);
        break;
    }
    default: {
        return;
    }
    }
}
