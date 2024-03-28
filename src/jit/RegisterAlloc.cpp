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

#include <set>

namespace Walrus {

void JITCompiler::allocateRegisters()
{
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
        const uint8_t* list = instr->getOperandDescriptor();
        uint32_t paramCount = instr->paramCount();
        uint32_t resultCount = instr->resultCount();
        uint32_t tmpIndex = 0;
        uint32_t nextIntIndex = SLJIT_R0;
        uint32_t nextFloatIndex = SLJIT_FR0;

        while (operand < end) {
            VariableRef ref = operand->ref;

            VariableList::Variable& variable = m_variableList->variables[ref];

            ASSERT((variable.info & Instruction::TypeMask) > 0);
            operand->ref = variable.value;

            operand++;
        }

        operand = instr->operands();
        instr->setRequiredRegsDescriptor(0);

        if (*list == 0) {
            // No register assignment required.
            ASSERT(instr->opcode() == ByteCode::EndOpcode || instr->opcode() == ByteCode::ThrowOpcode
                   || instr->opcode() == ByteCode::CallOpcode || instr->opcode() == ByteCode::CallIndirectOpcode
                   || instr->opcode() == ByteCode::ElemDropOpcode || instr->opcode() == ByteCode::DataDropOpcode
                   || instr->opcode() == ByteCode::JumpOpcode || instr->opcode() == ByteCode::UnreachableOpcode);
            continue;
        }

        for (uint32_t i = 0; i < paramCount; i++) {
            ASSERT((*list & Instruction::TypeMask) >= Instruction::Int32Operand
                   && (*list & Instruction::TypeMask) <= Instruction::Float64Operand);

            if ((*list & Instruction::TypeMask) >= Instruction::FloatOperandStart
                && !(*list & Instruction::TmpNotAllowed)) {
                // Source registers are read-only.
                if ((*list & Instruction::TmpRequired)
                    || (VARIABLE_TYPE(operand->ref) == Operand::Immediate)) {
                    instr->setRequiredReg(tmpIndex, nextFloatIndex++);
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

            if ((*list & Instruction::TypeMask) < Instruction::FloatOperandStart) {
                instr->setRequiredReg(tmpIndex, nextIntIndex++);
            } else {
                instr->setRequiredReg(tmpIndex, nextFloatIndex++);
#if (defined SLJIT_CONFIG_ARM_32 && SLJIT_CONFIG_ARM_32)
                if ((*list & Instruction::TypeMask) == Instruction::V128Operand) {
                    // Quad registers are register pairs.
                    nextFloatIndex++;
                }
#endif /* SLJIT_CONFIG_ARM_32 */
            }
            tmpIndex++;
        }

#if (defined SLJIT_32BIT_ARCHITECTURE && SLJIT_32BIT_ARCHITECTURE)
        // 64 bit shifts / rotates requires special handling.
        if ((instr->info() & (Instruction::kIsShift | Instruction::kIs32Bit)) == Instruction::kIsShift) {
            ASSERT(operand == instr->operands() + 2);
            bool isImmediate = (VARIABLE_TYPE(operand[-1].ref) == Operand::Immediate);

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

    delete m_variableList;
    m_variableList = nullptr;
}

} // namespace Walrus
