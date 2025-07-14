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
#include "runtime/ObjectType.h"
#include "runtime/Type.h"
#include "runtime/Value.h"

namespace Walrus {

class Module;

class TypeStore {
public:
    class RecursiveType : public ObjectType {
    public:
        friend class TypeStore;

        RecursiveType(RecursiveType* next, CompositeType* firstType, size_t typeCount, size_t hashCode)
            : ObjectType(ObjectType::RecursiveTypeKind)
            , m_next(next)
            , m_prev(nullptr)
            , m_firstType(firstType)
            , m_refCount(1)
            , m_typeCount(typeCount)
            , m_hashCode(hashCode)
        {
        }

    private:
        RecursiveType* m_next;
        RecursiveType* m_prev;
        CompositeType* m_firstType;
        size_t m_refCount;
        size_t m_typeCount;
        size_t m_hashCode;
    };

    TypeStore()
        : m_first(nullptr)
    {
    }

    void updateTypes(Vector<FunctionType*>& types);
    void releaseTypes(Vector<FunctionType*>& types);
    void releaseTypes(FunctionTypeVector& types);

private:
    static void updateRefs(CompositeType* type, const Vector<FunctionType*>& types);
    void releaseRecursiveType(RecursiveType* recType);

    RecursiveType* m_first;
};

} // namespace Walrus

#endif // __WalrusTypeStore__
