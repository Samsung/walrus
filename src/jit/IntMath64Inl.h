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

static void emitStoreImmediate(sljit_compiler* compiler, Operand* result, Instruction* instr)
{
    sljit_sw offset = static_cast<sljit_sw>(result->offset << 2);

    switch (instr->opcode()) {
#ifdef HAS_SIMD
    case ByteCode::Const128Opcode: {
        const uint8_t* value = reinterpret_cast<Const128*>(instr->byteCode())->value();

        sljit_emit_simd_mem(compiler, SLJIT_SIMD_MEM_LOAD | SLJIT_SIMD_MEM_REG_128 | SLJIT_SIMD_MEM_ELEM_128, SLJIT_FR0, SLJIT_MEM0(), (sljit_sw)value);
        sljit_emit_simd_mem(compiler, SLJIT_SIMD_MEM_STORE | SLJIT_SIMD_MEM_REG_128 | SLJIT_SIMD_MEM_ELEM_128, SLJIT_FR0, SLJIT_MEM1(kFrameReg), offset);
        return;
    }
#endif /* HAS_SIMD */
    case ByteCode::Const32Opcode: {
        uint32_t value32 = reinterpret_cast<Const32*>(instr->byteCode())->value();
        sljit_emit_op1(compiler, SLJIT_MOV32, SLJIT_MEM1(kFrameReg), offset, SLJIT_IMM, static_cast<sljit_s32>(value32));
        return;
    }
    default: {
        ASSERT(instr->opcode() == ByteCode::Const64Opcode);

        uint64_t value64 = reinterpret_cast<Const64*>(instr->byteCode())->value();
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(kFrameReg), offset, SLJIT_IMM, static_cast<sljit_sw>(value64));
        return;
    }
    }
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

            sljit_jump* cmp = sljit_emit_cmp(compiler, type, SLJIT_R0, 0, SLJIT_IMM, min);
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
    JITArg args[3] = { operands, operands + 1, operands + 2 };

    sljit_s32 opcode;

    switch (instr->opcode()) {
    case ByteCode::I32AddOpcode:
        opcode = SLJIT_ADD32;
        break;
    case ByteCode::I32SubOpcode:
        opcode = SLJIT_SUB32;
        break;
    case ByteCode::I32MulOpcode:
        opcode = SLJIT_MUL32;
        break;
    case ByteCode::I32DivSOpcode:
        emitDivRem(compiler, SLJIT_DIV_S32, args, DivRem32 | DivRemSigned);
        return;
    case ByteCode::I32DivUOpcode:
        emitDivRem(compiler, SLJIT_DIV_U32, args, DivRem32);
        return;
    case ByteCode::I32RemSOpcode:
        emitDivRem(compiler, SLJIT_DIVMOD_S32, args, DivRem32 | DivRemSigned | DivRemRemainder);
        return;
    case ByteCode::I32RemUOpcode:
        emitDivRem(compiler, SLJIT_DIVMOD_U32, args, DivRem32 | DivRemRemainder);
        return;
    case ByteCode::I32RotlOpcode:
        opcode = SLJIT_ROTL32;
        break;
    case ByteCode::I32RotrOpcode:
        opcode = SLJIT_ROTR32;
        break;
    case ByteCode::I32AndOpcode:
        opcode = SLJIT_AND32;
        break;
    case ByteCode::I32OrOpcode:
        opcode = SLJIT_OR32;
        break;
    case ByteCode::I32XorOpcode:
        opcode = SLJIT_XOR32;
        break;
    case ByteCode::I32ShlOpcode:
        opcode = SLJIT_SHL32;
        break;
    case ByteCode::I32ShrSOpcode:
        opcode = SLJIT_ASHR32;
        break;
    case ByteCode::I32ShrUOpcode:
        opcode = SLJIT_LSHR32;
        break;
    case ByteCode::I64AddOpcode:
        opcode = SLJIT_ADD;
        break;
    case ByteCode::I64SubOpcode:
        opcode = SLJIT_SUB;
        break;
    case ByteCode::I64MulOpcode:
        opcode = SLJIT_MUL;
        break;
    case ByteCode::I64DivSOpcode:
        emitDivRem(compiler, SLJIT_DIV_SW, args, DivRemSigned);
        return;
    case ByteCode::I64DivUOpcode:
        emitDivRem(compiler, SLJIT_DIV_UW, args, 0);
        return;
    case ByteCode::I64RemSOpcode:
        emitDivRem(compiler, SLJIT_DIVMOD_SW, args, DivRemSigned | DivRemRemainder);
        return;
    case ByteCode::I64RemUOpcode:
        emitDivRem(compiler, SLJIT_DIVMOD_UW, args, DivRemRemainder);
        return;
    case ByteCode::I64RotlOpcode:
        opcode = SLJIT_ROTL;
        break;
    case ByteCode::I64RotrOpcode:
        opcode = SLJIT_ROTR;
        break;
    case ByteCode::I64AndOpcode:
        opcode = SLJIT_AND;
        break;
    case ByteCode::I64OrOpcode:
        opcode = SLJIT_OR;
        break;
    case ByteCode::I64XorOpcode:
        opcode = SLJIT_XOR;
        break;
    case ByteCode::I64ShlOpcode:
        opcode = SLJIT_SHL;
        break;
    case ByteCode::I64ShrSOpcode:
        opcode = SLJIT_ASHR;
        break;
    case ByteCode::I64ShrUOpcode:
        opcode = SLJIT_LSHR;
        break;
    default:
        RELEASE_ASSERT_NOT_REACHED();
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

static void emitPopcnt(sljit_compiler* compiler, ByteCode::Opcode opcode, JITArg* args)
{
    sljit_s32 movOpcode = (opcode == ByteCode::I32PopcntOpcode) ? SLJIT_MOV32 : SLJIT_MOV;

    MOVE_TO_REG(compiler, movOpcode, SLJIT_R0, args[0].arg, args[0].argw);

    sljit_sw funcAddr = (opcode == ByteCode::I32PopcntOpcode) ? GET_FUNC_ADDR(sljit_sw, popcnt32) : GET_FUNC_ADDR(sljit_sw, popcnt64);
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
    JITArg args[2] = { operands, operands + 1 };

    sljit_s32 opcode;

    switch (instr->opcode()) {
    case ByteCode::I32ClzOpcode:
        opcode = SLJIT_CLZ32;
        break;
    case ByteCode::I32CtzOpcode:
        opcode = SLJIT_CTZ32;
        break;
    case ByteCode::I64ClzOpcode:
        opcode = SLJIT_CLZ;
        break;
    case ByteCode::I64CtzOpcode:
        opcode = SLJIT_CTZ;
        break;
    case ByteCode::I32PopcntOpcode:
    case ByteCode::I64PopcntOpcode:
        emitPopcnt(compiler, instr->opcode(), args);
        return;
    case ByteCode::I32Extend8SOpcode:
        emitExtend(compiler, SLJIT_MOV32_S8, 3, args);
        return;
    case ByteCode::I32Extend16SOpcode:
        emitExtend(compiler, SLJIT_MOV32_S16, 2, args);
        return;
    case ByteCode::I64Extend8SOpcode:
        emitExtend(compiler, SLJIT_MOV_S8, 7, args);
        return;
    case ByteCode::I64Extend16SOpcode:
        emitExtend(compiler, SLJIT_MOV_S16, 6, args);
        return;
    case ByteCode::I64Extend32SOpcode:
        emitExtend(compiler, SLJIT_MOV_S32, 4, args);
        return;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }

    // If the operand is an immediate then it is necesarry to move it into a
    // register because immediate source arguments are not supported.
    if (args[0].arg & SLJIT_IMM) {
        sljit_s32 mov = SLJIT_MOV;

        if (instr->info() & Instruction::kIs32Bit) {
            mov = SLJIT_MOV32;
        }

        sljit_emit_op1(compiler, mov, SLJIT_R0, 0, args[0].arg, args[0].argw);
        args[0].arg = SLJIT_R0;
        args[0].argw = 0;
    }

    sljit_emit_op1(compiler, opcode, args[1].arg, args[1].argw, args[0].arg, args[0].argw);
}

void emitSelect(sljit_compiler* compiler, Instruction* instr, sljit_s32 type)
{
    Operand* operands = instr->operands();
    assert(instr->opcode() == ByteCode::SelectOpcode && instr->paramCount() == 3);

    if (false) {
        return emitFloatSelect(compiler, instr, type);
    }

    bool is32 = reinterpret_cast<Select*>(instr->byteCode())->valueSize() == 4;
    sljit_s32 movOpcode = is32 ? SLJIT_MOV32 : SLJIT_MOV;
    JITArg args[3] = { operands, operands + 1, operands + 3 };

    if (type == -1) {
        JITArg cond(operands + 2);

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

    for (uint32_t i = 0; i < instr->paramCount(); ++i) {
        params[i].set(operand);
        operand++;
    }

    switch (instr->opcode()) {
    case ByteCode::I32EqzOpcode:
    case ByteCode::I64EqzOpcode:
        opcode = SLJIT_SUB | SLJIT_SET_Z;
        type = SLJIT_EQUAL;
        params[1].arg = SLJIT_IMM;
        params[1].argw = 0;
        break;
    case ByteCode::I32EqOpcode:
    case ByteCode::I64EqOpcode:
        opcode = SLJIT_SUB | SLJIT_SET_Z;
        type = SLJIT_EQUAL;
        break;
    case ByteCode::I32NeOpcode:
    case ByteCode::I64NeOpcode:
        opcode = SLJIT_SUB | SLJIT_SET_Z;
        type = SLJIT_NOT_EQUAL;
        break;
    case ByteCode::I32LtSOpcode:
    case ByteCode::I64LtSOpcode:
        opcode = SLJIT_SUB | SLJIT_SET_SIG_LESS;
        type = SLJIT_SIG_LESS;
        break;
    case ByteCode::I32LtUOpcode:
    case ByteCode::I64LtUOpcode:
        opcode = SLJIT_SUB | SLJIT_SET_LESS;
        type = SLJIT_LESS;
        break;
    case ByteCode::I32GtSOpcode:
    case ByteCode::I64GtSOpcode:
        opcode = SLJIT_SUB | SLJIT_SET_SIG_GREATER;
        type = SLJIT_SIG_GREATER;
        break;
    case ByteCode::I32GtUOpcode:
    case ByteCode::I64GtUOpcode:
        opcode = SLJIT_SUB | SLJIT_SET_GREATER;
        type = SLJIT_GREATER;
        break;
    case ByteCode::I32LeSOpcode:
    case ByteCode::I64LeSOpcode:
        opcode = SLJIT_SUB | SLJIT_SET_SIG_LESS_EQUAL;
        type = SLJIT_SIG_LESS_EQUAL;
        break;
    case ByteCode::I32LeUOpcode:
    case ByteCode::I64LeUOpcode:
        opcode = SLJIT_SUB | SLJIT_SET_LESS_EQUAL;
        type = SLJIT_LESS_EQUAL;
        break;
    case ByteCode::I32GeSOpcode:
    case ByteCode::I64GeSOpcode:
        opcode = SLJIT_SUB | SLJIT_SET_SIG_GREATER_EQUAL;
        type = SLJIT_SIG_GREATER_EQUAL;
        break;
    case ByteCode::I32GeUOpcode:
    case ByteCode::I64GeUOpcode:
        opcode = SLJIT_SUB | SLJIT_SET_GREATER_EQUAL;
        type = SLJIT_GREATER_EQUAL;
        break;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }

    Instruction* nextInstr = nullptr;
    bool isSelect = false;

    ASSERT(instr->next() != nullptr);

    if (instr->next()->isInstruction()) {
        nextInstr = instr->next()->asInstruction();

        switch (nextInstr->opcode()) {
        case ByteCode::JumpIfTrueOpcode:
        case ByteCode::JumpIfFalseOpcode:
            if (nextInstr->getParam(0)->item == instr) {
                if (nextInstr->opcode() == ByteCode::JumpIfFalseOpcode) {
                    type ^= 0x1;
                }

                if (instr->info() & Instruction::kIs32Bit) {
                    type |= SLJIT_32;
                }

                sljit_jump* jump = sljit_emit_cmp(compiler, type, params[0].arg, params[0].argw, params[1].arg, params[1].argw);
                nextInstr->asExtended()->value().targetLabel->jumpFrom(jump);
                return true;
            }
            break;
        case ByteCode::SelectOpcode:
            if (nextInstr->getParam(2)->item == instr) {
                isSelect = true;
            }
            break;
        default:
            break;
        }
    }

    if (instr->info() & Instruction::kIs32Bit) {
        opcode |= SLJIT_32;
    }

    sljit_emit_op2u(compiler, opcode, params[0].arg, params[0].argw, params[1].arg, params[1].argw);

    if (isSelect) {
        emitSelect(compiler, nextInstr, type);
        return true;
    }

    params[0].set(operand);
    sljit_emit_op_flags(compiler, SLJIT_MOV32, params[0].arg, params[0].argw, type);
    return false;
}

static void emitConvert(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[2] = { operands, operands + 1 };

    switch (instr->opcode()) {
    case ByteCode::I32WrapI64Opcode:
        if (args[0].arg & SLJIT_MEM) {
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, args[0].arg, args[0].argw);
            sljit_emit_op1(compiler, SLJIT_MOV32, args[1].arg, args[1].argw, SLJIT_R0, 0);
        } else {
            sljit_emit_op1(compiler, SLJIT_MOV32, args[1].arg, args[1].argw, args[0].arg, args[0].argw);
        }
        return;
    case ByteCode::I64ExtendI32SOpcode:
        if (SLJIT_IS_IMM(args[0].arg)) {
            sljit_emit_op1(compiler, SLJIT_MOV, args[1].arg, args[1].argw, args[0].arg, static_cast<sljit_s32>(args[0].argw));
        } else if (SLJIT_IS_MEM(args[0].arg)) {
            sljit_emit_op1(compiler, SLJIT_MOV_S32, SLJIT_R0, 0, args[0].arg, args[0].argw);
            sljit_emit_op1(compiler, SLJIT_MOV, args[1].arg, args[1].argw, SLJIT_R0, 0);
        } else {
            sljit_emit_op1(compiler, SLJIT_MOV_S32, args[1].arg, args[1].argw, args[0].arg, args[0].argw);
        }
        return;
    case ByteCode::I64ExtendI32UOpcode:
        if (SLJIT_IS_IMM(args[0].arg)) {
            sljit_emit_op1(compiler, SLJIT_MOV, args[1].arg, args[1].argw, args[0].arg, static_cast<sljit_u32>(args[0].argw));
        } else if (SLJIT_IS_MEM(args[0].arg)) {
            sljit_emit_op1(compiler, SLJIT_MOV_U32, SLJIT_R0, 0, args[0].arg, args[0].argw);
            sljit_emit_op1(compiler, SLJIT_MOV, args[1].arg, args[1].argw, SLJIT_R0, 0);
        } else {
            sljit_emit_op1(compiler, SLJIT_MOV_U32, args[1].arg, args[1].argw, args[0].arg, args[0].argw);
        }
        return;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
}

static void emitMove64(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg src(operands);
    JITArg dst(operands + 1);

    sljit_emit_op1(compiler, SLJIT_MOV, dst.arg, dst.argw, src.arg, src.argw);
}

static void emitGlobalGet64(sljit_compiler* compiler, Instruction* instr)
{
    CompileContext* context = CompileContext::get(compiler);
    Operand* operands = instr->operands();
    JITArg dst(operands);

    GlobalGet64* globalGet = reinterpret_cast<GlobalGet64*>(instr->byteCode());

    sljit_emit_op1(compiler, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_MEM1(kContextReg), OffsetOfContextField(instance));
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_MEM1(SLJIT_R0), context->globalsStart + globalGet->index() * sizeof(void*));
    sljit_emit_op1(compiler, SLJIT_MOV, dst.arg, dst.argw, SLJIT_MEM1(SLJIT_R0), JITFieldAccessor::globalValueOffset());
}

static void emitGlobalSet64(sljit_compiler* compiler, Instruction* instr)
{
    CompileContext* context = CompileContext::get(compiler);
    Operand* operands = instr->operands();
    JITArg src(operands);

    GlobalSet64* globalSet = reinterpret_cast<GlobalSet64*>(instr->byteCode());

    if (SLJIT_IS_MEM(src.arg)) {
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, src.arg, src.argw);
        src.arg = SLJIT_R1;
        src.argw = 0;
    }

    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_MEM1(kContextReg), OffsetOfContextField(instance));
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_MEM1(SLJIT_R0), context->globalsStart + globalSet->index() * sizeof(void*));
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_R0), JITFieldAccessor::globalValueOffset(), src.arg, src.argw);
}
