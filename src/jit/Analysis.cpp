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

    struct StackItem {
        StackItem()
            : options(0)
        {
        }

        std::vector<InstructionListItem*> instDeps;
        std::vector<InstructionListItem*> labelDeps;
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

static void insertDependency(std::vector<InstructionListItem*>& list, InstructionListItem* dependency)
{
    for (auto it : list) {
        if (it == dependency) {
            return;
        }
    }

    list.push_back(dependency);
}

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

        if (dependency->isInstruction()) {
            insertDependency(m_stack[i].instDeps, dependency);
        } else {
            insertDependency(m_stack[i].labelDeps, dependency);
        }
    }
}

static bool isSameConst(std::vector<InstructionListItem*>& instDeps)
{
    Instruction* constInstr = instDeps[0]->asInstruction();

    if (constInstr->group() != Instruction::Immediate) {
        return false;
    }

    if (constInstr->opcode() == Const32Opcode) {
        uint32_t value = reinterpret_cast<Const32*>(constInstr->byteCode())->value();
        size_t end = instDeps.size();

        for (size_t i = 1; i < end; i++) {
            Instruction* instr = instDeps[i]->asInstruction();

            if (instr->opcode() != Const32Opcode || reinterpret_cast<Const32*>(instr->byteCode())->value() != value) {
                return false;
            }
        }

        return true;
    }

    uint64_t value = reinterpret_cast<Const64*>(constInstr->byteCode())->value();
    size_t end = instDeps.size();

    for (size_t i = 1; i < end; i++) {
        Instruction* instr = instDeps[i]->asInstruction();

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
            Label* label = item->asLabel();
            label->m_dependencyCtx = new DependencyGenContext(requiredStackSize);
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

                    if (item.options & DependencyGenContext::kOptHasAnyInstruction) {
                        if (item.instDeps.size() == 0) {
                            item.options &= (uint8_t)~DependencyGenContext::kOptHasAnyInstruction;
                        }
                    } else if (item.instDeps.size() == 1) {
                        // A single reference is copied into the instruction.
                        param->item = item.instDeps[0];
                        item.options &= (uint8_t)~DependencyGenContext::kOptHasAnyInstruction;
                    }
                }
                param++;
            }
            continue;
        }

        lastStack = item->asLabel()->m_dependencyCtx->stack();

        for (uint32_t i = 0; i < requiredStackSize; ++i) {
            DependencyGenContext::StackItem& currentItem = (*lastStack)[i];

            if (!(currentItem.options & DependencyGenContext::kOptReferenced)) {
                continue;
            }

            std::vector<Label*> unprocessedLabels;
            std::set<InstructionListItem*> processedDeps;
            std::vector<InstructionListItem*>& instDeps = currentItem.instDeps;
            bool hasAnyInstruction = false;

            for (auto it : instDeps) {
                processedDeps.insert(it);
            }

            for (auto it : currentItem.labelDeps) {
                processedDeps.insert(it);
                unprocessedLabels.push_back(it->asLabel());
            }

            currentItem.labelDeps.clear();

            while (!unprocessedLabels.empty()) {
                Label* label = unprocessedLabels.back();
                DependencyGenContext::StackItem& item = label->m_dependencyCtx->stackItem(i);

                unprocessedLabels.pop_back();

                for (auto it : item.instDeps) {
                    if (processedDeps.insert(it).second) {
                        instDeps.push_back(it);
                    }
                }

                for (auto it : item.labelDeps) {
                    if (processedDeps.insert(it).second) {
                        unprocessedLabels.push_back(it->asLabel());
                    }
                }

                if (item.options & DependencyGenContext::kOptHasAnyInstruction) {
                    hasAnyInstruction = true;
                }
            }

            if (hasAnyInstruction) {
                currentItem.options |= DependencyGenContext::kOptHasAnyInstruction;
            } else if (!(currentItem.options & DependencyGenContext::kOptHasAnyInstruction)) {
                // At least one instruction dependency
                // must be present if the input is valid.
                assert(instDeps.size() > 0);

                if (isSameConst(instDeps)) {
                    instDeps.resize(1);
                    continue;
                }
            }

            for (auto it : instDeps) {
                if (it->group() == Instruction::Immediate) {
                    it->addInfo(Instruction::kKeepInstruction);
                }
            }
        }
    }

    // Phase 3: The indirect references for labels are
    // collected, and all temporary structures are deleted.
    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (!item->isLabel()) {
            continue;
        }

        // Copy the final dependency data into
        // the dependency list of the label.
        Label* label = item->asLabel();
        DependencyGenContext* context = label->m_dependencyCtx;
        std::vector<DependencyGenContext::StackItem>* stack = context->stack();
        size_t end = stack->size();

        while (end > 0) {
            if ((*stack)[end - 1].options & DependencyGenContext::kOptReferenced) {
                break;
            }
            end--;
        }

        // Single item dependencies are
        // moved into the corresponding operand data.
        label->m_dependencies.resize(end);

        for (size_t i = 0; i < end; ++i) {
            DependencyGenContext::StackItem& item = (*stack)[i];

            if (!(item.options & DependencyGenContext::kOptReferenced)) {
                continue;
            }

            assert(item.labelDeps.empty());

            Label::DependencyList& list = label->m_dependencies[i];

            size_t size = item.instDeps.size();

            if (item.options & DependencyGenContext::kOptHasAnyInstruction) {
                size++;
            }

            list.reserve(size);

            if (item.options & DependencyGenContext::kOptHasAnyInstruction) {
                list.push_back(nullptr);
            }

            for (auto it : item.instDeps) {
                list.push_back(it->asInstruction());
            }
        }

        delete context;
    }
}

} // namespace Walrus
