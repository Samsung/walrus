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
