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

namespace Walrus {

#ifndef NDEBUG
size_t Extern::g_externCount;
#endif
FunctionType* Store::g_defaultFunctionTypes[Value::Type::NUM];

Store::~Store()
{
    // deallocate Modules and Instances
    for (size_t i = 0; i < m_instances.size(); i++) {
        Instance::freeInstance(m_instances[i]);
    }

    for (size_t i = 0; i < m_modules.size(); i++) {
        delete m_modules[i];
    }

    for (size_t i = 0; i < m_externs.size(); i++) {
        delete m_externs[i];
    }

    Store::finalize();
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

FunctionType* Store::getDefaultFunctionType(Value::Type type)
{
    switch (type) {
    case Value::Type::I32:
        if (!g_defaultFunctionTypes[type])
            g_defaultFunctionTypes[type] = new FunctionType(new ValueTypeVector(), new ValueTypeVector({ Value::I32 }));
        break;
    case Value::Type::I64:
        if (!g_defaultFunctionTypes[type])
            g_defaultFunctionTypes[type] = new FunctionType(new ValueTypeVector(), new ValueTypeVector({ Value::I64 }));
        break;
    case Value::Type::F32:
        if (!g_defaultFunctionTypes[type])
            g_defaultFunctionTypes[type] = new FunctionType(new ValueTypeVector(), new ValueTypeVector({ Value::F32 }));
        break;
    case Value::Type::F64:
        if (!g_defaultFunctionTypes[type])
            g_defaultFunctionTypes[type] = new FunctionType(new ValueTypeVector(), new ValueTypeVector({ Value::F64 }));
        break;
    case Value::Type::V128:
        if (!g_defaultFunctionTypes[type])
            g_defaultFunctionTypes[type] = new FunctionType(new ValueTypeVector(), new ValueTypeVector({ Value::V128 }));
        break;
    case Value::Type::FuncRef:
        if (!g_defaultFunctionTypes[type])
            g_defaultFunctionTypes[type] = new FunctionType(new ValueTypeVector(), new ValueTypeVector({ Value::FuncRef }));
        break;
    case Value::Type::ExternRef:
        if (!g_defaultFunctionTypes[type])
            g_defaultFunctionTypes[type] = new FunctionType(new ValueTypeVector(), new ValueTypeVector({ Value::ExternRef }));
        break;
    default:
        RELEASE_ASSERT_NOT_REACHED();
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
