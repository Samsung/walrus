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

static void emitGCUnary(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[2] = { operands, operands + 1 };

    switch (instr->opcode()) {
    case ByteCode::RefI31Opcode: {
        if (SLJIT_IS_IMM(args[0].arg)) {
            sljit_emit_op1(compiler, SLJIT_MOV, args[1].arg, args[1].argw, SLJIT_IMM,
                           (args[0].argw << Value::RefI31Shift) | static_cast<sljit_sw>(Value::RefI31));
            return;
        }

        sljit_s32 srcReg = args[0].arg;
        sljit_s32 dstReg = GET_TARGET_REG(args[1].arg, SLJIT_TMP_DEST_REG);
        if (!SLJIT_IS_REG(srcReg)) {
            sljit_emit_op1(compiler, SLJIT_MOV_S32, dstReg, 0, args[0].arg, args[0].argw);
            srcReg = dstReg;
        }
        sljit_emit_op2(compiler, SLJIT_SHL, dstReg, 0, srcReg, 0, SLJIT_IMM, static_cast<sljit_sw>(Value::RefI31Shift));
        sljit_emit_op2(compiler, SLJIT_OR, dstReg, 0, dstReg, 0, SLJIT_IMM, static_cast<sljit_sw>(Value::RefI31));
        if (args[1].arg != dstReg) {
            sljit_emit_op1(compiler, SLJIT_MOV, args[1].arg, args[1].argw, dstReg, 0);
        }
        break;
    }
    case ByteCode::I31GetSOpcode:
    case ByteCode::I31GetUOpcode: {
        sljit_s32 dstReg = GET_TARGET_REG(args[1].arg, SLJIT_TMP_DEST_REG);

        if (reinterpret_cast<I31Get*>(instr->byteCode())->isNullable()) {
            CompileContext* context = CompileContext::get(compiler);

            if (!SLJIT_IS_REG(args[0].arg)) {
                sljit_emit_op1(compiler, SLJIT_MOV, dstReg, 0, args[0].arg, args[0].argw);
                args[0].arg = dstReg;
                args[0].argw = 0;
            }

            context->appendTrapJump(ExecutionContext::NullI31ReferenceError,
                                    sljit_emit_cmp(compiler, SLJIT_EQUAL, args[0].arg, args[0].argw, SLJIT_IMM, 0));
        } else {
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            sljit_emit_op2(compiler, instr->opcode() == ByteCode::I31GetSOpcode ? SLJIT_ASHR : SLJIT_LSHR,
                           args[1].arg, args[1].argw, args[0].arg, args[0].argw, SLJIT_IMM, static_cast<sljit_sw>(Value::RefI31Shift));
            return;
#endif /* SLJIT_32BIT_ARCHITECTURE */
        }

        sljit_emit_op2(compiler, instr->opcode() == ByteCode::I31GetSOpcode ? SLJIT_ASHR : SLJIT_LSHR,
                       dstReg, 0, args[0].arg, args[0].argw, SLJIT_IMM, static_cast<sljit_sw>(Value::RefI31Shift));

        sljit_emit_op1(compiler, SLJIT_IS_REG(args[1].arg) ? SLJIT_MOV32 : SLJIT_MOV_S32,
                       args[1].arg, args[1].argw, dstReg, 0);
        break;
    }
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
}

static void emitGCCast(sljit_compiler* compiler, Instruction* instr)
{
    CompileContext* context = CompileContext::get(compiler);
    const CompositeType** typeInfo = nullptr;
    Value::Type genericType = Value::Void;
    uint8_t srcInfo = 0;
    Operand* operands = instr->operands();
    JITArg args[2];
    bool isTest = false;

    args[0].set(operands);

    switch (instr->opcode()) {
    case ByteCode::RefCastGenericOpcode: {
        RefCastGeneric* refCastGeneric = reinterpret_cast<RefCastGeneric*>(instr->byteCode());
        genericType = refCastGeneric->typeInfo();
        srcInfo = refCastGeneric->srcInfo();
        break;
    }
    case ByteCode::RefCastDefinedOpcode: {
        RefCastDefined* refCastDefined = reinterpret_cast<RefCastDefined*>(instr->byteCode());
        typeInfo = refCastDefined->typeInfo();
        srcInfo = refCastDefined->srcInfo();
        break;
    }
    case ByteCode::RefTestGenericOpcode: {
        RefTestGeneric* refTestGeneric = reinterpret_cast<RefTestGeneric*>(instr->byteCode());
        genericType = refTestGeneric->typeInfo();
        srcInfo = refTestGeneric->srcInfo();
        isTest = true;
        break;
    }
    case ByteCode::RefTestDefinedOpcode: {
        RefTestDefined* refTestDefined = reinterpret_cast<RefTestDefined*>(instr->byteCode());
        typeInfo = refTestDefined->typeInfo();
        srcInfo = refTestDefined->srcInfo();
        isTest = true;
        break;
    }
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }

    // The code has specialzed cases for many checks.
    if (genericType == Value::AnyRef) {
        if ((srcInfo & RefCastGeneric::IsNullable) != 0) {
            // Everything matches to this case. Redundant.
            if (isTest) {
                sljit_emit_op1(compiler, SLJIT_MOV, args[1].arg, args[1].argw, SLJIT_IMM, 1);
            }
            return;
        }

        // Null check.
        if (!isTest) {
            context->appendTrapJump(ExecutionContext::NullReferenceError,
                                    sljit_emit_cmp(compiler, SLJIT_EQUAL, args[0].arg, args[0].argw, SLJIT_IMM, 0));
        } else {
            sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_Z, args[0].arg, args[0].argw, SLJIT_IMM, 0);
            sljit_emit_op_flags(compiler, SLJIT_MOV, args[1].arg, args[1].argw, SLJIT_NOT_EQUAL);
        }
        return;
    }

    if (genericType == Value::I31Ref) {
        if ((srcInfo & RefCastGeneric::IsNullable) == 0) {
            sljit_emit_op2u(compiler, SLJIT_AND | SLJIT_SET_Z, args[0].arg, args[0].argw, SLJIT_IMM, static_cast<sljit_sw>(Value::RefI31));

            if (!isTest) {
                context->appendTrapJump(ExecutionContext::CastFailureError, sljit_emit_jump(compiler, SLJIT_ZERO));
            } else {
                sljit_emit_op_flags(compiler, SLJIT_MOV, args[1].arg, args[1].argw, SLJIT_NOT_ZERO);
            }
            return;
        }

        sljit_emit_op2(compiler, SLJIT_ROTR, SLJIT_TMP_DEST_REG, 0, args[0].arg, args[0].argw, SLJIT_IMM, 1);

        if (!isTest) {
            context->appendTrapJump(ExecutionContext::CastFailureError, sljit_emit_cmp(compiler, SLJIT_SIG_GREATER, SLJIT_TMP_DEST_REG, 0, SLJIT_IMM, 0));
        } else {
            sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_SIG_LESS_EQUAL, SLJIT_TMP_DEST_REG, 0, SLJIT_IMM, 0);
            sljit_emit_op_flags(compiler, SLJIT_MOV, args[1].arg, args[1].argw, SLJIT_SIG_LESS_EQUAL);
        }
        return;
    }

    // TODO: Unimplemented cases.
    SLJIT_UNUSED_ARG(typeInfo);
    abort();
}

