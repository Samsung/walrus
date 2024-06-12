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
#include "jit/SljitLir.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <thread>

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
#if !defined(NDEBUG)
    , m_line(0)
#endif /* !NDEBUG */
{
    const char* path = getenv("WALRUS_PERF_DIR");

    if (path == nullptr || *path == '\0') {
        return;
    }

#if !defined(NDEBUG)
    m_sourceFileName = std::string(path) + "/jit-" + std::to_string(m_pid) + "-codedump.txt";
    this->m_sourceFile = fopen(m_sourceFileName.c_str(), "w");
#endif /* !NDEBUG */

    std::string tmpName = std::string(path) + "/jit-" + std::to_string(m_pid) + ".dump";
    this->m_file = fopen(tmpName.c_str(), "w");
    this->dumpFileHeader();

    // Perf keeps track only executable mappings. This mapping allows
    // the inject operation to find the location of the jitdump file later.
    int fd = open(tmpName.c_str(), O_RDONLY | O_CLOEXEC, 0);
    mmap(NULL, 1, PROT_READ | PROT_EXEC, MAP_SHARED, fd, 0);
    close(fd);
}

static uint64_t getMonotonicTime()
{
    struct timespec time;

    clock_gettime(CLOCK_MONOTONIC, &time);
    return static_cast<uint64_t>(time.tv_sec) * 1000000000 + static_cast<uint64_t>(time.tv_nsec);
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
        .timestamp = getMonotonicTime(),
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
        .timestamp = getMonotonicTime()
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
    } codeLoad = {
        .pid = m_pid,
        .tid = (uint32_t)std::hash<std::thread::id>{}(std::this_thread::get_id()),
        .vma = vma,
        .codeAddr = codeAddr,
        .codeSize = codeSize,
        .codeLoadIndex = m_codeLoadIndex++
    };

    const size_t nameSize = functionName.size() + 1;
    uint32_t size = sizeof(codeLoad) + nameSize + codeSize;
    uint32_t alignedSize = (size + 7) & ~(size_t)0x7;

    dumpRecordHeader(JIT_CODE_LOAD, alignedSize);
    fwrite(&codeLoad, sizeof(codeLoad), 1, m_file);
    fwrite(functionName.c_str(), nameSize, 1, m_file);

    ASSERT(size <= alignedSize && size + 8 > alignedSize);

    if (size < alignedSize) {
        uint8_t data[8] = { 0 };

        fwrite(data, 1, alignedSize - size, m_file);
    }

    fwrite(nativeCode, codeSize, 1, m_file);
}

#if !defined(NDEBUG)

struct DebugEntry {
    uint64_t codeAddress;
    uint32_t line;
    uint32_t discrim;
};

size_t PerfDump::dumpDebugInfo(std::vector<JITCompiler::DebugEntry>& debugEntries, size_t debugEntryStart, const uint64_t codeAddr)
{
    ASSERT(debugEntryStart < debugEntries.size());

    size_t end = debugEntryStart;

    do {
        end++;
    } while (debugEntries[end].line != 0);

    size_t numberOfEntries = end - debugEntryStart;

    struct {
        const uint64_t codeAddress;
        const uint64_t nrEntry;
    } debugInfo = {
        .codeAddress = codeAddr,
        .nrEntry = end - debugEntryStart
    };

    const size_t nameSize = m_sourceFileName.size() + 1;
    uint32_t size = sizeof(debugInfo) + (numberOfEntries * (sizeof(DebugEntry) + nameSize));
    uint32_t alignedSize = (size + 7) & ~(size_t)0x7;

    dumpRecordHeader(JIT_DEBUG_INFO, alignedSize);
    fwrite(&debugInfo, sizeof(debugInfo), 1, m_file);

    DebugEntry debugEntry;
    debugEntry.discrim = 0;

    for (size_t i = debugEntryStart; i < end; i++) {
        debugEntry.codeAddress = debugEntries[i].u.address;
        debugEntry.line = debugEntries[i].line;
        fwrite(&debugEntry, sizeof(debugEntry), 1, m_file);
        fwrite(m_sourceFileName.data(), 1, nameSize, m_file);
    }

    ASSERT(size <= alignedSize && size + 8 > alignedSize);

    if (size < alignedSize) {
        uint8_t data[8] = { 0 };

        fwrite(data, 1, alignedSize - size, m_file);
    }

    return end + 1;
}

