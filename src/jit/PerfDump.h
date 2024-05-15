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

#if defined WALRUS_JITPERF
#ifndef __WalrusJITPERF__
#define __WalrusJITPERF_

#include "jit/Compiler.h"

namespace Walrus {

class PerfDump {
public:
    static PerfDump& instance();

    PerfDump();
    bool perfEnabled() { return m_file != nullptr; }
    void dumpRecordHeader(const uint32_t recordType, const uint32_t entrySize);
    void dumpCodeLoad(const uint64_t vma, const uint64_t codeAddr, const uint64_t codeSize, const std::string& functionName, const uint8_t* nativeCode);
#if !defined(NDEBUG)
    size_t dumpDebugInfo(std::vector<JITCompiler::DebugEntry>& debugEntries, size_t debugEntryStart, const uint64_t codeAddr);
#endif /* !NDEBUG */
    void dumpCodeClose();
    ~PerfDump();

#if !defined(NDEBUG)
    uint32_t dumpProlog(Module* module, ModuleFunction* function);
    uint32_t dumpByteCode(InstructionListItem* item);
    uint32_t dumpEpilog();
#endif /* !NDEBUG */

private:
    void dumpFileHeader();

    FILE* m_file = nullptr;
    uint32_t m_pid;
    uint64_t m_codeLoadIndex;

#if !defined(NDEBUG)
    std::string m_sourceFileName;
    FILE* m_sourceFile = nullptr;
    uint32_t m_line;
#endif /* !NDEBUG */
};

} // namespace Walrus
#endif
#endif
