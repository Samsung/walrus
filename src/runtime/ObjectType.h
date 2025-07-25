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

#ifndef __WalrusObjectType__
#define __WalrusObjectType__

#include "runtime/Type.h"
#include "runtime/Value.h"

namespace Walrus {

class ModuleFunction;

class ObjectType {
public:
    enum Kind : uint8_t {
        Invalid,
        FunctionKind,
        RecursiveTypeKind,
        GlobalKind,
        TableKind,
        MemoryKind,
        TagKind,
    };

    virtual ~ObjectType() {}

    Kind kind() const { return m_kind; }

protected:
    ObjectType(Kind kind)
        : m_kind(kind)
    {
    }

    Kind m_kind;
};

class CompositeType : public ObjectType {
public:
    friend class TypeStore;

    ObjectType* getNextType() const
    {
        return m_nextType;
    }

    CompositeType* getNextCompositeType() const
    {
        ASSERT(m_nextType == nullptr || m_nextType->kind() == ObjectType::FunctionKind);
        return static_cast<CompositeType*>(m_nextType);
    }

    void setNextType(ObjectType* nextType)
    {
        m_nextType = nextType;
    }

protected:
    CompositeType(Kind kind)
        : ObjectType(kind)
        , m_nextType(nullptr)
    {
        ASSERT(kind == FunctionKind);
    }

private:
    ObjectType* m_nextType;
};

class FunctionType : public CompositeType {
public:
    friend class TypeStore;

    FunctionType(TypeVector* param,
                 TypeVector* result)
        : CompositeType(ObjectType::FunctionKind)
        , m_paramTypes(param)
        , m_resultTypes(result)
        , m_paramStackSize(computeStackSize(*m_paramTypes))
        , m_resultStackSize(computeStackSize(*m_resultTypes))
    {
    }

    ~FunctionType()
    {
        delete m_paramTypes;
        delete m_resultTypes;
    }

    const TypeVector& param() const { return *m_paramTypes; }
    const TypeVector& result() const { return *m_resultTypes; }
    size_t paramStackSize() const { return m_paramStackSize; }
    size_t resultStackSize() const { return m_resultStackSize; }

    bool equals(const FunctionType* other) const;

private:
    TypeVector* m_paramTypes;
    TypeVector* m_resultTypes;
    size_t m_paramStackSize;
    size_t m_resultStackSize;

    static size_t computeStackSize(const TypeVector& v)
    {
        size_t s = 0;
        for (size_t i = 0; i < v.size(); i++) {
            s += valueStackAllocatedSize(v[i]);
        }
        return s;
    }
};

class GlobalType : public ObjectType {
public:
    GlobalType(Type type, bool mut);
    ~GlobalType();

    Type type() const { return m_type; }
    bool isMutable() const { return m_mutable; }
    ModuleFunction* function() const { return m_function; }

    inline void setFunction(ModuleFunction* func)
    {
        ASSERT(!m_function);
        m_function = func;
    }

private:
    Type m_type;
    bool m_mutable;
    ModuleFunction* m_function;
};

class TableType : public ObjectType {
public:
    TableType(Type type, uint32_t initSize, uint32_t maxSize)
        : ObjectType(ObjectType::TableKind)
        , m_type(type)
        , m_initialSize(initSize)
        , m_maximumSize(maxSize)
    {
    }

    Type type() const { return m_type; }
    uint32_t initialSize() const { return m_initialSize; }
    uint32_t maximumSize() const { return m_maximumSize; }

private:
    Type m_type;
    uint32_t m_initialSize;
    uint32_t m_maximumSize;
};

class MemoryType : public ObjectType {
public:
    MemoryType(uint64_t initSize, uint64_t maxSize, bool isShared)
        : ObjectType(ObjectType::MemoryKind)
        , m_initialSize(initSize)
        , m_maximumSize(maxSize)
        , m_isShared(isShared)
    {
    }

    uint64_t initialSize() const { return m_initialSize; }
    uint64_t maximumSize() const { return m_maximumSize; }
    bool isShared() const { return m_isShared; }

private:
    // size should be uint64_t type
    uint64_t m_initialSize;
    uint64_t m_maximumSize;
    bool m_isShared;
};

class TagType : public ObjectType {
public:
    TagType(uint32_t sigIndex)
        : ObjectType(ObjectType::TagKind)
        , m_sigIndex(sigIndex)
    {
    }

    uint32_t sigIndex() const { return m_sigIndex; }

private:
    uint32_t m_sigIndex;
};

// ObjectType Vectors
typedef VectorWithFixedSize<FunctionType*, std::allocator<FunctionType*>> FunctionTypeVector;
typedef VectorWithFixedSize<GlobalType*, std::allocator<GlobalType*>> GlobalTypeVector;
typedef VectorWithFixedSize<TableType*, std::allocator<TableType*>> TableTypeVector;
typedef VectorWithFixedSize<MemoryType*, std::allocator<MemoryType*>> MemoryTypeVector;
typedef VectorWithFixedSize<TagType*, std::allocator<TagType*>> TagTypeVector;

} // namespace Walrus

#endif // __WalrusObjectType__
