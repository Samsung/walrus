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

static void emitImmediate(sljit_compiler* compiler, Instruction* instr)
{
    Operand* result = instr->operands();

    if (result->location.type == Operand::Unused) {
        return;
    }

    JITArg dst;
    operandToArg(result, dst);

    sljit_s32 opcode = SLJIT_MOV;

    if ((result->location.valueInfo & LocationInfo::kSizeMask) == 1) {
        opcode = SLJIT_MOV32;
    }

    sljit_sw imm = static_cast<sljit_sw>(instr->value().value64);
    sljit_emit_op1(compiler, opcode, dst.arg, dst.argw, SLJIT_IMM, imm);
}

static void emitLocalMove(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operand = instr->operands();

    if (operand->location.type == Operand::Unused) {
        assert(!(instr->info() & Instruction::kKeepInstruction));
        return;
    }

    assert(instr->info() & Instruction::kKeepInstruction);

    JITArg src, dst;

    if (instr->opcode() == LocalGetOpcode) {
        operandToArg(operand, dst);
        src.arg = SLJIT_MEM1(kFrameReg);
        src.argw = static_cast<sljit_sw>(instr->value().value);
    } else {
        dst.arg = SLJIT_MEM1(kFrameReg);
        dst.argw = static_cast<sljit_sw>(instr->value().value);
        operandToArg(operand, src);
    }

    sljit_s32 opcode = SLJIT_MOV;

    if ((operand->location.valueInfo & LocationInfo::kSizeMask) == 1) {
        opcode = SLJIT_MOV32;
    }

    sljit_emit_op1(compiler, opcode, dst.arg, dst.argw, src.arg, src.argw);
}

enum DivRemOptions : sljit_s32 {
    DivRem32 = 1 << 1,
    DivRemSigned = 1 << 0,
    DivRemRemainder = 2 << 1,
};

static void emitDivRem(sljit_compiler* compiler, sljit_s32 opcode, JITArg* args, sljit_s32 options)
{
    CompileContext* context = CompileContext::get(compiler);
    sljit_s32 movOpcode = (options & DivRem32) ? SLJIT_MOV32 : SLJIT_MOV;

    if (args[1].arg & SLJIT_IMM) {
        if (args[1].argw == 0) {
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM, ExecutionContext::DivideByZeroError);
            sljit_set_label(sljit_emit_jump(compiler, SLJIT_JUMP), context->trapLabel);
            return;
        } else if (args[1].argw == -1 && opcode == SLJIT_DIVMOD_SW) {
            sljit_emit_op1(compiler, movOpcode, args[2].arg, args[2].argw, SLJIT_IMM, 0);
            return;
        }
    }

    MOVE_TO_REG(compiler, movOpcode, SLJIT_R1, args[1].arg, args[1].argw);
    MOVE_TO_REG(compiler, movOpcode, SLJIT_R0, args[0].arg, args[0].argw);

    sljit_jump* moduloJumpFrom = nullptr;

    if (args[1].arg & SLJIT_IMM) {
        if ((options & DivRemSigned) && args[1].argw == -1) {
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM, ExecutionContext::IntegerOverflowError);

            sljit_s32 type = SLJIT_EQUAL;
            sljit_sw min = static_cast<sljit_sw>(INT64_MIN);

            if (options & DivRem32) {
                type |= SLJIT_32;
                min = static_cast<sljit_sw>(INT32_MIN);
            }

            sljit_jump* cmp = sljit_emit_cmp(compiler, type, SLJIT_R1, 0, SLJIT_IMM, min);
            sljit_set_label(cmp, context->trapLabel);
        }
    } else if (options & DivRemSigned) {
        sljit_s32 addOpcode = (options & DivRem32) ? SLJIT_ADD32 : SLJIT_ADD;
        sljit_s32 subOpcode = (options & DivRem32) ? SLJIT_SUB32 : SLJIT_SUB;

        sljit_emit_op2(compiler, addOpcode, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, 1);
        sljit_emit_op2u(compiler, subOpcode | SLJIT_SET_LESS_EQUAL | SLJIT_SET_Z, SLJIT_R1, 0, SLJIT_IMM, 1);

        moduloJumpFrom = sljit_emit_jump(compiler, SLJIT_LESS_EQUAL);

        if (!(options & DivRemRemainder)) {
            sljit_label* resumeLabel = sljit_emit_label(compiler);
            SlowCase::Type type = SlowCase::Type::SignedDivide;

            if (options & DivRem32) {
                type = SlowCase::Type::SignedDivide32;
            }

            context->add(new SlowCase(type, moduloJumpFrom, resumeLabel, nullptr));
            moduloJumpFrom = nullptr;
        }

        sljit_emit_op2(compiler, subOpcode, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, 1);
    } else {
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM, ExecutionContext::DivideByZeroError);

        sljit_s32 type = SLJIT_EQUAL;
        if (options & DivRem32) {
            type |= SLJIT_32;
        }

        sljit_jump* cmp = sljit_emit_cmp(compiler, type, SLJIT_R1, 0, SLJIT_IMM, 0);
        sljit_set_label(cmp, context->trapLabel);
    }

    sljit_emit_op0(compiler, opcode);

    if (moduloJumpFrom != nullptr) {
        sljit_label* resumeLabel = sljit_emit_label(compiler);
        SlowCase::Type type = SlowCase::Type::SignedModulo;

        if (options & DivRem32) {
            type = SlowCase::Type::SignedModulo32;
        }

        context->add(new SlowCase(type, moduloJumpFrom, resumeLabel, nullptr));
    }

    sljit_s32 resultReg = (options & DivRemRemainder) ? SLJIT_R1 : SLJIT_R0;
    MOVE_FROM_REG(compiler, movOpcode, args[2].arg, args[2].argw, resultReg);
}

