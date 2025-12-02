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

#ifndef __WalrusModule__
#define __WalrusModule__

#include "Walrus.h"
#include "runtime/ObjectType.h"
#include "runtime/Object.h"
#include "wabt/ir.h"
#include <cstdint>

namespace wabt {
class WASMBinaryReader;
}

namespace Walrus {

class Store;
class Module;
class Instance;
class JITFunction;
class JITModule;

struct WASMParsingResult;

enum JITFlagValue : uint32_t {
    useJIT = 1 << 0,
    JITVerbose = 1 << 1,
    JITVerboseColor = 1 << 2,
    disableRegAlloc = 1 << 3,
};

enum class SegmentMode {
    None,
    Active,
    Passive,
    Declared,
};

// https://webassembly.github.io/spec/core/syntax/modules.html#syntax-import
class ImportType {
public:
    enum Type { Function,
                Table,
                Memory,
                Global,
                Tag };

    ImportType(Type t,
               const std::string& moduleName,
               const std::string& fieldName,
               const ObjectType* type)
        : m_importType(t)
        , m_moduleName(moduleName)
        , m_fieldName(fieldName)
        , m_type(type)
    {
    }

    ImportType(const std::string& moduleName,
               const std::string& fieldName,
               const ObjectType* type)
        : m_importType(Function)
        , m_moduleName(moduleName)
        , m_fieldName(fieldName)
        , m_type(type)
    {
        switch (type->kind()) {
        case ObjectType::FunctionKind:
            m_importType = Function;
            break;
        case ObjectType::GlobalKind:
            m_importType = Global;
            break;
        case ObjectType::TableKind:
            m_importType = Table;
            break;
        case ObjectType::MemoryKind:
            m_importType = Memory;
            break;
        case ObjectType::TagKind:
            m_importType = Tag;
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
    }

    Type importType() const { return m_importType; }
    std::string& moduleName() { return m_moduleName; }
    std::string& fieldName() { return m_fieldName; }

    const ObjectType* type() const
    {
        return m_type;
    }

    const FunctionType* functionType() const
    {
        ASSERT(importType() == Type::Function);
        return static_cast<const FunctionType*>(m_type);
    }

    const GlobalType* globalType() const
    {
        ASSERT(importType() == Type::Global);
        return static_cast<const GlobalType*>(m_type);
    }

    const TableType* tableType() const
    {
        ASSERT(importType() == Type::Table);
        return static_cast<const TableType*>(m_type);
    }

    const MemoryType* memoryType() const
    {
        ASSERT(importType() == Type::Memory);
        return static_cast<const MemoryType*>(m_type);
    }

    std::string typeToString()
    {
        switch (m_importType) {
        case Type::Function: {
            return std::string("Function");
        }
        case Type::Global: {
            return std::string("Global");
        }
        case Type::Memory: {
            return std::string("Memory");
        }
        case Type::Table: {
            return std::string("Table");
        }
        case Type::Tag: {
            return std::string("Tag");
        }
        }

        RELEASE_ASSERT_NOT_REACHED();
    }

private:
    Type m_importType;
    std::string m_moduleName;
    std::string m_fieldName;
    const ObjectType* m_type;
};

class ExportType {
public:
    // matches binary format, do not change
    enum Type { Function,
                Table,
                Memory,
                Global,
                Tag };

    ExportType(Type type,
               std::string& name,
               uint32_t itemIndex)
        : m_exportType(type)
        , m_name(name)
        , m_itemIndex(itemIndex)
    {
    }

    Type exportType() const { return m_exportType; }
    std::string& name() { return m_name; }

    uint32_t itemIndex() const
    {
        return m_itemIndex;
    }

private:
    Type m_exportType;
    std::string m_name;
    uint32_t m_itemIndex;
};

class ModuleFunction {
    friend class wabt::WASMBinaryReader;

public:
    struct CatchInfo {
        size_t m_tryStart;
        size_t m_tryEnd;
        size_t m_catchStartPosition;
        size_t m_stackSizeToBe;
        uint32_t m_tagIndex;
    };

    ModuleFunction(FunctionType* functionType);
    ~ModuleFunction();

    bool hasTryCatch() const { return m_hasTryCatch; }
    uint16_t requiredStackSize() const { return m_requiredStackSize; }
    FunctionType* functionType() const { return m_functionType; }

