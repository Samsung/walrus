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

static sljit_sw callFunction(
    Call* code,
    uint8_t* bp,
    ExecutionContext* context)
{
    Instance* instance = context->instance;
    Function* target = instance->function(code->index());

    sljit_sw error = ExecutionContext::NoError;
    try {
        target->interpreterCall(context->state, bp, code->stackOffsets(), code->parameterOffsetsSize(), code->resultOffsetsSize());
    } catch (std::unique_ptr<Exception>& exception) {
        context->capturedException = exception.release();
        context->error = ExecutionContext::CapturedException;
        error = ExecutionContext::CapturedException;
    }

    return error;
}

static sljit_sw callFunctionIndirect(
    CallIndirect* code,
    uint8_t* bp,
    ExecutionContext* context)
{
    Instance* instance = context->instance;
    Table* table = instance->table(code->tableIndex());

    uint32_t idx = *reinterpret_cast<uint32_t*>(bp + code->calleeOffset());
    if (idx >= table->size()) {
        context->error = ExecutionContext::UndefinedElementError;
        return ExecutionContext::UndefinedElementError;
    }

    auto target = reinterpret_cast<Function*>(table->uncheckedGetElement(idx));
    if (UNLIKELY(Value::isNull(target))) {
        context->error = ExecutionContext::UninitializedElementError;
        return ExecutionContext::UninitializedElementError;
    }

    const FunctionType* ft = target->functionType();
    if (!ft->equals(code->functionType())) {
        context->error = ExecutionContext::IndirectCallTypeMismatchError;
        return ExecutionContext::IndirectCallTypeMismatchError;
    }

    sljit_sw error = ExecutionContext::NoError;
    try {
        target->interpreterCall(context->state, bp, code->stackOffsets(), code->parameterOffsetsSize(), code->resultOffsetsSize());
    } catch (std::unique_ptr<Exception>& exception) {
        context->capturedException = exception.release();
        context->error = ExecutionContext::CapturedException;
        error = ExecutionContext::CapturedException;
    }

    return error;
}

static void emitCall(sljit_compiler* compiler, Instruction* instr)
{
    FunctionType* functionType;
    CompileContext* context = CompileContext::get(compiler);
    ByteCodeStackOffset* stackOffset;
    sljit_sw addr;

    if (instr->opcode() == ByteCode::CallOpcode) {
        Call* call = reinterpret_cast<Call*>(instr->byteCode());
        addr = GET_FUNC_ADDR(sljit_sw, callFunction);
        functionType = context->compiler->module()->function(call->index())->functionType();
        stackOffset = call->stackOffsets();
    } else {
        CallIndirect* callIndirect = reinterpret_cast<CallIndirect*>(instr->byteCode());
        addr = GET_FUNC_ADDR(sljit_sw, callFunctionIndirect);
        functionType = callIndirect->functionType();
        stackOffset = callIndirect->stackOffsets();
    }

    Operand* operand = instr->operands();
    stackOffset = emitStoreOntoStack(compiler, operand, stackOffset, functionType->param(), true);
    operand += instr->paramCount();

    if (instr->opcode() == ByteCode::CallIndirectOpcode) {
        CallIndirect* callIndirect = reinterpret_cast<CallIndirect*>(instr->byteCode());
        operand--;

        switch (VARIABLE_TYPE(*operand)) {
        case Instruction::ConstPtr: {
            ASSERT(!(VARIABLE_GET_IMM(*operand)->info() & Instruction::kKeepInstruction));
            Const32* value = reinterpret_cast<Const32*>(VARIABLE_GET_IMM(*operand)->byteCode());
            sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_MEM1(kFrameReg), callIndirect->calleeOffset(), SLJIT_IMM, static_cast<sljit_s32>(value->value()));
            break;
        }
        case Instruction::Register: {
            sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_MEM1(kFrameReg), callIndirect->calleeOffset(), static_cast<sljit_s32>(VARIABLE_GET_REF(*operand)), 0);
            break;
        }
        }

        operand++;
    }

    sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_IMM, reinterpret_cast<sljit_sw>(instr->byteCode()));
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, kFrameReg, 0);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, kContextReg, 0);

    sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS3(W, W, W, W), SLJIT_IMM, addr);

    sljit_jump* jump = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, ExecutionContext::NoError);

    for (auto it : functionType->result()) {
        ASSERT(VARIABLE_TYPE(*operand) != Instruction::ConstPtr);

        if (VARIABLE_TYPE(*operand) == Instruction::Register) {
            Operand src = VARIABLE_SET(STACK_OFFSET(*stackOffset), Instruction::Offset);
            emitMove(compiler, Instruction::valueTypeToOperandType(it), &src, operand);
        }

        operand++;
        stackOffset += (valueSize(it) + (sizeof(size_t) - 1)) / sizeof(size_t);
    }

    if (context->currentTryBlock == InstanceConstData::globalTryBlock) {
        context->appendTrapJump(ExecutionContext::ReturnToLabel, jump);
        return;
    }

    std::vector<TryBlock>& tryBlocks = context->compiler->tryBlocks();
    tryBlocks[context->currentTryBlock].throwJumps.push_back(jump);
}