#endif /* !NDEBUG */

void PerfDump::dumpCodeClose()
{
    dumpRecordHeader(JIT_CODE_CLOSE, 0);
}

#if !defined(NDEBUG)

uint32_t PerfDump::dumpProlog(Module* module, ModuleFunction* function)
{
    if (m_line > 0) {
        fprintf(m_sourceFile, "\n");
        m_line++;
    }

    size_t size = module->numberOfFunctions();
    int functionIndex = 0;
    ExportType* exportType = nullptr;

    for (auto exp : module->exports()) {
        if (module->function(exp->itemIndex()) == function) {
            exportType = exp;
            break;
        }
    }

    for (size_t i = 0; i < size; i++) {
        if (module->function(i) == function) {
            functionIndex = static_cast<int>(i);
            break;
        }
    }

    if (exportType == nullptr) {
        fprintf(m_sourceFile, "--- FUNCTION:%d ---\nProlog\n", functionIndex);
    } else {
        fprintf(m_sourceFile, "--- FUNCTION:%d [%s] ---\nProlog\n", functionIndex, exportType->name().data());
    }

    m_line += 2;
    return m_line;
}

uint32_t PerfDump::dumpByteCode(InstructionListItem* item)
{
    const char** byteCodeNames = JITCompiler::byteCodeNames();

    if (!item->isInstruction()) {
        fprintf(m_sourceFile, "[%d] Label\n", static_cast<int>(item->id()));
        return ++m_line;
    }

    Instruction* instr = item->asInstruction();

    uint32_t paramCount = instr->paramCount();
    uint32_t size = paramCount + instr->resultCount();
    Operand* operand = instr->operands();
    VariableRef ref;
    bool newline = false;

    fprintf(m_sourceFile, "[%d] %s", static_cast<int>(instr->id()), byteCodeNames[instr->opcode()]);

    for (uint32_t i = 0; i < size; ++i) {
        char prefix = !newline ? ':' : ',';
        newline = true;

        if (i < paramCount) {
            fprintf(m_sourceFile, "%c P%d", prefix, static_cast<int>(i));
        } else {
            fprintf(m_sourceFile, "%c R%d", prefix, static_cast<int>(i - paramCount));
        }

        switch (VARIABLE_TYPE(*operand)) {
        case Instruction::ConstPtr:
            fprintf(m_sourceFile, "(imm)");
            break;
        case Instruction::Register:
            ref = VARIABLE_GET_REF(*operand);

            if (SLJIT_IS_REG_PAIR(ref)) {
                fprintf(m_sourceFile, "(r%d,r%d)", static_cast<int>((ref & 0xff) - 1), static_cast<int>((ref >> 8) - 1));
            } else {
                fprintf(m_sourceFile, "(r%d)", static_cast<int>(ref - 1));
            }
            break;
        default:
            ASSERT(VARIABLE_TYPE(*operand) == Instruction::Offset);
            fprintf(m_sourceFile, "(o:%d)", static_cast<int>(VARIABLE_GET_OFFSET(*operand)));
            break;
        }

        operand++;
    }

    for (uint32_t i = 0; i < 4; ++i) {
        uint8_t reg = instr->requiredReg(i);

        if (reg == 0) {
            continue;
        }

        fprintf(m_sourceFile, "%c T%d(r%d)", (!newline ? ':' : ','), static_cast<int>(i), static_cast<int>(reg - 1));
        newline = true;
    }

    fprintf(m_sourceFile, "\n");
    return ++m_line;
}

uint32_t PerfDump::dumpEpilog()
{
    fprintf(m_sourceFile, "Epilog\n");
    return ++m_line;
}

#endif /* !NDEBUG */

PerfDump::~PerfDump()
{
    if (this->m_file != nullptr) {
        this->dumpCodeClose();
        fclose(this->m_file);
#if !defined(NDEBUG)
        fclose(this->m_sourceFile);
#endif /* !NDEBUG */
    }
}

} // namespace Walrus
#endif
