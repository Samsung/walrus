/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

class DependencyGenContext {
public:
    static const uint8_t kOptReferenced = 1 << 0;
    static const uint8_t kOptHasAnyInstruction = 1 << 1;
    static const uint8_t kOptDependencyComputed = 1 << 2;

    struct StackItem {
        StackItem()
            : options(0)
        {
        }

        std::set<InstructionListItem*> dependencies;
        uint8_t options;
    };

    DependencyGenContext(uint32_t requiredStackSize)
    {
        m_stack.resize(requiredStackSize);
    }

    std::vector<StackItem>* stack() { return &m_stack; }
    StackItem& stackItem(size_t index) { return m_stack[index]; }

    void update(std::vector<InstructionListItem*>& dependencies);

private:
    std::vector<StackItem> m_stack;
};

void DependencyGenContext::update(std::vector<InstructionListItem*>& dependencies)
{
    assert(dependencies.size() == m_stack.size());

    size_t size = m_stack.size();

    for (size_t i = 0; i < size; i++) {
        InstructionListItem* dependency = dependencies[i];

        if (dependency == nullptr) {
            m_stack[i].options |= DependencyGenContext::kOptHasAnyInstruction;
            continue;
        }

        m_stack[i].dependencies.insert(dependency);
    }
}

static bool isSameConst(std::vector<Instruction*>& instDeps)
{
    Instruction* constInstr = instDeps[0];

    if (constInstr->group() != Instruction::Immediate) {
        return false;
    }

    if (constInstr->opcode() == Const32Opcode) {
        uint32_t value = reinterpret_cast<Const32*>(constInstr->byteCode())->value();
        size_t end = instDeps.size();

        for (size_t i = 1; i < end; i++) {
            Instruction* instr = instDeps[i];

            if (instr->opcode() != Const32Opcode || reinterpret_cast<Const32*>(instr->byteCode())->value() != value) {
                return false;
            }
        }

        return true;
    }

    uint64_t value = reinterpret_cast<Const64*>(constInstr->byteCode())->value();
    size_t end = instDeps.size();

    for (size_t i = 1; i < end; i++) {
        Instruction* instr = instDeps[i];

        if (instr->opcode() != Const64Opcode || reinterpret_cast<Const64*>(instr->byteCode())->value() != value) {
            return false;
        }
    }

    return true;
}