    template <typename CodeType>
    void pushByteCode(const CodeType& code)
    {
        char* first = (char*)&code;
        size_t start = m_byteCode.size();

        m_byteCode.resizeWithUninitializedValues(m_byteCode.size() + sizeof(CodeType));
        for (size_t i = 0; i < sizeof(CodeType); i++) {
            m_byteCode[start++] = *first;
            first++;
        }
    }

    template <typename CodeType>
    void pushByteCodeToFront(const CodeType& code)
    {
        char* first = (char*)&code;
        size_t start = m_byteCode.size();

        for (size_t i = 0; i < sizeof(CodeType); i++) {
            m_byteCode.insert(i, *first);
            first++;
        }
    }

    template <typename CodeType>
    CodeType* peekByteCode(size_t position)
    {
        return reinterpret_cast<CodeType*>(&m_byteCode[position]);
    }

    void expandByteCode(size_t s)
    {
        m_byteCode.resizeWithUninitializedValues(m_byteCode.size() + s);
    }

    void resizeByteCode(size_t newSize)
    {
        m_byteCode.resizeWithUninitializedValues(newSize);
    }

    size_t currentByteCodeSize() const
    {
        return m_byteCode.size();
    }

    const uint8_t* byteCode() const { return m_byteCode.data(); }
#if !defined(NDEBUG)
    void dumpByteCode();
#endif

    const Vector<CatchInfo, std::allocator<CatchInfo>>& catchInfo() const
    {
        return m_catchInfo;
    }

#if defined(WALRUS_ENABLE_JIT)
    void setJITFunction(JITFunction* jitFunction)
    {
        ASSERT(m_jitFunction == nullptr);
        m_jitFunction = jitFunction;
    }

    JITFunction* jitFunction()
    {
        return m_jitFunction;
    }
#endif
    void setStackSize(uint16_t size)
    {
        m_requiredStackSize = size;
    }

#if !defined(NDEBUG)
    void pushLocalDebugData(Walrus::ByteCodeStackOffset o)
    {
        m_localDebugData.push_back(o);
    }

    void pushConstDebugData(Walrus::Value value, Walrus::ByteCodeStackOffset o)
    {
        m_constantDebugData.push_back(std::pair<Walrus::Value, size_t>(value, o));
    }
#endif

private:
    bool m_hasTryCatch;
    uint16_t m_requiredStackSize;
    FunctionType* m_functionType;
    ValueTypeVector m_local;
    Vector<uint8_t, std::allocator<uint8_t>> m_byteCode;
#if !defined(NDEBUG)
    Vector<size_t, std::allocator<size_t>> m_localDebugData;
    Vector<std::pair<Value, size_t>, std::allocator<std::pair<Value, size_t>>> m_constantDebugData;
#endif
    Vector<CatchInfo, std::allocator<CatchInfo>> m_catchInfo;
#if defined(WALRUS_ENABLE_JIT)
    JITFunction* m_jitFunction;
#endif
};

class Data {
public:
    Data(uint32_t index, ModuleFunction* moduleFunction, Vector<uint8_t, std::allocator<uint8_t>>&& initData)
        : m_moduleFunction(moduleFunction)
        , m_initData(std::move(initData))
        , m_memIndex(index)
    {
    }

    ModuleFunction* moduleFunction() const
    {
        ASSERT(!!m_moduleFunction);
        return m_moduleFunction;
    }

    uint16_t memIndex() { return m_memIndex; }

    const VectorWithFixedSize<uint8_t, std::allocator<uint8_t>>& initData() const
    {
        return m_initData;
    }

private:
    ModuleFunction* m_moduleFunction;
    VectorWithFixedSize<uint8_t, std::allocator<uint8_t>> m_initData;
    uint16_t m_memIndex;
};

class Element {
public:
    Element(SegmentMode mode, uint32_t tableIndex, ModuleFunction* offsetFunction, Vector<ModuleFunction*>&& exprFunctions)
        : m_mode(mode)
        , m_tableIndex(tableIndex)
        , m_offsetFunction(offsetFunction)
        , m_exprFunctions(std::move(exprFunctions))
    {
    }

