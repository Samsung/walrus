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

class Function : public Extern {
public:
    virtual Object::Kind kind() const override
    {
        return Object::FunctionKind;
    }

    virtual bool isFunction() const override
    {
        return true;
    }

    const FunctionType* functionType() const { return m_functionType; }

    virtual void call(ExecutionState& state, Value* argv, Value* result) = 0;
    virtual void interpreterCall(ExecutionState& state, uint8_t* bp, ByteCodeStackOffset* offsets,
                                 uint16_t parameterOffsetCount, uint16_t resultOffsetCount)
        = 0;

    virtual bool isDefinedFunction() const
    {
        return false;
    }
    virtual bool isImportedFunction() const
    {
        return false;
    }

    DefinedFunction* asDefinedFunction()
    {
        ASSERT(isDefinedFunction());
        return reinterpret_cast<DefinedFunction*>(this);
    }

    ImportedFunction* asImportedFunction()
    {
        ASSERT(isImportedFunction());
        return reinterpret_cast<ImportedFunction*>(this);
    }

protected:
    Function(FunctionType* functionType)
        : m_functionType(functionType)
    {
    }

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

    virtual bool isDefinedFunction() const override
    {
        return true;
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

class DefinedFunctionWithTryCatch : public DefinedFunction {
    friend class DefinedFunction;
    friend class Module;

public:
    virtual void interpreterCall(ExecutionState& state, uint8_t* bp, ByteCodeStackOffset* offsets,
                                 uint16_t parameterOffsetCount, uint16_t resultOffsetCount) override;

protected:
    DefinedFunctionWithTryCatch(Instance* instance,
                                ModuleFunction* moduleFunction)
        : DefinedFunction(instance, moduleFunction)
    {
    }
};

class ImportedFunction : public Function {
public:
    typedef std::function<void(ExecutionState& state, Value* argv, Value* result, void* data)> ImportedFunctionCallback;

    static ImportedFunction* createImportedFunction(Store* store,
                                                    FunctionType* functionType,
                                                    ImportedFunctionCallback callback,
                                                    void* data);

    virtual bool isImportedFunction() const override
    {
        return true;
    }
    virtual void call(ExecutionState& state, Value* argv, Value* result) override;
    virtual void interpreterCall(ExecutionState& state, uint8_t* bp, ByteCodeStackOffset* offsets,
                                 uint16_t parameterOffsetCount, uint16_t resultOffsetCount) override;

protected:
    ImportedFunction(FunctionType* functionType,
                     ImportedFunctionCallback callback,
                     void* data)
        : Function(functionType)
        , m_callback(callback)
        , m_data(data)
    {
    }

    ImportedFunctionCallback m_callback;
    void* m_data;
};

} // namespace Walrus

#endif // __WalrusFunction__
