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

enum FloatConversionFlags : uint32_t {
    SourceIsFloat = 1 << 0,
    SourceIs64Bit = 1 << 1,
    DestinationIsFloat = 1 << 2,
    DestinationIs64Bit = 1 << 3,
    IsTruncSat = 1 << 4,
};

#define CONVERT_FROM_FLOAT(name, arg_type, result_type)               \
    static sljit_sw name(arg_type arg, result_type* result)           \
    {                                                                 \
        if (UNLIKELY(std::isnan(arg))) {                              \
            return ExecutionContext::InvalidConversionToIntegerError; \
        }                                                             \
                                                                      \
        if (UNLIKELY(!canConvert<result_type>(arg))) {                \
            return ExecutionContext::IntegerOverflowError;            \
        }                                                             \
                                                                      \
        *result = convert<result_type>(arg);                          \
        return ExecutionContext::NoError;                             \
    }

CONVERT_FROM_FLOAT(convertF32ToU64, sljit_f32, uint64_t)
CONVERT_FROM_FLOAT(convertF64ToU64, sljit_f64, uint64_t)

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
CONVERT_FROM_FLOAT(convertF32ToU32, sljit_f32, uint32_t)
CONVERT_FROM_FLOAT(convertF64ToU32, sljit_f64, uint32_t)
CONVERT_FROM_FLOAT(convertF32ToS64, sljit_f32, int64_t)
CONVERT_FROM_FLOAT(convertF64ToS64, sljit_f64, int64_t)
#endif /* SLJIT_32BIT_ARCHITECTURE */

#define TRUNC_SAT(name, arg_type, result_type)                              \
    static result_type name(arg_type arg)                                   \
    {                                                                       \
        if (canConvert<result_type>(arg)) {                                 \
            return convert<result_type>(arg);                               \
        }                                                                   \
                                                                            \
        if (std::isnan(arg)) {                                              \
            return 0;                                                       \
        }                                                                   \
                                                                            \
        return std::signbit(arg) ? std::numeric_limits<result_type>::min()  \
                                 : std::numeric_limits<result_type>::max(); \
    }

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)

TRUNC_SAT(truncSatF32ToU32, sljit_f32, uint32_t)
TRUNC_SAT(truncSatF64ToU32, sljit_f64, uint32_t)

#undef TRUNC_SAT
#define TRUNC_SAT(name, arg_type, result_type)                                 \
    static void name(arg_type arg, result_type* result)                        \
    {                                                                          \
        if (canConvert<result_type>(arg)) {                                    \
            *result = convert<result_type>(arg);                               \
            return;                                                            \
        }                                                                      \
                                                                               \
        if (std::isnan(arg)) {                                                 \
            *result = 0;                                                       \
            return;                                                            \
        }                                                                      \
                                                                               \
        *result = std::signbit(arg) ? std::numeric_limits<result_type>::min()  \
                                    : std::numeric_limits<result_type>::max(); \
    }

TRUNC_SAT(truncSatF32ToS64, sljit_f32, int64_t)
TRUNC_SAT(truncSatF64ToS64, sljit_f64, int64_t)

#endif /* SLJIT_32BIT_ARCHITECTURE */

TRUNC_SAT(truncSatF32ToU64, sljit_f32, uint64_t)
TRUNC_SAT(truncSatF64ToU64, sljit_f64, uint64_t)

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)

#define CONVERT_TO_FLOAT(name, arg_type, result_type)                                     \
    static result_type name(uint32_t arg1, uint32_t arg2)                                 \
    {                                                                                     \
        arg_type arg = static_cast<arg_type>(arg1) | (static_cast<arg_type>(arg2) << 32); \
        return convert<result_type>(arg);                                                 \
    }

CONVERT_TO_FLOAT(convertS64ToF32, int64_t, sljit_f32)
CONVERT_TO_FLOAT(convertS64ToF64, int64_t, sljit_f64)
CONVERT_TO_FLOAT(convertU64ToF32, uint64_t, sljit_f32)
CONVERT_TO_FLOAT(convertU64ToF64, uint64_t, sljit_f64)

#endif /* SLJIT_32BIT_ARCHITECTURE */

static void emitConvertFloatFromInteger(sljit_compiler* compiler, Instruction* instr, sljit_s32 opcode)
{
    ASSERT(!(instr->info() & Instruction::kIsCallback));

    Operand* operands = instr->operands();

    JITArg srcArg(operands);
    JITArg dstArg(operands + 1);

    sljit_emit_fop1(compiler, opcode, dstArg.arg, dstArg.argw, srcArg.arg, srcArg.argw);
}

class ConvertIntFromFloatSlowCase : public SlowCase {
public:
    ConvertIntFromFloatSlowCase(Type type, sljit_jump* jumpFrom, sljit_label* resumeLabel, Instruction* instr,
                                sljit_s32 opcode, sljit_s32 sourceReg, sljit_s32 resultReg)
        : SlowCase(type, jumpFrom, resumeLabel, instr)
        , m_opcode(opcode)
        , m_sourceReg(sourceReg)
        , m_resultReg(resultReg)
    {
    }

