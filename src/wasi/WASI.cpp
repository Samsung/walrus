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
#include "runtime/Value.h"
#include "runtime/Memory.h"
#include "runtime/Instance.h"

// https://github.com/WebAssembly/WASI/blob/main/legacy/preview1/docs.md

namespace Walrus {

uvwasi_t* WASI::g_uvwasi;
WASI::WasiFuncInfo WASI::g_wasiFunctions[WasiFuncIndex::FuncEnd];
std::vector<std::pair<std::string, uint32_t>> WASI::preopens;

static void* get_memory_pointer(Instance* instance, Value& value, size_t size)
{
    Memory* memory = instance->memory(0);
    uint32_t offset = value.asI32();

    // The uvwasi module always checks nullptr arguments.
    if (memory->sizeInByte() < size || memory->sizeInByte() - size < offset) {
        return nullptr;
    }

    return memory->buffer() + offset;
}

template <class T, int maxSize>
class TemporaryData {
public:
    TemporaryData(size_t size)
    {
        /* Check that more memory is requested than the maximum provided by malloc. */
        if (size > (std::numeric_limits<size_t>::max() / sizeof(T))) {
            m_data = nullptr;
            return;
        }

        size *= sizeof(T);

        if (size <= sizeof(m_stackData)) {
            m_data = m_stackData;
        } else {
            m_data = reinterpret_cast<T*>(malloc(size));
        }
    }

    ~TemporaryData()
    {
        if (m_data != nullptr && m_data != m_stackData) {
            free(m_data);
        }
    }