static void emitGCStructNew(sljit_compiler* compiler, Instruction* instr)
{
    CompileContext* context = CompileContext::get(compiler);

    if (instr->opcode() == ByteCode::StructNewDefaultOpcode) {
        StructNewDefault* structNewDefault = reinterpret_cast<StructNewDefault*>(instr->byteCode());
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, reinterpret_cast<sljit_sw>(structNewDefault->typeInfo()));
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS1(P, P), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, GCStruct::structNewDefault));
        context->appendTrapJump(ExecutionContext::AllocationError,
                                sljit_emit_cmp(compiler, SLJIT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, 0));
        JITArg dstArg(instr->operands());
        MOVE_FROM_REG(compiler, SLJIT_MOV, dstArg.arg, dstArg.argw, SLJIT_R0);
        return;
    }

    StructNew* structNew = reinterpret_cast<StructNew*>(instr->byteCode());
    ByteCodeStackOffset* stackOffset = structNew->dataOffsets();
    Operand* param = instr->params();

    for (auto it : structNew->typeInfo()->fields()) {
        Operand dst = VARIABLE_SET(STACK_OFFSET(*stackOffset), Instruction::Offset);

        switch (VARIABLE_TYPE(*param)) {
        case Instruction::ConstPtr:
            ASSERT(!(VARIABLE_GET_IMM(*param)->info() & Instruction::kKeepInstruction));
            emitStoreImmediate(compiler, &dst, VARIABLE_GET_IMM(*param), false);
            break;
        case Instruction::Register:
            emitMove(compiler, Instruction::valueTypeToOperandType(it.type()), param, &dst);
            break;
        }

        stackOffset++;
        param++;
    }

    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, reinterpret_cast<sljit_sw>(structNew->typeInfo()));
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_IMM, reinterpret_cast<sljit_sw>(structNew->dataOffsets()));
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, kFrameReg, 0);
    sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS3(P, P, P, P), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, GCStruct::structNew));
    context->appendTrapJump(ExecutionContext::AllocationError,
                            sljit_emit_cmp(compiler, SLJIT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, 0));
    JITArg dstArg(param);
    MOVE_FROM_REG(compiler, SLJIT_MOV, dstArg.arg, dstArg.argw, SLJIT_R0);
}

