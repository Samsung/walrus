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

#include "parser/WASMParser.h"
#include "interpreter/ByteCode.h"
#include "interpreter/Opcode.h"
#include "runtime/Module.h"

#include "wabt/walrus/binary-reader-walrus.h"

namespace wabt {

static Walrus::Value::Type toValueKindForFunctionType(Type type)
{
    switch (type) {
    case Type::I32:
        return Walrus::Value::Type::I32;
    case Type::I64:
        return Walrus::Value::Type::I64;
    case Type::F32:
        return Walrus::Value::Type::F32;
    case Type::F64:
        return Walrus::Value::Type::F64;
    case Type::FuncRef:
        return Walrus::Value::Type::FuncRef;
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
}

static Walrus::Value::Type toValueKindForLocalType(Type type)
{
    switch (type) {
    case Type::I32:
        return Walrus::Value::Type::I32;
    case Type::I64:
        return Walrus::Value::Type::I64;
    case Type::F32:
        return Walrus::Value::Type::F32;
    case Type::F64:
        return Walrus::Value::Type::F64;
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
}

class WASMBinaryReader : public wabt::WASMBinaryReaderDelegate {
public:
    struct BlockInfo {
        enum BlockType {
            IfElse,
            Loop,
            Block
        };
        BlockType m_blockType;
        Type m_returnValueType;
        size_t m_position;
        size_t m_stackPushCount;

        static_assert(sizeof(Walrus::JumpIfTrue) == sizeof(Walrus::JumpIfFalse), "");
        struct JumpToEndBrInfo {
            bool m_isJumpIf;
            size_t m_position;
        };

        std::vector<JumpToEndBrInfo> m_jumpToEndBrInfo;

        BlockInfo(BlockType type, Type returnValueType)
            : m_blockType(type)
            , m_returnValueType(returnValueType)
            , m_position(0)
            , m_stackPushCount(0)
        {
        }
    };

    WASMBinaryReader(Walrus::Module* module)
        : m_module(module)
        , m_currentFunction(nullptr)
        , m_currentFunctionType(nullptr)
        , m_functionStackSizeSoFar(0)
    {
    }

    virtual void BeginModule(uint32_t version) override
    {
        m_module->m_version = version;
    }

    virtual void EndModule() override {}

    virtual void OnTypeCount(Index count) override
    {
        // TODO reserve vector if possible
    }

    virtual void OnFuncType(Index index,
                            Index paramCount,
                            Type* paramTypes,
                            Index resultCount,
                            Type* resultTypes) override
    {
        Walrus::FunctionType::FunctionTypeVector param;
        param.reserve(paramCount);
        for (size_t i = 0; i < paramCount; i++) {
            param.push_back(toValueKindForFunctionType(paramTypes[i]));
        }
        Walrus::FunctionType::FunctionTypeVector result;
        for (size_t i = 0; i < resultCount; i++) {
            result.push_back(toValueKindForFunctionType(resultTypes[i]));
        }
        m_module->m_functionType.push_back(
            new Walrus::FunctionType(index, std::move(param), std::move(result)));
    }

    virtual void OnImportCount(Index count) override
    {
        m_module->m_import.reserve(count);
    }

    virtual void OnImportFunc(Index importIndex,
                              std::string moduleName,
                              std::string fieldName,
                              Index funcIndex,
                              Index sigIndex) override
    {
        m_module->m_function.push_back(
            new Walrus::ModuleFunction(m_module, funcIndex, sigIndex));
        m_module->m_import.push_back(new Walrus::ModuleImport(
            importIndex, new Walrus::String(moduleName),
            new Walrus::String(fieldName), funcIndex, sigIndex));
    }

    virtual void OnExportCount(Index count) override
    {
        m_module->m_export.reserve(count);
    }

    virtual void OnExport(int kind, Index exportIndex, std::string name, Index itemIndex) override
    {
        m_module->m_export.pushBack(new Walrus::ModuleExport(static_cast<Walrus::ModuleExport::Type>(kind), new Walrus::String(name), exportIndex, itemIndex));
    }

    /* Table section */
    virtual void OnTableCount(Index count) override
    {
        m_module->m_table.reserve(count);
    }

    virtual void OnTable(Index index, Type type, size_t initialSize, size_t maximumSize) override
    {
        ASSERT(index == m_module->m_table.size());
        ASSERT(type == Type::FuncRef || type == Type::ExternRef);
        m_module->m_table.pushBack(std::make_tuple(type == Type::FuncRef ? Walrus::Value::Type::FuncRef : Walrus::Value::Type::ExternRef, initialSize, maximumSize));
    }

    /* Memory section */
    virtual void OnMemoryCount(Index count) override
    {
        m_module->m_memory.reserve(count);
    }

    virtual void OnMemory(Index index, size_t initialSize, size_t maximumSize) override
    {
        ASSERT(index == m_module->m_memory.size());
        m_module->m_memory.pushBack(std::make_pair(initialSize, maximumSize));
    }

    /* Function section */
    virtual void OnFunctionCount(Index count) override
    {
        m_module->m_function.reserve(count);
    }

    virtual void OnFunction(Index index, Index sigIndex) override
    {
        ASSERT(m_currentFunction == nullptr);
        ASSERT(m_currentFunctionType == nullptr);
        m_module->m_function.push_back(new Walrus::ModuleFunction(m_module, index, sigIndex));
    }

    virtual void OnStartFunction(Index funcIndex) override
    {
        m_module->m_seenStartAttribute = true;
        m_module->m_start = funcIndex;
    }

    virtual void BeginFunctionBody(Index index, Offset size) override
    {
        ASSERT(m_currentFunction == nullptr);
        m_shouldContinueToGenerateByteCode = true;
        m_currentFunction = m_module->function(index);
        m_currentFunctionType = m_module->functionType(m_currentFunction->functionTypeIndex());
        m_functionStackSizeSoFar = m_currentFunctionType->paramStackSize();
    }

    virtual void OnLocalDeclCount(Index count) override
    {
        m_currentFunction->m_local.reserve(count);
    }

    virtual void OnLocalDecl(Index decl_index, Index count, Type type) override
    {
        while (count) {
            auto wType = toValueKindForLocalType(type);
            m_currentFunction->m_local.pushBack(wType);
            auto sz = Walrus::valueSizeInStack(wType);
            m_functionStackSizeSoFar += sz;
            m_currentFunction->m_requiredStackSizeDueToLocal += sz;
            count--;
        }
    }

    virtual void OnOpcode(uint32_t opcode) override
    {
    }

    virtual void OnCallExpr(uint32_t index) override
    {
        auto functionType = m_module->functionType(m_module->function(index)->functionIndex());

        size_t stackShrinkSize = functionType->paramStackSize();
        size_t stackGrowSize = functionType->resultStackSize();

        for (size_t i = 0; i < functionType->param().size(); i++) {
            ASSERT(peekVMStack() == Walrus::valueSizeInStack(functionType->param()[functionType->param().size() - i - 1]));
            popVMStack();
        }
        m_currentFunction->pushByteCode(Walrus::Call(index));
        for (size_t i = 0; i < functionType->result().size(); i++) {
            pushVMStack(Walrus::valueSizeInStack(functionType->result()[i]));
        }
    }

    virtual void OnI32ConstExpr(uint32_t value) override
    {
        m_currentFunction->pushByteCode(Walrus::I32Const(value));
        pushVMStack(Walrus::valueSizeInStack(Walrus::Value::Type::I32));
    }

    virtual void OnI64ConstExpr(uint64_t value) override
    {
        m_currentFunction->pushByteCode(Walrus::I64Const(value));
        pushVMStack(Walrus::valueSizeInStack(Walrus::Value::Type::I64));
    }

    virtual void OnF32ConstExpr(uint32_t value) override
    {
        float* f = reinterpret_cast<float*>(&value);
        m_currentFunction->pushByteCode(Walrus::F32Const(*f));
        pushVMStack(Walrus::valueSizeInStack(Walrus::Value::Type::F32));
    }

    virtual void OnF64ConstExpr(uint64_t value) override
    {
        double* f = reinterpret_cast<double*>(&value);
        m_currentFunction->pushByteCode(Walrus::F64Const(*f));
        pushVMStack(Walrus::valueSizeInStack(Walrus::Value::Type::F64));
    }

    std::pair<uint32_t, uint32_t> resolveLocalOffsetAndSize(Index localIndex)
    {
        if (localIndex < m_currentFunctionType->param().size()) {
            size_t offset = 0;
            for (Index i = 0; i < localIndex; i++) {
                offset += Walrus::valueSizeInStack(m_currentFunctionType->param()[i]);
            }
            return std::make_pair(offset, Walrus::valueSizeInStack(m_currentFunctionType->param()[localIndex]));
        } else {
            localIndex -= m_currentFunctionType->param().size();
            size_t offset = m_currentFunctionType->paramStackSize();
            for (Index i = 0; i < localIndex; i++) {
                offset += Walrus::valueSizeInStack(m_currentFunction->m_local[i]);
            }
            return std::make_pair(offset, Walrus::valueSizeInStack(m_currentFunction->m_local[localIndex]));
        }
    }

    virtual void OnLocalGetExpr(Index localIndex) override
    {
        auto r = resolveLocalOffsetAndSize(localIndex);
        if (r.second == 4) {
            m_currentFunction->pushByteCode(Walrus::LocalGet4(r.first));
        } else if (r.second == 8) {
            m_currentFunction->pushByteCode(Walrus::LocalGet8(r.first));
        } else {
            RELEASE_ASSERT_NOT_REACHED();
        }

        pushVMStack(r.second);
    }

    virtual void OnLocalSetExpr(Index localIndex) override
    {
        auto r = resolveLocalOffsetAndSize(localIndex);
        if (r.second == 4) {
            m_currentFunction->pushByteCode(Walrus::LocalSet4(r.first));
        } else if (r.second == 8) {
            m_currentFunction->pushByteCode(Walrus::LocalSet8(r.first));
        } else {
            RELEASE_ASSERT_NOT_REACHED();
        }
        ASSERT(r.second == peekVMStack());
        popVMStack();
    }

    virtual void OnLocalTeeExpr(Index localIndex) override
    {
        auto r = resolveLocalOffsetAndSize(localIndex);
        if (r.second == 4) {
            m_currentFunction->pushByteCode(Walrus::LocalTee4(r.first));
        } else if (r.second == 8) {
            m_currentFunction->pushByteCode(Walrus::LocalTee8(r.first));
        } else {
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    virtual void OnDropExpr() override
    {
        m_currentFunction->pushByteCode(Walrus::Drop(popVMStack()));
    }

    virtual void OnBinaryExpr(uint32_t opcode) override
    {
        auto code = static_cast<Walrus::OpcodeKind>(opcode);
        ASSERT(Walrus::ByteCodeInfo::byteCodeTypeToMemorySize(Walrus::g_byteCodeInfo[code].m_paramTypes[0]) == peekVMStack());
        popVMStack();
        ASSERT(Walrus::ByteCodeInfo::byteCodeTypeToMemorySize(Walrus::g_byteCodeInfo[code].m_paramTypes[1]) == peekVMStack());
        popVMStack();
        pushVMStack(Walrus::ByteCodeInfo::byteCodeTypeToMemorySize(Walrus::g_byteCodeInfo[code].m_resultType));
        m_currentFunction->pushByteCode(Walrus::BinaryOperation(code));
    }

    virtual void OnUnaryExpr(uint32_t opcode) override
    {
        auto code = static_cast<Walrus::OpcodeKind>(opcode);
        ASSERT(Walrus::ByteCodeInfo::byteCodeTypeToMemorySize(Walrus::g_byteCodeInfo[code].m_paramTypes[0]) == peekVMStack());
        popVMStack();
        pushVMStack(Walrus::ByteCodeInfo::byteCodeTypeToMemorySize(Walrus::g_byteCodeInfo[code].m_resultType));

        m_currentFunction->pushByteCode(Walrus::UnaryOperation(code));
    }

    virtual void OnIfExpr(Type sigType) override
    {
        ASSERT(peekVMStack() == Walrus::valueSizeInStack(toValueKindForLocalType(Type::I32)));
        popVMStack();

        BlockInfo b(BlockInfo::IfElse, sigType);
        b.m_position = m_currentFunction->currentByteCodeSize();
        b.m_jumpToEndBrInfo.push_back({ true, b.m_position });
        b.m_stackPushCount = m_vmStack.size();
        m_blockInfo.push_back(b);
        m_currentFunction->pushByteCode(Walrus::JumpIfFalse());

        if (sigType != Type::Void) {
            pushVMStack(Walrus::valueSizeInStack(toValueKindForLocalType(sigType)));
        }
    }

    virtual void OnElseExpr() override
    {
        BlockInfo& blockInfo = m_blockInfo.back();
        blockInfo.m_jumpToEndBrInfo.erase(blockInfo.m_jumpToEndBrInfo.begin());
        blockInfo.m_jumpToEndBrInfo.push_back({ false, m_currentFunction->currentByteCodeSize() });
        m_currentFunction->pushByteCode(Walrus::Jump());
        ASSERT(blockInfo.m_blockType == BlockInfo::IfElse);
        if (blockInfo.m_returnValueType != Type::Void) {
            ASSERT(peekVMStack() == Walrus::valueSizeInStack(toValueKindForLocalType(blockInfo.m_returnValueType)));
            popVMStack();
        }
        m_currentFunction->peekByteCode<Walrus::JumpIfFalse>(blockInfo.m_position)
            ->setOffset(m_currentFunction->currentByteCodeSize() - blockInfo.m_position);
    }

    virtual void OnLoopExpr(Type sigType) override
    {
        BlockInfo b(BlockInfo::Loop, sigType);
        b.m_position = m_currentFunction->currentByteCodeSize();
        b.m_stackPushCount = m_vmStack.size();
        m_blockInfo.push_back(b);

        if (sigType != Type::Void) {
            pushVMStack(Walrus::valueSizeInStack(toValueKindForLocalType(sigType)));
        }
    }

    virtual void OnBlockExpr(Type sigType) override
    {
        BlockInfo b(BlockInfo::Block, sigType);
        b.m_position = m_currentFunction->currentByteCodeSize();
        b.m_stackPushCount = m_vmStack.size();
        m_blockInfo.push_back(b);

        if (sigType != Type::Void) {
            pushVMStack(Walrus::valueSizeInStack(toValueKindForLocalType(sigType)));
        }
    }

    BlockInfo& findBlockInfoInBr(Index depth)
    {
        ASSERT(m_blockInfo.size());
        auto iter = m_blockInfo.rbegin();
        while (depth) {
            iter++;
            depth--;
        }
        return *iter;
    }

    size_t dropStackValuesBeforeBrIfNeeds(Index depth)
    {
        size_t dropValueSize = 0;
        if (depth < m_blockInfo.size()) {
            auto iter = m_blockInfo.rbegin() + depth;

            size_t start = iter->m_stackPushCount;
            for (size_t i = start; i < m_vmStack.size(); i++) {
                dropValueSize += m_vmStack[i];
            }

            if (iter->m_returnValueType != Type::Void) {
                dropValueSize -= Walrus::valueSizeInStack(toValueKindForLocalType(iter->m_returnValueType));
            }
        } else if (m_blockInfo.size()) {
            auto iter = m_blockInfo.begin();
            size_t start = iter->m_stackPushCount;
            for (size_t i = start; i < m_vmStack.size(); i++) {
                dropValueSize += m_vmStack[i];
            }
        }
        return dropValueSize;
    }

    virtual void OnBrExpr(Index depth) override
    {
        if (m_blockInfo.size() == depth) {
            // this case acts like return
            for (size_t i = 0; i < m_currentFunctionType->result().size(); i++) {
                ASSERT(*(m_vmStack.rbegin() + i) == Walrus::valueSizeInStack(m_currentFunctionType->result()[m_currentFunctionType->result().size() - i - 1]));
            }
            m_currentFunction->pushByteCode(Walrus::End());
            auto dropSize = dropStackValuesBeforeBrIfNeeds(depth);
            while (dropSize) {
                dropSize -= popVMStack();
            }

            if (!m_blockInfo.size()) {
                // stop to generate bytecode from here!
                m_shouldContinueToGenerateByteCode = false;
            }
            return;
        }
        auto& blockInfo = findBlockInfoInBr(depth);
        auto offset = (int32_t)blockInfo.m_position - (int32_t)m_currentFunction->currentByteCodeSize();
        auto dropSize = dropStackValuesBeforeBrIfNeeds(depth);
        if (dropSize) {
            m_currentFunction->pushByteCode(Walrus::Drop(dropSize));
        }
        if (blockInfo.m_blockType == BlockInfo::Block) {
            blockInfo.m_jumpToEndBrInfo.push_back({ false, m_currentFunction->currentByteCodeSize() });
        }
        m_currentFunction->pushByteCode(Walrus::Jump(offset));
    }

    virtual void OnBrIfExpr(Index depth) override
    {
        if (m_blockInfo.size() == depth) {
            // this case acts like return
            ASSERT(peekVMStack() == Walrus::valueSizeInStack(toValueKindForLocalType(Type::I32)));
            size_t pos = m_currentFunction->currentByteCodeSize();
            m_currentFunction->pushByteCode(Walrus::JumpIfFalse(sizeof(Walrus::JumpIfFalse) + sizeof(Walrus::End)));
            m_currentFunction->pushByteCode(Walrus::End());
            for (size_t i = 0; i < m_currentFunctionType->result().size(); i++) {
                ASSERT(*(m_vmStack.rbegin() + i) == Walrus::valueSizeInStack(m_currentFunctionType->result()[m_currentFunctionType->result().size() - i - 1]));
            }
            popVMStack();
            return;
        }

        ASSERT(peekVMStack() == Walrus::valueSizeInStack(toValueKindForLocalType(Type::I32)));
        popVMStack();

        auto& blockInfo = findBlockInfoInBr(depth);
        auto dropSize = dropStackValuesBeforeBrIfNeeds(depth);
        if (dropSize) {
            size_t pos = m_currentFunction->currentByteCodeSize();
            m_currentFunction->pushByteCode(Walrus::JumpIfFalse());
            m_currentFunction->pushByteCode(Walrus::Drop(dropSize));
            auto offset = (int32_t)blockInfo.m_position - (int32_t)m_currentFunction->currentByteCodeSize();
            if (blockInfo.m_blockType == BlockInfo::Block) {
                blockInfo.m_jumpToEndBrInfo.push_back({ false, m_currentFunction->currentByteCodeSize() });
            }
            m_currentFunction->pushByteCode(Walrus::Jump(offset));
            m_currentFunction->peekByteCode<Walrus::JumpIfFalse>(pos)
                ->setOffset(m_currentFunction->currentByteCodeSize() - pos);
        } else {
            auto offset = (int32_t)blockInfo.m_position - (int32_t)m_currentFunction->currentByteCodeSize();
            if (blockInfo.m_blockType == BlockInfo::Block) {
                blockInfo.m_jumpToEndBrInfo.push_back({ true, m_currentFunction->currentByteCodeSize() });
            }
            m_currentFunction->pushByteCode(Walrus::JumpIfTrue(offset));
        }
    }

    virtual void OnBrTableExpr(Index numTargets, Index* targetDepths, Index defaultTargetDepth) override
    {
        ASSERT(peekVMStack() == Walrus::valueSizeInStack(toValueKindForLocalType(Type::I32)));
        popVMStack();

        size_t brTableCode = m_currentFunction->currentByteCodeSize();
        m_currentFunction->pushByteCode(Walrus::BrTable(numTargets));

        if (numTargets) {
            m_currentFunction->expandByteCode(sizeof(int32_t) * numTargets);
            std::vector<size_t> offsets;

            for (Index i = 0; i < numTargets; i++) {
                offsets.push_back(m_currentFunction->currentByteCodeSize() - brTableCode);
                OnBrExpr(targetDepths[i]);
            }

            for (Index i = 0; i < numTargets; i++) {
                m_currentFunction->peekByteCode<Walrus::BrTable>(brTableCode)->jumpOffsets()[i] = offsets[i];
            }
        }

        // generate default
        size_t pos = m_currentFunction->currentByteCodeSize();
        OnBrExpr(defaultTargetDepth);
        m_currentFunction->peekByteCode<Walrus::BrTable>(brTableCode)->setDefaultOffset(pos - brTableCode);
    }

    virtual void OnSelectExpr(Index resultCount, Type* resultTypes) override
    {
        // TODO implement selectT
        ASSERT(resultCount == 0);
        ASSERT(peekVMStack() == Walrus::valueSizeInStack(toValueKindForLocalType(Type::I32)));
        popVMStack();

        ASSERT(m_vmStack.back() == *(m_vmStack.rbegin() + 1));
        size_t size = popVMStack();
        popVMStack();

        m_currentFunction->pushByteCode(Walrus::Select(size));
        pushVMStack(size);
    }

    virtual void OnMemoryGrowExpr(Index memidx) override
    {
        ASSERT(peekVMStack() == Walrus::valueSizeInStack(toValueKindForLocalType(Type::I32)));
        popVMStack();
        m_currentFunction->pushByteCode(Walrus::MemoryGrow(memidx));
        pushVMStack(Walrus::valueSizeInStack(Walrus::Value::Type::I32));
    }

    virtual void OnMemorySizeExpr(Index memidx) override
    {
        m_currentFunction->pushByteCode(Walrus::MemorySize(memidx));
        pushVMStack(Walrus::valueSizeInStack(Walrus::Value::Type::I32));
    }

    virtual void OnTableGetExpr(Index table_index) override
    {
        ASSERT(peekVMStack() == Walrus::valueSizeInStack(toValueKindForLocalType(Type::I32)));
        popVMStack();
        m_currentFunction->pushByteCode(Walrus::TableGet(table_index));
        pushVMStack(Walrus::valueSizeInStack(Walrus::Value::Type::FuncRef));
    }

    virtual void OnTableSetExpr(Index table_index) override
    {
        ASSERT(peekVMStack() == Walrus::valueSizeInStack(toValueKindForLocalType(Type::FuncRef)));
        popVMStack();
        ASSERT(peekVMStack() == Walrus::valueSizeInStack(toValueKindForLocalType(Type::I32)));
        popVMStack();
        m_currentFunction->pushByteCode(Walrus::TableSet(table_index));
    }

    virtual void OnNopExpr() override
    {
    }

    virtual void OnReturnExpr() override
    {
        OnBrExpr(m_blockInfo.size());
    }

    virtual void OnEndExpr() override
    {
        if (m_blockInfo.size()) {
            auto blockInfo = m_blockInfo.back();
            m_blockInfo.pop_back();

            if (blockInfo.m_returnValueType != Type::Void) {
                ASSERT(peekVMStack() == Walrus::valueSizeInStack(toValueKindForLocalType(blockInfo.m_returnValueType)));
                popVMStack();
            }

            for (size_t i = 0; i < blockInfo.m_jumpToEndBrInfo.size(); i++) {
                if (blockInfo.m_jumpToEndBrInfo[i].m_isJumpIf) {
                    m_currentFunction->peekByteCode<Walrus::JumpIfFalse>(blockInfo.m_jumpToEndBrInfo[i].m_position)
                        ->setOffset(m_currentFunction->currentByteCodeSize() - blockInfo.m_jumpToEndBrInfo[i].m_position);
                } else {
                    m_currentFunction->peekByteCode<Walrus::Jump>(blockInfo.m_jumpToEndBrInfo[i].m_position)->setOffset(m_currentFunction->currentByteCodeSize() - blockInfo.m_jumpToEndBrInfo[i].m_position);
                }
            }

        } else {
            m_currentFunction->pushByteCode(Walrus::End());
        }
    }

    virtual void EndFunctionBody(Index index) override
    {
#if !defined(NDEBUG)
        if (getenv("DUMP_BYTECODE") && strlen(getenv("DUMP_BYTECODE"))) {
            m_currentFunction->dumpByteCode();
        }
        for (size_t i = 0; i < m_currentFunctionType->result().size(); i++) {
            ASSERT(popVMStack() == Walrus::valueSizeInStack(m_currentFunctionType->result()[m_currentFunctionType->result().size() - i - 1]));
        }
        ASSERT(m_vmStack.empty());
#endif

        ASSERT(m_currentFunction == m_module->function(index));
        m_currentFunction = nullptr;
        m_currentFunctionType = nullptr;
        m_vmStack.clear();
    }

private:
    void pushVMStack(size_t s)
    {
        m_vmStack.push_back(s);
        m_functionStackSizeSoFar += s;
        m_currentFunction->m_requiredStackSize = std::max(
            m_currentFunction->m_requiredStackSize, m_functionStackSizeSoFar);
    }

    size_t popVMStack()
    {
        auto s = m_vmStack.back();
        m_functionStackSizeSoFar -= s;
        m_vmStack.pop_back();
        return s;
    }

    size_t peekVMStack()
    {
        return m_vmStack.back();
    }

    Walrus::Module* m_module;
    Walrus::ModuleFunction* m_currentFunction;
    Walrus::FunctionType* m_currentFunctionType;
    uint32_t m_functionStackSizeSoFar;
    std::vector<unsigned char> m_vmStack;
    std::vector<BlockInfo> m_blockInfo;
};

} // namespace wabt

namespace Walrus {

Optional<Module*> WASMParser::parseBinary(Store* store, const uint8_t* data, size_t len)
{
    Module* module = new Module(store);
    wabt::WASMBinaryReader delegate(module);

    ReadWasmBinary(data, len, &delegate);
    return module;
}

} // namespace Walrus
