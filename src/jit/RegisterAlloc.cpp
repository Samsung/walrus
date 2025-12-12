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
#include <set>

#if defined(NDEBUG)
#define NOP_CHECK false
#else /* !NDEBUG */
#define NOP_CHECK instr->opcode() == ByteCode::NopOpcode
#endif /* NDEBUG */

#if (defined SLJIT_SEPARATE_VECTOR_REGISTERS && SLJIT_SEPARATE_VECTOR_REGISTERS)
#define VECTOR_SELECT(COND, VECTOR, FLOAT) \
    if (COND) {                            \
        VECTOR;                            \
    } else {                               \
        FLOAT;                             \
    }
#else /* !SLJIT_SEPARATE_VECTOR_REGISTERS */
#define VECTOR_SELECT(COND, VECTOR, FLOAT) FLOAT;
#endif /* SLJIT_SEPARATE_VECTOR_REGISTERS */

#if (defined SLJIT_SEPARATE_VECTOR_REGISTERS && SLJIT_SEPARATE_VECTOR_REGISTERS)
#if (defined SLJIT_CONFIG_RISCV && SLJIT_CONFIG_RISCV)
#define FIRST_VECTOR_REG SLJIT_VR1
#else /* !SLJIT_CONFIG_RISCV */
#define FIRST_VECTOR_REG SLJIT_VR0
#endif /* SLJIT_CONFIG_RISCV */
#endif /* SLJIT_SEPARATE_VECTOR_REGISTERS */

namespace Walrus {

class RegisterSet {
public:
    RegisterSet(uint32_t numberOfScratchRegs, uint32_t numberOfSavedRegs, bool isInteger);

    void reserve(uint8_t reg) { m_registers[reg].rangeEnd = kReservedReg; }

    void updateVariable(uint8_t reg, VariableList::Variable* variable)
    {
        ASSERT(m_registers[reg].rangeEnd != kUnassignedReg && m_registers[reg].variable != nullptr);
        m_registers[reg].variable = variable;
    }

    uint8_t getSavedRegCount() { return static_cast<uint8_t>(m_usedSavedRegisters - m_savedStartIndex); }

    uint8_t toCPUReg(uint8_t reg, uint8_t scratchBase, uint8_t savedBase);
    bool check(uint8_t reg, uint16_t constraints);
    void freeUnusedRegisters(size_t id);
    uint8_t allocateRegister(VariableList::Variable* variable);
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    uint8_t allocateRegisterPair(VariableList::Variable* variable, uint8_t* otherReg);

    void setDestroysR0R1()
    {
        m_regStatus |= kDestroysR0R1;
    }

    void clearDestroysR0R1()
    {
        m_regStatus = static_cast<uint8_t>(m_regStatus & ~kDestroysR0R1);
    }
#endif /* SLJIT_32BIT_ARCHITECTURE */

#if (defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
    uint8_t allocateQuadRegister(VariableList::Variable* variable);
#endif /* SLJIT_CONFIG_ARM_32 */

private:
    static const uint8_t kIsInteger = 1 << 0;
    static const uint8_t kDestroysR0R1 = 1 << 1;

    static const size_t kReservedReg = 0;
    static const size_t kUnassignedReg = ~(size_t)0;

    struct RegisterInfo {
        RegisterInfo()
            : rangeEnd(kUnassignedReg)
            , variable(nullptr)
        {
        }

        size_t rangeEnd;
        VariableList::Variable* variable;
    };

    // Free registers.
    uint8_t m_regStatus;
    uint8_t m_savedStartIndex;
    uint8_t m_usedSavedRegisters;

    // Allocated registers.
    std::vector<RegisterInfo> m_registers;
};

class RegisterFile {
public:
    RegisterFile(uint32_t numberOfIntegerScratchRegs, uint32_t numberOfIntegerSavedRegs)
        : m_integerSet(numberOfIntegerScratchRegs, numberOfIntegerSavedRegs, true)
        , m_floatSet(SLJIT_NUMBER_OF_SCRATCH_FLOAT_REGISTERS, SLJIT_NUMBER_OF_SAVED_FLOAT_REGISTERS, false)
#if (defined SLJIT_SEPARATE_VECTOR_REGISTERS && SLJIT_SEPARATE_VECTOR_REGISTERS)
#if (defined SLJIT_CONFIG_RISCV && SLJIT_CONFIG_RISCV)
        , m_vectorSet(SLJIT_NUMBER_OF_SCRATCH_VECTOR_REGISTERS - 1, SLJIT_NUMBER_OF_SAVED_VECTOR_REGISTERS, false)
#else /* !SLJIT_CONFIG_RISCV */
        , m_vectorSet(SLJIT_NUMBER_OF_SCRATCH_VECTOR_REGISTERS, SLJIT_NUMBER_OF_SAVED_VECTOR_REGISTERS, false)
#endif /* SLJIT_CONFIG_RISCV */
#endif /* SLJIT_SEPARATE_VECTOR_REGISTERS */
    {
    }

    RegisterSet& integerSet()
    {
        return m_integerSet;
    }

    RegisterSet& floatSet()
    {
        return m_floatSet;
    }

#if (defined SLJIT_SEPARATE_VECTOR_REGISTERS && SLJIT_SEPARATE_VECTOR_REGISTERS)
    RegisterSet& vectorSet()
    {
        return m_vectorSet;
    }

#endif /* SLJIT_SEPARATE_VECTOR_REGISTERS */
    uint8_t toCPUIntegerReg(uint8_t reg)
    {
        return m_integerSet.toCPUReg(reg, SLJIT_R0, SLJIT_S2);
    }

    uint8_t toCPUFloatReg(uint8_t reg)
    {
        return m_floatSet.toCPUReg(reg, SLJIT_FR0, SLJIT_FS0);
    }

#if (defined SLJIT_SEPARATE_VECTOR_REGISTERS && SLJIT_SEPARATE_VECTOR_REGISTERS)
    uint8_t toCPUVectorReg(uint8_t reg)
    {
        return m_vectorSet.toCPUReg(reg, FIRST_VECTOR_REG, SLJIT_VS0);
    }
#endif /* SLJIT_SEPARATE_VECTOR_REGISTERS */

