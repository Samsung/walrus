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

static sljit_sw initTable(uint32_t dstStart, uint32_t srcStart, uint32_t srcSize, TableInitArguments* args)
{
    Table* table = args->table;
    ElementSegment* source = args->source;

    if (UNLIKELY((uint64_t)dstStart + (uint64_t)srcSize > table->size())
        || UNLIKELY((uint64_t)srcStart + (uint64_t)srcSize > source->size())) {
        return ExecutionContext::OutOfBoundsTableAccessError;
    }

    if (UNLIKELY(!table->type().isRef())) {
        return ExecutionContext::TypeMismatchError;
    }

    table->initTable(source, dstStart, srcStart, srcSize);
    return ExecutionContext::NoError;
}

#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)

static sljit_sw initTableM64(uint64_t dstStart, uint32_t srcStart, uint32_t srcSize, TableInitArguments* args)
{
    Table* table = args->table;
    ElementSegment* source = args->source;

    ASSERT(table->is64());
    if (UNLIKELY(srcSize > table->size() || dstStart > table->size() - srcSize)
        || UNLIKELY(((uint64_t)srcStart + (uint64_t)srcSize) > source->size())) {
        return ExecutionContext::OutOfBoundsTableAccessError;
    }

    if (UNLIKELY(!table->type().isRef())) {
        return ExecutionContext::TypeMismatchError;
    }

    table->initTable(source, dstStart, srcStart, srcSize);
    return ExecutionContext::NoError;
}

#endif /* SLJIT_64BIT_ARCHITECTURE */

static sljit_sw copyTable(uint32_t dstIndex, uint32_t srcIndex, uint32_t n, TableCopyArguments* args)
{
    Table* srcTable = args->srcTable;
    Table* dstTable = args->dstTable;

    if (UNLIKELY(((uint64_t)srcIndex + (uint64_t)n > (uint64_t)srcTable->size()))
        || UNLIKELY(((uint64_t)dstIndex + (uint64_t)n > (uint64_t)dstTable->size()))) {
        return ExecutionContext::OutOfBoundsTableAccessError;
    }

    dstTable->copyTable(srcTable, n, srcIndex, dstIndex);
    return ExecutionContext::NoError;
}

#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)

static sljit_sw copyTableM64(uint64_t dstIndex, uint64_t srcIndex, uint64_t n, TableCopyArguments* args)
{
    Table* srcTable = args->srcTable;
    Table* dstTable = args->dstTable;

    ASSERT(srcTable->is64() && dstTable->is64());
    if (UNLIKELY(n > srcTable->size() || srcIndex > srcTable->size() - n)
        || UNLIKELY(n > dstTable->size() || dstIndex > dstTable->size() - n)) {
        return ExecutionContext::OutOfBoundsTableAccessError;
    }

    dstTable->copyTable(srcTable, n, srcIndex, dstIndex);
    return ExecutionContext::NoError;
}

static sljit_sw copyTableM64M32(uint64_t dstIndex, uint32_t srcIndex, uint32_t n, TableCopyArguments* args)
{
    Table* srcTable = args->srcTable;
    Table* dstTable = args->dstTable;

    ASSERT(!srcTable->is64() && dstTable->is64());
    if (UNLIKELY((uint64_t)srcIndex + (uint64_t)n > (uint64_t)srcTable->size())
        || UNLIKELY(n > dstTable->size() || dstIndex > dstTable->size() - n)) {
        return ExecutionContext::OutOfBoundsTableAccessError;
    }

    dstTable->copyTable(srcTable, n, srcIndex, dstIndex);
    return ExecutionContext::NoError;
}

static sljit_sw copyTableM32M64(uint32_t dstIndex, uint64_t srcIndex, uint32_t n, TableCopyArguments* args)
{
    Table* srcTable = args->srcTable;
    Table* dstTable = args->dstTable;

    ASSERT(srcTable->is64() && !dstTable->is64());
    if (UNLIKELY(n > srcTable->size() || srcIndex > srcTable->size() - n)
        || UNLIKELY(n > dstTable->size() || dstIndex > dstTable->size() - n)) {
        return ExecutionContext::OutOfBoundsTableAccessError;
    }

    dstTable->copyTable(srcTable, n, srcIndex, dstIndex);
    return ExecutionContext::NoError;
}

