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

#include <set>

namespace wabt {
namespace interp {

class DependencyGenContext {
 public:
  struct Dependency {
    Dependency(InstructionListItem* item, size_t index)
        : item(item), index(index) {}

    bool operator<(const Dependency& other) const {
      uintptr_t left = reinterpret_cast<uintptr_t>(item);
      uintptr_t right = reinterpret_cast<uintptr_t>(other.item);

      return (left < right) || (left == right && index < other.index);
    }

    InstructionListItem* item;
    Index index;
  };

  struct StackItem {
    ValueInfo value_info;
    std::vector<Dependency> inst_deps;
    std::vector<Dependency> label_deps;
  };

  DependencyGenContext() : start_index_(0) {}

  size_t startIndex() { return start_index_; }
  void setStartIndex(size_t value) { start_index_ = value; }
  std::vector<StackItem>* stack() { return &stack_; }
  StackItem& stackItem(size_t index) { return stack_[index]; }

  void update(std::vector<Operand>& deps);

 private:
  static void append(std::vector<Dependency>& list,
                     InstructionListItem* item,
                     Index index);

  size_t start_index_;
  std::vector<StackItem> stack_;
};

void DependencyGenContext::update(std::vector<Operand>& deps) {
  size_t size = deps.size();

  assert(stack_.size() == 0 || stack_.size() == size);

  if (stack_.size() == 0) {
    stack_.resize(size);
  }

  for (size_t i = 0; i < size; i++) {
    Operand& dep = deps[i];

    if (dep.item->isInstruction()) {
      append(stack_[i].inst_deps, dep.item, dep.index);
    } else {
      append(stack_[i].label_deps, dep.item, dep.index);
    }
  }
}

void DependencyGenContext::append(std::vector<Dependency>& list,
                                  InstructionListItem* item,
                                  Index index) {
  for (auto it : list) {
    if (it.item == item && it.index == index) {
      return;
    }
  }

  list.push_back(Dependency(item, index));
}

void JITCompiler::buildParamDependencies() {
  for (InstructionListItem* item = first_; item != nullptr;
       item = item->next()) {
    if (item->isLabel()) {
      item->asLabel()->dependency_ctx_ = new DependencyGenContext();
    }
  }

  bool update_deps = true;
  size_t start_index = 0;
  std::vector<Operand> current_deps;

  // Phase 1: the direct dependencies are computed for instructions
  // and labels (only labels can have multiple dependencies).
  for (InstructionListItem* item = first_; item != nullptr;
       item = item->next()) {
    if (item->isLabel()) {
      if (start_index > 0) {
        assert(current_deps[0].item->isLabel());
        Label* label = current_deps[0].item->asLabel();
        label->dependency_ctx_->setStartIndex(start_index);
      }

      // Build a dependency list which refers to the last label.
      Label* label = item->asLabel();

      if (update_deps) {
        label->dependency_ctx_->update(current_deps);
        current_deps.clear();
      }

      start_index = label->dependency_ctx_->stack()->size();

      current_deps.resize(start_index);

      for (size_t i = 0; i < start_index; ++i) {
        Operand& dep = current_deps[i];
        dep.item = label;
        dep.index = i;
      }

      update_deps = true;
      continue;
    }

    Instruction* instr = item->asInstruction();

    // Pop params from the stack first.
    Index end = instr->paramCount();
    Operand* param = instr->params() + end;

    for (Index i = end; i > 0; --i) {
      *(--param) = current_deps.back();
      current_deps.pop_back();

      if (param->item->isLabel()) {
        // Values are consumed top to bottom.
        assert(start_index == param->index + 1);
        start_index = param->index;
      }
    }

    // Push results next.
    if (instr->hasResult()) {
      Operand dep;
      dep.item = instr;

      end = instr->resultCount();
      for (Index i = 0; i < end; ++i) {
        dep.index = i;
        current_deps.push_back(dep);
      }
    }

    if (instr->group() == Instruction::DirectBranch) {
      Label* label = instr->value().target_label;
      label->dependency_ctx_->update(current_deps);

      if (instr->opcode() == Opcode::Br) {
        update_deps = false;
      }
    } else if (instr->opcode() == Opcode::BrTable) {
      Label** label = instr->asBrTable()->targetLabels();
      Label** end = label + instr->value().target_label_count;
      std::set<Label*> updated_labels;

      while (label < end) {
        if (updated_labels.insert(*label).second) {
          (*label)->dependency_ctx_->update(current_deps);
        }
        label++;
      }
      update_deps = false;
    }
  }

  // Phase 2: the indirect instruction
  // references are computed for labels.
  std::vector<DependencyGenContext::StackItem>* last_stack = nullptr;
  Index last_start_index = 0;

  for (InstructionListItem* item = first_; item != nullptr;
       item = item->next()) {
    if (!item->isLabel()) {
      Instruction* instr = item->asInstruction();
      Operand* param = instr->params();

      for (Index i = instr->paramCount(); i > 0; --i) {
        if (param->item->isLabel()) {
          assert(param->item->asLabel()->dependency_ctx_->stack() ==
                 last_stack);
          assert(param->index >= last_start_index);

          std::vector<DependencyGenContext::Dependency> inst_deps =
              (*last_stack)[param->index].inst_deps;

          if (inst_deps.size() == 1) {
            // A single reference is copied into the instruction.
            param->item = inst_deps[0].item;
            param->index = inst_deps[0].index;
          } else {
            // References below last_start_index are deleted.
            param->index -= last_start_index;
          }
        }
        param++;
      }
      continue;
    }

    DependencyGenContext* context = item->asLabel()->dependency_ctx_;

    last_stack = context->stack();
    last_start_index = static_cast<Index>(context->startIndex());
    size_t end = last_stack->size();

    for (size_t i = last_start_index; i < end; ++i) {
      std::vector<DependencyGenContext::Dependency> unprocessed_labels;
      std::set<DependencyGenContext::Dependency> processed_deps;
      std::vector<DependencyGenContext::Dependency>& inst_deps =
          (*last_stack)[i].inst_deps;

      for (auto it : inst_deps) {
        processed_deps.insert(it);
      }

      for (auto it : (*last_stack)[i].label_deps) {
        processed_deps.insert(it);
        unprocessed_labels.push_back(it);
      }

      (*last_stack)[i].label_deps.clear();

      while (!unprocessed_labels.empty()) {
        DependencyGenContext::Dependency& dep = unprocessed_labels.back();
        DependencyGenContext::StackItem& item =
            dep.item->asLabel()->dependency_ctx_->stackItem(dep.index);

        unprocessed_labels.pop_back();

        for (auto it : item.inst_deps) {
          if (processed_deps.insert(it).second) {
            inst_deps.push_back(it);
          }
        }

        for (auto it : item.label_deps) {
          if (processed_deps.insert(it).second) {
            unprocessed_labels.push_back(it);
          }
        }
      }

      // At least one instruction dependency
      // must be present if the input is valid.
      assert(inst_deps.size() > 0);
    }
  }

  // Phase 3: The indirect references for labels are
  // collected, and all temporary structures are deleted.
  for (InstructionListItem* item = first_; item != nullptr;
       item = item->next()) {
    if (!item->isLabel()) {
      continue;
    }

    // Copy the final dependency data into
    // the dependency list of the label.
    Label* label = item->asLabel();
    DependencyGenContext* context = label->dependency_ctx_;
    std::vector<DependencyGenContext::StackItem>* stack = context->stack();
    size_t start_index = context->startIndex();
    size_t end = stack->size();

    // Single item dependencies are
    // moved into the corresponding oeprand data.
    label->dependencies_.resize(end - start_index);

    for (size_t i = start_index; i < end; ++i) {
      assert((*stack)[i].label_deps.empty());

      Label::DependencyList& list = label->dependencies_[i - start_index];

      size_t size = (*stack)[i].inst_deps.size();

      if (size > 1) {
        list.reserve(size);

        for (auto it : (*stack)[i].inst_deps) {
          list.push_back(Label::Dependency(it.item->asInstruction(), it.index));
        }
      }
    }

    delete context;
  }
}

void JITCompiler::optimizeBlocks() {
  for (InstructionListItem* item = first_; item != nullptr;
       item = item->next()) {
    if (item->group() == Instruction::Compare) {
      assert(item->next() != nullptr);

      if (item->next()->group() == Instruction::DirectBranch) {
        Instruction* next = item->next()->asInstruction();

        if (next->opcode() == Opcode::BrIf ||
            next->opcode() == Opcode::InterpBrUnless) {
          item->asInstruction()->getResult(0)->location.type = Operand::Unused;
        }
      }
    }
  }
}

void JITCompiler::computeOperandLocations(size_t params_size,
                                          std::vector<Type>& results) {
  // Build space for locals first.
  ValueLocationAllocator local_allocator;
  ValueLocationAllocator* stack_allocator = new ValueLocationAllocator();

  for (size_t i = 0; i < params_size; i++) {
    ValueInfo value_info = locals_[i];
    local_allocator.push(value_info);
    stack_allocator->push(value_info);
  }

  for (auto it : results) {
    local_allocator.push(ValueLocation::typeToValueInfo(it));
  }

  size_t locals_start = local_allocator.alignedSize();
  size_t size = results.size();

  for (size_t i = 0; i < size; i++) {
    local_allocator.pop();
  }

  local_allocator.setStart(locals_start);
  size = locals_.size();

  if (params_size < size) {
    for (size_t i = params_size; i < size; i++) {
      local_allocator.push(locals_[i]);
    }

    stack_allocator->skipRange(locals_start, local_allocator.alignedSize());
  }

  // Compute stack allocation.
  for (InstructionListItem* item = first_; item != nullptr;
       item = item->next()) {
    if (item->isLabel()) {
      item->asLabel()->stack_allocator_ = nullptr;
    }
  }

  for (InstructionListItem* item = first_; item != nullptr;
       item = item->next()) {
    if (item->isLabel()) {
      if (item->asLabel()->stack_allocator_ == nullptr) {
        // Avoid backward branches to clone the allocator.
        item->asLabel()->stack_allocator_ = stack_allocator;
        continue;
      }

      delete stack_allocator;
      stack_allocator = item->asLabel()->stack_allocator_;
      continue;
    }

    Instruction* instr = item->asInstruction();

    // Pop params from the stack first.
    Index end = instr->paramCount();
    Operand* operand = instr->params() + end;

    for (Index i = end; i > 0; --i) {
      const ValueLocation& location = stack_allocator->back();

      --operand;

      if (location.status & ValueLocation::kIsOffset) {
        operand->value = location.value;
        operand->location.type = Operand::Stack;
      } else if (location.status & ValueLocation::kIsLocal) {
        operand->value = location.value;
        operand->location.type = Operand::Stack;
      } else if (location.status & ValueLocation::kIsRegister) {
        operand->value = location.value;
        operand->location.type = Operand::Register;
      } else {
        assert(location.status & ValueLocation::kIsImmediate &&
               location.value == 0);
        if (operand->item->isLabel()) {
          // Since operand->index is unavaliable after this point,
          // the item must point to the constant instruction.
          const Label::DependencyList& deps =
              operand->item->asLabel()->dependencies(operand->index);
          operand->item = deps[0].instr;
        }
        operand->location.type = Operand::Immediate;
      }

      operand->location.value_info = location.value_info;
      stack_allocator->pop();
    }

    // Push results next.
    if (instr->hasResult()) {
      end = instr->resultCount();
      operand = instr->results();

      for (Index i = 0; i < end; ++i) {
        switch (operand->location.type) {
          case Operand::Stack: {
            stack_allocator->push(operand->location.value_info);
            operand->value = stack_allocator->back().value;
            break;
          }
          case Operand::Register: {
            stack_allocator->pushReg(operand->value,
                                     operand->location.value_info);
            break;
          }
          case Operand::LocalSet: {
            operand->location.type = Operand::Stack;
            operand->value =
                local_allocator.getValue(operand->local_index).value;
            break;
          }
          case Operand::LocalGet: {
            assert(instr->opcode() == Opcode::LocalGet);
            operand->location.type = Operand::Unused;
            Index offset =
                local_allocator.getValue(instr->value().local_index).value;
            stack_allocator->pushLocal(offset, operand->location.value_info);
            break;
          }
          default: {
            assert(instr->group() == Instruction::Immediate ||
                   instr->group() == Instruction::Compare);
            stack_allocator->pushImm(operand->location.value_info);
            operand->value = 0;
            break;
          }
        }
      }
    }

    if (instr->group() == Instruction::LocalMove) {
      instr->value().value =
          local_allocator.getValue(instr->value().local_index).value;
    } else if (instr->group() == Instruction::DirectBranch) {
      Label* label = instr->value().target_label;
      if (label->stack_allocator_ == nullptr) {
        label->stack_allocator_ = new ValueLocationAllocator(*stack_allocator);
      }
    } else if (instr->opcode() == Opcode::BrTable) {
      Label** label = instr->asBrTable()->targetLabels();
      Label** end = label + instr->value().target_label_count;

      while (label < end) {
        if ((*label)->stack_allocator_ == nullptr) {
          (*label)->stack_allocator_ =
              new ValueLocationAllocator(*stack_allocator);
        }
        label++;
      }
    }
  }

  assert(stack_allocator->empty());
  delete stack_allocator;
}

}  // namespace interp
}  // namespace wabt
