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

#define INSERT_INTO(CONTAINER, ...)                         \
    do {                                                    \
        CONTAINER.insert(CONTAINER.end(), { __VA_ARGS__ }); \
    } while (0);

#define MAKE_RESULT(OK, ERR) new ComponentTypeResult(ComponentTypeRef(OK), ComponentTypeRef(ERR))

WasiStoreData::WasiStoreData(int argc, const char** argv, const char** envp, Wasi02DirMap& preOpens)
    : m_prevNow(0)
    , m_prevClockNow(clock())
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

    for (auto& it : preOpens) {
        m_preOpens.push_back(std::pair<std::string, std::string>(it.mappedPath, it.realPath));
    }
}

WasiStoreData* wasi02InitData(int argc, const char** argv, const char** envp, Wasi02DirMap& preOpens)
{
    return new WasiStoreData(argc, argv, envp, preOpens);
}

void destroyWasi02Data(WasiStoreData* data)
{
    delete data;
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
    InstanceSocketsUdp02,
    InstanceSocketsTcp02,
    InstanceWasiNNTensor02,
    InstanceWasiNNErrors02,
    InstanceWasiNNInference02,
    InstanceWasiNNGraph02,
};

class ComponentInstanceWasi02 {
public:
    ComponentInstanceWasi02(Store* store, uint8_t version)
        : m_store(store)
        , m_type(nullptr)
        , m_version(version)
    {
    }

    ~ComponentInstanceWasi02()
    {
        if (m_type != nullptr) {
            m_type->releaseRef();
        }
    }

    ComponentInstance* loadInstance(size_t instanceId, bool useCache = true);

    ComponentInstance* loadIoErrorInstance();
    ComponentInstance* loadIoPollInstance();
    ComponentInstance* loadIoStreamsInstance();
    ComponentInstance* loadCliEnvironmentInstance();
    ComponentInstance* loadCliExitInstance();
    ComponentInstance* loadCliStdInstance(size_t streamIdx, size_t typeIdx, const char* exportName, const char* funcName, LiftedWasiFunction::Type funcType);
    ComponentInstance* loadCliTerminalInstance(std::string name);
    ComponentInstance* loadCliTerminalStdInstance(size_t id, const char* ioName, const char* funcName, LiftedWasiFunction::Type funcType);
    ComponentInstance* loadClockMonotonicInstance();
    ComponentInstance* loadClockWallInstance();
    ComponentInstance* loadFileSystemTypesInstance();
    ComponentInstance* loadFileSystemPreOpensInstance();
    ComponentInstance* loadSocketsUdpInstance();
    ComponentInstance* loadSocketsTcpInstance();
#if defined(ENABLE_WASI_NN)
    ComponentInstance* loadWasiNNTensorInstance();
    ComponentInstance* loadWasiNNErrorsInstance();
    ComponentInstance* loadWasiNNInferenceInstance();
    ComponentInstance* loadWasiNNGraphInstance();
#endif

private:
    FunctionType* getType(Store::DefinedFunctionType type)
    {
        return m_store->getDefinedFunctionType(type);
    }

    inline ComponentInstance* createWasiInstance()
    {
        m_type = new ComponentType(ComponentType::ComponentTypeKind);
        return ComponentInstance::createInstance(m_store, m_type);
    }

    static void aliasTypeExport(ComponentInstance* instance, const char* name, ComponentRefCounted* type);
    void addFuncExport(ComponentInstance* instance, const char* name, LiftedWasiFunction::Type type, ComponentTypeFunc* functionType);
    static void addTypeExport(ComponentInstance* instance, const char* name, ComponentRefCounted* type);
    static ComponentRefCounted* addResourceExport(ComponentInstance* instance, const char* name);

    Store* m_store;
    ComponentType* m_type;
    uint8_t m_version;
};

void ComponentInstanceWasi02::aliasTypeExport(ComponentInstance* instance, const char* name, ComponentRefCounted* type)
{
    type->addRef();
    addTypeExport(instance, name, type);
}

void ComponentInstanceWasi02::addFuncExport(ComponentInstance* instance, const char* name, LiftedWasiFunction::Type type, ComponentTypeFunc* functionType)
{
    instance->m_type->exports().push_back(ComponentType::External{ name, functionType, ComponentSort::Func, static_cast<uint32_t>(instance->m_funcs.size()) });
    instance->m_funcs.push_back(new LiftedWasiFunction(type, instance, functionType->createFunctionType(m_store, false)));
}

void ComponentInstanceWasi02::addTypeExport(ComponentInstance* instance, const char* name, ComponentRefCounted* type)
{
    type->addRef();
    instance->m_type->exports().push_back(ComponentType::External{ name, type, ComponentSort::Type, static_cast<uint32_t>(instance->m_type->types().size()) });
    instance->m_type->pushType(type);
}

ComponentRefCounted* ComponentInstanceWasi02::addResourceExport(ComponentInstance* instance, const char* name)
{
    ComponentRefCounted* type = new ComponentTypeResource(false, ComponentTypeResource::NotDefined);
    addTypeExport(instance, name, type);
    return type;
}

static bool compareName(const char* name, size_t length, const char* expected)
{
    if (strlen(expected) != length) {
        return false;
    }
    return memcmp(name, expected, length) == 0;
}

inline ComponentInstance* ComponentInstanceWasi02::loadIoErrorInstance()
{
    ComponentInstance* instance = createWasiInstance();
    addResourceExport(instance, "error");
    ComponentTypeFunc* errorToString = new ComponentTypeFunc(ComponentType::FuncKind);
    {
        errorToString->params().push_back(ComponentTypeFunc::Param{ "self", new ComponentTypeResourceRef(ComponentType::BorrowKind, instance->type()->getType(0)) });
        errorToString->result() = ComponentTypeRef(ComponentTypeRef::String);
        addFuncExport(instance, "[method]error.to-debug-string", LiftedWasiFunction::ioErrorToDebugString02, errorToString);
    }

    if (m_version < 12) {
        return instance;
    }

    return instance;
}

inline ComponentInstance* ComponentInstanceWasi02::loadIoPollInstance()
{
    ComponentInstance* instance = createWasiInstance();
    ComponentRefCounted* pollable = addResourceExport(instance, "pollable");
    ComponentTypeFunc* blockType = new ComponentTypeFunc(ComponentType::FuncKind);
    {
        blockType->params().push_back(ComponentTypeFunc::Param{ "self", new ComponentTypeResourceRef(ComponentType::BorrowKind, pollable) });
        addFuncExport(instance, "[method]pollable.block", LiftedWasiFunction::ioPollableBlock02, blockType);
    }
    ComponentTypeFunc* pollType = new ComponentTypeFunc(ComponentType::FuncKind);
    {
        pollType->params().push_back(ComponentTypeFunc::Param{ "in", new ComponentValueTypeRef(ComponentRefCounted::ListKind, new ComponentTypeResourceRef(ComponentType::BorrowKind, pollable)) });
        pollType->result() = new ComponentValueTypeRef(ComponentRefCounted::ListKind, ComponentTypeRef(ComponentTypeRef::U32));
        addFuncExport(instance, "poll", LiftedWasiFunction::ioPoll02, pollType);
    }

    if (m_version < 12) {
        return instance;
    }

    return instance;
}

