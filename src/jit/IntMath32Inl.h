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
    JITArgPair(Operand* operand)
    {
        this->set(operand);
    }

    JITArgPair() = default;

    void set(Operand* operand);

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

void JITArgPair::set(Operand* operand)
{
    if (VARIABLE_TYPE(*operand) != Instruction::ConstPtr) {
        if (VARIABLE_TYPE(*operand) == Instruction::Register) {
            sljit_sw regs = VARIABLE_GET_REF(*operand);

            this->arg1 = regs & 0xff;
            this->arg1w = 0;
            this->arg2 = regs >> 8;
            this->arg2w = 0;
            return;
        }

        sljit_sw offset = static_cast<sljit_sw>(VARIABLE_GET_OFFSET(*operand));

        this->arg1 = SLJIT_MEM1(kFrameReg);
        this->arg1w = offset + WORD_LOW_OFFSET;
        this->arg2 = SLJIT_MEM1(kFrameReg);
        this->arg2w = offset + WORD_HIGH_OFFSET;
        return;
    }

    this->arg1 = SLJIT_IMM;
    this->arg2 = SLJIT_IMM;
    Instruction* instr = VARIABLE_GET_IMM(*operand);

    uint64_t value64 = reinterpret_cast<Const64*>(instr->byteCode())->value();

    this->arg1w = static_cast<sljit_sw>(value64);
    this->arg2w = static_cast<sljit_sw>(value64 >> 32);
}

static void emitDivRem32(sljit_compiler* compiler, sljit_s32 opcode, JITArg* args)
{
    CompileContext* context = CompileContext::get(compiler);

    if (SLJIT_IS_IMM(args[1].arg)) {
        if (args[1].argw == 0) {
            context->appendTrapJump(ExecutionContext::DivideByZeroError, sljit_emit_jump(compiler, SLJIT_JUMP));
            return;
        } else if (args[1].argw == -1 && opcode == SLJIT_DIVMOD_SW) {
            sljit_emit_op1(compiler, SLJIT_MOV, args[2].arg, args[2].argw, SLJIT_IMM, 0);
            return;
        }
    }

    emitInitR0R1(compiler, SLJIT_MOV, SLJIT_MOV, args);
    sljit_jump* moduloJumpFrom = nullptr;

    if (SLJIT_IS_IMM(args[1].arg)) {
        if (opcode == SLJIT_DIV_SW && args[1].argw == -1) {
            sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, static_cast<sljit_sw>(INT32_MIN));
            context->appendTrapJump(ExecutionContext::IntegerOverflowError, cmp);
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
        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_EQUAL, SLJIT_R1, 0, SLJIT_IMM, 0);
        context->appendTrapJump(ExecutionContext::DivideByZeroError, cmp);
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

static void emitSimpleBinary64(sljit_compiler* compiler, sljit_s32 op1, sljit_s32 op2, Instruction* instr, JITArgPair* args)
{
    sljit_s32 dst0 = instr->requiredReg(0);
    sljit_s32 dst1 = instr->requiredReg(1);

    if (args[0].arg1 != dst0 && args[1].arg1 != dst0) {
        ASSERT(args[0].arg1 != dst1 && args[1].arg1 != dst1
               && args[0].arg1 != dst0 && args[0].arg2 != dst1
               && args[1].arg2 != dst0 && args[1].arg2 != dst1);

        for (int i = 0; i < 2; i++) {
            if (SLJIT_IS_MEM(args[i].arg1)) {
                sljit_emit_op1(compiler, SLJIT_MOV, dst0, 0, args[i].arg1, args[i].arg1w);
                sljit_emit_op1(compiler, SLJIT_MOV, dst1, 0, args[i].arg2, args[i].arg2w);

                args[i].arg1 = dst0;
                args[i].arg1w = 0;
                args[i].arg2 = dst1;
                args[i].arg2w = 0;
                break;
            }
        }
    }

    sljit_emit_op2(compiler, op1, args[2].arg1, args[2].arg1w, args[0].arg1, args[0].arg1w, args[1].arg1, args[1].arg1w);
    sljit_emit_op2(compiler, op2, args[2].arg2, args[2].arg2w, args[0].arg2, args[0].arg2w, args[1].arg2, args[1].arg2w);
}

static void emitShift64(sljit_compiler* compiler, sljit_s32 op, Instruction* instr, JITArgPair* args)
{
    sljit_s32 shiftIntoResultReg, otherResultReg;
    sljit_s32 shiftIntoArg, otherArg;
    sljit_s32 shiftIntoResultArg, otherResultArg;
    sljit_sw shiftIntoArgw, otherArgw;
    sljit_sw shiftIntoResultArgw, otherResultArgw;

    if (op == SLJIT_SHL) {
        shiftIntoResultReg = GET_TARGET_REG(args[2].arg2, instr->requiredReg(1));
        otherResultReg = GET_TARGET_REG(args[2].arg1, instr->requiredReg(0));

        shiftIntoArg = args[0].arg2;
        shiftIntoArgw = args[0].arg2w;
        otherArg = args[0].arg1;
        otherArgw = args[0].arg1w;

        shiftIntoResultArg = args[2].arg2;
        shiftIntoResultArgw = args[2].arg2w;
        otherResultArg = args[2].arg1;
        otherResultArgw = args[2].arg1w;
    } else {
        shiftIntoResultReg = GET_TARGET_REG(args[2].arg1, instr->requiredReg(0));
        otherResultReg = GET_TARGET_REG(args[2].arg2, instr->requiredReg(1));

        shiftIntoArg = args[0].arg1;
        shiftIntoArgw = args[0].arg1w;
        otherArg = args[0].arg2;
        otherArgw = args[0].arg2w;

        shiftIntoResultArg = args[2].arg1;
        shiftIntoResultArgw = args[2].arg1w;
        otherResultArg = args[2].arg2;
        otherResultArgw = args[2].arg2w;
    }

    if (SLJIT_IS_IMM(args[1].arg1)) {
        sljit_sw shift = (args[1].arg1w & 0x3f);

        if (shift & 0x20) {
            shift -= 0x20;

            if (op == SLJIT_ASHR && !SLJIT_IS_REG(otherArg)) {
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

        if (!SLJIT_IS_REG(shiftIntoArg)) {
            sljit_emit_op1(compiler, SLJIT_MOV, shiftIntoResultReg, 0, shiftIntoArg, shiftIntoArgw);
            shiftIntoArg = shiftIntoResultReg;
        }

        if (!SLJIT_IS_REG(otherArg)) {
            sljit_emit_op1(compiler, SLJIT_MOV, otherResultReg, 0, otherArg, otherArgw);
            otherArg = otherResultReg;
        }

        sljit_emit_shift_into(compiler, (op == SLJIT_SHL ? SLJIT_SHL : SLJIT_LSHR), shiftIntoResultReg, shiftIntoArg, otherArg, SLJIT_IMM, shift);
        sljit_emit_op2(compiler, op, otherResultReg, 0, otherArg, 0, SLJIT_IMM, shift);
        MOVE_FROM_REG(compiler, SLJIT_MOV, shiftIntoResultArg, shiftIntoResultArgw, shiftIntoResultReg);
        MOVE_FROM_REG(compiler, SLJIT_MOV, otherResultArg, otherResultArgw, otherResultReg);
        return;
    }

    sljit_s32 shiftArg = instr->requiredReg(2);
    MOVE_TO_REG(compiler, SLJIT_MOV, shiftArg, args[1].arg1, args[1].arg1w);

    if (!SLJIT_IS_REG(otherArg)) {
        sljit_emit_op1(compiler, SLJIT_MOV, otherResultReg, 0, otherArg, otherArgw);
        otherArg = otherResultReg;
        otherArgw = 0;
    }

    sljit_emit_op2u(compiler, SLJIT_AND | SLJIT_SET_Z, shiftArg, 0, SLJIT_IMM, 0x20);

    sljit_jump* jump1 = sljit_emit_jump(compiler, SLJIT_NOT_ZERO);

    if (!SLJIT_IS_REG(shiftIntoArg)) {
        sljit_emit_op1(compiler, SLJIT_MOV, shiftIntoResultReg, 0, shiftIntoArg, shiftIntoArgw);
        shiftIntoArg = shiftIntoResultReg;
    }

#if !(defined SLJIT_MASKED_SHIFT && SLJIT_MASKED_SHIFT)
    sljit_emit_op2(compiler, SLJIT_AND, shiftArg, 0, shiftArg, 0, SLJIT_IMM, 0x1f);
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

static void emitRotate64(sljit_compiler* compiler, sljit_s32 op, Instruction* instr, JITArgPair* args)
{
    sljit_s32 reg1 = GET_TARGET_REG(args[2].arg1, instr->requiredReg(0));
    sljit_s32 reg2 = GET_TARGET_REG(args[2].arg2, instr->requiredReg(1));
    sljit_s32 tmpReg = instr->requiredReg(2);
    sljit_s32 rotateReg = instr->requiredReg(3);

    if (SLJIT_IS_IMM(args[1].arg1)) {
        sljit_sw rotate = (args[1].arg1w & 0x3f);

        if (rotate & 0x20) {
            rotate -= 0x20;
            sljit_emit_op1(compiler, SLJIT_MOV, tmpReg, 0, args[0].arg2, args[0].arg2w);
            MOVE_TO_REG(compiler, SLJIT_MOV, reg2, args[0].arg1, args[0].arg1w);
            sljit_emit_op1(compiler, SLJIT_MOV, reg1, 0, tmpReg, 0);
        } else {
            sljit_emit_op1(compiler, SLJIT_MOV, tmpReg, 0, args[0].arg1, args[0].arg1w);
            MOVE_TO_REG(compiler, SLJIT_MOV, reg2, args[0].arg2, args[0].arg2w);
            sljit_emit_op1(compiler, SLJIT_MOV, reg1, 0, tmpReg, 0);
        }

        sljit_emit_shift_into(compiler, op, reg1, reg1, reg2, SLJIT_IMM, rotate);
        sljit_emit_shift_into(compiler, op, reg2, reg2, tmpReg, SLJIT_IMM, rotate);

        MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg1, args[2].arg1w, reg1);
        MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg2, args[2].arg2w, reg2);
        return;
    }

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
    sljit_emit_op2(compiler, SLJIT_AND, rotateReg, 0, rotateReg, 0, SLJIT_IMM, 0x1f);
#endif /* !SLJIT_MASKED_SHIFT32 */
    sljit_emit_op1(compiler, SLJIT_MOV, tmpReg, 0, reg1, 0);
    sljit_emit_shift_into(compiler, op, reg1, reg1, reg2, rotateReg, 0);
    sljit_emit_shift_into(compiler, op, reg2, reg2, tmpReg, rotateReg, 0);

    MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg1, args[2].arg1w, reg1);
    MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg2, args[2].arg2w, reg2);
}