    void emitSlowCase(sljit_compiler* compiler);

private:
    sljit_s32 m_opcode;
    sljit_s32 m_sourceReg;
    sljit_s32 m_resultReg;
};

static void emitConvertIntegerFromFloat(sljit_compiler* compiler, Instruction* instr, sljit_s32 opcode)
{
    ASSERT(!(instr->info() & Instruction::kIsCallback));

    CompileContext* context = CompileContext::get(compiler);
    Operand* operands = instr->operands();

    JITArg srcArg(operands);
    JITArg dstArg(operands + 1);

    sljit_s32 sourceReg = GET_SOURCE_REG(srcArg.arg, instr->requiredReg(0));
    sljit_s32 resultReg = GET_TARGET_REG(dstArg.arg, instr->requiredReg(1));
    sljit_s32 tmpReg = SLJIT_TMP_DEST_REG;

    floatOperandToArg(compiler, operands, srcArg, sourceReg);
    MOVE_TO_FREG(compiler, SLJIT_MOV_F64 | (opcode & SLJIT_32), sourceReg, srcArg.arg, srcArg.argw);

    sljit_jump* cmp = sljit_emit_fcmp(compiler, SLJIT_UNORDERED | (opcode & SLJIT_32), sourceReg, 0, sourceReg, 0);
    context->appendTrapJump(ExecutionContext::InvalidConversionToIntegerError, cmp);

    sljit_emit_fop1(compiler, opcode, resultReg, 0, sourceReg, 0);

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    ASSERT((opcode | SLJIT_32) != SLJIT_CONV_SW_FROM_F32);
    sljit_s32 flag32 = 0;
#else /* !SLJIT_32BIT_ARCHITECTURE */
    sljit_s32 flag32 = (opcode | SLJIT_32) == SLJIT_CONV_S32_FROM_F32 ? SLJIT_32 : 0;
#endif /* SLJIT_32BIT_ARCHITECTURE */

#if SLJIT_CONV_MAX_FLOAT != SLJIT_CONV_MIN_FLOAT
    sljit_emit_op2(compiler, SLJIT_ADD | flag32, resultReg, 0, resultReg, 0, SLJIT_IMM, 1);
    sljit_emit_op2(compiler, SLJIT_SUB | flag32, tmpReg, 0, resultReg, 0, SLJIT_IMM, 2);
    cmp = sljit_emit_cmp(compiler, SLJIT_SIG_LESS | flag32, resultReg, 0, tmpReg, 0);
#elif SLJIT_CONV_MAX_FLOAT == SLJIT_CONV_RESULT_MAX_INT
    sljit_emit_op2(compiler, SLJIT_ADD | flag32, tmpReg, 0, resultReg, 0, SLJIT_IMM, 1);
    cmp = sljit_emit_cmp(compiler, SLJIT_SIG_LESS | flag32, tmpReg, 0, resultReg, 0);
#else /* SLJIT_CONV_MAX_FLOAT != SLJIT_CONV_RESULT_MAX_INT */
    sljit_emit_op2(compiler, SLJIT_SUB | flag32, tmpReg, 0, resultReg, 0, SLJIT_IMM, 1);
    cmp = sljit_emit_cmp(compiler, SLJIT_SIG_LESS | flag32, resultReg, 0, tmpReg, 0);
#endif /* SLJIT_CONV_MAX_FLOAT != SLJIT_CONV_RESULT_MAX_INT */

    sljit_label* resumeLabel = sljit_emit_label(compiler);

#if SLJIT_CONV_MAX_FLOAT != SLJIT_CONV_MIN_FLOAT
    sljit_emit_op2(compiler, SLJIT_SUB | flag32, resultReg, 0, resultReg, 0, SLJIT_IMM, 1);
#endif /* SLJIT_CONV_MAX_FLOAT != SLJIT_CONV_MIN_FLOAT */

    context->add(new ConvertIntFromFloatSlowCase(SlowCase::Type::ConvertIntFromFloat,
                                                 cmp, resumeLabel, instr, opcode, sourceReg, resultReg));

    MOVE_FROM_REG(compiler, flag32 ? SLJIT_MOV32 : SLJIT_MOV, dstArg.arg, dstArg.argw, resultReg);
}