    Element(SegmentMode mode, uint32_t tableIndex, Vector<ModuleFunction*>&& exprFunctions)
        : m_mode(mode)
        , m_tableIndex(tableIndex)
        , m_offsetFunction(nullptr)
        , m_exprFunctions(std::move(exprFunctions))
    {
    }

    SegmentMode mode() const
    {
        return m_mode;
    }

    uint32_t tableIndex() const
    {
        return m_tableIndex;
    }

    bool hasOffsetFunction() const
    {
        return !!m_offsetFunction;
    }

    ModuleFunction* offsetFunction()
    {
        ASSERT(hasOffsetFunction());
        return m_offsetFunction;
    }

    const Vector<ModuleFunction*>& exprFunctions() const
    {
        return m_exprFunctions;
    }

private:
    SegmentMode m_mode;
    uint32_t m_tableIndex;
    ModuleFunction* m_offsetFunction;
    Vector<ModuleFunction*> m_exprFunctions;
};

class Module : public Object {
    friend class wabt::WASMBinaryReader;
    friend class JITCompiler;
    friend class Store;

public:
    Module(Store* store, WASMParsingResult& result);

    Store* store() const
    {
        return m_store;
    }

    size_t numberOfFunctions()
    {
        return m_functions.size();
    }

    ModuleFunction* function(uint32_t index) const
    {
        ASSERT(index < m_functions.size());
        return m_functions[index];
    }

    CompositeType* compositeType(uint32_t index) const
    {
        ASSERT(index < m_compositeTypes.size());
        return m_compositeTypes[index];
    }

    FunctionType* functionType(uint32_t index) const
    {
        ASSERT(index < m_compositeTypes.size());
        return m_compositeTypes[index]->asFunction();
    }

    size_t numberOfTableTypes()
    {
        return m_tableTypes.size();
    }

    TableType* tableType(uint32_t index) const
    {
        ASSERT(index < m_tableTypes.size());
        return m_tableTypes[index];
    }

    size_t numberOfMemoryTypes()
    {
        return m_memoryTypes.size();
    }

    MemoryType* memoryType(uint32_t index) const
    {
        ASSERT(index < m_memoryTypes.size());
        return m_memoryTypes[index];
    }

    size_t numberOfGlobalTypes()
    {
        return m_globalTypes.size();
    }

    GlobalType* globalType(uint32_t index) const
    {
        ASSERT(index < m_globalTypes.size());
        return m_globalTypes[index];
    }

    size_t numberOfTagTypes()
    {
        return m_tagTypes.size();
    }

    TagType* tagType(uint32_t index) const
    {
        ASSERT(index < m_tagTypes.size());
        return m_tagTypes[index];
    }

    size_t numberOfDataSegments() const
    {
        return m_datas.size();
    }

    size_t numberOfElemSegments() const
    {
        return m_elements.size();
    }

    const VectorWithFixedSize<ImportType*, std::allocator<ImportType*>>& imports() const
    {
        return m_imports;
    }

    const VectorWithFixedSize<ExportType*, std::allocator<ExportType*>>& exports() const
    {
        return m_exports;
    }

    void postParsing();

    Instance* instantiate(ExecutionState& state, const ExternVector& imports);

#if defined(WALRUS_ENABLE_JIT)
    /* Passing 0 as functionsLength compiles all functions. */
    void jitCompile(ModuleFunction** functions, size_t functionsLength, uint32_t JITFlags);
#endif

private:
    ~Module();

    Store* m_store;
    bool m_seenStartAttribute;
    uint32_t m_version;
    uint32_t m_start;

    VectorWithFixedSize<ImportType*, std::allocator<ImportType*>> m_imports;
    VectorWithFixedSize<ExportType*, std::allocator<ExportType*>> m_exports;

    VectorWithFixedSize<ModuleFunction*, std::allocator<ModuleFunction*>> m_functions;

    VectorWithFixedSize<Data*, std::allocator<Data*>> m_datas;
    VectorWithFixedSize<Element*, std::allocator<Element*>> m_elements;

    CompositeTypeVector m_compositeTypes;
    GlobalTypeVector m_globalTypes;
    TableTypeVector m_tableTypes;
    MemoryTypeVector m_memoryTypes;
    TagTypeVector m_tagTypes;
#if defined(WALRUS_ENABLE_JIT)
    JITModule* m_jitModule;
#endif
};

} // namespace Walrus

#endif // __WalrusModule__