#endif /* SLJIT_64BIT_ARCHITECTURE */

static sljit_sw fillTable(uint32_t index, void* ptr, uint32_t n, Table* table)
{
    if (UNLIKELY((uint64_t)index + (uint64_t)n > table->size())) {
        return ExecutionContext::OutOfBoundsTableAccessError;
    }

    table->fillTable(n, ptr, index);
    return ExecutionContext::NoError;
}

#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)

static sljit_sw fillTableM64(uint64_t index, void* ptr, uint64_t n, Table* table)
{
    ASSERT(table->is64());
    if (UNLIKELY(n > table->size() || index > table->size() - n)) {
        return ExecutionContext::OutOfBoundsTableAccessError;
    }

    table->fillTable(n, ptr, index);
    return ExecutionContext::NoError;
}

#endif /* SLJIT_64BIT_ARCHITECTURE */

static sljit_s32 growTable(void* ptr, uint32_t newSize, uint32_t tableIndex, Instance* instance)
{
    auto table = instance->table(tableIndex);
    auto oldSize = table->size();

    if (newSize <= table->maximumSize() - oldSize && table->grow(oldSize + newSize, ptr)) {
        return static_cast<sljit_uw>(oldSize);
    }

    return -1;
}

#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)

static sljit_sw growTableM64(void* ptr, sljit_uw newSize, uint32_t tableIndex, Instance* instance)
{
    auto table = instance->table(tableIndex);
    auto oldSize = table->size();

    ASSERT(table->is64());
    if (newSize <= table->maximumSize() - oldSize && table->grow(oldSize + newSize, ptr)) {
        return static_cast<sljit_uw>(oldSize);
    }

    return -1;
}

#endif /* SLJIT_64BIT_ARCHITECTURE */

static void dropElement(uint32_t segmentIndex, Instance* instance)
{
    instance->elementSegment(segmentIndex)->drop();
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
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_MEM1(SLJIT_SP), OffsetOfStackTmp(TableInitArguments, table),
                       SLJIT_MEM1(kInstanceReg), context->tableStart + (tableInit->tableIndex() * sizeof(void*)));
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_MEM1(SLJIT_SP), OffsetOfStackTmp(TableInitArguments, source),
                       kInstanceReg, 0, SLJIT_IMM, context->elementSegmentsStart + (tableInit->segmentIndex() * sizeof(ElementSegment)));
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(W, 32, 32, 32, P), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, initTable));

        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, ExecutionContext::NoError);
        context->appendTrapJump(ExecutionContext::GenericTrap, cmp);
        break;
    }
    case ByteCode::TableInitM64Opcode: {
        ASSERT(instr->info() & Instruction::kIsCallback);
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        emitInitR0R1R2(compiler, SLJIT_MOV, SLJIT_MOV32, SLJIT_MOV32, instr->operands());
#else /* !SLJIT_64BIT_ARCHITECTURE */
        Operand* operands = instr->operands();
        JITArgPair arg64(operands);
        JITArg args[3];

        sljit_jump* nonZero = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, arg64.arg2, arg64.arg2w, SLJIT_IMM, 0);
        context->appendTrapJump(ExecutionContext::OutOfBoundsTableAccessError, nonZero);

        args[0].arg = arg64.arg1;
        args[0].argw = arg64.arg1w;
        args[1].set(operands + 1);
        args[2].set(operands + 2);

        emitInitR0R1R2Args(compiler, SLJIT_MOV, SLJIT_MOV32, SLJIT_MOV32, args);
#endif /* SLJIT_64BIT_ARCHITECTURE */

        TableInit* tableInit = reinterpret_cast<TableInit*>(instr->byteCode());
        sljit_get_local_base(compiler, SLJIT_R3, 0, stackTmpStart);
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_MEM1(SLJIT_SP), OffsetOfStackTmp(TableInitArguments, table),
                       SLJIT_MEM1(kInstanceReg), context->tableStart + (tableInit->tableIndex() * sizeof(void*)));
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_MEM1(SLJIT_SP), OffsetOfStackTmp(TableInitArguments, source),
                       kInstanceReg, 0, SLJIT_IMM, context->elementSegmentsStart + (tableInit->segmentIndex() * sizeof(ElementSegment)));

