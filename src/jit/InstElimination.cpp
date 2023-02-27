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
            : expectedGroup(WABT_JIT_INVALID_INSTRUCTION)
        {
        }

        // Possible values for expectedGroup:
        //   instruction - head of a LocalSet or LocalGet group (might be dead)
        //   nullptr - dead group
        //   WABT_JIT_INVALID_INSTRUCTION - unknown group
        Instruction* expectedGroup;
        std::vector<Label*> labelDeps;
    };

    struct InitGroups {
        InitGroups(Instruction* headInstr)
            : headInstr(headInstr)
            , isDeadGroup(false)
        {
        }

        void updateLocalGet(Instruction* instr);
        void updateLocalSet(Instruction* instr, Index index);
        void updateImmediate(Instruction* instr);

        void reset()
        {
            headInstr = nullptr;
            isDeadGroup = false;
        }

        Instruction* headInstr;
        bool isDeadGroup;
    };

    ReduceMovesContext(Label* label)
        : m_preservedCount(label->preservedCount())
    {
        m_stack.resize(label->stackSize());
    }

    std::vector<StackItem>* stack() { return &m_stack; }
    StackItem& stackItem(size_t index) { return m_stack[index]; }

    void update(std::vector<InstructionListItem*>& itemStack);
    static Instruction* getHead(Instruction* instr);
    static void checkStack(Instruction* instr, std::vector<InstructionListItem*>& stack);

private:
    Index m_preservedCount;
    std::vector<StackItem> m_stack;
};

void ReduceMovesContext::update(std::vector<InstructionListItem*>& itemStack)
{
    assert(itemStack.size() >= m_stack.size());

    size_t size = m_stack.size();
    size_t current = 0;

    for (size_t i = 0; i < size; i++, current++) {
        if (current == m_preservedCount) {
            // These items are ignored when the branch is executed.
            current += itemStack.size() - m_stack.size();
        }

        InstructionListItem* item = itemStack[current];
        StackItem& stackItem = m_stack[i];

        if (item == nullptr) {
            if (stackItem.expectedGroup != nullptr && stackItem.expectedGroup != WABT_JIT_INVALID_INSTRUCTION) {
                stackItem.expectedGroup->addInfo(Instruction::kKeepInstruction);
            }

            stackItem.expectedGroup = nullptr;
            continue;
        }

        if (item->isInstruction()) {
            assert(stackItem.expectedGroup == nullptr || stackItem.expectedGroup == WABT_JIT_INVALID_INSTRUCTION || stackItem.expectedGroup == item);
            assert(!(item->asInstruction()->info() & Instruction::kKeepInstruction));

            if (stackItem.expectedGroup == WABT_JIT_INVALID_INSTRUCTION) {
                stackItem.expectedGroup = item->asInstruction();
            } else if (stackItem.expectedGroup == nullptr) {
                item->asInstruction()->addInfo(Instruction::kKeepInstruction);
                itemStack[current] = nullptr;
            }
            continue;
        }

        bool found = false;

        for (auto it : stackItem.labelDeps) {
            if (it == item) {
                found = true;
                break;
            }
        }

        if (!found) {
            stackItem.labelDeps.push_back(item->asLabel());
        }
    }
}

Instruction* ReduceMovesContext::getHead(Instruction* instr)
{
    assert(instr->group() == Instruction::LocalMove || instr->group() == Instruction::Immediate);
    assert(instr->info() & Instruction::kHasParent);

    Instruction* headInstr = instr;

    do {
        headInstr = headInstr->value().parent;
    } while (headInstr->info() & Instruction::kHasParent);

    instr->value().parent = headInstr;
    return headInstr;
}

void ReduceMovesContext::checkStack(Instruction* instr, std::vector<InstructionListItem*>& stack)
{
    Instruction* headInstr = instr;
    if (headInstr->asInstruction()->info() & Instruction::kHasParent) {
        headInstr = ReduceMovesContext::getHead(headInstr);
    }

    Index localIndex = headInstr->value().localIndex;
    size_t end = stack.size();
    OpcodeKind discarded = LocalGetOpcode;

    if (instr->opcode() == LocalSetOpcode) {
        discarded = EndOpcode;
    }

    for (size_t i = 0; i < end; i++) {
        InstructionListItem* stackItem = stack[i];

        if (stackItem != nullptr && stackItem->isInstruction()) {
            Instruction* stack_instr = stackItem->asInstruction();

            assert(!(stack_instr->info() & Instruction::kHasParent));

            if (stack_instr->value().localIndex == localIndex && stack_instr->opcode() != discarded) {
                stack_instr->addInfo(Instruction::kKeepInstruction);
                stack[i] = nullptr;
            }
        }
    }
}

