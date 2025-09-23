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

#ifndef __WalrusGCBase__
#define __WalrusGCBase__

#include "runtime/Object.h"

namespace Walrus {

class GCBase : public Object {
    friend class TypeStore;

public:
#ifdef ENABLE_GC
    void addRef();
    void releaseRef();
#endif

protected:
    static constexpr size_t UnassignedReference = ~static_cast<uintptr_t>(0);

    GCBase(const CompositeType** typeInfo)
        : Object(typeInfo)
#ifdef ENABLE_GC
        , m_refIndex(UnassignedReference)
#endif
    {
    }

#ifdef ENABLE_GC
    size_t m_refIndex;
#endif
};

} // namespace Walrus

#endif // __WalrusGCBase__
