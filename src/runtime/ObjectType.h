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

#include "runtime/Value.h"

namespace Walrus {

class ObjectType {
public:
    enum Kind : uint8_t {
        Invalid,
        FunctionKind,
        GlobalKind,
        TableKind,
        MemoryKind,
        TagKind,
    };

    Kind kind() const { return m_kind; }

protected:
    ObjectType(Kind kind)
        : m_kind(kind)
    {
    }

    Kind m_kind;
};

class FunctionType : public ObjectType {
public:
    FunctionType(ValueTypeVector* param,
                 ValueTypeVector* result)
        : ObjectType(ObjectType::FunctionKind)
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

    FunctionType* clone() const
    {
        ValueTypeVector* param = new ValueTypeVector();
        ValueTypeVector* result = new ValueTypeVector();
        param->reserve(m_paramTypes->size());
        result->reserve(m_resultTypes->size());
        for (size_t i = 0; i < m_paramTypes->size(); i++) {
            param->push_back(m_paramTypes->at(i));
        }
        for (size_t i = 0; i < m_resultTypes->size(); i++) {
            result->push_back(m_resultTypes->at(i));
        }

        return new FunctionType(param, result);
    }

    ValueTypeVector& param() const { return *m_paramTypes; }
    ValueTypeVector& result() const { return *m_resultTypes; }
    size_t paramStackSize() const { return m_paramStackSize; }
    size_t resultStackSize() const { return m_resultStackSize; }

    bool equals(const FunctionType* other) const
    {
        if (this == other) {
            return true;
        }

        if (m_paramTypes->size() != other->param().size()) {
            return false;
        }

        if (memcmp(m_paramTypes->data(), other->param().data(), sizeof(Value::Type) * other->param().size())) {
            return false;
        }

        if (m_resultTypes->size() != other->result().size()) {
            return false;
        }

        if (memcmp(m_resultTypes->data(), other->result().data(), sizeof(Value::Type) * other->result().size())) {
            return false;
        }

        return true;
    }

private:
    ValueTypeVector* m_paramTypes;
    ValueTypeVector* m_resultTypes;
    size_t m_paramStackSize;
    size_t m_resultStackSize;

    static size_t computeStackSize(const ValueTypeVector& v)
    {
        size_t s = 0;
        for (size_t i = 0; i < v.size(); i++) {
            s += valueSizeInStack(v[i]);
        }
        return s;
    }
};

class GlobalType : public ObjectType {
public:
    GlobalType(Value::Type type, bool mut)
        : ObjectType(ObjectType::GlobalKind)
        , m_type(type)
        , m_mutable(mut)
    {
    }

    GlobalType* clone() const
    {
        return new GlobalType(m_type, m_mutable);
    }

    Value::Type type() const { return m_type; }
    bool isMutable() const { return m_mutable; }

private:
    Value::Type m_type;
    bool m_mutable;
};

class TableType : public ObjectType {
public:
    TableType(Value::Type type, uint32_t initSize, uint32_t maxSize)
        : ObjectType(ObjectType::TableKind)
        , m_type(type)
        , m_initialSize(initSize)
        , m_maximumSize(maxSize)
    {
    }

    TableType* clone() const
    {
        return new TableType(m_type, m_initialSize, m_maximumSize);
    }

    Value::Type type() const { return m_type; }
    uint32_t initialSize() const { return m_initialSize; }
    uint32_t maximumSize() const { return m_maximumSize; }

private:
    Value::Type m_type;
    uint32_t m_initialSize;
    uint32_t m_maximumSize;
};

class MemoryType : public ObjectType {
public:
    MemoryType(uint32_t initSize, uint32_t maxSize)
        : ObjectType(ObjectType::MemoryKind)
        , m_initialSize(initSize)
        , m_maximumSize(maxSize)
    {
    }

    MemoryType* clone() const
    {
        return new MemoryType(m_initialSize, m_maximumSize);
    }

    uint32_t initialSize() const { return m_initialSize; }
    uint32_t maximumSize() const { return m_maximumSize; }

private:
    uint32_t m_initialSize;
    uint32_t m_maximumSize;
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
typedef Vector<FunctionType*, std::allocator<FunctionType*>> FunctionTypeVector;
typedef Vector<TableType*, std::allocator<TableType*>> TableTypeVector;
typedef Vector<MemoryType*, std::allocator<MemoryType*>> MemoryTypeVector;
typedef Vector<TagType*, std::allocator<TagType*>> TagTypeVector;

} // namespace Walrus

#endif // __WalrusObjectType__
