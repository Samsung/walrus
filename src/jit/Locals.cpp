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

class SetLocalContext {
public:
    static const size_t mask = 8 * sizeof(uint32_t) - 1;
    static const size_t shift = 5;

    SetLocalContext(size_t size)
        : m_initialized(getSize(size))
    {
    }

    void set(size_t i) { m_initialized[i >> shift] |= 1 << (i & mask); }
    bool get(size_t i)
    {
        return (m_initialized[i >> shift] & (1 << (i & mask))) != 0;
    }
    SetLocalContext* clone() { return new SetLocalContext(m_initialized); }
    void updateTarget(SetLocalContext* target);

private:
    SetLocalContext(std::vector<uint32_t>& other)
        : m_initialized(other)
    {
    }

    size_t getSize(size_t size) { return (size + mask) & ~mask; }

    // First initializetion of a local variable.
    std::vector<uint32_t> m_initialized;
};

void SetLocalContext::updateTarget(SetLocalContext* target)
{
    assert(m_initialized.size() == target->m_initialized.size());

    size_t end = m_initialized.size();
    std::vector<uint32_t>& targetInitialized = target->m_initialized;

    for (size_t i = 0; i < end; i++) {
        targetInitialized[i] &= m_initialized[i];
    }
}

struct LocalRange {
    LocalRange(size_t index)
        : start(0)
        , end(0)
        , index(index)
    {
    }

    bool operator<(const LocalRange& other) const
    {
        return (start < other.start);
    }

    // The range includes both start and end instructions.
    // Hence, if start equals to end, the range is one instruction long.
    size_t start;
    size_t end;
    size_t index;
};

