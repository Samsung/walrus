
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

static void emitLoad3Arguments(sljit_compiler* compiler, Operand* params)
{
    JITArg srcArg;

    for (int i = 0; i < 3; i++) {
        operandToArg(params + i, srcArg);
        MOVE_TO_REG(compiler, SLJIT_MOV32, SLJIT_R(i), srcArg.arg, srcArg.argw);
    }

    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R3, 0, kContextReg, 0);
}

static sljit_sw initTable(uint32_t dstStart, uint32_t srcStart, uint32_t srcSize, ExecutionContext* context)
{
    auto table = context->instance->table(context->tmp1);
    auto source = (context->instance->elementSegment(*(sljit_u32*)&context->tmp2));

    if (UNLIKELY((uint64_t)dstStart + (uint64_t)srcSize > (uint64_t)srcSize)
        || UNLIKELY(!source.element() || (srcStart + srcSize) > source.element()->functionIndex().size())
        || UNLIKELY(table->type() != Value::Type::FuncRef)) {
        return ExecutionContext::OutOfBoundsTableAccessError;
    }

    table->initTable(context->instance, &source, dstStart, srcStart, srcSize);
    return ExecutionContext::NoError;
}

static sljit_sw copyTable(uint32_t dstIndex, uint32_t srcIndex, uint32_t n, ExecutionContext* context)
{
    auto srcTable = context->instance->table(context->tmp1);
    auto dstTable = context->instance->table(context->tmp2);

    if (UNLIKELY(((uint64_t)srcIndex + (uint64_t)n > (uint64_t)srcTable->size()) || ((uint64_t)dstIndex + (uint64_t)n > (uint64_t)dstTable->size()))) {
        return ExecutionContext::OutOfBoundsTableAccessError;
    }

    dstTable->copyTable(srcTable, n, srcIndex, dstIndex);
    return ExecutionContext::NoError;
}

static sljit_sw fillTable(uint32_t index, void* ptr, uint32_t n, ExecutionContext* context)
{
    auto srcTable = context->instance->table(context->tmp1);

    if ((uint64_t)index + (uint64_t)n > (uint64_t)srcTable->size()) {
        return ExecutionContext::OutOfBoundsTableAccessError;
    }

    srcTable->fillTable(n, ptr, index);
    return ExecutionContext::NoError;
}

static sljit_s32 growTable(void* ptr, uint32_t newSize, uint32_t tableIndex, ExecutionContext* context)
{
    auto srcTable = context->instance->table(tableIndex);
    auto oldSize = srcTable->size();

    newSize += oldSize;

    if (newSize <= srcTable->maximumSize()) {
        srcTable->grow(newSize, ptr);
        return oldSize;
    }

    return -1;
}

static void dropElement(uint32_t segmentIndex, ExecutionContext* context)
{
    context->instance->elementSegment(segmentIndex).drop();
}

static void emitElemDrop(sljit_compiler* compiler, Instruction* instr)
{
    auto elemDrop = reinterpret_cast<ElemDrop*>(instr->byteCode());

    sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, static_cast<sljit_sw>(elemDrop->segmentIndex()));
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, kContextReg, 0);
    sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS2(VOID, 32, W), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, dropElement));
}

