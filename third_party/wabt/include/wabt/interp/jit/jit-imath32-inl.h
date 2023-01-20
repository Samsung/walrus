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

/* Only included by jit-backend.cc */

struct JITArgPair {
  sljit_s32 arg1;
  sljit_sw arg1w;
  sljit_s32 arg2;
  sljit_sw arg2w;
};

static void operandToArgPair(Operand* operand, JITArgPair& arg) {
  assert((operand->location.value_info & LocationInfo::kSizeMask) == 2);

  switch (operand->location.type) {
    case Operand::Stack: {
      arg.arg1 = SLJIT_MEM1(kFrameReg);
      arg.arg1w = static_cast<sljit_sw>(operand->value);
      arg.arg2 = SLJIT_MEM1(kFrameReg);
      arg.arg2w = static_cast<sljit_sw>(operand->value + sizeof(sljit_s32));
      break;
    }
    case Operand::Register: {
      arg.arg1 = static_cast<sljit_s32>(operand->value & 0xffff);
      arg.arg1w = 0;
      arg.arg2 = static_cast<sljit_s32>(operand->value >> 16);
      arg.arg2w = 0;
      break;
    }
    default: {
      assert(operand->location.type == Operand::Immediate);
      arg.arg1 = SLJIT_IMM;
      arg.arg2 = SLJIT_IMM;
      assert(operand->item->group() == Instruction::Immediate);

      Instruction* instr = operand->item->asInstruction();

      arg.arg1w = static_cast<sljit_sw>(instr->value().value64);
      arg.arg2w = static_cast<sljit_sw>(instr->value().value64 >> 32);
      break;
    }
  }
}

static void emitImmediate(sljit_compiler* compiler, Instruction* instr) {
  Operand* result = instr->operands();

  if (result->location.type == Operand::Unused) {
    return;
  }

  sljit_sw imm;

  if ((result->location.value_info & LocationInfo::kSizeMask) == 1) {
    JITArg dst;
    operandToArg(result, dst);

    imm = static_cast<sljit_sw>(instr->value().value64);
    sljit_emit_op1(compiler, SLJIT_MOV, dst.arg, dst.argw, SLJIT_IMM, imm);
    return;
  }

  JITArgPair dst;
  operandToArgPair(result, dst);

  imm = static_cast<sljit_sw>(instr->value().value64);
  sljit_emit_op1(compiler, SLJIT_MOV, dst.arg1, dst.arg1w, SLJIT_IMM, imm);
  imm = static_cast<sljit_sw>(instr->value().value64 >> 32);
  sljit_emit_op1(compiler, SLJIT_MOV, dst.arg2, dst.arg2w, SLJIT_IMM, imm);
}

static void emitLocalMove(sljit_compiler* compiler, Instruction* instr) {
  Operand* operand = instr->operands();

  if (operand->location.type == Operand::Unused) {
    assert(!(instr->info() & Instruction::kKeepInstruction));
    return;
  }

  assert(instr->info() & Instruction::kKeepInstruction);

  if ((operand->location.value_info & LocationInfo::kSizeMask) == 1) {
    JITArg src, dst;

    if (instr->opcode() == Opcode::LocalGet) {
      operandToArg(operand, dst);
      src.arg = SLJIT_MEM1(kFrameReg);
      src.argw = static_cast<sljit_sw>(instr->value().value);
    } else {
      dst.arg = SLJIT_MEM1(kFrameReg);
      dst.argw = static_cast<sljit_sw>(instr->value().value);
      operandToArg(operand, src);
    }

    sljit_emit_op1(compiler, SLJIT_MOV, dst.arg, dst.argw, src.arg, src.argw);
    return;
  }

  JITArgPair src, dst;

  if (instr->opcode() == Opcode::LocalGet) {
    operandToArgPair(operand, dst);
    src.arg1 = SLJIT_MEM1(kFrameReg);
    src.arg1w = static_cast<sljit_sw>(instr->value().value);
    src.arg2 = SLJIT_MEM1(kFrameReg);
    src.arg2w = static_cast<sljit_sw>(instr->value().value + sizeof(sljit_s32));
  } else {
    dst.arg1 = SLJIT_MEM1(kFrameReg);
    dst.arg1w = static_cast<sljit_sw>(instr->value().value);
    dst.arg2 = SLJIT_MEM1(kFrameReg);
    dst.arg2w = static_cast<sljit_sw>(instr->value().value + sizeof(sljit_s32));
    operandToArgPair(operand, src);
  }

  if (!(dst.arg1 & src.arg1 & SLJIT_MEM)) {
    /* At least one if them is not memory. */
    sljit_emit_op1(compiler, SLJIT_MOV, dst.arg1, dst.arg1w, src.arg1,
                   src.arg1w);
    sljit_emit_op1(compiler, SLJIT_MOV, dst.arg2, dst.arg2w, src.arg2,
                   src.arg2w);
    return;
  }

  sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, src.arg1, src.arg1w);
  sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, src.arg2, src.arg2w);
  sljit_emit_op1(compiler, SLJIT_MOV, dst.arg1, dst.arg1w, SLJIT_R0, 0);
  sljit_emit_op1(compiler, SLJIT_MOV, dst.arg2, dst.arg2w, SLJIT_R1, 0);
}