void ConvertIntFromFloatSlowCase::emitSlowCase(sljit_compiler* compiler)
{
    CompileContext* context = CompileContext::get(compiler);
    sljit_s32 tmpFReg = SLJIT_TMP_DEST_FREG;

    if (m_opcode != SLJIT_CONV_S32_FROM_F64) {
#if SLJIT_CONV_MAX_FLOAT != SLJIT_CONV_MIN_FLOAT

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        sljit_emit_fset32(compiler, tmpFReg, -2147483648.f);
#else /* !SLJIT_32BIT_ARCHITECTURE */
        switch (m_opcode) {
        case SLJIT_CONV_S32_FROM_F32:
            sljit_emit_fset32(compiler, tmpFReg, -2147483648.f);
            break;
        case SLJIT_CONV_SW_FROM_F64:
            sljit_emit_fset64(compiler, tmpFReg, -9223372036854775808.);
            break;
        default:
            ASSERT(m_opcode == SLJIT_CONV_SW_FROM_F32);
            sljit_emit_fset32(compiler, tmpFReg, -9223372036854775808.f);
            break;
        }
#endif /* SLJIT_32BIT_ARCHITECTURE */

        sljit_jump* cmp = sljit_emit_fcmp(compiler, SLJIT_F_EQUAL | (m_opcode & SLJIT_32), m_sourceReg, 0, tmpFReg, 0);
        sljit_set_label(cmp, m_resumeLabel);
#elif SLJIT_CONV_MAX_FLOAT != SLJIT_CONV_RESULT_MAX_INT

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        sljit_s32 convOpcode = SLJIT_CONV_F32_FROM_S32;
#else /* !SLJIT_32BIT_ARCHITECTURE */
        sljit_s32 convOpcode;

        switch (m_opcode) {
        case SLJIT_CONV_S32_FROM_F32:
            convOpcode = SLJIT_CONV_F32_FROM_S32;
            break;
        case SLJIT_CONV_SW_FROM_F64:
            convOpcode = SLJIT_CONV_F64_FROM_SW;
            break;
        default:
            ASSERT(m_opcode == SLJIT_CONV_SW_FROM_F32);
            convOpcode = SLJIT_CONV_F32_FROM_SW;
            break;
        }
#endif /* SLJIT_32BIT_ARCHITECTURE */

        sljit_emit_fop1(compiler, convOpcode, tmpFReg, 0, m_resultReg, 0);
        sljit_jump* cmp = sljit_emit_fcmp(compiler, SLJIT_F_EQUAL | (m_opcode & SLJIT_32), m_sourceReg, 0, tmpFReg, 0);
        sljit_set_label(cmp, m_resumeLabel);
#endif /* SLJIT_CONV_MAX_FLOAT != SLJIT_CONV_RESULT_MAX_INT */

        context->appendTrapJump(ExecutionContext::IntegerOverflowError, sljit_emit_jump(compiler, SLJIT_JUMP));
        return;
    }

#if (SLJIT_CONV_MAX_FLOAT == SLJIT_CONV_MIN_FLOAT) && (SLJIT_CONV_MAX_FLOAT == SLJIT_CONV_RESULT_MAX_INT)
    sljit_emit_fset64(compiler, tmpFReg, 0);
#else /* (SLJIT_CONV_MAX_FLOAT != SLJIT_CONV_MIN_FLOAT) || (SLJIT_CONV_MAX_FLOAT != SLJIT_CONV_RESULT_MAX_INT) */
    sljit_emit_fset64(compiler, tmpFReg, -2147483649.);
#endif /* (SLJIT_CONV_MAX_FLOAT == SLJIT_CONV_MIN_FLOAT) && (SLJIT_CONV_MAX_FLOAT == SLJIT_CONV_RESULT_MAX_INT) */

    sljit_jump* cmp = sljit_emit_fcmp(compiler, SLJIT_F_LESS_EQUAL, m_sourceReg, 0, tmpFReg, 0);
    context->appendTrapJump(ExecutionContext::IntegerOverflowError, cmp);

#if (SLJIT_CONV_MAX_FLOAT == SLJIT_CONV_MIN_FLOAT) && (SLJIT_CONV_MAX_FLOAT == SLJIT_CONV_RESULT_MIN_INT)
    sljit_emit_fset64(compiler, tmpFReg, 0.);
#else /* (SLJIT_CONV_MAX_FLOAT != SLJIT_CONV_MIN_FLOAT) || (SLJIT_CONV_MAX_FLOAT != SLJIT_CONV_RESULT_MIN_INT) */
    sljit_emit_fset64(compiler, tmpFReg, 2147483648.);
#endif /* (SLJIT_CONV_MAX_FLOAT == SLJIT_CONV_MIN_FLOAT) && (SLJIT_CONV_MAX_FLOAT == SLJIT_CONV_RESULT_MIN_INT) */

    cmp = sljit_emit_fcmp(compiler, SLJIT_F_GREATER_EQUAL, m_sourceReg, 0, tmpFReg, 0);
    context->appendTrapJump(ExecutionContext::IntegerOverflowError, cmp);
    sljit_set_label(sljit_emit_jump(compiler, SLJIT_JUMP), m_resumeLabel);
}

#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)

#if (SLJIT_CONV_NAN_FLOAT != SLJIT_CONV_RESULT_ZERO)

class ConvertUnsignedIntFromFloatSlowCase : public SlowCase {
public:
    ConvertUnsignedIntFromFloatSlowCase(Type type, sljit_jump* jumpFrom, Instruction* instr,
                                        sljit_s32 opcode, sljit_s32 sourceReg)
        : SlowCase(type, jumpFrom, nullptr, instr)
        , m_opcode(opcode)
        , m_sourceReg(sourceReg)
    {
    }