inline ComponentInstance* ComponentInstanceWasi02::loadIoStreamsInstance()
{
    enum typeIndex {
        error = 0,
        pollable = 1,
        ownError = 2,
        streamError = 3
    };

    ComponentInstance* instance = createWasiInstance();
    ComponentRefCounted* inputStream = addResourceExport(instance, "input-stream");
    ComponentRefCounted* outputStream = addResourceExport(instance, "output-stream");
    ComponentInstance* errorInstance = loadInstance(InstanceIoError02);
    instance->m_instances.push_back(errorInstance);
    ComponentRefCounted* errorType = errorInstance->type()->getType(typeIndex::error);
    aliasTypeExport(instance, "error", errorType); /* 0 */
    ComponentInstance* pollInstance = loadInstance(InstanceIoPoll02);
    instance->m_instances.push_back(pollInstance);
    aliasTypeExport(instance, "pollable", pollInstance->type()->getType(0)); /* 1 */
    ComponentRefCounted* ownErrorType = new ComponentTypeResourceRef(ComponentRefCounted::OwnKind, errorType);
    instance->type()->pushType(ownErrorType); /* 2 */
    ComponentTypeItems* variant = new ComponentTypeItems(ComponentRefCounted::VariantKind);
    ownErrorType->addRef();
    INSERT_INTO(variant->items(),
                ComponentTypeItems::Item{ "last-operation-failed", ComponentTypeRef(ownErrorType) },
                ComponentTypeItems::Item{ "closed", ComponentTypeRef() })
    addTypeExport(instance, "stream-error", variant); /* 3 */
    ComponentTypeFunc* streamRead = new ComponentTypeFunc(ComponentType::FuncKind);
    {
        INSERT_INTO(streamRead->params(),
                    ComponentTypeFunc::Param{ "self", ComponentTypeRef(new ComponentTypeResourceRef(ComponentType::BorrowKind, inputStream)) },
                    ComponentTypeFunc::Param{ "len", ComponentTypeRef(ComponentTypeRef::U64) })
        variant->addRef();
        streamRead->result() = MAKE_RESULT(new ComponentValueTypeRef(ComponentType::ListKind, ComponentTypeRef(ComponentTypeRef::U8)), (variant));
        addFuncExport(instance, "[method]input-stream.read", LiftedWasiFunction::ioInputStreamRead02, streamRead);
        streamRead->addRef();
        addFuncExport(instance, "[method]input-stream.blocking-read", LiftedWasiFunction::ioInputStreamRead02, streamRead);
    }
    ComponentTypeFunc* inputStreamSubscribe = new ComponentTypeFunc(ComponentType::FuncKind);
    {
        inputStreamSubscribe->params().push_back(ComponentTypeFunc::Param{ "self", ComponentTypeRef(new ComponentTypeResourceRef(ComponentType::BorrowKind, inputStream)) });
        inputStreamSubscribe->result() = new ComponentTypeResourceRef(ComponentType::OwnKind, instance->type()->getType(typeIndex::pollable));
        addFuncExport(instance, "[method]input-stream.subscribe", LiftedWasiFunction::ioInputStreamSubscribe02, inputStreamSubscribe);
    }
    ComponentTypeFunc* streamCheckWrite = new ComponentTypeFunc(ComponentType::FuncKind);
    {
        streamCheckWrite->params().push_back(ComponentTypeFunc::Param{ "self", new ComponentTypeResourceRef(ComponentType::BorrowKind, outputStream) });
        variant->addRef();
        streamCheckWrite->result() = MAKE_RESULT(ComponentTypeRef::U64, variant);
        addFuncExport(instance, "[method]output-stream.check-write", LiftedWasiFunction::ioOutputStreamCheckWrite02, streamCheckWrite);
    }
    ComponentTypeFunc* streamWrite = new ComponentTypeFunc(ComponentType::FuncKind);
    {
        INSERT_INTO(streamWrite->params(),
                    ComponentTypeFunc::Param{ "self", new ComponentTypeResourceRef(ComponentType::BorrowKind, outputStream) },
                    ComponentTypeFunc::Param{ "contents", new ComponentValueTypeRef(ComponentType::ListKind, ComponentTypeRef(ComponentTypeRef::U8)) })
        variant->addRef();
        streamWrite->result() = MAKE_RESULT(, variant);
        addFuncExport(instance, "[method]output-stream.write", LiftedWasiFunction::ioOutputStreamWrite02, streamWrite);
        streamWrite->addRef();
        addFuncExport(instance, "[method]output-stream.blocking-write-and-flush", LiftedWasiFunction::ioOutputStreamBlockingWriteAndFlush02, streamWrite);
    }
    ComponentTypeFunc* streamBlockingFlush = new ComponentTypeFunc(ComponentType::FuncKind);
    {
        streamBlockingFlush->params().push_back(ComponentTypeFunc::Param{ "self", new ComponentTypeResourceRef(ComponentType::BorrowKind, outputStream) });
        variant->addRef();
        streamBlockingFlush->result() = MAKE_RESULT(, variant);
        addFuncExport(instance, "[method]output-stream.blocking-flush", LiftedWasiFunction::ioOutputStreamBlockingFlush02, streamBlockingFlush);
    }
    ComponentTypeFunc* outputStreamSubscribe = new ComponentTypeFunc(ComponentType::FuncKind);
    {
        outputStreamSubscribe->params().push_back(ComponentTypeFunc::Param{ "self", new ComponentTypeResourceRef(ComponentType::BorrowKind, outputStream) });
        outputStreamSubscribe->result() = new ComponentTypeResourceRef(ComponentType::OwnKind, instance->type()->getType(typeIndex::pollable));
        addFuncExport(instance, "[method]output-stream.subscribe", LiftedWasiFunction::ioOutputStreamSubscribe02, outputStreamSubscribe);
    }

    if (m_version < 12) {
        return instance;
    }

    return instance;
}