#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(W, W, 32, 32, P), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, initTableM64));
#else /* !SLJIT_64BIT_ARCHITECTURE */
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(W, W, 32, 32, P), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, initTable));
#endif /* SLJIT_64BIT_ARCHITECTURE */

        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, ExecutionContext::NoError);
        context->appendTrapJump(ExecutionContext::GenericTrap, cmp);
        break;
    }
    case ByteCode::TableSizeOpcode: {
        ASSERT(!(instr->info() & Instruction::kIsCallback));
        JITArg dstArg(instr->operands());

        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(kInstanceReg), context->tableStart + ((reinterpret_cast<TableSize*>(instr->byteCode()))->tableIndex() * sizeof(void*)));
        moveIntToDest(compiler, SLJIT_MOV32, dstArg, JITFieldAccessor::tableSizeOffset() + WORD_LOW_OFFSET);
        break;
    }
    case ByteCode::TableSizeM64Opcode: {
        ASSERT(!(instr->info() & Instruction::kIsCallback));
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        JITArg dstArg(instr->operands());

        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(kInstanceReg), context->tableStart + ((reinterpret_cast<TableSize*>(instr->byteCode()))->tableIndex() * sizeof(void*)));
        moveIntToDest(compiler, SLJIT_MOV, dstArg, JITFieldAccessor::tableSizeOffset());
#else /* !SLJIT_64BIT_ARCHITECTURE */
        JITArgPair dstArgPair(instr->operands());

        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(kInstanceReg), context->tableStart + ((reinterpret_cast<TableSize*>(instr->byteCode()))->tableIndex() * sizeof(void*)));

        if (SLJIT_IS_REG(dstArgPair.arg1)) {
            sljit_emit_op1(compiler, SLJIT_MOV, dstArgPair.arg1, dstArgPair.arg1w, SLJIT_MEM1(SLJIT_TMP_DEST_REG), JITFieldAccessor::tableSizeOffset() + WORD_LOW_OFFSET);
            sljit_emit_op1(compiler, SLJIT_MOV, dstArgPair.arg2, dstArgPair.arg2w, SLJIT_IMM, 0);
        } else {
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), JITFieldAccessor::tableSizeOffset() + WORD_LOW_OFFSET);
            sljit_emit_op1(compiler, SLJIT_MOV, dstArgPair.arg2, dstArgPair.arg2w, SLJIT_IMM, 0);
            sljit_emit_op1(compiler, SLJIT_MOV, dstArgPair.arg1, dstArgPair.arg1w, SLJIT_TMP_DEST_REG, 0);
        }
#endif /* SLJIT_64BIT_ARCHITECTURE */
        break;
    }
    case ByteCode::TableCopyOpcode: {
        ASSERT(instr->info() & Instruction::kIsCallback);
        emitInitR0R1R2(compiler, SLJIT_MOV32, SLJIT_MOV32, SLJIT_MOV32, instr->operands());

        TableCopy* tableCopy = reinterpret_cast<TableCopy*>(instr->byteCode());
        sljit_get_local_base(compiler, SLJIT_R3, 0, stackTmpStart);
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_MEM1(SLJIT_SP), OffsetOfStackTmp(TableCopyArguments, srcTable),
                       SLJIT_MEM1(kInstanceReg), context->tableStart + (tableCopy->srcIndex() * sizeof(void*)));
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_MEM1(SLJIT_SP), OffsetOfStackTmp(TableCopyArguments, dstTable),
                       SLJIT_MEM1(kInstanceReg), context->tableStart + (tableCopy->dstIndex() * sizeof(void*)));
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(W, 32, 32, 32, P), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, copyTable));

        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, ExecutionContext::NoError);
        context->appendTrapJump(ExecutionContext::GenericTrap, cmp);
        break;
    }
    case ByteCode::TableCopyM64Opcode: {
        ASSERT(instr->info() & Instruction::kIsCallback);

#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        emitInitR0R1R2(compiler, SLJIT_MOV, SLJIT_MOV, SLJIT_MOV, instr->operands());
#else /* !SLJIT_64BIT_ARCHITECTURE */
        Operand* operands = instr->operands();
        JITArgPair args64[3] = { operands, operands + 1, operands + 2 };
        JITArg args[3];

        sljit_emit_op2u(compiler, SLJIT_OR | SLJIT_SET_Z, args64[0].arg2, args64[0].arg2w, args64[1].arg2, args64[1].arg2w);
        context->appendTrapJump(ExecutionContext::OutOfBoundsTableAccessError, sljit_emit_jump(compiler, SLJIT_NOT_ZERO));
        sljit_jump* nonZero = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, args64[2].arg2, args64[2].arg2w, SLJIT_IMM, 0);
        context->appendTrapJump(ExecutionContext::OutOfBoundsTableAccessError, nonZero);

        args[0].arg = args64[0].arg1;
        args[0].argw = args64[0].arg1w;
        args[1].arg = args64[1].arg1;
        args[1].argw = args64[1].arg1w;
        args[2].arg = args64[2].arg1;
        args[2].argw = args64[2].arg1w;

        emitInitR0R1R2Args(compiler, SLJIT_MOV, SLJIT_MOV, SLJIT_MOV, args);
