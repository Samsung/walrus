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

static Walrus::Value::Type toValueKind(Type type)
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
    case Type::ExternRef:
        return Walrus::Value::Type::ExternRef;
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
}

static Walrus::SegmentMode toSegmentMode(uint8_t flags)
{
    enum SegmentFlags : uint8_t {
        SegFlagsNone = 0,
        SegPassive = 1, // bit 0: Is passive
        SegExplicitIndex = 2, // bit 1: Has explict index (Implies table 0 if absent)
        SegDeclared = 3, // Only used for declared segments
        SegUseElemExprs = 4, // bit 2: Is elemexpr (Or else index sequence)
        SegFlagMax = (SegUseElemExprs << 1) - 1, // All bits set.
    };

    if ((flags & SegDeclared) == SegDeclared) {
        return Walrus::SegmentMode::Declared;
    } else if ((flags & SegPassive) == SegPassive) {
        return Walrus::SegmentMode::Passive;
    } else {
        return Walrus::SegmentMode::Active;
    }
}

class WASMBinaryReader : public wabt::WASMBinaryReaderDelegate {
private:
    struct VMStackInfo {
        size_t m_size;
        size_t m_position;
    };

    struct BlockInfo {
        enum BlockType {
            IfElse,
            Loop,
            Block,
            TryCatch,
        };
        BlockType m_blockType;
        Type m_returnValueType;
        size_t m_position;
        std::vector<VMStackInfo> m_vmStack;
        bool m_shouldRestoreVMStackAtEnd;

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
            , m_shouldRestoreVMStackAtEnd(false)
        {
        }
    };

    Walrus::Module* m_module;
    Walrus::ModuleFunction* m_currentFunction;
    Walrus::FunctionType* m_currentFunctionType;
    uint32_t m_initialFunctionStackSize;
    uint32_t m_functionStackSizeSoFar;
    std::vector<VMStackInfo> m_vmStack;
    std::vector<BlockInfo> m_blockInfo;
    struct CatchInfo {
        size_t m_tryCatchBlockDepth;
        size_t m_tryStart;
        size_t m_tryEnd;
        size_t m_catchStart;
        uint32_t m_tagIndex;
    };
    std::vector<CatchInfo> m_catchInfo;
    Walrus::Vector<uint8_t, GCUtil::gc_malloc_atomic_allocator<uint8_t>> m_memoryInitData;

    uint32_t m_elementTableIndex;
    Walrus::Optional<Walrus::ModuleFunction*> m_elementModuleFunction;
    Walrus::Vector<uint32_t, GCUtil::gc_malloc_atomic_allocator<uint32_t>> m_elementFunctionIndex;
    Walrus::SegmentMode m_segmentMode;

    size_t pushVMStack(size_t size)
    {
        auto pos = m_functionStackSizeSoFar;
        pushVMStack(size, pos);
        return pos;
    }

    void pushVMStack(size_t size, size_t pos)
    {
        m_vmStack.push_back({ size, pos });
        if (pos == m_functionStackSizeSoFar) {
            m_functionStackSizeSoFar += size;
            m_currentFunction->m_requiredStackSize = std::max(
                m_currentFunction->m_requiredStackSize, m_functionStackSizeSoFar);
        }
    }

    size_t popVMStackSize()
    {
        auto info = m_vmStack.back();
        if (info.m_position == m_functionStackSizeSoFar - info.m_size) {
            m_functionStackSizeSoFar -= info.m_size;
        }
        m_vmStack.pop_back();
        return info.m_size;
    }

    size_t popVMStack()
    {
        auto info = m_vmStack.back();
        if (info.m_position == m_functionStackSizeSoFar - info.m_size) {
            m_functionStackSizeSoFar -= info.m_size;
        }
        m_vmStack.pop_back();
        return info.m_position;
    }

    size_t peekVMStackSize()
    {
        return m_vmStack.back().m_size;
    }

    size_t peekVMStack()
    {
        return m_vmStack.back().m_position;
    }

    void beginFunction(Walrus::ModuleFunction* mf)
    {
        m_currentFunction = mf;
        m_currentFunctionType = mf->functionType();
        m_initialFunctionStackSize = m_functionStackSizeSoFar = m_currentFunctionType->paramStackSize();
        m_currentFunction->m_requiredStackSize = std::max(
            m_currentFunction->m_requiredStackSize, m_functionStackSizeSoFar);
    }

