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

CONVERT_FROM_FLOAT(convertF32ToS32, sljit_f32, int32_t)
CONVERT_FROM_FLOAT(convertF32ToU32, sljit_f32, uint32_t)
CONVERT_FROM_FLOAT(convertF64ToS32, sljit_f64, int32_t)
CONVERT_FROM_FLOAT(convertF64ToU32, sljit_f64, uint32_t)
CONVERT_FROM_FLOAT(convertF32ToS64, sljit_f32, int64_t)
CONVERT_FROM_FLOAT(convertF32ToU64, sljit_f32, uint64_t)
CONVERT_FROM_FLOAT(convertF64ToS64, sljit_f64, int64_t)
CONVERT_FROM_FLOAT(convertF64ToU64, sljit_f64, uint64_t)

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

TRUNC_SAT(truncSatF32ToS32, sljit_f32, int32_t)
TRUNC_SAT(truncSatF32ToU32, sljit_f32, uint32_t)
TRUNC_SAT(truncSatF64ToS32, sljit_f64, int32_t)
TRUNC_SAT(truncSatF64ToU32, sljit_f64, uint32_t)

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)

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

#endif /* SLJIT_32BIT_ARCHITECTURE */

TRUNC_SAT(truncSatF32ToS64, sljit_f32, int64_t)
TRUNC_SAT(truncSatF32ToU64, sljit_f32, uint64_t)
TRUNC_SAT(truncSatF64ToS64, sljit_f64, int64_t)
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
    Operand* operands = instr->operands();

    JITArg srcArg(operands);
    JITArg dstArg(operands + 1);

#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
    if (instr->opcode() == ByteCode::F32ConvertI32UOpcode || instr->opcode() == ByteCode::F64ConvertI32UOpcode) {
        sljit_emit_op1(compiler, SLJIT_MOV_U32, SLJIT_R0, 0, srcArg.arg, srcArg.argw);
        srcArg.arg = SLJIT_R0;
        srcArg.argw = 0;
    }
#endif /* SLJIT_64BIT_ARCHITECTURE */

    sljit_emit_fop1(compiler, opcode, dstArg.arg, dstArg.argw, srcArg.arg, srcArg.argw);
}

static void checkConvertResult(sljit_compiler* compiler)
{
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_R0, 0);

    sljit_jump* cmp = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, ExecutionContext::NoError);
    sljit_set_label(cmp, CompileContext::get(compiler)->trapLabel);
}

static void emitConvertFloat(sljit_compiler* compiler, Instruction* instr)
{
    uint32_t flags;
    sljit_sw addr = 0;

    switch (instr->opcode()) {
    case ByteCode::I32TruncF32SOpcode: {
        flags = SourceIsFloat;
        addr = GET_FUNC_ADDR(sljit_sw, convertF32ToS32);
        break;
    }
    case ByteCode::I32TruncF32UOpcode: {
        flags = SourceIsFloat;
        addr = GET_FUNC_ADDR(sljit_sw, convertF32ToU32);
        break;
    }
    case ByteCode::I32TruncF64SOpcode: {
        flags = SourceIsFloat | SourceIs64Bit;
        addr = GET_FUNC_ADDR(sljit_sw, convertF64ToS32);
        break;
    }
    case ByteCode::I32TruncF64UOpcode: {
        flags = SourceIsFloat | SourceIs64Bit;
        addr = GET_FUNC_ADDR(sljit_sw, convertF64ToU32);
        break;
    }
    case ByteCode::I64TruncF32SOpcode: {
        flags = SourceIsFloat | DestinationIs64Bit;
        addr = GET_FUNC_ADDR(sljit_sw, convertF32ToS64);
        break;
    }
    case ByteCode::I64TruncF32UOpcode: {
        flags = SourceIsFloat | DestinationIs64Bit;
        addr = GET_FUNC_ADDR(sljit_sw, convertF32ToU64);
        break;
    }
    case ByteCode::I64TruncF64SOpcode: {
        flags = SourceIsFloat | SourceIs64Bit | DestinationIs64Bit;
        addr = GET_FUNC_ADDR(sljit_sw, convertF64ToS64);
        break;
    }
    case ByteCode::I64TruncF64UOpcode: {
        flags = SourceIsFloat | SourceIs64Bit | DestinationIs64Bit;
        addr = GET_FUNC_ADDR(sljit_sw, convertF64ToU64);
        break;
    }
    case ByteCode::I32TruncSatF32SOpcode: {
        flags = SourceIsFloat | IsTruncSat;
        addr = GET_FUNC_ADDR(sljit_sw, truncSatF32ToS32);
        break;
    }
    case ByteCode::I32TruncSatF32UOpcode: {
        flags = SourceIsFloat | IsTruncSat;
        addr = GET_FUNC_ADDR(sljit_sw, truncSatF32ToU32);
        break;
    }
    case ByteCode::I32TruncSatF64SOpcode: {
        flags = SourceIsFloat | SourceIs64Bit | IsTruncSat;
        addr = GET_FUNC_ADDR(sljit_sw, truncSatF64ToS32);
        break;
    }
    case ByteCode::I32TruncSatF64UOpcode: {
        flags = SourceIsFloat | SourceIs64Bit | IsTruncSat;
        addr = GET_FUNC_ADDR(sljit_sw, truncSatF64ToU32);
        break;
    }
    case ByteCode::I64TruncSatF32SOpcode: {
        flags = SourceIsFloat | DestinationIs64Bit | IsTruncSat;
        addr = GET_FUNC_ADDR(sljit_sw, truncSatF32ToS64);
        break;
    }
    case ByteCode::I64TruncSatF32UOpcode: {
        flags = SourceIsFloat | DestinationIs64Bit | IsTruncSat;
        addr = GET_FUNC_ADDR(sljit_sw, truncSatF32ToU64);
        break;
    }
    case ByteCode::I64TruncSatF64SOpcode: {
        flags = SourceIsFloat | SourceIs64Bit | DestinationIs64Bit | IsTruncSat;
        addr = GET_FUNC_ADDR(sljit_sw, truncSatF64ToS64);
        break;
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
    ASSERT(operands[1].item == nullptr);

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
    argTypes |= (flags & IsTruncSat) ? SLJIT_ARG_RETURN(SLJIT_ARG_TYPE_VOID) : SLJIT_ARG_RETURN(SLJIT_ARG_TYPE_W);
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
