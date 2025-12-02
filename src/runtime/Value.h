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
#include <cmath>
#include <limits>

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

    operator std::string() const
    {
        char result[16 * 3];
        char* ptr = result;
        for (int i = 0; i < 16; i++) {
            char left = (m_data[i] & 0xf0) >> 4;
            char right = m_data[i] & 0x0f;
            ptr[0] = (left < 10) ? ('0' + left) : ('a' + (left - 10));
            ptr[1] = (right < 10) ? ('0' + right) : ('a' + (right - 10));
            ptr[2] = ':';
            ptr += 3;
        }
        ptr[-1] = '\0';
        return std::string(result);
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
    friend class JITFieldAccessor;

public:
    // https://webassembly.github.io/spec/core/syntax/types.html

    // RefNull
    static constexpr uintptr_t NullBits = uintptr_t(0x0);
    static constexpr uintptr_t RefI31 = uintptr_t(0x1);
    static constexpr uintptr_t RefI31Shift = ((sizeof(void*) * 8) - 31);
    enum RefNull { Null };
    enum ForceInit { Force };

    // Some tpyes (such as Void, GenericRef) are pseudo types, not used by WebAssembly
    enum Type : uint8_t {
        I32,
        I64,
        F32,
        F64,
        V128,
        // I8/I16 packed types are only used by structs/arrays
        I8,
        I16,
        // The any, extern, and eq groups represent the same objects in Walrus.
        // WebAssembly allows defining internal (so called host) types, and
        // these types might need special handling, but in Walrus, the internal
        // types follows the same rules as WebAssembly defined types. This
        // model simplifies operations, improves performance, and has no
        // disadvantages. For example, any.convert_extern and extern.convert_any
        // are no operations.

        // The order of references are important. They are grouped together to
        // improve type comparison speed.
        // See: isRefType / isTaggedRefType / isNullableRefType.
        ExternRef,
        NoExternRef,
        AnyRef,
        // NoAnyRef is (ref none), but this name follows the other No... naming conventions
        NoAnyRef,
        EqRef,
        I31Ref,
        StructRef,
        ArrayRef,
        FuncRef,
        NoFuncRef,
        DefinedRef,
        NullExternRef,
        NullNoExternRef,
        NullAnyRef,
        NullNoAnyRef,
        NullEqRef,
        NullI31Ref,
        NullStructRef,
        NullArrayRef,
        NullFuncRef,
        NullNoFuncRef,
        NullDefinedRef,
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
        , m_type(NullFuncRef)
    {
    }

    explicit Value(Type type, void* ptr)
        : m_ref(ptr)
        , m_type(type)
    {
    }

    Value(Type type)
        : m_v128()
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
        default:
            ASSERT(isRef());
            m_ref = *reinterpret_cast<void**>(const_cast<uint8_t*>(memory));
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
        ASSERT(type() == FuncRef || type() == NullFuncRef);
        return reinterpret_cast<Function*>(m_ref);
    }

    Object* asObject() const
    {
        ASSERT(type() == ExternRef || type() == NullExternRef);
        return reinterpret_cast<Object*>(m_ref);
    }

    Table* asTable() const
    {
        ASSERT(type() == ExternRef || type() == NullExternRef);
        return reinterpret_cast<Table*>(m_ref);
    }

    Memory* asMemory() const
    {
        ASSERT(type() == ExternRef || type() == NullExternRef);
        return reinterpret_cast<Memory*>(m_ref);
    }

    Global* asGlobal() const
    {
        ASSERT(type() == ExternRef || type() == NullExternRef);
        return reinterpret_cast<Global*>(m_ref);
    }

    void* asExternal() const
    {
        ASSERT(type() == ExternRef || type() == NullExternRef);
        return reinterpret_cast<void*>(m_ref);
    }

    void* asReference() const
    {
        ASSERT(isRef());
        return reinterpret_cast<void*>(m_ref);
    }

    inline void writeToMemory(uint8_t* ptr) const
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
        default: {
            ASSERT(isRef());
            *reinterpret_cast<void**>(ptr) = m_ref;
            break;
        }
        }
    }

    template <const size_t siz>
    inline void writeNBytesToMemory(uint8_t* ptr) const
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

    static bool isRefType(Type type)
    {
        return type >= ExternRef && type <= NullDefinedRef;
    }

    static bool isTaggedRefType(Type type)
    {
        return (type >= ExternRef && type <= I31Ref) || (type >= NullExternRef && type <= NullI31Ref);
    }

    static bool isNullableRefType(Type type)
    {
        return type >= NullExternRef && type <= NullDefinedRef;
    }

    static Type toNonNullableRefType(Type type)
    {
        if (isNullableRefType(type)) {
            return static_cast<Type>(static_cast<uint8_t>(type) - (static_cast<uint8_t>(NullAnyRef) - static_cast<uint8_t>(AnyRef)));
        }
        return type;
    }

    static bool isPackedType(Type type)
    {
        return type == I8 || type == I16;
    }

    static Value::Type unpackType(Value::Type type)
    {
        return isPackedType(type) ? Value::I32 : type;
    }

    static bool isI31Value(void* ref)
    {
        return (reinterpret_cast<uintptr_t>(ref) & RefI31) != 0;
    }

    static void* toI31Value(int32_t v)
    {
        return reinterpret_cast<void*>((static_cast<uintptr_t>(v) << RefI31Shift) | RefI31);
    }

    static int32_t getI31SValue(void* ref)
    {
        return static_cast<int32_t>(reinterpret_cast<intptr_t>(ref) >> RefI31Shift);
    }

    static int32_t getI31UValue(void* ref)
    {
        return static_cast<int32_t>(reinterpret_cast<uintptr_t>(ref) >> RefI31Shift);
    }

    bool isRef() const
    {
        return isRefType(m_type);
    }

    bool isTaggedRef() const
    {
        return isTaggedRefType(m_type);
    }

    bool isI31() const
    {
        ASSERT(isTaggedRef());
        return isI31Value(m_ref);
    }

    bool isNullableRef() const
    {
        return isNullableRefType(m_type);
    }

    bool isPacked() const
    {
        return isPackedType(m_type);
    }

    bool isNull() const
    {
        ASSERT(isNullableRef());
        return isNull(m_ref);
    }

    bool isZeroValue()
    {
        switch (m_type) {
        case I32:
            return m_i32 == 0;
        case F32:
            return m_f32 == 0.0f && !std::signbit(m_f32);
        case I64:
            return m_i64 == 0;
        case F64:
            return m_f64 == +0.0 && !std::signbit(m_f64);
        case V128: {
            for (uint8_t i = 0; i < 16; i++) {
                if (m_v128.m_data[i] != 0) {
                    return false;
                }
            }
            return true;
        }
        default:
            ASSERT(isRef());
            return m_ref == nullptr;
        }
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
            case V128:
                return m_v128 == v.m_v128;
            default:
                ASSERT(isRef());
                return m_ref == v.m_ref;
            }
        }
        return false;
    }

    operator std::string() const
    {
        switch (m_type) {
        case I32:
            return std::to_string(asI32());
        case I64:
            return std::to_string(asI64());
        case F32:
            return std::to_string(asF32());
        case F64:
            return std::to_string(asF64());
        case V128:
            return (std::string)(asV128());
        default:
            ASSERT(isRef());
            return std::to_string((size_t)(asReference()));
        }
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
    default:
        if (Value::isRefType(type)) {
            return sizeof(size_t);
        }
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
    default:
        if (Value::isRefType(type)) {
            return sizeof(size_t);
        }
        RELEASE_ASSERT_NOT_REACHED();
        return 0;
    }
}

inline size_t valueFunctionCopyCount(Value::Type type)
{
    size_t s = valueSize(type) / sizeof(size_t);
    if (s == 0) {
        return 1;
    }
    return s;
}

inline bool hasCPUWordAlignedSize(Value::Type type)
{
#if defined(WALRUS_32)
    ASSERT(valueStackAllocatedSize(type) == valueSize(type));
#endif
    return valueStackAllocatedSize(type) == valueSize(type);
}

inline bool needsCPUWordAlignedAddress(Value::Type type)
{
#if defined(WALRUS_32)
    // everything is already aligned!
    return false;
#else
    return Value::isRefType(type);
#endif
}

template <const size_t size>
inline void Value::readFromStack(uint8_t* ptr)
{
    ASSERT(valueSize(m_type) == size);
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
