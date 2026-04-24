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

#ifndef __WalrusComponent__
#define __WalrusComponent__

#include "runtime/Module.h"

namespace wabt {
class WASMComponentBinaryReader;
}

namespace Walrus {

enum class ComponentSort : uint8_t {
    CoreFunc,
    CoreTable,
    CoreMemory,
    CoreGlobal,
    CoreType,
    CoreModule,
    CoreInstance,
    Func,
    Value,
    Type,
    Component,
    Instance,
    Invalid,
};

class ComponentValueType;
class ComponentValueTypeRef;
class ComponentTypeItems;
class ComponentTypeListFixed;
class ComponentTypeTuple;
class ComponentTypeLabels;
class ComponentTypeResult;
class ComponentTypeFunc;
class ComponentTypeResource;
class ComponentTypeResourceRef;
class ComponentType;

class ComponentRefCounted {
public:
    enum Kind : uint8_t {
        ValueTypeKind,
        RecordKind,
        VariantKind,
        ListKind,
        ListFixedKind,
        TupleKind,
        FlagsKind,
        EnumKind,
        OptionKind,
        ResultKind,
        OwnKind,
        BorrowKind,
        StreamKind,
        FutureKind,
        FuncKind,
        AsyncFuncKind,
        InstanceTypeKind,
        ComponentTypeKind,
        InstanceKind,
        ResourceKind,
        ResourceAsyncKind,
    };

    ComponentRefCounted(Kind kind)
        : m_refCount(1)
        , m_kind(kind)
    {
    }

    virtual ~ComponentRefCounted() {}

    void addRef()
    {
        ASSERT(m_refCount > 0);
        m_refCount++;
    }

    void releaseRef()
    {
        ASSERT(m_refCount > 0);
        m_refCount--;
        if (m_refCount == 0) {
            releaseAllRefs(this);
        }
    }

    Kind kind() const
    {
        return m_kind;
    }

    bool isValueType() const
    {
        return kind() == ValueTypeKind;
    }

    ComponentValueType* asValueType()
    {
        ASSERT(isValueType());
        return reinterpret_cast<ComponentValueType*>(this);
    }

    bool isValueTypeRef() const
    {
        return kind() == ListKind || kind() == OptionKind
            || kind() == StreamKind || kind() == FutureKind;
    }

    ComponentValueTypeRef* asValueTypeRef()
    {
        ASSERT(isValueTypeRef());
        return reinterpret_cast<ComponentValueTypeRef*>(this);
    }

    bool isTypeItems() const
    {
        return kind() == RecordKind || kind() == VariantKind;
    }

    ComponentTypeItems* asTypeItems()
    {
        ASSERT(isTypeItems());
        return reinterpret_cast<ComponentTypeItems*>(this);
    }

    ComponentTypeListFixed* asTypeListFixed()
    {
        ASSERT(kind() == ListFixedKind);
        return reinterpret_cast<ComponentTypeListFixed*>(this);
    }

    ComponentTypeTuple* asTypeTuple()
    {
        ASSERT(kind() == TupleKind);
        return reinterpret_cast<ComponentTypeTuple*>(this);
    }

    bool isTypeLabels() const
    {
        return kind() == FlagsKind || kind() == EnumKind;
    }

    ComponentTypeLabels* asTypeLabels()
    {
        ASSERT(isTypeLabels());
        return reinterpret_cast<ComponentTypeLabels*>(this);
    }

    ComponentTypeResult* asTypeResult()
    {
        ASSERT(kind() == ResultKind);
        return reinterpret_cast<ComponentTypeResult*>(this);
    }

    bool isTypeResourceRef() const
    {
        return kind() == OwnKind || kind() == BorrowKind;
    }

    ComponentTypeResourceRef* asTypeResourceRef()
    {
        ASSERT(isTypeResourceRef());
        return reinterpret_cast<ComponentTypeResourceRef*>(this);
    }

    bool isTypeFunc() const
    {
        return kind() == FuncKind || kind() == AsyncFuncKind;
    }

    ComponentTypeFunc* asTypeFunc()
    {
        ASSERT(isTypeFunc());
        return reinterpret_cast<ComponentTypeFunc*>(this);
    }

    bool isTypeResource() const
    {
        return kind() == ResourceKind || kind() == ResourceAsyncKind;
    }

    ComponentTypeResource* asTypeResource()
    {
        ASSERT(isTypeResource());
        return reinterpret_cast<ComponentTypeResource*>(this);
    }

    bool isComponentType() const
    {
        return kind() == InstanceTypeKind || kind() == ComponentTypeKind;
    }

