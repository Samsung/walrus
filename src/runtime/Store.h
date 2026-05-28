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

#ifndef __WalrusStore__
#define __WalrusStore__

#include "util/Vector.h"
#include "runtime/TypeStore.h"
#include "runtime/Value.h"
#include <mutex>
#include <condition_variable>

namespace Walrus {

class Engine;
class Module;
class Instance;
class Component;
class ComponentInstance;
class Extern;
class FunctionType;

#ifdef ENABLE_WASI
class WasiStoreData;
#endif

struct Waiter {
    struct WaiterItem {
        WaiterItem(Waiter* waiter)
            : m_waiter(waiter)
        {
        }

        Waiter* m_waiter;
    };

    Waiter(void* addr)
        : m_address(addr)
    {
    }

    void* m_address;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::vector<WaiterItem*> m_waiterItemList;
};

class Store {
public:
    enum DefinedFunctionType : uint8_t {
        // The R is meant to represent the results, after R are the result types.
        NONE = 0,
        I32R,
        I32I32R,
        I32I32I32R,
        I32I64I32R,
        I32I32I32I32R,
        I32_RI32,
        I32I32_RI32,
        I32I64I32_RI32,
        I32I32I32_RI32,
        I32I32I32I32_RI32,
        I32I64I32I32_RI32,
        I32I64I64I32_RI32,
        I32I32I32I32I32_RI32,
        I32I32I32I64I32_RI32,
        I32I32I32I32I32I32_RI32,
        I32I32I32I32I32I32I32R,
        I32I32I32I32I64I64I32_RI32,
        I32I32I32I32I32I64I64I32I32_RI32,
        RI32,
        I64R,
        I64_RI32,
        RI64,
        F32R,
        F64R,
        I32F32R,
        F64F64R,
        INVALID,
        FUNC_TYPES_NUM,
    };

    class ComponentContext {
        MAKE_STACK_ALLOCATED()

    public:
        ComponentContext(Store* store, ComponentInstance* instance)
            : m_store(store)
            , m_prevContext(store->m_context)
            , m_instance(instance)
        {
            m_store->m_context = this;
        }

        ~ComponentContext()
        {
            m_store->m_context = m_prevContext;
        }

        ComponentContext* prevContext() const
        {
            return m_prevContext;
        }

        ComponentInstance* instance() const
        {
            return m_instance;
        }

    private:
        Store* m_store;
        ComponentContext* m_prevContext;
        ComponentInstance* m_instance;
    };

    Store(Engine* engine);

    ~Store();

    static void finalize();
    static FunctionType* getDefaultFunctionType(Value::Type type);

    FunctionType* getDefinedFunctionType(DefinedFunctionType type)
    {
        if (m_definedFuncTypes[type] != nullptr) {
            return m_definedFuncTypes[type];
        }
        return createDefinedFunctionType(type);
    }

    void appendModule(Module* module)
    {
        m_modules.push_back(module);
    }

    void appendInstance(Instance* instance)
    {
        m_instances.push_back(instance);
    }

    void appendComponent(Component* component)
    {
        m_components.push_back(component);
    }

    void appendComponentInstance(ComponentInstance* instance)
    {
        m_componentInstances.push_back(instance);
    }

    void appendExtern(Extern* ext)
    {
        m_externs.push_back(ext);
    }

    Instance* getLastInstance()
    {
        ASSERT(m_instances.size());
        return m_instances.back();
    }

    TypeStore& getTypeStore()
    {
        return m_typeStore;
    }

    Waiter* getWaiter(void* address);

    ComponentContext* context() const
    {
        return m_context;
    }

#ifdef ENABLE_WASI
    WasiStoreData* wasiData() const
    {
        return m_wasiData;
    }

    void initWasiData(WasiStoreData* data)
    {
        ASSERT(m_wasiData == nullptr);
        m_wasiData = data;
    }
#endif

private:
    FunctionType* createDefinedFunctionType(DefinedFunctionType type);

    Engine* m_engine;
    TypeStore m_typeStore;

    FunctionType* m_definedFuncTypes[FUNC_TYPES_NUM];

    Vector<Module*> m_modules;
    Vector<Instance*> m_instances;
    Vector<Component*> m_components;
    Vector<ComponentInstance*> m_componentInstances;
    Vector<Extern*> m_externs;

    std::mutex m_waiterListLock;
    std::vector<Waiter*> m_waiterList;

    ComponentContext* m_context;
#ifdef ENABLE_WASI
    WasiStoreData* m_wasiData;
#endif
};

} // namespace Walrus

#endif // __WalrusStore__
