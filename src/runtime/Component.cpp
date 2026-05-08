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

#include "runtime/Component.h"
#include "runtime/Store.h"

namespace Walrus {

void ComponentRefCounted::releaseAllRefs(ComponentRefCounted* ref)
{
    ref->m_nextFree = nullptr;
    do {
        switch (ref->kind()) {
        case RecordKind:
        case VariantKind:
            for (auto& it : ref->asTypeItems()->items()) {
                releaseAndInsert(ref, it.type.ref());
            }
            break;
        case ListKind:
        case OptionKind:
        case StreamKind:
        case FutureKind:
            releaseAndInsert(ref, ref->asValueTypeRef()->type().ref());
            break;
        case ListFixedKind:
            releaseAndInsert(ref, ref->asTypeListFixed()->type().ref());
            break;
        case TupleKind:
            for (auto& it : ref->asTypeTuple()->items()) {
                releaseAndInsert(ref, it.ref());
            }
            break;
        case ResultKind:
            releaseAndInsert(ref, ref->asTypeResult()->result().ref());
            releaseAndInsert(ref, ref->asTypeResult()->error().ref());
            break;
        case OwnKind:
        case BorrowKind:
            releaseAndInsert(ref, ref->asTypeResourceRef()->ref());
            break;
        case FuncKind:
            for (auto& it : ref->asTypeFunc()->params()) {
                releaseAndInsert(ref, it.type.ref());
            }
            releaseAndInsert(ref, ref->asTypeFunc()->result().ref());
            break;
        case InstanceTypeKind:
        case ComponentTypeKind: {
            ComponentType* componentType = ref->asComponentType();
            for (auto& it : componentType->types()) {
                releaseAndInsert(ref, it);
            }
            for (auto& it : componentType->exports()) {
                releaseAndInsert(ref, it.type);
            }
            for (auto& it : componentType->imports()) {
                releaseAndInsert(ref, it.type);
            }
            break;
        }
        default:
            ASSERT(ref->isValueType() || ref->isTypeLabels() || ref->isTypeResource());
            break;
        }

        ComponentRefCounted* nextRef = ref->m_nextFree;
        delete ref;
        ref = nextRef;
    } while (ref != nullptr);
}

Component::Component(Store* store)
{
    m_type = new ComponentType(ComponentRefCounted::ComponentTypeKind);
    store->appendComponent(this);
}

Component::~Component()
{
    for (auto it : m_declarations) {
        delete it;
    }
    m_type->releaseRef();
}

} // namespace Walrus