static void emitMul64(sljit_compiler* compiler, Instruction* instr, JITArgPair* args)
{
    sljit_s32 tmpReg = instr->requiredReg(0);

    if (!SLJIT_IS_IMM(args[1].arg1) && SLJIT_IS_IMM(args[0].arg1)) {
        // Swap arguments.
        JITArgPair tmp = args[0];
        args[0] = args[1];
        args[1] = tmp;
    }

    if (SLJIT_IS_IMM(args[1].arg1)) {
        if (args[1].arg1w == 0) {
            sljit_emit_op2(compiler, SLJIT_MUL, args[2].arg2, args[2].arg2w, args[0].arg1, args[0].arg1w, SLJIT_IMM, args[1].arg2w);
            sljit_emit_op1(compiler, SLJIT_MOV, args[2].arg1, args[2].arg1w, SLJIT_IMM, 0);
            return;
        }

        if (args[1].arg1w == 1) {
            MOVE_TO_REG(compiler, SLJIT_MOV, tmpReg, args[0].arg1, args[0].arg1w);
            if (args[0].arg1 != args[2].arg1 || args[0].arg1w != args[2].arg1w)
                sljit_emit_op1(compiler, SLJIT_MOV, args[2].arg1, args[2].arg1w, tmpReg, 0);

            sljit_emit_op2(compiler, SLJIT_MUL, tmpReg, 0, tmpReg, 0, SLJIT_IMM, args[1].arg2w);
            sljit_emit_op2(compiler, SLJIT_ADD, args[2].arg2, args[2].arg2w, args[0].arg2, args[0].arg2w, tmpReg, 0);
            return;
        }

        sljit_s32 baseReg = SLJIT_R0;
        sljit_s32 immReg = SLJIT_R1;

        if (args[0].arg1 == SLJIT_R1) {
            baseReg = SLJIT_R1;
            immReg = SLJIT_R0;
        } else {
            MOVE_TO_REG(compiler, SLJIT_MOV, baseReg, args[0].arg1, args[0].arg1w);
        }

        if (SLJIT_IS_MEM(args[0].arg2) || args[0].arg2 == immReg) {
            sljit_emit_op1(compiler, SLJIT_MOV, tmpReg, 0, args[0].arg2, args[0].arg2w);
            sljit_emit_op1(compiler, SLJIT_MOV, immReg, 0, SLJIT_IMM, args[1].arg1w);
            sljit_emit_op2(compiler, SLJIT_MUL, tmpReg, 0, tmpReg, 0, immReg, 0);
        } else {
            sljit_emit_op1(compiler, SLJIT_MOV, immReg, 0, SLJIT_IMM, args[1].arg1w);
            sljit_emit_op2(compiler, SLJIT_MUL, tmpReg, 0, args[0].arg2, args[0].arg2w, immReg, 0);
        }

        if (args[1].arg2w == 1) {
            sljit_emit_op2(compiler, SLJIT_ADD, tmpReg, 0, tmpReg, 0, baseReg, 0);
        } else if (args[1].arg2w != 0) {
            sljit_emit_op2r(compiler, SLJIT_MULADD, tmpReg, baseReg, 0, SLJIT_IMM, args[1].arg2w);
        }
    } else {
        if (args[1].arg1 == SLJIT_R0 || args[1].arg2 == SLJIT_R0) {
            // Swap arguments.
            JITArgPair tmp = args[0];
            args[0] = args[1];
            args[1] = tmp;
        }

        sljit_s32 lowReg0 = SLJIT_R0;
        sljit_s32 lowReg1 = SLJIT_R1;
        sljit_s32 firstIndex = 0;

        if (args[0].arg2 == SLJIT_R0) {
            lowReg0 = SLJIT_R1;
            lowReg1 = SLJIT_R0;
        } else {
            MOVE_TO_REG(compiler, SLJIT_MOV, SLJIT_R0, args[0].arg1, args[0].arg1w);
        }

        if (args[1].arg2 == lowReg1) {
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_TMP_DEST_REG, 0, args[1].arg2, args[1].arg2w);
            args[1].arg2 = SLJIT_TMP_DEST_REG;
            args[1].arg2w = 0;
            firstIndex = 1;
        } else if (args[0].arg2 == lowReg1 || SLJIT_IS_MEM(args[0].arg2)) {
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_TMP_DEST_REG, 0, args[0].arg2, args[0].arg2w);
            args[0].arg2 = SLJIT_TMP_DEST_REG;
            args[0].arg2w = 0;
        }

        MOVE_TO_REG(compiler, SLJIT_MOV, lowReg1, args[1].arg1, args[1].arg1w);
        sljit_emit_op2(compiler, SLJIT_MUL, tmpReg, 0, args[firstIndex].arg2, args[firstIndex].arg2w, lowReg1, 0);

        firstIndex ^= 0x1;
        if (args[firstIndex].arg2 != lowReg1) {
            sljit_emit_op2r(compiler, SLJIT_MULADD, tmpReg, args[firstIndex].arg2, args[firstIndex].arg2w, lowReg0, 0);
        } else {
            sljit_emit_op2(compiler, SLJIT_SHL, tmpReg, 0, tmpReg, 0, SLJIT_IMM, 1);
        }
    }

    sljit_emit_op0(compiler, SLJIT_LMUL_UW);
    sljit_emit_op2(compiler, SLJIT_ADD, tmpReg, 0, tmpReg, 0, SLJIT_R1, 0);
    MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg1, args[2].arg1w, SLJIT_R0);
    MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg2, args[2].arg2w, tmpReg);
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
    bool isImm = SLJIT_IS_IMM(args[1].arg1);
    CompileContext* context = CompileContext::get(compiler);
    sljit_sw stackTmpStart = context->stackTmpStart;

    if (isImm) {
        if ((args[1].arg1w | args[1].arg2w) == 0) {
            context->appendTrapJump(ExecutionContext::DivideByZeroError, sljit_emit_jump(compiler, SLJIT_JUMP));
            return;
        }
        if ((args[1].arg1w & args[1].arg2w) == -1 && opcode == SLJIT_DIVMOD_SW) {
            sljit_emit_op1(compiler, SLJIT_MOV, args[2].arg1, args[2].arg1w, SLJIT_IMM, 0);
            sljit_emit_op1(compiler, SLJIT_MOV, args[2].arg2, args[2].arg2w, SLJIT_IMM, 0);
            return;
        }
    }

    if (args[0].arg1 != SLJIT_MEM1(kFrameReg)) {
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), stackTmpStart + WORD_LOW_OFFSET, args[0].arg1, args[0].arg1w);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), stackTmpStart + WORD_HIGH_OFFSET, args[0].arg2, args[0].arg2w);
    }

    if (args[1].arg1 != SLJIT_MEM1(kFrameReg)) {
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), stackTmpStart + 8 + WORD_LOW_OFFSET, args[1].arg1, args[1].arg1w);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), stackTmpStart + 8 + WORD_HIGH_OFFSET, args[1].arg2, args[1].arg2w);
    }

    if (args[0].arg1 == SLJIT_MEM1(kFrameReg)) {
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R0, 0, kFrameReg, 0, SLJIT_IMM, args[0].arg1w - WORD_LOW_OFFSET);
    } else {
        ASSERT(SLJIT_IS_REG(args[0].arg1) || SLJIT_IS_IMM(args[0].arg1));
        sljit_get_local_base(compiler, SLJIT_R0, 0, stackTmpStart);
    }

    if (args[1].arg1 == SLJIT_MEM1(kFrameReg)) {
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R1, 0, kFrameReg, 0, SLJIT_IMM, args[1].arg1w - WORD_LOW_OFFSET);
    } else {
        ASSERT(SLJIT_IS_REG(args[1].arg1) || SLJIT_IS_IMM(args[1].arg1));
        sljit_get_local_base(compiler, SLJIT_R1, 0, stackTmpStart + 8);
    }

    if (args[2].arg1 == SLJIT_MEM1(kFrameReg)) {
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R2, 0, kFrameReg, 0, SLJIT_IMM, args[2].arg1w - WORD_LOW_OFFSET);
    } else {
        ASSERT(SLJIT_IS_REG(args[2].arg1));
        sljit_get_local_base(compiler, SLJIT_R2, 0, stackTmpStart);
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

    sljit_s32 argTypes = isImm ? SLJIT_ARGS3V(W, W, W) : SLJIT_ARGS3(W, W, W, W);
    sljit_emit_icall(compiler, SLJIT_CALL, argTypes, SLJIT_IMM, addr);

    if (!isImm) {
        sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, ExecutionContext::NoError);
        context->appendTrapJump(ExecutionContext::GenericTrap, cmp);
    }

    if (args[2].arg1 != SLJIT_MEM1(kFrameReg)) {
        sljit_emit_op1(compiler, SLJIT_MOV, args[2].arg1, args[2].arg1w, SLJIT_MEM1(SLJIT_SP), stackTmpStart + WORD_LOW_OFFSET);
        sljit_emit_op1(compiler, SLJIT_MOV, args[2].arg2, args[2].arg2w, SLJIT_MEM1(SLJIT_SP), stackTmpStart + WORD_HIGH_OFFSET);
    }
}

