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
#include "parser/LiveAnalysis.h"
#include "interpreter/ByteCode.h"
#include "runtime/GCArray.h"
#include "runtime/Module.h"
#include "runtime/Store.h"
#include "runtime/TypeStore.h"

#include "wabt/binary-reader.h"
#include "runtime/Value.h"
#include "wabt/walrus/binary-reader-walrus.h"
#include <cstdint>
#include <utility>

namespace wabt {

static size_t counter = 0;

enum class WASMOpcode : size_t {
#define WABT_OPCODE(rtype, type1, type2, type3, memSize, prefix, code, name, \
                    text, decomp)                                            \
    name##Opcode,
#include "wabt/opcode.def"
#undef WABT_OPCODE
    OpcodeKindEnd,
};

struct WASMCodeInfo {
    enum CodeType { ___,
                    I32,
                    I64,
                    F32,
                    F64,
                    V128 };
    WASMOpcode m_code;
    CodeType m_resultType;
    CodeType m_paramTypes[3];
    const char* m_name;

    size_t stackShrinkSize() const
    {
        ASSERT(m_code != WASMOpcode::OpcodeKindEnd);
        return codeTypeToMemorySize(m_paramTypes[0]) + codeTypeToMemorySize(m_paramTypes[1]) + codeTypeToMemorySize(m_paramTypes[2]);
    }

    size_t stackGrowSize() const
    {
        ASSERT(m_code != WASMOpcode::OpcodeKindEnd);
        return codeTypeToMemorySize(m_resultType);
    }

    static size_t codeTypeToMemorySize(CodeType tp)
    {
        switch (tp) {
        case I32:
            return Walrus::stackAllocatedSize<int32_t>();
        case F32:
            return Walrus::stackAllocatedSize<float>();
        case I64:
            return Walrus::stackAllocatedSize<int64_t>();
        case F64:
            return Walrus::stackAllocatedSize<double>();
        case V128:
            return 16;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return 0;
        }
    }

    static Walrus::Value::Type codeTypeToValueType(CodeType tp)
    {
        switch (tp) {
        case I32:
            return Walrus::Value::Type::I32;
        case F32:
            return Walrus::Value::Type::F32;
        case I64:
            return Walrus::Value::Type::I64;
        case F64:
            return Walrus::Value::Type::F64;
        case V128:
            return Walrus::Value::Type::V128;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return Walrus::Value::Type::Void;
        }
    }
};

WASMCodeInfo g_wasmCodeInfo[static_cast<size_t>(WASMOpcode::OpcodeKindEnd)] = {
#define WABT_OPCODE(rtype, type1, type2, type3, memSize, prefix, code, name, \
                    text, decomp)                                            \
    { WASMOpcode::name##Opcode,                                              \
      WASMCodeInfo::rtype,                                                   \
      { WASMCodeInfo::type1, WASMCodeInfo::type2, WASMCodeInfo::type3 },     \
      text },
#include "wabt/opcode.def"

#undef WABT_OPCODE
};

