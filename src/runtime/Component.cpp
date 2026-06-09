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
            ASSERT(ref->isValueType() || ref->isTypeLabels() || ref->isTypeResource() || ref->kind() == SubResourceKind);
            break;
        }

        ComponentRefCounted* nextRef = ref->m_nextFree;
        delete ref;
        ref = nextRef;
    } while (ref != nullptr);
}

class FunctionCoreTypeList {
public:
    static constexpr uint32_t TooMany = ~static_cast<uint32_t>(0);
    static constexpr uint8_t TypeF32 = 0x1;
    static constexpr uint8_t TypeI32 = 0x2;
    static constexpr uint8_t TypeF64 = 0x4;
    static constexpr uint8_t TypeI64 = 0x8;

    FunctionCoreTypeList(uint8_t* list, uint32_t maxIndex, bool is64)
        : m_list(list)
        , m_currentIndex(0)
        , m_maxIndex(maxIndex)
        , m_ptrType(is64 ? TypeI64 : TypeI32)
    {
    }

    uint32_t end() const
    {
        return m_currentIndex;
    }

    void append(const ComponentTypeRef& ref);
    static Value::Type toValueType(uint8_t type);

private:
    uint8_t* m_list;
    uint32_t m_currentIndex;
    uint32_t m_maxIndex;
    uint8_t m_ptrType;
};

void FunctionCoreTypeList::append(const ComponentTypeRef& ref)
{
    ASSERT(ref.type() != ComponentTypeRef::TypeNone);
    if (m_currentIndex >= m_maxIndex) {
        m_currentIndex = TooMany;
        return;
    }

    ComponentTypeRef::Type refType = ref.type();
    if (refType == ComponentTypeRef::TypeIndex && ref.ref()->kind() == ComponentRefCounted::ValueTypeKind) {
        refType = ref.ref()->asValueType()->type();
    }

    switch (refType) {
    case ComponentTypeRef::S64:
    case ComponentTypeRef::U64:
        m_list[m_currentIndex++] |= TypeI64;
        return;
    case ComponentTypeRef::F32:
        m_list[m_currentIndex++] |= TypeF32;
        return;
    case ComponentTypeRef::F64:
        m_list[m_currentIndex++] |= TypeF64;
        return;
    case ComponentTypeRef::String:
        m_list[m_currentIndex++] |= m_ptrType;
        if (m_currentIndex < m_maxIndex) {
            m_list[m_currentIndex++] |= m_ptrType;
        } else {
            m_currentIndex = TooMany;
        }
        return;
    case ComponentTypeRef::TypeIndex:
        break;
    default:
        ASSERT(ref.type() == ComponentTypeRef::Bool || ref.type() == ComponentTypeRef::Char
               || ref.type() == ComponentTypeRef::S8 || ref.type() == ComponentTypeRef::U8
               || ref.type() == ComponentTypeRef::S16 || ref.type() == ComponentTypeRef::U16
               || ref.type() == ComponentTypeRef::S32 || ref.type() == ComponentTypeRef::U32
               || ref.type() == ComponentTypeRef::ErrorContext);
        m_list[m_currentIndex++] |= TypeI32;
        return;
    }

    ComponentRefCounted* target = ref.ref();

    switch (target->kind()) {
    case ComponentRefCounted::RecordKind:
        for (auto& item : target->asTypeItems()->items()) {
            append(item.type);
            if (m_currentIndex == TooMany) {
                break;
            }
        }
        break;
    case ComponentRefCounted::VariantKind: {
        m_list[m_currentIndex++] |= TypeI32;
        uint32_t startIndex = m_currentIndex;
        uint32_t maxIndex = m_currentIndex;
        for (auto& item : target->asTypeItems()->items()) {
            if (item.type.type() == ComponentTypeRef::TypeNone) {
                continue;
            }
            m_currentIndex = startIndex;
            append(item.type);
            if (maxIndex < m_currentIndex) {
                maxIndex = m_currentIndex;
            }
            if (m_currentIndex == TooMany) {
                ASSERT(maxIndex == TooMany);
                break;
            }
        }
        m_currentIndex = maxIndex;
        break;
    }
    case ComponentRefCounted::ListKind: {
        m_list[m_currentIndex++] |= m_ptrType;
        if (m_currentIndex < m_maxIndex) {
            m_list[m_currentIndex++] |= m_ptrType;
        } else {
            m_currentIndex = TooMany;
        }
        break;
    }
    case ComponentRefCounted::ListFixedKind: {
        const ComponentTypeRef& item = target->asTypeListFixed()->type();
        uint32_t count = target->asTypeListFixed()->size();
        while (count > 0) {
            append(item);
            if (m_currentIndex == TooMany) {
                break;
            }
            count--;
        }
        break;
    }
    case ComponentRefCounted::TupleKind:
        for (auto& item : target->asTypeTuple()->items()) {
            append(item.type());
            if (m_currentIndex == TooMany) {
                break;
            }
        }
        break;
    case ComponentRefCounted::OptionKind:
        m_list[m_currentIndex++] |= TypeI32;
        if (m_currentIndex < m_maxIndex) {
            append(target->asValueTypeRef()->type());
        } else {
            m_currentIndex = TooMany;
        }
        break;
    case ComponentRefCounted::ResultKind: {
        m_list[m_currentIndex++] |= TypeI32;
        uint32_t startIndex = m_currentIndex;
        ComponentTypeResult* result = target->asTypeResult();
        if (result->result().type() != ComponentTypeRef::TypeNone) {
            append(result->result());
            if (m_currentIndex == TooMany) {
                break;
            }
        }

        uint32_t maxIndex = m_currentIndex;
        if (result->error().type() != ComponentTypeRef::TypeNone) {
            m_currentIndex = startIndex;
            append(result->error());
            if (m_currentIndex < maxIndex) {
                ASSERT(m_currentIndex != TooMany);
                m_currentIndex = maxIndex;
            }
        }
        break;
    }
    default:
        ASSERT(target->kind() == ComponentRefCounted::FlagsKind || target->kind() == ComponentRefCounted::EnumKind
               || target->kind() == ComponentRefCounted::OwnKind || target->kind() == ComponentRefCounted::BorrowKind
               || target->kind() == ComponentRefCounted::StreamKind || target->kind() == ComponentRefCounted::FutureKind);
        m_list[m_currentIndex++] |= TypeI32;
        break;
    }
}

