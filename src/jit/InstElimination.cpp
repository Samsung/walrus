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

#include <set>

namespace Walrus {

class ReduceMovesContext {
public:
    struct StackItem {
        StackItem()
            : expected_group(WABT_JIT_INVALID_INSTRUCTION)
        {
        }

        // Possible values for expected_group:
        //   instruction - head of a LocalSet or LocalGet group (might be dead)
        //   nullptr - dead group
        //   WABT_JIT_INVALID_INSTRUCTION - unknown group
        Instruction* expected_group;
        std::vector<Label*> label_deps;
    };

    struct InitGroups {
        InitGroups(Instruction* head_instr)
            : head_instr(head_instr)
            , is_dead_group(false)
        {
        }

        void updateLocalGet(Instruction* instr);
        void updateLocalSet(Instruction* instr, Index index);
        void updateImmediate(Instruction* instr);

        void reset()
        {
            head_instr = nullptr;
            is_dead_group = false;
        }

        Instruction* head_instr;
        bool is_dead_group;
    };

    ReduceMovesContext(Label* label)
        : m_preservedCount(label->preservedCount())
    {
        stack_.resize(label->stackSize());
    }

    std::vector<StackItem>* stack() { return &stack_; }
    StackItem& stackItem(size_t index) { return stack_[index]; }