    void integerReserve(uint8_t reg)
    {
        m_integerSet.reserve(reg);
    }

    void floatReserve(uint8_t reg)
    {
        m_floatSet.reserve(reg);
    }

#if (defined SLJIT_SEPARATE_VECTOR_REGISTERS && SLJIT_SEPARATE_VECTOR_REGISTERS)
    void vectorReserve(uint8_t reg)
    {
        m_vectorSet.reserve(reg);
    }
#endif /* SLJIT_SEPARATE_VECTOR_REGISTERS */

    void allocateVariable(VariableList::Variable* variable)
    {
        uint8_t type = variable->info & Instruction::TypeMask;
        ASSERT(type > 0);

        if (type & Instruction::FloatOperandMarker) {
#if (defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
            if (type == Instruction::V128Operand) {
                m_floatSet.allocateQuadRegister(variable);
                return;
            }
#endif /* SLJIT_CONFIG_ARM_32 */
#if (defined SLJIT_SEPARATE_VECTOR_REGISTERS && SLJIT_SEPARATE_VECTOR_REGISTERS)
            if (type == Instruction::V128Operand) {
                m_vectorSet.allocateRegister(variable);
                return;
            }
#endif
            m_floatSet.allocateRegister(variable);
            return;
        }

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        if (type == Instruction::Int64Operand) {
            m_integerSet.allocateRegisterPair(variable, nullptr);
            return;
        }
#endif /* SLJIT_32BIT_ARCHITECTURE */

        m_integerSet.allocateRegister(variable);
    }

    void freeUnusedRegisters(size_t id)
    {
        m_integerSet.freeUnusedRegisters(id);
        m_floatSet.freeUnusedRegisters(id);
#if (defined SLJIT_SEPARATE_VECTOR_REGISTERS && SLJIT_SEPARATE_VECTOR_REGISTERS)
        m_vectorSet.freeUnusedRegisters(id);
#endif /* SLJIT_SEPARATE_VECTOR_REGISTERS */
    }

