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

#if defined(WALRUS_ENABLE_JIT)

#include "Walrus.h"
#include "jit/Compiler.h"

namespace Walrus {

// Maximum preload distance, must be power of 2
#define PRELOAD_MAX 8
#define PRELOAD_SHIFT (8 / PRELOAD_MAX)
#define PRELOAD_MASK ((1 << PRELOAD_SHIFT) - 1)

// Two saved registers are reserved for frame and instance.
#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
#define NUMBER_OF_REGISTERS 4
#define NUMBER_OF_SCRATCH_REGISTERS 3
#else
#define NUMBER_OF_REGISTERS (SLJIT_NUMBER_OF_REGISTERS - 2)
#define NUMBER_OF_SCRATCH_REGISTERS SLJIT_NUMBER_OF_SCRATCH_REGISTERS
#endif

#define FROM_INTEGER_REG(r) \
    static_cast<uint32_t>((r) <= NUMBER_OF_SCRATCH_REGISTERS ? (r)-SLJIT_R0 : SLJIT_S2 + NUMBER_OF_SCRATCH_REGISTERS - (r))

#define TO_INTEGER_REG(r) \
    static_cast<uint32_t>((r) < NUMBER_OF_SCRATCH_REGISTERS ? (r) + SLJIT_R0 : SLJIT_S2 + NUMBER_OF_SCRATCH_REGISTERS - (r))

struct VariableStart {
    VariableStart(VariableList::Variable* variable)
        : variable(variable)
    {
    }

    bool operator<(const VariableStart& other) const
    {
        return variable->u.rangeStart < other.variable->u.rangeStart;
    }

    VariableList::Variable* variable;
};

struct VariableEnd {
    VariableEnd(VariableList::Variable* variable)
        : variable(variable)
    {
    }

    bool operator<(const VariableEnd& other) const
    {
        return variable->rangeEnd < other.variable->rangeEnd;
    }

    VariableList::Variable* variable;
};

struct VariableExtra {
    VariableExtra()
        : lastWrite(0)
        , lastRead(0)
        , reg1(VariableList::kUnusedReg)
        , reg2(VariableList::kUnusedReg)
    {
    }

