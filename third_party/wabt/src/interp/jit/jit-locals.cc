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

#include "wabt/interp/interp.h"
#include "wabt/interp/jit/jit-compiler.h"

#include <set>

namespace wabt {
namespace interp {

class SetLocalContext {
 public:
  static const size_t mask = 8 * sizeof(uint32_t) - 1;
  static const size_t shift = 5;

  SetLocalContext(size_t size) : _initialized(getSize(size)) {}

  void set(size_t i) { _initialized[i >> shift] |= 1 << (i & mask); }
  bool get(size_t i) {
    return (_initialized[i >> shift] & (1 << (i & mask))) != 0;
  }
  SetLocalContext* clone() { return new SetLocalContext(_initialized); }
  void updateTarget(SetLocalContext* target);

 private:
  SetLocalContext(std::vector<uint32_t>& other) : _initialized(other) {}

  size_t getSize(size_t size) { return (size + mask) & ~mask; }

  // First initializetion of a local variable.
  std::vector<uint32_t> _initialized;
};

void SetLocalContext::updateTarget(SetLocalContext* target) {
  assert(_initialized.size() == target->_initialized.size());

  size_t end = _initialized.size();
  std::vector<uint32_t>& target_initialized = target->_initialized;

  for (size_t i = 0; i < end; i++) {
    target_initialized[i] &= _initialized[i];
  }
}

struct LocalRange {
  LocalRange(size_t index) : start(0), end(0), index(index) {}

  bool operator<(const LocalRange& other) const {
    return (start < other.start);
  }

