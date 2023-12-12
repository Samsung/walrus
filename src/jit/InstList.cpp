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
#include "runtime/ObjectType.h"

#include <map>

namespace Walrus {

uint32_t Instruction::resultCount()
{
    if (group() != Instruction::Call) {
        return internalResultCount();
    }

    return reinterpret_cast<ExtendedInstruction*>(this)->value().resultCount;
}

ComplexInstruction::~ComplexInstruction()
{
    delete[] params();
}

ComplexExtendedInstruction::~ComplexExtendedInstruction()
{
    delete[] params();
}

BrTableInstruction::~BrTableInstruction()
{
    delete[] m_targetLabels;
}

BrTableInstruction::BrTableInstruction(ByteCode* byteCode, size_t targetLabelCount, InstructionListItem* prev)
    : Instruction(byteCode, Instruction::BrTable, ByteCode::BrTableOpcode, 1, &m_inlineParam, prev)
    , m_targetLabelCount(targetLabelCount)
{
    m_targetLabels = new Label*[targetLabelCount];
}

void Label::append(Instruction* instr)
{
    for (auto it : m_branches) {
        if (it == instr) {
            return;
        }
    }

    m_branches.push_back(instr);
}

void Label::merge(Label* other)
{
    ASSERT(this != other);

    for (auto it : other->m_branches) {
        if (it->group() != Instruction::BrTable) {
            ASSERT(it->group() == Instruction::DirectBranch);
            it->asExtended()->value().targetLabel = this;
            m_branches.push_back(it);
            continue;
        }

        BrTableInstruction* instr = it->asBrTable();

        Label** label = instr->targetLabels();
        Label** end = label + instr->asBrTable()->targetLabelCount();
        bool found = false;

        do {
            if (*label == other) {
                *label = this;
            } else if (*label == this) {
                found = true;
            }
        } while (++label < end);

        if (!found) {
            m_branches.push_back(it);
        }
    }
}

Instruction* JITCompiler::append(ByteCode* byteCode, Instruction::Group group, ByteCode::Opcode opcode, uint32_t paramCount, uint32_t resultCount)
{
    Instruction* instr;
    uint32_t operandCount = paramCount + resultCount;

    switch (operandCount) {
    case 0:
        instr = new Instruction(byteCode, group, opcode, 0, nullptr, m_last);
        break;
    case 1:
        instr = new SimpleInstruction<1>(byteCode, group, opcode, paramCount, m_last);
        break;
    case 2:
        instr = new SimpleInstruction<2>(byteCode, group, opcode, paramCount, m_last);
        break;
    case 3:
        instr = new SimpleInstruction<3>(byteCode, group, opcode, paramCount, m_last);
        break;
    case 4:
        instr = new SimpleInstruction<4>(byteCode, group, opcode, paramCount, m_last);
        break;
    default:
        instr = new ComplexInstruction(byteCode, group, opcode, paramCount, operandCount, m_last);
        break;
    }

    ASSERT(resultCount <= 1);
    instr->m_resultCount = static_cast<uint8_t>(resultCount);

    append(instr);
    return instr;
}

ExtendedInstruction* JITCompiler::appendExtended(ByteCode* byteCode, Instruction::Group group, ByteCode::Opcode opcode, uint32_t paramCount, uint32_t resultCount)
{
    ExtendedInstruction* instr;
    uint32_t operandCount = paramCount + resultCount;

    ASSERT(group == Instruction::Call);

    switch (operandCount) {
    case 0:
        instr = new ExtendedInstruction(byteCode, group, opcode, 0, nullptr, m_last);
        break;
    case 1:
        instr = new SimpleExtendedInstruction<1>(byteCode, group, opcode, paramCount, m_last);
        break;
    case 2:
        instr = new SimpleExtendedInstruction<2>(byteCode, group, opcode, paramCount, m_last);
        break;
    case 3:
        instr = new SimpleExtendedInstruction<3>(byteCode, group, opcode, paramCount, m_last);
        break;
    case 4:
        instr = new SimpleExtendedInstruction<4>(byteCode, group, opcode, paramCount, m_last);
        break;
    default:
        instr = new ComplexExtendedInstruction(byteCode, group, opcode, paramCount, operandCount, m_last);
        break;
    }

    instr->m_resultCount = resultCount > 0 ? 1 : 0;
    instr->m_value.resultCount = resultCount;

    append(instr);
    return instr;
}

void JITCompiler::appendBranch(ByteCode* byteCode, ByteCode::Opcode opcode, Label* label, uint32_t offset)
{
    ASSERT(opcode == ByteCode::JumpOpcode || opcode == ByteCode::JumpIfTrueOpcode || opcode == ByteCode::JumpIfFalseOpcode);
    ASSERT(label != nullptr);

    ExtendedInstruction* branch;

    if (opcode == ByteCode::JumpOpcode) {
        branch = new ExtendedInstruction(byteCode, Instruction::DirectBranch, ByteCode::JumpOpcode, 0, nullptr, m_last);
    } else {
        branch = new SimpleExtendedInstruction<1>(byteCode, Instruction::DirectBranch, opcode, 1, m_last);

        Operand* operands = branch->operands();
        operands[0].item = nullptr;
        operands[0].offset = offset;
    }

    branch->value().targetLabel = label;
    label->m_branches.push_back(branch);
    append(branch);
}

BrTableInstruction* JITCompiler::appendBrTable(ByteCode* byteCode, uint32_t numTargets, uint32_t offset)
{
    BrTableInstruction* branch = new BrTableInstruction(byteCode, numTargets + 1, m_last);

    Operand* operands = branch->operands();
    operands[0].item = nullptr;
    operands[0].offset = offset;

    append(branch);
    return branch;
}

void JITCompiler::dump()
{
    std::map<InstructionListItem*, int> instrIndex;
    bool enableColors = (verboseLevel() >= 2);
    int counter = 0;

    static const char* byteCodeName[] = {
#define BYTECODE_NAME(name, ...) #name,
        FOR_EACH_BYTECODE(BYTECODE_NAME)
#undef DECLARE_BYTECODE
    };

    for (InstructionListItem* item = first(); item != nullptr; item = item->next()) {
        instrIndex[item] = counter++;
    }

    const char* defaultText = enableColors ? "\033[0m" : "";
    const char* instrText = enableColors ? "\033[1;35m" : "";
    const char* labelText = enableColors ? "\033[1;36m" : "";
    const char* unusedText = enableColors ? "\033[1;33m" : "";

    for (InstructionListItem* item = first(); item != nullptr; item = item->next()) {
        if (item->isInstruction()) {
            Instruction* instr = item->asInstruction();
            printf("%s%d%s: ", instrText, instrIndex[item], defaultText);

            if (enableColors) {
                printf("(%p) ", item);
            }

            printf("Opcode: %s\n", byteCodeName[instr->opcode()]);

            switch (instr->group()) {
            case Instruction::Immediate: {
                if (!(instr->info() & Instruction::kKeepInstruction)) {
                    printf("  %sUnused%s\n", unusedText, defaultText);
                }
                break;
            }
            case Instruction::DirectBranch: {
                printf("  Jump to: %s%d%s\n", labelText, instrIndex[instr->asExtended()->value().targetLabel], defaultText);
                break;
            }
            case Instruction::BrTable: {
                size_t targetLabelCount = instr->asBrTable()->targetLabelCount();
                Label** targetLabels = instr->asBrTable()->targetLabels();

                for (size_t i = 0; i < targetLabelCount; i++) {
                    printf("  Jump to: %s%d%s\n", labelText, instrIndex[*targetLabels], defaultText);
                    targetLabels++;
                }
                break;
            }
            default: {
                break;
            }
            }

            uint32_t paramCount = instr->paramCount();
            uint32_t size = paramCount + instr->resultCount();
            Operand* param = instr->params();

            for (uint32_t i = 0; i < size; ++i) {
                if (i < paramCount) {
                    printf("  Param[%d]: %d", static_cast<int>(i), static_cast<int>(param->offset));

                    if (param->item != nullptr) {
                        printf(" ref:%s%d%s\n", param->item->isLabel() ? labelText : instrText, instrIndex[param->item], defaultText);
                    } else {
                        printf(" ref:%sAny%s\n", instrText, defaultText);
                    }
                } else {
                    printf("  Result[%d]: %d\n", static_cast<int>(i - paramCount), static_cast<int>(param->offset));
                }
                param++;
            }
        } else {
            printf("%s%d%s: ", labelText, instrIndex[item], defaultText);

            if (enableColors) {
                printf("(%p) ", item);
            }

            Label* label = item->asLabel();

            printf("Label:%s%s\n", (label->info() & Label::kHasTryInfo) ? " hasTryInfo" : "",
                   (label->info() & Label::kHasCatchInfo) ? " hasCatchInfo" : "");

            for (auto it : label->branches()) {
                printf("  Jump from: %s%d%s\n", instrText, instrIndex[it], defaultText);
            }

            size_t size = label->dependencyCount();

            for (size_t i = 0; i < size; ++i) {
                if (label->dependencies(i).size() == 0) {
                    continue;
                }

                printf("  Param[%d]\n", static_cast<int>(i));

                for (auto dependencyIt : label->dependencies(i)) {
                    if (dependencyIt == nullptr) {
                        printf("    %sAny%s instruction\n", instrText, defaultText);
                    } else {
                        printf("    %s%d%s instruction\n", instrText, instrIndex[dependencyIt], defaultText);
                    }
                }
            }
        }
    }
}

void JITCompiler::append(InstructionListItem* item)
{
    ASSERT(item->m_prev == m_last && item->m_next == nullptr);

    if (m_last != nullptr) {
        m_last->m_next = item;
    } else {
        m_first = item;
    }

    m_last = item;
}

InstructionListItem* JITCompiler::remove(InstructionListItem* item)
{
    InstructionListItem* prev = item->m_prev;
    InstructionListItem* next = item->m_next;

    if (prev == nullptr) {
        ASSERT(m_first == item);
        m_first = next;
    } else {
        ASSERT(m_first != item);
        prev->m_next = next;
    }

    if (next == nullptr) {
        ASSERT(m_last == item);
        m_last = prev;
    } else {
        ASSERT(m_last != item);
        next->m_prev = prev;
    }

    delete item;
    return next;
}

void JITCompiler::replace(InstructionListItem* item, InstructionListItem* newItem)
{
    ASSERT(item != newItem);

    InstructionListItem* prev = item->m_prev;
    InstructionListItem* next = item->m_next;

    newItem->m_prev = prev;
    newItem->m_next = next;

    if (prev == nullptr) {
        ASSERT(m_first == item);
        m_first = newItem;
    } else {
        ASSERT(m_first != item);
        prev->m_next = newItem;
    }

    if (next == nullptr) {
        ASSERT(m_last == item);
        m_last = newItem;
    } else {
        ASSERT(m_last != item);
        next->m_prev = newItem;
    }

    delete item;
}

} // namespace Walrus
