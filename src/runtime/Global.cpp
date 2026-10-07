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
#include "Walrus.h"

#include "Global.h"

#ifdef ENABLE_GC
#include "GCUtil.h"
#endif

namespace Walrus {

DEFINE_GLOBAL_TYPE_INFO(globalTypeInfo, GlobalKind);

Global::Global(const Value& value, const MutableType& type)
    : Extern(GET_GLOBAL_TYPE_INFO(globalTypeInfo))
    , m_value(value)
    , m_type(type)
{
}

Global* Global::createGlobal(Store* store, const Value& value, const MutableType& type)
{
#ifdef ENABLE_GC
    void* mem = Value::isRefType(type.type()) ? GC_MALLOC_UNCOLLECTABLE(sizeof(Global)) : GC_MALLOC_ATOMIC_UNCOLLECTABLE(sizeof(Global));
    Global* glob = new (mem) Global(value, type);
#else
    Global* glob = new Global(value, type);
#endif
    store->appendExtern(glob);
    return glob;
}

#ifdef ENABLE_GC
void Global::operator delete(void* ptr)
{
    GC_FREE(ptr);
}
#endif

} // namespace Walrus