static void emitSimpleBinary64(sljit_compiler* compiler,
                               sljit_s32 op1,
                               sljit_s32 op2,
                               JITArgPair* args) {
#if !(defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
  if (arg0arg1 & arg1arg1 & SLJIT_MEM) {
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, args[0].arg1,
                   args[0].arg1w);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, args[1].arg1,
                   args[1].arg1w);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, args[0].arg2,
                   args[0].arg2w);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R3, 0, args[1].arg2,
                   args[1].arg2w);

    sljit_emit_op2(compiler, op1, args[2].arg1, args[2].arg1w, SLJIT_R0, 0,
                   SLJIT_R1, 0);
    sljit_emit_op2(compiler, op2, args[2].arg2, args[2].arg2w, SLJIT_R2, 0,
                   SLJIT_R3, 0);
    return;
  }
#endif /* !SLJIT_CONFIG_X86_32 */

  if (args[1].arg1 & SLJIT_MEM) {
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, args[1].arg1,
                   args[1].arg1w);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, args[1].arg2,
                   args[1].arg2w);

    sljit_emit_op2(compiler, op1, args[2].arg1, args[2].arg1w, args[0].arg1,
                   args[0].arg1w, SLJIT_R0, 0);
    sljit_emit_op2(compiler, op2, args[2].arg2, args[2].arg2w, args[0].arg2,
                   args[0].arg2w, SLJIT_R1, 0);
    return;
  }

  if (args[0].arg1 & SLJIT_MEM) {
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, args[0].arg1,
                   args[0].arg1w);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, args[0].arg2,
                   args[0].arg2w);

    sljit_emit_op2(compiler, op1, args[2].arg1, args[2].arg1w, SLJIT_R0, 0,
                   args[1].arg1, args[1].arg1w);
    sljit_emit_op2(compiler, op2, args[2].arg2, args[2].arg2w, SLJIT_R1, 0,
                   args[1].arg2, args[1].arg2w);
    return;
  }

  sljit_emit_op2(compiler, op1, args[2].arg1, args[2].arg1w, args[0].arg1,
                 args[0].arg1w, args[1].arg1, args[1].arg1w);
  sljit_emit_op2(compiler, op2, args[2].arg2, args[2].arg2w, args[0].arg2,
                 args[0].arg2w, args[1].arg2, args[1].arg2w);
}

