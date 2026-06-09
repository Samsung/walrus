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
#include "runtime/Memory.h"

namespace Walrus {

enum OptionalTypes : uint8_t {
    optionalNone = 0,
    optionalSome = 1,
};

enum ResultTypes : uint8_t {
    resultOk = 0,
    resultError = 1,
};

enum StreamErrorTypes : uint8_t {
    streamErrLastOperationFailed = 0,
    streamErrClosed = 1,
};

enum DescriptorFlags : uint32_t {
    flagRead = 1 << 0,
    flagWrite = 1 << 1,
    flagFileIntegritySync = 1 << 2,
    flagDataIntegritySync = 1 << 3,
    flagRequestedWriteSync = 1 << 4,
    flagMutateDirectory = 1 << 5,
};

enum OpenFlags : uint32_t {
    openCreate = 1 << 0,
    openDirectory = 1 << 1,
    openExclusive = 1 << 2,
    openTruncate = 1 << 3,
};

enum FileType : uint8_t {
    fileTypeUnknown = 0,
    fileTypeBlockDevice = 1,
    fileTypeCharacterDevice = 2,
    fileTypeDirectory = 3,
    fileTypeFifo = 4,
    fileTypeSymbolicLink = 5,
    fileTypeRegularFile = 6,
    fileTypeSocket = 7,
};

static void throwNoMemory(ExecutionState& state)
{
    std::string message = "out of memory";
    Trap::throwException(state, message);
}

static inline ComponentResourceWasiStream* asStream(ComponentHandle* handle)
{
    ASSERT(handle->kind() == ComponentHandle::ResourceWasiInputStreamKind || handle->kind() == ComponentHandle::ResourceWasiOutputStreamKind);
    return reinterpret_cast<ComponentResourceWasiStream*>(handle);
}

static inline ComponentResourceWasiFile* asFile(ComponentHandle* handle)
{
    ASSERT(handle->kind() == ComponentHandle::ResourceWasiFileKind);
    return reinterpret_cast<ComponentResourceWasiFile*>(handle);
}

static inline ComponentResourceWasiDirectory* asDirectory(ComponentHandle* handle)
{
    ASSERT(handle->kind() == ComponentHandle::ResourceWasiDirectoryKind);
    return reinterpret_cast<ComponentResourceWasiDirectory*>(handle);
}

static inline long int maxFileOffset(uint64_t offset)
{
    unsigned long int max = ~static_cast<long unsigned int>(0) >> 1;
    return offset > max ? static_cast<long int>(max) : static_cast<long int>(offset);
}

void WasiRefCountedFile::destroyFile()
{
    ASSERT(m_refCount == 0);
    if (m_file != stdin && m_file != stdout && m_file != stderr) {
        fclose(m_file);
    }
    delete this;
}

LiftedWasiFunction::~LiftedWasiFunction()
{
    TypeStore::ReleaseRef(m_functionType->subTypeList());
}

void callWasiFunction(ExecutionState& state, Value* argv, Value* result, LiftedWasiFunction* function, CanonOptions* options)
{
    ComponentInstance* instance = function->instance();

    switch (function->type()) {
    case LiftedWasiFunction::ioPollableBlock02: {
        uint32_t index = argv[0].asI32();
        ComponentHandle* handle = options->instance()->getHandle(state, index);
        if (handle->kind() != ComponentHandle::ResourceWasiPollableKind) {
            ComponentInstance::throwInvalidHandle(state, index);
        }
        break;
    }
    case LiftedWasiFunction::ioOutputStreamCheckWrite02: {
        uint32_t offset = argv[1].asI32();
        uint64_t value = 0xffffffff;
        options->memory()->store(state, offset, 8, value);
        options->memory()->buffer()[offset] = resultOk;
        break;
    }
    case LiftedWasiFunction::ioInputStreamRead02: {
        uint32_t index = argv[0].asI32();
        long int size = maxFileOffset(argv[1].asI64());
        uint32_t offset = argv[2].asI32();

        ASSERT(!options->memory()->is64());
        options->memoryCheckRange32(state, 4, offset, 12);

        ComponentHandle* handle = options->instance()->getHandle(state, index);
        if (handle->kind() != ComponentHandle::ResourceWasiInputStreamKind) {
            ComponentInstance::throwInvalidHandle(state, index);
        }

        ComponentResourceWasiStream* stream = asStream(handle);
        if (stream->isClosed()) {
            options->memory()->buffer()[offset + 4] = streamErrClosed;
            options->memory()->buffer()[offset] = resultError;
            break;
        }

        if (stream->seekNeeded()) {
            fseek(stream->file(), stream->offset(), SEEK_SET);
        }
        std::vector<uint8_t> buffer(size);
        size_t read = fread(buffer.data(), 1, static_cast<size_t>(size), stream->file());
        stream->advanceOffset(read);
        uint32_t start = options->memoryMalloc32(state, 1, read);
        memcpy(options->memory()->buffer() + start, buffer.data(), size);

        options->memory()->buffer()[offset] = resultOk;
        uint32_t* list = reinterpret_cast<uint32_t*>(options->memory()->buffer() + offset);
        list[1] = start;
        list[2] = read;

        if (read < static_cast<size_t>(size)) {
            stream->dropFileRef();
        }
        break;
    }
    case LiftedWasiFunction::ioInputStreamSubscribe02:
    case LiftedWasiFunction::ioOutputStreamSubscribe02: {
        uint32_t index = argv[0].asI32();
        ComponentHandle* handle = options->instance()->getHandle(state, index);

        ComponentHandle::Kind kind = ComponentHandle::ResourceWasiInputStreamKind;
        if (function->type() == LiftedWasiFunction::ioOutputStreamSubscribe02) {
            kind = ComponentHandle::ResourceWasiOutputStreamKind;
        }

        if (handle->kind() != kind) {
            ComponentInstance::throwInvalidHandle(state, index);
        }

        ComponentResource* resource = new ComponentResourceWasiPollable(instance->type()->getType(0)->asTypeResource(), asStream(handle));
        result[0] = Value(static_cast<int32_t>(options->instance()->appendHandle(state, resource)));
        break;
    }
    case LiftedWasiFunction::ioOutputStreamWrite02: {
        uint32_t index = argv[0].asI32();
        uint32_t offset = argv[3].asI32();

        ComponentHandle* handle = options->instance()->getHandle(state, index);
        if (handle->kind() != ComponentHandle::ResourceWasiOutputStreamKind) {
            ComponentInstance::throwInvalidHandle(state, index);
        }

        ASSERT(!options->memory()->is64());
        options->memoryCheckRange32(state, 4, offset, 12);
        ComponentResourceWasiStream* stream = asStream(handle);
        if (stream->isClosed()) {
            options->memory()->buffer()[offset + 4] = streamErrClosed;
            options->memory()->buffer()[offset] = resultError;
            break;
        }

        uint32_t bufferStart = argv[1].asI32();
        uint32_t bufferSize = argv[2].asI32();
        options->memoryCheckRange32(state, 1, bufferStart, bufferSize);
        if (stream->seekNeeded()) {
            fseek(stream->file(), stream->offset(), SEEK_SET);
        }
        size_t written = fwrite(options->memory()->buffer() + bufferStart, bufferSize, 1, stream->file());
        stream->advanceOffset(written);

        options->memory()->buffer()[offset] = resultOk;
        break;
    }
    case LiftedWasiFunction::ioOutputStreamBlockingFlush02: {
        uint32_t index = argv[0].asI32();
        uint32_t offset = argv[1].asI32();

        ComponentHandle* handle = options->instance()->getHandle(state, index);
        if (handle->kind() != ComponentHandle::ResourceWasiOutputStreamKind) {
            ComponentInstance::throwInvalidHandle(state, index);
        }

        ASSERT(!options->memory()->is64());
        options->memoryCheckRange32(state, 4, offset, 12);
        options->memory()->buffer()[offset] = resultOk;
        break;
    }
    case LiftedWasiFunction::cliGetEnvironment02: {
        uint32_t offset = argv[0].asI32();
        const std::vector<std::pair<std::string, std::string>>& environment = instance->store()->wasiData()->environment();

        ASSERT(!options->memory()->is64());
        if (environment.size() >= Memory::s_maxMemory32 >> 4) {
            throwNoMemory(state);
        }

        uint32_t length = static_cast<uint32_t>(environment.size());
        uint32_t start = options->memoryMalloc32(state, 4, length << 4);
        uint32_t* argBuffer = reinterpret_cast<uint32_t*>(options->memory()->buffer() + start);
        options->memory()->store(state, offset, 0, start);
        options->memory()->store(state, offset, 4, length);

        for (auto& it : environment) {
            if (it.first.length() >= Component::MaxStringByteLength || it.second.length() >= Component::MaxStringByteLength) {
                throwNoMemory(state);
            }
            length = static_cast<uint32_t>(it.first.length());
            start = static_cast<uint32_t>(options->storeLatin1String(state, reinterpret_cast<const uint8_t*>(it.first.data()), &length));
            *argBuffer++ = start;
            *argBuffer++ = length;
            length = static_cast<uint32_t>(it.second.length());
            start = static_cast<uint32_t>(options->storeLatin1String(state, reinterpret_cast<const uint8_t*>(it.second.data()), &length));
            *argBuffer++ = start;
            *argBuffer++ = length;
        }
        break;
    }
    case LiftedWasiFunction::cliGetArguments02: {
        uint32_t offset = argv[0].asI32();
        const std::vector<std::string>& arguments = instance->store()->wasiData()->arguments();

        ASSERT(!options->memory()->is64());
        if (arguments.size() >= Memory::s_maxMemory32 >> 3) {
            throwNoMemory(state);
        }

        uint32_t length = static_cast<uint32_t>(arguments.size());
        uint32_t start = options->memoryMalloc32(state, 4, length << 3);
        uint32_t* argBuffer = reinterpret_cast<uint32_t*>(options->memory()->buffer() + start);
        options->memory()->store(state, offset, 0, start);
        options->memory()->store(state, offset, 4, length);

        for (auto& it : arguments) {
            if (it.length() >= Component::MaxStringByteLength) {
                throwNoMemory(state);
            }
            length = static_cast<uint32_t>(it.length());
            start = static_cast<uint32_t>(options->storeLatin1String(state, reinterpret_cast<const uint8_t*>(it.data()), &length));
            *argBuffer++ = start;
            *argBuffer++ = length;
        }
        break;
    }
    case LiftedWasiFunction::cliGetStdin02:
    case LiftedWasiFunction::cliGetStdout02:
    case LiftedWasiFunction::cliGetStderr02: {
        FILE* file = stdin;
        if (function->type() == LiftedWasiFunction::cliGetStdout02) {
            file = stdout;
        } else if (function->type() == LiftedWasiFunction::cliGetStderr02) {
            file = stderr;
        }
        WasiRefCountedFile* fileRef = new WasiRefCountedFile(file);
        ComponentTypeResource* resourceType = instance->type()->getType(0)->asTypeResource();
        ComponentResource* resource = new ComponentResourceWasiStream(resourceType, ComponentHandle::ResourceWasiOutputStreamKind, fileRef, ComponentResourceWasiStream::NoSeek);
        result[0] = Value(static_cast<int32_t>(options->instance()->appendHandle(state, resource)));
        break;
    }
    case LiftedWasiFunction::cliGetTerminalStdout02: {
        ComponentResource* resource = new ComponentResourceWasiTerminal(instance->type()->getType(0)->asTypeResource(), 1);
        uint32_t offset = argv[0].asI32();
        uint32_t value = options->instance()->appendHandle(state, resource);
        options->memory()->store(state, offset, 4, value);
        options->memory()->buffer()[offset] = optionalSome;
        break;
    }
    case LiftedWasiFunction::clockMonotonicNow02: {
        WasiStoreData* data = instance->store()->wasiData();
        clock_t now = clock();
        uint64_t now64 = data->prevNow() + static_cast<uint64_t>(now - data->prevClockNow()) * (1000000000 / CLOCKS_PER_SEC);
        data->setPrevClockNow(now);
        data->setPrevNow(now64);
        result[0] = Value(static_cast<int64_t>(now64));
        break;
    }
    case LiftedWasiFunction::fileSystemDescriptorReadViaStream02: {
        uint32_t descriptorIndex = argv[0].asI32();
        long int offset = maxFileOffset(argv[1].asI64());
        uint32_t resultOffset = argv[2].asI32();

        ComponentHandle* handle = options->instance()->getHandle(state, descriptorIndex);
        if (handle->kind() != ComponentHandle::ResourceWasiFileKind) {
            ComponentInstance::throwInvalidHandle(state, descriptorIndex);
        }

        WasiRefCountedFile* fileRef = asFile(handle)->addFileRef();
        ComponentTypeResource* resourceType = instance->type()->getType(2)->asTypeResource();
        ComponentResource* resource = new ComponentResourceWasiStream(resourceType, ComponentHandle::ResourceWasiInputStreamKind, fileRef, offset);
        uint32_t resultResource = options->instance()->appendHandle(state, resource);
        options->memory()->store(state, resultOffset, 4, resultResource);
        options->memory()->buffer()[resultOffset] = resultOk;
        break;
    }
    case LiftedWasiFunction::fileSystemDescriptorStat02: {
        uint32_t descriptorIndex = argv[0].asI32();
        uint32_t offset = argv[1].asI32();

        ComponentHandle* handle = options->instance()->getHandle(state, descriptorIndex);
        if (handle->kind() != ComponentHandle::ResourceWasiFileKind) {
            ComponentInstance::throwInvalidHandle(state, descriptorIndex);
        }

        ComponentResourceWasiFile* file = asFile(handle);

        options->memoryCheckRange32(state, 8, offset, 96);
        uint8_t* buffer = options->memory()->buffer() + offset;
        buffer[0] = resultOk;
        buffer[8] = fileTypeRegularFile;
        *reinterpret_cast<uint64_t*>(buffer + 16) = 1;
        fseek(file->file(), 0, SEEK_END);
        *reinterpret_cast<uint64_t*>(buffer + 24) = static_cast<uint64_t>(ftell(file->file()));
        // Optional(dateTime) is 24 bytes long.
        buffer[32] = optionalNone;
        buffer[56] = optionalNone;
        buffer[80] = optionalNone;
        break;
    }
    case LiftedWasiFunction::fileSystemDescriptorOpenAt02: {
        uint32_t descriptorIndex = argv[0].asI32();
        uint32_t pathStart = argv[2].asI32();
        uint32_t pathSize = argv[3].asI32();
        uint32_t openFlags = argv[4].asI32();
        uint32_t flags = argv[5].asI32();
        uint32_t resultOffset = argv[6].asI32();

        ASSERT(!options->memory()->is64());

        ComponentHandle* handle = options->instance()->getHandle(state, descriptorIndex);
        if (handle->kind() != ComponentHandle::ResourceWasiDirectoryKind) {
            ComponentInstance::throwInvalidHandle(state, descriptorIndex);
        }

        const char* access = "r";
        bool invalid = false;
        if ((flags & flagWrite) != 0) {
            if (openFlags & openCreate) {
                access = (flags & flagRead) != 0 ? "w+" : "w";
            } else {
                access = "r+";
            }
        } else if ((flags & flagRead) != 0) {
            if ((openFlags & openCreate) != 0) {
                invalid = true;
            }
        } else {
            invalid = true;
        }

        if ((openFlags & (openDirectory | openExclusive | openTruncate)) != 0) {
            invalid = true;
        }

        if (invalid) {
            std::string message = "invalid/unsupported flag combination for open";
            Trap::throwException(state, message);
        }

        std::string path = asDirectory(handle)->realPath();
        path.append("/");
        CanonOptions::UtfData utfData;
        options->validateString(state, pathStart, pathSize, &utfData);
        if (options->encoding() == ComponentCanonOptions::Utf8) {
            path.append(reinterpret_cast<const char*>(utfData.buffer()), utfData.length());
        } else {
            std::vector<uint8_t> utf8String(utfData.utf8Length());
            utfData.toUtf8String(utf8String.data());
            path.append(reinterpret_cast<const char*>(utf8String.data()), utf8String.size());
        }
        FILE* file = fopen(path.c_str(), access);

        if (file == NULL) {
            // Support more error codes.
            options->memory()->store(state, resultOffset, 4, 0);
            options->memory()->buffer()[resultOffset] = resultError;
            break;
        }

        WasiRefCountedFile* fileRef = new WasiRefCountedFile(file);
        ComponentResource* resource = new ComponentResourceWasiFile(instance->type()->getType(0)->asTypeResource(), fileRef);
        uint32_t resultResource = options->instance()->appendHandle(state, resource);
        options->memory()->store(state, resultOffset, 4, resultResource);
        options->memory()->buffer()[resultOffset] = resultOk;
        break;
    }
    case LiftedWasiFunction::fileSystemGetDirectories02: {
        uint32_t offset = argv[0].asI32();
        const std::vector<std::pair<std::string, std::string>>& preOpens = instance->store()->wasiData()->preOpens();

        ASSERT(!options->memory()->is64());
        if (preOpens.size() >= Memory::s_maxMemory32 / 12) {
            throwNoMemory(state);
        }

        uint32_t length = static_cast<uint32_t>(preOpens.size());
        uint32_t start = options->memoryMalloc32(state, 4, length * 12);
        uint32_t* argBuffer = reinterpret_cast<uint32_t*>(options->memory()->buffer() + start);
        options->memory()->store(state, offset, 0, start);
        options->memory()->store(state, offset, 4, length);

        for (auto& it : preOpens) {
            if (it.first.length() >= Component::MaxStringByteLength || it.second.length() >= Component::MaxStringByteLength) {
                throwNoMemory(state);
            }

            ComponentResource* resource = new ComponentResourceWasiDirectory(instance->type()->getType(0)->asTypeResource(), it.first, it.second);
            *argBuffer++ = options->instance()->appendHandle(state, resource);
            length = static_cast<uint32_t>(it.first.length());
            start = static_cast<uint32_t>(options->storeLatin1String(state, reinterpret_cast<const uint8_t*>(it.first.data()), &length));
            *argBuffer++ = start;
            *argBuffer++ = length;
        }
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
    case ComponentHandle::ResourceWasiInputStreamKind:
    case ComponentHandle::ResourceWasiOutputStreamKind:
        if (asStream(handle)->pollableCount() != 0) {
            std::string message = "stream cannot be destroyed (has assigned pollable)";
            Trap::throwException(state, message);
        }
        break;
    case ComponentHandle::ResourceWasiPollableKind:
    case ComponentHandle::ResourceWasiTerminalKind:
    case ComponentHandle::ResourceWasiFileKind:
    case ComponentHandle::ResourceWasiDirectoryKind:
        break;
    default:
        return false;
    }
    return true;
}

} // namespace Walrus

#endif
