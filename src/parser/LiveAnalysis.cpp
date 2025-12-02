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
void LiveAnalysis::assignBasicBlocks(Walrus::ByteCode* code, std::vector<LiveAnalysis::BasicBlock*>& basicBlocks, uint64_t byteCodeOffset)
{
    switch (code->opcode()) {
    case Walrus::ByteCode::JumpOpcode: {
        Walrus::Jump* jump = reinterpret_cast<Walrus::Jump*>(code);
        basicBlocks.push_back(new LiveAnalysis::BasicBlock(byteCodeOffset, byteCodeOffset + jump->offset()));
        break;
    }
    case Walrus::ByteCode::JumpIfTrueOpcode:
    case Walrus::ByteCode::JumpIfFalseOpcode:
    case Walrus::ByteCode::JumpIfNullOpcode:
    case Walrus::ByteCode::JumpIfNonNullOpcode:
    case Walrus::ByteCode::JumpIfCastGenericOpcode:
    case Walrus::ByteCode::JumpIfCastDefinedOpcode: {
        Walrus::ByteCodeOffsetValue* jumpIf = reinterpret_cast<Walrus::ByteCodeOffsetValue*>(code);
        basicBlocks.push_back(new LiveAnalysis::BasicBlock(byteCodeOffset, byteCodeOffset + jumpIf->int32Value()));
        break;
    }
    case Walrus::ByteCode::BrTableOpcode: {
        Walrus::BrTable* table = reinterpret_cast<Walrus::BrTable*>(code);
        basicBlocks.push_back(new LiveAnalysis::BasicBlock(byteCodeOffset, byteCodeOffset + table->defaultOffset()));
        for (size_t i = 0; i < table->tableSize(); i++) {
            if (byteCodeOffset + table->jumpOffsets()[i]) {
                basicBlocks.push_back(new LiveAnalysis::BasicBlock(byteCodeOffset, byteCodeOffset + table->jumpOffsets()[i]));
            }
        }
        break;
    }
    default: {
        return;
    }
    }
}

void LiveAnalysis::orderInsAndOuts(std::vector<LiveAnalysis::BasicBlock*>& basicBlocks, VariableRange* ranges, uint64_t rangesSize, uint64_t end, uint64_t position)
{
    size_t currentBlockIdx = 0;
    while (position < end && currentBlockIdx < basicBlocks.size()) {
        uint16_t blockStart = basicBlocks[currentBlockIdx]->from < basicBlocks[currentBlockIdx]->to ? basicBlocks[currentBlockIdx]->from : basicBlocks[currentBlockIdx]->to;
        uint16_t blockEnd = basicBlocks[currentBlockIdx]->from < basicBlocks[currentBlockIdx]->to ? basicBlocks[currentBlockIdx]->to : basicBlocks[currentBlockIdx]->from;

        for (uint64_t i = 0; i < rangesSize; i++) {
            if ((blockStart < ranges[i].start && ranges[i].end < blockEnd)) {
                // || (ranges[i].start < blockStart && blockStart < ranges[i].end)
                // || (ranges[i].start < blockEnd && blockEnd < ranges[i].end)) {
                basicBlocks[currentBlockIdx]->containedVariables.push_back(&ranges[i]);
                continue;
            }

            // Forward jump case
            if (basicBlocks[currentBlockIdx]->from < basicBlocks[currentBlockIdx]->to) {
                if (ranges[i].start < basicBlocks[currentBlockIdx]->from) {
                    if (std::find(basicBlocks[currentBlockIdx]->in.begin(),
                                  basicBlocks[currentBlockIdx]->in.end(), &ranges[i])
                        == basicBlocks[currentBlockIdx]->in.end()) {
                        basicBlocks[currentBlockIdx]->in.push_back(&ranges[i]);
                    }
                }
                if (ranges[i].end > basicBlocks[currentBlockIdx]->to) {
                    if (std::find(basicBlocks[currentBlockIdx]->out.begin(),
                                  basicBlocks[currentBlockIdx]->out.end(), &ranges[i])
                        == basicBlocks[currentBlockIdx]->out.end()) {
                        basicBlocks[currentBlockIdx]->out.push_back(&ranges[i]);
                    }
                }
                // Backward jump case.
            } else {
                if (ranges[i].start < basicBlocks[currentBlockIdx]->to) {
                    if (std::find(basicBlocks[currentBlockIdx]->in.begin(),
                                  basicBlocks[currentBlockIdx]->in.end(), &ranges[i])
                        == basicBlocks[currentBlockIdx]->in.end()) {
                        basicBlocks[currentBlockIdx]->in.push_back(&ranges[i]);
                    }
                }
                if (ranges[i].end > basicBlocks[currentBlockIdx]->from) {
                    if (std::find(basicBlocks[currentBlockIdx]->out.begin(),
                                  basicBlocks[currentBlockIdx]->out.end(), &ranges[i])
                        == basicBlocks[currentBlockIdx]->out.end()) {
                        basicBlocks[currentBlockIdx]->out.push_back(&ranges[i]);
                    }
                }
            }
        }

        for (BasicBlock* block : basicBlocks) {
            uint16_t insideBlockStart = block->from < block->to ? block->from : block->to;
            uint16_t insideBlockEnd = block->from < block->to ? block->to : block->from;

            if (blockStart < insideBlockStart && insideBlockEnd < blockEnd) {
                std::vector<BasicBlock*> containing = basicBlocks[currentBlockIdx]->containedBlocks;

                if (std::find(containing.begin(), containing.end(), block) == containing.end()) {
                    basicBlocks[currentBlockIdx]->containedBlocks.push_back(block);
                }
            }
        }

        if (!basicBlocks[currentBlockIdx]->containedBlocks.empty()) {
            orderInsAndOuts(basicBlocks[currentBlockIdx]->containedBlocks, ranges, rangesSize, blockStart, blockEnd);
        }

        currentBlockIdx++;
        position = blockEnd;
    }
}

