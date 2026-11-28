/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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

#ifndef __WalrusProfiler__
#define __WalrusProfiler__

#if defined(WALRUS_PROFILER)

#include "runtime/Module.h"

#include <cstdio>
#include <vector>

namespace Walrus {

class Memory;
class ExecutionState;
struct ExecutionContext;

struct ProfileCounter {
    uint32_t m_count;
    uint32_t m_length;
};

struct ProfileFunctionInfo {
    uint32_t m_functionIndex;
    uint32_t m_firstSlot;
    uint32_t m_blockCount;
    ProfileCounter* m_counters;
};

struct ProfileModuleInfo {
    uint32_t m_firstSlot;
    uint32_t m_blockCount;
    ProfileCounter* m_counters;
};

struct ProfileSnapshotEntry {
    uint32_t m_slot;
    uint32_t m_count;
};

struct ProfilePageEntry {
    uint64_t m_page;
    uint32_t m_memory;
    uint32_t m_reads;
    uint32_t m_writes;
};

struct ProfileMemoryLayout {
    uint32_t m_memory;
    uint64_t m_pageCount;
};

struct ProfileSnapshot {
    uint64_t m_instructions;
    size_t m_entryEnd;
    size_t m_pageEntryEnd;
    size_t m_keepDepth;
    size_t m_stackEntryEnd;
};

struct ProfileExitSite {
    uint32_t m_slot;
    uint32_t m_skipped;
};

struct ProfileExit {
    uint64_t m_snapshotId;
    uint32_t m_slot;
    uint32_t m_skipped : 31;
    uint32_t m_isUserException : 1;
};

class Profiler {
public:
    enum Feature : uint32_t {
        MemoryFeature = 1 << 0,
        ExitFeature = 1 << 1,
        StackFeature = 1 << 2,
        AllFeatures = MemoryFeature | ExitFeature | StackFeature,
    };

    static Profiler& instance()
    {
        return s_instance != nullptr ? *s_instance : create();
    }

    ALWAYS_INLINE void countBlock(ExecutionState& state, ProfileCounter* counter)
    {
        counter->m_count++;
        m_budget -= counter->m_length;
        if (UNLIKELY(m_budget <= 0)) {
            snapshot(&state);
        }
    }

    static uint64_t nextCallSequence() { return ++s_callSequence; }

    bool hasFeature(Feature feature) const { return (m_features & feature) != 0; }
    intptr_t* budgetAddress() { return &m_budget; }
    uint32_t pageShift() const { return m_pageShift; }
    uintptr_t* pendingExitAddress() { return &m_pendingExit; }
    static void snapshotFromJIT(ExecutionContext* context);

    uintptr_t addExitSite(const ProfileCounter* counter, uint32_t executed);
    void recordPendingExit(bool isUserException);

    void instrument(ModuleFunction* function, uint32_t functionIndex, Vector<uint8_t, std::allocator<uint8_t>>& byteCode,
                    Vector<ModuleFunction::CatchInfo, std::allocator<ModuleFunction::CatchInfo>>& catchInfo);
    void finishModule();
    void discardModule();
    uint32_t addMemory(Memory* memory);
    void retireMemory(Memory* memory);
    void recordExit(ModuleFunction* function, size_t position, bool isUserException);
    void finish();

private:
    Profiler();

    static Profiler& create();
    NEVER_INLINE void snapshot(ExecutionState* state);
    void commitKeyframe();
    uint32_t slotOf(const ProfileCounter* counter) const;
    void collectPages(Memory* memory);
    size_t collectStack(ExecutionState* state);

    static Profiler* s_instance;
    static uint64_t s_callSequence;

    intptr_t m_budget;
    intptr_t m_interval;
    uint32_t m_keyframeInterval;
    uint32_t m_pageShift;
    uint32_t m_features;
    uint32_t m_memoryCount;
    uint32_t m_blockCount;
    uint64_t m_snapshotCount;
    uint64_t m_keyframeCount;
    uintptr_t m_pendingExit;
    std::vector<ProfileFunctionInfo> m_functions;
    std::vector<ProfileModuleInfo> m_modules;
    std::vector<ModuleFunction*> m_pendingFunctions;
    std::vector<uint32_t> m_pendingLengths;
    std::vector<ProfileSnapshot> m_snapshots;
    std::vector<ProfileSnapshotEntry> m_snapshotEntries;
    std::vector<Memory*> m_memories;
    std::vector<ProfilePageEntry> m_pageEntries;
    std::vector<ProfileMemoryLayout> m_memoryLayouts;
    std::vector<ProfileExit> m_exits;
    std::vector<ProfileExitSite> m_exitSites;
    std::vector<uint64_t> m_stackSequences;
    std::vector<uint64_t> m_currentSequences;
    std::vector<uint32_t> m_currentSlots;
    std::vector<uint32_t> m_stackEntries;
    FILE* m_output;
};

} // namespace Walrus

#endif

#endif // __WalrusProfiler__
