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
    FunctionTypeVector m_param;
    FunctionTypeVector m_result;
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
                Global };

    ModuleImport(uint32_t importIndex,
                 String* moduleName,
                 String* fieldName,
                 uint32_t functionIndex,
                 uint32_t functionTypeIndex)
        : m_type(Type::Function)
        , m_importIndex(importIndex)
        , m_moduleName(std::move(moduleName))
        , m_fieldName(std::move(fieldName))
        , m_functionIndex(functionIndex)
        , m_functionTypeIndex(functionTypeIndex)
    {
    }

    Type type() const { return m_type; }

    uint32_t importIndex() const { return m_importIndex; }

    String* moduleName() const { return m_moduleName; }

    String* fieldName() const { return m_fieldName; }

    uint32_t functionIndex() const
    {
        ASSERT(type() == Type::Function);
        return m_functionIndex;
    }

    uint32_t functionTypeIndex() const
    {
        ASSERT(type() == Type::Function);
        return m_functionTypeIndex;
    }

private:
    Type m_type;
    uint32_t m_importIndex;
    String* m_moduleName;
    String* m_fieldName;

    union {
        struct {
            uint32_t m_functionIndex;
            uint32_t m_functionTypeIndex;
        };
    };
};

class ModuleFunction : public gc {
    friend class wabt::WASMBinaryReader;

public:
    ModuleFunction(Module* module, uint32_t functionIndex, uint32_t functionTypeIndex)
        : m_module(module)
        , m_functionIndex(functionIndex)
        , m_functionTypeIndex(functionTypeIndex)
        , m_requiredStackSize(0)
    {
    }

    Module* module() const { return m_module; }

    uint32_t functionIndex() const { return m_functionIndex; }

    uint32_t functionTypeIndex() const { return m_functionTypeIndex; }

    uint32_t requiredStackSize() const { return m_requiredStackSize; }

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

    uint8_t* byteCode() { return m_byteCode.data(); }

private:
    Module* m_module;
    uint32_t m_functionIndex;
    uint32_t m_functionTypeIndex;
    uint32_t m_requiredStackSize;
    Vector<uint8_t, GCUtil::gc_malloc_atomic_allocator<uint8_t>> m_byteCode;
};

class Module : public gc {
    friend class wabt::WASMBinaryReader;

public:
    Module()
        : m_seenStartAttribute(false)
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

    const Vector<ModuleImport*, GCUtil::gc_malloc_allocator<ModuleImport*>>& import() const
    {
        return m_import;
    }

    Instance* instantiate(const ValueVector& imports);

private:
    bool m_seenStartAttribute;
    uint32_t m_version;
    uint32_t m_start;
    Vector<ModuleImport*, GCUtil::gc_malloc_allocator<ModuleImport*>> m_import;
    Vector<FunctionType*, GCUtil::gc_malloc_allocator<FunctionType*>>
        m_functionType;
    Vector<ModuleFunction*, GCUtil::gc_malloc_allocator<ModuleFunction*>>
        m_function;
};

} // namespace Walrus

#endif // __WalrusModule__