void LiveAnalysis::extendNaiveRange(std::vector<LiveAnalysis::BasicBlock*>& basicBlocks, VariableRange* ranges, uint64_t rangesSize)
{
    for (BasicBlock* block : basicBlocks) {
        if (block->from > block->to) {
            for (VariableRange* in : block->in) {
                if (in->end < block->from) {
                    in->end = block->from;
                }
            }

            for (VariableRange*& out : block->out) {
                if (out->start < block->to) {
                    out->start = block->to;
                }
            }
        } else {
            for (VariableRange* in : block->in) {
                if (in->end < block->to) {
                    in->end = block->to;
                }
            }

            for (VariableRange* out : block->out) {
                if (out->start < block->from) {
                    out->start = block->from;
                }
            }
        }

        for (VariableRange* range : block->containedVariables) {
            if (range->isConstant || range->isParam) {
                continue;
            }

            if (block->from < block->to) {
                if (block->from < range->start) {
                    range->start = block->from;
                }
                if (range->end < block->to) {
                    range->end = block->to;
                }
            } else {
                if (block->to < range->start) {
                    range->start = block->to;
                }
                if (range->end < block->from) {
                    range->end = block->from;
                }
            }
        }
    }

    for (uint64_t i = 0; i < rangesSize; i++) {
        if (ranges[i].isConstant) {
            ranges[i].start = 0;
            ranges[i].needsInit = true;
        }

        if (ranges[i].reads.empty() && ranges[i].sets.empty()) {
            continue;
        }

        if (ranges[i].sets.empty() && !ranges[i].reads.empty()) {
            ranges[i].start = 0;
            ranges[i].needsInit = true;
            continue;
        }

        uint64_t setsMin = 0;
        uint64_t readsMin = 0;
        if (!ranges[i].sets.empty()) {
            *std::min_element(ranges[i].sets.begin(), ranges[i].sets.end());
        }

        if (!ranges[i].reads.empty()) {
            *std::min_element(ranges[i].reads.begin(), ranges[i].reads.end());
        }

        if ((!ranges[i].sets.empty() && !ranges[i].reads.empty()) && (readsMin <= setsMin)) {
            ranges[i].needsInit = true;
            ranges[i].start = 0;
        }

        if (!ranges[i].reads.empty() && !basicBlocks.empty()) {
            bool allSetsInBlocks = true;
            for (Walrus::ByteCodeStackOffset set : ranges[i].sets) {
                if (set < readsMin) {
                    for (BasicBlock* block : basicBlocks) {
                        if (!((block->from <= set && set <= block->to) || (block->to <= set && set <= block->from))
                            && (readsMin <= block->from && readsMin <= block->to)) {
                            allSetsInBlocks = false;
                        }
                    }
                }
            }
            if (allSetsInBlocks) {
                ranges[i].start = 0;
                ranges[i].needsInit = true;
            }
        }
    }
}

