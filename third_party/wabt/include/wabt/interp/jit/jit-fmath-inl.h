/*
 * Copyright 2020 WebAssembly Community Group participants
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

using f32Function2Param =
    std::add_pointer<sljit_f32(sljit_f32, sljit_f32)>::type;
using f64Function2Param =
    std::add_pointer<sljit_f64(sljit_f64, sljit_f64)>::type;
using f32Function1Param = std::add_pointer<sljit_f32(sljit_f32)>::type;
using f64Function1Param = std::add_pointer<sljit_f64(sljit_f64)>::type;

#define MOVE_FROM_FREG(compiler, mov_op, arg, argw, source_reg)        \
  if ((source_reg) != (arg)) {                                         \
    sljit_emit_fop1(compiler, mov_op, (arg), (argw), (source_reg), 0); \
  }
#define MOVE_TO_FREG(compiler, mov_op, target_reg, arg, argw)          \
  if ((target_reg) != (arg)) {                                         \
    sljit_emit_fop1(compiler, mov_op, (target_reg), 0, (arg), (argw)); \
  }

static void floatOperandToArg(sljit_compiler* compiler,
                              Operand* operand,
                              JITArg& arg,
                              unsigned int id) {
  if (operand->location.type != Operand::Immediate) {
    operandToArg(operand, arg);
    return;
  }

  Instruction* instr = operand->item->asInstruction();
  arg.arg = id == 0 ? SLJIT_FR0 : SLJIT_FR1;

  if ((operand->location.value_info & LocationInfo::kSizeMask) == 1) {
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM,
                   static_cast<sljit_s32>(instr->value().value32));
    sljit_emit_fcopy(compiler, SLJIT_COPY32_TO_F32, arg.arg, SLJIT_R0);
  } else {
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_IMM,
                   static_cast<sljit_sw>(instr->value().value32));
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM,
                   static_cast<sljit_sw>(instr->value().value64 >> 32));
    sljit_emit_fcopy(compiler, SLJIT_COPY_TO_F64, arg.arg,
                     SLJIT_REG_PAIR(SLJIT_R0, SLJIT_R1));
#else  /* !SLJIT_32BIT_ARCHITECTURE */
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM,
                   static_cast<sljit_sw>(instr->value().value64));
    sljit_emit_fcopy(compiler, SLJIT_COPY_TO_F64, arg.arg, SLJIT_R0);
#endif /* SLJIT_32BIT_ARCHITECTURE */
  }
}

// Float operations.
// TODO Canonical NaN
static sljit_f32 floatNearest(sljit_f32 val) {
  return std::nearbyint(val);
}

static sljit_f64 floatNearest(sljit_f64 val) {
  return std::nearbyint(val);
}

static sljit_f32 floatMin(sljit_f32 lhs, sljit_f32 rhs) {
  if (WABT_UNLIKELY(std::isnan(lhs) || std::isnan(rhs))) {
    return std::numeric_limits<float>::quiet_NaN();
  } else if (WABT_UNLIKELY(lhs == 0 && rhs == 0)) {
    return std::signbit(lhs) ? lhs : rhs;
  }

  return std::min(lhs, rhs);
}

static sljit_f64 floatMin(sljit_f64 lhs, sljit_f64 rhs) {
  if (WABT_UNLIKELY(std::isnan(lhs) || std::isnan(rhs))) {
    return std::numeric_limits<double>::quiet_NaN();
  } else if (WABT_UNLIKELY(lhs == 0 && rhs == 0)) {
    return std::signbit(lhs) ? lhs : rhs;
  }

  return std::min(lhs, rhs);
}

static sljit_f32 floatMax(sljit_f32 lhs, sljit_f32 rhs) {
  if (WABT_UNLIKELY(std::isnan(lhs) || std::isnan(rhs))) {
    return std::numeric_limits<float>::quiet_NaN();
  } else if (WABT_UNLIKELY(lhs == 0 && rhs == 0)) {
    return std::signbit(lhs) ? rhs : lhs;
  }

  return std::max(lhs, rhs);
}

static sljit_f64 floatMax(sljit_f64 lhs, sljit_f64 rhs) {
  if (WABT_UNLIKELY(std::isnan(lhs) || std::isnan(rhs))) {
    return std::numeric_limits<double>::quiet_NaN();
  } else if (WABT_UNLIKELY(lhs == 0 && rhs == 0)) {
    return std::signbit(lhs) ? rhs : lhs;
  }

  return std::max(lhs, rhs);
}

static sljit_f32 floatCopySign(sljit_f32 lhs, sljit_f32 rhs) {
  return std::copysign(lhs, rhs);
}

static sljit_f64 floatCopySign(sljit_f64 lhs, sljit_f64 rhs) {
  return std::copysign(lhs, rhs);
}

