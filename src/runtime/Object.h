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

class Object {
public:
    virtual ~Object() {}

    enum Kind : uint8_t {
        Invalid,
        ModuleKind,
        InstanceKind,
        FunctionKind,
        GlobalKind,
        TableKind,
        MemoryKind,
        TrapKind,
        TagKind,
    };

    virtual Kind kind() const = 0;

    virtual bool isModule() const
    {
        return false;
    }

    virtual bool isInstance() const
    {
        return false;
    }

    virtual bool isFunction() const
    {
        return false;
    }

    virtual bool isGlobal() const
    {
        return false;
    }

    virtual bool isTable() const
    {
        return false;
    }

    virtual bool isMemory() const
    {
        return false;
    }

    virtual bool isTrap() const
    {
        return false;
    }

    virtual bool isTag() const
    {
        return false;
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
};

typedef Vector<Object*, std::allocator<Object*>> ObjectVector;

} // namespace Walrus

#endif // __WalrusObject__
