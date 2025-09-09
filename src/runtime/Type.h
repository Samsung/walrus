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
public:
    Type()
        : m_type(static_cast<uintptr_t>(Value::Void))
        , m_ref(nullptr)
    {
    }

    Type(Value::Type kind)
        : m_type(static_cast<uintptr_t>(kind))
        , m_ref(nullptr)
    {
    }

    Type(Value::Type kind, CompositeType* ref)
        : m_type(static_cast<uintptr_t>(kind))
        , m_ref(ref)
    {
    }

    Value::Type type() const
    {
        return static_cast<Value::Type>(m_type);
    }

    // Storage type on the WebAssembly stack.
    Value::Type stackType() const
    {
        if (isPacked()) {
            return Value::I32;
        }
        return static_cast<Value::Type>(m_type);
    }

    operator Value::Type() const
    {
        ASSERT(!isPacked());
        return type();
    }

    CompositeType* ref() const
    {
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

    bool isSubType(const Type& expected) const;

private:
    // Type is uintptr_t to support memcmp comparison
    uintptr_t m_type;
    CompositeType* m_ref;
};

class MutableType : public Type {
public:
    MutableType()
        : Type()
        , m_isMutable(false)
    {
    }

    MutableType(Value::Type kind, bool isMutable)
        : Type(kind)
        , m_isMutable(isMutable)
    {
    }

    MutableType(Value::Type kind, CompositeType* ref, bool isMutable)
        : Type(kind, ref)
        , m_isMutable(isMutable)
    {
    }

    bool isMutable() const
    {
        return m_isMutable;
    }

private:
    bool m_isMutable;
};

typedef Vector<Type, std::allocator<Type>> TypeVector;
typedef Vector<MutableType, std::allocator<MutableType>> MutableTypeVector;

} // namespace Walrus

#endif // __WalrusValue__