static void emitFloatBinary(sljit_compiler* compiler, Instruction* instr) {
  Operand* operands = instr->operands();
  JITArg args[3];

  for (unsigned int i = 0; i < 3; ++i) {
    floatOperandToArg(compiler, operands + i, args[i], i);
  }

  sljit_s32 opcode;
  f32Function2Param f32Func = nullptr;
  f64Function2Param f64Func = nullptr;

  switch (instr->opcode()) {
    case Opcode::F32Add:
      opcode = SLJIT_ADD_F32;
      break;
    case Opcode::F32Sub:
      opcode = SLJIT_SUB_F32;
      break;
    case Opcode::F32Mul:
      opcode = SLJIT_MUL_F32;
      break;
    case Opcode::F32Div:
      opcode = SLJIT_DIV_F32;
      break;
    case Opcode::F32Max:
      f32Func = floatMax;
      break;
    case Opcode::F32Min:
      f32Func = floatMin;
      break;
    case Opcode::F32Copysign:
      f32Func = floatCopySign;
      break;
    case Opcode::F64Add:
      opcode = SLJIT_ADD_F64;
      break;
    case Opcode::F64Sub:
      opcode = SLJIT_SUB_F64;
      break;
    case Opcode::F64Mul:
      opcode = SLJIT_MUL_F64;
      break;
    case Opcode::F64Div:
      opcode = SLJIT_DIV_F64;
      break;
    case Opcode::F64Max:
      f64Func = floatMax;
      break;
    case Opcode::F64Min:
      f64Func = floatMin;
      break;
    case Opcode::F64Copysign:
      f64Func = floatCopySign;
      break;
    default:
      WABT_UNREACHABLE;
  }

  if (f32Func) {
    MOVE_TO_FREG(compiler, SLJIT_MOV_F32, SLJIT_FR0, args[0].arg, args[0].argw);
    MOVE_TO_FREG(compiler, SLJIT_MOV_F32, SLJIT_FR1, args[1].arg, args[1].argw);
    sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS2(F32, F32, F32),
                     SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, f32Func));
    MOVE_FROM_FREG(compiler, SLJIT_MOV_F32, args[2].arg, args[2].argw,
                   SLJIT_FR0);
  } else if (f64Func) {
    MOVE_TO_FREG(compiler, SLJIT_MOV_F64, SLJIT_FR0, args[0].arg, args[0].argw);
    MOVE_TO_FREG(compiler, SLJIT_MOV_F64, SLJIT_FR1, args[1].arg, args[1].argw);
    sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS2(F64, F64, F64),
                     SLJIT_IMM, GET_FUNC_ADDR(sljit_sw, f64Func));
    MOVE_FROM_FREG(compiler, SLJIT_MOV_F64, args[2].arg, args[2].argw,
                   SLJIT_FR0);
  } else {
    sljit_emit_fop2(compiler, opcode, args[2].arg, args[2].argw, args[0].arg,
                    args[0].argw, args[1].arg, args[1].argw);
  }
}

static void emitFloatUnary(sljit_compiler* compiler, Instruction* instr) {
  Operand* operand = instr->operands();
  JITArg args[2];

  for (int i = 0; i < 2; ++i) {
    floatOperandToArg(compiler, operand + i, args[i], i);
  }

  sljit_s32 opcode;
  f32Function1Param f32Func = nullptr;
  f64Function1Param f64Func = nullptr;

  switch (instr->opcode()) {
    case Opcode::F32Ceil:
      f32Func = ceil;
      break;
    case Opcode::F32Floor:
      f32Func = floor;
      break;
    case Opcode::F32Trunc:
      f32Func = trunc;
      break;
    case Opcode::F32Nearest:
      f32Func = floatNearest;
      break;
    case Opcode::F32Sqrt:
      f32Func = sqrt;
      break;
    case Opcode::F32Neg:
      opcode = SLJIT_NEG_F32;
      break;
    case Opcode::F32Abs:
      opcode = SLJIT_ABS_F32;
      break;
    case Opcode::F64Ceil:
      f64Func = ceil;
      break;
    case Opcode::F64Floor:
      f64Func = floor;
      break;
    case Opcode::F64Trunc:
      f64Func = trunc;
      break;
    case Opcode::F64Nearest:
      f64Func = floatNearest;
      break;
    case Opcode::F64Sqrt:
      f64Func = sqrt;
      break;
    case Opcode::F64Neg:
      opcode = SLJIT_NEG_F64;
      break;
    case Opcode::F64Abs:
      opcode = SLJIT_ABS_F64;
      break;
    default:
      WABT_UNREACHABLE;
  }

  if (f32Func) {
    MOVE_TO_FREG(compiler, SLJIT_MOV_F32, SLJIT_FR0, args[0].arg, args[0].argw);
    sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM,
                     GET_FUNC_ADDR(sljit_sw, f32Func));
    MOVE_FROM_FREG(compiler, SLJIT_MOV_F32, args[1].arg, args[1].argw,
                   SLJIT_FR0);
  } else if (f64Func) {
    MOVE_TO_FREG(compiler, SLJIT_MOV_F32, SLJIT_FR0, args[0].arg, args[0].argw);
    sljit_emit_icall(compiler, SLJIT_CALL, SLJIT_ARGS1(F64, F64), SLJIT_IMM,
                     GET_FUNC_ADDR(sljit_sw, f64Func));
    MOVE_FROM_FREG(compiler, SLJIT_MOV_F64, args[1].arg, args[1].argw,
                   SLJIT_FR0);
  } else {
    sljit_emit_fop1(compiler, opcode, args[1].arg, args[1].argw, args[0].arg,
                    0);
  }
}
