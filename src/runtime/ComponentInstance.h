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

#ifndef __WalrusComponentInstance__
#define __WalrusComponentInstance__

#include "runtime/Component.h"
#include "runtime/Function.h"

namespace Walrus {

class ComponentInstance;

class CanonOptions {
public:
    static constexpr uint32_t Utf16Tag = static_cast<uint32_t>(1) << 31;

    CanonOptions(ComponentInstance* instance, ComponentCanonOptions::StringEncoding encoding, bool isAsync,
                 Memory* memory, Function* realloc, Function* postReturn, Function* callback)
        : m_instance(instance)
        , m_encoding(encoding)
        , m_isAsync(isAsync)
        , m_memory(memory)
        , m_realloc(realloc)
        , m_postReturn(postReturn)
        , m_callback(callback)
    {
    }

    ComponentInstance* instance() const
    {
        return m_instance;
    }

    ComponentCanonOptions::StringEncoding encoding() const
    {
        return m_encoding;
    }

    bool isAsync() const
    {
        return m_isAsync;
    }

    Memory* memory() const
    {
        return m_memory;
    }

    Function* realloc() const
    {
        return m_realloc;
    }

    Function* postReturn() const
    {
        return m_postReturn;
    }

    Function* callback() const
    {
        return m_callback;
    }

    void memoryCheckRange32(ExecutionState& state, uint32_t align, uint32_t start, uint32_t size);
    void memoryCheckRange64(ExecutionState& state, uint64_t align, uint64_t start, uint64_t size);
    uint32_t memoryMalloc32(ExecutionState& state, uint32_t align, uint32_t size);
    uint64_t memoryMalloc64(ExecutionState& state, uint64_t align, uint64_t size);

    uint64_t storeLatin1String(ExecutionState& state, const uint8_t* src, uint32_t* length);

private:
    ComponentInstance* m_instance;
    ComponentCanonOptions::StringEncoding m_encoding;
    bool m_isAsync;
    Memory* m_memory;
    Function* m_realloc;
    Function* m_postReturn;
    Function* m_callback;
};

class LiftedCoreFunction;
#ifdef ENABLE_WASI
class LiftedWasiFunction;
#endif /* ENABLE_WASI */

class LiftedFunction {
public:
    enum Kind {
        CoreFunctionKind,
#ifdef ENABLE_WASI
        WasiFunctionKind,
#endif /* ENABLE_WASI */
    };

    virtual ~LiftedFunction() {}

    virtual Kind kind() const = 0;

    void addRef()
    {
        m_refCount++;
    }

    void releaseRef()
    {
        m_refCount--;
        if (m_refCount == 0) {
            delete this;
        }
    }

    LiftedCoreFunction* asLiftedCoreFunction()
    {
        ASSERT(kind() == CoreFunctionKind);
        return reinterpret_cast<LiftedCoreFunction*>(this);
    }

#ifdef ENABLE_WASI
    LiftedWasiFunction* asLiftedWasiFunction()
    {
        ASSERT(kind() == WasiFunctionKind);
        return reinterpret_cast<LiftedWasiFunction*>(this);
    }
#endif /* ENABLE_WASI */

protected:
    LiftedFunction()
        : m_refCount(1)
    {
    }

private:
    size_t m_refCount;
};

class LiftedCoreFunction : public LiftedFunction {
public:
    LiftedCoreFunction(Function* function, CanonOptions* options)
        : LiftedFunction()
        , m_function(function)
        , m_options(options)
    {
    }

    virtual Kind kind() const override
    {
        return CoreFunctionKind;
    }

    Function* function() const
    {
        return m_function;
    }

    CanonOptions* options() const
    {
        return m_options;
    }

private:
    Function* m_function;
    CanonOptions* m_options;
};

class LoweredFunction : public NativeFunction {
public:
    static LoweredFunction* createLoweredFunction(const FunctionType* functionType, LiftedFunction* liftedFunction, CanonOptions* options);

    virtual Kind kind() const override
    {
        return LoweredFunctionKind;
    }

    CanonOptions* options()
    {
        return m_options;
    }

    virtual void call(ExecutionState& state, Value* argv, Value* result) override;

private:
    LoweredFunction(const FunctionType* functionType, LiftedFunction* liftedFunction, CanonOptions* options)
        : NativeFunction(functionType)
        , m_liftedFunction(liftedFunction)
        , m_options(options)
    {
    }

    LiftedFunction* m_liftedFunction;
    CanonOptions* m_options;
};

class CanonFunction : public NativeFunction {
public:
    enum Type {
        ResourceNew,
        ResourceDrop,
        ResourceRep,
    };

    static CanonFunction* createCanonFunction(Store* store, const FunctionType* functionType, Type type);

    virtual Kind kind() const override
    {
        return CanonFunctionKind;
    }

    Type type() const
    {
        return m_type;
    }

    Store* store() const
    {
        return m_store;
    }

    virtual void call(ExecutionState& state, Value* argv, Value* result) override;

private:
    CanonFunction(const FunctionType* functionType, Type type, Store* store)
        : NativeFunction(functionType)
        , m_type(type)
        , m_store(store)
    {
    }

