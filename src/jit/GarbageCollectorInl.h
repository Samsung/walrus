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
    case ByteCode::ArrayLenOpcode: {
        Operand* operands = instr->operands();
        JITArg arg[2] = { operands, operands + 1 };

        sljit_s32 dstReg = args[0].arg;
        if (!SLJIT_IS_REG(dstReg)) {
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_TMP_DEST_REG, 0, args[0].arg, args[0].argw);
            dstReg = SLJIT_TMP_DEST_REG;
        }
        sljit_emit_op1(compiler, SLJIT_MOV, args[1].arg, args[1].argw, SLJIT_MEM1(dstReg), JITFieldAccessor::arrayLength());
        break;
    }
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
}

static void emitGCCastGeneric(sljit_compiler* compiler, Instruction* instr)
{
    CompileContext* context = CompileContext::get(compiler);
    Value::Type genericType = Value::Void;
    uint8_t srcInfo = 0;
    Operand* operands = instr->operands();
    JITArg args[2];
    bool isTestOrCastFail = false;
    Label* label = nullptr;

    args[0].set(operands);

    switch (instr->opcode()) {
    case ByteCode::RefCastGenericOpcode: {
        RefCastGeneric* refCastGeneric = reinterpret_cast<RefCastGeneric*>(instr->byteCode());
        genericType = refCastGeneric->typeInfo();
        srcInfo = refCastGeneric->srcInfo();
        break;
    }
    case ByteCode::RefTestGenericOpcode: {
        RefTestGeneric* refTestGeneric = reinterpret_cast<RefTestGeneric*>(instr->byteCode());
        genericType = refTestGeneric->typeInfo();
        srcInfo = refTestGeneric->srcInfo();
        isTestOrCastFail = true;
        args[1].set(operands + 1);
        break;
    }
    case ByteCode::JumpIfCastGenericOpcode: {
        JumpIfCastGeneric* jumpIfCastGeneric = reinterpret_cast<JumpIfCastGeneric*>(instr->byteCode());
        genericType = jumpIfCastGeneric->typeInfo();
        srcInfo = jumpIfCastGeneric->srcInfo();
        label = instr->asExtended()->value().targetLabel;
        isTestOrCastFail = (srcInfo & JumpIfCastGeneric::IsCastFail) != 0;
        break;
    }
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }

    // The code has specialzed cases for many checks.
    if (genericType == Value::AnyRef) {
        if ((srcInfo & JumpIfCastGeneric::IsNullable) != 0 || (srcInfo & JumpIfCastGeneric::IsSrcNullable) == 0) {
            // Everything matches to this case. Redundant.
            if (label != nullptr) {
                if (!isTestOrCastFail) {
                    label->jumpFrom(sljit_emit_jump(compiler, SLJIT_JUMP));
                }
            } else if (isTestOrCastFail) {
                sljit_emit_op1(compiler, SLJIT_MOV, args[1].arg, args[1].argw, SLJIT_IMM, 1);
            }
            return;
        }

        // Null check.
        if (label != nullptr) {
            label->jumpFrom(sljit_emit_cmp(compiler, isTestOrCastFail ? SLJIT_EQUAL : SLJIT_NOT_EQUAL, args[0].arg, args[0].argw, SLJIT_IMM, 0));
        } else if (!isTestOrCastFail) {
            context->appendTrapJump(ExecutionContext::CastFailureError,
                                    sljit_emit_cmp(compiler, SLJIT_EQUAL, args[0].arg, args[0].argw, SLJIT_IMM, 0));
        } else {
            sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_Z, args[0].arg, args[0].argw, SLJIT_IMM, 0);
            sljit_emit_op_flags(compiler, SLJIT_MOV, args[1].arg, args[1].argw, SLJIT_NOT_EQUAL);
        }
        return;
    }

    if (genericType == Value::NoAnyRef) {
        if ((srcInfo & JumpIfCastGeneric::IsNullable) == 0 || (srcInfo & JumpIfCastGeneric::IsSrcNullable) == 0) {
            // Nothing matches to this case. Redundant.
            if (label != nullptr) {
                if (isTestOrCastFail) {
                    label->jumpFrom(sljit_emit_jump(compiler, SLJIT_JUMP));
                }
            } else if (!isTestOrCastFail) {
                context->appendTrapJump(ExecutionContext::CastFailureError, sljit_emit_jump(compiler, SLJIT_JUMP));
            } else {
                sljit_emit_op1(compiler, SLJIT_MOV, args[1].arg, args[1].argw, SLJIT_IMM, 0);
            }
            return;
        }

        // Null check.
        if (label != nullptr) {
            label->jumpFrom(sljit_emit_cmp(compiler, isTestOrCastFail ? SLJIT_NOT_EQUAL : SLJIT_EQUAL, args[0].arg, args[0].argw, SLJIT_IMM, 0));
        } else if (!isTestOrCastFail) {
            context->appendTrapJump(ExecutionContext::CastFailureError,
                                    sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, args[0].arg, args[0].argw, SLJIT_IMM, 0));
        } else {
            sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_Z, args[0].arg, args[0].argw, SLJIT_IMM, 0);
            sljit_emit_op_flags(compiler, SLJIT_MOV, args[1].arg, args[1].argw, SLJIT_EQUAL);
        }
        return;
    }

    if (genericType == Value::I31Ref) {
        if ((srcInfo & JumpIfCastGeneric::IsNullable) == 0) {
            sljit_emit_op2u(compiler, SLJIT_AND | SLJIT_SET_Z, args[0].arg, args[0].argw, SLJIT_IMM, static_cast<sljit_sw>(Value::RefI31));

            if (label != nullptr) {
                label->jumpFrom(sljit_emit_jump(compiler, isTestOrCastFail ? SLJIT_ZERO : SLJIT_NOT_ZERO));
            } else if (!isTestOrCastFail) {
                context->appendTrapJump(ExecutionContext::CastFailureError, sljit_emit_jump(compiler, SLJIT_ZERO));
            } else {
                sljit_emit_op_flags(compiler, SLJIT_MOV, args[1].arg, args[1].argw, SLJIT_NOT_ZERO);
            }
            return;
        }

        sljit_emit_op2(compiler, SLJIT_ROTR, SLJIT_TMP_DEST_REG, 0, args[0].arg, args[0].argw, SLJIT_IMM, 1);

        if (label != nullptr) {
            label->jumpFrom(sljit_emit_cmp(compiler, isTestOrCastFail ? SLJIT_SIG_GREATER : SLJIT_SIG_LESS_EQUAL, SLJIT_TMP_DEST_REG, 0, SLJIT_IMM, 0));
        } else if (!isTestOrCastFail) {
            context->appendTrapJump(ExecutionContext::CastFailureError, sljit_emit_cmp(compiler, SLJIT_SIG_GREATER, SLJIT_TMP_DEST_REG, 0, SLJIT_IMM, 0));
        } else {
            sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_SIG_LESS_EQUAL, SLJIT_TMP_DEST_REG, 0, SLJIT_IMM, 0);
            sljit_emit_op_flags(compiler, SLJIT_MOV, args[1].arg, args[1].argw, SLJIT_SIG_LESS_EQUAL);
        }
        return;
    }

    sljit_s32 srcReg = GET_SOURCE_REG(args[0].arg, SLJIT_TMP_DEST_REG);
    MOVE_TO_REG(compiler, SLJIT_MOV, srcReg, args[0].arg, args[0].argw);
    sljit_sw kind = static_cast<sljit_sw>((genericType == Value::StructRef) ? Object::StructKind : Object::ArrayKind);

    COMPILE_ASSERT(Object::StructKind == 1 && Object::ArrayKind == 2, "Invalid GC kind constants");

    if ((srcInfo & (JumpIfCastGeneric::IsSrcNullable | JumpIfCastGeneric::IsSrcTagged)) == 0) {
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(srcReg), JITFieldAccessor::objectTypeInfo());
        if (label != nullptr) {
            sljit_s32 type = (genericType == Value::EqRef) ? SLJIT_LESS_EQUAL : SLJIT_EQUAL;
            if (isTestOrCastFail) {
                type ^= 0x1;
            }
            label->jumpFrom(sljit_emit_cmp(compiler, type, SLJIT_MEM1(SLJIT_TMP_DEST_REG), -static_cast<sljit_sw>(sizeof(sljit_up)), SLJIT_IMM, kind));
        } else if (!isTestOrCastFail) {
            sljit_s32 type = (genericType == Value::EqRef) ? SLJIT_GREATER : SLJIT_NOT_EQUAL;
            context->appendTrapJump(ExecutionContext::CastFailureError, sljit_emit_cmp(compiler, type, SLJIT_MEM1(SLJIT_TMP_DEST_REG), -static_cast<sljit_sw>(sizeof(sljit_up)), SLJIT_IMM, kind));
        } else {
            sljit_s32 type = (genericType == Value::EqRef) ? SLJIT_SET_LESS_EQUAL : SLJIT_SET_Z;
            sljit_emit_op2u(compiler, SLJIT_SUB | type, SLJIT_MEM1(SLJIT_TMP_DEST_REG), -static_cast<sljit_sw>(sizeof(sljit_up)), SLJIT_IMM, kind);
            type = (genericType == Value::EqRef) ? SLJIT_LESS_EQUAL : SLJIT_EQUAL;
            sljit_emit_op_flags(compiler, SLJIT_MOV, args[1].arg, args[1].argw, type);
        }
        return;
    }

    if (genericType == Value::EqRef) {
        if ((srcInfo & JumpIfCastGeneric::IsSrcTagged) != 0) {
            sljit_emit_op2(compiler, SLJIT_ROTR, SLJIT_TMP_DEST_REG, 0, srcReg, 0, SLJIT_IMM, 1);
            sljit_jump* jump = sljit_emit_cmp(compiler, SLJIT_SIG_LESS_EQUAL, SLJIT_TMP_DEST_REG, 0, SLJIT_IMM, 0);
            sljit_emit_op2(compiler, SLJIT_SHL, SLJIT_TMP_DEST_REG, 0, SLJIT_TMP_DEST_REG, 0, SLJIT_IMM, 1);

            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), JITFieldAccessor::objectTypeInfo());
            sljit_emit_op2(compiler, SLJIT_SUB, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), -static_cast<sljit_sw>(sizeof(sljit_up)), SLJIT_IMM, kind + 1);
            sljit_set_label(jump, sljit_emit_label(compiler));

            if (label != nullptr) {
                sljit_s32 type = ((srcInfo & JumpIfCastGeneric::IsNullable) == 0) ? SLJIT_SIG_LESS : SLJIT_SIG_LESS_EQUAL;
                if (isTestOrCastFail) {
                    type ^= 0x1;
                }
                label->jumpFrom(sljit_emit_cmp(compiler, type, SLJIT_TMP_DEST_REG, 0, SLJIT_IMM, 0));
            } else if (!isTestOrCastFail) {
                sljit_s32 type = ((srcInfo & JumpIfCastGeneric::IsNullable) == 0) ? SLJIT_SIG_GREATER_EQUAL : SLJIT_SIG_GREATER;
                context->appendTrapJump(ExecutionContext::CastFailureError, sljit_emit_cmp(compiler, type, SLJIT_TMP_DEST_REG, 0, SLJIT_IMM, 0));
            } else {
                sljit_s32 type = ((srcInfo & JumpIfCastGeneric::IsNullable) == 0) ? SLJIT_SET_SIG_LESS : SLJIT_SET_SIG_LESS_EQUAL;
                sljit_emit_op2u(compiler, SLJIT_SUB | type, SLJIT_TMP_DEST_REG, 0, SLJIT_IMM, 0);
                type = ((srcInfo & JumpIfCastGeneric::IsNullable) == 0) ? SLJIT_SIG_LESS : SLJIT_SIG_LESS_EQUAL;
                sljit_emit_op_flags(compiler, SLJIT_MOV, args[1].arg, args[1].argw, type);
            }
            return;
        }

        if (srcReg != SLJIT_TMP_DEST_REG && label == nullptr && isTestOrCastFail) {
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_TMP_DEST_REG, 0, srcReg, 0);
            srcReg = SLJIT_TMP_DEST_REG;
        }

        sljit_jump* jump = sljit_emit_cmp(compiler, SLJIT_EQUAL, srcReg, 0, SLJIT_IMM, 0);
        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(srcReg), JITFieldAccessor::objectTypeInfo());

        if (label != nullptr) {
            label->jumpFrom(sljit_emit_cmp(compiler, isTestOrCastFail ? SLJIT_GREATER : SLJIT_LESS_EQUAL, SLJIT_MEM1(SLJIT_TMP_DEST_REG), -static_cast<sljit_sw>(sizeof(sljit_up)), SLJIT_IMM, kind));
            if ((srcInfo & JumpIfCastGeneric::IsNullable) != 0) {
                isTestOrCastFail = !isTestOrCastFail;
            }

            if (isTestOrCastFail) {
                label->jumpFrom(jump);
            } else {
                sljit_set_label(jump, sljit_emit_label(compiler));
            }
        } else if (!isTestOrCastFail) {
            context->appendTrapJump(ExecutionContext::CastFailureError, sljit_emit_cmp(compiler, SLJIT_GREATER, SLJIT_MEM1(SLJIT_TMP_DEST_REG), -static_cast<sljit_sw>(sizeof(sljit_up)), SLJIT_IMM, kind));
            if ((srcInfo & JumpIfCastGeneric::IsNullable) != 0) {
                sljit_set_label(jump, sljit_emit_label(compiler));
            } else {
                context->appendTrapJump(ExecutionContext::CastFailureError, jump);
            }
        } else {
            ASSERT(srcReg == SLJIT_TMP_DEST_REG);
            kind++;
            if ((srcInfo & JumpIfCastGeneric::IsNullable) != 0) {
                sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), -static_cast<sljit_sw>(sizeof(sljit_up)));
            } else {
                sljit_emit_op2(compiler, SLJIT_SUB, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), -static_cast<sljit_sw>(sizeof(sljit_up)), SLJIT_IMM, kind);
                kind = 0;
            }
            sljit_set_label(jump, sljit_emit_label(compiler));
            sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_SIG_LESS, SLJIT_TMP_DEST_REG, 0, SLJIT_IMM, kind);
            sljit_emit_op_flags(compiler, SLJIT_MOV, args[1].arg, args[1].argw, SLJIT_SIG_LESS);
        }
        return;
    }

    if ((srcInfo & JumpIfCastGeneric::IsNullable) == 0) {
        sljit_jump* jump = nullptr;
        if (srcReg != SLJIT_TMP_DEST_REG && label == nullptr && isTestOrCastFail
            && (srcInfo & (JumpIfCastGeneric::IsSrcNullable | JumpIfCastGeneric::IsSrcTagged)) != (JumpIfCastGeneric::IsSrcNullable | JumpIfCastGeneric::IsSrcTagged)) {
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_TMP_DEST_REG, 0, srcReg, 0);
            srcReg = SLJIT_TMP_DEST_REG;
        }

        if ((srcInfo & JumpIfCastGeneric::IsSrcNullable) != 0) {
            if ((srcInfo & JumpIfCastGeneric::IsSrcTagged) != 0) {
                sljit_emit_op2(compiler, SLJIT_ROTR, SLJIT_TMP_DEST_REG, 0, srcReg, 0, SLJIT_IMM, 1);
                jump = sljit_emit_cmp(compiler, SLJIT_SIG_LESS_EQUAL, SLJIT_TMP_DEST_REG, 0, SLJIT_IMM, 0);
                sljit_emit_op2(compiler, SLJIT_SHL, SLJIT_TMP_DEST_REG, 0, SLJIT_TMP_DEST_REG, 0, SLJIT_IMM, 1);
                srcReg = SLJIT_TMP_DEST_REG;
            } else {
                jump = sljit_emit_cmp(compiler, SLJIT_EQUAL, srcReg, 0, SLJIT_IMM, 0);
            }
        } else {
            ASSERT((srcInfo & JumpIfCastGeneric::IsSrcTagged) != 0);
            sljit_emit_op2u(compiler, SLJIT_AND | SLJIT_SET_Z, srcReg, 0, SLJIT_IMM, static_cast<sljit_sw>(Value::RefI31));
            jump = sljit_emit_jump(compiler, SLJIT_NOT_ZERO);
        }

        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(srcReg), JITFieldAccessor::objectTypeInfo());

        if (label != nullptr) {
            label->jumpFrom(sljit_emit_cmp(compiler, isTestOrCastFail ? SLJIT_NOT_EQUAL : SLJIT_EQUAL, SLJIT_MEM1(SLJIT_TMP_DEST_REG), -static_cast<sljit_sw>(sizeof(sljit_up)), SLJIT_IMM, kind));
            if (isTestOrCastFail) {
                label->jumpFrom(jump);
            } else {
                sljit_set_label(jump, sljit_emit_label(compiler));
            }
        } else if (!isTestOrCastFail) {
            context->appendTrapJump(ExecutionContext::CastFailureError, jump);
            context->appendTrapJump(ExecutionContext::CastFailureError, sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_MEM1(SLJIT_TMP_DEST_REG), -static_cast<sljit_sw>(sizeof(sljit_up)), SLJIT_IMM, kind));
        } else {
            ASSERT(srcReg == SLJIT_TMP_DEST_REG);
            sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), -static_cast<sljit_sw>(sizeof(sljit_up)));
            sljit_set_label(jump, sljit_emit_label(compiler));
            sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_Z, SLJIT_TMP_DEST_REG, 0, SLJIT_IMM, kind);
            sljit_emit_op_flags(compiler, SLJIT_MOV, args[1].arg, args[1].argw, SLJIT_EQUAL);
        }
        return;
    }

    ASSERT((srcInfo & JumpIfCastGeneric::IsSrcNullable) != 0);

    if (label == nullptr && isTestOrCastFail) {
        sljit_jump* jump = nullptr;

        if ((srcInfo & JumpIfCastGeneric::IsSrcTagged) != 0) {
            sljit_emit_op2(compiler, SLJIT_ROTR, SLJIT_TMP_DEST_REG, 0, srcReg, 0, SLJIT_IMM, 1);
            jump = sljit_emit_cmp(compiler, SLJIT_SIG_LESS_EQUAL, SLJIT_TMP_DEST_REG, 0, SLJIT_IMM, 0);
            sljit_emit_op2(compiler, SLJIT_SHL, SLJIT_TMP_DEST_REG, 0, SLJIT_TMP_DEST_REG, 0, SLJIT_IMM, 1);
        } else {
            if (srcReg != SLJIT_TMP_DEST_REG) {
                sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_TMP_DEST_REG, 0, srcReg, 0);
            }
            jump = sljit_emit_cmp(compiler, SLJIT_EQUAL, srcReg, 0, SLJIT_IMM, 0);
        }

        sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), JITFieldAccessor::objectTypeInfo());
        sljit_set_label(jump, sljit_emit_label(compiler));

        if (genericType == Value::StructRef) {
            sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), -static_cast<sljit_sw>(sizeof(sljit_up)));
            sljit_set_label(jump, sljit_emit_label(compiler));
            sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_LESS_EQUAL, SLJIT_TMP_DEST_REG, 0, SLJIT_IMM, kind);
            sljit_emit_op_flags(compiler, SLJIT_MOV, args[1].arg, args[1].argw, SLJIT_LESS_EQUAL);
        } else {
            sljit_emit_op2(compiler, SLJIT_SUB, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(SLJIT_TMP_DEST_REG), -static_cast<sljit_sw>(sizeof(sljit_up)), SLJIT_IMM, kind);
            sljit_set_label(jump, sljit_emit_label(compiler));
            sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_Z, SLJIT_TMP_DEST_REG, 0, SLJIT_IMM, 0);
            sljit_emit_op_flags(compiler, SLJIT_MOV, args[1].arg, args[1].argw, SLJIT_EQUAL);
        }
        return;
    }

    sljit_jump* jumpIfZero = sljit_emit_cmp(compiler, SLJIT_EQUAL, srcReg, 0, SLJIT_IMM, 0);
    sljit_jump* jumpIfTagged = nullptr;

    if ((srcInfo & JumpIfCastGeneric::IsSrcTagged) != 0) {
        sljit_emit_op2u(compiler, SLJIT_AND | SLJIT_SET_Z, srcReg, 0, SLJIT_IMM, static_cast<sljit_sw>(Value::RefI31));
        jumpIfTagged = sljit_emit_jump(compiler, SLJIT_NOT_ZERO);
    }

    sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(srcReg), JITFieldAccessor::objectTypeInfo());

    if (label != nullptr) {
        label->jumpFrom(sljit_emit_cmp(compiler, isTestOrCastFail ? SLJIT_NOT_EQUAL : SLJIT_EQUAL, SLJIT_MEM1(SLJIT_TMP_DEST_REG), -static_cast<sljit_sw>(sizeof(sljit_up)), SLJIT_IMM, kind));
        if (isTestOrCastFail) {
            sljit_set_label(jumpIfZero, sljit_emit_label(compiler));
            label->jumpFrom(jumpIfTagged);
        } else {
            label->jumpFrom(jumpIfZero);
            sljit_set_label(jumpIfTagged, sljit_emit_label(compiler));
        }
    } else {
        if ((srcInfo & JumpIfCastGeneric::IsSrcTagged) != 0) {
            context->appendTrapJump(ExecutionContext::CastFailureError, jumpIfTagged);
        }
        context->appendTrapJump(ExecutionContext::CastFailureError, sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_MEM1(SLJIT_TMP_DEST_REG), -static_cast<sljit_sw>(sizeof(sljit_up)), SLJIT_IMM, kind));
        sljit_set_label(jumpIfZero, sljit_emit_label(compiler));
    }
}

