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
#include <math.h>
#include "sljitLir.h"
#include "wabt/interp/interp.h"
#include "wabt/interp/jit/jit.h"

#include <map>
#include <mutex>

// Inlined platform independent assembler backend.

#if defined SLJIT_CONFIG_UNSUPPORTED && SLJIT_CONFIG_UNSUPPORTED
#error Unsupported architecture
#endif

#define OffsetOfContextField(field)                           \
  (static_cast<sljit_sw>(offsetof(ExecutionContext, field)) - \
   static_cast<sljit_sw>(sizeof(ExecutionContext)))

#if !(defined SLJIT_INDIRECT_CALL && SLJIT_INDIRECT_CALL)
#define GET_FUNC_ADDR(type, func) (reinterpret_cast<type>(func))
#else
#define GET_FUNC_ADDR(type, func) (*reinterpret_cast<type*>(func))
#endif

namespace wabt {
namespace interp {

static const uint8_t kContextReg = SLJIT_S0;
static const uint8_t kFrameReg = SLJIT_S1;

struct JITArg {
  sljit_s32 arg;
  sljit_sw argw;
};

struct TrapBlock {
  TrapBlock(sljit_label* end_label, sljit_label* handler_label)
      : end_label(end_label), handler_label(handler_label) {}

  sljit_label* end_label;
  sljit_label* handler_label;
};

class SlowCase {
 public:
  enum class Type {
    SignedDivide,
    SignedDivide32,
  };

  SlowCase(Type type,
           sljit_jump* jump_from,
           sljit_label* resume_label,
           Instruction* instr)
      : type_(type),
        jump_from_(jump_from),
        resume_label_(resume_label),
        instr_(instr) {}

  virtual ~SlowCase() {}

  void emit(sljit_compiler* compiler);

 protected:
  Type type_;
  sljit_jump* jump_from_;
  sljit_label* resume_label_;
  Instruction* instr_;
};

struct CompileContext {
  CompileContext(JITCompiler* compiler)
      : compiler(compiler), frame_size(0), trap_label(nullptr) {}

  static CompileContext* get(sljit_compiler* compiler) {
    void* context = sljit_get_allocator_data(compiler);
    return reinterpret_cast<CompileContext*>(context);
  }

  void add(SlowCase* slow_case) { slow_cases.push_back(slow_case); }
  void emitSlowCases(sljit_compiler* compiler);

  JITCompiler* compiler;
  Index frame_size;
  sljit_label* trap_label;
  sljit_label* return_to_label;
  std::vector<TrapBlock> trap_blocks;
  std::vector<SlowCase*> slow_cases;
};

class TrapHandlerList {
 public:
  ~TrapHandlerList() {
    lock_.lock();
    destroyed_ = true;
    lock_.unlock();
  }

  void insert(std::vector<TrapBlock>& trap_blocks) {
    assert(!destroyed_);

    lock_.lock();
    for (auto it : trap_blocks) {
      sljit_uw end_addr = sljit_get_label_addr(it.end_label);
      trap_map_[end_addr] = sljit_get_label_addr(it.handler_label);
    }
    lock_.unlock();
  }

  sljit_uw get(sljit_uw return_addr) {
    assert(!destroyed_);
    lock_.lock();
    sljit_uw result = trap_map_.lower_bound(return_addr)->second;
    lock_.unlock();
    return result;
  }

  void erase(sljit_uw module_start, sljit_uw module_end) {
    if (!destroyed_) {
      lock_.lock();
      trap_map_.erase(trap_map_.lower_bound(module_start),
                      trap_map_.upper_bound(module_end));
      lock_.unlock();
    }
  }