Value::Type FunctionCoreTypeList::toValueType(uint8_t type)
{
    ASSERT(type != 0);
    if (type == TypeF32) {
        return Value::Type::F32;
    }
    if (type == TypeF64) {
        return Value::Type::F64;
    }
    if ((type & (TypeI64 | TypeF64)) == 0) {
        return Value::Type::I32;
    }
    return Value::Type::I64;
}

FunctionType* ComponentTypeFunc::createFunctionType(Store* store, bool is64)
{
    uint8_t coreParams[16] = { 0 };
    FunctionCoreTypeList coreParamList(coreParams, 16, is64);

    for (auto& param : params()) {
        coreParamList.append(param.type);
    }

    uint8_t coreResult[1] = { 0 };
    FunctionCoreTypeList coreResultList(coreResult, 1, is64);
    if (result().type() != ComponentTypeRef::TypeNone) {
        coreResultList.append(result());
    }

    uint32_t paramTypesCount = coreParamList.end();
    uint32_t resultTypesCount = coreResultList.end();

    if (paramTypesCount == FunctionCoreTypeList::TooMany) {
        paramTypesCount = 1;
    }

    if (resultTypesCount == FunctionCoreTypeList::TooMany) {
        paramTypesCount += 1;
        resultTypesCount = 0;
    }

    FunctionType* functionType = new FunctionType(paramTypesCount, 0, resultTypesCount, 0, true, reinterpret_cast<const CompositeType**>(TypeStore::NoIndex));
    TypeVector* param = functionType->initParam();

    paramTypesCount = coreParamList.end();
    if (paramTypesCount == FunctionCoreTypeList::TooMany) {
        param->setType(0, is64 ? Value::Type::I64 : Value::Type::I32);
        paramTypesCount = 1;
    } else {
        for (uint32_t i = 0; i < paramTypesCount; i++) {
            param->setType(i, FunctionCoreTypeList::toValueType(coreParams[i]));
        }
    }

    resultTypesCount = coreResultList.end();
    if (resultTypesCount == FunctionCoreTypeList::TooMany) {
        param->setType(paramTypesCount, is64 ? Value::Type::I64 : Value::Type::I32);
    } else if (resultTypesCount > 0) {
        ASSERT(resultTypesCount == 1);
        functionType->initResult()->setType(0, FunctionCoreTypeList::toValueType(coreResult[0]));
    }

    functionType->initDone();

    Vector<CompositeType*> typeList;
    typeList.push_back(functionType);
    store->getTypeStore().updateTypes(typeList);
    return typeList[0]->asFunction();
}

Component::Component(Store* store)
    : m_resourceCount(0)
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
