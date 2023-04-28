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

struct JITArgPair {
    sljit_s32 arg1;
    sljit_sw arg1w;
    sljit_s32 arg2;
    sljit_sw arg2w;
};

#if (defined SLJIT_BIG_ENDIAN && SLJIT_BIG_ENDIAN)
#define WORD_LOW_OFFSET (sizeof(sljit_s32))
#define WORD_HIGH_OFFSET 0
#else /* !SLJIT_BIG_ENDIAN */
#define WORD_LOW_OFFSET 0
#define WORD_HIGH_OFFSET (sizeof(sljit_s32))
#endif /* !SLJIT_BIG_ENDIAN */

static void operandToArgPair(Operand* operand, JITArgPair& arg)
{
    if (operand->item == nullptr || operand->item->group() != Instruction::Immediate) {
        arg.arg1 = SLJIT_MEM1(kFrameReg);
        arg.arg1w = static_cast<sljit_sw>((operand->offset << 2) + WORD_LOW_OFFSET);
        arg.arg2 = SLJIT_MEM1(kFrameReg);
        arg.arg2w = static_cast<sljit_sw>((operand->offset << 2) + WORD_HIGH_OFFSET);
        return;
    }

    arg.arg1 = SLJIT_IMM;
    arg.arg2 = SLJIT_IMM;
    ASSERT(operand->item->group() == Instruction::Immediate);

    Instruction* instr = operand->item->asInstruction();

    uint64_t value64 = reinterpret_cast<Const64*>(instr->byteCode())->value();

    arg.arg1w = static_cast<sljit_sw>(value64);
    arg.arg2w = static_cast<sljit_sw>(value64 >> 32);
}

static void emitStoreImmediate(sljit_compiler* compiler, Operand* result, Instruction* instr)
{
    sljit_sw offset = static_cast<sljit_sw>(result->offset << 2);

    if (instr->opcode() == Const32Opcode) {
        uint32_t value32 = reinterpret_cast<Const32*>(instr->byteCode())->value();
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(kFrameReg), offset, SLJIT_IMM, static_cast<sljit_sw>(value32));
        return;
    }

    ASSERT(instr->opcode() == Const64Opcode);

    uint64_t value64 = reinterpret_cast<Const64*>(instr->byteCode())->value();

    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(kFrameReg), offset + WORD_LOW_OFFSET, SLJIT_IMM, static_cast<sljit_sw>(value64));
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(kFrameReg), offset + WORD_HIGH_OFFSET, SLJIT_IMM, static_cast<sljit_sw>(value64 >> 32));
}

static void emitDivRem32(sljit_compiler* compiler, sljit_s32 opcode, JITArg* args)
{
    CompileContext* context = CompileContext::get(compiler);

    if (args[1].arg & SLJIT_IMM) {
        if (args[1].argw == 0) {
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM, ExecutionContext::DivideByZeroError);
            sljit_set_label(sljit_emit_jump(compiler, SLJIT_JUMP), context->trapLabel);
            return;
        } else if (args[1].argw == -1 && opcode == SLJIT_DIVMOD_SW) {
            sljit_emit_op1(compiler, SLJIT_MOV, args[2].arg, args[2].argw, SLJIT_IMM, 0);
            return;
        }
    }

    MOVE_TO_REG(compiler, SLJIT_MOV, SLJIT_R1, args[1].arg, args[1].argw);
    MOVE_TO_REG(compiler, SLJIT_MOV, SLJIT_R0, args[0].arg, args[0].argw);

    sljit_jump* moduloJumpFrom = nullptr;

    if (args[1].arg & SLJIT_IMM) {
        if (opcode == SLJIT_DIV_SW && args[1].argw == -1) {
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM, ExecutionContext::IntegerOverflowError);

            sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_EQUAL, SLJIT_R1, 0, SLJIT_IMM, static_cast<sljit_sw>(INT32_MIN));
            sljit_set_label(cmp, context->trapLabel);
        }
    } else if (opcode == SLJIT_DIV_SW || opcode == SLJIT_DIVMOD_SW) {
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, 1);
        sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_LESS_EQUAL | SLJIT_SET_Z, SLJIT_R1, 0, SLJIT_IMM, 1);

        moduloJumpFrom = sljit_emit_jump(compiler, SLJIT_LESS_EQUAL);

        if (opcode == SLJIT_DIV_SW) {
            sljit_label* resumeLabel = sljit_emit_label(compiler);
            context->add(new SlowCase(SlowCase::Type::SignedDivide, moduloJumpFrom, resumeLabel, nullptr));
            moduloJumpFrom = nullptr;
        }

        sljit_emit_op2(compiler, SLJIT_SUB, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, 1);
    } else {
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM, ExecutionContext::DivideByZeroError);

        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_EQUAL, SLJIT_R1, 0, SLJIT_IMM, 0);
        sljit_set_label(cmp, context->trapLabel);
    }

    sljit_emit_op0(compiler, opcode);

    sljit_s32 resultReg = SLJIT_R0;

    if (opcode == SLJIT_DIVMOD_SW || opcode == SLJIT_DIVMOD_UW) {
        resultReg = SLJIT_R1;
    }

    if (moduloJumpFrom != nullptr) {
        sljit_label* resumeLabel = sljit_emit_label(compiler);
        context->add(new SlowCase(SlowCase::Type::SignedModulo, moduloJumpFrom, resumeLabel, nullptr));
    }

    MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg, args[2].argw, resultReg);
}

static void emitSimpleBinary64(sljit_compiler* compiler, sljit_s32 op1, sljit_s32 op2, JITArgPair* args)
{
#if !(defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
    if (arg0arg1 & arg1arg1 & SLJIT_MEM) {
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, args[0].arg1, args[0].arg1w);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, args[1].arg1, args[1].arg1w);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, args[0].arg2, args[0].arg2w);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R3, 0, args[1].arg2, args[1].arg2w);

        sljit_emit_op2(compiler, op1, args[2].arg1, args[2].arg1w, SLJIT_R0, 0, SLJIT_R1, 0);
        sljit_emit_op2(compiler, op2, args[2].arg2, args[2].arg2w, SLJIT_R2, 0, SLJIT_R3, 0);
        return;
    }