static void emitShift64(sljit_compiler* compiler,
                        sljit_s32 op,
                        JITArgPair* args) {
  sljit_s32 shift_into_reg, other_reg;
  sljit_s32 shift_into_arg, other_arg;
  sljit_s32 shift_into_result_arg, other_result_arg;
  sljit_sw shift_into_argw, other_argw;
  sljit_sw shift_into_result_argw, other_result_argw;

  if (op == SLJIT_SHL) {
    shift_into_reg = GET_TARGET_REG(args[2].arg2, SLJIT_R0);
    other_reg = GET_TARGET_REG(args[2].arg1, SLJIT_R1);

    shift_into_arg = args[0].arg2;
    shift_into_argw = args[0].arg2w;
    other_arg = args[0].arg1;
    other_argw = args[0].arg1w;

    shift_into_result_arg = args[2].arg2;
    shift_into_result_argw = args[2].arg2w;
    other_result_arg = args[2].arg1;
    other_result_argw = args[2].arg1w;
  } else {
    shift_into_reg = GET_TARGET_REG(args[2].arg1, SLJIT_R0);
    other_reg = GET_TARGET_REG(args[2].arg2, SLJIT_R1);

    shift_into_arg = args[0].arg1;
    shift_into_argw = args[0].arg1w;
    other_arg = args[0].arg2;
    other_argw = args[0].arg2w;

    shift_into_result_arg = args[2].arg1;
    shift_into_result_argw = args[2].arg1w;
    other_result_arg = args[2].arg2;
    other_result_argw = args[2].arg2w;
  }

  if (args[1].arg1 & SLJIT_IMM) {
    sljit_sw shift = (args[1].arg1w & 0x3f);

    if (shift & 0x20) {
      shift -= 0x20;

      if (op == SLJIT_ASHR) {
        MOVE_TO_REG(compiler, SLJIT_MOV, other_reg, other_arg, other_argw);
        other_arg = other_reg;
        other_argw = 0;
      }

      if (shift == 0) {
        sljit_emit_op1(compiler, SLJIT_MOV, shift_into_result_arg,
                       shift_into_result_argw, other_arg, other_argw);
      } else {
        sljit_emit_op2(compiler, op, shift_into_result_arg,
                       shift_into_result_argw, other_arg, other_argw, SLJIT_IMM,
                       shift);
      }

      if (op == SLJIT_ASHR) {
        sljit_emit_op2(compiler, op, other_result_arg, other_result_argw,
                       other_arg, other_argw, SLJIT_IMM, 31);
      } else {
        sljit_emit_op1(compiler, SLJIT_MOV, other_result_arg, other_result_argw,
                       SLJIT_IMM, 0);
      }
      return;
    }

    MOVE_TO_REG(compiler, SLJIT_MOV, shift_into_reg, shift_into_arg,
                shift_into_argw);
    MOVE_TO_REG(compiler, SLJIT_MOV, other_reg, other_arg, other_argw);
    sljit_emit_shift_into(compiler, (op == SLJIT_SHL ? SLJIT_SHL : SLJIT_LSHR),
                          shift_into_reg, other_reg, 0, SLJIT_IMM, shift);
    sljit_emit_op2(compiler, op, other_reg, 0, other_reg, 0, SLJIT_IMM, shift);
    MOVE_FROM_REG(compiler, SLJIT_MOV, shift_into_result_arg,
                  shift_into_result_argw, shift_into_reg);
    MOVE_FROM_REG(compiler, SLJIT_MOV, other_result_arg, other_result_argw,
                  other_reg);
    return;
  }

#ifdef SLJIT_PREF_SHIFT_REG
  sljit_s32 shift_reg = SLJIT_PREF_SHIFT_REG;
#else  /* SLJIT_PREF_SHIFT_REG */
  sljit_s32 shift_reg = GET_TARGET_REG(args[1].arg1, SLJIT_R2);
#endif /* !SLJIT_PREF_SHIFT_REG */

  MOVE_TO_REG(compiler, SLJIT_MOV, shift_reg, args[1].arg1, args[1].arg1w);
  MOVE_TO_REG(compiler, SLJIT_MOV, other_reg, other_arg, other_argw);
  sljit_emit_op2u(compiler, SLJIT_AND | SLJIT_SET_Z, shift_reg, 0, SLJIT_IMM,
                  0x20);

  sljit_jump* jump1 = sljit_emit_jump(compiler, SLJIT_NOT_ZERO);

  MOVE_TO_REG(compiler, SLJIT_MOV, shift_into_reg, shift_into_arg,
              shift_into_argw);
  sljit_emit_op2(compiler, SLJIT_AND, shift_reg, 0, shift_reg, 0, SLJIT_IMM,
                 0x1f);
  sljit_emit_shift_into(compiler, (op == SLJIT_SHL ? SLJIT_SHL : SLJIT_LSHR),
                        shift_into_reg, other_reg, 0, shift_reg, 0);
  sljit_emit_op2(compiler, op, other_result_arg, other_result_argw, other_reg,
                 0, shift_reg, 0);

  sljit_jump* jump2 = sljit_emit_jump(compiler, SLJIT_JUMP);

  sljit_set_label(jump1, sljit_emit_label(compiler));

  WABT_STATIC_ASSERT(SLJIT_MSHL == SLJIT_SHL + 1);
  sljit_emit_op2(compiler, op + 1, shift_into_reg, 0, other_reg, 0, shift_reg,
                 0);

  if (op == SLJIT_ASHR) {
    sljit_emit_op2(compiler, op, other_result_arg, other_result_argw, other_arg,
                   other_argw, SLJIT_IMM, 31);
  } else {
    sljit_emit_op1(compiler, SLJIT_MOV, other_result_arg, other_result_argw,
                   SLJIT_IMM, 0);
  }

  sljit_set_label(jump2, sljit_emit_label(compiler));
  MOVE_FROM_REG(compiler, SLJIT_MOV, shift_into_result_arg,
                shift_into_result_argw, shift_into_reg);
}