    sljit_jump* emitCompareUnordered(sljit_compiler* compiler)
    {
        return sljit_emit_fcmp(compiler, SLJIT_UNORDERED | (m_opcode & SLJIT_32), m_sourceReg, 0, m_sourceReg, 0);
    }

private:
    sljit_s32 m_opcode;
    sljit_s32 m_sourceReg;
};

#endif /* SLJIT_CONV_NAN_FLOAT != SLJIT_CONV_RESULT_ZERO */

static void emitConvertUnsigned32FromFloat(sljit_compiler* compiler, Instruction* instr, sljit_s32 opcode)
{
    ASSERT(!(instr->info() & Instruction::kIsCallback));

    CompileContext* context = CompileContext::get(compiler);
    Operand* operands = instr->operands();

    JITArg srcArg(operands);
    JITArg dstArg(operands + 1);

    sljit_s32 sourceReg = GET_SOURCE_REG(srcArg.arg, instr->requiredReg(0));
    sljit_s32 resultReg = GET_TARGET_REG(dstArg.arg, instr->requiredReg(1));

    MOVE_TO_FREG(compiler, SLJIT_MOV_F64 | (opcode & SLJIT_32), sourceReg, srcArg.arg, srcArg.argw);
    sljit_emit_fop1(compiler, opcode, resultReg, 0, sourceReg, 0);

#if (SLJIT_CONV_NAN_FLOAT != SLJIT_CONV_RESULT_ZERO)
    sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_GREATER_EQUAL, resultReg, 0, SLJIT_IMM, 0x100000000);
    context->add(new ConvertUnsignedIntFromFloatSlowCase(SlowCase::Type::ConvertUnsignedIntFromFloat,
                                                         cmp, instr, opcode, sourceReg));
#else /* SLJIT_CONV_NAN_FLOAT == SLJIT_CONV_RESULT_ZERO */
    sljit_jump* cmp = sljit_emit_fcmp(compiler, SLJIT_UNORDERED | (opcode & SLJIT_32), sourceReg, 0, sourceReg, 0);
    context->appendTrapJump(ExecutionContext::InvalidConversionToIntegerError, cmp);

    cmp = sljit_emit_cmp(compiler, SLJIT_GREATER_EQUAL, resultReg, 0, SLJIT_IMM, 0x100000000);
    context->appendTrapJump(ExecutionContext::IntegerOverflowError, cmp);
#endif /* SLJIT_CONV_NAN_FLOAT != SLJIT_CONV_RESULT_ZERO */

    /* Usually a nop. */
    sljit_emit_op1(compiler, SLJIT_MOV32, resultReg, 0, resultReg, 0);
    MOVE_FROM_REG(compiler, SLJIT_MOV32, dstArg.arg, dstArg.argw, resultReg);
}

#endif /* SLJIT_64BIT_ARCHITECTURE */

static void emitSaturatedConvertIntegerFromFloat(sljit_compiler* compiler, Instruction* instr, sljit_s32 opcode)
{
    ASSERT(!(instr->info() & Instruction::kIsCallback));

    Operand* operands = instr->operands();

    JITArg srcArg(operands);
    JITArg dstArg(operands + 1);

    sljit_s32 sourceReg = GET_SOURCE_REG(srcArg.arg, instr->requiredReg(0));
    sljit_s32 resultReg = GET_TARGET_REG(dstArg.arg, instr->requiredReg(1));
    sljit_s32 tmpFReg = SLJIT_TMP_DEST_FREG;

    floatOperandToArg(compiler, operands, srcArg, sourceReg);
    MOVE_TO_FREG(compiler, SLJIT_MOV_F64 | (opcode & SLJIT_32), sourceReg, srcArg.arg, srcArg.argw);
    sljit_emit_fop1(compiler, opcode, resultReg, 0, sourceReg, 0);

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    ASSERT((opcode | SLJIT_32) != SLJIT_CONV_SW_FROM_F32);
    sljit_s32 flag32 = SLJIT_32;
#else /* !SLJIT_32BIT_ARCHITECTURE */
    sljit_s32 flag32 = (opcode | SLJIT_32) == SLJIT_CONV_S32_FROM_F32 ? SLJIT_32 : 0;
#endif /* SLJIT_32BIT_ARCHITECTURE */

#if SLJIT_CONV_MAX_FLOAT != SLJIT_CONV_RESULT_MAX_INT
    if (!flag32) {
        if (opcode & SLJIT_32) {
            sljit_emit_fset32(compiler, tmpFReg, 9223372036854775808.f);
        } else {
            sljit_emit_fset64(compiler, tmpFReg, 9223372036854775808.);
        }
    } else {
        if (opcode & SLJIT_32) {
            sljit_emit_fset32(compiler, tmpFReg, 2147483648.f);
        } else {
            sljit_emit_fset64(compiler, tmpFReg, 2147483648.);
        }
    }

    sljit_emit_fop1(compiler, SLJIT_CMP_F64 | SLJIT_SET_ORDERED_GREATER_EQUAL | (opcode & SLJIT_32), sourceReg, 0, tmpFReg, 0);

    sljit_sw imm = flag32 ? (sljit_sw)(((sljit_u32)1 << 31) - 1) : (sljit_sw)(((sljit_uw)1 << (sizeof(sljit_uw) * 8 - 1)) - 1);
    sljit_emit_select(compiler, SLJIT_ORDERED_GREATER_EQUAL | flag32, resultReg, SLJIT_IMM, imm, resultReg);
#elif SLJIT_CONV_MIN_FLOAT != SLJIT_CONV_RESULT_MIN_INT
    if (!flag32) {
        if (opcode & SLJIT_32) {
            sljit_emit_fset32(compiler, tmpFReg, -9223372036854775808.f);
        } else {
            sljit_emit_fset64(compiler, tmpFReg, -9223372036854775808.);
        }
    } else {
        if (opcode & SLJIT_32) {
            sljit_emit_fset32(compiler, tmpFReg, -2147483648.f);
        } else {
            sljit_emit_fset64(compiler, tmpFReg, -2147483649.);
        }
    }

    sljit_emit_fop1(compiler, SLJIT_CMP_F64 | SLJIT_SET_ORDERED_LESS_EQUAL | (opcode & SLJIT_32), sourceReg, 0, tmpFReg, 0);

    sljit_sw imm = flag32 ? (sljit_sw)((sljit_u32)1 << 31) : (sljit_sw)((sljit_uw)1 << (sizeof(sljit_uw) * 8 - 1));
    sljit_emit_select(compiler, SLJIT_ORDERED_LESS_EQUAL | flag32, resultReg, SLJIT_IMM, imm, resultReg);
#endif /* SLJIT_CONV_MAX_FLOAT != SLJIT_CONV_RESULT_MAX_INT */

#if SLJIT_CONV_NAN_FLOAT != SLJIT_CONV_RESULT_ZERO
    sljit_emit_fop1(compiler, SLJIT_CMP_F64 | SLJIT_SET_UNORDERED | (opcode & SLJIT_32), sourceReg, 0, sourceReg, 0);
    sljit_emit_select(compiler, SLJIT_UNORDERED | flag32, resultReg, SLJIT_IMM, 0, resultReg);
#endif /* SLJIT_CONV_NAN_FLOAT != SLJIT_CONV_RESULT_ZERO */

    MOVE_FROM_REG(compiler, flag32 ? SLJIT_MOV32 : SLJIT_MOV, dstArg.arg, dstArg.argw, resultReg);
}

