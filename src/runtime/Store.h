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

namespace Walrus {

class Engine;
class Module;
class Instance;

class Store {
public:
    Store(Engine* engine)
        : m_engine(engine)
    {
    }

    ~Store();

    void appendModule(const Module* module)
    {
        m_modules.push_back(module);
    }

    void appendInstance(const Instance* instance)
    {
        m_instances.push_back(instance);
    }

    void deleteModule(const Module* module);

private:
    Engine* m_engine;

    Vector<const Module*> m_modules;
    Vector<const Instance*> m_instances;
};

} // namespace Walrus

#endif // __WalrusStore__
