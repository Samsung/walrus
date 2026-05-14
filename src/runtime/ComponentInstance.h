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

#ifdef ENABLE_WASI

class LiftedWasiFunction : public LiftedFunction {
public:
    enum Type {
        cliExit026,
        ioPollableBlock026,
        ioOutputStreamCheckWrite026,
        ioOutputStreamWrite026,
        ioOutputStreamBlockingWriteAndFlush026,
        ioOutputStreamBlockingFlush026,
        ioOutputStreamSubscribe026,
        cliGetStdin026,
        cliGetStdout026,
        cliGetStderr026,
        cliGetTerminalStdin026,
        cliGetTerminalStdout026,
        cliGetTerminalStderr026,
    };

    LiftedWasiFunction(Type type, FunctionType* functionType)
        : LiftedFunction()
        , m_type(type)
        , m_functionType(functionType)
    {
    }

    virtual Kind kind() const override
    {
        return WasiFunctionKind;
    }

    FunctionType* functionType() const
    {
        return m_functionType;
    }

private:
    Type m_type;
    FunctionType* m_functionType;
};

#endif /* ENABLE_WASI */

class LoweredFunction : public NativeFunction {
public:
    static LoweredFunction* createLoweredFunction(Store* store, const FunctionType* functionType, LiftedFunction* liftedFunction, CanonOptions* options);

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

    Type type() const
    {
        return m_type;
    }

    virtual Kind kind() const override
    {
        return CanonFunctionKind;
    }

    virtual void call(ExecutionState& state, Value* argv, Value* result) override;

private:
    CanonFunction(const FunctionType* functionType, Type type)
        : NativeFunction(functionType)
        , m_type(type)
    {
    }

    Type m_type;
};

class DefinedFunctionTypes;

class ComponentInstance : public Object {
#ifdef ENABLE_WASI
    friend class ComponentInstanceWasi02;
#endif /* ENABLE_WASI */

public:
    static ComponentInstance* instantiate(ExecutionState& state, Store* store, DefinedFunctionTypes& functionTypes, Component* component);

    ~ComponentInstance();

    ComponentType* type()
    {
        return m_type;
    }

    LiftedFunction* getFunction(uint32_t index)
    {
        return m_funcs[index];
    }

    ComponentInstance* getInstance(uint32_t index)
    {
        return m_instances[index];
    }

private:
    ComponentInstance(ComponentType* type);

    class InstantiateContext {
    public:
        InstantiateContext(ExecutionState& state, Store* store, DefinedFunctionTypes& functionTypes)
            : m_state(state)
            , m_store(store)
            , m_functionTypes(functionTypes)
        {
        }

        ComponentInstance* instantiate(Component* component, ComponentInstance* parent, ComponentInstantiate* arg);

    private:
        ExecutionState& m_state;
        Store* m_store;
        DefinedFunctionTypes& m_functionTypes;
    };

    void coreInstantiate(ExecutionState& state, Store* store, Component* component, ComponentCoreInstantiate* instantiate);
    void aliasExport(ComponentAliasExport* alias);
    void aliasCoreExport(ComponentAliasExport* alias);
    void aliasInline(ComponentAliasInline* alias);
    void liftFunction(Store* store, std::vector<CanonOptions*>& canonOptions, ComponentCanonLift* lift);
    void lowerFunction(Store* store, std::vector<CanonOptions*>& canonOptions, ComponentCanonLower* lower);

    ComponentType* m_type;
    std::vector<Function*> m_coreFuncs;
    std::vector<Table*> m_coreTables;
    std::vector<Memory*> m_coreMemories;
    std::vector<Global*> m_coreGlobals;
    std::vector<Tag*> m_coreTags;
    std::vector<Instance*> m_coreInstances;
    std::vector<LiftedFunction*> m_funcs;
    std::vector<ComponentInstance*> m_instances;
    std::vector<CanonOptions*> m_canonOptions;
};

} // namespace Walrus

#endif // __WalrusComponentInstance__
