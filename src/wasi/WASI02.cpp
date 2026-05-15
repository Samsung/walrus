/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#ifdef ENABLE_WASI

#include "wasi/WASI02.h"
#include "runtime/ComponentInstance.h"
#include "runtime/DefinedFunctionTypes.h"
#include "runtime/Memory.h"
#include "runtime/Store.h"

namespace Walrus {

class ComponentResourceWasiStream : public ComponentResource {
    friend class ComponentResourceWasiPollable;

public:
    ComponentResourceWasiStream(ComponentTypeResource* type, int fileNo)
        : ComponentResource(ResourceWasiStreamKind, type)
        , m_fileNo(fileNo)
        , m_pollableCount(0)
    {
    }

    int fileNo() const
    {
        return m_fileNo;
    }

    size_t pollableCount() const
    {
        return m_pollableCount;
    }

private:
    int m_fileNo;
    size_t m_pollableCount;
};

class ComponentResourceWasiPollable : public ComponentResource {
public:
    ComponentResourceWasiPollable(ComponentTypeResource* type, ComponentResourceWasiStream* stream)
        : ComponentResource(ResourceWasiPollableKind, type)
        , m_stream(stream)
    {
        stream->m_pollableCount++;
    }

    ~ComponentResourceWasiPollable()
    {
        m_stream->m_pollableCount--;
    }

    ComponentResourceWasiStream* stream() const
    {
        return m_stream;
    }

private:
    ComponentResourceWasiStream* m_stream;
};

class ComponentResourceWasiTerminal : public ComponentResource {
public:
    ComponentResourceWasiTerminal(ComponentTypeResource* type, int fileNo)
        : ComponentResource(ResourceWasiTerminalKind, type)
        , m_fileNo(fileNo)
    {
    }

    int fileNo() const
    {
        return m_fileNo;
    }

private:
    int m_fileNo;
};

static ComponentResourceWasiStream* asStream(ComponentHandle* handle)
{
    ASSERT(handle->kind() == ComponentHandle::ResourceWasiStreamKind);
    return reinterpret_cast<ComponentResourceWasiStream*>(handle);
}

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

    LiftedWasiFunction(Type type, ComponentInstance* instance, FunctionType* functionType)
        : LiftedFunction()
        , m_type(type)
        , m_instance(instance)
        , m_functionType(functionType)
    {
    }

    virtual Kind kind() const override
    {
        return WasiFunctionKind;
    }

    Type type() const
    {
        return m_type;
    }

    ComponentInstance* instance() const
    {
        return m_instance;
    }

    FunctionType* functionType() const
    {
        return m_functionType;
    }

private:
    Type m_type;
    ComponentInstance* m_instance;
    FunctionType* m_functionType;
};

class ComponentInstanceWasi02 {
public:
    ComponentInstanceWasi02(Store* store, DefinedFunctionTypes& functionTypes)
        : m_store(store)
        , m_functionTypes(functionTypes)
        , m_type(nullptr)
    {
    }

    ~ComponentInstanceWasi02()
    {
        if (m_type != nullptr) {
            m_type->releaseRef();
        }
    }

    ComponentInstance* loadInstance(std::string& name);

private:
    static void aliasExport(ComponentInstance* instance, const char* name, ComponentRefCounted* type);
    static void addExport(ComponentInstance* instance, const char* name, LiftedWasiFunction::Type type, FunctionType* functionType);
    ComponentInstance* getInstance(const char* name);

    Store* m_store;
    DefinedFunctionTypes& m_functionTypes;
    ComponentType* m_type;
};

void ComponentInstanceWasi02::aliasExport(ComponentInstance* instance, const char* name, ComponentRefCounted* type)
{
    ComponentType* componentType = instance->type();
    type->addRef();
    componentType->pushType(type);
    type->addRef();
    instance->m_type->exports().push_back(ComponentType::External{ name, type, ComponentSort::Type, static_cast<uint32_t>(componentType->types().size()) });
}

void ComponentInstanceWasi02::addExport(ComponentInstance* instance, const char* name, LiftedWasiFunction::Type type, FunctionType* functionType)
{
    instance->m_type->exports().push_back(ComponentType::External{ name, nullptr, ComponentSort::Func, static_cast<uint32_t>(instance->m_funcs.size()) });
    instance->m_funcs.push_back(new LiftedWasiFunction(type, instance, functionType));
}

ComponentInstance* ComponentInstanceWasi02::getInstance(const char* name)
{
    std::string str(name);
    return wasi02LoadInstance(m_store, m_functionTypes, str);
}