static void emitRotate64(sljit_compiler* compiler,
                         sljit_s32 op,
                         JITArgPair* args) {
  sljit_s32 reg1 = GET_TARGET_REG(args[2].arg1, SLJIT_R0);
  sljit_s32 reg2 = GET_TARGET_REG(args[2].arg2, SLJIT_R1);

  if (args[1].arg1 & SLJIT_IMM) {
    sljit_sw rotate = (args[1].arg1w & 0x3f);

    if (rotate & 0x20) {
      rotate -= 0x20;
      sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, args[0].arg2,
                     args[0].arg2w);
      MOVE_TO_REG(compiler, SLJIT_MOV, reg2, args[0].arg1, args[0].arg1w);
      sljit_emit_op1(compiler, SLJIT_MOV, reg1, 0, SLJIT_R2, 0);
    } else {
      sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, args[0].arg1,
                     args[0].arg1w);
      MOVE_TO_REG(compiler, SLJIT_MOV, reg2, args[0].arg2, args[0].arg2w);
      sljit_emit_op1(compiler, SLJIT_MOV, reg1, 0, SLJIT_R2, 0);
    }

    sljit_emit_shift_into(compiler, op, reg1, reg2, 0, SLJIT_IMM, rotate);
    sljit_emit_shift_into(compiler, op, reg2, SLJIT_R2, 0, SLJIT_IMM, rotate);

    MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg1, args[2].arg1w, reg1);
    MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg2, args[2].arg2w, reg2);
    return;
  }

#ifdef SLJIT_PREF_SHIFT_REG
  sljit_s32 rotate_reg = SLJIT_PREF_SHIFT_REG;
#if SLJIT_PREF_SHIFT_REG == SLJIT_R2
  sljit_s32 tmp_reg = SLJIT_R3;
#else /* SLJIT_PREF_SHIFT_REG != SLJIT_R2 */
#error "Unknown shift register"
#endif /* SLJIT_PREF_SHIFT_REG == SLJIT_R2 */
#else  /* SLJIT_PREF_SHIFT_REG */
  sljit_s32 rotate_reg = GET_TARGET_REG(args[1].arg1, SLJIT_R2);
  sljit_s32 tmp_reg = (rotate_reg == SLJIT_R2) ? SLJIT_R3 : SLJIT_R2;
#endif /* !SLJIT_PREF_SHIFT_REG */

  MOVE_TO_REG(compiler, SLJIT_MOV, rotate_reg, args[1].arg1, args[1].arg1w);
  MOVE_TO_REG(compiler, SLJIT_MOV, reg1, args[0].arg1, args[0].arg1w);
  MOVE_TO_REG(compiler, SLJIT_MOV, reg2, args[0].arg2, args[0].arg2w);

  sljit_emit_op2u(compiler, SLJIT_AND | SLJIT_SET_Z, rotate_reg, 0, SLJIT_IMM,
                  0x20);
  sljit_jump* jump1 = sljit_emit_jump(compiler, SLJIT_ZERO);
  /* Swap registers. */
  sljit_emit_op2(compiler, SLJIT_XOR, reg1, 0, reg1, 0, reg2, 0);
  sljit_emit_op2(compiler, SLJIT_XOR, reg2, 0, reg2, 0, reg1, 0);
  sljit_emit_op2(compiler, SLJIT_XOR, reg1, 0, reg1, 0, reg2, 0);
  sljit_set_label(jump1, sljit_emit_label(compiler));

  sljit_emit_op2(compiler, SLJIT_AND, rotate_reg, 0, rotate_reg, 0, SLJIT_IMM,
                 0x1f);
  sljit_emit_op1(compiler, SLJIT_MOV, tmp_reg, 0, reg1, 0);
  sljit_emit_shift_into(compiler, op, reg1, reg2, 0, rotate_reg, 0);
  sljit_emit_shift_into(compiler, op, reg2, tmp_reg, 0, rotate_reg, 0);

  MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg1, args[2].arg1w, reg1);
  MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg2, args[2].arg2w, reg2);
}