    T* data()
    {
        return m_data;
    }

private:
    T m_stackData[maxSize];
    T* m_data;
};

void WASI::initialize(uvwasi_t* uvwasi)
{
    ASSERT(!!uvwasi);
    g_uvwasi = uvwasi;

    // fill wasi function table
#define WASI_FUNC_TABLE(NAME, FUNCTYPE)                                                       \
    g_wasiFunctions[WasiFuncIndex::NAME##FUNC].name = #NAME;                                  \
    g_wasiFunctions[WasiFuncIndex::NAME##FUNC].functionType = DefinedFunctionTypes::FUNCTYPE; \
    g_wasiFunctions[WasiFuncIndex::NAME##FUNC].ptr = &WASI::NAME;
    FOR_EACH_WASI_FUNC(WASI_FUNC_TABLE)
#undef WASI_FUNC_TABLE
}

WASI::WasiFuncInfo* WASI::find(const std::string& funcName)
{
    for (unsigned i = 0; i < WasiFuncIndex::FuncEnd; ++i) {
        if (g_wasiFunctions[i].name == funcName) {
            return &g_wasiFunctions[i];
        }
    }
    return nullptr;
}

void WASI::args_get(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uvwasi_size_t argc;
    uvwasi_size_t bufSize;
    uvwasi_args_sizes_get(WASI::g_uvwasi, &argc, &bufSize);

    uint32_t* uvArgv = reinterpret_cast<uint32_t*>(get_memory_pointer(instance, argv[0], argc * sizeof(uint32_t)));
    char* uvArgBuf = reinterpret_cast<char*>(get_memory_pointer(instance, argv[1], bufSize));

    if (uvArgv == nullptr || uvArgBuf == nullptr) {
        result[0] = Value(WasiErrNo::inval);
        return;
    }

    TemporaryData<void*, 8> pointers(argc);

    char** data = reinterpret_cast<char**>(pointers.data());
    uvwasi_errno_t error = uvwasi_args_get(WASI::g_uvwasi, data, uvArgBuf);

    if (error == WasiErrNo::success) {
        char* buffer = reinterpret_cast<char*>(instance->memory(0)->buffer());
        for (uvwasi_size_t i = 0; i < argc; i++) {
            uvArgv[i] = data[i] - buffer;
        }
    }

    result[0] = Value(error);
}

void WASI::args_sizes_get(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uvwasi_size_t* uvArgc = reinterpret_cast<uvwasi_size_t*>(get_memory_pointer(instance, argv[0], sizeof(uint32_t)));
    uvwasi_size_t* uvArgvBufSize = reinterpret_cast<uvwasi_size_t*>(get_memory_pointer(instance, argv[1], sizeof(uint32_t)));

    result[0] = Value(static_cast<int16_t>(uvwasi_args_sizes_get(WASI::g_uvwasi, uvArgc, uvArgvBufSize)));
}

void WASI::proc_exit(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    ASSERT(argv[0].type() == Value::I32);
    uvwasi_proc_exit(WASI::g_uvwasi, argv[0].asI32());
    ASSERT_NOT_REACHED();
}

void WASI::proc_raise(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    ASSERT(argv[0].type() == Value::I32);
    result[0] = Value(uvwasi_proc_raise(WASI::g_uvwasi, argv[0].asI32()));
}

void WASI::clock_res_get(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uvwasi_timestamp_t* out_addr = reinterpret_cast<uvwasi_timestamp_t*>(get_memory_pointer(instance, argv[1], sizeof(uvwasi_timestamp_t)));

    result[0] = Value(uvwasi_clock_res_get(WASI::g_uvwasi, argv[0].asI32(), out_addr));
}

void WASI::clock_time_get(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uvwasi_timestamp_t* out_addr = reinterpret_cast<uvwasi_timestamp_t*>(get_memory_pointer(instance, argv[2], sizeof(uvwasi_timestamp_t)));

    result[0] = Value(uvwasi_clock_time_get(WASI::g_uvwasi, argv[0].asI32(), argv[1].asI64(), out_addr));
}

void WASI::fd_write(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t fd = argv[0].asI32();
    size_t iovsLen = static_cast<size_t>(argv[2].asI32());
    uint32_t* nwritten = reinterpret_cast<uint32_t*>(get_memory_pointer(instance, argv[3], sizeof(uint32_t)));
    uint32_t* iovptr = reinterpret_cast<uint32_t*>(get_memory_pointer(instance, argv[1], iovsLen * (sizeof(uint32_t) << 1)));

    if (iovptr == nullptr || nwritten == nullptr) {
        result[0] = Value(WasiErrNo::inval);
        return;
    }

    TemporaryData<uvwasi_ciovec_t, 8> iovsBuffer(iovsLen);
    uvwasi_ciovec_t* iovs = iovsBuffer.data();
    uint64_t sizeInByte = instance->memory(0)->sizeInByte();
    uint8_t* buffer = instance->memory(0)->buffer();

    for (uint32_t i = 0; i < iovsLen; i++) {
        if (iovptr[1] > sizeInByte || iovptr[0] > sizeInByte - iovptr[1]) {
            result[0] = Value(WasiErrNo::inval);
            return;
        }

        iovs[i].buf = buffer + iovptr[0];
        iovs[i].buf_len = iovptr[1];
        iovptr += 2;
    }

    result[0] = Value(uvwasi_fd_write(WASI::g_uvwasi, fd, iovs, iovsLen, nwritten));
}

void WASI::fd_tell(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t fd = argv[0].asI32();
    uvwasi_filesize_t* offset = reinterpret_cast<uvwasi_filesize_t*>(get_memory_pointer(instance, argv[1], sizeof(uvwasi_filesize_t)));

    result[0] = Value(uvwasi_fd_tell(WASI::g_uvwasi, fd, offset));
}

void WASI::fd_read(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t fd = argv[0].asI32();
    size_t iovsLen = static_cast<size_t>(argv[2].asI32());
    uint32_t* nread = reinterpret_cast<uint32_t*>(get_memory_pointer(instance, argv[3], sizeof(uint32_t)));
    uint32_t* iovptr = reinterpret_cast<uint32_t*>(get_memory_pointer(instance, argv[1], iovsLen * (sizeof(uint32_t) << 1)));

    if (iovptr == nullptr || nread == nullptr) {
        result[0] = Value(WasiErrNo::inval);
        return;
    }

    TemporaryData<uvwasi_iovec_t, 8> iovsBuffer(iovsLen);
    uvwasi_iovec_t* iovs = iovsBuffer.data();
    uint64_t sizeInByte = instance->memory(0)->sizeInByte();
    uint8_t* buffer = instance->memory(0)->buffer();

    for (uint32_t i = 0; i < iovsLen; i++) {
        if (iovptr[1] > sizeInByte || iovptr[0] > sizeInByte - iovptr[1]) {
            result[0] = Value(WasiErrNo::inval);
            return;
        }

        iovs[i].buf = buffer + iovptr[0];
        iovs[i].buf_len = iovptr[1];
        iovptr += 2;
    }

    result[0] = Value(uvwasi_fd_read(WASI::g_uvwasi, fd, iovs, iovsLen, nread));
}

void WASI::fd_pread(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t fd = argv[0].asI32();
    uint64_t offset = argv[3].asI64();
    size_t iovsLen = static_cast<size_t>(argv[2].asI32());
    uint32_t* iovptr = reinterpret_cast<uint32_t*>(get_memory_pointer(instance, argv[1], iovsLen * (sizeof(uint32_t) << 1)));
    uint32_t* nread = reinterpret_cast<uint32_t*>(get_memory_pointer(instance, argv[4], sizeof(uint32_t)));

    if (iovptr == nullptr || nread == nullptr) {
        result[0] = Value(WasiErrNo::inval);
        return;
    }

    TemporaryData<uvwasi_iovec_t, 8> iovsBuffer(iovsLen);
    uvwasi_iovec_t* iovs = iovsBuffer.data();
    uint64_t sizeInByte = instance->memory(0)->sizeInByte();
    uint8_t* buffer = instance->memory(0)->buffer();

    for (uint32_t i = 0; i < iovsLen; i++) {
        if (iovptr[1] > sizeInByte || iovptr[0] > sizeInByte - iovptr[1]) {
            result[0] = Value(WasiErrNo::inval);
            return;
        }

        iovs[i].buf = buffer + iovptr[0];
        iovs[i].buf_len = iovptr[1];
        iovptr += 2;
    }

    result[0] = Value(uvwasi_fd_pread(WASI::g_uvwasi, fd, iovs, iovsLen, offset, nread));
}

void WASI::fd_readdir(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t fd = argv[0].asI32();
    uint32_t* buf = reinterpret_cast<uint32_t*>(get_memory_pointer(instance, argv[1], argv[2].asI32()));
    uint32_t bufLen = argv[2].asI32();
    uint64_t cookie = argv[3].asI64();
    uint32_t* bufUsed = reinterpret_cast<uint32_t*>(get_memory_pointer(instance, argv[4], sizeof(uint32_t)));

    result[0] = Value(uvwasi_fd_readdir(WASI::g_uvwasi, fd, buf, bufLen, cookie, bufUsed));
}

void WASI::fd_close(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t fd = argv[0].asI32();

    result[0] = Value(uvwasi_fd_close(WASI::g_uvwasi, fd));
}

void WASI::fd_fdstat_get(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t fd = argv[0].asI32();
    uvwasi_fdstat_t* fdstat = reinterpret_cast<uvwasi_fdstat_t*>(get_memory_pointer(instance, argv[1], sizeof(uvwasi_fdstat_t)));

    result[0] = Value(uvwasi_fd_fdstat_get(WASI::g_uvwasi, fd, fdstat));
}

void WASI::fd_fdstat_set_flags(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t fd = argv[0].asI32();
    uint32_t fdflags = argv[1].asI32();

    result[0] = Value(uvwasi_fd_fdstat_set_flags(WASI::g_uvwasi, fd, fdflags));
}

void WASI::fd_prestat_get(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t fd = argv[0].asI32();
    uvwasi_prestat_t* buf = reinterpret_cast<uvwasi_prestat_t*>(get_memory_pointer(instance, argv[1], sizeof(uvwasi_prestat_t)));

    result[0] = Value(uvwasi_fd_prestat_get(WASI::g_uvwasi, fd, buf));
}

void WASI::fd_prestat_dir_name(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t fd = argv[0].asI32();
    uint32_t length = argv[2].asI32();
    char* path = reinterpret_cast<char*>(get_memory_pointer(instance, argv[1], length));

    result[0] = Value(uvwasi_fd_prestat_dir_name(WASI::g_uvwasi, fd, path, length));
}

void WASI::fd_seek(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t fd = argv[0].asI32();
    int64_t fileDelta = argv[1].asI64();
    uint32_t whence = argv[2].asI32();
    uvwasi_filesize_t* file_size = reinterpret_cast<uvwasi_filesize_t*>(get_memory_pointer(instance, argv[3], sizeof(uvwasi_filesize_t)));

    result[0] = Value(uvwasi_fd_seek(WASI::g_uvwasi, fd, fileDelta, whence, file_size));
}

void WASI::fd_filestat_get(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t fd = argv[0].asI32();
    uvwasi_filestat_t* buf = reinterpret_cast<uvwasi_filestat_t*>(get_memory_pointer(instance, argv[1], sizeof(uvwasi_filestat_t)));

    result[0] = Value(uvwasi_fd_filestat_get(WASI::g_uvwasi, fd, buf));
}

void WASI::fd_advise(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t fd = argv[0].asI32();
    uint64_t offset = argv[1].asI64();
    uint64_t len = argv[2].asI64();
    uint32_t advise = argv[3].asI32();

    result[0] = Value(uvwasi_fd_advise(WASI::g_uvwasi, fd, offset, len, advise));
}

void WASI::path_open(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t fd = argv[0].asI32();
    uint32_t dirflags = argv[1].asI32();
    uint32_t length = argv[3].asI32();
    uint32_t oflags = argv[4].asI32();
    uint64_t rights = argv[5].asI64();
    uint64_t right_inheriting = argv[6].asI64();
    uint32_t fdflags = argv[7].asI32();
    const char* path = reinterpret_cast<char*>(get_memory_pointer(instance, argv[2], length));
    uvwasi_fd_t* ret_fd = reinterpret_cast<uvwasi_fd_t*>(get_memory_pointer(instance, argv[8], sizeof(uvwasi_fd_t)));

    result[0] = Value(uvwasi_path_open(WASI::g_uvwasi, fd, dirflags, path, length,
                                       oflags, rights, right_inheriting, fdflags, ret_fd));
}

void WASI::path_readlink(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t fd = argv[0].asI32();
    uint32_t path_len = argv[2].asI32();
    uint32_t buf_len = argv[4].asI32();
    uvwasi_size_t* bufused = reinterpret_cast<uvwasi_size_t*>(get_memory_pointer(instance, argv[5], sizeof(uvwasi_size_t)));
    const char* path = reinterpret_cast<char*>(get_memory_pointer(instance, argv[1], path_len));
    char* buf = reinterpret_cast<char*>(get_memory_pointer(instance, argv[3], buf_len));

    result[0] = Value(uvwasi_path_readlink(g_uvwasi, fd, path, path_len, buf, buf_len, bufused));
}

void WASI::path_create_directory(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t fd = argv[0].asI32();
    uint32_t length = argv[2].asI32();
    const char* path = reinterpret_cast<char*>(get_memory_pointer(instance, argv[1], length));

    result[0] = Value(uvwasi_path_create_directory(WASI::g_uvwasi, fd, path, length));
}

void WASI::path_remove_directory(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t fd = argv[0].asI32();
    uint32_t length = argv[2].asI32();
    const char* path = reinterpret_cast<char*>(get_memory_pointer(instance, argv[1], length));

    result[0] = Value(uvwasi_path_remove_directory(WASI::g_uvwasi, fd, path, length));
}

void WASI::path_filestat_get(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t fd = argv[0].asI32();
    uint32_t flags = argv[1].asI32();
    uint32_t length = argv[3].asI32();
    const char* path = reinterpret_cast<char*>(get_memory_pointer(instance, argv[2], length));
    uvwasi_filestat_t* buf = reinterpret_cast<uvwasi_filestat_t*>(get_memory_pointer(instance, argv[4], sizeof(uvwasi_filestat_t)));

    result[0] = Value(uvwasi_path_filestat_get(WASI::g_uvwasi, fd, flags, path, length, buf));
}

void WASI::path_filestat_set_times(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t fd = argv[0].asI32();
    uint32_t flags = argv[1].asI32();
    uint32_t length = argv[3].asI32();
    const char* path = reinterpret_cast<char*>(get_memory_pointer(instance, argv[2], length));
    uint64_t st_atim = argv[4].asI64();
    uint64_t st_mtim = argv[5].asI64();
    uint32_t fst_flags = argv[6].asI32();

    result[0] = Value(uvwasi_path_filestat_set_times(WASI::g_uvwasi, fd, flags, path, length, st_atim, st_mtim, fst_flags));
}

void WASI::path_rename(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t oldFd = argv[0].asI32();
    uint32_t oldLength = argv[2].asI32();
    const char* oldPath = reinterpret_cast<char*>(get_memory_pointer(instance, argv[1], oldLength));
    uint32_t newFd = argv[3].asI32();
    uint32_t newLength = argv[5].asI32();
    const char* newPath = reinterpret_cast<char*>(get_memory_pointer(instance, argv[4], oldLength));

    result[0] = Value(uvwasi_path_rename(WASI::g_uvwasi, oldFd, oldPath, oldLength, newFd, newPath, newLength));
}

void WASI::path_unlink_file(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t fd = argv[0].asI32();
    uint32_t length = argv[2].asI32();
    const char* path = reinterpret_cast<char*>(get_memory_pointer(instance, argv[1], length));

    result[0] = Value(uvwasi_path_unlink_file(WASI::g_uvwasi, fd, path, length));
}

void WASI::poll_oneoff(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uvwasi_subscription_t* in = reinterpret_cast<uvwasi_subscription_t*>(get_memory_pointer(instance, argv[0], sizeof(uvwasi_subscription_t)));
    uvwasi_event_t* out = reinterpret_cast<uvwasi_event_t*>(get_memory_pointer(instance, argv[1], sizeof(uvwasi_event_t)));
    uint32_t nsubscriptions = argv[2].asI32();
    uint32_t* nevents = reinterpret_cast<uint32_t*>(get_memory_pointer(instance, argv[3], sizeof(uint32_t)));

    result[0] = Value(uvwasi_poll_oneoff(WASI::g_uvwasi, in, out, nsubscriptions, nevents));
}

void WASI::environ_sizes_get(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uvwasi_size_t* uvCount = reinterpret_cast<uvwasi_size_t*>(get_memory_pointer(instance, argv[0], sizeof(uint32_t)));
    uvwasi_size_t* uvBufSize = reinterpret_cast<uvwasi_size_t*>(get_memory_pointer(instance, argv[1], sizeof(uint32_t)));

    result[0] = Value(uvwasi_environ_sizes_get(WASI::g_uvwasi, uvCount, uvBufSize));
}

void WASI::environ_get(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uvwasi_size_t count;
    uvwasi_size_t size;
    uvwasi_environ_sizes_get(WASI::g_uvwasi, &count, &size);

    uint32_t* uvEnviron = reinterpret_cast<uint32_t*>(get_memory_pointer(instance, argv[0], count * sizeof(uint32_t)));
    char* uvEnvironBuf = reinterpret_cast<char*>(get_memory_pointer(instance, argv[1], size));

    if (uvEnviron == nullptr || uvEnvironBuf == nullptr) {
        result[0] = Value(WasiErrNo::inval);
        return;
    }

    TemporaryData<void*, 8> pointers(count);

    char** data = reinterpret_cast<char**>(pointers.data());
    uvwasi_errno_t error = uvwasi_environ_get(WASI::g_uvwasi, data, uvEnvironBuf);

    if (error == WasiErrNo::success) {
        char* buffer = reinterpret_cast<char*>(instance->memory(0)->buffer());
        for (uvwasi_size_t i = 0; i < count; i++) {
            uvEnviron[i] = data[i] - buffer;
        }
    }

    result[0] = Value(error);
}

void WASI::random_get(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    uint32_t length = argv[1].asI32();
    void* buf = get_memory_pointer(instance, argv[0], length);

    result[0] = Value(uvwasi_random_get(WASI::g_uvwasi, buf, length));
}

void WASI::sched_yield(ExecutionState& state, Value* argv, Value* result, Instance* instance)
{
    result[0] = Value(uvwasi_sched_yield(WASI::g_uvwasi));
}

} // namespace Walrus

#endif