#endif /* !SLJIT_CONFIG_X86_32 */

    if (args[1].arg1 & SLJIT_MEM) {
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, args[1].arg1, args[1].arg1w);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, args[1].arg2, args[1].arg2w);

        sljit_emit_op2(compiler, op1, args[2].arg1, args[2].arg1w, args[0].arg1, args[0].arg1w, SLJIT_R0, 0);
        sljit_emit_op2(compiler, op2, args[2].arg2, args[2].arg2w, args[0].arg2, args[0].arg2w, SLJIT_R1, 0);
        return;
    }

    if (args[0].arg1 & SLJIT_MEM) {
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, args[0].arg1, args[0].arg1w);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, args[0].arg2, args[0].arg2w);

        sljit_emit_op2(compiler, op1, args[2].arg1, args[2].arg1w, SLJIT_R0, 0, args[1].arg1, args[1].arg1w);
        sljit_emit_op2(compiler, op2, args[2].arg2, args[2].arg2w, SLJIT_R1, 0, args[1].arg2, args[1].arg2w);
        return;
    }

    sljit_emit_op2(compiler, op1, args[2].arg1, args[2].arg1w, args[0].arg1, args[0].arg1w, args[1].arg1, args[1].arg1w);
    sljit_emit_op2(compiler, op2, args[2].arg2, args[2].arg2w, args[0].arg2, args[0].arg2w, args[1].arg2, args[1].arg2w);
}

static void emitShift64(sljit_compiler* compiler, sljit_s32 op, JITArgPair* args)
{
    sljit_s32 shiftIntoResultReg, otherResultReg;
    sljit_s32 shiftIntoArg, otherArg, shiftArg;
    sljit_s32 shiftIntoResultArg, otherResultArg;
    sljit_sw shiftIntoArgw, otherArgw;
    sljit_sw shiftIntoResultArgw, otherResultArgw;

    if (op == SLJIT_SHL) {
        shiftIntoResultReg = GET_TARGET_REG(args[2].arg2, SLJIT_R0);
        otherResultReg = GET_TARGET_REG(args[2].arg1, SLJIT_R1);

        shiftIntoArg = args[0].arg2;
        shiftIntoArgw = args[0].arg2w;
        otherArg = args[0].arg1;
        otherArgw = args[0].arg1w;

        shiftIntoResultArg = args[2].arg2;
        shiftIntoResultArgw = args[2].arg2w;
        otherResultArg = args[2].arg1;
        otherResultArgw = args[2].arg1w;
    } else {
        shiftIntoResultReg = GET_TARGET_REG(args[2].arg1, SLJIT_R0);
        otherResultReg = GET_TARGET_REG(args[2].arg2, SLJIT_R1);

        shiftIntoArg = args[0].arg1;
        shiftIntoArgw = args[0].arg1w;
        otherArg = args[0].arg2;
        otherArgw = args[0].arg2w;

        shiftIntoResultArg = args[2].arg1;
        shiftIntoResultArgw = args[2].arg1w;
        otherResultArg = args[2].arg2;
        otherResultArgw = args[2].arg2w;
    }

    if (args[1].arg1 & SLJIT_IMM) {
        sljit_sw shift = (args[1].arg1w & 0x3f);

        if (shift & 0x20) {
            shift -= 0x20;

            if (op == SLJIT_ASHR && !IS_SOURCE_REG(otherArg)) {
                sljit_emit_op1(compiler, SLJIT_MOV, otherResultReg, 0, otherArg, otherArgw);
                otherArg = otherResultReg;
                otherArgw = 0;
            }

            if (shift == 0) {
                sljit_emit_op1(compiler, SLJIT_MOV, shiftIntoResultArg, shiftIntoResultArgw, otherArg, otherArgw);
            } else {
                sljit_emit_op2(compiler, op, shiftIntoResultArg, shiftIntoResultArgw, otherArg, otherArgw, SLJIT_IMM, shift);
            }

            if (op == SLJIT_ASHR) {
                sljit_emit_op2(compiler, op, otherResultArg, otherResultArgw, otherArg, otherArgw, SLJIT_IMM, 31);
            } else {
                sljit_emit_op1(compiler, SLJIT_MOV, otherResultArg, otherResultArgw, SLJIT_IMM, 0);
            }
            return;
        }

        if (!IS_SOURCE_REG(shiftIntoArg)) {
            sljit_emit_op1(compiler, SLJIT_MOV, shiftIntoResultReg, 0, shiftIntoArg, shiftIntoArgw);
            shiftIntoArg = shiftIntoResultReg;
        }

        if (!IS_SOURCE_REG(otherArg)) {
            sljit_emit_op1(compiler, SLJIT_MOV, otherResultReg, 0, otherArg, otherArgw);
            otherArg = otherResultReg;
        }

        sljit_emit_shift_into(compiler, (op == SLJIT_SHL ? SLJIT_SHL : SLJIT_LSHR), shiftIntoResultReg, shiftIntoArg, otherArg, SLJIT_IMM, shift);
        sljit_emit_op2(compiler, op, otherResultReg, 0, otherArg, 0, SLJIT_IMM, shift);
        MOVE_FROM_REG(compiler, SLJIT_MOV, shiftIntoResultArg, shiftIntoResultArgw, shiftIntoResultReg);
        MOVE_FROM_REG(compiler, SLJIT_MOV, otherResultArg, otherResultArgw, otherResultReg);
        return;
    }

    shiftArg = args[1].arg1;

#ifdef SLJIT_PREF_SHIFT_REG
    if (shiftArg != SLJIT_PREF_SHIFT_REG) {
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_PREF_SHIFT_REG, 0, shiftArg, args[1].arg1w);
        shiftArg = SLJIT_PREF_SHIFT_REG;
    }