static void emitBinary(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();

    if (instr->info() & Instruction::kIs32Bit) {
        JITArg args[3] = { operands, operands + 1, operands + 2 };

        sljit_s32 opcode;

        switch (instr->opcode()) {
        case ByteCode::I32AddOpcode:
            opcode = SLJIT_ADD;
            break;
        case ByteCode::I32SubOpcode:
            opcode = SLJIT_SUB;
            break;
        case ByteCode::I32MulOpcode:
            opcode = SLJIT_MUL;
            break;
        case ByteCode::I32DivSOpcode:
            emitDivRem32(compiler, SLJIT_DIV_SW, args);
            return;
        case ByteCode::I32DivUOpcode:
            emitDivRem32(compiler, SLJIT_DIV_UW, args);
            return;
        case ByteCode::I32RemSOpcode:
            emitDivRem32(compiler, SLJIT_DIVMOD_SW, args);
            return;
        case ByteCode::I32RemUOpcode:
            emitDivRem32(compiler, SLJIT_DIVMOD_UW, args);
            return;
        case ByteCode::I32RotlOpcode:
            opcode = SLJIT_ROTL;
            break;
        case ByteCode::I32RotrOpcode:
            opcode = SLJIT_ROTR;
            break;
        case ByteCode::I32AndOpcode:
            opcode = SLJIT_AND;
            break;
        case ByteCode::I32OrOpcode:
            opcode = SLJIT_OR;
            break;
        case ByteCode::I32XorOpcode:
            opcode = SLJIT_XOR;
            break;
        case ByteCode::I32ShlOpcode:
            opcode = SLJIT_MSHL32;
            break;
        case ByteCode::I32ShrSOpcode:
            opcode = SLJIT_MASHR32;
            break;
        case ByteCode::I32ShrUOpcode:
            opcode = SLJIT_MLSHR32;
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }

        sljit_emit_op2(compiler, opcode, args[2].arg, args[2].argw, args[0].arg, args[0].argw, args[1].arg, args[1].argw);
        return;
    }

    JITArgPair args[3] = { operands, operands + 1, operands + 2 };

    switch (instr->opcode()) {
    case ByteCode::I64AddOpcode:
        emitSimpleBinary64(compiler, SLJIT_ADD | SLJIT_SET_CARRY, SLJIT_ADDC, instr, args);
        return;
    case ByteCode::I64SubOpcode:
        emitSimpleBinary64(compiler, SLJIT_SUB | SLJIT_SET_CARRY, SLJIT_SUBC, instr, args);
        return;
    case ByteCode::I64MulOpcode:
        emitMul64(compiler, instr, args);
        return;
    case ByteCode::I64DivSOpcode:
        emitDivRem64(compiler, SLJIT_DIV_SW, args);
        return;
    case ByteCode::I64DivUOpcode:
        emitDivRem64(compiler, SLJIT_DIV_UW, args);
        return;
    case ByteCode::I64RemSOpcode:
        emitDivRem64(compiler, SLJIT_DIVMOD_SW, args);
        return;
    case ByteCode::I64RemUOpcode:
        emitDivRem64(compiler, SLJIT_DIVMOD_UW, args);
        return;
    case ByteCode::I64RotlOpcode:
        emitRotate64(compiler, SLJIT_SHL, instr, args);
        return;
    case ByteCode::I64RotrOpcode:
        emitRotate64(compiler, SLJIT_LSHR, instr, args);
        return;
    case ByteCode::I64AndOpcode:
        emitSimpleBinary64(compiler, SLJIT_AND, SLJIT_AND, instr, args);
        return;
    case ByteCode::I64OrOpcode:
        emitSimpleBinary64(compiler, SLJIT_OR, SLJIT_OR, instr, args);
        return;
    case ByteCode::I64XorOpcode:
        emitSimpleBinary64(compiler, SLJIT_XOR, SLJIT_XOR, instr, args);
        return;
    case ByteCode::I64ShlOpcode:
        emitShift64(compiler, SLJIT_SHL, instr, args);
        return;
    case ByteCode::I64ShrSOpcode:
        emitShift64(compiler, SLJIT_ASHR, instr, args);
        return;
    case ByteCode::I64ShrUOpcode:
        emitShift64(compiler, SLJIT_LSHR, instr, args);
        return;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
}

static void emitCountZeroes(sljit_compiler* compiler, sljit_s32 op, Instruction* instr, JITArgPair* args)
{
    // The high register of the 64 bit result is used as temporary value.
    sljit_s32 firstReg;
    sljit_s32 secondReg;

    if (op == SLJIT_CLZ) {
        firstReg = instr->requiredReg(1);
        secondReg = instr->requiredReg(0);
        MOVE_TO_REG(compiler, SLJIT_MOV, firstReg, args[0].arg2, args[0].arg2w);
    } else {
        firstReg = instr->requiredReg(0);
        secondReg = instr->requiredReg(1);
        MOVE_TO_REG(compiler, SLJIT_MOV, firstReg, args[0].arg1, args[0].arg1w);
    }

    sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_Z, firstReg, 0, SLJIT_IMM, 0);

    if (op == SLJIT_CLZ) {
        sljit_emit_select(compiler, SLJIT_EQUAL, firstReg, args[0].arg1, args[0].arg1w, firstReg);
    } else {
        sljit_emit_select(compiler, SLJIT_EQUAL, firstReg, args[0].arg2, args[0].arg2w, firstReg);
    }

    sljit_emit_op_flags(compiler, SLJIT_MOV, secondReg, 0, SLJIT_EQUAL);
    sljit_emit_op2(compiler, SLJIT_SHL, secondReg, 0, secondReg, 0, SLJIT_IMM, 5);

    sljit_emit_op1(compiler, op, firstReg, 0, firstReg, 0);

    firstReg = instr->requiredReg(0);
    sljit_emit_op2(compiler, SLJIT_ADD, firstReg, 0, firstReg, 0, instr->requiredReg(1), 0);

    MOVE_FROM_REG(compiler, SLJIT_MOV, args[1].arg1, args[1].arg1w, firstReg);
    sljit_emit_op1(compiler, SLJIT_MOV, args[1].arg2, args[1].arg2w, SLJIT_IMM, 0);
}

