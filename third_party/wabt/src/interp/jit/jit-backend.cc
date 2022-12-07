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

#include "wabt/interp/jit/jit-backend.h"
#include "sljitLir.h"
#include "wabt/interp/jit/jit.h"

// Inlined platform independent assembler backend.

#if defined SLJIT_CONFIG_UNSUPPORTED && SLJIT_CONFIG_UNSUPPORTED
#error Unsupported architecture
#endif

namespace wabt {
namespace interp {

static const uint8_t kContextReg = SLJIT_S0;
static const uint8_t kFrameReg = SLJIT_S1;

struct JITArg {
  sljit_s32 arg;
  sljit_sw argw;
};

static void operandToArg(Operand* operand, JITArg& arg) {
  switch (operand->location.type) {
    case Operand::Stack: {
      arg.arg = SLJIT_MEM1(kFrameReg);
      arg.argw = static_cast<sljit_sw>(operand->value);
      break;
    }
    case Operand::Register: {
      arg.arg = static_cast<sljit_s32>(operand->value);
      arg.argw = 0;
      break;
    }
    default: {
      assert(operand->location.type == Operand::Immediate);
      arg.arg = SLJIT_IMM;
      assert(operand->item->group() == Instruction::Immediate);

      Instruction* instr = operand->item->asInstruction();

      if ((operand->location.value_info & LocationInfo::kSizeMask) == 1) {
        arg.argw = static_cast<sljit_s32>(instr->value().value32);
      } else {
        arg.argw = static_cast<sljit_sw>(instr->value().value64);
      }
      break;
    }
  }
}

static void emitImmediate(sljit_compiler* compiler, Instruction* instr) {
  Operand* result = instr->operands();

  if (result->location.type == Operand::Unused) {
    return;
  }

  Operand param;
  param.item = instr;
  param.location.type = Operand::Immediate;
  param.location.value_info = result->location.value_info;

  JITArg src, dst;
  operandToArg(&param, src);
  operandToArg(result, dst);

  sljit_s32 opcode = SLJIT_MOV;

  if ((param.location.value_info & LocationInfo::kSizeMask) == 1) {
    opcode = SLJIT_MOV32;
  }

  sljit_emit_op1(compiler, opcode, dst.arg, dst.argw, src.arg, src.argw);
}

static void emitLocalMove(sljit_compiler* compiler, Instruction* instr) {
  Operand* operand = instr->operands();

  if (operand->location.type == Operand::Unused) {
    assert(!(instr->info() & Instruction::kKeepInstruction));
    return;
  }

  assert(instr->info() & Instruction::kKeepInstruction);

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

  sljit_s32 opcode = SLJIT_MOV;

  if ((operand->location.value_info & LocationInfo::kSizeMask) == 1) {
    opcode = SLJIT_MOV32;
  }

  sljit_emit_op1(compiler, opcode, dst.arg, dst.argw, src.arg, src.argw);
}

static void emitBinary(sljit_compiler* compiler, Instruction* instr) {
  Operand* operands = instr->operands();
  JITArg args[3];

  for (int i = 0; i < 3; ++i) {
    operandToArg(operands + i, args[i]);
  }

  sljit_s32 opcode;

  switch (instr->opcode()) {
    case Opcode::I32Add:
      opcode = SLJIT_ADD32;
      break;
    case Opcode::I32Sub:
      opcode = SLJIT_SUB32;
      break;
    case Opcode::I32Mul:
      opcode = SLJIT_MUL32;
      break;
    case Opcode::I32DivS:
    case Opcode::I32DivU:
    case Opcode::I32RemS:
    case Opcode::I32RemU:
      return;
    case Opcode::I32Rotl:
      opcode = SLJIT_ROTL32;
      break;
    case Opcode::I32Rotr:
      opcode = SLJIT_ROTR32;
      break;
    case Opcode::I32And:
      opcode = SLJIT_AND32;
      break;
    case Opcode::I32Or:
      opcode = SLJIT_OR32;
      break;
    case Opcode::I32Xor:
      opcode = SLJIT_XOR32;
      break;
    case Opcode::I32Shl:
      opcode = SLJIT_SHL32;
      break;
    case Opcode::I32ShrS:
      opcode = SLJIT_ASHR32;
      break;
    case Opcode::I32ShrU:
      opcode = SLJIT_LSHR32;
      break;
    case Opcode::I64Add:
      opcode = SLJIT_ADD;
      break;
    case Opcode::I64Sub:
      opcode = SLJIT_SUB;
      break;
    case Opcode::I64Mul:
      opcode = SLJIT_MUL;
      break;
    case Opcode::I64DivS:
    case Opcode::I64DivU:
    case Opcode::I64RemS:
    case Opcode::I64RemU:
      return;
    case Opcode::I64Rotl:
      opcode = SLJIT_ROTL;
      break;
    case Opcode::I64Rotr:
      opcode = SLJIT_ROTR;
      break;
    case Opcode::I64And:
      opcode = SLJIT_AND;
      break;
    case Opcode::I64Or:
      opcode = SLJIT_OR;
      break;
    case Opcode::I64Xor:
      opcode = SLJIT_XOR;
      break;
    case Opcode::I64Shl:
      opcode = SLJIT_SHL;
      break;
    case Opcode::I64ShrS:
      opcode = SLJIT_ASHR;
      break;
    case Opcode::I64ShrU:
      opcode = SLJIT_LSHR;
      break;
    default:
      WABT_UNREACHABLE;
      break;
  }

  sljit_emit_op2(compiler, opcode, args[2].arg, args[2].argw, args[0].arg,
                 args[0].argw, args[1].arg, args[1].argw);
}

static void emitUnary(sljit_compiler* compiler, Instruction* instr) {
  Operand* operand = instr->operands();
  JITArg args[2];

  for (int i = 0; i < 2; ++i) {
    operandToArg(operand + i, args[i]);
  }

  sljit_s32 opcode;

  switch (instr->opcode()) {
    case Opcode::I32Ctz:
      opcode = SLJIT_CTZ32;
      break;
    case Opcode::I32Clz:
      opcode = SLJIT_CLZ32;
      break;
    case Opcode::I64Ctz:
      opcode = SLJIT_CTZ;
      break;
    case Opcode::I64Clz:
      opcode = SLJIT_CLZ;
      break;
    case Opcode::I32Popcnt:
    case Opcode::I64Popcnt:
      // Not supported yet.
      return;
    default:
      WABT_UNREACHABLE;
      break;
  }

  // If the operand is an immidiate then it is necesarry to move it into a
  // register because immidiate source arguments are not supported.
  if (args[0].arg & SLJIT_IMM) {
    sljit_s32 mov = SLJIT_MOV;

    if ((operand->location.value_info & LocationInfo::kSizeMask) == 1) {
      mov = SLJIT_MOV32;
    }

    sljit_emit_op1(compiler, mov, SLJIT_R0, 0, args[0].arg, args[0].argw);
    args[0].arg = SLJIT_R0;
    args[0].argw = 0;
  }

  sljit_emit_op1(compiler, opcode, args[1].arg, args[1].argw, args[0].arg,
                 args[0].argw);
}

static bool emitCompare(sljit_compiler* compiler, Instruction* instr) {
  Operand* operand = instr->operands();
  JITArg params[2];

  for (Index i = 0; i < instr->paramCount(); ++i) {
    operandToArg(operand, params[i]);
    operand++;
  }

  sljit_s32 opcode;
  sljit_s32 type;

  switch (instr->opcode()) {
    case Opcode::I32Eqz:
    case Opcode::I64Eqz:
      opcode = SLJIT_SUB | SLJIT_SET_Z;
      type = SLJIT_EQUAL;
      params[1].arg = SLJIT_IMM;
      params[1].argw = 0;
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

  if (operand->location.type != Operand::Unused) {
    if ((operand[-1].location.value_info & LocationInfo::kSizeMask) == 1) {
      opcode |= SLJIT_32;
    }

    sljit_emit_op2u(compiler, opcode, params[0].arg, params[0].argw,
                    params[1].arg, params[1].argw);
    operandToArg(operand, params[0]);
    sljit_emit_op_flags(compiler, SLJIT_MOV32, params[0].arg, params[0].argw,
                        type);
    return false;
  }

  Instruction* next_instr = instr->next()->asInstruction();

  assert(next_instr->opcode() == Opcode::BrIf ||
         next_instr->opcode() == Opcode::InterpBrUnless);

  if (next_instr->opcode() == Opcode::InterpBrUnless) {
    type ^= 0x1;
  }

  if ((operand[-1].location.value_info & LocationInfo::kSizeMask) == 1) {
    type |= SLJIT_32;
  }

  sljit_jump* jump =
      sljit_emit_cmp(compiler, type, params[0].arg, params[0].argw,
                     params[1].arg, params[1].argw);
  next_instr->value().target_label->jumpFrom(jump);
  return true;
}

static void emitDirectBranch(sljit_compiler* compiler, Instruction* instr) {
  sljit_jump* jump;

  if (instr->opcode() == Opcode::Br) {
    jump = sljit_emit_jump(compiler, SLJIT_JUMP);
  } else {
    Operand* result = instr->operands();
    JITArg src;

    operandToArg(result, src);

    sljit_s32 type =
        (instr->opcode() == Opcode::BrIf) ? SLJIT_NOT_EQUAL : SLJIT_EQUAL;

    if ((result->location.value_info & LocationInfo::kSizeMask) == 1) {
      type |= SLJIT_32;
    }

    jump = sljit_emit_cmp(compiler, type, src.arg, src.argw, SLJIT_IMM, 0);
  }

  instr->value().target_label->jumpFrom(jump);
}

JITModuleDescriptor::~JITModuleDescriptor() {
  sljit_free_code(machine_code, nullptr);
}

struct LabelJumpList {
  std::vector<sljit_jump*> jump_list;
};

struct LabelData {
  LabelData(sljit_label* label) : label(label) {}

  sljit_label* label;
};

void Label::jumpFrom(sljit_jump* jump) {
  if (info() & Label::kHasLabelData) {
    sljit_set_label(jump, label_data_->label);
    return;
  }

  if (!(info() & Label::kHasJumpList)) {
    jump_list_ = new LabelJumpList;
    addInfo(Label::kHasJumpList);
  }

  jump_list_->jump_list.push_back(jump);
}

void Label::emit(sljit_compiler* compiler) {
  assert(!(info() & Label::kHasLabelData));

  sljit_label* label = sljit_emit_label(compiler);

  if (info() & Label::kHasJumpList) {
    for (auto it : jump_list_->jump_list) {
      sljit_set_label(it, label);
    }

    delete jump_list_;
    setInfo(info() ^ Label::kHasJumpList);
  }

  label_data_ = new LabelData(label);
  addInfo(Label::kHasLabelData);
}

void JITCompiler::compile() {
  compiler_ = sljit_create_compiler(nullptr, nullptr);

  size_t current_function = 0;

  for (InstructionListItem* item = function_list_first_; item != nullptr;
       item = item->next()) {
    if (item->isLabel()) {
      Label* label = item->asLabel();

      if (!(label->info() & Label::kNewFunction)) {
        label->emit(compiler_);
      } else {
        if (label->prev() != nullptr) {
          emitEpilog(current_function);
          current_function++;
        }
        emitProlog(current_function);
      }
      continue;
    }

    switch (item->group()) {
      case Instruction::Immediate: {
        emitImmediate(compiler_, item->asInstruction());
        break;
      }
      case Instruction::LocalMove: {
        emitLocalMove(compiler_, item->asInstruction());
        break;
      }
      case Instruction::DirectBranch: {
        emitDirectBranch(compiler_, item->asInstruction());
        break;
      }
      case Instruction::Binary: {
        emitBinary(compiler_, item->asInstruction());
        break;
      }
      case Instruction::Unary: {
        emitUnary(compiler_, item->asInstruction());
        break;
      }
      case Instruction::Compare: {
        if (emitCompare(compiler_, item->asInstruction())) {
          item = item->next();
        }
        break;
      }
      default: {
        switch (item->asInstruction()->opcode()) {
          case Opcode::Drop:
          case Opcode::Return: {
            break;
          }
          default: {
            WABT_UNREACHABLE;
            break;
          }
        }
        break;
      }
    }
  }

  emitEpilog(current_function);

  void* code = sljit_generate_code(compiler_);

  if (code != nullptr) {
    for (auto it : function_list_) {
      it.jit_func->func_entry_ = it.is_external ? code : nullptr;
    }
  }

  sljit_free_compiler(compiler_);
}

void JITCompiler::releaseFunctionList() {
  InstructionListItem* item = function_list_first_;

  while (item != nullptr) {
    if (item->isLabel()) {
      Label* label = item->asLabel();

      assert((label->info() & Label::kNewFunction) ||
             label->branches().size() > 0);

      if (label->info() & Label::kHasJumpList) {
        assert(!(label->info() & Label::kHasLabelData));
        delete label->jump_list_;
      } else if (label->info() & Label::kHasLabelData) {
        delete label->label_data_;
      }
    }

    InstructionListItem* next = item->next();

    assert(next == nullptr || next->prev_ == item);

    delete item;
    item = next;
  }

  function_list_.clear();
}

void JITCompiler::emitProlog(size_t index) {
  if (function_list_[index].is_external) {
    // Follows the declaration of FunctionDescriptor::ExternalDecl().
    // Context stored in SLJIT_S0 (kContextReg)
    // Frame stored in SLJIT_S1 (kFrameReg)
    sljit_emit_enter(compiler_, 0, SLJIT_ARGS2(VOID, P, P), 2, 2, 0, 0, 0);
    sljit_jump* call =
        sljit_emit_call(compiler_, SLJIT_CALL_REG_ARG, SLJIT_ARGS0(VOID));
    sljit_emit_return_void(compiler_);

    sljit_set_label(call, sljit_emit_label(compiler_));
  }

  function_list_[index].entry_label->emit(compiler_);

  sljit_emit_enter(compiler_, SLJIT_ENTER_REG_ARG | SLJIT_ENTER_KEEP(2),
                   SLJIT_ARGS0(VOID), SLJIT_NUMBER_OF_SCRATCH_REGISTERS, 2,
                   SLJIT_NUMBER_OF_SCRATCH_FLOAT_REGISTERS, 0,
                   sizeof(ExecutionContext::CallFrame));

  // Setup new frame.
  sljit_emit_op1(compiler_, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_MEM1(kContextReg),
                 offsetof(ExecutionContext, last_frame));
  sljit_emit_op1(compiler_, SLJIT_MOV_P, SLJIT_MEM1(kContextReg),
                 offsetof(ExecutionContext, last_frame), kContextReg, 0);
  sljit_emit_op1(compiler_, SLJIT_MOV_P, SLJIT_MEM1(SLJIT_SP),
                 offsetof(ExecutionContext::CallFrame, frame_start), SLJIT_S1,
                 0);
  sljit_emit_op1(compiler_, SLJIT_MOV_P, SLJIT_MEM1(SLJIT_SP),
                 offsetof(ExecutionContext::CallFrame, prev_frame), SLJIT_R0,
                 0);
}

void JITCompiler::emitEpilog(size_t index) {
  // Restore previous frame.
  sljit_emit_op1(compiler_, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_MEM1(SLJIT_SP),
                 offsetof(ExecutionContext::CallFrame, prev_frame));
  sljit_emit_op1(compiler_, SLJIT_MOV_P, SLJIT_MEM1(kContextReg),
                 offsetof(ExecutionContext, last_frame), SLJIT_R0, 0);

  sljit_emit_return_void(compiler_);
}

}  // namespace interp
}  // namespace wabt
