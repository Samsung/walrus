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

#include "util/String.h"
#include "runtime/Value.h"
#include "runtime/Engine.h"

namespace Walrus {

class Engine;

class Store : public gc {
public:
    typedef Vector<std::pair<String*, Value>, GCUtil::gc_malloc_allocator<std::pair<String*, Value>>> GlobalVariableVector;

    Store(Engine* engine)
        : m_engine(engine)
    {
    }

    GlobalVariableVector& global()
    {
        return m_global;
    }

private:
    Engine* m_engine;
    GlobalVariableVector m_global;
};

} // namespace Walrus

#endif // __WalrusStore__