void ReduceMovesContext::InitGroups::updateLocalGet(Instruction* instr)
{
    assert(headInstr == nullptr || !(headInstr->info() & Instruction::kHasParent));
    assert(!isDeadGroup || headInstr == nullptr);

    if (instr->opcode() != LocalGetOpcode) {
        isDeadGroup = true;
    } else {
        if (instr->info() & Instruction::kHasParent) {
            instr = ReduceMovesContext::getHead(instr);
        }

        if (instr->info() & Instruction::kKeepInstruction) {
            isDeadGroup = true;
        } else if (isDeadGroup) {
            headInstr = instr;
        }
    }

    if (isDeadGroup) {
        // The instr is not LocalGetOpcode, already has the
        // kKeepInstruction flag, or moved to headInstr.
        if (headInstr != nullptr) {
            headInstr->addInfo(Instruction::kKeepInstruction);
            headInstr = nullptr;
        }
        return;
    }

    if (headInstr == nullptr || headInstr == instr) {
        headInstr = instr;
        return;
    }

    if (instr->value().localIndex == headInstr->value().localIndex) {
        // Combine only alive groups.
        instr->value().parent = headInstr;
        instr->addInfo(Instruction::kHasParent);
        return;
    }

    isDeadGroup = true;
    instr->addInfo(Instruction::kKeepInstruction);
    headInstr->addInfo(Instruction::kKeepInstruction);
    headInstr = nullptr;
}

void ReduceMovesContext::InitGroups::updateLocalSet(Instruction* instr, Index index)
{
    assert(headInstr == nullptr || !(headInstr->info() & Instruction::kHasParent));
    assert(isDeadGroup == (headInstr == nullptr));

    Operand* result = instr->getResult(index);

    if (result->item == nullptr) {
        if (!isDeadGroup && (instr->opcode() == LocalGetOpcode || instr->group() == Instruction::Immediate)) {
            if (instr->info() & Instruction::kHasParent) {
                instr = ReduceMovesContext::getHead(instr);
            }

            /* Ignore alive sets. */
            if (!(instr->info() & Instruction::kKeepInstruction)) {
                /* Alive sets cover all paths. */
                headInstr->addInfo(Instruction::kKeepInstruction);
                headInstr = nullptr;
                isDeadGroup = true;
            }
        }

        if (isDeadGroup) {
            result->item = WABT_JIT_INVALID_INSTRUCTION;
        } else {
            result->item = headInstr;
        }
        return;
    }

    if (result->item == WABT_JIT_INVALID_INSTRUCTION) {
        if (!isDeadGroup) {
            headInstr->addInfo(Instruction::kKeepInstruction);
            headInstr = nullptr;
        }

        isDeadGroup = true;
        return;
    }

    instr = result->item->asInstruction();

    if (instr->info() & Instruction::kHasParent) {
        instr = ReduceMovesContext::getHead(instr);
        result->item = instr;
    }

    if (isDeadGroup) {
        instr->addInfo(Instruction::kKeepInstruction);
        return;
    }

    if (!(instr->info() & Instruction::kKeepInstruction) && instr->value().localIndex == headInstr->value().localIndex) {
        // Combine only alive groups.
        if (instr != headInstr) {
            instr->value().parent = headInstr;
            instr->addInfo(Instruction::kHasParent);
            result->item = headInstr;
        }
        return;
    }

    isDeadGroup = true;
    result->item = WABT_JIT_INVALID_INSTRUCTION;

    instr->addInfo(Instruction::kKeepInstruction);
    headInstr->addInfo(Instruction::kKeepInstruction);
    headInstr = nullptr;
}

