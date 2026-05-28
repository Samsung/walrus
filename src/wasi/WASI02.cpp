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
#include "wasi/WASI02Impl.h"
#include "runtime/Store.h"

namespace Walrus {

WasiStoreData::WasiStoreData(int argc, const char** argv, const char** envp)
{
    m_arguments.reserve(static_cast<size_t>(argc));
    while (argc-- > 0) {
        m_arguments.push_back(*argv++);
    }

    if (envp != nullptr) {
        while (*envp != nullptr) {
            const char* name = *envp;
            const char* value = name;

            while (true) {
                if (*value == '\0') {
                    m_environment.push_back(std::pair<std::string, std::string>(std::string(name, value - name), ""));
                    break;
                } else if (*value == '=') {
                    value++;
                    m_environment.push_back(std::pair<std::string, std::string>(std::string(name, value - name - 1), value));
                    break;
                }
                value++;
            }
            envp++;
        }
    }
}

WasiStoreData* wasi02InitData(int argc, const char** argv, const char** envp)
{
    return new WasiStoreData(argc, argv, envp);
}

static ComponentInstance* findWasiComponentInstance(Store* store, size_t instanceId)
{
    auto it = store->wasiData()->wasiInstances().find(instanceId);
    if (it == store->wasiData()->wasiInstances().end()) {
        return nullptr;
    }
    return it->second;
}

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
        ComponentInstance* instance = findWasiComponentInstance(m_store, instanceId);
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
    ASSERT(store->wasiData() != nullptr);
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

    ComponentInstance* instance = findWasiComponentInstance(store, instanceId);
    if (instance != nullptr) {
        return instance;
    }

    ComponentInstanceWasi02 instanceCreator(store);
    instance = instanceCreator.loadInstance(instanceId, false);
    store->wasiData()->wasiInstances()[instanceId] = instance;
    return instance;
}

const FunctionType* getWasiFunctionType(LiftedWasiFunction* function)
{
    return function->functionType();
}

} // namespace Walrus

#endif