static void emitGCCastDefined(sljit_compiler* compiler, Instruction* instr)
{
    CompileContext* context = CompileContext::get(compiler);
    const CompositeType** typeInfo = nullptr;
    uint8_t srcInfo = 0;
    Operand* operands = instr->operands();
    JITArg args[2];
    bool isTestOrCastFail = false;
    Label* label = nullptr;

    args[0].set(operands);

    switch (instr->opcode()) {
    case ByteCode::RefCastDefinedOpcode: {
        RefCastDefined* refCastDefined = reinterpret_cast<RefCastDefined*>(instr->byteCode());
        typeInfo = refCastDefined->typeInfo();
        srcInfo = refCastDefined->srcInfo();
        break;
    }
    case ByteCode::RefTestDefinedOpcode: {
        RefTestDefined* refTestDefined = reinterpret_cast<RefTestDefined*>(instr->byteCode());
        typeInfo = refTestDefined->typeInfo();
        srcInfo = refTestDefined->srcInfo();
        isTestOrCastFail = true;
        args[1].set(operands + 1);
        break;
    }
    case ByteCode::JumpIfCastDefinedOpcode: {
        JumpIfCastDefined* jumpIfCastDefined = reinterpret_cast<JumpIfCastDefined*>(instr->byteCode());
        typeInfo = jumpIfCastDefined->typeInfo();
        srcInfo = jumpIfCastDefined->srcInfo();
        label = instr->asExtended()->value().targetLabel;
        isTestOrCastFail = (srcInfo & JumpIfCastGeneric::IsCastFail) != 0;
        break;
    }
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }

    sljit_s32 tmpReg = instr->requiredReg(0);
    sljit_s32 srcReg;
    sljit_sw typeIndex = reinterpret_cast<sljit_sw>(typeInfo[0]);
    sljit_sw typeValue = reinterpret_cast<sljit_sw>(typeInfo[typeIndex]);

    if (label == nullptr && isTestOrCastFail) {
        srcReg = tmpReg;
    } else {
        srcReg = GET_SOURCE_REG(args[0].arg, tmpReg);
    }

    MOVE_TO_REG(compiler, SLJIT_MOV, srcReg, args[0].arg, args[0].argw);

    sljit_jump* jumpIfZero = nullptr;
    sljit_jump* jumpIfNotObject = nullptr;

    if ((srcInfo & JumpIfCastGeneric::IsNullable) != 0) {
        ASSERT((srcInfo & JumpIfCastGeneric::IsSrcNullable) != 0);
        jumpIfZero = sljit_emit_cmp(compiler, SLJIT_EQUAL, srcReg, 0, SLJIT_IMM, 0);
        if ((srcInfo & JumpIfCastGeneric::IsSrcTagged) != 0) {
            sljit_emit_op2u(compiler, SLJIT_AND | SLJIT_SET_Z, srcReg, 0, SLJIT_IMM, static_cast<sljit_sw>(Value::RefI31));
            jumpIfNotObject = sljit_emit_jump(compiler, SLJIT_NOT_ZERO);
        }
    } else if ((srcInfo & JumpIfCastGeneric::IsSrcTagged) != 0) {
        if ((srcInfo & JumpIfCastGeneric::IsSrcNullable) != 0) {
            sljit_emit_op2(compiler, SLJIT_ROTR, SLJIT_TMP_DEST_REG, 0, srcReg, 0, SLJIT_IMM, 1);
            jumpIfNotObject = sljit_emit_cmp(compiler, SLJIT_SIG_LESS_EQUAL, SLJIT_TMP_DEST_REG, 0, SLJIT_IMM, 0);
        } else {
            sljit_emit_op2u(compiler, SLJIT_AND | SLJIT_SET_Z, srcReg, 0, SLJIT_IMM, static_cast<sljit_sw>(Value::RefI31));
            jumpIfNotObject = sljit_emit_jump(compiler, SLJIT_NOT_ZERO);
        }
    } else if ((srcInfo & JumpIfCastGeneric::IsSrcNullable) != 0) {
        jumpIfNotObject = sljit_emit_cmp(compiler, SLJIT_EQUAL, srcReg, 0, SLJIT_IMM, 0);
    }

    sljit_emit_op1(compiler, SLJIT_MOV_P, tmpReg, 0, SLJIT_MEM1(srcReg), JITFieldAccessor::objectTypeInfo());
    sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(tmpReg), 0);
    sljit_emit_select(compiler, SLJIT_COMPARE_SELECT | SLJIT_LESS, SLJIT_TMP_DEST_REG, SLJIT_IMM, typeIndex, SLJIT_TMP_DEST_REG);
    sljit_emit_op1(compiler, SLJIT_MOV_P, tmpReg, 0, SLJIT_MEM2(tmpReg, SLJIT_TMP_DEST_REG), SLJIT_POINTER_SHIFT);

    if (label != nullptr) {
        label->jumpFrom(sljit_emit_cmp(compiler, isTestOrCastFail ? SLJIT_NOT_EQUAL : SLJIT_EQUAL, tmpReg, 0, SLJIT_IMM, typeValue));

        if ((srcInfo & JumpIfCastGeneric::IsNullable) != 0) {
            if (isTestOrCastFail) {
                sljit_set_label(jumpIfZero, sljit_emit_label(compiler));
            } else {
                label->jumpFrom(jumpIfZero);
            }
        }

        if (jumpIfNotObject != nullptr) {
            if (isTestOrCastFail) {
                label->jumpFrom(jumpIfNotObject);
            } else {
                sljit_set_label(jumpIfNotObject, sljit_emit_label(compiler));
            }
        }
    } else if (!isTestOrCastFail) {
        if (jumpIfNotObject != nullptr) {
            context->appendTrapJump(ExecutionContext::CastFailureError, jumpIfNotObject);
        }
        context->appendTrapJump(ExecutionContext::CastFailureError, sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, tmpReg, 0, SLJIT_IMM, typeValue));
        if ((srcInfo & JumpIfCastGeneric::IsNullable) != 0) {
            sljit_set_label(jumpIfZero, sljit_emit_label(compiler));
        }
    } else {
        if ((srcInfo & JumpIfCastGeneric::IsNullable) != 0) {
            sljit_emit_op2(compiler, SLJIT_SUB, tmpReg, 0, tmpReg, 0, SLJIT_IMM, typeValue);
            sljit_set_label(jumpIfZero, sljit_emit_label(compiler));
            typeValue = 0;
        }
        if (jumpIfNotObject != nullptr) {
            sljit_set_label(jumpIfNotObject, sljit_emit_label(compiler));
        }
        sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_Z, tmpReg, 0, SLJIT_IMM, typeValue);
        sljit_emit_op_flags(compiler, SLJIT_MOV, args[1].arg, args[1].argw, SLJIT_EQUAL);
    }
}

