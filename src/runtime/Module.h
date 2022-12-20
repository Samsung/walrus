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
class ModuleImport : public gc {
public:
    enum Type { Function,
                Table,
                Memory,
                Global,
                Tag };

    ModuleImport(String* moduleName,
                 String* fieldName,
                 uint32_t functionIndex,
                 uint32_t functionTypeIndex)
        : m_type(Type::Function)
        , m_moduleName(std::move(moduleName))
        , m_fieldName(std::move(fieldName))
        , m_index(functionIndex)
        , m_functionTypeIndex(functionTypeIndex)
    {
    }

    ModuleImport(Type t,
                 String* moduleName,
                 String* fieldName,
                 uint32_t index,
                 uint32_t initialSize = std::numeric_limits<uint32_t>::max(),
                 uint32_t maximumSize = std::numeric_limits<uint32_t>::max(),
                 Value::Type tableType = Value::Type::Void)
        : m_type(t)
        , m_moduleName(std::move(moduleName))
        , m_fieldName(std::move(fieldName))
        , m_index(index)
        , m_initialSize(initialSize)
        , m_maximumSize(maximumSize)
        , m_tableType(tableType)
    {
    }

    Type type() const { return m_type; }

    String* moduleName() const { return m_moduleName; }

    String* fieldName() const { return m_fieldName; }

    uint32_t functionIndex() const
    {
        ASSERT(type() == Type::Function);
        return m_index;
    }

    uint32_t functionTypeIndex() const
    {
        ASSERT(type() == Type::Function);
        return m_functionTypeIndex;
    }

    uint32_t globalIndex() const
    {
        ASSERT(type() == Type::Global);
        return m_index;
    }

    uint32_t tableIndex() const
    {
        ASSERT(type() == Type::Table);
        return m_index;
    }

    Value::Type tableType() const
    {
        ASSERT(type() == Type::Table);
        return m_tableType;
    }

    uint32_t memoryIndex() const
    {
        ASSERT(type() == Type::Memory);
        return m_index;
    }

    uint32_t tagIndex() const
    {
        ASSERT(type() == Type::Tag);
        return m_index;
    }

    uint32_t initialSize() const
    {
        ASSERT(type() == Type::Memory || type() == Type::Table);
        return m_initialSize;
    }

    uint32_t maximumSize() const
    {
        ASSERT(type() == Type::Memory || type() == Type::Table);
        return m_maximumSize;
    }

private:
    Type m_type;
    String* m_moduleName;
    String* m_fieldName;
    uint32_t m_index;
    union {
        struct {
            uint32_t m_functionTypeIndex;
        };
        struct {
            uint32_t m_initialSize;
            uint32_t m_maximumSize;
            Value::Type m_tableType;
        };
    };
};

class ModuleExport : public gc {
public:
    // matches binary format, do not change
    enum Type { Function,
                Table,
                Memory,
                Global,
                Tag };

    ModuleExport(Type type,
                 String* name,
                 uint32_t itemIndex)
        : m_type(type)
        , m_name(name)
        , m_itemIndex(itemIndex)
    {
    }

    Type type() const { return m_type; }

    String* name() const { return m_name; }

    uint32_t itemIndex() const
    {
        return m_itemIndex;
    }

private:
    Type m_type;
    String* m_name;
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

    constexpr static uint32_t g_invalidFunctionTypeIndex = std::numeric_limits<uint32_t>::max();
    ModuleFunction(Module* module, uint32_t functionTypeIndex = g_invalidFunctionTypeIndex);

    Module* module() const { return m_module; }

    uint32_t functionTypeIndex() const { return m_functionTypeIndex; }

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

    void shrinkByteCode(size_t s)
    {
        m_byteCode.resize(m_byteCode.size() - s);
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
    uint32_t m_functionTypeIndex;
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
    Module(Store* store)
        : m_store(store)
        , m_seenStartAttribute(false)
        , m_version(0)
        , m_start(0)
    {
    }

    virtual Object::Kind kind() const override
    {
        return Object::ModuleKind;
    }

    virtual bool isModule() const override
    {
        return true;
    }

    ModuleFunction* function(uint32_t index)
    {
        ASSERT(index < m_function.size());
        return m_function[index];
    }

    FunctionType* functionType(uint32_t index)
    {
        ASSERT(index < m_functionTypes.size());
        return &m_functionTypes[index];
    }

    const Vector<ModuleImport*, GCUtil::gc_malloc_allocator<ModuleImport*>>& moduleImport() const
    {
        return m_import;
    }

    const Vector<ModuleExport*, GCUtil::gc_malloc_allocator<ModuleExport*>>& moduleExport() const
    {
        return m_export;
    }

    Instance* instantiate(ExecutionState& state, const ObjectVector& imports);

private:
    Store* m_store;
    bool m_seenStartAttribute;
    uint32_t m_version;
    uint32_t m_start;

    Vector<ModuleImport*, GCUtil::gc_malloc_allocator<ModuleImport*>> m_import;
    Vector<ModuleExport*, GCUtil::gc_malloc_allocator<ModuleExport*>> m_export;

    Vector<ModuleFunction*, GCUtil::gc_malloc_allocator<ModuleFunction*>>
        m_function;

    FunctionTypeVector m_functionTypes;
    GlobalTypeVector m_globalTypes;
    TableTypeVector m_tableTypes;
    MemoryTypeVector m_memoryTypes;
    TagTypeVector m_tagTypes;

    Vector<Element*, GCUtil::gc_malloc_allocator<Element*>> m_element;
    Vector<Data*, GCUtil::gc_malloc_allocator<Data*>> m_data;

    Optional<ModuleFunction*> m_globalInitBlock;
};

} // namespace Walrus

#endif // __WalrusModule__