    void update(std::vector<InstructionListItem*>& item_stack);
    static Instruction* getHead(Instruction* instr);
    static void checkStack(Instruction* instr,
                           std::vector<InstructionListItem*>& stack);

private:
    Index m_preservedCount;
    std::vector<StackItem> stack_;
};

void ReduceMovesContext::update(std::vector<InstructionListItem*>& item_stack)
{
    assert(item_stack.size() >= stack_.size());

    size_t size = stack_.size();
    size_t current = 0;

    for (size_t i = 0; i < size; i++, current++) {
        if (current == m_preservedCount) {
            // These items are ignored when the branch is executed.
            current += item_stack.size() - stack_.size();
        }

        InstructionListItem* item = item_stack[current];
        StackItem& stack_item = stack_[i];

        if (item == nullptr) {
            if (stack_item.expected_group != nullptr && stack_item.expected_group != WABT_JIT_INVALID_INSTRUCTION) {
                stack_item.expected_group->addInfo(Instruction::kKeepInstruction);
            }

            stack_item.expected_group = nullptr;
            continue;
        }

        if (item->isInstruction()) {
            assert(stack_item.expected_group == nullptr || stack_item.expected_group == WABT_JIT_INVALID_INSTRUCTION || stack_item.expected_group == item);
            assert(!(item->asInstruction()->info() & Instruction::kKeepInstruction));

            if (stack_item.expected_group == WABT_JIT_INVALID_INSTRUCTION) {
                stack_item.expected_group = item->asInstruction();
            } else if (stack_item.expected_group == nullptr) {
                item->asInstruction()->addInfo(Instruction::kKeepInstruction);
                item_stack[current] = nullptr;
            }
            continue;
        }

        bool found = false;

        for (auto it : stack_item.label_deps) {
            if (it == item) {
                found = true;
                break;
            }
        }

        if (!found) {
            stack_item.label_deps.push_back(item->asLabel());
        }
    }
}

Instruction* ReduceMovesContext::getHead(Instruction* instr)
{
    assert(instr->group() == Instruction::LocalMove || instr->group() == Instruction::Immediate);
    assert(instr->info() & Instruction::kHasParent);

    Instruction* head_instr = instr;

    do {
        head_instr = head_instr->value().parent;
    } while (head_instr->info() & Instruction::kHasParent);

    instr->value().parent = head_instr;
    return head_instr;
}

void ReduceMovesContext::checkStack(Instruction* instr,
                                    std::vector<InstructionListItem*>& stack)
{
    Instruction* head_instr = instr;
    if (head_instr->asInstruction()->info() & Instruction::kHasParent) {
        head_instr = ReduceMovesContext::getHead(head_instr);
    }

    Index local_index = head_instr->value().localIndex;
    size_t end = stack.size();
    OpcodeKind discarded = LocalGetOpcode;

    if (instr->opcode() == LocalSetOpcode) {
        discarded = EndOpcode;
    }

    for (size_t i = 0; i < end; i++) {
        InstructionListItem* stack_item = stack[i];

        if (stack_item != nullptr && stack_item->isInstruction()) {
            Instruction* stack_instr = stack_item->asInstruction();

            assert(!(stack_instr->info() & Instruction::kHasParent));

            if (stack_instr->value().localIndex == local_index && stack_instr->opcode() != discarded) {
                stack_instr->addInfo(Instruction::kKeepInstruction);
                stack[i] = nullptr;
            }
        }
    }
}

void ReduceMovesContext::InitGroups::updateLocalGet(Instruction* instr)
{
    assert(head_instr == nullptr || !(head_instr->info() & Instruction::kHasParent));
    assert(!is_dead_group || head_instr == nullptr);

    if (instr->opcode() != LocalGetOpcode) {
        is_dead_group = true;
    } else {
        if (instr->info() & Instruction::kHasParent) {
            instr = ReduceMovesContext::getHead(instr);
        }

        if (instr->info() & Instruction::kKeepInstruction) {
            is_dead_group = true;
        } else if (is_dead_group) {
            head_instr = instr;
        }
    }

    if (is_dead_group) {
        // The instr is not LocalGetOpcode, already has the
        // kKeepInstruction flag, or moved to head_instr.
        if (head_instr != nullptr) {
            head_instr->addInfo(Instruction::kKeepInstruction);
            head_instr = nullptr;
        }
        return;
    }

    if (head_instr == nullptr || head_instr == instr) {
        head_instr = instr;
        return;
    }

    if (instr->value().localIndex == head_instr->value().localIndex) {
        // Combine only alive groups.
        instr->value().parent = head_instr;
        instr->addInfo(Instruction::kHasParent);
        return;
    }

    is_dead_group = true;
    instr->addInfo(Instruction::kKeepInstruction);
    head_instr->addInfo(Instruction::kKeepInstruction);
    head_instr = nullptr;
}

void ReduceMovesContext::InitGroups::updateLocalSet(Instruction* instr,
                                                    Index index)
{
    assert(head_instr == nullptr || !(head_instr->info() & Instruction::kHasParent));
    assert(is_dead_group == (head_instr == nullptr));

    Operand* result = instr->getResult(index);

    if (result->item == nullptr) {
        if (!is_dead_group && (instr->opcode() == LocalGetOpcode || instr->group() == Instruction::Immediate)) {
            if (instr->info() & Instruction::kHasParent) {
                instr = ReduceMovesContext::getHead(instr);
            }

            /* Ignore alive sets. */
            if (!(instr->info() & Instruction::kKeepInstruction)) {
                /* Alive sets cover all paths. */
                head_instr->addInfo(Instruction::kKeepInstruction);
                head_instr = nullptr;
                is_dead_group = true;
            }
        }

        if (is_dead_group) {
            result->item = WABT_JIT_INVALID_INSTRUCTION;
        } else {
            result->item = head_instr;
        }
        return;
    }

    if (result->item == WABT_JIT_INVALID_INSTRUCTION) {
        if (!is_dead_group) {
            head_instr->addInfo(Instruction::kKeepInstruction);
            head_instr = nullptr;
        }

        is_dead_group = true;
        return;
    }

    instr = result->item->asInstruction();

    if (instr->info() & Instruction::kHasParent) {
        instr = ReduceMovesContext::getHead(instr);
        result->item = instr;
    }

    if (is_dead_group) {
        instr->addInfo(Instruction::kKeepInstruction);
        return;
    }

    if (!(instr->info() & Instruction::kKeepInstruction) && instr->value().localIndex == head_instr->value().localIndex) {
        // Combine only alive groups.
        if (instr != head_instr) {
            instr->value().parent = head_instr;
            instr->addInfo(Instruction::kHasParent);
            result->item = head_instr;
        }
        return;
    }

    is_dead_group = true;
    result->item = WABT_JIT_INVALID_INSTRUCTION;

    instr->addInfo(Instruction::kKeepInstruction);
    head_instr->addInfo(Instruction::kKeepInstruction);
    head_instr = nullptr;
}

void ReduceMovesContext::InitGroups::updateImmediate(Instruction* instr)
{
    assert(head_instr == nullptr || !(head_instr->info() & Instruction::kHasParent));
    assert(!is_dead_group || head_instr == nullptr);

    if (instr->group() != Instruction::Immediate) {
        is_dead_group = true;
    } else {
        if (instr->info() & Instruction::kHasParent) {
            instr = ReduceMovesContext::getHead(instr);
        }

        if (instr->info() & Instruction::kKeepInstruction) {
            is_dead_group = true;
        } else if (is_dead_group) {
            head_instr = instr;
        }
    }

    if (is_dead_group) {
        // The instr is not immediate, already has the
        // kKeepInstruction flag, or moved to head_instr.
        if (head_instr != nullptr) {
            head_instr->addInfo(Instruction::kKeepInstruction);
            head_instr = nullptr;
        }
        return;
    }

    if (head_instr == nullptr || head_instr == instr) {
        head_instr = instr;
        return;
    }

    assert(instr->opcode() == head_instr->opcode());

    bool same = false;

    switch (instr->opcode()) {
    case I32ConstOpcode:
    case F32ConstOpcode:
        same = instr->value().value32 == head_instr->value().value32;
        break;
    case I64ConstOpcode:
    case F64ConstOpcode:
        same = instr->value().value64 == head_instr->value().value64;
        break;
    default:
        WABT_UNREACHABLE;
    }

    if (same) {
        // Combine only alive groups.
        instr->value().parent = head_instr;
        instr->addInfo(Instruction::kHasParent);
        return;
    }

    is_dead_group = true;
    instr->addInfo(Instruction::kKeepInstruction);
    head_instr->addInfo(Instruction::kKeepInstruction);
    head_instr = nullptr;
}

void JITCompiler::reduceLocalAndConstantMoves()
{
    // Must be Return or unconditional branch
    assert(m_last != nullptr && m_last->isInstruction());

    if (m_last->group() == Instruction::Return && m_last->asInstruction()->paramCount() > 0) {
        Operand* operand = m_last->asInstruction()->params();

        for (Index i = m_last->asInstruction()->paramCount(); i > 0; --i) {
            if (!operand->item->isLabel()) {
                Instruction* instr = operand->item->asInstruction();

                if (instr->opcode() == LocalGetOpcode || instr->group() == Instruction::Immediate) {
                    instr->addInfo(Instruction::kKeepInstruction);
                }
            } else {
                const Label::DependencyList& deps = operand->item->asLabel()->dependencies(operand->index);

                for (auto it : deps) {
                    Instruction* instr = it.instr;

                    if (instr->opcode() == LocalGetOpcode || instr->group() == Instruction::Immediate) {
                        instr->addInfo(Instruction::kKeepInstruction);
                    }
                }
            }
            operand++;
        }
    }

    for (InstructionListItem* item = m_first; item != nullptr;
         item = item->next()) {
        if (item->isLabel()) {
            Label* label = item->asLabel();
            label->m_reduceMovesCtx = new ReduceMovesContext(label);
            continue;
        }

        Instruction* instr = item->asInstruction();

        Operand* operand = instr->params();

        for (Index i = instr->paramCount(); i > 0; --i) {
            ReduceMovesContext::InitGroups init_local_get(nullptr);
            ReduceMovesContext::InitGroups init_immediate(nullptr);

            if (!operand->item->isLabel()) {
                init_local_get.updateLocalGet(operand->item->asInstruction());
                init_immediate.updateImmediate(operand->item->asInstruction());
            } else {
                const Label::DependencyList& deps = operand->item->asLabel()->dependencies(operand->index);

                for (auto it : deps) {
                    init_local_get.updateLocalGet(it.instr);
                    init_immediate.updateImmediate(it.instr);
                }
            }
            operand++;
        }

        if (instr->hasResult()) {
            operand = instr->results();

            for (Index i = instr->resultCount(); i > 0; --i) {
                operand->item = nullptr;
                operand++;
            }
        }
    }

    // Phase 1: initialize live groups using data dependencies.
    for (InstructionListItem* item = m_first; item != nullptr;
         item = item->next()) {
        if (item->group() != Instruction::LocalMove || item->asInstruction()->opcode() != LocalSetOpcode) {
            continue;
        }

        Instruction* instr = item->asInstruction();

        assert(!(instr->info() & Instruction::kHasParent));
        ReduceMovesContext::InitGroups init_groups(instr);

        Operand* param = instr->getParam(0);

        if (!param->item->isLabel()) {
            init_groups.updateLocalSet(param->item->asInstruction(), param->index);
            continue;
        }

        const Label::DependencyList& deps = param->item->asLabel()->dependencies(param->index);

        for (auto it : deps) {
            init_groups.updateLocalSet(it.instr, it.index);
        }
    }

    // Possible values for stack items:
    //   instruction - head of a live LocalSet or LocalGet group
    //   label - unknown group
    //   nullptr - dead group
    std::vector<InstructionListItem*> stack;
    bool update_stack = true;

    // Phase 2: check instruction blocks.
    for (InstructionListItem* item = m_first; item != nullptr;
         item = item->next()) {
        if (item->isLabel()) {
            Label* label = item->asLabel();

            if (update_stack) {
                label->m_reduceMovesCtx->update(stack);
            }

            size_t size = label->m_reduceMovesCtx->stack()->size();
            stack.resize(size);

            for (size_t i = 0; i < size; ++i) {
                stack[i] = label;
            }

            update_stack = true;
            continue;
        }

        Instruction* instr = item->asInstruction();

        for (Index i = instr->paramCount(); i > 0; --i) {
            stack.pop_back();
        }

        if (instr->group() == Instruction::LocalMove) {
            ReduceMovesContext::checkStack(instr, stack);
        }

        if (instr->opcode() == LocalGetOpcode) {
            Instruction* head_instr = instr;

            if (head_instr->info() & Instruction::kHasParent) {
                head_instr = ReduceMovesContext::getHead(head_instr);
            }

            if (!(head_instr->info() & Instruction::kKeepInstruction)) {
                stack.push_back(head_instr);
                continue;
            }
        }

        if (instr->hasResult()) {
            Operand* result = instr->results();

            for (Index i = instr->resultCount(); i > 0; --i) {
                if (result->item == nullptr || result->item == WABT_JIT_INVALID_INSTRUCTION) {
                    stack.push_back(nullptr);
                } else {
                    Instruction* head_instr = result->item->asInstruction();

                    assert(head_instr->group() == Instruction::LocalMove);

                    if (head_instr->info() & Instruction::kHasParent) {
                        head_instr = ReduceMovesContext::getHead(head_instr);
                        result->item = head_instr;
                    }

                    if (!(head_instr->info() & Instruction::kKeepInstruction)) {
                        stack.push_back(head_instr);
                    } else {
                        result->item = nullptr;
                        stack.push_back(nullptr);
                    }
                }

                result++;
            }
            continue;
        }

        if (instr->group() == Instruction::DirectBranch) {
            instr->value().targetLabel->m_reduceMovesCtx->update(stack);

            if (instr->opcode() == BrOpcode) {
                update_stack = false;
            }
        } else if (instr->opcode() == BrTableOpcode) {
            Label** label = instr->asBrTable()->targetLabels();
            Label** end = label + instr->value().targetLabelCount;
            std::set<Label*> updated_labels;

            while (label < end) {
                if (updated_labels.insert(*label).second) {
                    (*label)->m_reduceMovesCtx->update(stack);
                }
                label++;
            }
            update_stack = false;
        }
    }

    stack.clear();

    // Phase 3: check invalid sets.
    for (InstructionListItem* item = m_first; item != nullptr;
         item = item->next()) {
        if (item->isInstruction()) {
            continue;
        }

        ReduceMovesContext* context = item->asLabel()->m_reduceMovesCtx;
        std::vector<ReduceMovesContext::StackItem>* stack = context->stack();

        size_t end = stack->size();
        for (size_t i = 0; i < end; ++i) {
            ReduceMovesContext::StackItem& item = (*stack)[i];
            Instruction* expected_group = item.expected_group;

            if (expected_group == nullptr) {
                continue;
            }

            std::vector<Label*> unprocessed_labels;
            std::set<Label*> processed_labels;

            if (expected_group != WABT_JIT_INVALID_INSTRUCTION) {
                assert(!(expected_group->info() & Instruction::kHasParent));

                if (expected_group->info() & Instruction::kKeepInstruction) {
                    item.expected_group = nullptr;
                    continue;
                }
            }

            for (auto it : item.label_deps) {
                if (processed_labels.insert(it).second) {
                    unprocessed_labels.push_back(it);
                }
            }

            while (!unprocessed_labels.empty()) {
                Label* label = unprocessed_labels.back();
                unprocessed_labels.pop_back();

                ReduceMovesContext::StackItem& current_item = label->m_reduceMovesCtx->stackItem(i);

                if (current_item.expected_group == nullptr) {
                    item.expected_group = nullptr;

                    if (expected_group != WABT_JIT_INVALID_INSTRUCTION) {
                        expected_group->addInfo(Instruction::kKeepInstruction);
                    }
                    break;
                }

                if (expected_group == WABT_JIT_INVALID_INSTRUCTION) {
                    expected_group = current_item.expected_group;
                    item.expected_group = expected_group;
                }

                assert(current_item.expected_group == expected_group);

                for (auto it : item.label_deps) {
                    if (processed_labels.insert(it).second) {
                        unprocessed_labels.push_back(it);
                    }
                }
            }

            assert(item.expected_group != WABT_JIT_INVALID_INSTRUCTION);
        }
    }

    // Phase 4: check instruction blocks again.
    for (InstructionListItem* item = m_first; item != nullptr;
         item = item->next()) {
        if (item->isLabel()) {
            std::vector<ReduceMovesContext::StackItem>* context_stack = item->asLabel()->m_reduceMovesCtx->stack();

            size_t size = context_stack->size();
            size_t start = size - item->asLabel()->m_dependencies.size();
            stack.resize(size - start);

            for (size_t i = start; i < size; ++i) {
                Instruction* expected_group = (*context_stack)[i].expected_group;

                assert(expected_group != WABT_JIT_INVALID_INSTRUCTION);

                if (expected_group != nullptr && (expected_group->info() & Instruction::kKeepInstruction)) {
                    expected_group = nullptr;
                }

                stack[i - start] = expected_group;
            }

            delete item->asLabel()->m_reduceMovesCtx;
            continue;
        }

        Instruction* instr = item->asInstruction();

        for (Index i = instr->paramCount(); i > 0; --i) {
            stack.pop_back();
        }

        if (instr->group() == Instruction::LocalMove) {
            ReduceMovesContext::checkStack(instr, stack);
        }

        if (instr->hasResult()) {
            for (Index i = instr->resultCount(); i > 0; --i) {
                stack.push_back(nullptr);
            }
        }

        if (stack.size() == 0) {
            // Skip the rest of the block.
            while (item->next() != nullptr && item->next()->isInstruction()) {
                item = item->next();
            }
        }
    }

    // Phase 5: restore local_index.
    for (InstructionListItem* item = m_first; item != nullptr;
         item = item->next()) {
        if (item->isLabel()) {
            continue;
        }

        Instruction* instr = item->asInstruction();

        if (instr->group() != Instruction::LocalMove && instr->group() != Instruction::Immediate) {
            continue;
        }

        if (instr->info() & Instruction::kHasParent) {
            Instruction* head_instr = ReduceMovesContext::getHead(instr);

            assert((head_instr->info() & Instruction::kKeepInstruction) || !(instr->info() & Instruction::kKeepInstruction));

            instr->setInfo(head_instr->info() & Instruction::kKeepInstruction);
            instr->value() = head_instr->value();
        }

        if (instr->info() & Instruction::kKeepInstruction) {
            continue;
        }

        if (instr->opcode() != LocalSetOpcode) {
            instr->getResult(0)->location.type = (instr->opcode() == LocalGetOpcode)
                ? Operand::LocalGet
                : Operand::Immediate;
            continue;
        }

        Operand* param = instr->getParam(0);
        Index local_index = instr->value().localIndex;

        if (!param->item->isLabel()) {
            Operand* result = param->item->asInstruction()->getResult(param->index);

            assert(result->location.type == Operand::Stack || (result->location.type == Operand::LocalSet && result->localIndex == local_index));

            result->location.type = Operand::LocalSet;
            result->localIndex = local_index;
            continue;
        }

        const Label::DependencyList& deps = param->item->asLabel()->dependencies(param->index);

        for (auto it : deps) {
            Operand* result = it.instr->getResult(it.index);

            assert(result->location.type == Operand::Stack || (result->location.type == Operand::LocalSet && result->localIndex == local_index));

            result->location.type = Operand::LocalSet;
            result->localIndex = local_index;
        }
    }
}

} // namespace Walrus