    bool reuseResult(uint8_t type, VariableList::Variable** reusableRegs, VariableList::Variable* resultVariable);

private:
    RegisterSet m_integerSet;
    RegisterSet m_floatSet;
#if (defined SLJIT_SEPARATE_VECTOR_REGISTERS && SLJIT_SEPARATE_VECTOR_REGISTERS)
    RegisterSet m_vectorSet;
#endif /* SLJIT_SEPARATE_VECTOR_REGISTERS */
};

RegisterSet::RegisterSet(uint32_t numberOfScratchRegs, uint32_t numberOfSavedRegs, bool isInteger)
    : m_regStatus(isInteger ? kIsInteger : 0)
    , m_savedStartIndex(numberOfScratchRegs)
    , m_usedSavedRegisters(numberOfScratchRegs)
{
    m_registers.resize(numberOfScratchRegs + numberOfSavedRegs);
}

uint8_t RegisterSet::toCPUReg(uint8_t reg, uint8_t scratchBase, uint8_t savedBase)
{
    if (reg < m_savedStartIndex) {
        return scratchBase + reg;
    }

    return savedBase - (reg - m_savedStartIndex);
}

bool RegisterSet::check(uint8_t reg, uint16_t constraints)
{
    if (constraints & VariableList::kIsCallback) {
        return reg >= m_savedStartIndex;
    }

    if ((m_regStatus & kIsInteger) && (constraints & VariableList::kDestroysR0R1)) {
        return reg >= 2;
    }

    return true;
}

void RegisterSet::freeUnusedRegisters(size_t id)
{
    size_t size = m_registers.size();

    for (size_t i = 0; i < size; i++) {
        RegisterInfo& info = m_registers[i];

        if (info.rangeEnd == kReservedReg) {
            if (info.variable == nullptr) {
                info.rangeEnd = kUnassignedReg;
                continue;
            }

            info.rangeEnd = info.variable->rangeEnd;
        }

        if (info.rangeEnd < id) {
            info.rangeEnd = kUnassignedReg;
            info.variable = nullptr;
        }
    }
}

uint8_t RegisterSet::allocateRegister(VariableList::Variable* variable)
{
    size_t maxRangeEnd = 0;
    size_t maxRangeIndex = 0;
    uint16_t constraints = variable != nullptr ? variable->info : 0;
    size_t size = m_registers.size();
    size_t i = 0;

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    if (m_regStatus & kDestroysR0R1) {
        constraints = VariableList::kDestroysR0R1;
    }
#endif /* SLJIT_32BIT_ARCHITECTURE */

    if (constraints & VariableList::kIsCallback) {
        i = m_savedStartIndex;
    } else if ((constraints & VariableList::kDestroysR0R1) && (m_regStatus & kIsInteger)) {
        i = 2;
    }

    while (i < size) {
        if (m_registers[i].rangeEnd == kUnassignedReg) {
            break;
        }

        if (m_registers[i].rangeEnd > maxRangeEnd) {
            maxRangeEnd = m_registers[i].rangeEnd;
            maxRangeIndex = i;
        }

        i++;
    }

    if (i == size) {
        ASSERT(maxRangeEnd != 0 || variable != nullptr);

        if (variable != nullptr && variable->rangeEnd >= maxRangeEnd) {
            return VariableList::kUnusedReg;
        }

        // Move variable into memory.
        i = maxRangeIndex;
        ASSERT(m_registers[i].variable != nullptr);

        VariableList::Variable* prevVariable = m_registers[i].variable;

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        if (prevVariable->reg2 != prevVariable->reg1) {
            size_t other = (i == prevVariable->reg1) ? prevVariable->reg2 : prevVariable->reg1;

            m_registers[other].rangeEnd = kUnassignedReg;
            m_registers[other].variable = nullptr;
        }
        prevVariable->reg2 = VariableList::kUnusedReg;
#endif /* SLJIT_32BIT_ARCHITECTURE */
        prevVariable->reg1 = VariableList::kUnusedReg;
    }

    // Allocated registers are also reserved for the current byte code.
    m_registers[i].rangeEnd = kReservedReg;
    m_registers[i].variable = variable;

    if (variable != nullptr) {
        variable->reg1 = i;
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        variable->reg2 = i;
#endif /* SLJIT_32BIT_ARCHITECTURE */
    }

    if (i >= m_usedSavedRegisters) {
        m_usedSavedRegisters = i + 1;
    }

    return i;
}

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
uint8_t RegisterSet::allocateRegisterPair(VariableList::Variable* variable, uint8_t* otherReg)
{
    size_t maxRangeEndSingle1 = 0;
    size_t maxRangeIndexSingle1 = 0;
    size_t maxRangeEndSingle2 = 0;
    size_t maxRangeIndexSingle2 = 0;
    size_t maxRangeEndPair = 0;
    size_t maxRangeIndexPair = 0;
    size_t freeReg = VariableList::kUnusedReg;
    uint16_t constraints = variable != nullptr ? variable->info : 0;
    size_t size = m_registers.size();
    size_t minIndex = 0;

    if (constraints & VariableList::kIsCallback) {
        minIndex = m_savedStartIndex;
    } else if ((constraints & VariableList::kDestroysR0R1) && (m_regStatus & kIsInteger)) {
        minIndex = 2;
    }

    size_t i = minIndex;

    while (i < size) {
        if (m_registers[i].rangeEnd == kUnassignedReg) {
            if (freeReg != VariableList::kUnusedReg) {
                break;
            }

            freeReg = i;
        } else if (m_registers[i].variable != nullptr) {
            VariableList::Variable* targetVariable = m_registers[i].variable;

            if (targetVariable->reg1 != targetVariable->reg2) {
                if (targetVariable->reg1 >= minIndex && targetVariable->reg2 >= minIndex
                    && targetVariable->rangeEnd > maxRangeEndPair) {
                    maxRangeEndPair = targetVariable->rangeEnd;
                    maxRangeIndexPair = i;
                }
            } else if (targetVariable->rangeEnd > maxRangeEndSingle1) {
                maxRangeEndSingle2 = maxRangeEndSingle1;
                maxRangeIndexSingle2 = maxRangeIndexSingle1;
                maxRangeEndSingle1 = targetVariable->rangeEnd;
                maxRangeIndexSingle1 = i;
            } else if (targetVariable->rangeEnd > maxRangeEndSingle2) {
                maxRangeEndSingle2 = targetVariable->rangeEnd;
                maxRangeIndexSingle2 = i;
            }
        }

        i++;
    }

    ASSERT(maxRangeEndSingle2 <= maxRangeEndSingle1);

    if (i == size) {
        if (maxRangeEndPair == 0
            && (maxRangeEndSingle1 == 0 || (maxRangeEndSingle2 == 0 && freeReg == VariableList::kUnusedReg))) {
            ASSERT(variable != nullptr);
            return VariableList::kUnusedReg;
        }

        size_t maxRangeEnd;

        if (freeReg != VariableList::kUnusedReg && maxRangeEndSingle1 >= maxRangeEndPair) {
            i = maxRangeIndexSingle1;
            maxRangeEnd = maxRangeEndSingle1;
        } else if (freeReg == VariableList::kUnusedReg && maxRangeEndPair < maxRangeEndSingle1 && maxRangeEndPair < maxRangeEndSingle2) {
            i = maxRangeIndexSingle1;
            maxRangeEnd = maxRangeEndSingle2;
            freeReg = maxRangeIndexSingle2;

            VariableList::Variable* prevVariable = m_registers[freeReg].variable;
            prevVariable->reg1 = VariableList::kUnusedReg;
            prevVariable->reg2 = VariableList::kUnusedReg;
        } else {
            i = maxRangeIndexPair;
            maxRangeEnd = maxRangeEndPair;

            VariableList::Variable* targetVariable = m_registers[i].variable;
            freeReg = (targetVariable->reg1 != i) ? targetVariable->reg1 : targetVariable->reg2;
        }

        if (variable != nullptr && variable->rangeEnd >= maxRangeEnd) {
            return VariableList::kUnusedReg;
        }

        // Move variable into memory.
        VariableList::Variable* prevVariable = m_registers[i].variable;
        ASSERT(prevVariable != nullptr);

        if (prevVariable->reg2 != prevVariable->reg1) {
            size_t other = (i == prevVariable->reg1) ? prevVariable->reg2 : prevVariable->reg1;

            ASSERT(other != VariableList::kUnusedReg);
            m_registers[other].rangeEnd = 0;
            m_registers[other].variable = nullptr;
        }
        prevVariable->reg2 = VariableList::kUnusedReg;
        prevVariable->reg1 = VariableList::kUnusedReg;
    }

    ASSERT(i < size && freeReg < size);

    // Allocated registers are also reserved for the current byte code.
    m_registers[i].rangeEnd = kReservedReg;
    m_registers[i].variable = variable;
    m_registers[freeReg].rangeEnd = kReservedReg;
    m_registers[freeReg].variable = variable;

    if (variable != nullptr) {
        variable->reg1 = freeReg;
        variable->reg2 = i;
    }

    if (i >= m_usedSavedRegisters) {
        m_usedSavedRegisters = i + 1;
    }

    if (freeReg >= m_usedSavedRegisters) {
        m_usedSavedRegisters = freeReg + 1;
    }

    if (otherReg != nullptr) {
        *otherReg = i;
    }

    return freeReg;
}
#endif /* SLJIT_32BIT_ARCHITECTURE */

#if (defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
uint8_t RegisterSet::allocateQuadRegister(VariableList::Variable* variable)
{
    size_t maxRangeEnd = 0;
    size_t maxRangeIndex = 0;
    uint16_t constraints = variable != nullptr ? variable->info : 0;
    size_t size = m_registers.size();
    size_t i = 0;

    ASSERT(!(m_regStatus & kIsInteger) && (size & 0x1) == 0);

    if (constraints & VariableList::kIsCallback) {
        i = m_savedStartIndex;
    }

    while (i < size) {
        if (m_registers[i].rangeEnd == kUnassignedReg) {
            if (m_registers[i + 1].rangeEnd == kUnassignedReg) {
                break;
            }

            if (m_registers[i + 1].rangeEnd > maxRangeEnd) {
                maxRangeEnd = m_registers[i + 1].rangeEnd;
                maxRangeIndex = i;
            }
        } else if (m_registers[i + 1].rangeEnd == kUnassignedReg) {
            if (m_registers[i].rangeEnd > maxRangeEnd) {
                maxRangeEnd = m_registers[i].rangeEnd;
                maxRangeIndex = i;
            }
        } else if (m_registers[i].rangeEnd > kReservedReg && m_registers[i].rangeEnd > kReservedReg) {
            size_t averageEnd = ((m_registers[i].rangeEnd + m_registers[i + 1].rangeEnd) + 1) >> 1;

            if (averageEnd > maxRangeEnd) {
                maxRangeEnd = averageEnd;
                maxRangeIndex = i;
            }
        }

        i += 2;
    }

    if (i == size) {
        ASSERT(maxRangeEnd != 0 || variable != nullptr);

        if (variable != nullptr && variable->rangeEnd >= maxRangeEnd) {
            return VariableList::kUnusedReg;
        }

        // Move variable into memory.
        i = maxRangeIndex;
        ASSERT(m_registers[i].rangeEnd > kReservedReg
               && m_registers[i + 1].rangeEnd > kReservedReg
               && (m_registers[i].variable != nullptr || m_registers[i + 1].variable != nullptr));

        VariableList::Variable* prevVariable = m_registers[i].variable;

        if (prevVariable != nullptr) {
            ASSERT(prevVariable->reg2 == prevVariable->reg1 || prevVariable->reg2 == prevVariable->reg1 + 1);
            prevVariable->reg1 = VariableList::kUnusedReg;
            prevVariable->reg2 = VariableList::kUnusedReg;
        }

        prevVariable = m_registers[i + 1].variable;
        if (prevVariable != nullptr) {
            ASSERT(prevVariable->reg2 == prevVariable->reg1 || prevVariable->reg2 == prevVariable->reg1 + 1);
            prevVariable->reg1 = VariableList::kUnusedReg;
            prevVariable->reg2 = VariableList::kUnusedReg;
        }
    }

    // Allocated registers are also reserved for the current byte code.
    m_registers[i].rangeEnd = kReservedReg;
    m_registers[i].variable = variable;
    m_registers[i + 1].rangeEnd = kReservedReg;
    m_registers[i + 1].variable = variable;

    if (variable != nullptr) {
        variable->reg1 = i;
        variable->reg2 = i + 1;
    }

    if (i + 1 >= static_cast<size_t>(m_usedSavedRegisters)) {
        m_usedSavedRegisters = i + 2;
    }

    return i;
}
#endif /* SLJIT_CONFIG_ARM_32 */

static inline int reuseTemporary(uint8_t type, VariableList::Variable** reusableRegs)
{
    if (!(type & (Instruction::Src0Allowed | Instruction::Src1Allowed | Instruction::Src2Allowed))) {
        return -1;
    }

    for (uint32_t i = 0; i < 3; i++) {
        if ((type & (Instruction::Src0Allowed << i)) && reusableRegs[i] != nullptr) {
            return i;
        }
    }

    return -1;
}

bool RegisterFile::reuseResult(uint8_t type, VariableList::Variable** reusableRegs, VariableList::Variable* resultVariable)
{
    if (!(type & (Instruction::Src0Allowed | Instruction::Src1Allowed | Instruction::Src2Allowed))) {
        return false;
    }

    uint16_t constraints = resultVariable->info;
    RegisterSet* registers = (type & Instruction::FloatOperandMarker) ? &m_floatSet : &m_integerSet;
#if (defined SLJIT_SEPARATE_VECTOR_REGISTERS && SLJIT_SEPARATE_VECTOR_REGISTERS)
    if ((type & Instruction::TypeMask) == Instruction::V128Operand) {
        registers = &m_vectorSet;
    }
#endif
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
    bool isInt64 = (type & Instruction::TypeMask) == Instruction::Int64Operand;
#endif /* SLJIT_32BIT_ARCHITECTURE */

    for (uint32_t i = 0; i < 3; i++) {
        VariableList::Variable* variable = reusableRegs[i];
        if ((type & (Instruction::Src0Allowed << i)) && variable != nullptr && registers->check(variable->reg1, constraints)) {
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            if (isInt64 && !registers->check(variable->reg2, constraints)) {
                continue;
            }
#endif /* SLJIT_32BIT_ARCHITECTURE */

            reusableRegs[i] = nullptr;

            resultVariable->reg1 = variable->reg1;
            registers->updateVariable(resultVariable->reg1, resultVariable);

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
            resultVariable->reg2 = variable->reg2;
            registers->updateVariable(resultVariable->reg2, resultVariable);
#endif /* SLJIT_32BIT_ARCHITECTURE */
            return true;
        }
    }

    return false;
}

void JITCompiler::allocateRegisters()
{
    if (m_variableList == nullptr) {
        m_savedIntegerRegCount = 0;
        m_savedFloatRegCount = 0;
#if (defined SLJIT_SEPARATE_VECTOR_REGISTERS && SLJIT_SEPARATE_VECTOR_REGISTERS)
        m_savedVectorRegCount = 0;
#endif /* SLJIT_SEPARATE_VECTOR_REGISTERS */
        return;
    }

#if (defined SLJIT_CONFIG_X86_32 && SLJIT_CONFIG_X86_32)
    const uint32_t numberOfscratchRegs = 3;
    const uint32_t numberOfsavedRegs = 1;
#else /* !SLJIT_CONFIG_X86_32 */
    const uint32_t numberOfscratchRegs = SLJIT_NUMBER_OF_SCRATCH_REGISTERS;
    const uint32_t numberOfsavedRegs = SLJIT_NUMBER_OF_SAVED_REGISTERS - 2;
#endif /* SLJIT_CONFIG_X86_32 */

    RegisterFile regs(numberOfscratchRegs, numberOfsavedRegs);

    size_t variableListParamCount = m_variableList->paramCount;
    for (size_t i = 0; i < variableListParamCount; i++) {
        VariableList::Variable* variable = m_variableList->variables.data() + i;

        ASSERT(!(variable->info & VariableList::kIsImmediate));

        if (variable->info & VariableList::kIsMerged) {
            continue;
        }

        ASSERT(variable->u.rangeStart == 0);

        if (variable->rangeEnd == 0) {
            continue;
        }

        regs.allocateVariable(variable);
    }

    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (item->isLabel()) {
            continue;
        }

        Instruction* instr = item->asInstruction();
        Operand* operand = instr->operands();
        const uint8_t* list = m_variableList->getOperandDescriptor(instr);
        uint32_t paramCount = instr->paramCount();
        size_t instrId = instr->id();
        bool hasResult = instr->resultCount() > 0;

        regs.freeUnusedRegisters(instrId + ((instr->info() & Instruction::kFreeUnusedEarly) ? 1 : 0));
        operand = instr->operands();
        instr->setRequiredRegsDescriptor(0);

        if (*list == 0) {
            // No register assignment required.
            ASSERT(instr->opcode() == ByteCode::EndOpcode || instr->opcode() == ByteCode::ThrowOpcode
                   || instr->opcode() == ByteCode::CallOpcode || instr->opcode() == ByteCode::CallIndirectOpcode
                   || instr->opcode() == ByteCode::CallRefOpcode || instr->opcode() == ByteCode::JumpOpcode
                   || instr->opcode() == ByteCode::ElemDropOpcode || instr->opcode() == ByteCode::DataDropOpcode
                   || instr->opcode() == ByteCode::StructNewOpcode || instr->opcode() == ByteCode::ArrayNewFixedOpcode
                   || instr->opcode() == ByteCode::ArrayInitDataOpcode || instr->opcode() == ByteCode::ArrayInitElemOpcode
                   || instr->opcode() == ByteCode::ArrayFillOpcode || instr->opcode() == ByteCode::ArrayCopyOpcode
                   || instr->opcode() == ByteCode::UnreachableOpcode || NOP_CHECK);

            if (!hasResult) {
                continue;
            }

            operand += paramCount;
            Operand* end = operand + instr->resultCount();

            while (operand < end) {
                VariableList::Variable* resultVariable = m_variableList->variables.data() + *operand;

                if (resultVariable->u.rangeStart == instrId && resultVariable->rangeEnd != instrId) {
                    regs.allocateVariable(resultVariable);
                }
                operand++;
            }
            continue;
        }

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        if (instr->opcode() == ByteCode::I64MulOpcode) {
            regs.integerSet().setDestroysR0R1();
            instr->setRequiredReg(0, regs.toCPUIntegerReg(regs.integerSet().allocateRegister(nullptr)));
            regs.integerSet().clearDestroysR0R1();
        }
#endif /* SLJIT_32BIT_ARCHITECTURE */

        // Step 1: check params first. Source registers which live range ends
        // can be reallocated if the corresponding SrcXAllowed flag is set.
        ASSERT(paramCount <= 3 && instr->resultCount() <= 1);
        VariableList::Variable* reusableRegs[3] = { nullptr };
        uint32_t tmpIndex = 0;

        for (uint32_t i = 0; i < paramCount; i++) {
            ASSERT((*list & Instruction::TypeMask) >= Instruction::Int32Operand
                   && (*list & Instruction::TypeMask) <= Instruction::Float64Operand);

            VariableList::Variable* variable = m_variableList->variables.data() + *operand;

            if (!(variable->info & VariableList::kIsImmediate)
                && variable->rangeEnd == instrId
                && variable->reg1 != VariableList::kUnusedReg) {
                reusableRegs[i] = variable;
            }

            ASSERT(!(*list & Instruction::TmpRequired) || (*list & Instruction::FloatOperandMarker));

            if ((*list & Instruction::FloatOperandMarker) && !(*list & Instruction::TmpNotAllowed)) {
                // Source registers are read-only.
                if ((*list & Instruction::TmpRequired) || (variable->info & VariableList::kIsImmediate)) {
                    uint8_t reg = variable->reg1;

                    if (reg != VariableList::kUnusedReg) {
                        ASSERT(!(variable->info & VariableList::kIsImmediate));
                        VECTOR_SELECT((*list & Instruction::TypeMask) == Instruction::V128Operand,
                                      regs.vectorReserve(reg),
                                      regs.floatReserve(reg))
#if (defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
                        if ((*list & Instruction::TypeMask) != Instruction::V128Operand) {
                            regs.floatReserve(reg + 1);
                        }
#endif /* SLJIT_CONFIG_ARM_32 */
                    } else {
#if (defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
                        if ((*list & Instruction::TypeMask) == Instruction::V128Operand) {
                            reg = regs.floatSet().allocateQuadRegister(nullptr);
                        } else {
#endif /* SLJIT_CONFIG_ARM_32 */
                            VECTOR_SELECT((*list & Instruction::TypeMask) == Instruction::V128Operand,
                                          reg = regs.vectorSet().allocateRegister(nullptr),
                                          reg = regs.floatSet().allocateRegister(nullptr))
#if (defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
                        }
#endif /* SLJIT_CONFIG_ARM_32 */
                    }
                    VECTOR_SELECT((*list & Instruction::TypeMask) == Instruction::V128Operand,
                                  instr->setRequiredReg(tmpIndex, regs.toCPUVectorReg(reg)),
                                  instr->setRequiredReg(tmpIndex, regs.toCPUFloatReg(reg)))
                }
                tmpIndex++;
            }

            operand++;
            list++;
        }

        if (instr->info() & Instruction::kIsMergeCompare) {
            ASSERT((list[0] & Instruction::TypeMask) == Instruction::Int32Operand && list[1] == 0);
            continue;
        }

        // Step 2: reuse as many registers as possible. Reusing
        // has limitations, which are described in the operand list.
        if (hasResult) {
            VariableList::Variable* resultVariable = m_variableList->variables.data() + *operand;
            uint8_t type = (*list & Instruction::TypeMask);

            if (resultVariable->u.rangeStart == instrId && resultVariable->rangeEnd != instrId) {
                if (!regs.reuseResult(*list, reusableRegs, resultVariable)) {
                    regs.allocateVariable(resultVariable);
                }
            }

            if (type & Instruction::FloatOperandMarker) {
                if (resultVariable->reg1 != VariableList::kUnusedReg) {
                    VECTOR_SELECT(type == Instruction::V128Operand,
                                  regs.vectorReserve(resultVariable->reg1),
                                  regs.floatReserve(resultVariable->reg1))
#if (defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
                    regs.floatReserve(resultVariable->reg2);
#endif /* SLJIT_CONFIG_ARM_32 */
                }
            } else if (resultVariable->reg1 != VariableList::kUnusedReg) {
                regs.integerReserve(resultVariable->reg1);
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
                regs.integerReserve(resultVariable->reg2);
#endif /* SLJIT_32BIT_ARCHITECTURE */
            }

            if (*list & Instruction::TmpRequired) {
                uint8_t resultReg = resultVariable->reg1;

                if (resultReg == VariableList::kUnusedReg) {
                    if (type & Instruction::FloatOperandMarker) {
#if (defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
                        if (type == Instruction::V128Operand) {
                            resultReg = regs.floatSet().allocateQuadRegister(nullptr);
                        } else {
#endif /* SLJIT_CONFIG_ARM_32 */
                            VECTOR_SELECT(type == Instruction::V128Operand,
                                          resultReg = regs.vectorSet().allocateRegister(nullptr),
                                          resultReg = regs.floatSet().allocateRegister(nullptr))
#if (defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
                        }
#endif /* SLJIT_CONFIG_ARM_32 */
                        VECTOR_SELECT(type == Instruction::V128Operand,
                                      resultReg = regs.toCPUVectorReg(resultReg),
                                      resultReg = regs.toCPUFloatReg(resultReg))
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
                    } else if (type == Instruction::Int64Operand) {
                        uint8_t otherReg;
                        resultReg = regs.integerSet().allocateRegisterPair(nullptr, &otherReg);
                        instr->setRequiredReg(tmpIndex++, regs.toCPUIntegerReg(otherReg));
                        resultReg = regs.toCPUIntegerReg(resultReg);
#endif /* SLJIT_32BIT_ARCHITECTURE */
                    } else {
                        resultReg = regs.integerSet().allocateRegister(nullptr);
                        resultReg = regs.toCPUIntegerReg(resultReg);
                    }
                } else if (type & Instruction::FloatOperandMarker) {
#if (defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
                    if (type == Instruction::V128Operand) {
                        regs.floatReserve(resultReg + 1);
                    }
#endif /* SLJIT_CONFIG_ARM_32 */
                    VECTOR_SELECT(type == Instruction::V128Operand,
                                  (regs.vectorReserve(resultReg), resultReg = regs.toCPUVectorReg(resultReg)),
                                  (regs.floatReserve(resultReg), resultReg = regs.toCPUFloatReg(resultReg)))
                } else {
#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
                    if (type == Instruction::Int64Operand) {
                        instr->setRequiredReg(tmpIndex++, regs.toCPUIntegerReg(resultReg));
                        resultReg = resultVariable->reg2;
                        ASSERT(resultReg != VariableList::kUnusedReg);
                    }
#endif /* SLJIT_32BIT_ARCHITECTURE */
                    regs.integerReserve(resultReg);
                    resultReg = regs.toCPUIntegerReg(resultReg);
                }

                instr->setRequiredReg(tmpIndex++, resultReg);
            }

            list++;
        }

        uint32_t reuseTmpIndex = tmpIndex;

        for (const uint8_t* nextType = list; *nextType != 0; nextType++) {
            int reuseIdx = reuseTemporary(*nextType, reusableRegs);

            if (reuseIdx >= 0) {
                VariableList::Variable* variable = reusableRegs[reuseIdx];
                // A register cannot be reused twice.
                reusableRegs[reuseIdx] = nullptr;

                uint8_t reg = variable->reg1;

                if (*nextType & Instruction::FloatOperandMarker) {
#if (defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
                    if ((*nextType & Instruction::TypeMask) == Instruction::V128Operand) {
                        regs.floatReserve(reg + 1);
                    }
#endif /* SLJIT_CONFIG_ARM_32 */
                    VECTOR_SELECT((*nextType & Instruction::TypeMask) == Instruction::V128Operand,
                                  (regs.vectorReserve(reg), instr->setRequiredReg(reuseTmpIndex, regs.toCPUVectorReg(reg))),
                                  (regs.floatReserve(reg), instr->setRequiredReg(reuseTmpIndex, regs.toCPUFloatReg(reg))))
                } else {
                    regs.integerReserve(reg);
                    instr->setRequiredReg(reuseTmpIndex, regs.toCPUIntegerReg(reg));
                }
            }

            reuseTmpIndex++;
        }

        // Step 3: initialize uninitialized temporary values.
        for (; *list != 0; list++) {
            // Assign temporary registers.
            ASSERT(((*list & Instruction::TypeMask) >= Instruction::Int32Operand || (*list & Instruction::TmpRequired))
                   && (*list & Instruction::TypeMask) <= Instruction::Float64Operand);

            if (instr->requiredReg(tmpIndex) == 0) {
                if (*list & Instruction::FloatOperandMarker) {
                    VECTOR_SELECT((*list & Instruction::TypeMask) == Instruction::V128Operand,
                                  instr->setRequiredReg(tmpIndex, regs.toCPUVectorReg(regs.vectorSet().allocateRegister(nullptr))),
                                  instr->setRequiredReg(tmpIndex, regs.toCPUFloatReg(regs.floatSet().allocateRegister(nullptr))))
                } else {
                    instr->setRequiredReg(tmpIndex, regs.toCPUIntegerReg(regs.integerSet().allocateRegister(nullptr)));
                }
            }

            tmpIndex++;
        }

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        // 64 bit shifts / rotates requires special handling.
        if ((instr->info() & (Instruction::kIsShift | Instruction::kIs32Bit)) == Instruction::kIsShift) {
            ASSERT(operand == instr->operands() + 2);
            VariableList::Variable* variable = m_variableList->variables.data() + operand[-1];
            bool isImmediate = (variable->info & VariableList::kIsImmediate) != 0;

            if (instr->opcode() == ByteCode::I64RotlOpcode || instr->opcode() == ByteCode::I64RotrOpcode) {
                instr->setRequiredReg(2, regs.toCPUIntegerReg(regs.integerSet().allocateRegister(nullptr)));

                if (!isImmediate) {
                    instr->setRequiredReg(3, regs.toCPUIntegerReg(regs.integerSet().allocateRegister(nullptr)));
                }
            } else if (!isImmediate) {
                instr->setRequiredReg(2, regs.toCPUIntegerReg(regs.integerSet().allocateRegister(nullptr)));
            }
        }
#endif /* SLJIT_32BIT_ARCHITECTURE */
    }

