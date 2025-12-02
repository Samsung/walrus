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

#ifndef __WalrusWASI__
#define __WalrusWASI__

#ifdef ENABLE_WASI

#include "Walrus.h"
#include "runtime/Function.h"
#include "runtime/ObjectType.h"
#include "runtime/DefinedFunctionTypes.h"
#include <uvwasi.h>

namespace Walrus {

class Value;
class Instance;

class WASI {
public:
    // type definitions according to preview1
    // https://github.com/WebAssembly/WASI/blob/main/legacy/preview1/docs.md

#define FOR_EACH_WASI_FUNC(F)                              \
    F(args_get, I32I32_RI32)                               \
    F(args_sizes_get, I32I32_RI32)                         \
    F(proc_exit, I32R)                                     \
    F(proc_raise, I32_RI32)                                \
    F(clock_res_get, I32I32_RI32)                          \
    F(clock_time_get, I32I64I32_RI32)                      \
    F(random_get, I32I32_RI32)                             \
    F(fd_write, I32I32I32I32_RI32)                         \
    F(fd_tell, I32I32_RI32)                                \
    F(fd_read, I32I32I32I32_RI32)                          \
    F(fd_pread, I32I32I32I64I32_RI32)                      \
    F(fd_readdir, I32I32I32I64I32_RI32)                    \
    F(fd_close, I32_RI32)                                  \
    F(fd_fdstat_get, I32I32_RI32)                          \
    F(fd_fdstat_set_flags, I32I32_RI32)                    \
    F(fd_prestat_get, I32I32_RI32)                         \
    F(fd_prestat_dir_name, I32I32I32_RI32)                 \
    F(fd_filestat_get, I32I32_RI32)                        \
    F(fd_seek, I32I64I32I32_RI32)                          \
    F(fd_advise, I32I64I64I32_RI32)                        \
    F(path_open, I32I32I32I32I32I64I64I32I32_RI32)         \
    F(path_readlink, I32I32I32I32I32I32_RI32)              \
    F(path_create_directory, I32I32I32_RI32)               \
    F(path_remove_directory, I32I32I32_RI32)               \
    F(path_filestat_get, I32I32I32I32I32_RI32)             \
    F(path_filestat_set_times, I32I32I32I32I64I64I32_RI32) \
    F(path_rename, I32I32I32I32I32I32_RI32)                \
    F(path_unlink_file, I32I32I32_RI32)                    \
    F(poll_oneoff, I32I32I32I32_RI32)                      \
    F(environ_get, I32I32_RI32)                            \
    F(environ_sizes_get, I32I32_RI32)                      \
    F(sched_yield, RI32)

#define ERRORS(ERR)                                                   \
    ERR(success, "no error")                                          \
    ERR(toobig, "argument list too long")                             \
    ERR(acces, "permission denied")                                   \
    ERR(addrinuse, "addres in use")                                   \
    ERR(addrnotavauil, "addres not available")                        \
    ERR(afnosupport, "address family not supported")                  \
    ERR(again, "resource unavailable, or operation would block")      \
    ERR(already, "connection already in progress")                    \
    ERR(badf, "bad file descriptor")                                  \
    ERR(badmsg, "bad message")                                        \
    ERR(busy, "device or resource busy")                              \
    ERR(canceled, "operation canceled")                               \
    ERR(child, "no child processes")                                  \
    ERR(connaborted, "connection aborted")                            \
    ERR(connrefused, "connection refused")                            \
    ERR(connresset, "connection reset")                               \
    ERR(deadlk, "resource deadlock would occur")                      \
    ERR(destaddrreq, "destination address required")                  \
    ERR(dom, "mathematics argument out of domain of function")        \
    ERR(dquot, "reserved")                                            \
    ERR(exist, "file exists")                                         \
    ERR(fault, "bad address")                                         \
    ERR(fbig, "file too large")                                       \
    ERR(hostunreach, "host unreachable")                              \
    ERR(idrm, "identifier removed")                                   \
    ERR(ilseq, "illegal byte sequence")                               \
    ERR(inprogress, "operation in progress")                          \
    ERR(intr, "interrupted function")                                 \
    ERR(inval, "invalid function argument")                           \
    ERR(io, "io error")                                               \
    ERR(isconn, "socket is connected")                                \
    ERR(isdir, "is a directory")                                      \
    ERR(loop, "too many levels of symbolic links")                    \
    ERR(mfile, "file descriptor value too large")                     \
    ERR(mlink, "too many links")                                      \
    ERR(msgsize, "message too large")                                 \
    ERR(multihop, "reserved")                                         \
    ERR(nametoolong, "filename too long")                             \
    ERR(netdown, "network is down")                                   \
    ERR(netreset, "connection aborted by network")                    \
    ERR(netunreach, "network unreachable")                            \
    ERR(nfile, "too many files open in system")                       \
    ERR(nobufs, "no buffer space available")                          \
    ERR(nodev, "no such device")                                      \
    ERR(noent, "no such file or directory")                           \
    ERR(noexec, "executable file format error")                       \
    ERR(nolck, "no locks available")                                  \
    ERR(nolink, "reserved")                                           \
    ERR(nomem, "not enough space")                                    \
    ERR(nomsg, "no message of the desired type")                      \
    ERR(noprotoopt, "protocol not available")                         \
    ERR(nospc, "no space left on device")                             \
    ERR(nosys, "function not supported")                              \
    ERR(notconn, "the socket is not connected")                       \
    ERR(notdir, "not a directory or a symbolic link to a directory")  \
    ERR(notempty, "directory not empty")                              \
    ERR(notrecoverable, "state not recoverable")                      \
    ERR(notsock, "not a socket")                                      \
    ERR(notsup, "not supported or operation not supported in socket") \
    ERR(notty, "inappropriate I/O control operation")                 \
    ERR(nxio, "no such device or address")                            \
    ERR(overflow, "value too large to be stored in data type")        \
    ERR(ownerdead, "previous owner died")                             \
    ERR(perm, "operation not permitted")                              \
    ERR(pipe, "broken pipe")                                          \
    ERR(proto, "protocol error")                                      \
    ERR(protonosupport, "protocol not supported")                     \
    ERR(prototype, "protocol wrong type for socket")                  \
    ERR(range, "result too large")                                    \
    ERR(rofst, "read-only file system")                               \
    ERR(spipe, "invalid seek")                                        \
    ERR(srch, "no such process")                                      \
    ERR(stale, "reserved")                                            \
    ERR(timedout, "connection timed out")                             \
    ERR(txtbsy, "text file busy")                                     \
    ERR(xdec, "cross-device link")                                    \
    ERR(notcapable, "capabilities insufficient")