#else /* SLJIT_PREF_SHIFT_REG */
    if (!IS_SOURCE_REG(shiftArg)) {
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, shiftArg, args[1].arg1w);
        shiftArg = SLJIT_R2;
    }
#endif /* !SLJIT_PREF_SHIFT_REG */

    if (!IS_SOURCE_REG(otherArg)) {
        sljit_emit_op1(compiler, SLJIT_MOV, otherResultReg, 0, otherArg, otherArgw);
        otherArg = otherResultReg;
        otherArgw = 0;
    }

    sljit_emit_op2u(compiler, SLJIT_AND | SLJIT_SET_Z, shiftArg, 0, SLJIT_IMM, 0x20);

    sljit_jump* jump1 = sljit_emit_jump(compiler, SLJIT_NOT_ZERO);

    if (!IS_SOURCE_REG(shiftIntoArg)) {
        sljit_emit_op1(compiler, SLJIT_MOV, shiftIntoResultReg, 0, shiftIntoArg, shiftIntoArgw);
        shiftIntoArg = shiftIntoResultReg;
    }
#if !(defined SLJIT_MASKED_SHIFT && SLJIT_MASKED_SHIFT)
    sljit_emit_op2(compiler, SLJIT_AND, SLJIT_R2, 0, shiftArg, 0, SLJIT_IMM, 0x1f);
    shiftArg = SLJIT_R2;
#endif /* !SLJIT_MASKED_SHIFT32 */
    sljit_emit_shift_into(compiler, (op == SLJIT_SHL ? SLJIT_SHL : SLJIT_LSHR), shiftIntoResultReg, shiftIntoArg, otherArg, shiftArg, 0);
    sljit_emit_op2(compiler, op, otherResultArg, otherResultArgw, otherArg, 0, shiftArg, 0);

    sljit_jump* jump2 = sljit_emit_jump(compiler, SLJIT_JUMP);

    sljit_set_label(jump1, sljit_emit_label(compiler));

    ASSERT_STATIC(SLJIT_MSHL == SLJIT_SHL + 1, "MSHL must be the next opcode after SHL");
    sljit_emit_op2(compiler, op + 1, shiftIntoResultReg, 0, otherArg, 0, shiftArg, 0);

    if (op == SLJIT_ASHR) {
        sljit_emit_op2(compiler, op, otherResultArg, otherResultArgw, otherArg, otherArgw, SLJIT_IMM, 31);
    } else {
        sljit_emit_op1(compiler, SLJIT_MOV, otherResultArg, otherResultArgw, SLJIT_IMM, 0);
    }

    sljit_set_label(jump2, sljit_emit_label(compiler));
    MOVE_FROM_REG(compiler, SLJIT_MOV, shiftIntoResultArg, shiftIntoResultArgw, shiftIntoResultReg);
}

static void emitRotate64(sljit_compiler* compiler, sljit_s32 op, JITArgPair* args)
{
    sljit_s32 reg1 = GET_TARGET_REG(args[2].arg1, SLJIT_R0);
    sljit_s32 reg2 = GET_TARGET_REG(args[2].arg2, SLJIT_R1);

    if (args[1].arg1 & SLJIT_IMM) {
        sljit_sw rotate = (args[1].arg1w & 0x3f);

        if (rotate & 0x20) {
            rotate -= 0x20;
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, args[0].arg2, args[0].arg2w);
            MOVE_TO_REG(compiler, SLJIT_MOV, reg2, args[0].arg1, args[0].arg1w);
            sljit_emit_op1(compiler, SLJIT_MOV, reg1, 0, SLJIT_R2, 0);
        } else {
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, args[0].arg1, args[0].arg1w);
            MOVE_TO_REG(compiler, SLJIT_MOV, reg2, args[0].arg2, args[0].arg2w);
            sljit_emit_op1(compiler, SLJIT_MOV, reg1, 0, SLJIT_R2, 0);
        }

        sljit_emit_shift_into(compiler, op, reg1, reg1, reg2, SLJIT_IMM, rotate);
        sljit_emit_shift_into(compiler, op, reg2, reg2, SLJIT_R2, SLJIT_IMM, rotate);

        MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg1, args[2].arg1w, reg1);
        MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg2, args[2].arg2w, reg2);
        return;
    }

#ifdef SLJIT_PREF_SHIFT_REG
    sljit_s32 rotateReg = SLJIT_PREF_SHIFT_REG;
#if SLJIT_PREF_SHIFT_REG == SLJIT_R2
    sljit_s32 tmpReg = SLJIT_R3;
#else /* SLJIT_PREF_SHIFT_REG != SLJIT_R2 */
#error "Unknown shift register"
#endif /* SLJIT_PREF_SHIFT_REG == SLJIT_R2 */
#else /* SLJIT_PREF_SHIFT_REG */
    sljit_s32 rotateReg = GET_TARGET_REG(args[1].arg1, SLJIT_R2);
    sljit_s32 tmpReg = (rotateReg == SLJIT_R2) ? SLJIT_R3 : SLJIT_R2;
#endif /* !SLJIT_PREF_SHIFT_REG */

    MOVE_TO_REG(compiler, SLJIT_MOV, rotateReg, args[1].arg1, args[1].arg1w);
    MOVE_TO_REG(compiler, SLJIT_MOV, reg1, args[0].arg1, args[0].arg1w);
    MOVE_TO_REG(compiler, SLJIT_MOV, reg2, args[0].arg2, args[0].arg2w);

    sljit_emit_op2u(compiler, SLJIT_AND | SLJIT_SET_Z, rotateReg, 0, SLJIT_IMM, 0x20);
    sljit_jump* jump1 = sljit_emit_jump(compiler, SLJIT_ZERO);
    /* Swap registers. */
    sljit_emit_op2(compiler, SLJIT_XOR, reg1, 0, reg1, 0, reg2, 0);
    sljit_emit_op2(compiler, SLJIT_XOR, reg2, 0, reg2, 0, reg1, 0);
    sljit_emit_op2(compiler, SLJIT_XOR, reg1, 0, reg1, 0, reg2, 0);
    sljit_set_label(jump1, sljit_emit_label(compiler));