void JITCompiler::checkLocals(size_t paramsSize)
{
    size_t localsSize = m_locals.size();
    SetLocalContext* current = new SetLocalContext(localsSize);

    std::vector<LocalRange> localRanges;
    std::vector<bool> localUninitialized(localsSize);

    localRanges.reserve(localsSize);

    for (size_t i = 0; i < localsSize; i++) {
        localRanges.push_back(LocalRange(i));
    }

    // Phase 1: compute the start and end regions of all locals,
    // and compute whether the local is accessible from the entry
    // point of the function

    size_t instrIndex = 0;
    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        instrIndex++;

        if (item->isLabel()) {
            Label* label = item->asLabel();

            label->addInfo(Label::kCheckLocalsReached);

            assert((label->info() & Label::kCheckLocalsHasContext) || current != nullptr);

            if (label->info() & Label::kCheckLocalsHasContext) {
                if (current != nullptr) {
                    current->updateTarget(label->m_setLocalCtx);
                }

                delete current;
                current = label->m_setLocalCtx;
            }

            label->m_instrIndex = instrIndex;
            continue;
        }

        Instruction* instr = item->asInstruction();

        switch (instr->group()) {
        case Instruction::LocalMove: {
            Index localIndex = instr->value().localIndex;
            LocalRange& range = localRanges[localIndex];

            if (range.start == 0) {
                range.start = instrIndex;
            }

            range.end = instrIndex;

            if (instr->opcode() == LocalSetOpcode) {
                current->set(localIndex);
            } else if (!current->get(localIndex)) {
                localUninitialized[localIndex] = true;
            }
            break;
        }
        case Instruction::DirectBranch: {
            Label* label = instr->value().targetLabel;

            if (!(label->info() & Label::kCheckLocalsReached)) {
                if (label->info() & Label::kCheckLocalsHasContext) {
                    current->updateTarget(label->m_setLocalCtx);
                } else {
                    label->m_setLocalCtx = current->clone();
                    label->addInfo(Label::kCheckLocalsHasContext);
                }
            }

            if (instr->opcode() == BrOpcode) {
                delete current;
                current = nullptr;
            }
            break;
        }
        case Instruction::BrTable: {
            Label** label = instr->asBrTable()->targetLabels();
            Label** end = label + instr->value().targetLabelCount;
            std::set<Label*> updatedLabels;

            while (label < end) {
                if (updatedLabels.insert(*label).second) {
                    if (!((*label)->info() & Label::kCheckLocalsReached)) {
                        if ((*label)->info() & Label::kCheckLocalsHasContext) {
                            current->updateTarget((*label)->m_setLocalCtx);
                        } else {
                            (*label)->m_setLocalCtx = current->clone();
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

    for (size_t i = 0; i < paramsSize; i++) {
        localRanges[i].start = 0;
    }

    for (size_t i = paramsSize; i < localsSize; i++) {
        if (localUninitialized[i]) {
            localRanges[i].start = 0;
        }
    }

    // Phase 2: extend the range ends with intersecting backward jumps

    instrIndex = 0;
    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        instrIndex++;

        switch (item->group()) {
        case InstructionListItem::CodeLabel: {
            item->clearInfo(Label::kCheckLocalsReached | Label::kCheckLocalsHasContext);
            break;
        }
        case Instruction::DirectBranch: {
            Instruction* instr = item->asInstruction();
            size_t labelInstrIndex = instr->value().targetLabel->m_instrIndex;

            if (labelInstrIndex <= instrIndex) {
                for (size_t i = 0; i < localsSize; i++) {
                    LocalRange& range = localRanges[i];

                    if (range.end < instrIndex && labelInstrIndex <= range.end && range.start <= labelInstrIndex) {
                        range.end = instrIndex;
                    }
                }
            }
            break;
        }
        case Instruction::BrTable: {
            Instruction* instr = item->asInstruction();
            Label** label = instr->asBrTable()->targetLabels();
            Label** end = label + instr->value().targetLabelCount;
            std::set<Label*> updatedLabels;

            while (label < end) {
                if (updatedLabels.insert(*label).second) {
                    size_t labelInstrIndex = (*label)->m_instrIndex;

                    if (labelInstrIndex <= instrIndex) {
                        for (size_t i = 0; i < localsSize; i++) {
                            LocalRange& range = localRanges[i];

                            if (range.end < instrIndex && labelInstrIndex <= range.end && range.start <= labelInstrIndex) {
                                range.end = instrIndex;
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

    std::sort(localRanges.begin() + paramsSize, localRanges.end());

    // Phase 3: assign new local indicies to the existing locals

    std::vector<ValueInfo> newLocals;
    std::vector<size_t> newLocalsEnd;
    std::vector<size_t> remap;

    remap.resize(localsSize);

    for (size_t i = 0; i < localsSize; i++) {
        LocalRange& range = localRanges[i];

        if (range.end == 0 && range.index >= paramsSize) {
            continue;
        }

        ValueInfo type = m_locals[range.index];
        size_t end = newLocals.size();
        size_t newId = 0;

        while (newId < end) {
            if (newLocalsEnd[newId] < range.start && type == newLocals[newId]) {
                newLocalsEnd[newId] = range.end;
                break;
            }
            newId++;
        }

        remap[range.index] = newId;

        if (newId < end) {
            continue;
        }

        newLocals.push_back(type);
        newLocalsEnd.push_back(range.end);
    }

    // Phase 4: remap values

    if (verboseLevel() >= 1) {
        for (size_t i = 0; i < localsSize; i++) {
            LocalRange& range = localRanges[i];
            printf("Local %d: mapped to: %d ", static_cast<int>(range.index), static_cast<int>(remap[range.index]));

            if (range.end == 0) {
                printf("unused\n");
            } else if (range.start == 0) {
                printf("range [BEGIN - %d]\n", static_cast<int>(range.end - 1));
            } else {
                printf("range [%d - %d]\n", static_cast<int>(range.start - 1), static_cast<int>(range.end - 1));
            }
        }

        printf("Number of locals reduced from %d to %d\n", static_cast<int>(localsSize), static_cast<int>(newLocals.size()));
    }

    for (InstructionListItem* item = m_last; item != nullptr; item = item->prev()) {
        if (item->group() == Instruction::LocalMove) {
            Instruction* instr = item->asInstruction();
            instr->value().localIndex = remap[instr->value().localIndex];
        }
    }

    m_locals = newLocals;
}

} // namespace Walrus