    ComponentType* asComponentType()
    {
        ASSERT(isComponentType());
        return reinterpret_cast<ComponentType*>(this);
    }

private:
    static void releaseAndInsert(ComponentRefCounted* current, ComponentRefCounted* ref)
    {
        if (ref != nullptr) {
            ref->m_refCount--;
            if (ref->m_refCount == 0) {
                ref->m_nextFree = current->m_nextFree;
                current->m_nextFree = ref;
            }
        }
    }

    static void releaseAllRefs(ComponentRefCounted* ref);

    union {
        size_t m_refCount;
        // Only used during object destruction.
        ComponentRefCounted* m_nextFree;
    };
    Kind m_kind;
};

class ComponentTypeRef {
public:
    enum Type : uint8_t {
        Bool,
        S8,
        U8,
        S16,
        U16,
        S32,
        U32,
        S64,
        U64,
        F32,
        F64,
        Char,
        String,
        ErrorContext,

        TypeIndex,
        TypeNone,
    };

    ComponentTypeRef()
        : m_type(TypeNone)
        , m_ref(nullptr)
    {
    }

    ComponentTypeRef(ComponentRefCounted* ref)
        : m_type(TypeIndex)
        , m_ref(ref)
    {
    }

    ComponentTypeRef(Type type)
        : m_type(type)
        , m_ref(nullptr)
    {
        ASSERT(type != TypeIndex && type != TypeNone);
    }

    Type type() const { return m_type; }
    ComponentRefCounted* ref() const { return m_ref; }

    bool equals(const ComponentTypeRef& other) const
    {
        return m_type == other.m_type && m_ref == other.m_ref;
    }

private:
    Type m_type;
    ComponentRefCounted* m_ref;
};

class ComponentValueType : public ComponentRefCounted {
public:
    ComponentValueType(ComponentTypeRef::Type type)
        : ComponentRefCounted(ValueTypeKind)
        , m_type(type)
    {
        ASSERT(type != ComponentTypeRef::TypeIndex && type != ComponentTypeRef::TypeNone);
    }

private:
    ComponentTypeRef::Type m_type;
};

class ComponentValueTypeRef : public ComponentRefCounted {
public:
    ComponentValueTypeRef(Kind kind, const ComponentTypeRef& type)
        : ComponentRefCounted(kind)
        , m_type(type)
    {
        ASSERT(isValueTypeRef());
    }

    const ComponentTypeRef& type() const
    {
        return m_type;
    }

private:
    ComponentTypeRef m_type;
};

class ComponentTypeItems : public ComponentRefCounted {
public:
    struct Item {
        std::string name;
        ComponentTypeRef type;
    };

    ComponentTypeItems(Kind kind)
        : ComponentRefCounted(kind)
    {
        ASSERT(isTypeItems());
    }

    std::vector<Item>& items()
    {
        return m_items;
    }

private:
    std::vector<Item> m_items;
};

class ComponentTypeListFixed : public ComponentRefCounted {
public:
    ComponentTypeListFixed(const ComponentTypeRef& type, uint32_t size)
        : ComponentRefCounted(ListFixedKind)
        , m_type(type)
        , m_size(size)
    {
    }

    const ComponentTypeRef& type() const
    {
        return m_type;
    }

    uint32_t size() const
    {
        return m_size;
    }

private:
    ComponentTypeRef m_type;
    uint32_t m_size;
};

class ComponentTypeTuple : public ComponentRefCounted {
public:
    ComponentTypeTuple()
        : ComponentRefCounted(TupleKind)
    {
    }

    std::vector<ComponentTypeRef>& items()
    {
        return m_items;
    }

private:
    std::vector<ComponentTypeRef> m_items;
};

class ComponentTypeLabels : public ComponentRefCounted {
public:
    ComponentTypeLabels(Kind kind)
        : ComponentRefCounted(kind)
    {
        ASSERT(isTypeLabels());
    }

    std::vector<std::string>& labels()
    {
        return m_labels;
    }

private:
    std::vector<std::string> m_labels;
};

class ComponentTypeResult : public ComponentRefCounted {
public:
    ComponentTypeResult(const ComponentTypeRef& result, const ComponentTypeRef& error)
        : ComponentRefCounted(ResultKind)
        , m_result(result)
        , m_error(error)
    {
    }

    const ComponentTypeRef& result() const
    {
        return m_result;
    }

    const ComponentTypeRef& error() const
    {
        return m_error;
    }

private:
    ComponentTypeRef m_result;
    ComponentTypeRef m_error;
};

class ComponentTypeFunc : public ComponentRefCounted {
public:
    struct Param {
        std::string name;
        ComponentTypeRef type;
    };

