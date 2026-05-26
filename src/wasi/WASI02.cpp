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
    InstanceUnknown,
    InstanceIoError02,
    InstanceIoPoll02,
    InstanceIoStreams02,
    InstanceCliEnvironment02,
    InstanceCliExit02,
    InstanceCliStdin02,
    InstanceCliStdout02,
    InstanceCliStderr02,
    InstanceCliTerminalInput02,
    InstanceCliTerminalOutput02,
    InstanceCliTerminalStdin02,
    InstanceCliTerminalStdout02,
    InstanceCliTerminalStderr02,
    InstanceClockMonotonic02,
    InstanceClockWall02,
    InstanceFileSystemTypes02,
    InstanceFileSystemPreOpens02,
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
        ioPoll02,
        ioInputStreamRead02,
        ioInputStreamSubscribe02,
        ioOutputStreamCheckWrite02,
        ioOutputStreamWrite02,
        ioOutputStreamBlockingWriteAndFlush02,
        ioOutputStreamBlockingFlush02,
        ioOutputStreamSubscribe02,
        cliGetEnvironment02,
        cliGetArguments02,
        cliGetStdin02,
        cliGetStdout02,
        cliGetStderr02,
        cliGetTerminalStdin02,
        cliGetTerminalStdout02,
        cliGetTerminalStderr02,
        clockMonotonicNow02,
        clockSubscribeDuration02,
        clockWallNow02,
        fileSystemDescriptorReadViaStream02,
        fileSystemDescriptorWriteViaStream02,
        fileSystemDescriptorAppendViaStream02,
        fileSystemDescriptorGetFlags02,
        fileSystemDescriptorStat02,
        fileSystemDescriptorOpenAt02,
        fileSystemDescriptorMetadataHash02,
        fileSystemGetDirectories02,
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
    case InstanceIoError02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);
        addResourceExport(instance, "error"); /* 0 */
        return instance;
    }
    case InstanceIoPoll02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);
        addResourceExport(instance, "pollable"); /* 0 */
        addExport(instance, "[method]pollable.block", LiftedWasiFunction::ioPollableBlock02, getType(Store::I32R));
        addExport(instance, "poll", LiftedWasiFunction::ioPoll02, getType(Store::I32I32I32R));
        return instance;
    }
    case InstanceIoStreams02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);
        addResourceExport(instance, "input-stream"); /* 0 */
        addResourceExport(instance, "output-stream"); /* 1 */

        ComponentInstance* errorIntance = loadInstance(InstanceIoError02);
        instance->m_instances.push_back(errorIntance);
        ComponentRefCounted* errorType = errorIntance->type()->getType(0);
        aliasTypeExport(instance, "error", errorType); /* 2 */
        ComponentInstance* pollIntance = loadInstance(InstanceIoPoll02);
        instance->m_instances.push_back(pollIntance);
        aliasTypeExport(instance, "pollable", pollIntance->type()->getType(0)); /* 3 */
        ComponentRefCounted* ownErrorType = new ComponentTypeResourceRef(ComponentRefCounted::OwnKind, errorType);
        instance->m_type->pushType(ownErrorType); /* 4 */
        ComponentTypeItems* variant = new ComponentTypeItems(ComponentRefCounted::VariantKind);
        ownErrorType->addRef();
        variant->items().push_back(ComponentTypeItems::Item{ "last-operation-failed", ComponentTypeRef(ownErrorType) });
        variant->items().push_back(ComponentTypeItems::Item{ "closed", ComponentTypeRef() });
        addTypeExport(instance, "stream-error", variant);
        addExport(instance, "[method]input-stream.read", LiftedWasiFunction::ioInputStreamRead02, getType(Store::I32I64I32R));
        addExport(instance, "[method]input-stream.subscribe", LiftedWasiFunction::ioInputStreamSubscribe02, getType(Store::I32_RI32));
        addExport(instance, "[method]output-stream.check-write", LiftedWasiFunction::ioOutputStreamCheckWrite02, getType(Store::I32I32R));
        addExport(instance, "[method]output-stream.write", LiftedWasiFunction::ioOutputStreamWrite02, getType(Store::I32I32I32I32R));
        addExport(instance, "[method]output-stream.blocking-write-and-flush", LiftedWasiFunction::ioOutputStreamBlockingWriteAndFlush02, getType(Store::I32I32I32I32R));
        addExport(instance, "[method]output-stream.blocking-flush", LiftedWasiFunction::ioOutputStreamBlockingFlush02, getType(Store::I32I32R));
        addExport(instance, "[method]output-stream.subscribe", LiftedWasiFunction::ioOutputStreamSubscribe02, getType(Store::I32_RI32));
        return instance;
    }
    case InstanceCliEnvironment02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);
        addExport(instance, "get-environment", LiftedWasiFunction::cliGetEnvironment02, getType(Store::I32R));
        addExport(instance, "get-arguments", LiftedWasiFunction::cliGetArguments02, getType(Store::I32R));
        return instance;
    }
    case InstanceCliExit02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);
        addExport(instance, "exit", LiftedWasiFunction::cliExit02, getType(Store::I32R));
        return instance;
    }
    case InstanceCliStdin02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

        ComponentInstance* streamsIntance = loadInstance(InstanceIoStreams02);
        instance->m_instances.push_back(streamsIntance);
        aliasTypeExport(instance, "input-stream", streamsIntance->type()->getType(0)); /* 0 */
        addExport(instance, "get-stdin", LiftedWasiFunction::cliGetStdin02, getType(Store::RI32));
        return instance;
    }
    case InstanceCliStdout02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

        ComponentInstance* streamsIntance = loadInstance(InstanceIoStreams02);
        instance->m_instances.push_back(streamsIntance);
        aliasTypeExport(instance, "output-stream", streamsIntance->type()->getType(1)); /* 0 */
        addExport(instance, "get-stdout", LiftedWasiFunction::cliGetStdout02, getType(Store::RI32));
        return instance;
    }
    case InstanceCliStderr02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

        ComponentInstance* streamsIntance = loadInstance(InstanceIoStreams02);
        instance->m_instances.push_back(streamsIntance);
        aliasTypeExport(instance, "output-stream", streamsIntance->type()->getType(1)); /* 0 */
        addExport(instance, "get-stderr", LiftedWasiFunction::cliGetStderr02, getType(Store::RI32));
        return instance;
    }
    case InstanceCliTerminalInput02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);
        addResourceExport(instance, "terminal-input"); /* 0 */
        return instance;
    }
    case InstanceCliTerminalOutput02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);
        addResourceExport(instance, "terminal-output"); /* 0 */
        return instance;
    }
    case InstanceCliTerminalStdin02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

        ComponentInstance* inputIntance = loadInstance(InstanceCliTerminalInput02);
        instance->m_instances.push_back(inputIntance);
        aliasTypeExport(instance, "terminal-input", inputIntance->type()->getType(0)); /* 0 */
        addExport(instance, "get-terminal-stdin", LiftedWasiFunction::cliGetTerminalStdin02, getType(Store::I32R));
        return instance;
    }
    case InstanceCliTerminalStdout02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

        ComponentInstance* outputIntance = loadInstance(InstanceCliTerminalOutput02);
        instance->m_instances.push_back(outputIntance);
        aliasTypeExport(instance, "terminal-output", outputIntance->type()->getType(0)); /* 0 */
        addExport(instance, "get-terminal-stdout", LiftedWasiFunction::cliGetTerminalStdout02, getType(Store::I32R));
        return instance;
    }
    case InstanceCliTerminalStderr02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

        ComponentInstance* outputIntance = loadInstance(InstanceCliTerminalOutput02);
        instance->m_instances.push_back(outputIntance);
        aliasTypeExport(instance, "terminal-output", outputIntance->type()->getType(0)); /* 0 */
        addExport(instance, "get-terminal-stderr", LiftedWasiFunction::cliGetTerminalStderr02, getType(Store::I32R));
        return instance;
    }
    case InstanceClockMonotonic02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

        ComponentValueType* timeType = new ComponentValueType(ComponentTypeRef::U64);
        addTypeExport(instance, "instant", timeType); /* 0 */
        aliasTypeExport(instance, "duration", timeType); /* 1 */
        ComponentInstance* pollIntance = loadInstance(InstanceIoPoll02);
        instance->m_instances.push_back(pollIntance);
        aliasTypeExport(instance, "pollable", pollIntance->type()->getType(0)); /* 2 */
        addExport(instance, "now", LiftedWasiFunction::clockMonotonicNow02, getType(Store::RI64));
        addExport(instance, "subscribe-duration", LiftedWasiFunction::clockSubscribeDuration02, getType(Store::I64_RI32));
        return instance;
    }
    case InstanceClockWall02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

        ComponentTypeItems* dateTime = new ComponentTypeItems(ComponentRefCounted::RecordKind);
        dateTime->items().push_back(ComponentTypeItems::Item{ "seconds", ComponentTypeRef(ComponentTypeRef::U64) });
        dateTime->items().push_back(ComponentTypeItems::Item{ "nanoseconds", ComponentTypeRef(ComponentTypeRef::U32) });
        addTypeExport(instance, "datetime", dateTime);
        addExport(instance, "now", LiftedWasiFunction::clockWallNow02, getType(Store::I32R));
        return instance;
    }
    case InstanceFileSystemTypes02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

        addResourceExport(instance, "descriptor"); /* 0 */
        ComponentValueType* u64Type = new ComponentValueType(ComponentTypeRef::U64);
        addTypeExport(instance, "filesize", u64Type); /* 1 */
        ComponentInstance* streamsIntance = loadInstance(InstanceIoStreams02);
        instance->m_instances.push_back(streamsIntance);
        aliasTypeExport(instance, "input-stream", streamsIntance->type()->getType(0)); /* 2 */
        aliasTypeExport(instance, "output-stream", streamsIntance->type()->getType(1)); /* 3 */
        ComponentTypeLabels* errorCode = new ComponentTypeLabels(ComponentRefCounted::EnumKind);
        errorCode->labels().push_back("access");
        errorCode->labels().push_back("would-block");
        errorCode->labels().push_back("already");
        errorCode->labels().push_back("bad-descriptor");
        errorCode->labels().push_back("busy");
        errorCode->labels().push_back("deadlock");
        errorCode->labels().push_back("quota");
        errorCode->labels().push_back("exist");
        errorCode->labels().push_back("file-too-large");
        errorCode->labels().push_back("illegal-byte-sequence");
        errorCode->labels().push_back("in-progress");
        errorCode->labels().push_back("interrupted");
        errorCode->labels().push_back("invalid");
        errorCode->labels().push_back("io");
        errorCode->labels().push_back("is-directory");
        errorCode->labels().push_back("loop");
        errorCode->labels().push_back("too-many-links");
        errorCode->labels().push_back("message-size");
        errorCode->labels().push_back("name-too-long");
        errorCode->labels().push_back("no-device");
        errorCode->labels().push_back("no-entry");
        errorCode->labels().push_back("no-lock");
        errorCode->labels().push_back("insufficient-memory");
        errorCode->labels().push_back("insufficient-space");
        errorCode->labels().push_back("not-directory");
        errorCode->labels().push_back("not-empty");
        errorCode->labels().push_back("not-recoverable");
        errorCode->labels().push_back("unsupported");
        errorCode->labels().push_back("no-tty");
        errorCode->labels().push_back("no-such-device");
        errorCode->labels().push_back("overflow");
        errorCode->labels().push_back("not-permitted");
        errorCode->labels().push_back("pipe");
        errorCode->labels().push_back("read-only");
        errorCode->labels().push_back("invalid-seek");
        errorCode->labels().push_back("text-file-busy");
        errorCode->labels().push_back("cross-device");
        addTypeExport(instance, "error-code", errorCode); /* 4 */
        ComponentTypeLabels* descriptorFlags = new ComponentTypeLabels(ComponentRefCounted::FlagsKind);
        descriptorFlags->labels().push_back("read");
        descriptorFlags->labels().push_back("write");
        descriptorFlags->labels().push_back("file-integrity-sync");
        descriptorFlags->labels().push_back("data-integrity-sync");
        descriptorFlags->labels().push_back("requested-write-sync");
        descriptorFlags->labels().push_back("mutate-directory");
        addTypeExport(instance, "descriptor-flags", descriptorFlags); /* 5 */
        ComponentTypeLabels* descriptorType = new ComponentTypeLabels(ComponentRefCounted::EnumKind);
        descriptorType->labels().push_back("unknown");
        descriptorType->labels().push_back("block-device");
        descriptorType->labels().push_back("character-device");
        descriptorType->labels().push_back("directory");
        descriptorType->labels().push_back("fifo");
        descriptorType->labels().push_back("symbolic-link");
        descriptorType->labels().push_back("regular-file");
        descriptorType->labels().push_back("socket");
        addTypeExport(instance, "descriptor-type", descriptorType); /* 6 */
        aliasTypeExport(instance, "link-count", u64Type); /* 7 */
        ComponentTypeItems* dateTime = new ComponentTypeItems(ComponentRefCounted::RecordKind);
        dateTime->items().push_back(ComponentTypeItems::Item{ "seconds", ComponentTypeRef(ComponentTypeRef::U64) });
        dateTime->items().push_back(ComponentTypeItems::Item{ "nanoseconds", ComponentTypeRef(ComponentTypeRef::U32) });
        addTypeExport(instance, "datetime", dateTime); /* 8 */
        ComponentTypeItems* descriptorStat = new ComponentTypeItems(ComponentRefCounted::RecordKind);
        descriptorType->addRef();
        descriptorStat->items().push_back(ComponentTypeItems::Item{ "type", ComponentTypeRef(descriptorType) });
        u64Type->addRef();
        descriptorStat->items().push_back(ComponentTypeItems::Item{ "link-count", ComponentTypeRef(u64Type) });
        u64Type->addRef();
        descriptorStat->items().push_back(ComponentTypeItems::Item{ "size", ComponentTypeRef(u64Type) });
        dateTime->addRef();
        ComponentValueTypeRef* optionalDateTime = new ComponentValueTypeRef(ComponentRefCounted::OptionKind, ComponentTypeRef(dateTime));
        descriptorStat->items().push_back(ComponentTypeItems::Item{ "data-access-timestamp", ComponentTypeRef(optionalDateTime) });
        optionalDateTime->addRef();
        descriptorStat->items().push_back(ComponentTypeItems::Item{ "data-modification-timestamp", ComponentTypeRef(optionalDateTime) });
        optionalDateTime->addRef();
        descriptorStat->items().push_back(ComponentTypeItems::Item{ "status-change-timestamp", ComponentTypeRef(optionalDateTime) });
        addTypeExport(instance, "descriptor-stat", descriptorStat); /* 9 */
        ComponentTypeLabels* pathFlags = new ComponentTypeLabels(ComponentRefCounted::FlagsKind);
        pathFlags->labels().push_back("symlink-follow");
        addTypeExport(instance, "path-flags", pathFlags); /* 10 */
        ComponentTypeLabels* openFlags = new ComponentTypeLabels(ComponentRefCounted::FlagsKind);
        openFlags->labels().push_back("create");
        openFlags->labels().push_back("directory");
        openFlags->labels().push_back("exclusive");
        openFlags->labels().push_back("truncate");
        addTypeExport(instance, "open-flags", openFlags); /* 11 */
        ComponentTypeItems* metadataHashValue = new ComponentTypeItems(ComponentRefCounted::RecordKind);
        metadataHashValue->items().push_back(ComponentTypeItems::Item{ "lower", ComponentTypeRef(ComponentTypeRef::U64) });
        metadataHashValue->items().push_back(ComponentTypeItems::Item{ "upper", ComponentTypeRef(ComponentTypeRef::U64) });
        addTypeExport(instance, "metadata-hash-value", metadataHashValue); /* 12 */
        addExport(instance, "[method]descriptor.read-via-stream", LiftedWasiFunction::fileSystemDescriptorReadViaStream02, getType(Store::I32I64I32R));
        addExport(instance, "[method]descriptor.write-via-stream", LiftedWasiFunction::fileSystemDescriptorWriteViaStream02, getType(Store::I32I64I32R));
        addExport(instance, "[method]descriptor.append-via-stream", LiftedWasiFunction::fileSystemDescriptorAppendViaStream02, getType(Store::I32I32R));
        addExport(instance, "[method]descriptor.get-flags", LiftedWasiFunction::fileSystemDescriptorGetFlags02, getType(Store::I32I32R));
        addExport(instance, "[method]descriptor.stat", LiftedWasiFunction::fileSystemDescriptorStat02, getType(Store::I32I32R));
        addExport(instance, "[method]descriptor.open-at", LiftedWasiFunction::fileSystemDescriptorOpenAt02, getType(Store::I32I32I32I32I32I32I32R));
        addExport(instance, "[method]descriptor.metadata-hash", LiftedWasiFunction::fileSystemDescriptorMetadataHash02, getType(Store::I32I32R));
        return instance;
    }
    case InstanceFileSystemPreOpens02: {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        ComponentInstance* instance = ComponentInstance::createInstance(m_store, m_type);

        ComponentInstance* typesIntance = loadInstance(InstanceFileSystemTypes02);
        instance->m_instances.push_back(typesIntance);
        aliasTypeExport(instance, "descriptor", typesIntance->type()->getType(0)); /* 0 */
        addExport(instance, "get-directories", LiftedWasiFunction::fileSystemGetDirectories02, getType(Store::I32R));
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

    size_t instanceId = InstanceUnknown;

    if (length > 8 && memcmp(charData, "wasi:io/", 8) == 0) {
        charData += 8;
        length -= 8;

        if (compareName(charData, length, "error")) {
            instanceId = InstanceIoError02;
        } else if (compareName(charData, length, "poll")) {
            instanceId = InstanceIoPoll02;
        } else if (compareName(charData, length, "streams")) {
            instanceId = InstanceIoStreams02;
        }
    } else if (length > 9 && memcmp(charData, "wasi:cli/", 9) == 0) {
        charData += 9;
        length -= 9;

        if (compareName(charData, length, "environment")) {
            instanceId = InstanceCliEnvironment02;
        } else if (compareName(charData, length, "exit")) {
            instanceId = InstanceCliExit02;
        } else if (compareName(charData, length, "stdin")) {
            instanceId = InstanceCliStdin02;
        } else if (compareName(charData, length, "stdout")) {
            instanceId = InstanceCliStdout02;
        } else if (compareName(charData, length, "stderr")) {
            instanceId = InstanceCliStderr02;
        } else if (compareName(charData, length, "terminal-input")) {
            instanceId = InstanceCliTerminalInput02;
        } else if (compareName(charData, length, "terminal-output")) {
            instanceId = InstanceCliTerminalOutput02;
        } else if (compareName(charData, length, "terminal-stdin")) {
            instanceId = InstanceCliTerminalStdin02;
        } else if (compareName(charData, length, "terminal-stdout")) {
            instanceId = InstanceCliTerminalStdout02;
        } else if (compareName(charData, length, "terminal-stderr")) {
            instanceId = InstanceCliTerminalStderr02;
        }
    } else if (length > 9 && memcmp(charData, "wasi:clocks/", 12) == 0) {
        charData += 12;
        length -= 12;

        if (compareName(charData, length, "monotonic-clock")) {
            instanceId = InstanceClockMonotonic02;
        } else if (compareName(charData, length, "wall-clock")) {
            instanceId = InstanceClockWall02;
        }
    } else if (length > 9 && memcmp(charData, "wasi:filesystem/", 16) == 0) {
        charData += 16;
        length -= 16;

        if (compareName(charData, length, "types")) {
            instanceId = InstanceFileSystemTypes02;
        } else if (compareName(charData, length, "preopens")) {
            instanceId = InstanceFileSystemPreOpens02;
        }
    }

    if (instanceId == InstanceUnknown) {
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
