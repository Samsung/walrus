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
class FunctionType;
class StructType;
class ArrayType;
class RecursiveType;

class ObjectType {
public:
    enum Kind : uint8_t {
        Invalid,
        FunctionKind,
        StructKind,
        ArrayKind,
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

    CompositeType* getNextType() const
    {
        return m_nextType;
    }

    RecursiveType* getRecursiveType() const
    {
        return m_recursiveType;
    }

    CompositeType** subTypeList() const
    {
        return m_subTypeList;
    }

    uintptr_t subTypeCount() const
    {
        return reinterpret_cast<uintptr_t>(m_subTypeList[0]);
    }

    // Type "less" comparison operation.
    // It is simple, and easy to implement in JIT.
    bool isSubTypeOf(CompositeType* actual) const
    {
        uintptr_t count = subTypeCount();
        return (count <= actual->subTypeCount() && actual->subTypeList()[count] == this);
    }

    bool isFinal() const
    {
        return m_isFinal;
    }

    FunctionType* asFunction()
    {
        ASSERT(kind() == ObjectType::FunctionKind);
        return reinterpret_cast<FunctionType*>(this);
    }

    StructType* asStruct()
    {
        ASSERT(kind() == ObjectType::StructKind);
        return reinterpret_cast<StructType*>(this);
    }

    ArrayType* asArray()
    {
        ASSERT(kind() == ObjectType::ArrayKind);
        return reinterpret_cast<ArrayType*>(this);
    }

protected:
    CompositeType(Kind kind, bool isFinal, CompositeType** subTypeList)
        : ObjectType(kind)
        , m_nextType(nullptr)
        , m_recursiveType(nullptr)
        , m_subTypeList(subTypeList)
        , m_isFinal(isFinal)
    {
        ASSERT(kind == FunctionKind || kind == StructKind || kind == ArrayKind);
    }

private:
    CompositeType* m_nextType;
    RecursiveType* m_recursiveType;
    // Subtype list is an index or TypeStore::NoIndex during parsing, not a pointer.
    // The m_subTypeList[0] is the szie of the items in the list, see subTypeCount().
    // The m_subTypeList[subTypeCount()] is pointer to the composite type.
    CompositeType** m_subTypeList;
    bool m_isFinal;
};

class FunctionType : public CompositeType {
public:
    friend class TypeStore;

    FunctionType(TypeVector* param,
                 TypeVector* result,
                 bool isFinal,
                 CompositeType** subTypeList)
        : CompositeType(ObjectType::FunctionKind, isFinal, subTypeList)
        , m_paramTypes(param)
        , m_resultTypes(result)
        , m_paramStackSize(computeStackSize(*m_paramTypes))
        , m_resultStackSize(computeStackSize(*m_resultTypes))
    {
    }

    FunctionType(TypeVector* param,
                 TypeVector* result)
        : CompositeType(ObjectType::FunctionKind, false, nullptr)
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

class StructType : public CompositeType {
public:
    friend class TypeStore;

    StructType(MutableTypeVector* fields,
               bool isFinal,
               CompositeType** subTypeList)
        : CompositeType(ObjectType::StructKind, isFinal, subTypeList)
        , m_fieldTypes(fields)
    {
    }

    ~StructType()
    {
        delete m_fieldTypes;
    }

    const MutableTypeVector& fields() const { return *m_fieldTypes; }

private:
    MutableTypeVector* m_fieldTypes;
};

class ArrayType : public CompositeType {
public:
    friend class TypeStore;

    ArrayType(MutableType field,
              bool isFinal,
              CompositeType** subTypeList)
        : CompositeType(ObjectType::ArrayKind, isFinal, subTypeList)
        , m_field(field)
    {
    }

    const MutableType& field() const { return m_field; }

private:
    MutableType m_field;
};

class GlobalType : public ObjectType {
public:
    GlobalType(const MutableType& type);
    ~GlobalType();

    MutableType type() const { return m_type; }
    bool isMutable() const { return m_type.isMutable(); }
    ModuleFunction* function() const { return m_function; }

    inline void setFunction(ModuleFunction* func)
    {
        ASSERT(!m_function);
        m_function = func;
    }

private:
    MutableType m_type;
    ModuleFunction* m_function;
};

class TableType : public ObjectType {
public:
    TableType(Type type, uint32_t initSize, uint32_t maxSize)
        : ObjectType(ObjectType::TableKind)
        , m_type(type)
        , m_initialSize(initSize)
        , m_maximumSize(maxSize)
        , m_function(nullptr)
    {
    }

    Type type() const { return m_type; }
    uint32_t initialSize() const { return m_initialSize; }
    uint32_t maximumSize() const { return m_maximumSize; }
    ModuleFunction* function() const { return m_function; }

    inline void setFunction(ModuleFunction* func)
    {
        ASSERT(!m_function);
        m_function = func;
    }

private:
    Type m_type;
    uint32_t m_initialSize;
    uint32_t m_maximumSize;
    ModuleFunction* m_function;
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
typedef VectorWithFixedSize<CompositeType*, std::allocator<CompositeType*>> CompositeTypeVector;
typedef VectorWithFixedSize<GlobalType*, std::allocator<GlobalType*>> GlobalTypeVector;
typedef VectorWithFixedSize<TableType*, std::allocator<TableType*>> TableTypeVector;
typedef VectorWithFixedSize<MemoryType*, std::allocator<MemoryType*>> MemoryTypeVector;
typedef VectorWithFixedSize<TagType*, std::allocator<TagType*>> TagTypeVector;

} // namespace Walrus

#endif // __WalrusObjectType__