    Type m_type;
    Store* m_store;
};

class ComponentResourceRep;

class ComponentHandle {
public:
    enum Kind {
        ResourceRepKind,
#ifdef ENABLE_WASI
        ResourceWasiInputStreamKind,
        ResourceWasiOutputStreamKind,
        ResourceWasiPollableKind,
        ResourceWasiTerminalKind,
        ResourceWasiFileKind,
        ResourceWasiDirectoryKind,
#endif /* ENABLE_WASI */
    };

    ~ComponentHandle() {}

    Kind kind()
    {
        return m_kind;
    }

    ComponentResourceRep* asResourceRep()
    {
        ASSERT(kind() == ResourceRepKind);
        return reinterpret_cast<ComponentResourceRep*>(this);
    }

protected:
    ComponentHandle(Kind kind)
        : m_kind(kind)
    {
    }

private:
    Kind m_kind;
};

class ComponentResource : public ComponentHandle {
public:
    ComponentTypeResource* type() const
    {
        return m_type;
    }

protected:
    ComponentResource(Kind kind, ComponentTypeResource* type)
        : ComponentHandle(kind)
        , m_type(type)
    {
    }

private:
    ComponentTypeResource* m_type;
};

class ComponentResourceRep : public ComponentResource {
public:
    ComponentResourceRep(ComponentTypeResource* type, ComponentInstance* instance, uint32_t rep)
        : ComponentResource(ResourceRepKind, type)
        , m_instance(instance)
        , m_rep32(rep)
    {
        ASSERT(!type->i64());
    }

    ComponentResourceRep(ComponentTypeResource* type, ComponentInstance* instance, uint64_t rep)
        : ComponentResource(ResourceRepKind, type)
        , m_instance(instance)
        , m_rep64(rep)
    {
        ASSERT(type->i64());
    }

    ComponentInstance* instance() const
    {
        return m_instance;
    }

    uint32_t rep32() const
    {
        ASSERT(!type()->i64());
        return m_rep32;
    }

    uint64_t rep64() const
    {
        ASSERT(type()->i64());
        return m_rep64;
    }

private:
    ComponentInstance* m_instance;
    union {
        uint32_t m_rep32;
        uint64_t m_rep64;
    };
};

class ComponentInstance : public Object {
#ifdef ENABLE_WASI
    friend class ComponentInstanceWasi02;
#endif /* ENABLE_WASI */

public:
    static ComponentInstance* instantiate(ExecutionState& state, Store* store, Component* component);

    ~ComponentInstance();

    ComponentType* type()
    {
        return m_type;
    }

    Store* store()
    {
        return m_store;
    }

    LiftedFunction* getFunction(uint32_t index)
    {
        return m_funcs[index];
    }

    ComponentInstance* getInstance(uint32_t index)
    {
        return m_instances[index];
    }

    // Handle 0 cannot be used.
    uint32_t appendHandle(ExecutionState& state, ComponentHandle* handle);
    ComponentHandle* getHandle(ExecutionState& state, uint32_t index);
    void removeHandle(uint32_t index);
    static void throwInvalidHandle(ExecutionState& state, uint32_t index);

    bool isBorrowedHandle(uint32_t index)
    {
        ASSERT(index >= FirstHandleIndex && (index - FirstHandleIndex) < m_handles.size()
               && (m_handles[index - FirstHandleIndex] & UnusedSlotMask) == 0);
        return (m_handles[index - FirstHandleIndex] & BorrowedHandleMask) != 0;
    }

private:
    ComponentInstance(Store* store, ComponentType* type);

    class InstantiateContext {
    public:
        InstantiateContext(ExecutionState& state, Store* store)
            : m_state(state)
            , m_store(store)
        {
        }

        ComponentInstance* instantiate(Component* component, ComponentInstance* parent, ComponentInstantiate* arg);

    private:
        ExecutionState& m_state;
        Store* m_store;
    };

    static constexpr uint32_t FirstHandleIndex = 1;
    static constexpr uintptr_t LastHandle = ~static_cast<uintptr_t>(0);
    static constexpr uintptr_t UnusedSlotMask = 0x1;
    static constexpr uintptr_t BorrowedHandleMask = 0x2;
    static constexpr int UnusedSlotShift = 1;

    static ComponentInstance* createInstance(Store* store, ComponentType* type);

    static bool compareTypes(ComponentRefCounted* expected, ComponentRefCounted* provided, std::vector<ComponentRefCounted*>& resources);
    void coreInstantiate(ExecutionState& state, Component* component, ComponentCoreInstantiate* instantiate);
    void aliasExport(ComponentAliasExport* alias);
    void aliasCoreExport(ComponentAliasExport* alias);
    void aliasInline(ComponentAliasInline* alias);
    void liftFunction(std::vector<CanonOptions*>& canonOptions, ComponentCanonLift* lift);
    void lowerFunction(std::vector<CanonOptions*>& canonOptions, ComponentCanonLower* lower);

    ComponentType* m_type;
    Store* m_store;
    uintptr_t m_freeResourceHandle;
    std::vector<Function*> m_coreFuncs;
    std::vector<Table*> m_coreTables;
    std::vector<Memory*> m_coreMemories;
    std::vector<Global*> m_coreGlobals;
    std::vector<Tag*> m_coreTags;
    std::vector<Instance*> m_coreInstances;
    std::vector<LiftedFunction*> m_funcs;
    std::vector<ComponentInstance*> m_instances;
    std::vector<CanonOptions*> m_canonOptions;
    std::vector<uintptr_t> m_handles;
};

} // namespace Walrus

#endif // __WalrusComponentInstance__
