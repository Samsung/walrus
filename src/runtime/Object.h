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

#ifndef __WalrusObject__
#define __WalrusObject__

#include "util/Vector.h"

namespace Walrus {

class Module;
class Instance;
class Function;
class Global;
class Table;
class Memory;
class Trap;
class Tag;
class CompositeType;

class Object {
public:
    enum Kind : uint8_t {
        // WebAssembly types
        // TODO: Implement Struct and Array
        StructKind,
        ArrayKind,
        FunctionKind,
        // Host types
        ModuleKind,
        InstanceKind,
        GlobalKind,
        TableKind,
        MemoryKind,
        TrapKind,
        TagKind,
        Invalid,
    };

    Object(const CompositeType** typeInfo)
        : m_typeInfo(typeInfo)
    {
    }

    virtual ~Object() {}

    Kind kind() const
    {
        return static_cast<Kind>(reinterpret_cast<uintptr_t>(m_typeInfo[-1]));
    }

    const CompositeType** typeInfo()
    {
        return m_typeInfo;
    }

    const CompositeType* objectTypeInfo()
    {
        return m_typeInfo[reinterpret_cast<size_t>(m_typeInfo[0])];
    }

    bool isModule() const
    {
        return kind() == ModuleKind;
    }

    bool isInstance() const
    {
        return kind() == InstanceKind;
    }

    bool isFunction() const
    {
        return kind() == FunctionKind;
    }

    bool isGlobal() const
    {
        return kind() == GlobalKind;
    }

    bool isTable() const
    {
        return kind() == TableKind;
    }

    bool isMemory() const
    {
        return kind() == MemoryKind;
    }

    bool isTrap() const
    {
        return kind() == TrapKind;
    }

    bool isTag() const
    {
        return kind() == TagKind;
    }

    Module* asModule()
    {
        ASSERT(isModule());
        return reinterpret_cast<Module*>(this);
    }

    Instance* asInstance()
    {
        ASSERT(isInstance());
        return reinterpret_cast<Instance*>(this);
    }

    Function* asFunction()
    {
        ASSERT(isFunction());
        return reinterpret_cast<Function*>(this);
    }

    Global* asGlobal()
    {
        ASSERT(isGlobal());
        return reinterpret_cast<Global*>(this);
    }

    Table* asTable()
    {
        ASSERT(isTable());
        return reinterpret_cast<Table*>(this);
    }

    Memory* asMemory()
    {
        ASSERT(isMemory());
        return reinterpret_cast<Memory*>(this);
    }

    Trap* asTrap()
    {
        ASSERT(isTrap());
        return reinterpret_cast<Trap*>(this);
    }

    Tag* asTag()
    {
        ASSERT(isTag());
        return reinterpret_cast<Tag*>(this);
    }

private:
    // Type info is the concatenation of type kind and type casting data.
    //   m_typeInfo[-1] is the type kind
    //   m_typeInfo[0] is the size of the base classes
    //   m_typeInfo[1..n] base classes
    const CompositeType** m_typeInfo;
};

// Extern objects could be shared with other Module
class Extern : public Object {
public:
#ifndef NDEBUG
    // count the total number of created Extern objects
    static size_t g_externCount;
#endif

    virtual ~Extern()
    {
#ifndef NDEBUG
        ASSERT(g_externCount);
        g_externCount--;
#endif
    }

protected:
    Extern(const CompositeType** typeInfo)
        : Object(typeInfo)
    {
#ifndef NDEBUG
        g_externCount++;
#endif
    }
};

#define DEFINE_GLOBAL_TYPE_INFO(name, type) \
    static const CompositeType* name[2] = { reinterpret_cast<CompositeType*>(Object::type), reinterpret_cast<CompositeType*>(0) }
#define GET_GLOBAL_TYPE_INFO(name) ((name) + 1)

typedef Vector<Object*> ObjectVector;
typedef Vector<Extern*> ExternVector;

} // namespace Walrus

#endif // __WalrusObject__
