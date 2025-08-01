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

#include "parser/WASMParser.h"
#include "runtime/TypeStore.h"
#include "runtime/Module.h"
#include "runtime/ObjectType.h"

namespace Walrus {

static size_t computeHash(size_t hash, size_t value)
{
    // Shift-Add-XOR hash
    return hash ^ ((hash << 5) + (hash >> 2) + value);
}

static size_t computeHash(size_t hash, const Type& type, size_t recStart, const Vector<CompositeType*>& types)
{
    hash = computeHash(hash, static_cast<size_t>(type.type()));
    if (type == Value::DefinedRef || type == Value::NullDefinedRef) {
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

static size_t computeHash(size_t hash, CompositeType* type, size_t recStart, const Vector<CompositeType*>& types)
{
    hash = computeHash(hash, static_cast<size_t>(type->kind()));
    hash = computeHash(hash, static_cast<size_t>(type->isFinal()));

    if (type->subType() != reinterpret_cast<CompositeType*>(TypeStore::NoIndex)) {
        uintptr_t index = reinterpret_cast<uintptr_t>(type->subType());
        if (index >= recStart) {
            index = recStart - index - 1;
        } else {
            index = reinterpret_cast<uintptr_t>(types[index]);
        }
        hash = computeHash(hash, static_cast<size_t>(index));
    }

    if (type->kind() == ObjectType::FunctionKind) {
        FunctionType* funcType = static_cast<FunctionType*>(type);

        hash = computeHash(hash, funcType->param().size());
        for (auto it : funcType->param()) {
            hash = computeHash(hash, it, recStart, types);
        }

        hash = computeHash(hash, funcType->result().size());
        for (auto it : funcType->result()) {
            hash = computeHash(hash, it, recStart, types);
        }
        return hash;
    }

    if (type->kind() == ObjectType::StructKind) {
        StructType* structType = static_cast<StructType*>(type);

        hash = computeHash(hash, structType->fields().size());
        for (auto it : structType->fields()) {
            hash = computeHash(hash, it, recStart, types);
            hash = computeHash(hash, static_cast<size_t>(it.isMutable()));
        }

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

static bool compareType(const Type& type1, const Type& type2, const Vector<CompositeType*>& types)
{
    if (type1.type() != type2.type()) {
        return false;
    }

    if (type1 != Value::DefinedRef && type1 != Value::NullDefinedRef) {
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

        size_t size = funcType1->param().size();
        for (size_t i = 0; i < size; i++) {
            if (!compareType(funcType1->param()[i], funcType2->param()[i], types)) {
                return false;
            }
        }

        size = funcType1->result().size();
        for (size_t i = 0; i < size; i++) {
            if (!compareType(funcType1->result()[i], funcType2->result()[i], types)) {
                return false;
            }
        }
        return true;
    }

    if (type1->kind() == ObjectType::StructKind) {
        StructType* structType1 = type1->asStruct();
        StructType* structType2 = type2->asStruct();

        if (structType1->fields().size() != structType2->fields().size()) {
            return false;
        }

        size_t size = structType1->fields().size();
        for (size_t i = 0; i < size; i++) {
            if (structType1->fields()[i].isMutable() != structType2->fields()[i].isMutable() || !compareType(structType1->fields()[i], structType2->fields()[i], types)) {
                return false;
            }
        }
        return true;
    }

    ASSERT(type1->kind() == ObjectType::ArrayKind);
    ArrayType* arrayType1 = type1->asArray();
    ArrayType* arrayType2 = type2->asArray();

    return arrayType1->field().isMutable() == arrayType2->field().isMutable() && compareType(arrayType1->field(), arrayType2->field(), types);
}

static void updateRef(Type& type, const Vector<CompositeType*>& types)
{
    if (type == Value::DefinedRef || type == Value::NullDefinedRef) {
        type = Type(type.type(), types[reinterpret_cast<uintptr_t>(type.ref())]);
    }
}

void TypeStore::updateRefs(CompositeType* type, const Vector<CompositeType*>& types)
{
    if (type->kind() == ObjectType::FunctionKind) {
        FunctionType* funcType = type->asFunction();

        size_t size = funcType->param().size();
        for (size_t i = 0; i < size; i++) {
            updateRef((*funcType->m_paramTypes)[i], types);
        }

        size = funcType->result().size();
        for (size_t i = 0; i < size; i++) {
            updateRef((*funcType->m_resultTypes)[i], types);
        }
        return;
    }

    if (type->kind() == ObjectType::StructKind) {
        StructType* structType = static_cast<StructType*>(type);

        size_t size = structType->fields().size();
        for (size_t i = 0; i < size; i++) {
            updateRef((*structType->m_fieldTypes)[i], types);
        }
        return;
    }

    ASSERT(type->kind() == ObjectType::ArrayKind);
    ArrayType* arrayType = static_cast<ArrayType*>(type);
    updateRef(arrayType->m_field, types);
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

        RecursiveType* recType = new RecursiveType(m_first, firstType, typeCount, hashCode);
        if (m_first != nullptr) {
            m_first->m_prev = recType;
        }
        m_first = recType;
        copyTypes(types, i, firstType);

        compType = firstType;
        do {
            updateRefs(compType, types);
            compType->m_recursiveType = recType;
            compType = compType->getNextType();
        } while (compType != nullptr);
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
    delete recType;
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

} // namespace Walrus
