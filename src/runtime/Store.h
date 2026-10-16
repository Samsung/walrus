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
#include <unordered_map>
#include <unordered_set>

namespace Walrus {

class Engine;
class Function;
class Module;
class Instance;
class Component;
class ComponentInstance;
class Extern;
class Object;
class StoreObject;
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
        I32_RI32,
        I32I32_RI32,
        I32I64I32_RI32,
        I32I32I32_RI32,
        I32I32I32I32_RI32,
        I32I64_RI32,
        I32I64I32I32_RI32,
        I32I64I64_RI32,
        I32I64I64I32_RI32,
        I32I32I32I32I32_RI32,
        I32I32I32I64I32_RI32,
        I32I32I32I32I32I32_RI32,
        I32I32I32I32I32I32I32_RI32,
        I32I32I32I32I64I64I32_RI32,
        I32I32I32I32I32I64I64I32I32_RI32,
        RI32,
        I64R,
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

    // Only C API stores use automatic collection. Internal C++ clients retain
    // store-lifetime ownership. Scopes cover wasm execution and bulk deletion.
    class CollectionScope {
    public:
        CollectionScope();
        ~CollectionScope();
        CollectionScope(const CollectionScope&) = delete;
        CollectionScope& operator=(const CollectionScope&) = delete;
    };
    void enableAPICollection();
    // A reference mutation invalidates the snapshot. Dropping only an API
    // root can reuse it. Never rebuild while wasm or a bulk API call runs.
    void requestCollection(bool referencesChanged = true);
    static void addAPIReference(const Object* object);
    static void removeAPIReference(const Object* object);

#ifdef WALRUS_API_TESTS
    size_t apiTestObjectCount() const
    {
        return m_modules.size() + m_instances.size() + m_externs.size();
    }
    size_t apiTestCollectionCount() const { return m_collectionCount; }
    size_t apiTestGraphBuildCount() const { return m_graphBuildCount; }
    size_t apiTestTableSlotCount() const { return m_tableSlotCount; }
    size_t apiTestGraphTraceCount() const { return m_graphTraceCount; }
    size_t apiTestGraphEdgeCount() const
    {
        size_t count = m_gcRootTargets.size();
        for (const auto& entry : m_referenceGraph) {
            count += entry.second.targets.size();
        }
        return count;
    }
#endif

    static void finalize();
    static FunctionType* getDefaultFunctionType(Value::Type type);

    FunctionType* getDefinedFunctionType(DefinedFunctionType type)
    {
        if (m_definedFuncTypes[type] != nullptr) {
            return m_definedFuncTypes[type];
        }
        return createDefinedFunctionType(type);
    }

    void appendModule(Module* module);

    void appendInstance(Instance* instance);

    void appendComponent(Component* component)
    {
        m_components.push_back(component);
    }

    void appendComponentInstance(ComponentInstance* instance)
    {
        m_componentInstances.push_back(instance);
    }

    void appendExtern(Extern* ext);

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
    static void collectPending();
    void collectUnreferenced();
    void rebuildReferenceGraph(std::vector<StoreObject*>& dead);
    void traceReferences(Object* object, std::vector<StoreObject*>& targets);
    void collectCycles(std::vector<StoreObject*>& dead);
    void eraseGraphObject(StoreObject* object);
    void freeObjects(std::vector<StoreObject*>& dead);

    struct ReferenceNode {
        // One edge per distinct target; incoming counts source objects, not slots.
        std::vector<StoreObject*> targets;
        size_t incoming = 0;
    };
    std::unordered_map<StoreObject*, ReferenceNode> m_referenceGraph;
    std::vector<StoreObject*> m_gcRootTargets;
    std::unordered_set<StoreObject*> m_collectionCandidates;
    bool m_referenceGraphDirty = true;
#ifdef WALRUS_API_TESTS
    size_t m_collectionCount = 0;
    size_t m_graphBuildCount = 0;
    size_t m_tableSlotCount = 0;
    size_t m_graphTraceCount = 0;
#endif
    bool m_apiCollection = false;
    bool m_collectionPending = false;

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