static sljit_sw popcnt32(sljit_sw arg)
{
    return popCount((unsigned)arg);
}

static void emitPopcnt(sljit_compiler* compiler, JITArg* args)
{
    MOVE_TO_REG(compiler, SLJIT_MOV, SLJIT_R0, args[0].arg, args[0].argw);

    sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS1(W, W), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, popcnt32));

    MOVE_FROM_REG(compiler, SLJIT_MOV, args[1].arg, args[1].argw, SLJIT_R0);
}

static sljit_sw popcnt64(sljit_sw arg1, sljit_sw arg2)
{
    return popCount((unsigned)arg1) + popCount((unsigned)arg2);
}

static void emitPopcnt64(sljit_compiler* compiler, JITArgPair* args)
{
    MOVE_TO_REG(compiler, SLJIT_MOV, SLJIT_R0, args[0].arg1, args[0].arg1w);
    MOVE_TO_REG(compiler, SLJIT_MOV, SLJIT_R1, args[0].arg2, args[0].arg2w);

    sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS2(W, W, W), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, popcnt64));

    MOVE_FROM_REG(compiler, SLJIT_MOV, args[1].arg1, args[1].arg1w, SLJIT_R0);
    sljit_emit_op1(compiler, SLJIT_MOV, args[1].arg2, args[1].arg2w, SLJIT_IMM, 0);
}