void LiveAnalysis::pushVariableInits(VariableRange* ranges, uint64_t rangesSize, Walrus::ModuleFunction* func)
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

    for (uint64_t i = 0; i < rangesSize; i++) {
        if (ranges[i].isParam || !ranges[i].needsInit || (ranges[i].sets.empty() && ranges[i].reads.empty())) {
            continue;
        }

        if (ranges[i].newOffset != UINT16_MAX && (ranges[i].newOffset == UnusedReads.pos || ranges[i].newOffset == UnusedWrites.pos)) {
            continue;
        }

        switch (ranges[i].value.type()) {
        case Walrus::Value::I32: {
            constSize += sizeof(Walrus::Const32);
            func->pushByteCodeToFront(Walrus::Const32(ranges[i].newOffset, ranges[i].value.asI32()));
            break;
        }
        case Walrus::Value::F32: {
            constSize += sizeof(Walrus::Const32);

            uint8_t constantBuffer[4];
            ranges[i].value.writeToMemory(constantBuffer);

            func->pushByteCodeToFront(Walrus::Const32(ranges[i].newOffset, *reinterpret_cast<uint32_t*>(constantBuffer)));
            break;
        }
        case Walrus::Value::I64: {
            constSize += sizeof(Walrus::Const64);
            func->pushByteCodeToFront(Walrus::Const64(ranges[i].newOffset, ranges[i].value.asI64()));
            break;
        }
        case Walrus::Value::F64: {
            constSize += sizeof(Walrus::Const64);

            uint8_t constantBuffer[8];
            ranges[i].value.writeToMemory(constantBuffer);

            func->pushByteCodeToFront(Walrus::Const64(ranges[i].newOffset, *reinterpret_cast<uint64_t*>(constantBuffer)));
            break;
        }
        case Walrus::Value::V128: {
            constSize += sizeof(Walrus::Const128);

            uint8_t constantBuffer[16];
            ranges[i].value.writeToMemory(constantBuffer);

            func->pushByteCodeToFront(Walrus::Const128(ranges[i].newOffset, constantBuffer));
            break;
        }
        case Walrus::Value::ExternRef:
        case Walrus::Value::FuncRef: {
            constSize += sizeof(Walrus::Const64);
            func->pushByteCodeToFront(Walrus::RefFunc(0, ranges[i].newOffset));
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

void LiveAnalysis::orderStack(Walrus::ModuleFunction* func, VariableRange* ranges, uint64_t rangesSize, uint16_t stackStart)
{
    std::vector<std::pair<Walrus::ByteCodeStackOffset, Walrus::ByteCodeStackOffset>> freeSpaces = { std::make_pair(stackStart, UINT16_MAX) };

    for (uint64_t i = 0; i < rangesSize; i++) {
        if ((ranges[i].reads.empty() && ranges[i].sets.empty()) || ranges[i].isParam || ranges[i].isConstant) {
            continue;
        }

        if (ranges[i].sets.empty() && !ranges[i].reads.empty()) {
            UnusedReads.elements.push_back(&ranges[i]);

            if (UnusedReads.valueSize < Walrus::valueSize(ranges[i].value.type())) {
                // if (UnusedReads.valueSize < Walrus::valueStackAllocatedSize(ranges[i].value.type())) {
                UnusedReads.valueSize = Walrus::valueSize(ranges[i].value.type());
                // UnusedReads.valueSize = Walrus::valueStackAllocatedSize(ranges[i].value.type());
            }
        } else if (!ranges[i].sets.empty() && ranges[i].reads.empty()) {
            UnusedWrites.elements.push_back(&ranges[i]);

            if (UnusedWrites.valueSize < Walrus::valueSize(ranges[i].value.type())) {
                // if (UnusedWrites.valueSize < Walrus::valueStackAllocatedSize(ranges[i].value.type())) {
                UnusedWrites.valueSize = Walrus::valueSize(ranges[i].value.type());
                // UnusedWrites.valueSize = Walrus::valueStackAllocatedSize(ranges[i].value.type());
            }
        }
    }

    if (!UnusedWrites.elements.empty()) {
        UnusedWrites.pos = freeSpaces.front().first;
        freeSpaces.front().first += UnusedWrites.valueSize;

        for (VariableRange* range : UnusedWrites.elements) {
            range->newOffset = UnusedWrites.pos;
            range->end = UINT64_MAX;
        }
    }

    if (!UnusedReads.elements.empty()) {
        UnusedReads.pos = freeSpaces.front().first;
        freeSpaces.front().first += UnusedReads.valueSize;

        for (VariableRange* range : UnusedReads.elements) {
            range->newOffset = UnusedReads.pos;
            range->end = UINT64_MAX;
        }
    }

    uint64_t byteCodeOffset = 0;
    while (byteCodeOffset < func->currentByteCodeSize()) {
        Walrus::ByteCode* code = reinterpret_cast<Walrus::ByteCode*>(const_cast<uint8_t*>(func->byteCode() + byteCodeOffset));

        std::vector<Walrus::ByteCodeStackOffset> offsets = code->getByteCodeStackOffsets(func->functionType());
        std::vector<bool> writtenOffsets(offsets.size(), false);
        for (uint64_t i = 0; i < rangesSize; i++) {
            if (ranges[i].start == UINT64_MAX && ranges[i].end == 0) {
                continue;
            }

            ASSERT(!freeSpaces.empty());
            bool isUnusedRead = std::find(UnusedReads.elements.begin(), UnusedReads.elements.end(), &ranges[i]) != UnusedReads.elements.end();
            bool isUnusedWrite = std::find(UnusedWrites.elements.begin(), UnusedWrites.elements.end(), &ranges[i]) != UnusedWrites.elements.end();

            if (ranges[i].start == byteCodeOffset && !ranges[i].isParam && !(isUnusedRead || isUnusedWrite)) {
                for (size_t j = freeSpaces.size() - 1; 0 <= j; j--) {
                    if ((freeSpaces[j].second - freeSpaces[j].first) >= (Walrus::ByteCodeStackOffset)Walrus::valueSize(ranges[i].value.type())) {
                        // if ((freeSpaces[j].second - freeSpaces[j].first) >= (Walrus::ByteCodeStackOffset)Walrus::valueStackAllocatedSize(ranges[i].value.type())) {
                        ranges[i].newOffset = freeSpaces[j].first;

                        if (freeSpaces[j].second - freeSpaces[j].first == 0) {
                            freeSpaces.erase(freeSpaces.begin() + i);
                        } else {
                            freeSpaces[j].first += Walrus::valueSize(ranges[i].value.type());
                            // freeSpaces[j].first += Walrus::valueStackAllocatedSize(ranges[i].value.type());
                        }
                        break;
                    }
                }
            }

            if (ranges[i].end == byteCodeOffset && ranges[i].newOffset != UINT16_MAX && !(isUnusedRead || isUnusedWrite) && !ranges[i].isParam && !ranges[i].isConstant) {
                bool insertedSpace = false;
                for (auto& space : freeSpaces) {
                    if (space.first - Walrus::valueSize(ranges[i].value.type()) == ranges[i].newOffset) {
                        // if (space.first - Walrus::valueStackAllocatedSize(ranges[i].value.type()) == ranges[i].newOffset) {
                        space.first -= Walrus::valueSize(ranges[i].value.type());
                        // space.first -= Walrus::valueStackAllocatedSize(ranges[i].value.type());
                        insertedSpace = true;
                        break;
                    } else if (space.second == ranges[i].newOffset) {
                        space.second += Walrus::valueSize(ranges[i].value.type());
                        // space.second += Walrus::valueStackAllocatedSize(ranges[i].value.type());
                        insertedSpace = true;
                        break;
                    }
                }

                if (!insertedSpace) {
                    freeSpaces.push_back(std::make_pair(ranges[i].newOffset, ranges[i].newOffset + Walrus::valueSize(ranges[i].value.type())));
                    // freeSpaces.push_back(std::make_pair(ranges[i].newOffset, ranges[i].newOffset + Walrus::valueStackAllocatedSize(ranges[i].value.type())));
                }
            }

            if (ranges[i].start <= byteCodeOffset && ranges[i].end >= byteCodeOffset) {
                for (uint8_t j = 0; j < offsets.size(); j++) {
                    if (ranges[i].originalOffset == offsets[j] && !writtenOffsets[j]) {
                        code->setByteCodeOffset(j, ranges[i].newOffset, ranges[i].originalOffset);
                        writtenOffsets[j] = true;

                        switch (code->opcode()) {
                        case Walrus::ByteCode::EndOpcode:
                        case Walrus::ByteCode::CallOpcode:
                        case Walrus::ByteCode::CallIndirectOpcode:
                        case Walrus::ByteCode::CallRefOpcode:
#if defined(WALRUS_64)
                            if (ranges[i].value.type() == Walrus::Value::V128) {
                                code->setByteCodeOffset(j + 1, ranges[i].newOffset + 8, ranges[i].originalOffset);
                                writtenOffsets[j + 1] = true;
                                j++;
                            }
#elif defined(WALRUS_32)
                            switch (ranges[i].value.type()) {
                            case Walrus::Value::Type::I64:
                            case Walrus::Value::Type::F64: {
                                code->setByteCodeOffset(j + 1, ranges[i].newOffset + 4, ranges[i].originalOffset);
                                writtenOffsets[j + 1] = true;
                                j++;
                                break;
                            }
                            case Walrus::Value::Type::V128: {
                                code->setByteCodeOffset(j + 1, ranges[i].newOffset + 4, ranges[i].originalOffset);
                                writtenOffsets[j + 1] = true;
                                j++;

                                code->setByteCodeOffset(j + 1, ranges[i].newOffset + 8, ranges[i].originalOffset);
                                writtenOffsets[j + 1] = true;
                                j++;

                                code->setByteCodeOffset(j + 1, ranges[i].newOffset + 12, ranges[i].originalOffset);
                                writtenOffsets[j + 1] = true;
                                j++;
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

    Walrus::ByteCodeStackOffset highestNewOffset = 0;
    Walrus::ByteCodeStackOffset highestOldOffset = 0;
    for (uint64_t i = 0; i < rangesSize; i++) {
        if (ranges[i].newOffset != UINT16_MAX && highestNewOffset < ranges[i].newOffset) {
            highestNewOffset = ranges[i].newOffset;
        }
        if (highestOldOffset < ranges[i].originalOffset) {
            highestOldOffset = ranges[i].originalOffset;
        }
    }

    Walrus::ByteCodeStackOffset offsetDiff = highestOldOffset - highestNewOffset;
    if (0 < offsetDiff && !func->hasTryCatch()) {
        uint64_t byteCodeOffset = 0;
        while (byteCodeOffset < func->currentByteCodeSize()) {
            Walrus::ByteCode* code = reinterpret_cast<Walrus::ByteCode*>(const_cast<uint8_t*>(func->byteCode() + byteCodeOffset));
            std::vector<Walrus::ByteCodeStackOffset> offsets = code->getByteCodeStackOffsets(func->functionType());

            for (uint8_t i = 0; i < offsets.size(); i++) {
                bool local = false;
                for (uint64_t j = 0; j < rangesSize; j++) {
                    if (offsets[i] == ranges[j].newOffset) {
                        local = true;

                        switch (code->opcode()) {
                        case Walrus::ByteCode::CallOpcode:
                        case Walrus::ByteCode::EndOpcode: {
                            if (ranges[i].value.type() == Walrus::Value::V128) {
                                i++;
                            }
                        }
                        default: {
                            break;
                        }
                        }

                        break;
                    }
                }

                if (!local) {
                    code->setByteCodeOffset(i, offsets[i] - offsetDiff, offsets[i]);
                }
            }

            byteCodeOffset += code->getSize();
        }

        func->setStackSize(func->requiredStackSize() - offsetDiff);
    }

#if !defined(NDEBUG)
    for (uint64_t i = 0; i < rangesSize; i++) {
        if (ranges[i].isConstant) {
            func->pushConstDebugData(ranges[i].value, ranges[i].newOffset);
        } else if (!ranges[i].isParam) {
            func->pushLocalDebugData(ranges[i].newOffset);
        }
    }
#endif

    pushVariableInits(ranges, rangesSize, func);
}

void LiveAnalysis::orderNaiveRange(Walrus::ByteCode* code, Walrus::ModuleFunction* func, VariableRange* ranges, uint64_t rangesSize, uint64_t byteCodeOffset)
{
    std::vector<Walrus::ByteCodeStackOffset> offsets = code->getByteCodeStackOffsets(func->functionType());
    for (uint8_t i = 0; i < offsets.size(); i++) {
        VariableRange* elem = nullptr;

        for (uint64_t j = 0; j < rangesSize; j++) {
            if (ranges[j].originalOffset == offsets[i]) {
                elem = &ranges[j];
            }
        }

        if (elem != nullptr) {
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
            case Walrus::ByteCode::MemoryCopyOpcode:
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
                const Walrus::TypeVector& types = call->functionType()->param();
                // for (auto& type : call->functionType()->param()) {
                for (uint32_t j = 0; j < types.size(); j++) {
                    resultStart++;

#if defined(WALRUS_64)
                    if (types.types()[j] == Walrus::Value::Type::V128) {
                        resultStart++;
                    }
#elif defined(WALRUS_32)
                    switch (types.types()[j]) {
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
    uint64_t rangesSize = locals.size();
    VariableRange* ranges = new VariableRange[rangesSize];
    std::vector<Walrus::ByteCodeStackOffset> offsets;

    for (uint32_t i = 0; i < locals.size(); i++) {
        ranges[i] = VariableRange(locals[i].first, locals[i].second);

        if (i < func->functionType()->param().size()) {
            ranges[i].isParam = true;
            ranges[i].newOffset = ranges[i].originalOffset;
        }

        if (i >= constantStart) {
            ranges[i].start = 0;
            ranges[i].end = UINT64_MAX;
            ranges[i].isConstant = true;
        }
    }

    uint64_t byteCodeOffset = 0;
    std::vector<LiveAnalysis::BasicBlock*> basicBlocks;
    while (byteCodeOffset < func->currentByteCodeSize()) {
        Walrus::ByteCode* code = reinterpret_cast<Walrus::ByteCode*>(const_cast<uint8_t*>(func->byteCode() + byteCodeOffset));
        offsets = code->getByteCodeStackOffsets(func->functionType());

        orderNaiveRange(code, func, ranges, rangesSize, byteCodeOffset);
        assignBasicBlocks(code, basicBlocks, byteCodeOffset);

        byteCodeOffset += code->getSize();
    }

    uint16_t stackStart = UINT16_MAX;
    if (!basicBlocks.empty()) {
        orderInsAndOuts(basicBlocks, ranges, rangesSize, func->currentByteCodeSize());
    }
    extendNaiveRange(basicBlocks, ranges, rangesSize);

    for (uint64_t i = 0; i < rangesSize; i++) {
        if (ranges[i].isParam) {
            continue;
        }

        if (ranges[i].originalOffset < stackStart) {
            stackStart = ranges[i].originalOffset;
        }
    }

    orderStack(func, ranges, rangesSize, stackStart);

    for (uint32_t i = 0; i < basicBlocks.size(); i++) {
        basicBlocks[i]->containedVariables.clear();
        delete basicBlocks[i];
    }
    delete[] ranges;
}
} // namespace wabt
