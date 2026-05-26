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

enum WasiNamedInstances : size_t {
    instanceUnknown,
    instanceIoError02,
    instanceIoPoll02,
    instanceIoStreams02,
    instanceCliExit02,
    instanceCliStdin02,
    instanceCliStdout02,
    instanceCliStderr02,
    instanceCliTerminalInput02,
    instanceCliTerminalOutput02,
    instanceCliTerminalStdin02,
    instanceCliTerminalStdout02,
    instanceCliTerminalStderr02,
};

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

    ComponentInstance* loadInstance(size_t instanceId, bool useCache = true);

private:
    FunctionType* getType(Store::DefinedFunctionType type)
    {
        return m_store->getDefinedFunctionType(type);
    }

    static void aliasTypeExport(ComponentInstance* instance, const char* name, ComponentRefCounted* type);
    static void addExport(ComponentInstance* instance, const char* name, LiftedWasiFunction::Type type, FunctionType* functionType);
    static void addTypeExport(ComponentInstance* instance, const char* name, ComponentRefCounted* type);
    static void addResourceExport(ComponentInstance* instance, const char* name);

    Store* m_store;
    ComponentType* m_type;
};

void ComponentInstanceWasi02::aliasTypeExport(ComponentInstance* instance, const char* name, ComponentRefCounted* type)
{
    type->addRef();
    addTypeExport(instance, name, type);
}

void ComponentInstanceWasi02::addExport(ComponentInstance* instance, const char* name, LiftedWasiFunction::Type type, FunctionType* functionType)
{
    instance->m_type->exports().push_back(ComponentType::External{ name, nullptr, ComponentSort::Func, static_cast<uint32_t>(instance->m_funcs.size()) });
    instance->m_funcs.push_back(new LiftedWasiFunction(type, instance, functionType));
}

void ComponentInstanceWasi02::addTypeExport(ComponentInstance* instance, const char* name, ComponentRefCounted* type)
{
    type->addRef();
    instance->m_type->exports().push_back(ComponentType::External{ name, type, ComponentSort::Type, static_cast<uint32_t>(instance->m_type->types().size()) });
    instance->m_type->pushType(type);
}

void ComponentInstanceWasi02::addResourceExport(ComponentInstance* instance, const char* name)
{
    addTypeExport(instance, name, new ComponentTypeResource(false, ComponentTypeResource::NotDefined));
}

static bool compareName(const char* name, size_t length, const char* expected)
{
    if (strlen(expected) != length) {
        return false;
    }
    return memcmp(name, expected, length) == 0;
}

ComponentInstance* ComponentInstanceWasi02::loadInstance(size_t instanceId, bool useCache)
{
    if (useCache) {
        ComponentInstance* instance = m_store->findWasiComponentInstance(instanceId);
        if (instance != nullptr) {
            return instance;
        }
    }

    switch (instanceId) {
    case instanceIoError02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);
        addResourceExport(instance, "error"); /* 0 */
        return instance;
    }
    case instanceIoPoll02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);
        addResourceExport(instance, "pollable"); /* 0 */
        addExport(instance, "[method]pollable.block", LiftedWasiFunction::ioPollableBlock02, getType(Store::I32R));
        return instance;
    }
    case instanceIoStreams02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);
        addResourceExport(instance, "input-stream"); /* 0 */
        addResourceExport(instance, "output-stream"); /* 1 */

        ComponentInstance* errorIntance = loadInstance(instanceIoError02);
        instance->m_instances.push_back(errorIntance);
        ComponentRefCounted* errorType = errorIntance->type()->getType(0);
        aliasTypeExport(instance, "error", errorType); /* 2 */
        ComponentInstance* pollIntance = loadInstance(instanceIoPoll02);
        instance->m_instances.push_back(pollIntance);
        aliasTypeExport(instance, "pollable", pollIntance->type()->getType(0)); /* 3 */
        ComponentRefCounted* ownErrorType = new ComponentTypeResourceRef(ComponentRefCounted::OwnKind, errorType);
        instance->m_type->pushType(ownErrorType); /* 4 */
        ComponentTypeItems* variant = new ComponentTypeItems(ComponentRefCounted::VariantKind);
        ownErrorType->addRef();
        variant->items().push_back(ComponentTypeItems::Item{ "last-operation-failed", ComponentTypeRef(ownErrorType) });
        variant->items().push_back(ComponentTypeItems::Item{ "closed", ComponentTypeRef() });
        addTypeExport(instance, "stream-error", variant);
        addExport(instance, "[method]output-stream.check-write", LiftedWasiFunction::ioOutputStreamCheckWrite02, getType(Store::I32I32R));
        addExport(instance, "[method]output-stream.write", LiftedWasiFunction::ioOutputStreamWrite02, getType(Store::I32I32I32I32R));
        addExport(instance, "[method]output-stream.blocking-write-and-flush", LiftedWasiFunction::ioOutputStreamBlockingWriteAndFlush02, getType(Store::I32I32I32I32R));
        addExport(instance, "[method]output-stream.blocking-flush", LiftedWasiFunction::ioOutputStreamBlockingFlush02, getType(Store::I32I32R));
        addExport(instance, "[method]output-stream.subscribe", LiftedWasiFunction::ioOutputStreamSubscribe02, getType(Store::I32_RI32));
        return instance;
    }
    case instanceCliExit02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);
        addExport(instance, "exit", LiftedWasiFunction::cliExit02, getType(Store::I32R));
        return instance;
    }
    case instanceCliStdin02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

        ComponentInstance* streamsIntance = loadInstance(instanceIoStreams02);
        instance->m_instances.push_back(streamsIntance);
        aliasTypeExport(instance, "input-stream", streamsIntance->type()->getType(0)); /* 0 */
        addExport(instance, "get-stdin", LiftedWasiFunction::cliGetStdin02, getType(Store::RI32));
        return instance;
    }
    case instanceCliStdout02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

        ComponentInstance* streamsIntance = loadInstance(instanceIoStreams02);
        instance->m_instances.push_back(streamsIntance);
        aliasTypeExport(instance, "output-stream", streamsIntance->type()->getType(1)); /* 0 */
        addExport(instance, "get-stdout", LiftedWasiFunction::cliGetStdout02, getType(Store::RI32));
        return instance;
    }
    case instanceCliStderr02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

        ComponentInstance* streamsIntance = loadInstance(instanceIoStreams02);
        instance->m_instances.push_back(streamsIntance);
        aliasTypeExport(instance, "output-stream", streamsIntance->type()->getType(1)); /* 0 */
        addExport(instance, "get-stderr", LiftedWasiFunction::cliGetStderr02, getType(Store::RI32));
        return instance;
    }
    case instanceCliTerminalInput02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);
        addResourceExport(instance, "terminal-input"); /* 0 */
        return instance;
    }
    case instanceCliTerminalOutput02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);
        addResourceExport(instance, "terminal-output"); /* 0 */
        return instance;
    }
    case instanceCliTerminalStdin02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

        ComponentInstance* inputIntance = loadInstance(instanceCliTerminalInput02);
        instance->m_instances.push_back(inputIntance);
        aliasTypeExport(instance, "terminal-input", inputIntance->type()->getType(0)); /* 0 */
        addExport(instance, "get-terminal-stdin", LiftedWasiFunction::cliGetTerminalStdin02, getType(Store::I32R));
        return instance;
    }
    case instanceCliTerminalStdout02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

        ComponentInstance* outputIntance = loadInstance(instanceCliTerminalOutput02);
        instance->m_instances.push_back(outputIntance);
        aliasTypeExport(instance, "terminal-output", outputIntance->type()->getType(0)); /* 0 */
        addExport(instance, "get-terminal-stdout", LiftedWasiFunction::cliGetTerminalStdout02, getType(Store::I32R));
        return instance;
    }
    case instanceCliTerminalStderr02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

        ComponentInstance* outputIntance = loadInstance(instanceCliTerminalOutput02);
        instance->m_instances.push_back(outputIntance);
        aliasTypeExport(instance, "terminal-output", outputIntance->type()->getType(0)); /* 0 */
        addExport(instance, "get-terminal-stderr", LiftedWasiFunction::cliGetTerminalStderr02, getType(Store::I32R));
        return instance;
    }
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return nullptr;
    }
}