static void emitMul64(sljit_compiler* compiler, JITArgPair* args) {
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
  sljit_s32 tmp_reg = SLJIT_S2;
#else
  sljit_s32 tmp_reg = SLJIT_R3;
#endif

  MOVE_TO_REG(compiler, SLJIT_MOV, SLJIT_R0, args[0].arg1, args[0].arg1w);
  MOVE_TO_REG(compiler, SLJIT_MOV, SLJIT_R1, args[1].arg1, args[1].arg1w);
  MOVE_TO_REG(compiler, SLJIT_MOV, SLJIT_R2, args[0].arg2, args[0].arg2w);
  MOVE_TO_REG(compiler, SLJIT_MOV, tmp_reg, args[1].arg2, args[1].arg2w);
  sljit_emit_op2(compiler, SLJIT_MUL, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_R1, 0);
  sljit_emit_op2(compiler, SLJIT_MUL, tmp_reg, 0, tmp_reg, 0, SLJIT_R0, 0);
  sljit_emit_op0(compiler, SLJIT_LMUL_UW);
  sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R2, 0, SLJIT_R2, 0, tmp_reg, 0);
  sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_R2, 0);
  MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg1, args[2].arg1w, SLJIT_R0);
  MOVE_FROM_REG(compiler, SLJIT_MOV, args[2].arg2, args[2].arg2w, SLJIT_R1);
}

static void emitBinary(sljit_compiler* compiler, Instruction* instr) {
  Operand* operands = instr->operands();

  if ((operands->location.value_info & LocationInfo::kSizeMask) == 1) {
    JITArg args[3];

    for (int i = 0; i < 3; ++i) {
      operandToArg(operands + i, args[i]);
    }

    sljit_s32 opcode;

    switch (instr->opcode()) {
      case Opcode::I32Add:
        opcode = SLJIT_ADD;
        break;
      case Opcode::I32Sub:
        opcode = SLJIT_SUB;
        break;
      case Opcode::I32Mul:
        opcode = SLJIT_MUL;
        break;
      case Opcode::I32DivS:
      case Opcode::I32DivU:
      case Opcode::I32RemS:
      case Opcode::I32RemU:
        // Not supported yet.
        return;
      case Opcode::I32Rotl:
        opcode = SLJIT_ROTL;
        break;
      case Opcode::I32Rotr:
        opcode = SLJIT_ROTR;
        break;
      case Opcode::I32And:
        opcode = SLJIT_AND;
        break;
      case Opcode::I32Or:
        opcode = SLJIT_OR;
        break;
      case Opcode::I32Xor:
        opcode = SLJIT_XOR;
        break;
      case Opcode::I32Shl:
        opcode = SLJIT_SHL;
        break;
      case Opcode::I32ShrS:
        opcode = SLJIT_ASHR;
        break;
      case Opcode::I32ShrU:
        opcode = SLJIT_LSHR;
        break;
      default:
        WABT_UNREACHABLE;
        break;
    }

    sljit_emit_op2(compiler, opcode, args[2].arg, args[2].argw, args[0].arg,
                   args[0].argw, args[1].arg, args[1].argw);
    return;
  }

  JITArgPair args[3];

  for (int i = 0; i < 3; ++i) {
    operandToArgPair(operands + i, args[i]);
  }

  switch (instr->opcode()) {
    case Opcode::I64Add:
      emitSimpleBinary64(compiler, SLJIT_ADD | SLJIT_SET_CARRY, SLJIT_ADDC,
                         args);
      return;
    case Opcode::I64Sub:
      emitSimpleBinary64(compiler, SLJIT_SUB | SLJIT_SET_CARRY, SLJIT_SUBC,
                         args);
      return;
    case Opcode::I64Mul:
      emitMul64(compiler, args);
      return;
    case Opcode::I64DivS:
    case Opcode::I64DivU:
    case Opcode::I64RemS:
    case Opcode::I64RemU:
      // Not supported yet.
      return;
    case Opcode::I64Rotl:
      emitRotate64(compiler, SLJIT_SHL, args);
      return;
    case Opcode::I64Rotr:
      emitRotate64(compiler, SLJIT_LSHR, args);
      return;
    case Opcode::I64And:
      emitSimpleBinary64(compiler, SLJIT_AND, SLJIT_AND, args);
      return;
    case Opcode::I64Or:
      emitSimpleBinary64(compiler, SLJIT_OR, SLJIT_OR, args);
      return;
    case Opcode::I64Xor:
      emitSimpleBinary64(compiler, SLJIT_XOR, SLJIT_XOR, args);
      return;
    case Opcode::I64Shl:
      emitShift64(compiler, SLJIT_SHL, args);
      return;
    case Opcode::I64ShrS:
      emitShift64(compiler, SLJIT_ASHR, args);
      return;
    case Opcode::I64ShrU:
      emitShift64(compiler, SLJIT_LSHR, args);
      return;
    default:
      WABT_UNREACHABLE;
      break;
  }
}

