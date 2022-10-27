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

#include "wabt/interp/jit/jit-compiler.h"

#include <map>

namespace wabt {
namespace interp {

Index Instruction::resultCount() {
  if (!hasResult()) {
    return 0;
  }

  return 1;
}

BrTableInstruction::~BrTableInstruction() {
  delete[] target_labels_;
}

ComplexInstruction::~ComplexInstruction() {
  delete[] params();
}

BrTableInstruction::BrTableInstruction(size_t target_label_count,
                                       InstructionListItem* prev)
    : Instruction(Instruction::Any,
                  Opcode::BrTable,
                  1,
                  &inlineParam_,
                  prev) {
  target_labels_ = new Label*[target_label_count];
  value().target_label_count = target_label_count;
}

void Label::append(Instruction* instr) {
  for (auto it : branches_) {
    if (it == instr) {
      return;
    }
  }

  branches_.push_back(instr);
}

void Label::remove(Instruction* instr) {
  for (size_t i = 0; i < branches_.size(); i++) {
    if (branches_[i] == instr) {
      branches_.erase(branches_.begin() + i);
      return;
    }
  }
}

void Label::replace(Instruction* instr, Instruction* other_instr) {
  for (size_t i = 0; i < branches_.size(); i++) {
    if (branches_[i] == instr) {
      branches_[i] = other_instr;
      return;
    }
  }
}

void Label::merge(Label* other) {
  assert(this != other);
  assert(stackSize() == other->stackSize());

  for (auto it : other->branches_) {
    if (it->opcode() != Opcode::BrTable) {
      assert(it->group() == Instruction::DirectBranch);
      it->value().target_label = this;
      branches_.push_back(it);
      continue;
    }

    BrTableInstruction* instr = it->asBrTable();

    Label** label = instr->targetLabels();
    Label** end = label + instr->value().target_label_count;
    bool found = false;

    do {
      if (*label == other) {
        *label = this;
      } else if (*label == this) {
        found = true;
      }
    } while (++label < end);

    if (!found) {
      branches_.push_back(it);
    }
  }
}

void JITCompiler::clear() {
  for (auto it : label_stack_) {
    if (it != nullptr && it->prev_ == nullptr) {
      delete it;
    }
  }

  stack_depth_ = 0;
  label_stack_.clear();
  else_blocks_.clear();
  locals_.clear();

  InstructionListItem* item = first_;

  first_ = nullptr;
  last_ = nullptr;

  while (item != nullptr) {
    InstructionListItem* next = item->next();

    assert(next == nullptr || next->prev_ == item);
    assert(!item->isLabel() || item->asLabel()->branches().size() > 0);

    delete item;
    item = next;
  }
}

Instruction* JITCompiler::append(Instruction::Group group,
                                 Opcode op,
                                 Index param_count) {
  return appendInternal(group, op, param_count, param_count,
                        LocationInfo::kNone);
}

Instruction* JITCompiler::append(Instruction::Group group,
                                 Opcode op,
                                 Index param_count,
                                 ValueInfo result) {
  assert(result != LocationInfo::kNone);
  return appendInternal(group, op, param_count, param_count + 1, result);
}

void JITCompiler::appendBranch(Opcode opcode, Index depth) {
  if (stack_depth_ == kInvalidIndex) {
    return;
  }

  assert(depth < label_stack_.size());
  assert(opcode == Opcode::Br || opcode == Opcode::BrIf);

  Label* label = label_stack_[label_stack_.size() - (depth + 1)];
  Instruction* branch;

  if (opcode == Opcode::Br) {
    stack_depth_ = kInvalidIndex;
    branch = new Instruction(Instruction::DirectBranch, Opcode::Br, last_);
  } else {
    stack_depth_--;
    branch = new SimpleInstruction<1>(Instruction::DirectBranch, opcode, last_);
  }

  branch->value().target_label = label;
  label->branches_.push_back(branch);
  append(branch);
}

void JITCompiler::appendElseLabel() {
  Label* label = label_stack_[label_stack_.size() - 1];

  if (label == nullptr) {
    assert(stack_depth_ == kInvalidIndex);
    return;
  }

  assert(label->info() & Label::kCanHaveElse);
  label->clearInfo(Label::kCanHaveElse);

  if (stack_depth_ != kInvalidIndex) {
    Instruction* branch =
        new Instruction(Instruction::DirectBranch, Opcode::Br, last_);
    branch->value().target_label = label;
    append(branch);
    label->branches_.push_back(branch);
  }

  ElseBlock& else_block = else_blocks_.back();

  label = new Label(else_block.result_count, else_block.preserved_count, last_);
  label->addInfo(Label::kAfterUncondBranch);
  append(label);
  stack_depth_ = else_block.preserved_count + else_block.result_count;

  Instruction* branch = else_block.branch;
  assert(branch->opcode() == Opcode::InterpBrUnless);

  branch->value().target_label = label;
  label->branches_.push_back(branch);
  else_blocks_.pop_back();
}

void JITCompiler::appendBrTable(Index num_targets,
                                Index* target_depths,
                                Index default_target_depth) {
  if (stack_depth_ == kInvalidIndex) {
    return;
  }

  BrTableInstruction* branch = new BrTableInstruction(num_targets + 1, last_);
  append(branch);
  stack_depth_ = kInvalidIndex;

  Label* label;
  Label** labels = branch->targetLabels();
  size_t label_stack_size = label_stack_.size();

  for (Index i = 0; i < num_targets; i++) {
    label = label_stack_[label_stack_size - (*target_depths + 1)];

    target_depths++;
    *labels++ = label;
    label->append(branch);
  }

  label = label_stack_[label_stack_size - (default_target_depth + 1)];
  *labels = label;
  label->append(branch);
}

void JITCompiler::pushLabel(Opcode opcode,
                            Index param_count,
                            Index result_count) {
  InstructionListItem* prev = nullptr;

  if (stack_depth_ == kInvalidIndex) {
    label_stack_.push_back(nullptr);
    return;
  }

  if (opcode == Opcode::Loop) {
    if (last_ != nullptr && last_->isLabel()) {
      label_stack_.push_back(last_->asLabel());
      return;
    }

    result_count = param_count;
    prev = last_;
  } else if (opcode == Opcode::If) {
    stack_depth_--;
  }

  Label* label = new Label(result_count, stack_depth_ - param_count, prev);
  label_stack_.push_back(label);

  if (opcode == Opcode::Loop) {
    append(label);
  } else if (opcode == Opcode::If) {
    label->addInfo(Label::kCanHaveElse);

    Instruction* branch = new SimpleInstruction<1>(
        Instruction::DirectBranch, Opcode::InterpBrUnless, last_);
    else_blocks_.push_back(
        ElseBlock(branch, param_count, stack_depth_ - param_count));
    append(branch);
  }
}

void JITCompiler::popLabel() {
  Label* label = label_stack_.back();
  label_stack_.pop_back();

  if (label == nullptr) {
    assert(stack_depth_ == kInvalidIndex);
    return;
  }

  if (label->prev_ != nullptr || first_ == label) {
    // Loop instruction.
    if (label->branches().size() == 0) {
      remove(label);
    }
    return;
  }

  if (label->info() & Label::kCanHaveElse) {
    Instruction* branch = else_blocks_.back().branch;

    branch->value().target_label = label;
    label->branches_.push_back(branch);
    label->clearInfo(Label::kCanHaveElse);

    else_blocks_.pop_back();
  }

  if (label->branches().size() == 0) {
    delete label;
    return;
  }

  if (last_ != nullptr && last_->isLabel()) {
    assert(last_->asLabel()->branches().size() > 0 &&
           stack_depth_ == last_->asLabel()->stackSize());
    last_->asLabel()->merge(label);
    delete label;
    return;
  }

  append(label);
  if (stack_depth_ == kInvalidIndex) {
    label->addInfo(Label::kAfterUncondBranch);
  }

  stack_depth_ = label->preservedCount() + label->resultCount();
}

void JITCompiler::append(InstructionListItem* item) {
  if (last_ != nullptr) {
    last_->next_ = item;
  } else {
    first_ = item;
  }

  item->prev_ = last_;
  last_ = item;
}

InstructionListItem* JITCompiler::remove(InstructionListItem* item) {
  InstructionListItem* prev = item->prev_;
  InstructionListItem* next = item->next_;

  if (prev == nullptr) {
    assert(first_ == item);
    first_ = next;
  } else {
    assert(first_ != item);
    prev->next_ = next;
  }

  if (next == nullptr) {
    assert(last_ == item);
    last_ = prev;
  } else {
    assert(last_ != item);
    next->prev_ = prev;
  }

  delete item;
  return next;
}

void JITCompiler::replace(InstructionListItem* item,
                          InstructionListItem* new_item) {
  assert(item != new_item);

  InstructionListItem* prev = item->prev_;
  InstructionListItem* next = item->next_;

  new_item->prev_ = prev;
  new_item->next_ = next;

  if (prev == nullptr) {
    assert(first_ == item);
    first_ = new_item;
  } else {
    assert(first_ != item);
    prev->next_ = new_item;
  }

  if (next == nullptr) {
    assert(last_ == item);
    last_ = new_item;
  } else {
    assert(last_ != item);
    next->prev_ = new_item;
  }

  delete item;
}

static const char* flagsToType(ValueInfo value_info) {
  ValueInfo size = value_info & LocationInfo::kSizeMask;

  if (value_info & LocationInfo::kReference) {
    return (size == LocationInfo::kEightByteSize) ? "ref8" : "ref4";
  }

  if (value_info & LocationInfo::kFloat) {
    return (size == LocationInfo::kEightByteSize) ? "f64" : "f32";
  }

  if (size == LocationInfo::kSixteenByteSize) {
    return "v128";
  }

  return (size == LocationInfo::kEightByteSize) ? "i64" : "i32";
}

void JITCompiler::appendFunction(JITFunction* jit_func, bool is_external) {
  assert(first_ != nullptr && last_ != nullptr);

  Label* entry_label = new Label(0, 0, function_list_last_);
  entry_label->addInfo(Label::kNewFunction);
  entry_label->next_ = first_;
  first_->prev_ = entry_label;

  if (function_list_first_ == nullptr) {
    assert(function_list_last_ == nullptr);
    function_list_first_ = entry_label;
  } else {
    assert(function_list_last_ != nullptr);
    function_list_last_->next_ = entry_label;
  }

  function_list_last_ = last_;
  first_ = nullptr;
  last_ = nullptr;
  function_list_.push_back(FunctionList(jit_func, entry_label, is_external));
}

void JITCompiler::dump(bool enable_colors, bool after_stack_computation) {
  std::map<InstructionListItem*, int> instr_index;
  int counter = 0;

  for (InstructionListItem* item = first(); item != nullptr;
       item = item->next()) {
    instr_index[item] = counter++;
  }

  const char* default_text = enable_colors ? "\033[0m" : "";
  const char* instr_text = enable_colors ? "\033[1;35m" : "";
  const char* label_text = enable_colors ? "\033[1;36m" : "";
  const char* unused_text = enable_colors ? "\033[1;33m" : "";

  for (InstructionListItem* item = first(); item != nullptr;
       item = item->next()) {
    if (item->isInstruction()) {
      Instruction* instr = item->asInstruction();
      printf("%s%d%s: ", instr_text, instr_index[item], default_text);

      if (enable_colors) {
        printf("(%p) ", item);
      }

      printf("Opcode: %s\n", instr->opcode().GetName());

      if (instr->group() == Instruction::DirectBranch) {
        printf("  Jump to: %s%d%s\n", label_text,
               instr_index[instr->value().target_label], default_text);
      } else if (instr->opcode() == Opcode::BrTable) {
        size_t target_label_count = instr->value().target_label_count;
        Label** target_labels = instr->asBrTable()->targetLabels();

        for (size_t i = 0; i < target_label_count; i++) {
          printf("  Jump to: %s%d%s\n", label_text, instr_index[*target_labels],
                 default_text);
          target_labels++;
        }
      } else if (instr->group() == Instruction::LocalMove) {
        printf("  Index: %d\n", instr->value().local_index);
      }

      Index param_count = instr->paramCount();
      Index size = param_count + instr->resultCount();
      Operand* param = instr->params();

      for (Index i = 0; i < size; ++i) {
        if (i < param_count) {
          printf("  Param[%d]: ", static_cast<int>(i));
        } else {
          printf("  Result[%d]: ", static_cast<int>(i - param_count));
        }

        if (after_stack_computation) {
          printf("%s ", flagsToType(param->location.value_info));

          switch (param->location.type) {
            case Operand::Stack: {
              printf("sp[%d]\n", static_cast<int>(param->value));
              break;
            }
            case Operand::Register: {
              printf("reg:%d\n", static_cast<int>(param->value));
              break;
            }
            case Operand::Immediate: {
              const char* color =
                  param->item->isLabel() ? label_text : instr_text;
              printf("const:%s%d%s\n", color, instr_index[param->item],
                     default_text);
              break;
            }
            case Operand::Unused: {
              printf("%sunused%s\n", unused_text, default_text);
              break;
            }
            default: {
              printf("%s[UNKNOWN]%s\n", unused_text, default_text);
              break;
            }
          }
        } else if (i < param_count) {
          const char* color = param->item->isLabel() ? label_text : instr_text;
          printf("ref:%s%d%s(%d)\n", color, instr_index[param->item],
                 default_text, static_cast<int>(param->index));
        } else {
          printf("%s\n", flagsToType(param->location.value_info));
        }
        param++;
      }
    } else {
      printf("%s%d%s: ", label_text, instr_index[item], default_text);

      if (enable_colors) {
        printf("(%p) ", item);
      }

      Label* label = item->asLabel();

      printf("Label: result_count: %d preserved_count: %d\n",
             static_cast<int>(label->resultCount()),
             static_cast<int>(label->preservedCount()));

      for (auto it : label->branches()) {
        printf("  Jump from: %s%d%s\n", instr_text, instr_index[it],
               default_text);
      }

      size_t size = label->dependencyCount();

      for (size_t i = 0; i < size; ++i) {
        printf("  Param[%d]\n", static_cast<int>(i));
        for (auto dep_it : label->dependencies(i)) {
          printf("    %s%d%s(%d) instruction\n", instr_text,
                 instr_index[dep_it.instr], default_text,
                 static_cast<int>(dep_it.index));
        }
      }
    }
  }
}

Instruction* JITCompiler::appendInternal(Instruction::Group group,
                                         Opcode op,
                                         Index param_count,
                                         Index operand_count,
                                         ValueInfo result) {
  if (stack_depth_ == kInvalidIndex) {
    return nullptr;
  }

  stack_depth_ -= param_count;

  Instruction* instr;
  switch (operand_count) {
    case 0:
      instr = new Instruction(group, op, 0, nullptr, last_);
      break;
    case 1:
      instr = new SimpleInstruction<1>(group, op, param_count, last_);
      break;
    case 2:
      instr = new SimpleInstruction<2>(group, op, param_count, last_);
      break;
    case 3:
      instr = new SimpleInstruction<3>(group, op, param_count, last_);
      break;
    case 4:
      instr = new SimpleInstruction<4>(group, op, param_count, last_);
      break;
    default:
      instr =
          new ComplexInstruction(group, op, param_count, operand_count, last_);
      break;
  }

  if (result != LocationInfo::kNone) {
    instr->result_count_ = 1;
    stack_depth_++;

    Operand* result_default = instr->getResult(0);
    result_default->location.type = Operand::Stack;
    result_default->location.value_info = result;
  }

  append(instr);
  return instr;
}

}  // namespace interp
}  // namespace wabt