static void emitBinary(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[3];

    for (int i = 0; i < 3; ++i) {
        operandToArg(operands + i, args[i]);
    }

    sljit_s32 opcode;

    switch (instr->opcode()) {
    case I32AddOpcode:
        opcode = SLJIT_ADD32;
        break;
    case I32SubOpcode:
        opcode = SLJIT_SUB32;
        break;
    case I32MulOpcode:
        opcode = SLJIT_MUL32;
        break;
    case I32DivSOpcode:
        emitDivRem(compiler, SLJIT_DIV_S32, args, DivRem32 | DivRemSigned);
        return;
    case I32DivUOpcode:
        emitDivRem(compiler, SLJIT_DIV_U32, args, DivRem32);
        return;
    case I32RemSOpcode:
        emitDivRem(compiler, SLJIT_DIVMOD_S32, args, DivRem32 | DivRemSigned | DivRemRemainder);
        return;
    case I32RemUOpcode:
        emitDivRem(compiler, SLJIT_DIVMOD_U32, args, DivRem32 | DivRemRemainder);
        return;
    case I32RotlOpcode:
        opcode = SLJIT_ROTL32;
        break;
    case I32RotrOpcode:
        opcode = SLJIT_ROTR32;
        break;
    case I32AndOpcode:
        opcode = SLJIT_AND32;
        break;
    case I32OrOpcode:
        opcode = SLJIT_OR32;
        break;
    case I32XorOpcode:
        opcode = SLJIT_XOR32;
        break;
    case I32ShlOpcode:
        opcode = SLJIT_SHL32;
        break;
    case I32ShrSOpcode:
        opcode = SLJIT_ASHR32;
        break;
    case I32ShrUOpcode:
        opcode = SLJIT_LSHR32;
        break;
    case I64AddOpcode:
        opcode = SLJIT_ADD;
        break;
    case I64SubOpcode:
        opcode = SLJIT_SUB;
        break;
    case I64MulOpcode:
        opcode = SLJIT_MUL;
        break;
    case I64DivSOpcode:
        emitDivRem(compiler, SLJIT_DIV_SW, args, DivRemSigned);
        return;
    case I64DivUOpcode:
        emitDivRem(compiler, SLJIT_DIV_UW, args, 0);
        return;
    case I64RemSOpcode:
        emitDivRem(compiler, SLJIT_DIVMOD_SW, args, DivRemSigned | DivRemRemainder);
        return;
    case I64RemUOpcode:
        emitDivRem(compiler, SLJIT_DIVMOD_UW, args, DivRemRemainder);
        return;
    case I64RotlOpcode:
        opcode = SLJIT_ROTL;
        break;
    case I64RotrOpcode:
        opcode = SLJIT_ROTR;
        break;
    case I64AndOpcode:
        opcode = SLJIT_AND;
        break;
    case I64OrOpcode:
        opcode = SLJIT_OR;
        break;
    case I64XorOpcode:
        opcode = SLJIT_XOR;
        break;
    case I64ShlOpcode:
        opcode = SLJIT_SHL;
        break;
    case I64ShrSOpcode:
        opcode = SLJIT_ASHR;
        break;
    case I64ShrUOpcode:
        opcode = SLJIT_LSHR;
        break;
    default:
        WABT_UNREACHABLE;
        break;
    }

    sljit_emit_op2(compiler, opcode, args[2].arg, args[2].argw, args[0].arg, args[0].argw, args[1].arg, args[1].argw);
}