#endif /* SLJIT_64BIT_ARCHITECTURE */

        TableCopy* tableCopy = reinterpret_cast<TableCopy*>(instr->byteCode());
        sljit_get_local_base(compiler, SLJIT_R3, 0, stackTmpStart);
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_MEM1(SLJIT_SP), OffsetOfStackTmp(TableCopyArguments, srcTable),
                       SLJIT_MEM1(kInstanceReg), context->tableStart + (tableCopy->srcIndex() * sizeof(void*)));
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_MEM1(SLJIT_SP), OffsetOfStackTmp(TableCopyArguments, dstTable),
                       SLJIT_MEM1(kInstanceReg), context->tableStart + (tableCopy->dstIndex() * sizeof(void*)));

#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(W, W, W, W, P), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, copyTableM64));
#else /* !SLJIT_64BIT_ARCHITECTURE */
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(W, W, W, W, P), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, copyTable));
#endif /* SLJIT_64BIT_ARCHITECTURE */

        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, ExecutionContext::NoError);
        context->appendTrapJump(ExecutionContext::GenericTrap, cmp);
        break;
    }
    case ByteCode::TableCopyM64M32Opcode: {
        ASSERT(instr->info() & Instruction::kIsCallback);

#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        emitInitR0R1R2(compiler, SLJIT_MOV, SLJIT_MOV32, SLJIT_MOV32, instr->operands());
#else /* !SLJIT_64BIT_ARCHITECTURE */
        Operand* operands = instr->operands();
        JITArgPair arg64(operands);
        JITArg args[3];

        sljit_jump* nonZero = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, arg64.arg2, arg64.arg2w, SLJIT_IMM, 0);
        context->appendTrapJump(ExecutionContext::OutOfBoundsTableAccessError, nonZero);

        args[0].arg = arg64.arg1;
        args[0].argw = arg64.arg1w;
        args[1].set(operands + 1);
        args[2].set(operands + 2);

        emitInitR0R1R2Args(compiler, SLJIT_MOV, SLJIT_MOV32, SLJIT_MOV32, args);
#endif /* SLJIT_64BIT_ARCHITECTURE */

        TableCopy* tableCopy = reinterpret_cast<TableCopy*>(instr->byteCode());
        sljit_get_local_base(compiler, SLJIT_R3, 0, stackTmpStart);
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_MEM1(SLJIT_SP), OffsetOfStackTmp(TableCopyArguments, srcTable),
                       SLJIT_MEM1(kInstanceReg), context->tableStart + (tableCopy->srcIndex() * sizeof(void*)));
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_MEM1(SLJIT_SP), OffsetOfStackTmp(TableCopyArguments, dstTable),
                       SLJIT_MEM1(kInstanceReg), context->tableStart + (tableCopy->dstIndex() * sizeof(void*)));

