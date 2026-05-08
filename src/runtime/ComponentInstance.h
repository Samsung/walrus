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

#ifdef ENABLE_WASI
class LiftedWasiFunction;
#endif /* ENABLE_WASI */

class LiftedFunction {
public:
    enum Type {
        CoreFunction,
#ifdef ENABLE_WASI
        WasiFunction,
#endif /* ENABLE_WASI */
    };

    virtual ~LiftedFunction() {}

    Type type() const
    {
        return m_type;
    }

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

#ifdef ENABLE_WASI
    LiftedWasiFunction* asLiftedWasiFunction()
    {
        ASSERT(type() == WasiFunction);
        return reinterpret_cast<LiftedWasiFunction*>(this);
    }
#endif /* ENABLE_WASI */

protected:
    LiftedFunction(Type type)
        : m_type(type)
        , m_refCount(1)
    {
    }

private:
    Type m_type;
    size_t m_refCount;
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
        : LiftedFunction(WasiFunction)
        , m_type(type)
        , m_functionType(functionType)
    {
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
    static LoweredFunction* createLoweredFunction(Store* store, FunctionType* functionType, LiftedFunction* liftedFunction);

    virtual void call(ExecutionState& state, Value* argv, Value* result) override;

private:
    LoweredFunction(FunctionType* functionType, LiftedFunction* liftedFunction)
        : NativeFunction(functionType)
        , m_liftedFunction(liftedFunction)
    {
    }

    LiftedFunction* m_liftedFunction;
};

class CanonFunction : public NativeFunction {
public:
    enum Type {
        ResourceNew,
        ResourceDrop,
        ResourceRep,
    };

    static CanonFunction* createCanonFunction(Store* store, FunctionType* functionType, Type type);

    Type type() const
    {
        return m_type;
    }

    virtual void call(ExecutionState& state, Value* argv, Value* result) override;

private:
    CanonFunction(FunctionType* functionType, Type type)
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

private:
    ComponentInstance(ComponentType* type);

    void coreInstantiate(ExecutionState& state, Store* store, Component* component, ComponentCoreInstantiate* instantiate);
    void aliasExport(ComponentAliasExport* alias);
    void aliasCoreExport(ComponentAliasExport* alias);
    void aliasInline(ComponentAliasInline* alias);
    void lowerFunction(Store* store, ComponentCanonLower* lower);

    ComponentType* m_type;
    std::vector<Function*> m_coreFuncs;
    std::vector<Table*> m_coreTables;
    std::vector<Memory*> m_coreMemories;
    std::vector<Global*> m_coreGlobals;
    std::vector<Tag*> m_coreTags;
    std::vector<Instance*> m_coreInstances;
    std::vector<LiftedFunction*> m_funcs;
    std::vector<ComponentInstance*> m_instances;
};

} // namespace Walrus

#endif // __WalrusComponentInstance__