inline ComponentInstance* ComponentInstanceWasi02::loadCliEnvironmentInstance()
{
    ComponentInstance* instance = createWasiInstance();
    ComponentTypeFunc* getEnvironmentType = new ComponentTypeFunc(ComponentRefCounted::FuncKind);
    {
        ComponentTypeTuple* resultTuple = new ComponentTypeTuple();
        INSERT_INTO(resultTuple->items(),
                    ComponentTypeRef(ComponentTypeRef::String),
                    ComponentTypeRef(ComponentTypeRef::String))
        getEnvironmentType->result() = new ComponentValueTypeRef(ComponentRefCounted::ListKind, resultTuple);
        addFuncExport(instance, "get-environment", LiftedWasiFunction::cliGetEnvironment02, getEnvironmentType);
    }
    ComponentTypeFunc* getArgsType = new ComponentTypeFunc(ComponentRefCounted::FuncKind);
    {
        getArgsType->result() = new ComponentValueTypeRef(ComponentRefCounted::ListKind, ComponentTypeRef::String);
        addFuncExport(instance, "get-arguments", LiftedWasiFunction::cliGetArguments02, getArgsType);
    }

    if (m_version < 12) {
        return instance;
    }

    return instance;
}

inline ComponentInstance* ComponentInstanceWasi02::loadCliExitInstance()
{
    ComponentInstance* instance = createWasiInstance();
    ComponentTypeFunc* exitType = new ComponentTypeFunc(ComponentRefCounted::FuncKind);
    ComponentRefCounted* exitParam = MAKE_RESULT(, );
    exitType->params().push_back(ComponentTypeFunc::Param{ "status", ComponentTypeRef(exitParam) });
    addFuncExport(instance, "exit", LiftedWasiFunction::cliExit02, exitType);

    if (m_version < 12) {
        return instance;
    }

    return instance;
}

inline ComponentInstance* ComponentInstanceWasi02::loadCliStdInstance(size_t streamIdx, size_t typeIdx, const char* exportName, const char* funcName, LiftedWasiFunction::Type funcType)
{
    ComponentInstance* instance = createWasiInstance();
    ComponentInstance* streamsInstance = loadInstance(streamIdx);
    instance->m_instances.push_back(streamsInstance);
    aliasTypeExport(instance, exportName, streamsInstance->type()->getType(typeIdx)); /* 0 */
    ComponentTypeFunc* getStream = new ComponentTypeFunc(ComponentRefCounted::FuncKind);
    getStream->result() = new ComponentTypeResourceRef(ComponentType::OwnKind, instance->type()->getType(0));
    addFuncExport(instance, funcName, funcType, getStream);

    if (m_version < 12) {
        return instance;
    }

    return instance;
}

inline ComponentInstance* ComponentInstanceWasi02::loadCliTerminalInstance(std::string name)
{
    ComponentInstance* instance = createWasiInstance();
    addResourceExport(instance, name.c_str());

    if (m_version < 12) {
        return instance;
    }

    return instance;
}

inline ComponentInstance* ComponentInstanceWasi02::loadCliTerminalStdInstance(size_t id, const char* ioName, const char* funcName, LiftedWasiFunction::Type funcType)
{
    ComponentInstance* instance = createWasiInstance();
    ComponentInstance* ioInstance = loadInstance(id);
    instance->m_instances.push_back(ioInstance);
    aliasTypeExport(instance, ioName, ioInstance->type()->getType(0));
    ComponentTypeFunc* getTerminal = new ComponentTypeFunc(ComponentRefCounted::FuncKind);
    getTerminal->result() = new ComponentValueTypeRef(ComponentType::OptionKind, new ComponentTypeResourceRef(ComponentType::OwnKind, instance->type()->getType(0)));
    addFuncExport(instance, funcName, funcType, getTerminal);

    if (m_version < 12) {
        return instance;
    }

    return instance;
}

inline ComponentInstance* ComponentInstanceWasi02::loadClockMonotonicInstance()
{
    enum typeIndex {
        instant = 0,
        duration = 1,
        pollable = 2
    };

    ComponentInstance* instance = createWasiInstance();
    ComponentValueType* timeType = new ComponentValueType(ComponentTypeRef::U64);
    addTypeExport(instance, "instant", timeType); /* 0 */
    aliasTypeExport(instance, "duration", timeType); /* 1 */
    ComponentInstance* pollInstance = loadInstance(InstanceIoPoll02);
    instance->m_instances.push_back(pollInstance);
    aliasTypeExport(instance, "pollable", pollInstance->type()->getType(typeIndex::instant)); /* 2 */
    ComponentTypeFunc* nowType = new ComponentTypeFunc(ComponentRefCounted::FuncKind);
    timeType->addRef();
    nowType->result() = timeType;
    addFuncExport(instance, "now", LiftedWasiFunction::clockMonotonicNow02, nowType);
    ComponentTypeFunc* subscribeDurationType = new ComponentTypeFunc(ComponentRefCounted::FuncKind);
    instance->type()->getType(typeIndex::duration)->addRef();
    subscribeDurationType->params().push_back(ComponentTypeFunc::Param{ "when", ComponentTypeRef(instance->type()->getType(typeIndex::duration)) });
    subscribeDurationType->result() = new ComponentTypeResourceRef(ComponentType::OwnKind, instance->type()->getType(typeIndex::pollable));
    addFuncExport(instance, "subscribe-duration", LiftedWasiFunction::clockSubscribeDuration02, subscribeDurationType);

    if (m_version < 12) {
        return instance;
    }

    return instance;
}

inline ComponentInstance* ComponentInstanceWasi02::loadClockWallInstance()
{
    ComponentInstance* instance = createWasiInstance();
    ComponentTypeItems* dateTime = new ComponentTypeItems(ComponentRefCounted::RecordKind);
    INSERT_INTO(dateTime->items(),
                ComponentTypeItems::Item{ "seconds", ComponentTypeRef(ComponentTypeRef::U64) },
                ComponentTypeItems::Item{ "nanoseconds", ComponentTypeRef(ComponentTypeRef::U32) })
    addTypeExport(instance, "datetime", dateTime); /* 0 */
    ComponentTypeFunc* nowType = new ComponentTypeFunc(ComponentRefCounted::FuncKind);
    instance->type()->getType(0)->addRef();
    nowType->result() = ComponentTypeRef(instance->type()->getType(0));
    addFuncExport(instance, "now", LiftedWasiFunction::clockWallNow02, nowType);

    if (m_version < 12) {
        return instance;
    }

    return instance;
}