 private:
  bool destroyed_ = false;
  std::map<sljit_uw, sljit_uw> trap_map_;
  std::mutex lock_;
};

static TrapHandlerList trap_handler_list;

static sljit_uw SLJIT_FUNC getTrapHandler(sljit_uw return_addr) {
  return trap_handler_list.get(return_addr);
}

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

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
      assert((operand->location.value_info & LocationInfo::kSizeMask) == 1);
      arg.argw = static_cast<sljit_s32>(instr->value().value32);
#else  /* !SLJIT_32BIT_ARCHITECTURE */
      if ((operand->location.value_info & LocationInfo::kSizeMask) == 1) {
        arg.argw = static_cast<sljit_s32>(instr->value().value32);
      } else {
        arg.argw = static_cast<sljit_sw>(instr->value().value64);
      }
#endif /* SLJIT_32BIT_ARCHITECTURE */
      break;
    }
  }
}

#define GET_TARGET_REG(arg, default_reg) \
  (((arg)&SLJIT_MEM) ? (default_reg) : (arg))
#define IS_SOURCE_REG(arg) (!((arg) & (SLJIT_MEM | SLJIT_IMM)))
#define GET_SOURCE_REG(arg, default_reg) \
  (IS_SOURCE_REG(arg) ? (arg) : (default_reg))
#define MOVE_TO_REG(compiler, mov_op, target_reg, arg, argw)          \
  if ((target_reg) != (arg)) {                                        \
    sljit_emit_op1(compiler, mov_op, (target_reg), 0, (arg), (argw)); \
  }
#define MOVE_FROM_REG(compiler, mov_op, arg, argw, source_reg)        \
  if ((source_reg) != (arg)) {                                        \
    sljit_emit_op1(compiler, mov_op, (arg), (argw), (source_reg), 0); \
  }

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
#include "wabt/interp/jit/jit-imath32-inl.h"
#else /* !SLJIT_32BIT_ARCHITECTURE */
#include "wabt/interp/jit/jit-imath64-inl.h"
#endif /* SLJIT_32BIT_ARCHITECTURE */

#include "wabt/interp/jit/jit-fmath-inl.h"

void CompileContext::emitSlowCases(sljit_compiler* compiler) {
  for (auto it : slow_cases) {
    SlowCase* slow_case = it;

    slow_case->emit(compiler);
    delete slow_case;
  }
  slow_cases.clear();
}

void SlowCase::emit(sljit_compiler* compiler) {
  sljit_set_label(jump_from_, sljit_emit_label(compiler));

  CompileContext* context = CompileContext::get(compiler);

  switch (type_) {
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
    case Type::SignedDivide32:
#endif /* SLJIT_64BIT_ARCHITECTURE */
    case Type::SignedDivide: {
      sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM,
                     ExecutionContext::DivisionError);

      sljit_s32 current_flags = SLJIT_CURRENT_FLAGS_SUB |
                                SLJIT_CURRENT_FLAGS_COMPARE |
                                SLJIT_SET_LESS_EQUAL | SLJIT_SET_Z;
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
      if (type_ == Type::SignedDivide32) {
        current_flags |= SLJIT_32;
      }
#endif /* SLJIT_64BIT_ARCHITECTURE */

      sljit_set_current_flags(compiler, current_flags);
      /* Division by zero. */
      sljit_jump* jump = sljit_emit_jump(compiler, SLJIT_EQUAL);
      sljit_set_label(jump, context->trap_label);

      sljit_s32 type = SLJIT_NOT_EQUAL;
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
      sljit_sw int_min = static_cast<sljit_sw>(INT64_MIN);

      if (type_ == Type::SignedDivide32) {
        type |= SLJIT_32;
        int_min = static_cast<sljit_sw>(INT32_MIN);
      }
#else  /* !SLJIT_64BIT_ARCHITECTURE */
      sljit_sw int_min = static_cast<sljit_sw>(INT32_MIN);
#endif /* SLJIT_64BIT_ARCHITECTURE */

      sljit_jump* cmp =
          sljit_emit_cmp(compiler, type, SLJIT_R0, 0, SLJIT_IMM, int_min);
      sljit_set_label(cmp, resume_label_);

      jump = sljit_emit_jump(compiler, SLJIT_JUMP);
      sljit_set_label(jump, context->trap_label);
      return;
    }
    default: {
      WABT_UNREACHABLE;
      break;
    }
  }
}

