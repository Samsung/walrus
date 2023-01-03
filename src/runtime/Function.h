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
#include "runtime/Trap.h"
#include "runtime/Object.h"

namespace Walrus {

class Instance;
class FunctionType;
class ModuleFunction;
class DefinedFunction;
class ImportedFunction;

class Function : public Object {
public:
    Function(FunctionType* functionType)
        : m_functionType(functionType)
    {
    }

    virtual Object::Kind kind() const override
    {
        return Object::FunctionKind;
    }

    virtual bool isFunction() const override
    {
        return true;
    }

    const FunctionType* functionType() const { return m_functionType; }

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
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
    static inline void* currentStackPointer()
    {
        return __builtin_frame_address(0);
    }
#elif defined(COMPILER_MSVC)
    static inline void* currentStackPointer()
    {
        volatile int temp;
        return (void*)&temp;
    }
#else
#error
#endif

    static ALWAYS_INLINE void checkStackLimit(ExecutionState& state)
    {
        if (UNLIKELY(state.stackLimit()
#ifdef STACK_GROWS_DOWN
                     >
#else
                     <
#endif
                     (size_t)currentStackPointer())) {
            Trap::throwException(state, "call stack exhausted");
        }
    }
    virtual ~Function() {}
    const FunctionType* m_functionType;
};


class DefinedFunction : public Function {
public:
    DefinedFunction(Instance* instance,
                    ModuleFunction* moduleFunction);

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

    ImportedFunction(FunctionType* functionType,
                     ImportedFunctionCallback callback,
                     void* data)
        : Function(functionType)
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
