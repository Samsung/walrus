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

#ifndef __WalrusValue__
#define __WalrusValue__

#include "util/Vector.h"
#include "runtime/ExecutionState.h"
#include "runtime/Exception.h"

namespace Walrus {

class Function;
class Table;
class Memory;
class Global;

class V128 {
public:
    uint8_t m_data[16];
};

template <typename T>
size_t stackAllocatedSize()
{
    if (sizeof(T) < sizeof(size_t) && sizeof(T) % sizeof(size_t)) {
        return sizeof(size_t);
    } else if (sizeof(T) > sizeof(size_t) && sizeof(T) % sizeof(size_t)) {
        return sizeof(size_t) * ((sizeof(T) / sizeof(size_t)) + 1);
    } else {
        return sizeof(T);
    }
}

class Value {
public:
    // https://webassembly.github.io/spec/core/syntax/types.html

    // RefNull
    static constexpr uintptr_t NullBits = ~uintptr_t(0);
    enum RefNull { Null };
    enum ForceInit { Force };

    enum Type : uint8_t {
        I32,
        I64,
        F32,
        F64,
        V128,
        FuncRef,
        ExternRef,
        Void,
    };

    Value()
        : m_ref(nullptr)
        , m_type(Void)
    {
    }

    explicit Value(const int32_t& v)
        : m_i32(v)
        , m_type(I32)
    {
    }

    explicit Value(const int64_t& v)
        : m_i64(v)
        , m_type(I64)
    {
    }

    explicit Value(const float& v)
        : m_f32(v)
        , m_type(F32)
    {
    }

    explicit Value(const double& v)
        : m_f64(v)
        , m_type(F64)
    {
    }

    explicit Value(Function* func)
        : m_ref(func)
        , m_type(FuncRef)
    {
    }

    explicit Value(void* ptr)
        : m_ref(ptr)
        , m_type(ExternRef)
    {
    }

    Value(Type type)
        : m_i64(0)
        , m_type(type)
    {
    }

    Value(Type type, RefNull)
        : m_ref(reinterpret_cast<void*>(NullBits))
        , m_type(type)
    {
    }

    Value(Type type, uintptr_t value, ForceInit)
        : m_ref(reinterpret_cast<void*>(value))
        , m_type(type)
    {
    }

    Value(Type type, const uint8_t* memory)
        : m_type(type)
    {
        switch (m_type) {
        case I32:
            m_i32 = *reinterpret_cast<const int32_t*>(memory);
            break;
        case F32:
            m_f32 = *reinterpret_cast<const float*>(memory);
            break;
        case F64:
            m_f64 = *reinterpret_cast<const double*>(memory);
            break;
        case I64:
            m_i64 = *reinterpret_cast<const int64_t*>(memory);
            break;
        case FuncRef:
        case ExternRef:
            m_ref = *reinterpret_cast<void**>(const_cast<uint8_t*>(memory));
            break;
        default:
            ASSERT_NOT_REACHED();
            break;
        }
    }

    Type type() const { return m_type; }

    int32_t asI32() const
    {
        ASSERT(type() == I32);
        return m_i32;
    }

    int64_t asI64() const
    {
        ASSERT(type() == I64);
        return m_i64;
    }

    float asF32() const
    {
        ASSERT(type() == F32);
        return m_f32;
    }

    uint32_t asF32Bits() const
    {
        ASSERT(type() == F32);
        return m_i32;
    }

    double asF64() const
    {
        ASSERT(type() == F64);
        return m_f64;
    }

    uint64_t asF64Bits() const
    {
        ASSERT(type() == F64);
        return m_i64;
    }

    Function* asFunction() const
    {
        ASSERT(type() == FuncRef);
        return reinterpret_cast<Function*>(m_ref);
    }

    Table* asTable() const
    {
        ASSERT(type() == ExternRef);
        return reinterpret_cast<Table*>(m_ref);
    }

    Memory* asMemory() const
    {
        ASSERT(type() == ExternRef);
        return reinterpret_cast<Memory*>(m_ref);
    }