static Walrus::Type toRefValueKind(Type type, Walrus::WASMParsingResult* result)
{
    switch (type) {
    case Type::NullFuncRef:
        return type.IsNullableNonTypedRef() ? Walrus::Value::NullNoFuncRef : Walrus::Value::NoFuncRef;
    case Type::NullExternRef:
        return type.IsNullableNonTypedRef() ? Walrus::Value::NullNoExternRef : Walrus::Value::NoExternRef;
    case Type::NullRef:
        return type.IsNullableNonTypedRef() ? Walrus::Value::NullNoAnyRef : Walrus::Value::NoAnyRef;
    case Type::FuncRef:
        return type.IsNullableNonTypedRef() ? Walrus::Value::NullFuncRef : Walrus::Value::FuncRef;
    case Type::ExternRef:
        return type.IsNullableNonTypedRef() ? Walrus::Value::NullExternRef : Walrus::Value::ExternRef;
    case Type::AnyRef:
        return type.IsNullableNonTypedRef() ? Walrus::Value::NullAnyRef : Walrus::Value::AnyRef;
    case Type::EqRef:
        return type.IsNullableNonTypedRef() ? Walrus::Value::NullEqRef : Walrus::Value::EqRef;
    case Type::I31Ref:
        return type.IsNullableNonTypedRef() ? Walrus::Value::NullI31Ref : Walrus::Value::I31Ref;
    case Type::StructRef:
        return type.IsNullableNonTypedRef() ? Walrus::Value::NullStructRef : Walrus::Value::StructRef;
    case Type::ArrayRef:
        return type.IsNullableNonTypedRef() ? Walrus::Value::NullArrayRef : Walrus::Value::ArrayRef;
    case Type::Ref:
        if (result != nullptr) {
            return Walrus::Type(Walrus::Value::DefinedRef, result->m_compositeTypes[type.GetReferenceIndex()]);
        }
        return Walrus::Type(Walrus::Value::DefinedRef, reinterpret_cast<Walrus::CompositeType*>(type.GetReferenceIndex()));
    case Type::RefNull:
        if (result != nullptr) {
            return Walrus::Type(Walrus::Value::NullDefinedRef, result->m_compositeTypes[type.GetReferenceIndex()]);
        }
        return Walrus::Type(Walrus::Value::NullDefinedRef, reinterpret_cast<Walrus::CompositeType*>(type.GetReferenceIndex()));
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
}

static Walrus::Type toValueKind(Type type, Walrus::WASMParsingResult* result)
{
    switch (type) {
    case Type::I32:
        return Walrus::Value::I32;
    case Type::I64:
        return Walrus::Value::I64;
    case Type::F32:
        return Walrus::Value::F32;
    case Type::F64:
        return Walrus::Value::F64;
    case Type::V128:
        return Walrus::Value::V128;
    case Type::I8:
        return Walrus::Value::I8;
    case Type::I16:
        return Walrus::Value::I16;
    default:
        return toRefValueKind(type, result);
    }
}

static size_t getRefCountOfTypes(Type* types, Index count)
{
    size_t result = 0;
    for (Index i = 0; i < count; i++) {
        if (types[i] == Type::Ref || types[i] == Type::RefNull) {
            result++;
        }
    }
    return result;
}

static size_t getRefCountOfMutTypes(TypeMut* types, Index count)
{
    size_t result = 0;
    for (Index i = 0; i < count; i++) {
        if (types[i].type == Type::Ref || types[i].type == Type::RefNull) {
            result++;
        }
    }
    return result;
}

static const Walrus::CompositeType** toSubType(GCTypeExtension* gcExt)
{
    if (gcExt->sub_type_count == 0) {
        return reinterpret_cast<const Walrus::CompositeType**>(Walrus::TypeStore::NoIndex);
    }
    return reinterpret_cast<const Walrus::CompositeType**>(gcExt->sub_types[0]);
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

#if !defined(NDEBUG)
// Type checking should be done by validator.
static Walrus::Value::Type toDebugType(Walrus::Value::Type type)
{
    if (Walrus::Value::isRefType(type)) {
        return Walrus::Value::DefinedRef;
    }
    return type;
}
#endif

class WASMBinaryReader : public wabt::WASMBinaryReaderDelegate {
private:
    struct VMStackInfo {
    public:
        VMStackInfo(WASMBinaryReader& reader, Walrus::Value::Type valueType, size_t position, size_t nonOptimizedPosition, size_t localIndex)
            : m_valueType(valueType)
            , m_position(position)
            , m_nonOptimizedPosition(nonOptimizedPosition)
            , m_localIndex(localIndex)
        {
        }

        VMStackInfo(const VMStackInfo& src)
            : m_valueType(src.m_valueType)
            , m_position(src.m_position)
            , m_nonOptimizedPosition(src.m_nonOptimizedPosition)
            , m_localIndex(src.m_localIndex)
        {
        }

        ~VMStackInfo()
        {
        }

        const VMStackInfo& operator=(const VMStackInfo& src)
        {
            m_valueType = src.m_valueType;
            m_position = src.m_position;
            m_nonOptimizedPosition = src.m_nonOptimizedPosition;
            m_localIndex = src.m_localIndex;
            return *this;
        }

        bool hasValidLocalIndex() const
        {
            return m_localIndex != std::numeric_limits<size_t>::max();
        }

        void clearLocalIndex()
        {
            m_localIndex = std::numeric_limits<size_t>::max();
        }

        size_t position() const
        {
            return m_position;
        }

        void setPosition(size_t position)
        {
            m_position = position;
        }

        Walrus::Value::Type valueType() const
        {
            return m_valueType;
        }

        void toNonNullableRef()
        {
            m_valueType = Walrus::Value::toNonNullableRefType(m_valueType);
        }

        void refCast(Walrus::Value::Type valueType)
        {
            ASSERT(Walrus::Value::isRefType(m_valueType) && Walrus::Value::isRefType(valueType));
            m_valueType = valueType;
        }

        size_t stackAllocatedSize() const
        {
            return Walrus::valueStackAllocatedSize(m_valueType);
        }

        size_t nonOptimizedPosition() const
        {
            return m_nonOptimizedPosition;
        }

        size_t localIndex() const
        {
            return m_localIndex;
        }

    private:
        Walrus::Value::Type m_valueType;
        size_t m_position; // effective position (local values will have different position)
        size_t m_nonOptimizedPosition; // non-optimized position (same with m_functionStackSizeSoFar)
        size_t m_localIndex;
    };

    struct BlockInfo {
        enum BlockType {
            IfElse,
            Loop,
            Block,
            TryCatch,
        };

        static_assert(sizeof(Walrus::JumpIfTrue) == sizeof(Walrus::JumpIfFalse), "");
        struct JumpToEndBrInfo {
            enum JumpToEndType {
                IsJump,
                IsJumpIf,
                IsBrTable,
            };

            JumpToEndType m_type;
            size_t m_position;
        };

        static constexpr uint16_t ReturnValueIsIndex = 0x1;
        static constexpr uint16_t ShouldRestoreVMStackAtEnd = 0x2;
        static constexpr uint16_t ByteCodeGenerationStopped = 0x4;
        static constexpr uint16_t SeenBranch = 0x8;

        std::vector<VMStackInfo> m_vmStack;
        std::vector<JumpToEndBrInfo> m_jumpToEndBrInfo;
        size_t m_position;
        uint32_t m_functionStackSizeSoFar;
        union {
            Index m_returnValueIndex;
            Walrus::Value::Type m_returnValueType;
        };
        BlockType m_blockType;
        uint16_t m_flags;

        BlockInfo(BlockType type, Type returnValueType, WASMBinaryReader& binaryReader)
            : m_position(0)
            , m_functionStackSizeSoFar(binaryReader.m_functionStackSizeSoFar)
            , m_blockType(type)
            , m_flags(0)
        {
            if (returnValueType.IsIndex()) {
                m_flags |= ReturnValueIsIndex;
                m_returnValueIndex = returnValueType;
                if (binaryReader.getFunctionType(m_returnValueIndex)->result().size()) {
                    m_flags |= ShouldRestoreVMStackAtEnd;
                }
            } else if (returnValueType != Type::Void) {
                m_returnValueType = toValueKind(returnValueType, nullptr);
                m_flags |= ShouldRestoreVMStackAtEnd;
            } else {
                m_returnValueType = Walrus::Value::Void;
            }

            if (returnValueIsIndex() && binaryReader.getFunctionType(m_returnValueIndex)->param().size()) {
                // record parameter positions
                auto& param = binaryReader.getFunctionType(m_returnValueIndex)->param();
                auto endIter = binaryReader.m_vmStack.rbegin() + param.size();
                auto iter = binaryReader.m_vmStack.rbegin();
                while (iter != endIter) {
                    if (iter->position() != iter->nonOptimizedPosition()) {
                        binaryReader.generateMoveCodeIfNeeds(iter->position(), iter->nonOptimizedPosition(), iter->valueType());
                        iter->setPosition(iter->nonOptimizedPosition());
                        if (binaryReader.m_preprocessData.m_inPreprocess && iter->hasValidLocalIndex()) {
                            size_t pos = *binaryReader.m_readerOffsetPointer;
                            auto localVariableIter = binaryReader.m_preprocessData
                                                         .m_localVariableInfo[iter->localIndex()]
                                                         .m_usageInfo.rbegin();
                            while (true) {
                                ASSERT(localVariableIter != binaryReader.m_preprocessData.m_localVariableInfo[iter->localIndex()].m_usageInfo.rend());
                                if (localVariableIter->m_endPosition == std::numeric_limits<size_t>::max()) {
                                    localVariableIter->m_endPosition = *binaryReader.m_readerOffsetPointer;
                                    break;
                                }
                                localVariableIter++;
                            }
                        }
                        iter->clearLocalIndex();
                    }
                    iter++;
                }
            }

            m_vmStack = binaryReader.m_vmStack;
            m_position = binaryReader.m_currentFunction->currentByteCodeSize();
        }

        bool returnValueIsIndex() const
        {
            return (m_flags & ReturnValueIsIndex) != 0;
        }

        bool shouldRestoreVMStackAtEnd() const
        {
            return (m_flags & ShouldRestoreVMStackAtEnd) != 0;
        }

        bool byteCodeGenerationStopped() const
        {
            return (m_flags & ByteCodeGenerationStopped) != 0;
        }

        bool seenBranch() const
        {
            return (m_flags & SeenBranch) != 0;
        }

        void clearByteCodeGenerationStopped()
        {
            m_flags = static_cast<uint16_t>(m_flags & ~ByteCodeGenerationStopped);
        }
    };

    size_t* m_readerOffsetPointer;
    const uint8_t* m_readerDataPointer;
    size_t m_codeEndOffset;
    Walrus::TypeStore& typeStore;

    struct PreprocessData {
        struct LocalVariableInfo {
            bool m_needsExplicitInitOnStartup;
            LocalVariableInfo()
                : m_needsExplicitInitOnStartup(false)
            {
            }

            struct UsageInfo {
                size_t m_startPosition;
                size_t m_endPosition;
                size_t m_pushCount;
                bool m_hasWriteUsage;

                UsageInfo(size_t startPosition, size_t pushCount)
                    : m_startPosition(startPosition)
                    , m_endPosition(std::numeric_limits<size_t>::max())
                    , m_pushCount(pushCount)
                    , m_hasWriteUsage(false)
                {
                }
            };
            std::vector<size_t> m_definitelyWritePlaces;
            std::vector<size_t> m_writePlacesBetweenBranches;
            std::vector<UsageInfo> m_usageInfo;
        };

        PreprocessData(WASMBinaryReader& reader)
            : m_inPreprocess(false)
            , m_reader(reader)
        {
        }

        void clear()
        {
            m_localVariableInfo.clear();
            m_localVariableInfo.resize(m_reader.m_localInfo.size());
            m_constantData.clear();
        }

        void seenBranch()
        {
            if (m_inPreprocess) {
                if (m_reader.m_blockInfo.size()) {
                    m_reader.m_blockInfo.back().m_flags |= BlockInfo::SeenBranch;
                }
                for (auto& info : m_localVariableInfo) {
                    info.m_writePlacesBetweenBranches.clear();
                }
            }
        }

        void addLocalVariableUsage(size_t localIndex)
        {
            if (m_inPreprocess) {
                size_t pushCount = 0;
                size_t pos = *m_reader.m_readerOffsetPointer;
                for (const auto& stack : m_reader.m_vmStack) {
                    if (stack.localIndex() == localIndex) {
                        pushCount++;
                    }
                }
                m_localVariableInfo[localIndex].m_usageInfo.push_back(LocalVariableInfo::UsageInfo(pos, pushCount));
                if (!m_localVariableInfo[localIndex].m_needsExplicitInitOnStartup) {
                    if (!m_localVariableInfo[localIndex].m_writePlacesBetweenBranches.size()) {
                        bool writeFound = false;
                        const auto& definitelyWritePlaces = m_localVariableInfo[localIndex].m_definitelyWritePlaces;
                        for (size_t i = 0; i < definitelyWritePlaces.size(); i++) {
                            if (definitelyWritePlaces[i] < pos) {
                                writeFound = true;
                                break;
                            }
                        }
                        if (!writeFound) {
                            m_localVariableInfo[localIndex].m_needsExplicitInitOnStartup = true;
                        }
                    }
                }
            }
        }

        void addLocalVariableWrite(Index localIndex)
        {
            if (m_inPreprocess) {
                size_t pos = *m_reader.m_readerOffsetPointer;
                auto iter = m_localVariableInfo[localIndex].m_usageInfo.begin();
                while (iter != m_localVariableInfo[localIndex].m_usageInfo.end()) {
                    if (iter->m_startPosition <= pos && pos <= iter->m_endPosition) {
                        iter->m_hasWriteUsage = true;
                    }
                    iter++;
                }

                bool isDefinitelyWritePlaces = true;
                auto blockIter = m_reader.m_blockInfo.rbegin();
                while (blockIter != m_reader.m_blockInfo.rend()) {
                    if (blockIter->seenBranch()) {
                        isDefinitelyWritePlaces = false;
                        break;
                    }
                    blockIter++;
                }

                if (isDefinitelyWritePlaces) {
                    m_localVariableInfo[localIndex].m_definitelyWritePlaces.push_back(pos);
                }

                m_localVariableInfo[localIndex].m_writePlacesBetweenBranches.push_back(pos);
            }
        }

        void addConstantData(const Walrus::Value& v)
        {
            if (m_inPreprocess) {
                bool found = false;
                for (size_t i = 0; i < m_constantData.size(); i++) {
                    if (m_constantData[i].first == v) {
                        m_constantData[i].second++;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    m_constantData.push_back(std::make_pair(v, 1));
                }

#ifndef WALRUS_ASSIGN_CONSTANT_ON_STACK_MAX_COUNT
#define WALRUS_ASSIGN_CONSTANT_ON_STACK_MAX_COUNT 6
#endif
                constexpr size_t maxConstantData = WALRUS_ASSIGN_CONSTANT_ON_STACK_MAX_COUNT;
                if (m_constantData.size() > maxConstantData) {
                    organizeConstantData();
                    m_constantData.erase(m_constantData.end() - maxConstantData / 4, m_constantData.end());
                }
            }
        }

        void organizeConstantData()
        {
            std::sort(m_constantData.begin(), m_constantData.end(),
                      [](const std::pair<Walrus::Value, size_t>& a, const std::pair<Walrus::Value, size_t>& b) -> bool {
                          return a.second > b.second;
                      });
        }

        void organizeData()
        {
            organizeConstantData();
        }

        bool m_inPreprocess;
        WASMBinaryReader& m_reader;
        std::vector<LocalVariableInfo> m_localVariableInfo;
        // <ConstantValue, reference count or position>
        std::vector<std::pair<Walrus::Value, size_t>> m_constantData;
    };

    bool m_inInitExpr;
    Walrus::ModuleFunction* m_currentFunction;
    Walrus::FunctionType* m_currentFunctionType;
    uint32_t m_initialFunctionStackSize;
    uint16_t m_functionStackSizeSoFar;
    Index m_recursiveTypeStart;
    Index m_recursiveTypeEnd;

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
    struct LocalInfo {
        Walrus::Value::Type m_valueType;
        size_t m_position;
        LocalInfo(Walrus::Value::Type type, size_t position)
            : m_valueType(type)
            , m_position(position)
        {
        }
    };
    std::vector<LocalInfo> m_localInfo;

    Walrus::Vector<uint8_t, std::allocator<uint8_t>> m_memoryInitData;
    size_t m_dataSegmentMemIndex = -1;

    uint32_t m_elementTableIndex;
    Type m_elementType;
    Walrus::Optional<Walrus::ModuleFunction*> m_elementOffsetFunction;
    Walrus::Vector<Walrus::ModuleFunction*> m_elementExprFunctions;
    Walrus::SegmentMode m_segmentMode;

    Walrus::WASMParsingResult m_result;
    std::string m_walrusParseError;

    PreprocessData m_preprocessData;

    // i32.eqz and JumpIf can be unified in some cases
    static const size_t s_noI32Eqz = SIZE_MAX - sizeof(Walrus::I32Eqz);
    size_t m_lastI32EqzPos;

    Walrus::FunctionType* getFunctionType(Index index)
    {
        return m_result.m_compositeTypes[index]->asFunction();
    }

    virtual void OnSetOffsetAddress(size_t* ptr) override
    {
        m_readerOffsetPointer = ptr;
    }

    virtual void OnSetDataAddress(const uint8_t* data) override
    {
        m_readerDataPointer = data;
    }

    size_t pushVMStack(Walrus::Value::Type type)
    {
        auto pos = m_functionStackSizeSoFar;
        pushVMStack(type, pos);
        return pos;
    }

    void pushVMStack(Walrus::Value::Type type, size_t pos, size_t localIndex = std::numeric_limits<size_t>::max())
    {
        if (localIndex != std::numeric_limits<size_t>::max()) {
            m_preprocessData.addLocalVariableUsage(localIndex);
        }

        m_vmStack.push_back(VMStackInfo(*this, type, pos, m_functionStackSizeSoFar, localIndex));
        size_t allocSize = Walrus::valueStackAllocatedSize(type);

        m_functionStackSizeSoFar += allocSize;
        m_currentFunction->m_requiredStackSize = std::max(
            m_currentFunction->m_requiredStackSize, m_functionStackSizeSoFar);
    }

    VMStackInfo popVMStackInfo()
    {
        // FIXME This error can occur during the parsing process because of invalid wasm instructions
        // e.g. a function with no end opcode
        ASSERT(m_vmStack.size() > 0);

        auto info = m_vmStack.back();
        m_functionStackSizeSoFar -= Walrus::valueStackAllocatedSize(info.valueType());
        m_vmStack.pop_back();

        if (m_preprocessData.m_inPreprocess) {
            if (info.hasValidLocalIndex()) {
                size_t pos = *m_readerOffsetPointer;
                auto iter = m_preprocessData.m_localVariableInfo[info.localIndex()].m_usageInfo.rbegin();
                while (true) {
                    ASSERT(iter != m_preprocessData.m_localVariableInfo[info.localIndex()].m_usageInfo.rend());
                    if (iter->m_endPosition == std::numeric_limits<size_t>::max()) {
                        iter->m_endPosition = *m_readerOffsetPointer;
                        break;
                    }
                    iter++;
                }
            }
        }

        return info;
    }

    VMStackInfo& peekVMStackInfo()
    {
        // FIXME This error can occur during the parsing process because of invalid wasm instructions
        // e.g. a function with no end opcode
        ASSERT(m_vmStack.size() > 0);
        return m_vmStack.back();
    }

    size_t popVMStack()
    {
        return popVMStackInfo().position();
    }

    size_t peekVMStack()
    {
        return peekVMStackInfo().position();
    }

    Walrus::Value::Type peekVMStackValueType()
    {
        return peekVMStackInfo().valueType();
    }

    void beginFunction(Walrus::ModuleFunction* mf, bool inInitExpr)
    {
        m_inInitExpr = inInitExpr;
        m_currentFunction = mf;
        m_currentFunctionType = mf->functionType();
        m_localInfo.clear();
        m_localInfo.reserve(m_currentFunctionType->param().size());
        size_t pos = 0;
        const Walrus::TypeVector::Types& param = m_currentFunctionType->param().types();
        for (size_t i = 0; i < param.size(); i++) {
            m_localInfo.push_back(LocalInfo(param[i], pos));
            pos += Walrus::valueStackAllocatedSize(m_localInfo[i].m_valueType);
        }
        m_initialFunctionStackSize = m_functionStackSizeSoFar = m_currentFunctionType->paramStackSize();
        m_currentFunction->m_requiredStackSize = std::max(
            m_currentFunction->m_requiredStackSize, m_functionStackSizeSoFar);
    }

    void beginNewFunction(Walrus::Value::Type type, bool inInitExpr)
    {
        Walrus::ModuleFunction* mf = new Walrus::ModuleFunction(Walrus::Store::getDefaultFunctionType(type));
        m_result.m_functions.push_back(mf);
        beginFunction(mf, inInitExpr);
    }

    void endFunction()
    {
        m_currentFunction = nullptr;
        m_currentFunctionType = nullptr;
        m_vmStack.clear();
        m_shouldContinueToGenerateByteCode = true;
    }

    template <typename CodeType>
    void pushByteCode(const CodeType& code, WASMOpcode opcode)
    {
        m_currentFunction->pushByteCode(code);
    }

    template <typename CodeType>
    void pushByteCode(const CodeType& code)
    {
        m_currentFunction->pushByteCode(code);
    }

    void pushByteCode(const Walrus::I32Eqz& code, WASMOpcode opcode)
    {
        m_lastI32EqzPos = m_currentFunction->currentByteCodeSize();
        m_currentFunction->pushByteCode(code);
    }

    inline bool canBeInverted(size_t stackPos)
    {
        /**
         * m_lastI32EqzPos + sizeof(Walrus::I32Eqz) == m_currentFunction->currentByteCodeSize()
         * checks if last byteCode is I32Eqz
         *
         * m_currentFunction->peekByteCode<Walrus::UnaryOperation>(m_lastI32EqzPos)->dstOffset() == stackPos
         * checks if the output of I32Eqz is the input of JumpIfTrue/JumpIfFalse
         */
        return (m_lastI32EqzPos + sizeof(Walrus::I32Eqz) == m_currentFunction->currentByteCodeSize())
            && (m_currentFunction->peekByteCode<Walrus::UnaryOperation>(m_lastI32EqzPos)->dstOffset() == stackPos);
    }

    Walrus::Optional<uint8_t> lookaheadUnsigned8(size_t offset = 0)
    {
        if (*m_readerOffsetPointer + offset < m_codeEndOffset) {
            return m_readerDataPointer[*m_readerOffsetPointer + offset];
        }
        return Walrus::Optional<uint8_t>();
    }

    std::pair<Walrus::Optional<uint32_t>, size_t> lookaheadUnsigned32(size_t offset = 0) // returns result value and data-length
    {
        if (*m_readerOffsetPointer + offset < m_codeEndOffset) {
            uint32_t out = 0;
            auto ptr = m_readerDataPointer + *m_readerOffsetPointer + offset;
            auto len = ReadU32Leb128(ptr, m_readerDataPointer + m_codeEndOffset, &out);
            return std::make_pair(out, len);
        }
        return std::make_pair(Walrus::Optional<uint32_t>(), 0);
    }

    std::pair<Walrus::Optional<uint32_t>, size_t> readAheadLocalGetIfExists() // return localIndex and code length if exists
    {
        Walrus::Optional<uint8_t> mayLoadGetCode = lookaheadUnsigned8();
        if (mayLoadGetCode.hasValue() && mayLoadGetCode.value() == 0x21) {
            auto r = lookaheadUnsigned32(1);
            if (r.first) {
                return std::make_pair(r.first, r.second + 1);
            }
        }

        return std::make_pair(Walrus::Optional<uint32_t>(), 0);
    }

public:
    WASMBinaryReader(Walrus::TypeStore& typeStore)
        : m_readerOffsetPointer(nullptr)
        , m_readerDataPointer(nullptr)
        , m_codeEndOffset(0)
        , typeStore(typeStore)
        , m_inInitExpr(false)
        , m_currentFunction(nullptr)
        , m_currentFunctionType(nullptr)
        , m_initialFunctionStackSize(0)
        , m_functionStackSizeSoFar(0)
        , m_recursiveTypeStart(0)
        , m_recursiveTypeEnd(0)
        , m_elementTableIndex(0)
        , m_segmentMode(Walrus::SegmentMode::None)
        , m_preprocessData(*this)
        , m_lastI32EqzPos(s_noI32Eqz)
    {
    }

    ~WASMBinaryReader()
    {
        // clear stack first! because vmStack refer localInfo
        m_vmStack.clear();
        m_localInfo.clear();

        m_result.clear();
    }

    // should be allocated on the stack
    static void* operator new(size_t) = delete;
    static void* operator new[](size_t) = delete;

    std::string WalrusParseError()
    {
        return m_walrusParseError;
    }

    virtual void BeginModule(uint32_t version) override
    {
        m_result.m_version = version;
    }

    virtual void EndModule() override {}

    virtual void OnFeatureCount(Index count) override {}

    virtual bool OnFeature(uint8_t prefix, std::string name) override
    {
        return true;
    }

    virtual void OnTypeCount(Index count) override
    {
        // TODO reserve vector if possible
    }

    virtual void OnRecursiveType(Index firstTypeIndex, Index typeCount) override
    {
        m_recursiveTypeStart = firstTypeIndex;
        m_recursiveTypeEnd = firstTypeIndex + typeCount;
    }

    virtual void OnFuncType(Index index,
                            Index paramCount,
                            Type* paramTypes,
                            Index resultCount,
                            Type* resultTypes,
                            GCTypeExtension* gcExt) override
    {
        Walrus::TypeVector* param = new Walrus::TypeVector(paramCount, getRefCountOfTypes(paramTypes, paramCount));
        size_t refIdx = 0;
        for (size_t i = 0; i < paramCount; i++) {
            Walrus::Type type = toValueKind(paramTypes[i], nullptr);
            param->setType(i, type.type());
            if (type.isConcreteType()) {
                param->setRef(refIdx++, type.ref());
            }
        }
        Walrus::TypeVector* result = new Walrus::TypeVector(resultCount, getRefCountOfTypes(resultTypes, resultCount));
        refIdx = 0;
        for (size_t i = 0; i < resultCount; i++) {
            Walrus::Type type = toValueKind(resultTypes[i], nullptr);
            result->setType(i, type.type());
            if (type.isConcreteType()) {
                result->setRef(refIdx++, type.ref());
            }
        }
        ASSERT(index == m_result.m_compositeTypes.size());
        m_result.m_compositeTypes.push_back(new Walrus::FunctionType(param, result, gcExt->is_final_sub_type, toSubType(gcExt)));
        if (index < m_recursiveTypeEnd && index > m_recursiveTypeStart) {
            Walrus::TypeStore::ConnectTypes(m_result.m_compositeTypes, index);
        }
    }

    virtual bool OnStructType(Index index,
                              Index fieldCount,
                              TypeMut* fieldTypes,
                              GCTypeExtension* gcExt) override
    {
        Walrus::MutableTypeVector* fields = new Walrus::MutableTypeVector(fieldCount, getRefCountOfMutTypes(fieldTypes, fieldCount));
        size_t refIdx = 0;
        for (size_t i = 0; i < fieldCount; i++) {
            Walrus::Type type = toValueKind(fieldTypes[i].type, nullptr);
            fields->setType(i, Walrus::MutableTypeVector::TypeData(type.type(), fieldTypes[i].mutable_));
            if (type.isConcreteType()) {
                fields->setRef(refIdx++, type.ref());
            }
        }

        ASSERT(index == m_result.m_compositeTypes.size());
        Walrus::StructType* type = new Walrus::StructType(fields, gcExt->is_final_sub_type, toSubType(gcExt));
        if (!type->initialize()) {
            delete type;
            return false;
        }

        m_result.m_compositeTypes.push_back(type);
        if (index < m_recursiveTypeEnd && index > m_recursiveTypeStart) {
            Walrus::TypeStore::ConnectTypes(m_result.m_compositeTypes, index);
        }
        return true;
    }

    virtual void OnArrayType(Index index,
                             TypeMut fieldType,
                             GCTypeExtension* gcExt) override
    {
        ASSERT(index == m_result.m_compositeTypes.size());
        Walrus::Type type = toValueKind(fieldType.type, nullptr);
        m_result.m_compositeTypes.push_back(new Walrus::ArrayType(Walrus::MutableType(type.type(), type.ref(), fieldType.mutable_),
                                                                  gcExt->is_final_sub_type, toSubType(gcExt)));
        if (index < m_recursiveTypeEnd && index > m_recursiveTypeStart) {
            Walrus::TypeStore::ConnectTypes(m_result.m_compositeTypes, index);
        }
    }

    virtual void EndTypeSection() override
    {
        m_result.m_typesAddedToStore = true;
        typeStore.updateTypes(m_result.m_compositeTypes);
    }

    virtual void OnImportCount(Index count) override
    {
        m_result.m_imports.reserve(count);
    }

    virtual void OnImportFunc(Index importIndex,
                              std::string moduleName,
                              std::string fieldName,
                              Index funcIndex,
                              Index sigIndex) override
    {
        ASSERT(m_result.m_functions.size() == funcIndex);
        ASSERT(m_result.m_imports.size() == importIndex);
        Walrus::FunctionType* ft = getFunctionType(sigIndex);
        m_result.m_functions.push_back(
            new Walrus::ModuleFunction(ft));
        m_result.m_imports.push_back(new Walrus::ImportType(
            Walrus::ImportType::Function,
            moduleName, fieldName, ft));
    }

    virtual void OnImportGlobal(Index importIndex, std::string moduleName, std::string fieldName, Index globalIndex, Type type, bool mutable_) override
    {
        ASSERT(globalIndex == m_result.m_globalTypes.size());
        ASSERT(m_result.m_imports.size() == importIndex);
        Walrus::Type globalType = toValueKind(type, &m_result);
        m_result.m_globalTypes.push_back(new Walrus::GlobalType(Walrus::MutableType(globalType.type(), globalType.ref(), mutable_)));
        m_result.m_imports.push_back(new Walrus::ImportType(
            Walrus::ImportType::Global,
            moduleName, fieldName, m_result.m_globalTypes[globalIndex]));
    }

    virtual void OnImportTable(Index importIndex, std::string moduleName, std::string fieldName, Index tableIndex, Type type, size_t initialSize, size_t maximumSize) override
    {
        ASSERT(tableIndex == m_result.m_tableTypes.size());
        ASSERT(m_result.m_imports.size() == importIndex);
        ASSERT(type.IsRef());

        m_result.m_tableTypes.push_back(new Walrus::TableType(toRefValueKind(type, &m_result), initialSize, maximumSize));
        m_result.m_imports.push_back(new Walrus::ImportType(
            Walrus::ImportType::Table,
            moduleName, fieldName, m_result.m_tableTypes[tableIndex]));
    }

    virtual void OnImportMemory(Index importIndex, std::string moduleName, std::string fieldName, Index memoryIndex, size_t initialSize, size_t maximumSize, bool isShared) override
    {
        ASSERT(memoryIndex == m_result.m_memoryTypes.size());
        ASSERT(m_result.m_imports.size() == importIndex);
        m_result.m_memoryTypes.push_back(new Walrus::MemoryType(initialSize, maximumSize, isShared));
        m_result.m_imports.push_back(new Walrus::ImportType(
            Walrus::ImportType::Memory,
            moduleName, fieldName, m_result.m_memoryTypes[memoryIndex]));
    }

    virtual void OnImportTag(Index importIndex, std::string moduleName, std::string fieldName, Index tagIndex, Index sigIndex) override
    {
        ASSERT(tagIndex == m_result.m_tagTypes.size());
        ASSERT(m_result.m_imports.size() == importIndex);
        m_result.m_tagTypes.push_back(new Walrus::TagType(sigIndex));
        m_result.m_imports.push_back(new Walrus::ImportType(
            Walrus::ImportType::Tag,
            moduleName, fieldName, m_result.m_tagTypes[tagIndex]));
    }

    virtual void OnExportCount(Index count) override
    {
        m_result.m_exports.reserve(count);
    }

    virtual void OnExport(int kind, Index exportIndex, std::string name, Index itemIndex) override
    {
        ASSERT(m_result.m_exports.size() == exportIndex);
        m_result.m_exports.push_back(new Walrus::ExportType(static_cast<Walrus::ExportType::Type>(kind), name, itemIndex));
    }

    /* Table section */
    virtual void OnTableCount(Index count) override
    {
        m_result.m_tableTypes.reserve(count);
    }

    virtual void OnTable(Index index, Type type, size_t initialSize, size_t maximumSize) override
    {
        ASSERT(index == m_result.m_tableTypes.size());
        ASSERT(type.IsRef());
        m_result.m_tableTypes.push_back(new Walrus::TableType(toRefValueKind(type, &m_result), initialSize, maximumSize));
    }

    virtual void BeginTableInitExpr(Index index) override
    {
        auto ft = Walrus::Store::getDefaultFunctionType(m_result.m_tableTypes[index]->type());
        Walrus::ModuleFunction* mf = new Walrus::ModuleFunction(ft);
        m_result.m_tableTypes[index]->setFunction(mf);
        beginFunction(mf, true);
    }

    virtual void EndTableInitExpr(Index index) override
    {
        endFunction();
    }

    virtual void OnElemSegmentCount(Index count) override
    {
        m_result.m_elements.reserve(count);
    }

    virtual void BeginElemSegment(Index index, Index tableIndex, uint8_t flags) override
    {
        m_elementTableIndex = tableIndex;
        m_elementOffsetFunction = nullptr;
        m_segmentMode = toSegmentMode(flags);
    }

    virtual void BeginElemSegmentInitExpr(Index index) override
    {
        beginNewFunction(Walrus::Value::I32, true);
    }

    virtual void EndElemSegmentInitExpr(Index index) override
    {
        m_elementOffsetFunction = m_currentFunction;
        endFunction();
    }

    virtual void OnElemSegmentElemType(Index index, Type elemType) override
    {
        m_elementType = elemType;
    }

    virtual void OnElemSegmentElemExprCount(Index index, Index count) override
    {
        m_elementExprFunctions.reserve(count);
    }

    virtual void BeginElemExpr(Index elem_index, Index expr_index) override
    {
        beginNewFunction(toRefValueKind(m_elementType, &m_result), true);
    }

    virtual void EndElemExpr(Index elem_index, Index expr_index) override
    {
        m_elementExprFunctions.push_back(m_currentFunction);
        endFunction();
    }

    virtual void EndElemSegment(Index index) override
    {
        ASSERT(m_result.m_elements.size() == index);
        if (m_elementOffsetFunction) {
            m_result.m_elements.push_back(new Walrus::Element(m_segmentMode, m_elementTableIndex, m_elementOffsetFunction.value(), std::move(m_elementExprFunctions)));
        } else {
            m_result.m_elements.push_back(new Walrus::Element(m_segmentMode, m_elementTableIndex, std::move(m_elementExprFunctions)));
        }

        m_elementOffsetFunction = nullptr;
        m_elementTableIndex = 0;
        m_segmentMode = Walrus::SegmentMode::None;
    }

    /* Memory section */
    virtual void OnMemoryCount(Index count) override
    {
        m_result.m_memoryTypes.reserve(count);
    }

    virtual void OnMemory(Index index, uint64_t initialSize, uint64_t maximumSize, bool isShared) override
    {
        ASSERT(index == m_result.m_memoryTypes.size());
        m_result.m_memoryTypes.push_back(new Walrus::MemoryType(initialSize, maximumSize, isShared));
    }

    virtual void OnDataSegmentCount(Index count) override
    {
        m_result.m_datas.reserve(count);
    }

    virtual void BeginDataSegment(Index index, Index memoryIndex, uint8_t flags) override
    {
        ASSERT(index == m_result.m_datas.size());
        ASSERT(m_dataSegmentMemIndex == static_cast<size_t>(-1));
        m_dataSegmentMemIndex = memoryIndex;
        beginNewFunction(Walrus::Value::I32, true);
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
        ASSERT(index == m_result.m_datas.size());
        m_result.m_datas.push_back(new Walrus::Data(m_dataSegmentMemIndex, m_currentFunction, std::move(m_memoryInitData)));
        m_dataSegmentMemIndex = -1;
        endFunction();
    }

    /* Function section */
    virtual void OnFunctionCount(Index count) override
    {
        m_result.m_functions.reserve(count);
    }

    virtual void OnFunction(Index index, Index sigIndex) override
    {
        ASSERT(m_currentFunction == nullptr);
        ASSERT(m_currentFunctionType == nullptr);
        ASSERT(m_result.m_functions.size() == index);
        m_result.m_functions.push_back(new Walrus::ModuleFunction(getFunctionType(sigIndex)));
    }

    virtual void OnGlobalCount(Index count) override
    {
        m_result.m_globalTypes.reserve(count);
    }

    virtual void BeginGlobal(Index index, Type type, bool mutable_) override
    {
        ASSERT(m_result.m_globalTypes.size() == index);
        Walrus::Type globalType = toValueKind(type, &m_result);
        m_result.m_globalTypes.push_back(new Walrus::GlobalType(Walrus::MutableType(globalType.type(), globalType.ref(), mutable_)));
    }

    virtual void BeginGlobalInitExpr(Index index) override
    {
        auto ft = Walrus::Store::getDefaultFunctionType(m_result.m_globalTypes[index]->type());
        Walrus::ModuleFunction* mf = new Walrus::ModuleFunction(ft);
        m_result.m_globalTypes[index]->setFunction(mf);
        beginFunction(mf, true);
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
        m_result.m_tagTypes.reserve(count);
    }

    virtual void OnTagType(Index index, Index sigIndex) override
    {
        ASSERT(index == m_result.m_tagTypes.size());
        m_result.m_tagTypes.push_back(new Walrus::TagType(sigIndex));
    }

    virtual void OnStartFunction(Index funcIndex) override
    {
        m_result.m_seenStartAttribute = true;
        m_result.m_start = funcIndex;
    }

    virtual void BeginFunctionBody(Index index, Offset size) override
    {
        ASSERT(resumeGenerateByteCodeAfterNBlockEnd() == 0);
        ASSERT(m_currentFunction == nullptr);
        beginFunction(m_result.m_functions[index], false);
    }

    virtual void OnLocalDeclCount(Index count) override
    {
        m_currentFunction->m_local.reserve(count);
        m_localInfo.reserve(count + m_currentFunctionType->param().size());
    }

    virtual void OnLocalDecl(Index decl_index, Index count, Type type) override
    {
        while (count) {
            auto wType = toValueKind(type, &m_result);
            m_currentFunction->m_local.push_back(wType);
            m_localInfo.push_back(LocalInfo(wType, m_functionStackSizeSoFar));
            auto sz = Walrus::valueStackAllocatedSize(wType);

            // FIXME too many stack usage. we could not support this(yet)

            m_initialFunctionStackSize += sz;
            m_functionStackSizeSoFar += sz;
            count--;
        }
        m_currentFunction->m_requiredStackSize = std::max(
            m_currentFunction->m_requiredStackSize, m_functionStackSizeSoFar);
    }

    // FIXME remove preprocess
    virtual void OnStartReadInstructions(Offset start, Offset end) override
    {
        ASSERT(start == *m_readerOffsetPointer);
        m_codeEndOffset = end;
    }

    virtual void OnStartPreprocess() override
    {
        m_preprocessData.m_inPreprocess = true;
        m_preprocessData.clear();
    }

    virtual void OnEndPreprocess() override
    {
        m_preprocessData.m_inPreprocess = false;
        m_skipValidationUntil = *m_readerOffsetPointer - 1;
        m_shouldContinueToGenerateByteCode = true;
        m_recursiveTypeStart = 0;
        m_recursiveTypeEnd = 0;
        setResumeGenerateByteCodeAfterNBlockEnd(0);

        m_currentFunction->m_byteCode.clear();
        m_currentFunction->m_catchInfo.clear();
        m_blockInfo.clear();
        m_catchInfo.clear();

        m_vmStack.clear();

        m_preprocessData.organizeData();

        // set const variables position
        for (size_t i = 0; i < m_preprocessData.m_constantData.size(); i++) {
            auto constType = m_preprocessData.m_constantData[i].first.type();
            m_preprocessData.m_constantData[i].second = m_initialFunctionStackSize;
            m_initialFunctionStackSize += Walrus::valueStackAllocatedSize(constType);
        }

#if defined(WALRUS_64)
#ifndef WALRUS_ENABLE_LOCAL_VARIABLE_PACKING_MIN_SIZE
#define WALRUS_ENABLE_LOCAL_VARIABLE_PACKING_MIN_SIZE 64
#endif
        // pack local variables if needs
        constexpr size_t enableLocalVaraiblePackingMinSize = WALRUS_ENABLE_LOCAL_VARIABLE_PACKING_MIN_SIZE;
        if (m_initialFunctionStackSize >= enableLocalVaraiblePackingMinSize) {
            m_initialFunctionStackSize = m_currentFunctionType->paramStackSize();
            // put already aligned variables first
            for (size_t i = m_currentFunctionType->param().size(); i < m_localInfo.size(); i++) {
                auto& info = m_localInfo[i];
                if (Walrus::hasCPUWordAlignedSize(info.m_valueType) || needsCPUWordAlignedAddress(info.m_valueType)) {
                    info.m_position = m_initialFunctionStackSize;
                    m_initialFunctionStackSize += Walrus::valueStackAllocatedSize(info.m_valueType);
                }
            }
            for (size_t i = 0; i < m_preprocessData.m_constantData.size(); i++) {
                auto constType = m_preprocessData.m_constantData[i].first.type();
                if (Walrus::hasCPUWordAlignedSize(constType) || needsCPUWordAlignedAddress(constType)) {
                    m_preprocessData.m_constantData[i].second = m_initialFunctionStackSize;
                    m_initialFunctionStackSize += Walrus::valueStackAllocatedSize(constType);
                }
            }

            // pack rest values
            for (size_t i = m_currentFunctionType->param().size(); i < m_localInfo.size(); i++) {
                auto& info = m_localInfo[i];
                if (!Walrus::hasCPUWordAlignedSize(info.m_valueType) && !needsCPUWordAlignedAddress(info.m_valueType)) {
                    info.m_position = m_initialFunctionStackSize;
                    m_initialFunctionStackSize += Walrus::valueSize(info.m_valueType);
                }
            }
            for (size_t i = 0; i < m_preprocessData.m_constantData.size(); i++) {
                auto constType = m_preprocessData.m_constantData[i].first.type();
                if (!Walrus::hasCPUWordAlignedSize(constType) && !needsCPUWordAlignedAddress(constType)) {
                    m_preprocessData.m_constantData[i].second = m_initialFunctionStackSize;
                    m_initialFunctionStackSize += Walrus::valueSize(constType);
                }
            }

            if (m_initialFunctionStackSize % sizeof(size_t)) {
                m_initialFunctionStackSize += (sizeof(size_t) - m_initialFunctionStackSize % sizeof(size_t));
            }
        }
#endif

        m_functionStackSizeSoFar = m_initialFunctionStackSize;
        m_currentFunction->m_requiredStackSize = std::max(m_currentFunction->m_requiredStackSize, m_functionStackSizeSoFar);

        if (false) {
            // Explicit init local variable if needs
            for (size_t i = m_currentFunctionType->param().size(); i < m_localInfo.size(); i++) {
                if (m_preprocessData.m_localVariableInfo[i].m_needsExplicitInitOnStartup) {
#if false
                        auto localPos = m_localInfo[i].m_position;
                        auto size = Walrus::valueSize(m_localInfo[i].m_valueType);
                        if (size == 4) {
                            pushByteCode(Walrus::Const32(localPos, 0), WASMOpcode::I32ConstOpcode);
                        } else if (size == 8) {
                            pushByteCode(Walrus::Const64(localPos, 0), WASMOpcode::I64ConstOpcode);
                        } else {
                            ASSERT(size == 16);
                            uint8_t empty[16] = {
                                0,
                            };
                            pushByteCode(Walrus::Const128(localPos, empty), WASMOpcode::V128ConstOpcode);
                        }
                    }
#endif
                    // #if !defined(NDEBUG)
                    //                     m_currentFunction->m_localDebugData.push_back(m_localInfo[i].m_position);
                    // #endif
                }

                // init constant space
                for (size_t i = 0; i < m_preprocessData.m_constantData.size(); i++) {
#if false
                    const auto& constValue = m_preprocessData.m_constantData[i].first;
                    auto constType = m_preprocessData.m_constantData[i].first.type();
                    auto constPos = m_preprocessData.m_constantData[i].second;
                    size_t constSize = Walrus::valueSize(constType);

                    uint8_t constantBuffer[16];
                    constValue.writeToMemory(constantBuffer);
                    if (constSize == 4) {
                        pushByteCode(Walrus::Const32(constPos, *reinterpret_cast<uint32_t*>(constantBuffer)), WASMOpcode::I32ConstOpcode);
                    } else if (constSize == 8) {
                        pushByteCode(Walrus::Const64(constPos, *reinterpret_cast<uint64_t*>(constantBuffer)), WASMOpcode::I64ConstOpcode);
                    } else {
                        ASSERT(constSize == 16);
                        pushByteCode(Walrus::Const128(constPos, constantBuffer), WASMOpcode::V128ConstOpcode);
                    }
#endif
                    // #if !defined(NDEBUG)
                    //                     m_currentFunction->m_constantDebugData.pushBack(m_preprocessData.m_constantData[i]);
                    // #endif
                }
            }
        }
    }

    virtual void OnOpcode(uint32_t opcode) override
    {
    }

    uint16_t computeFunctionParameterOrResultOffsetCount(const Walrus::TypeVector& types)
    {
        uint16_t result = 0;
        for (auto t : types.types()) {
            result += valueFunctionCopyCount(t);
        }
        return result;
    }

    template <typename CodeType>
    void generateCallExpr(CodeType* code, uint16_t parameterCount, uint16_t resultCount,
                          Walrus::FunctionType* functionType)
    {
        size_t offsetIndex = 0;
        const Walrus::TypeVector::Types& param = functionType->param().types();
        size_t siz = param.size();

        for (size_t i = 0; i < siz; i++) {
            ASSERT(toDebugType(peekVMStackValueType()) == toDebugType(param[siz - i - 1]));
            size_t sourcePos = popVMStack();
            auto type = param[siz - i - 1];
            size_t s = Walrus::valueSize(type);
            size_t offsetSubCount = 0;
            size_t subIndexCount = valueFunctionCopyCount(type);
            for (size_t j = 0; j < s; j += sizeof(size_t)) {
                code->stackOffsets()[parameterCount - offsetIndex - subIndexCount + offsetSubCount++] = sourcePos + j;
            }
            offsetIndex += subIndexCount;
        }

        const Walrus::TypeVector::Types& result = functionType->result().types();
        siz = result.size();
        for (size_t i = 0; i < siz; i++) {
            size_t dstPos = pushVMStack(result[i]);
            size_t itemSize = Walrus::valueSize(result[i]);
            for (size_t j = 0; j < itemSize; j += sizeof(size_t)) {
                code->stackOffsets()[offsetIndex++] = dstPos + j;
            }
        }
        ASSERT(offsetIndex == (code->parameterOffsetsSize() + code->resultOffsetsSize()));
    }

    virtual void OnCallExpr(uint32_t index) override
    {
        auto functionType = m_result.m_functions[index]->functionType();
        auto callPos = m_currentFunction->currentByteCodeSize();
        auto parameterCount = computeFunctionParameterOrResultOffsetCount(functionType->param());
        auto resultCount = computeFunctionParameterOrResultOffsetCount(functionType->result());
        pushByteCode(Walrus::Call(index, parameterCount, resultCount), WASMOpcode::CallOpcode);

        m_currentFunction->expandByteCode(Walrus::ByteCode::pointerAlignedSize(sizeof(Walrus::ByteCodeStackOffset) * (parameterCount + resultCount)));
        ASSERT(m_currentFunction->currentByteCodeSize() % sizeof(void*) == 0);
        auto code = m_currentFunction->peekByteCode<Walrus::Call>(callPos);

        generateCallExpr(code, parameterCount, resultCount, functionType);
    }

    virtual void OnCallIndirectExpr(Index sigIndex, Index tableIndex) override
    {
        ASSERT(peekVMStackValueType() == Walrus::Value::I32);
        auto functionType = getFunctionType(sigIndex);
        auto callPos = m_currentFunction->currentByteCodeSize();
        auto parameterCount = computeFunctionParameterOrResultOffsetCount(functionType->param());
        auto resultCount = computeFunctionParameterOrResultOffsetCount(functionType->result());
        pushByteCode(Walrus::CallIndirect(popVMStack(), tableIndex, functionType, parameterCount, resultCount),
                     WASMOpcode::CallIndirectOpcode);
        m_currentFunction->expandByteCode(Walrus::ByteCode::pointerAlignedSize(sizeof(Walrus::ByteCodeStackOffset) * (parameterCount + resultCount)));
        ASSERT(m_currentFunction->currentByteCodeSize() % sizeof(void*) == 0);

        auto code = m_currentFunction->peekByteCode<Walrus::CallIndirect>(callPos);
        generateCallExpr(code, parameterCount, resultCount, functionType);
    }

    virtual void OnCallRefExpr(Type sig_type) override
    {
        ASSERT(Walrus::Value::isRefType(peekVMStackValueType()));
        auto functionType = getFunctionType(sig_type.GetReferenceIndex());
        auto callPos = m_currentFunction->currentByteCodeSize();
        auto parameterCount = computeFunctionParameterOrResultOffsetCount(functionType->param());
        auto resultCount = computeFunctionParameterOrResultOffsetCount(functionType->result());
        pushByteCode(Walrus::CallRef(popVMStack(), functionType, parameterCount, resultCount), WASMOpcode::CallRefOpcode);
        m_currentFunction->expandByteCode(Walrus::ByteCode::pointerAlignedSize(sizeof(Walrus::ByteCodeStackOffset) * (parameterCount + resultCount)));
        ASSERT(m_currentFunction->currentByteCodeSize() % sizeof(void*) == 0);

        auto code = m_currentFunction->peekByteCode<Walrus::CallRef>(callPos);
        generateCallExpr(code, parameterCount, resultCount, functionType);
    }

    bool processConstValue(const Walrus::Value& value)
    {
        if (!m_inInitExpr) {
            m_preprocessData.addConstantData(value);
            if (!m_preprocessData.m_inPreprocess) {
                for (size_t i = 0; i < m_preprocessData.m_constantData.size(); i++) {
                    if (m_preprocessData.m_constantData[i].first == value) {
                        pushVMStack(value.type(), m_preprocessData.m_constantData[i].second);
                        return true;
                    }
                }
            }
        }
        return false;
    }


    virtual void OnI32ConstExpr(uint32_t value) override
    {
        if (processConstValue(Walrus::Value(Walrus::Value::Type::I32, reinterpret_cast<uint8_t*>(&value)))) {
            return;
        }
        pushByteCode(Walrus::Const32(computeExprResultPosition(Walrus::Value::Type::I32), value), WASMOpcode::I32ConstOpcode);
    }

    virtual void OnI64ConstExpr(uint64_t value) override
    {
        if (processConstValue(Walrus::Value(Walrus::Value::Type::I64, reinterpret_cast<uint8_t*>(&value)))) {
            return;
        }
        pushByteCode(Walrus::Const64(computeExprResultPosition(Walrus::Value::Type::I64), value), WASMOpcode::I64ConstOpcode);
    }

    virtual void OnF32ConstExpr(uint32_t value) override
    {
        if (processConstValue(Walrus::Value(Walrus::Value::Type::F32, reinterpret_cast<uint8_t*>(&value)))) {
            return;
        }
        pushByteCode(Walrus::Const32(computeExprResultPosition(Walrus::Value::Type::F32), value), WASMOpcode::F32ConstOpcode);
    }

    virtual void OnF64ConstExpr(uint64_t value) override
    {
        if (processConstValue(Walrus::Value(Walrus::Value::Type::F64, reinterpret_cast<uint8_t*>(&value)))) {
            return;
        }
        pushByteCode(Walrus::Const64(computeExprResultPosition(Walrus::Value::Type::F64), value), WASMOpcode::F64ConstOpcode);
    }

    virtual void OnV128ConstExpr(uint8_t* value) override
    {
        if (processConstValue(Walrus::Value(Walrus::Value::Type::V128, value))) {
            return;
        }
        pushByteCode(Walrus::Const128(computeExprResultPosition(Walrus::Value::Type::V128), value), WASMOpcode::V128ConstOpcode);
    }

    size_t computeExprResultPosition(Walrus::Value::Type type)
    {
        if (!m_preprocessData.m_inPreprocess && !m_localInfo.empty()) {
            // if there is local.set code ahead,
            // we can use local variable position as expr target position
            auto localSetInfo = readAheadLocalGetIfExists();
            if (localSetInfo.first) {
                auto pos = m_localInfo[localSetInfo.first.value()].m_position;
                // skip local.set opcode
                *m_readerOffsetPointer += localSetInfo.second;
                return pos;
            }
        }

        return pushVMStack(type);
    }

    virtual void OnLocalGetExpr(Index localIndex) override
    {
        auto localPos = m_localInfo[localIndex].m_position;
        auto localValueType = m_localInfo[localIndex].m_valueType;

        bool canUseDirectReference = true;
        size_t pos = *m_readerOffsetPointer;
        for (const auto& r : m_preprocessData.m_localVariableInfo[localIndex].m_usageInfo) {
            if (r.m_startPosition <= pos && pos <= r.m_endPosition) {
                if (r.m_hasWriteUsage) {
                    canUseDirectReference = false;
                    break;
                }
            }
        }

        if (canUseDirectReference) {
            pushVMStack(localValueType, localPos, localIndex);
        } else {
            auto pos = m_functionStackSizeSoFar;
            pushVMStack(localValueType, pos, localIndex);
            generateMoveCodeIfNeeds(localPos, pos, localValueType);
        }
    }

    virtual void OnLocalSetExpr(Index localIndex) override
    {
        auto localPos = m_localInfo[localIndex].m_position;

        ASSERT(toDebugType(m_localInfo[localIndex].m_valueType) == toDebugType(peekVMStackValueType()));
        auto src = popVMStackInfo();
        generateMoveCodeIfNeeds(src.position(), localPos, src.valueType());
        m_preprocessData.addLocalVariableWrite(localIndex);
    }

    virtual void OnLocalTeeExpr(Index localIndex) override
    {
        auto valueType = m_localInfo[localIndex].m_valueType;
        auto localPos = m_localInfo[localIndex].m_position;
        ASSERT(toDebugType(valueType) == toDebugType(peekVMStackValueType()));
        auto dstInfo = peekVMStackInfo();
        generateMoveCodeIfNeeds(dstInfo.position(), localPos, valueType);
        m_preprocessData.addLocalVariableWrite(localIndex);
    }

    virtual void OnGlobalGetExpr(Index index) override
    {
        auto valueType = m_result.m_globalTypes[index]->type();
        auto sz = Walrus::valueSize(valueType);
        auto stackPos = computeExprResultPosition(valueType);
        if (sz == 4) {
            pushByteCode(Walrus::GlobalGet32(stackPos, index), WASMOpcode::GlobalGetOpcode);
        } else if (sz == 8) {
            pushByteCode(Walrus::GlobalGet64(stackPos, index), WASMOpcode::GlobalGetOpcode);
        } else {
            ASSERT(sz == 16);
            pushByteCode(Walrus::GlobalGet128(stackPos, index), WASMOpcode::GlobalGetOpcode);
        }
    }

    virtual void OnGlobalSetExpr(Index index) override
    {
        auto valueType = m_result.m_globalTypes[index]->type();
        auto stackPos = peekVMStack();

        ASSERT(toDebugType(peekVMStackValueType()) == toDebugType(valueType));
        auto sz = Walrus::valueSize(valueType);
        if (sz == 4) {
            pushByteCode(Walrus::GlobalSet32(stackPos, index), WASMOpcode::GlobalSetOpcode);
        } else if (sz == 8) {
            pushByteCode(Walrus::GlobalSet64(stackPos, index), WASMOpcode::GlobalSetOpcode);
        } else {
            pushByteCode(Walrus::GlobalSet128(stackPos, index), WASMOpcode::GlobalSetOpcode);
        }
        popVMStack();
    }

    virtual void OnDropExpr() override
    {
        popVMStack();
    }

    virtual void OnBinaryExpr(uint32_t opcode) override
    {
        auto code = static_cast<WASMOpcode>(opcode);
        ASSERT(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_paramTypes[1]) == peekVMStackValueType());
        auto src1 = popVMStack();
        ASSERT(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_paramTypes[0]) == peekVMStackValueType());
        auto src0 = popVMStack();
        auto dst = computeExprResultPosition(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_resultType));
        generateBinaryCode(code, src0, src1, dst);
    }

    virtual void OnUnaryExpr(uint32_t opcode) override
    {
        auto code = static_cast<WASMOpcode>(opcode);
        ASSERT(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_paramTypes[0]) == peekVMStackValueType());
        auto src = popVMStack();
        auto dst = computeExprResultPosition(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_resultType));
        generateUnaryCode(code, src, dst);
    }

    virtual void OnTernaryExpr(uint32_t opcode) override
    {
        auto code = static_cast<WASMOpcode>(opcode);
        ASSERT(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_paramTypes[2]) == peekVMStackValueType());
        auto src2 = popVMStack();
        ASSERT(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_paramTypes[1]) == peekVMStackValueType());
        auto src1 = popVMStack();
        ASSERT(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_paramTypes[0]) == peekVMStackValueType());
        auto src0 = popVMStack();
        auto dst = computeExprResultPosition(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_resultType));
        switch (code) {
#define GENERATE_TERNARY_CODE_CASE(name, ...)                    \
    case WASMOpcode::name##Opcode: {                             \
        pushByteCode(Walrus::name(src0, src1, src2, dst), code); \
        break;                                                   \
    }
            FOR_EACH_BYTECODE_RELAXED_SIMD_TERNARY_OP(GENERATE_TERNARY_CODE_CASE)
            FOR_EACH_BYTECODE_RELAXED_SIMD_TERNARY_OTHER(GENERATE_TERNARY_CODE_CASE)