#if !(defined SLJIT_MASKED_SHIFT && SLJIT_MASKED_SHIFT)
    if (tmpReg != SLJIT_R2) {
        sljit_emit_op2(compiler, SLJIT_AND, SLJIT_R3, 0, rotateReg, 0, SLJIT_IMM, 0x1f);
        rotateReg = SLJIT_R3;
    } else {
        sljit_emit_op2(compiler, SLJIT_AND, SLJIT_R2, 0, rotateReg, 0, SLJIT_IMM, 0x1f);
        rotateReg = SLJIT_R2;
    }
#endif /* !SLJIT_MASKED_SHIFT32 */
    sljit_emit_op1(compiler, SLJIT_MOV, tmpReg, 0, reg1, 0);
    sljit_emit_shift_into(compiler, op, reg1, reg1, reg2, rotateReg, 0);
    sljit_emit_shift_into(compiler, op, reg2, reg2, tmpReg, rotateReg, 0);

    MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg1, args[2].arg1w, reg1);
    MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg2, args[2].arg2w, reg2);
}

static void emitMul64(sljit_compiler* compiler, JITArgPair* args)
{
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
    sljit_s32 tmpReg = SLJIT_S2;
#else
    sljit_s32 tmpReg = SLJIT_R3;
#endif

    MOVE_TO_REG(compiler, SLJIT_MOV, SLJIT_R0, args[0].arg1, args[0].arg1w);
    MOVE_TO_REG(compiler, SLJIT_MOV, SLJIT_R1, args[1].arg1, args[1].arg1w);
    MOVE_TO_REG(compiler, SLJIT_MOV, SLJIT_R2, args[0].arg2, args[0].arg2w);
    MOVE_TO_REG(compiler, SLJIT_MOV, tmpReg, args[1].arg2, args[1].arg2w);
    sljit_emit_op2(compiler, SLJIT_MUL, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_R1, 0);
    sljit_emit_op2(compiler, SLJIT_MUL, tmpReg, 0, tmpReg, 0, SLJIT_R0, 0);
    sljit_emit_op0(compiler, SLJIT_LMUL_UW);
    sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R2, 0, SLJIT_R2, 0, tmpReg, 0);
    sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_R2, 0);
    MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg1, args[2].arg1w, SLJIT_R0);
    MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg2, args[2].arg2w, SLJIT_R1);
}

static sljit_sw signedDiv64(int64_t* dividend, int64_t* divisor, int64_t* quotient)
{
    if (*divisor == 0) {
        return ExecutionContext::DivideByZeroError;
    }

    if (*divisor == -1 && *dividend == INT64_MIN) {
        return ExecutionContext::IntegerOverflowError;
    }

    *quotient = *dividend / *divisor;
    return ExecutionContext::NoError;
}

static void signedDiv64Imm(int64_t* dividend, int64_t* divisor, int64_t* quotient)
{
    *quotient = *dividend / *divisor;
}

static sljit_sw unsignedDiv64(uint64_t* dividend, uint64_t* divisor, uint64_t* quotient)
{
    if (*divisor == 0) {
        return ExecutionContext::DivideByZeroError;
    }

    *quotient = *dividend / *divisor;
    return ExecutionContext::NoError;
}

static void unsignedDiv64Imm(uint64_t* dividend, uint64_t* divisor, uint64_t* quotient)
{
    *quotient = *dividend / *divisor;
}

static sljit_sw signedRem64(int64_t* dividend, int64_t* divisor, int64_t* quotient)
{
    if (*divisor == 0) {
        return ExecutionContext::DivideByZeroError;
    }

    if (*divisor == -1) {
        *quotient = 0;
        return ExecutionContext::NoError;
    }

    *quotient = *dividend % *divisor;
    return ExecutionContext::NoError;
}

static void signedRem64Imm(int64_t* dividend, int64_t* divisor, int64_t* quotient)
{
    *quotient = *dividend % *divisor;
}

static sljit_sw unsignedRem64(uint64_t* dividend, uint64_t* divisor, uint64_t* quotient)
{
    if (*divisor == 0) {
        return ExecutionContext::DivideByZeroError;
    }

    *quotient = *dividend % *divisor;
    return ExecutionContext::NoError;
}

static void unsignedRem64Imm(uint64_t* dividend, uint64_t* divisor, uint64_t* quotient)
{
    *quotient = *dividend % *divisor;
}

