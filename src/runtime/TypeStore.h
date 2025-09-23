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

#ifndef __WalrusTypeStore__
#define __WalrusTypeStore__

#include "util/Vector.h"
#include "runtime/GCBase.h"
#include "runtime/ObjectType.h"
#include "runtime/Type.h"
#include "runtime/Value.h"

namespace Walrus {

class Module;
class Object;
class TypeStore;

// This class is managed through TypeStore.
class RecursiveType {
public:
    friend class TypeStore;

    static RecursiveType* create(TypeStore* typeStore, RecursiveType* next, CompositeType* firstType,
                                 size_t typeCount, size_t hashCode, size_t totalSubTypeSize);

    bool isSingleType() const
    {
        return m_firstType->getNextType() == nullptr;
    }

    TypeStore* typeStore() const
    {
        return m_typeStore;
    }

private:
    RecursiveType(TypeStore* typeStore, RecursiveType* next, CompositeType* firstType, size_t typeCount, size_t hashCode)
        : m_typeStore(typeStore)
        , m_next(next)
        , m_prev(nullptr)
        , m_firstType(firstType)
        , m_refCount(1)
        , m_typeCount(typeCount)
        , m_hashCode(hashCode)
    {
    }

    ~RecursiveType()
    {
    }

    static void destroy(RecursiveType* type);

    TypeStore* m_typeStore;
    RecursiveType* m_next;
    RecursiveType* m_prev;
    CompositeType* m_firstType;
    size_t m_refCount;
    size_t m_typeCount;
    size_t m_hashCode;
    // Concatenation of subtype arrays used by all types
    const CompositeType* m_subTypes[1];
};

class TypeStore {
public:
    static constexpr uintptr_t NoIndex = ~static_cast<uintptr_t>(0);

    TypeStore()
        : m_first(nullptr)
#ifdef ENABLE_GC
        , m_rootRefs(nullptr)
        , m_refCounts(nullptr)
        , m_rootRefsSize(0)
        , m_rootRefsFreeListHead(NoIndex)
#endif
    {
    }

    static void ConnectTypes(Vector<CompositeType*>& types, size_t index)
    {
        types[index - 1]->m_nextType = types[index];
    }

    void updateTypes(Vector<CompositeType*>& types);
    void releaseTypes(Vector<CompositeType*>& types);
    void releaseTypes(CompositeTypeVector& types);

    static void AddRef(const CompositeType* type)
    {
        type->getRecursiveType()->m_refCount++;
    }

    static void ReleaseRef(const CompositeType** typeInfo);

#ifdef ENABLE_GC
    inline void addRef(GCBase* object)
    {
        if (object->m_refIndex != GCBase::UnassignedReference) {
            m_refCounts[object->m_refIndex]++;
        } else {
            insertRootRef(object);
            ASSERT(m_refCounts[object->m_refIndex] == 1);
        }
    }

    inline void releaseRef(GCBase* object)
    {
        ASSERT(object->m_refIndex != GCBase::UnassignedReference);
        if (--m_refCounts[object->m_refIndex] == 0) {
            deleteRootRef(object);
        }
    }
#endif

private:
#ifdef ENABLE_GC
    static constexpr uintptr_t RootRefsGrowthFactor = 32;
#endif

    static const CompositeType** updateRefs(CompositeType* type, const Vector<CompositeType*>& types, const CompositeType** nextSubType);
    void releaseRecursiveType(RecursiveType* recType);

#ifdef ENABLE_GC
    void insertRootRef(GCBase* object);
    void deleteRootRef(GCBase* object);
#endif

    RecursiveType* m_first;

#ifdef ENABLE_GC
    Object** m_rootRefs;
    size_t* m_refCounts;
    size_t m_rootRefsSize;
    size_t m_rootRefsFreeListHead;
#endif
};

} // namespace Walrus

#endif // __WalrusTypeStore__