static void emitExtend(sljit_compiler* compiler, sljit_s32 opcode, Instruction* instr, JITArg* args)
{
    sljit_s32 resultReg = GET_TARGET_REG(args[1].arg, SLJIT_TMP_DEST_REG);

#if (defined SLJIT_BIG_ENDIAN && SLJIT_BIG_ENDIAN)
    if (SLJIT_IS_MEM(args[0].arg)) {
        args[0].argw += (opcode == SLJIT_MOV_S8) ? 3 : 2;
    }
#endif /* SLJIT_BIG_ENDIAN */

    sljit_emit_op1(compiler, opcode, resultReg, 0, args[0].arg, args[0].argw);
    MOVE_FROM_REG(compiler, SLJIT_MOV, args[1].arg, args[1].argw, resultReg);
}

static void emitExtend64(sljit_compiler* compiler, sljit_s32 opcode, Instruction* instr, JITArgPair* args)
{
    sljit_s32 resultReg = GET_TARGET_REG(args[1].arg1, SLJIT_TMP_DEST_REG);

#if (defined SLJIT_BIG_ENDIAN && SLJIT_BIG_ENDIAN)
    if (opcode != SLJIT_MOV && SLJIT_IS_MEM(args[0].arg1)) {
        args[0].arg1w += (opcode == SLJIT_MOV_S8) ? 3 : 2;
    }
#endif /* SLJIT_BIG_ENDIAN */

    sljit_emit_op1(compiler, opcode, resultReg, 0, args[0].arg1, args[0].arg1w);
    MOVE_FROM_REG(compiler, SLJIT_MOV, args[1].arg1, args[1].arg1w, resultReg);

    if (SLJIT_IS_REG(args[1].arg2)) {
        sljit_emit_op2(compiler, SLJIT_ASHR, args[1].arg2, args[1].arg2w, resultReg, 0, SLJIT_IMM, 31);
        return;
    }

    sljit_emit_op2(compiler, SLJIT_ASHR, resultReg, 0, resultReg, 0, SLJIT_IMM, 31);
    sljit_emit_op1(compiler, SLJIT_MOV, args[1].arg2, args[1].arg2w, resultReg, 0);
}

static void emitUnary(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();

    if (instr->info() & Instruction::kIs32Bit) {
        JITArg args[2] = { operands, operands + 1 };

        sljit_s32 opcode;

        switch (instr->opcode()) {
        case ByteCode::I32ClzOpcode:
            if (SLJIT_IS_IMM(args[0].arg)) {
                sljit_emit_op1(compiler, SLJIT_MOV, args[1].arg, args[1].argw, SLJIT_IMM, clz((sljit_u32)args[0].argw));
                return;
            }
            opcode = SLJIT_CLZ;
            break;
        case ByteCode::I32CtzOpcode:
            if (SLJIT_IS_IMM(args[0].arg)) {
                sljit_emit_op1(compiler, SLJIT_MOV, args[1].arg, args[1].argw, SLJIT_IMM, ctz((sljit_u32)args[0].argw));
                return;
            }
            opcode = SLJIT_CTZ;
            break;
        case ByteCode::I32PopcntOpcode:
            emitPopcnt(compiler, args);
            return;
        case ByteCode::I32Extend8SOpcode:
            emitExtend(compiler, SLJIT_MOV_S8, instr, args);
            return;
        case ByteCode::I32Extend16SOpcode:
            emitExtend(compiler, SLJIT_MOV_S16, instr, args);
            return;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }

        sljit_emit_op1(compiler, opcode, args[1].arg, args[1].argw, args[0].arg, args[0].argw);
        return;
    }

    JITArgPair args[2] = { operands, operands + 1 };

    switch (instr->opcode()) {
    case ByteCode::I64ClzOpcode:
        emitCountZeroes(compiler, SLJIT_CLZ, instr, args);
        return;
    case ByteCode::I64CtzOpcode:
        emitCountZeroes(compiler, SLJIT_CTZ, instr, args);
        return;
    case ByteCode::I64PopcntOpcode:
        emitPopcnt64(compiler, args);
        return;
    case ByteCode::I64Extend8SOpcode:
        emitExtend64(compiler, SLJIT_MOV_S8, instr, args);
        return;
    case ByteCode::I64Extend16SOpcode:
        emitExtend64(compiler, SLJIT_MOV_S16, instr, args);
        return;
    case ByteCode::I64Extend32SOpcode:
        if (args[0].arg1 == args[1].arg1 && args[0].arg1w == args[1].arg1w) {
            sljit_emit_op2(compiler, SLJIT_ASHR, args[1].arg2, args[1].arg2w, args[0].arg1, args[0].arg1w, SLJIT_IMM, 31);
            return;
        }
        emitExtend64(compiler, SLJIT_MOV, instr, args);
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
    ASSERT(instr->opcode() == ByteCode::SelectOpcode && instr->paramCount() == 3);

    Select* select = reinterpret_cast<Select*>(instr->byteCode());

    if (select->isFloat()) {
        return emitFloatSelect(compiler, instr, type);
    }

    if (select->valueSize() == 16) {
#ifdef HAS_SIMD
        return emitSelect128(compiler, instr, type);
#else /* HAS_SIMD */
        RELEASE_ASSERT_NOT_REACHED();
#endif /* HAS_SIMD */
    }

    if (select->valueSize() == 4) {
        JITArg args[3] = { operands, operands + 1, operands + 3 };

        if (type == -1) {
            cond.set(operands + 2);
            sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_Z, cond.arg, cond.argw, SLJIT_IMM, 0);

            type = SLJIT_NOT_ZERO;
        }

        sljit_s32 targetReg = GET_TARGET_REG(args[2].arg, SLJIT_TMP_DEST_REG);
        sljit_s32 baseReg = 0;

        if (args[1].arg == targetReg || (SLJIT_IS_IMM(args[1].arg) && !SLJIT_IS_IMM(args[0].arg) && args[0].arg != targetReg)) {
            baseReg = 1;
        } else {
            type ^= 1;
        }

        if (!SLJIT_IS_REG(args[baseReg].arg)) {
            sljit_emit_op1(compiler, SLJIT_MOV, targetReg, 0, args[baseReg].arg, args[baseReg].argw);
            args[baseReg].arg = targetReg;
        }

        sljit_s32 otherReg = baseReg ^ 0x1;
        sljit_emit_select(compiler, type, targetReg, args[otherReg].arg, args[otherReg].argw, args[baseReg].arg);
        MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg, args[2].argw, targetReg);
        return;
    }

    JITArgPair args[3] = { operands, operands + 1, operands + 3 };

    if (type == -1) {
        cond.set(operands + 2);
        sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_Z, cond.arg, cond.argw, SLJIT_IMM, 0);

        type = SLJIT_NOT_ZERO;
    }

    sljit_s32 targetReg1 = GET_TARGET_REG(args[2].arg1, instr->requiredReg(0));
    sljit_s32 targetReg2 = GET_TARGET_REG(args[2].arg2, SLJIT_TMP_DEST_REG);
    sljit_s32 baseReg = 0;

    ASSERT(args[0].arg1 != targetReg2 && args[0].arg2 != targetReg1
           && args[1].arg1 != targetReg2 && args[1].arg2 != targetReg1);

    if (args[1].arg1 == targetReg1 || (SLJIT_IS_IMM(args[1].arg1) && !SLJIT_IS_IMM(args[0].arg1) && args[0].arg1 != targetReg1)) {
        baseReg = 1;
    } else {
        type ^= 1;
    }

    if (!sljit_has_cpu_feature(SLJIT_HAS_CMOV)) {
        MOVE_TO_REG(compiler, SLJIT_MOV, targetReg1, args[baseReg].arg1, args[baseReg].arg1w);
        MOVE_TO_REG(compiler, SLJIT_MOV, targetReg2, args[baseReg].arg2, args[baseReg].arg2w);

        sljit_jump* jump = sljit_emit_jump(compiler, type ^ 0x1);
        sljit_s32 otherReg = baseReg ^ 0x1;

        MOVE_TO_REG(compiler, SLJIT_MOV, targetReg1, args[otherReg].arg1, args[otherReg].arg1w);
        MOVE_TO_REG(compiler, SLJIT_MOV, targetReg2, args[otherReg].arg2, args[otherReg].arg2w);
        sljit_set_label(jump, sljit_emit_label(compiler));
        MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg1, args[2].arg1w, targetReg1);
        MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg2, args[2].arg2w, targetReg2);
        return;
    }

    ASSERT(SLJIT_IS_REG(args[baseReg].arg1) == SLJIT_IS_REG(args[baseReg].arg2));

    if (!SLJIT_IS_REG(args[baseReg].arg1)) {
        sljit_emit_op1(compiler, SLJIT_MOV, targetReg1, 0, args[baseReg].arg1, args[baseReg].arg1w);
        sljit_emit_op1(compiler, SLJIT_MOV, targetReg2, 0, args[baseReg].arg2, args[baseReg].arg2w);
        args[baseReg].arg1 = targetReg1;
        args[baseReg].arg2 = targetReg2;
    }

    sljit_s32 otherReg = baseReg ^ 0x1;
    sljit_emit_select(compiler, type, targetReg1, args[otherReg].arg1, args[otherReg].arg1w, args[baseReg].arg1);
    sljit_emit_select(compiler, type, targetReg2, args[otherReg].arg2, args[otherReg].arg2w, args[baseReg].arg2);
    MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg1, args[2].arg1w, targetReg1);
    MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg2, args[2].arg2w, targetReg2);
}