#undef GENERATE_TERNARY_CODE_CASE
        case WASMOpcode::V128BitSelectOpcode:
            pushByteCode(Walrus::V128BitSelect(src0, src1, src2, dst), code);
            break;
        default:
            ASSERT_NOT_REACHED();
            break;
        }
    }

    virtual void OnIfExpr(Type sigType) override
    {
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto stackPos = popVMStack();

        bool isInverted = canBeInverted(stackPos);
        if (UNLIKELY(isInverted)) {
            stackPos = m_currentFunction->peekByteCode<Walrus::UnaryOperation>(m_lastI32EqzPos)->srcOffset();
            m_currentFunction->resizeByteCode(m_lastI32EqzPos);
            m_lastI32EqzPos = s_noI32Eqz;
        }

        BlockInfo b(BlockInfo::IfElse, sigType, *this);
        b.m_jumpToEndBrInfo.push_back({ BlockInfo::JumpToEndBrInfo::IsJumpIf, b.m_position });
        m_blockInfo.push_back(b);

        if (UNLIKELY(isInverted)) {
            pushByteCode(Walrus::JumpIfTrue(stackPos), WASMOpcode::IfOpcode);
        } else {
            pushByteCode(Walrus::JumpIfFalse(stackPos), WASMOpcode::IfOpcode);
        }
        m_preprocessData.seenBranch();
    }

    void restoreVMStackBy(const BlockInfo& blockInfo)
    {
        if (blockInfo.m_vmStack.size() <= m_vmStack.size()) {
            size_t diff = m_vmStack.size() - blockInfo.m_vmStack.size();
            for (size_t i = 0; i < diff; i++) {
                popVMStack();
            }
            ASSERT(blockInfo.m_vmStack.size() == m_vmStack.size());
        }
        m_vmStack = blockInfo.m_vmStack;
        m_functionStackSizeSoFar = blockInfo.m_functionStackSizeSoFar;
    }

    void keepBlockResultsIfNeeds(BlockInfo& blockInfo)
    {
        auto dropSize = dropStackValuesBeforeBrIfNeeds(0);
        keepBlockResultsIfNeeds(blockInfo, dropSize);
    }

    void keepBlockResultsIfNeeds(BlockInfo& blockInfo, const std::pair<size_t, size_t>& dropSize)
    {
        if (blockInfo.shouldRestoreVMStackAtEnd()) {
            if (!blockInfo.byteCodeGenerationStopped()) {
                if (blockInfo.returnValueIsIndex()) {
                    auto ft = getFunctionType(blockInfo.m_returnValueIndex);
                    const auto& result = ft->result().types();
                    for (size_t i = 0; i < result.size(); i++) {
                        ASSERT(toDebugType(peekVMStackValueType()) == toDebugType(result[result.size() - i - 1]));
                        auto info = peekVMStackInfo();
                        generateMoveCodeIfNeeds(info.position(), info.nonOptimizedPosition(), info.valueType());
                        info.setPosition(info.nonOptimizedPosition());
                        popVMStack();
                    }
                } else if (blockInfo.m_returnValueType != Walrus::Value::Void) {
                    ASSERT(toDebugType(peekVMStackValueType()) == toDebugType(blockInfo.m_returnValueType));
                    auto info = peekVMStackInfo();
                    generateMoveCodeIfNeeds(info.position(), info.nonOptimizedPosition(), info.valueType());
                    info.setPosition(info.nonOptimizedPosition());
                    popVMStack();
                }
            }
        }
    }

    virtual void OnElseExpr() override
    {
        m_preprocessData.seenBranch();
        BlockInfo& blockInfo = m_blockInfo.back();
        keepBlockResultsIfNeeds(blockInfo);

        ASSERT(blockInfo.m_blockType == BlockInfo::IfElse);
        blockInfo.m_jumpToEndBrInfo.erase(blockInfo.m_jumpToEndBrInfo.begin());

        if (!blockInfo.byteCodeGenerationStopped()) {
            blockInfo.m_jumpToEndBrInfo.push_back({ BlockInfo::JumpToEndBrInfo::IsJump, m_currentFunction->currentByteCodeSize() });
            pushByteCode(Walrus::Jump(), WASMOpcode::ElseOpcode);
        }

        blockInfo.clearByteCodeGenerationStopped();
        restoreVMStackBy(blockInfo);
        m_currentFunction->peekByteCode<Walrus::JumpIfFalse>(blockInfo.m_position)
            ->setOffset(m_currentFunction->currentByteCodeSize() - blockInfo.m_position);
    }

    virtual void OnLoopExpr(Type sigType) override
    {
        BlockInfo b(BlockInfo::Loop, sigType, *this);
        m_blockInfo.push_back(b);
    }

    virtual void OnBlockExpr(Type sigType) override
    {
        BlockInfo b(BlockInfo::Block, sigType, *this);
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
            auto& blockInfo = m_blockInfo.back();
            blockInfo.m_flags |= BlockInfo::ShouldRestoreVMStackAtEnd | BlockInfo::ByteCodeGenerationStopped;
        } else {
            while (m_vmStack.size()) {
                popVMStack();
            }
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
                    dropValueSize += m_vmStack[i].stackAllocatedSize();
                }

                if (iter->m_blockType == BlockInfo::Loop) {
                    if (iter->returnValueIsIndex()) {
                        auto ft = getFunctionType(iter->m_returnValueIndex);
                        dropValueSize += ft->paramStackSize();
                        parameterSize += ft->paramStackSize();
                    }
                } else {
                    if (iter->returnValueIsIndex()) {
                        auto ft = getFunctionType(iter->m_returnValueIndex);
                        const auto& result = ft->result().types();
                        for (size_t i = 0; i < result.size(); i++) {
                            parameterSize += Walrus::valueStackAllocatedSize(result[i]);
                        }
                    } else if (iter->m_returnValueType != Walrus::Value::Void) {
                        parameterSize += Walrus::valueStackAllocatedSize(iter->m_returnValueType);
                    }
                }
            }
        } else if (m_blockInfo.size()) {
            auto iter = m_blockInfo.begin();
            size_t start = iter->m_vmStack.size();
            for (size_t i = start; i < m_vmStack.size(); i++) {
                dropValueSize += m_vmStack[i].stackAllocatedSize();
            }
        }

        return std::make_pair(dropValueSize, parameterSize);
    }

    void generateMoveCodeIfNeeds(size_t srcPosition, size_t dstPosition, Walrus::Value::Type type)
    {
        if (srcPosition != dstPosition) {
            switch (type) {
            case Walrus::Value::I32:
                pushByteCode(Walrus::MoveI32(srcPosition, dstPosition));
                break;
            case Walrus::Value::F32:
                pushByteCode(Walrus::MoveF32(srcPosition, dstPosition));
                break;
            case Walrus::Value::I64:
                pushByteCode(Walrus::MoveI64(srcPosition, dstPosition));
                break;
            case Walrus::Value::F64:
                pushByteCode(Walrus::MoveF64(srcPosition, dstPosition));
                break;
            case Walrus::Value::V128:
                pushByteCode(Walrus::MoveV128(srcPosition, dstPosition));
                break;
            default:
                ASSERT(Walrus::Value::isRefType(type));

                if (sizeof(size_t) == 4) {
                    pushByteCode(Walrus::MoveI32(srcPosition, dstPosition));
                } else {
                    pushByteCode(Walrus::MoveI64(srcPosition, dstPosition));
                }
                break;
            }
        }
    }

    void generateMoveValuesCodeRegardToDrop(std::pair<size_t, size_t> dropSize)
    {
        ASSERT(dropSize.second);
        int64_t remainSize = dropSize.second;
        auto srcIter = m_vmStack.rbegin();
        while (true) {
            remainSize -= srcIter->stackAllocatedSize();
            if (!remainSize) {
                break;
            }
            if (remainSize < 0) {
                // stack mismatch! we don't need to generate code
                return;
            }
            srcIter++;
        }

        remainSize = dropSize.first;
        auto dstIter = m_vmStack.rbegin();
        while (true) {
            remainSize -= dstIter->stackAllocatedSize();
            if (!remainSize) {
                break;
            }
            if (remainSize < 0) {
                // stack mismatch! we don't need to generate code
                return;
            }
            dstIter++;
        }

        // reverse order copy to protect newer values
        remainSize = dropSize.second;
        while (true) {
            generateMoveCodeIfNeeds(srcIter->position(), dstIter->nonOptimizedPosition(), srcIter->valueType());
            remainSize -= srcIter->stackAllocatedSize();
            if (!remainSize) {
                break;
            }
            srcIter--;
            dstIter--;
        }
    }

    void generateEndCode(bool shouldClearVMStack = false)
    {
        if (UNLIKELY(m_currentFunctionType->result().size() > m_vmStack.size())) {
            // error case of global init expr
            return;
        }
        auto pos = m_currentFunction->currentByteCodeSize();
        auto offsetCount = computeFunctionParameterOrResultOffsetCount(m_currentFunctionType->result());
        pushByteCode(Walrus::End(offsetCount), WASMOpcode::EndOpcode);

        auto& result = m_currentFunctionType->result().types();
        m_currentFunction->expandByteCode(Walrus::ByteCode::pointerAlignedSize(sizeof(Walrus::ByteCodeStackOffset) * offsetCount));
        ASSERT(m_currentFunction->currentByteCodeSize() % sizeof(void*) == 0);
        Walrus::End* end = m_currentFunction->peekByteCode<Walrus::End>(pos);
        size_t offsetIndex = 0;
        for (size_t i = 0; i < result.size(); i++) {
            auto type = result[result.size() - 1 - i];
            size_t s = Walrus::valueSize(type);
            size_t offsetSubCount = 0;
            size_t subIndexCount = valueFunctionCopyCount(type);
            for (size_t j = 0; j < s; j += sizeof(size_t)) {
                end->resultOffsets()[offsetCount - offsetIndex - subIndexCount + offsetSubCount++] = (m_vmStack.rbegin() + i)->position() + j;
            }
            offsetIndex += subIndexCount;
        }
        ASSERT(offsetIndex == computeFunctionParameterOrResultOffsetCount(m_currentFunctionType->result()));

        if (shouldClearVMStack) {
            for (size_t i = 0; i < result.size(); i++) {
                popVMStack();
            }
        }
    }

    void generateFunctionReturnCode(bool shouldClearVMStack = false)
    {
#if !defined(NDEBUG)
        for (size_t i = 0; i < m_currentFunctionType->result().size(); i++) {
            ASSERT(toDebugType((m_vmStack.rbegin() + i)->valueType()) == toDebugType(m_currentFunctionType->result().types()[m_currentFunctionType->result().size() - i - 1]));
        }
#endif
        generateEndCode();
        if (shouldClearVMStack) {
            auto dropSize = dropStackValuesBeforeBrIfNeeds(m_blockInfo.size()).first;
            while (dropSize) {
                dropSize -= popVMStackInfo().stackAllocatedSize();
            }
        } else {
            for (size_t i = 0; i < m_currentFunctionType->result().size(); i++) {
                popVMStack();
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
        m_preprocessData.seenBranch();
        if (m_blockInfo.size() == depth) {
            // this case acts like return
            generateFunctionReturnCode(true);
            stopToGenerateByteCodeWhileBlockEnd();
            return;
        }
        auto& blockInfo = findBlockInfoInBr(depth);
        auto offset = (int32_t)blockInfo.m_position - (int32_t)m_currentFunction->currentByteCodeSize();
        auto dropSize = dropStackValuesBeforeBrIfNeeds(depth);
        if (dropSize.second) {
            generateMoveValuesCodeRegardToDrop(dropSize);
        } else if (blockInfo.m_blockType == BlockInfo::Loop && blockInfo.returnValueIsIndex() && getFunctionType(blockInfo.m_returnValueIndex)->param().size()) {
            size_t pos = m_currentFunction->currentByteCodeSize();

            auto ft = getFunctionType(blockInfo.m_returnValueIndex);
            const auto& param = ft->param().types();
            for (size_t i = 0; i < param.size(); i++) {
                ASSERT((m_vmStack.rbegin() + i)->valueType() == param[param.size() - i - 1]);
                auto info = m_vmStack.rbegin() + i;
                generateMoveCodeIfNeeds(info->position(), info->nonOptimizedPosition(), info->valueType());
                info->setPosition(info->nonOptimizedPosition());
            }
        }
        if (blockInfo.m_blockType != BlockInfo::Loop) {
            ASSERT(blockInfo.m_blockType == BlockInfo::Block || blockInfo.m_blockType == BlockInfo::IfElse || blockInfo.m_blockType == BlockInfo::TryCatch);
            blockInfo.m_jumpToEndBrInfo.push_back({ BlockInfo::JumpToEndBrInfo::IsJump, m_currentFunction->currentByteCodeSize() });
        }
        pushByteCode(Walrus::Jump(offset), WASMOpcode::BrOpcode);

        stopToGenerateByteCodeWhileBlockEnd();
    }

    template <typename JumpType, typename JumpTypeInverted, WASMOpcode opcode>
    size_t GenerateConditionalBranch(Index depth, size_t stackPos)
    {
        if (m_blockInfo.size() == depth) {
            // this case acts like return
            size_t pos = m_currentFunction->currentByteCodeSize();
            pushByteCode(JumpTypeInverted(stackPos, sizeof(JumpTypeInverted) + sizeof(Walrus::End) + sizeof(Walrus::ByteCodeStackOffset) * m_currentFunctionType->result().size()), opcode);
            for (size_t i = 0; i < m_currentFunctionType->result().size(); i++) {
                ASSERT(toDebugType((m_vmStack.rbegin() + i)->valueType()) == toDebugType(m_currentFunctionType->result().types()[m_currentFunctionType->result().size() - i - 1]));
            }
            generateEndCode();
            m_currentFunction->peekByteCode<JumpTypeInverted>(pos)->setOffset(m_currentFunction->currentByteCodeSize() - pos);
            return pos;
        }

        auto& blockInfo = findBlockInfoInBr(depth);
        auto dropSize = dropStackValuesBeforeBrIfNeeds(depth);
        if (dropSize.second) {
            size_t pos = m_currentFunction->currentByteCodeSize();
            pushByteCode(JumpTypeInverted(stackPos), opcode);
            generateMoveValuesCodeRegardToDrop(dropSize);

            auto offset = (int32_t)blockInfo.m_position - (int32_t)m_currentFunction->currentByteCodeSize();
            if (blockInfo.m_blockType != BlockInfo::Loop) {
                ASSERT(blockInfo.m_blockType == BlockInfo::Block || blockInfo.m_blockType == BlockInfo::IfElse || blockInfo.m_blockType == BlockInfo::TryCatch);
                blockInfo.m_jumpToEndBrInfo.push_back({ BlockInfo::JumpToEndBrInfo::IsJump, m_currentFunction->currentByteCodeSize() });
            }
            pushByteCode(Walrus::Jump(offset), WASMOpcode::BrIfOpcode);
            m_currentFunction->peekByteCode<JumpTypeInverted>(pos)->setOffset(m_currentFunction->currentByteCodeSize() - pos);
            return pos;
        }

        if (blockInfo.m_blockType == BlockInfo::Loop && blockInfo.returnValueIsIndex() && getFunctionType(blockInfo.m_returnValueIndex)->param().size()) {
            size_t pos = m_currentFunction->currentByteCodeSize();
            pushByteCode(JumpTypeInverted(stackPos), opcode);
            auto ft = getFunctionType(blockInfo.m_returnValueIndex);
            const auto& param = ft->param().types();
            for (size_t i = 0; i < param.size(); i++) {
                ASSERT(toDebugType((m_vmStack.rbegin() + i)->valueType()) == toDebugType(param[param.size() - i - 1]));
                auto info = m_vmStack.rbegin() + i;
                generateMoveCodeIfNeeds(info->position(), info->nonOptimizedPosition(), info->valueType());
                info->setPosition(info->nonOptimizedPosition());
            }

            auto offset = (int32_t)blockInfo.m_position - (int32_t)m_currentFunction->currentByteCodeSize();
            if (blockInfo.m_blockType != BlockInfo::Loop) {
                ASSERT(blockInfo.m_blockType == BlockInfo::Block || blockInfo.m_blockType == BlockInfo::IfElse || blockInfo.m_blockType == BlockInfo::TryCatch);
                blockInfo.m_jumpToEndBrInfo.push_back({ BlockInfo::JumpToEndBrInfo::IsJump, m_currentFunction->currentByteCodeSize() });
            }
            pushByteCode(Walrus::Jump(offset), WASMOpcode::BrIfOpcode);
            m_currentFunction->peekByteCode<JumpTypeInverted>(pos)->setOffset(m_currentFunction->currentByteCodeSize() - pos);
            return pos;
        }

        auto offset = (int32_t)blockInfo.m_position - (int32_t)m_currentFunction->currentByteCodeSize();
        if (blockInfo.m_blockType != BlockInfo::Loop) {
            ASSERT(blockInfo.m_blockType == BlockInfo::Block || blockInfo.m_blockType == BlockInfo::IfElse || blockInfo.m_blockType == BlockInfo::TryCatch);
            blockInfo.m_jumpToEndBrInfo.push_back({ BlockInfo::JumpToEndBrInfo::IsJumpIf, m_currentFunction->currentByteCodeSize() });
        }

        size_t pos = m_currentFunction->currentByteCodeSize();
        pushByteCode(JumpType(stackPos, offset), opcode);
        return pos;
    }

    virtual void OnBrIfExpr(Index depth) override
    {
        m_preprocessData.seenBranch();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        size_t stackPos = popVMStack();
        bool isInverted = canBeInverted(stackPos);

        if (UNLIKELY(isInverted)) {
            stackPos = m_currentFunction->peekByteCode<Walrus::UnaryOperation>(m_lastI32EqzPos)->srcOffset();
            m_currentFunction->resizeByteCode(m_lastI32EqzPos);
            m_lastI32EqzPos = s_noI32Eqz;
            GenerateConditionalBranch<Walrus::JumpIfFalse, Walrus::JumpIfTrue, WASMOpcode::BrIfOpcode>(depth, stackPos);
        } else {
            GenerateConditionalBranch<Walrus::JumpIfTrue, Walrus::JumpIfFalse, WASMOpcode::BrIfOpcode>(depth, stackPos);
        }
    }

    virtual void OnBrOnNonNullExpr(Index depth) override
    {
        m_preprocessData.seenBranch();
        ASSERT(Walrus::Value::isRefType(peekVMStackValueType()));
        VMStackInfo& info = peekVMStackInfo();
        info.toNonNullableRef();
        GenerateConditionalBranch<Walrus::JumpIfNonNull, Walrus::JumpIfNull, WASMOpcode::BrIfOpcode>(depth, info.position());
        popVMStack();
    }

    virtual void OnBrOnNullExpr(Index depth) override
    {
        m_preprocessData.seenBranch();
        ASSERT(Walrus::Value::isRefType(peekVMStackValueType()));
        // Temporarily remove the top element of the stack for the sake of variable copying.
        VMStackInfo info = m_vmStack.back();
        m_vmStack.pop_back();
        GenerateConditionalBranch<Walrus::JumpIfNull, Walrus::JumpIfNonNull, WASMOpcode::BrIfOpcode>(depth, info.position());
        info.toNonNullableRef();
        m_vmStack.push_back(info);
    }

    void emitBrTableCase(size_t brTableCode, Index depth, size_t jumpOffset)
    {
        int32_t offset = (int32_t)(m_currentFunction->currentByteCodeSize() - brTableCode);

        if (m_blockInfo.size() == depth) {
            // this case acts like return
#if !defined(NDEBUG)
            for (size_t i = 0; i < m_currentFunctionType->result().size(); i++) {
                ASSERT((m_vmStack.rbegin() + i)->valueType() == m_currentFunctionType->result().types()[m_currentFunctionType->result().size() - i - 1]);
            }
#endif
            *(int32_t*)(m_currentFunction->peekByteCode<uint8_t>(brTableCode) + jumpOffset) = offset;
            generateEndCode();
            return;
        }

        auto dropSize = dropStackValuesBeforeBrIfNeeds(depth);

        if (UNLIKELY(dropSize.second)) {
            *(int32_t*)(m_currentFunction->peekByteCode<uint8_t>(brTableCode) + jumpOffset) = offset;
            OnBrExpr(depth);
            return;
        }

        auto& blockInfo = findBlockInfoInBr(depth);

        offset = (int32_t)(blockInfo.m_position - brTableCode);

        if (blockInfo.m_blockType != BlockInfo::Loop) {
            ASSERT(blockInfo.m_blockType == BlockInfo::Block || blockInfo.m_blockType == BlockInfo::IfElse || blockInfo.m_blockType == BlockInfo::TryCatch);
            offset = jumpOffset;
            blockInfo.m_jumpToEndBrInfo.push_back({ BlockInfo::JumpToEndBrInfo::IsBrTable, brTableCode + jumpOffset });
        }

        *(int32_t*)(m_currentFunction->peekByteCode<uint8_t>(brTableCode) + jumpOffset) = offset;
    }

    virtual void OnBrTableExpr(Index numTargets, Index* targetDepths, Index defaultTargetDepth) override
    {
        m_preprocessData.seenBranch();
        ASSERT(peekVMStackValueType() == Walrus::Value::I32);
        auto stackPos = popVMStack();

        size_t brTableCode = m_currentFunction->currentByteCodeSize();
        pushByteCode(Walrus::BrTable(stackPos, numTargets), WASMOpcode::BrTableOpcode);

        if (numTargets) {
            m_currentFunction->expandByteCode(Walrus::ByteCode::pointerAlignedSize(sizeof(int32_t) * numTargets));
            ASSERT(m_currentFunction->currentByteCodeSize() % sizeof(void*) == 0);

            for (Index i = 0; i < numTargets; i++) {
                emitBrTableCase(brTableCode, targetDepths[i], sizeof(Walrus::BrTable) + i * sizeof(int32_t));
            }
        }

        // generate default
        emitBrTableCase(brTableCode, defaultTargetDepth, Walrus::BrTable::offsetOfDefault());
        stopToGenerateByteCodeWhileBlockEnd();
    }

    virtual void OnSelectExpr(Index resultCount, Type* resultTypes) override
    {
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        ASSERT(resultCount == 0 || resultCount == 1);
        auto stackPos = popVMStack();

        auto type = peekVMStackValueType();
        auto src1 = popVMStack();
        auto src0 = popVMStack();
        auto dst = computeExprResultPosition(type);
        bool isFloat = type == Walrus::Value::F32 || type == Walrus::Value::F64;
        pushByteCode(Walrus::Select(stackPos, Walrus::valueSize(type), isFloat, src0, src1, dst), WASMOpcode::SelectOpcode);
    }

    virtual void OnThrowExpr(Index tagIndex) override
    {
        m_preprocessData.seenBranch();
        auto pos = m_currentFunction->currentByteCodeSize();
        uint32_t offsetsSize = 0;

        if (tagIndex != std::numeric_limits<Index>::max()) {
            offsetsSize = getFunctionType(m_result.m_tagTypes[tagIndex]->sigIndex())->param().size();
        }

        pushByteCode(Walrus::Throw(tagIndex, offsetsSize), WASMOpcode::ThrowOpcode);

        if (tagIndex != std::numeric_limits<Index>::max()) {
            auto functionType = getFunctionType(m_result.m_tagTypes[tagIndex]->sigIndex());
            auto& param = functionType->param().types();
            m_currentFunction->expandByteCode(Walrus::ByteCode::pointerAlignedSize(sizeof(Walrus::ByteCodeStackOffset) * param.size()));
            ASSERT(m_currentFunction->currentByteCodeSize() % sizeof(void*) == 0);
            Walrus::Throw* code = m_currentFunction->peekByteCode<Walrus::Throw>(pos);
            for (size_t i = 0; i < param.size(); i++) {
                ASSERT(peekVMStackValueType() == param[functionType->param().size() - i - 1]);
                code->dataOffsets()[param.size() - i - 1] = popVMStack();
            }
        }

        stopToGenerateByteCodeWhileBlockEnd();
    }

    virtual void OnTryExpr(Type sigType) override
    {
        BlockInfo b(BlockInfo::TryCatch, sigType, *this);
        m_blockInfo.push_back(b);
        m_currentFunction->m_hasTryCatch = true;
    }

    void processCatchExpr(Index tagIndex)
    {
        ASSERT(m_blockInfo.back().m_blockType == BlockInfo::TryCatch);

        m_preprocessData.seenBranch();
        auto& blockInfo = m_blockInfo.back();
        keepBlockResultsIfNeeds(blockInfo);
        restoreVMStackBy(blockInfo);

        size_t tryEnd = m_currentFunction->currentByteCodeSize();
        if (m_catchInfo.size() && m_catchInfo.back().m_tryCatchBlockDepth == m_blockInfo.size()) {
            // not first catch
            tryEnd = m_catchInfo.back().m_tryEnd;
        }

        if (!blockInfo.byteCodeGenerationStopped()) {
            blockInfo.m_jumpToEndBrInfo.push_back({ BlockInfo::JumpToEndBrInfo::IsJump, m_currentFunction->currentByteCodeSize() });
            pushByteCode(Walrus::Jump(), WASMOpcode::CatchOpcode);
        }

        blockInfo.clearByteCodeGenerationStopped();

        m_catchInfo.push_back({ m_blockInfo.size(), m_blockInfo.back().m_position, tryEnd, m_currentFunction->currentByteCodeSize(), tagIndex });

        if (tagIndex != std::numeric_limits<Index>::max()) {
            auto functionType = getFunctionType(m_result.m_tagTypes[tagIndex]->sigIndex());
            auto& param = functionType->param().types();
            for (size_t i = 0; i < param.size(); i++) {
                pushVMStack(param[i]);
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
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src2 = popVMStack();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src1 = popVMStack();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src0 = popVMStack();

        pushByteCode(Walrus::MemoryInit(memidx, segmentIndex, src0, src1, src2), WASMOpcode::MemoryInitOpcode);
    }

    virtual void OnMemoryCopyExpr(Index srcMemIndex, Index dstMemIndex) override
    {
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src2 = popVMStack();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src1 = popVMStack();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src0 = popVMStack();

        pushByteCode(Walrus::MemoryCopy(srcMemIndex, dstMemIndex, src0, src1, src2), WASMOpcode::MemoryCopyOpcode);
    }

    virtual void OnMemoryFillExpr(Index memidx) override
    {
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src2 = popVMStack();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src1 = popVMStack();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src0 = popVMStack();

        pushByteCode(Walrus::MemoryFill(memidx, src0, src1, src2), WASMOpcode::MemoryFillOpcode);
    }

    virtual void OnDataDropExpr(Index segmentIndex) override
    {
        pushByteCode(Walrus::DataDrop(segmentIndex), WASMOpcode::DataDropOpcode);
    }

    virtual void OnMemoryGrowExpr(Index memidx) override
    {
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src = popVMStack();
        auto dst = computeExprResultPosition(Walrus::Value::Type::I32);
        pushByteCode(Walrus::MemoryGrow(memidx, src, dst), WASMOpcode::MemoryGrowOpcode);
    }

    virtual void OnMemorySizeExpr(Index memidx) override
    {
        auto stackPos = computeExprResultPosition(Walrus::Value::Type::I32);
        pushByteCode(Walrus::MemorySize(memidx, stackPos), WASMOpcode::MemorySizeOpcode);
    }

    virtual void OnTableGetExpr(Index tableIndex) override
    {
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src = popVMStack();
        auto dst = computeExprResultPosition(m_result.m_tableTypes[tableIndex]->type());
        pushByteCode(Walrus::TableGet(tableIndex, src, dst), WASMOpcode::TableGetOpcode);
    }

    virtual void OnTableSetExpr(Index tableIndex) override
    {
        ASSERT(toDebugType(peekVMStackValueType()) == toDebugType(m_result.m_tableTypes[tableIndex]->type()));
        auto src1 = popVMStack();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src0 = popVMStack();
        pushByteCode(Walrus::TableSet(tableIndex, src0, src1), WASMOpcode::TableSetOpcode);
    }

    virtual void OnTableGrowExpr(Index tableIndex) override
    {
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src1 = popVMStack();
        ASSERT(toDebugType(peekVMStackValueType()) == toDebugType(m_result.m_tableTypes[tableIndex]->type()));
        auto src0 = popVMStack();
        auto dst = computeExprResultPosition(Walrus::Value::Type::I32);
        pushByteCode(Walrus::TableGrow(tableIndex, src0, src1, dst), WASMOpcode::TableGrowOpcode);
    }

    virtual void OnTableSizeExpr(Index tableIndex) override
    {
        auto dst = computeExprResultPosition(Walrus::Value::Type::I32);
        pushByteCode(Walrus::TableSize(tableIndex, dst), WASMOpcode::TableSizeOpcode);
    }

    virtual void OnTableCopyExpr(Index dst_index, Index src_index) override
    {
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src2 = popVMStack();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src1 = popVMStack();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src0 = popVMStack();
        pushByteCode(Walrus::TableCopy(dst_index, src_index, src0, src1, src2), WASMOpcode::TableCopyOpcode);
    }

    virtual void OnTableFillExpr(Index tableIndex) override
    {
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src2 = popVMStack();
        ASSERT(toDebugType(peekVMStackValueType()) == toDebugType(m_result.m_tableTypes[tableIndex]->type()));
        auto src1 = popVMStack();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src0 = popVMStack();
        pushByteCode(Walrus::TableFill(tableIndex, src0, src1, src2), WASMOpcode::TableFillOpcode);
    }

    virtual void OnElemDropExpr(Index segmentIndex) override
    {
        pushByteCode(Walrus::ElemDrop(segmentIndex), WASMOpcode::ElemDropOpcode);
    }

    virtual void OnTableInitExpr(Index segmentIndex, Index tableIndex) override
    {
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src2 = popVMStack();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src1 = popVMStack();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src0 = popVMStack();
        pushByteCode(Walrus::TableInit(tableIndex, segmentIndex, src0, src1, src2), WASMOpcode::TableInitOpcode);
    }

    virtual void OnLoadExpr(int opcode, Index memidx, Address alignmentLog2, Address offset) override
    {
        auto code = static_cast<WASMOpcode>(opcode);
        ASSERT(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_paramTypes[0]) == peekVMStackValueType());
        auto src = popVMStack();
        auto dst = computeExprResultPosition(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_resultType));
        if ((opcode == (int)WASMOpcode::I32LoadOpcode || opcode == (int)WASMOpcode::F32LoadOpcode) && offset == 0 && memidx == 0) {
            pushByteCode(Walrus::Load32(src, dst), code);
        } else if ((opcode == (int)WASMOpcode::I64LoadOpcode || opcode == (int)WASMOpcode::F64LoadOpcode) && offset == 0 && memidx == 0) {
            pushByteCode(Walrus::Load64(src, dst), code);

        } else {
            generateMemoryLoadCode(code, memidx, alignmentLog2, offset, src, dst);
        }
    }

    virtual void OnStoreExpr(int opcode, Index memidx, Address alignmentLog2, Address offset) override
    {
        auto code = static_cast<WASMOpcode>(opcode);
        ASSERT(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_paramTypes[1]) == peekVMStackValueType());
        auto src1 = popVMStack();
        ASSERT(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_paramTypes[0]) == peekVMStackValueType());
        auto src0 = popVMStack();
        if ((opcode == (int)WASMOpcode::I32StoreOpcode || opcode == (int)WASMOpcode::F32StoreOpcode) && offset == 0 && memidx == 0) {
            pushByteCode(Walrus::Store32(src0, src1), code);
        } else if ((opcode == (int)WASMOpcode::I64StoreOpcode || opcode == (int)WASMOpcode::F64StoreOpcode) && offset == 0 && memidx == 0) {
            pushByteCode(Walrus::Store64(src0, src1), code);
        } else {
            generateMemoryStoreCode(code, memidx, alignmentLog2, offset, src0, src1);
        }
    }

    // Extended Features
    virtual void OnAtomicLoadExpr(int opcode, Index memidx, Address alignmentLog2, Address offset) override
    {
        auto code = static_cast<WASMOpcode>(opcode);
        ASSERT(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_paramTypes[0]) == peekVMStackValueType());
        auto src = popVMStack();
        auto dst = computeExprResultPosition(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_resultType));
        switch (code) {
#define GENERATE_LOAD_CODE_CASE(name, ...)                                               \
    case WASMOpcode::name##Opcode: {                                                     \
        if (memidx == 0) {                                                               \
            pushByteCode(Walrus::name(offset, src, dst), code);                          \
        } else {                                                                         \
            pushByteCode(Walrus::name##MemIdx(memidx, alignmentLog2, offset, src, dst)); \
        }                                                                                \
        break;                                                                           \
    }

            FOR_EACH_BYTECODE_ATOMIC_LOAD_OP(GENERATE_LOAD_CODE_CASE)
#undef GENERATE_LOAD_CODE_CASE
        default:
            ASSERT_NOT_REACHED();
        }
    }

    virtual void OnAtomicStoreExpr(int opcode, Index memidx, Address alignmentLog2, Address offset) override
    {
        auto code = static_cast<WASMOpcode>(opcode);
        ASSERT(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_paramTypes[1]) == peekVMStackValueType());
        auto src1 = popVMStack();
        ASSERT(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_paramTypes[0]) == peekVMStackValueType());
        auto src0 = popVMStack();
        switch (code) {
#define GENERATE_STORE_CODE_CASE(name, readType, writeType)                                \
    case WASMOpcode::name##Opcode: {                                                       \
        if (memidx == 0) {                                                                 \
            pushByteCode(Walrus::name(offset, src0, src1), code);                          \
        } else {                                                                           \
            pushByteCode(Walrus::name##MemIdx(memidx, alignmentLog2, offset, src0, src1)); \
        }                                                                                  \
        break;                                                                             \
    }

            FOR_EACH_BYTECODE_ATOMIC_STORE_OP(GENERATE_STORE_CODE_CASE)
#undef GENERATE_STORE_CODE_CASE
        default:
            ASSERT_NOT_REACHED();
        }
    }

    virtual void OnAtomicRmwExpr(int opcode, Index memidx, Address alignmentLog2, Address offset) override
    {
        auto code = static_cast<WASMOpcode>(opcode);
        ASSERT(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_paramTypes[1]) == peekVMStackValueType());
        auto src1 = popVMStack();
        ASSERT(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_paramTypes[0]) == peekVMStackValueType());
        auto src0 = popVMStack();
        auto dst = computeExprResultPosition(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_resultType));
        switch (code) {
#define GENERATE_RMW_CODE_CASE(name, ...)                                                       \
    case WASMOpcode::name##Opcode: {                                                            \
        if (memidx == 0) {                                                                      \
            pushByteCode(Walrus::name(offset, src0, src1, dst), code);                          \
        } else {                                                                                \
            pushByteCode(Walrus::name##MemIdx(memidx, alignmentLog2, offset, src0, src1, dst)); \
        }                                                                                       \
        break;                                                                                  \
    }

            FOR_EACH_BYTECODE_ATOMIC_RMW_OP(GENERATE_RMW_CODE_CASE)
#undef GENERATE_RMW_CODE_CASE
        default:
            ASSERT_NOT_REACHED();
        }
    }

    virtual void OnAtomicCmpxchgExpr(int opcode, Index memidx, Address alignmentLog2, Address offset) override
    {
        auto code = static_cast<WASMOpcode>(opcode);
        ASSERT(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_paramTypes[2]) == peekVMStackValueType());
        auto src2 = popVMStack();
        ASSERT(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_paramTypes[1]) == peekVMStackValueType());
        auto src1 = popVMStack();
        ASSERT(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_paramTypes[0]) == peekVMStackValueType());
        auto src0 = popVMStack();
        auto dst = computeExprResultPosition(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_resultType));
        switch (code) {
#define GENERATE_RMW_CMPXCHG_CODE_CASE(name, ...)                                                     \
    case WASMOpcode::name##Opcode: {                                                                  \
        if (memidx == 0) {                                                                            \
            pushByteCode(Walrus::name(offset, src0, src1, src2, dst), code);                          \
        } else {                                                                                      \
            pushByteCode(Walrus::name##MemIdx(memidx, alignmentLog2, offset, src0, src1, src2, dst)); \
        }                                                                                             \
        break;                                                                                        \
    }

            FOR_EACH_BYTECODE_ATOMIC_RMW_CMPXCHG_OP(GENERATE_RMW_CMPXCHG_CODE_CASE)
#undef GENERATE_RMW_CMPXCHG_CODE_CASE
        default:
            ASSERT_NOT_REACHED();
        }
    }

    virtual void OnAtomicWaitExpr(int opcode, Index memidx, Address alignmentLog2, Address offset) override
    {
        auto code = static_cast<WASMOpcode>(opcode);
        ASSERT(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_paramTypes[2]) == peekVMStackValueType());
        auto src2 = popVMStack();
        ASSERT(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_paramTypes[1]) == peekVMStackValueType());
        auto src1 = popVMStack();
        ASSERT(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_paramTypes[0]) == peekVMStackValueType());
        auto src0 = popVMStack();
        auto dst = computeExprResultPosition(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_resultType));
        switch (code) {
        case WASMOpcode::MemoryAtomicWait32Opcode: {
            if (memidx == 0) {
                pushByteCode(Walrus::MemoryAtomicWait32(offset, src0, src1, src2, dst), code);
            } else {
                pushByteCode(Walrus::MemoryAtomicWait32MemIdx(memidx, alignmentLog2, offset, src0, src1, src2, dst));
            }
            break;
        }
        case WASMOpcode::MemoryAtomicWait64Opcode: {
            if (memidx == 0) {
                pushByteCode(Walrus::MemoryAtomicWait64(offset, src0, src1, src2, dst), code);
            } else {
                pushByteCode(Walrus::MemoryAtomicWait64MemIdx(memidx, alignmentLog2, offset, src0, src1, src2, dst));
            }
            break;
        }
        default:
            ASSERT_NOT_REACHED();
        }
    }

    virtual void OnAtomicFenceExpr(uint32_t consistency_model) override
    {
        pushByteCode(Walrus::AtomicFence(), WASMOpcode::AtomicFenceOpcode);
    }

    virtual void OnAtomicNotifyExpr(int opcode, Index memidx, Address alignmentLog2, Address offset) override
    {
        auto code = static_cast<WASMOpcode>(opcode);
        ASSERT(code == WASMOpcode::MemoryAtomicNotifyOpcode);
        ASSERT(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_paramTypes[1]) == peekVMStackValueType());
        auto src1 = popVMStack();
        ASSERT(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_paramTypes[0]) == peekVMStackValueType());
        auto src0 = popVMStack();
        auto dst = computeExprResultPosition(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_resultType));
        if (memidx == 0) {
            pushByteCode(Walrus::MemoryAtomicNotify(offset, src0, src1, dst), code);
        } else {
            pushByteCode(Walrus::MemoryAtomicNotifyMemIdx(memidx, alignmentLog2, offset, src0, src1, dst));
        }
    }

    virtual void OnRefFuncExpr(Index func_index) override
    {
        auto dst = computeExprResultPosition(Walrus::Value::Type::NullFuncRef);
        pushByteCode(Walrus::RefFunc(func_index, dst), WASMOpcode::RefFuncOpcode);
    }

    virtual void OnRefNullExpr(Type type) override
    {
        Walrus::ByteCodeStackOffset dst = computeExprResultPosition(toValueKind(type, &m_result));
#if defined(WALRUS_32)
        pushByteCode(Walrus::Const32(dst, Walrus::Value::Null));
#else
        pushByteCode(Walrus::Const64(dst, Walrus::Value::Null));
#endif
    }

    virtual void OnRefIsNullExpr() override
    {
        auto src = popVMStack();
        auto dst = computeExprResultPosition(Walrus::Value::Type::I32);
#if defined(WALRUS_32)
        pushByteCode(Walrus::I32Eqz(src, dst), WASMOpcode::RefIsNullOpcode);
#else
        pushByteCode(Walrus::I64Eqz(src, dst), WASMOpcode::RefIsNullOpcode);
#endif
    }

    virtual void OnRefAsNonNullExpr() override
    {
        VMStackInfo& info = peekVMStackInfo();
        info.toNonNullableRef();
        pushByteCode(Walrus::RefAsNonNull(info.position()), WASMOpcode::RefAsNonNullOpcode);
    }

    uint8_t computeSrcInfo(Walrus::Type& targetType)
    {
        VMStackInfo& info = peekVMStackInfo();
        uint8_t srcInfo = 0;

        if (Walrus::Value::isNullableRefType(info.valueType())) {
            srcInfo |= Walrus::JumpIfCastGeneric::IsSrcNullable;

            if (Walrus::Value::isNullableRefType(targetType)) {
                srcInfo |= Walrus::JumpIfCastGeneric::IsNullable;
            }
        }

        if (Walrus::Value::isTaggedRefType(info.valueType())) {
            srcInfo |= Walrus::JumpIfCastGeneric::IsSrcTagged;
        }

        targetType = Walrus::Type(Walrus::Value::toNonNullableRefType(targetType), targetType.ref());

        switch (targetType) {
        case Walrus::Value::I31Ref:
        case Walrus::Value::StructRef:
        case Walrus::Value::ArrayRef:
        case Walrus::Value::EqRef:
            break;
        case Walrus::Value::NoAnyRef:
        case Walrus::Value::NoExternRef:
        case Walrus::Value::NoFuncRef:
            targetType = Walrus::Value::NoAnyRef;
            break;
        default:
            // Casting to other generic types are always
            // successful if the validator accepts them.
            targetType = Walrus::Value::AnyRef;
            break;
        }

        return srcInfo;
    }

    virtual void OnRefCastExpr(Type type) override
    {
        Walrus::Type targetType = toRefValueKind(type, &m_result);
        uint8_t srcInfo = computeSrcInfo(targetType);

        VMStackInfo& info = peekVMStackInfo();
        info.refCast(targetType);

        if (type.IsReferenceWithIndex()) {
            const Walrus::CompositeType** typeInfo = m_result.m_compositeTypes[type.GetReferenceIndex()]->subTypeList();
            pushByteCode(Walrus::RefCastDefined(info.position(), typeInfo, srcInfo), WASMOpcode::RefCastOpcode);
        } else {
            pushByteCode(Walrus::RefCastGeneric(info.position(), targetType, srcInfo), WASMOpcode::RefCastOpcode);
        }
    }

    virtual void OnRefTestExpr(Type type) override
    {
        Walrus::Type targetType = toRefValueKind(type, &m_result);
        uint8_t srcInfo = computeSrcInfo(targetType);

        auto src = popVMStack();
        auto dst = computeExprResultPosition(Walrus::Value::Type::I32);

        if (type.IsReferenceWithIndex()) {
            const Walrus::CompositeType** typeInfo = m_result.m_compositeTypes[type.GetReferenceIndex()]->subTypeList();
            pushByteCode(Walrus::RefTestDefined(src, dst, typeInfo, srcInfo), WASMOpcode::RefTestOpcode);
        } else {
            pushByteCode(Walrus::RefTestGeneric(src, dst, targetType, srcInfo), WASMOpcode::RefTestOpcode);
        }
    }

    // Dummy classes which inverts the fail bit for GenerateConditionalBranch template.
    class JumpIfCastFailGeneric : public Walrus::JumpIfCastGeneric {
    public:
        JumpIfCastFailGeneric(Walrus::ByteCodeStackOffset srcOffset, int32_t offset = 0)
            : JumpIfCastGeneric(srcOffset, offset)
        {
            init(Walrus::Value::Void, Walrus::JumpIfCastGeneric::IsCastFail);
        }
    };

    class JumpIfCastFailDefined : public Walrus::JumpIfCastDefined {
    public:
        JumpIfCastFailDefined(Walrus::ByteCodeStackOffset srcOffset, int32_t offset = 0)
            : JumpIfCastDefined(srcOffset, offset)
        {
            init(nullptr, Walrus::JumpIfCastGeneric::IsCastFail);
        }
    };

    virtual void OnBrOnCastExpr(Opcode opcode, Index depth, Type type) override
    {
        Walrus::Type targetType = toRefValueKind(type, &m_result);

        m_preprocessData.seenBranch();
        ASSERT(Walrus::Value::isRefType(peekVMStackValueType()));

        VMStackInfo& info = peekVMStackInfo();
        uint8_t srcInfo = computeSrcInfo(targetType);

        if (opcode == Opcode::BrOnCastFail) {
            // Inverted.
            srcInfo |= Walrus::JumpIfCastGeneric::IsCastFail;
        }

        if (type.IsReferenceWithIndex()) {
            size_t pos = GenerateConditionalBranch<Walrus::JumpIfCastDefined, JumpIfCastFailDefined, WASMOpcode::BrOnCastOpcode>(depth, info.position());
            const Walrus::CompositeType** typeInfo = m_result.m_compositeTypes[type.GetReferenceIndex()]->subTypeList();
            m_currentFunction->peekByteCode<Walrus::JumpIfCastDefined>(pos)->init(typeInfo, srcInfo);
        } else {
            size_t pos = GenerateConditionalBranch<Walrus::JumpIfCastGeneric, JumpIfCastFailGeneric, WASMOpcode::BrOnCastOpcode>(depth, info.position());
            m_currentFunction->peekByteCode<Walrus::JumpIfCastGeneric>(pos)->init(targetType, srcInfo);
        }

        if (opcode == Opcode::BrOnCastFail) {
            info.refCast(targetType);
        }
    }

    virtual void OnGCUnaryExpr(int opcode) override
    {
        switch (opcode) {
        case Opcode::RefEq: {
            auto src1 = popVMStack();
            auto src0 = popVMStack();
            auto dst = computeExprResultPosition(Walrus::Value::Type::I32);
            if (sizeof(void*) == 4) {
                pushByteCode(Walrus::I32Eq(src0, src1, dst), WASMOpcode::RefEqOpcode);
            } else {
                pushByteCode(Walrus::I64Eq(src0, src1, dst), WASMOpcode::RefEqOpcode);
            }
            break;
        }
        case Opcode::ArrayLen: {
            bool isNullable = Walrus::Value::isNullableRefType(peekVMStackInfo().valueType());
            auto src = popVMStack();
            auto dst = computeExprResultPosition(Walrus::Value::Type::I32);
            pushByteCode(Walrus::ArrayLen(src, dst, isNullable), WASMOpcode::ArrayLenOpcode);
            break;
        }
        case Opcode::AnyConvertExtern:
        case Opcode::ExternConvertAny: {
            Walrus::Value::Type type = (opcode == Opcode::AnyConvertExtern) ? Walrus::Value::Type::AnyRef : Walrus::Value::Type::ExternRef;
            auto src = popVMStack();
            auto dst = computeExprResultPosition(type);
            generateMoveCodeIfNeeds(src, dst, type);
            break;
        }
        case Opcode::RefI31: {
            auto src = popVMStack();
            auto dst = computeExprResultPosition(Walrus::Value::Type::I31Ref);
            pushByteCode(Walrus::RefI31(src, dst), WASMOpcode::RefI31Opcode);
            break;
        }
        case Opcode::I31GetS: {
            bool isNullable = Walrus::Value::isNullableRefType(peekVMStackInfo().valueType());
            auto src = popVMStack();
            auto dst = computeExprResultPosition(Walrus::Value::Type::I32);
            pushByteCode(Walrus::I31GetS(src, dst, isNullable), WASMOpcode::I31GetSOpcode);
            break;
        }
        case Opcode::I31GetU: {
            bool isNullable = Walrus::Value::isNullableRefType(peekVMStackInfo().valueType());
            auto src = popVMStack();
            auto dst = computeExprResultPosition(Walrus::Value::Type::I32);
            pushByteCode(Walrus::I31GetU(src, dst, isNullable), WASMOpcode::I31GetSOpcode);
            break;
        }
        }
    }

    virtual void OnArrayNewExpr(Index type_index) override
    {
        const Walrus::ArrayType* typeInfo = m_result.m_compositeTypes[type_index]->asArray();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src1 = popVMStack();
        auto src0 = popVMStack();
        auto dst = computeExprResultPosition(Walrus::Value::Type::DefinedRef);
        pushByteCode(Walrus::ArrayNew(typeInfo, src0, src1, dst), WASMOpcode::ArrayNewOpcode);
    }

    virtual void OnArrayNewDefaultExpr(Index type_index) override
    {
        const Walrus::ArrayType* typeInfo = m_result.m_compositeTypes[type_index]->asArray();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src = popVMStack();
        auto dst = computeExprResultPosition(Walrus::Value::Type::DefinedRef);
        pushByteCode(Walrus::ArrayNewDefault(typeInfo, src, dst), WASMOpcode::ArrayNewDefaultOpcode);
    }

    virtual void OnArrayNewFixedExpr(Index type_index, Index count) override
    {
        const Walrus::ArrayType* typeInfo = m_result.m_compositeTypes[type_index]->asArray();
        auto pos = m_currentFunction->currentByteCodeSize();

        pushByteCode(Walrus::ArrayNewFixed(typeInfo, count), WASMOpcode::ArrayNewFixedOpcode);

        m_currentFunction->expandByteCode(Walrus::ByteCode::pointerAlignedSize(sizeof(Walrus::ByteCodeStackOffset) * count));
        ASSERT(m_currentFunction->currentByteCodeSize() % sizeof(void*) == 0);
        Walrus::ArrayNewFixed* code = m_currentFunction->peekByteCode<Walrus::ArrayNewFixed>(pos);
        for (size_t i = 0; i < count; i++) {
            ASSERT(peekVMStackValueType() == toDebugType(typeInfo->field().stackType()));
            code->dataOffsets()[count - i - 1] = popVMStack();
        }

        code->setDstOffset(computeExprResultPosition(Walrus::Value::Type::DefinedRef));
    }

    virtual void OnArrayNewDataExpr(Index type_index, Index data_index) override
    {
        const Walrus::ArrayType* typeInfo = m_result.m_compositeTypes[type_index]->asArray();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src1 = popVMStack();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src0 = popVMStack();
        auto dst = computeExprResultPosition(Walrus::Value::Type::DefinedRef);
        pushByteCode(Walrus::ArrayNewData(typeInfo, data_index, src0, src1, dst), WASMOpcode::ArrayNewDataOpcode);
    }

    virtual void OnArrayNewElemExpr(Index type_index, Index elem_index) override
    {
        const Walrus::ArrayType* typeInfo = m_result.m_compositeTypes[type_index]->asArray();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src1 = popVMStack();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src0 = popVMStack();
        auto dst = computeExprResultPosition(Walrus::Value::Type::DefinedRef);
        pushByteCode(Walrus::ArrayNewElem(typeInfo, elem_index, src0, src1, dst), WASMOpcode::ArrayNewElemOpcode);
    }

    virtual void OnArrayInitDataExpr(Index type_index, Index data_index) override
    {
        Walrus::Value::Type type = m_result.m_compositeTypes[type_index]->asArray()->field().type();
        uint8_t log2Size = static_cast<uint8_t>(Walrus::GCArray::getLog2Size(type));
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src3 = popVMStack();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src2 = popVMStack();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src1 = popVMStack();
        ASSERT(Walrus::Value::isRefType(peekVMStackValueType()));
        bool isNullable = Walrus::Value::isNullableRefType(peekVMStackValueType());
        auto src0 = popVMStack();
        pushByteCode(Walrus::ArrayInitData(log2Size, data_index, isNullable, src0, src1, src2, src3), WASMOpcode::ArrayInitDataOpcode);
    }

    virtual void OnArrayInitElemExpr(Index type_index, Index elem_index) override
    {
        Walrus::Value::Type type = m_result.m_compositeTypes[type_index]->asArray()->field().type();
        uint8_t log2Size = static_cast<uint8_t>(Walrus::GCArray::getLog2Size(type));
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src3 = popVMStack();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src2 = popVMStack();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src1 = popVMStack();
        ASSERT(Walrus::Value::isRefType(peekVMStackValueType()));
        bool isNullable = Walrus::Value::isNullableRefType(peekVMStackValueType());
        auto src0 = popVMStack();
        pushByteCode(Walrus::ArrayInitElem(log2Size, elem_index, isNullable, src0, src1, src2, src3), WASMOpcode::ArrayInitElemOpcode);
    }

    virtual void OnArrayFillExpr(Index type_index) override
    {
        const Walrus::ArrayType* typeInfo = m_result.m_compositeTypes[type_index]->asArray();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src3 = popVMStack();
        auto src2 = popVMStack();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src1 = popVMStack();
        ASSERT(Walrus::Value::isRefType(peekVMStackValueType()));
        bool isNullable = Walrus::Value::isNullableRefType(peekVMStackValueType());
        auto src0 = popVMStack();
        pushByteCode(Walrus::ArrayFill(typeInfo->field().type(), isNullable, src0, src1, src2, src3), WASMOpcode::ArrayFillOpcode);
    }

    virtual void OnArrayCopyExpr(Index dst_type_index, Index src_type_index) override
    {
        const Walrus::ArrayType* dstTypeInfo = m_result.m_compositeTypes[dst_type_index]->asArray();
        const Walrus::ArrayType* srcTypeInfo = m_result.m_compositeTypes[src_type_index]->asArray();
        ASSERT(dstTypeInfo->field().type() == srcTypeInfo->field().type());
        const uint8_t log2Size = Walrus::GCArray::getLog2Size(dstTypeInfo->field().type());
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src4 = popVMStack();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src3 = popVMStack();
        ASSERT(Walrus::Value::isRefType(peekVMStackValueType()));
        bool srcIsNullable = Walrus::Value::isNullableRefType(peekVMStackValueType());
        auto src2 = popVMStack();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src1 = popVMStack();
        ASSERT(Walrus::Value::isRefType(peekVMStackValueType()));
        bool dstIsNullable = Walrus::Value::isNullableRefType(peekVMStackValueType());
        auto src0 = popVMStack();
        pushByteCode(Walrus::ArrayCopy(log2Size, dstIsNullable, srcIsNullable, src0, src1, src2, src3, src4), WASMOpcode::ArrayCopyOpcode);
    }

    virtual void OnArrayGetExpr(Opcode opcode, Index type_index) override
    {
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src1 = popVMStack();
        uint8_t info = 0;

        if (opcode == Opcode::ArrayGetS) {
            info |= Walrus::ArrayGet::IsSigned;
        }

        if (Walrus::Value::isNullableRefType(peekVMStackInfo().valueType())) {
            info |= Walrus::ArrayGet::IsNullable;
        }

        const Walrus::ArrayType* typeInfo = m_result.m_compositeTypes[type_index]->asArray();
        Walrus::Value::Type type = typeInfo->field().type();

        auto src0 = popVMStack();
        auto dst = computeExprResultPosition(Walrus::Value::unpackType(type));
        pushByteCode(Walrus::ArrayGet(src0, src1, dst, type, info), WASMOpcode::ArrayGetSOpcode);
    }

    virtual void OnArraySetExpr(Index type_index) override
    {
        uint8_t info = 0;
        auto src2 = popVMStack();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src1 = popVMStack();

        if (Walrus::Value::isNullableRefType(peekVMStackInfo().valueType())) {
            info |= Walrus::ArrayGet::IsNullable;
        }

        const Walrus::ArrayType* typeInfo = m_result.m_compositeTypes[type_index]->asArray();
        Walrus::Value::Type type = typeInfo->field().type();

        auto src0 = popVMStack();
        pushByteCode(Walrus::ArraySet(src0, src1, src2, type, info), WASMOpcode::ArraySetOpcode);
    }

    virtual void OnStructNewExpr(Index type_index) override
    {
        const Walrus::StructType* typeInfo = m_result.m_compositeTypes[type_index]->asStruct();
        auto pos = m_currentFunction->currentByteCodeSize();

        pushByteCode(Walrus::StructNew(typeInfo), WASMOpcode::StructNewOpcode);

        const Walrus::MutableTypeVector& fields = typeInfo->fields();
        m_currentFunction->expandByteCode(Walrus::ByteCode::pointerAlignedSize(sizeof(Walrus::ByteCodeStackOffset) * fields.size()));
        ASSERT(m_currentFunction->currentByteCodeSize() % sizeof(void*) == 0);
        Walrus::StructNew* code = m_currentFunction->peekByteCode<Walrus::StructNew>(pos);
        for (size_t i = 0; i < fields.size(); i++) {
            ASSERT(peekVMStackValueType() == toDebugType(fields.types()[fields.size() - i - 1].stackType()));
            code->dataOffsets()[fields.size() - i - 1] = popVMStack();
        }

        code->setDstOffset(computeExprResultPosition(Walrus::Value::Type::DefinedRef));
    }

    virtual void OnStructNewDefaultExpr(Index type_index) override
    {
        const Walrus::StructType* typeInfo = m_result.m_compositeTypes[type_index]->asStruct();
        auto dst = computeExprResultPosition(Walrus::Value::Type::DefinedRef);
        pushByteCode(Walrus::StructNewDefault(typeInfo, dst), WASMOpcode::StructNewDefaultOpcode);
    }

    virtual void OnStructGetExpr(Opcode opcode, Index type_index, Index field_index) override
    {
        uint8_t info = 0;

        if (opcode == Opcode::StructGetS) {
            info |= Walrus::StructGet::IsSigned;
        }

        if (Walrus::Value::isNullableRefType(peekVMStackInfo().valueType())) {
            info |= Walrus::StructGet::IsNullable;
        }

        const Walrus::StructType* typeInfo = m_result.m_compositeTypes[type_index]->asStruct();
        Walrus::Value::Type type = typeInfo->fields().types()[field_index].type();
        uint32_t memberOffset = typeInfo->fieldOffsets()[field_index];

        auto src = popVMStack();
        auto dst = computeExprResultPosition(Walrus::Value::unpackType(type));
        pushByteCode(Walrus::StructGet(src, dst, memberOffset, type, info), WASMOpcode::StructGetSOpcode);
    }

    virtual void OnStructSetExpr(Index type_index, Index field_index) override
    {
        uint8_t info = 0;
        auto src1 = popVMStack();

        if (Walrus::Value::isNullableRefType(peekVMStackInfo().valueType())) {
            info |= Walrus::StructGet::IsNullable;
        }

        const Walrus::StructType* typeInfo = m_result.m_compositeTypes[type_index]->asStruct();
        Walrus::Value::Type type = typeInfo->fields().types()[field_index].type();
        uint32_t memberOffset = typeInfo->fieldOffsets()[field_index];

        auto src0 = popVMStack();
        pushByteCode(Walrus::StructSet(src0, src1, memberOffset, type, info), WASMOpcode::StructSetOpcode);
    }

    virtual void OnNopExpr() override
    {
#if !defined(NDEBUG)
        pushByteCode(Walrus::Nop(), WASMOpcode::NopOpcode);
#endif /* !NDEBUG */
    }

    virtual void OnReturnExpr() override
    {
        m_preprocessData.seenBranch();
        generateFunctionReturnCode();
    }

    virtual void OnEndExpr() override
    {
        // combining an i32.eqz at the end of a block followed by a JumpIf cannot be combined
        // because it is possible to jump to the location after i32.eqz
        m_lastI32EqzPos = s_noI32Eqz;
        if (m_blockInfo.size()) {
            auto dropSize = dropStackValuesBeforeBrIfNeeds(0);
            auto blockInfo = m_blockInfo.back();
            m_blockInfo.pop_back();

#if !defined(NDEBUG)
            if (!blockInfo.shouldRestoreVMStackAtEnd()) {
                if (!blockInfo.returnValueIsIndex() && blockInfo.m_returnValueType != Walrus::Value::Void) {
                    ASSERT(peekVMStackValueType() == blockInfo.m_returnValueType);
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
                        stackSizeToBe += m_vmStack[i].stackAllocatedSize();
                    }
                    m_currentFunction->m_catchInfo.push_back({ iter->m_tryStart, iter->m_tryEnd, iter->m_catchStart, stackSizeToBe, iter->m_tagIndex });
                    iter = m_catchInfo.erase(iter);
                }
            }

            if (blockInfo.byteCodeGenerationStopped() && blockInfo.m_jumpToEndBrInfo.size() == 0) {
                stopToGenerateByteCodeWhileBlockEnd();
                return;
            }

            keepBlockResultsIfNeeds(blockInfo, dropSize);

            if (blockInfo.shouldRestoreVMStackAtEnd()) {
                restoreVMStackBy(blockInfo);
                if (blockInfo.returnValueIsIndex()) {
                    auto ft = getFunctionType(blockInfo.m_returnValueType);
                    const auto& param = ft->param().types();
                    for (size_t i = 0; i < param.size(); i++) {
                        ASSERT(toDebugType(peekVMStackValueType()) == toDebugType(param[param.size() - i - 1]));
                        popVMStack();
                    }

                    const auto& result = ft->result().types();
                    for (size_t i = 0; i < result.size(); i++) {
                        pushVMStack(result[i]);
                    }
                } else if (blockInfo.m_returnValueType != Walrus::Value::Void) {
                    pushVMStack(blockInfo.m_returnValueType);
                }
            }

            for (size_t i = 0; i < blockInfo.m_jumpToEndBrInfo.size(); i++) {
                switch (blockInfo.m_jumpToEndBrInfo[i].m_type) {
                case BlockInfo::JumpToEndBrInfo::IsJump:
                    m_currentFunction->peekByteCode<Walrus::Jump>(blockInfo.m_jumpToEndBrInfo[i].m_position)->setOffset(m_currentFunction->currentByteCodeSize() - blockInfo.m_jumpToEndBrInfo[i].m_position);
                    break;
                case BlockInfo::JumpToEndBrInfo::IsJumpIf:
                    m_currentFunction->peekByteCode<Walrus::JumpIfFalse>(blockInfo.m_jumpToEndBrInfo[i].m_position)
                        ->setOffset(m_currentFunction->currentByteCodeSize() - blockInfo.m_jumpToEndBrInfo[i].m_position);
                    break;
                default:
                    ASSERT(blockInfo.m_jumpToEndBrInfo[i].m_type == BlockInfo::JumpToEndBrInfo::IsBrTable);

                    int32_t* offset = m_currentFunction->peekByteCode<int32_t>(blockInfo.m_jumpToEndBrInfo[i].m_position);
                    *offset = m_currentFunction->currentByteCodeSize() + (size_t)*offset - blockInfo.m_jumpToEndBrInfo[i].m_position;
                    break;
                }
            }
        } else {
            generateEndCode(true);
        }
    }

    virtual void OnUnreachableExpr() override
    {
        m_preprocessData.seenBranch();
        pushByteCode(Walrus::Unreachable(), WASMOpcode::UnreachableOpcode);
        stopToGenerateByteCodeWhileBlockEnd();
    }

    virtual void EndFunctionBody(Index index) override
    {
        // FIXME too many stack usage. we could not support this(yet)
        if (m_initialFunctionStackSize > std::numeric_limits<Walrus::ByteCodeStackOffset>::max()) {
            m_walrusParseError = std::string("Function stack usage is larger then supported maxium (65535 bytes).");
        }

        m_lastI32EqzPos = s_noI32Eqz;

        ASSERT(m_currentFunction == m_result.m_functions[index]);
        std::vector<std::pair<size_t, Walrus::Value>> locals;
        for (uint32_t i = 0; i < m_localInfo.size(); i++) {
            locals.push_back(std::make_pair(m_localInfo[i].m_position, Walrus::Value(m_localInfo[i].m_valueType)));
        }

        for (auto constant : m_preprocessData.m_constantData) {
            locals.push_back(std::make_pair(constant.second, constant.first));
        }

        m_currentFunction->m_localDebugData.clear();
        m_currentFunction->m_constantDebugData.clear();

        LiveAnalysis analysis;
        analysis.optimizeLocals(m_currentFunction, locals, m_localInfo.size());

#if !defined(NDEBUG)
        if (getenv("DUMP_BYTECODE") && strlen(getenv("DUMP_BYTECODE"))) {
            m_currentFunction->dumpByteCode();
        }
        if (m_shouldContinueToGenerateByteCode) {
            for (size_t i = 0; i < m_currentFunctionType->result().size() && m_vmStack.size(); i++) {
                ASSERT(toDebugType(popVMStackInfo().valueType()) == toDebugType(m_currentFunctionType->result().types()[m_currentFunctionType->result().size() - i - 1]));
            }
            ASSERT(m_vmStack.empty());
        }
#endif

        counter++;
        endFunction();
    }

    // SIMD Instructions
    virtual void OnLoadSplatExpr(int opcode, Index memidx, Address alignmentLog2, Address offset) override
    {
        auto code = static_cast<WASMOpcode>(opcode);
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src = popVMStack();
        auto dst = computeExprResultPosition(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_resultType));
        switch (code) {
#define GENERATE_LOAD_CODE_CASE(name, ...)                                               \
    case WASMOpcode::name##Opcode: {                                                     \
        if (memidx == 0) {                                                               \
            pushByteCode(Walrus::name(offset, src, dst), code);                          \
        } else {                                                                         \
            pushByteCode(Walrus::name##MemIdx(memidx, alignmentLog2, offset, src, dst)); \
        }                                                                                \
        break;                                                                           \
    }

            FOR_EACH_BYTECODE_SIMD_LOAD_SPLAT_OP(GENERATE_LOAD_CODE_CASE)
#undef GENERATE_LOAD_CODE_CASE
        default:
            ASSERT_NOT_REACHED();
        }
    }

    virtual void OnLoadZeroExpr(int opcode, Index memidx, Address alignmentLog2, Address offset) override
    {
        auto code = static_cast<WASMOpcode>(opcode);
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src = popVMStack();
        auto dst = computeExprResultPosition(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_resultType));
        switch (code) {
        case WASMOpcode::V128Load32ZeroOpcode: {
            if (memidx == 0) {
                pushByteCode(Walrus::V128Load32Zero(offset, src, dst), code);
            } else {
                pushByteCode(Walrus::V128Load32ZeroMemIdx(memidx, alignmentLog2, offset, src, dst));
            }
            break;
        }
        case WASMOpcode::V128Load64ZeroOpcode: {
            if (memidx == 0) {
                pushByteCode(Walrus::V128Load64Zero(offset, src, dst), code);
            } else {
                pushByteCode(Walrus::V128Load64ZeroMemIdx(memidx, alignmentLog2, offset, src, dst));
            }
            break;
        }
        default:
            ASSERT_NOT_REACHED();
            break;
        }
    }

    virtual void OnSimdLaneOpExpr(int opcode, uint64_t value) override
    {
        auto code = static_cast<WASMOpcode>(opcode);
        switch (code) {
#define GENERATE_SIMD_EXTRACT_LANE_CODE_CASE(name, ...)                                                               \
    case WASMOpcode::name##Opcode: {                                                                                  \
        ASSERT(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_paramTypes[0]) == peekVMStackValueType());  \
        auto src = popVMStack();                                                                                      \
        auto dst = computeExprResultPosition(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_resultType)); \
        pushByteCode(Walrus::name(static_cast<uint8_t>(value), src, dst), code);                                      \
        break;                                                                                                        \
    }