#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(W, W, 32, 32, P), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, copyTableM64M32));
#else /* !SLJIT_64BIT_ARCHITECTURE */
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(W, W, 32, 32, P), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, copyTable));
#endif /* SLJIT_64BIT_ARCHITECTURE */

        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, ExecutionContext::NoError);
        context->appendTrapJump(ExecutionContext::GenericTrap, cmp);
        break;
    }
    case ByteCode::TableCopyM32M64Opcode: {
        ASSERT(instr->info() & Instruction::kIsCallback);

#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        emitInitR0R1R2(compiler, SLJIT_MOV32, SLJIT_MOV, SLJIT_MOV32, instr->operands());
#else /* !SLJIT_64BIT_ARCHITECTURE */
        Operand* operands = instr->operands();
        JITArgPair arg64(operands + 1);
        JITArg args[3];

        sljit_jump* nonZero = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, arg64.arg2, arg64.arg2w, SLJIT_IMM, 0);
        context->appendTrapJump(ExecutionContext::OutOfBoundsTableAccessError, nonZero);

        args[0].set(operands);
        args[1].arg = arg64.arg1;
        args[1].argw = arg64.arg1w;
        args[2].set(operands + 2);

        emitInitR0R1R2Args(compiler, SLJIT_MOV32, SLJIT_MOV, SLJIT_MOV32, args);
#endif /* SLJIT_64BIT_ARCHITECTURE */

        TableCopy* tableCopy = reinterpret_cast<TableCopy*>(instr->byteCode());
        sljit_get_local_base(compiler, SLJIT_R3, 0, stackTmpStart);
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_MEM1(SLJIT_SP), OffsetOfStackTmp(TableCopyArguments, srcTable),
                       SLJIT_MEM1(kInstanceReg), context->tableStart + (tableCopy->srcIndex() * sizeof(void*)));
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_MEM1(SLJIT_SP), OffsetOfStackTmp(TableCopyArguments, dstTable),
                       SLJIT_MEM1(kInstanceReg), context->tableStart + (tableCopy->dstIndex() * sizeof(void*)));

#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(W, 32, W, 32, P), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, copyTableM32M64));
#else /* !SLJIT_64BIT_ARCHITECTURE */
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(W, 32, W, 32, P), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, copyTable));
#endif /* SLJIT_64BIT_ARCHITECTURE */

        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, ExecutionContext::NoError);
        context->appendTrapJump(ExecutionContext::GenericTrap, cmp);
        break;
    }
    case ByteCode::TableFillOpcode: {
        ASSERT(instr->info() & Instruction::kIsCallback);
        emitInitR0R1R2(compiler, SLJIT_MOV32, SLJIT_MOV_P, SLJIT_MOV32, instr->operands());
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R3, 0, SLJIT_MEM1(kInstanceReg), context->tableStart + ((reinterpret_cast<TableFill*>(instr->byteCode()))->tableIndex() * sizeof(void*)));
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(W, 32, P, 32, P), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, fillTable));

        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, ExecutionContext::NoError);
        context->appendTrapJump(ExecutionContext::GenericTrap, cmp);
        break;
    }
    case ByteCode::TableFillM64Opcode: {
        ASSERT(instr->info() & Instruction::kIsCallback);
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        emitInitR0R1R2(compiler, SLJIT_MOV, SLJIT_MOV_P, SLJIT_MOV, instr->operands());
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R3, 0, SLJIT_MEM1(kInstanceReg), context->tableStart + ((reinterpret_cast<TableFill*>(instr->byteCode()))->tableIndex() * sizeof(void*)));
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(W, W, P, W, P), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, fillTableM64));
#else /* !SLJIT_64BIT_ARCHITECTURE */
        Operand* operands = instr->operands();
        JITArgPair args64[3] = { operands, operands + 2 };
        JITArg args[3];

        sljit_emit_op2u(compiler, SLJIT_OR | SLJIT_SET_Z, args64[0].arg2, args64[0].arg2w, args64[1].arg2, args64[1].arg2w);
        context->appendTrapJump(ExecutionContext::OutOfBoundsTableAccessError, sljit_emit_jump(compiler, SLJIT_NOT_ZERO));

        args[0].arg = args64[0].arg1;
        args[0].argw = args64[0].arg1w;
        args[1].set(operands + 1);
        args[2].arg = args64[1].arg1;
        args[2].argw = args64[1].arg1w;

        emitInitR0R1R2Args(compiler, SLJIT_MOV, SLJIT_MOV_P, SLJIT_MOV, args);
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R3, 0, SLJIT_MEM1(kInstanceReg), context->tableStart + ((reinterpret_cast<TableFill*>(instr->byteCode()))->tableIndex() * sizeof(void*)));
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(W, 32, P, 32, P), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, fillTable));
#endif /* SLJIT_64BIT_ARCHITECTURE */

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
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(W, P, W, 32, P), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, growTable));
        MOVE_FROM_REG(compiler, SLJIT_MOV32, args[2].arg, args[2].argw, SLJIT_R0);
        break;
    }
    case ByteCode::TableGrowM64Opcode: {
        ASSERT(instr->info() & Instruction::kIsCallback);

        Operand* operands = instr->operands();
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        JITArg args[3] = { operands, operands + 1, operands + 2 };

        emitInitR0R1(compiler, SLJIT_MOV_P, SLJIT_MOV, args);

        sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_R2, 0, SLJIT_IMM, ((reinterpret_cast<TableGrow*>(instr->byteCode()))->tableIndex()));
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R3, 0, kInstanceReg, 0);
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(W, P, W, 32, P), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, growTableM64));
        MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg, args[2].argw, SLJIT_R0);