static void emitDirectBranch(sljit_compiler* compiler, Instruction* instr) {
  sljit_jump* jump;

  if (instr->opcode() == Opcode::Br) {
    jump = sljit_emit_jump(compiler, SLJIT_JUMP);

    CompileContext::get(compiler)->emitSlowCases(compiler);
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

class MoveContext {
 public:
  MoveContext(sljit_compiler* compiler);

  void move(ValueInfo value_info, JITArg from, JITArg to);
  void done();

 private:
  static const int delay_size = 2;

  void reset();

  sljit_compiler* compiler_;
  int index_;
  sljit_s32 reg_[delay_size];
  sljit_s32 op_[delay_size];
  JITArg target_[delay_size];
};

MoveContext::MoveContext(sljit_compiler* compiler) : compiler_(compiler) {
  reset();
}

void MoveContext::move(ValueInfo value_info, JITArg from, JITArg to) {
  sljit_s32 op;

  if (value_info & LocationInfo::kFloat) {
    op = (value_info & LocationInfo::kEightByteSize) ? SLJIT_MOV_F64
                                                     : SLJIT_MOV_F32;
  } else {
    op = (value_info & LocationInfo::kEightByteSize) ? SLJIT_MOV : SLJIT_MOV32;
  }

  if (!(from.arg & SLJIT_MEM) || !(to.arg & SLJIT_MEM)) {
    if (value_info & LocationInfo::kFloat) {
      sljit_emit_fop1(compiler_, op, to.arg, to.argw, from.arg, from.argw);
      return;
    }

    sljit_emit_op1(compiler_, op, to.arg, to.argw, from.arg, from.argw);
    return;
  }

  if (reg_[index_] != 0) {
    if ((op_[index_] | SLJIT_32) == SLJIT_MOV_F32) {
      sljit_emit_fop1(compiler_, op_[index_], target_[index_].arg,
                      target_[index_].argw, reg_[index_], 0);
    } else {
      sljit_emit_op1(compiler_, op_[index_], target_[index_].arg,
                     target_[index_].argw, reg_[index_], 0);
    }
  }

  if (value_info & LocationInfo::kFloat) {
    op_[index_] = op;
    reg_[index_] = (index_ == 0) ? SLJIT_FR0 : SLJIT_FR1;

    sljit_emit_fop1(compiler_, op, reg_[index_], 0, from.arg, from.argw);
  } else {
    op_[index_] = op;
    reg_[index_] = (index_ == 0) ? SLJIT_R0 : SLJIT_R1;

    sljit_emit_op1(compiler_, op, reg_[index_], 0, from.arg, from.argw);
  }

  target_[index_] = to;
  index_ = 1 - index_;
}

void MoveContext::done() {
  for (int i = 0; i < delay_size; i++) {
    if (reg_[index_] != 0) {
      if ((op_[index_] | SLJIT_32) == SLJIT_MOV_F32) {
        sljit_emit_fop1(compiler_, op_[index_], target_[index_].arg,
                        target_[index_].argw, reg_[index_], 0);
      } else {
        sljit_emit_op1(compiler_, op_[index_], target_[index_].arg,
                       target_[index_].argw, reg_[index_], 0);
      }
    }

    index_ = 1 - index_;
  }

  reset();
}

void MoveContext::reset() {
  index_ = 0;
  reg_[0] = 0;
  reg_[1] = 0;
}

static void emitCall(sljit_compiler* compiler, CallInstruction* call_instr) {
  CompileContext* context = CompileContext::get(compiler);
  Operand* operand = call_instr->operands();
  Operand* operand_end = operand + call_instr->paramCount();
  MoveContext move_context(compiler);
  JITArg from, to;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
  JITArgPair arg64;
#endif /* SLJIT_32BIT_ARCHITECTURE */

  LocalsAllocator locals_allocator(call_instr->paramStart());

  while (operand < operand_end) {
    locals_allocator.allocate(operand->location.value_info);

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    if (!(operand->location.value_info & LocationInfo::kFloat) &&
        (operand->location.value_info & LocationInfo::kSizeMask) == 2) {
      operandToArgPair(operand, arg64);

      from.arg = arg64.arg1;
      from.argw = arg64.arg1w;
      to.arg = SLJIT_MEM1(kFrameReg);
      to.argw = static_cast<sljit_sw>(locals_allocator.last().value);

      if (from.arg != to.arg || from.argw != to.argw) {
        move_context.move(operand->location.value_info, from, to);
      }

      from.arg = arg64.arg2;
      from.argw = arg64.arg2w;
      to.argw += sizeof(sljit_sw);

      if (from.arg != to.arg || from.argw != to.argw) {
        move_context.move(operand->location.value_info, from, to);
      }

      operand++;
      continue;
    }
#endif /* SLJIT_32BIT_ARCHITECTURE */

    operandToArg(operand, from);
    to.arg = SLJIT_MEM1(kFrameReg);
    to.argw = static_cast<sljit_sw>(locals_allocator.last().value);

    if (from.arg != to.arg || from.argw != to.argw) {
      move_context.move(operand->location.value_info, from, to);
    }

    operand++;
  }

  move_context.done();

  if (!(call_instr->info() & CallInstruction::kIndirect)) {
    sljit_jump* jump =
        sljit_emit_call(compiler, SLJIT_CALL_REG_ARG, SLJIT_ARGS0(VOID));
    Index func_index = call_instr->value().func_index;

    context->compiler->getFunctionEntry(func_index)->jumpFrom(jump);
  }

  StackAllocator stack_allocator;
  operand_end = operand + call_instr->resultCount();

  while (operand < operand_end) {
    stack_allocator.push(operand->location.value_info);

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    if (!(operand->location.value_info & LocationInfo::kFloat) &&
        (operand->location.value_info & LocationInfo::kSizeMask) == 2) {
      operandToArgPair(operand, arg64);

      from.arg = SLJIT_MEM1(kFrameReg);
      from.argw = static_cast<sljit_sw>(stack_allocator.last().value);
      to.arg = arg64.arg1;
      to.argw = arg64.arg1w;

      if (from.arg != to.arg || from.argw != to.argw) {
        move_context.move(operand->location.value_info, from, to);
      }

      from.argw += sizeof(sljit_sw);
      to.arg = arg64.arg2;
      to.argw = arg64.arg2w;

      if (from.arg != to.arg || from.argw != to.argw) {
        move_context.move(operand->location.value_info, from, to);
      }

      operand++;
      continue;
    }
#endif /* SLJIT_32BIT_ARCHITECTURE */

    operandToArg(operand, to);
    from.arg = SLJIT_MEM1(kFrameReg);
    from.argw = static_cast<sljit_sw>(stack_allocator.last().value);

    if (from.arg != to.arg || from.argw != to.argw) {
      move_context.move(operand->location.value_info, from, to);
    }

    operand++;
  }

  move_context.done();
}

JITModuleDescriptor::~JITModuleDescriptor() {
  trap_handler_list.erase(GET_FUNC_ADDR(sljit_uw, module_start_), module_end_);
  sljit_free_code(module_start_, nullptr);
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

JITModuleDescriptor* JITCompiler::compile() {
  CompileContext compile_context(this);
  compiler_ =
      sljit_create_compiler(reinterpret_cast<void*>(&compile_context), nullptr);

  // Follows the declaration of FunctionDescriptor::ExternalDecl().
  // Context stored in SLJIT_S0 (kContextReg)
  // Frame stored in SLJIT_S1 (kFrameReg)
  sljit_emit_enter(compiler_, 0, SLJIT_ARGS3(VOID, P, P, P_R), 3, 2, 0, 0, 0);
  sljit_emit_icall(compiler_, SLJIT_CALL_REG_ARG, SLJIT_ARGS0(VOID), SLJIT_R2,
                   0);
  sljit_label* return_to_label = sljit_emit_label(compiler_);
  sljit_emit_return_void(compiler_);

  compile_context.trap_blocks.push_back(
      TrapBlock(sljit_emit_label(compiler_), return_to_label));

  size_t current_function = 0;

  for (InstructionListItem* item = function_list_first_; item != nullptr;
       item = item->next()) {
    if (item->isLabel()) {
      Label* label = item->asLabel();

      if (!(label->info() & Label::kNewFunction)) {
        label->emit(compiler_);
      } else {
        if (label->prev() != nullptr) {
          emitEpilog(current_function, compile_context);
          current_function++;
        }
        emitProlog(current_function, compile_context);
        compile_context.frame_size =
            function_list_[current_function].jit_func->frameSize();
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
      case Instruction::Call: {
        emitCall(compiler_, item->asInstruction()->asCall());
        break;
      }
      case Instruction::Binary: {
        emitBinary(compiler_, item->asInstruction());
        break;
      }
      case Instruction::BinaryFloat: {
        emitFloatBinary(compiler_, item->asInstruction());
        break;
      }
      case Instruction::Unary: {
        emitUnary(compiler_, item->asInstruction());
        break;
      }
      case Instruction::UnaryFloat: {
        emitFloatUnary(compiler_, item->asInstruction());
        break;
      }
      case Instruction::Compare: {
        if (emitCompare(compiler_, item->asInstruction())) {
          item = item->next();
        }
        break;
      }
      case Instruction::Convert: {
        emitConvert(compiler_, item->asInstruction());
        break;
      }
      default: {
        switch (item->asInstruction()->opcode()) {
          case Opcode::Drop:
          case Opcode::Return: {
            break;
          }
          case Opcode::Unreachable: {
            sljit_emit_op1(compiler_, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM,
                           ExecutionContext::UnreachableError);
            sljit_set_label(sljit_emit_jump(compiler_, SLJIT_JUMP),
                            compile_context.trap_label);
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

  sljit_label* end_label = emitEpilog(current_function, compile_context);

  void* code = sljit_generate_code(compiler_);
  JITModuleDescriptor* module_descriptor = nullptr;

  if (code != nullptr) {
    sljit_uw end_addr = sljit_get_label_addr(end_label);
    module_descriptor = new JITModuleDescriptor(code, end_addr);

    trap_handler_list.insert(compile_context.trap_blocks);

    for (auto it : function_list_) {
      it.jit_func->module_ = module_descriptor;

      if (!it.is_exported) {
        it.jit_func->export_entry_ = nullptr;
        continue;
      }

      it.jit_func->export_entry_ =
          reinterpret_cast<void*>(sljit_get_label_addr(it.export_entry_label));
    }
  }

  sljit_free_compiler(compiler_);
  return module_descriptor;
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

void JITCompiler::emitProlog(size_t index, CompileContext& context) {
  FunctionList& func = function_list_[index];
  sljit_s32 saved_reg_count = 4;

  sljit_set_context(compiler_, SLJIT_ENTER_REG_ARG | SLJIT_ENTER_KEEP(2),
                    SLJIT_ARGS0(VOID), SLJIT_NUMBER_OF_SCRATCH_REGISTERS,
                    saved_reg_count, SLJIT_NUMBER_OF_SCRATCH_FLOAT_REGISTERS, 0,
                    sizeof(ExecutionContext::CallFrame));

  context.trap_label = sljit_emit_label(compiler_);
  sljit_emit_op1(compiler_, SLJIT_MOV_U32, SLJIT_MEM1(kContextReg),
                 OffsetOfContextField(error), SLJIT_R2, 0);

  context.return_to_label = sljit_emit_label(compiler_);

  sljit_emit_op_dst(compiler_, SLJIT_GET_RETURN_ADDRESS, SLJIT_R0, 0);
  sljit_emit_icall(compiler_, SLJIT_CALL, SLJIT_ARGS1(W, W), SLJIT_IMM,
                   GET_FUNC_ADDR(sljit_sw, getTrapHandler));
  sljit_emit_return_to(compiler_, SLJIT_R0, 0);

  if (func.is_exported) {
    func.export_entry_label = sljit_emit_label(compiler_);
  }

  func.entry_label->emit(compiler_);

  sljit_emit_enter(compiler_, SLJIT_ENTER_REG_ARG | SLJIT_ENTER_KEEP(2),
                   SLJIT_ARGS0(VOID), SLJIT_NUMBER_OF_SCRATCH_REGISTERS,
                   saved_reg_count, SLJIT_NUMBER_OF_SCRATCH_FLOAT_REGISTERS, 0,
                   sizeof(ExecutionContext::CallFrame));

  // Setup new frame.
  sljit_emit_op1(compiler_, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_MEM1(kContextReg),
                 OffsetOfContextField(last_frame));

  if (func.jit_func->frameSize() > 0) {
    sljit_emit_op2(compiler_, SLJIT_SUB, kFrameReg, 0, kFrameReg, 0, SLJIT_IMM,
                   static_cast<sljit_sw>(func.jit_func->frameSize()));

    sljit_emit_op1(compiler_, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM,
                   ExecutionContext::OutOfStackError);
    sljit_jump* cmp =
        sljit_emit_cmp(compiler_, SLJIT_LESS, kFrameReg, 0, kContextReg, 0);
    sljit_set_label(cmp, context.trap_label);
  }

  sljit_get_local_base(compiler_, SLJIT_R1, 0, 0);
  sljit_emit_op1(compiler_, SLJIT_MOV_P, SLJIT_MEM1(kContextReg),
                 OffsetOfContextField(last_frame), SLJIT_R1, 0);
  sljit_emit_op1(compiler_, SLJIT_MOV_P, SLJIT_MEM1(SLJIT_SP),
                 offsetof(ExecutionContext::CallFrame, frame_start), kFrameReg,
                 0);
  sljit_emit_op1(compiler_, SLJIT_MOV_P, SLJIT_MEM1(SLJIT_SP),
                 offsetof(ExecutionContext::CallFrame, prev_frame), SLJIT_R0,
                 0);
}

sljit_label* JITCompiler::emitEpilog(size_t index, CompileContext& context) {
  FunctionList& func = function_list_[index];

  // Restore previous frame.
  sljit_emit_op1(compiler_, SLJIT_MOV_P, SLJIT_R0, 0, SLJIT_MEM1(SLJIT_SP),
                 offsetof(ExecutionContext::CallFrame, prev_frame));
  if (func.jit_func->frameSize() > 0) {
    sljit_emit_op2(compiler_, SLJIT_ADD, kFrameReg, 0, kFrameReg, 0, SLJIT_IMM,
                   static_cast<sljit_sw>(func.jit_func->frameSize()));
  }
  sljit_emit_op1(compiler_, SLJIT_MOV_P, SLJIT_MEM1(kContextReg),
                 OffsetOfContextField(last_frame), SLJIT_R0, 0);

  sljit_emit_return_void(compiler_);

  context.emitSlowCases(compiler_);

  sljit_label* end_label = sljit_emit_label(compiler_);
  context.trap_blocks.push_back(TrapBlock(end_label, context.return_to_label));
  return end_label;
}

}  // namespace interp
}  // namespace wabt