static void emitDivRem64(sljit_compiler* compiler, sljit_s32 opcode, JITArgPair* args)
{
    sljit_sw addr;
    bool isImm = (args[1].arg1 & SLJIT_IMM) != 0;
    CompileContext* context = CompileContext::get(compiler);

    if (isImm) {
        if ((args[1].arg1w | args[1].arg2w) == 0) {
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM, ExecutionContext::DivideByZeroError);
            sljit_set_label(sljit_emit_jump(compiler, SLJIT_JUMP), context->trapLabel);
            return;
        }
        if ((args[1].arg1w & args[1].arg2w) == -1 && opcode == SLJIT_DIVMOD_SW) {
            sljit_emit_op1(compiler, SLJIT_MOV, args[2].arg1, args[2].arg1w, SLJIT_IMM, 0);
            sljit_emit_op1(compiler, SLJIT_MOV, args[2].arg2, args[2].arg2w, SLJIT_IMM, 0);
            return;
        }
    }

    if (args[0].arg1 != SLJIT_MEM1(kFrameReg)) {
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(kContextReg), OffsetOfContextField(tmp1) + WORD_LOW_OFFSET, args[0].arg1, args[0].arg1w);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(kContextReg), OffsetOfContextField(tmp1) + WORD_HIGH_OFFSET, args[0].arg2, args[0].arg2w);
    }

    if (args[1].arg1 != SLJIT_MEM1(kFrameReg)) {
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(kContextReg), OffsetOfContextField(tmp2) + WORD_LOW_OFFSET, args[1].arg1, args[1].arg1w);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(kContextReg), OffsetOfContextField(tmp2) + WORD_HIGH_OFFSET, args[1].arg2, args[1].arg2w);
    }

    if (args[0].arg1 == SLJIT_MEM1(kFrameReg)) {
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R0, 0, kFrameReg, 0, SLJIT_IMM, args[0].arg1w - WORD_LOW_OFFSET);
    } else {
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R0, 0, kContextReg, 0, SLJIT_IMM, OffsetOfContextField(tmp1));
    }

    if (args[1].arg1 == SLJIT_MEM1(kFrameReg)) {
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R1, 0, kFrameReg, 0, SLJIT_IMM, args[1].arg1w - WORD_LOW_OFFSET);
    } else {
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R1, 0, kContextReg, 0, SLJIT_IMM, OffsetOfContextField(tmp2));
    }

    if (args[2].arg1 == SLJIT_MEM1(kFrameReg)) {
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R2, 0, kFrameReg, 0, SLJIT_IMM, args[2].arg1w - WORD_LOW_OFFSET);
    } else {
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R2, 0, kContextReg, 0, SLJIT_IMM, OffsetOfContextField(tmp1));
    }

    switch (opcode) {
    case SLJIT_DIV_SW:
        if (isImm && (args[1].arg1w & args[1].arg2w) == -1) {
            isImm = false;
        }

        addr = isImm ? GET_FUNC_ADDR(sljit_sw, signedDiv64Imm) : GET_FUNC_ADDR(sljit_sw, signedDiv64);
        break;
    case SLJIT_DIV_UW:
        addr = isImm ? GET_FUNC_ADDR(sljit_sw, unsignedDiv64Imm) : GET_FUNC_ADDR(sljit_sw, unsignedDiv64);
        break;
    case SLJIT_DIVMOD_SW:
        addr = isImm ? GET_FUNC_ADDR(sljit_sw, signedRem64Imm) : GET_FUNC_ADDR(sljit_sw, signedRem64);
        break;
    default:
        ASSERT(opcode == SLJIT_DIVMOD_UW);
        addr = isImm ? GET_FUNC_ADDR(sljit_sw, unsignedRem64Imm) : GET_FUNC_ADDR(sljit_sw, unsignedRem64);
        break;
    }

    sljit_s32 type = isImm ? SLJIT_ARGS3(VOID, W, W, W) : SLJIT_ARGS3(W, W, W, W);
    sljit_emit_icall(compiler, SLJIT_CALL, type, SLJIT_IMM, addr);

    if (!isImm) {
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_R0, 0);
        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, ExecutionContext::NoError);
        sljit_set_label(cmp, context->trapLabel);
    }

    if (args[2].arg1 != SLJIT_MEM1(kFrameReg)) {
        sljit_emit_op1(compiler, SLJIT_MOV, args[2].arg1, args[2].arg1w, SLJIT_MEM1(kContextReg), OffsetOfContextField(tmp1) + WORD_LOW_OFFSET);
        sljit_emit_op1(compiler, SLJIT_MOV, args[2].arg2, args[2].arg2w, SLJIT_MEM1(kContextReg), OffsetOfContextField(tmp1) + WORD_HIGH_OFFSET);
    }
}

static void emitBinary(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();

    if (instr->info() & Instruction::kIs32Bit) {
        JITArg args[3];

        for (int i = 0; i < 3; ++i) {
            operandToArg(operands + i, args[i]);
        }

        sljit_s32 opcode;

        switch (instr->opcode()) {
        case I32AddOpcode:
            opcode = SLJIT_ADD;
            break;
        case I32SubOpcode:
            opcode = SLJIT_SUB;
            break;
        case I32MulOpcode:
            opcode = SLJIT_MUL;
            break;
        case I32DivSOpcode:
            emitDivRem32(compiler, SLJIT_DIV_SW, args);
            return;
        case I32DivUOpcode:
            emitDivRem32(compiler, SLJIT_DIV_UW, args);
            return;
        case I32RemSOpcode:
            emitDivRem32(compiler, SLJIT_DIVMOD_SW, args);
            return;
        case I32RemUOpcode:
            emitDivRem32(compiler, SLJIT_DIVMOD_UW, args);
            return;
        case I32RotlOpcode:
            opcode = SLJIT_ROTL;
            break;
        case I32RotrOpcode:
            opcode = SLJIT_ROTR;
            break;
        case I32AndOpcode:
            opcode = SLJIT_AND;
            break;
        case I32OrOpcode:
            opcode = SLJIT_OR;
            break;
        case I32XorOpcode:
            opcode = SLJIT_XOR;
            break;
        case I32ShlOpcode:
            opcode = SLJIT_SHL;
            break;
        case I32ShrSOpcode:
            opcode = SLJIT_ASHR;
            break;
        case I32ShrUOpcode:
            opcode = SLJIT_LSHR;
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }

        sljit_emit_op2(compiler, opcode, args[2].arg, args[2].argw, args[0].arg, args[0].argw, args[1].arg, args[1].argw);
        return;
    }

    JITArgPair args[3];

    for (int i = 0; i < 3; ++i) {
        operandToArgPair(operands + i, args[i]);
    }

    switch (instr->opcode()) {
    case I64AddOpcode:
        emitSimpleBinary64(compiler, SLJIT_ADD | SLJIT_SET_CARRY, SLJIT_ADDC, args);
        return;
    case I64SubOpcode:
        emitSimpleBinary64(compiler, SLJIT_SUB | SLJIT_SET_CARRY, SLJIT_SUBC, args);
        return;
    case I64MulOpcode:
        emitMul64(compiler, args);
        return;
    case I64DivSOpcode:
        emitDivRem64(compiler, SLJIT_DIV_SW, args);
        return;
    case I64DivUOpcode:
        emitDivRem64(compiler, SLJIT_DIV_UW, args);
        return;
    case I64RemSOpcode:
        emitDivRem64(compiler, SLJIT_DIVMOD_SW, args);
        return;
    case I64RemUOpcode:
        emitDivRem64(compiler, SLJIT_DIVMOD_UW, args);
        return;
    case I64RotlOpcode:
        emitRotate64(compiler, SLJIT_SHL, args);
        return;
    case I64RotrOpcode:
        emitRotate64(compiler, SLJIT_LSHR, args);
        return;
    case I64AndOpcode:
        emitSimpleBinary64(compiler, SLJIT_AND, SLJIT_AND, args);
        return;
    case I64OrOpcode:
        emitSimpleBinary64(compiler, SLJIT_OR, SLJIT_OR, args);
        return;
    case I64XorOpcode:
        emitSimpleBinary64(compiler, SLJIT_XOR, SLJIT_XOR, args);
        return;
    case I64ShlOpcode:
        emitShift64(compiler, SLJIT_SHL, args);
        return;
    case I64ShrSOpcode:
        emitShift64(compiler, SLJIT_ASHR, args);
        return;
    case I64ShrUOpcode:
        emitShift64(compiler, SLJIT_LSHR, args);
        return;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
}

static void emitCountZeroes(sljit_compiler* compiler, sljit_s32 op, JITArgPair* args)
{
    sljit_s32 resultReg;

    resultReg = GET_TARGET_REG(args[1].arg1, SLJIT_R1);

    if (op == SLJIT_CLZ) {
        MOVE_TO_REG(compiler, SLJIT_MOV, resultReg, args[0].arg2, args[0].arg2w);
    } else {
        MOVE_TO_REG(compiler, SLJIT_MOV, resultReg, args[0].arg1, args[0].arg1w);
    }

    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, 0);
    sljit_jump* jump = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, resultReg, 0, SLJIT_IMM, 0);

    if (op == SLJIT_CLZ) {
        MOVE_TO_REG(compiler, SLJIT_MOV, resultReg, args[0].arg1, args[0].arg1w);
    } else {
        MOVE_TO_REG(compiler, SLJIT_MOV, resultReg, args[0].arg2, args[0].arg2w);
    }

    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, 32);
    sljit_set_label(jump, sljit_emit_label(compiler));

    sljit_emit_op1(compiler, op, resultReg, 0, resultReg, 0);
    sljit_emit_op2(compiler, SLJIT_ADD, resultReg, 0, resultReg, 0, SLJIT_R0, 0);

    MOVE_FROM_REG(compiler, SLJIT_MOV, args[1].arg1, args[1].arg1w, resultReg);
    sljit_emit_op1(compiler, SLJIT_MOV, args[1].arg2, args[1].arg2w, SLJIT_IMM, 0);
}

