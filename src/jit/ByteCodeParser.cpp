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
#include "runtime/JITExec.h"
#include "runtime/Module.h"

#include <map>

namespace Walrus {

#define STACK_OFFSET(v) ((v) >> 2)
#define COMPUTE_OFFSET(idx, offset) \
    (static_cast<size_t>(static_cast<ssize_t>(idx) + (offset)))

static void createInstructionList(JITCompiler* compiler, ModuleFunction* function)
{
    size_t idx = 0;
    size_t endIdx = function->currentByteCodeSize();

    std::map<size_t, Label*> labels;

    // Construct labels first
    while (idx < endIdx) {
        ByteCode* byteCode = function->peekByteCode<ByteCode>(idx);
        OpcodeKind opcode = byteCode->opcode();

        switch (opcode) {
        case OpcodeKind::JumpOpcode: {
            Jump* jump = reinterpret_cast<Jump*>(byteCode);
            labels[COMPUTE_OFFSET(idx, jump->offset())] = nullptr;
            break;
        }
        case OpcodeKind::JumpIfTrueOpcode: {
            JumpIfTrue* jumpIfTrue = reinterpret_cast<JumpIfTrue*>(byteCode);
            labels[COMPUTE_OFFSET(idx, jumpIfTrue->offset())] = nullptr;
            break;
        }
        case OpcodeKind::JumpIfFalseOpcode: {
            JumpIfFalse* jumpIfFalse = reinterpret_cast<JumpIfFalse*>(byteCode);
            labels[COMPUTE_OFFSET(idx, jumpIfFalse->offset())] = nullptr;
            break;
        }
        case BrTableOpcode: {
            BrTable* brTable = reinterpret_cast<BrTable*>(byteCode);
            labels[COMPUTE_OFFSET(idx, brTable->defaultOffset())] = nullptr;

            int32_t* jumpOffsets = brTable->jumpOffsets();
            int32_t* jumpOffsetsEnd = jumpOffsets + brTable->tableSize();

            while (jumpOffsets < jumpOffsetsEnd) {
                labels[COMPUTE_OFFSET(idx, *jumpOffsets)] = nullptr;
                jumpOffsets++;
            }
            break;
        }
        default: {
            break;
        }
        }

        idx += byteCode->byteCodeSize();
    }

    std::map<size_t, Label*>::iterator it;

    // Values needs to be modified.
    for (it = labels.begin(); it != labels.end(); it++) {
        it->second = new Label();
    }

    it = labels.begin();
    size_t nextLabelIndex = ~static_cast<size_t>(0);

    if (it != labels.end()) {
        nextLabelIndex = it->first;
    }

    idx = 0;
    while (idx < endIdx) {
        if (idx == nextLabelIndex) {
            compiler->appendLabel(it->second);

            it++;
            nextLabelIndex = ~static_cast<size_t>(0);

            if (it != labels.end()) {
                nextLabelIndex = it->first;
            }
        }

        ByteCode* byteCode = function->peekByteCode<ByteCode>(idx);
        OpcodeKind opcode = byteCode->opcode();
        Instruction::Group group = Instruction::Any;
        uint8_t paramCount = 0;
        uint16_t info = 0;

        switch (opcode) {
        // Binary operation
        case I32AddOpcode:
        case I32SubOpcode:
        case I32MulOpcode:
        case I32DivSOpcode:
        case I32DivUOpcode:
        case I32RemSOpcode:
        case I32RemUOpcode:
        case I32RotlOpcode:
        case I32RotrOpcode:
        case I32AndOpcode:
        case I32OrOpcode:
        case I32XorOpcode:
        case I32ShlOpcode:
        case I32ShrSOpcode:
        case I32ShrUOpcode: {
            info = Instruction::kIs32Bit;
            group = Instruction::Binary;
            paramCount = 2;
            break;
        }
        case I64AddOpcode:
        case I64SubOpcode:
        case I64MulOpcode:
        case I64DivSOpcode:
        case I64DivUOpcode:
        case I64RemSOpcode:
        case I64RemUOpcode:
        case I64RotlOpcode:
        case I64RotrOpcode:
        case I64AndOpcode:
        case I64OrOpcode:
        case I64XorOpcode:
        case I64ShlOpcode:
        case I64ShrSOpcode:
        case I64ShrUOpcode: {
            group = Instruction::Binary;
            paramCount = 2;
            break;
        }
        case I32EqOpcode:
        case I32NeOpcode:
        case I32LtSOpcode:
        case I32LtUOpcode:
        case I32GtSOpcode:
        case I32GtUOpcode:
        case I32LeSOpcode:
        case I32LeUOpcode:
        case I32GeSOpcode:
        case I32GeUOpcode: {
            info = Instruction::kIs32Bit;
            group = Instruction::Compare;
            paramCount = 2;
            break;
        }
        case I64EqOpcode:
        case I64NeOpcode:
        case I64LtSOpcode:
        case I64LtUOpcode:
        case I64GtSOpcode:
        case I64GtUOpcode:
        case I64LeSOpcode:
        case I64LeUOpcode:
        case I64GeSOpcode:
        case I64GeUOpcode: {
            group = Instruction::Compare;
            paramCount = 2;
            break;
        }
        case F32AddOpcode:
        case F32SubOpcode:
        case F32MulOpcode:
        case F32DivOpcode:
        case F32MaxOpcode:
        case F32MinOpcode:
        case F32CopysignOpcode:
        case F64AddOpcode:
        case F64SubOpcode:
        case F64MulOpcode:
        case F64DivOpcode:
        case F64MaxOpcode:
        case F64MinOpcode:
        case F64CopysignOpcode: {
            group = Instruction::BinaryFloat;
            paramCount = 2;
            break;
        }
        case F32EqOpcode:
        case F32NeOpcode:
        case F32LtOpcode:
        case F32GtOpcode:
        case F32LeOpcode:
        case F32GeOpcode:
        case F64EqOpcode:
        case F64NeOpcode:
        case F64LtOpcode:
        case F64GtOpcode:
        case F64LeOpcode:
        case F64GeOpcode: {
            group = Instruction::CompareFloat;
            paramCount = 2;
            break;
        }
        case I32ClzOpcode:
        case I32CtzOpcode:
        case I32PopcntOpcode:
        case I32Extend8SOpcode:
        case I32Extend16SOpcode: {
            group = Instruction::Unary;
            paramCount = 1;
            info = Instruction::kIs32Bit;
            break;
        }
        case I64ClzOpcode:
        case I64CtzOpcode:
        case I64PopcntOpcode:
        case I64Extend8SOpcode:
        case I64Extend16SOpcode:
        case I64Extend32SOpcode: {
            group = Instruction::Unary;
            paramCount = 1;
            break;
        }
        case F32CeilOpcode:
        case F32FloorOpcode:
        case F32TruncOpcode:
        case F32NearestOpcode:
        case F32SqrtOpcode:
        case F32AbsOpcode:
        case F32NegOpcode:
        case F64CeilOpcode:
        case F64FloorOpcode:
        case F64TruncOpcode:
        case F64NearestOpcode:
        case F64SqrtOpcode:
        case F64AbsOpcode:
        case F64NegOpcode: {
            group = Instruction::UnaryFloat;
            paramCount = 1;
            break;
        }
        case I32EqzOpcode: {
            group = Instruction::Compare;
            paramCount = 1;
            info = Instruction::kIs32Bit;
            break;
        }
        case I64EqzOpcode: {
            group = Instruction::Compare;
            paramCount = 1;
            break;
        }
        case I32WrapI64Opcode:
        case I64ExtendI32SOpcode:
        case I64ExtendI32UOpcode: {
            group = Instruction::Convert;
            paramCount = 1;
            break;
        }
        case NopOpcode:
        case UnreachableOpcode: {
            compiler->append(byteCode, group, opcode, 0, 0);
            break;
        }
        case JumpOpcode: {
            Jump* jump = reinterpret_cast<Jump*>(byteCode);
            compiler->appendBranch(jump, opcode, labels[COMPUTE_OFFSET(idx, jump->offset())], 0);
            break;
        }
        case JumpIfTrueOpcode: {
            JumpIfTrue* jumpIfTrue = reinterpret_cast<JumpIfTrue*>(byteCode);
            compiler->appendBranch(jumpIfTrue, opcode, labels[COMPUTE_OFFSET(idx, jumpIfTrue->offset())], STACK_OFFSET(jumpIfTrue->srcOffset()));
            break;
        }
        case JumpIfFalseOpcode: {
            JumpIfFalse* jumpIfFalse = reinterpret_cast<JumpIfFalse*>(byteCode);
            compiler->appendBranch(jumpIfFalse, opcode, labels[COMPUTE_OFFSET(idx, jumpIfFalse->offset())], STACK_OFFSET(jumpIfFalse->srcOffset()));
            break;
        }
        case BrTableOpcode: {
            BrTable* brTable = reinterpret_cast<BrTable*>(byteCode);
            uint32_t tableSize = brTable->tableSize();
            BrTableInstruction* instr = compiler->appendBrTable(brTable, tableSize, STACK_OFFSET(brTable->condOffset()));
            Label** labelList = instr->targetLabels();

            int32_t* jumpOffsets = brTable->jumpOffsets();
            int32_t* jumpOffsetsEnd = jumpOffsets + tableSize;

            while (jumpOffsets < jumpOffsetsEnd) {
                Label* label = labels[COMPUTE_OFFSET(idx, *jumpOffsets)];

                label->append(instr);
                *labelList++ = label;
                jumpOffsets++;
            }

            Label* label = labels[COMPUTE_OFFSET(idx, brTable->defaultOffset())];

            label->append(instr);
            *labelList = label;

            if (compiler->options() & JITCompiler::kHasCondMov) {
                tableSize++;
            }

            compiler->increaseBranchTableSize(tableSize);
            break;
        }
        case Const32Opcode: {
            Instruction* instr = compiler->append(byteCode, Instruction::Immediate, Const32Opcode, 0, 1);

            Const32* const32 = reinterpret_cast<Const32*>(byteCode);
            Operand* operands = instr->operands();

            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(const32->dstOffset());
            break;
        }
        case Const64Opcode: {
            Instruction* instr = compiler->append(byteCode, Instruction::Immediate, Const64Opcode, 0, 1);

            Const64* const64 = reinterpret_cast<Const64*>(byteCode);
            Operand* operands = instr->operands();

            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(const64->dstOffset());
            break;
        }
        case Move32Opcode: {
            Instruction* instr = compiler->append(byteCode, Instruction::Move, Move32Opcode, 1, 1);

            Move32* move32 = reinterpret_cast<Move32*>(byteCode);
            Operand* operands = instr->operands();

            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(move32->srcOffset());
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(move32->dstOffset());
            break;
        }
        case Move64Opcode: {
            Instruction* instr = compiler->append(byteCode, Instruction::Move, Move64Opcode, 1, 1);

            Move64* move64 = reinterpret_cast<Move64*>(byteCode);
            Operand* operands = instr->operands();

            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(move64->srcOffset());
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(move64->dstOffset());
            break;
        }
        case EndOpcode: {
            uint32_t size = function->functionType()->result().size();

            Instruction* instr = compiler->append(byteCode, Instruction::Any, opcode, size, 0);
            Operand* param = instr->params();
            Operand* end = param + size;
            ByteCodeStackOffset* offsets = reinterpret_cast<End*>(byteCode)->resultOffsets();

            while (param < end) {
                param->item = nullptr;
                param->offset = STACK_OFFSET(*offsets++);
                param++;
            }

            idx += byteCode->byteCodeSize();

            if (idx != endIdx) {
                instr->addInfo(Instruction::kEarlyReturn);
            }

            continue;
        }
        default: {
            break;
        }
        }

        if (paramCount == 2) {
            ASSERT(group != Instruction::Any);

            Instruction* instr = compiler->append(byteCode, group, opcode, 2, 1);
            instr->addInfo(info);

            BinaryOperation* binaryOperation = reinterpret_cast<BinaryOperation*>(byteCode);
            Operand* operands = instr->operands();

            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(binaryOperation->srcOffset()[0]);
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(binaryOperation->srcOffset()[1]);
            operands[2].item = nullptr;
            operands[2].offset = STACK_OFFSET(binaryOperation->dstOffset());
        } else if (paramCount == 1) {
            ASSERT(group != Instruction::Any);

            Instruction* instr = compiler->append(byteCode, group, opcode, 1, 1);
            instr->addInfo(info);

            UnaryOperation* unaryOperation = reinterpret_cast<UnaryOperation*>(byteCode);

            Operand* operands = instr->operands();
            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(unaryOperation->srcOffset());
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(unaryOperation->dstOffset());
        }

        idx += byteCode->byteCodeSize();
    }

    compiler->buildParamDependencies(STACK_OFFSET(function->requiredStackSize()));

    if (compiler->verboseLevel() >= 1) {
        compiler->dump();
    }

    Walrus::JITFunction* jitFunc = new JITFunction();

    function->setJITFunction(jitFunc);
    compiler->appendFunction(jitFunc, true);
    compiler->clear();
}

void Module::jitCompile(int verboseLevel)
{
    size_t functionCount = m_functions.size();

    JITCompiler compiler(verboseLevel);

    for (size_t i = 0; i < functionCount; i++) {
        if (verboseLevel >= 1) {
            printf("[[[[[[[  Function %3d  ]]]]]]]\n", static_cast<int>(i));
        }

        createInstructionList(&compiler, m_functions[i]);
    }

    m_jitModule = compiler.compile();
}

} // namespace Walrus