static void emitGCStructAccess(sljit_compiler* compiler, Instruction* instr)
{
    bool isGet = true;
    sljit_sw memberOffset;
    Value::Type type;
    bool isNullable;
    bool isSigned = false;

    if (instr->opcode() == ByteCode::StructGetOpcode) {
        StructGet* structGet = reinterpret_cast<StructGet*>(instr->byteCode());
        memberOffset = static_cast<sljit_sw>(structGet->memberOffset());
        type = structGet->type();
        isNullable = structGet->isNullable();
        isSigned = structGet->isSigned();
    } else {
        StructSet* structSet = reinterpret_cast<StructSet*>(instr->byteCode());
        isGet = false;
        memberOffset = static_cast<sljit_sw>(structSet->memberOffset());
        type = structSet->type();
        isNullable = structSet->isNullable();
    }

    JITArg structArg(instr->operands());
    sljit_s32 structReg = GET_TARGET_REG(structArg.arg, SLJIT_TMP_DEST_REG);
    MOVE_TO_REG(compiler, SLJIT_MOV, structReg, structArg.arg, structArg.argw);

    if (isNullable) {
        CompileContext::get(compiler)->appendTrapJump(ExecutionContext::NullStructReferenceError,
                                                      sljit_emit_cmp(compiler, SLJIT_EQUAL, structReg, 0, SLJIT_IMM, 0));
    }

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    if (type == Value::I64) {
        JITArgPair argPair(instr->operands() + 1);

        if (SLJIT_IS_MEM(argPair.arg1)) {
            // Copy as double
            if (isGet) {
                sljit_emit_fop1(compiler, SLJIT_MOV_F64, argPair.arg1, argPair.arg1w, SLJIT_MEM1(structReg), memberOffset);
            } else {
                sljit_emit_fop1(compiler, SLJIT_MOV_F64, SLJIT_MEM1(structReg), memberOffset, argPair.arg1, argPair.arg1w);
            }
            return;
        }

        if (isGet) {
            sljit_emit_op1(compiler, SLJIT_MOV, argPair.arg1, argPair.arg1w, SLJIT_MEM1(structReg), memberOffset + WORD_LOW_OFFSET);
            sljit_emit_op1(compiler, SLJIT_MOV, argPair.arg2, argPair.arg2w, SLJIT_MEM1(structReg), memberOffset + WORD_HIGH_OFFSET);
        } else {
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(structReg), memberOffset + WORD_LOW_OFFSET, argPair.arg1, argPair.arg1w);
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(structReg), memberOffset + WORD_HIGH_OFFSET, argPair.arg2, argPair.arg2w);
        }
        return;
    }
