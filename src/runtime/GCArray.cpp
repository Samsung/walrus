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
#include "Walrus.h"

#include "GCArray.h"
#include "runtime/Instance.h"
#include "runtime/GCStruct.h"
#include "runtime/TypeStore.h"

#ifdef ENABLE_GC
#include "GCUtil.h"
#endif /* ENABLE_GC */

namespace Walrus {

#ifdef ENABLE_GC
static void GC_CALLBACK arrayFinalizer(void* /* ignored ptr */, void* subTypeList)
{
    TypeStore::ReleaseRef(reinterpret_cast<const CompositeType**>(subTypeList));
}

static inline uint32_t getAlignedStartOffset(uint32_t size)
{
    return (static_cast<uint32_t>(sizeof(GCArray)) + (size - 1)) & ~(size - 1);
}

static inline uint32_t getAlignedTotalSize(uint32_t size)
{
    return (size + static_cast<uint32_t>(sizeof(void*) - 1)) & ~static_cast<uint32_t>(sizeof(void*) - 1);
}
#endif // ENABLE_GC

GCArray* GCArray::arrayNew(uint32_t length, const ArrayType* type, uint8_t* value)
{
#ifdef ENABLE_GC
    Value::Type valueType = type->field().type();
    uint32_t log2Size = getLog2Size(valueType);
    uint32_t size = 1 << log2Size;

    uint32_t startOffset = getAlignedStartOffset(size);

    if (UNLIKELY(length >= (std::numeric_limits<uint32_t>::max() - startOffset) >> log2Size)) {
        // Array is larger than 4GB.
        return nullptr;
    }

    uint32_t totalSize = getAlignedTotalSize(startOffset + (length << log2Size));
    GCArray* result = reinterpret_cast<GCArray*>(Value::isRefType(valueType) ? GC_MALLOC(totalSize) : GC_MALLOC_ATOMIC(totalSize));
    if (UNLIKELY(result == nullptr)) {
        return result;
    }

    // Placement new to initialize the common part.
    new (result) GCArray(type, length);

    if (length == 0) {
        return result;
    }

    uint8_t* dst = reinterpret_cast<uint8_t*>(result) + startOffset;
    GCStruct::set(dst, value, valueType);

    uint32_t currentSize = size;
    size = length << log2Size;

    while (size - currentSize >= currentSize) {
        // Duplicate the items with memcpy
        memcpy(dst + currentSize, dst, currentSize);
        currentSize += currentSize;
    }

    if (currentSize < size) {
        memcpy(dst + currentSize, dst, size - currentSize);
    }

    TypeStore::AddRef(type);
    GC_REGISTER_FINALIZER_NO_ORDER(result, arrayFinalizer, type->subTypeList(), nullptr, nullptr);
    return result;
#else // !ENABLE_GC
    return nullptr;
#endif // ENABLE_GC
}

GCArray* GCArray::arrayNewDefault(uint32_t length, const ArrayType* type)
{
#ifdef ENABLE_GC
    Value::Type valueType = type->field().type();
    uint32_t log2Size = getLog2Size(valueType);
    uint32_t size = 1 << log2Size;

    uint32_t startOffset = getAlignedStartOffset(size);

    if (UNLIKELY(length >= (std::numeric_limits<uint32_t>::max() - startOffset) >> log2Size)) {
        // Array is larger than 4GB.
        return nullptr;
    }

    uint32_t totalSize = getAlignedTotalSize(startOffset + (length << log2Size));
    GCArray* result = reinterpret_cast<GCArray*>(Value::isRefType(valueType) ? GC_MALLOC(totalSize) : GC_MALLOC_ATOMIC(totalSize));
    if (UNLIKELY(result == nullptr)) {
        return result;
    }

    // Placement new to initialize the common part.
    new (result) GCArray(type, length);

    memset(reinterpret_cast<uint8_t*>(result) + startOffset, 0, length << log2Size);

    TypeStore::AddRef(type);
    GC_REGISTER_FINALIZER_NO_ORDER(result, arrayFinalizer, type->subTypeList(), nullptr, nullptr);
    return result;
#else // !ENABLE_GC
    return nullptr;
#endif // ENABLE_GC
}

GCArray* GCArray::arrayNewFixed(uint32_t length, const ArrayType* type, ByteCodeStackOffset* offsets, uint8_t* bp)
{
#ifdef ENABLE_GC
    Value::Type valueType = type->field().type();
    uint32_t log2Size = getLog2Size(valueType);
    uint32_t size = 1 << log2Size;

    uint32_t startOffset = getAlignedStartOffset(size);

    // Currently this case is not possible.
    if (UNLIKELY(length >= (std::numeric_limits<uint32_t>::max() - startOffset) >> log2Size)) {
        // Array is larger than 4GB.
        return nullptr;
    }

    uint32_t totalSize = getAlignedTotalSize(startOffset + (length << log2Size));
    GCArray* result = reinterpret_cast<GCArray*>(Value::isRefType(valueType) ? GC_MALLOC(totalSize) : GC_MALLOC_ATOMIC(totalSize));
    if (UNLIKELY(result == nullptr)) {
        return result;
    }

    // Placement new to initialize the common part.
    new (result) GCArray(type, length);

    uint8_t* dst = reinterpret_cast<uint8_t*>(result) + startOffset;

    switch (valueType) {
    case Value::I32:
    case Value::F32:
        for (uint32_t i = 0; i < length; i++) {
            reinterpret_cast<uint32_t*>(dst)[i] = *reinterpret_cast<uint32_t*>(bp + offsets[i]);
        }
        break;
    case Value::I64:
    case Value::F64:
        for (uint32_t i = 0; i < length; i++) {
            reinterpret_cast<uint64_t*>(dst)[i] = *reinterpret_cast<uint64_t*>(bp + offsets[i]);
        }
        break;
    case Value::V128:
        for (uint32_t i = 0; i < length; i++) {
            memcpy(dst + (i << 4), bp + offsets[i], 16);
        }
        break;
    case Value::I8:
        for (uint32_t i = 0; i < length; i++) {
            reinterpret_cast<uint8_t*>(dst)[i] = *reinterpret_cast<uint32_t*>(bp + offsets[i]);
        }
        break;
    case Value::I16:
        for (uint32_t i = 0; i < length; i++) {
            reinterpret_cast<uint16_t*>(dst)[i] = *reinterpret_cast<uint32_t*>(bp + offsets[i]);
        }
        break;
    default:
        ASSERT(Value::isRefType(valueType));
        for (uint32_t i = 0; i < length; i++) {
            reinterpret_cast<void**>(dst)[i] = *reinterpret_cast<void**>(bp + offsets[i]);
        }
        break;
    }

    TypeStore::AddRef(type);
    GC_REGISTER_FINALIZER_NO_ORDER(result, arrayFinalizer, type->subTypeList(), nullptr, nullptr);
    return result;
#else // !ENABLE_GC
    return nullptr;
#endif // ENABLE_GC
}

GCArray* GCArray::arrayNewData(uint32_t offset, uint32_t length, const ArrayType* type, DataSegment* data)
{
    Value::Type valueType = type->field().type();
    uint32_t log2Size = getLog2Size(valueType);
    size_t size = data->sizeInByte() >> log2Size;

    ASSERT(!Value::isRefType(valueType));

    if (size < offset || (size - offset) < length) {
        return reinterpret_cast<GCArray*>(OutOfBoundsAccess);
    }

#ifdef ENABLE_GC
    size = 1 << log2Size;

    uint32_t startOffset = getAlignedStartOffset(size);

    if (UNLIKELY(length >= (std::numeric_limits<uint32_t>::max() - startOffset) >> log2Size)) {
        // Array is larger than 4GB.
        return nullptr;
    }

    uint32_t totalSize = getAlignedTotalSize(startOffset + (length << log2Size));
    GCArray* result = reinterpret_cast<GCArray*>(GC_MALLOC_ATOMIC(totalSize));
    if (UNLIKELY(result == nullptr)) {
        return result;
    }

    // Placement new to initialize the common part.
    new (result) GCArray(type, length);

    memcpy(reinterpret_cast<uint8_t*>(result) + startOffset, data->data()->initData().data() + (offset << log2Size), length << log2Size);

    TypeStore::AddRef(type);
    GC_REGISTER_FINALIZER_NO_ORDER(result, arrayFinalizer, type->subTypeList(), nullptr, nullptr);
    return result;
#else // !ENABLE_GC
    return nullptr;
#endif // ENABLE_GC
}

GCArray* GCArray::arrayNewElem(uint32_t offset, uint32_t length, const ArrayType* type, ElementSegment* elem)
{
    ASSERT(Value::isRefType(type->field().type()));

    const VectorWithFixedSize<void*, std::allocator<void*>>& elements = elem->elements();
    if (elements.size() < offset || (elements.size() - offset) < length) {
        return reinterpret_cast<GCArray*>(OutOfBoundsAccess);
    }

#ifdef ENABLE_GC
    size_t size = sizeof(void*);

    uint32_t startOffset = getAlignedStartOffset(size);

    if (UNLIKELY(length >= (std::numeric_limits<uint32_t>::max() - startOffset) / size)) {
        // Array is larger than 4GB.
        return nullptr;
    }

    uint32_t totalSize = getAlignedTotalSize(startOffset + (length * size));
    GCArray* result = reinterpret_cast<GCArray*>(GC_MALLOC(totalSize));
    if (UNLIKELY(result == nullptr)) {
        return result;
    }

    // Placement new to initialize the common part.
    new (result) GCArray(type, length);

    memcpy(reinterpret_cast<uint8_t*>(result) + startOffset, elements.data() + offset, length * size);

    TypeStore::AddRef(type);
    GC_REGISTER_FINALIZER_NO_ORDER(result, arrayFinalizer, type->subTypeList(), nullptr, nullptr);
    return result;
#else // !ENABLE_GC
    return nullptr;
#endif // ENABLE_GC
}

#ifdef ENABLE_GC
void GCArray::addRef()
{
    if (++m_refCount == 1) {
        objectTypeInfo()->getRecursiveType()->typeStore()->insertRootRef(this);
    }
}

void GCArray::releaseRef()
{
    if (--m_refCount == 0) {
        objectTypeInfo()->getRecursiveType()->typeStore()->deleteRootRef(this);
    }
}
#endif // ENABLE_GC

} // namespace Walrus