    void endFunction()
    {
        m_currentFunction = nullptr;
        m_currentFunctionType = nullptr;
        m_vmStack.clear();
        m_shouldContinueToGenerateByteCode = true;
    }

public:
    WASMBinaryReader(Walrus::Module* module)
        : m_module(module)
        , m_currentFunction(nullptr)
        , m_currentFunctionType(nullptr)
        , m_initialFunctionStackSize(0)
        , m_functionStackSizeSoFar(0)
        , m_elementTableIndex(0)
        , m_segmentMode(Walrus::SegmentMode::None)
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
        Walrus::ValueTypeVector* param = new Walrus::ValueTypeVector();
        param->reserve(paramCount);
        for (size_t i = 0; i < paramCount; i++) {
            param->push_back(toValueKind(paramTypes[i]));
        }
        Walrus::ValueTypeVector* result = new Walrus::ValueTypeVector();
        for (size_t i = 0; i < resultCount; i++) {
            result->push_back(toValueKind(resultTypes[i]));
        }
        ASSERT(index == m_module->m_functionTypes.size());
        m_module->m_functionTypes.push_back(Walrus::FunctionType(param, result));
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
        ASSERT(m_module->m_function.size() == funcIndex);
        ASSERT(m_module->m_import.size() == importIndex);
        m_module->m_function.push_back(
            new Walrus::ModuleFunction(m_module, m_module->functionType(sigIndex)));
        m_module->m_import.push_back(new Walrus::ModuleImport(
            new Walrus::String(moduleName),
            new Walrus::String(fieldName), funcIndex, sigIndex));
    }

    virtual void OnImportGlobal(Index importIndex, std::string moduleName, std::string fieldName, Index globalIndex, Type type, bool mutable_) override
    {
        ASSERT(m_module->m_global.size() == globalIndex);
        ASSERT(m_module->m_import.size() == importIndex);
        m_module->m_global.pushBack(std::make_pair(Walrus::GlobalType(toValueKind(type), mutable_), nullptr));
        m_module->m_import.push_back(new Walrus::ModuleImport(
            Walrus::ModuleImport::Global,
            new Walrus::String(moduleName),
            new Walrus::String(fieldName), globalIndex));
    }

    virtual void OnImportTable(Index importIndex, std::string moduleName, std::string fieldName, Index tableIndex, Type type, size_t initialSize, size_t maximumSize) override
    {
        ASSERT(tableIndex == m_module->m_tableTypes.size());
        ASSERT(m_module->m_import.size() == importIndex);
        ASSERT(type == Type::FuncRef || type == Type::ExternRef);

        m_module->m_tableTypes.pushBack(Walrus::TableType(type == Type::FuncRef ? Walrus::Value::Type::FuncRef : Walrus::Value::Type::ExternRef, initialSize, maximumSize));
        m_module->m_import.push_back(new Walrus::ModuleImport(
            Walrus::ModuleImport::Table,
            new Walrus::String(moduleName),
            new Walrus::String(fieldName), tableIndex,
            initialSize, maximumSize, toValueKind(type)));
    }

    virtual void OnImportMemory(Index importIndex, std::string moduleName, std::string fieldName, Index memoryIndex, size_t initialSize, size_t maximumSize) override
    {
        ASSERT(memoryIndex == m_module->m_memoryTypes.size());
        ASSERT(m_module->m_import.size() == importIndex);
        m_module->m_memoryTypes.pushBack(Walrus::MemoryType(initialSize, maximumSize));
        m_module->m_import.push_back(new Walrus::ModuleImport(
            Walrus::ModuleImport::Memory,
            new Walrus::String(moduleName),
            new Walrus::String(fieldName), memoryIndex,
            initialSize, maximumSize));
    }

    virtual void OnImportTag(Index importIndex, std::string moduleName, std::string fieldName, Index tagIndex, Index sigIndex) override
    {
        ASSERT(tagIndex == m_module->m_tagTypes.size());
        ASSERT(m_module->m_import.size() == importIndex);
        m_module->m_tagTypes.pushBack(Walrus::TagType(sigIndex));
        m_module->m_import.push_back(new Walrus::ModuleImport(
            Walrus::ModuleImport::Tag,
            new Walrus::String(moduleName),
            new Walrus::String(fieldName), sigIndex));
    }

    virtual void OnExportCount(Index count) override
    {
        m_module->m_export.reserve(count);
    }

    virtual void OnExport(int kind, Index exportIndex, std::string name, Index itemIndex) override
    {
        ASSERT(m_module->m_export.size() == exportIndex);
        m_module->m_export.pushBack(new Walrus::ModuleExport(static_cast<Walrus::ModuleExport::Type>(kind), new Walrus::String(name), itemIndex));
    }

    /* Table section */
    virtual void OnTableCount(Index count) override
    {
        m_module->m_tableTypes.reserve(count);
    }

    virtual void OnTable(Index index, Type type, size_t initialSize, size_t maximumSize) override
    {
        ASSERT(index == m_module->m_tableTypes.size());
        ASSERT(type == Type::FuncRef || type == Type::ExternRef);
        m_module->m_tableTypes.pushBack(Walrus::TableType(type == Type::FuncRef ? Walrus::Value::Type::FuncRef : Walrus::Value::Type::ExternRef, initialSize, maximumSize));
    }

    virtual void OnElemSegmentCount(Index count) override
    {
        m_module->m_element.reserve(count);
    }

    virtual void BeginElemSegment(Index index, Index tableIndex, uint8_t flags) override
    {
        m_elementTableIndex = tableIndex;
        m_elementModuleFunction = nullptr;
        m_segmentMode = toSegmentMode(flags);
    }

    virtual void BeginElemSegmentInitExpr(Index index) override
    {
        beginFunction(new Walrus::ModuleFunction(m_module, Walrus::Module::initIndexFunctionType()));
    }

    virtual void EndElemSegmentInitExpr(Index index) override
    {
        m_elementModuleFunction = m_currentFunction;
        endFunction();
    }

    virtual void OnElemSegmentElemType(Index index, Type elemType) override
    {
    }

    virtual void OnElemSegmentElemExprCount(Index index, Index count) override
    {
        m_elementFunctionIndex.reserve(count);
    }

    virtual void OnElemSegmentElemExpr_RefNull(Index segmentIndex, Type type) override
    {
        m_elementFunctionIndex.pushBack(std::numeric_limits<uint32_t>::max());
    }

    virtual void OnElemSegmentElemExpr_RefFunc(Index segmentIndex, Index funcIndex) override
    {
        m_elementFunctionIndex.pushBack(funcIndex);
    }

    virtual void EndElemSegment(Index index) override
    {
        ASSERT(m_module->m_element.size() == index);
        if (m_elementModuleFunction) {
            m_module->m_element.pushBack(new Walrus::Element(m_segmentMode, m_elementTableIndex, m_elementModuleFunction.value(), std::move(m_elementFunctionIndex)));
        } else {
            m_module->m_element.pushBack(new Walrus::Element(m_segmentMode, m_elementTableIndex, std::move(m_elementFunctionIndex)));
        }

        m_elementModuleFunction = nullptr;
        m_elementTableIndex = 0;
        m_elementFunctionIndex.clear();
        m_segmentMode = Walrus::SegmentMode::None;
    }

    /* Memory section */
    virtual void OnMemoryCount(Index count) override
    {
        m_module->m_memoryTypes.reserve(count);
    }

    virtual void OnMemory(Index index, size_t initialSize, size_t maximumSize) override
    {
        ASSERT(index == m_module->m_memoryTypes.size());
        m_module->m_memoryTypes.pushBack(Walrus::MemoryType(initialSize, maximumSize));
    }

    virtual void OnDataSegmentCount(Index count) override
    {
        m_module->m_data.reserve(count);
    }

    virtual void BeginDataSegment(Index index, Index memoryIndex, uint8_t flags) override
    {
        ASSERT(index == m_module->m_data.size());
        beginFunction(new Walrus::ModuleFunction(m_module, Walrus::Module::initIndexFunctionType()));
    }

    virtual void BeginDataSegmentInitExpr(Index index) override
    {
    }

    virtual void EndDataSegmentInitExpr(Index index) override
    {
    }

    virtual void OnDataSegmentData(Index index, const void* data, Address size) override
    {
        m_memoryInitData.resizeWithUninitializedValues(size);
        memcpy(m_memoryInitData.data(), data, size);
    }

    virtual void EndDataSegment(Index index) override
    {
        m_module->m_data.pushBack(new Walrus::Data(m_currentFunction, std::move(m_memoryInitData)));
        endFunction();
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
        ASSERT(m_module->m_function.size() == index);
        m_module->m_function.push_back(new Walrus::ModuleFunction(m_module, m_module->functionType(sigIndex)));
    }

    virtual void OnGlobalCount(Index count) override
    {
        m_module->m_global.reserve(count);
    }

    virtual void BeginGlobal(Index index, Type type, bool mutable_) override
    {
        ASSERT(m_module->m_global.size() == index);
        m_module->m_global.pushBack(std::make_pair(Walrus::GlobalType(toValueKind(type), mutable_), nullptr));
    }

    virtual void BeginGlobalInitExpr(Index index) override
    {
        auto ft = Walrus::Module::initGlobalFunctionType(m_module->m_global[index].first.type());
        m_module->m_global[index].second = new Walrus::ModuleFunction(m_module, ft);
        beginFunction(m_module->m_global[index].second.value());
    }

    virtual void EndGlobalInitExpr(Index index) override
    {
        endFunction();
    }

    virtual void EndGlobal(Index index) override
    {
    }

    virtual void EndGlobalSection() override
    {
    }

    virtual void OnTagCount(Index count) override
    {
        m_module->m_tagTypes.reserve(count);
    }

    virtual void OnTagType(Index index, Index sigIndex) override
    {
        ASSERT(index == m_module->m_tagTypes.size());
        m_module->m_tagTypes.pushBack(Walrus::TagType(sigIndex));
    }

    virtual void OnStartFunction(Index funcIndex) override
    {
        m_module->m_seenStartAttribute = true;
        m_module->m_start = funcIndex;
    }

    virtual void BeginFunctionBody(Index index, Offset size) override
    {
        ASSERT(m_currentFunction == nullptr);
        beginFunction(m_module->function(index));
    }

    virtual void OnLocalDeclCount(Index count) override
    {
        m_currentFunction->m_local.reserve(count);
    }

    virtual void OnLocalDecl(Index decl_index, Index count, Type type) override
    {
        while (count) {
            auto wType = toValueKind(type);
            m_currentFunction->m_local.pushBack(wType);
            auto sz = Walrus::valueSizeInStack(wType);
            m_initialFunctionStackSize += sz;
            m_functionStackSizeSoFar += sz;
            m_currentFunction->m_requiredStackSizeDueToLocal += sz;
            count--;
        }
        m_currentFunction->m_requiredStackSize = std::max(
            m_currentFunction->m_requiredStackSize, m_functionStackSizeSoFar);
    }

    virtual void OnOpcode(uint32_t opcode) override
    {
    }

    virtual void OnCallExpr(uint32_t index) override
    {
        auto functionType = m_module->function(index)->functionType();
        auto callPos = m_currentFunction->currentByteCodeSize();
        m_currentFunction->pushByteCode(Walrus::Call(index
#if !defined(NDEBUG)
                                                     ,
                                                     functionType
#endif
                                                     ));

        m_currentFunction->expandByteCode(sizeof(uint32_t) * (functionType->param().size() + functionType->result().size()));
        auto code = m_currentFunction->peekByteCode<Walrus::Call>(callPos);

        size_t c = 0;
        size_t siz = functionType->param().size();
        for (size_t i = 0; i < siz; i++) {
            ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(functionType->param()[siz - i - 1]));
            code->stackOffsets()[siz - c++ - 1] = popVMStack();
        }
        siz = functionType->result().size();
        for (size_t i = 0; i < siz; i++) {
            code->stackOffsets()[c++] = pushVMStack(Walrus::valueSizeInStack(functionType->result()[i]));
        }
    }

    virtual void OnCallIndirectExpr(Index sigIndex, Index tableIndex) override
    {
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
        auto functionType = m_module->functionType(sigIndex);
        auto callPos = m_currentFunction->currentByteCodeSize();
        m_currentFunction->pushByteCode(Walrus::CallIndirect(popVMStack(), tableIndex, functionType));
        m_currentFunction->expandByteCode(sizeof(uint32_t) * (functionType->param().size() + functionType->result().size()));

        auto code = m_currentFunction->peekByteCode<Walrus::CallIndirect>(callPos);

        size_t c = 0;
        size_t siz = functionType->param().size();
        for (size_t i = 0; i < siz; i++) {
            ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(functionType->param()[siz - i - 1]));
            code->stackOffsets()[siz - c++ - 1] = popVMStack();
        }
        siz = functionType->result().size();
        for (size_t i = 0; i < siz; i++) {
            code->stackOffsets()[c++] = pushVMStack(Walrus::valueSizeInStack(functionType->result()[i]));
        }
    }

    virtual void OnI32ConstExpr(uint32_t value) override
    {
        m_currentFunction->pushByteCode(Walrus::Const32(pushVMStack(Walrus::valueSizeInStack(Walrus::Value::Type::I32)), value));
    }

    virtual void OnI64ConstExpr(uint64_t value) override
    {
        m_currentFunction->pushByteCode(Walrus::Const64(pushVMStack(Walrus::valueSizeInStack(Walrus::Value::Type::I64)), value));
    }

    virtual void OnF32ConstExpr(uint32_t value) override
    {
        m_currentFunction->pushByteCode(Walrus::Const32(pushVMStack(Walrus::valueSizeInStack(Walrus::Value::Type::F32)), value));
    }

    virtual void OnF64ConstExpr(uint64_t value) override
    {
        m_currentFunction->pushByteCode(Walrus::Const64(pushVMStack(Walrus::valueSizeInStack(Walrus::Value::Type::F64)), value));
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
        auto offset = pushVMStack(r.second);
        if (r.second == 4) {
            m_currentFunction->pushByteCode(Walrus::LocalGet32(offset, r.first));
        } else if (r.second == 8) {
            m_currentFunction->pushByteCode(Walrus::LocalGet64(offset, r.first));
        } else {
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    virtual void OnLocalSetExpr(Index localIndex) override
    {
        auto r = resolveLocalOffsetAndSize(localIndex);
        ASSERT(r.second == peekVMStackSize());
        auto offset = popVMStack();

        if (r.second == 4) {
            m_currentFunction->pushByteCode(Walrus::LocalSet32(offset, r.first));
        } else if (r.second == 8) {
            m_currentFunction->pushByteCode(Walrus::LocalSet64(offset, r.first));
        } else {
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    virtual void OnLocalTeeExpr(Index localIndex) override
    {
        auto r = resolveLocalOffsetAndSize(localIndex);
        ASSERT(r.second == peekVMStackSize());
        auto offset = peekVMStack();

        if (r.second == 4) {
            m_currentFunction->pushByteCode(Walrus::LocalSet32(offset, r.first));
        } else if (r.second == 8) {
            m_currentFunction->pushByteCode(Walrus::LocalSet64(offset, r.first));
        } else {
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    virtual void OnGlobalGetExpr(Index index) override
    {
        auto sz = Walrus::valueSizeInStack(m_module->m_global[index].first.type());
        auto stackPos = pushVMStack(sz);
        if (sz == 4) {
            m_currentFunction->pushByteCode(Walrus::GlobalGet32(stackPos, index));
        } else {
            ASSERT(sz == 8);
            m_currentFunction->pushByteCode(Walrus::GlobalGet64(stackPos, index));
        }
    }

    virtual void OnGlobalSetExpr(Index index) override
    {
        auto stackPos = peekVMStack();

        auto sz = Walrus::valueSizeInStack(m_module->m_global[index].first.type());
        if (sz == 4) {
            ASSERT(peekVMStackSize() == 4);
            m_currentFunction->pushByteCode(Walrus::GlobalSet32(stackPos, index));
        } else {
            ASSERT(sz == 8);
            ASSERT(peekVMStackSize() == 8);
            m_currentFunction->pushByteCode(Walrus::GlobalSet64(stackPos, index));
        }
        popVMStack();
    }

    virtual void OnDropExpr() override
    {
        popVMStack();
    }

    virtual void OnBinaryExpr(uint32_t opcode) override
    {
        auto code = static_cast<Walrus::OpcodeKind>(opcode);
        ASSERT(Walrus::ByteCodeInfo::byteCodeTypeToMemorySize(Walrus::g_byteCodeInfo[code].m_paramTypes[0]) == peekVMStackSize());
        auto src1 = popVMStack();
        ASSERT(Walrus::ByteCodeInfo::byteCodeTypeToMemorySize(Walrus::g_byteCodeInfo[code].m_paramTypes[1]) == peekVMStackSize());
        auto src0 = popVMStack();
        auto dst = pushVMStack(Walrus::ByteCodeInfo::byteCodeTypeToMemorySize(Walrus::g_byteCodeInfo[code].m_resultType));
        m_currentFunction->pushByteCode(Walrus::BinaryOperation(code, src0, src1, dst));
    }

    virtual void OnUnaryExpr(uint32_t opcode) override
    {
        auto code = static_cast<Walrus::OpcodeKind>(opcode);
        ASSERT(Walrus::ByteCodeInfo::byteCodeTypeToMemorySize(Walrus::g_byteCodeInfo[code].m_paramTypes[0]) == peekVMStackSize());
        auto src = popVMStack();
        auto dst = pushVMStack(Walrus::ByteCodeInfo::byteCodeTypeToMemorySize(Walrus::g_byteCodeInfo[code].m_resultType));
        m_currentFunction->pushByteCode(Walrus::UnaryOperation(code, src, dst));
    }

    virtual void OnIfExpr(Type sigType) override
    {
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
        auto stackPos = popVMStack();

        BlockInfo b(BlockInfo::IfElse, sigType);
        b.m_position = m_currentFunction->currentByteCodeSize();
        b.m_jumpToEndBrInfo.push_back({ true, b.m_position });
        b.m_vmStack = m_vmStack;
        m_blockInfo.push_back(b);
        m_currentFunction->pushByteCode(Walrus::JumpIfFalse(stackPos));
    }

    void restoreVMStackBy(const std::vector<VMStackInfo>& src)
    {
        m_vmStack = src;
        m_functionStackSizeSoFar = m_initialFunctionStackSize;
        for (auto s : m_vmStack) {
            m_functionStackSizeSoFar += s.m_size;
        }
    }

    void restoreVMStackRegardToPartOfBlockEnd(const BlockInfo& blockInfo)
    {
        if (blockInfo.m_shouldRestoreVMStackAtEnd) {
            restoreVMStackBy(blockInfo.m_vmStack);
        } else if (blockInfo.m_returnValueType.IsIndex()) {
            auto ft = m_module->functionType(blockInfo.m_returnValueType);
            if (ft->param().size()) {
                restoreVMStackBy(blockInfo.m_vmStack);
            } else {
                const auto& result = ft->result();
                for (size_t i = 0; i < result.size(); i++) {
                    ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(result[result.size() - i - 1]));
                    popVMStackSize();
                }
            }
        } else if (blockInfo.m_returnValueType != Type::Void) {
            ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(blockInfo.m_returnValueType)));
            popVMStackSize();
        }
    }

    virtual void OnElseExpr() override
    {
        BlockInfo& blockInfo = m_blockInfo.back();
        blockInfo.m_jumpToEndBrInfo.erase(blockInfo.m_jumpToEndBrInfo.begin());
        blockInfo.m_jumpToEndBrInfo.push_back({ false, m_currentFunction->currentByteCodeSize() });
        m_currentFunction->pushByteCode(Walrus::Jump());
        ASSERT(blockInfo.m_blockType == BlockInfo::IfElse);
        restoreVMStackRegardToPartOfBlockEnd(blockInfo);
        m_currentFunction->peekByteCode<Walrus::JumpIfFalse>(blockInfo.m_position)
            ->setOffset(m_currentFunction->currentByteCodeSize() - blockInfo.m_position);
    }

    virtual void OnLoopExpr(Type sigType) override
    {
        BlockInfo b(BlockInfo::Loop, sigType);
        b.m_position = m_currentFunction->currentByteCodeSize();
        b.m_vmStack = m_vmStack;
        m_blockInfo.push_back(b);
    }

    virtual void OnBlockExpr(Type sigType) override
    {
        BlockInfo b(BlockInfo::Block, sigType);
        b.m_position = m_currentFunction->currentByteCodeSize();
        b.m_vmStack = m_vmStack;
        m_blockInfo.push_back(b);
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

    void stopToGenerateByteCodeWhileBlockEnd()
    {
        if (m_resumeGenerateByteCodeAfterNBlockEnd) {
            return;
        }

        if (m_blockInfo.size()) {
            m_resumeGenerateByteCodeAfterNBlockEnd = 1;
            m_blockInfo.back().m_shouldRestoreVMStackAtEnd = true;
        }
        m_shouldContinueToGenerateByteCode = false;
    }

    // return drop size, parameter size
    std::pair<size_t, size_t> dropStackValuesBeforeBrIfNeeds(Index depth)
    {
        size_t dropValueSize = 0;
        size_t parameterSize = 0;
        if (depth < m_blockInfo.size()) {
            auto iter = m_blockInfo.rbegin() + depth;
            if (iter->m_vmStack.size() < m_vmStack.size()) {
                size_t start = iter->m_vmStack.size();
                for (size_t i = start; i < m_vmStack.size(); i++) {
                    dropValueSize += m_vmStack[i].m_size;
                }

                if (iter->m_blockType == BlockInfo::Loop) {
                    if (iter->m_returnValueType.IsIndex()) {
                        auto ft = m_module->functionType(iter->m_returnValueType);
                        dropValueSize += ft->paramStackSize();
                        parameterSize += ft->paramStackSize();
                    }
                } else {
                    if (iter->m_returnValueType.IsIndex()) {
                        auto ft = m_module->functionType(iter->m_returnValueType);
                        const auto& result = ft->result();
                        for (size_t i = 0; i < result.size(); i++) {
                            parameterSize += Walrus::valueSizeInStack(result[i]);
                        }
                    } else if (iter->m_returnValueType != Type::Void) {
                        parameterSize += Walrus::valueSizeInStack(toValueKind(iter->m_returnValueType));
                    }
                }
            }
        } else if (m_blockInfo.size()) {
            auto iter = m_blockInfo.begin();
            size_t start = iter->m_vmStack.size();
            for (size_t i = start; i < m_vmStack.size(); i++) {
                dropValueSize += m_vmStack[i].m_size;
            }
        }

        if (dropValueSize == parameterSize) {
            dropValueSize = 0;
            parameterSize = 0;
        }

        return std::make_pair(dropValueSize, parameterSize);
    }

    void generateEndCode()
    {
        if (UNLIKELY(m_currentFunctionType->result().size() > m_vmStack.size())) {
            // error case of global init expr
            return;
        }
        auto pos = m_currentFunction->currentByteCodeSize();
        m_currentFunction->pushByteCode(Walrus::End(
#if !defined(NDEBUG)
            m_currentFunctionType
#endif
            ));

        auto& result = m_currentFunctionType->result();
        m_currentFunction->expandByteCode(sizeof(uint32_t) * result.size());
        Walrus::End* end = m_currentFunction->peekByteCode<Walrus::End>(pos);
        for (size_t i = 0; i < result.size(); i++) {
            end->resultOffsets()[result.size() - i - 1] = (m_vmStack.rbegin() + i)->m_position;
        }
    }

    void generateFunctionReturnCode(bool shouldClearVMStack = false)
    {
        for (size_t i = 0; i < m_currentFunctionType->result().size(); i++) {
            ASSERT((m_vmStack.rbegin() + i)->m_size == Walrus::valueSizeInStack(m_currentFunctionType->result()[m_currentFunctionType->result().size() - i - 1]));
        }
        generateEndCode();
        if (shouldClearVMStack) {
            auto dropSize = dropStackValuesBeforeBrIfNeeds(m_blockInfo.size()).first;
            while (dropSize) {
                dropSize -= popVMStackSize();
            }
        } else {
            for (size_t i = 0; i < m_currentFunctionType->result().size(); i++) {
                popVMStackSize();
            }
            stopToGenerateByteCodeWhileBlockEnd();
        }

        if (!m_blockInfo.size()) {
            // stop to generate bytecode from here!
            m_shouldContinueToGenerateByteCode = false;
            m_resumeGenerateByteCodeAfterNBlockEnd = 0;
        }
    }

    virtual void OnBrExpr(Index depth) override
    {
        if (m_blockInfo.size() == depth) {
            // this case acts like return
            generateFunctionReturnCode(true);
            return;
        }
        auto& blockInfo = findBlockInfoInBr(depth);
        auto offset = (int32_t)blockInfo.m_position - (int32_t)m_currentFunction->currentByteCodeSize();
        auto dropSize = dropStackValuesBeforeBrIfNeeds(depth);
        if (dropSize.first) {
            m_currentFunction->pushByteCode(Walrus::Drop(m_functionStackSizeSoFar, dropSize.first, dropSize.second));
        }
        if (blockInfo.m_blockType == BlockInfo::Block || blockInfo.m_blockType == BlockInfo::IfElse) {
            blockInfo.m_jumpToEndBrInfo.push_back({ false, m_currentFunction->currentByteCodeSize() });
        }
        m_currentFunction->pushByteCode(Walrus::Jump(offset));

        stopToGenerateByteCodeWhileBlockEnd();
    }

    virtual void OnBrIfExpr(Index depth) override
    {
        if (m_blockInfo.size() == depth) {
            // this case acts like return
            ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
            auto stackPos = popVMStack();
            size_t pos = m_currentFunction->currentByteCodeSize();
            m_currentFunction->pushByteCode(Walrus::JumpIfFalse(stackPos, sizeof(Walrus::JumpIfFalse) + sizeof(Walrus::End) + sizeof(uint32_t) * m_currentFunctionType->result().size()));
            for (size_t i = 0; i < m_currentFunctionType->result().size(); i++) {
                ASSERT((m_vmStack.rbegin() + i)->m_size == Walrus::valueSizeInStack(m_currentFunctionType->result()[m_currentFunctionType->result().size() - i - 1]));
            }
            generateEndCode();
            return;
        }

        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
        auto stackPos = popVMStack();

        auto& blockInfo = findBlockInfoInBr(depth);
        auto dropSize = dropStackValuesBeforeBrIfNeeds(depth);
        if (dropSize.first) {
            size_t pos = m_currentFunction->currentByteCodeSize();
            m_currentFunction->pushByteCode(Walrus::JumpIfFalse(stackPos));
            m_currentFunction->pushByteCode(Walrus::Drop(stackPos, dropSize.first, dropSize.second));
            auto offset = (int32_t)blockInfo.m_position - (int32_t)m_currentFunction->currentByteCodeSize();
            if (blockInfo.m_blockType == BlockInfo::Block || blockInfo.m_blockType == BlockInfo::IfElse) {
                blockInfo.m_jumpToEndBrInfo.push_back({ false, m_currentFunction->currentByteCodeSize() });
            }
            m_currentFunction->pushByteCode(Walrus::Jump(offset));
            m_currentFunction->peekByteCode<Walrus::JumpIfFalse>(pos)
                ->setOffset(m_currentFunction->currentByteCodeSize() - pos);
        } else {
            auto offset = (int32_t)blockInfo.m_position - (int32_t)m_currentFunction->currentByteCodeSize();
            if (blockInfo.m_blockType == BlockInfo::Block || blockInfo.m_blockType == BlockInfo::IfElse) {
                blockInfo.m_jumpToEndBrInfo.push_back({ true, m_currentFunction->currentByteCodeSize() });
            }
            m_currentFunction->pushByteCode(Walrus::JumpIfTrue(stackPos, offset));
        }
    }

    virtual void OnBrTableExpr(Index numTargets, Index* targetDepths, Index defaultTargetDepth) override
    {
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
        auto stackPos = popVMStack();

        size_t brTableCode = m_currentFunction->currentByteCodeSize();
        m_currentFunction->pushByteCode(Walrus::BrTable(stackPos, numTargets));

        if (numTargets) {
            m_currentFunction->expandByteCode(sizeof(int32_t) * numTargets);
            std::vector<size_t> offsets;

            for (Index i = 0; i < numTargets; i++) {
                offsets.push_back(m_currentFunction->currentByteCodeSize() - brTableCode);
                if (m_blockInfo.size() == targetDepths[i]) {
                    // this case acts like return
                    for (size_t i = 0; i < m_currentFunctionType->result().size(); i++) {
                        ASSERT((m_vmStack.rbegin() + i)->m_size == Walrus::valueSizeInStack(m_currentFunctionType->result()[m_currentFunctionType->result().size() - i - 1]));
                    }
                    generateEndCode();
                } else {
                    OnBrExpr(targetDepths[i]);
                }
            }

            for (Index i = 0; i < numTargets; i++) {
                m_currentFunction->peekByteCode<Walrus::BrTable>(brTableCode)->jumpOffsets()[i] = offsets[i];
            }
        }

        // generate default
        size_t pos = m_currentFunction->currentByteCodeSize();
        OnBrExpr(defaultTargetDepth);
        m_currentFunction->peekByteCode<Walrus::BrTable>(brTableCode)->setDefaultOffset(pos - brTableCode);

        stopToGenerateByteCodeWhileBlockEnd();
    }

    virtual void OnSelectExpr(Index resultCount, Type* resultTypes) override
    {
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
        ASSERT(resultCount == 0 || resultCount == 1);
        auto stackPos = popVMStack();

        auto size = peekVMStackSize();
        auto src1 = popVMStack();
        auto src0 = popVMStack();
        auto dst = pushVMStack(size);
        m_currentFunction->pushByteCode(Walrus::Select(stackPos, size, src0, src1, dst));
    }

    virtual void OnThrowExpr(Index tagIndex) override
    {
        auto pos = m_currentFunction->currentByteCodeSize();
        m_currentFunction->pushByteCode(Walrus::Throw(tagIndex
#if !defined(NDEBUG)
                                                      ,
                                                      tagIndex != std::numeric_limits<Index>::max() ? m_module->functionType(m_module->m_tagTypes[tagIndex].sigIndex()) : nullptr
#endif
                                                      ));

        if (tagIndex != std::numeric_limits<Index>::max()) {
            auto functionType = m_module->functionType(m_module->m_tagTypes[tagIndex].sigIndex());
            auto& param = functionType->param();
            m_currentFunction->expandByteCode(sizeof(uint32_t) * param.size());
            Walrus::Throw* code = m_currentFunction->peekByteCode<Walrus::Throw>(pos);
            for (size_t i = 0; i < param.size(); i++) {
                code->dataOffsets()[param.size() - i - 1] = (m_vmStack.rbegin() + i)->m_position;
            }
            for (size_t i = 0; i < param.size(); i++) {
                ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(functionType->param()[functionType->param().size() - i - 1]));
                popVMStack();
            }
        }

        stopToGenerateByteCodeWhileBlockEnd();
    }

    virtual void OnTryExpr(Type sigType) override
    {
        BlockInfo b(BlockInfo::TryCatch, sigType);
        b.m_position = m_currentFunction->currentByteCodeSize();
        b.m_vmStack = m_vmStack;
        m_blockInfo.push_back(b);
    }

    void processCatchExpr(Index tagIndex)
    {
        ASSERT(m_blockInfo.back().m_blockType == BlockInfo::TryCatch);

        auto& blockInfo = m_blockInfo.back();
        restoreVMStackRegardToPartOfBlockEnd(blockInfo);

        size_t tryEnd = m_currentFunction->currentByteCodeSize();
        if (m_catchInfo.size() && m_catchInfo.back().m_tryCatchBlockDepth == m_blockInfo.size()) {
            // not first catch
            tryEnd = m_catchInfo.back().m_tryEnd;
            blockInfo.m_jumpToEndBrInfo.push_back({ false, m_currentFunction->currentByteCodeSize() });
            m_currentFunction->pushByteCode(Walrus::Jump());
        } else {
            // first catch
            blockInfo.m_jumpToEndBrInfo.push_back({ false, m_currentFunction->currentByteCodeSize() });
            m_currentFunction->pushByteCode(Walrus::Jump());
        }

        m_catchInfo.push_back({ m_blockInfo.size(), m_blockInfo.back().m_position, tryEnd, m_currentFunction->currentByteCodeSize(), tagIndex });

        if (tagIndex != std::numeric_limits<Index>::max()) {
            auto functionType = m_module->functionType(m_module->m_tagTypes[tagIndex].sigIndex());
            for (size_t i = 0; i < functionType->param().size(); i++) {
                pushVMStack(Walrus::valueSizeInStack(functionType->param()[i]));
            }
        }
    }

    virtual void OnCatchExpr(Index tagIndex) override
    {
        processCatchExpr(tagIndex);
    }

    virtual void OnCatchAllExpr() override
    {
        processCatchExpr(std::numeric_limits<Index>::max());
    }

    virtual void OnMemoryInitExpr(Index segmentIndex, Index memidx) override
    {
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
        auto src2 = popVMStack();
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
        auto src1 = popVMStack();
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
        auto src0 = popVMStack();

        m_currentFunction->pushByteCode(Walrus::MemoryInit(memidx, segmentIndex, src0, src1, src2));
    }

    virtual void OnMemoryCopyExpr(Index srcMemIndex, Index dstMemIndex) override
    {
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
        auto src2 = popVMStack();
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
        auto src1 = popVMStack();
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
        auto src0 = popVMStack();

        m_currentFunction->pushByteCode(Walrus::MemoryCopy(srcMemIndex, dstMemIndex, src0, src1, src2));
    }

    virtual void OnMemoryFillExpr(Index memidx) override
    {
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
        auto src2 = popVMStack();
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
        auto src1 = popVMStack();
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
        auto src0 = popVMStack();

        m_currentFunction->pushByteCode(Walrus::MemoryFill(memidx, src0, src1, src2));
    }

    virtual void OnDataDropExpr(Index segmentIndex) override
    {
        m_currentFunction->pushByteCode(Walrus::DataDrop(segmentIndex));
    }

    virtual void OnMemoryGrowExpr(Index memidx) override
    {
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
        auto src = popVMStack();
        auto dst = pushVMStack(Walrus::valueSizeInStack(Walrus::Value::Type::I32));
        m_currentFunction->pushByteCode(Walrus::MemoryGrow(memidx, src, dst));
    }

    virtual void OnMemorySizeExpr(Index memidx) override
    {
        auto stackPos = pushVMStack(Walrus::valueSizeInStack(Walrus::Value::Type::I32));
        m_currentFunction->pushByteCode(Walrus::MemorySize(memidx, stackPos));
    }

    virtual void OnTableGetExpr(Index tableIndex) override
    {
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
        auto src = popVMStack();
        auto dst = pushVMStack(Walrus::valueSizeInStack(Walrus::Value::Type::FuncRef));
        m_currentFunction->pushByteCode(Walrus::TableGet(tableIndex, src, dst));
    }

    virtual void OnTableSetExpr(Index table_index) override
    {
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::FuncRef)));
        auto src1 = popVMStack();
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
        auto src0 = popVMStack();
        m_currentFunction->pushByteCode(Walrus::TableSet(table_index, src0, src1));
    }

    virtual void OnTableGrowExpr(Index table_index) override
    {
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
        auto src1 = popVMStack();
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::FuncRef)));
        auto src0 = popVMStack();
        auto dst = pushVMStack(Walrus::valueSizeInStack(Walrus::Value::Type::I32));
        m_currentFunction->pushByteCode(Walrus::TableGrow(table_index, src0, src1, dst));
    }

    virtual void OnTableSizeExpr(Index table_index) override
    {
        auto dst = pushVMStack(Walrus::valueSizeInStack(Walrus::Value::Type::I32));
        m_currentFunction->pushByteCode(Walrus::TableSize(table_index, dst));
    }

    virtual void OnTableCopyExpr(Index dst_index, Index src_index) override
    {
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
        auto src2 = popVMStack();
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
        auto src1 = popVMStack();
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
        auto src0 = popVMStack();
        m_currentFunction->pushByteCode(Walrus::TableCopy(dst_index, src_index, src0, src1, src2));
    }

    virtual void OnTableFillExpr(Index table_index) override
    {
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
        auto src2 = popVMStack();
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::FuncRef)));
        auto src1 = popVMStack();
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
        auto src0 = popVMStack();
        m_currentFunction->pushByteCode(Walrus::TableFill(table_index, src0, src1, src2));
    }

    virtual void OnElemDropExpr(Index segmentIndex) override
    {
        m_currentFunction->pushByteCode(Walrus::ElemDrop(segmentIndex));
    }

    virtual void OnTableInitExpr(Index segmentIndex, Index tableIndex) override
    {
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
        auto src2 = popVMStack();
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
        auto src1 = popVMStack();
        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(Type::I32)));
        auto src0 = popVMStack();
        m_currentFunction->pushByteCode(Walrus::TableInit(tableIndex, segmentIndex, src0, src1, src2));
    }

    virtual void OnLoadExpr(int opcode, Index memidx, Address alignmentLog2, Address offset) override
    {
        auto code = static_cast<Walrus::OpcodeKind>(opcode);
        ASSERT(Walrus::ByteCodeInfo::byteCodeTypeToMemorySize(Walrus::g_byteCodeInfo[code].m_paramTypes[0]) == peekVMStackSize());
        auto src = popVMStack();
        auto dst = pushVMStack(Walrus::ByteCodeInfo::byteCodeTypeToMemorySize(Walrus::g_byteCodeInfo[code].m_resultType));
        if ((opcode == Walrus::I32LoadOpcode || opcode == Walrus::F32LoadOpcode) && offset == 0) {
            m_currentFunction->pushByteCode(Walrus::Load32(src, dst));
        } else if ((opcode == Walrus::I64LoadOpcode || opcode == Walrus::F64LoadOpcode) && offset == 0) {
            m_currentFunction->pushByteCode(Walrus::Load64(src, dst));
        } else {
            m_currentFunction->pushByteCode(Walrus::MemoryLoad(code, offset, src, dst));
        }
    }

    virtual void OnStoreExpr(int opcode, Index memidx, Address alignmentLog2, Address offset) override
    {
        auto code = static_cast<Walrus::OpcodeKind>(opcode);
        ASSERT(Walrus::ByteCodeInfo::byteCodeTypeToMemorySize(Walrus::g_byteCodeInfo[code].m_paramTypes[1]) == peekVMStackSize());
        auto src1 = popVMStack();
        ASSERT(Walrus::ByteCodeInfo::byteCodeTypeToMemorySize(Walrus::g_byteCodeInfo[code].m_paramTypes[0]) == peekVMStackSize());
        auto src0 = popVMStack();
        if ((opcode == Walrus::I32StoreOpcode || opcode == Walrus::F32StoreOpcode) && offset == 0) {
            m_currentFunction->pushByteCode(Walrus::Store32(src0, src1));
        } else if ((opcode == Walrus::I64StoreOpcode || opcode == Walrus::F64StoreOpcode) && offset == 0) {
            m_currentFunction->pushByteCode(Walrus::Store64(src0, src1));
        } else {
            m_currentFunction->pushByteCode(Walrus::MemoryStore(code, offset, src0, src1));
        }
    }

    virtual void OnRefFuncExpr(Index func_index) override
    {
        auto dst = pushVMStack(Walrus::valueSizeInStack(Walrus::Value::Type::FuncRef));
        m_currentFunction->pushByteCode(Walrus::RefFunc(func_index, dst));
    }

    virtual void OnRefNullExpr(Type type) override
    {
        auto dst = pushVMStack(Walrus::valueSizeInStack(Walrus::Value::Type::FuncRef));
        m_currentFunction->pushByteCode(Walrus::RefNull(toValueKind(type), dst));
    }

    virtual void OnRefIsNullExpr() override
    {
        auto src = popVMStack();
        m_currentFunction->pushByteCode(Walrus::RefIsNull(src, pushVMStack(Walrus::valueSizeInStack(Walrus::Value::Type::I32))));
    }

    virtual void OnNopExpr() override
    {
    }

    virtual void OnReturnExpr() override
    {
        generateFunctionReturnCode();
    }

    virtual void OnEndExpr() override
    {
        if (m_blockInfo.size()) {
            auto blockInfo = m_blockInfo.back();
            m_blockInfo.pop_back();

#if !defined(NDEBUG)
            if (!blockInfo.m_shouldRestoreVMStackAtEnd) {
                if (!blockInfo.m_returnValueType.IsIndex() && blockInfo.m_returnValueType != Type::Void) {
                    ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(toValueKind(blockInfo.m_returnValueType)));
                }
            }
#endif

            if (blockInfo.m_blockType == BlockInfo::TryCatch) {
                auto iter = m_catchInfo.begin();
                while (iter != m_catchInfo.end()) {
                    if (iter->m_tryCatchBlockDepth - 1 != m_blockInfo.size()) {
                        iter++;
                        continue;
                    }
                    size_t stackSizeToBe = m_initialFunctionStackSize;
                    for (size_t i = 0; i < blockInfo.m_vmStack.size(); i++) {
                        stackSizeToBe += m_vmStack[i].m_size;
                    }
                    m_currentFunction->m_catchInfo.pushBack({ iter->m_tryStart, iter->m_tryEnd, iter->m_tagIndex, iter->m_catchStart, stackSizeToBe });
                    iter = m_catchInfo.erase(iter);
                }
            }

            for (size_t i = 0; i < blockInfo.m_jumpToEndBrInfo.size(); i++) {
                if (blockInfo.m_jumpToEndBrInfo[i].m_isJumpIf) {
                    m_currentFunction->peekByteCode<Walrus::JumpIfFalse>(blockInfo.m_jumpToEndBrInfo[i].m_position)
                        ->setOffset(m_currentFunction->currentByteCodeSize() - blockInfo.m_jumpToEndBrInfo[i].m_position);
                } else {
                    m_currentFunction->peekByteCode<Walrus::Jump>(blockInfo.m_jumpToEndBrInfo[i].m_position)->setOffset(m_currentFunction->currentByteCodeSize() - blockInfo.m_jumpToEndBrInfo[i].m_position);
                }
            }

            if (blockInfo.m_shouldRestoreVMStackAtEnd) {
                restoreVMStackBy(blockInfo.m_vmStack);
                if (blockInfo.m_returnValueType.IsIndex()) {
                    auto ft = m_module->functionType(blockInfo.m_returnValueType);
                    const auto& param = ft->param();
                    for (size_t i = 0; i < param.size(); i++) {
                        ASSERT(peekVMStackSize() == Walrus::valueSizeInStack(param[param.size() - i - 1]));
                        popVMStackSize();
                    }

                    const auto& result = ft->result();
                    for (size_t i = 0; i < result.size(); i++) {
                        pushVMStack(Walrus::valueSizeInStack(result[i]));
                    }
                } else if (blockInfo.m_returnValueType != Type::Void) {
                    pushVMStack(Walrus::valueSizeInStack(toValueKind(blockInfo.m_returnValueType)));
                }
            }
        } else {
            generateEndCode();
        }
    }

    virtual void OnUnreachableExpr() override
    {
        m_currentFunction->pushByteCode(Walrus::Unreachable());
        stopToGenerateByteCodeWhileBlockEnd();
    }

    virtual void EndFunctionBody(Index index) override
    {
#if !defined(NDEBUG)
        if (getenv("DUMP_BYTECODE") && strlen(getenv("DUMP_BYTECODE"))) {
            m_currentFunction->dumpByteCode();
        }
        if (m_shouldContinueToGenerateByteCode) {
            for (size_t i = 0; i < m_currentFunctionType->result().size() && m_vmStack.size(); i++) {
                ASSERT(popVMStackSize() == Walrus::valueSizeInStack(m_currentFunctionType->result()[m_currentFunctionType->result().size() - i - 1]));
            }
            ASSERT(m_vmStack.empty());
        }
#endif

        ASSERT(m_currentFunction == m_module->function(index));
        endFunction();
    }
};

} // namespace wabt

namespace Walrus {

std::pair<Optional<Module*>, Optional<String*>> WASMParser::parseBinary(Store* store, const std::string& filename, const uint8_t* data, size_t len)
{
    Module* module = new Module(store);
    wabt::WASMBinaryReader delegate(module);

    std::string error = ReadWasmBinary(filename, data, len, &delegate);
    if (error.length()) {
        return std::make_pair(nullptr, new String(error));
    }
    return std::make_pair(module, nullptr);
}

} // namespace Walrus
