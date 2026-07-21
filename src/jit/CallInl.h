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

static sljit_sw callFunctionIndirectM64(
    CallIndirect* code,
    uint8_t* bp,
    ExecutionContext* context)
{
    Instance* instance = context->instance;
    Table* table = instance->table(code->tableIndex());

    uint64_t idx = *reinterpret_cast<uint64_t*>(bp + code->calleeOffset());
    if (idx >= table->size()) {
        context->error = ExecutionContext::UndefinedElementError;
        return ExecutionContext::UndefinedElementError;
    }

    auto target = reinterpret_cast<Function*>(table->uncheckedGetElementM64(idx));
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

static sljit_sw callFunctionRef(
    CallRef* code,
    uint8_t* bp,
    ExecutionContext* context)
{
    Instance* instance = context->instance;

    auto target = *reinterpret_cast<Function**>(bp + code->calleeOffset());
    if (UNLIKELY(Value::isNull(target))) {
        context->error = ExecutionContext::NullFunctionReferenceError;
        return ExecutionContext::NullFunctionReferenceError;
    }

    const FunctionType* ft = target->functionType();
    if (!ft->equals(code->functionType())) {
        context->error = ExecutionContext::CallRefTypeMismatchError;
        return ExecutionContext::CallRefTypeMismatchError;
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

static sljit_sw shuffleTailCallSelfArguments(uint8_t* bp, ByteCodeStackOffset* offsets, sljit_uw parameterOffsetCount)
{
    Vector<size_t> paramBuffer;
    paramBuffer.resizeWithUninitializedValues(parameterOffsetCount);

    for (sljit_uw i = 0; i < parameterOffsetCount; i++) {
        paramBuffer[i] = *reinterpret_cast<size_t*>(bp + offsets[i]);
    }

    for (sljit_uw i = 0; i < parameterOffsetCount; i++) {
        reinterpret_cast<size_t*>(bp)[i] = paramBuffer[i];
    }

    return ExecutionContext::NoError;
}

static void emitCall(sljit_compiler* compiler, Instruction* instr)
{
    FunctionType* functionType;
    CompileContext* context = CompileContext::get(compiler);
    ByteCodeStackOffset* stackOffset;
    sljit_sw addr;
    ByteCodeStackOffset calleeOffset = 0;
    uint8_t calleeType = 0;

    switch (instr->opcode()) {
    case ByteCode::CallOpcode: {
        Call* call = reinterpret_cast<Call*>(instr->byteCode());
        addr = GET_FUNC_ADDR(sljit_sw, callFunction);
        functionType = context->compiler->module()->function(call->index())->functionType();
        stackOffset = call->stackOffsets();
        break;
    }
    case ByteCode::ReturnCallOpcode: {
        ReturnCall* call = reinterpret_cast<ReturnCall*>(instr->byteCode());
        addr = GET_FUNC_ADDR(sljit_sw, callFunction);
        functionType = context->compiler->module()->function(call->index())->functionType();
        stackOffset = call->stackOffsets();
        break;
    }
    case ByteCode::CallIndirectOpcode:
    case ByteCode::CallIndirectM64Opcode:
    case ByteCode::ReturnCallIndirectOpcode:
    case ByteCode::ReturnCallIndirectM64Opcode: {
        CallTable* callTable = reinterpret_cast<CallTable*>(instr->byteCode());
        if (instr->opcode() == ByteCode::CallIndirectOpcode || instr->opcode() == ByteCode::ReturnCallIndirectOpcode) {
            addr = GET_FUNC_ADDR(sljit_sw, callFunctionIndirect);
            calleeType = Instruction::Int32Operand;
        } else {
            addr = GET_FUNC_ADDR(sljit_sw, callFunctionIndirectM64);
            calleeType = Instruction::Int64Operand;
        }
        functionType = callTable->functionType();
        stackOffset = callTable->stackOffsets();
        calleeOffset = callTable->calleeOffset();
        break;
    }
    case ByteCode::CallRefOpcode: {
        CallRef* callRef = reinterpret_cast<CallRef*>(instr->byteCode());
        addr = GET_FUNC_ADDR(sljit_sw, callFunctionRef);
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        calleeType = Instruction::Int64Operand;
#else /* !SLJIT_64BIT_ARCHITECTURE */
        calleeType = Instruction::Int32Operand;
#endif /* SLJIT_64BIT_ARCHITECTURE */
        functionType = callRef->functionType();
        stackOffset = callRef->stackOffsets();
        calleeOffset = callRef->calleeOffset();
        break;
    }
    default: {
        ASSERT(instr->opcode() == ByteCode::ReturnCallRefOpcode);
        ReturnCallRef* callRef = reinterpret_cast<ReturnCallRef*>(instr->byteCode());
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        calleeType = Instruction::Int64Operand;
#else /* !SLJIT_64BIT_ARCHITECTURE */
        calleeType = Instruction::Int32Operand;
#endif /* SLJIT_64BIT_ARCHITECTURE */
        addr = GET_FUNC_ADDR(sljit_sw, callFunctionRef);
        functionType = callRef->functionType();
        stackOffset = callRef->stackOffsets();
        calleeOffset = callRef->calleeOffset();
        break;
    }
    }

    Operand* operand = instr->operands();

    if (instr->opcode() == ByteCode::ReturnCallOpcode) {
        ReturnCall* returnCall = reinterpret_cast<ReturnCall*>(instr->byteCode());

        // Detect memory offset instr for copy all oprands related to memory
        bool hasMemoryArgument = false;
        for (uint32_t i = 0; i < instr->paramCount(); i++) {
            if (VARIABLE_TYPE(operand[i]) == Instruction::Offset) {
                hasMemoryArgument = true;
                break;
            }
        }

        if (!hasMemoryArgument) {
            sljit_sw paramOffset = 0;

            for (auto it : functionType->param().types()) {
                Operand dst = VARIABLE_SET(STACK_OFFSET(paramOffset), Instruction::Offset);

                if (VARIABLE_TYPE(*operand) == Instruction::ConstPtr) {
                    ASSERT(!(VARIABLE_GET_IMM(*operand)->info() & Instruction::kKeepInstruction));
                    emitStoreImmediate(compiler, &dst, VARIABLE_GET_IMM(*operand), false);
                } else {
                    ASSERT(VARIABLE_TYPE(*operand) == Instruction::Register);
                    emitMove(compiler, Instruction::valueTypeToOperandType(it), operand, &dst);
                }

                paramOffset += static_cast<sljit_sw>(valueStackAllocatedSize(it));
                operand++;
            }
        } else {
            // At least one argument is in memory and may alias a parameter slot.
            // arguments are all shuffled when start to enter a function.
            emitStoreOntoStack(compiler, operand, returnCall->stackOffsets(), functionType->param(), true);

            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, kFrameReg, 0);
            sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R1, 0, SLJIT_IMM, reinterpret_cast<sljit_sw>(returnCall->stackOffsets()));
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM, static_cast<sljit_sw>(returnCall->parameterOffsetsSize()));
            sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS3(W, W, W, W), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, shuffleTailCallSelfArguments));
        }

        sljit_set_label(sljit_emit_jump(compiler, SLJIT_JUMP), context->tailCallLabel);
        return;
    }

    stackOffset = emitStoreOntoStack(compiler, operand, stackOffset, functionType->param(), true);
    operand += instr->paramCount();

    ByteCode::Opcode callOpcode = instr->opcode();
    if (calleeType != 0) {
        Operand dst = VARIABLE_SET(STACK_OFFSET(calleeOffset), Instruction::Offset);
        operand--;

        switch (VARIABLE_TYPE(*operand)) {
        case Instruction::ConstPtr:
            ASSERT(!(VARIABLE_GET_IMM(*operand)->info() & Instruction::kKeepInstruction));
            emitStoreImmediate(compiler, &dst, VARIABLE_GET_IMM(*operand), false);
            break;
        case Instruction::Register:
            emitMove(compiler, calleeType, operand, &dst);
            break;
        }
        operand++;
    }

    sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_IMM, reinterpret_cast<sljit_sw>(instr->byteCode()));
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, kFrameReg, 0);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_MEM1(SLJIT_SP), kContextOffset);
    sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS3(W, W, W, W), SLJIT_IMM, addr);

    sljit_jump* jump = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, ExecutionContext::NoError);

    for (auto it : functionType->result().types()) {
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