#define GENERATE_SIMD_REPLACE_LANE_CODE_CASE(name, ...)                                                               \
    case WASMOpcode::name##Opcode: {                                                                                  \
        ASSERT(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_paramTypes[1]) == peekVMStackValueType());  \
        auto src1 = popVMStack();                                                                                     \
        ASSERT(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_paramTypes[0]) == peekVMStackValueType());  \
        auto src0 = popVMStack();                                                                                     \
        auto dst = computeExprResultPosition(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_resultType)); \
        pushByteCode(Walrus::name(static_cast<uint8_t>(value), src0, src1, dst), code);                               \
        break;                                                                                                        \
    }

            FOR_EACH_BYTECODE_SIMD_EXTRACT_LANE_OP(GENERATE_SIMD_EXTRACT_LANE_CODE_CASE)
            FOR_EACH_BYTECODE_SIMD_REPLACE_LANE_OP(GENERATE_SIMD_REPLACE_LANE_CODE_CASE)
#undef GENERATE_SIMD_EXTRACT_LANE_CODE_CASE
#undef GENERATE_SIMD_REPLACE_LANE_CODE_CASE
        default:
            ASSERT_NOT_REACHED();
            break;
        }
    }

    virtual void OnSimdLoadLaneExpr(int opcode, Index memidx, Address alignmentLog2, Address offset, uint64_t value) override
    {
        auto code = static_cast<WASMOpcode>(opcode);
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::V128);
        auto src1 = popVMStack();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src0 = popVMStack();
        auto dst = computeExprResultPosition(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_resultType));
        switch (code) {
#define GENERATE_LOAD_CODE_CASE(name, opType)                                                                                                    \
    case WASMOpcode::name##Opcode: {                                                                                                             \
        if (memidx == 0) {                                                                                                                       \
            pushByteCode(Walrus::name(offset, src0, src1, static_cast<Walrus::ByteCodeStackOffset>(value), dst), code);                          \
        } else {                                                                                                                                 \
            pushByteCode(Walrus::name##MemIdx(memidx, alignmentLog2, offset, src0, src1, static_cast<Walrus::ByteCodeStackOffset>(value), dst)); \
        }                                                                                                                                        \
        break;                                                                                                                                   \
    }
            FOR_EACH_BYTECODE_SIMD_LOAD_LANE_OP(GENERATE_LOAD_CODE_CASE)
#undef GENERATE_LOAD_CODE_CASE
        default:
            ASSERT_NOT_REACHED();
        }
    }

    virtual void OnSimdStoreLaneExpr(int opcode, Index memidx, Address alignmentLog2, Address offset, uint64_t value) override
    {
        auto code = static_cast<WASMOpcode>(opcode);
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::V128);
        auto src1 = popVMStack();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::I32);
        auto src0 = popVMStack();
        switch (code) {
#define GENERATE_STORE_CODE_CASE(name, opType)                                                                                              \
    case WASMOpcode::name##Opcode: {                                                                                                        \
        if (memidx == 0) {                                                                                                                  \
            pushByteCode(Walrus::name(offset, src0, src1, static_cast<Walrus::ByteCodeStackOffset>(value)), code);                          \
        } else {                                                                                                                            \
            pushByteCode(Walrus::name##MemIdx(memidx, alignmentLog2, offset, src0, src1, static_cast<Walrus::ByteCodeStackOffset>(value))); \
        }                                                                                                                                   \
        break;                                                                                                                              \
    }
            FOR_EACH_BYTECODE_SIMD_STORE_LANE_OP(GENERATE_STORE_CODE_CASE)
#undef GENERATE_STORE_CODE_CASE
        default:
            ASSERT_NOT_REACHED();
        }
    }

    virtual void OnSimdShuffleOpExpr(int opcode, uint8_t* value) override
    {
        ASSERT(static_cast<WASMOpcode>(opcode) == WASMOpcode::I8X16ShuffleOpcode);
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::V128);
        auto src1 = popVMStack();
        ASSERT(peekVMStackValueType() == Walrus::Value::Type::V128);
        auto src0 = popVMStack();
        auto dst = computeExprResultPosition(WASMCodeInfo::codeTypeToValueType(g_wasmCodeInfo[opcode].m_resultType));
        pushByteCode(Walrus::I8X16Shuffle(src0, src1, dst, value), WASMOpcode::I8X16ShuffleOpcode);
    }

    void generateBinaryCode(WASMOpcode code, size_t src0, size_t src1, size_t dst)
    {
        switch (code) {
#define GENERATE_BINARY_CODE_CASE(name, ...)               \
    case WASMOpcode::name##Opcode: {                       \
        pushByteCode(Walrus::name(src0, src1, dst), code); \
        break;                                             \
    }
            FOR_EACH_BYTECODE_BINARY_OP(GENERATE_BINARY_CODE_CASE)
            FOR_EACH_BYTECODE_SIMD_BINARY_OP(GENERATE_BINARY_CODE_CASE)
            FOR_EACH_BYTECODE_SIMD_BINARY_SHIFT_OP(GENERATE_BINARY_CODE_CASE)
            FOR_EACH_BYTECODE_SIMD_BINARY_OTHER(GENERATE_BINARY_CODE_CASE)
            FOR_EACH_BYTECODE_RELAXED_SIMD_BINARY_OP(GENERATE_BINARY_CODE_CASE)
            FOR_EACH_BYTECODE_RELAXED_SIMD_BINARY_OTHER(GENERATE_BINARY_CODE_CASE)
#undef GENERATE_BINARY_CODE_CASE
        default:
            ASSERT_NOT_REACHED();
            break;
        }
    }

    void generateUnaryCode(WASMOpcode code, size_t src, size_t dst)
    {
        switch (code) {
#define GENERATE_UNARY_CODE_CASE(name, ...)         \
    case WASMOpcode::name##Opcode: {                \
        pushByteCode(Walrus::name(src, dst), code); \
        break;                                      \
    }
            FOR_EACH_BYTECODE_UNARY_OP(GENERATE_UNARY_CODE_CASE)
            FOR_EACH_BYTECODE_UNARY_OP_2(GENERATE_UNARY_CODE_CASE)
            FOR_EACH_BYTECODE_SIMD_UNARY_OP(GENERATE_UNARY_CODE_CASE)
            FOR_EACH_BYTECODE_SIMD_UNARY_CONVERT_OP(GENERATE_UNARY_CODE_CASE)
            FOR_EACH_BYTECODE_SIMD_UNARY_OTHER(GENERATE_UNARY_CODE_CASE)
            FOR_EACH_BYTECODE_RELAXED_SIMD_UNARY_OTHER(GENERATE_UNARY_CODE_CASE)
#undef GENERATE_UNARY_CODE_CASE
        case WASMOpcode::I32ReinterpretF32Opcode:
            pushByteCode(Walrus::I32ReinterpretF32(src, dst), code);
            break;
        case WASMOpcode::I64ReinterpretF64Opcode:
            pushByteCode(Walrus::I64ReinterpretF64(src, dst), code);
            break;
        case WASMOpcode::F32ReinterpretI32Opcode:
            pushByteCode(Walrus::F32ReinterpretI32(src, dst), code);
            break;
        case WASMOpcode::F64ReinterpretI64Opcode:
            pushByteCode(Walrus::F64ReinterpretI64(src, dst), code);
            break;
        default:
            ASSERT_NOT_REACHED();
            break;
        }
    }

    void generateMemoryLoadCode(WASMOpcode code, Index memidx, Address alignmentLog2, size_t offset, size_t src, size_t dst)
    {
        switch (code) {
#define GENERATE_LOAD_CODE_CASE(name, readType, writeType)                               \
    case WASMOpcode::name##Opcode: {                                                     \
        if (memidx == 0) {                                                               \
            pushByteCode(Walrus::name(offset, src, dst), code);                          \
        } else {                                                                         \
            pushByteCode(Walrus::name##MemIdx(memidx, alignmentLog2, offset, src, dst)); \
        }                                                                                \
        break;                                                                           \
    }
            FOR_EACH_BYTECODE_LOAD_OP(GENERATE_LOAD_CODE_CASE)
            FOR_EACH_BYTECODE_SIMD_LOAD_EXTEND_OP(GENERATE_LOAD_CODE_CASE)
#undef GENERATE_LOAD_CODE_CASE
        default:
            ASSERT_NOT_REACHED();
            break;
        }
    }

    void generateMemoryStoreCode(WASMOpcode code, Index memidx, Address alignmentLog2, size_t offset, size_t src0, size_t src1)
    {
        switch (code) {
#define GENERATE_STORE_CODE_CASE(name, readType, writeType)                                \
    case WASMOpcode::name##Opcode: {                                                       \
        if (memidx == 0) {                                                                 \
            pushByteCode(Walrus::name(offset, src0, src1), code);                          \
        } else {                                                                           \
            pushByteCode(Walrus::name##MemIdx(memidx, alignmentLog2, offset, src0, src1)); \
        }                                                                                  \
        break;                                                                             \
    }
            FOR_EACH_BYTECODE_STORE_OP(GENERATE_STORE_CODE_CASE)
#undef GENERATE_STORE_CODE_CASE
        default:
            ASSERT_NOT_REACHED();
            break;
        }
    }

    bool isBinaryOperation(WASMOpcode opcode)
    {
        switch (opcode) {
#define GENERATE_BINARY_CODE_CASE(name, op, paramType, returnType) \
    case WASMOpcode::name##Opcode:
            FOR_EACH_BYTECODE_BINARY_OP(GENERATE_BINARY_CODE_CASE)
#undef GENERATE_BINARY_CODE_CASE
            return true;
        default:
            return false;
        }
    }

    Walrus::WASMParsingResult& parsingResult() { return m_result; }
};

} // namespace wabt