ComponentInstance* wasi02LoadInstance(Store* store, std::string& name)
{
    size_t length = name.length();
    const char* charData = name.data();

    if (length < 12 || memcmp(charData, "wasi:", 5) != 0) {
        return nullptr;
    }

    if (charData[length - 1] < '0' || charData[length - 1] > '9') {
        return nullptr;
    }

    size_t postfixLength = 1;
    if (charData[length - 2] != '.') {
        postfixLength = 2;
        if (charData[length - 1] > '1' || charData[length - 2] != '1' || charData[length - 3] != '.') {
            return nullptr;
        }
    }

    length -= postfixLength;
    const char* postfix = charData + length;

    if (memcmp(charData + length - 5, "@0.2.", 5) != 0) {
        return nullptr;
    }
    length -= 5;

    size_t instanceId = instanceUnknown;

    if (length > 8 && memcmp(charData, "wasi:io/", 8) == 0) {
        charData += 8;
        length -= 8;

        if (compareName(charData, length, "error")) {
            instanceId = instanceIoError02;
        } else if (compareName(charData, length, "poll")) {
            instanceId = instanceIoPoll02;
        } else if (compareName(charData, length, "streams")) {
            instanceId = instanceIoStreams02;
        }
    } else if (length > 9 && memcmp(charData, "wasi:cli/", 9) == 0) {
        charData += 9;
        length -= 9;

        if (compareName(charData, length, "exit")) {
            instanceId = instanceCliExit02;
        } else if (compareName(charData, length, "stdin")) {
            instanceId = instanceCliStdin02;
        } else if (compareName(charData, length, "stdout")) {
            instanceId = instanceCliStdout02;
        } else if (compareName(charData, length, "stderr")) {
            instanceId = instanceCliStderr02;
        } else if (compareName(charData, length, "terminal-input")) {
            instanceId = instanceCliTerminalInput02;
        } else if (compareName(charData, length, "terminal-output")) {
            instanceId = instanceCliTerminalOutput02;
        } else if (compareName(charData, length, "terminal-stdin")) {
            instanceId = instanceCliTerminalStdin02;
        } else if (compareName(charData, length, "terminal-stdout")) {
            instanceId = instanceCliTerminalStdout02;
        } else if (compareName(charData, length, "terminal-stderr")) {
            instanceId = instanceCliTerminalStderr02;
        }
    }

    if (instanceId == instanceUnknown) {
        return nullptr;
    }

    ComponentInstance* instance = store->findWasiComponentInstance(instanceId);
    if (instance != nullptr) {
        return instance;
    }

    ComponentInstanceWasi02 instanceCreator(store);
    instance = instanceCreator.loadInstance(instanceId, false);
    store->registerWasiComponentInstance(instanceId, instance);
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