  // The range includes both start and end instructions.
  // Hence, if start equals to end, the range is one instruction long.
  size_t start;
  size_t end;
  size_t index;
};

void JITCompiler::checkLocals(size_t params_size) {
  size_t locals_size = locals_.size();
  SetLocalContext* current = new SetLocalContext(locals_size);

  std::vector<LocalRange> local_ranges;
  std::vector<bool> local_uninitialized(locals_size);

  local_ranges.reserve(locals_size);

  for (size_t i = 0; i < locals_size; i++) {
    local_ranges.push_back(LocalRange(i));
  }

  // Phase 1: compute the start and end regions of all locals,
  // and compute whether the local is accessible from the entry
  // point of the function

  size_t instr_index = 0;
  for (InstructionListItem* item = first_; item != nullptr;
       item = item->next()) {
    instr_index++;

    if (item->isLabel()) {
      Label* label = item->asLabel();

      label->addInfo(Label::kCheckLocalsReached);

      assert((label->info() & Label::kCheckLocalsHasContext) ||
             current != nullptr);

      if (label->info() & Label::kCheckLocalsHasContext) {
        if (current != nullptr) {
          current->updateTarget(label->set_local_ctx_);
        }

        delete current;
        current = label->set_local_ctx_;
      }

      label->instr_index_ = instr_index;
      continue;
    }

    Instruction* instr = item->asInstruction();

    switch (instr->group()) {
      case Instruction::LocalMove: {
        Index local_index = instr->value().local_index;
        LocalRange& range = local_ranges[local_index];

        if (range.start == 0) {
          range.start = instr_index;
        }

        range.end = instr_index;

        if (instr->opcode() == Opcode::LocalSet) {
          current->set(local_index);
        } else if (!current->get(local_index)) {
          local_uninitialized[local_index] = true;
        }
        break;
      }
      case Instruction::DirectBranch: {
        Label* label = instr->value().target_label;

        if (!(label->info() & Label::kCheckLocalsReached)) {
          if (label->info() & Label::kCheckLocalsHasContext) {
            current->updateTarget(label->set_local_ctx_);
          } else {
            label->set_local_ctx_ = current->clone();
            label->addInfo(Label::kCheckLocalsHasContext);
          }
        }

        if (instr->opcode() == Opcode::Br) {
          delete current;
          current = nullptr;
        }
        break;
      }
      case Instruction::BrTable: {
        Label** label = instr->asBrTable()->targetLabels();
        Label** end = label + instr->value().target_label_count;
        std::set<Label*> updated_labels;

        while (label < end) {
          if (updated_labels.insert(*label).second) {
            if (!((*label)->info() & Label::kCheckLocalsReached)) {
              if ((*label)->info() & Label::kCheckLocalsHasContext) {
                current->updateTarget((*label)->set_local_ctx_);
              } else {
                (*label)->set_local_ctx_ = current->clone();
                (*label)->addInfo(Label::kCheckLocalsHasContext);
              }
            }
          }
          label++;
        }

        delete current;
        current = nullptr;
        break;
      }
      default: {
        break;
      }
    }
  }

  delete current;

  for (size_t i = 0; i < params_size; i++) {
    local_ranges[i].start = 0;
  }

  for (size_t i = params_size; i < locals_size; i++) {
    if (local_uninitialized[i]) {
      local_ranges[i].start = 0;
    }
  }

  // Phase 2: extend the range ends with intersecting backward jumps

  instr_index = 0;
  for (InstructionListItem* item = first_; item != nullptr;
       item = item->next()) {
    instr_index++;

    switch (item->group()) {
      case InstructionListItem::CodeLabel: {
        item->clearInfo(Label::kCheckLocalsReached |
                        Label::kCheckLocalsHasContext);
        break;
      }
      case Instruction::DirectBranch: {
        Instruction* instr = item->asInstruction();
        size_t label_instr_index = instr->value().target_label->instr_index_;

        if (label_instr_index <= instr_index) {
          for (size_t i = 0; i < locals_size; i++) {
            LocalRange& range = local_ranges[i];

            if (range.end < instr_index && label_instr_index <= range.end &&
                range.start <= label_instr_index) {
              range.end = instr_index;
            }
          }
        }
        break;
      }
      case Instruction::BrTable: {
        Instruction* instr = item->asInstruction();
        Label** label = instr->asBrTable()->targetLabels();
        Label** end = label + instr->value().target_label_count;
        std::set<Label*> updated_labels;

        while (label < end) {
          if (updated_labels.insert(*label).second) {
            size_t label_instr_index = (*label)->instr_index_;

            if (label_instr_index <= instr_index) {
              for (size_t i = 0; i < locals_size; i++) {
                LocalRange& range = local_ranges[i];

                if (range.end < instr_index && label_instr_index <= range.end &&
                    range.start <= label_instr_index) {
                  range.end = instr_index;
                }
              }
            }
          }
        }
        break;
      }
      default: {
        break;
      }
    }
  }

  std::sort(local_ranges.begin() + params_size, local_ranges.end());

  // Phase 3: assign new local indicies to the existing locals

  std::vector<ValueInfo> new_locals;
  std::vector<size_t> new_locals_end;
  std::vector<size_t> remap;

  remap.resize(locals_size);

  for (size_t i = 0; i < locals_size; i++) {
    LocalRange& range = local_ranges[i];

    if (range.end == 0 && range.index >= params_size) {
      continue;
    }

    ValueInfo type = locals_[range.index];
    size_t end = new_locals.size();
    size_t new_id = 0;

    while (new_id < end) {
      if (new_locals_end[new_id] < range.start && type == new_locals[new_id]) {
        new_locals_end[new_id] = range.end;
        break;
      }
      new_id++;
    }

    remap[range.index] = new_id;

    if (new_id < end) {
      continue;
    }

    new_locals.push_back(type);
    new_locals_end.push_back(range.end);
  }

  // Phase 4: remap values

  if (verboseLevel() >= 1) {
    for (size_t i = 0; i < locals_size; i++) {
      LocalRange& range = local_ranges[i];
      printf("Local %d: mapped to: %d ", static_cast<int>(range.index),
             static_cast<int>(remap[range.index]));

      if (range.end == 0) {
        printf("unused\n");
      } else if (range.start == 0) {
        printf("range [BEGIN - %d]\n", static_cast<int>(range.end - 1));
      } else {
        printf("range [%d - %d]\n", static_cast<int>(range.start - 1),
               static_cast<int>(range.end - 1));
      }
    }

    printf("Number of locals reduced from %d to %d\n",
           static_cast<int>(locals_size), static_cast<int>(new_locals.size()));
  }

  for (InstructionListItem* item = last_; item != nullptr;
       item = item->prev()) {
    if (item->group() == Instruction::LocalMove) {
      Instruction* instr = item->asInstruction();
      instr->value().local_index = remap[instr->value().local_index];
    }
  }

  locals_ = new_locals;
}

}  // namespace interp
}  // namespace wabt
