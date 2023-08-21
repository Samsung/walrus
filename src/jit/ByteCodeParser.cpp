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

#define COMPUTE_OFFSET(idx, offset) \
    (static_cast<size_t>(static_cast<ssize_t>(idx) + (offset)))

class TryRange {
public:
    TryRange(size_t start, size_t end)
        : m_start(start)
        , m_end(end)
    {
    }

    size_t start() const { return m_start; }
    size_t end() const { return m_end; }

    bool operator<(const TryRange& other) const
    {
        return m_start < other.m_start || (m_start == other.m_start && m_end > other.m_end);
    }

private:
    size_t m_start;
    size_t m_end;
};

void buildCatchInfo(JITCompiler* compiler, ModuleFunction* function, std::map<size_t, Label*>& labels)
{
    std::map<TryRange, size_t> ranges;

    for (auto it : function->catchInfo()) {
        ranges[TryRange(it.m_tryStart, it.m_tryEnd)]++;
    }

    std::vector<TryBlock>& tryBlocks = compiler->tryBlocks();
    size_t counter = tryBlocks.size();

    tryBlocks.reserve(counter + ranges.size());

    // The it->second assignment does not work with auto iterator.
    for (std::map<TryRange, size_t>::iterator it = ranges.begin(); it != ranges.end(); it++) {
        Label* start = labels[it->first.start()];

        start->addInfo(Label::kHasTryInfo);

        tryBlocks.push_back(TryBlock(start, it->second));
        it->second = counter++;
    }

    for (auto it : function->catchInfo()) {
        Label* catchLabel = labels[it.m_catchStartPosition];
        size_t idx = ranges[TryRange(it.m_tryStart, it.m_tryEnd)];

        if (tryBlocks[idx].catchBlocks.size() == 0) {
            // The first catch block terminates the try block.
            catchLabel->addInfo(Label::kHasCatchInfo);
        }

        tryBlocks[idx].catchBlocks.push_back(TryBlock::CatchBlock(catchLabel, it.m_stackSizeToBe, it.m_tagIndex));
    }
}

