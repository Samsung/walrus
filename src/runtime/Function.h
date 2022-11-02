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

#include "runtime/Value.h"

namespace Walrus {

class Store;
class Instance;
class FunctionType;
class ModuleFunction;
class DefinedFunction;
class ImportedFunction;

class Function : public gc {
public:
    Function(Store* store, FunctionType* functionType)
        : m_store(store)
        , m_functionType(functionType)
    {
    }

    FunctionType* functionType() const { return m_functionType; }

    virtual void call(ExecutionState& state, const uint32_t argc, Value* argv, Value* result) = 0;
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
    virtual ~Function() {}
    Store* m_store;
    FunctionType* m_functionType;
};


class DefinedFunction : public Function {
public:
    DefinedFunction(Store* store,
                    FunctionType* functionType,
                    Instance* instance,
                    ModuleFunction* moduleFunction)
        : Function(store, functionType)
        , m_instance(instance)
        , m_moduleFunction(moduleFunction)
    {
    }

    ModuleFunction* moduleFunction() const { return m_moduleFunction; }
    Instance* instance() const { return m_instance; }

    virtual bool isDefinedFunction() const override
    {
        return true;
    }
    virtual void call(ExecutionState& state, const uint32_t argc, Value* argv, Value* result) override;

protected:
    Instance* m_instance;
    ModuleFunction* m_moduleFunction;
};

class ImportedFunction : public Function {
public:
    typedef void (*ImportedFunctionCallback)(ExecutionState& state, const uint32_t argc, Value* argv, Value* result, void* data);

    ImportedFunction(Store* store,
                     FunctionType* functionType,
                     ImportedFunctionCallback callback,
                     void* data)
        : Function(store, functionType)
        , m_callback(callback)
        , m_data(data)
    {
    }

    virtual bool isImportedFunction() const override
    {
        return true;
    }
    virtual void call(ExecutionState& state, const uint32_t argc, Value* argv, Value* result) override;

protected:
    ImportedFunctionCallback m_callback;
    void* m_data;
};

} // namespace Walrus

#endif // __WalrusFunction__