static void emitCountZeroes(sljit_compiler* compiler,
                            sljit_s32 op,
                            JITArgPair* args) {
  sljit_s32 result_reg;

  result_reg = GET_TARGET_REG(args[1].arg1, SLJIT_R1);

  if (op == SLJIT_CLZ) {
    MOVE_TO_REG(compiler, SLJIT_MOV, result_reg, args[0].arg2, args[0].arg2w);
  } else {
    MOVE_TO_REG(compiler, SLJIT_MOV, result_reg, args[0].arg1, args[0].arg1w);
  }

  sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, 0);
  sljit_jump* jump =
      sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, result_reg, 0, SLJIT_IMM, 0);

  if (op == SLJIT_CLZ) {
    MOVE_TO_REG(compiler, SLJIT_MOV, result_reg, args[0].arg1, args[0].arg1w);
  } else {
    MOVE_TO_REG(compiler, SLJIT_MOV, result_reg, args[0].arg2, args[0].arg2w);
  }

  sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, 32);
  sljit_set_label(jump, sljit_emit_label(compiler));

  sljit_emit_op1(compiler, op, result_reg, 0, result_reg, 0);
  sljit_emit_op2(compiler, SLJIT_ADD, result_reg, 0, result_reg, 0, SLJIT_R0,
                 0);

  MOVE_FROM_REG(compiler, SLJIT_MOV, args[1].arg1, args[1].arg1w, result_reg);
  sljit_emit_op1(compiler, SLJIT_MOV, args[1].arg2, args[1].arg2w, SLJIT_IMM,
                 0);
}

static void emitExtend(sljit_compiler* compiler,
                       sljit_s32 opcode,
                       JITArg* args) {
  sljit_s32 reg = GET_TARGET_REG(args[1].arg, SLJIT_R0);

  if (args[0].arg & SLJIT_MEM) {
    sljit_emit_op1(compiler, SLJIT_MOV, reg, 0, args[0].arg, args[0].argw);
    args[0].arg = reg;
    args[0].argw = 0;
  }

  sljit_emit_op1(compiler, opcode, reg, 0, args[0].arg, args[0].argw);
  MOVE_FROM_REG(compiler, SLJIT_MOV, args[1].arg, args[1].argw, reg);
}

static void emitExtend64(sljit_compiler* compiler,
                         sljit_s32 opcode,
                         JITArgPair* args) {
  sljit_s32 reg = GET_TARGET_REG(args[1].arg1, SLJIT_R0);

  if (args[0].arg1 & SLJIT_MEM) {
    sljit_emit_op1(compiler, SLJIT_MOV, reg, 0, args[0].arg1, args[0].arg1w);
    args[0].arg1 = reg;
    args[0].arg1w = 0;
  }

  sljit_emit_op1(compiler, opcode, reg, 0, args[0].arg1, args[0].arg1w);
  MOVE_FROM_REG(compiler, SLJIT_MOV, args[1].arg1, args[1].arg1w, reg);

  if (args[1].arg1 & SLJIT_MEM) {
    sljit_emit_op2(compiler, SLJIT_ASHR, reg, 0, reg, 0, SLJIT_IMM, 31);
    sljit_emit_op1(compiler, SLJIT_MOV, args[1].arg2, args[1].arg2w, reg, 0);
    return;
  }

  sljit_emit_op2(compiler, SLJIT_ASHR, args[1].arg2, args[1].arg2w, reg, 0,
                 SLJIT_IMM, 31);
}

