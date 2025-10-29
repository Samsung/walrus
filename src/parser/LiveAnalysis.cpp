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
#include "interpreter/ByteCode.h"
#include "runtime/Module.h"
#include "runtime/Value.h"
#include <cstdint>
#include "parser/LiveAnalysis.h"

namespace wabt {
void LiveAnalysis::assignBasicBlocks(Walrus::ByteCode* code, std::vector<LiveAnalysis::BasicBlock>& basicBlocks, size_t byteCodeOffset)
{
    switch (code->opcode()) {
    case Walrus::ByteCode::JumpOpcode: {
        Walrus::Jump* jump = reinterpret_cast<Walrus::Jump*>(code);
        basicBlocks.push_back(LiveAnalysis::BasicBlock(byteCodeOffset, byteCodeOffset + jump->offset()));
        break;
    }
    case Walrus::ByteCode::JumpIfTrueOpcode:
    case Walrus::ByteCode::JumpIfFalseOpcode:
    case Walrus::ByteCode::JumpIfNullOpcode:
    case Walrus::ByteCode::JumpIfNonNullOpcode:
    case Walrus::ByteCode::JumpIfCastGenericOpcode:
    case Walrus::ByteCode::JumpIfCastDefinedOpcode: {
        Walrus::ByteCodeOffsetValue* jumpIf = reinterpret_cast<Walrus::ByteCodeOffsetValue*>(code);
        basicBlocks.push_back(LiveAnalysis::BasicBlock(byteCodeOffset, byteCodeOffset + jumpIf->int32Value()));
        break;
    }
    case Walrus::ByteCode::BrTableOpcode: {
        Walrus::BrTable* table = reinterpret_cast<Walrus::BrTable*>(code);
        basicBlocks.push_back(LiveAnalysis::BasicBlock(byteCodeOffset, byteCodeOffset + table->defaultOffset()));
        for (size_t i = 0; i < table->tableSize(); i++) {
            if (byteCodeOffset + table->jumpOffsets()[i]) {
                basicBlocks.push_back(LiveAnalysis::BasicBlock(byteCodeOffset, byteCodeOffset + table->jumpOffsets()[i]));
            }
        }
        break;
    }
    default: {
        return;
    }
    }
}

void LiveAnalysis::orderInsAndOuts(std::vector<LiveAnalysis::BasicBlock>& basicBlocks, std::vector<VariableRange>& ranges, size_t end, size_t position)
{
    size_t currentBlockIdx = 0;
    while (position < end && currentBlockIdx < basicBlocks.size()) {
        uint16_t blockStart = basicBlocks[currentBlockIdx].from < basicBlocks[currentBlockIdx].to ? basicBlocks[currentBlockIdx].from : basicBlocks[currentBlockIdx].to;
        uint16_t blockEnd = basicBlocks[currentBlockIdx].from < basicBlocks[currentBlockIdx].to ? basicBlocks[currentBlockIdx].to : basicBlocks[currentBlockIdx].from;

        for (LiveAnalysis::VariableRange range : ranges) {
            if (blockStart < range.start && range.end < blockEnd) {
                continue;
            }

            // Forward jump case
            if (basicBlocks[currentBlockIdx].from < basicBlocks[currentBlockIdx].to) {
                if (range.start < basicBlocks[currentBlockIdx].from) {
                    basicBlocks[currentBlockIdx].in.push_back(range);
                }
                if (range.end > basicBlocks[currentBlockIdx].to) {
                    basicBlocks[currentBlockIdx].out.push_back(range);
                }
                // Backward jump case.
            } else {
                if (range.start < basicBlocks[currentBlockIdx].to) {
                    basicBlocks[currentBlockIdx].in.push_back(range);
                }
                if (range.end > basicBlocks[currentBlockIdx].from) {
                    basicBlocks[currentBlockIdx].out.push_back(range);
                }
            }
        }

        std::vector<LiveAnalysis::BasicBlock> insideBlocks;
        for (BasicBlock& block : basicBlocks) {
            uint16_t insideBlockStart = block.from < block.to ? block.from : block.to;
            uint16_t insideBlockEnd = block.from < block.to ? block.to : block.from;

            if (blockStart < insideBlockStart && insideBlockEnd < blockEnd) {
                insideBlocks.push_back(block);
            }
        }

        if (!insideBlocks.empty()) {
            orderInsAndOuts(insideBlocks, ranges, blockStart, blockEnd);
        }

        currentBlockIdx++;
        position = blockEnd;
    }
}

/*
void LiveAnalysis::orderInsAndOuts(std::vector<LiveAnalysis::BasicBlock>& basicBlocks, std::vector<VariableRange>& ranges, size_t end, size_t position)
{
    while (position < end) {
        auto block = std::lower_bound(basicBlocks.begin(),
                                      basicBlocks.end(),
                                      position,
                                      [](LiveAnalysis::BasicBlock& block, size_t position) {
                                          return block.from <= position;
                                      });

        if (block == basicBlocks.end()) {
            break;
        }

        for (LiveAnalysis::VariableRange range : ranges) {
            // Forward jump case.
            if (block->from < block->to) {
                if (range.start < block->from) {
                    block->in.push_back(range);
                }
                if (range.end > block->to) {
                    block->out.push_back(range);
                }
                // Backward jump case.
            } else {
                if (range.start < block->to) {
                    block->in.push_back(range);
                }
                if (range.end > block->from) {
                    block->out.push_back(range);
                }
            }
        }

        auto insideBlock = std::lower_bound(basicBlocks.begin(),
                                            basicBlocks.end(),
                                            block->from,
                                            [](LiveAnalysis::BasicBlock& block, size_t position) {
                                                return block.from <= position;
                                            });

        if (insideBlock != basicBlocks.end()) {
            orderInsAndOuts(basicBlocks, ranges, block->to, block->from);
        }

        if (block->from < block->to) {
            position = block->to;
        } else {
            position = block->from;
        }
    }
}
*/

void LiveAnalysis::extendNaiveRange(std::vector<LiveAnalysis::BasicBlock>& basicBlocks, std::vector<VariableRange>& ranges)
{
    for (BasicBlock block : basicBlocks) {
        if (block.from > block.to) {
            for (VariableRange in : block.in) {
                if (in.end < block.from) {
                    in.end = block.from;
                }
            }

            for (VariableRange out : block.out) {
                if (out.start < block.to) {
                    out.start = block.to;
                }
            }
        } else {
            for (VariableRange in : block.in) {
                if (in.end < block.to) {
                    in.end = block.to;
                }
            }

            for (VariableRange out : block.out) {
                if (out.start < block.from) {
                    out.start = block.from;
                }
            }
        }

        for (VariableRange& range : ranges) {
            bool allSetsInBlocks = true;
            for (Walrus::ByteCodeStackOffset set : range.sets) {
                if (!(block.from < set && block.to > set)
                    && !(block.from > set && block.to < set)) {
                    allSetsInBlocks = false;
                }
            }

            if (allSetsInBlocks) {
                range.needsInit = true;
            }
        }
    }

    for (VariableRange& range : ranges) {
        if (!range.value.isZeroValue()) {
            range.needsInit = true;
            range.start = 0;
        }

        if (range.reads.empty() && range.sets.empty()) {
            continue;
        }

        if (range.sets.empty() && !range.reads.empty()) {
            range.needsInit = true;
            continue;
        }

        if ((!range.sets.empty() && !range.reads.empty()) && (range.reads.front() <= range.sets.front())) {
            range.needsInit = true;
            range.start = 0;
        }
    }
}

void LiveAnalysis::pushVariableInits(std::vector<LiveAnalysis::VariableRange>& ranges, Walrus::ModuleFunction* func)
{
    uint32_t constSize = 0;
    if (!UnusedReads.elements.empty()) {
        if (UnusedReads.valueSize == 4) {
            func->pushByteCodeToFront(Walrus::Const32(UnusedReads.pos, 0));
            constSize += sizeof(Walrus::Const32);
        } else if (UnusedReads.valueSize == 8) {
            func->pushByteCodeToFront(Walrus::Const64(UnusedReads.pos, 0));
            constSize += sizeof(Walrus::Const64);
        } else if (UnusedReads.valueSize == 16) {
            uint8_t empty[16] = { 0 };
            func->pushByteCodeToFront(Walrus::Const128(UnusedReads.pos, empty));
            constSize += sizeof(Walrus::Const128);
        } else {
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    for (auto& range : ranges) {
        if (range.isParam || !range.needsInit || (range.sets.empty() && range.reads.empty())) {
            continue;
        }

        if (range.newOffset != UINT16_MAX && (range.newOffset == UnusedReads.pos || range.newOffset == UnusedWrites.pos)) {
            continue;
        }

        switch (range.value.type()) {
#if defined(WALRUS_32)
        case Walrus::Value::ExternRef:
        case Walrus::Value::FuncRef:
#endif
        case Walrus::Value::I32: {
            constSize += sizeof(Walrus::Const32);
            func->pushByteCodeToFront(Walrus::Const32(range.newOffset, range.value.asI32()));
            break;
        }
        case Walrus::Value::F32: {
            constSize += sizeof(Walrus::Const32);

            uint8_t constantBuffer[4];
            range.value.writeToMemory(constantBuffer);

            func->pushByteCodeToFront(Walrus::Const32(range.newOffset, *reinterpret_cast<uint32_t*>(constantBuffer)));
            break;
        }
#if defined(WALRUS_64)
        case Walrus::Value::ExternRef:
        case Walrus::Value::FuncRef:
#endif
        case Walrus::Value::I64: {
            constSize += sizeof(Walrus::Const64);
            func->pushByteCodeToFront(Walrus::Const64(range.newOffset, range.value.asI64()));
            break;
        }
        case Walrus::Value::F64: {
            constSize += sizeof(Walrus::Const64);

            uint8_t constantBuffer[8];
            range.value.writeToMemory(constantBuffer);

            func->pushByteCodeToFront(Walrus::Const64(range.newOffset, *reinterpret_cast<uint64_t*>(constantBuffer)));
            break;
        }
        case Walrus::Value::V128: {
            constSize += sizeof(Walrus::Const128);

            uint8_t constantBuffer[16];
            range.value.writeToMemory(constantBuffer);

            func->pushByteCodeToFront(Walrus::Const128(range.newOffset, constantBuffer));
            break;
        }
        default: {
            break;
        }
        }
    }

    for (auto& tryCatch : func->catchInfo()) {
        tryCatch.m_tryStart += constSize;
        tryCatch.m_tryEnd += constSize;
        tryCatch.m_catchStartPosition += constSize;
    }
}

void LiveAnalysis::orderStack(Walrus::ModuleFunction* func, std::vector<VariableRange>& ranges, uint16_t stackStart, uint16_t stackEnd)
{
    std::vector<std::pair<size_t, size_t>> freeSpaces = { std::make_pair(stackStart, stackEnd) };

    for (VariableRange& range : ranges) {
        if ((range.reads.empty() && range.sets.empty()) || !range.value.isZeroValue() || range.isParam) {
            continue;
        }

        // if (range.sets.empty() && !range.reads.empty() && !range.needsInit) {
        if (range.sets.empty() && !range.reads.empty()) {
            UnusedReads.elements.push_back(&range);

            if (UnusedReads.valueSize < Walrus::valueSize(range.value.type())) {
                UnusedReads.valueSize = Walrus::valueSize(range.value.type());
            }
        } else if (!range.sets.empty() && range.reads.empty()) {
            UnusedWrites.elements.push_back(&range);

            if (UnusedWrites.valueSize < Walrus::valueSize(range.value.type())) {
                UnusedWrites.valueSize = Walrus::valueSize(range.value.type());
            }
        }
    }

    if (!UnusedWrites.elements.empty()) {
        UnusedWrites.pos = freeSpaces.front().first;
        freeSpaces.front().first += UnusedWrites.valueSize;

        for (VariableRange* range : UnusedWrites.elements) {
            range->newOffset = UnusedWrites.pos;
        }
    }

    if (!UnusedReads.elements.empty()) {
        UnusedReads.pos = freeSpaces.front().first;
        freeSpaces.front().first += UnusedReads.valueSize;

        for (VariableRange* range : UnusedReads.elements) {
            range->newOffset = UnusedReads.pos;
        }
    }

    size_t byteCodeOffset = 0;
    while (byteCodeOffset < func->currentByteCodeSize()) {
        Walrus::ByteCode* code = reinterpret_cast<Walrus::ByteCode*>(const_cast<uint8_t*>(func->byteCode() + byteCodeOffset));

        std::vector<Walrus::ByteCodeStackOffset> offsets = code->getByteCodeStackOffsets(func->functionType());
        std::vector<bool> writtenOffsets(offsets.size(), false);
        for (VariableRange& range : ranges) {
            if (range.start == UINT16_MAX && range.end == 0) {
                continue;
            }

            ASSERT(!freeSpaces.empty());
            bool isUnusedRead = std::find(UnusedReads.elements.begin(), UnusedReads.elements.end(), &range) != UnusedReads.elements.end();
            bool isUnusedWrite = std::find(UnusedWrites.elements.begin(), UnusedWrites.elements.end(), &range) != UnusedWrites.elements.end();

            if (range.start == byteCodeOffset && !range.isParam && !(isUnusedRead || isUnusedWrite)) {
                for (size_t i = 0; i < freeSpaces.size(); i++) {
                    if ((freeSpaces[i].second - freeSpaces[i].first) >= Walrus::valueSize(range.value.type())) {
                        range.newOffset = freeSpaces[i].first;

                        if (freeSpaces[i].second - freeSpaces[i].first == 0) {
                            freeSpaces.erase(freeSpaces.begin() + i);
                        } else {
                            freeSpaces[i].first += Walrus::valueSize(range.value.type());
                        }
                        break;
                    }
                }
            }

            if (range.end == byteCodeOffset && range.newOffset != UINT16_MAX && !(isUnusedRead || isUnusedWrite) && !range.isParam) {
                bool insertedSpace = false;
                for (auto& space : freeSpaces) {
                    if (space.first - Walrus::valueSize(range.value.type()) == range.newOffset) {
                        space.first -= Walrus::valueSize(range.value.type());
                        insertedSpace = true;
                        break;
                    } else if (space.second == range.newOffset) {
                        space.second += Walrus::valueSize(range.value.type());
                        insertedSpace = true;
                        break;
                    }
                }

                if (!insertedSpace) {
                    freeSpaces.push_back(std::make_pair(range.newOffset, range.newOffset + Walrus::valueSize(range.value.type())));
                }
            }

            if (range.start <= byteCodeOffset && range.end >= byteCodeOffset) {
                for (size_t i = 0; i < offsets.size(); i++) {
                    if (range.originalOffset == offsets[i] && !writtenOffsets[i]) {
                        code->setByteCodeOffset(i, range.newOffset, range.originalOffset);
                        writtenOffsets[i] = true;

                        switch (code->opcode()) {
                        case Walrus::ByteCode::EndOpcode:
                        case Walrus::ByteCode::CallOpcode:
                        case Walrus::ByteCode::CallIndirectOpcode:
                        case Walrus::ByteCode::CallRefOpcode:
#if defined(WALRUS_64)
                            if (range.value.type() == Walrus::Value::V128) {
                                code->setByteCodeOffset(i + 1, range.newOffset + 8, range.originalOffset);
                                writtenOffsets[i + 1] = true;
                                i++;
                            }
#elif defined(WALRUS_32)
                            switch (range.value.type()) {
                            case Walrus::Value::Type::I64:
                            case Walrus::Value::Type::F64: {
                                code->setByteCodeOffset(i + 1, range.newOffset + 4, range.originalOffset);
                                writtenOffsets[i + 1] = true;
                                i++;
                                break;
                            }
                            case Walrus::Value::Type::V128: {
                                code->setByteCodeOffset(i + 1, range.newOffset + 4, range.originalOffset);
                                writtenOffsets[i + 1] = true;
                                i++;

                                code->setByteCodeOffset(i + 1, range.newOffset + 8, range.originalOffset);
                                writtenOffsets[i + 1] = true;
                                i++;

                                code->setByteCodeOffset(i + 1, range.newOffset + 12, range.originalOffset);
                                writtenOffsets[i + 1] = true;
                                i++;
                                break;
                            }
                            default: {
                                break;
                            }
                            }
#endif
                        default: {
                            break;
                        }
                        }
                    }
                }
            }
        }

        byteCodeOffset += code->getSize();
    }

#if !defined(NDEBUG)
    for (auto range : ranges) {
        if (range.isConstant) {
            func->pushConstDebugData(range.value, range.newOffset);
        } else {
            func->pushLocalDebugData(range.newOffset);
        }
    }
#endif

    pushVariableInits(ranges, func);

    // func->setStackSize(func->requiredStackSize() + constSize);
}

void LiveAnalysis::orderNaiveRange(Walrus::ByteCode* code, Walrus::ModuleFunction* func, std::vector<VariableRange>& ranges, Walrus::ByteCodeStackOffset byteCodeOffset)
{
    std::vector<Walrus::ByteCodeStackOffset> offsets = code->getByteCodeStackOffsets(func->functionType());
    for (size_t i = 0; i < offsets.size(); i++) {
        auto elem = std::find_if(
            ranges.begin(),
            ranges.end(),
            [&, offsets, i](VariableRange& elem) { return elem.originalOffset == offsets[i]; });
        if (elem != ranges.end()) {
            if (elem->end < byteCodeOffset) {
                elem->end = byteCodeOffset;
            }
            if (elem->start > byteCodeOffset) {
                elem->start = byteCodeOffset;
            }

            // Calls and End opcode need special cases.
            switch (code->opcode()) {
            case Walrus::ByteCode::I32StoreOpcode:
            case Walrus::ByteCode::I32Store16Opcode:
            case Walrus::ByteCode::I32Store8Opcode:
            case Walrus::ByteCode::I64StoreOpcode:
            case Walrus::ByteCode::I64Store32Opcode:
            case Walrus::ByteCode::I64Store16Opcode:
            case Walrus::ByteCode::I64Store8Opcode:
            case Walrus::ByteCode::F32StoreOpcode:
            case Walrus::ByteCode::F64StoreOpcode:
            case Walrus::ByteCode::V128StoreOpcode:
            case Walrus::ByteCode::I32StoreMemIdxOpcode:
            case Walrus::ByteCode::I32Store16MemIdxOpcode:
            case Walrus::ByteCode::I32Store8MemIdxOpcode:
            case Walrus::ByteCode::I64StoreMemIdxOpcode:
            case Walrus::ByteCode::I64Store32MemIdxOpcode:
            case Walrus::ByteCode::I64Store16MemIdxOpcode:
            case Walrus::ByteCode::I64Store8MemIdxOpcode:
            case Walrus::ByteCode::F32StoreMemIdxOpcode:
            case Walrus::ByteCode::F64StoreMemIdxOpcode:
            case Walrus::ByteCode::V128StoreMemIdxOpcode:
            case Walrus::ByteCode::Store32Opcode:
            case Walrus::ByteCode::Store64Opcode:
            case Walrus::ByteCode::JumpIfFalseOpcode:
            case Walrus::ByteCode::JumpIfTrueOpcode:
            case Walrus::ByteCode::JumpIfNullOpcode:
            case Walrus::ByteCode::JumpIfNonNullOpcode:
            case Walrus::ByteCode::JumpIfCastGenericOpcode:
            case Walrus::ByteCode::JumpIfCastDefinedOpcode:
            case Walrus::ByteCode::TableInitOpcode:
            case Walrus::ByteCode::TableCopyOpcode:
            case Walrus::ByteCode::TableSetOpcode:
            case Walrus::ByteCode::TableFillOpcode:
            case Walrus::ByteCode::MemoryFillOpcode:
            case Walrus::ByteCode::MemoryInitOpcode:
            case Walrus::ByteCode::ThrowOpcode:
            case Walrus::ByteCode::BrTableOpcode:
            case Walrus::ByteCode::EndOpcode:
            case Walrus::ByteCode::GlobalSet32Opcode:
            case Walrus::ByteCode::GlobalSet64Opcode:
            case Walrus::ByteCode::GlobalSet128Opcode:
            // WebAsm3
            case Walrus::ByteCode::ArrayFillOpcode:
            case Walrus::ByteCode::ArrayCopyOpcode:
            // SIMD ByteCodes
            case Walrus::ByteCode::V128Store8LaneOpcode:
            case Walrus::ByteCode::V128Store8LaneMemIdxOpcode:
            case Walrus::ByteCode::V128Store16LaneOpcode:
            case Walrus::ByteCode::V128Store16LaneMemIdxOpcode:
            case Walrus::ByteCode::V128Store32LaneOpcode:
            case Walrus::ByteCode::V128Store32LaneMemIdxOpcode:
            case Walrus::ByteCode::V128Store64LaneOpcode:
            case Walrus::ByteCode::V128Store64LaneMemIdxOpcode: {
                elem->reads.push_back(byteCodeOffset);
                break;
            }
            case Walrus::ByteCode::GlobalGet32Opcode:
            case Walrus::ByteCode::GlobalGet64Opcode:
            case Walrus::ByteCode::GlobalGet128Opcode: {
                elem->sets.push_back(byteCodeOffset);
                break;
            }
            case Walrus::ByteCode::CallOpcode: {
                Walrus::Call* call = reinterpret_cast<Walrus::Call*>(const_cast<Walrus::ByteCode*>(code));
                if (i < call->parameterOffsetsSize()) {
                    elem->reads.push_back(byteCodeOffset);
                } else {
                    elem->sets.push_back(byteCodeOffset);
                }
                break;
            }
            case Walrus::ByteCode::CallIndirectOpcode: {
                Walrus::CallIndirect* call = reinterpret_cast<Walrus::CallIndirect*>(const_cast<Walrus::ByteCode*>(code));
                if (offsets[i] == call->calleeOffset()) {
                    elem->reads.push_back(byteCodeOffset);
                    break;
                }

                size_t resultStart = 0;
                for (auto& type : call->functionType()->param()) {
                    resultStart++;

#if defined(WALRUS_64)
                    if (type == Walrus::Value::Type::V128) {
                        resultStart++;
                    }
#elif defined(WALRUS_32)
                    switch (type) {
                    case Walrus::Value::Type::I64:
                    case Walrus::Value::Type::F64: {
                        resultStart++;
                        break;
                    }
                    case Walrus::Value::Type::V128: {
                        resultStart += 3;
                        break;
                    }
                    default: {
                        break;
                    }
                    }
#endif
                }

                if (i <= resultStart) {
                    elem->reads.push_back(byteCodeOffset);
                } else {
                    elem->sets.push_back(byteCodeOffset);
                }

                break;
            }
            case Walrus::ByteCode::CallRefOpcode: {
                Walrus::CallRef* callRef = reinterpret_cast<Walrus::CallRef*>(const_cast<Walrus::ByteCode*>(code));

                if (i < callRef->parameterOffsetsSize()) {
                    elem->reads.push_back(byteCodeOffset);
                } else {
                    elem->sets.push_back(byteCodeOffset);
                }

                break;
            }
            default: {
                if (&offsets[i] == &offsets.back()) {
                    elem->sets.push_back(byteCodeOffset);
                } else {
                    elem->reads.push_back(byteCodeOffset);
                }
                break;
            }
            }

            offsets[i] = UINT16_MAX;
        }
    }
}

void LiveAnalysis::optimizeLocals(Walrus::ModuleFunction* func, std::vector<std::pair<size_t, Walrus::Value>>& locals, size_t constantStart)
{
    std::vector<VariableRange> ranges;
    std::vector<Walrus::ByteCodeStackOffset> offsets;

    ranges.reserve(locals.size());
    size_t byteCodeOffset = 0;

    for (uint32_t i = 0; i < locals.size(); i++) {
        ranges.push_back(VariableRange(locals[i].first, locals[i].second));

        if (i < func->functionType()->param().size()) {
            ranges[i].isParam = true;
            ranges[i].newOffset = ranges[i].originalOffset;
        }

        if (i >= constantStart) {
            ranges[i].start = 0;
            ranges[i].end = UINT16_MAX;
#if !defined(NDEBUG)
            ranges[i].isConstant = true;
#endif
        }
    }

    std::vector<LiveAnalysis::BasicBlock> basicBlocks;

    while (byteCodeOffset < func->currentByteCodeSize()) {
        Walrus::ByteCode* code = reinterpret_cast<Walrus::ByteCode*>(const_cast<uint8_t*>(func->byteCode() + byteCodeOffset));
        offsets = code->getByteCodeStackOffsets(func->functionType());

        orderNaiveRange(code, func, ranges, byteCodeOffset);
        assignBasicBlocks(code, basicBlocks, byteCodeOffset);

        byteCodeOffset += code->getSize();
    }

    uint16_t stackStart = UINT16_MAX;
    uint16_t stackEnd = 0;
    if (!basicBlocks.empty()) {
        orderInsAndOuts(basicBlocks, ranges, func->currentByteCodeSize());
    }
    extendNaiveRange(basicBlocks, ranges);

    for (VariableRange& range : ranges) {
        if (range.isParam) {
            continue;
        }

        if (range.originalOffset < stackStart) {
            stackStart = range.originalOffset;
        }

        if (stackEnd < range.originalOffset + Walrus::valueSize(range.value.type())) {
            stackEnd = range.originalOffset + Walrus::valueSize(range.value.type());
        }

        if (range.sets.empty() || range.reads.empty()) {
            continue;
        }

        if ((range.reads.front() < range.sets.front())) {
            range.start = 0;
        }
    }
    orderStack(func, ranges, stackStart, stackEnd);
}
} // namespace wabt