static sljit_sw popcnt32(sljit_sw arg)
{
    return __builtin_popcount(arg);
}

static void emitPopcnt(sljit_compiler* compiler, JITArg* args)
{
    MOVE_TO_REG(compiler, SLJIT_MOV, SLJIT_R0, args[0].arg, args[0].argw);

    sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS1(W, W), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, popcnt32));

    MOVE_FROM_REG(compiler, SLJIT_MOV, args[1].arg, args[1].argw, SLJIT_R0);
}

static sljit_sw popcnt64(sljit_sw arg1, sljit_sw arg2)
{
    return __builtin_popcount(arg1) + __builtin_popcount(arg2);
}

static void emitPopcnt64(sljit_compiler* compiler, JITArgPair* args)
{
    MOVE_TO_REG(compiler, SLJIT_MOV, SLJIT_R0, args[0].arg1, args[0].arg1w);
    MOVE_TO_REG(compiler, SLJIT_MOV, SLJIT_R1, args[0].arg2, args[0].arg2w);

    sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS2(W, W, W), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, popcnt64));

    MOVE_FROM_REG(compiler, SLJIT_MOV, args[1].arg1, args[1].arg1w, SLJIT_R0);
    sljit_emit_op1(compiler, SLJIT_MOV, args[1].arg2, args[1].arg2w, SLJIT_IMM, 0);
}

static void emitExtend(sljit_compiler* compiler, sljit_s32 opcode, JITArg* args)
{
    sljit_s32 reg = GET_TARGET_REG(args[1].arg, SLJIT_R0);

    ASSERT((args[0].arg >> 8) == 0);
#if (defined SLJIT_BIG_ENDIAN && SLJIT_BIG_ENDIAN)
    if (args[0].arg & SLJIT_MEM) {
        args[0].argw += (opcode == SLJIT_MOV_S8) ? 3 : 2;
    }
#endif /* SLJIT_BIG_ENDIAN */

    sljit_emit_op1(compiler, opcode, reg, 0, args[0].arg, args[0].argw);
    MOVE_FROM_REG(compiler, SLJIT_MOV, args[1].arg, args[1].argw, reg);
}

static void emitExtend64(sljit_compiler* compiler, sljit_s32 opcode, JITArgPair* args)
{
    sljit_s32 reg = GET_TARGET_REG(args[1].arg1, SLJIT_R0);

    ASSERT((args[0].arg1 >> 8) == 0);
#if (defined SLJIT_BIG_ENDIAN && SLJIT_BIG_ENDIAN)
    if (args[0].arg1 & SLJIT_MEM) {
        args[0].arg1w += (opcode == SLJIT_MOV_S8) ? 3 : 2;
    }
#endif /* SLJIT_BIG_ENDIAN */

    sljit_emit_op1(compiler, opcode, reg, 0, args[0].arg1, args[0].arg1w);
    MOVE_FROM_REG(compiler, SLJIT_MOV, args[1].arg1, args[1].arg1w, reg);

    if (args[1].arg1 & SLJIT_MEM) {
        sljit_emit_op2(compiler, SLJIT_ASHR, reg, 0, reg, 0, SLJIT_IMM, 31);
        sljit_emit_op1(compiler, SLJIT_MOV, args[1].arg2, args[1].arg2w, reg, 0);
        return;
    }

    sljit_emit_op2(compiler, SLJIT_ASHR, args[1].arg2, args[1].arg2w, reg, 0, SLJIT_IMM, 31);
}