static bool emitCompare(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operand = instr->operands();
    Instruction* nextInstr = nullptr;
    bool isBranch = false;

    ASSERT(instr->next() != nullptr);

    if (instr->info() & Instruction::kIsMergeCompare) {
        nextInstr = instr->next()->asInstruction();

        ASSERT(nextInstr->opcode() == ByteCode::SelectOpcode
               || nextInstr->opcode() == ByteCode::JumpIfTrueOpcode || nextInstr->opcode() == ByteCode::JumpIfFalseOpcode);

        isBranch = (nextInstr->opcode() != ByteCode::SelectOpcode);
    }

    if (instr->info() & Instruction::kIs32Bit) {
        sljit_s32 opcode, type;

        switch (instr->opcode()) {
        case ByteCode::I32EqzOpcode:
            opcode = SLJIT_SUB | SLJIT_SET_Z;
            type = SLJIT_EQUAL;
            break;
        case ByteCode::I32EqOpcode:
            opcode = SLJIT_SUB | SLJIT_SET_Z;
            type = SLJIT_EQUAL;
            break;
        case ByteCode::I32NeOpcode:
            opcode = SLJIT_SUB | SLJIT_SET_Z;
            type = SLJIT_NOT_EQUAL;
            break;
        case ByteCode::I32LtSOpcode:
            opcode = SLJIT_SUB | SLJIT_SET_SIG_LESS;
            type = SLJIT_SIG_LESS;
            break;
        case ByteCode::I32LtUOpcode:
            opcode = SLJIT_SUB | SLJIT_SET_LESS;
            type = SLJIT_LESS;
            break;
        case ByteCode::I32GtSOpcode:
            opcode = SLJIT_SUB | SLJIT_SET_SIG_GREATER;
            type = SLJIT_SIG_GREATER;
            break;
        case ByteCode::I32GtUOpcode:
            opcode = SLJIT_SUB | SLJIT_SET_GREATER;
            type = SLJIT_GREATER;
            break;
        case ByteCode::I32LeSOpcode:
            opcode = SLJIT_SUB | SLJIT_SET_SIG_LESS_EQUAL;
            type = SLJIT_SIG_LESS_EQUAL;
            break;
        case ByteCode::I32LeUOpcode:
            opcode = SLJIT_SUB | SLJIT_SET_LESS_EQUAL;
            type = SLJIT_LESS_EQUAL;
            break;
        case ByteCode::I32GeSOpcode:
            opcode = SLJIT_SUB | SLJIT_SET_SIG_GREATER_EQUAL;
            type = SLJIT_SIG_GREATER_EQUAL;
            break;
        case ByteCode::I32GeUOpcode:
            opcode = SLJIT_SUB | SLJIT_SET_GREATER_EQUAL;
            type = SLJIT_GREATER_EQUAL;
            break;
        case ByteCode::I32AndOpcode:
            opcode = SLJIT_AND | SLJIT_SET_Z;
            type = SLJIT_NOT_ZERO;
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }

        JITArg params[2];

        for (uint32_t i = 0; i < instr->paramCount(); ++i) {
            params[i].set(operand);
            operand++;
        }

        if (instr->opcode() == ByteCode::I32EqzOpcode) {
            params[1].arg = SLJIT_IMM;
            params[1].argw = 0;
        }

        if (isBranch) {
            if (nextInstr->opcode() == ByteCode::JumpIfFalseOpcode) {
                type ^= 0x1;
            }

            sljit_jump* jump;

            if (instr->opcode() != ByteCode::I32AndOpcode && (*operand == VARIABLE_SET_PTR(nullptr))) {
                jump = sljit_emit_cmp(compiler, type, params[0].arg, params[0].argw, params[1].arg, params[1].argw);
            } else {
                sljit_emit_op2u(compiler, opcode, params[0].arg, params[0].argw, params[1].arg, params[1].argw);

                if (*operand != VARIABLE_SET_PTR(nullptr)) {
                    JITArg result(operand);
                    sljit_emit_op_flags(compiler, SLJIT_MOV, result.arg, result.argw, type);
                }

                jump = sljit_emit_jump(compiler, type);
            }

            nextInstr->asExtended()->value().targetLabel->jumpFrom(jump);
            return true;
        }

        sljit_emit_op2u(compiler, opcode, params[0].arg, params[0].argw, params[1].arg, params[1].argw);

        if (*operand != VARIABLE_SET_PTR(nullptr)) {
            JITArg result(operand);
            sljit_emit_op_flags(compiler, SLJIT_MOV, result.arg, result.argw, type);
        }

        if (nextInstr != nullptr) {
            emitSelect(compiler, nextInstr, type);
            return true;
        }

        return false;
    }

    sljit_s32 type = SLJIT_EQUAL;
    sljit_s32 firstIndex = 0;
#ifdef SLJIT_SHARED_COMPARISON_FLAGS
    sljit_s32 signedCompare = 0;
    sljit_s32 unsignedCompare = 0;
#endif /* SLJIT_SHARED_COMPARISON_FLAGS */

    switch (instr->opcode()) {
    case ByteCode::I64EqzOpcode:
    case ByteCode::I64EqOpcode:
        break;
    case ByteCode::I64NeOpcode:
        type = SLJIT_NOT_EQUAL;
        break;
    case ByteCode::I64LtSOpcode:
#ifdef SLJIT_SHARED_COMPARISON_FLAGS
        signedCompare = SLJIT_SUB | SLJIT_SET_SIG_LESS;
        unsignedCompare = SLJIT_SUB | SLJIT_SET_LESS;
#endif /* SLJIT_SHARED_COMPARISON_FLAGS */
        type = SLJIT_SIG_LESS;
        break;
    case ByteCode::I64LtUOpcode:
        type = SLJIT_CARRY;
        break;
    case ByteCode::I64GtSOpcode:
#ifdef SLJIT_SHARED_COMPARISON_FLAGS
        signedCompare = SLJIT_SUB | SLJIT_SET_SIG_LESS;
        unsignedCompare = SLJIT_SUB | SLJIT_SET_LESS;
#endif /* SLJIT_SHARED_COMPARISON_FLAGS */
        type = SLJIT_SIG_LESS;
        firstIndex = 1;
        break;
    case ByteCode::I64GtUOpcode:
        type = SLJIT_CARRY;
        firstIndex = 1;
        break;
    case ByteCode::I64LeSOpcode:
#ifdef SLJIT_SHARED_COMPARISON_FLAGS
        signedCompare = SLJIT_SUB | SLJIT_SET_SIG_GREATER_EQUAL;
        unsignedCompare = SLJIT_SUB | SLJIT_SET_GREATER_EQUAL;
#endif /* SLJIT_SHARED_COMPARISON_FLAGS */
        type = SLJIT_SIG_GREATER_EQUAL;
        firstIndex = 1;
        break;
    case ByteCode::I64LeUOpcode:
        type = SLJIT_NOT_CARRY;
        firstIndex = 1;
        break;
    case ByteCode::I64GeSOpcode:
#ifdef SLJIT_SHARED_COMPARISON_FLAGS
        signedCompare = SLJIT_SUB | SLJIT_SET_SIG_GREATER_EQUAL;
        unsignedCompare = SLJIT_SUB | SLJIT_SET_GREATER_EQUAL;
#endif /* SLJIT_SHARED_COMPARISON_FLAGS */
        type = SLJIT_SIG_GREATER_EQUAL;
        break;
    case ByteCode::I64GeUOpcode:
        type = SLJIT_NOT_CARRY;
        break;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }

    JITArgPair params[2];

    if (instr->opcode() == ByteCode::I64EqzOpcode) {
        ASSERT(type == SLJIT_EQUAL);
        params[0].set(operand);
        operand++;
        sljit_emit_op2u(compiler, SLJIT_OR | SLJIT_SET_Z, params[0].arg1, params[0].arg1w, params[0].arg2, params[0].arg2w);
    } else if (instr->opcode() == ByteCode::I64EqOpcode || instr->opcode() == ByteCode::I64NeOpcode) {
        params[0].set(operand);
        params[1].set(operand + 1);
        operand += 2;

        ASSERT(type == SLJIT_EQUAL || type == SLJIT_NOT_EQUAL);

        sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_Z, params[0].arg2, params[0].arg2w, params[1].arg2, params[1].arg2w);
        sljit_jump* jump = sljit_emit_jump(compiler, SLJIT_NOT_ZERO);
        sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_Z, params[0].arg1, params[0].arg1w, params[1].arg1, params[1].arg1w);
        sljit_set_label(jump, sljit_emit_label(compiler));
        sljit_set_current_flags(compiler, SLJIT_SET_Z | SLJIT_CURRENT_FLAGS_SUB | SLJIT_CURRENT_FLAGS_COMPARE);
    } else {
        params[firstIndex].set(operand);
        params[1 - firstIndex].set(operand + 1);
        operand += 2;

        ASSERT(type == SLJIT_SIG_LESS || type == SLJIT_SIG_GREATER_EQUAL
               || type == SLJIT_CARRY || type == SLJIT_NOT_CARRY);

#ifdef SLJIT_SUBC_SETS_SIGNED
        sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_CARRY, params[0].arg1, params[0].arg1w, params[1].arg1, params[1].arg1w);
        sljit_emit_op2u(compiler, SLJIT_SUBC | SLJIT_SET_CARRY, params[0].arg2, params[0].arg2w, params[1].arg2, params[1].arg2w);

        if (type == SLJIT_SIG_LESS || type == SLJIT_SIG_GREATER_EQUAL) {
            sljit_set_current_flags(compiler, SLJIT_SET_SIG_LESS | SLJIT_CURRENT_FLAGS_SUB);
        }