static void emitUnary(sljit_compiler* compiler, Instruction* instr) {
  Operand* operands = instr->operands();

  if ((operands->location.value_info & LocationInfo::kSizeMask) == 1) {
    JITArg args[2];

    for (int i = 0; i < 2; ++i) {
      operandToArg(operands + i, args[i]);
    }

    sljit_s32 opcode;

    switch (instr->opcode()) {
      case Opcode::I32Clz:
        opcode = SLJIT_CLZ;
        break;
      case Opcode::I32Ctz:
        opcode = SLJIT_CTZ;
        break;
      case Opcode::I32Popcnt:
        // Not supported yet.
        return;
      case Opcode::I32Extend8S:
        emitExtend(compiler, SLJIT_MOV_S8, args);
        return;
      case Opcode::I32Extend16S:
        emitExtend(compiler, SLJIT_MOV_S16, args);
        return;
      default:
        WABT_UNREACHABLE;
        break;
    }

    // If the operand is an immediate then it is necesarry to move it into a
    // register because immediate source arguments are not supported.
    if (args[0].arg & SLJIT_IMM) {
      sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, args[0].arg,
                     args[0].argw);
      args[0].arg = SLJIT_R0;
      args[0].argw = 0;
    }

    sljit_emit_op1(compiler, opcode, args[1].arg, args[1].argw, args[0].arg,
                   args[0].argw);
    return;
  }

  JITArgPair args[2];

  for (int i = 0; i < 2; ++i) {
    operandToArgPair(operands + i, args[i]);
  }

  switch (instr->opcode()) {
    case Opcode::I64Clz:
      emitCountZeroes(compiler, SLJIT_CLZ, args);
      return;
    case Opcode::I64Ctz:
      emitCountZeroes(compiler, SLJIT_CTZ, args);
      return;
    case Opcode::I64Popcnt:
      // Not supported yet.
      return;
    case Opcode::I64Extend8S:
      emitExtend64(compiler, SLJIT_MOV_S8, args);
      return;
    case Opcode::I64Extend16S:
      emitExtend64(compiler, SLJIT_MOV_S16, args);
      return;
    case Opcode::I64Extend32S:
      if (args[0].arg1 == args[1].arg1 && args[0].arg1w == args[1].arg1w) {
        sljit_emit_op2(compiler, SLJIT_ASHR, args[1].arg2, args[1].arg2w,
                       args[0].arg1, args[0].arg1w, SLJIT_IMM, 31);
        return;
      }
      emitExtend64(compiler, SLJIT_MOV, args);
      return;
    default:
      WABT_UNREACHABLE;
      break;
  }
}

static bool emitCompare(sljit_compiler* compiler, Instruction* instr) {
  Operand* operand = instr->operands();
  sljit_s32 opcode, type;

  switch (instr->opcode()) {
    case Opcode::I32Eqz:
    case Opcode::I64Eqz:
      opcode = SLJIT_SUB | SLJIT_SET_Z;
      type = SLJIT_EQUAL;
      break;
    case Opcode::I32Eq:
    case Opcode::I64Eq:
      opcode = SLJIT_SUB | SLJIT_SET_Z;
      type = SLJIT_EQUAL;
      break;
    case Opcode::I32Ne:
    case Opcode::I64Ne:
      opcode = SLJIT_SUB | SLJIT_SET_Z;
      type = SLJIT_NOT_EQUAL;
      break;
    case Opcode::I32LtS:
    case Opcode::I64LtS:
      opcode = SLJIT_SUB | SLJIT_SET_SIG_LESS;
      type = SLJIT_SIG_LESS;
      break;
    case Opcode::I32LtU:
    case Opcode::I64LtU:
      opcode = SLJIT_SUB | SLJIT_SET_LESS;
      type = SLJIT_LESS;
      break;
    case Opcode::I32GtS:
    case Opcode::I64GtS:
      opcode = SLJIT_SUB | SLJIT_SET_SIG_GREATER;
      type = SLJIT_SIG_GREATER;
      break;
    case Opcode::I32GtU:
    case Opcode::I64GtU:
      opcode = SLJIT_SUB | SLJIT_SET_GREATER;
      type = SLJIT_GREATER;
      break;
    case Opcode::I32LeS:
    case Opcode::I64LeS:
      opcode = SLJIT_SUB | SLJIT_SET_SIG_LESS_EQUAL;
      type = SLJIT_SIG_LESS_EQUAL;
      break;
    case Opcode::I32LeU:
    case Opcode::I64LeU:
      opcode = SLJIT_SUB | SLJIT_SET_LESS_EQUAL;
      type = SLJIT_LESS_EQUAL;
      break;
    case Opcode::I32GeS:
    case Opcode::I64GeS:
      opcode = SLJIT_SUB | SLJIT_SET_SIG_GREATER_EQUAL;
      type = SLJIT_SIG_GREATER_EQUAL;
      break;
    case Opcode::I32GeU:
    case Opcode::I64GeU:
      opcode = SLJIT_SUB | SLJIT_SET_GREATER_EQUAL;
      type = SLJIT_GREATER_EQUAL;
      break;
    default:
      WABT_UNREACHABLE;
      break;
  }

  if ((operand->location.value_info & LocationInfo::kSizeMask) == 2) {
    JITArgPair params[2];

    for (Index i = 0; i < instr->paramCount(); ++i) {
      operandToArgPair(operand, params[i]);
      operand++;
    }

    Instruction* next_instr = nullptr;
    if (operand->location.type == Operand::Unused) {
      next_instr = instr->next()->asInstruction();

      assert(next_instr->opcode() == Opcode::BrIf ||
             next_instr->opcode() == Opcode::InterpBrUnless);

      if (next_instr->opcode() == Opcode::InterpBrUnless) {
        type ^= 0x1;

        WABT_STATIC_ASSERT((SLJIT_SET_LESS ^ SLJIT_SET(0x1)) ==
                           SLJIT_SET_GREATER_EQUAL);

        if (!(opcode & SLJIT_SET_Z)) {
          opcode ^= SLJIT_SET(0x1);
        }
      }
    }

    if (instr->opcode() == Opcode::I64Eqz) {
      sljit_emit_op2u(compiler, SLJIT_OR | SLJIT_SET_Z, params[0].arg1,
                      params[0].arg1w, params[0].arg2, params[0].arg2w);
    } else {
      sljit_emit_op2u(compiler, opcode | SLJIT_SET_Z, params[0].arg1,
                      params[0].arg1w, params[1].arg1, params[1].arg1w);
      sljit_jump* jump = sljit_emit_jump(compiler, SLJIT_NOT_ZERO);
      sljit_emit_op2u(compiler, opcode, params[0].arg2, params[0].arg2w,
                      params[1].arg2, params[1].arg2w);
      sljit_set_label(jump, sljit_emit_label(compiler));
      sljit_set_current_flags(compiler,
                              (opcode - SLJIT_SUB) | SLJIT_CURRENT_FLAGS_SUB);
    }

    if (next_instr == nullptr) {
      JITArg result;

      operandToArg(operand, result);
      sljit_emit_op_flags(compiler, SLJIT_MOV, result.arg, result.argw, type);
      return false;
    }

    sljit_jump* jump = sljit_emit_jump(compiler, type);
    next_instr->value().target_label->jumpFrom(jump);
    return true;
  }

  JITArg params[2];

  for (Index i = 0; i < instr->paramCount(); ++i) {
    operandToArg(operand, params[i]);
    operand++;
  }

  if (instr->opcode() == Opcode::I32Eqz) {
    params[1].arg = SLJIT_IMM;
    params[1].argw = 0;
  }

  if (operand->location.type != Operand::Unused) {
    JITArg result;

    sljit_emit_op2u(compiler, opcode, params[0].arg, params[0].argw,
                    params[1].arg, params[1].argw);
    operandToArg(operand, result);
    sljit_emit_op_flags(compiler, SLJIT_MOV, result.arg, result.argw, type);
    return false;
  }

  Instruction* next_instr = instr->next()->asInstruction();

  assert(next_instr->opcode() == Opcode::BrIf ||
         next_instr->opcode() == Opcode::InterpBrUnless);

  if (next_instr->opcode() == Opcode::InterpBrUnless) {
    type ^= 0x1;
  }

  sljit_jump* jump =
      sljit_emit_cmp(compiler, type, params[0].arg, params[0].argw,
                     params[1].arg, params[1].argw);
  next_instr->value().target_label->jumpFrom(jump);
  return true;
}

