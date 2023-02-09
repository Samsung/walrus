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

#include "Walrus.h"

#include "jit/Compiler.h"
#include "runtime/JITExec.h"
#include "runtime/ObjectType.h"

#include <set>

namespace Walrus {

class DependencyGenContext {
public:
    struct Dependency {
        Dependency(InstructionListItem* item, size_t index)
            : item(item)
            , index(index)
        {
        }

        bool operator<(const Dependency& other) const
        {
            uintptr_t left = reinterpret_cast<uintptr_t>(item);
            uintptr_t right = reinterpret_cast<uintptr_t>(other.item);

            return (left < right) || (left == right && index < other.index);
        }

        InstructionListItem* item;
        Index index;
    };

    struct StackItem {
        std::vector<Dependency> inst_deps;
        std::vector<Dependency> label_deps;
    };

    DependencyGenContext(Label* label)
        : m_preservedCount(label->preservedCount())
        , start_index_(0)
    {
        stack_.resize(label->stackSize());
    }

    size_t startIndex() { return start_index_; }
    void setStartIndex(size_t value) { start_index_ = value; }
    std::vector<StackItem>* stack() { return &stack_; }
    StackItem& stackItem(size_t index) { return stack_[index]; }

    void update(std::vector<Operand>& deps);

private:
    static void append(std::vector<Dependency>& list,
                       InstructionListItem* item,
                       Index index);

