/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#include "runtime/TypeStore.h"
#include "runtime/GCArray.h"
#include "runtime/GCStruct.h"
#include "runtime/Module.h"
#include "runtime/ObjectType.h"
#include "parser/WASMParser.h"

#ifdef ENABLE_GC
#include "GCUtil.h"
#endif /* ENABLE_GC */

namespace Walrus {

RecursiveType* RecursiveType::create(TypeStore* typeStore, RecursiveType* next, CompositeType* firstType,
                                     size_t typeCount, size_t hashCode, size_t totalSubTypeSize)
{
    RecursiveType* result = reinterpret_cast<RecursiveType*>(malloc(sizeof(RecursiveType) + (totalSubTypeSize - 1) * sizeof(CompositeType*)));
    new (result) RecursiveType(typeStore, next, firstType, typeCount, hashCode);
    return result;
}

void RecursiveType::destroy(RecursiveType* type)
{
    type->~RecursiveType();
    free(type);
}

static Type mutableTypeToType(MutableType& type)
{
    return Type(type.type(), type.ref());
}

static size_t computeHash(size_t hash, size_t value)
{
    // Shift-Add-XOR hash
    return hash ^ ((hash << 5) + (hash >> 2) + value);
}

static size_t computeHash(size_t hash, const Type& type, size_t recStart, const Vector<CompositeType*>& types)
{
    hash = computeHash(hash, static_cast<size_t>(type.type()));
    if (type.type() == Value::DefinedRef || type.type() == Value::NullDefinedRef) {
        uintptr_t index = reinterpret_cast<uintptr_t>(type.ref());
        if (index >= recStart) {
            index = recStart - index - 1;
        } else {
            index = reinterpret_cast<uintptr_t>(types[index]);
        }
        hash = computeHash(hash, static_cast<size_t>(index));
    }
    return hash;
}

static size_t computeHash(size_t hash, const TypeVector::Refs& refs, size_t recStart, const Vector<CompositeType*>& types)
{
    for (auto it : refs) {
        uintptr_t index = reinterpret_cast<uintptr_t>(it);
        if (index >= recStart) {
            index = recStart - index - 1;
        } else {
            index = reinterpret_cast<uintptr_t>(types[index]);
        }
        hash = computeHash(hash, static_cast<size_t>(index));
    }
    return hash;
}

static size_t computeHash(size_t hash, CompositeType* type, size_t recStart, const Vector<CompositeType*>& types)
{
    hash = computeHash(hash, static_cast<size_t>(type->kind()));
    hash = computeHash(hash, static_cast<size_t>(type->isFinal()));

    if (type->subTypeList() != reinterpret_cast<CompositeType**>(TypeStore::NoIndex)) {
        uintptr_t index = reinterpret_cast<uintptr_t>(type->subTypeList());
        if (index >= recStart) {
            index = recStart - index - 1;
        } else {
            index = reinterpret_cast<uintptr_t>(types[index]);
        }
        hash = computeHash(hash, static_cast<size_t>(index));
    }

    if (type->kind() == ObjectType::FunctionKind) {
        FunctionType* funcType = static_cast<FunctionType*>(type);

        for (auto it : funcType->param().types()) {
            hash = computeHash(hash, static_cast<size_t>(it));
        }
        hash = computeHash(hash, funcType->param().refs(), recStart, types);

        for (auto it : funcType->result().types()) {
            hash = computeHash(hash, static_cast<size_t>(it));
        }
        hash = computeHash(hash, funcType->result().refs(), recStart, types);
        return hash;
    }

    if (type->kind() == ObjectType::StructKind) {
        StructType* structType = static_cast<StructType*>(type);

        hash = computeHash(hash, structType->fields().size());
        for (auto it : structType->fields().types()) {
            hash = computeHash(hash, it.rawValue());
        }
        hash = computeHash(hash, structType->fields().refs(), recStart, types);
        return hash;
    }

    ASSERT(type->kind() == ObjectType::ArrayKind);
    ArrayType* arrayType = static_cast<ArrayType*>(type);
    hash = computeHash(hash, arrayType->field(), recStart, types);
    hash = computeHash(hash, static_cast<size_t>(arrayType->field().isMutable()));
    return hash;
}

static void copyTypes(Vector<CompositeType*>& types, size_t idx, ObjectType* type)
{
    do {
        types[idx++] = static_cast<CompositeType*>(type);
        type = static_cast<CompositeType*>(type)->getNextType();
    } while (type != nullptr);
}

static bool compareRefs(const TypeVector::Refs& refs1, const TypeVector::Refs& refs2, const Vector<CompositeType*>& types)
{
    // The type vectors were compared before, so types for all references must match.
    size_t size = refs1.size();
    ASSERT(refs2.size() == size);

    for (size_t i = 0; i < size; i++) {
        if (types[reinterpret_cast<uintptr_t>(refs1[i])] != refs2[i]) {
            return false;
        }
    }
    return true;
}

static bool compareType(const Type& type1, const Type& type2, const Vector<CompositeType*>& types)
{
    if (type1.type() != type2.type()) {
        return false;
    }

    if (type1.type() != Value::DefinedRef && type1.type() != Value::NullDefinedRef) {
        return true;
    }

    return types[reinterpret_cast<uintptr_t>(type1.ref())] == type2.ref();
}

static bool compareTypes(CompositeType* type1, size_t index2, const Vector<CompositeType*>& types)
{
    CompositeType* type2 = types[index2];

    if (type1->kind() != type2->kind()) {
        return false;
    }

    if (type1->kind() == ObjectType::FunctionKind) {
        FunctionType* funcType1 = type1->asFunction();
        FunctionType* funcType2 = type2->asFunction();

        if (funcType1->param().size() != funcType2->param().size()
            || funcType1->result().size() != funcType2->result().size()) {
            return false;
        }

        size_t paramSize = funcType1->param().size();
        size_t resultSize = funcType1->result().size();

        return paramSize == funcType2->param().size()
            && resultSize == funcType2->result().size()
            && memcmp(funcType1->param().types().data(), funcType2->param().types().data(), paramSize * sizeof(Value::Type)) == 0
            && memcmp(funcType1->result().types().data(), funcType2->result().types().data(), resultSize * sizeof(Value::Type)) == 0
            && compareRefs(funcType1->param().refs(), funcType2->param().refs(), types)
            && compareRefs(funcType1->result().refs(), funcType2->result().refs(), types);
    }

    if (type1->kind() == ObjectType::StructKind) {
        StructType* structType1 = type1->asStruct();
        StructType* structType2 = type2->asStruct();

        size_t fieldsSize = structType1->fields().size();

        return fieldsSize == structType2->fields().size()
            // Compares both type and isMutable.
            && memcmp(structType1->fields().types().data(), structType2->fields().types().data(), fieldsSize * sizeof(MutableTypeVector::TypeData)) == 0
            && compareRefs(structType1->fields().refs(), structType2->fields().refs(), types);
    }

    ASSERT(type1->kind() == ObjectType::ArrayKind);
    ArrayType* arrayType1 = type1->asArray();
    ArrayType* arrayType2 = type2->asArray();
    return arrayType1->field().isMutable() == arrayType2->field().isMutable()
        && compareType(arrayType1->field(), arrayType2->field(), types);
}

static bool updateRefVector(TypeVector::Refs& refs, const Vector<CompositeType*>& types)
{
    // The type vectors were compared before, so types for all references must match.
    size_t size = refs.size();

    for (size_t i = 0; i < size; i++) {
        refs[i] = types[reinterpret_cast<uintptr_t>(refs[i])];
    }
    return true;
}

static void updateRef(Type& type, const Vector<CompositeType*>& types)
{
    if (type.type() == Value::DefinedRef || type.type() == Value::NullDefinedRef) {
        type = Type(type.type(), types[reinterpret_cast<uintptr_t>(type.ref())]);
    }
}

const CompositeType** TypeStore::updateRefs(CompositeType* type, const Vector<CompositeType*>& types, const CompositeType** nextSubType)
{
    uintptr_t index = reinterpret_cast<uintptr_t>(type->subTypeList());
    type->m_subTypeList = nextSubType + 1;

    switch (type->kind()) {
    case ObjectType::StructKind:
        nextSubType[0] = reinterpret_cast<CompositeType*>(Object::StructKind);
        break;
    case ObjectType::ArrayKind:
        nextSubType[0] = reinterpret_cast<CompositeType*>(Object::ArrayKind);
        break;
    default:
        ASSERT(type->kind() == ObjectType::FunctionKind);
        nextSubType[0] = reinterpret_cast<CompositeType*>(Object::FunctionKind);
        break;
    }

    if (index == TypeStore::NoIndex) {
        nextSubType[1] = reinterpret_cast<CompositeType*>(1);
        nextSubType[2] = type;
        nextSubType += 3;
    } else {
        // Subtype index is always less than type index.
        uintptr_t size = types[index]->subTypeCount();
        nextSubType[1] = reinterpret_cast<CompositeType*>(size + 1);
        memcpy(nextSubType + 2, types[index]->subTypeList() + 1, sizeof(CompositeType*) * size);
        nextSubType[size + 2] = type;
        nextSubType += size + 3;
    }

    if (type->kind() == ObjectType::FunctionKind) {
        FunctionType* funcType = type->asFunction();

        updateRefVector(const_cast<TypeVector::Refs&>(funcType->param().refs()), types);
        updateRefVector(const_cast<TypeVector::Refs&>(funcType->result().refs()), types);
        return nextSubType;
    }

    if (type->kind() == ObjectType::StructKind) {
        StructType* structType = static_cast<StructType*>(type);

        updateRefVector(const_cast<TypeVector::Refs&>(structType->fields().refs()), types);
        return nextSubType;
    }

    ASSERT(type->kind() == ObjectType::ArrayKind);
    ArrayType* arrayType = static_cast<ArrayType*>(type);
    updateRef(arrayType->m_field, types);
    return nextSubType;
}

void TypeStore::updateTypes(Vector<CompositeType*>& types)
{
    // Iterate through each recursive types
    size_t size = types.size();
    size_t typeCount = 0;

    for (size_t i = 0; i < size; i += typeCount) {
        // At this point, a RecursiveType is not appended at the end
        // of the recursive type list. Instead the list ends with a nullptr.
        CompositeType* firstType = types[i];

        // Compute hash
        size_t hashCode = 0;
        CompositeType* compType = firstType;
        typeCount = 0;

        do {
            hashCode = computeHash(hashCode, compType, i, types);
            typeCount++;
            compType = compType->getNextType();
        } while (compType != nullptr);

        RecursiveType* current = m_first;

        while (current != nullptr) {
            if (current->m_typeCount == typeCount && current->m_hashCode == hashCode) {
                copyTypes(types, i, current->m_firstType);

                compType = firstType;
                size_t j = i;

                do {
                    if (!compareTypes(compType, j, types)) {
                        break;
                    }
                    compType = compType->getNextType();
                    j++;
                } while (compType != nullptr);

                if (compType == nullptr) {
                    break;
                }
            }
            current = current->m_next;
        }

        if (current != nullptr) {
            CompositeType* compType = firstType;
            do {
                CompositeType* next = compType->getNextType();
                delete compType;
                compType = next;
            } while (compType != nullptr);

            ASSERT(current->m_refCount > 0);
            current->m_refCount++;
            continue;
        }

        size_t totalSize = 0;
        compType = firstType;
        do {
            uintptr_t index = reinterpret_cast<uintptr_t>(compType->subTypeList());
            totalSize += 3;

            while (index != TypeStore::NoIndex) {
                if (index >= i) {
                    totalSize++;
                    index = reinterpret_cast<uintptr_t>(types[index]->subTypeList());
                } else {
                    totalSize += types[index]->subTypeCount();
                    break;
                }
            }
            compType = compType->getNextType();
        } while (compType != nullptr);

        RecursiveType* recType = RecursiveType::create(this, m_first, firstType, typeCount, hashCode, totalSize);

        if (m_first != nullptr) {
            m_first->m_prev = recType;
        }
        m_first = recType;
        copyTypes(types, i, firstType);

        compType = firstType;
        const CompositeType** nextSubType = recType->m_subTypes;
        do {
            nextSubType = updateRefs(compType, types, nextSubType);
            compType->m_recursiveType = recType;
            compType = compType->getNextType();
        } while (compType != nullptr);

        ASSERT(nextSubType == recType->m_subTypes + totalSize);
    }
}

void TypeStore::releaseRecursiveType(RecursiveType* recType)
{
    recType->m_refCount--;
    if (recType->m_refCount != 0) {
        return;
    }

    if (recType->m_prev == nullptr) {
        m_first = recType->m_next;
    } else {
        recType->m_prev->m_next = recType->m_next;
    }

    if (recType->m_next != nullptr) {
        recType->m_next->m_prev = recType->m_prev;
    }

    CompositeType* current = recType->m_firstType;
    ASSERT(current != nullptr);

    do {
        CompositeType* next = current->getNextType();
        delete current;
        current = next;
    } while (current != nullptr);

    RecursiveType::destroy(recType);
}

void TypeStore::releaseTypes(Vector<CompositeType*>& types)
{
    size_t size = types.size();
    for (size_t i = 0; i < size; i++) {
        if (types[i]->getNextType() == nullptr) {
            releaseRecursiveType(types[i]->getRecursiveType());
        }
    }
}

void TypeStore::releaseTypes(CompositeTypeVector& types)
{
    size_t size = types.size();
    for (size_t i = 0; i < size; i++) {
        if (types[i]->getNextType() == nullptr) {
            releaseRecursiveType(types[i]->getRecursiveType());
        }
    }
}

void TypeStore::ReleaseRef(const CompositeType** typeInfo)
{
    size_t index = reinterpret_cast<size_t>(typeInfo[0]);
    ASSERT(index > 0);
    RecursiveType* recType = typeInfo[index]->getRecursiveType();
    if (--recType->m_refCount == 0) {
        recType->m_typeStore->releaseRecursiveType(recType);
    }
}

#ifdef ENABLE_GC

void TypeStore::insertRootRef(GCBase* object)
{
    if (m_rootRefsFreeListHead == NoIndex) {
        m_rootRefsFreeListHead = m_rootRefsSize;
        m_rootRefsSize += RootRefsGrowthFactor;

        if (m_rootRefs == nullptr) {
            m_rootRefs = reinterpret_cast<Object**>(GC_MALLOC_UNCOLLECTABLE(m_rootRefsSize * sizeof(Object*)));
            m_refCounts = reinterpret_cast<size_t*>(GC_MALLOC_ATOMIC_UNCOLLECTABLE(m_rootRefsSize * sizeof(size_t)));
        } else {
            m_rootRefs = reinterpret_cast<Object**>(GC_REALLOC(m_rootRefs, m_rootRefsSize * sizeof(Object*)));
            m_refCounts = reinterpret_cast<size_t*>(GC_REALLOC(m_refCounts, m_rootRefsSize * sizeof(size_t)));
        }

        // Insert all entries as free references.
        for (size_t i = m_rootRefsFreeListHead; i < m_rootRefsSize - 1; i++) {
            m_rootRefs[i] = nullptr;
            m_refCounts[i] = i + 1;
        }
        m_rootRefs[m_rootRefsSize - 1] = nullptr;
        m_refCounts[m_rootRefsSize - 1] = static_cast<size_t>(NoIndex);
    }

    object->m_refIndex = m_rootRefsFreeListHead;
    size_t freeRef = m_refCounts[m_rootRefsFreeListHead];
    m_rootRefs[m_rootRefsFreeListHead] = object;
    m_refCounts[m_rootRefsFreeListHead] = 1;
    m_rootRefsFreeListHead = freeRef;
}

void TypeStore::deleteRootRef(GCBase* object)
{
    uintptr_t freeRef;

    ASSERT(m_refCounts[object->m_refIndex] == 0);
    freeRef = object->m_refIndex;

    ASSERT(m_rootRefs[freeRef] == object);
    m_rootRefs[freeRef] = nullptr;
    m_refCounts[freeRef] = m_rootRefsFreeListHead;
    m_rootRefsFreeListHead = freeRef;
}

#endif

} // namespace Walrus