static void emitConvert(sljit_compiler* compiler, Instruction* instr) {
  Operand* operand = instr->operands();

  switch (instr->opcode()) {
    case Opcode::I32WrapI64: {
      /* Just copy the lower word. */
      JITArgPair param;
      JITArg result;

      operandToArgPair(operand, param);
      operandToArg(operand + 1, result);

      sljit_emit_op1(compiler, SLJIT_MOV, result.arg, result.argw, param.arg1,
                     param.arg1w);
      return;
    }
    case Opcode::I64ExtendI32S: {
      JITArg param;
      JITArgPair result;

      operandToArg(operand, param);
      operandToArgPair(operand + 1, result);

      sljit_s32 param_reg = GET_TARGET_REG(param.arg, SLJIT_R0);
      MOVE_TO_REG(compiler, SLJIT_MOV, param_reg, param.arg, param.argw);
      MOVE_FROM_REG(compiler, SLJIT_MOV, result.arg1, result.arg1w, param_reg);
      sljit_emit_op2(compiler, SLJIT_ASHR, result.arg2, result.arg2w, param_reg,
                     0, SLJIT_IMM, 31);
      return;
    }
    case Opcode::I64ExtendI32U: {
      JITArg param;
      JITArgPair result;

      operandToArg(operand, param);
      operandToArgPair(operand + 1, result);

      sljit_emit_op1(compiler, SLJIT_MOV, result.arg1, result.arg1w, param.arg,
                     param.argw);
      sljit_emit_op1(compiler, SLJIT_MOV, result.arg2, result.arg2w, SLJIT_IMM,
                     0);
      return;
    }
    default: {
      WABT_UNREACHABLE;
      break;
    }
  }
}