namespace Walrus {

WASMParsingResult::WASMParsingResult()
    : m_seenStartAttribute(false)
    , m_typesAddedToStore(false)
    , m_version(0)
    , m_start(0)
{
}

void WASMParsingResult::clear()
{
    for (size_t i = 0; i < m_imports.size(); i++) {
        delete m_imports[i];
    }

    for (size_t i = 0; i < m_exports.size(); i++) {
        delete m_exports[i];
    }

    for (size_t i = 0; i < m_functions.size(); i++) {
        delete m_functions[i];
    }

    for (size_t i = 0; i < m_datas.size(); i++) {
        delete m_datas[i];
    }

    for (size_t i = 0; i < m_elements.size(); i++) {
        delete m_elements[i];
    }

    if (!m_typesAddedToStore) {
        for (size_t i = 0; i < m_compositeTypes.size(); i++) {
            delete m_compositeTypes[i];
        }
    }

    for (size_t i = 0; i < m_globalTypes.size(); i++) {
        delete m_globalTypes[i];
    }

    for (size_t i = 0; i < m_tableTypes.size(); i++) {
        delete m_tableTypes[i];
    }

    for (size_t i = 0; i < m_memoryTypes.size(); i++) {
        delete m_memoryTypes[i];
    }

    for (size_t i = 0; i < m_tagTypes.size(); i++) {
        delete m_tagTypes[i];
    }
}

std::pair<Optional<Module*>, std::string> WASMParser::parseBinary(Store* store, const std::string& filename, const uint8_t* data, size_t len, const uint32_t JITFlags, const uint32_t featureFlags)
{
    wabt::WASMBinaryReader delegate(store->getTypeStore());

    std::string error = ReadWasmBinary(filename, data, len, &delegate, featureFlags);
    if (error.length()) {
        if (delegate.parsingResult().m_typesAddedToStore) {
            store->getTypeStore().releaseTypes(delegate.parsingResult().m_compositeTypes);
        }
        return std::make_pair(nullptr, error);
    }

    if (delegate.WalrusParseError().length()) {
        if (delegate.parsingResult().m_typesAddedToStore) {
            store->getTypeStore().releaseTypes(delegate.parsingResult().m_compositeTypes);
        }
        return std::make_pair(nullptr, delegate.WalrusParseError());
    }

    Module* module = new Module(store, delegate.parsingResult());
#if defined(WALRUS_ENABLE_JIT)
    if (JITFlags & JITFlagValue::useJIT) {
        module->jitCompile(nullptr, 0, JITFlags);
    }
#endif

    return std::make_pair(module, std::string());
}

} // namespace Walrus