    ComponentTypeFunc(Kind kind)
        : ComponentRefCounted(kind)
    {
        ASSERT(isTypeFunc());
    }

    std::vector<Param>& params()
    {
        return m_params;
    }

    ComponentTypeRef& result()
    {
        return m_result;
    }

private:
    std::vector<Param> m_params;
    ComponentTypeRef m_result;
};

class ComponentTypeResource : public ComponentRefCounted {
public:
    ComponentTypeResource(Kind kind, bool i64)
        : ComponentRefCounted(kind)
        , m_i64(i64)
    {
        ASSERT(isTypeResource());
    }

    bool i64() const
    {
        return m_i64;
    }

private:
    bool m_i64;
};

class ComponentTypeResourceRef : public ComponentRefCounted {
public:
    ComponentTypeResourceRef(Kind kind, ComponentTypeResource* ref)
        : ComponentRefCounted(kind)
        , m_ref(ref)
    {
        ASSERT(isTypeResourceRef());
    }

    ComponentTypeResource* ref() const
    {
        return m_ref;
    }

private:
    ComponentTypeResource* m_ref;
};

// Immutable type declarations for components, instances
class ComponentType : public ComponentRefCounted {
public:
    struct External {
        std::string name;
        ComponentRefCounted* type;
    };

    ComponentType(Kind kind)
        : ComponentRefCounted(kind)
    {
        ASSERT(isComponentType());
    }

    void pushType(ComponentRefCounted* type)
    {
        m_types.push_back(type);
    }

    ComponentRefCounted* getType(size_t index)
    {
        return m_types[index];
    }

    ComponentRefCounted* lastType()
    {
        ASSERT(m_types.size() > 0);
        return m_types.back();
    }

    std::vector<ComponentRefCounted*>& types()
    {
        return m_types;
    }

    std::vector<External>& exports()
    {
        return m_exports;
    }

    std::vector<External>& imports()
    {
        return m_imports;
    }

private:
    std::vector<ComponentRefCounted*> m_types;
    std::vector<External> m_exports;
    std::vector<External> m_imports;
};

class CanonicalOptions {
public:
    enum StringEncoding {
        Utf8,
        Utf16,
        Latin1Utf16,
    };

    static constexpr uint32_t NoIndex = ~static_cast<uint32_t>(0);

    CanonicalOptions(StringEncoding encoding, bool isAsync, uint32_t memoryIndex,
                     uint32_t reallocIndex, uint32_t postReturnIndex, uint32_t callbackIndex)
        : m_encoding(encoding)
        , m_isAsync(isAsync)
        , m_memoryIndex(memoryIndex)
        , m_reallocIndex(reallocIndex)
        , m_postReturnIndex(postReturnIndex)
        , m_callbackIndex(callbackIndex)
    {
    }

    StringEncoding encoding() const
    {
        return m_encoding;
    }

    uint32_t memoryIndex() const
    {
        return m_memoryIndex;
    }

    uint32_t reallocIndex() const
    {
        return m_reallocIndex;
    }

    uint32_t postReturnIndex() const
    {
        return m_postReturnIndex;
    }

private:
    StringEncoding m_encoding;
    bool m_isAsync;
    uint32_t m_memoryIndex;
    uint32_t m_reallocIndex;
    uint32_t m_postReturnIndex;
    uint32_t m_callbackIndex;
};

class ComponentInstantiate;
class ComponentInstantiateInline;
class ComponentCanonLift;
class ComponentCanonLower;
class ComponentCanonType;

class ComponentDeclaration {
public:
    enum Kind : uint8_t {
        ComponentKind,
        InstantiateKind,
        InstantiateInlineKind,
        CanonLiftKind,
        CanonLowerKind,
        CanonResourceNew,
        CanonResourceDrop,
        CanonResourceRep,
        ImportKind,
    };

    ComponentDeclaration(Kind kind)
        : m_kind(kind)
    {
    }

    virtual ~ComponentDeclaration() {}

    Kind kind() const
    {
        return m_kind;
    }

    ComponentInstantiate* asInstantiate()
    {
        ASSERT(kind() == InstantiateKind);
        return reinterpret_cast<ComponentInstantiate*>(this);
    }

    ComponentInstantiateInline* asInstantiateInline()
    {
        ASSERT(kind() == InstantiateInlineKind);
        return reinterpret_cast<ComponentInstantiateInline*>(this);
    }