    size_t lastWrite;
    size_t lastRead;
    uint8_t reg1;
    uint8_t reg2;
};

void JITCompiler::preloadVariables()
{
    if (m_variableList == nullptr) {
        return;
    }

    size_t size = m_variableList->variables.size();
    std::vector<VariableStart> integerStarts;
    std::vector<VariableEnd> integerEnds;
    std::vector<VariableExtra> extras(size);
    integerStarts.reserve(size);
    integerEnds.reserve(size);

    VariableList::Variable* current = m_variableList->variables.data();
    VariableList::Variable* end = current + size;

    while (current < end) {
        if (!(current->info & (VariableList::kIsMerged | VariableList::kIsImmediate | Instruction::FloatOperandMarker))
            && current->reg1 != VariableList::kUnusedReg) {
            ASSERT(VARIABLE_TYPE(current->value) == Instruction::Register);
            integerStarts.push_back(current);
            integerEnds.push_back(current);
        }
        current++;
    }

    std::sort(integerStarts.begin(), integerStarts.end());
    std::sort(integerEnds.begin(), integerEnds.end());

    // Two saved registers are reserved for frame and instance.
    size_t integerUsedRegs[NUMBER_OF_REGISTERS];
    uint8_t integerVariableRegs[NUMBER_OF_REGISTERS];
    InstructionListItem* items[PRELOAD_MAX + 1];
    size_t itemCounter = 1;
    uint32_t currentItem = PRELOAD_MASK;
    uint32_t integerEnd = NUMBER_OF_SCRATCH_REGISTERS + m_savedIntegerRegCount;
    VariableStart* integerStartCurrent = integerStarts.data();
    VariableStart* integerStartEnd = integerStartCurrent + integerStarts.size();
    VariableEnd* integerEndCurrent = integerEnds.data();
    VariableEnd* integerEndEnd = integerEndCurrent + integerEnds.size();

    items[PRELOAD_MAX] = nullptr;
    memset(integerUsedRegs, 0, sizeof(integerUsedRegs));
    memset(integerVariableRegs, 0, sizeof(integerVariableRegs));

    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (item->isLabel()) {
            for (uint32_t i = 0; i < integerEnd; i++) {
                integerUsedRegs[i] = itemCounter;
            }
            itemCounter++;
            currentItem = (currentItem + 1) & PRELOAD_MASK;
            items[currentItem] = items[PRELOAD_MAX];
            items[PRELOAD_MAX] = item;
            continue;
        }

        while (integerEndCurrent < integerEndEnd && item->id() >= integerEndCurrent->variable->rangeEnd) {
            uint8_t reg = integerEndCurrent->variable->reg1;
            ASSERT(integerVariableRegs[reg] == 1);
            integerVariableRegs[reg] = 0;
            integerUsedRegs[reg] = itemCounter;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            reg = integerEndCurrent->variable->reg2;
            if (reg != VariableList::kUnusedReg) {
                ASSERT(reg == integerEndCurrent->variable->reg1 || integerVariableRegs[reg] == 1);
                integerVariableRegs[reg] = 0;
                integerUsedRegs[reg] = itemCounter;
            }
#endif /* SLJIT_32BIT_ARCHITECTURE */
            integerEndCurrent++;
        }

        while (integerStartCurrent < integerStartEnd && item->id() >= integerStartCurrent->variable->u.rangeStart) {
            uint8_t reg = integerStartCurrent->variable->reg1;
            ASSERT(integerVariableRegs[reg] == 0);
            integerVariableRegs[reg] = 1;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            reg = integerStartCurrent->variable->reg2;
            if (reg != VariableList::kUnusedReg) {
                ASSERT(reg == integerStartCurrent->variable->reg1 || integerVariableRegs[reg] == 0);
                integerVariableRegs[reg] = 1;
            }
#endif /* SLJIT_32BIT_ARCHITECTURE */
            integerStartCurrent++;
        }

        // Update used register set.
        for (uint32_t i = 0; i < integerEnd; i++) {
            if (integerVariableRegs[i] != 0) {
                integerUsedRegs[i] = itemCounter;
            }
        }

        if (item->info() & Instruction::kIsCallback) {
            for (uint32_t i = 0; i < NUMBER_OF_SCRATCH_REGISTERS; i++) {
                integerUsedRegs[i] = itemCounter;
            }
        } else if (item->info() & Instruction::kDestroysR0R1) {
            integerUsedRegs[0] = itemCounter;
            integerUsedRegs[1] = itemCounter;
        }

        Instruction* instr = item->asInstruction();
        uint32_t paramCount = instr->paramCount();
        const uint8_t* list = instr->getOperandDescriptor();
        if (*list != 0) {
            uint32_t tmpIndex = 0;
            uint32_t resultCount = instr->resultCount();

            for (uint32_t i = 0; i < paramCount; i++) {
                if ((*list & Instruction::TypeMask & Instruction::FloatOperandMarker)
                    && !(*list & Instruction::TmpNotAllowed)) {
                    tmpIndex++;
                }
                list++;
            }

            for (; *list != 0; list++) {
                if (resultCount > 0) {
                    --resultCount;

                    if (!(*list & Instruction::TmpRequired)) {
                        continue;
                    }

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
                    if ((*list & Instruction::TypeMask) == Instruction::Int64Operand) {
                        ASSERT(instr->requiredReg(tmpIndex) > 0);
                        uint8_t reg = instr->requiredReg(tmpIndex);
                        ASSERT(FROM_INTEGER_REG(reg) < integerEnd);
                        integerUsedRegs[FROM_INTEGER_REG(reg)] = itemCounter;
                        tmpIndex++;
                    }
#endif /* SLJIT_32BIT_ARCHITECTURE */
                }

                ASSERT(instr->requiredReg(tmpIndex) > 0);

                if (!(*list & Instruction::FloatOperandMarker)) {
                    uint8_t reg = instr->requiredReg(tmpIndex);
                    ASSERT(FROM_INTEGER_REG(reg) < integerEnd);
                    integerUsedRegs[FROM_INTEGER_REG(reg)] = itemCounter;
                }
                tmpIndex++;
            }

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            // 64 bit shifts / rotates requires special handling.
            if ((instr->info() & (Instruction::kIsShift | Instruction::kIs32Bit)) == Instruction::kIsShift) {
                for (tmpIndex = 2; tmpIndex < 4; tmpIndex++) {
                    if (instr->requiredReg(tmpIndex) != 0) {
                        uint8_t reg = instr->requiredReg(tmpIndex);
                        ASSERT(FROM_INTEGER_REG(reg) < integerEnd);
                        integerUsedRegs[FROM_INTEGER_REG(reg)] = itemCounter;
                    }
                }
            } else if (instr->opcode() == ByteCode::I64MulOpcode) {
                ASSERT(tmpIndex == 0);
                uint8_t reg = instr->requiredReg(0);
                ASSERT(reg > 0 && FROM_INTEGER_REG(reg) < integerEnd);
                integerUsedRegs[FROM_INTEGER_REG(reg)] = itemCounter;
            }
#endif /* SLJIT_32BIT_ARCHITECTURE */

            ASSERT((tmpIndex > 0 || instr->requiredReg(0) == 0 || instr->opcode() == ByteCode::I64MulOpcode)
                   && (tmpIndex > 1 || instr->requiredReg(1) == 0)
                   && (tmpIndex > 2 || instr->requiredReg(2) == 0)
                   && (tmpIndex > 3 || instr->requiredReg(3) == 0));
        }

        Operand* operand = instr->operands();
        Operand* end = operand + paramCount;

        if (instr->group() == Instruction::Move) {
            // The source argument of move instructions are not preloaded.
            ASSERT(paramCount == 1);
            operand++;
        }

        while (operand < end) {
            VariableList::Variable& variable = m_variableList->variables[*operand];
            VariableExtra& extra = extras[*operand];
            ASSERT(!(variable.info & VariableList::kIsMerged));

            if (!(variable.info & (VariableList::kIsImmediate | Instruction::FloatOperandMarker))
                && VARIABLE_TYPE(variable.value) == Instruction::Offset) {
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
                if ((variable.info & Instruction::TypeMask) == Instruction::Int64Operand) {
                    operand++;
                    continue;
                }
#endif /* SLJIT_32BIT_ARCHITECTURE */

                size_t lastRead = extra.lastRead;
                extra.lastRead = itemCounter;

                if (extra.reg1 != VariableList::kUnusedReg && integerUsedRegs[extra.reg1] <= lastRead) {
                    ASSERT(lastRead > 0);
                    *operand++ = Instruction::kAbsoluteOperand | VARIABLE_SET(TO_INTEGER_REG(extra.reg1), Instruction::Register);
                    integerUsedRegs[extra.reg1] = itemCounter;
                    continue;
                }

                extra.reg1 = VariableList::kUnusedReg;

                size_t max = itemCounter;
                uint32_t maxIndex = 0;
                for (uint32_t i = 0; i < integerEnd; i++) {
                    if (integerUsedRegs[i] < max) {
                        max = integerUsedRegs[i];
                        maxIndex = i;
                    }
                }

                if (extra.lastWrite > max) {
                    max = extras[*operand].lastWrite;
                }

                if (itemCounter - max > 1) {
                    max = itemCounter - max - 2;
                    if (max >= PRELOAD_MAX) {
                        max = PRELOAD_MAX - 1;
                    }

                    Operand value = Instruction::kAbsoluteOperand | VARIABLE_SET(TO_INTEGER_REG(maxIndex), Instruction::Register);
                    insertStackInit(items[(currentItem - max) & PRELOAD_MASK], variable, value);
                    *operand = value;
                    extra.reg1 = maxIndex;
                    integerUsedRegs[maxIndex] = itemCounter;
                }
            }

            operand++;
        }

        end = operand + instr->resultCount();
        while (operand < end) {
            VariableExtra& extra = extras[*operand];
            extra.lastWrite = itemCounter;
            extra.lastRead = 0;
            operand++;
        }

        // The register usage of combined instructions are merged.
        if (!(instr->info() & Instruction::kIsMergeCompare)) {
            itemCounter++;
            currentItem = (currentItem + 1) & PRELOAD_MASK;
            items[currentItem] = items[PRELOAD_MAX];
            items[PRELOAD_MAX] = item;
        }
    }
    ASSERT(integerStartCurrent == integerStartEnd);
}

} // namespace Walrus

#endif // WALRUS_ENABLE_JIT
