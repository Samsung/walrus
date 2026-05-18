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
#include "runtime/Memory.h"
#include "runtime/Store.h"

namespace Walrus {

class ComponentResourceWasiStream : public ComponentResource {
    friend class ComponentResourceWasiPollable;

public:
    ComponentResourceWasiStream(ComponentTypeResource* type, FILE* file)
        : ComponentResource(ResourceWasiStreamKind, type)
        , m_file(file)
        , m_pollableCount(0)
    {
    }

    FILE* file() const
    {
        return m_file;
    }

    size_t pollableCount() const
    {
        return m_pollableCount;
    }

private:
    FILE* m_file;
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
        cliExit02,
        ioPollableBlock02,
        ioOutputStreamCheckWrite02,
        ioOutputStreamWrite02,
        ioOutputStreamBlockingWriteAndFlush02,
        ioOutputStreamBlockingFlush02,
        ioOutputStreamSubscribe02,
        cliGetStdin02,
        cliGetStdout02,
        cliGetStderr02,
        cliGetTerminalStdin02,
        cliGetTerminalStdout02,
        cliGetTerminalStderr02,
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
    ComponentInstanceWasi02(Store* store)
        : m_store(store)
        , m_type(nullptr)
    {
    }

    ~ComponentInstanceWasi02()
    {
        if (m_type != nullptr) {
            m_type->releaseRef();
        }
    }

    ComponentInstance* loadInstance(const char* name, size_t length);

private:
    FunctionType* getType(Store::DefinedFunctionType type)
    {
        return m_store->getDefinedFunctionType(type);
    }

    static void aliasExport(ComponentInstance* instance, const char* name, ComponentRefCounted* type);
    static void addExport(ComponentInstance* instance, const char* name, LiftedWasiFunction::Type type, FunctionType* functionType);
    ComponentInstance* getInstance(const char* name, const char* postfix, size_t postfixLength);

    Store* m_store;
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

ComponentInstance* ComponentInstanceWasi02::getInstance(const char* name, const char* postfix, size_t postfixLength)
{
    size_t length = strlen(name);
    ASSERT(length < 62);
    char buffer[64];
    memcpy(buffer, name, length);
    memcpy(buffer + length, postfix, postfixLength);
    std::string str(buffer, length + postfixLength);
    return wasi02LoadInstance(m_store, str);
}

static bool compareName(const char* name, size_t length, const char* expected)
{
    if (strlen(expected) != length) {
        return false;
    }
    return memcmp(name, expected, length) == 0;
}

ComponentInstance* ComponentInstanceWasi02::loadInstance(const char* name, size_t length)
{
    if (length < 12 || memcmp(name, "wasi:", 5) != 0) {
        return nullptr;
    }

    if (name[length - 1] < '0' || name[length - 1] > '9') {
        return nullptr;
    }

    size_t postfixLength = 1;
    if (name[length - 2] != '.') {
        postfixLength = 2;
        if (name[length - 1] > '1' || name[length - 2] != '1' || name[length - 3] != '.') {
            return nullptr;
        }
    }

    length -= postfixLength;
    const char* postfix = name + length;

    if (memcmp(name + length - 5, "@0.2.", 5) != 0) {
        return nullptr;
    }
    length -= 5;

    if (length > 8 && memcmp(name, "wasi:io/", 8) == 0) {
        name += 8;
        length -= 8;

        if (compareName(name, length, "error")) {
            m_type = new ComponentType(ComponentType::ComponentTypeKind);
            ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);
            m_type->pushType(new ComponentTypeResource(false, ComponentTypeResource::NotDefined)); /* 0 */
            return instance;
        }
        if (compareName(name, length, "poll")) {
            m_type = new ComponentType(ComponentType::ComponentTypeKind);
            ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);
            m_type->pushType(new ComponentTypeResource(false, ComponentTypeResource::NotDefined)); /* 0 */
            addExport(instance, "[method]pollable.block", LiftedWasiFunction::ioPollableBlock02, getType(Store::I32R));
            return instance;
        }
        if (compareName(name, length, "streams")) {
            m_type = new ComponentType(ComponentType::ComponentTypeKind);
            ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);
            m_type->pushType(new ComponentTypeResource(false, ComponentTypeResource::NotDefined)); /* 0 */
            m_type->pushType(new ComponentTypeResource(false, ComponentTypeResource::NotDefined)); /* 1 */

