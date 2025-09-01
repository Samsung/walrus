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

#ifndef __WalrusGCStruct__
#define __WalrusGCStruct__

#include "runtime/Store.h"
#include "runtime/Object.h"
#include "runtime/Value.h"

namespace Walrus {

class GCStruct : public Object {
    friend class TypeStore;

public:
    static GCStruct* structNew(const StructType* type, ByteCodeStackOffset* offsets, uint8_t* bp);
    static GCStruct* structNewDefault(const StructType* type);

#ifdef ENABLE_GC
    void addRef();
    void releaseRef();
#endif

    static inline void set(uint8_t* dst, uint8_t* src, Value::Type type)
    {
        switch (type) {
        case Value::I32:
        case Value::F32:
            *reinterpret_cast<uint32_t*>(dst) = *reinterpret_cast<uint32_t*>(src);
            break;
        case Value::I64:
        case Value::F64:
            *reinterpret_cast<uint64_t*>(dst) = *reinterpret_cast<uint64_t*>(src);
            break;
        case Value::V128:
            memcpy(dst, src, 16);
            break;
        case Value::I8:
            *reinterpret_cast<uint8_t*>(dst) = *reinterpret_cast<uint32_t*>(src);
            break;
        case Value::I16:
            *reinterpret_cast<uint16_t*>(dst) = *reinterpret_cast<uint32_t*>(src);
            break;
        default:
            ASSERT(Value::isRefType(type));
            *reinterpret_cast<void**>(dst) = *reinterpret_cast<void**>(src);
            break;
        }
    }

    static inline void get(uint8_t* dst, uint8_t* src, Value::Type type, bool isSigned)
    {
        switch (type) {
        case Value::I32:
        case Value::F32:
            *reinterpret_cast<uint32_t*>(dst) = *reinterpret_cast<uint32_t*>(src);
            break;
        case Value::I64:
        case Value::F64:
            *reinterpret_cast<uint64_t*>(dst) = *reinterpret_cast<uint64_t*>(src);
            break;
        case Value::V128:
            memcpy(dst, src, 16);
            break;
        case Value::I8:
            if (isSigned) {
                *reinterpret_cast<int32_t*>(dst) = *reinterpret_cast<int8_t*>(src);
            } else {
                *reinterpret_cast<uint32_t*>(dst) = *reinterpret_cast<uint8_t*>(src);
            }
            break;
        case Value::I16:
            if (isSigned) {
                *reinterpret_cast<int32_t*>(dst) = *reinterpret_cast<int16_t*>(src);
            } else {
                *reinterpret_cast<uint32_t*>(dst) = *reinterpret_cast<uint16_t*>(src);
            }
            break;
        default:
            ASSERT(Value::isRefType(type));
            *reinterpret_cast<void**>(dst) = *reinterpret_cast<void**>(src);
            break;
        }
    }

private:
    GCStruct(const StructType* type)
        : Object(type->subTypeList())
#ifdef ENABLE_GC
        , m_refCount(0)
        , m_rootIndex(0)
#endif
    {
    }

#ifdef ENABLE_GC
    size_t m_refCount;
    size_t m_rootIndex;
#endif
};

} // namespace Walrus

#endif // __WalrusGCStruct__