#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)

static void emitSaturatedConvertUnsigned32FromFloat(sljit_compiler* compiler, Instruction* instr, sljit_s32 opcode)
{
    ASSERT(!(instr->info() & Instruction::kIsCallback));

    Operand* operands = instr->operands();

    JITArg srcArg(operands);
    JITArg dstArg(operands + 1);

    sljit_s32 sourceReg = GET_SOURCE_REG(srcArg.arg, instr->requiredReg(0));
    sljit_s32 resultReg = GET_TARGET_REG(dstArg.arg, instr->requiredReg(1));
    sljit_s32 tmpFReg = SLJIT_TMP_DEST_FREG;

    MOVE_TO_FREG(compiler, SLJIT_MOV_F64 | (opcode & SLJIT_32), sourceReg, srcArg.arg, srcArg.argw);
    sljit_emit_fop1(compiler, opcode, resultReg, 0, sourceReg, 0);

    if (opcode == SLJIT_CONV_SW_FROM_F64) {
        sljit_emit_fset64(compiler, tmpFReg, 0.0);
    } else {
        sljit_emit_fset32(compiler, tmpFReg, 0.0);
    }

    sljit_emit_fop1(compiler, SLJIT_CMP_F64 | SLJIT_SET_UNORDERED_OR_LESS | (opcode & SLJIT_32), sourceReg, 0, tmpFReg, 0);
    sljit_emit_select(compiler, SLJIT_UNORDERED_OR_LESS, resultReg, SLJIT_IMM, 0, resultReg);

    sljit_emit_op2u(compiler, SLJIT_SUB | SLJIT_SET_GREATER_EQUAL, resultReg, 0, SLJIT_IMM, 0x100000000);
    sljit_emit_select(compiler, SLJIT_GREATER_EQUAL, resultReg, SLJIT_IMM, -1, resultReg);

    /* Usually a nop. */
    sljit_emit_op1(compiler, SLJIT_MOV32, resultReg, 0, resultReg, 0);
    MOVE_FROM_REG(compiler, SLJIT_MOV32, dstArg.arg, dstArg.argw, resultReg);
}

#endif /* SLJIT_64BIT_ARCHITECTURE */

static void checkConvertResult(sljit_compiler* compiler)
{
    sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, ExecutionContext::NoError);
    CompileContext::get(compiler)->appendTrapJump(ExecutionContext::GenericTrap, cmp);
}