    m_savedIntegerRegCount = regs.integerSet().getSavedRegCount();
    m_savedFloatRegCount = regs.floatSet().getSavedRegCount();
#if (defined SLJIT_SEPARATE_VECTOR_REGISTERS && SLJIT_SEPARATE_VECTOR_REGISTERS)
    m_savedVectorRegCount = regs.vectorSet().getSavedRegCount();
#endif /* SLJIT_SEPARATE_VECTOR_REGISTERS */

    // Insert stack inits before the offsets are destroyed.
    insertStackInitList(nullptr, 0, variableListParamCount);

    for (auto it : m_variableList->catchUpdates) {
        insertStackInitList(it.handler, it.variableListStart, it.variableListSize);
    }

    size_t size = m_variableList->variables.size();
    for (size_t i = 0; i < size; i++) {
        VariableList::Variable& variable = m_variableList->variables[i];

        if (variable.reg1 != VariableList::kUnusedReg) {
            ASSERT(!(variable.info & (VariableList::kIsMerged | VariableList::kIsImmediate)));
            uint8_t reg1;

            if (variable.info & Instruction::FloatOperandMarker) {
                VECTOR_SELECT(variable.info == Instruction::V128Operand,
                              reg1 = regs.toCPUVectorReg(variable.reg1),
                              reg1 = regs.toCPUFloatReg(variable.reg1))
            } else {
                reg1 = regs.toCPUIntegerReg(variable.reg1);

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
                if (variable.reg2 != variable.reg1) {
                    uint8_t reg2;

                    if (variable.info & Instruction::FloatOperandMarker) {
                        reg2 = regs.toCPUFloatReg(variable.reg2);
                    } else {
                        reg2 = regs.toCPUIntegerReg(variable.reg2);
                    }

                    variable.value = VARIABLE_SET(SLJIT_REG_PAIR(reg1, reg2), Instruction::Register);
                    continue;
                }
#endif /* SLJIT_32BIT_ARCHITECTURE */
            }

            variable.value = VARIABLE_SET(reg1, Instruction::Register);
        }
    }
}