            ComponentInstance* errorIntance = getInstance("wasi:io/error@0.2.", postfix, postfixLength);
            instance->m_instances.push_back(errorIntance);
            aliasExport(instance, "error", errorIntance->type()->getType(0)); /* 2 */
            ComponentInstance* pollIntance = getInstance("wasi:io/poll@0.2.", postfix, postfixLength);
            instance->m_instances.push_back(pollIntance);
            aliasExport(instance, "pollable", pollIntance->type()->getType(0)); /* 3 */
            addExport(instance, "[method]output-stream.check-write", LiftedWasiFunction::ioOutputStreamCheckWrite02, getType(Store::I32I32R));
            addExport(instance, "[method]output-stream.write", LiftedWasiFunction::ioOutputStreamWrite02, getType(Store::I32I32I32I32R));
            addExport(instance, "[method]output-stream.blocking-write-and-flush", LiftedWasiFunction::ioOutputStreamBlockingWriteAndFlush02, getType(Store::I32I32I32I32R));
            addExport(instance, "[method]output-stream.blocking-flush", LiftedWasiFunction::ioOutputStreamBlockingFlush02, getType(Store::I32I32R));
            addExport(instance, "[method]output-stream.subscribe", LiftedWasiFunction::ioOutputStreamSubscribe02, getType(Store::I32_RI32));
            return instance;
        }
        return nullptr;
    }

    if (length > 9 && memcmp(name, "wasi:cli/", 9) == 0) {
        name += 9;
        length -= 9;

        if (compareName(name, length, "exit")) {
            m_type = new ComponentType(ComponentType::ComponentTypeKind);
            ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);
            addExport(instance, "exit", LiftedWasiFunction::cliExit02, getType(Store::I32R));
            return instance;
        }
        if (compareName(name, length, "stdin")) {
            m_type = new ComponentType(ComponentType::ComponentTypeKind);
            ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

            ComponentInstance* streamsIntance = getInstance("wasi:io/streams@0.2.", postfix, postfixLength);
            instance->m_instances.push_back(streamsIntance);
            aliasExport(instance, "input-stream", streamsIntance->type()->getType(0)); /* 0 */
            addExport(instance, "get-stdin", LiftedWasiFunction::cliGetStdin02, getType(Store::RI32));
            return instance;
        }
        if (compareName(name, length, "stdout")) {
            m_type = new ComponentType(ComponentType::ComponentTypeKind);
            ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

            ComponentInstance* streamsIntance = getInstance("wasi:io/streams@0.2.", postfix, postfixLength);
            instance->m_instances.push_back(streamsIntance);
            aliasExport(instance, "output-stream", streamsIntance->type()->getType(1)); /* 0 */
            addExport(instance, "get-stdout", LiftedWasiFunction::cliGetStdout02, getType(Store::RI32));
            return instance;
        }
        if (compareName(name, length, "stderr")) {
            m_type = new ComponentType(ComponentType::ComponentTypeKind);
            ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

            ComponentInstance* streamsIntance = getInstance("wasi:io/streams@0.2.", postfix, postfixLength);
            instance->m_instances.push_back(streamsIntance);
            aliasExport(instance, "output-stream", streamsIntance->type()->getType(1)); /* 0 */
            addExport(instance, "get-stderr", LiftedWasiFunction::cliGetStderr02, getType(Store::RI32));
            return instance;
        }
        if (compareName(name, length, "terminal-input")) {
            m_type = new ComponentType(ComponentType::ComponentTypeKind);
            ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);
            m_type->pushType(new ComponentTypeResource(false, ComponentTypeResource::NotDefined)); /* 0 */
            return instance;
        }
        if (compareName(name, length, "terminal-output")) {
            m_type = new ComponentType(ComponentType::ComponentTypeKind);
            ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);
            m_type->pushType(new ComponentTypeResource(false, ComponentTypeResource::NotDefined)); /* 0 */
            return instance;
        }
        if (compareName(name, length, "terminal-stdin")) {
            m_type = new ComponentType(ComponentType::ComponentTypeKind);
            ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

            ComponentInstance* inputIntance = getInstance("wasi:cli/terminal-input@0.2.", postfix, postfixLength);
            instance->m_instances.push_back(inputIntance);
            aliasExport(instance, "terminal-input", inputIntance->type()->getType(0)); /* 0 */
            addExport(instance, "get-terminal-stdin", LiftedWasiFunction::cliGetTerminalStdin02, getType(Store::I32R));
            return instance;
        }
        if (compareName(name, length, "terminal-stdout")) {
            m_type = new ComponentType(ComponentType::ComponentTypeKind);
            ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

            ComponentInstance* outputIntance = getInstance("wasi:cli/terminal-output@0.2.", postfix, postfixLength);
            instance->m_instances.push_back(outputIntance);
            aliasExport(instance, "terminal-output", outputIntance->type()->getType(0)); /* 0 */
            addExport(instance, "get-terminal-stdout", LiftedWasiFunction::cliGetTerminalStdout02, getType(Store::I32R));
            return instance;
        }
        if (compareName(name, length, "terminal-stderr")) {
            m_type = new ComponentType(ComponentType::ComponentTypeKind);
            ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

            ComponentInstance* outputIntance = getInstance("wasi:cli/terminal-output@0.2.", postfix, postfixLength);
            instance->m_instances.push_back(outputIntance);
            aliasExport(instance, "terminal-output", outputIntance->type()->getType(0)); /* 0 */
            addExport(instance, "get-terminal-stderr", LiftedWasiFunction::cliGetTerminalStderr02, getType(Store::I32R));
            return instance;
        }
        return nullptr;
    }
    return nullptr;
}