#else /* !SLJIT_64BIT_ARCHITECTURE */
        JITArgPair sizes[2] = { operands + 1, operands + 2 };
        JITArg args[2];

        sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_Z, sizes[0].arg2, sizes[0].arg2w, SLJIT_IMM, 0);

        args[0].set(operands);
        args[1].arg = sizes[0].arg1;
        args[1].argw = sizes[0].arg1w;

        emitInitR0R1(compiler, SLJIT_MOV_P, SLJIT_MOV32, args);
        sljit_emit_select(compiler, SLJIT_NOT_ZERO, SLJIT_R1, SLJIT_IMM, -1, SLJIT_R1);

        sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_R2, 0, SLJIT_IMM, ((reinterpret_cast<TableGrow*>(instr->byteCode()))->tableIndex()));
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R3, 0, kInstanceReg, 0);
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(W, P, W, 32, P), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, growTable));

        sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_Z, SLJIT_R0, 0, SLJIT_IMM, -1);
        sljit_emit_op_flags(compiler, SLJIT_MOV, SLJIT_TMP_DEST_REG, 0, SLJIT_NOT_ZERO);

        MOVE_FROM_REG(compiler, SLJIT_MOV, sizes[1].arg1, sizes[1].arg1w, SLJIT_R0);
        sljit_emit_op2(compiler, SLJIT_SUB, SLJIT_TMP_DEST_REG, 0, SLJIT_TMP_DEST_REG, 0, SLJIT_IMM, 1);
        sljit_emit_op1(compiler, SLJIT_MOV, sizes[1].arg2, sizes[1].arg2w, SLJIT_TMP_DEST_REG, 0);
#endif /* SLJIT_64BIT_ARCHITECTURE */
        break;
    }
    case ByteCode::TableSetOpcode: {
        ASSERT(!(instr->info() & Instruction::kIsCallback));
        JITArg src[2] = { instr->operands(), instr->operands() + 1 };

        sljit_s32 destinationReg = instr->requiredReg(0);
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        sljit_emit_op1(compiler, SLJIT_MOV_U32, destinationReg, 0, src[0].arg, src[0].argw);
#else /* !SLJIT_64BIT_ARCHITECTURE */
        MOVE_TO_REG(compiler, SLJIT_MOV, destinationReg, src[0].arg, src[0].argw);
#endif /* SLJIT_64BIT_ARCHITECTURE */
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(kInstanceReg), context->tableStart + ((reinterpret_cast<TableSet*>(instr->byteCode()))->tableIndex() * sizeof(void*)));

#if (defined SLJIT_CONFIG_X86 && SLJIT_CONFIG_X86)
        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_GREATER_EQUAL | SLJIT_32, destinationReg, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), JITFieldAccessor::tableSizeOffset());
#else /* !SLJIT_CONFIG_X86 */
        sljit_emit_op1(compiler, SLJIT_MOV_U32, SLJIT_TMP_OPT_REG, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), JITFieldAccessor::tableSizeOffset() + WORD_LOW_OFFSET);
        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_GREATER_EQUAL, destinationReg, 0, SLJIT_TMP_OPT_REG, 0);