void JITCompiler::allocateRegistersSimple()
{
    m_savedIntegerRegCount = 0;
    m_savedFloatRegCount = 0;
#if (defined SLJIT_SEPARATE_VECTOR_REGISTERS && SLJIT_SEPARATE_VECTOR_REGISTERS)
    m_savedVectorRegCount = 0;
#endif /* SLJIT_SEPARATE_VECTOR_REGISTERS */

    if (m_variableList == nullptr) {
        return;
    }

    // Dummy register allocator.
    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (item->isLabel()) {
            continue;
        }

        Instruction* instr = item->asInstruction();
        Operand* operand = instr->operands();
        Operand* end = operand + instr->paramCount() + instr->resultCount();
        const uint8_t* list = m_variableList->getOperandDescriptor(instr);
        uint32_t paramCount = instr->paramCount();
        uint32_t resultCount = instr->resultCount();
        uint32_t tmpIndex = 0;
        uint32_t nextIntIndex = SLJIT_R0;
        uint32_t nextFloatIndex = SLJIT_FR0;
#if (defined SLJIT_SEPARATE_VECTOR_REGISTERS && SLJIT_SEPARATE_VECTOR_REGISTERS)
        uint32_t nextVectorIndex = FIRST_VECTOR_REG;
#endif /* SLJIT_SEPARATE_VECTOR_REGISTERS */

        instr->setRequiredRegsDescriptor(0);

        if (*list == 0) {
            // No register assignment required.
            ASSERT(instr->opcode() == ByteCode::EndOpcode || instr->opcode() == ByteCode::ThrowOpcode
                   || instr->opcode() == ByteCode::CallOpcode || instr->opcode() == ByteCode::CallIndirectOpcode
                   || instr->opcode() == ByteCode::CallRefOpcode || instr->opcode() == ByteCode::JumpOpcode
                   || instr->opcode() == ByteCode::ElemDropOpcode || instr->opcode() == ByteCode::DataDropOpcode
                   || instr->opcode() == ByteCode::StructNewOpcode || instr->opcode() == ByteCode::ArrayNewFixedOpcode
                   || instr->opcode() == ByteCode::ArrayInitDataOpcode || instr->opcode() == ByteCode::ArrayInitElemOpcode
                   || instr->opcode() == ByteCode::ArrayFillOpcode || instr->opcode() == ByteCode::ArrayCopyOpcode
                   || instr->opcode() == ByteCode::UnreachableOpcode || NOP_CHECK);
            continue;
        }

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        if (instr->opcode() == ByteCode::I64MulOpcode) {
            instr->setRequiredReg(0, SLJIT_R2);
        }
#endif /* SLJIT_32BIT_ARCHITECTURE */

        for (uint32_t i = 0; i < paramCount; i++) {
            ASSERT((*list & Instruction::TypeMask) >= Instruction::Int32Operand
                   && (*list & Instruction::TypeMask) <= Instruction::Float64Operand);

            if ((*list & Instruction::TypeMask & Instruction::FloatOperandMarker)
                && !(*list & Instruction::TmpNotAllowed)) {
                VariableList::Variable& variable = m_variableList->variables[*operand];

                // Source registers are read-only.
                if ((*list & Instruction::TmpRequired) || (variable.info & VariableList::kIsImmediate)) {
                    VECTOR_SELECT((*list & Instruction::TypeMask) == Instruction::V128Operand,
                                  instr->setRequiredReg(tmpIndex, nextVectorIndex++),
                                  instr->setRequiredReg(tmpIndex, nextFloatIndex++))
#if (defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
                    if ((*list & Instruction::TypeMask) == Instruction::V128Operand) {
                        // Quad registers are register pairs.
                        nextFloatIndex++;
                    }
#endif /* SLJIT_CONFIG_ARM_32 */
                }
                tmpIndex++;
            }

            operand++;
            list++;
        }

        for (; *list != 0; list++) {
            // Assign temporary registers.
            ASSERT(((*list & Instruction::TypeMask) >= Instruction::Int32Operand || (*list & Instruction::TmpRequired))
                   && (*list & Instruction::TypeMask) <= Instruction::Float64Operand);

            if (resultCount > 0) {
                --resultCount;

                if (!(*list & Instruction::TmpRequired)) {
                    continue;
                }

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
                if ((*list & Instruction::TypeMask) == Instruction::Int64Operand) {
                    instr->setRequiredReg(tmpIndex, nextIntIndex++);
                    tmpIndex++;
                }
#endif /* SLJIT_32BIT_ARCHITECTURE */
            }

            if (*list & Instruction::FloatOperandMarker) {
                VECTOR_SELECT((*list & Instruction::TypeMask) == Instruction::V128Operand,
                              instr->setRequiredReg(tmpIndex, nextVectorIndex++),
                              instr->setRequiredReg(tmpIndex, nextFloatIndex++))
#if (defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
                if ((*list & Instruction::TypeMask) == Instruction::V128Operand) {
                    // Quad registers are register pairs.
                    nextFloatIndex++;
                }
#endif /* SLJIT_CONFIG_ARM_32 */
            } else {
                instr->setRequiredReg(tmpIndex, nextIntIndex++);
            }
            tmpIndex++;
        }

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        // 64 bit shifts / rotates requires special handling.
        if ((instr->info() & (Instruction::kIsShift | Instruction::kIs32Bit)) == Instruction::kIsShift) {
            ASSERT(operand == instr->operands() + 2);
            VariableList::Variable* variable = m_variableList->variables.data() + operand[-1];
            bool isImmediate = (variable->info & VariableList::kIsImmediate) != 0;

            if (instr->opcode() == ByteCode::I64RotlOpcode || instr->opcode() == ByteCode::I64RotrOpcode) {
                instr->setRequiredReg(2, SLJIT_R2);

                if (!isImmediate) {
#if (defined SLJIT_PREF_SHIFT_REG) && (SLJIT_PREF_SHIFT_REG == SLJIT_R2)
                    instr->setRequiredReg(2, SLJIT_R3);
                    instr->setRequiredReg(3, SLJIT_R2);
#else /* SLJIT_PREF_SHIFT_REG != SLJIT_R2 */
                    instr->setRequiredReg(3, SLJIT_R3);
#endif /* !SLJIT_PREF_SHIFT_REG */
                }
            } else if (!isImmediate) {
#ifdef SLJIT_PREF_SHIFT_REG
                instr->setRequiredReg(2, SLJIT_PREF_SHIFT_REG);
#else /* !SLJIT_PREF_SHIFT_REG */
                instr->setRequiredReg(2, SLJIT_R2);
#endif /* SLJIT_PREF_SHIFT_REG */
            }
        }
#endif /* SLJIT_32BIT_ARCHITECTURE */

        ASSERT(resultCount == 0);
    }
}

