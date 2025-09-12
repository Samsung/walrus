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
#include "runtime/ObjectType.h"

#ifdef ENABLE_GC
#include "GCUtil.h"
#endif /* ENABLE_GC */

namespace Walrus {

#ifndef NDEBUG
size_t Extern::g_externCount;
#endif
FunctionType* Store::g_defaultFunctionTypes[Value::Type::NUM];

Store::Store(Engine* engine)
    : m_engine(engine)
{
#ifdef ENABLE_GC
    GC_INIT();
#endif /* ENABLE_GC */
}

Store::~Store()
{
    // deallocate Modules and Instances
    for (size_t i = 0; i < m_instances.size(); i++) {
        Instance::freeInstance(m_instances[i]);
    }

    for (size_t i = 0; i < m_modules.size(); i++) {
        getTypeStore().releaseTypes(m_modules[i]->m_compositeTypes);
        delete m_modules[i];
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
    for (size_t i = 0; i < Value::Type::NUM; i++) {
        if (g_defaultFunctionTypes[i]) {
            delete g_defaultFunctionTypes[i];
        }
    }

#ifndef NDEBUG
    // check if all Extern objects has been deallocated
    ASSERT(Extern::g_externCount == 0);
#endif
}

#define ALLOCATE_DEFAULT_TYPE(type)                                                                                                  \
    case Value::Type::type:                                                                                                          \
        g_defaultFunctionTypes[Value::Type::type] = new FunctionType(new TypeVector(), new TypeVector({ Type(Value::Type::type) })); \
        break;

FunctionType* Store::getDefaultFunctionType(Value::Type type)
{
    if (!g_defaultFunctionTypes[type]) {
        switch (type) {
            ALLOCATE_DEFAULT_TYPE(I32)
            ALLOCATE_DEFAULT_TYPE(I64)
            ALLOCATE_DEFAULT_TYPE(F32)
            ALLOCATE_DEFAULT_TYPE(F64)
            ALLOCATE_DEFAULT_TYPE(V128)
            ALLOCATE_DEFAULT_TYPE(AnyRef)
            ALLOCATE_DEFAULT_TYPE(NoAnyRef)
            ALLOCATE_DEFAULT_TYPE(EqRef)
            ALLOCATE_DEFAULT_TYPE(I31Ref)
            ALLOCATE_DEFAULT_TYPE(StructRef)
            ALLOCATE_DEFAULT_TYPE(ArrayRef)
            ALLOCATE_DEFAULT_TYPE(ExternRef)
            ALLOCATE_DEFAULT_TYPE(NoExternRef)
            ALLOCATE_DEFAULT_TYPE(FuncRef)
            ALLOCATE_DEFAULT_TYPE(DefinedRef)
            ALLOCATE_DEFAULT_TYPE(NoFuncRef)
            ALLOCATE_DEFAULT_TYPE(NullAnyRef)
            ALLOCATE_DEFAULT_TYPE(NullNoAnyRef)
            ALLOCATE_DEFAULT_TYPE(NullEqRef)
            ALLOCATE_DEFAULT_TYPE(NullI31Ref)
            ALLOCATE_DEFAULT_TYPE(NullStructRef)
            ALLOCATE_DEFAULT_TYPE(NullArrayRef)
            ALLOCATE_DEFAULT_TYPE(NullExternRef)
            ALLOCATE_DEFAULT_TYPE(NullNoExternRef)
            ALLOCATE_DEFAULT_TYPE(NullFuncRef)
            ALLOCATE_DEFAULT_TYPE(NullNoFuncRef)
            ALLOCATE_DEFAULT_TYPE(NullDefinedRef)
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    return g_defaultFunctionTypes[type];
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
} // namespace Walrus