static void emitGCStore(sljit_compiler* compiler, ByteCodeStackOffset stackOffset, Operand* param, Value::Type type)
{
    Operand dst = VARIABLE_SET(STACK_OFFSET(stackOffset), Instruction::Offset);

    switch (VARIABLE_TYPE(*param)) {
    case Instruction::ConstPtr:
        ASSERT(!(VARIABLE_GET_IMM(*param)->info() & Instruction::kKeepInstruction));
        emitStoreImmediate(compiler, &dst, VARIABLE_GET_IMM(*param), false);
        break;
    case Instruction::Register:
        emitMove(compiler, Instruction::valueTypeToOperandType(type), param, &dst);
        break;
    }
}

enum GCCopyMode : int {
    GCCopyGetU,
    GCCopyGetS,
    GCCopySet,
};

static void emitGCDataCopy(sljit_compiler* compiler, Operand* operand, sljit_s32 baseReg, sljit_sw offset, Value::Type type, GCCopyMode mode)
{
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    if (type == Value::I64) {
        JITArgPair argPair(operand);

        if (SLJIT_IS_MEM(argPair.arg1)) {
            // Copy as double
            if (mode == GCCopySet) {
                sljit_emit_fop1(compiler, SLJIT_MOV_F64, SLJIT_MEM1(baseReg), offset, argPair.arg1, argPair.arg1w);
            } else {
                sljit_emit_fop1(compiler, SLJIT_MOV_F64, argPair.arg1, argPair.arg1w, SLJIT_MEM1(baseReg), offset);
            }
            return;
        }

        if (mode == GCCopySet) {
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(baseReg), offset + WORD_LOW_OFFSET, argPair.arg1, argPair.arg1w);
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(baseReg), offset + WORD_HIGH_OFFSET, argPair.arg2, argPair.arg2w);
        } else {
            sljit_emit_op1(compiler, SLJIT_MOV, argPair.arg1, argPair.arg1w, SLJIT_MEM1(baseReg), offset + WORD_LOW_OFFSET);
            sljit_emit_op1(compiler, SLJIT_MOV, argPair.arg2, argPair.arg2w, SLJIT_MEM1(baseReg), offset + WORD_HIGH_OFFSET);
        }
        return;
    }