static sljit_s32 popcnt32(sljit_s32 arg)
{
    return __builtin_popcount(arg);
}

static sljit_sw popcnt64(sljit_sw arg)
{
    return __builtin_popcountl(arg);
}

static void emitPopcnt(sljit_compiler* compiler, OpcodeKind opcode, JITArg* args)
{
    sljit_s32 movOpcode = (opcode == I32PopcntOpcode) ? SLJIT_MOV32 : SLJIT_MOV;

    MOVE_TO_REG(compiler, movOpcode, SLJIT_R0, args[0].arg, args[0].argw);

    sljit_sw funcAddr = (opcode == I32PopcntOpcode) ? GET_FUNC_ADDR(sljit_sw, popcnt32) : GET_FUNC_ADDR(sljit_sw, popcnt64);
    sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS1(W, W), SLJIT_IMM, funcAddr);

    MOVE_FROM_REG(compiler, movOpcode, args[1].arg, args[1].argw, SLJIT_R0);
}

static void emitExtend(sljit_compiler* compiler, sljit_s32 opcode, sljit_s32 bigEndianIncrease, JITArg* args)
{
    sljit_s32 reg = GET_TARGET_REG(args[1].arg, SLJIT_R0);

    assert((args[0].arg >> 8) == 0);
#if (defined SLJIT_BIG_ENDIAN && SLJIT_BIG_ENDIAN)
    if (args[0].arg & SLJIT_MEM) {
        args[0].argw += bigEndianIncrease;
    }
#endif /* SLJIT_BIG_ENDIAN */

    sljit_emit_op1(compiler, opcode, reg, 0, args[0].arg, args[0].argw);

    sljit_s32 movOpcode = (bigEndianIncrease < 4) ? SLJIT_MOV32 : SLJIT_MOV;
    MOVE_FROM_REG(compiler, movOpcode, args[1].arg, args[1].argw, reg);
}

static void emitUnary(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[2];

    for (int i = 0; i < 2; ++i) {
        operandToArg(operands + i, args[i]);
    }

    sljit_s32 opcode;

    switch (instr->opcode()) {
    case I32ClzOpcode:
        opcode = SLJIT_CLZ32;
        break;
    case I32CtzOpcode:
        opcode = SLJIT_CTZ32;
        break;
    case I64ClzOpcode:
        opcode = SLJIT_CLZ;
        break;
    case I64CtzOpcode:
        opcode = SLJIT_CTZ;
        break;
    case I32PopcntOpcode:
    case I64PopcntOpcode:
        emitPopcnt(compiler, instr->opcode(), args);
        return;
    case I32Extend8SOpcode:
        emitExtend(compiler, SLJIT_MOV32_S8, 3, args);
        return;
    case I32Extend16SOpcode:
        emitExtend(compiler, SLJIT_MOV32_S16, 2, args);
        return;
    case I64Extend8SOpcode:
        emitExtend(compiler, SLJIT_MOV_S8, 7, args);
        return;
    case I64Extend16SOpcode:
        emitExtend(compiler, SLJIT_MOV_S16, 6, args);
        return;
    case I64Extend32SOpcode:
        emitExtend(compiler, SLJIT_MOV_S32, 4, args);
        return;
    default:
        WABT_UNREACHABLE;
        break;
    }

    // If the operand is an immediate then it is necesarry to move it into a
    // register because immediate source arguments are not supported.
    if (args[0].arg & SLJIT_IMM) {
        sljit_s32 mov = SLJIT_MOV;

        if ((operands->location.valueInfo & LocationInfo::kSizeMask) == 1) {
            mov = SLJIT_MOV32;
        }

        sljit_emit_op1(compiler, mov, SLJIT_R0, 0, args[0].arg, args[0].argw);
        args[0].arg = SLJIT_R0;
        args[0].argw = 0;
    }

    sljit_emit_op1(compiler, opcode, args[1].arg, args[1].argw, args[0].arg, args[0].argw);
}

