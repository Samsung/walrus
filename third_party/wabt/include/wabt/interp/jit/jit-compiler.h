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

#ifndef WABT_JIT_COMPILER_H_
#define WABT_JIT_COMPILER_H_

#include "wabt/common.h"
#include "wabt/interp/jit/jit-allocator.h"
#include "wabt/opcode.h"

// Backend compiler structures.
struct sljit_compiler;
struct sljit_jump;
struct sljit_label;

namespace wabt {
namespace interp {

class JITFunction;
class Instruction;
class BrTableInstruction;
class Label;

class InstructionListItem {
  friend class JITCompiler;

 public:
  enum Group : uint8_t {
    // Not in a group.
    CodeLabel,
    // Generic instruction, also start of the instructions.
    Any,
    // I32Const, I64Const, F32Const, F64Const, V128Const.
    Immediate,
    // LocalGet or LocalSet.
    LocalMove,
    // Br, BrIf, InterpBrUnless.
    DirectBranch,
    // Return.
    Return,
    // Binary operation (e.g. I32Add, I64Sub).
    Binary,
    // Unary operation (e.g. I32Ctz, U64Clz).
    Unary,
    // Compare operation. (e.g. I32Eqz, I64LtU)
    Compare,
  };

  virtual ~InstructionListItem() {}

  InstructionListItem* next() { return next_; }
  InstructionListItem* prev() { return prev_; }

  uint16_t info() { return info_; }
  void setInfo(uint16_t value) { info_ = value; }
  void addInfo(uint16_t value) { info_ |= value; }
  void clearInfo(uint16_t value) { info_ &= static_cast<uint16_t>(~value); }
  Group group() { return static_cast<Group>(group_); }
  bool hasResult() { return result_count_ != 0; }

  bool isInstruction() { return group() >= Group::Any; }

  Instruction* asInstruction() {
    assert(isInstruction());
    return reinterpret_cast<Instruction*>(this);
  }

  bool isLabel() { return group() == Group::CodeLabel; }

  Label* asLabel() {
    assert(isLabel());
    return reinterpret_cast<Label*>(this);
  }

 protected:
  explicit InstructionListItem(Group group, InstructionListItem* prev)
      : next_(nullptr),
        prev_(prev),
        group_(group),
        result_count_(0),
        info_(0) {}

 private:
  InstructionListItem* next_;
  InstructionListItem* prev_;
  uint8_t group_;
  uint8_t result_count_;
  uint16_t info_;
};

struct Operand {
  enum LocationType : uint8_t {
    Stack,
    Register,
    // Represents a value that is not passed on the stack.
    // Usually happens when instructions are merged.
    Unused,
    // Cannot be a result type.
    Immediate,
    // Not allowed after computeOperandLocations() call.
    LocalSet,
    LocalGet,
  };

  static const uint16_t kIsOffset = 1 << 0;
  static const uint16_t kIsLocal = 1 << 1;
  static const uint16_t kIsRegister = 1 << 2;

  struct Location {
    uint8_t type;
    ValueInfo value_info;
  };

  union {
    // Dependency / immedate tracking.
    InstructionListItem* item;
    // For removed local.set instructions.
    Index local_index;
    // After the computeStackAllocation call, the value member
    // follows the same rules as the value in LocationInfo
    // except for constants where the item pointer is kept.
    Index value;
  };

  union {
    Index index;
    Location location;
  };
};

union InstructionValue {
  // For immediates.
  uint32_t value32;
  uint64_t value64;
  // For local management.
  Index local_index;
  Index value;
  // For direct branches.
  Label* target_label;
  // For BrTable instruction.
  size_t target_label_count;
  // For immediate and local groups.
  Instruction* parent;
};

#define WABT_JIT_INVALID_INSTRUCTION reinterpret_cast<Instruction*>(1)

class Instruction : public InstructionListItem {
  friend class JITCompiler;

 public:
  // Various info bits. Depends on type.
  static const uint16_t kKeepInstruction = 1 << 0;
  static const uint16_t kHasParent = 1 << 1;

