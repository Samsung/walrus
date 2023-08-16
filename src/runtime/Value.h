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
#include "util/BitOperation.h"
#include "runtime/ExecutionState.h"
#include "runtime/Exception.h"

namespace Walrus {

class Object;
class Function;
class Table;
class Memory;
class Global;

struct Vec128 {
    template <typename T>
    T to(int lane) const
    {
        COMPILE_ASSERT(sizeof(T) <= sizeof(m_data), "");
        ASSERT((lane + 1) * sizeof(T) <= sizeof(m_data));
        T result;
        memcpyEndianAware(&result, m_data, sizeof(result), sizeof(m_data), 0, lane * sizeof(T), sizeof(result));
        return result;
    }

    Vec128()
        : m_data{
            0,
        }
    {
    }

    bool operator==(const Vec128& src) const
    {
        return memcmp(m_data, src.m_data, 16) == 0;
    }

    float asF32(int lane) const { return to<float>(lane); }
    uint32_t asF32Bits(int lane) const { return to<uint32_t>(lane); }
    double asF64(int lane) const { return to<double>(lane); }
    uint64_t asF64Bits(int lane) const { return to<uint64_t>(lane); }

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
    static constexpr uintptr_t NullBits = uintptr_t(0);
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
        Void, // Void type should be located at end
        NUM = Void
    };

    Value()
        : m_v128()
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

    explicit Value(const Vec128& v)
        : m_v128(v)
        , m_type(V128)
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
        case V128:
            m_v128 = *reinterpret_cast<const Vec128*>(memory);
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

    const Vec128& asV128() const
    {
        ASSERT(type() == V128);
        return m_v128;
    }

    const uint8_t* asV128Addr() const
    {
        ASSERT(type() == V128);
        return m_v128.m_data;
    }

    Function* asFunction() const
    {
        ASSERT(type() == FuncRef);
        return reinterpret_cast<Function*>(m_ref);
    }

    Object* asObject() const
    {
        ASSERT(type() == ExternRef);
        return reinterpret_cast<Object*>(m_ref);
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

    inline void writeToMemory(uint8_t* ptr)
    {
        switch (m_type) {
        case I32: {
            *reinterpret_cast<int32_t*>(ptr) = m_i32;
            break;
        }
        case F32: {
            *reinterpret_cast<float*>(ptr) = m_f32;
            break;
        }
        case F64: {
            *reinterpret_cast<double*>(ptr) = m_f64;
            break;
        }
        case I64: {
            *reinterpret_cast<int64_t*>(ptr) = m_i64;
            break;
        }
        case V128: {
            *reinterpret_cast<Vec128*>(ptr) = m_v128;
            break;
        }
        case FuncRef:
        case ExternRef: {
            *reinterpret_cast<void**>(ptr) = m_ref;
            break;
        }
        default: {
            ASSERT_NOT_REACHED();
            break;
        }
        }
    }

    template <const size_t siz>
    inline void writeNBytesToMemory(uint8_t* ptr)
    {
        if (siz == 4) {
            *reinterpret_cast<int32_t*>(ptr) = m_i32;
        } else if (siz == 8) {
            *reinterpret_cast<int64_t*>(ptr) = m_i64;
        } else {
            ASSERT(siz == 16);
            memcpy(ptr, m_v128.m_data, 16);
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
    void readFromStack(uint8_t* ptr);

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
            case V128:
                return m_v128 == v.m_v128;
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
        Vec128 m_v128;
    };

    Type m_type;
};

inline size_t valueSize(Value::Type type)
{
    switch (type) {
    case Value::I32:
        return sizeof(int32_t);
    case Value::F32:
        return sizeof(float);
    case Value::I64:
        return sizeof(int64_t);
    case Value::F64:
        return sizeof(double);
    case Value::V128:
        return 16;
    case Value::FuncRef:
    case Value::ExternRef:
        return sizeof(size_t);
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return 0;
    }
}

inline size_t valueStackAllocatedSize(Value::Type type)
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
    case Value::Void:
        return 0;
    case Value::FuncRef:
    case Value::ExternRef:
        return sizeof(size_t);
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return 0;
    }
}

template <const size_t size>
inline void Value::readFromStack(uint8_t* ptr)
{
    ASSERT(valueStackAllocatedSize(m_type) == size);
    if (size == 4) {
        m_i32 = *reinterpret_cast<int32_t*>(ptr);
    } else if (size == 8) {
        m_i64 = *reinterpret_cast<int64_t*>(ptr);
    } else {
        ASSERT(size == 16);
        memcpy(m_v128.m_data, ptr, 16);
    }
}

typedef Vector<Value, std::allocator<Value>> ValueVector;
typedef Vector<Value::Type, std::allocator<Value::Type>> ValueTypeVector;

} // namespace Walrus

#endif // __WalrusValue__