#endif /* SLJIT_32BIT_ARCHITECTURE */

    sljit_s32 op;

    switch (type) {
    case Value::I8:
        op = isSigned ? SLJIT_MOV32_S8 : SLJIT_MOV32_U8;
        break;
    case Value::I16:
        op = isSigned ? SLJIT_MOV32_S16 : SLJIT_MOV32_U16;
        break;
    case Value::I32:
        op = SLJIT_MOV32;
        break;
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
    case Value::I64:
        op = SLJIT_MOV;
        break;
#endif /* SLJIT_64BIT_ARCHITECTURE */
    case Value::F32:
    case Value::F64: {
        JITArg arg;
        // Immediate value is only used when the operation is get
        floatOperandToArg(compiler, instr->operands() + 1, arg, SLJIT_TMP_DEST_FREG);
        op = (type == Value::F32) ? SLJIT_MOV_F32 : SLJIT_MOV_F64;

        if (isGet) {
            sljit_emit_fop1(compiler, op, arg.arg, arg.argw, SLJIT_MEM1(structReg), memberOffset);
        } else {
            sljit_emit_fop1(compiler, op, SLJIT_MEM1(structReg), memberOffset, arg.arg, arg.argw);
        }
        return;
    }
    case Value::V128: {
        JITArg arg;

        if (!isGet) {
            simdOperandToArg(compiler, instr->operands() + 1, arg, SLJIT_SIMD_REG_128, SLJIT_TMP_DEST_VREG);
            sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128, arg.arg, SLJIT_MEM1(structReg), memberOffset);
            return;
        }

        arg.set(instr->operands() + 1);
        sljit_s32 reg = GET_TARGET_REG(arg.arg, SLJIT_TMP_DEST_VREG);

        sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128, reg, SLJIT_MEM1(structReg), memberOffset);
        if (arg.arg != reg) {
            sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128, reg, arg.arg, arg.argw);
        }
        return;
    }
    default:
        op = SLJIT_MOV;
        break;
    }

    JITArg arg(instr->operands() + 1);
    sljit_s32 reg = GET_SOURCE_REG(arg.arg, SLJIT_TMP_DEST_REG);

    if (isGet) {
        sljit_emit_op1(compiler, op, reg, 0, SLJIT_MEM1(structReg), memberOffset);
        MOVE_FROM_REG(compiler, op == SLJIT_MOV ? SLJIT_MOV : SLJIT_MOV32, arg.arg, arg.argw, reg);
        return;
    }

#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
    if (structReg != SLJIT_TMP_DEST_REG
        || (!SLJIT_IS_MEM(arg.arg) && (type != Value::I8 || !SLJIT_IS_REG(arg.arg) || sljit_get_register_index(SLJIT_GP_REGISTER, arg.arg) < 4))) {
        sljit_emit_op1(compiler, op, SLJIT_MEM1(structReg), memberOffset, arg.arg, arg.argw);
        return;
    }

    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R3, 0, SLJIT_R0, 0);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, arg.arg, arg.argw);
    sljit_emit_op1(compiler, op, SLJIT_MEM1(structReg), memberOffset, SLJIT_R0, 0);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_R3, 0);
#else /* !SLJIT_CONFIG_X86_32 */
#ifndef SLJIT_TMP_OPT_REG
#error "Missing implementation"
#endif
    if (!SLJIT_IS_MEM(arg.arg) || structReg != SLJIT_TMP_DEST_REG) {
        sljit_emit_op1(compiler, op, SLJIT_MEM1(structReg), memberOffset, arg.arg, arg.argw);
        return;
    }

    sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_TMP_DEST_REG, 0, structReg, 0, SLJIT_IMM, memberOffset);
    sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_TMP_OPT_REG, 0, arg.arg, arg.argw);
    sljit_emit_op1(compiler, op, SLJIT_MEM1(SLJIT_TMP_DEST_REG), 0, SLJIT_TMP_OPT_REG, 0);
#endif /* SLJIT_CONFIG_X86_32 */
}
