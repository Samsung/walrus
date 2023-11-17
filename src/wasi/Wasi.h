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

#include "Walrus.h"
#include "runtime/Value.h"
#include "runtime/Function.h"
#include "runtime/ObjectType.h"
#include "runtime/SpecTest.h"
#include "runtime/Memory.h"
#include "runtime/Instance.h"
#include <uvwasi.h>

namespace Walrus {

class WASI {
public:
    // type definitions according to preview1
    // https://github.com/WebAssembly/WASI/blob/main/legacy/preview1/docs.md

#define ERRORS(ERR)                                                   \
    ERR(success, "no error")                                          \
    ERR(toobig, "argument list too long")                             \
    ERR(acces, "permission denied")                                   \
    ERR(addrinuse, "addres in use")                                   \
    ERR(addrnotavauil, "addres not abailable")                        \
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

#define TO_ENUM(ERR, MSG) ERR,
    typedef enum wasi_errno : uint16_t {
        ERRORS(TO_ENUM)
    } wasi_errno_t;
#undef TO_ENUM
    // end of type definitions

    WASI();

    ~WASI()
    {
        ::uvwasi_destroy(m_uvwasi);
    }

    struct WasiFunc {
        std::string name;
        SpecTestFunctionTypes::Index functionType;
        WasiFunction::WasiFunctionCallback ptr;
    };

#define FOR_EACH_WASI_FUNC(F)  \
    F(proc_exit, I32R)         \
    F(proc_raise, I32_RI32)    \
    F(random_get, I32I32_RI32) \
    F(fd_write, I32I32I32I32_RI32)

    enum WasiFuncName : size_t {
#define DECLARE_FUNCTION(NAME, FUNCTYPE) NAME##FUNC,
        FOR_EACH_WASI_FUNC(DECLARE_FUNCTION)
#undef DECLARE_FUNCTION
            FuncEnd,
    };

    void fillWasiFuncTable();
    static WasiFunc* find(std::string funcName);
    static bool checkStr(Memory* memory, uint32_t memoryOffset, std::string& str);
    static bool checkMemOffset(Memory* memory, uint32_t memoryOffset, uint32_t length);

    static void proc_exit(ExecutionState& state, Value* argv, Value* result, Instance* instance);
    static void proc_raise(ExecutionState& state, Value* argv, Value* result, Instance* instance);
    static void fd_write(ExecutionState& state, Value* argv, Value* result, Instance* instance);
    static void random_get(ExecutionState& state, Value* argv, Value* result, Instance* instance);

    static WasiFunc m_wasiFunctions[FuncEnd];
    static uvwasi_t* m_uvwasi;
};

} // namespace Walrus
