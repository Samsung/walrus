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

namespace Walrus {

class Function;

class V128 {
public:
    uint8_t m_data[16];
};

class Value {
public:
    // https://webassembly.github.io/spec/core/syntax/types.html
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
        : m_i32(0)
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

    Value(Type type, const uint8_t* const memory)
        : m_type(type)
    {
        switch (m_type) {
        case I32:
            m_i32 = *reinterpret_cast<const int32_t* const>(memory);
            break;
        case F32:
            m_f32 = *reinterpret_cast<const float* const>(memory);
            break;
        case F64:
            m_f64 = *reinterpret_cast<const double* const>(memory);
            break;
        case I64:
            m_i64 = *reinterpret_cast<const int64_t* const>(memory);
            break;
        case FuncRef:
            m_ref = const_cast<Function*>(reinterpret_cast<const Function* const>(memory));
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

    double asF64() const
    {
        ASSERT(type() == F64);
        return m_f64;
    }

    Function* asFunction() const
    {
        ASSERT(type() == FuncRef);
        return reinterpret_cast<Function*>(m_ref);
    }

    void writeToStack(uint8_t* ptr)
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
        case FuncRef: {
            *reinterpret_cast<void**>(ptr) = m_ref;
            break;
        }
        default: {
            ASSERT_NOT_REACHED();
            break;
        }
        }
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

inline size_t valueSizeInStack(Value::Type type)
{
    switch (type) {
    case Value::I32:
    case Value::F32:
        return stackAllocatedSize<int32_t>();
    case Value::I64:
    case Value::F64:
        return 8;
    case Value::V128:
        return 16;
    default:
        return sizeof(size_t);
    }
}

typedef Vector<Value, GCUtil::gc_malloc_allocator<Value>> ValueVector;

} // namespace Walrus

#endif // __WalrusValue__