    ComponentCanonLift* asCanonLift()
    {
        ASSERT(kind() == CanonLiftKind);
        return reinterpret_cast<ComponentCanonLift*>(this);
    }

    ComponentCanonLower* asCanonLower()
    {
        ASSERT(kind() == CanonLowerKind);
        return reinterpret_cast<ComponentCanonLower*>(this);
    }

    bool isCanonType()
    {
        return kind() == CanonResourceNew || kind() == CanonResourceDrop || kind() == CanonResourceRep;
    }

    ComponentCanonType* asCanonType()
    {
        ASSERT(isCanonType());
        return reinterpret_cast<ComponentCanonType*>(this);
    }

private:
    Kind m_kind;
};

class ComponentInstantiate : public ComponentDeclaration {
public:
    struct Argument {
        std::string name;
        ComponentSort sort;
        uint32_t index;
    };

    ComponentInstantiate(uint32_t componentIndex)
        : ComponentDeclaration(InstantiateKind)
        , m_componentIndex(componentIndex)
    {
    }

    uint32_t componentIndex()
    {
        return m_componentIndex;
    }

    std::vector<Argument>& arguments()
    {
        return m_arguments;
    }

private:
    uint32_t m_componentIndex;
    std::vector<Argument> m_arguments;
};

class ComponentInstantiateInline : public ComponentDeclaration {
public:
    struct Argument {
        ComponentSort sort;
        uint32_t index;
    };

    ComponentInstantiateInline()
        : ComponentDeclaration(InstantiateInlineKind)
    {
        m_type = new ComponentType(ComponentRefCounted::InstanceTypeKind);
    }

    ComponentType* type()
    {
        return m_type;
    }

    std::vector<Argument>& arguments()
    {
        return m_arguments;
    }

private:
    ComponentType* m_type;
    std::vector<Argument> m_arguments;
};

class ComponentCanonLift : public ComponentDeclaration {
public:
    ComponentCanonLift(uint32_t coreFuncIndex, const CanonicalOptions& options, ComponentTypeFunc* funcType)
        : ComponentDeclaration(CanonLiftKind)
        , m_options(options)
        , m_coreFuncIndex(coreFuncIndex)
        , m_funcType(funcType)
    {
    }

    const CanonicalOptions& options() const
    {
        return m_options;
    }

    uint32_t coreFuncIndex() const
    {
        return m_coreFuncIndex;
    }

    ComponentTypeFunc* funcType() const
    {
        return m_funcType;
    }

private:
    CanonicalOptions m_options;
    uint32_t m_coreFuncIndex;
    ComponentTypeFunc* m_funcType;
};

class ComponentCanonLower : public ComponentDeclaration {
public:
    ComponentCanonLower(uint32_t funcIndex, const CanonicalOptions& options)
        : ComponentDeclaration(CanonLowerKind)
        , m_options(options)
        , m_funcIndex(funcIndex)
    {
    }

    const CanonicalOptions& options() const
    {
        return m_options;
    }

    uint32_t funcIndex() const
    {
        return m_funcIndex;
    }

private:
    CanonicalOptions m_options;
    uint32_t m_funcIndex;
};

class ComponentCanonType : public ComponentDeclaration {
public:
    ComponentCanonType(Kind kind, ComponentRefCounted* type)
        : ComponentDeclaration(kind)
        , m_type(type)
    {
        ASSERT(isCanonType());
        type->addRef();
    }

    ~ComponentCanonType()
    {
        m_type->releaseRef();
    }

private:
    ComponentRefCounted* m_type;
};

class ComponentImport : public ComponentDeclaration {
public:
    ComponentImport(uint32_t importIndex)
        : ComponentDeclaration(ImportKind)
        , m_importIndex(importIndex)
    {
    }

    uint32_t importIndex() const
    {
        return m_importIndex;
    }

private:
    uint32_t m_importIndex;
};

class Component : public ComponentDeclaration {
    friend class wabt::WASMComponentBinaryReader;

public:
    Component()
        : ComponentDeclaration(ComponentKind)
    {
        m_type = new ComponentType(ComponentRefCounted::ComponentTypeKind);
    }

    ~Component();

    ComponentType* type()
    {
        return m_type;
    }

    void pushDeclaration(ComponentDeclaration* declaration)
    {
        m_declarations.push_back(declaration);
    }

    std::vector<ComponentDeclaration*>& declarations()
    {
        return m_declarations;
    }

private:
    // Declarations for instantiation.
    ComponentType* m_type;
    std::vector<ComponentDeclaration*> m_declarations;
};

} // namespace Walrus

#endif // __WalrusComponent__
