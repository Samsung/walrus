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
    emitStoreImmediateParams(compiler, instr);

    sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_IMM, reinterpret_cast<sljit_sw>(instr->byteCode()));
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, kFrameReg, 0);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, kContextReg, 0);

    sljit_sw addr = GET_FUNC_ADDR(sljit_sw, callFunction);
    if (instr->opcode() == ByteCode::CallIndirectOpcode) {
        addr = GET_FUNC_ADDR(sljit_sw, callFunctionIndirect);
    }

    sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS3(W, W, W, W), SLJIT_IMM, addr);

    sljit_jump* jump = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, ExecutionContext::NoError);
    CompileContext* context = CompileContext::get(compiler);

    if (context->currentTryBlock == InstanceConstData::globalTryBlock) {
        context->appendTrapJump(ExecutionContext::ReturnToLabel, jump);
        return;
    }

    std::vector<TryBlock>& tryBlocks = context->compiler->tryBlocks();
    tryBlocks[context->currentTryBlock].throwJumps.push_back(jump);
}
