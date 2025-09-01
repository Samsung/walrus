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

#ifndef __WalrusGCArray__
#define __WalrusGCArray__

#include "runtime/Store.h"
#include "runtime/Object.h"
#include "runtime/Value.h"

namespace Walrus {

class DataSegment;
class ElementSegment;

class GCArray : public Object {
    friend class JITFieldAccessor;
    friend class TypeStore;

public:
    static constexpr uintptr_t OutOfBoundsAccess = static_cast<uintptr_t>(0x1);

    static GCArray* arrayNew(uint32_t length, const ArrayType* type, uint8_t* value);
    static GCArray* arrayNewDefault(uint32_t length, const ArrayType* type);
    static GCArray* arrayNewFixed(uint32_t length, const ArrayType* type, ByteCodeStackOffset* offsets, uint8_t* bp);
    static GCArray* arrayNewData(uint32_t offset, uint32_t length, const ArrayType* type, DataSegment* data);
    static GCArray* arrayNewElem(uint32_t offset, uint32_t length, const ArrayType* type, ElementSegment* elem);

    uint32_t length() const
    {
        return m_length;
    }

#ifdef ENABLE_GC
    void addRef();
    void releaseRef();
#endif

    static inline uint32_t getLog2Size(Value::Type type)
    {
        switch (type) {
        case Value::I32:
        case Value::F32:
            return 2;
        case Value::I64:
        case Value::F64:
            return 3;
        case Value::V128:
            return 4;
        case Value::I8:
            return 0;
        case Value::I16:
            return 1;
        default:
            ASSERT(Value::isRefType(type));
            return (sizeof(void*) == 4) ? 2 : 3;
        }
    }

    static inline void set(uint8_t* dst, uint8_t* src, uint32_t pos, Value::Type type)
    {
        switch (type) {
        case Value::I32:
        case Value::F32:
            dst += (sizeof(GCArray) + sizeof(uint32_t) - 1) & ~(sizeof(uint32_t) - 1);
            reinterpret_cast<uint32_t*>(dst)[pos] = *reinterpret_cast<uint32_t*>(src);
            break;
        case Value::I64:
        case Value::F64:
            dst += (sizeof(GCArray) + sizeof(uint64_t) - 1) & ~(sizeof(uint64_t) - 1);
            reinterpret_cast<uint64_t*>(dst)[pos] = *reinterpret_cast<uint64_t*>(src);
            break;
        case Value::V128:
            dst += (sizeof(GCArray) + static_cast<size_t>(16) - 1) & ~(static_cast<size_t>(16) - 1);
            memcpy(dst + (pos << 4), src, 16);
            break;
        case Value::I8:
            dst += sizeof(GCArray);
            reinterpret_cast<uint8_t*>(dst)[pos] = *reinterpret_cast<uint32_t*>(src);
            break;
        case Value::I16:
            dst += (sizeof(GCArray) + sizeof(uint16_t) - 1) & ~(sizeof(uint16_t) - 1);
            reinterpret_cast<uint16_t*>(dst)[pos] = *reinterpret_cast<uint32_t*>(src);
            break;
        default:
            ASSERT(Value::isRefType(type));
            dst += (sizeof(GCArray) + sizeof(void*) - 1) & ~(sizeof(void*) - 1);
            reinterpret_cast<void**>(dst)[pos] = *reinterpret_cast<void**>(src);
            break;
        }
    }

    static inline void get(uint8_t* dst, uint8_t* src, uint32_t pos, Value::Type type, bool isSigned)
    {
        switch (type) {
        case Value::I32:
        case Value::F32:
            src += (sizeof(GCArray) + sizeof(uint32_t) - 1) & ~(sizeof(uint32_t) - 1);
            *reinterpret_cast<uint32_t*>(dst) = reinterpret_cast<uint32_t*>(src)[pos];
            break;
        case Value::I64:
        case Value::F64:
            src += (sizeof(GCArray) + sizeof(uint64_t) - 1) & ~(sizeof(uint64_t) - 1);
            *reinterpret_cast<uint64_t*>(dst) = reinterpret_cast<uint64_t*>(src)[pos];
            break;
        case Value::V128:
            src += (sizeof(GCArray) + static_cast<size_t>(16) - 1) & ~(static_cast<size_t>(16) - 1);
            memcpy(dst, src + (pos << 4), 16);
            break;
        case Value::I8:
            src += sizeof(GCArray);
            if (isSigned) {
                *reinterpret_cast<int32_t*>(dst) = reinterpret_cast<int8_t*>(src)[pos];
            } else {
                *reinterpret_cast<uint32_t*>(dst) = reinterpret_cast<uint8_t*>(src)[pos];
            }
            break;
        case Value::I16:
            src += (sizeof(GCArray) + sizeof(uint16_t) - 1) & ~(sizeof(uint16_t) - 1);
            if (isSigned) {
                *reinterpret_cast<int32_t*>(dst) = reinterpret_cast<int16_t*>(src)[pos];
            } else {
                *reinterpret_cast<uint32_t*>(dst) = reinterpret_cast<uint16_t*>(src)[pos];
            }
            break;
        default:
            ASSERT(Value::isRefType(type));
            src += (sizeof(GCArray) + sizeof(void*) - 1) & ~(sizeof(void*) - 1);
            *reinterpret_cast<void**>(dst) = reinterpret_cast<void**>(src)[pos];
            break;
        }
    }

private:
    GCArray(const ArrayType* type, uint32_t length)
        : Object(type->subTypeList())
        , m_length(length)
#ifdef ENABLE_GC
        , m_refCount(0)
        , m_rootIndex(0)
#endif
    {
    }

    uint32_t m_length;
#ifdef ENABLE_GC
    size_t m_refCount;
    size_t m_rootIndex;
#endif
};

} // namespace Walrus

#endif // __WalrusGCArray__