inline ComponentInstance* ComponentInstanceWasi02::loadFileSystemTypesInstance()
{
    enum typeIndex {
        descriptor = 0,
        filesize = 1,
        inputStream = 2,
        outputStream = 3,
        errorCodeIdx = 4,
        descriptorFlagsIdx = 5,
        descriptorTypeIdx = 6,
        linkCount = 7,
        datetime = 8,
        descriptorStatidx = 9,
        pathFlagsIdx = 10,
        openFlagsIdx = 11,
        metadataHashValueIdx = 12,
        directoryEntryStream = 13,
        error = 14
    };

    ComponentInstance* instance = createWasiInstance();
    addResourceExport(instance, "descriptor"); /* 0 */
    // ComponentValueType* u64Type = new ComponentValueType(ComponentTypeRef::U64);
    addTypeExport(instance, "filesize", new ComponentValueType(ComponentTypeRef::U64)); /* 1 */
    ComponentInstance* streamsInstance = loadInstance(InstanceIoStreams02);
    instance->m_instances.push_back(streamsInstance);
    aliasTypeExport(instance, "input-stream", streamsInstance->type()->getType(0)); /* 2 */
    aliasTypeExport(instance, "output-stream", streamsInstance->type()->getType(1)); /* 3 */
    ComponentTypeLabels* errorCode = new ComponentTypeLabels(ComponentRefCounted::EnumKind);
    INSERT_INTO(errorCode->labels(),
                "access",
                "would-block",
                "already",
                "bad-descriptor",
                "busy",
                "deadlock",
                "quota",
                "exist",
                "file-too-large",
                "illegal-byte-sequence",
                "in-progress",
                "interrupted",
                "invalid",
                "io",
                "is-directory",
                "loop",
                "too-many-links",
                "message-size",
                "name-too-long",
                "no-device",
                "no-entry",
                "no-lock",
                "insufficient-memory",
                "insufficient-space",
                "not-directory",
                "not-empty",
                "not-recoverable",
                "unsupported",
                "no-tty",
                "no-such-device",
                "overflow",
                "not-permitted",
                "pipe",
                "read-only",
                "invalid-seek",
                "text-file-busy",
                "cross-device")
    addTypeExport(instance, "error-code", errorCode); /* 4 */
    ComponentTypeLabels* descriptorFlags = new ComponentTypeLabels(ComponentRefCounted::FlagsKind);
    INSERT_INTO(descriptorFlags->labels(),
                "read",
                "write",
                "file-integrity-sync",
                "data-integrity-sync",
                "requested-write-sync",
                "mutate-directory");
    addTypeExport(instance, "descriptor-flags", descriptorFlags); /* 5 */
    ComponentTypeLabels* descriptorType = new ComponentTypeLabels(ComponentRefCounted::EnumKind);
    INSERT_INTO(descriptorType->labels(),
                "unknown",
                "block-device",
                "character-device",
                "directory",
                "fifo",
                "symbolic-link",
                "regular-file",
                "socket")
    addTypeExport(instance, "descriptor-type", descriptorType); /* 6 */
    addTypeExport(instance, "link-count", new ComponentValueType(ComponentTypeRef::U64)); /* 7 */
    ComponentTypeItems* dateTime = new ComponentTypeItems(ComponentRefCounted::RecordKind);
    INSERT_INTO(dateTime->items(),
                ComponentTypeItems::Item{ "seconds", ComponentTypeRef(ComponentTypeRef::U64) },
                ComponentTypeItems::Item{ "nanoseconds", ComponentTypeRef(ComponentTypeRef::U32) })
    addTypeExport(instance, "datetime", dateTime); /* 8 */
    ComponentTypeItems* descriptorStat = new ComponentTypeItems(ComponentRefCounted::RecordKind);
    INSERT_INTO(descriptorStat->items(),
                ComponentTypeItems::Item{ "type", ComponentTypeRef(descriptorType) },
                ComponentTypeItems::Item{ "link-count", ComponentTypeRef(new ComponentValueType(ComponentTypeRef::U64)) },
                ComponentTypeItems::Item{ "size", ComponentTypeRef(new ComponentValueType(ComponentTypeRef::U64)) })
    descriptorType->addRef();
    // u64Type->addRef();
    dateTime->addRef();
    ComponentValueTypeRef* optionalDateTime = new ComponentValueTypeRef(ComponentRefCounted::OptionKind, ComponentTypeRef(dateTime));
    INSERT_INTO(descriptorStat->items(),
                ComponentTypeItems::Item{ "data-access-timestamp", ComponentTypeRef(optionalDateTime) },
                ComponentTypeItems::Item{ "data-modification-timestamp", ComponentTypeRef(optionalDateTime) },
                ComponentTypeItems::Item{ "status-change-timestamp", ComponentTypeRef(optionalDateTime) })
    optionalDateTime->addRef();
    optionalDateTime->addRef();
    addTypeExport(instance, "descriptor-stat", descriptorStat); /* 9 */
    ComponentTypeLabels* pathFlags = new ComponentTypeLabels(ComponentRefCounted::FlagsKind);
    pathFlags->labels().push_back("symlink-follow");
    addTypeExport(instance, "path-flags", pathFlags); /* 10 */
    ComponentTypeLabels* openFlags = new ComponentTypeLabels(ComponentRefCounted::FlagsKind);
    INSERT_INTO(openFlags->labels(),
                "create",
                "directory",
                "exclusive",
                "truncate", )
    addTypeExport(instance, "open-flags", openFlags); /* 11 */
    ComponentTypeItems* metadataHashValue = new ComponentTypeItems(ComponentRefCounted::RecordKind);
    INSERT_INTO(metadataHashValue->items(),
                ComponentTypeItems::Item{ "lower", ComponentTypeRef(ComponentTypeRef::U64) },
                ComponentTypeItems::Item{ "upper", ComponentTypeRef(ComponentTypeRef::U64) })
    addTypeExport(instance, "metadata-hash-value", metadataHashValue); /* 12 */
    ComponentTypeFunc* readViaStream = new ComponentTypeFunc(ComponentRefCounted::FuncKind);
    {
        readViaStream->params().push_back(ComponentTypeFunc::Param{ "self", ComponentTypeRef(new ComponentTypeResourceRef(ComponentType::BorrowKind, instance->type()->getType(typeIndex::descriptor))) });
        instance->type()->getType(1)->addRef();
        readViaStream->params().push_back(ComponentTypeFunc::Param{ "offset", ComponentTypeRef(instance->type()->getType(typeIndex::filesize)) });
        instance->type()->getType(4)->addRef();
        readViaStream->result() = MAKE_RESULT(new ComponentTypeResourceRef(ComponentType::OwnKind, instance->type()->getType(2)), instance->type()->getType(4));
        addFuncExport(instance, "[method]descriptor.read-via-stream", LiftedWasiFunction::fileSystemDescriptorReadViaStream02, readViaStream);
    }
    ComponentTypeFunc* writeViaStream = new ComponentTypeFunc(ComponentRefCounted::FuncKind);
    {
        writeViaStream->params().push_back(ComponentTypeFunc::Param{ "self", ComponentTypeRef(new ComponentTypeResourceRef(ComponentType::BorrowKind, instance->type()->getType(0))) });
        instance->type()->getType(1)->addRef();
        writeViaStream->params().push_back(ComponentTypeFunc::Param{ "offset", ComponentTypeRef(instance->type()->getType(1)) });
        instance->type()->getType(4)->addRef();
        writeViaStream->result() = MAKE_RESULT(new ComponentTypeResourceRef(ComponentType::OwnKind, instance->type()->getType(3)), instance->type()->getType(4));
        addFuncExport(instance, "[method]descriptor.write-via-stream", LiftedWasiFunction::fileSystemDescriptorWriteViaStream02, writeViaStream);
    }
    ComponentTypeFunc* appendViaStream = new ComponentTypeFunc(ComponentRefCounted::FuncKind);
    {
        appendViaStream->params().push_back(ComponentTypeFunc::Param{ "self", ComponentTypeRef(new ComponentTypeResourceRef(ComponentType::BorrowKind, instance->type()->getType(0))) });
        instance->type()->getType(4)->addRef();
        appendViaStream->result() = MAKE_RESULT(new ComponentTypeResourceRef(ComponentType::OwnKind, instance->type()->getType(3)), instance->type()->getType(4));
        addFuncExport(instance, "[method]descriptor.append-via-stream", LiftedWasiFunction::fileSystemDescriptorAppendViaStream02, appendViaStream);
    }
    ComponentTypeFunc* getFlags = new ComponentTypeFunc(ComponentRefCounted::FuncKind);
    {
        getFlags->params().push_back(ComponentTypeFunc::Param{ "self", ComponentTypeRef(new ComponentTypeResourceRef(ComponentType::BorrowKind, instance->type()->getType(0))) });
        instance->type()->getType(4)->addRef();
        instance->type()->getType(5)->addRef();
        getFlags->result() = MAKE_RESULT(instance->type()->getType(5), instance->type()->getType(4));
        addFuncExport(instance, "[method]descriptor.get-flags", LiftedWasiFunction::fileSystemDescriptorGetFlags02, getFlags);
    }
    ComponentTypeFunc* stat = new ComponentTypeFunc(ComponentRefCounted::FuncKind);
    {
        stat->params().push_back(ComponentTypeFunc::Param{ "self", ComponentTypeRef(new ComponentTypeResourceRef(ComponentType::BorrowKind, instance->type()->getType(0))) });
        instance->type()->getType(9)->addRef();
        instance->type()->getType(4)->addRef();
        stat->result() = MAKE_RESULT(instance->type()->getType(9), instance->type()->getType(4));
        addFuncExport(instance, "[method]descriptor.stat", LiftedWasiFunction::fileSystemDescriptorStat02, stat);
    }
    ComponentTypeFunc* statAt = new ComponentTypeFunc(ComponentRefCounted::FuncKind);
    {
        statAt->params().push_back(ComponentTypeFunc::Param{ "self", ComponentTypeRef(new ComponentTypeResourceRef(ComponentType::BorrowKind, instance->type()->getType(0))) });
        instance->type()->getType(10)->addRef();
        statAt->params().push_back(ComponentTypeFunc::Param{ "path-flags", ComponentTypeRef(instance->type()->getType(10)) });
        statAt->params().push_back(ComponentTypeFunc::Param{ "path", ComponentTypeRef(ComponentTypeRef::String) });
        instance->type()->getType(9)->addRef();
        instance->type()->getType(4)->addRef();
        statAt->result() = MAKE_RESULT(instance->type()->getType(9), instance->type()->getType(4));
        addFuncExport(instance, "[method]descriptor.stat-at", LiftedWasiFunction::fileSystemDescriptorStatAt02, statAt);
    }
    ComponentTypeFunc* openAt = new ComponentTypeFunc(ComponentRefCounted::FuncKind);
    {
        openAt->params().push_back(ComponentTypeFunc::Param{ "self", ComponentTypeRef(new ComponentTypeResourceRef(ComponentType::BorrowKind, instance->type()->getType(0))) });
        instance->type()->getType(10)->addRef();
        openAt->params().push_back(ComponentTypeFunc::Param{ "path-flags", ComponentTypeRef(instance->type()->getType(10)) });
        openAt->params().push_back(ComponentTypeFunc::Param{ "path", ComponentTypeRef(ComponentTypeRef::String) });
        instance->type()->getType(11)->addRef();
        openAt->params().push_back(ComponentTypeFunc::Param{ "open-flags", ComponentTypeRef(instance->type()->getType(11)) });
        instance->type()->getType(5)->addRef();
        openAt->params().push_back(ComponentTypeFunc::Param{ "flags", ComponentTypeRef(instance->type()->getType(5)) });
        instance->type()->getType(4)->addRef();
        openAt->result() = MAKE_RESULT(new ComponentTypeResourceRef(ComponentType::OwnKind, instance->type()->getType(0)), instance->type()->getType(4));
        addFuncExport(instance, "[method]descriptor.open-at", LiftedWasiFunction::fileSystemDescriptorOpenAt02, openAt);
    }
    ComponentTypeFunc* metadataHash = new ComponentTypeFunc(ComponentRefCounted::FuncKind);
    {
        metadataHash->params().push_back(ComponentTypeFunc::Param{ "self", ComponentTypeRef(new ComponentTypeResourceRef(ComponentType::BorrowKind, instance->type()->getType(0))) });
        instance->type()->getType(12)->addRef();
        instance->type()->getType(4)->addRef();
        metadataHash->result() = MAKE_RESULT(instance->type()->getType(12), instance->type()->getType(4));
        addFuncExport(instance, "[method]descriptor.metadata-hash", LiftedWasiFunction::fileSystemDescriptorMetadataHash02, metadataHash);
    }
    ComponentTypeFunc* metadataHashAt = new ComponentTypeFunc(ComponentRefCounted::FuncKind);
    {
        metadataHashAt->params().push_back(ComponentTypeFunc::Param{ "self", ComponentTypeRef(new ComponentTypeResourceRef(ComponentType::BorrowKind, instance->type()->getType(0))) });
        instance->type()->getType(10)->addRef();
        metadataHashAt->params().push_back(ComponentTypeFunc::Param{ "path-flags", ComponentTypeRef(instance->type()->getType(10)) });
        metadataHashAt->params().push_back(ComponentTypeFunc::Param{ "path", ComponentTypeRef(ComponentTypeRef::String) });
        instance->type()->getType(12)->addRef();
        instance->type()->getType(4)->addRef();
        metadataHashAt->result() = MAKE_RESULT(instance->type()->getType(12), instance->type()->getType(4));
        addFuncExport(instance, "[method]descriptor.metadata-hash-at", LiftedWasiFunction::fileSystemDescriptorMetadataHashAt02, metadataHashAt);
    }
    addResourceExport(instance, "directory-entry-stream"); /* 13 */
    ComponentTypeFunc* getType = new ComponentTypeFunc(ComponentRefCounted::FuncKind);
    {
        getType->params().push_back(ComponentTypeFunc::Param{ "self", ComponentTypeRef(new ComponentTypeResourceRef(ComponentType::BorrowKind, instance->type()->getType(0))) });
        instance->type()->getType(6)->addRef();
        instance->type()->getType(4)->addRef();
        getType->result() = MAKE_RESULT(instance->type()->getType(6), instance->type()->getType(4));
        addFuncExport(instance, "[method]descriptor.get-type", LiftedWasiFunction::fileSystemDescriptorGetType02, getType);
    }
    ComponentTypeFunc* fsErrorCode = new ComponentTypeFunc(ComponentRefCounted::FuncKind);
    {
        ComponentInstance* errorInstance = loadInstance(InstanceIoError02);
        instance->m_instances.push_back(errorInstance);
        aliasTypeExport(instance, "error", errorInstance->type()->getType(0)); /* 14 */
        fsErrorCode->params().push_back(ComponentTypeFunc::Param{ "err", ComponentTypeRef(new ComponentTypeResourceRef(ComponentType::BorrowKind, instance->type()->getType(14))) });
        instance->type()->getType(4)->addRef();
        fsErrorCode->result() = new ComponentValueTypeRef(ComponentType::OptionKind, instance->type()->getType(4));
        addFuncExport(instance, "filesystem-error-code", LiftedWasiFunction::fileSystemErrorCode02, fsErrorCode);
    }

    if (m_version < 12) {
        return instance;
    }

    return instance;
}