#endif /* SLJIT_CONFIG_X86 */

        context->appendTrapJump(ExecutionContext::OutOfBoundsTableAccessError, cmp);
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), JITFieldAccessor::tableElements());
        sljit_emit_op2_shift(compiler, SLJIT_ADD | SLJIT_SHL_IMM, destinationReg, 0, SLJIT_TMP_DEST_REG, 0, destinationReg, 0, SLJIT_WORD_SHIFT);
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_MEM1(destinationReg), 0, src[1].arg, src[1].argw);
        break;
    }
    case ByteCode::TableSetM64Opcode: {
        ASSERT(!(instr->info() & Instruction::kIsCallback));
        sljit_s32 destinationReg = instr->requiredReg(0);
        JITArg src(instr->operands() + 1);

#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        JITArg destination(instr->operands());

        MOVE_TO_REG(compiler, SLJIT_MOV, destinationReg, destination.arg, destination.argw);
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(kInstanceReg), context->tableStart + ((reinterpret_cast<TableSet*>(instr->byteCode()))->tableIndex() * sizeof(void*)));

#if (defined SLJIT_CONFIG_X86 && SLJIT_CONFIG_X86)
        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_GREATER_EQUAL, destinationReg, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), JITFieldAccessor::tableSizeOffset());
#else /* !SLJIT_CONFIG_X86 */
        sljit_emit_op1(compiler, SLJIT_MOV_U32, SLJIT_TMP_OPT_REG, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), JITFieldAccessor::tableSizeOffset() + WORD_LOW_OFFSET);
        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_GREATER_EQUAL, destinationReg, 0, SLJIT_TMP_OPT_REG, 0);
#endif /* SLJIT_CONFIG_X86 */
#else /* !SLJIT_64BIT_ARCHITECTURE */
        JITArgPair destination(instr->operands());

        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, destination.arg2, destination.arg2w, SLJIT_IMM, 0);
        context->appendTrapJump(ExecutionContext::OutOfBoundsTableAccessError, cmp);

        MOVE_TO_REG(compiler, SLJIT_MOV, destinationReg, destination.arg1, destination.arg1w);
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(kInstanceReg), context->tableStart + ((reinterpret_cast<TableSet*>(instr->byteCode()))->tableIndex() * sizeof(void*)));

#if (defined SLJIT_CONFIG_X86 && SLJIT_CONFIG_X86)
        cmp = sljit_emit_cmp(compiler, SLJIT_GREATER_EQUAL, destinationReg, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), JITFieldAccessor::tableSizeOffset());
#else /* !SLJIT_CONFIG_X86 */
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_TMP_OPT_REG, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), JITFieldAccessor::tableSizeOffset() + WORD_LOW_OFFSET);
        cmp = sljit_emit_cmp(compiler, SLJIT_GREATER_EQUAL, destinationReg, 0, SLJIT_TMP_OPT_REG, 0);
#endif /* SLJIT_CONFIG_X86 */
#endif /* SLJIT_64BIT_ARCHITECTURE */

        context->appendTrapJump(ExecutionContext::OutOfBoundsTableAccessError, cmp);
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), JITFieldAccessor::tableElements());
        sljit_emit_op2_shift(compiler, SLJIT_ADD | SLJIT_SHL_IMM, destinationReg, 0, SLJIT_TMP_DEST_REG, 0, destinationReg, 0, SLJIT_WORD_SHIFT);
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_MEM1(destinationReg), 0, src.arg, src.argw);
        break;
    }
    case ByteCode::TableGetOpcode: {
        ASSERT(!(instr->info() & Instruction::kIsCallback));
        JITArg src[2] = { instr->operands(), instr->operands() + 1 };

        sljit_s32 sourceReg = instr->requiredReg(0);
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        sljit_emit_op1(compiler, SLJIT_MOV_U32, sourceReg, 0, src[0].arg, src[0].argw);
#else /* !SLJIT_64BIT_ARCHITECTURE */
        MOVE_TO_REG(compiler, SLJIT_MOV, sourceReg, src[0].arg, src[0].argw);
#endif /* SLJIT_64BIT_ARCHITECTURE */
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(kInstanceReg), context->tableStart + ((reinterpret_cast<TableSet*>(instr->byteCode()))->tableIndex() * sizeof(void*)));