static void emitConvertFloat(sljit_compiler* compiler, Instruction* instr)
{
    uint32_t flags;
    sljit_sw addr = 0;

    switch (instr->opcode()) {
    case ByteCode::I32TruncF32SOpcode: {
        emitConvertIntegerFromFloat(compiler, instr, SLJIT_CONV_S32_FROM_F32);
        return;
    }
    case ByteCode::I32TruncF32UOpcode: {
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        flags = SourceIsFloat;
        addr = GET_FUNC_ADDR(sljit_sw, convertF32ToU32);
        break;
#else /* !SLJIT_32BIT_ARCHITECTURE */
        emitConvertUnsigned32FromFloat(compiler, instr, SLJIT_CONV_SW_FROM_F32);
        return;
#endif /* SLJIT_32BIT_ARCHITECTURE */
    }
    case ByteCode::I32TruncF64SOpcode: {
        emitConvertIntegerFromFloat(compiler, instr, SLJIT_CONV_S32_FROM_F64);
        return;
    }
    case ByteCode::I32TruncF64UOpcode: {
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        flags = SourceIsFloat | SourceIs64Bit;
        addr = GET_FUNC_ADDR(sljit_sw, convertF64ToU32);
        break;
#else /* !SLJIT_32BIT_ARCHITECTURE */
        emitConvertUnsigned32FromFloat(compiler, instr, SLJIT_CONV_SW_FROM_F64);
        return;
#endif /* SLJIT_32BIT_ARCHITECTURE */
    }
    case ByteCode::I64TruncF32SOpcode: {
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        flags = SourceIsFloat | DestinationIs64Bit;
        addr = GET_FUNC_ADDR(sljit_sw, convertF32ToS64);
        break;
#else /* !SLJIT_32BIT_ARCHITECTURE */
        emitConvertIntegerFromFloat(compiler, instr, SLJIT_CONV_SW_FROM_F32);
        return;
#endif /* SLJIT_32BIT_ARCHITECTURE */
    }
    case ByteCode::I64TruncF32UOpcode: {
        flags = SourceIsFloat | DestinationIs64Bit;
        addr = GET_FUNC_ADDR(sljit_sw, convertF32ToU64);
        break;
    }
    case ByteCode::I64TruncF64SOpcode: {
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        flags = SourceIsFloat | SourceIs64Bit | DestinationIs64Bit;
        addr = GET_FUNC_ADDR(sljit_sw, convertF64ToS64);
        break;
#else /* !SLJIT_32BIT_ARCHITECTURE */
        emitConvertIntegerFromFloat(compiler, instr, SLJIT_CONV_SW_FROM_F64);
        return;
#endif /* SLJIT_32BIT_ARCHITECTURE */
    }
    case ByteCode::I64TruncF64UOpcode: {
        flags = SourceIsFloat | SourceIs64Bit | DestinationIs64Bit;
        addr = GET_FUNC_ADDR(sljit_sw, convertF64ToU64);
        break;
    }
    case ByteCode::I32TruncSatF32SOpcode: {
        emitSaturatedConvertIntegerFromFloat(compiler, instr, SLJIT_CONV_S32_FROM_F32);
        return;
    }
    case ByteCode::I32TruncSatF32UOpcode: {
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        flags = SourceIsFloat | IsTruncSat;
        addr = GET_FUNC_ADDR(sljit_sw, truncSatF32ToU32);
        break;
#else /* !SLJIT_32BIT_ARCHITECTURE */
        emitSaturatedConvertUnsigned32FromFloat(compiler, instr, SLJIT_CONV_SW_FROM_F32);
        return;
#endif /* SLJIT_32BIT_ARCHITECTURE */
    }
    case ByteCode::I32TruncSatF64SOpcode: {
        emitSaturatedConvertIntegerFromFloat(compiler, instr, SLJIT_CONV_S32_FROM_F64);
        return;
    }
    case ByteCode::I32TruncSatF64UOpcode: {
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        flags = SourceIsFloat | SourceIs64Bit | IsTruncSat;
        addr = GET_FUNC_ADDR(sljit_sw, truncSatF64ToU32);
        break;
#else /* !SLJIT_32BIT_ARCHITECTURE */
        emitSaturatedConvertUnsigned32FromFloat(compiler, instr, SLJIT_CONV_SW_FROM_F64);
        return;
#endif /* SLJIT_32BIT_ARCHITECTURE */
    }
    case ByteCode::I64TruncSatF32SOpcode: {
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        flags = SourceIsFloat | DestinationIs64Bit | IsTruncSat;
        addr = GET_FUNC_ADDR(sljit_sw, truncSatF32ToS64);
        break;
#else /* !SLJIT_32BIT_ARCHITECTURE */
        emitSaturatedConvertIntegerFromFloat(compiler, instr, SLJIT_CONV_SW_FROM_F32);
        return;
#endif /* SLJIT_32BIT_ARCHITECTURE */
    }
    case ByteCode::I64TruncSatF32UOpcode: {
        flags = SourceIsFloat | DestinationIs64Bit | IsTruncSat;
        addr = GET_FUNC_ADDR(sljit_sw, truncSatF32ToU64);
        break;
    }
    case ByteCode::I64TruncSatF64SOpcode: {
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        flags = SourceIsFloat | SourceIs64Bit | DestinationIs64Bit | IsTruncSat;
        addr = GET_FUNC_ADDR(sljit_sw, truncSatF64ToS64);
        break;
#else /* !SLJIT_32BIT_ARCHITECTURE */
        emitSaturatedConvertIntegerFromFloat(compiler, instr, SLJIT_CONV_SW_FROM_F64);
        return;
#endif /* SLJIT_32BIT_ARCHITECTURE */
    }
    case ByteCode::I64TruncSatF64UOpcode: {
        flags = SourceIsFloat | SourceIs64Bit | DestinationIs64Bit | IsTruncSat;
        addr = GET_FUNC_ADDR(sljit_sw, truncSatF64ToU64);
        break;
    }
    case ByteCode::F32ConvertI32SOpcode: {
        emitConvertFloatFromInteger(compiler, instr, SLJIT_CONV_F32_FROM_S32);
        return;
    }
    case ByteCode::F32ConvertI32UOpcode: {
        emitConvertFloatFromInteger(compiler, instr, SLJIT_CONV_F32_FROM_U32);
        return;
    }
    case ByteCode::F32ConvertI64SOpcode: {
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        flags = SourceIs64Bit | DestinationIsFloat;
        addr = GET_FUNC_ADDR(sljit_sw, convertS64ToF32);
        break;
#else /* !SLJIT_32BIT_ARCHITECTURE */
        emitConvertFloatFromInteger(compiler, instr, SLJIT_CONV_F32_FROM_SW);
        return;
#endif /* SLJIT_32BIT_ARCHITECTURE */
    }
    case ByteCode::F32ConvertI64UOpcode: {
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        flags = SourceIs64Bit | DestinationIsFloat;
        addr = GET_FUNC_ADDR(sljit_sw, convertU64ToF32);
        break;
#else /* !SLJIT_32BIT_ARCHITECTURE */
        emitConvertFloatFromInteger(compiler, instr, SLJIT_CONV_F32_FROM_UW);
        return;
#endif /* SLJIT_32BIT_ARCHITECTURE */
    }
    case ByteCode::F64ConvertI32SOpcode: {
        emitConvertFloatFromInteger(compiler, instr, SLJIT_CONV_F64_FROM_S32);
        return;
    }
    case ByteCode::F64ConvertI32UOpcode: {
        emitConvertFloatFromInteger(compiler, instr, SLJIT_CONV_F64_FROM_U32);
        return;
    }
    case ByteCode::F64ConvertI64SOpcode: {
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        flags = SourceIs64Bit | DestinationIsFloat | DestinationIs64Bit;
        addr = GET_FUNC_ADDR(sljit_sw, convertS64ToF64);
        break;
#else /* !SLJIT_32BIT_ARCHITECTURE */
        emitConvertFloatFromInteger(compiler, instr, SLJIT_CONV_F64_FROM_SW);
        return;
#endif /* SLJIT_32BIT_ARCHITECTURE */
    }
    default: {
        ASSERT(instr->opcode() == ByteCode::F64ConvertI64UOpcode);
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        flags = SourceIs64Bit | DestinationIsFloat | DestinationIs64Bit;
        addr = GET_FUNC_ADDR(sljit_sw, convertU64ToF64);
        break;
#else /* !SLJIT_32BIT_ARCHITECTURE */
        emitConvertFloatFromInteger(compiler, instr, SLJIT_CONV_F64_FROM_UW);
        return;
#endif /* SLJIT_32BIT_ARCHITECTURE */
    }
    }

    ASSERT(instr->info() & Instruction::kIsCallback);
    ASSERT((flags & SourceIsFloat) | (flags & DestinationIsFloat));

    JITArg arg;
    Operand* operands = instr->operands();

    if (flags & DestinationIsFloat) {
        sljit_s32 argTypes = (flags & DestinationIs64Bit) ? SLJIT_ARG_RETURN(SLJIT_ARG_TYPE_F64) : SLJIT_ARG_RETURN(SLJIT_ARG_TYPE_F32);

#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
        arg.set(operands);

        sljit_s32 movOp = (flags & SourceIs64Bit) ? SLJIT_MOV : SLJIT_MOV32;
        MOVE_TO_REG(compiler, movOp, SLJIT_R0, arg.arg, arg.argw);
        argTypes |= (flags & SourceIs64Bit) ? SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 1) : SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_32, 1);
