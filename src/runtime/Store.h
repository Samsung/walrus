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
class Extern;
class FunctionType;

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
    Store(Engine* engine)
        : m_engine(engine)
    {
    }

    ~Store();

    static void finalize();
    static FunctionType* getDefaultFunctionType(Value::Type type);

    void appendModule(Module* module)
    {
        m_modules.push_back(module);
    }

    void appendInstance(Instance* instance)
    {
        m_instances.push_back(instance);
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

private:
    Engine* m_engine;
    TypeStore m_typeStore;

    Vector<Module*> m_modules;
    Vector<Instance*> m_instances;
    Vector<Extern*> m_externs;

    // default FunctionTypes used for initialization of Data, Element and Global
    static FunctionType* g_defaultFunctionTypes[Value::Type::NUM];
    std::mutex m_waiterListLock;
    std::vector<Waiter*> m_waiterList;
};

} // namespace Walrus

#endif // __WalrusStore__