#elif (defined SLJIT_SHARED_COMPARISON_FLAGS)
        if (type < SLJIT_CARRY) {
            sljit_emit_op2u(compiler, signedCompare | SLJIT_SET_Z, params[0].arg2, params[0].arg2w, params[1].arg2, params[1].arg2w);
            sljit_jump* jump = sljit_emit_jump(compiler, SLJIT_NOT_ZERO);
            sljit_emit_op2u(compiler, unsignedCompare, params[0].arg1, params[0].arg1w, params[1].arg1, params[1].arg1w);
            sljit_set_label(jump, sljit_emit_label(compiler));
            sljit_set_current_flags(compiler, (signedCompare - SLJIT_SUB) | SLJIT_CURRENT_FLAGS_SUB);
        } else {
            ASSERT(type == SLJIT_CARRY || type == SLJIT_NOT_CARRY);
            sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_CARRY, params[0].arg2, params[0].arg2w, params[1].arg2, params[1].arg2w);
            sljit_emit_op2u(compiler, SLJIT_SUBC | SLJIT_SET_CARRY, params[0].arg2, params[0].arg2w, params[1].arg2, params[1].arg2w);
        }
#else
#error "Implementation required"
#endif
    }

    ASSERT(operand == instr->operands() + instr->paramCount());
    if (*operand != VARIABLE_SET_PTR(nullptr)) {
        JITArg result(operand);
        sljit_emit_op_flags(compiler, SLJIT_MOV32, result.arg, result.argw, type);
    }

    if (nextInstr != nullptr) {
        if (!isBranch) {
            emitSelect(compiler, nextInstr, type);
            return true;
        }

        if (nextInstr->opcode() == ByteCode::JumpIfFalseOpcode) {
            type ^= 0x1;
        }

        sljit_jump* jump = sljit_emit_jump(compiler, type);
        nextInstr->asExtended()->value().targetLabel->jumpFrom(jump);
        return true;
    }

    return false;
}