ComponentInstance* ComponentInstanceWasi02::loadInstance(std::string& name)
{
    if (name == "wasi:cli/exit@0.2.6") {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);
        addExport(instance, "exit", LiftedWasiFunction::cliExit026, m_functionTypes[DefinedFunctionTypes::I32R]);
        return instance;
    }
    if (name == "wasi:io/error@0.2.6") {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);
        m_type->pushType(new ComponentTypeResource(false, ComponentTypeResource::NotDefined)); /* 0 */
        return instance;
    }
    if (name == "wasi:io/poll@0.2.6") {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);
        m_type->pushType(new ComponentTypeResource(false, ComponentTypeResource::NotDefined)); /* 0 */
        addExport(instance, "[method]pollable.block", LiftedWasiFunction::ioPollableBlock026, m_functionTypes[DefinedFunctionTypes::I32R]);
        return instance;
    }
    if (name == "wasi:io/streams@0.2.6") {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);
        m_type->pushType(new ComponentTypeResource(false, ComponentTypeResource::NotDefined)); /* 0 */
        m_type->pushType(new ComponentTypeResource(false, ComponentTypeResource::NotDefined)); /* 1 */

        ComponentInstance* errorIntance = getInstance("wasi:io/error@0.2.6");
        instance->m_instances.push_back(errorIntance);
        aliasExport(instance, "error", errorIntance->type()->getType(0)); /* 2 */
        ComponentInstance* pollIntance = getInstance("wasi:io/poll@0.2.6");
        instance->m_instances.push_back(pollIntance);
        aliasExport(instance, "pollable", pollIntance->type()->getType(0)); /* 3 */
        addExport(instance, "[method]output-stream.check-write", LiftedWasiFunction::ioOutputStreamCheckWrite026, m_functionTypes[DefinedFunctionTypes::I32I32R]);
        addExport(instance, "[method]output-stream.write", LiftedWasiFunction::ioOutputStreamWrite026, m_functionTypes[DefinedFunctionTypes::I32I32I32I32R]);
        addExport(instance, "[method]output-stream.blocking-write-and-flush", LiftedWasiFunction::ioOutputStreamBlockingWriteAndFlush026, m_functionTypes[DefinedFunctionTypes::I32I32I32I32R]);
        addExport(instance, "[method]output-stream.blocking-flush", LiftedWasiFunction::ioOutputStreamBlockingFlush026, m_functionTypes[DefinedFunctionTypes::I32I32R]);
        addExport(instance, "[method]output-stream.subscribe", LiftedWasiFunction::ioOutputStreamSubscribe026, m_functionTypes[DefinedFunctionTypes::I32_RI32]);
        return instance;
    }
    if (name == "wasi:cli/stdin@0.2.6") {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

        ComponentInstance* streamsIntance = getInstance("wasi:io/streams@0.2.6");
        instance->m_instances.push_back(streamsIntance);
        aliasExport(instance, "input-stream", streamsIntance->type()->getType(0)); /* 0 */
        addExport(instance, "get-stdin", LiftedWasiFunction::cliGetStdin026, m_functionTypes[DefinedFunctionTypes::RI32]);
        return instance;
    }
    if (name == "wasi:cli/stdout@0.2.6") {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

        ComponentInstance* streamsIntance = getInstance("wasi:io/streams@0.2.6");
        instance->m_instances.push_back(streamsIntance);
        aliasExport(instance, "output-stream", streamsIntance->type()->getType(1)); /* 0 */
        addExport(instance, "get-stdout", LiftedWasiFunction::cliGetStdout026, m_functionTypes[DefinedFunctionTypes::RI32]);
        return instance;
    }
    if (name == "wasi:cli/stderr@0.2.6") {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

        ComponentInstance* streamsIntance = getInstance("wasi:io/streams@0.2.6");
        instance->m_instances.push_back(streamsIntance);
        aliasExport(instance, "output-stream", streamsIntance->type()->getType(1)); /* 0 */
        addExport(instance, "get-stderr", LiftedWasiFunction::cliGetStderr026, m_functionTypes[DefinedFunctionTypes::RI32]);
        return instance;
    }
    if (name == "wasi:cli/terminal-input@0.2.6") {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);
        m_type->pushType(new ComponentTypeResource(false, ComponentTypeResource::NotDefined)); /* 0 */
        return instance;
    }
    if (name == "wasi:cli/terminal-output@0.2.6") {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);
        m_type->pushType(new ComponentTypeResource(false, ComponentTypeResource::NotDefined)); /* 0 */
        return instance;
    }
    if (name == "wasi:cli/terminal-stdin@0.2.6") {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

        ComponentInstance* inputIntance = getInstance("wasi:cli/terminal-input@0.2.6");
        instance->m_instances.push_back(inputIntance);
        aliasExport(instance, "terminal-input", inputIntance->type()->getType(0)); /* 0 */
        addExport(instance, "get-terminal-stdin", LiftedWasiFunction::cliGetTerminalStdin026, m_functionTypes[DefinedFunctionTypes::I32R]);
        return instance;
    }
    if (name == "wasi:cli/terminal-stdout@0.2.6") {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

        ComponentInstance* outputIntance = getInstance("wasi:cli/terminal-output@0.2.6");
        instance->m_instances.push_back(outputIntance);
        aliasExport(instance, "terminal-output", outputIntance->type()->getType(0)); /* 0 */
        addExport(instance, "get-terminal-stdout", LiftedWasiFunction::cliGetTerminalStdout026, m_functionTypes[DefinedFunctionTypes::I32R]);
        return instance;
    }
    if (name == "wasi:cli/terminal-stderr@0.2.6") {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

        ComponentInstance* outputIntance = getInstance("wasi:cli/terminal-output@0.2.6");
        instance->m_instances.push_back(outputIntance);
        aliasExport(instance, "terminal-output", outputIntance->type()->getType(0)); /* 0 */
        addExport(instance, "get-terminal-stderr", LiftedWasiFunction::cliGetTerminalStderr026, m_functionTypes[DefinedFunctionTypes::I32R]);
        return instance;
    }
    return nullptr;
}