static void emitUnary(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();

    if (instr->info() & Instruction::kIs32Bit) {
        JITArg args[2];

        for (int i = 0; i < 2; ++i) {
            operandToArg(operands + i, args[i]);
        }

        sljit_s32 opcode;

        switch (instr->opcode()) {
        case I32ClzOpcode:
            opcode = SLJIT_CLZ;
            break;
        case I32CtzOpcode:
            opcode = SLJIT_CTZ;
            break;
        case I32PopcntOpcode:
            emitPopcnt(compiler, args);
            return;
        case I32Extend8SOpcode:
            emitExtend(compiler, SLJIT_MOV_S8, args);
            return;
        case I32Extend16SOpcode:
            emitExtend(compiler, SLJIT_MOV_S16, args);
            return;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }

        // If the operand is an immediate then it is necesarry to move it into a
        // register because immediate source arguments are not supported.
        if (args[0].arg & SLJIT_IMM) {
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, args[0].arg, args[0].argw);
            args[0].arg = SLJIT_R0;
            args[0].argw = 0;
        }

        sljit_emit_op1(compiler, opcode, args[1].arg, args[1].argw, args[0].arg, args[0].argw);
        return;
    }

    JITArgPair args[2];

    for (int i = 0; i < 2; ++i) {
        operandToArgPair(operands + i, args[i]);
    }

    switch (instr->opcode()) {
    case I64ClzOpcode:
        emitCountZeroes(compiler, SLJIT_CLZ, args);
        return;
    case I64CtzOpcode:
        emitCountZeroes(compiler, SLJIT_CTZ, args);
        return;
    case I64PopcntOpcode:
        emitPopcnt64(compiler, args);
        return;
    case I64Extend8SOpcode:
        emitExtend64(compiler, SLJIT_MOV_S8, args);
        return;
    case I64Extend16SOpcode:
        emitExtend64(compiler, SLJIT_MOV_S16, args);
        return;
    case I64Extend32SOpcode:
        if (args[0].arg1 == args[1].arg1 && args[0].arg1w == args[1].arg1w) {
            sljit_emit_op2(compiler, SLJIT_ASHR, args[1].arg2, args[1].arg2w, args[0].arg1, args[0].arg1w, SLJIT_IMM, 31);
            return;
        }
        emitExtend64(compiler, SLJIT_MOV, args);
        return;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
}

void emitSelect(sljit_compiler* compiler, Instruction* instr, sljit_s32 type)
{
    Operand* operands = instr->operands();
    JITArg cond;
    ASSERT(instr->opcode() == SelectOpcode && instr->paramCount() == 3);

    if (false) {
        return emitFloatSelect(compiler, instr, type);
    }

    if (reinterpret_cast<Select*>(instr->byteCode())->valueSize() == 4) {
        JITArg args[3];

        operandToArg(operands + 0, args[0]);
        operandToArg(operands + 1, args[1]);
        operandToArg(operands + 3, args[2]);

        if (type == -1) {
            operandToArg(operands + 2, cond);
            sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_Z, cond.arg, cond.argw, SLJIT_IMM, 0);

            type = SLJIT_NOT_ZERO;
        }

        sljit_s32 targetReg = GET_TARGET_REG(args[2].arg, SLJIT_R0);

        if (!IS_SOURCE_REG(args[1].arg)) {
            sljit_emit_op1(compiler, SLJIT_MOV, targetReg, 0, args[1].arg, args[1].argw);
            args[1].arg = targetReg;
        }

        sljit_emit_select(compiler, type, targetReg, args[0].arg, args[0].argw, args[1].arg);
        MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg, args[2].argw, targetReg);
        return;
    }

    JITArgPair args[3];

    operandToArgPair(operands + 0, args[0]);
    operandToArgPair(operands + 1, args[1]);
    operandToArgPair(operands + 3, args[2]);

    if (type == -1) {
        operandToArg(operands + 2, cond);
        sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_Z, cond.arg, cond.argw, SLJIT_IMM, 0);

        type = SLJIT_NOT_ZERO;
    }

    sljit_s32 targetReg1 = GET_TARGET_REG(args[2].arg1, SLJIT_R0);
    sljit_s32 targetReg2 = GET_TARGET_REG(args[2].arg2, SLJIT_R1);

    if (!sljit_has_cpu_feature(SLJIT_HAS_CMOV)) {
        MOVE_TO_REG(compiler, SLJIT_MOV, targetReg1, args[0].arg1, args[0].arg1w);
        MOVE_TO_REG(compiler, SLJIT_MOV, targetReg2, args[0].arg2, args[0].arg2w);
        sljit_jump* jump = sljit_emit_jump(compiler, type);
        MOVE_TO_REG(compiler, SLJIT_MOV, targetReg1, args[1].arg1, args[1].arg1w);
        MOVE_TO_REG(compiler, SLJIT_MOV, targetReg2, args[1].arg2, args[1].arg2w);
        sljit_set_label(jump, sljit_emit_label(compiler));
        MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg1, args[2].arg1w, targetReg1);
        MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg2, args[2].arg2w, targetReg2);
        return;
    }

    if (!IS_SOURCE_REG(args[1].arg1)) {
        sljit_emit_op1(compiler, SLJIT_MOV, targetReg1, 0, args[1].arg1, args[1].arg1w);
        args[1].arg1 = targetReg1;
    }

    if (!IS_SOURCE_REG(args[1].arg2)) {
        sljit_emit_op1(compiler, SLJIT_MOV, targetReg2, 0, args[1].arg2, args[1].arg2w);
        args[1].arg2 = targetReg2;
    }

    sljit_emit_select(compiler, type, targetReg1, args[0].arg1, args[0].arg1w, args[1].arg1);
    sljit_emit_select(compiler, type, targetReg2, args[0].arg2, args[0].arg2w, args[1].arg2);
    MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg1, args[2].arg1w, targetReg1);
    MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg2, args[2].arg2w, targetReg2);
}

