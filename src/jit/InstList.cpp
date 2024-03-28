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
#include "jit/SljitLir.h"
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

uint32_t Instruction::valueTypeToOperandType(Value::Type type)
{
    switch (type) {
    case Value::I32:
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    case Value::FuncRef:
    case Value::ExternRef:
#endif /* SLJIT_32BIT_ARCHITECTURE */
        return Instruction::Int32Operand;
    case Value::I64:
#if (defined SLJIT_64BIT_ARCHITECTURE && SLJIT_64BIT_ARCHITECTURE)
    case Value::FuncRef:
    case Value::ExternRef:
#endif /* SLJIT_32BIT_ARCHITECTURE */
        return Instruction::Int64Operand;
    case Value::V128:
        return Instruction::V128Operand;
    case Value::F32:
        return Instruction::Float32Operand;
    default:
        ASSERT(type == Value::F64);
        return Instruction::Float64Operand;
    }
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

BrTableInstruction::BrTableInstruction(ByteCode* byteCode, size_t targetLabelCount)
    : Instruction(byteCode, Instruction::BrTable, ByteCode::BrTableOpcode, 1, &m_inlineParam)
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

VariableRef VariableList::getMergeHeadSlowCase(VariableRef ref)
{
    ASSERT(variables[ref].info & kIsMerged);

    VariableRef parent = variables[ref].u.parent;

    ASSERT(parent < ref);

    if (!(variables[parent].info & kIsMerged)) {
        return parent;
    }

    // Update parents.
    parent = variables[parent].u.parent;

    while (variables[parent].info & kIsMerged) {
        ASSERT(variables[parent].u.parent < parent);
        parent = variables[parent].u.parent;
    }

    do {
        VariableRef next = variables[ref].u.parent;
        variables[ref].u.parent = parent;
        ref = next;
    } while (variables[ref].info & kIsMerged);

    return parent;
}

Instruction* JITCompiler::append(ByteCode* byteCode, Instruction::Group group, ByteCode::Opcode opcode, uint32_t paramCount, uint32_t resultCount)
{
    Instruction* instr;
    uint32_t operandCount = paramCount + resultCount;

    switch (operandCount) {
    case 0:
        instr = new Instruction(byteCode, group, opcode, 0, nullptr);
        break;
    case 1:
        instr = new SimpleInstruction<1>(byteCode, group, opcode, paramCount);
        break;
    case 2:
        instr = new SimpleInstruction<2>(byteCode, group, opcode, paramCount);
        break;
    case 3:
        instr = new SimpleInstruction<3>(byteCode, group, opcode, paramCount);
        break;
    case 4:
        instr = new SimpleInstruction<4>(byteCode, group, opcode, paramCount);
        break;
    default:
        instr = new ComplexInstruction(byteCode, group, opcode, paramCount, operandCount);
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
        instr = new ExtendedInstruction(byteCode, group, opcode, 0, nullptr);
        break;
    case 1:
        instr = new SimpleExtendedInstruction<1>(byteCode, group, opcode, paramCount);
        break;
    case 2:
        instr = new SimpleExtendedInstruction<2>(byteCode, group, opcode, paramCount);
        break;
    case 3:
        instr = new SimpleExtendedInstruction<3>(byteCode, group, opcode, paramCount);
        break;
    case 4:
        instr = new SimpleExtendedInstruction<4>(byteCode, group, opcode, paramCount);
        break;
    default:
        instr = new ComplexExtendedInstruction(byteCode, group, opcode, paramCount, operandCount);
        break;
    }

    instr->m_resultCount = resultCount > 0 ? 1 : 0;
    instr->m_value.resultCount = resultCount;

    append(instr);
    return instr;
}

Instruction* JITCompiler::appendBranch(ByteCode* byteCode, ByteCode::Opcode opcode, Label* label, uint32_t offset)
{
    ASSERT(opcode == ByteCode::JumpOpcode || opcode == ByteCode::JumpIfTrueOpcode || opcode == ByteCode::JumpIfFalseOpcode);
    ASSERT(label != nullptr);

    ExtendedInstruction* branch;

    if (opcode == ByteCode::JumpOpcode) {
        branch = new ExtendedInstruction(byteCode, Instruction::DirectBranch, ByteCode::JumpOpcode, 0, nullptr);
    } else {
        branch = new SimpleExtendedInstruction<1>(byteCode, Instruction::DirectBranch, opcode, 1);

        Operand* operands = branch->operands();
        operands[0].ref = offset;
    }

    branch->value().targetLabel = label;
    label->m_branches.push_back(branch);
    append(branch);
    return branch;
}

BrTableInstruction* JITCompiler::appendBrTable(ByteCode* byteCode, uint32_t numTargets, uint32_t offset)
{
    BrTableInstruction* branch = new BrTableInstruction(byteCode, numTargets + 1);

    Operand* operands = branch->operands();
    operands[0].ref = offset;

    append(branch);
    return branch;
}

void JITCompiler::dump()
{
    bool enableColors = (verboseLevel() >= 2);
    int counter = 0;

    static const char* byteCodeName[] = {
#define BYTECODE_NAME(name, ...) #name,
        FOR_EACH_BYTECODE(BYTECODE_NAME)
#undef DECLARE_BYTECODE
    };

    const char* defaultText = enableColors ? "\033[0m" : "";
    const char* instrText = enableColors ? "\033[1;35m" : "";
    const char* variableText = enableColors ? "\033[1;32m" : "";
    const char* labelText = enableColors ? "\033[1;36m" : "";
    const char* highlightFlagText = enableColors ? "\033[1;33m" : "";

    for (InstructionListItem* item = first(); item != nullptr; item = item->next()) {
        if (item->isInstruction()) {
            Instruction* instr = item->asInstruction();
            printf("%d: %s%s%s", static_cast<int>(instr->id()), instrText, byteCodeName[instr->opcode()], defaultText);

            if (enableColors) {
                printf(" (%p)", item);
            }

            printf("\n");

            switch (instr->group()) {
            case Instruction::Immediate: {
                if (!(instr->info() & Instruction::kKeepInstruction)) {
                    printf("  %sUnused%s\n", highlightFlagText, defaultText);
                }
                break;
            }
            case Instruction::DirectBranch: {
                printf("  Jump to: %s%d%s\n", labelText, static_cast<int>(instr->asExtended()->value().targetLabel->id()), defaultText);
                break;
            }
            case Instruction::BrTable: {
                size_t targetLabelCount = instr->asBrTable()->targetLabelCount();
                Label** targetLabels = instr->asBrTable()->targetLabels();

                for (size_t i = 0; i < targetLabelCount; i++) {
                    printf("  Jump to: %s%d%s\n", labelText, static_cast<int>((*targetLabels)->id()), defaultText);
                    targetLabels++;
                }
                break;
            }
            default: {
                break;
            }
            }

            if (instr->info() & Instruction::kIsMergeCompare) {
                printf("  %sMergeCompare%s\n", highlightFlagText, defaultText);
            }

            uint32_t paramCount = instr->paramCount();
            uint32_t size = paramCount + instr->resultCount();
            Operand* operand = instr->operands();

            for (uint32_t i = 0; i < size; ++i) {
                if (i < paramCount) {
                    printf("  Param[%d]: ", static_cast<int>(i));
                } else {
                    printf("  Result[%d]: ", static_cast<int>(i - paramCount));
                }

                VariableRef ref = operand->ref;
                VariableList::Variable& variable = m_variableList->variables[ref];

                if (variable.value != VARIABLE_SET_PTR(nullptr)) {
                    const char* type = "Invalid";

                    switch (variable.info & Instruction::TypeMask) {
                    case Instruction::Int32Operand:
                        type = "I32";
                        break;
                    case Instruction::Int64Operand:
                        type = "I64";
                        break;
                    case Instruction::V128Operand:
                        type = "V128";
                        break;
                    case Instruction::Float32Operand:
                        type = "F32";
                        break;
                    case Instruction::Float64Operand:
                        type = "F64";
                        break;
                    }

                    printf("%s%d%s.%s", variableText, static_cast<int>(ref), defaultText, type);

                    if (VARIABLE_TYPE(variable.value) == Operand::Offset) {
                        printf(" (O:%d) [%d-%d]", static_cast<int>(VARIABLE_GET_OFFSET(variable.value)),
                               static_cast<int>(variable.u.rangeStart), static_cast<int>(variable.rangeEnd));
                    } else if (enableColors) {
                        printf(" (I:%p)", VARIABLE_GET_IMM(variable.value));
                    } else {
                        printf(" (I)");
                    }
                } else {
                    printf("(flag)");
                }

                if (variable.info & VariableList::kIsCallback) {
                    printf(" %sCallBack%s", highlightFlagText, defaultText);
                } else if (variable.info & VariableList::kDestroysR0R1) {
                    printf(" %sDestroysR0R1%s", highlightFlagText, defaultText);
                }

                printf("\n");
                operand++;
            }
        } else {
            printf("%s%d%s: Label", labelText, static_cast<int>(item->id()), defaultText);

            if (enableColors) {
                printf(" (%p)", item);
            }

            Label* label = item->asLabel();

            printf("%s%s\n", (label->info() & Label::kHasTryInfo) ? " hasTryInfo" : "",
                   (label->info() & Label::kHasCatchInfo) ? " hasCatchInfo" : "");

            for (auto it : label->branches()) {
                printf("  Jump from: %s%d%s\n", instrText, static_cast<int>(it->id()), defaultText);
            }
        }
    }
}

void JITCompiler::append(InstructionListItem* item)
{
    ASSERT(item->m_next == nullptr);

    if (m_last != nullptr) {
        m_last->m_next = item;
    } else {
        m_first = item;
    }

    m_last = item;
}

} // namespace Walrus