#else /* !SLJIT_64BIT_ARCHITECTURE */
        if (flags & SourceIs64Bit) {
            JITArgPair srcArgPair(operands);

            MOVE_TO_REG(compiler, SLJIT_MOV, SLJIT_R0, srcArgPair.arg1, srcArgPair.arg1w);
            MOVE_TO_REG(compiler, SLJIT_MOV, SLJIT_R1, srcArgPair.arg2, srcArgPair.arg2w);
            argTypes |= SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 1) | SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 2);
        } else {
            arg.set(operands);

            MOVE_TO_REG(compiler, SLJIT_MOV, SLJIT_R0, arg.arg, arg.argw);
            argTypes |= SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 1);
        }
#endif /* SLJIT_64BIT_ARCHITECTURE */

        sljit_emit_icall(compiler, SLJIT_CALL, argTypes, SLJIT_IMM, addr);
        arg.set(operands + 1);

        sljit_s32 floatMovOp = (flags & DestinationIs64Bit) ? SLJIT_MOV_F64 : SLJIT_MOV_F32;
        MOVE_FROM_FREG(compiler, floatMovOp, arg.arg, arg.argw, SLJIT_FR0);
        return;
    }

    floatOperandToArg(compiler, operands, arg, SLJIT_FR0);

    sljit_s32 floatMovOp = (flags & SourceIs64Bit) ? SLJIT_MOV_F64 : SLJIT_MOV_F32;
    MOVE_TO_FREG(compiler, floatMovOp, SLJIT_FR0, arg.arg, arg.argw);

    sljit_s32 argTypes = (flags & SourceIs64Bit) ? SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_F64, 1) : SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_F32, 1);

    /* Destination must not be immediate. */
    ASSERT(VARIABLE_TYPE(operands[1]) != Instruction::ConstPtr);

