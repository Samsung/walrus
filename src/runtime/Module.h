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

#include <numeric>
#include "runtime/Value.h"
#include "util/Vector.h"

namespace wabt {
class WASMBinaryReader;
}

namespace Walrus {

class Store;
class Module;
class Instance;

class FunctionType : public gc {
public:
    typedef Vector<Value::Type, GCUtil::gc_malloc_atomic_allocator<Value::Type>>
        FunctionTypeVector;
    FunctionType(uint32_t index,
                 FunctionTypeVector&& param,
                 FunctionTypeVector&& result)
        : m_index(index)
        , m_param(std::move(param))
        , m_result(std::move(result))
        , m_paramStackSize(computeStackSize(m_param))
        , m_resultStackSize(computeStackSize(m_result))
    {
    }

    uint32_t index() const { return m_index; }

    const FunctionTypeVector& param() const { return m_param; }

    const FunctionTypeVector& result() const { return m_result; }

    size_t paramStackSize() const { return m_paramStackSize; }

    size_t resultStackSize() const { return m_resultStackSize; }

private:
    uint32_t m_index;
    const FunctionTypeVector m_param;
    const FunctionTypeVector m_result;
    size_t m_paramStackSize;
    size_t m_resultStackSize;

    static size_t computeStackSize(const FunctionTypeVector& v)
    {
        size_t s = 0;
        for (size_t i = 0; i < v.size(); i++) {
            s += valueSizeInStack(v[i]);
        }
        return s;
    }
};

// https://webassembly.github.io/spec/core/syntax/modules.html#syntax-import
class ModuleImport : public gc {
public:
    enum Type { Function,
                Table,
                Memory,
                Global,
                Tag };

    ModuleImport(uint32_t importIndex,
                 String* moduleName,
                 String* fieldName,
                 uint32_t functionIndex,
                 uint32_t functionTypeIndex)
        : m_type(Type::Function)
        , m_importIndex(importIndex)
        , m_moduleName(std::move(moduleName))
        , m_fieldName(std::move(fieldName))
        , m_index(functionIndex)
        , m_functionTypeIndex(functionTypeIndex)
    {
    }

    ModuleImport(Type t,
                 uint32_t importIndex,
                 String* moduleName,
                 String* fieldName,
                 uint32_t index)
        : m_type(t)
        , m_importIndex(importIndex)
        , m_moduleName(std::move(moduleName))
        , m_fieldName(std::move(fieldName))
        , m_index(index)
    {
    }

    Type type() const { return m_type; }

    uint32_t importIndex() const { return m_importIndex; }

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

private:
    Type m_type;
    uint32_t m_importIndex;
    String* m_moduleName;
    String* m_fieldName;
    uint32_t m_index;
    uint32_t m_functionTypeIndex;
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
                 uint32_t exportIndex,
                 uint32_t itemIndex)
        : m_type(type)
        , m_name(name)
        , m_exportIndex(exportIndex)
        , m_itemIndex(itemIndex)
    {
    }

    Type type() const { return m_type; }

    uint32_t exportIndex() const { return m_exportIndex; }

    String* name() const { return m_name; }

    uint32_t itemIndex() const
    {
        return m_itemIndex;
    }

private:
    Type m_type;
    String* m_name;
    uint32_t m_exportIndex;
    uint32_t m_itemIndex;
};

class ModuleFunction : public gc {
    friend class wabt::WASMBinaryReader;

public:
    typedef Vector<Value::Type, GCUtil::gc_malloc_atomic_allocator<Value::Type>>
        LocalValueVector;
    struct CatchInfo {
        size_t m_tryStart;
        size_t m_tryEnd;
        uint32_t m_tagIndex;
        size_t m_catchStartPosition;
        size_t m_stackSizeToBe;
    };

    ModuleFunction(Module* module, uint32_t functionIndex, uint32_t functionTypeIndex)
        : m_module(module)
        , m_functionIndex(functionIndex)
        , m_functionTypeIndex(functionTypeIndex)
        , m_requiredStackSize(0)
        , m_requiredStackSizeDueToLocal(0)
    {
    }

    Module* module() const { return m_module; }

    uint32_t functionIndex() const { return m_functionIndex; }

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
    uint32_t m_functionIndex;
    uint32_t m_functionTypeIndex;
    uint32_t m_requiredStackSize;
    uint32_t m_requiredStackSizeDueToLocal;
    LocalValueVector m_local;
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

class Module : public gc {
    friend class wabt::WASMBinaryReader;

public:
    Module(Store* store)
        : m_store(store)
        , m_seenStartAttribute(false)
        , m_version(0)
        , m_start(0)
    {
    }

    ModuleFunction* function(uint32_t index)
    {
        for (size_t i = 0; i < m_function.size(); i++) {
            if (m_function[i]->functionIndex() == index) {
                return m_function[i];
            }
        }
        ASSERT_NOT_REACHED();
        return nullptr;
    }

    FunctionType* functionType(uint32_t index)
    {
        for (size_t i = 0; i < m_functionType.size(); i++) {
            if (m_functionType[i]->index() == index) {
                return m_functionType[i];
            }
        }
        ASSERT_NOT_REACHED();
        return nullptr;
    }

    const Vector<ModuleImport*, GCUtil::gc_malloc_allocator<ModuleImport*>>& moduleImport() const
    {
        return m_import;
    }

    const Vector<ModuleExport*, GCUtil::gc_malloc_allocator<ModuleExport*>>& moduleExport() const
    {
        return m_export;
    }

    Instance* instantiate(const ValueVector& imports);

private:
    Store* m_store;
    bool m_seenStartAttribute;
    uint32_t m_version;
    uint32_t m_start;

    Vector<ModuleImport*, GCUtil::gc_malloc_allocator<ModuleImport*>> m_import;
    Vector<ModuleExport*, GCUtil::gc_malloc_allocator<ModuleExport*>> m_export;

    Vector<FunctionType*, GCUtil::gc_malloc_allocator<FunctionType*>>
        m_functionType;
    Vector<ModuleFunction*, GCUtil::gc_malloc_allocator<ModuleFunction*>>
        m_function;
    Vector<std::tuple<Value::Type, size_t, size_t>, GCUtil::gc_malloc_atomic_allocator<std::tuple<Value::Type, size_t, size_t>>>
        m_table;
    /* initialSize, maximumSize */
    Vector<std::pair<size_t, size_t>, GCUtil::gc_malloc_atomic_allocator<std::pair<size_t, size_t>>>
        m_memory;
    Vector<Data*, GCUtil::gc_malloc_allocator<Data*>> m_data;
    Vector<std::tuple<Value::Type, bool>, GCUtil::gc_malloc_atomic_allocator<std::tuple<Value::Type, bool>>>
        m_global;
    Vector<uint32_t, GCUtil::gc_malloc_atomic_allocator<uint32_t>> m_tag;
    Optional<ModuleFunction*> m_globalInitBlock;
};

} // namespace Walrus

#endif // __WalrusModule__