  Opcode opcode() { return opcode_; }

  bool isConditionalBranch() {
    return opcode() == Opcode::BrIf || opcode() == Opcode::InterpBrUnless;
  }

  InstructionValue& value() { return value_; }

  // Params and results are stored in the same operands
  // array, where params come first followed by results.
  Operand* operands() { return operands_; }
  Index paramCount() { return param_count_; }
  Operand* getParam(size_t i) { return operands_ + i; }
  Operand* params() { return operands_; }
  Index resultCount();
  Operand* getResult(size_t i) {
    assert(hasResult());
    return operands_ + param_count_ + i;
  }
  Operand* results() {
    assert(hasResult());
    return operands_ + param_count_;
  }

  BrTableInstruction* asBrTable() {
    assert(opcode() == Opcode::BrTable);
    return reinterpret_cast<BrTableInstruction*>(this);
  }

 protected:
  explicit Instruction(Group group,
                       Opcode opcode,
                       Index param_count,
                       Operand* operands,
                       InstructionListItem* prev)
      : InstructionListItem(group, prev),
        opcode_(opcode),
        param_count_(param_count),
        operands_(operands) {}

  explicit Instruction(Group group, Opcode opcode, InstructionListItem* prev)
      : InstructionListItem(group, prev),
        opcode_(opcode),
        param_count_(0),
        operands_(nullptr) {}

 private:
  Opcode opcode_;
  Index param_count_;
  InstructionValue value_;
  Operand* operands_;
};

class BrTableInstruction : public Instruction {
  friend class JITCompiler;

 public:
  ~BrTableInstruction() override;

  Label** targetLabels() { return target_labels_; }

 protected:
  BrTableInstruction(size_t target_label_count, InstructionListItem* prev);

 private:
  Operand inlineParam_;
  Label** target_labels_;
};

template <int n>
class SimpleInstruction : public Instruction {
  friend class JITCompiler;

 protected:
  explicit SimpleInstruction(Group group,
                             Opcode opcode,
                             Index param_count,
                             InstructionListItem* prev)
      : Instruction(group, opcode, param_count, inlineOperands_, prev) {
    assert(param_count == n || param_count + 1 == n);
  }

  explicit SimpleInstruction(Group group,
                             Opcode opcode,
                             InstructionListItem* prev)
      : Instruction(group, opcode, n, inlineOperands_, prev) {}

 private:
  Operand inlineOperands_[n];
};

class ComplexInstruction : public Instruction {
  friend class JITCompiler;

 public:
  ~ComplexInstruction() override;

 protected:
  explicit ComplexInstruction(Group group,
                              Opcode opcode,
                              Index param_count,
                              Index operand_count,
                              InstructionListItem* prev)
      : Instruction(group,
                    opcode,
                    param_count,
                    new Operand[operand_count],
                    prev) {
    assert(operand_count >= param_count);
    assert(opcode == Opcode::Return);
  }
};

class DependencyGenContext;
class ReduceMovesContext;
struct LabelJumpList;
struct LabelData;

class Label : public InstructionListItem {
  friend class JITCompiler;

 public:
  // Various info bits.
  static const uint16_t kNewFunction = 1 << 0;
  static const uint16_t kAfterUncondBranch = 1 << 1;
  static const uint16_t kCanHaveElse = 1 << 2;
  static const uint16_t kHasJumpList = 1 << 3;
  static const uint16_t kHasLabelData = 1 << 4;

  struct Dependency {
    Dependency(Instruction* instr, size_t index) : instr(instr), index(index) {}

    Instruction* instr;
    Index index;
  };

  typedef std::vector<Dependency> DependencyList;

  Index resultCount() { return result_count_; }
  Index preservedCount() { return preserved_count_; }
  Index stackSize() { return preserved_count_ + result_count_; }

  const std::vector<Instruction*>& branches() { return branches_; }
  size_t dependencyCount() { return dependencies_.size(); }
  const DependencyList& dependencies(size_t i) { return dependencies_[i]; }