static void createInstructionList(JITCompiler* compiler, ModuleFunction* function, Module* module)
{
    size_t idx = 0;
    size_t endIdx = function->currentByteCodeSize();

    if (endIdx == 0) {
        // If a function has no End opcode, it is an imported function.
        return;
    }

    std::map<size_t, Label*> labels;

    // Construct labels first
    while (idx < endIdx) {
        ByteCode* byteCode = function->peekByteCode<ByteCode>(idx);
        ByteCode::Opcode opcode = byteCode->opcode();

        switch (opcode) {
        case ByteCode::JumpOpcode: {
            Jump* jump = reinterpret_cast<Jump*>(byteCode);
            labels[COMPUTE_OFFSET(idx, jump->offset())] = nullptr;
            break;
        }
        case ByteCode::JumpIfTrueOpcode: {
            JumpIfTrue* jumpIfTrue = reinterpret_cast<JumpIfTrue*>(byteCode);
            labels[COMPUTE_OFFSET(idx, jumpIfTrue->offset())] = nullptr;
            break;
        }
        case ByteCode::JumpIfFalseOpcode: {
            JumpIfFalse* jumpIfFalse = reinterpret_cast<JumpIfFalse*>(byteCode);
            labels[COMPUTE_OFFSET(idx, jumpIfFalse->offset())] = nullptr;
            break;
        }
        case ByteCode::BrTableOpcode: {
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

        idx += byteCode->getSize();
    }

    for (auto it : function->catchInfo()) {
        labels[it.m_tryStart] = nullptr;
        labels[it.m_catchStartPosition] = nullptr;
    }

    std::map<size_t, Label*>::iterator it;

    // Values needs to be modified.
    for (it = labels.begin(); it != labels.end(); it++) {
        it->second = new Label();
    }

    size_t nextTryBlock = compiler->tryBlocks().size();
    buildCatchInfo(compiler, function, labels);

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
        ByteCode::Opcode opcode = byteCode->opcode();
        Instruction::Group group = Instruction::Any;
        uint8_t paramCount = 0;
        uint16_t info = 0;

        switch (opcode) {
        // Binary operation
        case ByteCode::I32AddOpcode:
        case ByteCode::I32SubOpcode:
        case ByteCode::I32MulOpcode:
        case ByteCode::I32DivSOpcode:
        case ByteCode::I32DivUOpcode:
        case ByteCode::I32RemSOpcode:
        case ByteCode::I32RemUOpcode:
        case ByteCode::I32RotlOpcode:
        case ByteCode::I32RotrOpcode:
        case ByteCode::I32AndOpcode:
        case ByteCode::I32OrOpcode:
        case ByteCode::I32XorOpcode:
        case ByteCode::I32ShlOpcode:
        case ByteCode::I32ShrSOpcode:
        case ByteCode::I32ShrUOpcode: {
            info = Instruction::kIs32Bit;
            group = Instruction::Binary;
            paramCount = 2;
            break;
        }
        case ByteCode::I64AddOpcode:
        case ByteCode::I64SubOpcode:
        case ByteCode::I64MulOpcode:
        case ByteCode::I64DivSOpcode:
        case ByteCode::I64DivUOpcode:
        case ByteCode::I64RemSOpcode:
        case ByteCode::I64RemUOpcode:
        case ByteCode::I64RotlOpcode:
        case ByteCode::I64RotrOpcode:
        case ByteCode::I64AndOpcode:
        case ByteCode::I64OrOpcode:
        case ByteCode::I64XorOpcode:
        case ByteCode::I64ShlOpcode:
        case ByteCode::I64ShrSOpcode:
        case ByteCode::I64ShrUOpcode: {
            group = Instruction::Binary;
            paramCount = 2;
            break;
        }
        case ByteCode::I32EqOpcode:
        case ByteCode::I32NeOpcode:
        case ByteCode::I32LtSOpcode:
        case ByteCode::I32LtUOpcode:
        case ByteCode::I32GtSOpcode:
        case ByteCode::I32GtUOpcode:
        case ByteCode::I32LeSOpcode:
        case ByteCode::I32LeUOpcode:
        case ByteCode::I32GeSOpcode:
        case ByteCode::I32GeUOpcode: {
            info = Instruction::kIs32Bit;
            group = Instruction::Compare;
            paramCount = 2;
            break;
        }
        case ByteCode::I64EqOpcode:
        case ByteCode::I64NeOpcode:
        case ByteCode::I64LtSOpcode:
        case ByteCode::I64LtUOpcode:
        case ByteCode::I64GtSOpcode:
        case ByteCode::I64GtUOpcode:
        case ByteCode::I64LeSOpcode:
        case ByteCode::I64LeUOpcode:
        case ByteCode::I64GeSOpcode:
        case ByteCode::I64GeUOpcode: {
            group = Instruction::Compare;
            paramCount = 2;
            break;
        }
        case ByteCode::F32AddOpcode:
        case ByteCode::F32SubOpcode:
        case ByteCode::F32MulOpcode:
        case ByteCode::F32DivOpcode:
        case ByteCode::F32MaxOpcode:
        case ByteCode::F32MinOpcode:
        case ByteCode::F32CopysignOpcode:
        case ByteCode::F64AddOpcode:
        case ByteCode::F64SubOpcode:
        case ByteCode::F64MulOpcode:
        case ByteCode::F64DivOpcode:
        case ByteCode::F64MaxOpcode:
        case ByteCode::F64MinOpcode:
        case ByteCode::F64CopysignOpcode: {
            group = Instruction::BinaryFloat;
            paramCount = 2;
            break;
        }
        case ByteCode::F32EqOpcode:
        case ByteCode::F32NeOpcode:
        case ByteCode::F32LtOpcode:
        case ByteCode::F32GtOpcode:
        case ByteCode::F32LeOpcode:
        case ByteCode::F32GeOpcode:
        case ByteCode::F64EqOpcode:
        case ByteCode::F64NeOpcode:
        case ByteCode::F64LtOpcode:
        case ByteCode::F64GtOpcode:
        case ByteCode::F64LeOpcode:
        case ByteCode::F64GeOpcode: {
            group = Instruction::CompareFloat;
            paramCount = 2;
            break;
        }
        case ByteCode::I32ClzOpcode:
        case ByteCode::I32CtzOpcode:
        case ByteCode::I32PopcntOpcode:
        case ByteCode::I32Extend8SOpcode:
        case ByteCode::I32Extend16SOpcode: {
            group = Instruction::Unary;
            paramCount = 1;
            info = Instruction::kIs32Bit;
            break;
        }
        case ByteCode::I64ClzOpcode:
        case ByteCode::I64CtzOpcode:
        case ByteCode::I64PopcntOpcode:
        case ByteCode::I64Extend8SOpcode:
        case ByteCode::I64Extend16SOpcode:
        case ByteCode::I64Extend32SOpcode: {
            group = Instruction::Unary;
            paramCount = 1;
            break;
        }
        case ByteCode::F32CeilOpcode:
        case ByteCode::F32FloorOpcode:
        case ByteCode::F32TruncOpcode:
        case ByteCode::F32NearestOpcode:
        case ByteCode::F32SqrtOpcode:
        case ByteCode::F32AbsOpcode:
        case ByteCode::F32NegOpcode:
        case ByteCode::F64CeilOpcode:
        case ByteCode::F64FloorOpcode:
        case ByteCode::F64TruncOpcode:
        case ByteCode::F64NearestOpcode:
        case ByteCode::F64SqrtOpcode:
        case ByteCode::F64AbsOpcode:
        case ByteCode::F64NegOpcode:
        case ByteCode::F32DemoteF64Opcode:
        case ByteCode::F64PromoteF32Opcode: {
            group = Instruction::UnaryFloat;
            paramCount = 1;
            break;
        }
        case ByteCode::I32EqzOpcode: {
            group = Instruction::Compare;
            paramCount = 1;
            info = Instruction::kIs32Bit;
            break;
        }
        case ByteCode::I64EqzOpcode: {
            group = Instruction::Compare;
            paramCount = 1;
            break;
        }
        case ByteCode::I32WrapI64Opcode:
        case ByteCode::I64ExtendI32SOpcode:
        case ByteCode::I64ExtendI32UOpcode: {
            group = Instruction::Convert;
            paramCount = 1;
            break;
        }
        case ByteCode::I32TruncF32SOpcode:
        case ByteCode::I32TruncF32UOpcode:
        case ByteCode::I32TruncF64SOpcode:
        case ByteCode::I32TruncF64UOpcode:
        case ByteCode::I64TruncF32SOpcode:
        case ByteCode::I64TruncF32UOpcode:
        case ByteCode::I64TruncF64SOpcode:
        case ByteCode::I64TruncF64UOpcode:
        case ByteCode::I32TruncSatF32SOpcode:
        case ByteCode::I32TruncSatF32UOpcode:
        case ByteCode::I32TruncSatF64SOpcode:
        case ByteCode::I32TruncSatF64UOpcode:
        case ByteCode::I64TruncSatF32SOpcode:
        case ByteCode::I64TruncSatF32UOpcode:
        case ByteCode::I64TruncSatF64SOpcode:
        case ByteCode::I64TruncSatF64UOpcode:
        case ByteCode::F32ConvertI32SOpcode:
        case ByteCode::F32ConvertI32UOpcode:
        case ByteCode::F32ConvertI64SOpcode:
        case ByteCode::F32ConvertI64UOpcode:
        case ByteCode::F64ConvertI32SOpcode:
        case ByteCode::F64ConvertI32UOpcode:
        case ByteCode::F64ConvertI64SOpcode:
        case ByteCode::F64ConvertI64UOpcode: {
            group = Instruction::ConvertFloat;
            paramCount = 1;
            break;
        }
        case ByteCode::SelectOpcode: {
            auto instr = compiler->append(byteCode, group, opcode, 3, 1);
            auto select = reinterpret_cast<Select*>(byteCode);
            auto operands = instr->operands();

            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(select->src0Offset());
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(select->src1Offset());
            operands[2].item = nullptr;
            operands[2].offset = STACK_OFFSET(select->condOffset());
            operands[3].item = nullptr;
            operands[3].offset = STACK_OFFSET(select->dstOffset());
            break;
        }
        case ByteCode::CallOpcode:
        case ByteCode::CallIndirectOpcode: {
            FunctionType* functionType;
            ByteCodeStackOffset* stackOffset;
            uint32_t callerCount;

            if (opcode == ByteCode::CallOpcode) {
                Call* call = reinterpret_cast<Call*>(byteCode);
                functionType = module->function(call->index())->functionType();
                stackOffset = call->stackOffsets();
                callerCount = 0;
            } else {
                CallIndirect* callIndirect = reinterpret_cast<CallIndirect*>(byteCode);
                functionType = callIndirect->functionType();
                stackOffset = callIndirect->stackOffsets();
                callerCount = 1;
            }

            uint32_t paramCount = functionType->param().size();
            uint32_t resultCount = functionType->result().size();

            Instruction* instr = compiler->appendExtended(byteCode, Instruction::Call,
                                                          opcode, paramCount + callerCount, resultCount);
            Operand* operand = instr->operands();
            Operand* end = operand + paramCount;

            while (operand < end) {
                operand->item = nullptr;
                operand->offset = STACK_OFFSET(*stackOffset);
                operand++;
                stackOffset++;
            }

            if (opcode == ByteCode::CallIndirectOpcode) {
                operand->item = nullptr;
                operand->offset = STACK_OFFSET(reinterpret_cast<CallIndirect*>(byteCode)->calleeOffset());
                operand++;
            }

            end = operand + resultCount;

            while (operand < end) {
                operand->item = nullptr;
                operand->offset = STACK_OFFSET(*stackOffset);
                operand++;
                stackOffset++;
            }
            break;
        }
        case ByteCode::ThrowOpcode: {
            Throw* throwTag = reinterpret_cast<Throw*>(byteCode);
            TagType* tagType = module->tagType(throwTag->tagIndex());
            uint32_t size = module->functionType(tagType->sigIndex())->param().size();

            Instruction* instr = compiler->append(byteCode, Instruction::Any, opcode, size, 0);
            Operand* param = instr->params();
            Operand* end = param + size;
            ByteCodeStackOffset* offsets = throwTag->dataOffsets();

            while (param < end) {
                param->item = nullptr;
                param->offset = STACK_OFFSET(*offsets++);
                param++;
            }
            break;
        }
        case ByteCode::Load32Opcode: {
            Load32* load32 = reinterpret_cast<Load32*>(byteCode);
            Instruction* instr = compiler->append(byteCode, Instruction::Load, opcode, 1, 1);

            Operand* operands = instr->operands();
            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(load32->srcOffset());
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(load32->dstOffset());
            break;
        }
        case ByteCode::Load64Opcode: {
            Load64* load64 = reinterpret_cast<Load64*>(byteCode);
            Instruction* instr = compiler->append(byteCode, Instruction::Load, opcode, 1, 1);

            Operand* operands = instr->operands();
            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(load64->srcOffset());
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(load64->dstOffset());
            break;
        }
        case ByteCode::I32LoadOpcode:
        case ByteCode::I32Load8SOpcode:
        case ByteCode::I32Load8UOpcode:
        case ByteCode::I32Load16SOpcode:
        case ByteCode::I32Load16UOpcode:
        case ByteCode::I64LoadOpcode:
        case ByteCode::I64Load8SOpcode:
        case ByteCode::I64Load8UOpcode:
        case ByteCode::I64Load16SOpcode:
        case ByteCode::I64Load16UOpcode:
        case ByteCode::I64Load32SOpcode:
        case ByteCode::I64Load32UOpcode:
        case ByteCode::F32LoadOpcode:
        case ByteCode::F64LoadOpcode:
        case ByteCode::V128LoadOpcode:
        case ByteCode::V128Load8SplatOpcode:
        case ByteCode::V128Load16SplatOpcode:
        case ByteCode::V128Load32SplatOpcode:
        case ByteCode::V128Load64SplatOpcode:
        case ByteCode::V128Load8X8SOpcode:
        case ByteCode::V128Load8X8UOpcode:
        case ByteCode::V128Load16X4SOpcode:
        case ByteCode::V128Load16X4UOpcode:
        case ByteCode::V128Load32X2SOpcode:
        case ByteCode::V128Load32X2UOpcode:
        case ByteCode::V128Load32ZeroOpcode:
        case ByteCode::V128Load64ZeroOpcode: {
            MemoryLoad* loadOperation = reinterpret_cast<MemoryLoad*>(byteCode);
            Instruction* instr = compiler->append(byteCode, Instruction::Load, opcode, 1, 1);

            Operand* operands = instr->operands();
            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(loadOperation->srcOffset());
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(loadOperation->dstOffset());
            break;
        }
        case ByteCode::V128Load8LaneOpcode:
        case ByteCode::V128Load16LaneOpcode:
        case ByteCode::V128Load32LaneOpcode:
        case ByteCode::V128Load64LaneOpcode: {
            SIMDMemoryLoad* loadOperation = reinterpret_cast<SIMDMemoryLoad*>(byteCode);
            Instruction* instr = compiler->append(byteCode, Instruction::LoadLaneSIMD, opcode, 2, 1);

            Operand* operands = instr->operands();
            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(loadOperation->src0Offset());
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(loadOperation->src1Offset());
            operands[2].item = nullptr;
            operands[2].offset = STACK_OFFSET(loadOperation->dstOffset());
            break;
        }
        case ByteCode::Store32Opcode: {
            Store32* store32 = reinterpret_cast<Store32*>(byteCode);
            Instruction* instr = compiler->append(byteCode, Instruction::Store, opcode, 2, 0);

            Operand* operands = instr->operands();
            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(store32->src0Offset());
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(store32->src1Offset());
            break;
        }
        case ByteCode::Store64Opcode: {
            Store64* store64 = reinterpret_cast<Store64*>(byteCode);
            Instruction* instr = compiler->append(byteCode, Instruction::Store, opcode, 2, 0);

            Operand* operands = instr->operands();
            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(store64->src0Offset());
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(store64->src1Offset());
            break;
        }
        case ByteCode::I32StoreOpcode:
        case ByteCode::I32Store8Opcode:
        case ByteCode::I32Store16Opcode:
        case ByteCode::I64StoreOpcode:
        case ByteCode::I64Store8Opcode:
        case ByteCode::I64Store16Opcode:
        case ByteCode::I64Store32Opcode:
        case ByteCode::F32StoreOpcode:
        case ByteCode::F64StoreOpcode:
        case ByteCode::V128StoreOpcode: {
            MemoryStore* storeOperation = reinterpret_cast<MemoryStore*>(byteCode);
            Instruction* instr = compiler->append(byteCode, Instruction::Store, opcode, 2, 0);

            Operand* operands = instr->operands();
            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(storeOperation->src0Offset());
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(storeOperation->src1Offset());
            break;
        }
        case ByteCode::V128Store8LaneOpcode:
        case ByteCode::V128Store16LaneOpcode:
        case ByteCode::V128Store32LaneOpcode:
        case ByteCode::V128Store64LaneOpcode: {
            SIMDMemoryStore* storeOperation = reinterpret_cast<SIMDMemoryStore*>(byteCode);
            Instruction* instr = compiler->append(byteCode, Instruction::Store, opcode, 2, 0);

            Operand* operands = instr->operands();
            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(storeOperation->src0Offset());
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(storeOperation->src1Offset());
            break;
        }
        case ByteCode::I8X16ExtractLaneSOpcode:
        case ByteCode::I8X16ExtractLaneUOpcode:
        case ByteCode::I16X8ExtractLaneSOpcode:
        case ByteCode::I16X8ExtractLaneUOpcode:
        case ByteCode::I32X4ExtractLaneOpcode:
        case ByteCode::I64X2ExtractLaneOpcode:
        case ByteCode::F32X4ExtractLaneOpcode:
        case ByteCode::F64X2ExtractLaneOpcode: {
            SIMDExtractLane* extractLane = reinterpret_cast<SIMDExtractLane*>(byteCode);
            Instruction* instr = compiler->append(byteCode, Instruction::ExtractLaneSIMD, opcode, 2, 0);

            Operand* operands = instr->operands();
            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(extractLane->srcOffset());
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(extractLane->dstOffset());
            break;
        }
        case ByteCode::I8X16ReplaceLaneOpcode:
        case ByteCode::I16X8ReplaceLaneOpcode:
        case ByteCode::I32X4ReplaceLaneOpcode:
        case ByteCode::I64X2ReplaceLaneOpcode:
        case ByteCode::F32X4ReplaceLaneOpcode:
        case ByteCode::F64X2ReplaceLaneOpcode: {
            SIMDReplaceLane* replaceLane = reinterpret_cast<SIMDReplaceLane*>(byteCode);
            Instruction* instr = compiler->append(byteCode, Instruction::ReplaceLaneSIMD, opcode, 2, 1);

            Operand* operands = instr->operands();
            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(replaceLane->srcOffsets()[0]);
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(replaceLane->srcOffsets()[1]);
            operands[2].item = nullptr;
            operands[2].offset = STACK_OFFSET(replaceLane->dstOffset());
            break;
        }
        case ByteCode::I8X16SplatOpcode:
        case ByteCode::I16X8SplatOpcode:
        case ByteCode::I32X4SplatOpcode:
        case ByteCode::I64X2SplatOpcode:
        case ByteCode::F32X4SplatOpcode:
        case ByteCode::F64X2SplatOpcode: {
            group = Instruction::SplatSIMD;
            paramCount = 1;
            break;
        }
        case ByteCode::TableInitOpcode: {
            auto tableInit = reinterpret_cast<TableInit*>(byteCode);

            Instruction* instr = compiler->append(byteCode, Instruction::Table, opcode, 3, 0);

            Operand* operands = instr->operands();
            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(tableInit->srcOffsets()[0]);
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(tableInit->srcOffsets()[1]);
            operands[2].item = nullptr;
            operands[2].offset = STACK_OFFSET(tableInit->srcOffsets()[2]);
            break;
        }
        case ByteCode::TableSizeOpcode: {
            auto tableSize = reinterpret_cast<TableSize*>(byteCode);

            Instruction* instr = compiler->append(byteCode, Instruction::Table, opcode, 0, 1);

            Operand* operands = instr->operands();
            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(tableSize->dstOffset());
            break;
        }
        case ByteCode::TableCopyOpcode: {
            auto tableCopy = reinterpret_cast<TableCopy*>(byteCode);

            Instruction* instr = compiler->append(byteCode, Instruction::Table, opcode, 3, 0);

            Operand* operands = instr->operands();
            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(tableCopy->srcOffsets()[0]);
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(tableCopy->srcOffsets()[1]);
            operands[2].item = nullptr;
            operands[2].offset = STACK_OFFSET(tableCopy->srcOffsets()[2]);
            break;
        }
        case ByteCode::TableFillOpcode: {
            auto tableFill = reinterpret_cast<TableFill*>(byteCode);

            Instruction* instr = compiler->append(byteCode, Instruction::Table, opcode, 3, 0);

            Operand* operands = instr->operands();
            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(tableFill->srcOffsets()[0]);
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(tableFill->srcOffsets()[1]);
            operands[2].item = nullptr;
            operands[2].offset = STACK_OFFSET(tableFill->srcOffsets()[2]);
            break;
        }
        case ByteCode::TableGrowOpcode: {
            auto tableGrow = reinterpret_cast<TableGrow*>(byteCode);

            Instruction* instr = compiler->append(byteCode, Instruction::Table, opcode, 2, 1);

            Operand* operands = instr->operands();
            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(tableGrow->src0Offset());
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(tableGrow->src1Offset());
            operands[2].item = nullptr;
            operands[2].offset = STACK_OFFSET(tableGrow->dstOffset());
            break;
        }
        case ByteCode::TableSetOpcode: {
            auto tableSet = reinterpret_cast<TableSet*>(byteCode);

            Instruction* instr = compiler->append(byteCode, Instruction::Table, opcode, 2, 0);

            Operand* operands = instr->operands();
            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(tableSet->src0Offset());
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(tableSet->src1Offset());
            break;
        }
        case ByteCode::TableGetOpcode: {
            auto tableGet = reinterpret_cast<TableGet*>(byteCode);

            Instruction* instr = compiler->append(byteCode, Instruction::Table, opcode, 1, 1);

            Operand* operands = instr->operands();
            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(tableGet->srcOffset());
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(tableGet->dstOffset());
            break;
        }
        case ByteCode::MemorySizeOpcode: {
            MemorySize* memorySize = reinterpret_cast<MemorySize*>(byteCode);

            Instruction* instr = compiler->append(byteCode, Instruction::Memory, opcode, 0, 1);

            Operand* operands = instr->operands();
            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(memorySize->dstOffset());
            break;
        }
        case ByteCode::MemoryInitOpcode: {
            MemoryInit* memoryInit = reinterpret_cast<MemoryInit*>(byteCode);

            Instruction* instr = compiler->append(byteCode, Instruction::Memory, opcode, 3, 0);

            Operand* operands = instr->operands();
            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(memoryInit->srcOffsets()[0]);
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(memoryInit->srcOffsets()[1]);
            operands[2].item = nullptr;
            operands[2].offset = STACK_OFFSET(memoryInit->srcOffsets()[2]);
            break;
        }
        case ByteCode::MemoryCopyOpcode: {
            MemoryCopy* memoryCopy = reinterpret_cast<MemoryCopy*>(byteCode);

            Instruction* instr = compiler->append(byteCode, Instruction::Memory, opcode, 3, 0);

            Operand* operands = instr->operands();
            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(memoryCopy->srcOffsets()[0]);
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(memoryCopy->srcOffsets()[1]);
            operands[2].item = nullptr;
            operands[2].offset = STACK_OFFSET(memoryCopy->srcOffsets()[2]);
            break;
        }
        case ByteCode::MemoryFillOpcode: {
            MemoryFill* memoryFill = reinterpret_cast<MemoryFill*>(byteCode);

            Instruction* instr = compiler->append(byteCode, Instruction::Memory, opcode, 3, 0);

            Operand* operands = instr->operands();
            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(memoryFill->srcOffsets()[0]);
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(memoryFill->srcOffsets()[1]);
            operands[2].item = nullptr;
            operands[2].offset = STACK_OFFSET(memoryFill->srcOffsets()[2]);
            break;
        }
        case ByteCode::MemoryGrowOpcode: {
            MemoryGrow* memoryGrow = reinterpret_cast<MemoryGrow*>(byteCode);

            Instruction* instr = compiler->append(byteCode, Instruction::Memory, opcode, 1, 1);

            Operand* operands = instr->operands();
            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(memoryGrow->srcOffset());
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(memoryGrow->dstOffset());
            break;
        }
        case ByteCode::DataDropOpcode:
        case ByteCode::ElemDropOpcode:
        case ByteCode::UnreachableOpcode: {
            compiler->append(byteCode, group, opcode, 0, 0);
            break;
        }
        case ByteCode::JumpOpcode: {
            Jump* jump = reinterpret_cast<Jump*>(byteCode);
            compiler->appendBranch(jump, opcode, labels[COMPUTE_OFFSET(idx, jump->offset())], 0);
            break;
        }
        case ByteCode::JumpIfTrueOpcode: {
            JumpIfTrue* jumpIfTrue = reinterpret_cast<JumpIfTrue*>(byteCode);
            compiler->appendBranch(jumpIfTrue, opcode, labels[COMPUTE_OFFSET(idx, jumpIfTrue->offset())], STACK_OFFSET(jumpIfTrue->srcOffset()));
            break;
        }
        case ByteCode::JumpIfFalseOpcode: {
            JumpIfFalse* jumpIfFalse = reinterpret_cast<JumpIfFalse*>(byteCode);
            compiler->appendBranch(jumpIfFalse, opcode, labels[COMPUTE_OFFSET(idx, jumpIfFalse->offset())], STACK_OFFSET(jumpIfFalse->srcOffset()));
            break;
        }
        case ByteCode::BrTableOpcode: {
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
        case ByteCode::Const32Opcode: {
            Instruction* instr = compiler->append(byteCode, Instruction::Immediate, ByteCode::Const32Opcode, 0, 1);

            Const32* const32 = reinterpret_cast<Const32*>(byteCode);
            Operand* operands = instr->operands();

            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(const32->dstOffset());
            break;
        }
        case ByteCode::Const64Opcode: {
            Instruction* instr = compiler->append(byteCode, Instruction::Immediate, ByteCode::Const64Opcode, 0, 1);

            Const64* const64 = reinterpret_cast<Const64*>(byteCode);
            Operand* operands = instr->operands();

            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(const64->dstOffset());
            break;
        }
        case ByteCode::Move32Opcode: {
            Instruction* instr = compiler->append(byteCode, Instruction::Move, ByteCode::Move32Opcode, 1, 1);

            Move32* move32 = reinterpret_cast<Move32*>(byteCode);
            Operand* operands = instr->operands();

            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(move32->srcOffset());
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(move32->dstOffset());
            break;
        }
        case ByteCode::Move64Opcode: {
            Instruction* instr = compiler->append(byteCode, Instruction::Move, ByteCode::Move64Opcode, 1, 1);

            Move64* move64 = reinterpret_cast<Move64*>(byteCode);
            Operand* operands = instr->operands();

            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(move64->srcOffset());
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(move64->dstOffset());
            break;
        }
        case ByteCode::GlobalGet32Opcode: {
            Instruction* instr = compiler->append(byteCode, Instruction::Any, ByteCode::GlobalGet32Opcode, 0, 1);

            GlobalGet32* globalGet32 = reinterpret_cast<GlobalGet32*>(byteCode);
            Operand* operands = instr->operands();

            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(globalGet32->dstOffset());
            break;
        }
        case ByteCode::GlobalGet64Opcode: {
            Instruction* instr = compiler->append(byteCode, Instruction::Any, ByteCode::GlobalGet64Opcode, 0, 1);

            GlobalGet64* globalGet64 = reinterpret_cast<GlobalGet64*>(byteCode);
            Operand* operands = instr->operands();

            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(globalGet64->dstOffset());
            break;
        }
        case ByteCode::GlobalGet128Opcode: {
            Instruction* instr = compiler->append(byteCode, Instruction::Any, ByteCode::GlobalGet128Opcode, 0, 1);

            GlobalGet128* globalGet128 = reinterpret_cast<GlobalGet128*>(byteCode);
            Operand* operands = instr->operands();

            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(globalGet128->dstOffset());
            break;
        }
        case ByteCode::GlobalSet32Opcode: {
            Instruction* instr = compiler->append(byteCode, Instruction::Any, ByteCode::GlobalSet32Opcode, 1, 0);

            GlobalSet32* globalSet32 = reinterpret_cast<GlobalSet32*>(byteCode);
            Operand* operands = instr->operands();

            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(globalSet32->srcOffset());
            break;
        }
        case ByteCode::GlobalSet64Opcode: {
            Instruction* instr = compiler->append(byteCode, Instruction::Any, ByteCode::GlobalSet64Opcode, 1, 0);

            GlobalSet64* globalSet64 = reinterpret_cast<GlobalSet64*>(byteCode);
            Operand* operands = instr->operands();

            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(globalSet64->srcOffset());
            break;
        }
        case ByteCode::GlobalSet128Opcode: {
            Instruction* instr = compiler->append(byteCode, Instruction::Any, ByteCode::GlobalSet128Opcode, 1, 0);

            GlobalSet128* globalSet128 = reinterpret_cast<GlobalSet128*>(byteCode);
            Operand* operands = instr->operands();

            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(globalSet128->srcOffset());
            break;
        }
        case ByteCode::RefFuncOpcode: {
            Instruction* instr = compiler->append(byteCode, Instruction::Any, ByteCode::RefFuncOpcode, 0, 1);

            auto refFunc = reinterpret_cast<RefFunc*>(byteCode);
            auto operands = instr->operands();

            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(refFunc->dstOffset());
            break;
        }
        case ByteCode::EndOpcode: {
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

            idx += byteCode->getSize();

            if (idx != endIdx) {
                instr->addInfo(Instruction::kEarlyReturn);
            }

            continue;
        }
        /* SIMD support. */
        case ByteCode::Const128Opcode: {
            Instruction* instr = compiler->append(byteCode, Instruction::Immediate, ByteCode::Const128Opcode, 0, 1);

            Const128* const128 = reinterpret_cast<Const128*>(byteCode);
            Operand* operands = instr->operands();

            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(const128->dstOffset());
            break;
        }
        case ByteCode::I8X16SubOpcode:
        case ByteCode::I8X16AddOpcode:
        case ByteCode::I8X16AddSatSOpcode:
        case ByteCode::I8X16AddSatUOpcode:
        case ByteCode::I8X16SubSatSOpcode:
        case ByteCode::I8X16SubSatUOpcode:
        case ByteCode::I8X16EqOpcode:
        case ByteCode::I8X16NeOpcode:
        case ByteCode::I8X16LtSOpcode:
        case ByteCode::I8X16LtUOpcode:
        case ByteCode::I8X16LeSOpcode:
        case ByteCode::I8X16LeUOpcode:
        case ByteCode::I8X16GtSOpcode:
        case ByteCode::I8X16GtUOpcode:
        case ByteCode::I8X16GeSOpcode:
        case ByteCode::I8X16GeUOpcode:
        case ByteCode::I8X16NarrowI16X8SOpcode:
        case ByteCode::I8X16NarrowI16X8UOpcode:
        case ByteCode::I16X8AddOpcode:
        case ByteCode::I16X8SubOpcode:
        case ByteCode::I16X8MulOpcode:
        case ByteCode::I16X8AddSatSOpcode:
        case ByteCode::I16X8AddSatUOpcode:
        case ByteCode::I16X8SubSatSOpcode:
        case ByteCode::I16X8SubSatUOpcode:
        case ByteCode::I16X8EqOpcode:
        case ByteCode::I16X8NeOpcode:
        case ByteCode::I16X8LtSOpcode:
        case ByteCode::I16X8LtUOpcode:
        case ByteCode::I16X8LeSOpcode:
        case ByteCode::I16X8LeUOpcode:
        case ByteCode::I16X8GtSOpcode:
        case ByteCode::I16X8GtUOpcode:
        case ByteCode::I16X8GeSOpcode:
        case ByteCode::I16X8GeUOpcode:
        case ByteCode::I16X8ExtmulLowI8X16SOpcode:
        case ByteCode::I16X8ExtmulHighI8X16SOpcode:
        case ByteCode::I16X8ExtmulLowI8X16UOpcode:
        case ByteCode::I16X8ExtmulHighI8X16UOpcode:
        case ByteCode::I16X8NarrowI32X4SOpcode:
        case ByteCode::I16X8NarrowI32X4UOpcode:
        case ByteCode::I32X4AddOpcode:
        case ByteCode::I32X4SubOpcode:
        case ByteCode::I32X4MulOpcode:
        case ByteCode::I32X4EqOpcode:
        case ByteCode::I32X4NeOpcode:
        case ByteCode::I32X4LtSOpcode:
        case ByteCode::I32X4LtUOpcode:
        case ByteCode::I32X4LeSOpcode:
        case ByteCode::I32X4LeUOpcode:
        case ByteCode::I32X4GtSOpcode:
        case ByteCode::I32X4GtUOpcode:
        case ByteCode::I32X4GeSOpcode:
        case ByteCode::I32X4GeUOpcode:
        case ByteCode::I32X4ExtmulLowI16X8SOpcode:
        case ByteCode::I32X4ExtmulHighI16X8SOpcode:
        case ByteCode::I32X4ExtmulLowI16X8UOpcode:
        case ByteCode::I32X4ExtmulHighI16X8UOpcode:
        case ByteCode::I64X2AddOpcode:
        case ByteCode::I64X2SubOpcode:
        case ByteCode::I64X2MulOpcode:
        case ByteCode::I64X2EqOpcode:
        case ByteCode::I64X2NeOpcode:
        case ByteCode::I64X2LtSOpcode:
        case ByteCode::I64X2LeSOpcode:
        case ByteCode::I64X2GtSOpcode:
        case ByteCode::I64X2GeSOpcode:
        case ByteCode::I64X2ExtmulLowI32X4SOpcode:
        case ByteCode::I64X2ExtmulHighI32X4SOpcode:
        case ByteCode::I64X2ExtmulLowI32X4UOpcode:
        case ByteCode::I64X2ExtmulHighI32X4UOpcode:
        case ByteCode::F32X4EqOpcode:
        case ByteCode::F32X4NeOpcode:
        case ByteCode::F32X4LtOpcode:
        case ByteCode::F32X4LeOpcode:
        case ByteCode::F32X4GtOpcode:
        case ByteCode::F32X4GeOpcode:
        case ByteCode::F32X4AddOpcode:
        case ByteCode::F32X4DivOpcode:
        case ByteCode::F32X4MaxOpcode:
        case ByteCode::F32X4MinOpcode:
        case ByteCode::F32X4MulOpcode:
        case ByteCode::F32X4PMaxOpcode:
        case ByteCode::F32X4PMinOpcode:
        case ByteCode::F32X4SubOpcode:
        case ByteCode::F64X2AddOpcode:
        case ByteCode::F64X2DivOpcode:
        case ByteCode::F64X2MaxOpcode:
        case ByteCode::F64X2MinOpcode:
        case ByteCode::F64X2MulOpcode:
        case ByteCode::F64X2PMaxOpcode:
        case ByteCode::F64X2PMinOpcode:
        case ByteCode::F64X2SubOpcode:
        case ByteCode::F64X2EqOpcode:
        case ByteCode::F64X2NeOpcode:
        case ByteCode::F64X2LtOpcode:
        case ByteCode::F64X2LeOpcode:
        case ByteCode::F64X2GtOpcode:
        case ByteCode::F64X2GeOpcode:
        case ByteCode::V128AndOpcode:
        case ByteCode::V128OrOpcode:
        case ByteCode::V128XorOpcode:
        case ByteCode::V128AndnotOpcode:
        case ByteCode::I8X16SwizzleOpcode: {
            group = Instruction::BinarySIMD;
            paramCount = 2;
            break;
        }
        case ByteCode::I8X16NegOpcode:
        case ByteCode::I16X8NegOpcode:
        case ByteCode::I16X8ExtaddPairwiseI8X16SOpcode:
        case ByteCode::I16X8ExtaddPairwiseI8X16UOpcode:
        case ByteCode::I16X8ExtendLowI8X16SOpcode:
        case ByteCode::I16X8ExtendHighI8X16SOpcode:
        case ByteCode::I16X8ExtendLowI8X16UOpcode:
        case ByteCode::I16X8ExtendHighI8X16UOpcode:
        case ByteCode::I32X4NegOpcode:
        case ByteCode::I32X4ExtaddPairwiseI16X8SOpcode:
        case ByteCode::I32X4ExtaddPairwiseI16X8UOpcode:
        case ByteCode::I32X4ExtendLowI16X8SOpcode:
        case ByteCode::I32X4ExtendHighI16X8SOpcode:
        case ByteCode::I32X4ExtendLowI16X8UOpcode:
        case ByteCode::I32X4ExtendHighI16X8UOpcode:
        case ByteCode::I64X2NegOpcode:
        case ByteCode::F32X4AbsOpcode:
        case ByteCode::F32X4CeilOpcode:
        case ByteCode::F32X4FloorOpcode:
        case ByteCode::F32X4NearestOpcode:
        case ByteCode::F32X4NegOpcode:
        case ByteCode::F32X4SqrtOpcode:
        case ByteCode::F32X4TruncOpcode:
        case ByteCode::F32X4DemoteF64X2ZeroOpcode:
        case ByteCode::F32X4ConvertI32X4SOpcode:
        case ByteCode::F32X4ConvertI32X4UOpcode:
        case ByteCode::F64X2AbsOpcode:
        case ByteCode::F64X2CeilOpcode:
        case ByteCode::F64X2FloorOpcode:
        case ByteCode::F64X2NearestOpcode:
        case ByteCode::F64X2NegOpcode:
        case ByteCode::F64X2SqrtOpcode:
        case ByteCode::F64X2TruncOpcode:
        case ByteCode::F64X2PromoteLowF32X4Opcode:
        case ByteCode::F64X2ConvertLowI32X4SOpcode:
        case ByteCode::F64X2ConvertLowI32X4UOpcode:
        case ByteCode::V128NotOpcode: {
            group = Instruction::UnarySIMD;
            paramCount = 1;
            break;
        }
        case ByteCode::I8X16ShlOpcode:
        case ByteCode::I8X16ShrSOpcode:
        case ByteCode::I8X16ShrUOpcode:
        case ByteCode::I16X8ShlOpcode:
        case ByteCode::I16X8ShrSOpcode:
        case ByteCode::I16X8ShrUOpcode:
        case ByteCode::I32X4ShlOpcode:
        case ByteCode::I32X4ShrSOpcode:
        case ByteCode::I32X4ShrUOpcode:
        case ByteCode::I64X2ShlOpcode:
        case ByteCode::I64X2ShrSOpcode:
        case ByteCode::I64X2ShrUOpcode: {
            group = Instruction::ShiftSIMD;
            paramCount = 2;
            break;
        }
        case ByteCode::V128BitSelectOpcode: {
            Instruction* instr = compiler->append(byteCode, Instruction::Any, opcode, 3, 1);

            V128BitSelect* bitSelect = reinterpret_cast<V128BitSelect*>(byteCode);
            Operand* operands = instr->operands();

            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(bitSelect->srcOffsets()[0]);
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(bitSelect->srcOffsets()[1]);
            operands[2].item = nullptr;
            operands[2].offset = STACK_OFFSET(bitSelect->srcOffsets()[2]);
            operands[3].item = nullptr;
            operands[3].offset = STACK_OFFSET(bitSelect->dstOffset());
            break;
        }
        case ByteCode::I8X16ShuffleOpcode: {
            Instruction* instr = compiler->append(byteCode, Instruction::Any, opcode, 2, 1);

            I8X16Shuffle* shuffle = reinterpret_cast<I8X16Shuffle*>(byteCode);
            Operand* operands = instr->operands();

            operands[0].item = nullptr;
            operands[0].offset = STACK_OFFSET(shuffle->srcOffsets()[0]);
            operands[1].item = nullptr;
            operands[1].offset = STACK_OFFSET(shuffle->srcOffsets()[1]);
            operands[2].item = nullptr;
            operands[2].offset = STACK_OFFSET(shuffle->dstOffset());
            break;
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

        idx += byteCode->getSize();
    }

    compiler->buildParamDependencies(STACK_OFFSET(function->requiredStackSize()), nextTryBlock);

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

    if (functionCount == 0) {
        return;
    }

    JITCompiler compiler(this, verboseLevel);

    for (size_t i = 0; i < functionCount; i++) {
        if (verboseLevel >= 1) {
            printf("[[[[[[[  Function %3d  ]]]]]]]\n", static_cast<int>(i));
        }

        createInstructionList(&compiler, m_functions[i], this);
    }

    m_jitModule = compiler.compile();
}

} // namespace Walrus
