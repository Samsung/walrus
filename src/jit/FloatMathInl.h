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

using f32Function2Param = std::add_pointer<sljit_f32(sljit_f32, sljit_f32)>::type;
using f64Function2Param = std::add_pointer<sljit_f64(sljit_f64, sljit_f64)>::type;
using f32Function1Param = std::add_pointer<sljit_f32(sljit_f32)>::type;
using f64Function1Param = std::add_pointer<sljit_f64(sljit_f64)>::type;

#define MOVE_FROM_FREG(compiler, movOp, arg, argw, sourceReg)            \
    if ((sourceReg) != (arg)) {                                          \
        sljit_emit_fop1(compiler, movOp, (arg), (argw), (sourceReg), 0); \
    }
#define MOVE_TO_FREG(compiler, movOp, targetReg, arg, argw)              \
    if ((targetReg) != (arg)) {                                          \
        sljit_emit_fop1(compiler, movOp, (targetReg), 0, (arg), (argw)); \
    }

static void floatOperandToArg(sljit_compiler* compiler, Operand* operand, JITArg& arg, sljit_s32 srcReg)
{
    if (operand->location.type != Operand::Immediate) {
        operandToArg(operand, arg);
        return;
    }

    Instruction* instr = operand->item->asInstruction();
    ASSERT(srcReg != 0);

    arg.arg = srcReg;
    arg.argw = 0;

    if ((operand->location.valueInfo & LocationInfo::kSizeMask) == 1) {
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, static_cast<sljit_s32>(instr->value().value32));
        sljit_emit_fcopy(compiler, SLJIT_COPY32_TO_F32, arg.arg, SLJIT_R0);
    } else {
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, static_cast<sljit_sw>(instr->value().value64 >> 32));
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_IMM, static_cast<sljit_sw>(instr->value().value64));
        sljit_emit_fcopy(compiler, SLJIT_COPY_TO_F64, arg.arg, SLJIT_REG_PAIR(SLJIT_R0, SLJIT_R1));
#else /* !SLJIT_32BIT_ARCHITECTURE */
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, static_cast<sljit_sw>(instr->value().value64));
        sljit_emit_fcopy(compiler, SLJIT_COPY_TO_F64, arg.arg, SLJIT_R0);
#endif /* SLJIT_32BIT_ARCHITECTURE */
    }
}

// Float operations.
// TODO Canonical NaN
static sljit_f32 floatNearest(sljit_f32 val)
{
    return std::nearbyint(val);
}

static sljit_f64 floatNearest(sljit_f64 val)
{
    return std::nearbyint(val);
}

static sljit_f32 floatMin(sljit_f32 lhs, sljit_f32 rhs)
{
    if (WABT_UNLIKELY(std::isnan(lhs) || std::isnan(rhs))) {
        return std::numeric_limits<float>::quiet_NaN();
    } else if (WABT_UNLIKELY(lhs == 0 && rhs == 0)) {
        return std::signbit(lhs) ? lhs : rhs;
    }

    return std::min(lhs, rhs);
}

static sljit_f64 floatMin(sljit_f64 lhs, sljit_f64 rhs)
{
    if (WABT_UNLIKELY(std::isnan(lhs) || std::isnan(rhs))) {
        return std::numeric_limits<double>::quiet_NaN();
    } else if (WABT_UNLIKELY(lhs == 0 && rhs == 0)) {
        return std::signbit(lhs) ? lhs : rhs;
    }

    return std::min(lhs, rhs);
}

static sljit_f32 floatMax(sljit_f32 lhs, sljit_f32 rhs)
{
    if (WABT_UNLIKELY(std::isnan(lhs) || std::isnan(rhs))) {
        return std::numeric_limits<float>::quiet_NaN();
    } else if (WABT_UNLIKELY(lhs == 0 && rhs == 0)) {
        return std::signbit(lhs) ? rhs : lhs;
    }

    return std::max(lhs, rhs);
}