static bool emitCompare(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operand = instr->operands();
    sljit_s32 opcode, type;

    switch (instr->opcode()) {
    case I32EqzOpcode:
    case I64EqzOpcode:
        opcode = SLJIT_SUB | SLJIT_SET_Z;
        type = SLJIT_EQUAL;
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
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }

    Instruction* nextInstr = nullptr;
    bool isBranch = false;
    bool isSelect = false;

    ASSERT(instr->next() != nullptr);

    if (instr->next()->isInstruction()) {
        nextInstr = instr->next()->asInstruction();

        switch (nextInstr->opcode()) {
        case JumpIfTrueOpcode:
        case JumpIfFalseOpcode:
            if (nextInstr->getParam(0)->item == instr) {
                isBranch = true;
            }
            break;
        case SelectOpcode:
            if (nextInstr->getParam(2)->item == instr) {
                isSelect = true;
            }
            break;
        default:
            break;
        }
    }

    if (!(instr->info() & Instruction::kIs32Bit)) {
        JITArgPair params[2];

        for (uint32_t i = 0; i < instr->paramCount(); ++i) {
            operandToArgPair(operand, params[i]);
            operand++;
        }

        if (instr->opcode() == I64EqzOpcode) {
            sljit_emit_op2u(compiler, SLJIT_OR | SLJIT_SET_Z, params[0].arg1, params[0].arg1w, params[0].arg2, params[0].arg2w);
        } else {
            sljit_emit_op2u(compiler, opcode | SLJIT_SET_Z, params[0].arg2, params[0].arg2w, params[1].arg2, params[1].arg2w);
            sljit_jump* jump = sljit_emit_jump(compiler, SLJIT_NOT_ZERO);
            sljit_emit_op2u(compiler, opcode, params[0].arg1, params[0].arg1w, params[1].arg1, params[1].arg1w);
            sljit_set_label(jump, sljit_emit_label(compiler));
            sljit_set_current_flags(compiler, (opcode - SLJIT_SUB) | SLJIT_CURRENT_FLAGS_SUB);
        }

        if (isBranch) {
            if (nextInstr->opcode() == JumpIfFalseOpcode) {
                type ^= 0x1;
            }

            sljit_jump* jump = sljit_emit_jump(compiler, type);
            nextInstr->asExtended()->value().targetLabel->jumpFrom(jump);
            return true;
        }

        if (isSelect) {
            emitSelect(compiler, instr, type);
            return true;
        }

        JITArg result;
        operandToArg(operand, result);
        sljit_emit_op_flags(compiler, SLJIT_MOV, result.arg, result.argw, type);
        return false;
    }

    JITArg params[2];

    for (uint32_t i = 0; i < instr->paramCount(); ++i) {
        operandToArg(operand, params[i]);
        operand++;
    }

    if (instr->opcode() == I32EqzOpcode) {
        params[1].arg = SLJIT_IMM;
        params[1].argw = 0;
    }

    if (isBranch) {
        if (nextInstr->opcode() == JumpIfFalseOpcode) {
            type ^= 0x1;
        }

        sljit_jump* jump = sljit_emit_cmp(compiler, type, params[0].arg, params[0].argw, params[1].arg, params[1].argw);
        nextInstr->asExtended()->value().targetLabel->jumpFrom(jump);
        return true;
    }

    sljit_emit_op2u(compiler, opcode, params[0].arg, params[0].argw, params[1].arg, params[1].argw);

    if (isSelect) {
        emitSelect(compiler, nextInstr, type);
        return true;
    }

    JITArg result;
    operandToArg(operand, result);
    sljit_emit_op_flags(compiler, SLJIT_MOV, result.arg, result.argw, type);
    return false;
}

static void emitConvert(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operand = instr->operands();

    switch (instr->opcode()) {
    case I32WrapI64Opcode: {
        /* Just copy the lower word. */
        JITArgPair param;
        JITArg result;

        operandToArgPair(operand, param);
        operandToArg(operand + 1, result);

        sljit_emit_op1(compiler, SLJIT_MOV, result.arg, result.argw, param.arg1, param.arg1w);
        return;
    }
    case I64ExtendI32SOpcode: {
        JITArg param;
        JITArgPair result;

        operandToArg(operand, param);
        operandToArgPair(operand + 1, result);

        if (SLJIT_IS_IMM(param.arg)) {
            sljit_emit_op1(compiler, SLJIT_MOV, result.arg1, result.arg1w, SLJIT_IMM, param.argw);
            sljit_emit_op1(compiler, SLJIT_MOV, result.arg2, result.arg2w, SLJIT_IMM, param.argw >> 31);
            return;
        }

        sljit_s32 param_reg = GET_SOURCE_REG(param.arg, SLJIT_R0);
        MOVE_TO_REG(compiler, SLJIT_MOV, param_reg, param.arg, param.argw);
        MOVE_FROM_REG(compiler, SLJIT_MOV, result.arg1, result.arg1w, param_reg);
        sljit_emit_op2(compiler, SLJIT_ASHR, result.arg2, result.arg2w, param_reg, 0, SLJIT_IMM, 31);
        return;
    }
    case I64ExtendI32UOpcode: {
        JITArg param;
        JITArgPair result;

        operandToArg(operand, param);
        operandToArgPair(operand + 1, result);

        sljit_emit_op1(compiler, SLJIT_MOV, result.arg1, result.arg1w, param.arg, param.argw);
        sljit_emit_op1(compiler, SLJIT_MOV, result.arg2, result.arg2w, SLJIT_IMM, 0);
        return;
    }
    default: {
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
    }
}

static void emitMove64(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArgPair src;
    JITArgPair dst;

    operandToArgPair(operands, src);
    operandToArgPair(operands + 1, dst);

    sljit_emit_op1(compiler, SLJIT_MOV, dst.arg1, dst.arg1w, src.arg1, src.arg1w);
    sljit_emit_op1(compiler, SLJIT_MOV, dst.arg2, dst.arg2w, src.arg2, src.arg2w);
}