    Index m_preservedCount;
    size_t start_index_;
    std::vector<StackItem> stack_;
};

void DependencyGenContext::update(std::vector<Operand>& deps)
{
    assert(deps.size() >= stack_.size());

    size_t size = stack_.size();
    size_t current = 0;

    for (size_t i = 0; i < size; i++, current++) {
        if (current == m_preservedCount) {
            // These items are ignored when the branch is executed.
            current += deps.size() - stack_.size();
        }

        Operand& dep = deps[current];

        if (dep.item->isInstruction()) {
            append(stack_[i].inst_deps, dep.item, dep.index);
        } else {
            append(stack_[i].label_deps, dep.item, dep.index);
        }
    }

    assert(current == deps.size() || size == m_preservedCount);
}

void DependencyGenContext::append(std::vector<Dependency>& list,
                                  InstructionListItem* item,
                                  Index index)
{
    for (auto it : list) {
        if (it.item == item && it.index == index) {
            return;
        }
    }

    list.push_back(Dependency(item, index));
}

static void findSelectResult(Instruction* select_instr)
{
    /* Must depend on an instruction which is placed before. */
    assert(!(select_instr->info() & Instruction::kHasResultValueInfo));

    ValueInfo value_info = LocationInfo::kNone;
    Operand* operand = select_instr->getParam(0);

    if (operand->item->isLabel()) {
        const Label::DependencyList& deps = operand->item->asLabel()->dependencies(operand->index);

        for (auto it : deps) {
            if (it.instr->opcode() != SelectOpcode || (it.instr->info() & Instruction::kHasResultValueInfo)) {
                value_info = it.instr->getResult(it.index)->location.valueInfo;
                break;
            }
        }

        assert(value_info != LocationInfo::kNone);
    } else {
        Instruction* instr = operand->item->asInstruction();

        assert(instr->opcode() != SelectOpcode || (instr->info() & Instruction::kHasResultValueInfo));

        value_info = instr->getResult(operand->index)->location.valueInfo;
        assert(value_info != LocationInfo::kNone);
    }

    select_instr->getResult(0)->location.valueInfo = value_info;
    select_instr->addInfo(Instruction::kHasResultValueInfo);
}

void JITCompiler::buildParamDependencies()
{
    for (InstructionListItem* item = m_first; item != nullptr;
         item = item->next()) {
        if (item->isLabel()) {
            Label* label = item->asLabel();
            label->m_dependencyCtx = new DependencyGenContext(label);
        }
    }

    bool update_deps = true;
    size_t start_index = 0;
    std::vector<Operand> current_deps;

    // Phase 1: the direct dependencies are computed for instructions
    // and labels (only labels can have multiple dependencies).
    for (InstructionListItem* item = m_first; item != nullptr;
         item = item->next()) {
        if (item->isLabel()) {
            if (start_index > 0) {
                assert(current_deps[0].item->isLabel());
                Label* label = current_deps[0].item->asLabel();
                label->m_dependencyCtx->setStartIndex(start_index);
            }

            // Build a dependency list which refers to the last label.
            Label* label = item->asLabel();

            if (update_deps) {
                label->m_dependencyCtx->update(current_deps);
                current_deps.clear();
            }

            start_index = label->m_dependencyCtx->stack()->size();

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
            Label* label = instr->value().targetLabel;
            label->m_dependencyCtx->update(current_deps);

            if (instr->opcode() == BrOpcode) {
                update_deps = false;
            }
        } else if (instr->group() == Instruction::BrTable) {
            Label** label = instr->asBrTable()->targetLabels();
            Label** end = label + instr->value().targetLabelCount;
            std::set<Label*> updated_labels;

            while (label < end) {
                if (updated_labels.insert(*label).second) {
                    (*label)->m_dependencyCtx->update(current_deps);
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

    for (InstructionListItem* item = m_first; item != nullptr;
         item = item->next()) {
        if (!item->isLabel()) {
            Instruction* instr = item->asInstruction();
            Operand* param = instr->params();

            for (Index i = instr->paramCount(); i > 0; --i) {
                if (param->item->isLabel()) {
                    assert(param->item->asLabel()->m_dependencyCtx->stack() == last_stack);
                    assert(param->index >= last_start_index);

                    std::vector<DependencyGenContext::Dependency> inst_deps = (*last_stack)[param->index].inst_deps;

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

        DependencyGenContext* context = item->asLabel()->m_dependencyCtx;

        last_stack = context->stack();
        last_start_index = static_cast<Index>(context->startIndex());
        size_t end = last_stack->size();

        for (size_t i = last_start_index; i < end; ++i) {
            std::vector<DependencyGenContext::Dependency> unprocessed_labels;
            std::set<DependencyGenContext::Dependency> processed_deps;
            std::vector<DependencyGenContext::Dependency>& inst_deps = (*last_stack)[i].inst_deps;

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
                DependencyGenContext::StackItem& item = dep.item->asLabel()->m_dependencyCtx->stackItem(dep.index);

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
    for (InstructionListItem* item = m_first; item != nullptr;
         item = item->next()) {
        if (!item->isLabel()) {
            if (item->group() == Instruction::Any && item->asInstruction()->opcode() == SelectOpcode) {
                // Label data has been computed.
                findSelectResult(item->asInstruction());
            }
            continue;
        }

        // Copy the final dependency data into
        // the dependency list of the label.
        Label* label = item->asLabel();
        DependencyGenContext* context = label->m_dependencyCtx;
        std::vector<DependencyGenContext::StackItem>* stack = context->stack();
        size_t start_index = context->startIndex();
        size_t end = stack->size();

        // Single item dependencies are
        // moved into the corresponding oeprand data.
        label->m_dependencies.resize(end - start_index);

        for (size_t i = start_index; i < end; ++i) {
            assert((*stack)[i].label_deps.empty());

            Label::DependencyList& list = label->m_dependencies[i - start_index];

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

void JITCompiler::optimizeBlocks()
{
    std::vector<Label::Dependency> stack;

    for (InstructionListItem* item = m_first; item != nullptr;
         item = item->next()) {
        if (item->isLabel()) {
            stack.clear();
            continue;
        }

        switch (item->group()) {
        case Instruction::Call: {
            CallInstruction* call_instr = reinterpret_cast<CallInstruction*>(item);

            LocalsAllocator locals_allocator(call_instr->paramStart());

            for (auto it : call_instr->functionType()->param()) {
                locals_allocator.allocate(LocationInfo::typeToValueInfo(it));
            }

            Index end = call_instr->paramCount();
            size_t size = stack.size();

            for (Index i = 0; i < end; ++i) {
                if (size + i >= end) {
                    Label::Dependency& item = stack[size + i - end];

                    Operand* operand = item.instr->getResult(item.index);

                    if (operand->location.type == Operand::Stack) {
                        operand->location.type = Operand::CallArg;
                        operand->value = locals_allocator.get(i).value;
                    }
                }
            }

            stack.clear();
            continue;
        }
        case Instruction::Compare: {
            assert(item->next() != nullptr);

            if (item->next()->group() == Instruction::DirectBranch || item->next()->group() == Instruction::Any) {
                Instruction* next = item->next()->asInstruction();

                if (next->opcode() == BrIfOpcode || next->opcode() == InterpBrUnlessOpcode || next->opcode() == SelectOpcode) {
                    item->asInstruction()->getResult(0)->location.type = Operand::Unused;
                }
            }
            break;
        }
        default: {
            break;
        }
        }

        Instruction* instr = item->asInstruction();
        Index end = instr->paramCount();

        for (Index i = end; i > 0; --i) {
            if (stack.empty()) {
                break;
            }

            stack.pop_back();
        }

        end = instr->resultCount();

        for (Index i = 0; i < end; i++) {
            stack.push_back(Label::Dependency(instr, i));
        }
    }
}

static StackAllocator* cloneAllocator(Label* label, StackAllocator* other)
{
    assert(other->values().size() >= label->stackSize());

    if (other->values().size() == label->stackSize()) {
        return new StackAllocator(*other);
    }

    StackAllocator* allocator = new StackAllocator(other, label->preservedCount());

    std::vector<LocationInfo>& other_values = other->values();
    Index end = label->resultCount();
    Index current = other_values.size() - end;

    for (Index i = 0; i < end; i++, current++) {
        LocationInfo& info = other_values[current];

        if (info.status & LocationInfo::kIsOffset) {
            allocator->push(info.valueInfo);
        } else {
            allocator->values().push_back(info);
        }
    }

    return allocator;
}

void JITCompiler::computeOperandLocations(JITFunction* jit_func,
                                          ValueTypeVector& results)
{
    // Build space for results first.
    StackAllocator* stack_allocator = new StackAllocator();

    for (auto it : results) {
        stack_allocator->push(LocationInfo::typeToValueInfo(it));
    }

    Index locals_start = stack_allocator->size();
    LocalsAllocator locals_allocator(locals_start);

    size_t size = m_locals.size();

    for (size_t i = 0; i < size; i++) {
        locals_allocator.allocate(m_locals[i]);
    }

    Index max_stack_size = locals_allocator.size();
    Index args_size = StackAllocator::alignedSize(max_stack_size);
    Index total_frame_size = args_size;
    Index max_call_frame_size = 0;

    stack_allocator->skipRange(locals_start, max_stack_size);

    // Compute stack allocation.
    for (InstructionListItem* item = m_first; item != nullptr;
         item = item->next()) {
        if (item->isLabel()) {
            Label* label = item->asLabel();
            label->m_stackAllocator = nullptr;
        }
    }

    for (InstructionListItem* item = m_first; item != nullptr;
         item = item->next()) {
        if (item->isLabel()) {
            Label* label = item->asLabel();

            max_call_frame_size += StackAllocator::alignedSize(max_stack_size);
            if (max_call_frame_size > total_frame_size) {
                total_frame_size = max_call_frame_size;
            }

            max_call_frame_size = 0;
            max_stack_size = 0;

            if (label->m_stackAllocator == nullptr) {
                // Avoid cloning the allocator when a block
                // can be executed after the previous block.
                assert(!(label->info() & Label::kAfterUncondBranch));
                label->m_stackAllocator = stack_allocator;
                continue;
            }

            assert(label->info() & Label::kAfterUncondBranch);
            delete stack_allocator;
            stack_allocator = item->asLabel()->m_stackAllocator;
            continue;
        }

        Instruction* instr = item->asInstruction();

        // Pop params from the stack first.
        Index end = instr->paramCount();
        Operand* operand = instr->params() + end;

        for (Index i = end; i > 0; --i) {
            const LocationInfo& location = stack_allocator->values().back();

            --operand;

            if (location.status & LocationInfo::kIsOffset) {
                operand->value = location.value;
                operand->location.type = Operand::Stack;
            } else if (location.status & LocationInfo::kIsLocal) {
                operand->value = location.value;
                operand->location.type = Operand::Stack;
            } else if (location.status & LocationInfo::kIsRegister) {
                operand->value = location.value;
                operand->location.type = Operand::Register;
            } else if (location.status & LocationInfo::kIsCallArg) {
                operand->value = location.value;
                operand->location.type = Operand::CallArg;
            } else if (location.status & LocationInfo::kIsImmediate) {
                assert(location.value == 0);
                if (operand->item->isLabel()) {
                    // Since operand->index is unavaliable after this point,
                    // the item must point to the constant instruction.
                    const Label::DependencyList& deps = operand->item->asLabel()->dependencies(operand->index);
                    operand->item = deps[0].instr;
                }
                operand->location.type = Operand::Immediate;
            } else {
                assert((location.status & LocationInfo::kIsUnused) && location.value == 0);
                operand->location.type = Operand::Unused;
            }

            operand->location.valueInfo = location.valueInfo;
            stack_allocator->pop();
        }

        // Push results next.
        if (instr->hasResult()) {
            end = instr->resultCount();
            operand = instr->results();

            for (Index i = 0; i < end; ++i) {
                switch (operand->location.type) {
                case Operand::Stack: {
                    stack_allocator->push(operand->location.valueInfo);
                    operand->value = stack_allocator->values().back().value;
                    break;
                }
                case Operand::Register: {
                    stack_allocator->pushReg(operand->value,
                                             operand->location.valueInfo);
                    break;
                }
                case Operand::CallArg: {
                    operand->location.type = Operand::CallArg;
                    stack_allocator->pushCallArg(operand->value,
                                                 operand->location.valueInfo);
                    break;
                }
                case Operand::Immediate: {
                    assert(instr->group() == Instruction::Immediate);
                    stack_allocator->pushImmediate(operand->location.valueInfo);
                    operand->location.type = Operand::Unused;
                    operand->value = 0;
                    break;
                }
                case Operand::LocalSet: {
                    operand->location.type = Operand::Stack;
                    operand->value = locals_allocator.get(operand->localIndex).value;
                    stack_allocator->pushUnused(operand->location.valueInfo);
                    break;
                }
                case Operand::LocalGet: {
                    assert(instr->opcode() == LocalGetOpcode);
                    operand->location.type = Operand::Unused;
                    Index offset = locals_allocator.get(instr->value().localIndex).value;
                    stack_allocator->pushLocal(offset, operand->location.valueInfo);
                    break;
                }
                default: {
                    assert(operand->location.type == Operand::Unused);
                    stack_allocator->pushUnused(operand->location.valueInfo);
                    break;
                }
                }

                operand++;
            }

            if (max_stack_size < stack_allocator->size()) {
                max_stack_size = stack_allocator->size();
            }
        }

        switch (instr->group()) {
        case Instruction::DirectBranch: {
            Label* label = instr->value().targetLabel;
            if ((label->info() & Label::kAfterUncondBranch) && label->m_stackAllocator == nullptr) {
                label->m_stackAllocator = cloneAllocator(label, stack_allocator);
            }
            break;
        }
        case Instruction::BrTable: {
            Label** label = instr->asBrTable()->targetLabels();
            Label** end = label + instr->value().targetLabelCount;

            while (label < end) {
                if (((*label)->info() & Label::kAfterUncondBranch) && (*label)->m_stackAllocator == nullptr) {
                    (*label)->m_stackAllocator = cloneAllocator(*label, stack_allocator);
                }
                label++;
            }
            break;
        }
        case Instruction::Call: {
            Index frame_size = instr->asCall()->frameSize();

            if (max_call_frame_size < frame_size) {
                max_call_frame_size = frame_size;
            }
            break;
        }
        default: {
            break;
        }
        }
    }

    assert(stack_allocator->empty() || m_last->group() != Instruction::Return);
    delete stack_allocator;

    max_call_frame_size += StackAllocator::alignedSize(max_stack_size);
    if (max_call_frame_size > total_frame_size) {
        total_frame_size = max_call_frame_size;
    }

    assert(total_frame_size >= args_size && StackAllocator::alignedSize(total_frame_size) == total_frame_size);
    jit_func->initSizes(args_size, total_frame_size - args_size);

    Index args_start = total_frame_size - args_size;

    for (InstructionListItem* item = m_first; item != nullptr;
         item = item->next()) {
        if (item->isLabel()) {
            continue;
        }

        Instruction* instr = item->asInstruction();

        // Pop params from the stack first.
        Operand* operand = instr->operands();
        Operand* operand_end = operand + instr->paramCount() + instr->resultCount();

        if (instr->group() == Instruction::LocalMove) {
            assert(operand + 1 == operand_end);

            Index value = locals_allocator.get(instr->value().localIndex).value;

            if (value < args_size) {
                value += args_start;
            } else {
                Index length = LocationInfo::length(operand->location.valueInfo);

                assert(total_frame_size >= value + length);
                value = total_frame_size - value - length;
            }

            instr->value().value = value;
        }

        while (operand < operand_end) {
            if (operand->location.type == Operand::CallArg) {
                operand->location.type = Operand::Stack;
            } else if (operand->location.type == Operand::Stack) {
                if (operand->value < args_size) {
                    operand->value += args_start;
                } else {
                    Index length = LocationInfo::length(operand->location.valueInfo);

                    assert(operand->value >= operand->value + length);
                    operand->value = total_frame_size - operand->value - length;
                }
            }

            operand++;
        }
    }
}

} // namespace Walrus
