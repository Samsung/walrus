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

#ifndef __WalrusFunction__
#define __WalrusFunction__

#include "util/Util.h"
#include "runtime/Value.h"
#include "runtime/Trap.h"
#include "runtime/Object.h"

#ifdef STACK_GROWS_DOWN
#define CHECK_STACK_LIMIT(state)                                        \
    if (UNLIKELY(state.stackLimit() > (size_t)currentStackPointer())) { \
        Trap::throwException(state, "call stack exhausted");            \
    }
#else
#define CHECK_STACK_LIMIT(state)                                        \
    if (UNLIKELY(state.stackLimit() < (size_t)currentStackPointer())) { \
        Trap::throwException(state, "call stack exhausted");            \
    }
#endif


namespace Walrus {

class Store;
class Instance;
class FunctionType;
class ModuleFunction;
class DefinedFunction;
class ImportedFunction;
class LoweredFunction;
class WasiFunction;

class Function : public Extern {
public:
    enum Kind {
        DefinedFunctionKind,
        ImportedFunctionKind,
        WasiFunctionKind,
        // Function types used by component support.
        LoweredFunctionKind,
        CanonFunctionKind,
        WasiResourceReleaseKind,
    };

    const FunctionType* functionType() const { return m_functionType; }

    virtual Kind kind() const = 0;
    virtual void call(ExecutionState& state, Value* argv, Value* result) = 0;
    virtual void interpreterCall(ExecutionState& state, uint8_t* bp, ByteCodeStackOffset* offsets,
                                 uint16_t parameterOffsetCount, uint16_t resultOffsetCount)
        = 0;

    DefinedFunction* asDefinedFunction()
    {
        ASSERT(kind() == DefinedFunctionKind);
        return reinterpret_cast<DefinedFunction*>(this);
    }

    LoweredFunction* asLoweredFunction()
    {
        ASSERT(kind() == LoweredFunctionKind);
        return reinterpret_cast<LoweredFunction*>(this);
    }

    WasiFunction* asWasiFunction()
    {
        ASSERT(kind() == WasiFunctionKind);
        return reinterpret_cast<WasiFunction*>(this);
    }

protected:
    Function(const FunctionType* functionType);

    const FunctionType* m_functionType;
};

class DefinedFunction : public Function {
    friend class Module;

public:
    static DefinedFunction* createDefinedFunction(Store* store,
                                                  Instance* instance,
                                                  ModuleFunction* moduleFunction);

    ModuleFunction* moduleFunction() const { return m_moduleFunction; }
    Instance* instance() const { return m_instance; }

    virtual Kind kind() const override
    {
        return DefinedFunctionKind;
    }

    virtual void call(ExecutionState& state, Value* argv, Value* result) override;
    virtual void interpreterCall(ExecutionState& state, uint8_t* bp, ByteCodeStackOffset* offsets,
                                 uint16_t parameterOffsetCount, uint16_t resultOffsetCount) override;

protected:
    DefinedFunction(Instance* instance,
                    ModuleFunction* moduleFunction);

    Instance* m_instance;
    ModuleFunction* m_moduleFunction;
};

class NativeFunction : public Function {
public:
    virtual void interpreterCall(ExecutionState& state, uint8_t* bp, ByteCodeStackOffset* offsets,
                                 uint16_t parameterOffsetCount, uint16_t resultOffsetCount) override;

protected:
    NativeFunction(const FunctionType* functionType)
        : Function(functionType)
    {
    }
};

class ImportedFunction : public NativeFunction {
public:
    typedef std::function<void(ExecutionState& state, Value* argv, Value* result, void* data)> ImportedFunctionCallback;

    static ImportedFunction* createImportedFunction(Store* store,
                                                    FunctionType* functionType,
                                                    ImportedFunctionCallback callback,
                                                    void* data);

    virtual Kind kind() const override
    {
        return ImportedFunctionKind;
    }

    virtual void call(ExecutionState& state, Value* argv, Value* result) override;

protected:
    ImportedFunction(FunctionType* functionType,
                     ImportedFunctionCallback callback,
                     void* data)
        : NativeFunction(functionType)
        , m_callback(callback)
        , m_data(data)
    {
    }

    ImportedFunctionCallback m_callback;
    void* m_data;
};

class WasiFunction : public NativeFunction {
public:
    typedef std::function<void(ExecutionState& state, Value* argv, Value* result, Instance* instance)> WasiFunctionCallback;

    static WasiFunction* createWasiFunction(Store* store,
                                            FunctionType* functionType,
                                            WasiFunctionCallback callback);

    virtual Kind kind() const override
    {
        return WasiFunctionKind;
    }

    void setRunningInstance(Instance* instance)
    {
        m_runningInstance = instance;
    }

    virtual void call(ExecutionState& state, Value* argv, Value* result) override;

protected:
    WasiFunction(FunctionType* functionType,
                 WasiFunctionCallback callback)
        : NativeFunction(functionType)
        , m_callback(callback)
        , m_runningInstance(nullptr)
    {
    }

    WasiFunctionCallback m_callback;
    Instance* m_runningInstance;
};

} // namespace Walrus

#endif // __WalrusFunction__