static void emitSelect(sljit_compiler* compiler, Instruction* instr, sljit_s32 type)
{
    Operand* operands = instr->operands();
    assert(instr->opcode() == SelectOpcode && instr->paramCount() == 3);

    if (operands->location.valueInfo & LocationInfo::kFloat) {
        return emitFloatSelect(compiler, instr, type);
    }

    bool is32 = (operands->location.valueInfo & LocationInfo::kSizeMask) == 1;
    sljit_s32 movOpcode = is32 ? SLJIT_MOV32 : SLJIT_MOV;
    JITArg args[3];

    operandToArg(operands + 0, args[0]);
    operandToArg(operands + 1, args[1]);
    operandToArg(operands + 3, args[2]);

    if (type == -1) {
        JITArg cond;

        operandToArg(operands + 2, cond);
        sljit_emit_op2u(compiler, SLJIT_SUB32 | SLJIT_SET_Z, cond.arg, cond.argw, SLJIT_IMM, 0);

        type = SLJIT_NOT_ZERO;
    }

    sljit_s32 targetReg = GET_TARGET_REG(args[2].arg, SLJIT_R0);

    if (is32) {
        type |= SLJIT_32;
    }

    if (!IS_SOURCE_REG(args[1].arg)) {
        sljit_emit_op1(compiler, movOpcode, targetReg, 0, args[1].arg, args[1].argw);
        args[1].arg = targetReg;
    }

    sljit_emit_select(compiler, type, targetReg, args[0].arg, args[0].argw, args[1].arg);
    MOVE_FROM_REG(compiler, movOpcode, args[2].arg, args[2].argw, targetReg);
    return;
}

