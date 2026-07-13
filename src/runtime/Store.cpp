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

#include "runtime/Store.h"
#include "runtime/Module.h"
#include "runtime/Instance.h"
#include "runtime/Component.h"
#include "runtime/ComponentInstance.h"
#include "runtime/ObjectType.h"

#ifdef ENABLE_GC
#include "GCUtil.h"
#endif /* ENABLE_GC */

namespace Walrus {

#ifndef NDEBUG
size_t Extern::g_externCount;
#endif

static const FunctionType g_defaultFunctionTypes[] = {
#define DEFINE_RESULT_TYPE(name) FunctionType(Value::name),
    FOR_EACH_VALUE_TYPE(DEFINE_RESULT_TYPE)
#undef DEFINE_RESULT_TYPE
};

Store::Store(Engine* engine)
    : m_engine(engine)
#ifdef ENABLE_WASI
    , m_wasiData(nullptr)
#endif
{
    memset(m_definedFuncTypes, 0, sizeof(m_definedFuncTypes));
#ifdef ENABLE_GC
    GC_INIT();
#endif /* ENABLE_GC */
}

Store::~Store()
{
    for (size_t i = 0; i < FUNC_TYPES_NUM; i++) {
        FunctionType* type = m_definedFuncTypes[i];
        if (type != nullptr) {
            TypeStore::ReleaseRef(type->subTypeList());
        }
    }

    // deallocate Modules and Instances
    for (size_t i = 0; i < m_instances.size(); i++) {
        Instance::freeInstance(m_instances[i]);
    }

    for (size_t i = 0; i < m_modules.size(); i++) {
        getTypeStore().releaseTypes(m_modules[i]->m_compositeTypes);
        delete m_modules[i];
    }

    for (size_t i = 0; i < m_componentInstances.size(); i++) {
        delete m_componentInstances[i];
    }

    for (size_t i = 0; i < m_components.size(); i++) {
        delete m_components[i];
    }

    for (size_t i = 0; i < m_externs.size(); i++) {
        delete m_externs[i];
    }

    for (size_t i = 0; i < m_waiterList.size(); i++) {
        delete m_waiterList[i];
    }

    Store::finalize();

#ifdef ENABLE_GC
    GC_gcollect_and_unmap();
    GC_invoke_finalizers();
#endif /* ENABLE_GC */
}

void Store::finalize()
{
#ifndef NDEBUG
    // check if all Extern objects has been deallocated
    ASSERT(Extern::g_externCount == 0);
#endif
}

FunctionType* Store::getDefaultFunctionType(Value::Type type)
{
    return const_cast<FunctionType*>(g_defaultFunctionTypes + static_cast<size_t>(type));
}

Waiter* Store::getWaiter(void* address)
{
    std::lock_guard<std::mutex> guard(m_waiterListLock);
    for (size_t i = 0; i < m_waiterList.size(); i++) {
        if (m_waiterList[i]->m_address == address) {
            return m_waiterList[i];
        }
    }

    Waiter* waiter = new Waiter(address);
    m_waiterList.push_back(waiter);
    return waiter;
}

FunctionType* Store::createDefinedFunctionType(DefinedFunctionType type)
{
    const CompositeType** noIndex = reinterpret_cast<const CompositeType**>(TypeStore::NoIndex);
    FunctionType* functionType;
    TypeVector* param;
    TypeVector* result;

    switch (type) {
    case NONE:
        functionType = new FunctionType(0, 0, 0, 0, true, noIndex);
        break;
    case I32R:
        functionType = new FunctionType(1, 0, 0, 0, true, noIndex);
        param = functionType->initParam();
        param->setType(0, Value::Type::I32);
        break;
    case I32_RI32:
        functionType = new FunctionType(1, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        result->setType(0, Value::Type::I32);
        break;
    case I32I32_RI32:
        functionType = new FunctionType(2, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I32);
        result->setType(0, Value::Type::I32);
        break;
    case I32I64I64_RI32:
        functionType = new FunctionType(3, 0, 1, 0, true, noIndex);

        param = functionType->initParam();
        result = functionType->initResult();

        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I64);
        param->setType(2, Value::Type::I64);

        result->setType(0, Value::Type::I32);
        break;
    case I32I64I32_RI32:
        functionType = new FunctionType(3, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I64);
        param->setType(2, Value::Type::I32);
        result->setType(0, Value::Type::I32);
        break;
    case I32I32I32_RI32:
        functionType = new FunctionType(3, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I32);
        param->setType(2, Value::Type::I32);
        result->setType(0, Value::Type::I32);
        break;
    case I32I32I32I32_RI32:
        functionType = new FunctionType(4, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I32);
        param->setType(2, Value::Type::I32);
        param->setType(3, Value::Type::I32);
        result->setType(0, Value::Type::I32);
        break;
    case I32I64_RI32:
        functionType = new FunctionType(2, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I64);
        result->setType(0, Value::Type::I32);
        break;
    case I32I64I32I32_RI32:
        functionType = new FunctionType(4, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I64);
        param->setType(2, Value::Type::I32);
        param->setType(3, Value::Type::I32);
        result->setType(0, Value::Type::I32);
        break;
    case I32I64I64I32_RI32:
        functionType = new FunctionType(4, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I64);
        param->setType(2, Value::Type::I64);
        param->setType(3, Value::Type::I32);
        result->setType(0, Value::Type::I32);
        break;
    case I32I32I32I32I32_RI32:
        functionType = new FunctionType(5, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I32);
        param->setType(2, Value::Type::I32);
        param->setType(3, Value::Type::I32);
        param->setType(4, Value::Type::I32);
        result->setType(0, Value::Type::I32);
        break;
    case I32I32I32I64I32_RI32:
        functionType = new FunctionType(5, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I32);
        param->setType(2, Value::Type::I32);
        param->setType(3, Value::Type::I64);
        param->setType(4, Value::Type::I32);
        result->setType(0, Value::Type::I32);
        break;
    case I32I32I32I32I32I32_RI32:
        functionType = new FunctionType(6, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I32);
        param->setType(2, Value::Type::I32);
        param->setType(3, Value::Type::I32);
        param->setType(4, Value::Type::I32);
        param->setType(5, Value::Type::I32);
        result->setType(0, Value::Type::I32);
        break;
    case I32I32I32I32I64I64I32_RI32:
        functionType = new FunctionType(7, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I32);
        param->setType(2, Value::Type::I32);
        param->setType(3, Value::Type::I32);
        param->setType(4, Value::Type::I64);
        param->setType(5, Value::Type::I64);
        param->setType(6, Value::Type::I32);
        result->setType(0, Value::Type::I32);
        break;
    case I32I32I32I32I32I64I64I32I32_RI32:
        functionType = new FunctionType(9, 0, 1, 0, true, noIndex);
        param = functionType->initParam();
        result = functionType->initResult();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::I32);
        param->setType(2, Value::Type::I32);
        param->setType(3, Value::Type::I32);
        param->setType(4, Value::Type::I32);
        param->setType(5, Value::Type::I64);
        param->setType(6, Value::Type::I64);
        param->setType(7, Value::Type::I32);
        param->setType(8, Value::Type::I32);
        result->setType(0, Value::Type::I32);
        break;
    case RI32:
        functionType = new FunctionType(0, 0, 1, 0, true, noIndex);
        result = functionType->initResult();
        result->setType(0, Value::Type::I32);
        break;
    case I64R:
        functionType = new FunctionType(1, 0, 0, 0, true, noIndex);
        param = functionType->initParam();
        param->setType(0, Value::Type::I64);
        break;
    case F32R:
        functionType = new FunctionType(1, 0, 0, 0, true, noIndex);
        param = functionType->initParam();
        param->setType(0, Value::Type::F32);
        break;
    case F64R:
        functionType = new FunctionType(1, 0, 0, 0, true, noIndex);
        param = functionType->initParam();
        param->setType(0, Value::Type::F64);
        break;
    case I32F32R:
        functionType = new FunctionType(2, 0, 0, 0, true, noIndex);
        param = functionType->initParam();
        param->setType(0, Value::Type::I32);
        param->setType(1, Value::Type::F32);
        break;
    case F64F64R:
        functionType = new FunctionType(2, 0, 0, 0, true, noIndex);
        param = functionType->initParam();
        param->setType(0, Value::Type::F64);
        param->setType(1, Value::Type::F64);
        break;
    default:
        ASSERT(type == INVALID);
        functionType = new FunctionType(1, 0, 0, 0, true, noIndex);
        param = functionType->initParam();
        // Temporary types cannot be used as params
        param->setType(0, Value::Type::Void);
        break;
    }

    functionType->initDone();

    Vector<CompositeType*> typeList;
    typeList.push_back(functionType);
    m_typeStore.updateTypes(typeList);
    functionType = typeList[0]->asFunction();

    m_definedFuncTypes[type] = functionType;
    return functionType;
}

} // namespace Walrus