inline ComponentInstance* ComponentInstanceWasi02::loadFileSystemPreOpensInstance()
{
    ComponentInstance* instance = createWasiInstance();
    ComponentInstance* typesInstance = loadInstance(InstanceFileSystemTypes02);
    instance->m_instances.push_back(typesInstance);
    aliasTypeExport(instance, "descriptor", typesInstance->type()->getType(0)); /* 0 */
    ComponentTypeFunc* getDirectories = new ComponentTypeFunc(ComponentRefCounted::FuncKind);
    {
        ComponentTypeTuple* tuple = new ComponentTypeTuple();
        tuple->items().push_back(ComponentTypeRef(new ComponentTypeResourceRef(ComponentType::OwnKind, instance->type()->getType(0))));
        tuple->items().push_back(ComponentTypeRef(ComponentTypeRef::String));
        getDirectories->result() = new ComponentValueTypeRef(ComponentType::ListKind, tuple);
        addFuncExport(instance, "get-directories", LiftedWasiFunction::fileSystemGetDirectories02, getDirectories);
    }

    if (m_version < 12) {
        return instance;
    }

    return instance;
}

inline ComponentInstance* ComponentInstanceWasi02::loadSocketsUdpInstance()
{
    ComponentInstance* instance = createWasiInstance();
    addResourceExport(instance, "udp-socket");
    addResourceExport(instance, "incoming-datagram-stream");
    addResourceExport(instance, "outgoing-datagram-stream");

    if (m_version < 12) {
        return instance;
    }

    return instance;
}