void JITCompiler::freeVariables()
{
    for (InstructionListItem* item = m_first; item != nullptr; item = item->next()) {
        if (item->isLabel()) {
            continue;
        }

        Instruction* instr = item->asInstruction();
        Operand* operand = instr->operands();
        Operand* end = operand + instr->paramCount() + instr->resultCount();
        uint16_t info = 0;

        if (instr->opcode() == ByteCode::MoveF32Opcode || instr->opcode() == ByteCode::MoveF64Opcode) {
            ASSERT(end - operand == 2);

            VariableList::Variable& src = m_variableList->variables[*operand];
            ASSERT((src.info & Instruction::TypeMask) > 0);
            info = src.info;
            *operand++ = src.value;

            VariableList::Variable& dst = m_variableList->variables[*operand];
            ASSERT((dst.info & Instruction::TypeMask) == Instruction::Float32Operand
                   || (dst.info & Instruction::TypeMask) == Instruction::Float64Operand);
            *operand = dst.value;
        } else {
            while (operand < end) {
                VariableList::Variable& variable = m_variableList->variables[*operand];

                ASSERT((variable.info & Instruction::TypeMask) > 0);
                info |= variable.info;

                *operand++ = variable.value;
            }
        }

        if (info & Instruction::FloatOperandMarker) {
            instr->addInfo(Instruction::kHasFloatOperand);
        }
    }

    delete m_variableList;
    m_variableList = nullptr;
}

} // namespace Walrus

#undef VECTOR_SELECT
#endif // WALRUS_ENABLE_JIT