void ReduceMovesContext::InitGroups::updateImmediate(Instruction* instr)
{
    assert(headInstr == nullptr || !(headInstr->info() & Instruction::kHasParent));
    assert(!isDeadGroup || headInstr == nullptr);

    if (instr->group() != Instruction::Immediate) {
        isDeadGroup = true;
    } else {
        if (instr->info() & Instruction::kHasParent) {
            instr = ReduceMovesContext::getHead(instr);
        }

        if (instr->info() & Instruction::kKeepInstruction) {
            isDeadGroup = true;
        } else if (isDeadGroup) {
            headInstr = instr;
        }
    }

    if (isDeadGroup) {
        // The instr is not immediate, already has the
        // kKeepInstruction flag, or moved to headInstr.
        if (headInstr != nullptr) {
            headInstr->addInfo(Instruction::kKeepInstruction);
            headInstr = nullptr;
        }
        return;
    }

    if (headInstr == nullptr || headInstr == instr) {
        headInstr = instr;
        return;
    }

    assert(instr->opcode() == headInstr->opcode());

    bool same = false;

    switch (instr->opcode()) {
    case I32ConstOpcode:
    case F32ConstOpcode:
        same = instr->value().value32 == headInstr->value().value32;
        break;
    case I64ConstOpcode:
    case F64ConstOpcode:
        same = instr->value().value64 == headInstr->value().value64;
        break;
    default:
        WABT_UNREACHABLE;
    }

    if (same) {
        // Combine only alive groups.
        instr->value().parent = headInstr;
        instr->addInfo(Instruction::kHasParent);
        return;
    }

    isDeadGroup = true;
    instr->addInfo(Instruction::kKeepInstruction);
    headInstr->addInfo(Instruction::kKeepInstruction);
    headInstr = nullptr;
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

    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (item->isLabel()) {
            Label* label = item->asLabel();
            label->m_reduceMovesCtx = new ReduceMovesContext(label);
            continue;
        }

        Instruction* instr = item->asInstruction();

        Operand* operand = instr->params();

        for (Index i = instr->paramCount(); i > 0; --i) {
            ReduceMovesContext::InitGroups initLocalGet(nullptr);
            ReduceMovesContext::InitGroups initImmediate(nullptr);

            if (!operand->item->isLabel()) {
                initLocalGet.updateLocalGet(operand->item->asInstruction());
                initImmediate.updateImmediate(operand->item->asInstruction());
            } else {
                const Label::DependencyList& deps = operand->item->asLabel()->dependencies(operand->index);

                for (auto it : deps) {
                    initLocalGet.updateLocalGet(it.instr);
                    initImmediate.updateImmediate(it.instr);
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
    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (item->group() != Instruction::LocalMove || item->asInstruction()->opcode() != LocalSetOpcode) {
            continue;
        }

        Instruction* instr = item->asInstruction();

        assert(!(instr->info() & Instruction::kHasParent));
        ReduceMovesContext::InitGroups initGroups(instr);

        Operand* param = instr->getParam(0);

        if (!param->item->isLabel()) {
            initGroups.updateLocalSet(param->item->asInstruction(), param->index);
            continue;
        }

        const Label::DependencyList& deps = param->item->asLabel()->dependencies(param->index);

        for (auto it : deps) {
            initGroups.updateLocalSet(it.instr, it.index);
        }
    }

    // Possible values for stack items:
    //   instruction - head of a live LocalSet or LocalGet group
    //   label - unknown group
    //   nullptr - dead group
    std::vector<InstructionListItem*> stack;
    bool updateStack = true;

    // Phase 2: check instruction blocks.
    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (item->isLabel()) {
            Label* label = item->asLabel();

            if (updateStack) {
                label->m_reduceMovesCtx->update(stack);
            }

            size_t size = label->m_reduceMovesCtx->stack()->size();
            stack.resize(size);

            for (size_t i = 0; i < size; ++i) {
                stack[i] = label;
            }

            updateStack = true;
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
            Instruction* headInstr = instr;

            if (headInstr->info() & Instruction::kHasParent) {
                headInstr = ReduceMovesContext::getHead(headInstr);
            }

            if (!(headInstr->info() & Instruction::kKeepInstruction)) {
                stack.push_back(headInstr);
                continue;
            }
        }

        if (instr->hasResult()) {
            Operand* result = instr->results();

            for (Index i = instr->resultCount(); i > 0; --i) {
                if (result->item == nullptr || result->item == WABT_JIT_INVALID_INSTRUCTION) {
                    stack.push_back(nullptr);
                } else {
                    Instruction* headInstr = result->item->asInstruction();

                    assert(headInstr->group() == Instruction::LocalMove);

                    if (headInstr->info() & Instruction::kHasParent) {
                        headInstr = ReduceMovesContext::getHead(headInstr);
                        result->item = headInstr;
                    }

                    if (!(headInstr->info() & Instruction::kKeepInstruction)) {
                        stack.push_back(headInstr);
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
                updateStack = false;
            }
        } else if (instr->opcode() == BrTableOpcode) {
            Label** label = instr->asBrTable()->targetLabels();
            Label** end = label + instr->value().targetLabelCount;
            std::set<Label*> updatedLabels;

            while (label < end) {
                if (updatedLabels.insert(*label).second) {
                    (*label)->m_reduceMovesCtx->update(stack);
                }
                label++;
            }
            updateStack = false;
        }
    }

    stack.clear();

    // Phase 3: check invalid sets.
    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (item->isInstruction()) {
            continue;
        }

        ReduceMovesContext* context = item->asLabel()->m_reduceMovesCtx;
        std::vector<ReduceMovesContext::StackItem>* stack = context->stack();

        size_t end = stack->size();
        for (size_t i = 0; i < end; ++i) {
            ReduceMovesContext::StackItem& item = (*stack)[i];
            Instruction* expectedGroup = item.expectedGroup;

            if (expectedGroup == nullptr) {
                continue;
            }

            std::vector<Label*> unprocessedLabels;
            std::set<Label*> processedLabels;

            if (expectedGroup != WABT_JIT_INVALID_INSTRUCTION) {
                assert(!(expectedGroup->info() & Instruction::kHasParent));

                if (expectedGroup->info() & Instruction::kKeepInstruction) {
                    item.expectedGroup = nullptr;
                    continue;
                }
            }

            for (auto it : item.labelDeps) {
                if (processedLabels.insert(it).second) {
                    unprocessedLabels.push_back(it);
                }
            }

            while (!unprocessedLabels.empty()) {
                Label* label = unprocessedLabels.back();
                unprocessedLabels.pop_back();

                ReduceMovesContext::StackItem& currentItem = label->m_reduceMovesCtx->stackItem(i);

                if (currentItem.expectedGroup == nullptr) {
                    item.expectedGroup = nullptr;

                    if (expectedGroup != WABT_JIT_INVALID_INSTRUCTION) {
                        expectedGroup->addInfo(Instruction::kKeepInstruction);
                    }
                    break;
                }

                if (expectedGroup == WABT_JIT_INVALID_INSTRUCTION) {
                    expectedGroup = currentItem.expectedGroup;
                    item.expectedGroup = expectedGroup;
                }

                assert(currentItem.expectedGroup == expectedGroup);

                for (auto it : item.labelDeps) {
                    if (processedLabels.insert(it).second) {
                        unprocessedLabels.push_back(it);
                    }
                }
            }

            assert(item.expectedGroup != WABT_JIT_INVALID_INSTRUCTION);
        }
    }

    // Phase 4: check instruction blocks again.
    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (item->isLabel()) {
            std::vector<ReduceMovesContext::StackItem>* contextStack = item->asLabel()->m_reduceMovesCtx->stack();

            size_t size = contextStack->size();
            size_t start = size - item->asLabel()->m_dependencies.size();
            stack.resize(size - start);

            for (size_t i = start; i < size; ++i) {
                Instruction* expectedGroup = (*contextStack)[i].expectedGroup;

                assert(expectedGroup != WABT_JIT_INVALID_INSTRUCTION);

                if (expectedGroup != nullptr && (expectedGroup->info() & Instruction::kKeepInstruction)) {
                    expectedGroup = nullptr;
                }

                stack[i - start] = expectedGroup;
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

    // Phase 5: restore localIndex.
    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (item->isLabel()) {
            continue;
        }

        Instruction* instr = item->asInstruction();

        if (instr->group() != Instruction::LocalMove && instr->group() != Instruction::Immediate) {
            continue;
        }

        if (instr->info() & Instruction::kHasParent) {
            Instruction* headInstr = ReduceMovesContext::getHead(instr);

            assert((headInstr->info() & Instruction::kKeepInstruction) || !(instr->info() & Instruction::kKeepInstruction));

            instr->setInfo(headInstr->info() & Instruction::kKeepInstruction);
            instr->value() = headInstr->value();
        }

        if (instr->info() & Instruction::kKeepInstruction) {
            continue;
        }

        if (instr->opcode() != LocalSetOpcode) {
            instr->getResult(0)->location.type = (instr->opcode() == LocalGetOpcode) ? Operand::LocalGet : Operand::Immediate;
            continue;
        }

        Operand* param = instr->getParam(0);
        Index localIndex = instr->value().localIndex;

        if (!param->item->isLabel()) {
            Operand* result = param->item->asInstruction()->getResult(param->index);

            assert(result->location.type == Operand::Stack || (result->location.type == Operand::LocalSet && result->localIndex == localIndex));

            result->location.type = Operand::LocalSet;
            result->localIndex = localIndex;
            continue;
        }

        const Label::DependencyList& deps = param->item->asLabel()->dependencies(param->index);

        for (auto it : deps) {
            Operand* result = it.instr->getResult(it.index);

            assert(result->location.type == Operand::Stack || (result->location.type == Operand::LocalSet && result->localIndex == localIndex));

            result->location.type = Operand::LocalSet;
            result->localIndex = localIndex;
        }
    }
}

} // namespace Walrus