static sljit_f64 floatMax(sljit_f64 lhs, sljit_f64 rhs)
{
    if (WABT_UNLIKELY(std::isnan(lhs) || std::isnan(rhs))) {
        return std::numeric_limits<double>::quiet_NaN();
    } else if (WABT_UNLIKELY(lhs == 0 && rhs == 0)) {
        return std::signbit(lhs) ? rhs : lhs;
    }

    return std::max(lhs, rhs);
}

static sljit_f32 floatCopySign(sljit_f32 lhs, sljit_f32 rhs)
{
    return std::copysign(lhs, rhs);
}

static sljit_f64 floatCopySign(sljit_f64 lhs, sljit_f64 rhs)
{
    return std::copysign(lhs, rhs);
}

static void emitFloatBinary(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[3];

    ASSERT(instr->paramCount() == 2 && instr->resultCount() == 1);

    floatOperandToArg(compiler, operands, args[0], SLJIT_FR0);
    floatOperandToArg(compiler, operands + 1, args[1], SLJIT_FR1);
    floatOperandToArg(compiler, operands + 2, args[2], 0);

    sljit_s32 opcode;
    f32Function2Param f32Func = nullptr;
    f64Function2Param f64Func = nullptr;

    switch (instr->opcode()) {
    case F32AddOpcode:
        opcode = SLJIT_ADD_F32;
        break;
    case F32SubOpcode:
        opcode = SLJIT_SUB_F32;
        break;
    case F32MulOpcode:
        opcode = SLJIT_MUL_F32;
        break;
    case F32DivOpcode:
        opcode = SLJIT_DIV_F32;
        break;
    case F32MaxOpcode:
        f32Func = floatMax;
        break;
    case F32MinOpcode:
        f32Func = floatMin;
        break;
    case F32CopysignOpcode:
        f32Func = floatCopySign;
        break;
    case F64AddOpcode:
        opcode = SLJIT_ADD_F64;
        break;
    case F64SubOpcode:
        opcode = SLJIT_SUB_F64;
        break;
    case F64MulOpcode:
        opcode = SLJIT_MUL_F64;
        break;
    case F64DivOpcode:
        opcode = SLJIT_DIV_F64;
        break;
    case F64MaxOpcode:
        f64Func = floatMax;
        break;
    case F64MinOpcode:
        f64Func = floatMin;
        break;
    case F64CopysignOpcode:
        f64Func = floatCopySign;
        break;
    default:
        WABT_UNREACHABLE;
    }

    if (f32Func) {
        MOVE_TO_FREG(compiler, SLJIT_MOV_F32, SLJIT_FR0, args[0].arg, args[0].argw);
        MOVE_TO_FREG(compiler, SLJIT_MOV_F32, SLJIT_FR1, args[1].arg, args[1].argw);
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS2(F32, F32, F32), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, f32Func));
        MOVE_FROM_FREG(compiler, SLJIT_MOV_F32, args[2].arg, args[2].argw, SLJIT_FR0);
    } else if (f64Func) {
        MOVE_TO_FREG(compiler, SLJIT_MOV_F64, SLJIT_FR0, args[0].arg, args[0].argw);
        MOVE_TO_FREG(compiler, SLJIT_MOV_F64, SLJIT_FR1, args[1].arg, args[1].argw);
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS2(F64, F64, F64), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, f64Func));
        MOVE_FROM_FREG(compiler, SLJIT_MOV_F64, args[2].arg, args[2].argw, SLJIT_FR0);
    } else {
        sljit_emit_fop2(compiler, opcode, args[2].arg, args[2].argw, args[0].arg, args[0].argw, args[1].arg, args[1].argw);
    }
}