  void append(Instruction* instr);
  void remove(Instruction* instr);
  void replace(Instruction* instr, Instruction* other_instr);
  // Should be called before removing the other instruction.
  void merge(Label* other);

  void jumpFrom(sljit_jump* jump);
  void emit(sljit_compiler* compiler);

 private:
  explicit Label(Index result_count,
                 Index preserved_count,
                 InstructionListItem* prev)
      : InstructionListItem(CodeLabel, prev),
        result_count_(result_count),
        preserved_count_(preserved_count) {}

  // The number of items on the stack should
  // be the sum of result and preserved count.
  Index result_count_;
  Index preserved_count_;

  std::vector<Instruction*> branches_;
  std::vector<DependencyList> dependencies_;

  // Contexts used by different compiling stages
  union {
    DependencyGenContext* dependency_ctx_;
    ReduceMovesContext* reduce_moves_ctx_;
    StackAllocator* stack_allocator_;
    LabelJumpList* jump_list_;
    LabelData* label_data_;
  };
};

class JITCompiler {
 public:
  ~JITCompiler() {
    clear();
    releaseFunctionList();
  }

  InstructionListItem* first() { return first_; }
  InstructionListItem* last() { return last_; }

  void clear();

  Instruction* append(Instruction::Group group,
                      Opcode opcode,
                      Index param_count);
  Instruction* append(Instruction::Group group,
                      Opcode opcode,
                      Index param_count,
                      ValueInfo result);
  void appendBranch(Opcode opcode, Index depth);
  void appendElseLabel();
  void appendBrTable(Index num_targets,
                     Index* target_depths,
                     Index default_target_depth);

  void pushLabel(Opcode opcode, Index param_count, Index result_count);
  void popLabel();
  size_t labelCount() { return label_stack_.size(); }
  ValueInfo local(size_t i) { return locals_[i]; }
  std::vector<ValueInfo>& locals() { return locals_; }

  void appendFunction(JITFunction* jit_func, bool is_external);
  void dump(bool enable_colors, bool after_stack_computation);

  void buildParamDependencies();
  void reduceLocalAndConstantMoves();
  void optimizeBlocks();
  void computeOperandLocations(size_t params_size, std::vector<Type>& results);
  void compile();

 private:
  struct FunctionList {
    FunctionList(JITFunction* jit_func, Label* entry_label, bool is_external)
        : jit_func(jit_func),
          entry_label(entry_label),
          is_external(is_external) {}

    JITFunction* jit_func;
    Label* entry_label;
    bool is_external;
  };

  struct ElseBlock {
    ElseBlock(Instruction* branch, Index result_count, Index preserved_count)
        : branch(branch),
          result_count(result_count),
          preserved_count(preserved_count) {}

    Instruction* branch;
    Index result_count;
    Index preserved_count;
  };

  void append(InstructionListItem* item);
  InstructionListItem* remove(InstructionListItem* item);
  void replace(InstructionListItem* item, InstructionListItem* new_item);

  Instruction* appendInternal(Instruction::Group group,
                              Opcode opcode,
                              Index param_count,
                              Index operand_count,
                              ValueInfo result);

  // Backend operations.
  void releaseFunctionList();
  void emitProlog(size_t index);
  void emitEpilog(size_t index);

  InstructionListItem* first_ = nullptr;
  InstructionListItem* last_ = nullptr;

  // List of functions. Multiple functions are compiled
  // in one step to allow direct calls between them.
  InstructionListItem* function_list_first_ = nullptr;
  InstructionListItem* function_list_last_ = nullptr;

  Index stack_depth_ = 0;

  sljit_compiler* compiler_ = nullptr;

  std::vector<Label*> label_stack_;
  std::vector<ElseBlock> else_blocks_;
  std::vector<ValueInfo> locals_;
  std::vector<FunctionList> function_list_;
};

}  // namespace interp
}  // namespace wabt

#endif  // WABT_JIT_COMPILER_H_