ComponentInstance* wasi02LoadInstance(Store* store, std::string& name)
{
    ComponentInstance* instance = store->findComponentInstance(name);
    if (instance != nullptr) {
        return instance;
    }

    ComponentInstanceWasi02 instanceCreator(store);
    instance = instanceCreator.loadInstance(name.data(), name.length());
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
    case LiftedWasiFunction::ioOutputStreamCheckWrite02: {
        uint32_t offset = argv[1].asI32();
        uint64_t value = 0xffffffff;
        options->memory()->store(state, offset, 8, value);
        options->memory()->buffer()[offset] = 0;
        break;
    }
    case LiftedWasiFunction::ioOutputStreamWrite02: {
        uint32_t index = argv[0].asI32();
        ComponentHandle* handle = options->instance()->getHandle(state, index);
        if (handle->kind() != ComponentHandle::ResourceWasiStreamKind) {
            ComponentInstance::throwInvalidHandle(state, index);
        }

        uint32_t bufferStart = argv[1].asI32();
        uint32_t bufferLength = argv[2].asI32();
        fwrite(options->memory()->buffer() + bufferStart, bufferLength, 1, asStream(handle)->file());

        uint32_t offset = argv[3].asI32();
        options->memory()->buffer()[offset] = 0;
        break;
    }
    case LiftedWasiFunction::ioOutputStreamBlockingFlush02: {
        uint32_t offset = argv[1].asI32();
        options->memory()->buffer()[offset] = 0;
        break;
    }
    case LiftedWasiFunction::ioOutputStreamSubscribe02: {
        uint32_t index = argv[0].asI32();
        ComponentHandle* handle = options->instance()->getHandle(state, index);
        if (handle->kind() != ComponentHandle::ResourceWasiStreamKind) {
            ComponentInstance::throwInvalidHandle(state, index);
        }

        ComponentResource* resource = new ComponentResourceWasiPollable(instance->type()->getType(0)->asTypeResource(), asStream(handle));
        result[0] = Value(static_cast<int32_t>(options->instance()->appendHandle(state, resource)));
        break;
    }
    case LiftedWasiFunction::cliGetStdout02: {
        ComponentResource* resource = new ComponentResourceWasiStream(instance->type()->getType(0)->asTypeResource(), stdout);
        result[0] = Value(static_cast<int32_t>(options->instance()->appendHandle(state, resource)));
        break;
    }
    case LiftedWasiFunction::cliGetTerminalStdout02: {
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
