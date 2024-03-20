/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#if defined WALRUS_JITPERF
#include "jit/PerfDump.h"
#include <fcntl.h>
#include <sys/mman.h>

#ifndef DUMP_MAGIC
#define DUMP_MAGIC 0x4A695444
#endif

#ifndef DUMP_VERSION
#define DUMP_VERSION 1
#endif

#ifndef ELFMACH
#if defined(CPU_X86_64)
#define ELFMACH 62

#elif defined(CPU_X86)
#define ELFMACH 3

#elif defined(CPU_ARM32)
#define ELFMACH 40

#elif defined(CPU_ARM64)
#define ELFMACH 183

#else
#error "Could't find cpu arch."
#endif
#endif

namespace Walrus {
/* Record types */
enum RecordType : uint32_t {
    JIT_CODE_LOAD = 0, // record describing a jitted function
    JIT_CODE_MOVE = 1, // record describing an already jitted function which is moved (optional)
    JIT_DEBUG_INFO = 2, // record describing the debug information for a jitted function (optional)
    JIT_CODE_CLOSE = 3, // record marking the end of the jit runtime (optional)
    JIT_CODE_UNWINDING_INFO = 4 // record describing a function unwinding information
};

PerfDump& PerfDump::instance()
{
    static PerfDump instance;
    return instance;
}

PerfDump::PerfDump()
    : m_pid((uint32_t)getpid())
    , m_codeLoadIndex(0)
{
    std::string tmpName = "./jit-" + std::to_string(m_pid) + ".dump";
    this->m_file = fopen(tmpName.c_str(), "w");
    this->dumpFileHeader();

    int fd = open(tmpName.c_str(), O_RDONLY | O_CLOEXEC, 0);
    mmap(NULL, 1, PROT_READ | PROT_EXEC, MAP_SHARED, fd, 0);
    close(fd);
}

void PerfDump::dumpFileHeader()
{
    struct {
        const uint32_t magic;
        const uint32_t version;
        const uint32_t totalSize;
        const uint32_t elfMach;
        const uint32_t pad;
        const uint32_t pid;
        const uint64_t timestamp;
        const uint64_t flags;
    } fileHeader = {
        .magic = DUMP_MAGIC,
        .version = DUMP_VERSION,
        .totalSize = sizeof(uint32_t) * 6 + sizeof(uint64_t) * 2,
        .elfMach = ELFMACH,
        .pad = 0,
        .pid = m_pid,
        .timestamp = (uint64_t)std::time(0),
        .flags = 0
    };

    fwrite(&fileHeader, sizeof(fileHeader), 1, m_file);
}

void PerfDump::dumpRecordHeader(const uint32_t recordType, const uint32_t entrySize)
{
    struct {
        const uint32_t recordType;
        const uint32_t size;
        const uint64_t timestamp;
    } recordHeader = {
        .recordType = recordType,
        .size = entrySize + ((uint32_t)sizeof(uint32_t)) * 2 + (uint32_t)sizeof(uint64_t),
        .timestamp = (uint64_t)std::time(0)
    };

    fwrite(&recordHeader, sizeof(recordHeader), 1, m_file);
}

void PerfDump::dumpCodeLoad(const uint64_t vma, const uint64_t codeAddr, const uint64_t codeSize, const std::string& functionName, const uint8_t* nativeCode)
{
    struct {
        const uint32_t pid;
        const uint32_t tid;
        const uint64_t vma;
        const uint64_t codeAddr;
        const uint64_t codeSize;
        const uint64_t codeLoadIndex;
    } codeLoad1 = {
        .pid = m_pid,
        .tid = (uint32_t)std::hash<std::thread::id>{}(std::this_thread::get_id()),
        .vma = vma,
        .codeAddr = codeAddr,
        .codeSize = codeSize,
        .codeLoadIndex = m_codeLoadIndex++
    };
    const size_t nameSize = functionName.size() + 1;
    dumpRecordHeader(JIT_CODE_LOAD, sizeof(codeLoad1) + nameSize + codeSize);
    fwrite(&codeLoad1, sizeof(codeLoad1), 1, m_file);
    fwrite(functionName.c_str(), nameSize, 1, m_file);
    fwrite(nativeCode, codeSize, 1, m_file);
}

void PerfDump::dumpCodeClose()
{
    dumpRecordHeader(JIT_CODE_CLOSE, 0);
}

PerfDump::~PerfDump()
{
    if (this->m_file != nullptr) {
        this->dumpCodeClose();
        fclose(this->m_file);
    }
}
} // namespace Walrus
#endif