ComponentInstance* wasi02LoadInstance(Store* store, DefinedFunctionTypes& functionTypes, std::string& name)
{
    ComponentInstance* instance = store->findComponentInstance(name);
    if (instance != nullptr) {
        return instance;
    }

    ComponentInstanceWasi02 instanceCreator(store, functionTypes);
    instance = instanceCreator.loadInstance(name);
    store->registerComponentInstance(name, instance);
    return instance;
}

const FunctionType* getWasiFunctionType(LiftedWasiFunction* function)
{
    return function->functionType();
}

void callWasiFunction(ExecutionState& state, Value* argv, Value* result, LiftedWasiFunction* function, CanonOptions* options)
{
    ComponentInstance* instance = function->instance();

    switch (function->type()) {
    case LiftedWasiFunction::ioOutputStreamSubscribe026: {
        uint32_t index = argv[0].asI32();
        ComponentHandle* handle = options->instance()->getHandle(state, index);
        if (handle->kind() != ComponentHandle::ResourceWasiStreamKind) {
            ComponentInstance::throwInvalidHandle(state, index);
        }

        ComponentResource* resource = new ComponentResourceWasiPollable(instance->type()->getType(0)->asTypeResource(), asStream(handle));
        result[0] = Value(static_cast<int32_t>(options->instance()->appendHandle(state, resource)));
        break;
    }
    case LiftedWasiFunction::cliGetStdout026: {
        ComponentResource* resource = new ComponentResourceWasiStream(instance->type()->getType(0)->asTypeResource(), 1);
        result[0] = Value(static_cast<int32_t>(options->instance()->appendHandle(state, resource)));
        break;
    }
    case LiftedWasiFunction::cliGetTerminalStdout026: {
        ComponentResource* resource = new ComponentResourceWasiTerminal(instance->type()->getType(0)->asTypeResource(), 1);
        uint32_t offset = argv[0].asI32();
        uint32_t value = options->instance()->appendHandle(state, resource);
        options->memory()->store(state, offset, 4, value);
        options->memory()->buffer()[offset] = 1;
        break;
    }
    default:
        std::string message = "unimplemented wasi function";
        Trap::throwException(state, message);
        break;
    }
}

bool dropWasiResource(ExecutionState& state, ComponentHandle* handle)
{
    switch (handle->kind()) {
    case ComponentHandle::ResourceWasiStreamKind:
        if (asStream(handle)->pollableCount() != 0) {
            std::string message = "stream cannot be destroyed (has assigned pollable)";
            Trap::throwException(state, message);
        }
        break;
    case ComponentHandle::ResourceWasiPollableKind:
        break;
    case ComponentHandle::ResourceWasiTerminalKind:
        break;
    default:
        return false;
    }
    return true;
}

} // namespace Walrus

#endif