static void emitTable(sljit_compiler* compiler, Instruction* instr)
{
    sljit_sw addr;
    CompileContext* context = CompileContext::get(compiler);

    switch (instr->opcode()) {
    case ByteCode::TableInitOpcode: {
        emitLoad3Arguments(compiler, instr->operands());

        sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_MEM1(kContextReg), OffsetOfContextField(tmp1), SLJIT_IMM, (reinterpret_cast<TableInit*>(instr->byteCode()))->tableIndex());
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(VOID, 32, 32, 32, W), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, initTable));
        break;
    }
    case ByteCode::TableSizeOpcode: {
        JITArg dstArg;

        operandToArg(instr->operands(), dstArg);

        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_MEM1(kContextReg), OffsetOfContextField(instance));
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_MEM1(SLJIT_R0), context->tableStart + ((reinterpret_cast<TableSize*>(instr->byteCode()))->tableIndex() * sizeof(void*)));
        sljit_emit_op1(compiler, SLJIT_MOV32, dstArg.arg, dstArg.argw, SLJIT_MEM1(SLJIT_R0), JITFieldAccessor::tableSizeOffset());
        break;
    }
    case ByteCode::TableCopyOpcode: {
        emitLoad3Arguments(compiler, instr->operands());

        sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_MEM1(kContextReg), OffsetOfContextField(tmp1), SLJIT_IMM, ((reinterpret_cast<TableCopy*>(instr->byteCode()))->srcIndex()));
        sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_MEM1(kContextReg), OffsetOfContextField(tmp2), SLJIT_IMM, ((reinterpret_cast<TableCopy*>(instr->byteCode()))->dstIndex()));
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(VOID, 32, 32, 32, W), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, copyTable));
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_R0, 0);
        sljit_set_label(sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, ExecutionContext::NoError), context->trapLabel);
        break;
    }
    case ByteCode::TableFillOpcode: {
        emitLoad3Arguments(compiler, instr->operands());

        sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_MEM1(kContextReg), OffsetOfContextField(tmp1), SLJIT_IMM, ((reinterpret_cast<TableFill*>(instr->byteCode()))->tableIndex()));
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(VOID, 32, P, 32, W), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, fillTable));
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_R0, 0);
        sljit_set_label(sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, ExecutionContext::NoError), context->trapLabel);
        break;
    }
    case ByteCode::TableGrowOpcode: {
        JITArg arg;

        operandToArg(instr->operands(), arg);

        MOVE_TO_REG(compiler, SLJIT_MOV32, SLJIT_R0, arg.arg, arg.argw);

        operandToArg(instr->operands() + 1, arg);

        MOVE_TO_REG(compiler, SLJIT_MOV_P, SLJIT_R1, arg.arg, arg.argw);
        sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_R2, 0, SLJIT_IMM, ((reinterpret_cast<TableGrow*>(instr->byteCode()))->tableIndex()));
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R3, 0, kContextReg, 0);
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(W, P, 32, 32, W), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, growTable));

        operandToArg(instr->operands() + 2, arg);

        MOVE_FROM_REG(compiler, SLJIT_MOV32, arg.arg, arg.argw, SLJIT_R0);
        break;
    }
    case ByteCode::TableSetOpcode: {
        JITArg srcArg0;
        JITArg srcArg1;

        operandToArg(instr->operands(), srcArg0);
        operandToArg(instr->operands() + 1, srcArg1);

        sljit_s32 sourceReg = GET_SOURCE_REG(srcArg0.arg, SLJIT_R0);

        if (!IS_SOURCE_REG(srcArg0.arg)) {
            MOVE_TO_REG(compiler, SLJIT_MOV32, sourceReg, srcArg0.arg, srcArg0.argw);
            srcArg0.arg = sourceReg;
            srcArg0.argw = 0;
        }

        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R1, 0, SLJIT_MEM1(kContextReg), OffsetOfContextField(instance));
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM, ExecutionContext::OutOfBoundsTableAccessError);
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R1, 0, SLJIT_MEM1(SLJIT_R1), context->tableStart + ((reinterpret_cast<TableSet*>(instr->byteCode()))->tableIndex() * sizeof(void*)));
        sljit_set_label(sljit_emit_cmp(compiler, SLJIT_GREATER_EQUAL | SLJIT_32, sourceReg, 0, SLJIT_MEM1(SLJIT_R1), JITFieldAccessor::tableSizeOffset()), context->trapLabel);
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R1, 0, SLJIT_MEM1(SLJIT_R1), JITFieldAccessor::tableElements());
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM2(SLJIT_R1, sourceReg), SLJIT_WORD_SHIFT, srcArg1.arg, srcArg1.argw);
        break;
    }
    case ByteCode::TableGetOpcode: {
        JITArg srcArg;

        operandToArg(instr->operands(), srcArg);

        sljit_s32 sourceReg = GET_SOURCE_REG(srcArg.arg, SLJIT_R0);

        if (!IS_SOURCE_REG(srcArg.arg)) {
            MOVE_TO_REG(compiler, SLJIT_MOV32, sourceReg, srcArg.arg, srcArg.argw);
            srcArg.arg = sourceReg;
            srcArg.argw = 0;
        }

        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R1, 0, SLJIT_MEM1(kContextReg), OffsetOfContextField(instance));
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM, ExecutionContext::OutOfBoundsTableAccessError);
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R1, 0, SLJIT_MEM1(SLJIT_R1), context->tableStart + ((reinterpret_cast<TableGet*>(instr->byteCode()))->tableIndex() * sizeof(void*)));
        sljit_set_label(sljit_emit_cmp(compiler, SLJIT_GREATER_EQUAL | SLJIT_32, sourceReg, 0, SLJIT_MEM1(SLJIT_R1), JITFieldAccessor::tableSizeOffset()), context->trapLabel);

        JITArg dstArg;

        operandToArg(instr->operands() + 1, dstArg);

        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R1, 0, SLJIT_MEM1(SLJIT_R1), JITFieldAccessor::tableElements());
        sljit_emit_op1(compiler, SLJIT_MOV_P, dstArg.arg, dstArg.argw, SLJIT_MEM2(SLJIT_R1, sourceReg), SLJIT_WORD_SHIFT);
        break;
    }
    default: {
        return;
    }
    }
}
