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

#include "runtime/ObjectType.h"
#include "runtime/Object.h"

namespace wabt {
class WASMBinaryReader;
}

namespace Walrus {

class Store;
class Module;
class Instance;

enum class SegmentMode {
    None,
    Active,
    Passive,
    Declared,
};

// https://webassembly.github.io/spec/core/syntax/modules.html#syntax-import
class ImportType : public gc {
public:
    enum Type { Function,
                Table,
                Memory,
                Global,
                Tag };

    ImportType(Type t,
               std::string& moduleName,
               std::string& fieldName,
               const ObjectType* type)
        : m_importType(t)
        , m_moduleName(moduleName)
        , m_fieldName(fieldName)
        , m_type(type)
    {
    }

    ImportType(std::string& moduleName,
               std::string& fieldName,
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

private:
    Type m_importType;
    std::string m_moduleName;
    std::string m_fieldName;
    const ObjectType* m_type;
};

class ExportType : public gc {
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

class ModuleFunction : public gc {
    friend class wabt::WASMBinaryReader;

public:
    struct CatchInfo {
        size_t m_tryStart;
        size_t m_tryEnd;
        uint32_t m_tagIndex;
        size_t m_catchStartPosition;
        size_t m_stackSizeToBe;
    };

    ModuleFunction(Module* module, FunctionType* functionType);

    Module* module() const { return m_module; }
    FunctionType* functionType() const { return m_functionType; }
    uint32_t requiredStackSize() const { return m_requiredStackSize; }
    uint32_t requiredStackSizeDueToLocal() const { return m_requiredStackSizeDueToLocal; }

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
    CodeType* peekByteCode(size_t position)
    {
        return reinterpret_cast<CodeType*>(&m_byteCode[position]);
    }

    void expandByteCode(size_t s)
    {
        m_byteCode.resizeWithUninitializedValues(m_byteCode.size() + s);
    }

    size_t currentByteCodeSize() const
    {
        return m_byteCode.size();
    }

    uint8_t* byteCode() { return m_byteCode.data(); }
#if !defined(NDEBUG)
    void dumpByteCode();
#endif

    const Vector<CatchInfo, GCUtil::gc_malloc_atomic_allocator<CatchInfo>>& catchInfo() const
    {
        return m_catchInfo;
    }

private:
    Module* m_module;
    FunctionType* m_functionType;
    uint32_t m_requiredStackSize;
    uint32_t m_requiredStackSizeDueToLocal;
    ValueTypeVector m_local;
    Vector<uint8_t, GCUtil::gc_malloc_atomic_allocator<uint8_t>> m_byteCode;
    Vector<CatchInfo, GCUtil::gc_malloc_atomic_allocator<CatchInfo>> m_catchInfo;
};

class Data : public gc {
public:
    Data(ModuleFunction* moduleFunction, Vector<uint8_t, GCUtil::gc_malloc_atomic_allocator<uint8_t>>&& initData)
        : m_moduleFunction(moduleFunction)
        , m_initData(std::move(initData))
    {
    }

    ModuleFunction* moduleFunction() const
    {
        return m_moduleFunction;
    }

    const Vector<uint8_t, GCUtil::gc_malloc_atomic_allocator<uint8_t>>& initData() const
    {
        return m_initData;
    }

private:
    ModuleFunction* m_moduleFunction;
    Vector<uint8_t, GCUtil::gc_malloc_atomic_allocator<uint8_t>> m_initData;
};

class Element : public gc {
public:
    Element(SegmentMode mode, uint32_t tableIndex, ModuleFunction* moduleFunction, Vector<uint32_t, GCUtil::gc_malloc_atomic_allocator<uint32_t>>&& functionIndex)
        : m_mode(mode)
        , m_tableIndex(tableIndex)
        , m_moduleFunction(moduleFunction)
        , m_functionIndex(std::move(functionIndex))
    {
    }

    Element(SegmentMode mode, uint32_t tableIndex, Vector<uint32_t, GCUtil::gc_malloc_atomic_allocator<uint32_t>>&& functionIndex)
        : m_mode(mode)
        , m_tableIndex(tableIndex)
        , m_moduleFunction()
        , m_functionIndex(std::move(functionIndex))
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

    bool hasModuleFunction() const
    {
        return !!m_moduleFunction;
    }

    ModuleFunction* moduleFunction()
    {
        ASSERT(hasModuleFunction());
        return m_moduleFunction.value();
    }

    const Vector<uint32_t, GCUtil::gc_malloc_atomic_allocator<uint32_t>>& functionIndex() const
    {
        return m_functionIndex;
    }

private:
    SegmentMode m_mode;
    uint32_t m_tableIndex;
    Optional<ModuleFunction*> m_moduleFunction;
    Vector<uint32_t, GCUtil::gc_malloc_atomic_allocator<uint32_t>> m_functionIndex;
};

class Module : public Object {
    friend class wabt::WASMBinaryReader;

public:
    Module(Store* store);

    virtual Object::Kind kind() const override
    {
        return Object::ModuleKind;
    }

    virtual bool isModule() const override
    {
        return true;
    }

    Store* store() const
    {
        return m_store;
    }

    ModuleFunction* function(uint32_t index)
    {
        ASSERT(index < m_function.size());
        return m_function[index];
    }

    FunctionType* functionType(uint32_t index) const
    {
        ASSERT(index < m_functionTypes.size());
        return m_functionTypes[index];
    }

    TableType* tableType(uint32_t index) const
    {
        ASSERT(index < m_tableTypes.size());
        return m_tableTypes[index];
    }

    MemoryType* memoryType(uint32_t index) const
    {
        ASSERT(index < m_memoryTypes.size());
        return m_memoryTypes[index];
    }

    GlobalType* globalType(uint32_t index) const
    {
        ASSERT(index < m_global.size());
        return const_cast<GlobalType*>(&m_global[index].first);
    }

    const Vector<ImportType*, GCUtil::gc_malloc_allocator<ImportType*>>& imports() const
    {
        return m_imports;
    }

    const Vector<ExportType*, GCUtil::gc_malloc_allocator<ExportType*>>& exports() const
    {
        return m_exports;
    }

    Instance* instantiate(ExecutionState& state, const ObjectVector& imports);

    static FunctionType* initIndexFunctionType();
    static FunctionType* initGlobalFunctionType(Value::Type type);

private:
    Store* m_store;
    bool m_seenStartAttribute;
    uint32_t m_version;
    uint32_t m_start;

    Vector<ImportType*, GCUtil::gc_malloc_allocator<ImportType*>> m_imports;
    Vector<ExportType*, GCUtil::gc_malloc_allocator<ExportType*>> m_exports;

    Vector<ModuleFunction*, GCUtil::gc_malloc_allocator<ModuleFunction*>>
        m_function;

    FunctionTypeVector m_functionTypes;
    TableTypeVector m_tableTypes;
    MemoryTypeVector m_memoryTypes;
    TagTypeVector m_tagTypes;

    Vector<Element*, GCUtil::gc_malloc_allocator<Element*>> m_element;
    Vector<Data*, GCUtil::gc_malloc_allocator<Data*>> m_data;
    Vector<std::pair<GlobalType, Optional<ModuleFunction*>>, GCUtil::gc_malloc_allocator<std::pair<GlobalType, Optional<ModuleFunction*>>>> m_global;
};

} // namespace Walrus

#endif // __WalrusModule__
