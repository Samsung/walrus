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

#include "wasi/WASI.h"
#include "wasi/WASI02.h"
#include "wasi/WASI02Impl.h"
#include "runtime/Memory.h"

namespace Walrus {

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

static uvwasi_errno_t resolvePathByComponents(ComponentResourceWasiDirectory* directory, const std::string& guestPath, uvwasi_lookupflags_t pathFlags, std::string& resolvedPath)
{
    if (guestPath.empty()) {
        return UVWASI_EINVAL;
    }

    if (guestPath.front() == '/') {
        return UVWASI_ENOTCAPABLE;
    }

    if (guestPath.find('\0') != std::string::npos) {
        return UVWASI_EINVAL;
    }

    if ((pathFlags & ~UVWASI_LOOKUP_SYMLINK_FOLLOW) != 0) {
        return UVWASI_EINVAL;
    }

    const std::string mappedPath = directory->mappedPath();
    const std::string realPath = directory->realPath();

    size_t slash = guestPath.find('/');

    while (slash != std::string::npos) {
        if (slash > 0 && guestPath[slash - 1] != '/') {
            std::string prefix = guestPath.substr(0, slash);

            uvwasi_errno_t error = WASI::resolvePath(mappedPath, realPath, prefix, UVWASI_LOOKUP_SYMLINK_FOLLOW, resolvedPath);
            if (error != UVWASI_ESUCCESS) {
                return error;
            }
        }

        slash = guestPath.find('/', slash + 1);
    }

    return WASI::resolvePath(mappedPath, realPath, guestPath, UVWASI_LOOKUP_SYMLINK_FOLLOW, resolvedPath);
}

static inline long int maxFileOffset(uint64_t offset)
{
    unsigned long int max = ~static_cast<long unsigned int>(0) >> 1;
    return offset > max ? static_cast<long int>(max) : static_cast<long int>(offset);
}

void WasiRefCountedFile::destroyFile()
{
    ASSERT(m_refCount == 0);
    if (m_uvDescriptor > WASI_STDERR) {
        uv_fs_t req;
        uv_fs_close(nullptr, &req, m_uvDescriptor, nullptr);
        uv_fs_req_cleanup(&req);
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

        if (!(stream->file()->flags() & DescriptorFlags::flagRead)) {
            options->memory()->buffer()[offset + 4] = streamErrClosed;
            options->memory()->buffer()[offset] = resultError;
            break;
        }

        uv_fs_t req;
        std::vector<uint8_t> buffer;
        buffer.reserve(size);
        uv_buf_t iov = uv_buf_init((char*)buffer.data(), size);

        int r = uv_fs_read(nullptr, &req, stream->fileDescriptor(), &iov, 1, -1, nullptr);
        if (r < 0) {
            options->memory()->buffer()[offset + 4] = streamErrClosed;
            options->memory()->buffer()[offset] = resultError;
            break;
        }
        size_t read = req.result;

        uint32_t start = options->memoryMalloc32(state, 1, read);
        memcpy(options->memory()->buffer() + start, buffer.data(), read);

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
    case LiftedWasiFunction::ioOutputStreamBlockingWriteAndFlush02:
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

        if (!(stream->file()->flags() & DescriptorFlags::flagWrite)) {
            options->memory()->buffer()[offset + 4] = streamErrClosed;
            options->memory()->buffer()[offset] = resultError;
            break;
        }

        uint32_t bufferStart = argv[1].asI32();
        uint32_t bufferSize = argv[2].asI32();
        options->memoryCheckRange32(state, 1, bufferStart, bufferSize);

        uv_fs_t req;
        uv_buf_t iovs = uv_buf_init((char*)(options->memory()->buffer()) + bufferStart, bufferSize);
        int r = uv_fs_write(nullptr, &req, stream->fileDescriptor(), &iovs, 1, -1, nullptr);
        if (r < 0) {
            options->memory()->buffer()[offset + 4] = streamErrClosed;
            options->memory()->buffer()[offset] = resultError;
            break;
        }
        stream->advanceOffset(req.result);

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
    case LiftedWasiFunction::cliGetStdin02: {
        WasiRefCountedFile* fileRef = new WasiRefCountedFile(WASI_STDIN, std::string(), DescriptorFlags::flagRead);

        ComponentTypeResource* resourceType = instance->type()->getType(0)->asTypeResource();
        ComponentResource* resource = new ComponentResourceWasiStream(resourceType, ComponentHandle::ResourceWasiOutputStreamKind, fileRef);
        result[0] = Value(static_cast<int32_t>(options->instance()->appendHandle(state, resource)));
        break;
    }
    case LiftedWasiFunction::cliGetStdout02: {
        WasiRefCountedFile* fileRef = new WasiRefCountedFile(WASI_STDOUT, std::string(), DescriptorFlags::flagWrite);

        ComponentTypeResource* resourceType = instance->type()->getType(0)->asTypeResource();
        ComponentResource* resource = new ComponentResourceWasiStream(resourceType, ComponentHandle::ResourceWasiOutputStreamKind, fileRef);
        result[0] = Value(static_cast<int32_t>(options->instance()->appendHandle(state, resource)));
        break;
    }
    case LiftedWasiFunction::cliGetStderr02: {
        WasiRefCountedFile* fileRef = new WasiRefCountedFile(WASI_STDERR, std::string(), DescriptorFlags::flagWrite);

        ComponentTypeResource* resourceType = instance->type()->getType(0)->asTypeResource();
        ComponentResource* resource = new ComponentResourceWasiStream(resourceType, ComponentHandle::ResourceWasiOutputStreamKind, fileRef);
        result[0] = Value(static_cast<int32_t>(options->instance()->appendHandle(state, resource)));
        break;
    }
    case LiftedWasiFunction::cliGetTerminalStdin02: {
        ComponentResource* resource = new ComponentResourceWasiTerminal(instance->type()->getType(0)->asTypeResource(), WASI_STDIN);
        uint32_t offset = argv[0].asI32();
        uint32_t value = options->instance()->appendHandle(state, resource);
        options->memory()->store(state, offset, 4, value);
        options->memory()->buffer()[offset] = optionalSome;
        break;
    }
    case LiftedWasiFunction::cliGetTerminalStdout02: {
        ComponentResource* resource = new ComponentResourceWasiTerminal(instance->type()->getType(0)->asTypeResource(), WASI_STDOUT);
        uint32_t offset = argv[0].asI32();
        uint32_t value = options->instance()->appendHandle(state, resource);
        options->memory()->store(state, offset, 4, value);
        options->memory()->buffer()[offset] = optionalSome;
        break;
    }
    case LiftedWasiFunction::cliGetTerminalStderr02: {
        ComponentResource* resource = new ComponentResourceWasiTerminal(instance->type()->getType(0)->asTypeResource(), WASI_STDERR);
        uint32_t offset = argv[0].asI32();
        uint32_t value = options->instance()->appendHandle(state, resource);
        options->memory()->store(state, offset, 4, value);
        options->memory()->buffer()[offset] = optionalSome;
        break;
    }
    case LiftedWasiFunction::clockMonotonicNow02: {
        uv_timespec64_t time;
        int r = uv_clock_gettime(UV_CLOCK_MONOTONIC, &time);
        if (r < 0) {
            result[0] = Value(static_cast<int64_t>(0));
            break;
        }

        result[0] = Value(time.tv_sec);

        break;
    }
    case LiftedWasiFunction::clockSubscribeDuration02: {
        uint64_t duration = argv[0].asI64();

        uint64_t start = uv_hrtime() + duration;
        ComponentResource* timer = new ComponentResourceWasiPollableTimer(instance->type()->getType(2)->asTypeResource(), start);
        result[0] = Value(static_cast<int32_t>(options->instance()->appendHandle(state, timer)));
        break;
    }
    case LiftedWasiFunction::clockWallNow02: {
        uint32_t offset = argv[0].asI32();

        uv_timespec64_t time;
        int r = uv_clock_gettime(UV_CLOCK_REALTIME, &time);
        if (r < 0) {
            result[0] = Value(static_cast<int64_t>(0));
            break;
        }

        options->memory()->store(state, offset, time.tv_sec);
        options->memory()->store(state, offset, 8, time.tv_nsec);
        break;
    }
    case LiftedWasiFunction::fileSystemDescriptorReadViaStream02:
    case LiftedWasiFunction::fileSystemDescriptorWriteViaStream02:
    case LiftedWasiFunction::fileSystemDescriptorAppendViaStream02: {
        ComponentHandle::Kind streamKind = ComponentHandle::ResourceWasiInputStreamKind;
        if (function->type() == LiftedWasiFunction::fileSystemDescriptorWriteViaStream02 || function->type() == LiftedWasiFunction::fileSystemDescriptorAppendViaStream02) {
            streamKind = ComponentHandle::ResourceWasiOutputStreamKind;
        }

        uint32_t descriptorIndex = argv[0].asI32();
        long int offset = maxFileOffset(argv[1].asI64());
        uint32_t resultOffset = argv[2].asI32();

        ASSERT(!options->memory()->is64());
        options->memoryCheckRange32(state, 4, resultOffset, 8);

        ComponentHandle* handle = options->instance()->getHandle(state, descriptorIndex);
        if (handle->kind() != ComponentHandle::ResourceWasiFileKind) {
            ComponentInstance::throwInvalidHandle(state, descriptorIndex);
        }

        uint32_t flags = asFile(handle)->file()->flags();
        if (streamKind == ComponentHandle::ResourceWasiInputStreamKind && !(flags & DescriptorFlags::flagRead)) {
            options->memory()->buffer()[resultOffset + 4] = streamErrClosed;
            options->memory()->buffer()[resultOffset] = resultError;
            break;
        } else if (streamKind == ComponentHandle::ResourceWasiOutputStreamKind && !(flags & DescriptorFlags::flagWrite)) {
            options->memory()->buffer()[resultOffset + 4] = streamErrClosed;
            options->memory()->buffer()[resultOffset] = resultError;
            break;
        }


        WasiRefCountedFile* fileRef = asFile(handle)->addFileRef();
        ComponentTypeResource* resourceType = instance->type()->getType(2)->asTypeResource();
        ComponentResource* resource = new ComponentResourceWasiStream(resourceType, streamKind, fileRef, offset);
        uint32_t resultResource = options->instance()->appendHandle(state, resource);
        options->memory()->store(state, resultOffset, 4, resultResource);
        options->memory()->buffer()[resultOffset] = resultOk;
        break;
    }
    case Walrus::LiftedWasiFunction::fileSystemDescriptorGetFlags02: {
        uint32_t descriptorIndex = argv[0].asI32();
        uint32_t resultOffset = argv[1].asI32();

        int result = 0;
        ComponentHandle* handle = options->instance()->getHandle(state, descriptorIndex);
        if (handle->kind() != ComponentHandle::ResourceWasiFileKind) {
            ComponentInstance::throwInvalidHandle(state, descriptorIndex);
        }

        uint32_t flags = asFile(handle)->file()->flags();
        if (flags & DescriptorFlags::flagFileIntegritySync) {
            result |= DescriptorFlags::flagFileIntegritySync;
        }
        if (flags & DescriptorFlags::flagDataIntegritySync) {
            result |= DescriptorFlags::flagDataIntegritySync;
        }
        if (flags & DescriptorFlags::flagRequestedWriteSync) {
            result |= DescriptorFlags::flagRequestedWriteSync;
        }
        if (flags & DescriptorFlags::flagMutateDirectory) {
            result |= DescriptorFlags::flagMutateDirectory;
        }
        if (flags & DescriptorFlags::flagRead) {
            result |= DescriptorFlags::flagRead;
        }
        if (flags & DescriptorFlags::flagWrite) {
            result |= DescriptorFlags::flagWrite;
        }

        options->memory()->store(state, resultOffset, resultOk);
        options->memory()->store(state, resultOffset, 4, result);
        break;
    }
    case LiftedWasiFunction::fileSystemDescriptorStat02: {
        uint32_t descriptorIndex = argv[0].asI32();
        uint32_t offset = argv[1].asI32();

        std::string path = "";
        ComponentHandle* handle = options->instance()->getHandle(state, descriptorIndex);
        if (handle->kind() == ComponentHandle::ResourceWasiDirectoryKind) {
            ComponentResourceWasiDirectory* dir = asDirectory(handle);
            path = dir->realPath();
        } else if (handle->kind() == ComponentHandle::ResourceWasiFileKind) {
            ComponentResourceWasiFile* file = asFile(handle);
            path = file->path();
        } else {
            ComponentInstance::throwInvalidHandle(state, descriptorIndex);
        }

        options->memoryCheckRange32(state, 8, offset, 96);

        uv_fs_t req;
        int r = uv_fs_stat(nullptr, &req, path.c_str(), nullptr);
        uv_fs_req_cleanup(&req);

        if (r < 0) {
            options->memory()->store(state, offset, 4, 0);
            options->memory()->buffer()[offset] = resultError;
            break;
        }

        FileType ft = FileType::Unknown;
        switch (req.statbuf.st_mode) {
        case UV_DIRENT_FILE:
            ft = FileType::RegularFile;
            break;
        case UV_DIRENT_SOCKET:
            ft = FileType::Socket;
            break;
        case UV_DIRENT_LINK:
            ft = FileType::SymbolicLink;
            break;
        case UV_DIRENT_BLOCK:
            ft = FileType::BlockDevice;
            break;
        case UV_DIRENT_DIR:
            ft = FileType::Directory;
            break;
        case UV_DIRENT_CHAR:
            ft = FileType::CharacterDevice;
            break;
        case UV_DIRENT_FIFO:
            ft = FileType::Fifo;
            break;
        }

        uint8_t* buffer = options->memory()->buffer() + offset;
        buffer[0] = resultOk;
        buffer[8] = ft;
        *reinterpret_cast<uint64_t*>(buffer + 16) = req.statbuf.st_nlink;
        *reinterpret_cast<uint64_t*>(buffer + 24) = req.statbuf.st_size;
        // Optional(dateTime) is 24 bytes long.
        options->memory()->store(state, offset, 36, req.statbuf.st_atim.tv_sec);
        options->memory()->store(state, offset, 44, (int32_t)req.statbuf.st_atim.tv_nsec);
        buffer[32] = optionalSome;
        options->memory()->store(state, offset, 60, req.statbuf.st_mtim.tv_sec);
        options->memory()->store(state, offset, 68, (int32_t)req.statbuf.st_mtim.tv_nsec);
        buffer[56] = optionalSome;
        options->memory()->store(state, offset, 84, req.statbuf.st_ctim.tv_sec);
        options->memory()->store(state, offset, 92, (int32_t)req.statbuf.st_ctim.tv_nsec);
        buffer[80] = optionalSome;
        break;
    }
    case LiftedWasiFunction::fileSystemDescriptorStatAt02: {
        uint32_t descriptorIndex = argv[0].asI32();
        uvwasi_lookupflags_t pathFlags = static_cast<uvwasi_lookupflags_t>(argv[1].asI32());
        uint32_t pathStart = argv[2].asI32();
        uint32_t pathSize = argv[3].asI32();
        uint32_t offset = argv[4].asI32();
        ASSERT(!options->memory()->is64());

        ComponentHandle* handle = options->instance()->getHandle(state, descriptorIndex);
        if (handle->kind() != ComponentHandle::ResourceWasiDirectoryKind) {
            ComponentInstance::throwInvalidHandle(state, descriptorIndex);
        }

        std::string guestPath;
        CanonOptions::UtfData utfData;
        options->validateString(state, pathStart, pathSize, &utfData);
        if (options->encoding() == ComponentCanonOptions::Utf8) {
            guestPath.assign(reinterpret_cast<const char*>(utfData.buffer()), utfData.length());
        } else {
            std::vector<uint8_t> utf8String(utfData.utf8Length());
            utfData.toUtf8String(utf8String.data());
            guestPath.assign(reinterpret_cast<const char*>(utf8String.data()), utf8String.size());
        }

        std::string resolvedPath;

        uvwasi_errno_t resolveError = resolvePathByComponents(asDirectory(handle), guestPath, pathFlags, resolvedPath);

        if (resolveError != UVWASI_ESUCCESS) {
            options->memory()->store(state, offset, 4, FilesystemError::badDescriptor);
            options->memory()->buffer()[offset] = resultError;
            break;
        }

        uv_fs_t req;
        int r = uv_fs_stat(nullptr, &req, resolvedPath.c_str(), nullptr);
        uv_fs_req_cleanup(&req);

        if (r < 0) {
            options->memory()->store(state, offset, 4, 0);
            options->memory()->buffer()[offset] = resultError;
            break;
        }

        FileType fp = FileType::Unknown;
        switch (req.statbuf.st_mode) {
        case UV_DIRENT_FILE:
            fp = FileType::RegularFile;
            break;
        case UV_DIRENT_SOCKET:
            fp = FileType::Socket;
            break;
        case UV_DIRENT_LINK:
            fp = FileType::SymbolicLink;
            break;
        case UV_DIRENT_BLOCK:
            fp = FileType::BlockDevice;
            break;
        case UV_DIRENT_DIR:
            fp = FileType::Directory;
            break;
        case UV_DIRENT_CHAR:
            fp = FileType::CharacterDevice;
            break;
        case UV_DIRENT_FIFO:
            fp = FileType::Fifo;
            break;
        }

        uint8_t* buffer = options->memory()->buffer() + offset;
        buffer[0] = resultOk;
        buffer[8] = fp;
        *reinterpret_cast<uint64_t*>(buffer + 16) = req.statbuf.st_nlink;
        *reinterpret_cast<uint64_t*>(buffer + 24) = req.statbuf.st_size;
        // Optional(dateTime) is 24 bytes long.
        options->memory()->store(state, offset, 36, req.statbuf.st_atim.tv_sec);
        options->memory()->store(state, offset, 44, (int32_t)req.statbuf.st_atim.tv_nsec);
        buffer[32] = optionalSome;
        options->memory()->store(state, offset, 60, req.statbuf.st_mtim.tv_sec);
        options->memory()->store(state, offset, 68, (int32_t)req.statbuf.st_mtim.tv_nsec);
        buffer[56] = optionalSome;
        options->memory()->store(state, offset, 84, req.statbuf.st_ctim.tv_sec);
        options->memory()->store(state, offset, 92, (int32_t)req.statbuf.st_ctim.tv_nsec);
        buffer[80] = optionalSome;
        break;
    }
    case LiftedWasiFunction::fileSystemDescriptorOpenAt02: {
        uint32_t descriptorIndex = argv[0].asI32();
        uvwasi_lookupflags_t pathFlags = static_cast<uvwasi_lookupflags_t>(argv[1].asI32());
        uint32_t pathStart = argv[2].asI32();
        uint32_t pathSize = argv[3].asI32();
        OpenFlags openFlags = static_cast<OpenFlags>(argv[4].asI32());
        DescriptorFlags descFlags = static_cast<DescriptorFlags>(argv[5].asI32());
        uint32_t resultOffset = argv[6].asI32();
        ASSERT(!options->memory()->is64());

        ComponentHandle* handle = options->instance()->getHandle(state, descriptorIndex);
        if (handle->kind() != ComponentHandle::ResourceWasiDirectoryKind) {
            ComponentInstance::throwInvalidHandle(state, descriptorIndex);
        }

        ComponentResourceWasiDirectory* dir = asDirectory(handle);

        bool baseHasMutate = (dir->flags() & DescriptorFlags::flagMutateDirectory);
        bool flagHasMutate = (descFlags & DescriptorFlags::flagMutateDirectory);
        bool flagHasWrite = (descFlags & DescriptorFlags::flagWrite);
        bool flagHasCreate = (((openFlags & OpenFlags::openTruncate) == OpenFlags::openTruncate) || ((openFlags & OpenFlags::openCreate) == OpenFlags::openCreate));

        if ((flagHasMutate && !baseHasMutate) || (flagHasWrite && !baseHasMutate) || (flagHasMutate && !baseHasMutate)) {
            options->memory()->store(state, resultOffset, 4, FilesystemError::readOnly);
            options->memory()->buffer()[resultOffset] = resultError;
        }

        std::string guestPath;

        CanonOptions::UtfData utfData;
        options->validateString(state, pathStart, pathSize, &utfData);
        if (options->encoding() == ComponentCanonOptions::Utf8) {
            guestPath.assign(reinterpret_cast<const char*>(utfData.buffer()), utfData.length());
        } else {
            std::vector<uint8_t> utf8String(utfData.utf8Length());
            utfData.toUtf8String(utf8String.data());
            guestPath.assign(reinterpret_cast<const char*>(utf8String.data()), utf8String.size());
        }

        std::string resolvedPath;

        uvwasi_errno_t resolveError = resolvePathByComponents(dir, guestPath, pathFlags, resolvedPath);

        if (resolveError != UVWASI_ESUCCESS) {
            options->memory()->store(state, resultOffset, 4, FilesystemError::badDescriptor);
            options->memory()->buffer()[resultOffset] = resultError;
            break;
        }

        if (openFlags == OpenFlags::openDirectory) {
            ComponentResource* resource = new ComponentResourceWasiDirectory(instance->type()->getType(0)->asTypeResource(), guestPath, resolvedPath, descFlags);
            uint32_t resultResource = options->instance()->appendHandle(state, resource);
            options->memory()->store(state, resultOffset, 4, resultResource);
            options->memory()->buffer()[resultOffset] = resultOk;
            break;
        }

        int flags = 0;
        if ((openFlags & OpenFlags::openCreate) == OpenFlags::openCreate) {
            flags |= UV_FS_O_CREAT;
        }
        if ((openFlags & OpenFlags::openDirectory) == OpenFlags::openDirectory) {
            flags |= UV_FS_O_DIRECTORY;
        }
        if ((openFlags & OpenFlags::openExclusive) == OpenFlags::openExclusive) {
            flags |= UV_FS_O_EXCL;
        }
        if ((openFlags & OpenFlags::openTruncate) == OpenFlags::openTruncate) {
            flags |= UV_FS_O_TRUNC;
        }

        if ((descFlags & DescriptorFlags::flagRead) == DescriptorFlags::flagRead
            && (descFlags & DescriptorFlags::flagWrite) == DescriptorFlags::flagWrite) {
            flags |= UV_FS_O_RDWR;
        } else if ((descFlags & DescriptorFlags::flagRead) == DescriptorFlags::flagRead) {
            flags |= UV_FS_O_RDONLY;
        } else if ((descFlags & DescriptorFlags::flagWrite) == DescriptorFlags::flagWrite) {
            flags |= UV_FS_O_WRONLY;
        }

        if ((descFlags & DescriptorFlags::flagDataIntegritySync) == DescriptorFlags::flagDataIntegritySync) {
            flags |= UV_FS_O_DSYNC;
        }
        if ((descFlags & DescriptorFlags::flagFileIntegritySync) == DescriptorFlags::flagFileIntegritySync
            || (descFlags & DescriptorFlags::flagRequestedWriteSync) == DescriptorFlags::flagRequestedWriteSync) {
            flags |= UV_FS_O_SYNC;
        }

        uv_fs_t req;
        int descriptor = uv_fs_open(nullptr, &req, resolvedPath.c_str(), flags, 0666, nullptr);
        uv_fs_req_cleanup(&req);

        if (descriptor < 0) {
            options->memory()->store(state, resultOffset, 4, FilesystemError::badDescriptor);
            options->memory()->buffer()[resultOffset] = resultError;
            break;
        }

        WasiRefCountedFile* fileRef = new WasiRefCountedFile(descriptor, resolvedPath, descFlags);
        ComponentResource* resource = new ComponentResourceWasiFile(instance->type()->getType(0)->asTypeResource(), fileRef);
        uint32_t resultResource = options->instance()->appendHandle(state, resource);
        options->memory()->store(state, resultOffset, 4, resultResource);
        options->memory()->buffer()[resultOffset] = resultOk;
        break;
    }
    case LiftedWasiFunction::fileSystemDescriptorMetadataHash02: {
        uint32_t descriptorIndex = argv[0].asI32();
        uint32_t offset = argv[1].asI32();

        ASSERT(!options->memory()->is64());

        std::string path = "";
        ComponentHandle* handle = options->instance()->getHandle(state, descriptorIndex);
        if (handle->kind() == ComponentHandle::ResourceWasiFileKind) {
            path = asFile(handle)->path();
        } else if (handle->kind() == ComponentHandle::ResourceWasiDirectoryKind) {
            path = asDirectory(handle)->realPath();
        } else {
            ComponentInstance::throwInvalidHandle(state, descriptorIndex);
        }

        uv_fs_t req;
        int r = uv_fs_stat(nullptr, &req, path.c_str(), nullptr);
        uv_fs_req_cleanup(&req);

        if (r < 0) {
            options->memory()->store(state, offset, 4, FilesystemError::badDescriptor);
            options->memory()->buffer()[offset] = resultError;
            break;
        }

        std::hash<uint64_t> hash;
        options->memory()->store(state, offset, 4, hash(req.statbuf.st_mtim.tv_sec));
        options->memory()->store(state, offset, 12, hash(req.statbuf.st_size));
        options->memory()->buffer()[offset] = resultOk;
        break;
    }
    case LiftedWasiFunction::fileSystemDescriptorMetadataHashAt02: {
        uint32_t descriptorIndex = argv[0].asI32();
        uint32_t pathFlags = argv[1].asI32();
        uint32_t pathStart = argv[2].asI32();
        uint32_t pathSize = argv[3].asI32();
        uint32_t offset = argv[4].asI32();

        ASSERT(!options->memory()->is64());

        ComponentHandle* handle = options->instance()->getHandle(state, descriptorIndex);
        if (handle->kind() != ComponentHandle::ResourceWasiDirectoryKind) {
            ComponentInstance::throwInvalidHandle(state, descriptorIndex);
        }

        std::string guestPath;
        CanonOptions::UtfData utfData;
        options->validateString(state, pathStart, pathSize, &utfData);
        if (options->encoding() == ComponentCanonOptions::Utf8) {
            guestPath.assign(reinterpret_cast<const char*>(utfData.buffer()), utfData.length());
        } else {
            std::vector<uint8_t> utf8String(utfData.utf8Length());
            utfData.toUtf8String(utf8String.data());
            guestPath.assign(reinterpret_cast<const char*>(utf8String.data()), utf8String.size());
        }

        std::string resolvedPath;
        uvwasi_errno_t resolveError = resolvePathByComponents(asDirectory(handle), guestPath, pathFlags, resolvedPath);
        if (resolveError != UVWASI_ESUCCESS) {
            options->memory()->store(state, offset, 4, FilesystemError::badDescriptor);
            options->memory()->buffer()[offset] = resultError;
            break;
        }

        uv_fs_t req;
        int r = uv_fs_stat(nullptr, &req, resolvedPath.c_str(), nullptr);
        uv_fs_req_cleanup(&req);

        if (r < 0) {
            options->memory()->store(state, offset, 4, 0);
            options->memory()->buffer()[offset] = resultError;
            break;
        }

        std::hash<uint64_t> hash;
        options->memory()->store(state, offset, 4, hash(req.statbuf.st_mtim.tv_sec));
        options->memory()->store(state, offset, 12, hash(req.statbuf.st_size));
        options->memory()->buffer()[offset] = resultOk;
        break;
    }
    case LiftedWasiFunction::fileSystemDescriptorGetType02: {
        uint32_t descriptorIndex = argv[0].asI32();
        uint32_t offset = argv[1].asI32();

        ASSERT(!options->memory()->is64());

        ComponentHandle* handle = options->instance()->getHandle(state, descriptorIndex);
        if (handle->kind() != ComponentHandle::ResourceWasiFileKind && handle->kind() != ComponentHandle::ResourceWasiDirectoryKind) {
            options->memory()->store(state, offset, 4, FilesystemError::badDescriptor);
            options->memory()->buffer()[offset] = resultError;
        }

        uv_fs_t req;
        int r = uv_fs_fstat(nullptr, &req, asFile(handle)->file()->fileDescriptor(), nullptr);
        uv_fs_req_cleanup(&req);
        if (r < 0) {
            options->memory()->store(state, offset, 4, 0);
            options->memory()->buffer()[offset] = resultError;
            break;
        }

        FileType ft = FileType::Unknown;
        switch (req.statbuf.st_mode) {
        case UV_DIRENT_FILE:
            ft = FileType::RegularFile;
            break;
        case UV_DIRENT_SOCKET:
            ft = FileType::Socket;
            break;
        case UV_DIRENT_LINK:
            ft = FileType::SymbolicLink;
            break;
        case UV_DIRENT_BLOCK:
            ft = FileType::BlockDevice;
            break;
        case UV_DIRENT_DIR:
            ft = FileType::Directory;
            break;
        case UV_DIRENT_CHAR:
            ft = FileType::CharacterDevice;
            break;
        case UV_DIRENT_FIFO:
            ft = FileType::Fifo;
            break;
        }

        options->memory()->store(state, offset, 1, ft);
        options->memory()->buffer()[offset] = resultOk;
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

        int32_t flag = DescriptorFlags::flagRead | DescriptorFlags::flagWrite | DescriptorFlags::flagMutateDirectory;
        for (auto& it : preOpens) {
            if (it.first.length() >= Component::MaxStringByteLength || it.second.length() >= Component::MaxStringByteLength) {
                throwNoMemory(state);
            }

            ComponentResource* resource = new ComponentResourceWasiDirectory(instance->type()->getType(0)->asTypeResource(), it.first, it.second, flag);
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