void JITCompiler::buildParamDependencies(uint32_t requiredStackSize)
{
    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (item->isLabel()) {
            item->asLabel()->m_dependencyCtx = new DependencyGenContext(requiredStackSize);
        }
    }

    bool updateDeps = true;
    std::vector<InstructionListItem*> currentDeps(requiredStackSize);

    // Phase 1: the direct dependencies are computed for instructions
    // and labels (only labels can have multiple dependencies).
    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (item->isLabel()) {
            // Build a dependency list which refers to the last label.
            Label* label = item->asLabel();

            if (updateDeps) {
                label->m_dependencyCtx->update(currentDeps);
            }

            for (uint32_t i = 0; i < requiredStackSize; ++i) {
                currentDeps[i] = label;
            }

            updateDeps = true;
            continue;
        }

        Instruction* instr = item->asInstruction();
        Operand* operand = instr->operands();
        Operand* end = operand + instr->paramCount();

        while (operand < end) {
            InstructionListItem* dependency = currentDeps[operand->offset];

            if (dependency != nullptr) {
                operand->item = dependency;

                if (dependency->isLabel()) {
                    dependency->asLabel()->m_dependencyCtx->stackItem(operand->offset).options |= DependencyGenContext::kOptReferenced;
                }
            }

            operand++;
        }

        if (instr->group() == Instruction::DirectBranch) {
            Label* label = instr->asExtended()->value().targetLabel;
            label->m_dependencyCtx->update(currentDeps);

            if (instr->opcode() == JumpOpcode) {
                updateDeps = false;
            }
            continue;
        }

        if (instr->group() == Instruction::BrTable) {
            Label** label = instr->asBrTable()->targetLabels();
            Label** end = label + instr->asBrTable()->targetLabelCount();
            std::set<Label*> updatedLabels;

            while (label < end) {
                if (updatedLabels.insert(*label).second) {
                    (*label)->m_dependencyCtx->update(currentDeps);
                }
                label++;
            }
            updateDeps = false;
            continue;
        }

        end = operand + instr->resultCount();

        // Only certain instructions are collected
        if (instr->group() != Instruction::Immediate && instr->group() != Instruction::Compare && instr->group() != Instruction::CompareFloat) {
            instr = nullptr;
        }

        while (operand < end) {
            currentDeps[operand->offset] = instr;
            operand++;
        }
    }

    // Phase 2: the indirect instruction
    // references are computed for labels.
    std::vector<DependencyGenContext::StackItem>* lastStack = nullptr;

    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (!item->isLabel()) {
            Instruction* instr = item->asInstruction();
            Operand* param = instr->params();

            for (uint32_t i = instr->paramCount(); i > 0; --i) {
                if (param->item != nullptr && param->item->isLabel()) {
                    ASSERT(param->item->asLabel()->m_dependencyCtx->stack() == lastStack);

                    DependencyGenContext::StackItem& item = (*lastStack)[param->offset];

                    ASSERT(item.options & DependencyGenContext::kOptReferenced);

                    if (!(item.options & DependencyGenContext::kOptDependencyComputed)) {
                        // Exactly one item is present in the list, which is stored in
                        // the instruction, not in the label to reduce memory consumption.
                        if (item.options & DependencyGenContext::kOptHasAnyInstruction) {
                            ASSERT(item.dependencies.begin() == item.dependencies.end());
                            param->item = nullptr;
                        } else {
                            ASSERT(++item.dependencies.begin() == item.dependencies.end());
                            param->item = *item.dependencies.begin();
                        }
                    }
                }
                param++;
            }
            continue;
        }

        Label* currentLabel = item->asLabel();
        lastStack = currentLabel->m_dependencyCtx->stack();

        // Compute the required size first
        ASSERT(requiredStackSize == lastStack->size())
        size_t end = requiredStackSize;

        while (end > 0) {
            if ((*lastStack)[end - 1].options & DependencyGenContext::kOptReferenced) {
                break;
            }
            end--;
        }

        currentLabel->m_dependencies.resize(end);

        for (uint32_t i = 0; i < end; ++i) {
            DependencyGenContext::StackItem& currentItem = (*lastStack)[i];

            ASSERT(!(currentItem.options & DependencyGenContext::kOptDependencyComputed));

            if (!(currentItem.options & DependencyGenContext::kOptReferenced)) {
                continue;
            }

            std::vector<Label*> unprocessedLabels;
            Label::DependencyList instrDependencies;
            std::set<InstructionListItem*>& dependencies = currentItem.dependencies;
            bool hasAnyInstruction = (currentItem.options & DependencyGenContext::kOptHasAnyInstruction) != 0;

            for (auto it : dependencies) {
                if (it->isLabel()) {
                    unprocessedLabels.push_back(it->asLabel());
                } else {
                    instrDependencies.push_back(it->asInstruction());
                }
            }

            while (!unprocessedLabels.empty()) {
                Label* label = unprocessedLabels.back();
                DependencyGenContext::StackItem& item = label->m_dependencyCtx->stackItem(i);

                unprocessedLabels.pop_back();

                if (item.options & DependencyGenContext::kOptDependencyComputed) {
                    for (auto it : label->dependencies(i)) {
                        if (it == nullptr) {
                            hasAnyInstruction = true;
                        } else if (dependencies.insert(it).second) {
                            instrDependencies.push_back(it);
                        }
                    }
                } else {
                    for (auto it : item.dependencies) {
                        if (dependencies.insert(it).second) {
                            if (it->isLabel()) {
                                unprocessedLabels.push_back(it->asLabel());
                            } else {
                                instrDependencies.push_back(it->asInstruction());
                            }
                        }
                    }
                }

                if (item.options & DependencyGenContext::kOptHasAnyInstruction) {
                    hasAnyInstruction = true;
                }
            }

            dependencies.clear();

            if (hasAnyInstruction) {
                currentItem.options |= DependencyGenContext::kOptHasAnyInstruction;
            } else {
                // At least one instruction dependency
                // must be present if the input is valid.
                assert(instrDependencies.size() > 0);

                if (isSameConst(instrDependencies)) {
                    dependencies.insert(instrDependencies[0]);
                    continue;
                }
            }

            size_t size = instrDependencies.size();

            if (hasAnyInstruction) {
                if (size == 0) {
                    // One unknown instruction is present.
                    continue;
                }
                size++;
            } else if (size == 1) {
                dependencies.insert(instrDependencies[0]);
                continue;
            }

            ASSERT(size > 0);

            currentItem.options |= DependencyGenContext::kOptDependencyComputed;

            Label::DependencyList& list = currentLabel->m_dependencies[i];
            list.reserve(size);

            for (auto it : instrDependencies) {
                list.push_back(it);

                if (it->group() == Instruction::Immediate) {
                    it->addInfo(Instruction::kKeepInstruction);
                }
            }

            if (hasAnyInstruction) {
                list.push_back(nullptr);
            }
        }
    }

    // Cleanup
    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (item->isLabel()) {
            delete item->asLabel()->m_dependencyCtx;
        }
    }
}

} // namespace Walrus