inline ComponentInstance* ComponentInstanceWasi02::loadSocketsTcpInstance()
{
    ComponentInstance* instance = createWasiInstance();
    addResourceExport(instance, "tcp-socket");

    if (m_version < 12) {
        return instance;
    }

    return instance;
}

#if defined(ENABLE_WASI_NN)
inline ComponentInstance* ComponentInstanceWasi02::loadWasiNNTensorInstance()
{
    ComponentInstance* instance = createWasiInstance();
    ComponentValueTypeRef* tensorDimensions = new ComponentValueTypeRef(ComponentType::ListKind, ComponentTypeRef::U32);
    addTypeExport(instance, "tensor-dimensions", tensorDimensions); /* 0 */
    ComponentTypeLabels* tensorType = new ComponentTypeLabels(ComponentRefCounted::EnumKind);
    INSERT_INTO(tensorType->labels(),
                "FP16",
                "FP32",
                "FP64",
                "BF16",
                "U8",
                "I32",
                "I64")
    addTypeExport(instance, "tensor-type", tensorType); /* 1 */
    ComponentValueTypeRef* tensorData = new ComponentValueTypeRef(ComponentType::ListKind, ComponentTypeRef::U8);
    addTypeExport(instance, "tensor-data", tensorData); /* 2 */
    addResourceExport(instance, "tensor"); /* 3 */
    ComponentTypeFunc* tensorConstructorType = new ComponentTypeFunc(ComponentType::FuncKind);
    {
        tensorDimensions->addRef();
        tensorConstructorType->params().push_back(ComponentTypeFunc::Param{ "dimensions", ComponentTypeRef(tensorDimensions) });
        tensorType->addRef();
        tensorConstructorType->params().push_back(ComponentTypeFunc::Param{ "ty", ComponentTypeRef(tensorType) });
        tensorData->addRef();
        tensorConstructorType->params().push_back(ComponentTypeFunc::Param{ "data", ComponentTypeRef(tensorData) });
        tensorConstructorType->result() = ComponentTypeRef(new ComponentTypeResourceRef(ComponentRefCounted::OwnKind, (instance->type()->getType(3))));
        addFuncExport(instance, "[constructor]tensor", LiftedWasiFunction::neuralNetworkTensorConstructor02, tensorConstructorType);
    }
    ComponentTypeFunc* tensorDataFunc = new ComponentTypeFunc(ComponentType::FuncKind);
    {
        tensorDataFunc->params().push_back(ComponentTypeFunc::Param{ "self", ComponentTypeRef(new ComponentTypeResourceRef(ComponentRefCounted::BorrowKind, (instance->type()->getType(3)))) });
        tensorData->addRef();
        tensorDataFunc->result() = tensorData;
        addFuncExport(instance, "[method]tensor.data", LiftedWasiFunction::neuralNetworkTensorData02, tensorDataFunc);
    }

    if (m_version < 12) {
        return instance;
    }

    return instance;
}