    Global* asGlobal() const
    {
        ASSERT(type() == ExternRef);
        return reinterpret_cast<Global*>(m_ref);
    }

    void* asExternal() const
    {
        ASSERT(type() == ExternRef);
        return reinterpret_cast<void*>(m_ref);
    }

    inline void writeToStack(uint8_t*& ptr)
    {
        switch (m_type) {
        case I32: {
            *reinterpret_cast<int32_t*>(ptr) = m_i32;
            ptr += stackAllocatedSize<int32_t>();
            break;
        }
        case F32: {
            *reinterpret_cast<float*>(ptr) = m_f32;
            ptr += stackAllocatedSize<float>();
            break;
        }
        case F64: {
            *reinterpret_cast<double*>(ptr) = m_f64;
            ptr += stackAllocatedSize<double>();
            break;
        }
        case I64: {
            *reinterpret_cast<int64_t*>(ptr) = m_i64;
            ptr += stackAllocatedSize<int64_t>();
            break;
        }
        case FuncRef:
        case ExternRef: {
            *reinterpret_cast<void**>(ptr) = m_ref;
            ptr += stackAllocatedSize<void*>();
            break;
        }
        default: {
            ASSERT_NOT_REACHED();
            break;
        }
        }
    }

    template <const size_t siz>
    inline void writeNBytesToStack(uint8_t*& ptr)
    {
        if (siz == 4) {
            *reinterpret_cast<int32_t*>(ptr) = m_i32;
            ptr += siz;
        } else if (siz == 8) {
            *reinterpret_cast<int64_t*>(ptr) = m_i64;
            ptr += siz;
        } else {
            ASSERT_NOT_REACHED();
        }
    }

    bool isNull() const
    {
        ASSERT(m_type == ExternRef || m_type == FuncRef);
        return isNull(m_ref);
    }

    static bool isNull(void* ptr)
    {
        return ptr == reinterpret_cast<void*>(NullBits);
    }

    template <const size_t size>
    void writeToStack(uint8_t*& ptr);

    template <const size_t size>
    void readFromStack(uint8_t*& ptr);

    bool operator==(const Value& v) const
    {
        if (m_type == v.m_type) {
            switch (m_type) {
            case I32:
            case F32:
                return m_i32 == v.m_i32;
            case F64:
            case I64:
                return m_i64 == v.m_i64;
            case FuncRef:
            case ExternRef:
                return m_ref == v.m_ref;
            default:
                ASSERT_NOT_REACHED();
                break;
            }
        }
        return false;
    }

private:
    union {
        int32_t m_i32;
        int64_t m_i64;
        float m_f32;
        double m_f64;
        void* m_ref;
    };

    Type m_type;
};

inline size_t valueSizeInStack(Value::Type type)
{
    switch (type) {
    case Value::I32:
        return stackAllocatedSize<int32_t>();
    case Value::F32:
        return stackAllocatedSize<float>();
    case Value::I64:
        return stackAllocatedSize<int64_t>();
    case Value::F64:
        return stackAllocatedSize<double>();
    case Value::V128:
        return 16;
    default:
        return sizeof(size_t);
    }
}

template <const size_t size>
inline void Value::readFromStack(uint8_t*& ptr)
{
    ASSERT(valueSizeInStack(m_type) == size);
    if (size == 4) {
        ptr -= stackAllocatedSize<int32_t>();
        m_i32 = *reinterpret_cast<int32_t*>(ptr);
    } else {
        ASSERT(size == 8);
        ptr -= stackAllocatedSize<int64_t>();
        m_i64 = *reinterpret_cast<int64_t*>(ptr);
    }
}

typedef Vector<Value, GCUtil::gc_malloc_allocator<Value>> ValueVector;
typedef Vector<Value::Type, GCUtil::gc_malloc_atomic_allocator<Value::Type>> ValueTypeVector;

} // namespace Walrus

#endif // __WalrusValue__