static void emitFloatUnary(sljit_compiler* compiler, Instruction* instr)
{
    Operand* operands = instr->operands();
    JITArg args[2];

    ASSERT(instr->paramCount() == 1 && instr->resultCount() == 1);

    floatOperandToArg(compiler, operands, args[0], SLJIT_FR0);
    floatOperandToArg(compiler, operands + 1, args[1], 0);

    sljit_s32 opcode;
    f32Function1Param f32Func = nullptr;
    f64Function1Param f64Func = nullptr;

    switch (instr->opcode()) {
    case F32CeilOpcode:
        f32Func = ceil;
        break;
    case F32FloorOpcode:
        f32Func = floor;
        break;
    case F32TruncOpcode:
        f32Func = trunc;
        break;
    case F32NearestOpcode:
        f32Func = floatNearest;
        break;
    case F32SqrtOpcode:
        f32Func = sqrt;
        break;
    case F32NegOpcode:
        opcode = SLJIT_NEG_F32;
        break;
    case F32AbsOpcode:
        opcode = SLJIT_ABS_F32;
        break;
    case F64CeilOpcode:
        f64Func = ceil;
        break;
    case F64FloorOpcode:
        f64Func = floor;
        break;
    case F64TruncOpcode:
        f64Func = trunc;
        break;
    case F64NearestOpcode:
        f64Func = floatNearest;
        break;
    case F64SqrtOpcode:
        f64Func = sqrt;
        break;
    case F64NegOpcode:
        opcode = SLJIT_NEG_F64;
        break;
    case F64AbsOpcode:
        opcode = SLJIT_ABS_F64;
        break;
    default:
        WABT_UNREACHABLE;
    }

    if (f32Func) {
        MOVE_TO_FREG(compiler, SLJIT_MOV_F32, SLJIT_FR0, args[0].arg, args[0].argw);
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, f32Func));
        MOVE_FROM_FREG(compiler, SLJIT_MOV_F32, args[1].arg, args[1].argw, SLJIT_FR0);
    } else if (f64Func) {
        MOVE_TO_FREG(compiler, SLJIT_MOV_F32, SLJIT_FR0, args[0].arg, args[0].argw);
        sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS1(F64, F64), SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, f64Func));
        MOVE_FROM_FREG(compiler, SLJIT_MOV_F64, args[1].arg, args[1].argw, SLJIT_FR0);
    } else {
        sljit_emit_fop1(compiler, opcode, args[1].arg, args[1].argw, args[0].arg, 0);
    }
}

static void emitFloatSelect(sljit_compiler* compiler, Instruction* instr, sljit_s32 type)
{
    Operand* operands = instr->operands();
    bool is32 = (operands->location.valueInfo & LocationInfo::kSizeMask) == 1;
    sljit_s32 movOpcode = is32 ? SLJIT_MOV_F32 : SLJIT_MOV_F64;
    JITArg args[3];

    floatOperandToArg(compiler, operands + 0, args[0], SLJIT_FR0);
    floatOperandToArg(compiler, operands + 1, args[1], SLJIT_FR1);
    floatOperandToArg(compiler, operands + 3, args[2], 0);

    if (type == -1) {
        JITArg cond;

        operandToArg(operands + 2, cond);
        sljit_emit_op2u(compiler, SLJIT_SUB32 | SLJIT_SET_Z, cond.arg, cond.argw, SLJIT_IMM, 0);

        type = SLJIT_NOT_ZERO;
    }

    sljit_s32 targetReg = GET_TARGET_REG(args[2].arg, SLJIT_FR0);

    MOVE_TO_FREG(compiler, movOpcode, targetReg, args[0].arg, args[0].argw);
    sljit_jump* jump = sljit_emit_jump(compiler, type);
    MOVE_TO_FREG(compiler, movOpcode, targetReg, args[1].arg, args[1].argw);
    sljit_set_label(jump, sljit_emit_label(compiler));
    MOVE_FROM_FREG(compiler, movOpcode, args[2].arg, args[2].argw, targetReg);
}