#endif /* SLJIT_32BIT_ARCHITECTURE */

    sljit_s32 op;

    switch (type) {
    case Value::I8:
        op = (mode == GCCopyGetS) ? SLJIT_MOV32_S8 : SLJIT_MOV32_U8;
        break;
    case Value::I16:
        op = (mode == GCCopyGetS) ? SLJIT_MOV32_S16 : SLJIT_MOV32_U16;
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
        floatOperandToArg(compiler, operand, arg, SLJIT_TMP_DEST_FREG);
        op = (type == Value::F32) ? SLJIT_MOV_F32 : SLJIT_MOV_F64;

        if (mode == GCCopySet) {
            sljit_emit_fop1(compiler, op, SLJIT_MEM1(baseReg), offset, arg.arg, arg.argw);
        } else {
            sljit_emit_fop1(compiler, op, arg.arg, arg.argw, SLJIT_MEM1(baseReg), offset);
        }
        return;
    }
    case Value::V128: {
        JITArg arg;

        if (mode == GCCopySet) {
            simdOperandToArg(compiler, operand, arg, SLJIT_SIMD_REG_128, SLJIT_TMP_DEST_VREG);
            sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128, arg.arg, SLJIT_MEM1(baseReg), offset);
            return;
        }

        arg.set(operand);
        sljit_s32 reg = GET_TARGET_REG(arg.arg, SLJIT_TMP_DEST_VREG);

        sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | SLJIT_SIMD_REG_128, reg, SLJIT_MEM1(baseReg), offset);
        if (arg.arg != reg) {
            sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | SLJIT_SIMD_REG_128, reg, arg.arg, arg.argw);
        }
        return;
    }
    default:
        op = SLJIT_MOV;
        break;
    }

    JITArg arg(operand);
    sljit_s32 reg = GET_SOURCE_REG(arg.arg, SLJIT_TMP_DEST_REG);

    if (mode != GCCopySet) {
        sljit_emit_op1(compiler, op, reg, 0, SLJIT_MEM1(baseReg), offset);
        MOVE_FROM_REG(compiler, op == SLJIT_MOV ? SLJIT_MOV : SLJIT_MOV32, arg.arg, arg.argw, reg);
        return;
    }