#if (defined SLJIT_CONFIG_X86 && SLJIT_CONFIG_X86)
        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_GREATER_EQUAL | SLJIT_32, sourceReg, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), JITFieldAccessor::tableSizeOffset());
#else /* !SLJIT_CONFIG_X86 */
        sljit_emit_op1(compiler, SLJIT_MOV_U32, SLJIT_TMP_OPT_REG, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), JITFieldAccessor::tableSizeOffset() + WORD_LOW_OFFSET);
        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_GREATER_EQUAL, sourceReg, 0, SLJIT_TMP_OPT_REG, 0);
#endif /* SLJIT_CONFIG_X86 */

        context->appendTrapJump(ExecutionContext::OutOfBoundsTableAccessError, cmp);
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), JITFieldAccessor::tableElements());
        sljit_emit_op2_shift(compiler, SLJIT_ADD | SLJIT_SHL_IMM, sourceReg, 0, SLJIT_TMP_DEST_REG, 0, sourceReg, 0, SLJIT_WORD_SHIFT);
        sljit_emit_op1(compiler, SLJIT_MOV_P, src[1].arg, src[1].argw, SLJIT_MEM1(sourceReg), 0);
        break;
    }
    case ByteCode::TableGetM64Opcode: {
        ASSERT(!(instr->info() & Instruction::kIsCallback));
        sljit_s32 sourceReg = instr->requiredReg(0);
        JITArg dst(instr->operands() + 1);

#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        JITArg source(instr->operands());

        MOVE_TO_REG(compiler, SLJIT_MOV, sourceReg, source.arg, source.argw);
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(kInstanceReg), context->tableStart + ((reinterpret_cast<TableSet*>(instr->byteCode()))->tableIndex() * sizeof(void*)));

#if (defined SLJIT_CONFIG_X86 && SLJIT_CONFIG_X86)
        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_GREATER_EQUAL, sourceReg, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), JITFieldAccessor::tableSizeOffset());
#else /* !SLJIT_CONFIG_X86 */
        sljit_emit_op1(compiler, SLJIT_MOV_U32, SLJIT_TMP_OPT_REG, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), JITFieldAccessor::tableSizeOffset() + WORD_LOW_OFFSET);
        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_GREATER_EQUAL, sourceReg, 0, SLJIT_TMP_OPT_REG, 0);
#endif /* SLJIT_CONFIG_X86 */
#else /* !SLJIT_64BIT_ARCHITECTURE */
        JITArgPair source(instr->operands());

        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, source.arg2, source.arg2w, SLJIT_IMM, 0);
        context->appendTrapJump(ExecutionContext::OutOfBoundsTableAccessError, cmp);

        MOVE_TO_REG(compiler, SLJIT_MOV, sourceReg, source.arg1, source.arg1w);
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(kInstanceReg), context->tableStart + ((reinterpret_cast<TableSet*>(instr->byteCode()))->tableIndex() * sizeof(void*)));

#if (defined SLJIT_CONFIG_X86 && SLJIT_CONFIG_X86)
        cmp = sljit_emit_cmp(compiler, SLJIT_GREATER_EQUAL, sourceReg, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), JITFieldAccessor::tableSizeOffset());
#else /* !SLJIT_CONFIG_X86 */
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_TMP_OPT_REG, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), JITFieldAccessor::tableSizeOffset() + WORD_LOW_OFFSET);
        cmp = sljit_emit_cmp(compiler, SLJIT_GREATER_EQUAL, sourceReg, 0, SLJIT_TMP_OPT_REG, 0);
#endif /* SLJIT_CONFIG_X86 */
#endif /* SLJIT_64BIT_ARCHITECTURE */

        context->appendTrapJump(ExecutionContext::OutOfBoundsTableAccessError, cmp);
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), JITFieldAccessor::tableElements());
        sljit_emit_op2_shift(compiler, SLJIT_ADD | SLJIT_SHL_IMM, sourceReg, 0, SLJIT_TMP_DEST_REG, 0, sourceReg, 0, SLJIT_WORD_SHIFT);
        sljit_emit_op1(compiler, SLJIT_MOV_P, dst.arg, dst.argw, SLJIT_MEM1(sourceReg), 0);
        break;
    }
    default: {
        return;
    }
    }
}
