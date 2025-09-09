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

#include "GCStruct.h"
#include "runtime/TypeStore.h"

#ifdef ENABLE_GC
#include "GCUtil.h"
#endif /* ENABLE_GC */

namespace Walrus {

#ifdef ENABLE_GC
static void GC_CALLBACK structFinalizer(void* /* ignored ptr */, void* subTypeList)
{
    TypeStore::ReleaseRef(reinterpret_cast<const CompositeType**>(subTypeList));
}
#endif // ENABLE_GC

GCStruct* GCStruct::structNew(const StructType* type, ByteCodeStackOffset* offsets, uint8_t* bp)
{
#ifdef ENABLE_GC
    // TODO: The object is currently stored on the stack, which is good enough for testing,
    // but several GC related improvements needs to be added to the code later.
    GCStruct* result = reinterpret_cast<GCStruct*>(GC_MALLOC(type->structSize()));
    if (UNLIKELY(result == nullptr)) {
        return result;
    }

    // Placement new to initialize the common part.
    new (result) GCStruct(type);

    const MutableTypeVector& fields = type->fields();
    const VectorWithFixedSize<uint32_t, std::allocator<uint32_t>>& fieldOffsets = type->fieldOffsets();
    size_t size = fields.size();

    uint8_t* dst = reinterpret_cast<uint8_t*>(result);
    for (size_t i = 0; i < size; i++) {
        set(dst + fieldOffsets[i], bp + offsets[i], fields[i].type());
    }

    TypeStore::AddRef(type);
    GC_REGISTER_FINALIZER_NO_ORDER(result, structFinalizer, type->subTypeList(), nullptr, nullptr);
    return result;
#else // !ENABLE_GC
    return nullptr;
#endif // ENABLE_GC
}

GCStruct* GCStruct::structNewDefault(const StructType* type)
{
#ifdef ENABLE_GC
    GCStruct* result = reinterpret_cast<GCStruct*>(GC_MALLOC(type->structSize()));
    if (UNLIKELY(result == nullptr)) {
        return result;
    }

    // Placement new to initialize the common part.
    new (result) GCStruct(type);

    const VectorWithFixedSize<uint32_t, std::allocator<uint32_t>>& fieldOffsets = type->fieldOffsets();
    memset(reinterpret_cast<uint8_t*>(result) + fieldOffsets[0], 0, type->structSize() - fieldOffsets[0]);

    TypeStore::AddRef(type);
    GC_REGISTER_FINALIZER_NO_ORDER(result, structFinalizer, type->subTypeList(), nullptr, nullptr);
    return result;
#else // !ENABLE_GC
    return nullptr;
#endif // ENABLE_GC
}

void GCStruct::addRef()
{
    if (++m_refCount == 1) {
        objectTypeInfo()->getRecursiveType()->typeStore()->insertRootRef(this);
    }
}

void GCStruct::releaseRef()
{
    if (--m_refCount == 0) {
        objectTypeInfo()->getRecursiveType()->typeStore()->deleteRootRef(this);
    }
}

} // namespace Walrus