inline ComponentInstance* ComponentInstanceWasi02::loadWasiNNErrorsInstance()
{
    ComponentInstance* instance = createWasiInstance();
    ComponentRefCounted* error = addResourceExport(instance, "error"); /* 0 */
    ComponentTypeLabels* errorCode = new ComponentTypeLabels(ComponentRefCounted::EnumKind);
    INSERT_INTO(errorCode->labels(),
                "invalid-argument",
                "invalid-encoding",
                "timeout",
                "runtime-error",
                "unsupported-operation",
                "too-large",
                "not-found",
                "security",
                "unknown");
    aliasTypeExport(instance, "error-code", errorCode); /* 1 */
    ComponentTypeFunc* errorMethod = new ComponentTypeFunc(ComponentType::FuncKind);
    {
        errorMethod->params().push_back(ComponentTypeFunc::Param{ "self", ComponentTypeRef(new ComponentTypeResourceRef(ComponentRefCounted::BorrowKind, error)) });
        errorCode->addRef();
        errorMethod->result() = errorCode;
        addFuncExport(instance, "[method]error.code", LiftedWasiFunction::neuralNetworkErrorCode02, errorMethod);
    }

    if (m_version < 12) {
        return instance;
    }

    return instance;
}

inline ComponentInstance* ComponentInstanceWasi02::loadWasiNNInferenceInstance()
{
    ComponentInstance* instance = createWasiInstance();
    addResourceExport(instance, "graph-execution-context"); /* 0 */
    ComponentInstance* tensorInstance = loadInstance(InstanceWasiNNTensor02);
    instance->m_instances.push_back(tensorInstance);
    ComponentRefCounted* tensorType = tensorInstance->type()->getType(3);
    aliasTypeExport(instance, "tensor", tensorType); /* 1 */
    ComponentTypeTuple* namedTensorTuple = new ComponentTypeTuple();
    INSERT_INTO(namedTensorTuple->items(),
                ComponentTypeRef(ComponentTypeRef::String),
                ComponentTypeRef(new ComponentTypeResourceRef(ComponentRefCounted::OwnKind, tensorType)))
    addTypeExport(instance, "named-tensor", namedTensorTuple); /* 2 */
    ComponentInstance* wasiNNError = loadInstance(InstanceWasiNNErrors02);
    instance->m_instances.push_back(wasiNNError);
    ComponentRefCounted* wasiNNErrorType = wasiNNError->type()->getType(0);
    aliasTypeExport(instance, "error", wasiNNErrorType); /* 3 */
    ComponentTypeFunc* computeType = new ComponentTypeFunc(ComponentType::FuncKind);
    {
        computeType->params().push_back(ComponentTypeFunc::Param{ "self", ComponentTypeRef(new ComponentTypeResourceRef(ComponentRefCounted::BorrowKind, (instance->type()->getType(0)))) });
        namedTensorTuple->addRef();
        computeType->params().push_back(ComponentTypeFunc::Param{ "inputs", new ComponentValueTypeRef(ComponentType::ListKind, ComponentTypeRef(namedTensorTuple)) });
        namedTensorTuple->addRef();
        ComponentTypeRef resultOk = ComponentTypeRef(new ComponentValueTypeRef(ComponentRefCounted::ListKind, namedTensorTuple));
        ComponentTypeRef resultErr = ComponentTypeRef(new ComponentTypeResourceRef(ComponentRefCounted::OwnKind, wasiNNErrorType));
        computeType->result() = MAKE_RESULT(resultOk, resultErr);
        addFuncExport(instance, "[method]graph-execution-context.compute", LiftedWasiFunction::neuralNetworkInferenceGraphExecutionContextCompute02, computeType);
    }

    if (m_version < 12) {
        return instance;
    }

    return instance;
}

inline ComponentInstance* ComponentInstanceWasi02::loadWasiNNGraphInstance()
{
    ComponentInstance* instance = createWasiInstance();
    addResourceExport(instance, "graph"); /* 0 */
    ComponentInstance* inferenceInstance = loadInstance(InstanceWasiNNInference02);
    instance->m_instances.push_back(inferenceInstance);
    ComponentRefCounted* graphExecutionContextType = inferenceInstance->type()->getType(0);
    aliasTypeExport(instance, "graph-execution-context", graphExecutionContextType); /* 1 */
    ComponentInstance* wasiNNError = loadInstance(InstanceWasiNNErrors02);
    instance->m_instances.push_back(wasiNNError);
    ComponentRefCounted* wasiNNErrorType = wasiNNError->type()->getType(0);
    aliasTypeExport(instance, "error", wasiNNErrorType); /* 2 */
    addTypeExport(instance, "graph-builder", new ComponentValueTypeRef(ComponentType::ListKind, ComponentTypeRef::U8)); /* 3 */
    ComponentTypeLabels* encodingEnum = new ComponentTypeLabels(ComponentRefCounted::EnumKind);
    INSERT_INTO(encodingEnum->labels(),
                "openvino",
                "onnx",
                "tensorflow",
                "pytorch",
                "tensorflowlite",
                "ggml",
                "autodetect")
    addTypeExport(instance, "graph-encoding", encodingEnum); /* 4 */
    ComponentTypeLabels* executionTarget = new ComponentTypeLabels(ComponentRefCounted::EnumKind);
    INSERT_INTO(executionTarget->labels(), "cpu", "gpu", "tpu")
    addTypeExport(instance, "execution-target", executionTarget); /* 5 */
    ComponentTypeRef resultOk = ComponentTypeRef(new ComponentTypeResourceRef(ComponentRefCounted::OwnKind, instance->type()->getType(1)));
    ComponentTypeRef resultErr = ComponentTypeRef(new ComponentTypeResourceRef(ComponentRefCounted::OwnKind, instance->type()->getType(2)));
    ComponentTypeFunc* initExecutionContext = new ComponentTypeFunc(ComponentType::FuncKind);
    {
        initExecutionContext->params().push_back(ComponentTypeFunc::Param{ "self", new ComponentTypeResourceRef(ComponentRefCounted::OwnKind, instance->type()->getType(0)) });
        initExecutionContext->result() = MAKE_RESULT(resultOk, resultErr);
        addFuncExport(instance, "[method]graph.init-execution-context", LiftedWasiFunction::neuralNetworkGraphInitExectionContext02, initExecutionContext);
    }
    ComponentTypeFunc* load = new ComponentTypeFunc(ComponentType::FuncKind);
    {
        load->params().push_back(ComponentTypeFunc::Param{ "builder", ComponentTypeRef(new ComponentValueTypeRef(ComponentType::ListKind, new ComponentValueTypeRef(ComponentType::ListKind, ComponentTypeRef::U8))) });
        encodingEnum->addRef();
        load->params().push_back(ComponentTypeFunc::Param{ "encoding", ComponentTypeRef(encodingEnum) });
        executionTarget->addRef();
        load->params().push_back(ComponentTypeFunc::Param{ "target", ComponentTypeRef(executionTarget) });
        resultOk = ComponentTypeRef(new ComponentTypeResourceRef(ComponentRefCounted::OwnKind, instance->type()->getType(0)));
        resultErr = ComponentTypeRef(new ComponentTypeResourceRef(ComponentRefCounted::OwnKind, instance->type()->getType(2)));
        load->result() = MAKE_RESULT(resultOk, resultErr);
        addFuncExport(instance, "load", LiftedWasiFunction::neuralNetworkGraphLoad02, load);
    }
    ComponentTypeFunc* loadByName = new ComponentTypeFunc(ComponentType::FuncKind);
    {
        loadByName->params().push_back(ComponentTypeFunc::Param{ "name", ComponentTypeRef(ComponentTypeRef::String) });
        resultOk = ComponentTypeRef(new ComponentTypeResourceRef(ComponentRefCounted::OwnKind, instance->type()->getType(0)));
        resultErr = ComponentTypeRef(new ComponentTypeResourceRef(ComponentRefCounted::OwnKind, instance->type()->getType(2)));
        loadByName->result() = MAKE_RESULT(resultOk, resultErr);
        addFuncExport(instance, "load-by-name", LiftedWasiFunction::neuralNetworkGraphLoadByName02, loadByName);
    }

    if (m_version < 12) {
        return instance;
    }

    return instance;
}
#endif