    enum WasiErrNo : uint16_t {
#define TO_ENUM(ERR, MSG) ERR,
        ERRORS(TO_ENUM)
#undef TO_ENUM
    };

    struct WasiFuncInfo {
        std::string name;
        DefinedFunctionTypes::Index functionType;
        WasiFunction::WasiFunctionCallback ptr;
    };

    enum WasiFuncIndex : size_t {
#define DECLARE_FUNCTION(NAME, FUNCTYPE) NAME##FUNC,
        FOR_EACH_WASI_FUNC(DECLARE_FUNCTION)
#undef DECLARE_FUNCTION
            FuncEnd,
    };

    static void initialize(uvwasi_t* uvwasi);
    static WasiFuncInfo* find(const std::string& funcName);
    static uvwasi_t* getUvwasi()
    {
        return g_uvwasi;
    }
    static std::vector<std::pair<std::string, uint32_t>>& getPreopen()
    {
        return preopens;
    }

    static void setPreopen(std::vector<std::pair<std::string, uint32_t>> pre)
    {
        WASI::preopens = pre;
    }


private:
    // wasi functions
#define DECLARE_FUNCTION(NAME, FUNCTYPE) static void NAME(ExecutionState& state, Value* argv, Value* result, Instance* instance);
    FOR_EACH_WASI_FUNC(DECLARE_FUNCTION)
#undef DECLARE_FUNCTION

    static uvwasi_t* g_uvwasi;
    static WasiFuncInfo g_wasiFunctions[FuncEnd];
    static std::vector<std::pair<std::string, uint32_t>> preopens;
};

} // namespace Walrus

#endif

#endif // __WalrusWASI__