static void emitConvert(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operand = instr->operands();

    switch (instr->opcode()) {
    case ByteCode::I32WrapI64Opcode: {
        /* Just copy the lower word. */
        JITArgPair param(operand);
        JITArg result(operand + 1);

        sljit_emit_op1(compiler, SLJIT_MOV, result.arg, result.argw, param.arg1, param.arg1w);
        return;
    }
    case ByteCode::I64ExtendI32SOpcode: {
        JITArg param(operand);
        JITArgPair result(operand + 1);

        if (SLJIT_IS_IMM(param.arg)) {
            sljit_emit_op1(compiler, SLJIT_MOV, result.arg1, result.arg1w, SLJIT_IMM, param.argw);
            sljit_emit_op1(compiler, SLJIT_MOV, result.arg2, result.arg2w, SLJIT_IMM, param.argw >> 31);
            return;
        }

        sljit_s32 resultReg = GET_SOURCE_REG(result.arg1, SLJIT_TMP_DEST_REG);
        MOVE_TO_REG(compiler, SLJIT_MOV, resultReg, param.arg, param.argw);
        MOVE_FROM_REG(compiler, SLJIT_MOV, result.arg1, result.arg1w, resultReg);

        if (SLJIT_IS_REG(result.arg2)) {
            sljit_emit_op2(compiler, SLJIT_ASHR, result.arg2, result.arg2w, resultReg, 0, SLJIT_IMM, 31);
            return;
        }

        sljit_emit_op2(compiler, SLJIT_ASHR, resultReg, 0, resultReg, 0, SLJIT_IMM, 31);
        sljit_emit_op1(compiler, SLJIT_MOV, result.arg2, result.arg2w, resultReg, 0);
        return;
    }
    case ByteCode::I64ExtendI32UOpcode: {
        JITArg param(operand);
        JITArgPair result(operand + 1);

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

static void emitMoveI64(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArgPair src(operands);
    JITArgPair dst(operands + 1);

    sljit_emit_op1(compiler, SLJIT_MOV, dst.arg1, dst.arg1w, src.arg1, src.arg1w);
    sljit_emit_op1(compiler, SLJIT_MOV, dst.arg2, dst.arg2w, src.arg2, src.arg2w);
}

static void emitGlobalGet64(sljit_compiler* compiler, Instruction* instr)
{
    CompileContext* context = CompileContext::get(compiler);
    GlobalGet64* globalGet = reinterpret_cast<GlobalGet64*>(instr->byteCode());
    sljit_s32 baseReg = (instr->info() & Instruction::kHasFloatOperand) ? SLJIT_TMP_DEST_REG : instr->requiredReg(0);

    sljit_emit_op1(compiler, SLJIT_MOV, baseReg, 0, SLJIT_MEM1(kInstanceReg), context->globalsStart + globalGet->index() * sizeof(void*));

    if (instr->info() & Instruction::kHasFloatOperand) {
        JITArg dstArg(instr->operands());
        moveFloatToDest(compiler, SLJIT_MOV_F64, dstArg, JITFieldAccessor::globalValueOffset());
        return;
    }

    JITArgPair dstArg(instr->operands());

    if (SLJIT_IS_REG(dstArg.arg1)) {
        SLJIT_ASSERT(dstArg.arg1 == baseReg);
        sljit_emit_op1(compiler, SLJIT_MOV, dstArg.arg2, dstArg.arg2w, SLJIT_MEM1(baseReg), JITFieldAccessor::globalValueOffset() + WORD_HIGH_OFFSET);
        sljit_emit_op1(compiler, SLJIT_MOV, dstArg.arg1, dstArg.arg1w, SLJIT_MEM1(baseReg), JITFieldAccessor::globalValueOffset() + WORD_LOW_OFFSET);
        return;
    }

    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(baseReg), JITFieldAccessor::globalValueOffset() + WORD_LOW_OFFSET);
    sljit_emit_op1(compiler, SLJIT_MOV, baseReg, 0, SLJIT_MEM1(baseReg), JITFieldAccessor::globalValueOffset() + WORD_HIGH_OFFSET);
    sljit_emit_op1(compiler, SLJIT_MOV, dstArg.arg1, dstArg.arg1w, SLJIT_TMP_DEST_REG, 0);
    sljit_emit_op1(compiler, SLJIT_MOV, dstArg.arg2, dstArg.arg2w, baseReg, 0);
}

static void emitGlobalSet64(sljit_compiler* compiler, Instruction* instr)
{
    CompileContext* context = CompileContext::get(compiler);

    GlobalSet32* globalSet = reinterpret_cast<GlobalSet32*>(instr->byteCode());

    if (instr->info() & Instruction::kHasFloatOperand) {
        JITArg src;
        floatOperandToArg(compiler, instr->operands(), src, SLJIT_TMP_DEST_FREG);
        sljit_s32 baseReg = SLJIT_TMP_DEST_REG;

        sljit_emit_op1(compiler, SLJIT_MOV, baseReg, 0, SLJIT_MEM1(kInstanceReg), context->globalsStart + globalSet->index() * sizeof(void*));

        if (SLJIT_IS_MEM(src.arg)) {
            sljit_emit_fop1(compiler, SLJIT_MOV_F64, SLJIT_TMP_DEST_FREG, 0, src.arg, src.argw);
            src.arg = SLJIT_TMP_DEST_FREG;
            src.argw = 0;
        }

        sljit_emit_fop1(compiler, SLJIT_MOV_F64, SLJIT_MEM1(baseReg), JITFieldAccessor::globalValueOffset(), src.arg, src.argw);
        return;
    }

    JITArgPair src(instr->operands());
    sljit_s32 baseReg = instr->requiredReg(0);

    sljit_emit_op1(compiler, SLJIT_MOV, baseReg, 0, SLJIT_MEM1(kInstanceReg), context->globalsStart + globalSet->index() * sizeof(void*));

    if (SLJIT_IS_MEM(src.arg1)) {
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_TMP_DEST_REG, 0, src.arg1, src.arg1w);
        src.arg1 = SLJIT_TMP_DEST_REG;
        src.arg1w = 0;
    }

    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(baseReg), JITFieldAccessor::globalValueOffset() + WORD_LOW_OFFSET, src.arg1, src.arg1w);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(baseReg), JITFieldAccessor::globalValueOffset() + WORD_HIGH_OFFSET, src.arg2, src.arg2w);
}