ComponentInstance* ComponentInstanceWasi02::loadInstance(size_t instanceId, bool useCache)
{
    if (useCache) {
        ComponentInstance* instance = findWasiComponentInstance(m_store, instanceId);
        if (instance != nullptr) {
            return instance;
        }
    }

    switch (instanceId) {
    case InstanceIoError02:
        return loadIoErrorInstance();
    case InstanceIoPoll02:
        return loadIoPollInstance();
    case InstanceIoStreams02:
        return loadIoStreamsInstance();
    case InstanceCliEnvironment02:
        return loadCliEnvironmentInstance();
    case InstanceCliExit02:
        return loadCliExitInstance();
    case InstanceCliStdin02:
        return loadCliStdInstance(InstanceIoStreams02, 0, "input-stream", "get-stdin", LiftedWasiFunction::cliGetStdin02);
    case InstanceCliStdout02:
        return loadCliStdInstance(InstanceIoStreams02, 1, "output-stream", "get-stdout", LiftedWasiFunction::cliGetStdout02);
    case InstanceCliStderr02:
        return loadCliStdInstance(InstanceIoStreams02, 1, "output-stream", "get-stderr", LiftedWasiFunction::cliGetStderr02);
    case InstanceCliTerminalInput02:
        return loadCliTerminalInstance("terminal-input");
    case InstanceCliTerminalOutput02:
        return loadCliTerminalInstance("terminal-output");
    case InstanceCliTerminalStdin02:
        return loadCliTerminalStdInstance(InstanceCliTerminalInput02, "terminal-input", "get-terminal-stdin", LiftedWasiFunction::cliGetTerminalStdin02);
    case InstanceCliTerminalStdout02:
        return loadCliTerminalStdInstance(InstanceCliTerminalOutput02, "terminal-output", "get-terminal-stdout", LiftedWasiFunction::cliGetTerminalStdout02);
    case InstanceCliTerminalStderr02:
        return loadCliTerminalStdInstance(InstanceCliTerminalOutput02, "terminal-output", "get-terminal-stderr", LiftedWasiFunction::cliGetTerminalStderr02);
    case InstanceClockMonotonic02:
        return loadClockMonotonicInstance();
    case InstanceClockWall02:
        return loadClockWallInstance();
    case InstanceFileSystemTypes02:
        return loadFileSystemTypesInstance();
    case InstanceFileSystemPreOpens02:
        return loadFileSystemPreOpensInstance();
    case InstanceSocketsUdp02:
        return loadSocketsUdpInstance();
    case InstanceSocketsTcp02:
        return loadSocketsTcpInstance();
#if defined(ENABLE_WASI_NN)
    case InstanceWasiNNTensor02:
        return loadWasiNNTensorInstance();
    case InstanceWasiNNErrors02:
        return loadWasiNNErrorsInstance();
    case InstanceWasiNNInference02:
        return loadWasiNNInferenceInstance();
    case InstanceWasiNNGraph02:
        return loadWasiNNGraphInstance();
#endif
    default:
        WALRUS_LOG_ERROR("loadInstance: unknown WASI instance id: %zu\n", instanceId);
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

    // remove the date part from wasi:nn
    if (memcmp(charData, "wasi:nn", 7) == 0) {
        name.erase(length - 14, 14);
        length = name.length();
    }

    size_t postfixLength = 1;
    if (charData[length - 2] != '.') {
        postfixLength = 2;
    }

    length -= postfixLength;
    uint8_t postfix = std::atoi(charData + length);
    if (postfix > 6) {
        WALRUS_LOG_ERROR("WASI preview2 version 0.2.%d is unsupported!\n", postfix);
        return nullptr;
    }

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
    } else if (length > 9 && memcmp(charData, "wasi:sockets/", 13) == 0) {
        charData += 13;
        length -= 13;

        if (compareName(charData, length, "udp")) {
            instanceId = InstanceFileSystemTypes02;
        } else if (compareName(charData, length, "tcp")) {
            instanceId = InstanceFileSystemPreOpens02;
        }
    }
#if ENABLE_WASI_NN
    else if (length > 9 && memcmp(charData, "wasi:nn/", 8) == 0) {
        charData += 8;
        length -= 8;

        if (compareName(charData, length, "tensor")) {
            instanceId = InstanceWasiNNTensor02;
        } else if (compareName(charData, length, "errors")) {
            instanceId = InstanceWasiNNErrors02;
        } else if (compareName(charData, length, "inference")) {
            instanceId = InstanceWasiNNInference02;
        } else if (compareName(charData, length, "graph")) {
            instanceId = InstanceWasiNNGraph02;
        }
    }
#endif

    if (instanceId == InstanceUnknown) {
        return nullptr;
    }

    ComponentInstance* instance = findWasiComponentInstance(store, instanceId);
    if (instance != nullptr) {
        return instance;
    }

    ComponentInstanceWasi02 instanceCreator(store, postfix);
    instance = instanceCreator.loadInstance(instanceId, false);
    store->wasiData()->wasiInstances()[instanceId] = instance;
    return instance;
}

const FunctionType* getWasiFunctionType(LiftedWasiFunction* function)
{
    return function->functionType();
}

#undef INSERT_INTO
#undef MAKE_MAKE_RESULT

} // namespace Walrus

#endif
