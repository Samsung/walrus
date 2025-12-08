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

static const FunctionType g_defaultFunctionTypes[] = {
#define DEFINE_RESULT_TYPE(name) FunctionType(Value::name),
    FOR_EACH_VALUE_TYPE(DEFINE_RESULT_TYPE)
#undef DEFINE_RESULT_TYPE
};

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
} // namespace Walrus