#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
    if (baseReg != SLJIT_TMP_DEST_REG
        || (!SLJIT_IS_MEM(arg.arg) && (type != Value::I8 || !SLJIT_IS_REG(arg.arg) || sljit_get_register_index(SLJIT_GP_REGISTER, arg.arg) < 4))) {
        sljit_emit_op1(compiler, op, SLJIT_MEM1(baseReg), offset, arg.arg, arg.argw);
        return;
    }

    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R3, 0, SLJIT_R0, 0);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, arg.arg, arg.argw);
    sljit_emit_op1(compiler, op, SLJIT_MEM1(baseReg), offset, SLJIT_R0, 0);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_R3, 0);
#else /* !SLJIT_CONFIG_X86_32 */
#ifndef SLJIT_TMP_OPT_REG
#error "Missing implementation"
#endif
    if (!SLJIT_IS_MEM(arg.arg) || baseReg != SLJIT_TMP_DEST_REG) {
        sljit_emit_op1(compiler, op, SLJIT_MEM1(baseReg), offset, arg.arg, arg.argw);
        return;
    }

    sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_TMP_DEST_REG, 0, baseReg, 0, SLJIT_IMM, offset);
    sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_TMP_OPT_REG, 0, arg.arg, arg.argw);
    sljit_emit_op1(compiler, op, SLJIT_MEM1(SLJIT_TMP_DEST_REG), 0, SLJIT_TMP_OPT_REG, 0);