#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
    arg.set(operands + 1);
#else /* !SLJIT_64BIT_ARCHITECTURE */
    JITArgPair argPair;

    if (flags & DestinationIs64Bit) {
        argPair.set(operands + 1);
        arg.arg = argPair.arg1;
        arg.argw = argPair.arg1w - WORD_LOW_OFFSET;
    } else {
        arg.set(operands + 1);
    }
#endif /* SLJIT_64BIT_ARCHITECTURE */

#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
    if (flags & IsTruncSat) {
        sljit_s32 movOp = (flags & DestinationIs64Bit) ? SLJIT_MOV : SLJIT_MOV32;
        argTypes |= (flags & DestinationIs64Bit) ? SLJIT_ARG_RETURN(SLJIT_ARG_TYPE_W) : SLJIT_ARG_RETURN(SLJIT_ARG_TYPE_32);
        sljit_emit_icall(compiler, SLJIT_CALL, argTypes, SLJIT_IMM, addr);
        MOVE_FROM_REG(compiler, movOp, arg.arg, arg.argw, SLJIT_R0);
        return;
    }
#else /* !SLJIT_64BIT_ARCHITECTURE */
    if ((flags & (IsTruncSat | DestinationIs64Bit)) == IsTruncSat) {
        argTypes |= SLJIT_ARG_RETURN(SLJIT_ARG_TYPE_W);
        sljit_emit_icall(compiler, SLJIT_CALL, argTypes, SLJIT_IMM, addr);
        MOVE_FROM_REG(compiler, SLJIT_MOV, arg.arg, arg.argw, SLJIT_R0);
        return;
    }
#endif /* SLJIT_64BIT_ARCHITECTURE */

    if (arg.arg == SLJIT_MEM1(kFrameReg)) {
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R0, 0, kFrameReg, 0, SLJIT_IMM, arg.argw);
    } else {
        sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R0, 0, kContextReg, 0, SLJIT_IMM, OffsetOfContextField(tmp1));
    }

#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
    argTypes |= SLJIT_ARG_RETURN(SLJIT_ARG_TYPE_W) | SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 2);
#else /* !SLJIT_64BIT_ARCHITECTURE */
    argTypes |= SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 2);
    argTypes |= (flags & IsTruncSat) ? SLJIT_ARG_RETURN(SLJIT_ARG_TYPE_RET_VOID) : SLJIT_ARG_RETURN(SLJIT_ARG_TYPE_W);
#endif /* SLJIT_64BIT_ARCHITECTURE */

    sljit_emit_icall(compiler, SLJIT_CALL, argTypes, SLJIT_IMM, addr);

#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
    checkConvertResult(compiler);
#else /* !SLJIT_64BIT_ARCHITECTURE */
    if (!(flags & IsTruncSat)) {
        checkConvertResult(compiler);
    }
#endif /* SLJIT_64BIT_ARCHITECTURE */

    if (arg.arg == SLJIT_MEM1(kFrameReg)) {
        return;
    }

#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
    sljit_s32 movOp = (flags & DestinationIs64Bit) ? SLJIT_MOV : SLJIT_MOV32;
    sljit_emit_op1(compiler, movOp, arg.arg, arg.argw, SLJIT_MEM1(kContextReg), OffsetOfContextField(tmp1));
#else /* !SLJIT_64BIT_ARCHITECTURE */
    if (!(flags & DestinationIs64Bit)) {
        sljit_emit_op1(compiler, SLJIT_MOV, arg.arg, arg.argw, SLJIT_MEM1(kContextReg), OffsetOfContextField(tmp1));
        return;
    }

    sljit_emit_op1(compiler, SLJIT_MOV, argPair.arg1, argPair.arg1w, SLJIT_MEM1(kContextReg), OffsetOfContextField(tmp1) + WORD_LOW_OFFSET);
    sljit_emit_op1(compiler, SLJIT_MOV, argPair.arg2, argPair.arg2w, SLJIT_MEM1(kContextReg), OffsetOfContextField(tmp1) + WORD_HIGH_OFFSET);
#endif /* SLJIT_32BIT_ARCHITECTURE */
}
