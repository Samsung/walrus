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

#ifndef __WalrusType__
#define __WalrusType__

#include "util/Vector.h"
#include "runtime/Value.h"

namespace Walrus {

class CompositeType;

class Type {
    friend class MutableType;

public:
    Type()
        : m_type(Value::Void)
        , m_isMutable(false)
        , m_ref(nullptr)
    {
    }

    Type(Value::Type kind)
        : m_type(kind)
        , m_isMutable(false)
        , m_ref(nullptr)
    {
        ASSERT(!isConcreteType());
    }

    Type(Value::Type kind, const CompositeType* ref)
        : m_type(kind)
        , m_isMutable(false)
        , m_ref(ref)
    {
        ASSERT(isConcreteType() || ref == nullptr);
    }

    Value::Type type() const
    {
        return m_type;
    }

    // Storage type on the WebAssembly stack.
    Value::Type stackType() const
    {
        if (isPacked()) {
            return Value::I32;
        }
        return m_type;
    }

    operator Value::Type() const
    {
        ASSERT(!isPacked());
        return type();
    }

    const CompositeType* ref() const
    {
        ASSERT(isConcreteType() || m_ref == nullptr);
        return m_ref;
    }

    bool isRef() const
    {
        return Value::isRefType(type());
    }

    bool isNullableRef() const
    {
        return Value::isNullableRefType(type());
    }

    bool isPacked() const
    {
        return Value::isPackedType(type());
    }

    bool isConcreteType() const
    {
        return type() == Value::DefinedRef || type() == Value::NullDefinedRef;
    }

    bool isSubType(const Type& expected) const;

private:
    Value::Type m_type;
    // Only used by MutableType.
    // Stored here to reduce MutableType size.
    bool m_isMutable;
    const CompositeType* m_ref;
};

class MutableType : public Type {
public:
    MutableType()
        : Type()
    {
        m_isMutable = 0;
    }

    MutableType(Value::Type kind, bool isMutable)
        : Type(kind)
    {
        m_isMutable = isMutable;
    }

    MutableType(Value::Type kind, const CompositeType* ref, bool isMutable)
        : Type(kind, ref)
    {
        m_isMutable = isMutable;
    }

    bool isMutable() const
    {
        return m_isMutable;
    }
};

class TypeVector {
    friend class WASMBinaryReader;

public:
    typedef VectorWithFixedSize<Value::Type, std::allocator<Value::Type>> Types;
    typedef VectorWithFixedSize<const CompositeType*, std::allocator<const CompositeType*>> Refs;

    TypeVector(size_t typesCount, size_t refsCount)
    {
        m_types.reserve(typesCount);
        m_refs.reserve(refsCount);
    }

    const Types& types() const
    {
        return m_types;
    }

    const Refs& refs() const
    {
        return m_refs;
    }

    size_t size() const
    {
        return types().size();
    }

    // Setters should only be used by the parser.
    void setType(size_t idx, Value::Type type)
    {
        m_types[idx] = type;
    }

    void setRef(size_t idx, const CompositeType* ref)
    {
        m_refs[idx] = ref;
    }

private:
    Types m_types;
    Refs m_refs;
};

class MutableTypeVector {
    friend class WASMBinaryReader;

public:
    class TypeData {
    public:
        TypeData(Value::Type kind, bool isMutable)
            : m_type(static_cast<uint8_t>((kind << TypeShift) | (isMutable ? IsMutable : 0)))
        {
            ASSERT(type() == kind);
        }

        Value::Type type() const
        {
            return static_cast<Value::Type>(m_type >> TypeShift);
        }

        bool isMutable() const
        {
            return (m_type & IsMutable) != 0;
        }

        // Used by hashing.
        size_t rawValue()
        {
            return m_type;
        }

        // Storage type on the WebAssembly stack.
        Value::Type stackType() const
        {
            if (Value::isPackedType(type())) {
                return Value::I32;
            }
            return type();
        }

    private:
        static constexpr int TypeShift = 1;
        static constexpr uint8_t IsMutable = 0x1;

        uint8_t m_type;
    };

    typedef VectorWithFixedSize<TypeData, std::allocator<TypeData>> Types;

    MutableTypeVector(size_t typesCount, size_t refsCount)
    {
        m_types.reserve(typesCount);
        m_refs.reserve(refsCount);
    }

    const Types& types() const
    {
        return m_types;
    }

    const TypeVector::Refs& refs() const
    {
        return m_refs;
    }

    size_t size() const
    {
        return types().size();
    }

    // Setters should only be used by the parser.
    void setType(size_t idx, TypeData type)
    {
        m_types[idx] = type;
    }

    void setRef(size_t idx, const CompositeType* ref)
    {
        m_refs[idx] = ref;
    }

private:
    Types m_types;
    TypeVector::Refs m_refs;
};

} // namespace Walrus

#endif // __WalrusValue__