#endif /* SLJIT_CONFIG_X86_32 */
}

static void emitGCArrayNew(sljit_compiler* compiler, Instruction* instr)
{
    CompileContext* context = CompileContext::get(compiler);

    switch (instr->opcode()) {
    case ByteCode::ArrayNewOpcode: {
        ArrayNew* arrayNew = reinterpret_cast<ArrayNew*>(instr->byteCode());
        JITArg arg(instr->operands() + 1);
        emitGCStore(compiler, arrayNew->src0Offset(), instr->operands(), arrayNew->typeInfo()->field().type());
        MOVE_TO_REG(compiler, SLJIT_MOV32, SLJIT_R0, arg.arg, arg.argw);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_IMM, reinterpret_cast<sljit_sw>(arrayNew->typeInfo()));
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R2, 0, kFrameReg, 0, SLJIT_IMM, static_cast<sljit_sw>(arrayNew->src0Offset()));
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS3(P, P, P, 32), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, GCArray::arrayNew));
        context->appendTrapJump(ExecutionContext::AllocationError,
                                sljit_emit_cmp(compiler, SLJIT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, 0));
        arg.set(instr->operands() + 2);
        MOVE_FROM_REG(compiler, SLJIT_MOV, arg.arg, arg.argw, SLJIT_R0);
        break;
    }
    case ByteCode::ArrayNewDefaultOpcode: {
        ArrayNewDefault* arrayNewDefault = reinterpret_cast<ArrayNewDefault*>(instr->byteCode());
        JITArg arg(instr->operands());
        MOVE_TO_REG(compiler, SLJIT_MOV32, SLJIT_R0, arg.arg, arg.argw);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_IMM, reinterpret_cast<sljit_sw>(arrayNewDefault->typeInfo()));
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS2(P, P, 32), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, GCArray::arrayNewDefault));
        context->appendTrapJump(ExecutionContext::AllocationError,
                                sljit_emit_cmp(compiler, SLJIT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, 0));
        arg.set(instr->operands() + 1);
        MOVE_FROM_REG(compiler, SLJIT_MOV, arg.arg, arg.argw, SLJIT_R0);
        break;
    }
    case ByteCode::ArrayNewFixedOpcode: {
        ArrayNewFixed* arrayNewFixed = reinterpret_cast<ArrayNewFixed*>(instr->byteCode());
        ByteCodeStackOffset* stackOffset = arrayNewFixed->dataOffsets();
        ByteCodeStackOffset* end = stackOffset + arrayNewFixed->offsetsSize();
        Operand* param = instr->params();
        Value::Type type = arrayNewFixed->typeInfo()->field().type();

        while (stackOffset < end) {
            emitGCStore(compiler, *stackOffset++, param++, type);
        }

        sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, static_cast<sljit_sw>(arrayNewFixed->length()));
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_IMM, reinterpret_cast<sljit_sw>(arrayNewFixed->typeInfo()));
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM, reinterpret_cast<sljit_sw>(arrayNewFixed->dataOffsets()));
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R3, 0, kFrameReg, 0);
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(P, P, P, P, 32), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, GCArray::arrayNewFixed));
        context->appendTrapJump(ExecutionContext::AllocationError,
                                sljit_emit_cmp(compiler, SLJIT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, 0));
        JITArg dstArg(param);
        MOVE_FROM_REG(compiler, SLJIT_MOV, dstArg.arg, dstArg.argw, SLJIT_R0);
        break;
    }
    case ByteCode::ArrayNewDataOpcode:
    case ByteCode::ArrayNewElemOpcode: {
        ArrayNewFrom* arrayNewFrom = reinterpret_cast<ArrayNewFrom*>(instr->byteCode());
        bool isNewData = instr->opcode() == ByteCode::ArrayNewDataOpcode;
        size_t start;
        Operand* operands = instr->operands();
        JITArg args[2] = { operands, operands + 1 };
        emitInitR0R1(compiler, SLJIT_MOV32, SLJIT_MOV32, args);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM, reinterpret_cast<sljit_sw>(arrayNewFrom->typeInfo()));
        if (isNewData) {
            start = context->dataSegmentsStart + arrayNewFrom->index() * sizeof(DataSegment);
        } else {
            start = context->elementSegmentsStart + arrayNewFrom->index() * sizeof(ElementSegment);
        }
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R3, 0, kInstanceReg, 0, SLJIT_IMM, static_cast<sljit_sw>(start));
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS4(P, 32, 32, P, P), SLJIT_IMM,
                         isNewData ? GET_FUNC_ADDR(sljit_sw, GCArray::arrayNewData) : GET_FUNC_ADDR(sljit_sw, GCArray::arrayNewElem));
        context->appendTrapJump(ExecutionContext::AllocationError,
                                sljit_emit_cmp(compiler, SLJIT_LESS_EQUAL, SLJIT_R0, 0, SLJIT_IMM, static_cast<sljit_sw>(GCArray::OutOfBoundsAccess)));
        args[0].set(operands + 2);
        MOVE_FROM_REG(compiler, SLJIT_MOV, args[0].arg, args[0].argw, SLJIT_R0);
        break;
    }
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
}