static bool emitCompare(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operand = instr->operands();
    sljit_s32 opcode, type;
    JITArg params[2];

    for (Index i = 0; i < instr->paramCount(); ++i) {
        operandToArg(operand, params[i]);
        operand++;
    }

    switch (instr->opcode()) {
    case I32EqzOpcode:
    case I64EqzOpcode:
        opcode = SLJIT_SUB | SLJIT_SET_Z;
        type = SLJIT_EQUAL;
        params[1].arg = SLJIT_IMM;
        params[1].argw = 0;
        break;
    case I32EqOpcode:
    case I64EqOpcode:
        opcode = SLJIT_SUB | SLJIT_SET_Z;
        type = SLJIT_EQUAL;
        break;
    case I32NeOpcode:
    case I64NeOpcode:
        opcode = SLJIT_SUB | SLJIT_SET_Z;
        type = SLJIT_NOT_EQUAL;
        break;
    case I32LtSOpcode:
    case I64LtSOpcode:
        opcode = SLJIT_SUB | SLJIT_SET_SIG_LESS;
        type = SLJIT_SIG_LESS;
        break;
    case I32LtUOpcode:
    case I64LtUOpcode:
        opcode = SLJIT_SUB | SLJIT_SET_LESS;
        type = SLJIT_LESS;
        break;
    case I32GtSOpcode:
    case I64GtSOpcode:
        opcode = SLJIT_SUB | SLJIT_SET_SIG_GREATER;
        type = SLJIT_SIG_GREATER;
        break;
    case I32GtUOpcode:
    case I64GtUOpcode:
        opcode = SLJIT_SUB | SLJIT_SET_GREATER;
        type = SLJIT_GREATER;
        break;
    case I32LeSOpcode:
    case I64LeSOpcode:
        opcode = SLJIT_SUB | SLJIT_SET_SIG_LESS_EQUAL;
        type = SLJIT_SIG_LESS_EQUAL;
        break;
    case I32LeUOpcode:
    case I64LeUOpcode:
        opcode = SLJIT_SUB | SLJIT_SET_LESS_EQUAL;
        type = SLJIT_LESS_EQUAL;
        break;
    case I32GeSOpcode:
    case I64GeSOpcode:
        opcode = SLJIT_SUB | SLJIT_SET_SIG_GREATER_EQUAL;
        type = SLJIT_SIG_GREATER_EQUAL;
        break;
    case I32GeUOpcode:
    case I64GeUOpcode:
        opcode = SLJIT_SUB | SLJIT_SET_GREATER_EQUAL;
        type = SLJIT_GREATER_EQUAL;
        break;
    default:
        WABT_UNREACHABLE;
        break;
    }

    if (operand->location.type != Operand::Unused || instr->next()->asInstruction()->opcode() == SelectOpcode) {
        if ((operand[-1].location.valueInfo & LocationInfo::kSizeMask) == 1) {
            opcode |= SLJIT_32;
        }

        sljit_emit_op2u(compiler, opcode, params[0].arg, params[0].argw, params[1].arg, params[1].argw);
    }

    if (operand->location.type != Operand::Unused) {
        operandToArg(operand, params[0]);
        sljit_emit_op_flags(compiler, SLJIT_MOV32, params[0].arg, params[0].argw, type);
        return false;
    }

    Instruction* nextInstr = instr->next()->asInstruction();

    if (nextInstr->opcode() == SelectOpcode) {
        emitSelect(compiler, nextInstr, type);
        return true;
    }

    assert(nextInstr->opcode() == BrIfOpcode || nextInstr->opcode() == InterpBrUnlessOpcode);

    if (nextInstr->opcode() == InterpBrUnlessOpcode) {
        type ^= 0x1;
    }

    if ((operand[-1].location.valueInfo & LocationInfo::kSizeMask) == 1) {
        type |= SLJIT_32;
    }

    sljit_jump* jump = sljit_emit_cmp(compiler, type, params[0].arg, params[0].argw, params[1].arg, params[1].argw);
    nextInstr->value().targetLabel->jumpFrom(jump);
    return true;
}

static void emitConvert(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[2];

    for (int i = 0; i < 2; ++i) {
        operandToArg(operands + i, args[i]);
    }

    switch (instr->opcode()) {
    case I32WrapI64Opcode:
        if (args[0].arg & SLJIT_MEM) {
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, args[0].arg, args[0].argw);
            sljit_emit_op1(compiler, SLJIT_MOV32, args[1].arg, args[1].argw, SLJIT_R0, 0);
        } else {
            sljit_emit_op1(compiler, SLJIT_MOV32, args[1].arg, args[1].argw, args[0].arg, args[0].argw);
        }
        return;
    case I64ExtendI32SOpcode:
        if (!(args[0].arg & SLJIT_MEM)) {
            sljit_emit_op1(compiler, SLJIT_MOV_S32, args[1].arg, args[1].argw, args[0].arg, args[0].argw);
        } else {
            sljit_emit_op1(compiler, SLJIT_MOV_S32, SLJIT_R0, 0, args[0].arg, args[0].argw);
            sljit_emit_op1(compiler, SLJIT_MOV, args[1].arg, args[1].argw, SLJIT_R0, 0);
        }
        return;
    case I64ExtendI32UOpcode:
        if (!(args[0].arg & SLJIT_MEM)) {
            sljit_emit_op1(compiler, SLJIT_MOV_U32, args[1].arg, args[1].argw, args[0].arg, args[0].argw);
        } else {
            sljit_emit_op1(compiler, SLJIT_MOV_U32, SLJIT_R0, 0, args[0].arg, args[0].argw);
            sljit_emit_op1(compiler, SLJIT_MOV, args[1].arg, args[1].argw, SLJIT_R0, 0);
        }
        return;
    default:
        WABT_UNREACHABLE;
        break;
    }
}
