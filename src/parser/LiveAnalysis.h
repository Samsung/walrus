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
#include "runtime/ObjectType.h"
#include "runtime/Value.h"
#include <cstdint>
#include <vector>

namespace wabt {

class LiveAnalysis {
public:
    struct VariableRange {
        Walrus::Value value;
        Walrus::ByteCodeStackOffset start;
        Walrus::ByteCodeStackOffset end;
        Walrus::ByteCodeStackOffset originalOffset;
        Walrus::ByteCodeStackOffset newOffset;
        bool isParam;
        bool needsInit;
        std::vector<Walrus::ByteCodeStackOffset> sets;
        std::vector<Walrus::ByteCodeStackOffset> reads;
#if !defined(NDEBUG)
        bool isConstant;
#endif

        VariableRange(Walrus::ByteCodeStackOffset o, Walrus::Value value)
            : value(value)
            , start(UINT16_MAX)
            , end(0)
            , originalOffset(o)
            , newOffset(UINT16_MAX)
            , isParam(false)
            , needsInit(false)
#if !defined(NDEBUG)
            , isConstant(false)
#endif
        {
        }
    };

    struct BasicBlock {
        size_t from;
        size_t to;
        std::vector<VariableRange> in;
        std::vector<VariableRange> out;

        BasicBlock(size_t from, size_t to)
            : from(from)
            , to(to)
        {
        }
    };

    struct StackElement {
        Walrus::ByteCodeStackOffset pos;
        uint8_t valueSize;
        std::vector<VariableRange*> elements;

        StackElement()
            : pos(UINT16_MAX)
            , valueSize(0)
        {
        }
    };

    void orderStack(Walrus::ModuleFunction* func, std::vector<VariableRange>& ranges, uint16_t stackStart, uint16_t stackEnd);
    void extendNaiveRange(std::vector<BasicBlock>& basicBlocks, std::vector<VariableRange>& ranges);
    void orderInsAndOuts(std::vector<BasicBlock>& basicBlocks, std::vector<VariableRange>& ranges, size_t end, size_t position = 0);
    void assignBasicBlocks(Walrus::ByteCode* code, std::vector<BasicBlock>& basicBlocks, size_t byteCodeOffset);
    void optimizeLocals(Walrus::ModuleFunction* func, std::vector<std::pair<size_t, Walrus::Value>>& locals, size_t constantStart);
    void orderNaiveRange(Walrus::ByteCode* code, Walrus::ModuleFunction* func, std::vector<VariableRange>& ranges, Walrus::ByteCodeStackOffset byteCodeOffset);
    // void pushVariableInits(std::vector<LiveAnalysis::VariableRange>& ranges, Walrus::ModuleFunction* func, bool neverUsedElementExists, Walrus::ByteCodeStackOffset neverUsedElementPos, uint8_t neverUsedElementSize);
    void pushVariableInits(std::vector<LiveAnalysis::VariableRange>& ranges, Walrus::ModuleFunction* func);

    StackElement UnusedReads;
    StackElement UnusedWrites;
};
} // namespace wabt