static void emitGCArrayAccess(sljit_compiler* compiler, Instruction* instr)
{
    Value::Type type;
    bool isNullable;
    GCCopyMode mode = GCCopySet;

    if (instr->opcode() == ByteCode::ArrayGetOpcode) {
        ArrayGet* arrayGet = reinterpret_cast<ArrayGet*>(instr->byteCode());
        type = arrayGet->type();
        isNullable = arrayGet->isNullable();
        mode = arrayGet->isSigned() ? GCCopyGetS : GCCopyGetU;
    } else {
        ArraySet* arraySet = reinterpret_cast<ArraySet*>(instr->byteCode());
        type = arraySet->type();
        isNullable = arraySet->isNullable();
    }

    Operand* operands = instr->operands();
    JITArg arrayArg(operands);
    sljit_s32 tmpReg = instr->requiredReg(0);
    sljit_s32 arrayReg = GET_TARGET_REG(arrayArg.arg, tmpReg);
    MOVE_TO_REG(compiler, SLJIT_MOV, arrayReg, arrayArg.arg, arrayArg.argw);

    if (isNullable) {
        CompileContext::get(compiler)->appendTrapJump(ExecutionContext::NullArrayReferenceError,
                                                      sljit_emit_cmp(compiler, SLJIT_EQUAL, arrayReg, 0, SLJIT_IMM, 0));
    }

    JITArg indexArg(operands + 1);
    sljit_emit_op1(compiler, SLJIT_MOV_U32, SLJIT_TMP_DEST_REG, 0, indexArg.arg, indexArg.argw);

#if (defined SLJIT_CONFIG_X86 && SLJIT_CONFIG_X86)
    CompileContext::get(compiler)->appendTrapJump(ExecutionContext::ArrayOutOfBounds,
                                                  sljit_emit_cmp(compiler, SLJIT_LESS_EQUAL | SLJIT_32, SLJIT_MEM1(arrayReg), JITFieldAccessor::arrayLength(), SLJIT_TMP_DEST_REG, 0));
#else /* !SLJIT_CONFIG_X86 */
#ifndef SLJIT_TMP_OPT_REG
#error "Missing implementation"
#endif
    sljit_emit_op1(compiler, SLJIT_MOV_U32, SLJIT_TMP_OPT_REG, 0, SLJIT_MEM1(arrayReg), JITFieldAccessor::arrayLength());
    CompileContext::get(compiler)->appendTrapJump(ExecutionContext::ArrayOutOfBounds,
                                                  sljit_emit_cmp(compiler, SLJIT_LESS_EQUAL, SLJIT_TMP_OPT_REG, 0, SLJIT_TMP_DEST_REG, 0));
#endif /* SLJIT_CONFIG_X86 */

    sljit_sw log2Size = static_cast<sljit_sw>(GCArray::getLog2Size(type));

    sljit_emit_op2_shift(compiler, SLJIT_ADD | SLJIT_SHL_IMM, tmpReg, 0, arrayReg, 0, SLJIT_TMP_DEST_REG, 0, log2Size);

    sljit_sw mask = (static_cast<sljit_sw>(1) << log2Size) - 1;
    sljit_sw startOffset = (static_cast<sljit_sw>(sizeof(GCArray)) + mask) & ~mask;
    emitGCDataCopy(compiler, operands + 2, tmpReg, startOffset, type, mode);
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
        emitGCStore(compiler, *stackOffset++, param++, it.type());
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
    sljit_sw memberOffset;
    Value::Type type;
    bool isNullable;
    GCCopyMode mode = GCCopySet;

    if (instr->opcode() == ByteCode::StructGetOpcode) {
        StructGet* structGet = reinterpret_cast<StructGet*>(instr->byteCode());
        memberOffset = static_cast<sljit_sw>(structGet->memberOffset());
        type = structGet->type();
        isNullable = structGet->isNullable();
        mode = structGet->isSigned() ? GCCopyGetS : GCCopyGetU;
    } else {
        StructSet* structSet = reinterpret_cast<StructSet*>(instr->byteCode());
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

    emitGCDataCopy(compiler, instr->operands() + 1, structReg, memberOffset, type, mode);
}
